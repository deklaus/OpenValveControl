/** @file   ESPValveControl.ino
 *  @author  (c) Klaus Deutschkämer (https://github.com/deklaus)
 *  License: This software is licensed under the European Union Public Licence EUPL-1.2
 *           (see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12 for details).
 *
 *  @brief  WiFi-Interface for OpenValveControl (PIC-)Controller.
 *  \b Functions:
 *   - Display all 4 valve positions
 *   - Display motor current
 *   - Selection of a distribution zone (VZ)
 *   - Input of max. current for MOVE or HOME
 *   - Triggering a MOVE (VZ) to desired position
 *   - Triggering a reference run (HOME)
 *   - Temperature measurement with DS18B20
 *   - Status display, p.e. positions, firmware (ESP and PIC), WiFi,...
 *   - Access point (until local WiFi has been configured)
 *   - OTA Firmware-Update ESP
 *   - Firmware-Update (PIC) via Web UI (planned)
 *
 *  \b IDE: <c> Arduino IDE 2.2.1 &rarr; <c> esptool.py v3.0 \n
 *  \b Device: \n
 *     - <c> LOLIN(WEMOS) D1  mini (clone) </c> (current state)
 *     - <c> <b> WEMOS D1 mini ESP32 <\b></c>   (optional) \n
 *  \b Libs: \n
 *     - <c> ESP8266WiFi       Version 1.0     path: ~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/ESP8266WiFi </c>
 *     - <c> ESP8266WebServer  Version 1.0     path: ~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/ESP8266WebServer </c>
 *     - <c> LittleFS          Version 0.1.0   path: ~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/LittleFS </c>
 *     - <c> DallasTemperature Version 3.9.0   path: ~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/DallasTemperature </c>
 *     - <c> MAX31850 OneWire  Version 1.1.1   path: ~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/MAX31850_OneWire </c>
 *     - <c> U8g2              Version 2.34.22 path: ~/Arduino/libraries/U8g2 </c> or
 *     - <c> SPI               Version 1.0     path: ~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/SPI </c>
 *     - <c> Wire              Version 1.0     path: ~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/Wire </c>
 *  \n
 *  Weblinks: \n
 *  Doxygen:    https://www.doxygen.nl/manual/docblocks.html \n
 * 
 * History:
 * v0.6 2023-11-15
 * - Added acces point mode (OVC-access-point/PVC-password).
 * - Reading and writing of setup file "ovc.ini". LittleFS.ini supplemented with proper mime/ContentType.
 * v0.5 Test version
 * v0.4 2023-11-12
 * - Added OTA support. Enter URI/update to select and upload new firmware binary file (.bin).
 * v0.3 2023-11-07
 * - Added LittleFS. Web-UI must now be uploaded as separate file (index.html) via LittleFS (URI/fs.html)
 *   Requires Web-UI (index.html) v0.3 or newer.
 * v0.2 2023-11-02
 * - Added reading of DS18B20 and added temperature in handle_status and handle-info.
 *
 * @todo - poor handling of refset[] (maybe by PIC?). 
 *       - home drive sometimes doesn't reset position.
 *       - Download of ovc.ini should be password protected or PSK should be encrypted in ovc.ini.
 */

// *** includes
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>   
#include <LittleFS.h>

// *** function prototypes
extern void   handleHome (void);
extern void   handleInfo (void);
extern void   handleMove (void);
extern void   handleNotFound (void);
//extern void   handleRoot (void);
extern void   handleSave (void);
extern void   handleStatus (void);

extern void   OLED_init (void);
extern void   OLED_show (unsigned row, char *s);
extern void   OLED_update_status (void);

extern void   DS18B20_init (void);
extern float  DS18B20_TempC (uint8_t index);

extern void   setupFS ();

extern int    setup_GetCstring (const char *path, const char *identifier, char *result, int Nmax);
extern int    setup_GetInt (const char *path, const char *identifier, int *result);
extern int    setup_GetFloat (const char *path, const char *identifier, float *result);
extern int    setup_ReadINI (const char *path);
extern int    setup_WriteCstring (const char *path, const char *identifier, char *s);

// *** private function prototypes
static int    cmd2pic (void);

