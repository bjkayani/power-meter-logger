# Power Meter Logger

Modify off the shelf power meters from Peacefair to pull the power data and log it using a microcontroller. 

<img src="https://i.ibb.co/Ssvgg7T/IMG-3449-1.jpg" width="50%">

## Supported Power Meters

- PZEM-031 (V4.0)
- PZEM-051 (V5.0)

## Supported Boards

- ESP32-WROOM

## Guide

1. Buy the above mentioned power meter.
2. Open up the back cover and solder four wires to the connections in the diagram below. (PZEM-051 (V5.0) has convenient testpoints)
3. 3D print the backplate.
4. Solder the wires to a 4 pin female header and push it in the new back plate. 
5. Label the pinout on the back of the cover for later.
6. Hot glue the connections and close up the back cover.
7. Connect to the pins of the microcontroller as specified in the code. 
8. Flash the code using Arduino IDE. 
9. Wire up the power meter as shown in its manual or original back plate.
10. The micrcontroller should be spitting out the data on the serial console. 

<br>

<img src="https://i.ibb.co/ZLCGgG0/Screenshot-2023-06-03-182436.png" width="50%">

<br>

## [Blog Post](https://duckduckgo.com)

## Future Work
- Convert code example to a library
- Add support for more power meters
- Add support for more dev boards/microcontrollers
