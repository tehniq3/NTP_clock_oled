// https://www.hackster.io/alankrantas/esp8266-ntp-clock-on-ssd1306-oled-arduino-ide-35116e
// small changes by Nicu FLORICA (niq_ro)
// v.0 - changed date mode (day.mounth not month/day)
// v.1 - added hardware switch for DST (Daylight Saving Time)

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <U8x8lib.h> // https://github.com/olikraus/u8g2/wiki/u8g2install

#define DSTpin 14 // GPIO14 = D5, see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

const char *ssid = "niq_ro"; // your wifi name and password
const char *pw   = "bererecepecaldura";
const long timezoneOffset = 2 * 60 * 60; // ? hours * 60 * 60

const char          *ntpServer  = "pool.ntp.org"; // change it to local NTP server if needed
const unsigned long updateDelay = 900000;         // update time every 15 min
const unsigned long retryDelay  = 5000;           // retry 5 sec later if time query failed
//const String        weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};  // english 
const String        weekDays[7] = {"Dum ", "Luni", "Mar ", "Mie ", "Joi ", "Vin ", "Sam "};  // lb. romana

unsigned long lastUpdatedTime = updateDelay * -1;
unsigned int  second_prev = 0;
bool          colon_switch = false;

ESP8266WiFiMulti WiFiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);

// Display constructors: https://github.com/olikraus/u8g2/wiki/u8x8setupcpp
// Fonts: https://github.com/olikraus/u8g2/wiki/u8x8reference
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(SCL, SDA, U8X8_PIN_NONE);

byte DST = 0;
byte DST0;
bool updated;

void setup() {
  pinMode (DSTpin, INPUT);
  
  Serial.begin(9600);
  u8x8.begin();
  u8x8.setFont(u8x8_font_7x14B_1x2_f);
  u8x8.drawString(0, 0, "Connecting");
  u8x8.drawString(0, 3, "  to WiFi...");

  Serial.println("Connecting to WiFi...");
  WiFiMulti.addAP(ssid, pw); // multiple ssid/pw can be added
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nConnected.");

  u8x8.clear();

  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;
  timeClient.setTimeOffset(timezoneOffset + DST*3600);
  timeClient.begin();
  DST0 = DST;
}

void loop() {
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;

  if (WiFiMulti.run() == WL_CONNECTED && millis() - lastUpdatedTime >= updateDelay) {
    updated = timeClient.update();
    if (updated) {
      Serial.println("NTP time updated.");
      lastUpdatedTime = millis();
    } else {
      Serial.println("Failed to update time. Waiting for retry...");
      lastUpdatedTime = millis() - updateDelay + retryDelay;
    }
  } else {
    if (WiFiMulti.run() != WL_CONNECTED) Serial.println("WiFi disconnected!");
  }

  unsigned long t = millis();

  unsigned int year = getYear();
  unsigned int month = getMonth();
  unsigned int day = getDate();
  unsigned int hour = timeClient.getHours();
  unsigned int minute = timeClient.getMinutes();
  unsigned int second = timeClient.getSeconds();
  String weekDay = weekDays[timeClient.getDay()];

  if (second != second_prev) colon_switch = !colon_switch;

  String fYear = String(year);
//  String fDate = (month < 10 ? "0" : "") + String(month) + "/" + (day < 10 ? "0" : "") + String(day);  // month/day
  String fDate = (day < 10 ? "0" : "") + String(day) + "." + (month < 10 ? "0" : "") + String(month)  ;  // day.month
  String fTime = (hour < 10 ? "0" : "") + String(hour) + (colon_switch ? ":" : " ") + (minute < 10 ? "0" : "") + String(minute);

  u8x8.setFont(u8x8_font_lucasarts_scumm_subtitle_o_2x2_f);
  u8x8.drawString(1, 0, strcpy(new char[fDate.length() + 1], fDate.c_str()));
  u8x8.setFont(u8x8_font_pxplusibmcga_f);
  u8x8.drawString(12, 0, strcpy(new char[fYear.length() + 1], fYear.c_str()));
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString(12, 1, strcpy(new char[weekDay.length() + 1], weekDay.c_str()));
  u8x8.setFont(u8x8_font_inb33_3x6_f);
  u8x8.drawString(1, 2, strcpy(new char[fTime.length() + 1], fTime.c_str()));

  second_prev = second;

  int diff = millis() - t;
  delay(diff >= 0 ? (500 - (millis() - t)) : 0);

if (DST != DST0)
{
  timeClient.setTimeOffset(timezoneOffset + DST*3600);
  timeClient.begin();
  updated = timeClient.update();
  if (updated) {
      Serial.println("NTP time updated.");
      lastUpdatedTime = millis();
    } else {
      Serial.println("Failed to update time. Waiting for retry...");
      lastUpdatedTime = millis() - updateDelay + retryDelay;
    }
DST0 = DST;
}
delay(10);
} // end main loop

unsigned int getYear() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int year = ti->tm_year + 1900;
  return year;
}

unsigned int getMonth() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int month = ti->tm_mon + 1;
  return month;
}

unsigned int getDate() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int month = ti->tm_mday;
  return month;
}
