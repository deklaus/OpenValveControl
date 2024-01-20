/** @file main.h
 *  @brief Declarations for  Project "ValveControl"
 *  @par  (c) 2023 Klaus Deutschkämer
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

#define _XTAL_FREQ  16000000L   /* oscillator frequency, required for _delay() */

// Base address of EEPROM, valid for PIC18F04/05/06/14/15/16Q41.
#define EEPROM_BASE 0x380000

#define LED     LATAbits.LATA2

#define interrupt_GlobalHighEnable()   (INTCON0bits.GIEH = 1)
#define interrupt_GlobalHighDisable()  (INTCON0bits.GIEH = 0)

#define interrupt_GlobalLowEnable()    (INTCON0bits.GIEL = 1)
#define interrupt_GlobalLowDisable()   (INTCON0bits.GIEL = 0)

#define NUM_VZ  4           /* no. of valve zones */
#define	MSperTICK	100     /* time [ms] to move a motor by 1 % of max. travel */
#define	TIMEOUThome 120     /* timeout [s] for homeing */

#define VBEMF_NO_DIA    1   /* AD conversion without calibration */ 

//#define TEST_DACOUT_A2    1   /* monitor variables via DAC (@RA2 = /LED) */
//#define TEST_SETTINGS     1   /* to enable some TESTS during development */
#define TEST_SETREF       1   /* sets ref position flags for testing */
//#define TEST_AUTO_RETURN  1   /* MOVE auto returns to zero position */

typedef union {     // 'low Byte', weak errors: mission may be continued.
    struct {
        uint8_t ref         :4;  //  0-3: vz  referenced
        uint8_t vz          :4;  //  4-7: vz  in process
        uint8_t             :8;  //  8-15 spare
    };
    struct {
        uint8_t ref1        :1;  //  0: vz1 referenced
        uint8_t ref2        :1;  //  1: vz2 referenced
        uint8_t ref3        :1;  //  2: vz3 referenced
        uint8_t ref4        :1;  //  3: vz4 referenced
        uint8_t vz1         :1;  //  4: vz1 in process
        uint8_t vz2         :1;  //  5: vz2 in process
        uint8_t vz3         :1;  //  6: vz3 in process
        uint8_t vz4         :1;  //  7: vz4 in process
        uint8_t move        :1;  //  8: 'move' is executing
        uint8_t home        :1;  //  9: 'home' is executing 
        uint8_t bootload    :1;  // 10: 'bootload' reboot and start bootloader
        uint8_t             :5;  // 11-15 spare
    };
} STATUSflags_t;    // token "STATUSbits" reserved by microchip

typedef union {     // 'low Byte', weak errors: mission may be continued.
    struct {
        uint8_t CRC         :1;  // 0 Program checksum error 
        uint8_t UNEXP_INT   :1;  // 1 Unexpected interrupt
        uint8_t OVER_CURR   :1;  // 2 Overcurrent during move
        uint8_t             :5;  // 3-7 spare
    };
    uint8_t v;
} ERRORflags_t;  

enum Errs {     /* ALL errnos must be negative (see adc_read() as example) */
    E_ADC_TIMEOUT     = -127,   // AD converter timeout

    E_HOMEING_ACTIVE  = -6,     // Move command, whereas Home is active
    E_NO_REFERENCE    = -5,     // reference not set
    E_UNDEF_CMD       = -4,     // undefined command
    E_MA_MAX          = -3,     // ma_max out of range
    E_SET_POS_RANGE   = -2,     // set_pos out of range
    E_VZ_RANGE        = -1,     // vz out of range
};

// function prototypes


// global variables
uint16_t    FVRA2X;
uint16_t    FVRC2X;

extern volatile STATUSflags_t  g_STATUSflags;
extern volatile ERRORflags_t   g_ERRORflags;

extern volatile uint8_t    g_rs232_request;
extern volatile uint8_t    g_rs232_response;

extern volatile uint8_t    g_rx232_buf[48];
extern volatile uint8_t    g_tx232_buf[48];
extern volatile uint8_t    g_rx232_count;

extern volatile uint16_t   g_timer_ms;
extern volatile bool       g_tovfl_ms;

extern volatile uint8_t    g_vz;
extern volatile uint8_t    g_setpos[NUM_VZ + 1]; 
extern volatile uint8_t    g_position[NUM_VZ + 1];
extern volatile int16_t    g_zcd[NUM_VZ + 1];
extern volatile int16_t    g_max_mAx10[NUM_VZ + 1];
extern volatile int16_t    g_mAx10;
extern volatile uint16_t   g_vbemf;
extern volatile int8_t     g_dir;

extern volatile uint8_t    g_bemf8[5];
extern volatile uint8_t    g_zerocount;

#endif	/* _MAIN_H */