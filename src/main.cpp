#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9

// Constructor for 1.3" SH1106 I2C OLED
// Use U8G2_SH1106_128X64_NONAME_F_HW_I2C for 1.3" displays
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup(void) {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  u8g2.begin();
}

void loop(void) {
  u8g2.clearBuffer();					// clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr);	// choose a suitable font
  u8g2.drawStr(0,10,"Hello ESP32!");	// write something to the internal memory
  u8g2.drawStr(0,30,"1.3 inch OLED");
  u8g2.sendBuffer();					// transfer internal memory to the display
  delay(1000);
}

