

#define CS_PIN 19
#define CLK_PIN 18
#define DATA_PIN 5

#define BUFFER_SIZE 32
volatile uint32_t read_buffer[BUFFER_SIZE];
volatile uint8_t buffer_idx = 0;
volatile uint8_t read_digit = 0;

#define SERIAL_DEBUG

uint8_t data_digit_map[2][10] = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
                              {117, 5, 83, 23, 39, 54, 118, 21, 119, 55}};

uint8_t data_digit_map_power[2][10] = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
                              {125, 5, 94, 79, 39, 107, 123, 69, 127, 111}};

void clock_itr() {
  read_buffer[buffer_idx] |= (digitalRead(DATA_PIN) << (16-read_digit));
  read_digit++;
}

void select_itr() {
  if (!digitalRead(CS_PIN)) {
    read_digit = 0;
    read_buffer[buffer_idx] = 0;
    attachInterrupt(CLK_PIN, clock_itr, RISING);
  } else {
    detachInterrupt(CLK_PIN);
    if (++buffer_idx >= BUFFER_SIZE) { buffer_idx = 0; }
  }
}

uint32_t data_buffer[16];

bool process_data_buffer(){
  uint32_t addr = 0;
  uint32_t cmd = 0;
  uint32_t expected_addr = 0;
  bool  data_start = false;
  uint8_t k = 0;
  bool success = false;

  for (int i = 0; i < BUFFER_SIZE; i++) {

    cmd = (read_buffer[i] & 0x0001C000) >> 14;

    if(cmd == 5){
      // Extract packet address
      addr = (read_buffer[i] & 0x00003F00) >> 8;
      // Check for data starting point
      if (addr == 0){data_start = true;}

      if(data_start){
        // Validate the packet
        if(addr != expected_addr){
          success = false;
          break;
        }
        expected_addr += 2;

        if (addr <= 14){
          data_buffer[k] = read_buffer[i] & 0x00000077;
        } else {
          data_buffer[k] = read_buffer[i] & 0x000000FF;
        }
        
        // Serial.print(addr);
        // Serial.print(":");
        // Serial.println(data_buffer[k], DEC);
        k++;

        if(addr == 30){
          success = true;
          break;
        }

      }

    }
  }

  return success;
}

float get_voltage(){
  if(!process_data_buffer()){
    return -1.0; 
  }

  float voltage_f = 0.0;
  uint32_t voltage = 0;

  for(int i=0; i < 4; i++){
    for(int j=0; j<10; j++){
      if(data_buffer[i]==data_digit_map[1][j]){
        voltage += data_digit_map[0][j] * pow(10, 3-i);
      }
    }
  }

  voltage_f =  voltage / 100.0;;

  return voltage_f;
}

float get_current(){
  if(!process_data_buffer()){
    return -1.0; 
  }

  float current_f = 0;
  uint32_t current = 0;

  for(int i=4; i < 8; i++){
    for(int j=0; j<10; j++){
      if(data_buffer[i]==data_digit_map[1][j]){
        current += data_digit_map[0][j] * pow(10, 7-i);
      }
    }
  }

  current_f = current / 100.0;

  return current_f;
}

float  get_power(){
  if(!process_data_buffer()){
    return -1.0; 
  }

  uint32_t power = 0;
  float power_f = 0.0;

  for(int i=8; i < 12; i++){
    for(int j=0; j<10; j++){
      if((data_buffer[i]>>1)==data_digit_map_power[1][j]){
        power += data_digit_map_power[0][j] * pow(10, 11-i);
        
      }
    }
  }

  // Decimal point is active
  if((data_buffer[10] & 0x01)){
    power_f = (float)power / 10.0;
  } else {
    power_f = (float)power;
  }

  return power_f;
}

int32_t get_energy(){
  if(!process_data_buffer()){
    return -1; 
  }

  uint32_t energy = 0;

  for(int i=12; i < 16; i++){
    for(int j=0; j<10; j++){
      if((data_buffer[i]>>1)==data_digit_map_power[1][j]){
        energy += data_digit_map_power[0][j] * pow(10, 15-i);
        
      }
    }
  }

  if((data_buffer[15] & 0x01)){
    energy = energy * 1000;
  }

  return energy;
}

void print_raw(){
  process_data_buffer();

  for(int i=0; i < 16; i++){
    Serial.print(i);
    Serial.print(": ");
    Serial.println(data_buffer[i], BIN);
  }

}



void setup() {
  Serial.begin(115200);
  pinMode(CS_PIN, INPUT);
  pinMode(CLK_PIN, INPUT);
  pinMode(DATA_PIN, INPUT);

  pinMode(2, OUTPUT);

  attachInterrupt(CS_PIN, select_itr, CHANGE);
}

void loop() {
  Serial.print("Voltage: ");
  Serial.print(get_voltage());
  Serial.print(" V  Current: ");
  Serial.print(get_current());
  Serial.print(" A  Power: ");
  Serial.print(get_power());
  Serial.print(" W  Energy: ");
  Serial.print(get_energy());
  Serial.println(" Wh");
  // print_raw();


  delay(1000);
}
