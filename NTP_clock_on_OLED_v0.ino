// https://www.hackster.io/alankrantas/esp8266-ntp-clock-on-ssd1306-oled-arduino-ide-35116e
// small changes by Nicu FLORICA (niq_ro)
// v.0 - changed date mode (day.mounth not month/day)
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <U8x8lib.h> // https://github.com/olikraus/u8g2/wiki/u8g2install

const char *ssid = "niq_ro"; // your wifi name and password
const char *pw   = "bererecepecaldura";
const long timezoneOffset = 2 * 60 * 60; // ? hours * 60 * 60 - no DST 
const long timezoneOffset = 3 * 60 * 60; // ? hours * 60 * 60 - DST (Daylight Saving Time - summer time

const char          *ntpServer  = "pool.ntp.org"; // change it to local NTP server if needed
const unsigned long updateDelay = 900000;         // update time every 15 min
const unsigned long retryDelay  = 5000;           // retry 5 sec later if time query failed
//const String        weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String        weekDays[7] = {"Dum ", "Luni", "Mar ", "Mie ", "Joi ", "Vin ", "Sam "};

unsigned long lastUpdatedTime = updateDelay * -1;
unsigned int  second_prev = 0;
bool          colon_switch = false;

ESP8266WiFiMulti WiFiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);

// Display constructors: https://github.com/olikraus/u8g2/wiki/u8x8setupcpp
// Fonts: https://github.com/olikraus/u8g2/wiki/u8x8reference
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(SCL, SDA, U8X8_PIN_NONE);

void setup() {

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

  timeClient.setTimeOffset(timezoneOffset);
  timeClient.begin();

  server.begin(); // Start web server!
}

void loop() {

  if (WiFiMulti.run() == WL_CONNECTED && millis() - lastUpdatedTime >= updateDelay) {
    bool updated = timeClient.update();
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
//  String fDate = (month < 10 ? "0" : "") + String(month) + "/" + (day < 10 ? "0" : "") + String(day);
  String fDate = (day < 10 ? "0" : "") + String(day) + "." + (month < 10 ? "0" : "") + String(month)  ;
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

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client is connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) { // If the current line is blank, you got two newline characters in a row. That's the end of the client HTTP request, so send a response:
            client.println("HTTP/1.1 200 OK"); // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            client.println("Content-type:text/html"); // and a content-type so the client knows what's coming, then a blank line:
            client.println("Connection: close");
            client.println();
            
            if (header.indexOf("GET /vara/on") >= 0) { // If the user clicked the alarm's on button
              Serial.println("Daylight saving time (DST) was activated !");
              oravara = "on";
              timeClient.setTimeOffset((TIMEZONE+1)*3600); // Offset time from the GMT standard
            } 
            else if (header.indexOf("GET /vara/off") >= 0) { // If the user clicked the alarm's off button
              Serial.println("Daylight saving time (DST) was deactivated !");
              oravara = "off";
              timeClient.setTimeOffset((TIMEZONE+0)*3600); // Offset time from the GMT standard
            }

            else if (header.indexOf("GET /time") >= 0) { // If the user submitted the time input form
              // Strip the data from the GET request
              int index = header.indexOf("GET /time");
              String timeData = header.substring(index + 15, index + 22);
           
              Serial.println(timeData);
              // Update our alarm DateTime with the user selected time, using the current date.
              // Since we just compare the hours and minutes on each loop, I do not think the day or month matters.
              DateTime temp = DateTime(timeClient.getEpochTime()); 
            }
            
            // Display the HTML web page
            // Head
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<link rel=\"stylesheet\" href=\"//stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css\">"); // Bootstrap
            client.println("</head>");
            
            // Body
            client.println("<body>");
            client.println("<h1 class=\"text-center mt-3\"NTP / DSP Clock</h1>"); // Title

            // Current Time
            client.print("<h1 class=\"text-center\">"); 
            client.print(timeClient.getFormattedTime());
            client.println("</h1>");
            
            
            // Display current state, and ON/OFF buttons for Alarm  
            client.println("<h2 class=\"text-center\">Daylight Saving Time - " + oravara + "</h2>");
            if (oravara=="off") {
              client.println("<p class=\"text-center\"><a href=\"/vara/on\"><button class=\"btn btn-sm btn-danger\">ON</button></a></p>");
            }
            else {
              client.println("<p class=\"text-center\"><a href=\"/vara/off\"><button class=\"btn btn-success btn-sm\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            client.println(); // The HTTP response ends with another blank line
            break; // Break out of the while loop
            
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    
    header = ""; // Clear the header variable
    client.stop(); // Close the connection
    Serial.println("Client disconnected.");
    Serial.println("");
  }

if(oravara == "on")
timeClient.setTimeOffset((TIMEZONE+1)*3600); // Offset time from the GMT standard
else
timeClient.setTimeOffset(TIMEZONE*3600); // Offset time from the GMT standard


}  //end main loop

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
