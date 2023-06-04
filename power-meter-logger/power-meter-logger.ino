/**
 * Power Meter Logger
 * Author: Badar Jahangir Kayani
 * Date: 6/3/2023
 * Email: badarjahangir@gmail.comap
 * 
 * Read data from the Peacefair power module display driver and decode it to log
 * the voltage, current, power and energy.
 *  
 * Tested on ESP32-WROOM dev board.
 * 3.3V logic and interrupt capability required on CLK_PIN and CS_PIN
 * Display driver: TM1621B
 * Datasheet link: https://datasheet.lcsc.com/lcsc/2203031730_TM-Shenzhen-Titan-Micro-Elec-TM1621B_C2980111.pdf
 */

// Pin definitions
#define CS_PIN 19
#define CLK_PIN 18
#define DATA_PIN 5

// Data read stuff
#define BUFFER_SIZE 32
volatile uint32_t read_buffer[BUFFER_SIZE];
volatile uint8_t buffer_idx = 0;  // Packet read index
volatile uint8_t read_digit = 0;  // Bit read index (within a packet)
uint32_t data_buffer[16]; // Store the entire packet transmission sequence

// Data decoding look up table
uint8_t data_digit_lut_1[2][10] = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
                                  {117, 5, 83, 23, 39, 54, 118, 21, 119, 55}};

uint8_t data_digit_lut_2[2][10] = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
                                  {125, 5, 94, 79, 39, 107, 123, 69, 127, 111}};

/**
 * @brief ISR for the clock signal
 * Read the bit from the data pin and stuff it in the packet byte 
 * at the correct location to form the packet value one bit at a time
 */
void clock_isr() {
  read_buffer[buffer_idx] |= (digitalRead(DATA_PIN) << (16-read_digit));
  read_digit++;
}

/**
 * @brief ISR for the select signal
 * Select signal signifies start or end of packet transmision.
 */
void select_isr() {
  // Enable data reading when CS is pulled low
  if (!digitalRead(CS_PIN)) {
    read_digit = 0;
    read_buffer[buffer_idx] = 0;
    attachInterrupt(CLK_PIN, clock_isr, RISING);
  } else { // disable reading when its pulled high
    detachInterrupt(CLK_PIN);
    if (++buffer_idx >= BUFFER_SIZE) { buffer_idx = 0; }
  }
}

/**
 * @brief Process the read buffer to extract a single tranmission sequence
 * 
 * The display driver has 32 nibbles of memory that are written to control what
 * is displayed on the display. The power meter sends 16 packets of data, each with
 * the following structure:
 * 
 * | 3 bit Packet Type | 6 bit Memory Address | 4 bit Data | 4 bit Data |
 * 
 * Each packet has two data nibbles hence it sends 16 total packets to write to the 
 * entire memory. Each packet is 17 bit long. 
 * 
 * We can use the memory address value to verify that we read the data correctly,
 * then get the value of the data and store it in the read buffer. Since each packet
 * corresponds to one digit, I save the two nibbles as a single number. 
 * 
 * @return true Data processing was successful
 * @return false Data processing failed
 */
bool process_data_buffer(){
  uint32_t addr = 0;
  uint32_t cmd = 0;
  uint32_t expected_addr = 0;
  bool  data_start = false;
  uint8_t data_idx = 0;
  bool success = false;

  // Iterate over the entire read buffer
  for (int i = 0; i < BUFFER_SIZE; i++) {

    // Read and verify that the packet type is correct
    cmd = (read_buffer[i] & 0x0001C000) >> 14;

    if(cmd == 5){
      // Extract packet address
      addr = (read_buffer[i] & 0x00003F00) >> 8;
      // Check for the first address and set boolean to continue reading
      if (addr == 0){data_start = true;}

      if(data_start){
        // Validate each packet address
        if(addr != expected_addr){
          success = false;
          break;
        }

        expected_addr += 2;

        // Store the data value from each packet
        if (addr <= 14){  // First 8 packets 
          data_buffer[data_idx] = read_buffer[i] & 0x00000077;
        } else {  // Last 8 packets
          data_buffer[data_idx] = read_buffer[i] & 0x000000FF;
        }
        
        // Serial.print(addr);
        // Serial.print(":");
        // Serial.println(data_buffer[data_idx], DEC);
        data_idx++;

        // Break when the entire sequence has been read
        if(addr == 30){
          success = true;
          break;
        }
      }
    }
  }
  return success;
}

/**
 * @brief Get the voltage from the data buffer
 * 
 * @return float Voltage in volts
 */
float get_voltage(){
  if(!process_data_buffer()){
    return -1.0; 
  }

  float voltage_f = 0.0;
  uint32_t voltage = 0;

  for(int i=0; i < 4; i++){
    for(int j=0; j<10; j++){
      if(data_buffer[i]==data_digit_lut_1[1][j]){
        // Stuff the voltage digits in the correct unit location
        voltage += data_digit_lut_1[0][j] * pow(10, 3-i);
      }
    }
  }

  // Convert to volts
  voltage_f =  voltage / 100.0;;

  return voltage_f;
}

/**
 * @brief Get the current from the data buffer
 * 
 * @return float Current in amps
 */
float get_current(){
  if(!process_data_buffer()){
    return -1.0; 
  }

  float current_f = 0;
  uint32_t current = 0;

  for(int i=4; i < 8; i++){
    for(int j=0; j<10; j++){
      if(data_buffer[i]==data_digit_lut_1[1][j]){
        // Stuff the current digits in the correct unit location
        current += data_digit_lut_1[0][j] * pow(10, 7-i);
      }
    }
  }

  // Convert to amps
  current_f = current / 100.0;

  return current_f;
}

/**
 * @brief Get the power from the data buffer
 * 
 * @return float Power in Watts
 */
float  get_power(){
  if(!process_data_buffer()){
    return -1.0; 
  }

  uint32_t power = 0;
  float power_f = 0.0;

  for(int i=8; i < 12; i++){
    for(int j=0; j<10; j++){
      if((data_buffer[i]>>1)==data_digit_lut_2[1][j]){
        // Stuff the power digits in the correct unit location
        power += data_digit_lut_2[0][j] * pow(10, 11-i);
        
      }
    }
  }

  // Check if the decimal point is active or not and scale accordingly
  if((data_buffer[10] & 0x01)){
    power_f = (float)power / 10.0;
  } else {
    power_f = (float)power;
  }

  return power_f;
}

/**
 * @brief Get the energy from the data buffer
 * 
 * @return int32_t Energy in Wh
 */
int32_t get_energy(){
  if(!process_data_buffer()){
    return -1; 
  }

  uint32_t energy = 0;

  for(int i=12; i < 16; i++){
    for(int j=0; j<10; j++){
      if((data_buffer[i]>>1)==data_digit_lut_2[1][j]){
        // Stuff the energy digits in the correct unit location
        energy += data_digit_lut_2[0][j] * pow(10, 15-i);
        
      }
    }
  }

  // Check if the kilo digit is active or not and scale accordingly
  if((data_buffer[15] & 0x01)){
    energy = energy * 1000;
  }

  return energy;
}

/**
 * @brief Print the raw data buffer in binary
 * Use for debugging
 */
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

  // Attach interrupt on chip select pin to monitor rising and falling edges
  attachInterrupt(CS_PIN, select_isr, CHANGE);
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

  delay(1000);
}
