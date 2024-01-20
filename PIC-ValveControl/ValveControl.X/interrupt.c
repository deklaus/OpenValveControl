/** @file   interrupt.c
 *  @brief  Implementations of vectored interrupts (PIC18F..)
 *  @par  (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */
/*  ChangeLog:
 * 2024-01-xx v0.7
 * Randomly we missed some ESP commands due to overflow, due to the delays
 * in the IOC ISR (as expected). Now trying a solution without delays in ISR:
 * - IOC (reading motor current and BEMF) replaced by TMR2 and TMR4 INTs.
 * - TMR2 was used to generate PWM using CCP -> substituded by PWM module.
 * - TMR2 now generates INT some delay after PWM rises H -> ISR reads current.
 * - TMR4 now generates INT some delay after PWM goes L -> ISR reads BEMF.
 * 2023-10-17 V0.1
 * - Initial issue
 */

// *** includes
#include <xc.h>
#include "main.h"
#include "daq.h"
#include "init.h"
#include "i2c.h"

// *** data type, constant and macro definitions

#define IVT1_BASE_ADDRESS   0x0808      /* required with bootloader */
//#define IVT1_BASE_ADDRESS   0x0008
#define KLPASS      1       
#define KLPASS_I    1       
/*  Low Pass Filter
    y(k) = a * u(k) + b * y(k-1);       b = 1 - a;
    y(k) = a * u(k) + y(k-1) - a * y(k-1);
A very fast implementation of 'a * u(k)' and '- b * y(k-1)' simply shifts 
the values u(k) and y(k-1) by KLPASS bit to the right. This results in:
KLPASS  1       2       3       4       5
a:      1/2     1/4     1/8     1/16    1/32    = (1 >> KLPASS)
Ts/T:	1.44	3.48	7.49	15.5	31.5    Time Constant / Sample Interval
*/

// *** global variables
// *** private variables

uint16_t emk[128];

// *** private function prototypes
// *** public function bodies

/** @brief Vectored Interrupt Manager. \n
 The interrupt sources are placed to high and low priorities:
\verbatim
 High Priority Interrupt |  Low Priority Interrupt \n
 Vector |     Source     |  Vector  |    Source   \n
 -------+----------------+----------+---------------------------------
 0x27   | PWM1 Parameter |   0x07   |  IOC  (interrupt on change)
        |                |   0x1B   |  TMR2 (hogged for PWM)
        |                |   0x1C   |  TMR1 (wake-up from sleep)
        |                |   0x1E   |  CCP1 (Capture/Compare)
        |                |   0x1F   |  TMR0 (1 ms system clock)
        |                |   0x20   |  U1RX (UART1 RX)
\endverbatim
 */ 


void interrupt_initialize (void)
{
    INTCON0bits.IPEN = 1;   // Enable Interrupt Priority Vectors

    // Set IVTBASE (XC8 default: 0x000008)
    IVTBASEU = 0x00;
    IVTBASEH = 0x08;    // (default: 0x00, 0x08 when using BL)
    IVTBASEL = 0x08;
    
    // Assign peripheral interrupt priorities (default after reset varies!!!)
    IPR0bits.OSFIP = 1;     // OSFI  - high priority (#0x02) default
    IPR4bits.PWM1IP = 1;    // PWM1I - high priority (#0x27)

    /* more than one interrupt with the same user specified priority level 
     * uses the natural order priority scheme, going from high-to-low with 
     * increasing vector numbers, with 0 being the highest.
     */
    IPR0bits.IOCIP  = 0;    // IOCI - low priority (#0x07)
    IPR3bits.TMR2IP = 0;    // TMR2 - low priority (#0x1B)
    IPR3bits.TMR1IP = 0;    // TMR1 - low priority (#0x1C)
    IPR3bits.CCP1IP = 0;    // CCP1 - low priority (#0x1E)
    IPR3bits.TMR0IP = 0;    // TMR0 - low priority (#0x1F)
    IPR4bits.U1TXIP = 0;    // U1RX - low priority (#0x20)
    
} // interrupt_initialize()


