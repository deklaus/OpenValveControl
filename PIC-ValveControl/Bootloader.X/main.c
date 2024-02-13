/** @file   main.c
 *  @brief  Bootloader for OpenValveControl
 *  @par    (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 * 
 *  \b IDE:      <c> MPLAB X IDE v5.45 &rarr; </c> 
 *  \b Packs:    <c> PIC18F-Q_DFP (v1.8.154) </c> \n
 *  \b Device:   <c> Microchip \b PIC18F16Q41 </c> \n
 *  \b Compiler: <c> XC8 (v2.32) FREE Version, C-Standard: C99, 
 *                   Optimization Level: 2 </c> \n * 
 *  \b Linker:   <c> XC8 Global Options | XC8 Linker | Options:
 *                   - Runtime | Initialize Data: unchecked
 *                   - Memory Model | Rom Range: 0-7FF
 * 
 * Change Log:
 * 2024-01-29 v0.7.1
* - Error corrected when copying code to bufferRam.
 * 2023-11-23 v0.6
 * - Initial issue
 */

// *** includes
#include <xc.h>       // Standard include
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "init.h"

// *** data type, constant and macro definitions
#define  NEW_RESET_VECTOR            0x0800
#define  NEW_INTERRUPT_VECTOR_HIGH   0x0808
#define  NEW_INTERRUPT_VECTOR_LOW    0x0818

#define PAGESIZE           128      /* no. of WORDs */

#define  _str(x)  #x
#define  str(x)  _str(x)

/******************************************************************************
 * The bootloader code does not use any interrupts.
 * Because the application project uses Multi-vector interrupts, the (legacy) 
 * interrupt vectors at  address 0x0008 (High) and 0x0018 (Low) may be ignored. 
 * Otherwise a jump to the application interrupt vectors would be required.
 * 
 * @note In the OpenValveControl project we must specify the code offset
 * in the project settings:  XC8 Global Options | XC8 Linker | Options |
 * Additional Options | Codeoffset: 0x0800
*****************************************************************************/
    
// *** global variables
record_t    record;
uint16_t    procID;         // processor ID
uint16_t   *bufferRamPtr;

// *** private variables
// *** private function prototypes
static uint8_t  uart1_Read (void);
static uint8_t 	xtou8 (uint8_t *);
static char     u8tox (uint8_t n);

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
} // putch ()


/** @brief this is the bootloader, called by the startup code.
 *  - When EEPROM[0x00] == 0xFF, the bootloader code will be executed, 
 *    else the µC jumps to the @ NEW_RESET_VECTOR (application program).
 * 
 * Memory Map
 *   -----------------
 *   |    0x0000     |   Reset vector
 *   |               |
 *   |    0x0008     |   High Priority Interrupt vector
 *   |               |
 *   |    0x0018     |   Low Priority Interrupt vector
 *   |               |
 *   |  Boot Block   |   (Bootloader program)
 *   |               |
 *   |    0x0800     |   Re-mapped Reset Vector
 *   |    0x0808     |   Re-mapped High Priority Interrupt Vector
 *   |    0x0818     |   Re-mapped Low Priority Interrupt Vector
 *   |       |       |
 *   |               |
 *   |  Code Space   |   Application program space
 *   |               |
 *   |       |       |
 *   |               |
 *   | End of Flash  |  (0xFFFF - PIC18F16Q41, 0x7FFF - PIC18F15Q41 )
 *   -----------------
 *
 */
