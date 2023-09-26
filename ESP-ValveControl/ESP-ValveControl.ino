/** @file   ESPValveControl.ino
 *  @author Klaus Deutschkämer (https://github.com/deklaus)
 *
 *  @brief  WLAN-Interface for OpenValveControl (PIC-)Controller.
 *  \b Functions:
 *   - Display all 4 valve positions
 *   - Display motor current
 *   - Selection of a distribution zone (VZ)
 *   - Input of max. current for MOVE or HOME
 *   - Triggering a MOVE (VZ) to desired position
 *   - Triggering a learning run (HOME)
 *   - Status display, p.e. positions, firmware (ESP and PIC), WLAN,...
 *   -
 *   - OTA Firmware-Update ESP (planned)
 *   - Firmware-Update (PIC) via Web UI (planned)
 *
 *  \b IDE: <c> Arduino IDE 2.1.0 &rarr; <c> esptool.py v3.0 \n
 *  \b Device: \n
 *     - <c> LOLIN(WEMOS) D1  mini (clone) </c> (current state)
 *     - <c> <b> WEMOS D1 mini ESP32 <\b></c>   (optional) \n
 *  \b Libs: \n
 *     - <c> ESP8266WiFi      Version 1.0     path: ~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/ESP8266WiFi </c>
 *     - <c> ESP8266WebServer Version 1.0     path: ~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/ESP8266WebServer </c>
 *     - <c> U8g2             Version 2.34.22 path: ~/Arduino/libraries/U8g2 </c> or
 *     - <c> SPI              Version 1.0     path: ~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/SPI </c>
 *  \n
 *  Weblinks: \n
 *  Doxygen:    https://www.doxygen.nl/manual/docblocks.html \n
 * 
*/
/**
 * @todo 
*/

// *** includes
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// *** function prototypes
extern void   handleHome (void);
extern void   handleInfo (void);
extern void   handleMove (void);
extern void   handleNotFound (void);
extern void   handleRoot (void);
extern void   handleSave (void);
extern void   handleStatus (void);

extern void   OLED_init (void);
extern void   OLED_show (unsigned row, char *s);
extern void   OLED_update_status (void);

// *** private function prototypes
static int    cmd2pic (void);

// *** data type, constant and macro definitions
//#define DEBUG_OUTPUT_STATUS   1   /* enable serial monitor: status read from PIC */
//#define DEBUG_OUTPUT_WIFI     1   /* enable serial monitor: WIFI status after connect */

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

/** Global variables
 *  =============== */
char  ESPversion[32] = "Version 0.12";    // Version of ESP-Firmware
char  PICversion[32] = "NN";              // Version of PIC-Firmware

// WLAN credentials (customize to your settings)
char  ssid[64] = "Your Router's SSID";
char  psk[64] = "Your Router's Password";

// vars sourced by PIC µC
uint16_t  status;         // status word from PIC
float     mAmps = 0.0;    // actual current [mA]
int       position[numVZ + 1] = {-1, 0, 65, 36, 100 };        // actual VZ positions (index 0 is dummy)
bool      refset[numVZ + 1];                                  // home position set?

