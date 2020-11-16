#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h> // --> https://github.com/tzapu/WiFiManager
//#include <ESP8266mDNS.h>
#include <Ticker.h>  //Ticker Library
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "Settings.h"
#include "AccuWeatherLibrary.h"
#include "SlackWebhook.h"

AccuweatherDailyData dataD;
AccuweatherHourlyData dataH;
AccuweatherCurrentData dataC;
AccuweatherLocationData dataL;
Accuweather aw(AccuWeatherApiKey, CityID, "en-us", IS_METRIC);

//tickers
Ticker updateCurrentWeatherTicker;
Ticker showWeatherTicker;
Ticker showTimeTicker;
bool isUpdateCurrentWeatherData = true;

bool isUpdateDailyWeatherData = true;
bool isShowWeather = false;
bool isShowTime = true;
bool isUpdateTime = false;

// LED Settings
const int offset = 1;
int refresh, updateDailyWeatherCounter, updateTimeCounter = 0;
String message = "hello";
int spacer = 1;  // dots between letters
int width = 5 + spacer; // The font width is 5 pixels + spacer

SlackWebhook webhook(host, slack_hook_url, fingerprint);

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
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(90);
  if (!wifiManager.startConfigPortal(HOSTNAME)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    if (!wifiManager.autoConnect(HOSTNAME)) {
      ESP.reset();
      delay(5000);
    }
  }
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected...yeey :)");
  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("Error setting up MDNS responder!");
  }
  WiFi.mode(WIFI_STA);

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  // print the received signal strength:
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(getWifiQuality());
  Serial.println("%");
  updateCurrentWeatherTicker.attach(updateCurrentWeatherInterval, shouldUpdateCurrentWeather);
  showWeatherTicker.attach(showWeatherInterval, shouldShowWeather);
  showTimeTicker.attach(updateTimeInterval, shouldShowTime);
  timeClient.begin();
  clearScreen();
  int maxRetries = 0;
  while (dataL.GmtOffset == 0 && maxRetries++ < 3) {
    updateTime();
  }

}

void loop() {
  if (isUpdateCurrentWeatherData || isUpdateDailyWeatherData) {
    centerPrint("Update");
    char output[100];
    sprintf(output, "Today Hi: %f, Lo: %fF", dataD.TempMax, dataD.TempMin);
    Serial.println(output);
    getData();
    isShowTime = true;
    clearScreen();
  }
  if (isShowWeather) {
    Serial.println("Showing weather...");

    showWeather();
    isShowWeather = false;
    isShowTime = true;
    clearScreen();
  }
  if (isShowTime) {
    Serial.println("Showing time...");
    centerPrint(hourMinutes(false));
    isShowTime = false;
  }
  if (isUpdateTime) {
    updateTime();
  }
  ArduinoOTA.handle();
}

