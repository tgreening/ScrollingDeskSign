/** The MIT License (MIT)

  Copyright (c) 2018 David Payne

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/******************************************************************************
   This is designed for the Wemos D1 ESP8266
   Wemos D1 Mini:  https://amzn.to/2qLyKJd
   MAX7219 Dot Matrix Module 4-in-1 Display For Arduino
   Matrix Display:  https://amzn.to/2HtnQlD
 ******************************************************************************/
/******************************************************************************
   NOTE: The settings here are the default settings for the first loading.
   After loading you will manage changes to the settings via the Web Interface.
   If you want to change settings again in the settings.h, you will need to
   erase the file system on the Wemos or use the “Reset Settings” option in
   the Web Interface.
 ******************************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiManager.h> // --> https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>




#define PIN D4
#define HOSTNAME "ScrollingDeskSign"

//******************************
// Start Settings
//******************************

const char* AccuWeatherApiKey = "xxxxxx";
// Default City Location (use http://openweathermap.org/find to find city ID)
int CityID = xxxxx; //Only USE ONE for weather marquee
String marqueeMessage = "Hello";
bool IS_METRIC = false; // false = Imperial and true = Metric
boolean IS_24HOUR = false; // 23:00 millitary 24 hour clock
boolean IS_PM = true; // Show PM indicator on Clock when in AM/PM mode

int showWeatherInterval = 300; //show weather every five minutes
int updateCurrentWeatherInterval = 3600;  //update data every 60 minutes
int updateDailyWeatherInterval = 10800;  //update data every 120 minutes
int updateTimeZoneInterval = 10800;
int updateTimeInterval = 30;
String timeDisplayTurnsOn = "06:30";  // 24 Hour Format HH:MM -- Leave blank for always on. (ie 05:30)
String timeDisplayTurnsOff = "23:00"; // 24 Hour Format HH:MM -- Leave blank for always on. Both must be set to work.


// (some) Default Weather Settings
boolean SHOW_DATE = false;
boolean SHOW_CITY = false;
boolean SHOW_CONDITION = true;
boolean SHOW_HUMIDITY = true;
boolean SHOW_WIND = false;
boolean SHOW_WINDDIR = false;
boolean SHOW_PRESSURE = false;
boolean SHOW_HIGHLOW = true;
//******************************
// End Settings
//******************************
