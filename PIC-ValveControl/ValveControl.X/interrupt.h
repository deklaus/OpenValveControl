/** @file interrupt.h
 *  @brief Prototypes and definitions for module interrupt.c (project "ValveControl")
 *  @par  (c) 2023 Klaus Deutschämer \n
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */
/*  Change Log:
 * 17.10.2023 V0.1
 *  - First issue
 */
#ifndef _INTERRUPT_H
#define	_INTERRUPT_H
   
// global variables
// data type, constant and macro definitions   
#define interrupt_GlobalHighEnable()   (INTCON0bits.GIEH = 1)
#define interrupt_GlobalHighDisable()  (INTCON0bits.GIEH = 0)

#define interrupt_GlobalLowEnable()    (INTCON0bits.GIEL = 1)
#define interrupt_GlobalLowDisable()   (INTCON0bits.GIEL = 0)

// function prototypes
void interrupt_initialize (void);

#endif  // _INTERRUPT_H

