/** @file     main.c
 *  @brief    Main program for Project "ValveControl"
 *  @par  (c) 2023 Klaus Deutschämer \n
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 * 
 *  \b IDE:      <c> MPLAB X IDE v5.45 &rarr; </c> 
 *  \b Packs:    <c> PIC18F-Q_DFP (v1.14.237) </c> \n
 *  \b Device:   <c> Microchip \b PIC18F16Q41 </c> \n
 *  \b Compiler: <c> XC8 (v2.32) FREE Version, C-Standard: C99, 
 *                   Optimization Level: 2 </c> \n
 *  @todo
 *  - After HOME run, sometimes position is not reset to 0.
 *  - Implement timeout on all position and home jobs. Within IDLE, proper 
 *    handling of parallel jobs via UI must be solved (priority and
 *    selected valve must not accidentally be swapped.
 *  - Back EMF doesn't work yet reliably. Has to be analyzed in detail.
 *    Maybe other PWM settings give better readings?
 *  - If position evaluation using back EMF doesn't work reliably, 
 *    relative positioning might be more useful.
 *  - When battery supply is desirable, sleep mode must be used with
 *    wakeup on messages from ESP.
 *  - Implement bootloader and remote fimrware update via command interface
 *    (e.g. using transparent web interface...)
 */

/* Change Log:
 * 30.10.2023 V0.2 
 * - Some motor directions fixed.
 * - doxygen comments updated.
 * 17.10.2023 V0.1
 * - Initial issue
 */

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "adc.h"
#include "daq.h"
#include "i2c.h"
#include "init.h"

// *** data type, constant and macro definitions

/// States of the main loop (state machine)
enum states {
    state_idle = 0, //!< default state (no commands pending)
    state_home,     //!< homing of selected axis (close direction)
    state_move,     //!< move selected axis
};

// *** global variables
const char  *g_version = "v0.2";    ///< Software version


uint16_t    FVRA2X;     ///< DIA: @ADC FVR1 voltage for 2x setting (in mV)
uint16_t    FVRC2X;     ///< DIA: @CMP/DAC FVR2 voltage for 2x setting (in mV)
uint16_t    VDD;        ///< VDD [0.01 V] (battery voltage)
int16_t     temp_indi;  ///< temp. read from temperature indicator (future option)
float       temp_ds;    ///< temperature read from DS18B20 (future option)

static void sleep_config(void);
static void timer1_config(void);

volatile uint8_t    g_rx232_buf[48];    ///< RS232 RX buffer (> sizeof(S1-record)!
volatile uint8_t    g_rx232_count;      ///< counts buffered RX chars
volatile uint8_t    g_tx232_buf[48];    ///< RS232 TX buffer

volatile uint8_t    g_rs232_request;    ///< new RS232 request received from ESP 
volatile uint8_t    g_rs232_response;   ///< response pending (not yet sent)

volatile STATUSflags_t  g_STATUSflags;   ///< status feedback for ESP
volatile ERRORflags_t   g_ERRORflags;    ///< error flags

volatile uint16_t   g_timer_ms;     ///< incremented every 1 ms by TMR0 isr
volatile bool       g_tovfl_ms;     ///< g_timer_ms > 0xFFFF

volatile uint8_t    g_vz;                    ///< selected vz  {(0), 1 - NUM_VZ}
volatile uint8_t    g_setpos[NUM_VZ + 1];    ///< set position {(0), 1 - NUM_VZ}
volatile uint8_t    g_position[NUM_VZ + 1];  ///< position     {0 .. 100%}
volatile int16_t    g_zcd[NUM_VZ + 1];       ///< ticks by zcd {0 .. 1000 .. }
volatile int16_t    g_max_mAx10[NUM_VZ + 1]; ///< max current  {-3276.7 .. 3276.7} mA

volatile int16_t    g_mAx10;        ///< actual motor current / 0.1 mA
volatile uint16_t   g_vbemf;        ///< actual Back EMK (ADC raw)
volatile int8_t     g_dir;          ///< motor direction [-1, 0, +1]

volatile uint8_t    g_bemf8[5];     ///< buffer to find local minimum 
volatile uint8_t    g_zerocount;    ///< counts pwm cycles since last zero cross