// *** data type, constant and macro definitions
//#define DEBUG_OUTPUT_DS1820   1   /* enable serial monitor: status DS18B20 */
//#define DEBUG_OUTPUT_STATUS   1   /* enable serial monitor: status read from PIC */
//#define DEBUG_OUTPUT_WIFI     1   /* enable serial monitor: WIFI status after connect */
//#define DEBUG_OUTPUT_INI      1   /* enable serial monitor: INI file functions */

#define numVZ   4             // no. of available Valve Zones / motors

#define CYCLE_TIME    500   /* cycle time of main loop */
#define MAX_ACK_TIME  200   /* timeout in milliseconds for command acknowledge from PIC */

// bitfields are not really the optimum - a boolean flag_move, flag_home, ... might be the better choice!?
struct FLAGS  //!<   flags to start tasks within loop()
{
  uint8_t move   :1;  //!< 0 exec move
  uint8_t home   :1;  //!< 1 exec home
  uint8_t status :1;  //!< 2 exec status
  uint8_t save   :1;  //!< 3 exec save
  uint8_t        :1;  //!< 4 
  uint8_t        :1;  //!< 5 
  uint8_t        :1;  //!< 6 
  uint8_t        :1;  //!< 7 
}; 

/* uint16_t status word (read from PIC)
 * Bit definitions/macros are more safe than structs, when using different compilers)
 */
#define REF(x)    (x & 0x000F)
#define REF1(x)   (x & 0x0001)
#define REF2(x)   (x & 0x0002)
#define REF3(x)   (x & 0x0004)
#define REF4(x)   (x & 0x0008)
#define VZ(x)    ((x & 0x00F0) >> 4)    /* 'vz' of active command as bit pattern: which motor is moving/homing? */
#define VZ1(x)   ((x & 0x0010) >> 4)    /* 1: vz1 is under process */
#define VZ2(x)   ((x & 0x0020) >> 5)    /* 1: vz2 is under process */
#define VZ3(x)   ((x & 0x0040) >> 6)    /* 1: vz3 is under process */
#define VZ4(x)   ((x & 0x0080) >> 7)    /* 1: vz4 is under process */
#define MOVE(x)  ((x & 0x0100)          /* 'move' is executing */
#define HOME(x)  ((x & 0x0200)          /* 'home' is executing */

/** LIBRARY INSTANCES */

/** Webserver
 *  ========= */
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

/** Global variables
 *  =============== */
char  ESPversion[32] = "v0.6";  // Version of ESP-Firmware
char  PICversion[32] = "NN";    // Version of PIC-Firmware

// WiFi credentials: Initial values are used for access point!
// The credential aof your routers WiFi can be customized in "ovc.ini" (uploaded into LittleFS)
char  ssid[64] = "OVC-access-point";  // SSID (access point)
char  psk[64]  = "OVC-password";      // Password (access point)

char  alias1[8];    // aliases for "VZ1" to "VZ4"
char  alias2[8];
char  alias3[8];
char  alias4[8];

// Vars sourced by PIC µC
uint16_t  status;         // status word from PIC
float     mAmps = 0.0;    // actual current [mA]
float     tempC = 0.0;    // temperature in Celsius (DS18B20)
float     dTemp = 0.0;    // temperature adjust (ovc.ini)
int       position[numVZ + 1] = {-1, 0, 65, 36, 100 };        // actual VZ positions (index 0 is dummy)
bool      refset[numVZ + 1];                                  // home position set?

// Vars sourced by (html) User Interface 
struct FLAGS  flags;      // processing flags (Web UI -> loop)
int       vz = 0;         // selected valve zone (motor), [1 .. 4], 0 = none!
int       set_pos[numVZ + 1]  = { -1, 0, 65, 36, 100 };       // valve set positions
float     max_mA[numVZ + 1]   = { 0.0, 25.0, 25.0, 25.0, 25.0 };  // motor current limits [mA]

char      txbuf[64];
char      rxbuf[64];
int       nrx;        // number of received chars in rxbuf

unsigned long TimeStamp = 0;        /* used for general delay purposes (local)  */


/** @brief ESP8266 Initialization
 *  @todo Implement access point (AP) mode to enter WiFi Credentials.
 */
