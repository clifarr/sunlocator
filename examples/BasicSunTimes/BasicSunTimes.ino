/*
  BasicSunTimes

  The simplest possible example: print the main solar events for one day.

  This sketch is deliberately hardware-independent so it runs on every
  supported board (AVR, SAMD, ESP8266, ESP32, RP2040, ...). In a real
  project you would take the date from an RTC or from NTP instead of
  hard-coding it.

  Created by Clifford Farrugia, 2026.
  Released into the public domain / MIT.
*/

#include <SunLocator.h>

// ---------------------------------------------------------------------
// Configure your location and time zone.
//
//   latitude   degrees, north positive, south negative
//   longitude  degrees, east positive, west negative
//   timeZone   hours from UTC (2 = CEST, 1 = CET, -5 = EST, 5.5 = IST)
// ---------------------------------------------------------------------
const double LATITUDE  = 35.8989;  // Valletta, Malta
const double LONGITUDE = 14.5146;
const double TIME_ZONE = 2.0;      // CEST

// Create the calculator once, at global scope.
SunLocator sun(LATITUDE, LONGITUDE, TIME_ZONE);

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 3000) {
  }

  // Pick a date. (Year, month, day.)
  sun.setDate(2026, 6, 21);

  char time[8];  // "HH:MM" plus terminator

  Serial.println(F("SunLocator - basic example"));
  Serial.print(F("Year: "));
  Serial.println(sun.getYear());
  Serial.print(F("Day of year: "));
  Serial.println(sun.getDayOfYear());
  Serial.println();

  SunLocator::formatTime(sun.sunrise().minutes, time, sizeof(time));
  Serial.print(F("Sunrise:             "));
  Serial.println(time);

  SunLocator::formatTime(sun.sunset().minutes, time, sizeof(time));
  Serial.print(F("Sunset:              "));
  Serial.println(time);

  SunLocator::formatTime(sun.solarNoon(), time, sizeof(time));
  Serial.print(F("Solar noon:          "));
  Serial.println(time);

  Serial.print(F("Day length:          "));
  Serial.print(sun.dayLength() / 60.0, 2);
  Serial.println(F(" hours"));

  // The optional String helper avoids the manual buffer:
  Serial.print(F("Civil dawn:          "));
  Serial.println(SunLocator::timeToString(sun.civilDawn().minutes));

  Serial.print(F("Civil dusk:          "));
  Serial.println(SunLocator::timeToString(sun.civilDusk().minutes));
}

void loop() {
  // Nothing to do.
}
