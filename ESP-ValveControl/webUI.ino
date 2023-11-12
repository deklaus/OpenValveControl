/** @file  webUI.c
 *  @author Klaus Deutschkämer (https://github.com/deklaus)
 *
 *  @brief Functions for WLAN User Interface (UI)
 *  @todo 
 */
/*  Change Log:
 *  07.11.2023 v0.3
 *  - handleRoot() eliminated. Web-UI is now a separate file (index.html) and uploaded via LittleFS.
 *  26.09.2023 v0.12
 *  - First issue
 */

// *** data type, constant and macro definitions
// *** global variables
// *** private variables
// *** public function bodies


/** @brief Handler for MOVE. Arguments: <ESP_IP>/move?vz=[1..4]&set_pos=[0..100]&max_mA=[0.0 .. 100.0]
 */
void handleMove ()
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
      htmlPage = F("{\n"
      "\"move\": {\n"
      "  \"vz\": \"");  htmlPage += sel;  htmlPage += F("\",\n"
      "  \"set_pos\": \""); htmlPage += pos;  htmlPage += F("\",\n"
      "  \"max_mA\": \""); sprintf(buf, "%.1f", mA);  htmlPage += buf;  htmlPage += F("\"\n"
      "  }\n}");      
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

} // handleMove()


/** @brief Handler for HOME. Arguments: <ESP_IP>/home?vz=[1..4]&max_mA=[0.0 .. 100.0]
 */
void handleHome ()
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
      htmlPage = F("{\n"
      "\"home\": {\n"
      "  \"vz\": \"");  htmlPage += sel;  htmlPage += F("\",\n"
      "  \"max_mA\": \""); sprintf(buf, "%.1f", mA);  htmlPage += buf;  htmlPage += F("\"\n"
      "  }\n}");
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

} // handleHome ()


/** @brief Handler for INFO request.
 */
void handleInfo ()
{
  char  buf[128];
  String htmlPage;
  htmlPage.reserve(128);  // prevent ram fragmentation

  htmlPage = F(
  "{ \n"
  "\"ESP\": \"" ); htmlPage += ESPversion; htmlPage += F("\",\n"
  "\"PIC\": \"" ); htmlPage += PICversion; htmlPage += F("\",\n"
  "\"SSID\": \""); htmlPage += ssid;       htmlPage += F("\"\n" // no comma at end
  "}\n");

    //server.sendHeader("Access-Control-Allow-Origin","*"); 
    server.send(200, "text/plain", htmlPage);

} // handleInfo ()


/** @brief Handler for "SAVE" (@todo: POST). Arguments: <ESP_IP>/save?ssid=<string>&psk=<password>
 */
void handleSave ()
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
    strncpy(psk,  pwd.c_str(), sizeof(pwd));
    ssid[sizeof(pwd)-1] = '\0';

    flags.save = 1;   /** @todo set flag for new WLAN credentials (loop: save in Flash) */
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

} // handleSave ()


/** @brief Handler for STATUS request. \n
 *  Contains all readings and status' from PIC µC.
 *  May be extended, because data is parsed as JSON object (name:value).
 */
void handleStatus ()
{
  char  buf[128];
  String htmlPage;
  htmlPage.reserve(1000);  // prevent ram fragmentation

  flags.status = 1;   // set flag for status request  OBSOLETE - we do this every loop cycle

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

} // handleStatus ()


/** @brief Handler for all NotFound requests (invalid URIs)
 */
void handleNotFound () 
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

} // handleNotFound ()

// *** private function bodies

/**
 End of File
 */