// *** static variables
static  uint8_t     main_state = 0;     ///< main: state machine
static  uint16_t    last_tick;          ///< timer value at last PWM start
static  uint8_t     n_overcurr;         ///< counts overcurrent events

// *** private function prototypes
static void     cmd_interpreter (void);
static bool     over_current (uint8_t vz);
static void     set_pwm (uint8_t vz, int8_t dir);


// *** public function bodies

/** @brief This function outputs one char to the EUSART and is also the helper 
 *  function for the printf() function.
 *  When the TXIF bit is not set within approx. 1 ms, the function returns
 *  and data gets lost.
 */
void putch (char data)
{
    for (uint8_t timeout = 20; (0 == PIR4bits.U1TXIF) && (timeout > 0); --timeout)
    {
        __delay_us(50);                   
    }    
    if (PIR4bits.U1TXIF)
    {
        U1TXB = data;   // send data
    }    
} // putch


/** @brief This is the Main program
 */
void main(void)
{
    int16_t     ival16;
    uint16_t    t_start;
    int8_t      diff;
    uint8_t     motor = 0;
       
    interrupt_GlobalHighDisable();    
    
    /** - Initialize the device
     */
    init_system();    

#ifdef TEST_DACOUT_A2
    // TEST: Monitor output via DAC1 -> RA2
    LATAbits.LATA2 = 1;    // TEST (for DAC: A2 = HiZ)
    ODCONAbits.ODCA2 = 1;
    TRISAbits.TRISA2 = 1;
    ANSELAbits.ANSELA2 = 1;    
    DAC1DATL = 0;       // DAC voltage = 0 V
    DAC1CON = 0xA0;     // DACxEN = 1, Vout@RA2, Vref+ = VDD, Vref- = GND
#endif
    
#ifdef TEST_SETREF
    //g_STATUSflags.ref1 = 1;     // set some ref positions
    g_STATUSflags.ref = 0x0f;   // set all  ref positions
#endif
    
    while (1)   // This is the main loop
    {   
        /** Main loop:
         *  - Measure VDD [0.01 V] (optional battery check) */
        VDD = daq_vdd();        

        /** - Check for requests from ESP via RS232
		 */        
        if (g_rs232_request)    // command received from ESP (flag gets set by ISR)?
        {
            cmd_interpreter();  // sets main_state according to command
        }
        
        switch(main_state) 
        {
            /** - MOVE command
			 */
            case state_move:
                
                diff = (int8_t) (g_setpos[g_vz] - g_position[g_vz]);
                if      (diff > 0) g_dir = +1;
                else if (diff < 0) g_dir = -1;
                else  // diff == 0 
                {
                    g_dir = 0;
                    g_STATUSflags.move = 0;     // position reached
                    g_STATUSflags.vz = 0;       // deselect drive

#ifdef TEST_AUTO_RETURN
                    // TEST: auto return to zero position
                    if (0 == g_setpos[g_vz])
                    {
                        main_state = state_idle;
                    }
                    else
                    {
                        set_pwm(g_vz, g_dir);   // stop pwm
                        __delay_ms(100);       // stop motor before reverse dir
                        g_setpos[g_vz] = 0;
                        g_vbemf = 0;
                        g_zerocount = 0;            
                        g_dir = -1;
                        for (uint8_t i = 0; i < 4; i++) g_bemf8[i] = 0;
                    }
#else
                    main_state = state_idle;
#endif                    
                }

                if (over_current(g_vz))     // check for over current
                {                           // overrides g_dir if true
                    main_state = state_idle;
                    g_STATUSflags.move = 0;
                }

                // set PWM outputs according g_vz and moving direction
                set_pwm(g_vz, g_dir);

                // run a defined amount of time per tick
                if ((g_timer_ms - last_tick) > MSperTICK)
                {   // update position: 1 tick per xxx ms
                    g_position[g_vz] += g_dir;
                    last_tick = g_timer_ms;
                }

                break;
                      
            /// - HOME command (close direction)
            case state_home:
                LED = 0;    // test only
                g_dir = 0;
                if ((0 == g_vz) || (g_vz > NUM_VZ)) // cancel homeing
                {                    
                    g_STATUSflags.home = 0;     // done
                    g_STATUSflags.vz = 0;       // deselect drive
                    main_state = state_idle;
                }
                else if (over_current (g_vz))   // end position reached?
                {
                    g_position[g_vz] = 0;           // set home position
                    main_state = state_idle;
                    g_STATUSflags.home = 0;         // done
                    g_STATUSflags.ref |= (uint8_t) (1 << (g_vz - 1));  // ref set  
//                    main_state = state_move;        // move to set_position
//                    g_STATUSflags.move = 1;                      
                }              
                else    // proceed homeing in close direction
                {
                    g_dir = -1;     // set PWM and outputs for close direction
					set_pwm(g_vz, g_dir);
                }
                
                if ((g_timer_ms - last_tick) > MSperTICK)
                {   // update position: 1 tick per xxx ms:                    
                    if (g_position[g_vz] > 0) g_position[g_vz] -= 1;
                    else g_position[g_vz] = 99;    // rollover                 
                    last_tick = g_timer_ms;                   
                }
                
                break;
                
            /** - IDLE (kind of scheduler: check status bits for pending jobs). \n
             *    + For sanity, the IOC INT is disabled and enabled only during 
             *      MOVE and HOME.
             *    + Measure the motor current in idle mode (PWM is off) 
             *      as a control feature. 
             *    + Check the status flags for incoming requests from ESP and
             *      set main_state and parameters as needed.
             */  
            default:
                LED = 1;    // 1 = off
                
                PIE0bits.IOCIE = 0;     // Disable IOCI interrupt 

                RA5PPS = RA4PPS = 0;        
                RC5PPS = RC4PPS = 0;
                RC3PPS = RC6PPS = 0;
                RC7PPS = RB7PPS = 0;              

                /* Read motor current via INA219 Shunt Current Register
                 * (1 LSB = 10μV, Rs = 0,1 Ohm => I / 0.1 mA = Us / 10µV).
                 * Exectime ~171 µs.
                 */
                ina219_reg(1);              // exec time ca. 49 µs (@SCL 400 kHz)
                g_mAx10 = ina219_read();    // exec time ca. 72 µs (@SCL 400 kHz)
                if (g_mAx10 < 0) g_mAx10 = 0;   // offset may cause negative readings

                n_overcurr = 0;  // reset spike counter
                
                if ((g_vz > 0) && (g_vz <= NUM_VZ))
                {
                    IOCAF = IOCBF = IOCCF = 0;  // clear all IOC Flags

                    if      (g_STATUSflags.home)	// home always has prioritiy over move
                    {
                        g_vbemf = 0;    // reset LP filter
                        g_zerocount = 0;                        
                        for (uint8_t i = 0; i < 4; i++) g_bemf8[i] = 0;
                        main_state = state_home; // prio    
                    }
                    else if (g_STATUSflags.move) 
                    {
                        g_vbemf = 0;    // reset LP filter
                        g_zerocount = 0;
                        for (uint8_t i = 0; i < 4; i++) g_bemf8[i] = 0;
                        main_state = state_move;
                    }
                }             
                
                last_tick = g_timer_ms;     // reset time reference
                break;                        
        } // switch
            
    } // while()        
    
} // main()



