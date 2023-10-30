/** @file   init.c
 *  @brief  Initialization functions 
 *  @par  (c) 2023 Klaus Deutschämer \n
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */
/* Change Log:
 * 17.10.2023 V0.1
 * - Initial issue
 * @test
 * @bug
 */

// Configuration bits: selected in the code configurator GUI (see mcc.c)
// pragma config statements should precede project file includes.
// CONFIG1
#pragma config FEXTOSC = OFF            // External Oscillator Selection
#pragma config RSTOSC = HFINTOSC_64MHZ  // Reset Oscillator Selection

// CONFIG2
#pragma config CLKOUTEN = OFF   // CLKOUT function is disabled
#pragma config PR1WAY = OFF     // PRLOCKED bit can be set and cleared
#pragma config CSWEN = ON       // Clock Switch: Writing NOSC and NDIV allowed
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor disabled
#pragma config FCMENP = ON      // FSCM will flag FSCMP/OSFIF on EXTOSC failure
#pragma config FCMENS = OFF     // FSCM will not flag FSCMP/OSFIF on SOSC fail

// CONFIG3
#pragma config MCLRE = EXTMCLR  // MCLR: If LVP = 0, MCLR pin is MCLR
#pragma config PWRTS = PWRT_OFF // Power-up timer (PWRT) is disabled
#pragma config MVECEN = ON      // Multi-vector enabled, Vector table is used
#pragma config IVT1WAY = OFF    // IVTLOCKED bit can be cleared and set
#pragma config LPBOREN = ON     // Low-Power BOR enabled
#pragma config BOREN = ON       // Brown-out Reset enabled according to SBOREN

// CONFIG4
#pragma config BORV = VBOR_1P9  // Brown-out Reset Voltage Selection: 1.9 V
#pragma config ZCD = OFF        // ZCD module is disabled (at POR)
#pragma config PPS1WAY = OFF    // PPSLOCK bit can be written as needed
#pragma config STVREN = ON      // Stack full/underflow will cause Reset
#pragma config LVP = OFF        // HV on MCLR/VPP must be used for programming
#pragma config XINST = OFF      // Extended Instr. Set: Not supported by XC8

// CONFIG5
#pragma config WDTCPS = WDTCPS_31 // WDT Period -> software control
#pragma config WDTE = OFF         // WDT -> Disabled, SWDTEN is ignored

// CONFIG6
#pragma config WDTCWS = WDTCWS_7  // WDT Window -> always open (100%)
#pragma config WDTCCS = SC        // WDT input clock -> Software Control

// CONFIG7
#pragma config BBSIZE = BBSIZE_1024	// Boot Block Size -> 1024 words (0..0x07FF)
#pragma config BBEN = OFF          	// Boot Block Enable -> disabled
#pragma config SAFEN = OFF         	// SAF Enable -> SAF disabled
#pragma config DEBUG = OFF         	// Background Debugger disabled

// CONFIG8
#pragma config WRTB = OFF         // Boot Block -> NOT write protected
#pragma config WRTC = OFF         // Config Register -> NOT write protected
#pragma config WRTD = OFF         // Data EEPROM -> NOT write protected
#pragma config WRTSAF = OFF       // Storage Area Flash -> NOT write protected
#pragma config WRTAPP = OFF       // Application Block -> NOT write protected

// CONFIG9
#pragma config CP = OFF           // Code Protection (Flash and EEPROM) -> OFF


#include <xc.h>             /* XC8 General Include File */
#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */

#include "main.h"
#include "i2c.h"
#include "init.h"
#include "interrupt.h"


// *** data type, constant and macro definitions
// *** global variables
// *** private variables
// *** private function prototypes
// *** public function bodies

/** @brief  This is the main initialization function.
 */
