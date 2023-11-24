/**
 * @file i2c.c
 *  @brief  Implementations for I2C operations
 *  @par  (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */
/*  ChangeLog:
 * 2023-10-17 V0.1
 * - Initial issue
 */

#include <xc.h>             /* XC8 General Include File */
#include "main.h"
#include "i2c.h"


/** @brief Initializes the I2C module.
 *  - CLK: 400 kHz
 *  - MODE: 7-bit address
 */
void i2c_init (void)
{
    //EN disabled; RSEN disabled; S Cleared by hardware after Start; CSTR Enable clocking; MODE 7-bit address; 
    I2C1CON0 = 0x04;
    //ACKCNT Acknowledge; ACKDT Acknowledge; P Cleared by hardware after sending Stop; RXO No overflow; TXU No underflow; CSD Clock Stretching enabled; 
    I2C1CON1 = 0x80;
    //ACNT disabled; GCEN disabled; FME disabled; ABD disabled; SDAHT 30 ns hold time; BFRET 8 I2C Clock pulses; 
    I2C1CON2 = 0x18;
    //CLK Fosc/4 = 4 MHz
    I2C1CLK = 0x00;     // obsolete 0x03: MFINTOSC = 500 kHz
    //CNTIF Byte count is not zero; ACKTIF Acknowledge sequence not detected; WRIF Data byte not detected; ADRIF Address not detected; PCIF Stop condition not detected; RSCIF Restart condition not detected; SCIF Start condition not detected; 
    I2C1PIR = 0x00;
    //CNTIE disabled; ACKTIE disabled; WRIE disabled; ADRIE disabled; PCIE disabled; RSCIE disabled; SCIE disabled; 
    I2C1PIE = 0x00;
    //BTOIF No bus timeout; BCLIF No bus collision detected; NACKIF No NACK/Error detected; BTOIE disabled; BCLIE disabled; NACKIE disabled; 
    I2C1ERR = 0x00;
    //Count register
    I2C1CNTL = 0x00;
    I2C1CNTH = 0x00;
    //BAUD 9    obsolete 0; 
    I2C1BAUD = 0x01;
    
} // i2c_init ()


/**	@brief Read 2 byte of data from INA219. \n
 *  NOTE: register must be set prior read! \n
 *  When ABD is set (ABD = 1), the address buffer is disabled. In this case, 
 *  the number of data bytes are loaded into I2CxCNT, and the client’s 7-bit 
 *  address and R/W bit are loaded into I2CxTXB. A write to I2CxTXB will cause 
 *  host hardware to automatically issue a Start condition once the bus is 
 *  Idle (BFRE = 1). Software writes to the Start bit are ignored. \n
 *  See datasheet (36.4.2.7.1), page 635.
 */
int16_t ina219_read (void)
{
    int16_t value = 0;

    for (uint8_t timeout = 0; !I2C1STAT0bits.BFRE && (++timeout < 50); );
    if (I2C1STAT0bits.BFRE)
    {    
        I2C1STAT1bits.TXWE = 0; // Clear Transmit Write Error Status
        I2C1PIRbits.PCIF = 0;
        if (I2C1STAT1bits.RXBF) value = I2C1RXB;    // Clear rxbuf
        I2C1STAT1 = 0;           // clear error status
        
        I2C1CNTL = 0x02;    // Count register
        I2C1CNTH = 0x00;
        
        // Transmit the INA's I2C address (0x40 << 1) + READ
        I2C1TXB  = I2C_ADDR_INA219 + I2C_READ;    // address + read

        // Receive the INA's Register content (MSB first)
        for (uint8_t timeout = 0; !I2C1STAT1bits.RXBF && (++timeout < 100); );
        value = ((int16_t) I2C1RXB) << 8;

        for (uint8_t timeout = 0; !I2C1STAT1bits.RXBF && (++timeout < 100); );
        value += I2C1RXB;   // LB
        
        // wait 'til transfer has completed (stop condition detected)
        for (uint8_t timeout = 0; !I2C1PIRbits.PCIF && (++timeout < 50); );
        // a minimum delay is required following every write_i2c() !       
        __delay_us(10);    // min 4 µs
        
    }
    else
    {   // error handler
        __delay_ms(1);
    }
    
    return(value);
    
} // ina219_read()


/**	@brief Set INA219 register address.
 */
void ina219_reg (uint8_t ina219reg)
{
    for (uint8_t timeout = 0; !I2C1STAT0bits.BFRE && (++timeout < 50); );
    if (I2C1STAT0bits.BFRE)
    {    
        I2C1STAT1bits.TXWE = 0; // Clear Transmit Write Error Status
        I2C1PIRbits.PCIF = 0;
        
        I2C1CNTL = 0x01;    // Count register
        I2C1CNTH = 0x00;
        
        // Transmit the INA's I2C address (0x40 << 1)
        I2C1TXB  = I2C_ADDR_INA219 + I2C_WRITE;    // address + write

        // The next data byte is the INA's Register Address
        for (uint8_t timeout = 0; !I2C1STAT1bits.TXBE && (++timeout < 100); );
        I2C1TXB  = ina219reg;

        // wait 'til transfer has completed (stop condition detected)
        for (uint8_t timeout = 0; !I2C1PIRbits.PCIF && (++timeout < 50); );
        // Nach jedem write_i2c muss ein min. Delay eingehalten werden!       
        __delay_us(10);    // min 4 µs
    }
    else
    {   // error handler
        __delay_ms(1);
    }
    
} // ina219_reg()


/**	@brief Write data to INA219 register. \n
 *  When ABD is set (ABD = 1), the address buffer is disabled. In this case, 
 *  the number of data bytes are loaded into I2CxCNT, and the client’s 7-bit 
 *  address and R/W bit are loaded into I2CxTXB. A write to I2CxTXB will cause 
 *  host hardware to automatically issue a Start condition once the bus is 
 *  Idle (BFRE = 1). Software writes to the Start bit are ignored.
 *  See datasheet (36.4.2.7.1), page 635.
 */
void ina219_write (uint8_t ina219reg, int16_t value)
{
    for (uint8_t timeout = 0; !I2C1STAT0bits.BFRE && (++timeout < 50); );
    if (I2C1STAT0bits.BFRE)
    {    
        I2C1STAT1bits.TXWE = 0; // Clear Transmit Write Error Status
        I2C1PIRbits.PCIF = 0;
        
        I2C1CNTL = 0x03;    // Count register
        I2C1CNTH = 0x00;
        
        // Transmit the INA's I2C address (0x40 << 1) + write
        I2C1TXB  = I2C_ADDR_INA219 + I2C_WRITE;

        // The 1st data byte is the INA's Register Address
        for (uint8_t timeout = 0; !I2C1STAT1bits.TXBE && (++timeout < 100); );
        I2C1TXB  = ina219reg;

        // The next two bytes are written to the addressed INA219 register
        for (uint8_t timeout = 0; !I2C1STAT1bits.TXBE && (++timeout < 100); );
        I2C1TXB  = (uint16_t)value >> 8;
        for (uint8_t timeout = 0; !I2C1STAT1bits.TXBE && (++timeout < 100); );
        I2C1TXB  = (uint8_t) value;
        
        // wait 'til transfer has completed (stop condition detected)
        for (uint8_t timeout = 0; !I2C1PIRbits.PCIF && (++timeout < 50); );
        // Nach jedem write_i2c muss ein min. Delay eingehalten werden!       
        __delay_us(10);    // min 4 µs
    }
    else
    {   // error handler
        __delay_ms(1);
    }
    
} // ina219_write()


// *** private function bodies


/**
 End of File
 */