void setup ()
{
  int    error;
  String param;

  /** - Initialize Serial (UART0) to 38400 Bd. \n
   *    Serial uses UART0 which is mapped to pins GPIO1 (TX) and GPIO3 (RX).  
   *    For communication with the PIC µC, we remap UART0 to GPIO15 (TX) and GPIO13 (RX) by Serial.swap(). 
   *    Calling swap again maps UART0 back to GPIO1 and GPIO3. (Serial1/UART1 can not be used to receive).
   *    Weblink: https://esp8266-arduino.readthedocs.io/en/latest/reference.html "Serial".
   */
  Serial.begin(38400);
  Serial.swap();  // disconnect serial console on ESP8266 and swap connection to PIC µC

  /** - Initialize OLED Status Display \n
   *    Show local IP address and initial valve positions.
   */
  OLED_init();

#ifdef SHOW_SKETCH_SPACE
// TEST: show sketch size and free sketch space in OLED
  float sketchSize = (float)ESP.getSketchSize();
  float freeSketchSpace = (float)ESP.getFreeSketchSpace();
  snprintf(txbuf, 21, "size: %.0f", sketchSize); 
  OLED_show(0, txbuf);
  snprintf(txbuf, 21, "free: %.0f", freeSketchSpace); 
  OLED_show(1, txbuf);
#endif

  /** - Initialize temperature sensor DS18B20 */
  DS18B20_init();

  /** - Setup Little FileSystem
   *    Also configures the server for filesystem operations (format, upload, ...) */
  setupFS();

  /* Read setup data from LittleFS.
   * Weblinks: https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
   *           https://randomnerdtutorials.com/esp32-write-data-littlefs-arduino/#esp32-save-variable-littlefs
   * Also reads the WiFi credentials (if uploaded to /ovc.ini)   */
  error = setup_ReadINI("/ovc.ini");

  /** - Initialize WiFi  
        Weblink: https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-class.html
   */
  OLED_show(0, ssid);
  if (0 == strcmp(ssid, "OVC-access-point"))  // default ssid (no setup by ovc.ini)
  { // create access point
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP(ssid, psk)) OLED_show(1, psk);     // AP setup okay
    else                        OLED_show(1, (char *)"Error");
  }
  else
  { // try: connect to router with credentials from ovc.ini
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, psk);
    for (int timeout = 0; (WiFi.status() != WL_CONNECTED) &&  (timeout < 120); timeout++)
    { // Wait for WiFi connection, cancel after 2 minutes.
      OLED_show(0, (char *)" ");
      delay(500);
      OLED_show(0, ssid);
      delay(500);
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    // Show local IP address on OLED, so we can connect to the webserver UI
    // (one OLED row is max. 21 chars wide)
    snprintf(txbuf, 21, "IP: %s", WiFi.localIP().toString().c_str()); 
    OLED_show(0, txbuf);
  }
  else { // if connection failed, use accesspoint to make corrections
    strcpy(ssid, "OVC-access-point"); // reset
    strcpy(psk,  "OVC-password");
    WiFi.mode(WIFI_AP);
    OLED_show(0, ssid);
    if (WiFi.softAP(ssid, psk)) OLED_show(1, psk);     // AP setup okay
    else                        OLED_show(1, (char *)"Error");
  }

  if (WiFi.getMode() == WIFI_AP)
  { // default IP address of AP is 192.168.4.1 and may be changed using softAPConfig, see:
    // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/soft-access-point-class.html#softapip
    delay(5000);  // 5 sec for reading the credentials, then show IP
    snprintf(txbuf, 21, "IP: %s", WiFi.softAPIP().toString().c_str()); 
    OLED_show(0, txbuf);
  }