void init_system (void)
{
    INTCON0bits.GIE = 0;    // Global Interrupt Disable
    
    init_oscillator();
    init_pmd();
    init_pin_manager();     // define I/Os

    init_timer0();          // TMR0: 1 ms system clock (used for timeouts etc.)    
    i2c_init();             // I2C: 400 kHz, current sensor module INA216 
    init_ina219();          // using defaults only
    
    init_fvr();             // fixed voltage reference (measure VDD)
    init_pwm();             // pulse width modulation (motor driver module)
    init_uart1();

    I2C1CON0bits.EN = 1;
    
/// - Read Device Information Area (DIA)
    NVMADR = 0x2C0032;          // @ADC FVR1 voltage for 2x setting (in mV)
    NVMCON1bits.CMD = 0b000;    // read command (PFM: word)
    NVMCON0bits.GO = 1;         // start word read
    while (NVMCON0bits.GO);     // wait for the read operation to complete
    FVRA2X = NVMDAT;            // 0x0808 = 2056 (Beispiel Eval-Board: +0,39 %)

    NVMADR = 0x2C0038;          // @CMP/DAC FVR2 voltage for 2x setting (in mV)
    NVMCON1bits.CMD = 0b000;    // read command (PFM: word)
    NVMCON0bits.GO = 1;         // start word read
    while (NVMCON0bits.GO);     // wait for the read operation to complete
    FVRC2X = NVMDAT;            // 0x0814 = 2068 (Beispiel Eval-Board: +0,98 %)    
     

/// - Initialize Interrupts
    interrupt_initialize();  // Enable Priority Vectors, set high/low priorities
    
    // PIR4bits.U1RXIF = 0;   // U1RXIF is read only
    PIE4bits.U1RXIE = 1;      // enable RX INT
    
    interrupt_GlobalLowEnable();    // Enables low-priority INTs (if GIE is set)
    interrupt_GlobalHighEnable();   // Enables all high and low priority INTs    
    
} // init_system ()


/** @brief Init the FVR and enable the temperature indicator.
 * 
 * - Enables fixed volatge reference (FVR)
 * - Enabels temperature indicator (TI): High Range (Vout = 3VT)
 * - FVR Buffer 1 (ADC):       2x (2.048 V)
 * - FVR Buffer 2 (CMP & DAC): 2x (2.048 V)
 */
void init_fvr (void)
{
    FVRCONbits.TSRNG = 1;       // Hi range: Vout = 3VT (requires VDD >= 2.5 V)
    FVRCONbits.ADFVR = 0b10;    // ADC FVR Gain is 2x (output = 2.048 V)
    FVRCONbits.CDAFVR = 0b10;   // Comparator FVR Gain is 2x (output = 2.048 V)
    FVRCONbits.FVREN = 1;       // enabled Fixed Voltage Reference
    FVRCONbits.TSEN = 1;        // Temperature Sensor Enable

} // init_fvr ()


/* @brief Setup of the INA219 Module to measure current.
 * The default I2C address of the INA219 is 0x40
 * 
 * Hex Register          POR       Type
 * 00  Configuration     0x399F    R/W
 * 01  Shunt voltage     -         R
 * 02  Bus voltage       -         R
 * 03  Power             0000      R
 * 04  Current           0000      R
 * 05  Calibration       0000     R/W
 */
void init_ina219 (void)
{
    /* We are using the INA219 in default mode. Then nothing has to be 
     * initialized, see datashett 8.5.3:
     * 
     * Simple Current Shunt Monitor Usage (No Programming Necessary)
     * The INA219 can be used without any programming if it is only necessary to 
     * read a shunt voltage drop and bus voltage with the default 12-bit resolution, 
     * 320-mV shunt full-scale range (PGA = /8), 32-V bus full-scale range,
     * and continuous conversion of shunt and bus voltage.
     * Without programming, current is measured by reading the shunt voltage.
     */ 
    // ina219_write(0x05, 10240);  // Calibration register (test)

} // init_ina219 ()


