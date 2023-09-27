# This folder contains the ESP Firmware
This project was compiled on **Linux Mint 21** with the **Arduino IDE 2.1.0**, <br>
it uses a ESP8266 board "**LOLIN(WEMOS) D1  mini (clone)**" and
the following Arduino libraries:

- **ESP8266WiFi**, Version 1.0
- **ESP8266WebServer**, Version 1.0
- **U8g2**, Version 2.34.22

## ESP-ValveControl.ino
This is the Arduino project file and must reside in a folder with the same name (ESP-ValveControl).
As usual, it contains the **setup()** and the main **loop()** blocs.

#### setup
- Initialize Serial (UART0) to 38400 Bd. <br>
  Serial uses UART0 which is mapped to pins GPIO1 (TX) and GPIO3 (RX).  <br>
  For communication with the PIC-µC, we remap UART0 to GPIO15 (TX) and GPIO13 (RX) by Serial.swap(). 
  Calling swap again maps UART0 back to GPIO1 and GPIO3. (Serial1/UART1 can not be used to receive).
- Initialize OLED Status Display <br>
  Show local IP address and initial valve positions.
- Initialize WiFi <br>
  When connected, show local IP address on OLED, so we can connect to the webserver UI.
- Read the PIC Firmware Version
- Setup Webserver <br>
  This implements the client request handlers. You can append the GET commands and parameters to the URI, e.g. <br>
  http://192.168.2.108/move?vz=1&set_pos=25&max_mA=30 <br>
  http://192.168.2.108/status?

#### loop
- Process request handler and execute commands from UI
- Periodically read status from PIC (valve controller)
- Update OLED status display

## OLED.ino

![grafik](https://github.com/deklaus/OpenValveControl/assets/134941062/381b864e-4c95-4f8c-b542-b32fa9c08f5e)

#### OLED_init
- Initializes the Display.

#### OLED_show
- Updates OLED **row** with text message **char \*s**.

#### OLED_update_status
- Updates the OLED status display:
  - graphical position (horizontal bars)
  - numeric position (if reference is set)
  - actual drive current 

## webUI.ino
This file creates the HTML Page of the webserver. Actually it is defined as String, but in the future it shall 
be placed into LittleFS.

How does it work?



