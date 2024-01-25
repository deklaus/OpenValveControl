# This folder contains the PIC Firmware
This project uses a **PIC18F16Q41** microcontroller. <br> 
The firmware was compiled with the **MPLAB X IDE v5.45** on **Linux Mint 21**. <br>

Compiler:
- **XC8 (v2.32) FREE Version**
- C-Standard: **C99**
- Optimization Level: 2
- Device Family Pack **PIC18F-Q_DFP (v1.14.237)**

# Software
See the **doxygen** documentation in sub folder ./ValveControl.X/doxygen/.
The following paragraphs provide some insights into the software structure:

### main.c
- Initializes the system (see #init.c)
- processes the main loop
  - Measure VDD [0.01 V] (optional battery check)
  - Check RX buffer, run cmd_interpreter if flag is set
  - process state machine:
    - move (pending MOVE command), finish at target position, abort on over_current
    - home (pending HOME command), finish on over_current, abort on timeout (2 minutes)
    - default (idle: check STATUSflags and switch state to move, home or bootload)
      - STATUSflags.move? set main_state to state_move
      - STATUSflags.home? set main_state to state_home
      - STATUSflags.bootload? generate RESET

**cmd interpreter**
Process commands and queries (?) from ESP:
  - Move:    save active drive and current limit, set g_STATUSflags.move
  - Home:    save active drive and current limit, set g_STATUSflags.home
  - Status?  send status data (position[1..4], motor current, STATUSflags
  - Version? send PIC version
  - SetPos?  send positions[1..4]
  - max:mA?  send max_mAx10[1..4]
  - Bootload!

### init.c
- Configures system, I/Os, Timers etc.
  - HFINTOSC (16 MHz),
  - I²C (master, 400 kHz)
  - UART (38400 Bd, 8 data bit, 1 stop bit) 
  - PWM1_SaP1_out: (125 Hz: 7.2 ms High + 0.8 ms Low = 8 ms) <br>
    PWM1_SaP2_out: (125 Hz: 2.0 ms High + 6.0 ms Low = 8 ms)
  - Interrupts (see #interrupt.c)

### interrupt.c
Configures the Vectored Interrupt Manager and contains the corresponding 
interrupt service routines (ISR).
  - PWM1 Parameter Interrupt
    - on falling edge of PWM1_SaP2_out (2 ms after H-bridge ON): read motor current
    - on falling edge of PWM1_SaP1_out (~ 100 µs after H-bridge OFF): read Back EMF
  - TMR0 (1 ms system clock)
  - U1RX (UART1 RX data from ESP)

### adc.c
Basic analog to digital converter functions.

### daq.c
Implements data acquisition functions:
- Temperature estimation of PIC µC
- Voltage VDD of PIC µC
- Read back EMF voltage of channel vz, exec time ~ 100 µs

### i2c.c
Read/write functions for the INA219 I2C Current Monitor.<br> 
Exec time to read the motor current: ~ 125 µs @400 kHz I2C clock.