void init_oscillator (void)
{
    // NOSC HFINTOSC; NDIV 1; 
    OSCCON1 = 0x60;
    // CSWHOLD may proceed; SOSCPWR Low power; 
    OSCCON3 = 0x00;
    // MFOEN disabled; LFOEN disabled; ADOEN disabled; PLLEN disabled; SOSCEN disabled; EXTOEN disabled; HFOEN disabled; 
    OSCEN = 0x50;
    // HFFRQ 16_MHz; 
    OSCFRQ = 0x05;
    // TUN 0; 
    OSCTUNE = 0x00;
    // ACTUD enabled; ACTEN disabled; 
    ACTCON = 0x00;
	
} // init_oscillator ()


/* Pins assignments:
 * RA5: IN1
 * RA4: IN2
 * RA3: INP RESET
 * RA2: /LED
 * RA1: I/O ICSPCLK
 * RA0: I/O ICSPDAT
 * 
 * RB7: IN8
 * RB6: OUT SCL
 * RB5: RXD
 * RB4: I/O SDA
 * 
 * RC7: IN7
 * RC6: IN6
 * RC5: IN3
 * RC4: IN4
 * RC3: IN5
 * RC2: TXD
 * RC1: TEST
 * RC0: V_BEMF
 */
void init_pin_manager (void)
{
    // LATx registers
    LATA = 0x04;        // RA2 = /LED
    LATB = 0x00;
    LATC = 0x00;

    // ANSELx registers
    ANSELA = 0x03;      // RA0/1 : VBEMF2/1
    ANSELB = 0x00;
    ANSELC = 0x03;      // RC0/1 : VBEMF4/3

    // WPUx registers
    WPUA = 0x00;
    WPUB = 0x00;
    WPUC = 0x00;

    // ODx registers
    ODCONA = 0x00;
    ODCONB = 0b01010000;    // SDA and SCL must be configured as open-drain
    ODCONC = 0x00;

    // TRISx registers (RXD/TXD from PIC's point of view)
    TRISA = 0b00001011;     // RA4/5 = IN1/IN2, RA2 = /LED, RA0/1 = VBEMF2/1
    TRISB = 0b00000000;     // RB6 = SCL1, RB5 = TXD, RB4 = SDA1
    TRISC = 0b00000011;     // RC2 = RXD, RC0/1 = VBEMF4/3
    
    // SLRCONx registers (0 = max Slew Rate, 1 = limited Slew Rate)
    SLRCONA = 0xFF;         // default after Reset: 1
    SLRCONB = 0xFF;
    SLRCONC = 0xFF;

    // INLVLx registers (0 = TTL input, 1 = ST input)
    INLVLA = 0xFF;         // default after Reset: 1
    INLVLB = 0xFF;
    INLVLC = 0xFF;
    
	// I2C Pins
    I2C1SDAPPS = 014;   // RB4->I2C1:SDA1;  octal
    RB4PPS = 0x22;      // RB4->I2C1:SDA1;    
    I2C1SCLPPS = 016;   // RB6->I2C1:SCL1;  octal
    RB6PPS = 0x21;      // RB6->I2C1:SCL1;    

    LATBbits.LATB4 = 1; // TRIS RB4 = SDA1
    LATBbits.LATB6 = 1; // TRIS RB6 = SCL1       

} // init_pin_manager())


void init_pmd (void)
{
    PMD0 = 0b00111010;  // Disable FOSC, FVR, HLVD, CRC, SCAN, -, CLKR, IOC
    PMD1 = 0b00111000;  // Disable CMP1, ZCD, SMT1, TMR4/TMR3/TMR2/-/TMR0
    PMD2 = 0b01100001;  // Disable CCP1, CWG1, DSM1, NCO1, ACT, DAC1, ADC, CMP2?
    PMD3 = 0b00110110;  // Disable UART2/1, SPI2/1, I2C1, PWM3/2/1
    PMD4 = 0b11111111;  // Disable DMA3/2/2, CLC4/4/2/1, UART3,
    PMD5 = 0b00000111;  // Disable OPA1, DAC2, DMA4    
} // init_pmd ()