void updateTime() {
  Serial.println("Updating time...");
  isUpdateTime = false;
  AccuweatherLocationData localLocation;
  int ret = aw.getLocation(&localLocation);
  Serial.println(ret);
  while (aw.continueDownload() > 0) {
  }
  Serial.println(localLocation.GmtOffset);
  if ((localLocation.GmtOffset != 0  && localLocation.GmtOffset != NULL) && dataL.GmtOffset == 0) { //got something and didn't have anything before
    dataL = localLocation;
  } else if (abs(dataL.GmtOffset - localLocation.GmtOffset) <= 1 && dataL.GmtOffset != 0) { //had a something and daylight savings - only one hour change
    dataL = localLocation;
  } else if (dataL.GmtOffset == 0) { // didn't get anything and didn't have anything so default 
    Serial.println("Error getting location..");
    postToSlack("Location Error");
    dataL.GmtOffset = -4;
  }
  /*char gmt[25];
  Serial.println("Preparing message...");
  sprintf(gmt, "GMToffset set to: %i", dataL.GmtOffset);
  Serial.println("About to send message...");
  postToSlack(gmt);*/
  timeClient.setTimeOffset(dataL.GmtOffset * 3600);
  timeClient.update();

}
void showWeather() {
  clearScreen();
  String temperature = roundFloat(dataC.Temperature);
  String description = dataC.WeatherText;
  description.toUpperCase();
  String msg;
  msg += " ";


  if (SHOW_CITY) {
    msg += dataL.Name + "  ";
  }
  msg += temperature + getTempSymbol() + "  ";


  if (SHOW_CONDITION) {
    msg += description + "  ";
  }
  if (SHOW_HUMIDITY) {
    msg += "Humid:" + roundFloat(dataC.RelativeHumidity) + "%  ";
  }
  if (SHOW_WIND) {
    msg += "Wind: " + String(dataC.WindDirection) + " @ " + roundFloat(dataC.WindSpeed) + " " + getSpeedSymbol() + "  ";
  }
  //show high/low temperature
  if (SHOW_HIGHLOW) {
    msg += "Hi/Lo:" + roundFloat(dataD.TempMax) + "/" + roundFloat(dataD.TempMin) + getTempSymbol() + "  ";
  }
  //line to show barometric pressure
  /*if (SHOW_PRESSURE) {
    msg += "Press:" + weatherClient.getPressure(0) + getPressureSymbol() + "  ";
    }*/
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
  Serial.println("Updating Data...");
  if (isUpdateCurrentWeatherData) {
    AccuweatherCurrentData localCurrentData;
    Serial.println("Updating Current Weather...");
    int ret = aw.getCurrent(&localCurrentData);
    while (aw.continueDownload() > 0) {
    }
    if (localCurrentData.Temperature != 0) {
      dataC = localCurrentData;
      Serial.println("Got Current Weather....");
    } else {
      Serial.println("Current Weather Error");
      postToSlack("Current Weather Error");
    }

    isUpdateCurrentWeatherData = false;
    updateDailyWeatherCounter++;
    if (updateDailyWeatherCounter * updateCurrentWeatherInterval >= updateDailyWeatherInterval) {
      isUpdateDailyWeatherData = true;
      updateDailyWeatherCounter = 0;
    }
    updateTimeCounter++;
    if (updateTimeCounter * updateCurrentWeatherInterval >= updateTimeZoneInterval) {
      isUpdateTime = true;
      updateTimeCounter = 0;
    }

  }
  if (isUpdateDailyWeatherData) {
    Serial.println("Updating Daily Weather...");
    AccuweatherDailyData localDailyWeather;
    int ret = aw.getToday(&localDailyWeather);

    while (aw.continueDownload() > 0) {
    }
    char output[100];
    isUpdateDailyWeatherData = false;
    if (localDailyWeather.TempMax == 0) {
      Serial.println("Error Getting Daily Weather....");
      postToSlack("Daily Weather Error");
      return;
    } else {
      dataD = localDailyWeather;
    }
    Serial.println("Got Daily Weather....");
    sprintf(output, "Today Hi: %f, Lo: %fF", dataD.TempMax, dataD.TempMin);
    Serial.println(output);
   // postToSlack(output);
  }
  matrix.fillScreen(0);
  matrix.drawPixel(0, 4, HIGH);
  matrix.drawPixel(0, 3, HIGH);
  matrix.drawPixel(0, 2, HIGH);
  matrix.show();

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

void shouldUpdateCurrentWeather() {
  isUpdateCurrentWeatherData = true;
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

String roundFloat(float incoming) {
  int temp = static_cast<int>(incoming + 0.5f);
  String tempString = String(temp);
  tempString.replace(" ", "");
  return tempString;
}

void postToSlack(char* message) {
  char temp[150];
  sprintf(temp, "{\"username\": \"Scrolling Sign\", \"text\": \"%s\"}", message);
  webhook.postMessageToSlack(String(temp));
}
