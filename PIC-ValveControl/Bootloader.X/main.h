/** @file main.h
 *  @par    (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */ 
/*  Change Log:
 *  2023-11-23 V0.6
 *  - First issue
 */

#include <stdint.h>     // defines C99 standard types as 'uint8_t' and 'int16_t'
#include <stdbool.h>    // defines type 'bool' 

#ifndef _MAIN_H
#define	_MAIN_H

// data type, constant and macro definitions

#define _XTAL_FREQ  16000000L   /* oscillator frequency for _delay() */

// Base address of EEPROM, valid for PIC18F04/05/06/14/15/16Q41.
#define EEPROM_BASE     0x380000

#define WRITE_FLASH_BLOCKSIZE   1
#define ERASE_FLASH_BLOCKSIZE   128
#define END_FLASH               0x00FFFF    /* PIC18F16Q41 */
#define RECORD_SIZE             2*16        /* databytes in INTEL Hex-records */

typedef volatile union
{
    struct
    {
        uint8_t     code;       // ':' start code of INTEL Hex records
        uint8_t     length[2];  // number of data bytes (to be programmed)
        uint8_t     address[4];
        uint8_t     type[2];    // record type: 
                    // 00: Data record              01: End of File,  
                    // 02: Extended Segment Address 03: Start Segment Address (CS:IP)
                    // 04: Extended Linear Address  05: Start Linear Address
        uint8_t     data[RECORD_SIZE + 4];      // data bytes + checksum + 4 EOL
    };
    uint8_t  buffer[RECORD_SIZE + 13];
} record_t;


// function prototypes
// global variables
extern record_t  record;
extern uint16_t  procID;
extern uint16_t *bufferRamPtr;

#endif	/* _MAIN_H */