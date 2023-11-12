/** OLED defines and initialization.
 *  @author Klaus Deutschkämer (https://github.com/deklaus)
 *
 *  This application uses the following I2C OLED:
 *  1,3" OLED display SH1106-OLED-I2C-128x64
 *                    ======================
 *  There are a "8x8" library and a "graphics" lib (preferred).
 *  Weblinks:
 *  https://github.com/olikraus/u8g2/wiki  U8g2 is a monochrome graphics library for embedded devices
 *  https://forum.arduino.cc/t/128x64-oled-with-u8g2-lib-drawbox-with-map-function/1126406   bar graph example
 *
 *  OLED Layout: 128x64 pixels, with 6 total rows we get 2 x 10 pixels (IP + Status) + 4 x 11 px (positions)
 *  X   Row
 *  0   0  SSID / IP Address
 * 10   1  STATUS (HOME:VZ1) & ERRORs
 * 20   2  VZ1 XXXX            32 %  align right
 * 31   3  VZ2 XXXXXXXXXXXX          if reference not set: no numeric position
 * 42   4  VZ3 X                0 %
 * 53   5  VZ4 XXXXXXXXXXXXXX 100 %
 *             ^-col 30..89-^ ^-96..127
 *
 * @note As an option, commands could show up on display page 2, automatically returning to page 1 after a short delay.
 *       This would allow debugging commands via OLED.
 */

/*  We choose the "graphics" library. If you prefer the "8x8" OLED lib you have to replace all graphics output 
 *  by the corresponding functions. You might consider using #ifdef as we did in some examples.
 */
#include <U8g2lib.h>
//#include <U8x8lib.h>

/** OLED Display
 *  ============ */
#ifdef U8G2LIB_HH
  // Rotation: U8G2_Rn = n * 90°cw or U8G2_MIRROR: No rotation, landscape, display is mirrored (v2.6.x)
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);
#elif U8X8LIB_HH
  U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);     /* 8x8 */
#endif

// Bottom Y-Positions (in pixels) of for 6 OLED rows, for use in u8g2.drawStr()
uint8_t PXrow[6] = { 9, 19, 30, 41, 52, 63 };


/** @brief init OLED 
 */
void OLED_init (void)
{
  char buf[24];

  #ifdef U8G2LIB_HH
    u8g2.begin();
    u8g2.enableUTF8Print();           // enable UTF8 support for the Arduino print()  
    u8g2.setFont(u8g2_font_6x12_mr);  // choose a suitable font

    u8g2.setDrawColor(1);
    snprintf(buf, 21, "ESP: %s", ESPversion);     
    u8g2.drawStr(0, PXrow[1], buf);
    u8g2.drawStr(0, PXrow[2], "VZ1");
    u8g2.drawStr(0, PXrow[3], "VZ2");
    u8g2.drawStr(0, PXrow[4], "VZ3");
    u8g2.drawStr(0, PXrow[5], "VZ4");
    u8g2.sendBuffer();
  #elif define U8X8LIB_HH
    u8x8.begin();
    /// @todo: add code as with g2lib
  #endif
} // OLED_init ()


/** @brief updates OLED row with text message
 */
void OLED_show (unsigned row, char *msg)
{
  #ifdef U8G2LIB_HH
    u8g2.setDrawColor(0);   // erase row content
    if (row > 0) u8g2.drawBox(0, PXrow[row-1] + 1, 128, PXrow[row] - PXrow[row-1]);
    else         u8g2.drawBox(0, 0, 128, PXrow[0]);
    u8g2.setDrawColor(1);
    u8g2.drawStr(0, PXrow[row], msg); // show message
    u8g2.sendBuffer();
  #elif define U8X8LIB_HH
    /// @todo: add code as with g2lib
  #endif

} // OLED_show ()


/** @brief update OLED status display:
 *         - hbars
 *         - numeric position (only if reference is set)
 *         - actual drive current 
 *  @todo: Add code as with g2lib
 */
void OLED_update_status (void)
{
  char        buf[32];
  u8g2_uint_t w;
  u8g2_uint_t w100 = u8g2.getStrWidth("100 %");

  for (int i = 1; i <= 4; i++)
  { // erase old numeric value
    u8g2.setDrawColor(0);
    u8g2.drawBox(128 - w100, PXrow[i] + 1, w, PXrow[i + 1]- PXrow[i]);

    // update hbar
    w = 60 * position[i] / 100;
    if (0 == w) w = 1;  // leave minimum 1 px as zero position
    u8g2.drawBox(26+w, PXrow[i] + 5, 90 - (26 + w), 6);
    u8g2.setDrawColor(1);
    u8g2.drawBox(25, PXrow[i] + 5, w, 6);

    // update numeric position
    if (status & (1 << i))
    {
      sprintf(buf, "%d %%", position[i]);
      w = u8g2.getStrWidth(buf);
      u8g2.drawStr(128-w, PXrow[i+1], buf);
    }
    u8g2.sendBuffer();
  }

  // update drive current
  w = u8g2.getStrWidth("99.9 mA");
  u8g2.setDrawColor(0);
  u8g2.drawBox(128-w, PXrow[0] + 1, w, PXrow[1] - PXrow[0]);
  u8g2.setDrawColor(1);
  sprintf(buf, "%.1f mA", mAmps);
  w = u8g2.getStrWidth(buf);
  u8g2.drawStr(128-w, PXrow[1], buf);
  u8g2.sendBuffer();

// update temperature
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, PXrow[0] + 1, 128 - w, PXrow[1] - PXrow[0]);
  u8g2.setDrawColor(1);
  sprintf(buf, "%.1f C", tempC);
  u8g2.drawStr(0, PXrow[1], buf);
  u8g2.sendBuffer();

} // OLED_update_status ()
