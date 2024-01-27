#include <WiFi.h>
#include <ESP32Ping.h>
#include <WiFiManager.h>
#include "time.h"
#include <U8g2lib.h>
#include <string.h>
#include <FastLED.h>

#define BUZ 12
#define DATA_PIN 21
#define NUM_LEDS 4
#define CHIPSET WS2812
#define BRIGHTNESS 25
#define COLOR_ORDER GRB
#define WIFI_CONNECT_STATUS_LED 0
#define UPLOAD_DOWNLOAD_STATUS_LED 1
#define INTERNET_STATUS_LED 2
#define TIME_STATUS_LED 3

CRGB leds[NUM_LEDS];

TaskHandle_t Task1;



void remoteHost();
void ipCheck();
void tostring();
void wifiSignalQuality();
void connectWiFi();
void wifiConnectStatusLed();
void printLocalTime();
void printSSID();
void internetStatus();
void noInternetBeep();
void iconUpDown();
void uploadDownloadLed();
void pingTest();
void welcomeMsg();
void clearLCD();
void loading();



U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, 18, 23, 5, 22); //for full buffer mode

//U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, 18, 23, 5, 22); //for page buffer mode

const IPAddress remote_ip(8, 8, 8, 8); //8.8.8.8
const char* remote_host = "www.google.com";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;

unsigned long previousMillis = 0;
const long buzzerDuration = 200;
const long interval = 300000;

uint8_t wifiRSSI = 0;
uint8_t pingStatus = 0;
uint16_t pingTime = 0;
String ssid = "";


static unsigned char upload_logo_bits[] = {
  0x04,
  0x0E,
  0x1F,
  0x04,
  0x04,
  0x04,
  0x04,
  0x04
};
static unsigned char download_logo_bits[] = {
  0x04,
  0x04,
  0x04,
  0x04,
  0x04,
  0x1F,
  0x0E,
  0x04
};


int signalQuality[] = {99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
                       99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 97, 97, 96, 96, 95, 95, 94, 93, 93, 92,
                       91, 90, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 76, 75, 74, 73, 71, 70, 69, 67, 66, 64,
                       63, 61, 60, 58, 56, 55, 53, 51, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20,
                       17, 15, 13, 10, 8, 6, 3, 1, 1, 1, 1, 1, 1, 1, 1
                      };

void setup() {
  Serial.begin(115200);
  pinMode(BUZ, OUTPUT);
  u8g2.begin();
  welcomeMsg();
  delay(2000);
//  connectWiFi(0, 10);
  u8g2.clearBuffer();

  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();


  xTaskCreatePinnedToCore(
    loop1,
    "Task1",
    100000,
    NULL,
    1,
    &Task1,
    1);
  delay(500);
}

///////////////////////////////////////////////////////////////////

void remoteHost(uint8_t rhx, uint8_t rhy) {

  clearLCD(rhx, rhy - 10, 128, 10);

  u8g2.setFont(u8g2_font_helvR08_tr); //8px height
  u8g2.drawStr(rhx, rhy, "ping -t");
  u8g2.drawStr(rhx + 37, rhy, remote_host); //remote_ip, remote_host
  u8g2.sendBuffer();

}

///////////////////////////////////////////////////////////////////

void ipCheck(uint8_t ipx, uint8_t ipy) {

  String rawIP = WiFi.localIP().toString(); //toString () used for convert char to string

  String IPAdd = "IP - " + rawIP;

  clearLCD(ipx, ipy - 10, 98, 10);

  u8g2.setFont(u8g2_font_helvR08_tr);
  u8g2.drawStr(ipx, ipy, IPAdd.c_str()); //c_str() function used for convert string to const char *
  u8g2.sendBuffer();

}

//////////////////////////////////////////////////////////

void tostring(char str[], int num)
{
  int i, rem, len = 0, n;

  n = num;
  while (n != 0)
  {
    len++;
    n /= 10;
  }
  for (i = 0; i < len; i++)
  {
    rem = num % 10;
    num = num / 10;
    str[len - (i + 1)] = rem + '0';
  }
  str[len] = '\0';
}

//////////////////////////////////////////////////////////