#ifdef DEBUG_OUTPUT_WIFI
  Serial.flush();   // Wait for the transmission of outgoing serial data to complete
  Serial.swap();    // map UART0 to serial monitor
  Serial.println("");
  Serial.print("Connected:  ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("PSK: ");
  Serial.println(psk);      // security risc!
  //Serial.println("****"); // optional
  Serial.flush();   // Waits for the transmission of outgoing serial data to complete
  Serial.swap();    // remap output to PIC
#endif

  /** - Read PIC Firmware Version and save in 'PICversion'
   */
  sprintf(txbuf, "Version?\n");
  error = cmd2pic();
  if (!error && (strncmp(rxbuf, "Version:", 8) == 0))  // compare first N chars of response with command
  { 
    strncpy(PICversion, &rxbuf[9], sizeof(PICversion));
    PICversion[sizeof(PICversion) - 1] = '\0';
  }

  /** - Setup Webserver for ESP ValveControl
   *  This implements our client request handlers. You can append the GET commands and parameters to the URI, e.g.
   *  http://192.168.2.75/move?vz=1&set_pos=25&max_mA=30   move selected valve to 25 % position.
   *  http://192.168.2.75/home?vz=3&max_mA=45              request homing of the selected valve.
   *  http://192.168.2.75/status                           get status information (current, temperature, valve states)
   *  http://192.168.2.75/info                             get system information (Firmware releases, WiFi SSID)
   */
// server.on("/", handleRoot);  // handled by LittleFS -> invokes /index.html (our Web UI)
  server.on("/move",   HTTP_GET, handleMove);
  server.on("/home",   HTTP_GET, handleHome);
  server.on("/info",   HTTP_GET, handleInfo);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/save",   HTTP_GET, handleSave);
  //server.on("/format");  // handled by LittleFS
  //server.on("/upload");  // handled by LittleFS
  //server.onNotFound(handleNotFound);  // already handled by LittleFS

  httpUpdater.setup(&server);
  server.begin();

} // setup()



/** @brief  ESP8266 main loop
 *  - Process request handler and execute commands from UI
 *  - Periodically read status from PIC (valve controller)
 *  - Update OLED status display
*/
void loop ()
{
  unsigned long LoopStamp = millis();   // used to run main loop with constant execution time
  int       error;
  uint16_t  uval;
  char      buf[64];
  char      *p;

  if (flags.move)       // MOVE command?
  {
   /* Send MOVE command regardless of reference points and other conditions - errors must be handled by PIC.
    * Due to long execution time, the PIC µC acknowledges only reception of the command.
    */    
    sprintf(txbuf, "Move:%d,%d,%d\n", vz, set_pos[vz], (uint16_t)(10 * max_mA[vz]));      // Move:vz:set_pos[vz]:10 x max_mA[vz]
    OLED_show(1, txbuf);    // optional show on OLED.row 1 (0..5)
    error = cmd2pic();
    // check if response contains command token ("Move"), else it is an error reponse
    if (strncmp(rxbuf, "Move:", 5) != 0)  // compare first N chars of response with command
    { /** @todo optional error handler, response might contain error number */
      error = -2; 
    }
    if (!error) flags.move = 0;  // PIC has acknowledged
  } // if flags.move

  else if (flags.home)       // HOME command?
  {
   /* Send HOME command regardless of reference points and other conditions - errors must be handled by PIC.
    * Due to long execution time, the PIC µC acknowledges only reception of the command.
    */    
    sprintf(txbuf, "Home:%d,%d\n", vz, (uint16_t)(10 * max_mA[vz]));      // Home:vz:10 x max_mA[vz]
    OLED_show(1, txbuf);    // optional show on OLED.row 1 (0..5)
    error = cmd2pic();
    if (strncmp(rxbuf, "Home:", 5) != 0)  // compare first N chars of response with command
    { /** @todo optional error handler, response might contain error number */
      error = -3; 
    }
    if (!error) flags.home = 0;  // PIC has acknowledged
  }

  else if (flags.save)       // SAVE & REBOOT?
  {
    flags.save = 0;
    setup_WriteCstring((char *)"/ovc.ini", "SSID", ssid);
    setup_WriteCstring((char *)"/ovc.ini", "PSK",  psk);

    // reboot
    OLED_show(0, (char *)"reBOOT-please wait..");
    server.send(200, "text/html", "Restart_Ok");
    delay(500);
    ESP.restart();
  }

  /* Repeatedly process request handler until mid of loop cycle   */
  do
  {
    server.handleClient();  // mandatory
  } while ((millis() - LoopStamp) < CYCLE_TIME/2);

  /* Read status from PIC, result format: "Status:Pos1,Pos2,Pos3,Pos4,mAx10,0xstatus"  */
  sprintf(txbuf, "Status?\n");
  error = cmd2pic();
  // check if response contains command token ("Status:"), else it is an error reponse
  if (strncmp(rxbuf, "Status:", 7) != 0)  // compare first N chars of response with command
  { /// @todo optional error handler, response might contain error number
    error = -4; 
  }

  /* Process status or error message  */
  if (error)
  {
    /// @todo add appropriate error handler
#ifdef DEBUG_OUTPUT_STATUS
    Serial.flush();   // Waits for the transmission of outgoing serial data to complete
    Serial.swap();    // output to serial monitor
    Serial.println(rxbuf);
    Serial.flush();   // Waits for the transmission of outgoing serial data to complete
                      // (prior to Arduino 1.0, this instead removed any buffered incoming serial data)
    Serial.swap();    // output to PIC µC
#endif
  }
  else
  { // find separator, then convert values
    p = strstr(rxbuf, ":");
    for (int i = 1; i <= 4; i++)
    {
      if (p)    // positions[1..4]
      {
        uval = atoi(++p);
        if (uval <= 100) position[i] = uval;
        p = strstr(p, ",");
      }
    }
    if (p) {    // mAmps
      uval = atoi(++p);
      if (uval <= 500) mAmps = (float) uval / 10.;
      p = strstr(p, ",");
    }
    if (p)      // status word
    {
      if (sscanf(++p, "0x%04x", &uval) == 1) 
      {
        status = uval;
      } 
    }

#ifdef DEBUG_OUTPUT_STATUS
      Serial.flush();   // Waits for the transmission of outgoing serial data to complete
      Serial.swap();    // output to serial monitor
      Serial.print("Positions: ");
      for (int i=1; i <= 4; i++) {    Serial.print(position[i]); Serial.print(","); }
      sprintf(buf, "%.1f,", mAmps);   Serial.print(buf);
      sprintf(buf, "0x%04X", status); Serial.println(buf);
      Serial.flush();   // Waits for the transmission of outgoing serial data to complete
                        // (prior to Arduino 1.0, this instead removed any buffered incoming serial data)
      Serial.swap();    // output to PIC µC
#endif

    /* read temperature sensor (index 0), usually the heating flow temperature.  */
    tempC = DS18B20_TempC(0) + dTemp;

    /* update OLED position display */
    OLED_update_status();
  }

  /* Repeatedly process request handler until end of loop cycle */
  do  
  {
    server.handleClient();  // mandatory
  } while ((millis() - LoopStamp) < CYCLE_TIME);

} // loop()



