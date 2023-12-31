/** @file  DS18B20.c
 *  @author  (c) Klaus Deutschkämer (https://github.com/deklaus)
 *  License: This software is licensed under the European Union Public Licence EUPL-1.2
 *           (see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12 for details).
 *
 *  @brief Functions for DS18B20 temperature sensor. \n
 *  Weblink: https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/DallasTemperature.h
 */
/*  Change Log:
 *  2023-11-02
 *  - First issue
 */
#include <DallasTemperature.h>

// *** data type, constant and macro definitions

const byte ONE_WIRE_PIN = D5;   // configure one wire pin @ESP8266

// *** global variables
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature DS18B20(&oneWire);

// *** private variables
// *** public function bodies

/** @brief initializes the DS18B20.
 *  Declare this function in your project .ino.
 *  Call this function within setup().        
 */
void DS18B20_init ()
{
  char      buf[64];

  DS18B20.begin();

#ifdef DEBUG_OUTPUT_DS1820
  Serial.flush();   // Wait for the transmission of outgoing serial data to complete
  Serial.swap();    // map UART0 to serial monitor
  Serial.println();
  Serial.print("Parasite power is: "); 
  if (DS18B20.isParasitePowerMode()) Serial.println("ON");
  else                               Serial.println("OFF");
# endif

  if (DS18B20.getDS18Count() == 0)
  {
#ifdef DEBUG_OUTPUT_DS1820
    sprintf(buf, "No DS18B20 detected at pin %d", ONE_WIRE_PIN); 
    Serial.println(buf);
# endif
  }
  else
  {
    DS18B20.setResolution(12);    // allowed values: 9, 10, 11, 12 bit
    DS18B20.setWaitForConversion(false);
    DS18B20.requestTemperatures();
#ifdef DEBUG_OUTPUT_DS1820
    sprintf(buf, "DS18B20 detected at pin %d", ONE_WIRE_PIN); 
    Serial.println(buf);
    sprintf(buf, "(resolution set to %d bit)", DS18B20.getResolution()); 
    Serial.println(buf);
# endif
  }

#ifdef DEBUG_OUTPUT_DS1820
  Serial.flush();   // Waits for the transmission of outgoing serial data to complete
  Serial.swap();    // remap output to PIC
# endif

} // DS18B20_init ()


/** @brief Returns DS18B20 temperature in Celsius.
 *  Declare this function in your project .ino.
 */
float DS18B20_TempC (uint8_t index)
{                  
  float temp = DS18B20.getTempCByIndex(index);
  DS18B20.requestTemperatures();  // new request
  
  return (temp);

} // DS18B20_TempC()

