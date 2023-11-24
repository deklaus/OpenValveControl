/** @file adc.h
 *  @brief Prototypes and definitions for module adc.c (project "ValveControl")
 *  @par    (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */ 
/*  Change Log:
 *  2023-11-23 V0.6
 *  - First issue
 */
#ifndef _ADC_H
#define	_ADC_H

// global variables
// data type, constant and macro definitions
#define ACQ_US_DELAY    50
#define ACQ_US_TIMEOUT  50

// function prototypes
void    adc_init (uint8_t chs);
void    adc_start (uint8_t chs);
int8_t  adc_wait (void);

#endif	/* _ADC_H */