void wifiSignalQuality(uint8_t sqx, uint8_t sqy) {

  wifiRSSI = WiFi.RSSI() * (-1);
  char str[3];
  char str2[3] = "%";

  tostring(str, signalQuality[wifiRSSI]);

  strcat(str, str2);
  clearLCD(sqx, sqy - 10, 27, 10);
  u8g2.setFont(u8g2_font_helvR08_tr);
  u8g2.drawStr(sqx, sqy, str);
  u8g2.sendBuffer();
}

/////////////////////////////////////////////////////

void connectWiFi(uint8_t cwx, uint8_t cwy) {

  clearLCD(0, 0, 128, 64);

  WiFiManager wm;
  u8g2.setFont(u8g2_font_helvR08_tr);
  u8g2.drawStr(cwx, cwy, "Connecting to WiFi ...");
  u8g2.sendBuffer();

  WiFi.disconnect();
  delay(50);
  bool success = false;
  while (!success) {
    wm.setConfigPortalTimeout(60);
    success = wm.autoConnect("ESP PING-MASTER");
    if (!success) {

      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_helvR08_tr);
      u8g2.drawStr(cwx, cwy + 12, "AP - ESP PING-MASTER");
      u8g2.drawStr(cwx, cwy + 24, "Setup IP - 192.168.4.1");
      u8g2.drawStr(cwx, cwy + 36, "Conection Failed!");
      u8g2.sendBuffer();
    }
  }

  ssid = WiFi.SSID();
  u8g2.clearBuffer();
  u8g2.drawStr(cwx, cwy + 10, "Connected SSID - ");
  u8g2.drawStr(cwx, cwy + 22, ssid.c_str());
  u8g2.sendBuffer();

  ipCheck(cwx, cwy + 34);
  wifiSignalQuality(cwx, cwy + 46);
  delay(3000);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

}

////////////////////////////////////////////////////////////////////

void wifiConnectStatusLed(uint8_t wifiConnectStatus) {

  if (wifiConnectStatus == 1) {
    leds[WIFI_CONNECT_STATUS_LED] = CRGB(255, 64, 0);
    FastLED.show();
  }
  //  if (wifiConnectStatus == 2) {
  //
  //    leds[WIFI_CONNECT_STATUS_LED] = CRGB(255, 255, 255);
  //    FastLED.show();
  //  }
}

///////////////////////////////////////////////////////////////////

