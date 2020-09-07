#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h> // --> https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <Ticker.h>  //Ticker Library
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "Settings.h"

#define PIN D4
#define HOSTNAME "ScrollingDeskSign"


//tickers
Ticker updateDataTicker;
Ticker showWeatherTicker;
Ticker showTimeTicker;
bool isUpdateData = false;
bool isShowWeather = false;
bool isShowTime = true;

// LED Settings
const int offset = 1;
int refresh = 0;
String message = "hello";
int spacer = 1;  // dots between letters
int width = 5 + spacer; // The font width is 5 pixels + spacer

// Time
/*TimeDB TimeDB("");
  String lastMinute = "xx";
  int displayRefreshCount = 1;
  long lastEpoch = 0;
  long firstEpoch = 0;
  long displayOffEpoch = 0;M
*/

// MATRIX DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, PIN,
                            NEO_MATRIX_BOTTOM    + NEO_MATRIX_RIGHT +
                            NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
                            NEO_GRB            + NEO_KHZ800);


// Change the externalLight to the pin you wish to use if other than the Built-in LED
int externalLight = LED_BUILTIN; // LED_BUILTIN is is the built in LED on the Wemos

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  Serial.begin(115200);
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setTextColor(matrix.Color(255, 0, 0));
  matrix.setBrightness(20);
  clearScreen();
  centerPrint("Boot");
  WiFi.begin(wifi_ssid, wifi_password);   // access Wi-FI point

  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected\r\n");


  // print the received signal strength:
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(getWifiQuality());
  Serial.println("%");
  updateDataTicker.attach(updateDataInterval, shouldUpdateData);
  showWeatherTicker.attach(showWeatherInterval, shouldShowWeather);
  showTimeTicker.attach(updateTimeInterval, shouldShowTime);
  isUpdateData = true;
  timeClient.begin();
  clearScreen();
}

void loop() {
  if (isUpdateData) {
    clearScreen();
    centerPrint("Update");
    getData();
    isUpdateData = false;
    isShowTime = true;
    clearScreen();
  }
  if (isShowWeather) {
    showWeather();
    isShowWeather = false;
    isShowTime = true;
    clearScreen();
  }
  if (isShowTime) {
    centerPrint(hourMinutes(false));
    isShowTime = false;
  }
}

void showWeather() {
  clearScreen();
  String temperature = weatherClient.getTempRounded(0);
  String description = weatherClient.getDescription(0);
  description.toUpperCase();
  String msg;
  msg += " ";

  if (SHOW_DATE) {
    /*   msg += TimeDB.getDayName() + ", ";
       msg += TimeDB.getMonthName() + " " + day() + "  ";*/
  }
  if (SHOW_CITY) {
    msg += weatherClient.getCity(0) + "  ";
  }
  msg += temperature + getTempSymbol() + "  ";

  //show high/low temperature
  if (SHOW_HIGHLOW) {
    msg += "Hi/Lo:" + weatherClient.getHigh(0).substring(0, 2) + "/" + weatherClient.getLow(0).substring(0, 2) + " " + getTempSymbol() + "  ";
  }

  if (SHOW_CONDITION) {
    msg += description + "  ";
  }
  if (SHOW_HUMIDITY) {
    msg += "Humid:" + weatherClient.getHumidityRounded(0) + "%  ";
  }
  if (SHOW_WIND) {
    msg += "Wind: " + weatherClient.getDirectionText(0) + " @ " + weatherClient.getWindRounded(0) + " " + getSpeedSymbol() + "  ";
  }
  //line to show barometric pressure
  if (SHOW_PRESSURE) {
    msg += "Press:" + weatherClient.getPressure(0) + getPressureSymbol() + "  ";
  }
  scrollMessage(msg);
}

void scrollMessage(String message) {
  int x = matrix.width();
  int lastLetterPosition = -1 * (message.length() * 6);
  while (--x > lastLetterPosition) {
    matrix.fillScreen(0);    //Turn off all the LEDs
    matrix.setCursor(x, 1);
    matrix.print(message);
    matrix.show();
    delay(100);
    clearScreen();
  }
}

//***********************************************************************
void getData() //client function to send/receive GET request data.
{
  digitalWrite(externalLight, LOW);
  matrix.fillScreen(0); // show black
  Serial.println();

  matrix.drawPixel(0, 7, HIGH);
  matrix.drawPixel(0, 6, HIGH);
  matrix.drawPixel(0, 5, HIGH);
  matrix.show();
  weatherClient.updateWeather();
  if (weatherClient.getError() != "") {
    scrollMessage(weatherClient.getError());
  }

  Serial.println("Updating Time...");
  //Update the Time
  matrix.fillScreen(0);
  matrix.drawPixel(0, 4, HIGH);
  matrix.drawPixel(0, 3, HIGH);
  matrix.drawPixel(0, 2, HIGH);
  matrix.show();
  timeClient.setTimeOffset(weatherClient.getTimeZone(0) * 3600);
  timeClient.update();
  /* TimeDB.updateConfig(TIMEDBKEY, weatherClient.getLat(0), weatherClient.getLon(0));
    TimeDB.updateTime();
    Serial.println();*/
  digitalWrite(externalLight, HIGH);
}

void centerPrint(String msg) {
  centerPrint(msg, false);
}

void centerPrint(String msg, boolean extraStuff) {
  clearScreen();
  int x = (matrix.width() - (msg.length() * width)) / 2;

  // Print the static portions of the display before the main Message
  if (extraStuff) {
    /*    if (!IS_24HOUR && IS_PM && isPM()) {
          matrix.drawPixel(matrix.width() - 1, 8, HIGH);
        }*/
  }

  matrix.setCursor(x, 1);
  matrix.print(msg);
  matrix.show();

  //  matrix.write();
}

void clearScreen() {
  matrix.fillScreen(0);
  matrix.show();
}

String hourMinutes(boolean isRefresh) {
  int currentHour = timeClient.getHours();
  Serial.println(currentHour);
  if (IS_24HOUR) {
    return String(currentHour) + ":" + zeroPad(timeClient.getMinutes());
  } else {
    int displayHour = currentHour < 13 ? currentHour : currentHour - 12 ;
    return  String(displayHour) + ":" + zeroPad(timeClient.getMinutes());
  }
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) {
    return 0;
  } else if (dbm >= -50) {
    return 100;
  } else {
    return 2 * (dbm + 100);
  }
}
String getTempSymbol() {
  String rtnValue = "F";
  if (IS_METRIC) {
    rtnValue = "C";
  }
  return rtnValue;
}

String getSpeedSymbol() {
  String rtnValue = "mph";
  if (IS_METRIC) {
    rtnValue = "kph";
  }
  return rtnValue;
}

String getPressureSymbol()
{
  String rtnValue = "";
  if (IS_METRIC)
  {
    rtnValue = "mb";
  }
  return rtnValue;
}

void shouldUpdateData() {
  isUpdateData = true;
}

void shouldShowWeather() {
  isShowWeather = true;
}

void shouldShowTime() {
  isShowTime = true;
}

String zeroPad(int number) {
  if (number < 10) {
    return "0" + String(number);
  } else {
    return String(number);
  }
}