/** @brief Command Interpreter
 *  Commands and Queries:
 *  - Move: vz, setpos, max_mA
 *  - Home: vz, max_mA
 *  - max_mA?	Current Limits
 *  - SetPos?	Set positions
 *  - Status?	Detailed status
 *  - Version?	Version of PIC Firmware
 */
static 
void cmd_interpreter (void)
{
    char        *p;
    uint8_t     u8;
    unsigned    vz = 0, pos = 0;
    int16_t     mAx10 = 0;
    int8_t      error = 0;
    int16_t     timeout;

/// Commands:
    
    // STATUS
    p = strstr((const char *)g_rx232_buf, "Status?");  // STATUS
    if (p != NULL) 
    {
        sprintf((char *)g_tx232_buf, "Status:%u,%u,%u,%u,%u,0x%04X\n", 
            g_position[1], g_position[2], g_position[3], g_position[4],
            g_mAx10, g_STATUSflags);
        goto _done;
    }

    
    // MOVE
    p = strstr((const char *)g_rx232_buf, "Move:");  // MOVE
    if (p != NULL) 
    {
        if (g_STATUSflags.home)
        {
            error = E_HOMEING_ACTIVE;
            goto _done;
        }
        if (sscanf(p+5, "%u,%u,%d\n", &vz, &pos, &mAx10) == 3)
        {   // vz = 0: deselect all, [1..4] selects drive 1 to 4
            if (0 == vz) { g_STATUSflags.move = 0; goto _done; }
            else if (vz > NUM_VZ) { error = E_VZ_RANGE; goto _done; }
            
            g_vz = (uint8_t) vz;  // [1 .. 4] 

            if (pos <= 100) g_setpos[vz] = (uint8_t) pos;  // [0 .. 100]
            else {  error = E_SET_POS_RANGE; goto _done; }

             // [ .1 .. 200.0]
            if ((mAx10 > 0) && (mAx10 <= 2000))  g_max_mAx10[vz] = mAx10;
            else {  error = E_SET_POS_RANGE; goto _done; }
        }
        sprintf((char *)g_tx232_buf, "Move:%u,%u,%d\n", 
                                      vz, g_setpos[vz], g_max_mAx10[vz]);
        
        // check if reference is set
        if (g_STATUSflags.ref & (1 << (vz - 1)))
        {
            g_STATUSflags.vz = (uint8_t) (1 << (vz - 1));   // active drive
            g_STATUSflags.move = 1;              // make move active
        }
        else error = E_NO_REFERENCE;
        
        goto _done;
    }

    // HOME
    p = strstr((const char *)g_rx232_buf, "Home:");  // HOME
    if (p != NULL) 
    {
        if (sscanf(p+5, "%u,%u\n", &vz, &mAx10) == 2)
        {   // vz = 0: deselect all, [1..4] selects drive 1 to 4
            if (0 == vz) { g_STATUSflags.ref = 0; goto _done; }
            else if (vz > NUM_VZ) { error = E_VZ_RANGE; goto _done; }

            g_vz = (uint8_t) vz;  // [1 .. 4]

             // [ 0.1 .. 100.0]
            if ((mAx10 > 0) && (mAx10 <= 1000))  g_max_mAx10[vz] = mAx10;
            else { error = E_SET_POS_RANGE; goto _done; }
        }
        sprintf((char *)g_tx232_buf, "Home:%d,%d\n", vz, g_max_mAx10[vz]);
        
        g_STATUSflags.vz = (uint8_t) (1 << (vz - 1));   // active drive
        g_STATUSflags.home = 1;    // make home active
        goto _done;
    }
    
    
    // Firmware Version
    p = strstr((const char *)g_rx232_buf, "Version?");
    if (p != NULL) 
    {
        sprintf((char *)g_tx232_buf, "Version: %s\n", g_version);
        goto _done;
    }

    // Set Positions
    p = strstr((const char *)g_rx232_buf, "SetPos?");  // STATUS
    if (p != NULL) 
    {
        sprintf((char *)g_tx232_buf, "SetPos:%u,%u,%u,%u\n", 
            g_setpos[1], g_setpos[2], g_setpos[3], g_setpos[4]);
        goto _done;
    }    

    // Current Limits
    p = strstr((const char *)g_rx232_buf, "max_mA?");  // STATUS
    if (p != NULL) 
    {
        sprintf((char *)g_tx232_buf, "max_mA:%d,%d,%d,%d\n", 
            g_max_mAx10[1], g_max_mAx10[2], g_max_mAx10[3], g_max_mAx10[4]);
        goto _done;
    }    
    
/// Command not found:
    error = E_UNDEF_CMD;
    //goto _done;
    
_done:                
    if (error)
    {
        sprintf((char *)g_tx232_buf, "ERROR %d\n", error);
    }                
    printf((char *)g_tx232_buf);    // send response        
    g_rs232_request = 0;    // ack flag

    // clear uart shift register and RX buffer
    for (uint8_t i = 0; i < sizeof(g_rx232_buf); i++) g_rx232_buf[i] = 0;
    g_rx232_count = 0;
    for (uint8_t c = 0; PIR4bits.U1RXIF; ) c = U1RXB;   // clr RXB (mainly 0x0A)
    PIE4bits.U1RXIE = 1;    // re-enable U1RX interrupts
    
} // cmd_interpreter ()