/** @brief  The default interrupt service routine.
 *  - Handles all unexpected interrupts.
 *  - Only action: Set error flag UNEXP_INT.
 */
void __interrupt (irq(default), base(IVT1_BASE_ADDRESS), low_priority) 
default_isr (void)
{
    g_ERRORflags.UNEXP_INT = 1;    // signal unexpected interrupt
	
} // default_isr (void)


static uint8_t ns;


/** @brief  Handles the PWM1 parameter interrupts. \n
 *  We use two sources of PWM1 interrupts:
 *  - on the right flank of slice2 (after settling of motor current): read Imot.
 *  - on the right flank of slice1 (H-bridge turned off): read Back EMF.
 */
void __interrupt (irq(IRQ_PWM1), base(IVT1_BASE_ADDRESS), high_priority)
PWM1_isr (void)
{
    int16_t     mAmps;
    uint16_t    uk;
    int8_t      f1a, f1b;
            
    // cause of interrupt: slice 1 parameter 2?
    if (PWM1GIRbits.S1P2IF)     // ca. 2 ms after H-bridge ON
    {
        PWM1GIRbits.S1P2IF = 0; // clear interrupt flag
        
        ina219_reg(1);          // exec time ca. 49 µs (@SCL 400 kHz)
        mAmps  = ina219_read(); // exec time ca. 72 µs (@SCL 400 kHz)
        if (mAmps < 0) mAmps = 0;   // offset may cause negative readings ??
        
        g_mAx10 += (mAmps - g_mAx10) >> KLPASS_I;   // LowPass Filter        
#ifdef TEST_SETTINGS  
        DAC1DATL = (uint8_t) (g_mAx10 >> 2);    // current monitor (16 -> 8 bit)
#endif
    } // slice 1 parameter 2
    
    // cause of interrupt: slice 1 parameter 1?
    if (PWM1GIRbits.S1P1IF)     // immediately after H-bridge OFF
    {
        PWM1GIRbits.S1P1IF = 0; // clear interrupt flag
        
        __delay_us(400);     // allow vbemf to stabilize
        /** @todo  At 4800 Bd one char is received every 260 µs. This delay can 
         * still cause overrun in the UART! We could spend a TMR to add delay.
         * We will try this when researching the VBEMF problem...
         */
        
        // LP Filter, see also microchip AN2749
        uk = daq_vbemf(g_vz);   // Exec time ca. 100 µs
        g_vbemf += ((int16_t)uk - (int16_t)g_vbemf) >> KLPASS;
        // g_vbemf = uk;

        // save new reading at end of shift register
        for (uint8_t i = 0; i < 4; i++) g_bemf8[i] = g_bemf8[i + 1];
        g_bemf8[4] = (uint8_t) (g_vbemf >> 4);
        
        // calculate 1st and 2nd derivation (kind of)
        f1a = g_bemf8[2] - ((g_bemf8[0] + g_bemf8[1]) >> 1);  // avg 1st 2 vals
        f1b = g_bemf8[4] - ((g_bemf8[2] + g_bemf8[3]) >> 1);  // avg last 2 vals
        
        // if f1a < 0 and f1b > 0 it's an extremum,
        // and if f1a < f1b then it is a local minimum
        if (   (f1a < 0) && (f1b > 0) && (f1a < f1b) && g_zerocount > 3)
        {   // increment position count of active axis
            LATCbits.LATC1 = !LATCbits.LATC1;   // toggle
            g_zerocount = 0;
            g_zcd[g_vz] += g_dir;
            if (g_zcd[g_vz] < 0) g_zcd[g_vz] = 0;
        }
        else if (g_zerocount < 127) g_zerocount++;
        
/**     @note Low pass filter could be initialized using the first reading
        to avoid exponential course at the beginning. Alternatively, the 
		low pass can be omitted, then there is also no phase shift. 
		If the evaluation is done by SW, noise has to be suitably ignored.
		Furthermore: two successive sampling moments cannot be a be a zero 
		crossing - must be checked by algorithm.
*/
        if((g_dir > 0) && (ns < sizeof(emk)/2))
        {
            emk[ns++] = g_vbemf;
        }
        if (ns == 25)
        {
            g_vbemf = 0;    // just to trigger
        }

        // TEST: monitor VBEMF (16 -> 8 bit) - requires DACout routed to a pin
        // DAC1DATL = (uint8_t) (g_vbemf >> 4);    
        /* see also: PIC datasheet 40.5.6 Low-Pass Filter Mode, Burst Average Mode */
        
    } // slice 1 parameter 1
    
} // PWM1_isr ()


