/** @file  webUI.c
 *  @author  (c) Klaus Deutschkämer (https://github.com/deklaus)
 *  License: This software is licensed under the European Union Public Licence EUPL-1.2
 *           (see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12 for details).
 *
 *  @brief Functions for WiFi User Interface (UI)
 */
/*  Change Log:
 *  2023-11-23 v0.6
 *  - Added webUI_bootload. Essentially displays a notification and sets "flags.bootload",
 *    which then is processed in the main loop.
 *  2023-11-07 v0.3
 *  - webUI_Root() eliminated. Web-UI is now a separate file (index.html) and uploaded via LittleFS.
 *  2023-09-26 v0.12
 *  - First issue
 *
 *  Weblinks: 
 *  https://links2004.github.io/Arduino/d3/d58/class_e_s_p8266_web_server.html
 *  https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
 *  https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
 */

// *** data type, constant and macro definitions
// *** global variables
// *** private variables
// *** public function bodies


/** @brief  Sends Intel Hex-File to PIC Bootloader
 *  @param  char *path  Filename (must start with "/" and end with .hex)
 *  @return int  error
*/
void webUI_bootload (void)
{
  int     error = 0;
  String  str;
  String  htmlPage;
  htmlPage.reserve(128);  // prevent ram fragmentation

  htmlPage  = F("Update of PIC Firmware\n");

  if (server.hasArg("hexfile"))
  {
    str = server.arg("hexfile");
    str.toCharArray(hexfilename, sizeof(hexfilename));
    htmlPage += F("with hexfile: "); htmlPage += hexfilename; htmlPage += F("\n");

    if (!LittleFS.exists(hexfilename))
    {
      htmlPage += F("File doesn't exist! \n");
      for (uint8_t i = 0; i < server.args(); i++) 
      {
        htmlPage += server.argName(i) + "=" + server.arg(i) + "\n";
      }
    }
    else
    {
      htmlPage += "Starting bootloader - please wait for completion\n";
      htmlPage += "(check OLED display for status).\n";
      flags.bootload = 1;
    }    
  }
  else
  {
    htmlPage += F("Usage: <ip>/bootload?hexfile=filename.hex\n");
  }
  server.send(200, "text/plain", htmlPage);

} // webUI_bootload ()


/** @brief Handler for MOVE. Arguments: <ESP_IP>/move?vz=[1..4]&set_pos=[0..100]&max_mA=[0.0 .. 100.0]
 */
void webUI_move ()
{
  int error = 1;  // preset!
  char  buf[64];
  String htmlPage;
  htmlPage.reserve(128);  // prevent ram fragmentation
  int   sel;
  int   pos;
  float mA;

  if (server.hasArg("vz") && server.hasArg("set_pos") && server.hasArg("max_mA")) 
  {
    sel = server.arg("vz").toInt();
    pos = server.arg("set_pos").toInt();
    mA  = server.arg("max_mA").toFloat();
    if (sel >= 1 && sel <= 4 && pos >= 0 && pos <= 100 && mA >= 0.0 && mA <= 100.) 
    {
      htmlPage  = F("/move?vz="); htmlPage += sel;  
      htmlPage += F("&set_pos=");     htmlPage += pos;  
      htmlPage += F("&max_mA=");
      sprintf(buf, "%.1f", mA);       htmlPage += buf;
      htmlPage += F("\n");

      vz = sel;
      set_pos[sel] = pos;
      max_mA[sel] = mA;
      flags.move = 1;   // set flag for triggering a MOVE command to PIC
      error = 0;
    }
  }
  if (error) 
  {
    htmlPage = F("Parameter error: \n");
    for (uint8_t i = 0; i < server.args(); i++) 
    {
      htmlPage += server.argName(i) + "=" + server.arg(i) + "\n";
    }
  }    
  server.send(200, "text/plain", htmlPage);

} // webUI_move()


/** @brief Handler for HOME. Arguments: <ESP_IP>/home?vz=[1..4]&max_mA=[0.0 .. 100.0]
 */
void webUI_home ()
{
  int error = 1;  // preset!
  char  buf[128];
  String htmlPage;
  htmlPage.reserve(128);  // prevent ram fragmentation
  int   sel;
  float mA;

  if (server.hasArg("vz") && server.hasArg("max_mA")) 
  {
    sel = server.arg("vz").toInt();
    mA  = server.arg("max_mA").toFloat();
    if (sel >= 1 && sel <= 4 && mA >= 0. && mA <= 100.) 
    {
      htmlPage  = F("/home?vz="); htmlPage += sel;  
      htmlPage += F("&max_mA=");
      sprintf(buf, "%.1f", mA);   htmlPage += buf;
      htmlPage += F("\n");

      vz = sel;
      max_mA[sel] = mA;
      flags.home = 1;   // set flag for triggering a HOME command to PIC
      error = 0;
    }
  }
  if (error) 
  {
    htmlPage = F("Parameter error: \n");
    for (uint8_t i = 0; i < server.args(); i++) 
    {
       htmlPage += server.argName(i) + "=" + server.arg(i) + "\n";
    }    
  }
  server.send(200, "text/plain", htmlPage);

} // webUI_home ()


/** @brief Handler for INFO request.
 */