void main(void)
{
    int8_t     error = 0;
    
    bool        eof;    // End Of File
    bool        eol;    // End Of Line
    uint8_t     ch;     // received ascii char
    uint8_t     index;
    
    uint8_t     cs;
    uint8_t     length;
    uint8_t     type;
    uint16_t    address;
    uint8_t     offset;         // addr offset relative to base_addr
    uint16_t    data;           // data word (2 byte) to get flashed
    uint16_t   *bufPtr;
    
    // SYSTEM_Initialize:
    init_pmd();
    init_osc();
    
    // Read EEPROM @0x00: FF = run bootloader, * = goto application program
    NVMDATL = 0x00;
    NVMADR = EEPROM_BASE + 0x00;
    NVMCON1bits.CMD = 0b000;
    NVMCON0bits.GO = 1;
    for (uint8_t timeout = 0; timeout < 20; ++timeout)
    {
        if (!NVMCON0bits.GO) break;
        else __delay_ms(1);
    }

    if (NVMDATL != 0xFF)
    {   
        STKPTR = 0x00;
        BSR = 0x00;
        asm ("goto  "  str(NEW_RESET_VECTOR));  // goto application program
    }
    
    init_bootloader();

// *****************************************************************************   
    eof = false;
    while (!eof)
    {
    // Read from UART1 until line terminator LF
    // @38400 Bd we have ca. 200 µs per char
        for (uint8_t i = 0; i < sizeof(record.buffer); i++) 
        {   record.buffer[i] = '\0';
        }
        error = 0;
        for (eol = false, index = 0; !eol && (index < sizeof(record.buffer)); )
        {
            while (!PIR4bits.U1RXIF) ;
            
//          if (U1ERRIRbits.RXFOIF || U1ERRIRbits.FERIF)
//          (FERIF: ignore "break" caused by "Serial.swap()" in ESP)
            if (U1ERRIRbits.RXFOIF)
            {
                ch = U1RXB;
                break;
            }

            ch = U1RXB;
            if (ch == '\n')            // ignore CR
            {
                if (index > 1)  eol = true; 
                else            break;
            }
            record.buffer[index++] = ch;
        } 
        record.buffer[index] = '\0';
        
    // Process received INTEL Hex record.
        if ((index >= 11) && eol && (record.code == ':'))
        {
            type    = xtou8((uint8_t *)&record.type);
            length  = xtou8((uint8_t *)&record.length);     // no. of data BYTES
            address = xtou8((uint8_t *)&record.address[0]); // MSB
            address <<= 8;
            address += xtou8((uint8_t *)&record.address[2]);
            cs      = xtou8((uint8_t *)&record.buffer[9 + (length << 1)]);
            offset = (address & ((PAGESIZE * 2) - 1)) / 2;  // WORD offset

            if (0x00 == type)  // data records
            {   // sanity check
                if (   (address < 0x0800)  // protect bootloader
                    || (address & 1)       // even record address only
                    || ((offset + length/2) > PAGESIZE))   // crossing pages?
                {
                    // errs and bootloader addresses get fully echoed 
                }
                else
                {                    
                // read the entire page (128 words) into buffer RAM
                    NVMADR = address;
                    NVMCON1bits.CMD = 0x02; // Set the page read command
                    INTCON0bits.GIE = 0;    // Disable interrupts
                    NVMCON0bits.GO = 1;     // Start page read
                    while (NVMCON0bits.GO); // Wait for read operation to complete

                // copy record into Buffer RAM, convert big to little endian
                    bufPtr = bufferRamPtr + offset;
                    for (uint8_t i = 0; i < (length << 1); i += 4) // i counts ascii chars
                    {   // data: 16 bit, 2 ascii char => 1 byte
                        data = xtou8((uint8_t *)&record.data[i + 2]);   // MSB
                        data <<= 8;
                        data += xtou8((uint8_t *)&record.data[i]);
                        *bufPtr++ = data;     // program flash is 16 bit
                    }

                // Erase current page
                    NVMADR = address;
                    NVMCON1bits.CMD = 0x06; // Set the page erase command
                    //????????? Required Unlock Sequence 
                    NVMLOCK = 0x55;
                    NVMLOCK = 0xAA;
                    NVMCON0bits.GO = 1; // Start page erase
                    //??????????????????????????????????
                    while (NVMCON0bits.GO); // Wait for the erase operation to complete

                // Verify erase operation success
                    if (NVMCON1bits.WRERR)  continue;   // don't acknowledge this record

                // write updated page into flash
                    NVMCON1bits.CMD = 0x05; // page write command
                    //????????? Required Unlock Sequence
                    NVMLOCK = 0x55;
                    NVMLOCK = 0xAA;
                    NVMCON0bits.GO = 1; // Start page write
                    //??????????????????????????????????
                    while (NVMCON0bits.GO); // Wait for write operation to complete
                    if (NVMCON1bits.WRERR)  continue;   // don't acknowledge on error

                    NVMCON1bits.CMD = 0x00;

                // acknowledge the record's checksum only:
                    putch(u8tox(cs >> 4));      // convert checksum to ASCII
                    putch(u8tox(cs & 0x0F));
                    putch('\r'); 
                    putch('\n');

                    continue;
                }
            } // if data record (type 0)

            /// if End Of File, write EEPROM to disable bootloader
            if (0x01 == type)
            {
                eof = true;

                INTCON0bits.GIEH = 0;     // disable INTs
                NVMADR = EEPROM_BASE + 0x00; // write to EEPROM[0] anything ..
                NVMDATL = 0x00;              // .. but 0xFF to disable bootloader
                NVMCON1bits.CMD = 0x03;
                NVMLOCK = 0x55;              // unlock EEPROM
                NVMLOCK = 0xAA;
                NVMCON0bits.GO = 1;          // perform write

                for (uint8_t timeout = 0; timeout < 20; ++timeout)
                {
                    if (!NVMCON0bits.GO) break;
                    else __delay_ms(1);
                }
                NVMCON1bits.CMD = 0;
            } // if type == EOF
            
        } // if : data record
        
        // echo back unprocessed input line ("Bootload!", BL addresses or Errs)
        for (uint8_t k = 0; k < sizeof(record.buffer); k++) 
        {
            ch = record.buffer[k];
            if (ch < ' ') break;
            putch(ch);
        }
        putch('\r'); 
        putch('\n');
        while (!U1ERRIRbits.TXMTIF) ;  // until shift reg. is empty
        
    } // while (!EOF)
    
    RESET();    // if type == EOF
// *****************************************************************************   
    /* It is recommended that the main() function does not end: */
    while(1) continue;
    
} // main ()

// *** private function bodies


/** @brief Function to convert two ANSI hexadecimal chars to an uint8_t result.
 *  The 2st char is the high nibble and the 2nd char is the low nibble.
 *  This function does not expect a termination, but simply reads the 2 chars, 
 *  pointed to by p, and converts them to a uint8_t result. */
static uint8_t
xtou8 (uint8_t *p)
{
    uint8_t c;
    uint8_t i = 0;

    c = *p;
    if(c >= 'a') c &= ~0x20;
    if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A'))) return(i);
    c -= '0';
    if (c > 9) c -= 7;
    i = c;

    c = *(++p);
    if(c >= 'a') c &= ~0x20;
    if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A'))) return(i);
    c -= '0';
    if (c > 9) c -= 7;
    i = (uint8_t)(i << 4) + c;

    return i;

} // xtou8 ()


static char 
u8tox (uint8_t n)
{
    if (n < 10) {
        return (n + '0');
    } else {
        return ((n - 10) + 'A');
    }
} // u8tox ())

/**
 End of File
*/