/** @brief  Handles all IOC events. \n
 *  We currently don't use any IOC interrupt. Just reset the flags.
 */
void __interrupt (irq(IRQ_IOC), base(IVT1_BASE_ADDRESS), low_priority)
IOC_isr (void)
{
    IOCAF = IOCBF = IOCCF = 0;  // clear all IOC Flags    

} // IOC_isr()


/** @brief Timer0 interrupt (1 ms system clock and timeout).
 *  - Increments g_timer_ms (system timer)
 */
void __interrupt (irq(IRQ_TMR0), base(IVT1_BASE_ADDRESS), low_priority) 
TMR0_isr (void)
{
    PIR3bits.TMR0IF = 0;    // on entry: clear interrupt flag
    if (++g_timer_ms == 0x0000) g_tovfl_ms = true;  // > 65.535 ms elapsed
    
} // TMR0_isr()


/** @brief Timer1 interrupt.
 *  - TMR1 is used for wake-up from sleep (not yet implemented).
 */
void __interrupt (irq(IRQ_TMR1), base(IVT1_BASE_ADDRESS), low_priority) 
TMR1_isr (void)
{
    PIR3bits.TMR1IF = 0;    // on entry: clear interrupt flag
    
} // TMR1_isr()


/** @brief Timer2 interrupt.
 * - (not used yet)
 */
void __interrupt (irq(IRQ_TMR2), base(IVT1_BASE_ADDRESS), low_priority) 
TMR2_isr (void)
{
    PIR3bits.TMR2IF = 0;    // on entry: clear interrupt flag
    
} // TMR2_isr()


/** @brief  Handles UART receive events.
 *  - Interrupt when receiving data from ESP via the UART
 *  - Stores chars in g_rx232_buf[]
 *  - Sets flag g_rs232_request = 1 after detection of CR or LF.
 */
void __interrupt (irq(IRQ_U1RX), base(IVT1_BASE_ADDRESS), low_priority)
U1RX_isr (void)
{
    uint8_t  ch;    // received ascii char

    // PIR4bits.U1RXIF = 0;    // U1RXIF cannot be cleared by software

    if (U1ERRIRbits.RXFOIF || U1ERRIRbits.FERIF) 
    {   // overrun or framing error?
        U1ERRIRbits.RXFOIF = U1ERRIRbits.FERIF = 0;
        ch = U1RXB;            
        init_uart1();               // reset serial port
    }
    else 
    {  // valid char received, save to buffer, check if CR
        ch = U1RXB;
        if (ch == '\r' || ch == '\n')  
        {
            ch = 0;
            if (g_rx232_count > 1)  
            {
                g_rs232_request = 1;
                PIE4bits.U1RXIE = 0;    // disable further U1RX interrupts
            }                           // until cmd has been processed
            else    // ignore leading CRLF
            {
                goto _exit;
            }
        } // EOL received
        if (g_rx232_count < sizeof(g_rx232_buf)) 
        {
            g_rx232_buf[g_rx232_count++] = ch;    // save to buffer
        }
        else    // buffer overflow, flush RS232 buf
        {
            for (uint8_t i = 0; i < sizeof(g_rx232_buf); i++) g_rx232_buf[i] = 0;
            g_rx232_count = 0;
        }
    } // valid char    
_exit:
    NOP();

} // U1RX_isr()


// *** private function bodies

/**
 End of File
 */