/** @brief  This function sends the command string in txbuf[] via UART to the PIC µC 
 *          and waits for a response or timeout. 
 *          The response is returned in global rxbuf[].
 *  @return int error  0: no error
 *                    -1: timeout
*/
int cmd2pic (void)
{
  unsigned long tstart;
  int   error = 0;
  bool  eol;
  char  c;

  // flush response buffer 
  while (Serial.available() > 0) Serial.read();   // clean up serial input
  eol = 0;
  nrx = 0;
  rxbuf[0] = '\0';  // flush rxbuf

  Serial.println(txbuf);  // transmit query to PIC µC
  tstart = millis();

  // wait for eol := command ack (reception only, not yet execution)
  for (error = 0; !eol && !error;  )
  {
    while (Serial.available() > 0 && !eol) 
    {
      c = Serial.read();
      if (c == '\r') continue;  // skip
      if (c == '\n') 
      {
        if (nrx < 2)  continue;
        eol = true;
        c = '\0';
      }
      if (nrx < sizeof(rxbuf))  // if not buffer overflow
      {
        rxbuf[nrx++] = c;       // store received char in rxbuf, increment count
      }
      else rxbuf[sizeof(rxbuf) - 1] = '\0'; // terminate rxbuf
    } // while
    if ((millis() - tstart) > MAX_ACK_TIME) 
    {
      error = -1;  // timeout?
    }
    server.handleClient();      // process WebUI during wait
  } // for

  if (eol)  // we got a response
  { 
    /** @todo response could contain an error mesg - how to handle this?)
      TEST:
      Serial.flush();   // Waits for the transmission of outgoing serial data to complete
      Serial.swap();    // output to serial monitor
      Serial.println(rxbuf);
      Serial.flush();   // Waits for the transmission of outgoing serial data to complete
      Serial.swap();    // output to PIC µC
    */
  } // if
  else  // no response (timeout)
  { 
    /** @todo show err mesg in LCD (and WebUI? / status?) */
  } // else

  return (error);

} // cmd2pic

