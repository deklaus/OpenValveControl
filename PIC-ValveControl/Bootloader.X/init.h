/** @file init.h
 *  @par    (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */ 
/*  Change Log:
 *  2023-11-23 V0.6
 *  - First issue
 */

#ifndef _INIT_H
#define	_INIT_H

// function ptototypes
void    init_bootloader (void);
void    init_comparator2 (void);
void    init_fvr (void);
void    init_opa1 (void);
void    init_osc (void);
void    init_pins (void);
void    init_pmd (void);
void    init_timer4 (void);
void    init_uart1 (void);

// variable declarations
// extern uint8_t ...

#endif	/* _INIT_H */