void printLocalTime(uint8_t ltx, uint8_t lty) {

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  if (!getLocalTime(&timeinfo))
  {
    leds[TIME_STATUS_LED] = CRGB(0, 0, 0);
    FastLED.show();

    clearLCD(ltx, lty - 18, 128, 18);
    u8g2.setFont(u8g2_font_unifont_tr);
    u8g2.drawStr(ltx, lty, "Time Failed!");
    u8g2.sendBuffer();

  }

  if (getLocalTime(&timeinfo)) {

    leds[TIME_STATUS_LED] = CRGB(255, 0, 255);

    clearLCD(ltx, lty - 18, 128, 18);

    char timeStringBuff[10];
    char secStringBuff[10];//10 chars should be enough
    char ampmStringBuff[10];
    char wDayStringBuff[10];
    char mDayStringBuff[10];
    char mNameStringBuff[10];
    char monthStringBuff[10];
    char yearStringBuff[10];

    strftime(timeStringBuff, sizeof(timeStringBuff), "%I:%M", &timeinfo);
    strftime(secStringBuff, sizeof(secStringBuff), "%S", &timeinfo);
    strftime(ampmStringBuff, sizeof(ampmStringBuff), "%p", &timeinfo);
    strftime(wDayStringBuff, sizeof(wDayStringBuff), "%a", &timeinfo);
    strftime(mDayStringBuff, sizeof(mDayStringBuff), "%d", &timeinfo);
    strftime(mNameStringBuff, sizeof(mNameStringBuff), "%b", &timeinfo);
    strftime(monthStringBuff, sizeof(monthStringBuff), "%m", &timeinfo);
    strftime(yearStringBuff, sizeof(yearStringBuff), "%y", &timeinfo);

    /*
      %a Abbreviated weekday name.
      %A  Full weekday name.
      %b  Abbreviated month name.
      %B  Full month name.
      %c  Date/Time in the format of the locale.
      %C  Century number [00-99], the year divided by 100 and truncated to an integer.
      %d  Day of the month [01-31].
      %D  Date Format, same as %m/%d/%y.
      %e  Same as %d, except single digit is preceded by a space [1-31].
      %g  2 digit year portion of ISO week date [00,99].
      %F  ISO Date Format, same as %Y-%m-%d.
      %G  4 digit year portion of ISO week date. Can be negative.
      %h  Same as %b.
      %H  Hour in 24-hour format [00-23].
      %I  Hour in 12-hour format [01-12].
      %j  Day of the year [001-366].
      %m  Month [01-12].
      %M  Minute [00-59].
      %n  Newline character.
      %p  AM or PM string.
      %r  Time in AM/PM format of the locale. If not available in the locale time format, defaults to the POSIX time AM/PM format: %I:%M:%S %p.
      %R  24-hour time format without seconds, same as %H:%M.
      %S  Second [00-61]. The range for seconds allows for a leap second and a double leap second.
      %t  Tab character.
      %T  24-hour time format with seconds, same as %H:%M:%S.
      %u  Weekday [1,7]. Monday is 1 and Sunday is 7.
      %U  Week number of the year [00-53]. Sunday is the first day of the week.
      %V  ISO week number of the year [01-53]. Monday is the first day of the week. If the week containing January 1st has four or more days in the new year then it is considered week 1. Otherwise, it is the last week of the previous year, and the next year is week 1 of the new year.
      %w  Weekday [0,6], Sunday is 0.
      %W  Week number of the year [00-53]. Monday is the first day of the week.
      %x  Date in the format of the locale.
      %X  Time in the format of the locale.
      %y  2 digit year [00,99].
      %Y  4-digit year. Can be negative.
      %z  UTC offset. Output is a string with format +HHMM or -HHMM, where + indicates east of GMT, - indicates west of GMT, HH indicates the number of hours from GMT, and MM indicates the number of minutes from GMT.
      %Z  Time zone name.
      %%  % character.
    */

    u8g2.setFont(u8g2_font_timB18_tr);
    u8g2.drawStr(ltx, lty, timeStringBuff);

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(ltx + 59, lty, ampmStringBuff);
    u8g2.setFont(u8g2_font_t0_11b_tr);
    u8g2.drawStr(ltx + 78, lty, mDayStringBuff);
    u8g2.drawStr(ltx + 94, lty, monthStringBuff);
    u8g2.drawStr(ltx + 110, lty, yearStringBuff);


    u8g2.drawStr(ltx + 59, lty - 9, secStringBuff);
    u8g2.drawStr(ltx + 78, lty - 9, wDayStringBuff);
    u8g2.drawStr(ltx + 100, lty - 9, mNameStringBuff);

    u8g2.sendBuffer();
  }

}

/////////////////////////////////////////////////////////////////////////////

void printSSID(uint8_t ssidx, uint8_t ssidy) {

  if (strlen(ssid.c_str()) > 7) {
    String shortSSID = ssid.substring(0, 7);
    String wifiName = "SSID " + shortSSID + " ...";
    clearLCD(ssidx, ssidy - 10, 99, 8);

    u8g2.setFont(u8g2_font_helvR08_tr);
    u8g2.drawStr(ssidx, ssidy, wifiName.c_str()); //c_str() function used for convert string to const char *
    u8g2.sendBuffer();
  } else {
    String wifiName = "SSID " + ssid;
    clearLCD(ssidx, ssidy - 10, 99, 8);

    u8g2.setFont(u8g2_font_helvR08_tr);
    u8g2.drawStr(ssidx, ssidy, wifiName.c_str()); //c_str() function used for convert string to const char *
    u8g2.sendBuffer();
  }

}

///////////////////////////////////////////////////////////////////////////////////

void internetStatusLed(uint8_t iStatus) {
  if (iStatus == 1) {
    leds[INTERNET_STATUS_LED] = CRGB(0, 255, 0);
    FastLED.show();
  }
  if (iStatus == 2) {
    leds[INTERNET_STATUS_LED] = CRGB(255, 0, 0);
    FastLED.show();
  }
}

//////////////////////////////////////////////////////////////////////////////////

