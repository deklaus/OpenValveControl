/** @file  setup.ino
 *  @author  (c) Klaus Deutschkämer (https://github.com/deklaus)
 *  License: This software is licensed under the European Union Public Licence EUPL-1.2
 *           (see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12 for details).
 *
 *  @brief Utilities for setup and (INI) file access. \n
 *  Simple implementations of GetPrivateProfileString(), GetPrivateProfileInt(),
 *  a function which reads a float value, and a WriteCString() to save values in INI file.
 */
/*  Change Log:
 *  2023-11-15
 *  - First issue
 */

/** @brief  Reads one char array from ini File (in LittleFS)
 *  @param  char *path         Filename (must start with "/")
 *  @param  char *identifier   Identifier of string (e.g. "dTemp = ") - must match including the spaces!
 *  @param  char *result[len]  Result char array with minimum size len
 *  @return int  error
*/
int setup_GetCstring (const char *path, const char *identifier, char *result, int len)
{
  int     i, error = 0;
  boolean found = false;
  String  str;
  File    ovc = LittleFS.open(path, "r");

  if(!ovc)
  {
    error = -1; // file not found
#ifdef DEBUG_OUTPUT_INI
    Serial.flush();   // Waits for the transmission of outgoing serial data to complete
    Serial.swap();    // output to serial monitor
    Serial.print(path);  Serial.println(" not found");
    Serial.flush();   // Waits for the transmission of outgoing serial data to complete
    Serial.swap();    // output to serial monitor
#endif
  }
  else
  {
    ovc.setTimeout(100);        // sets the maximum milliseconds to wait for stream data, it defaults to 1000 milliseconds.
    while (ovc.available())
    {
      str = ovc.readStringUntil('\n');    // read line, discard terminator
      if (str.startsWith(identifier)) 
      {
        if ((i = str.indexOf('=')) > 0)   // (paranthesis) required!
        {
          found = true;
          str = str.substring(i + 1);
          str.trim();
          str.toCharArray(result, len);    
          break;  // stop after 1st occurence
        }
      }
    } // while
    ovc.close();

    if (!found)
    {
      error = -2; // identifier not found
#ifdef DEBUG_OUTPUT_INI
      Serial.flush();
      Serial.swap();
      Serial.print("Identifier "); Serial.print(identifier); Serial.println(" not found!");
      Serial.flush();
      Serial.swap();
#endif
    } 
  }

  return (error);

} // setup_GetCstring ()


/** @brief  Reads one Integer from ini File (in LittleFS)
 *  @param  char *path          Filename (must start with "/")
 *  @param  char *identifier    Identifier of string (e.g. "ItsName = ") - must match including the spaces!
 *  @param  int  *result
 *  @return int  error
*/
int setup_GetInt (const char *path, const char *identifier, int *result)
{
  int     i, error = 0;
  boolean found = false;
  String  str;
  File ovc = LittleFS.open(path, "r");

  if(!ovc)
  {
    error = -1; // file not found
#ifdef DEBUG_OUTPUT_INI
    Serial.flush();   // Waits for the transmission of outgoing serial data to complete
    Serial.swap();    // output to serial monitor
    Serial.print(path);  Serial.println(" not found");
    Serial.flush();   // Waits for the transmission of outgoing serial data to complete
    Serial.swap();    // output to serial monitor
#endif
  }
  else
  {
    ovc.setTimeout(100);        // sets the maximum milliseconds to wait for stream data, it defaults to 1000 milliseconds.
    while (ovc.available())
    {
      str = ovc.readStringUntil('\n');    // read line, discard terminator
      if (str.startsWith(identifier)) 
      {
        if ((i = str.indexOf('=')) > 0)   // (paranthesis) required!
        {
          found = true;
          *result = str.substring(i + 1).toInt();
          break;  // stop after 1st occurence
        }
      }
    }
    ovc.close();

    if (!found)
    {
      error = -2; // identifier not found
#ifdef DEBUG_OUTPUT_INI
      Serial.flush();
      Serial.swap();
      Serial.print("Identifier "); Serial.print(identifier); Serial.println(" not found!");
      Serial.flush();
      Serial.swap();
#endif
    } 
    ovc.close();
  }
  return (error);

} // setup_GetInt ()