void webUI_info ()
{
  char  buf[128];
  String htmlPage;
  htmlPage.reserve(128);  // prevent ram fragmentation

  htmlPage = F(
  "{ \n"
  "\"ESP\": \"" );  htmlPage += ESPversion; htmlPage += F("\",\n"
  "\"PIC\": \"" );  htmlPage += PICversion; htmlPage += F("\",\n"
  "\"dTemp\": \""); htmlPage += dTemp;      htmlPage += F("\",\n"
  "\"SSID\": \"");  htmlPage += ssid;       htmlPage += F("\"\n" // no comma at end
  "}\n");

    //server.sendHeader("Access-Control-Allow-Origin","*"); 
    server.send(200, "text/plain", htmlPage);

} // webUI_info ()


/** @brief Handler for "SAVE" (@todo: POST). Arguments: <ESP_IP>/save?ssid=<string>&psk=<password>
 */
void webUI_save ()
{
  int error = 1;  // preset!
  char  buf[128];
  String htmlPage;
  htmlPage.reserve(256);  // prevent ram fragmentation
  String  id;
  String  pwd;

  if (server.hasArg("ssid") && server.hasArg("psk")) 
  {
    id = server.arg("ssid");
    pwd = server.arg("psk");
    htmlPage = F("{\n"
    "\"save\": {\n"
    "  \"ssid\": \""); htmlPage += id;  htmlPage += F("\",\n"
//    "  \"psk\": \"");  htmlPage += pwd; htmlPage += F("\"\n"
    "  \"psk\": \"");  htmlPage += "***"; htmlPage += F("\"\n"
    "  }\n}");

    strncpy(ssid, id.c_str(),  sizeof(ssid)); 
    ssid[sizeof(ssid)-1] = '\0';
    strncpy(psk,  pwd.c_str(), sizeof(psk));
    psk[sizeof(psk)-1] = '\0';

    flags.save = 1;   // set flag for new credentials (loop: save + reboot)
    error = 0;
  }
  if (error) 
  {
    htmlPage = F("Parameter error: \n");
    for (uint8_t i = 0; i < server.args(); i++) 
    {
       htmlPage += server.argName(i) + "=" + server.arg(i) + "\n";
    }    
  }
  server.send(200, "text/plain", htmlPage);

} // webUI_save ()


/** @brief Handler for STATUS request. \n
 *  Contains all readings and status' from PIC µC.
 *  May be extended, because data is parsed as JSON object (name:value).
 */
void webUI_status ()
{
  char  buf[128];
  String htmlPage;
  htmlPage.reserve(1000);  // prevent ram fragmentation

  htmlPage = F(
  "{ \n"
  "\"mAmps\": \""); sprintf(buf, "%.1f", mAmps); htmlPage += buf; htmlPage += F("\",\n"
  "\"tempC\": \""); sprintf(buf, "%.1f", tempC); htmlPage += buf; htmlPage += F("\",\n"

  "\"VZ1\": {\n"
  "    \"Position\": "); htmlPage += position[1]; htmlPage += F(",\n"
  "    \"Set_Pos\": ");  htmlPage += set_pos[1];  htmlPage += F(",\n"
  "    \"Ref_Set\": ");  htmlPage += refset[1];   htmlPage += F(",\n"
  "    \"max_mA\": \""); sprintf(buf, "%.1f", max_mA[1]); htmlPage += buf;   htmlPage += F("\"\n"
  "  },\n"

  "\"VZ2\": {\n"
  "    \"Position\": "); htmlPage += position[2]; htmlPage += F(",\n"
  "    \"Set_Pos\": ");  htmlPage += set_pos[2];  htmlPage += F(",\n"
  "    \"Ref_Set\": ");  htmlPage += refset[2];   htmlPage += F(",\n"
  "    \"max_mA\": \""); sprintf(buf, "%.1f", max_mA[2]); htmlPage += buf;   htmlPage += F("\"\n"
  "  },\n"

  "\"VZ3\": {\n"
  "    \"Position\": "); htmlPage += position[3]; htmlPage += F(",\n"
  "    \"Set_Pos\": ");  htmlPage += set_pos[3];  htmlPage += F(",\n"
  "    \"Ref_Set\": ");  htmlPage += refset[3];   htmlPage += F(",\n"
  "    \"max_mA\": \""); sprintf(buf, "%.1f", max_mA[3]); htmlPage += buf;   htmlPage += F("\"\n"
  "  },\n"

  "\"VZ4\": {\n"
  "    \"Position\": "); htmlPage += position[4]; htmlPage += F(",\n"
  "    \"Set_Pos\": ");  htmlPage += set_pos[4];  htmlPage += F(",\n"
  "    \"Ref_Set\": ");  htmlPage += refset[4];   htmlPage += F(",\n"
  "    \"max_mA\": \""); sprintf(buf, "%.1f", max_mA[4]); htmlPage += buf;   htmlPage += F("\"\n"
  "  }\n"

  "}\n");

  server.send(200, "text/plain", htmlPage);

} // webUI_status ()


/** @brief Handler for all NotFound requests (invalid URIs)
 */
void webUI_notFound () 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) 
  { 
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; 
  }
  server.send(404, "text/plain", message);

} // webUI_notFound ()

// *** private function bodies

/**
 End of File
 */