/** @brief Check for over current (e.g. due to blocked drive)
 *  - Compares actual current g_mAx10 with limit of selected valve zone vz
 *  - On overcurrent, counter n_overcurr gets incremented, else counter is reset
 *  - If counter exceeds 12 (ca. 100 ms), 
 *    - PWM is stopped, g_dir forced to 0
 *    - Error flag OVER_CURR is set
 *  @param  vz: valve zone [0, 1 - 4]
 * @return  0:  no overcurrent, 1: overcurrent detected
 */
static bool over_current (uint8_t vz)
{
    bool result = false;

    if (g_mAx10 > g_max_mAx10[vz])    // if over current
    {
        if (++n_overcurr > 12)        // lasts longer than 100 ms
        {
            set_pwm(vz, 0);           // stop pwm
            g_dir = 0;
            g_ERRORflags.OVER_CURR = 1;
            result = true;
        }
    }
    else n_overcurr = 0;  // reset spike counter

    return (result);
    
} // over_current ()


/** @brief Configures Port Pins and PPS (Peripheral Pin Selects) to output the
 *  PWM signal to the required ports for given valve zone vz and direction dir.
 *  @param  vz:     valve zone [0, 1 - 4]
 *          dir:    direction [-1, 0, +1] (+1 = open, 0 = stop, -1 = close)
 */ 
