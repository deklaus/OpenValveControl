/** @file init.h
 *  @brief Prototypes and definitions for module init.c (project "ValveControl")
 *  @par  (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */
/*  Change Log:
 * 17.10.2023 V0.1
 *  - First issue
 */
#ifndef _INIT_H
#define	_INIT_H

// global variables
// data type, constant and macro definitions 

#define TIMER_PRESCALER                 128
#define FREQUENCY_TO_PR_CONVERT(F)      (uint8_t)(((_XTAL_FREQ)/ \
                                        (4*(F))/(TIMER_PRESCALER))-1)
#define DUTYCYCLE_TO_CCPR_CONVERT(D,F)  (uint16_t)((float)(D)*(((_XTAL_FREQ)/ \
                                        (F)/(TIMER_PRESCALER))-1)/100.0)
/* Hz */
#define FREQUENCY_MAX                      4
#define FREQUENCY_MIN                      1
#define FREQUENCY_STEP                     1
/* percents */
#define DUTYCYCLE_MAX                     75
#define DUTYCYCLE_MIN                     25
#define DUTYCYCLE_STEP                    25


// function ptototypes
void init_oscillator(void);
void init_pin_manager (void);
void init_pmd(void);
void init_system(void);
void init_fvr(void);
void init_ina219 (void);
void init_pwm(void);
void init_timer0 (void);
void init_uart1 (void);

#endif	/* _INIT_H */
