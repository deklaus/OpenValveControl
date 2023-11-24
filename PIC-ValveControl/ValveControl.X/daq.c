/** @file  daq.c
 *  @brief Functions for Data Acquisition (DAQ)
 *  @par  (c) 2023 Klaus Deutschkämer
 *  License: EUROPEAN UNION PUBLIC LICENCE v. 1.2 \n
 *  see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 */
/*  Change Log:
 * 2023-10-17 V0.1
 * - Initial issue
 */

#include <xc.h>
#include "main.h"
#include "adc.h"
#include "daq.h"


/** @brief Returns an uncalibrated temperature estimation. \n
 *  Uses the temperature indicator (TI) of the PIC µC. 
 *  Before calling this function, FVR and TI must have been 
 *  initialized and enabled. \n
 *  Execution time ca. 890 µs (incl. 8-fold averaging).
 *  @return int16_t temperature [0.1 °C] (-2731 on ADC error)
 */
int16_t 
daq_temperature (void)
{
    int8_t      error = 0;
    int24_t     tempC = -2731;
    int32_t     sum;
    int16_t     TSHR1;  // Temp. indicator gain (hi range)
    int16_t     TSHR3;  // Temp. indicator offset (hi range)
    
    NVMADR = 0x2C002A;          // @TSHR1 = Gain, high range
    NVMCON1bits.CMD = 0b000;    // read command (PFM: word)
    NVMCON0bits.GO = 1;         // start word read
    while (NVMCON0bits.GO);     // wait for the read operation to complete
    TSHR1 = (int16_t) NVMDAT;   // 0xFDEE = -530

    NVMADR = 0x2C002E;          // @TSHR3 = Offset (high range)
    NVMCON1bits.CMD = 0x00;     // set the word read command
    NVMCON0bits.GO = 1;         // start word read
    while (NVMCON0bits.GO);     // wait for the read operation to complete
    TSHR3 = (int16_t) NVMDAT;   // 0x1A03 = 6659
    
    adc_init (0x3C);        // ADPCH = Temp. Indicator, VREF+ = ADFVR (2x)
    sum = 0;
    for (uint8_t i = 0; (i < 8) && (error == 0); i++)
    {   // averaging (8x)
        adc_start(0x3C);
        error = adc_wait();
        sum += ADRES;
    }
    
    if (error == 0)
    {   // temp = ((ADRES*Gain)/256 + Offset) / 10;  get Offset & Gain from DIA
        sum >>= 3;
        tempC = (int24_t) (sum) * TSHR1;    // ADRES * gain
        tempC >>= 8;
        tempC += TSHR3;
        //tempC /= 10;      don't divide by 10 - we return in 1/10th degree
    }   
    return ((int16_t) tempC);
    
} // daq_temperature()


/** @brief Measures the voltage VDD of the controller \n
 *  With ADC_VREF+ = VDD and ADC_Positive_Input = FVR1 we get: \n
 *  u(VFR1) = ADRES * Vdd / 4095 := FVRA2X / 1000 => \n
    Vdd = FVR2AX * 4095 / 1000 / ADRES      ; in [V]
 *  @return  Voltage [0.01 V]
 */
uint16_t
daq_vdd (void)
{
    uint32_t    v;
    uint16_t    vdd;
    
    adc_init(0b111110);     // ADC Positive Input = FVR1 (ADC module)
    ADREF = 0;              // change VREF+ = VDD
    adc_start(0b111110);

#ifdef VDD_WITHOUT_DIA
    // Measured value Eval Board using default FVR = 2.048 V: 3,29 V
    // Vref = adc * Vdd / 4095 = 2.048 V => 
    // Vdd = 4095 * 2.048 / adc = 8386,56 / adc
    if (adc_wait() < 0) vdd = 999;  // signal error: 9.99 V    
    else                vdd = (uint16_t) (838656L / ADRES);     // [0.01 V]
#else
    // Measured value Eval Board using FVRA2X from DIA: 3,32 V
    if (adc_wait() < 0) vdd = 999;  // signal error: 9.99 V    
    else
    {
        v = (uint32_t) FVRA2X * 4095 / 10;
        vdd = (uint16_t) (v / ADRES);     // in [0.01 V]
    }
#endif    
    return (vdd);
    
} // daq_vdd ()



/** @brief Reads the back EMF voltage of channel vz. \n
 *  Returns the raw ADC RESULT. \n
 *  Exec time ca. 100 µs
 *  @return  Voltage [ADC raw]
 */
uint16_t 
daq_vbemf (uint8_t vz)
{
    uint8_t     chs;
    uint32_t    uval32;
    uint16_t    vbemf = 0;
    
    switch (vz)
    {
        case 4: 
            chs = 0b00010000;   // VBEMF4 Input = RC0/ANC0
            break;
        case 3:
            chs = 0b00010001;   // VBEMF3 Input = RC1/ANC1
            break;
        case 2:
            chs = 0b00000000;   // VBEMF4 Input = RA0/ANA0
            break;
        default:
//            chs = 0b00000001;   // VBEMF4 Input = RA1/ANA1
            chs = 0b00010000;   // VBEMF Input = RC0/ANC0                      (TEST)
            break;
    }

    adc_init(chs);     // ADC Positive Input = FVR1 (ADC module), FVR: 2.048 V
    adc_start(chs);

    if (adc_wait() < 0) vbemf = g_vbemf;    // if timeout: keep previous value
    else                vbemf = ADRES;      // Basic mode

    // read twice and average
    adc_start(chs);
    if (adc_wait() < 0) vbemf += g_vbemf;    // if timeout: keep previous value
    else    vbemf += ADRES;    
    vbemf >>= 1;
    
    return (vbemf);

} // daq_vbemf ()


/**
 End of File
 */