/** @brief Init the PWM (125 Hz) with 90% duty cycle (7.2 ms + 0.8 ms = 8 ms).
    see datasheet 28.4.1f.
*/
void init_pwm (void)
{
   
/*  Use Timer2 and configure the default period
 *  PR2 = FREQUENCY_TO_PR_CONVERT(FREQUENCY_MIN);
 *  PWM Period = (T2PR + 1) x Tosc x TMR2_Prescaler  ; Tosc = 4/Fosc = 250 ns
 *             = (T2PR + 1) x 32 µs                  ; CKPS = 128
 *  max.Period = 256 x 32 µs = 8,192 ms = 122 Hz
 *  min.Period =   1 x 32 µs = 32 µs = 31,25 kHz
 */
    T2CONbits.ON = 0;       // TIMERx stop during setup
    T2TMR = 0x00;           // TIMERx counter reset (forced by stop)
    T2CLKCONbits.CS = 1;    // TIMERx clock source is FOSC/4

    // Configure the PWM period
    T2PR = FREQUENCY_TO_PR_CONVERT(125L);      // PWM -> 125 Hz (T2PR = 249)
    T2CONbits.CKPS = 0b111;     // TIMERx prescaler 1:128
    T2CONbits.OUTPS = 0;        // TIMERx postscaler 1:1   
    T2CONbits.ON = 1;           // TIMERx ON    
    
    /* - Configure the CCP module for the PWM mode by loading the CCPxCON 
     *   register with the appropriate values.
     * - Load the CCPRx register with the PWM duty cycle value and configure 
     *   the FMT bit to set the proper register alignment.
     */
    CCP1CONbits.MODE = 0x0C;    // MODE PWM
    CCP1CONbits.FMT = 1;        // FMT left_aligned
    CCP1CONbits.EN = 1;         // EN enabled
    CCPTMRS0bits.C1TSEL = 1;    // Selecting Timer 2

    // Configure the PWM duty cycle (percent))
    CCPR1 = (DUTYCYCLE_TO_CCPR_CONVERT(90, 125)) << 6;
    
} // init_pwm ()


/** @brief Initializes Timer 0 to generate an interrupt every 1 ms.
 *         The TMR0 ISR increments the global variable (uint16_t) g_timer_ms.
 */
void 
init_timer0 (void) 
{
    T0CON0 = 0;     // TMR0 disable
    T0CON1 = 0x54;  // T0CS FOSC/4; T0CKPS 1:16; T0ASYNC not_synchronised
    TMR0H = 249;    // Match every TMR0H+1 clocks -> 1 ms
    TMR0L = 0;
    PIR3bits.TMR0IF = 0;    // clear interrupt flag bit
    PIE3bits.TMR0IE = 1;    // TMR0 Interrupt Enable
    T0CON0bits.EN = 1;      // T0EN enabled; T0OUTPS 1:1; T016BIT 8-bit; 
    
} // init_timer0 ()


/** @brief Initialisiert UART1 für die Kommunikation mit dem ESP8266 D1-mini \n
 *  - RXD = RC2     RXD/TXD from PIC's point of view
 *  - TxD = RB5
 *  - 38400 Bd, 8 bit, 1 stop.
 *  - receive interrupts enabled
 */
void 
init_uart1 (void) 
{
    TRISCbits.TRISC2 = 1;
    ANSELCbits.ANSELC2 = 0;
    
    // configure receiver
    U1CON1bits.ON = 0;      // Serial port disabled (held in Reset)
    
    U1BRG =  25;            // ~38400 Baud  (@16 MHz: U1BRG = 1e6/Baud - 1)
    // Baudrate normal speed, no auto bd, TX enabled, RX enabled, 8 bit ASYNC
    U1CON0 = 0b00110000;
    
    U1CON2 = 0;
    U1CON2bits.RXPOL = 0;   // RX polarity is not inverted, Idle state is high
    U1CON2bits.TXPOL = 0;   // TX polarity is not inverted, Idle state is high
    U1RXPPS = 0b010010;     // UART1 Receive  on pin RC2
    RB5PPS = 0x10;          // UART1 Transmit on Pin RB5
    
    U1CON1bits.ON = 1;      // serial port enabled

} // init_uart1 ()


// *** private function bodies

/**
 End of File
 */