void internetStatus(uint8_t iSx, uint8_t iSy, uint8_t statusValue) {

  char avg_time_str[7];
  char unit_str[7] = " ms";
  if (statusValue == 1)
  {
    internetStatusLed(statusValue);

    tostring(avg_time_str, pingTime);
    strcat(avg_time_str, unit_str);

    clearLCD(iSx, iSy - 9, 80, 9);

    u8g2.setFont(u8g2_font_t0_11b_tr);
    u8g2.drawStr(iSx, iSy, avg_time_str);

    clearLCD(iSx + 82, iSy - 9, 36, 9);
    u8g2.drawStr(iSx + 82, iSy, "Online");

    u8g2.sendBuffer();

  }
  if (statusValue == 2)
  {
    internetStatusLed(statusValue);

    clearLCD(iSx, iSy - 9, 128, 9);
    u8g2.setFont(u8g2_font_t0_11b_tr);
    u8g2.drawStr(iSx, iSy, "No internet!");

  }
}


///////////////////////////////////////////////////////////

void iconUpDown(uint8_t ix, uint8_t iy, uint8_t udFlag) {

  if (udFlag == 1) {
    uploadDownloadLed(udFlag);
    clearLCD(ix, iy, 12, 8);
    u8g2.drawXBM(ix, iy, 5, 8, upload_logo_bits);
  }
  if (udFlag == 2) {
    uploadDownloadLed(udFlag);
    clearLCD(ix, iy, 12, 8);
    u8g2.drawXBM(ix + 7, iy, 5, 8, download_logo_bits);
  }

}

//////////////////////////////////////////////////////////////

void uploadDownloadLed(uint8_t udLed) {
  if (udLed == 1) {
    leds[UPLOAD_DOWNLOAD_STATUS_LED] = CRGB(0, 0, 255);
    FastLED.show();
  }
  if (udLed == 2) {
    leds[UPLOAD_DOWNLOAD_STATUS_LED] = CRGB(0, 204, 255);
    FastLED.show();
  }
}

/////////////////////////////////////////////////////////

void pingTest() {

  iconUpDown(107, 55, 1);

  if (Ping.ping(remote_host)) //remote_ip, remote_host
  {

    pingTime = Ping.averageTime();
    pingStatus = 1;

    iconUpDown(107, 55, 2);
  }
  else
  {
    pingStatus = 2;
    iconUpDown(107, 55, 2);
  }

  delay(3000);
}

////////////////////////////////////////////////////////////

void welcomeMsg() {

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_halftone_tr);
  u8g2.drawStr(7, 25, "ESP PING MASTER");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(45, 60, "...by mrinal");

  u8g2.sendBuffer();
  u8g2.clearBuffer();

}

//////////////////////////////////////////////

void clearLCD(const long x, uint8_t y, uint8_t wid, uint8_t hig) {
  u8g2.setDrawColor(0);
  u8g2.drawBox(x, y, wid, hig);
  u8g2.setDrawColor(1);

}

//////////////////////////////////////////////////////////////

void noInternetBeep(int netStatus) {
  if (netStatus == 2) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      digitalWrite(BUZ, HIGH);
      delay(buzzerDuration);
      digitalWrite(BUZ, LOW);
      delay(50);
      digitalWrite(BUZ, HIGH);
      delay(buzzerDuration);
      digitalWrite(BUZ, LOW);
    }
  }
}

/////////////////////////////////////////////////////////////
void loading()
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend( leds[pixelnumber], newcolor, 64);
  }
}

///////////////////////////////////////////////////////////////

void loop1(void * parameter) {
  while (1) {
    if (WiFi.status() == WL_CONNECTED)
    {
      pingTest();
    }

    while (WiFi.status() != WL_CONNECTED) {
      loading();
      FastLED.show();
    }
  }

}

//////////////////////////////////////////////////////////////////
void loop() {

  if (WiFi.status() == WL_CONNECTED)
  {
    wifiConnectStatusLed(1);
    printSSID(0, 53);
    wifiSignalQuality(100, 53);
    ipCheck(0, 63);
    printLocalTime(0, 18);
    remoteHost(0, 29);
    internetStatus(0, 42, pingStatus);
    noInternetBeep(pingStatus);

  }
  else
  {
    wifiConnectStatusLed(2);
    connectWiFi(0, 10);
    clearLCD(0, 0, 128, 64);
  }
}
