/** @file   adc.c
 *  @brief  Implementations for driver APIs for Analog/Digital Converter
 *  @par  (c) 2023 Klaus Deutschämer \n
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */
/*  ChangeLog:
 * 17.10.2023 V0.1
 * - Initial issue
 */

// *** includes

#include <xc.h>
#include <stdint.h>         // uint8_t ... (C99 standard types)

#include "main.h"
#include "adc.h"

// *** data type, constant and macro definitions
// *** global variables
// *** private variables

// *** public function bodies

/** @brief  Initializes the 12-bit A/D Converter (right adjusted result)
 *          - Clock = FOSC/32, VREF+ = 2,048 V ADFVR, VREF- = GND, no INTs
 *  @param  chs     binary code of voltage source
 */
void 
adc_init (uint8_t chs)
{
    ADCON0 = 0;                 /// - ADC stop & disable, clock supplied by FOSC
    ADCON0bits.FM = 1;          /// - ADRES+ADPREV data right-justified
    ADPRE = 0;                  /// - no precharge
    ADCON1 = 0;                 /// - default
    ADCON2 = 0;                 /// - Legacy mode, no filtering, ADRES->ADPREV
    ADCON3 = 0;                 /// - no math functions
    ADREF = 0x03;               /// - VREF- = AVSS, VREF+ = internal ADFVR  
    ADCLK = 15;                 /// - Clock divider: Conversion clock = FOSC/32
    ADPCH = chs;                /// - ADC Positive Input = chs
    ADACT = 0x00;               /// - External Trigger Disabled
    ADRES = 0;                  /// - Result = 0

    // Setup FVR:
    FVRCONbits.ADFVR = 0b10;    /// - FVR Gain is 2x (-> 2.048 V)
    FVRCONbits.FVREN = 1;       /// - Enable FVR

    PIE1bits.ADIE = 0;          /// - Disable ADC interrupt
    PIR1bits.ADIF = 0;          /// - Clear interrupt flag
    
} // adc_init ()


/** @brief  Waits for completion of the AD conversion. The 10 bit positive raw 
 *          AD result must be read from register pair ADRES (right adjusted).
 *  @return 0               No errors detected. \n
 *          E_ADC_TIMEOUT   AD converter timeout.
 */
int8_t 
adc_read (void) 
{
    uint8_t timeout;
    int8_t error = 0;
    
    // start polling the GO/DONE bit
    for (timeout = ACQ_US_TIMEOUT; ADCON0bits.GO && timeout > 0;  )
    {
        __delay_us(1);  // wait min. 1 µs
        timeout--;
    }    
    
    if (ADCON0bits.GO_nDONE)
    {
        error = E_ADC_TIMEOUT;
    }
    return(error);
    
} // adc_read ()


/** @brief Starts ADC analog to digital conversion
 *  @param chs = { 0 .. 0x3F } (ANA0 .. ANC7, AVss, TI, DAC1, VFR1, FVR2)
 *               Note: Some values are reserved (see the datasheet)
 */
void 
adc_start (uint8_t chs) 
{
    ADPCH = chs;                /// - Select the A/D channel
    ADCON0bits.ADON = 1;        /// - Turn on the ADC module
    __delay_us(ACQ_US_DELAY);   /// - Acquisition time delay
    ADCON0bits.GO_nDONE = 1;    /// - Start the conversion
    
} // adc_start ()


// *** private function bodies

/**
 End of File
 */