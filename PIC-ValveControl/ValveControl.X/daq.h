/** @file daq.h
 *  @brief Prototypes and definitions for module daq.c (project "ValveControl")
 *  @par  (c) 2023 Klaus Deutschämer \n
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */
/*  Change Log:
 * 17.10.2023 V0.1
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
