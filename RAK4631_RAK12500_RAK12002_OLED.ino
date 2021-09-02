/**
   @file RAK4631_RAK12500_RAK12002_OLED.ino
   @author rakwireless.com
   @brief use I2C get and parse gps data, including time
   @version 0.1
   @date 2021-9-1
   @copyright Copyright (c) 2021
**/

#include <Wire.h> // Needed for I2C to GNSS
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
// http://librarymanager/All#SparkFun_u-blox_GNSS
#include "SSD1306Ascii.h"
// http://librarymanager/All#SSD1306Ascii
#include "SSD1306AsciiWire.h"
#include <RV-3028-C7.h>

RV3028 rtc;
bool TimeAdjusted = false;

#define I2C_ADDRESS 0x3C
#define RST_PIN -1
#define OLED_FORMAT &Adafruit128x64
SSD1306AsciiWire oled;

SFE_UBLOX_GNSS g_myGNSS;
long g_lastTime = 0;
// Simple local timer. Limits amount of I2C traffic to u-blox module.

/**@brief Pretty-prints a buffer in hexadecimal, 16 bytes a line
          wth ASCII representation on the right àla hexdump -C
*/
void hexDump(uint8_t *buf, uint16_t len) {
  String s = "|", t = "| |";
  Serial.println(F("  |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f |"));
  Serial.println(F("  +------------------------------------------------+ +----------------+"));
  for (uint16_t i = 0; i < len; i += 16) {
    for (uint8_t j = 0; j < 16; j++) {
      if (i + j >= len) {
        s = s + "   "; t = t + " ";
      } else {
        char c = buf[i + j];
        if (c < 16) s = s + "0";
        s = s + String(c, HEX) + " ";
        if (c < 32 || c > 127) t = t + ".";
        else t = t + (String(c));
      }
    }
    uint8_t index = i / 16;
    Serial.print(index, HEX); Serial.write('.');
    Serial.println(s + t + "|");
    s = "|"; t = "| |";
  }
  Serial.println(F("  +------------------------------------------------+ +----------------+"));
}

void setup() {
  pinMode(WB_IO1, OUTPUT); // SLOT_A SLOT_B
  pinMode(WB_IO2, OUTPUT); // SLOT_A SLOT_B
  pinMode(WB_IO3, OUTPUT); // SLOT_C
  pinMode(WB_IO4, OUTPUT); // SLOT_C
  pinMode(WB_IO5, OUTPUT); // SLOT_D
  pinMode(WB_IO6, OUTPUT); // SLOT_D
  pinMode(WB_A0, OUTPUT); // IO_SLOT
  pinMode(WB_A1, OUTPUT); // IO_SLOT
  digitalWrite(WB_IO1, 0);
  digitalWrite(WB_IO2, 0);
  digitalWrite(WB_IO3, 0);
  digitalWrite(WB_IO4, 0);
  digitalWrite(WB_IO5, 0);
  digitalWrite(WB_IO6, 0);
  digitalWrite(WB_A0, 0);
  digitalWrite(WB_A1, 0);
  delay(1000);
  digitalWrite(WB_IO1, 1);
  digitalWrite(WB_IO2, 1);
  digitalWrite(WB_IO3, 1);
  digitalWrite(WB_IO4, 1);
  digitalWrite(WB_IO5, 1);
  digitalWrite(WB_IO6, 1);
  delay(1000);
  Wire.begin();
  oled.begin(OLED_FORMAT, I2C_ADDRESS);
  oled.setFont(System5x7);
  oled.setScrollMode(SCROLL_MODE_AUTO);
  oled.println("\n\n\nWe're live!");
  // Initialize Serial for debug output
  time_t timeout = millis();
  Serial.begin(115200);
  delay(2500);
  Serial.println("GPS ZOE-M8Q Example(I2C)");
  oled.println("GPS ZOE-M8Q (I2C)");
  if (g_myGNSS.begin() == false) {
    // Connect to the u-blox module using Wire port
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    oled.println(F("u-blox GNSS not detected.\nCheck wiring.\nFreezing."));
    while (1);
  }
  g_myGNSS.setI2COutput(COM_TYPE_UBX);
  //Set the I2C port to output UBX only (turn off NMEA noise)
  g_myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);
  //Save (only) the communications port settings to flash and BBR
  if (rtc.begin() == false) {
    Serial.println("Something went wrong, check wiring");
    oled.println("RTC: Something's wrong");
    while (1);
  } else {
    Serial.println("RTC online!");
    oled.println("RTC online!");
    rtc.set24Hour();
    char buff[16];
    memset(buff, 0, 16);
    strcpy(buff, "Kongduino");
    for (uint8_t i = 0; i < 8; i++) {
      rtc.writeConfigEEPROM_RAMmirror(i, buff[i]);
      delay(20);
    }
    memset(buff, 0, 16);
    for (uint8_t i = 0; i < 8; i++) {
      buff[i] = rtc.readConfigEEPROM_RAMmirror(i);
      delay(20);
    }
    Serial.println("buff:");
    hexDump((uint8_t *)buff, 16);
  }
}

