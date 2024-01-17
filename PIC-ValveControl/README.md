# This folder contains the PIC Firmware
This project uses a **PIC18F16Q41** microcontroller. <br> 
The firmware was compiled with the **MPLAB X IDE v5.45** on **Linux Mint 21**. <br>

Compiler:
- **XC8 (v2.32) FREE Version**
- C-Standard: **C99**
- Optimization Level: 2
- Device Family Pack **PIC18F-Q_DFP (v1.14.237)**

# Software
See the doxygen documentation in sub folder ./ValveControl.X/doxygen/.
The following chapters provide some insights into the software concept:

### main
- Initializes the system (see #init_system)
- processes the main loop
  - Measure VDD [0.01 V] (optional battery check)
  - Check RX buffer, run cmd_interpreter if flag is set
  - process state machine:
    - move (pending MOVE command), finish at target position, abort on over_current
	- home (pending HOME command), finish on over_current, abort on timeout (2 minutes)
	- default (idle: check STATUSflags and switch state to move, home or bootload)
	  - .move? set state to move
	  - .home? set state to home
	  - .bootload? generate RESET

#### cmd interpreter
Process commands and queries (?) from ESP:
  - Move:    state = move
  - Home:    state = home
  - Status?  send status data (position[1..4], motor current, STATUSflags
  - Version? send PIC version
  - SetPos?  send positions[1..4]
  - max:mA?  send max_mAx10[1..4]
  - Bootload!

### init_system
- Configures system, I/Os, Timers etc.
  - HFINTOSC (16 MHz),
  - I²C (master, 400 kHz)
  - UART (38400 Bd, 8 data bit, 1 stop bit) 
  - PWM (125 Hz: 7.2 ms High + 0.8 ms Low = 8 ms)
  - Interrupts (see #interrupt)

### interrupt
Configures the Vectored Interrupt Manager and contains the corresponding 
interrups service routines (ISR).
  - IOC  Interrupt On Change, high priority (see user manual)
    - on any rising  edge on port RA5/4, RC5/4, RC3/6, RC7/RB7: read motor current.
    - on any falling edge on port RA5/4, RC5/4, RC3/6, RC7/RB7: read Back EMF.
  - TMR0 (1 ms system clock)
  - U1RX (UART1 RX data from ESP)

### adc
Used to read the INA219 Back EMF voltage, 
exec time ~ 125 µs @400 kHz I2C clock.

### daq
Implements data acquisition functions
- Temperature estimation (of PIC µC)
- Voltage VDD of PIC µC
- Read back EMF voltage of channel vz

### i2c
Read/write functions for the INA219 I2C Current Monitor. 
Exec time to read the motor current: ~ 125 µs @400 kHz I2C clock.