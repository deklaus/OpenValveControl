/** @file    init.c
 *  @brief    Initialization functions for bootloader
 *  @par    (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */
/* Change Log:
 * 2023-11-23 V0.6
 * - Initial issue. 
 */

/* ********************************
 * THESE ARE THE CONFIG BITS OpenValveControl - UNCOMMENT ONLY FOR DEBUGGING!!
 * /
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
//***********************************/

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <pic18f16q41.h>

#include "main.h"
#include "init.h"

// *** global variables
// *** private variables
// *** private function prototypes
static void     read_procID (void);

/** @brief Initializes the bootloader
 */
void 
init_bootloader (void) 
{
    INTCON0bits.GIE = 0;    // Disable interrupts
    
    init_osc();
    init_pmd();
    init_pins();   
    init_uart1();

    // Read processor ID and set bufferRamPtr
    NVMADR = 0x3FFFFE;
    NVMCON1bits.CMD = 0x00;     // Set the word read command
    NVMCON0bits.GO = 1;         // Start word read
    while (NVMCON0bits.GO);     // Wait for read operation to complete
    procID = NVMDAT;
    switch (procID)
    {   // see "PIC18FXXQ41 Family Programming Specification.pdf"
        case 0x7520: bufferRamPtr = (uint16_t *) (9 * 256);
                     break;         // PIC18F14Q41: Bank 9
        case 0x74E0: bufferRamPtr = (uint16_t *) (13 * 256);
                     break;         // PIC18F15Q41: Bank 13
        case 0x7560: 
        default:     bufferRamPtr = (uint16_t *) (21 * 256);
                     break;         // PIC18F16Q41: Bank 21
    } 
    
} // init_bootloader ()


/** @brief Init the system clock: 
 *  - HFINTOSC, 16 MHz
 */
void init_osc (void)
{
    OSCCON1 = 0x60;     // NOSC HFINTOSC, NDIV 1
    OSCCON3 = 0x00;     // CSWHOLD may proceed, SOSCPWR Low power
    OSCEN = 0x50;       // disable all but LFINTOS and HFINTOSC
    OSCFRQ = 0x05;      // HFFRQ 16_MHz
    OSCTUNE = 0x00;     // TUN 0
    ACTCON = 0x00;      // HFINTOSC tuning is controlled by the OSCTUNE register

} // init_osc ()


/** @brief Init the used ports and pins:
 *  - After reset all pins are inputs. Direction of port pins (RXD, TXD) will be 
 *  controlled by the UART.
 */
void 
init_pins (void)
{
    TRISAbits.TRISA2 = 0;
    LATAbits.LATA2 = 0;     // switch on /LED @RA2 to indicate bootloading.
    
} // init_pins ()


/** @brief Init the Peripheral Module Disable: 
 *  - All modules enabled, power safe not yet required.
 */
void 
init_pmd (void)
{
    PMD0 = 0x00;  // FOSC,FVR,HLVD,CRC,SCAN,-,CLKR,IOC
    PMD1 = 0x00;  // CMP1, ZCD, SMT1, TMR4/TMR3/TMR2/-/TMR0
    PMD2 = 0x00;  // CCP1, CWG1, DSM1, NCO1, ACT, DAC1, ADC CMP2
    PMD3 = 0x00;  // UART2/1, SPI2/1, I2C1, PWM3/2/1
    PMD4 = 0x00;  // DMA3/2/1, CLC4/3/2/1, UART3
    PMD5 = 0x00;  // DAC2, DMA4
    
} // init_pmd ()


/** @brief Initializes UART1 for communication with the ESP8266 D1-mini.
 *  - RXD = RC2     RXD/TXD from PIC's point of view
 *  - TxD = RB5
 *  - 38400 Bd, 8 bit, 1 stop
 *  - receive interrupts disabled
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