/** @brief  Reads one float from ini File (in LittleFS)
 *  @param  char  *path          Filename (must start with "/")
 *  @param  char  *identifier    Identifier of string (e.g. "dTemp = ") - must match including the spaces!
 *  @param  float *result
 *  @return int   error
*/
int setup_GetFloat (const char *path, const char *identifier, float *result)
{
  int     i, error = 0;
  boolean found = false;
  String  str;
  File ovc = LittleFS.open(path, "r");  // use "w" to write, r+ for reading and writing, .remove(file) to delete

  if(!ovc)
  {
    error = -1; // file not found
#ifdef DEBUG_OUTPUT_INI
    Serial.flush();   // Waits for the transmission of outgoing serial data to complete
    Serial.swap();    // output to serial monitor
    Serial.print(path);  Serial.println(" not found");
    Serial.flush();   // Waits for the transmission of outgoing serial data to complete
    Serial.swap();    // output to serial monitor
#endif
  }
  else
  {
    ovc.setTimeout(100);        // sets the maximum milliseconds to wait for stream data, it defaults to 1000 milliseconds.
    while (ovc.available())
    {
      str = ovc.readStringUntil('\n');    // read line, discard terminator
      if (str.startsWith(identifier)) 
      {
        if ((i = str.indexOf('=')) > 0)   // (paranthesis) required!
        {
          found = true;
          *result = str.substring(i + 1).toFloat();
          break;  // stop after 1st occurence
        }
      }
    }
    ovc.close();

    if (!found)
    {
      error = -2; // identifier not found
#ifdef DEBUG_OUTPUT_INI
      Serial.flush();
      Serial.swap();
      Serial.print("Identifier "); Serial.print(identifier); Serial.println(" not found!");
      Serial.flush();
      Serial.swap();
#endif
    } 
    ovc.close();
  }
  return (error);

} // setup_GetFloat ()


/** @brief  Reads setup data from ini File ovc.ini (in LittleFS)
 *  @return int  error
*/
int setup_ReadINI (const char *path)
{
  int error = 0;

  error += setup_GetCstring(path, "SSID", ssid, sizeof(ssid));
  error += setup_GetCstring(path, "PSK",  psk, sizeof(psk));

  error += setup_GetCstring(path, "VZ1",  alias1, sizeof(alias1));
  error += setup_GetCstring(path, "VZ2",  alias2, sizeof(alias2));
  error += setup_GetCstring(path, "VZ3",  alias3, sizeof(alias3));
  error += setup_GetCstring(path, "VZ4",  alias4, sizeof(alias4));
  
  error += setup_GetFloat(path, "dTemp",  &dTemp);

  return(error);

} // setup_ReadINI ()


/** @brief  Substitudes or writes one string parameter into ini File (in LittleFS)
 *          If ini file does not exist, it will be created.
 *  @param  char *path         Filename (must start with "/")
 *  @param  char *identifier   Identifier of string (e.g. "dTemp = ") - must match including the spaces!
 *  @param  char *s            char array to write
 *  @return int  error
 *  Weblinks:
 *  https://www.arduino.cc/reference/en/language/functions/communication/stream/
 *  https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html 
*/
int setup_WriteCstring (const char *path, const char *identifier, char *s)
{
  int     error = 0;
  boolean found = false;
  String  str;
  String  newContent = "";

  File   ovc = LittleFS.open(path, "r");   // Open for reading.  

  if(ovc)   // file exists
  {
    ovc.setTimeout(100);
    while (ovc.available())
    {
      str = ovc.readStringUntil('\n');    // read line, discard terminator
      if (str.startsWith(identifier)) 
      {
        found = true;
        newContent += identifier + String(" = ") + s + String("\r\n");
      }
      else
      {
        newContent += str + String("\n");
      }
    } // while
    ovc.close();  // close original file
  }

  if (!found)
  {
    newContent += identifier + String(" = ") + s + String("\r\n");
  }

  // write new content, open file for writing (+create if it does not exist).
  ovc = LittleFS.open(path, "w+");
  ovc.print(newContent);
  ovc.flush();
  ovc.close();

  return(error);

} // setup_WriteCstring ()