void loop() {
  if (millis() - g_lastTime > 10000) {
    Serial.println("\nGetting GNSS data...");
    long latitude = g_myGNSS.getLatitude();
    long longitude = g_myGNSS.getLongitude();
    long altitude = g_myGNSS.getAltitude();
    long speed = g_myGNSS.getGroundSpeed();
    long heading = g_myGNSS.getHeading();
    byte SIV = g_myGNSS.getSIV();
    char buff[24];
    sprintf(buff, "SIV: %d", SIV);
    Serial.println(buff);
    if (SIV > 0) {
      // No point if there are no satellites in view
      oled.println(buff);
      sprintf(buff, "Lat: %3.7f", (latitude / 1e7));
      oled.println(buff);
      Serial.println(buff);
      sprintf(buff, "Long: %3.7f", (longitude / 1e7));
      oled.println(buff);
      Serial.println(buff);
      sprintf(buff, "Alt: %3.3f m", (altitude / 1e3));
      oled.println(buff);
      Serial.println(buff);
      sprintf(buff, "Speed: %3.3f m/s", (speed / 1e3));
      // oled.println(buff);
      Serial.println(buff);
      sprintf(buff, "Heading: %3.7f", (heading / 1e5));
      // oled.println(buff);
      Serial.println(buff);
    }
    if (g_myGNSS.getTimeValid()) {
      uint8_t hour = (g_myGNSS.getHour() + 8) % 24;
      // Adjust for HK Time
      // Warning: this only adjusts the time. Overflow will rotate back to 0
      // [this is 24-hour system remember] but it doesn't take care of the date.
      // which is fine here, but you might need to use a library to adjust properly.
      sprintf(buff, "gTime: %02d:%02d:%02d HKT", hour, g_myGNSS.getMinute(), g_myGNSS.getSecond());
      oled.println(buff);
      Serial.println(buff);
      if (!TimeAdjusted) {
        // if we haven't adjusted the time, let's do so.
        if (rtc.setTime(
              g_myGNSS.getSecond(), g_myGNSS.getMinute(), g_myGNSS.getHour(), g_myGNSS.getDay(),
              g_myGNSS.getTimeOfWeek(), g_myGNSS.getMonth(), g_myGNSS.getYear()) == false) {
          Serial.println("Something went wrong setting the time");
        }
        TimeAdjusted = true;
      } else {
        // Do we need to adjust the time?
        // Check for excessive difference between GNSS time and RTC time
        // rtc.updateTime() has to be called first!
        rtc.updateTime();
        if (rtc.getMinutes() != g_myGNSS.getMinute() || g_myGNSS.getHour() != rtc.getHours()) {
          Serial.println("Readjusting RTC");
          if (rtc.setTime(
                g_myGNSS.getSecond(), g_myGNSS.getMinute(), g_myGNSS.getHour(), g_myGNSS.getDay(),
                g_myGNSS.getTimeOfWeek(), g_myGNSS.getMonth(), g_myGNSS.getYear()) == false
             ) {
            Serial.println(" . Something went wrong setting the time");
            oled.println("RTC Time Error");
          }
        }
      }
      rtc.updateTime();
      sprintf(buff, "rTime: %02d:%02d:%02d UTC", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
      oled.println(buff);
      Serial.println(buff);
    }
    g_lastTime = millis(); //Update the timer
  }
}
