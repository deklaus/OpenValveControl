/** @file daq.h
 *  @brief Prototypes and definitions for module daq.c (project "ValveControl")
 *  @par    (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */ 
/*  Change Log:
 *  2023-11-23 V0.6
 *  - First issue
 */
#ifndef _DAQ_H
#define	_DAQ_H

// global variables
// data type, constant and macro definitions

// function prototypes
int16_t  daq_temperature (void);
uint16_t daq_vbemf (uint8_t vz);
uint16_t daq_vdd (void);

#endif	/* _DAQ_H */
