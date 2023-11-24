/**
 *  @file i2c.h
 *  @brief Deklarations for module i2c.c (project "ValveControl")
 *  @par    (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */ 
/*  Change Log:
 *  2023-11-23 V0.6
 *  - First issue
 */
#ifndef _I2C_H
#define	_I2C_H

// global variables
// data type, constant and macro definitions 
#define I2C_WRITE   0
#define I2C_READ    1

#define I2C_ACK     1
#define I2C_NACK    0

#define I2C_ADDR_INA219     0x80

// function prototypes
extern void     i2c_idle (void);
extern void     i2c_init (void);
extern uint8_t  i2c_send (uint8_t byte);
extern void     i2c_start (void);
extern void     i2c_stop (void);

extern int16_t  ina219_read (void);
extern void     ina219_reg (uint8_t ina216reg);
extern void     ina219_write (uint8_t ina216reg, int16_t value);

#endif	/* _I2C_H */

