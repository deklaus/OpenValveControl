/** @file  fw.ino
 *  @author  (c) Klaus Deutschkämer (https://github.com/deklaus)
 *  License: This software is licensed under the European Union Public Licence EUPL-1.2
 *           (see https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12 for details).
 *
 *  Change Log:
 *  2023-11-23 v0.6
 *  - First issue
 *
 *  Weblinks: 
 *  https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
 *  https://www.arduino.cc/reference/en/language/variables/data-types/stringobject/
 */ 

// *** data type, constant and macro definitions
// *** global variables
// *** private variables
// *** public function bodies

/*  This struct describes the INTEL hex format. Notice, that this is the ASCII notation, 
    for the binary format we have to convert 2 ASCII chars to 1 hex Byte!

    struct
    {
      uint8_t start;      // ':' start code of INTEL Hex records
      uint8_t length[2];  // number of data bytes in this record
      uint8_t address[4];
      uint8_t type[2];    // record type: 
                          // 00: Data record              01: End of File,  
                          // 02: Extended Segment Address 03: Start Segment Address (CS:IP)
                          // 04: Extended Linear Address  05: Start Linear Address
      uint8_t data[16];   // max no. of data bytes: 16 (mostly, may vary)
      uint8_t cs;         // checksum 
      uint8_t eor[n];     // end of record/end of line (CRLF + '\0'), set as required.
    } INTEL_record_t;
*/

/**  @brief This function invokes the PIC bootloader and then downloads a hexfile with 
 *          INTEL Hex records to the PIC µC.
 */
int fw_download (void)
{
    int    error = 0;
    int    type;   // record type
    String str;
    File   hexfile;

    hexfile = LittleFS.open(hexfilename, "r");
    if(hexfile)
    {
      // acitvate PIC bootloader
      strcpy(txbuf, "Bootload!");
      if (error = cmd2pic())
      {
        sprintf(txbuf, "Error %d", error);
        OLED_show(1, txbuf);
        error = 0;  // proceed (e.g. to terminate a bootloader with a single EOF record)
      }
      delay(2000);  // allow PIC to reboot (and to read OLED)

      for (error = 0; hexfile.available() && !error; )
      { 
        str = hexfile.readStringUntil('\n');    // read line, discard terminator

        /* perform some syntax check(s) */        
        if (str.length() < 11)
        {  // minimum bytecount in INTEL record: 12 (including terminator)
          error = -1;
          OLED_show(1, (char *)"Err: Record length");
          break;
        }
        if (!str.startsWith(":"))     
        {
          error = -2;
          OLED_show(1, (char *)"Err: No Intel record");
          break;
        }

        if (str.substring(7, 9) == "01")  // end index is exclusiv!
        { // EOF record terminates transmission
          OLED_show(1, (char *)"EOF record");
        }

        // txmit record to PIC
        str.toCharArray(txbuf, sizeof(txbuf));  // send to PIC
        if (error = cmd2pic())
        {
          sprintf(txbuf, "PIC error %d", error);
          OLED_show(1, txbuf);
          delay(2000);  // delay to read OLED
//          break;
        }
        else
        {
          /** @todo compare checksum, 
           *  show response (address) in OLED 
           */
          str.toCharArray(txbuf, 8);   // truncate: :NNAAAA (OLED row up to 21 chars)
          OLED_show(1, txbuf);
        }
        server.handleClient();  // mandatory
      } // for
      hexfile.close();

      /* after completion show success message ("EOF record")
         add 2 s delay, to not override with temperature */
      delay(2000);
      flags.version = 1;  // try to read (new) PIC firmware version

    } // if file open

    else
    {
      error = -1;
      OLED_show(1, (char *)"Cannot open Hexfile!");
    }    
 
    return(error);
   
} // fw_download()