// vars sourced by (html) User Interface 
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
void setup()
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

  /** - Initialize WiFi 
   */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, psk);
  while (WiFi.status() != WL_CONNECTED)
  { // Wait for WiFi connection
    delay(500);
    /// @todo show something (hour glass) on OLED
  }
  // Show local IP address on OLED, so we can connect to the webserver UI
  // (one OLED row is max. 21 chars wide)
  snprintf(txbuf, 21, "IP: %s", WiFi.localIP().toString().c_str()); 
  OLED_show(0, txbuf);

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

  /** - Read PIC Firmware Version and safe in 'PICversion'
   */
  sprintf(txbuf, "Version?\n");
  error = cmd2pic();
  if (!error && (strncmp(rxbuf, "Version:", 8) == 0))  // compare first N chars of response with command
  { 
    strncpy(PICversion, &rxbuf[9], sizeof(PICversion));
    PICversion[sizeof(PICversion) - 1] = '\0';
  }

  /** - Setup Webserver 
   *  This implements our client request handlers. You can append the GET commands and parameters to the URI, e.g.
   *  http://192.168.2.108/move?vz=1&set_pos=25&max_mA=30  as a move command or to read positions:
   *  http://192.168.2.108/status?
   */
  server.on("/", handleRoot);
  server.on("/move",   HTTP_GET, handleMove);
  server.on("/home",   HTTP_GET, handleHome);
  server.on("/info",   HTTP_GET, handleInfo);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/save",   HTTP_GET, handleSave);
  server.onNotFound(handleNotFound);
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

  /* Repeatedly process request handler until mid of loop cycle 
   */
  do
  {
    server.handleClient();  // mandatory
  } while ((millis() - LoopStamp) < CYCLE_TIME/2);


  /* Read status from PIC, result format: "Status:Pos1,Pos2,Pos3,Pos4,mAx10,0xstatus" 
   */
  sprintf(txbuf, "Status?\n");
  error = cmd2pic();
  // check if response contains command token ("Status:"), else it is an error reponse
  if (strncmp(rxbuf, "Status:", 7) != 0)  // compare first N chars of response with command
  { /// @todo optional error handler, response might contain error number
    error = -4; 
  }

  /* Process status or error message
   */
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

    /* update OLED position display
     */
    OLED_update_status();
  }


  /* Repeatedly process request handler until end of loop cycle 
   */
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
/*
      Serial.flush();   // Waits for the transmission of outgoing serial data to complete
      Serial.swap();    // output to serial monitor
      Serial.print("millis - tstart = ");
      Serial.println(millis() - tstart);
      Serial.flush();   // Waits for the transmission of outgoing serial data to complete
                        // (prior to Arduino 1.0, this instead removed any buffered incoming serial data)
      Serial.swap();    // output to PIC µC
*/
      error = -1;  // timeout?
    }
    server.handleClient();      // process WebUI during wait
  } // for

  /** @todo Ausgaben müssen in Funktionen ausgelagert werden, Status- und Fehlermeldungen auf OLED festlegen */
  if (eol)  // we got a response
  { 
/*
      Serial.flush();   // Waits for the transmission of outgoing serial data to complete
      Serial.swap();    // output to serial monitor
      Serial.println(rxbuf);
      Serial.flush();   // Waits for the transmission of outgoing serial data to complete
                        // (prior to Arduino 1.0, this instead removed any buffered incoming serial data)
      Serial.swap();    // output to PIC µC
*/
  } // if
  else  // no response
  { /** @todo show err mesg in LCD (and WebUI? / status?) */

  } // else

  return (error);

} // cmd2pic





/** @brief  This function helps you reconnect wifi as well as broker if connection gets disconnected.
*
void reconnect()
{
  while (!MQTTclient.connected())
  {
    status = WiFi.status();
    if ( status != WL_CONNECTED) 
    {
      WiFi.begin(ssid, psk);
      while (WiFi.status() != WL_CONNECTED) 
      {
        delay(500);
        //Serial.print(".");
      }
      //Serial.println("Connected to AP");
    }
    
    if(!MQTTclient.connected())
    {
      //Serial.print("Connecting to Broker ");
      //Serial.print(mqtt_broker);
      MQTTclient.connect("ESP-Control"); 
      MQTTclient.subscribe(token_cmd);
    }
    else 
    {
      //Serial.println( " : retrying in 5 seconds]" );
      delay(5000);
    }
  } // while
} // reconnect()
*/


/** @brief MQTT callback
    @return
    @NOTE: PAYLOAD IS NOT NULL TERMINATED - DON'T READ PAST LENGTH PARAMETER
*
void callback (char* topic, byte* payload, unsigned int length)
{
  heater = true;   // for safety this is the default case
                   // so that the software switches off on any errors. 

  if (strncmp("off", (char *)payload, length) == 0)  // strings are equal
  {
    heater = false;
  }

} // MQTT callback
 */


 /** @brief  Liest eine bestimmte (Parameter-)Datei und gibt den String 
 *          als Returnwert zurück.
*/
/*
String readFile (fs::FS &fs, const char * path)
{
//  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
//    Serial.println("- empty file or failed to open file");
    return String();
  }
  String fileContent;
  while (file.available())
  {
    fileContent += String((char)file.read());
  }
//  Serial.println(fileContent);
  
  return fileContent;
  
} // readFile
*/


/** @brief  Schreibt einen Wert in eine bestimmte (Parameter-)Datei.
*/
/*
void writeFile (fs::FS &fs, const char * path, const char * message)
{
//  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file)
  {
//    Serial.println("- failed to open file for writing");
    return;
  }

  if (file.print(message)) 
  {
//    Serial.println("- file written");
  } 
  else {
//    Serial.println("- write failed");
  }

} // writeFile()
*/

 