static void set_pwm (uint8_t vz,    // valve zone [0, 1 - 4]
                     int8_t  dir)   // direction [-1, 0, +1]
{
    // Wait until TMR2 overflows to get a complete duty cycle and period 
    // on the first PWM output
    for (uint8_t timout = 0; (T2TMR > 0) && (timout < 200); timout++)
    {
        __delay_us(50);
    }
    
    switch (vz)   // select vz
    {
        case 4:
            if      (+1 == dir) // open
            {   /* - switch PWM @RC3 = IN7 On
                 * - enable IOC @C3 falling edge: read Current
                 * - enable IOC @C3 rising edge:  read Back EMF */
                RC3PPS = 0x09; 
                RC6PPS = 0; 
                IOCCNbits.IOCCN3 = 1;
                IOCCPbits.IOCCP3 = 1;
                IOCCFbits.IOCCF3 = 0;
                PIE0bits.IOCIE = 1;
            }
            else if (-1 == dir) // close
            {   /* - switch PWM @RC6 = IN8 On
                 * - enable IOC @RC6 falling edge: read Current
                 * - enable IOC @RC6 rising edge:  read Back EMF */
                RC3PPS = 0;     // map RC3 as port pin (LATxy = 0)
                RC6PPS = 0x09;  // map RC6 as pwm output
                IOCCNbits.IOCCN6 = 1;
                IOCCPbits.IOCCP6 = 1;
                IOCCFbits.IOCCF6 = 0;   // reset port change flag
                PIE0bits.IOCIE = 1;     // enable IOCI interrupt
            }
            else    // stop
            { 
                RC3PPS = RC6PPS = 0;    // IN7 = IN8 = 0 (NO RUN))
                PIE0bits.IOCIE = 0;     // Disable IOCI interrupt 
                IOCCNbits.IOCCN3 = 0;
                IOCCNbits.IOCCN6 = 0;
                IOCCPbits.IOCCP3 = 0;
                IOCCPbits.IOCCP6 = 0;
            }
            break;
            
        case 3:
            if      (+1 == dir) // open
            {   /* - switch PWM @RC7 = IN5 On
                 * - enable IOC @RC7 falling edge: read Current
                 * - enable IOC @RC7 rising edge:  read Back EMF */
                RC7PPS = 0x09; 
                RB7PPS = 0; 
                IOCCNbits.IOCCN7 = 1;
                IOCCPbits.IOCCP7 = 1;
                IOCCFbits.IOCCF7 = 0;   // reset port change flag
                PIE0bits.IOCIE = 1;     // Enable IOCI interrupt                 
            }
            else if (-1 == dir) // close
            {   /* - switch PWM @RB7 = IN6 On
                 * - enable IOC @RB7 falling edge: read Current
                 * - enable IOC @RB7 rising edge:  read Back EMF */
                RC7PPS = 0;     // map RC7 as port pin (LATxy = 0)
                RB7PPS = 0x09;  // map RB7 as pwm output
                IOCBNbits.IOCBN7 = 1;
                IOCBPbits.IOCBP7 = 1;
                IOCBFbits.IOCBF7 = 0;   // reset port change flag
                PIE0bits.IOCIE = 1;     // Enable IOCI interrupt                             

            }
            else    // stop
            { 
                RC7PPS = RB7PPS = 0;    // IN5 = IN6 = 0 (NO RUN))
                PIE0bits.IOCIE = 0;     // Disable IOCI interrupt 
                IOCBNbits.IOCBN7 = 0;
                IOCCNbits.IOCCN7 = 0;
                IOCBPbits.IOCBP7 = 0;
                IOCCPbits.IOCCP7 = 0;
            }
            break;
            
        case 2:
            if      (+1 == dir) // open
            {   /* - switch PWM @RC4 = IN3 On
                 * - enable IOC @RC4 falling edge: read Current
                 * - enable IOC @RC4 rising edge:  read Back EMF */
                RC4PPS = 0x09; 
                RC5PPS = 0; 
                IOCCNbits.IOCCN4 = 1;
                IOCCPbits.IOCCP4 = 1;
                IOCCFbits.IOCCF4 = 0;   // reset port change flag
                PIE0bits.IOCIE = 1;     // Enable IOCI interrupt 

            }
            else if (-1 == dir) // close
            {   /* - switch PWM @RC5 = IN4 On
                 * - enable IOC @RC5 falling edge: read Current
                 * - enable IOC @RC5 rising edge:  read Back EMF */
                RC4PPS = 0;     // map RC4 as port pin (LATxy = 0)
                RC5PPS = 0x09;  // map RC5 as pwm output
                IOCCNbits.IOCCN5 = 1;
                IOCCPbits.IOCCP5 = 1;
                IOCCFbits.IOCCF5 = 0;   // reset port change flag
                PIE0bits.IOCIE = 1;     // Enable IOCI interrupt                             
            }
            else    // stop
            { 
                RC4PPS = RC5PPS = 0;    // IN3 = IN4 = 0 (NO RUN))
                PIE0bits.IOCIE = 0;     // Disable IOCI interrupt 
                IOCANbits.IOCAN4 = 0;
                IOCANbits.IOCAN5 = 0;
                IOCAPbits.IOCAP4 = 0;
                IOCAPbits.IOCAP5 = 0;
            }
            break;
            
        case 1:
            if      (+1 == dir) // open
            {   /* - switch PWM @RA4 = IN1 On
                 * - enable IOC @RA4 falling edge: read Current
                 * - enable IOC @RA4 rising edge:  read Back EMF */
                RA4PPS = 0x09; 
                RA5PPS = 0; 
                IOCANbits.IOCAN4 = 1;
                IOCAPbits.IOCAP4 = 1;
                IOCAFbits.IOCAF4 = 0;   // reset port change flag
                PIE0bits.IOCIE = 1;     // Enable IOCI interrupt 
            }
            else if (-1 == dir) // close
            {   /* - switch PWM @RA5 = IN2 On
                 * - enable IOC @RA5 falling edge: read Current
                 * - enable IOC @RA5 rising edge:  read Back EMF */
                RA4PPS = 0;     // map RA4 as port pin (LATxy = 0)
                RA5PPS = 0x09;  // map RA5 as pwm output
                IOCANbits.IOCAN5 = 1;
                IOCAPbits.IOCAP5 = 1;
                IOCAFbits.IOCAF5 = 0;   // reset port change flag
                PIE0bits.IOCIE = 1;     // Enable IOCI interrupt                             
            }
            else    // stop
            { 
                RA4PPS = RA5PPS = 0;    // IN1 = IN2 = 0 (NO RUN))
                PIE0bits.IOCIE = 0;     // Disable IOCI interrupt 
                IOCANbits.IOCAN4 = 0;
                IOCANbits.IOCAN5 = 0;
                IOCAPbits.IOCAP4 = 0;
                IOCAPbits.IOCAP5 = 0;
            }
            break;
            
        case 0:     // off
        default:
            
            break;
    } // switch
    
} // set_pwm()
        
        
/**
 End of File
*/