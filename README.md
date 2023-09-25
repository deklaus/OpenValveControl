# OpenValveControl
Low cost controller for motorized floor heating actuators (like VdMOT and similar)

# Purpose
Our underfloor heating regulates the supply temperature by means of two characteristic curves (slope and offset). 
This works reasonably well in steady state. However, if one additionally heats a room (e.g. visitor room), the 
charateristics are no longer correct. Or you just want to have it half a degree warmer, but our heating only knows 
whole degrees. Up to now, we have turned the valves until it was more or less right again. 
To automate this, I would like to be able to vary one or more heating circuits via WLAN or a SmartHome interface.
Since my control valves are distributed over two floors, the effort for two Homematic FALMOT or corresponding clones 
is too high for me and the documentation in most cases too little detailed.  
Since I am interested in the whole topic, I started to develop my own solution for the task and make it available 
to the community as an OpenSource project.

# Features
- [x] Connect up to 4 actuators
- [x] OLED Display (visualization of valve positions, motor current and status)
- [x] WLAN-Interface (visualization, command execution and status requests using GET, POST, JSON)
- [ ] Filesystem support on ESP (Webserver and setting data)
- [ ] OTA-Support for ESP (Over-the-Air Update)
- [ ] OTA-Support (Over-the-Air Bootloader) for PIC-µC
- [ ] MQTT Support
- [ ] Examples for NodeRed, MQTT, FHEM, Raspberry Pi, ...

# Bill of Materials
- [x] 1 x LOLIN(WEMOS) D1  mini (clone), ca. 5 EUR (e.g. AZ-Delivery)
- [x] 1 x PIC18F16Q41/P  (20-lead PDIP), ca. 2,50 EUR (e.g. Reichelt)
- [x] 2 x MX1508 Brushed DC Motor Driver (each board supports 2 motors), ca. 3 EUR (e.g. Amazon)
- [x] 1 x INA219 Bidirectional Current/Power Monitor With I2C Interface, ca. 4 EUR (e.g. Amazon)
- [x] 1 x DC-DC-Converter TRACO TSR1-2433, ca. 6 EUR (e.g. Reichelt)
- [x] 4 x RJ11 connectors, ca. 3 EUR (e.g. Amazon B00TX440GO)
- [x] PCB or breadboard, ca. 10 EUR
- [x] Small parts (resistors, diodes, connectors, LED, ..), ca. 2 EUR
- [x] 1 x WITTKOWARE KRA-Z109JFP, DIN rail housing, 90x88x65mm, ca. 7 EUR (+ 5 EUR PP, e.g. Amazon)
- [x] 1 x 5 V / 1 A Power Supply, micro-USB, ca. 10 EUR

Total: ca. 45 EUR

# License
- Still thinking about
- No commercial use

