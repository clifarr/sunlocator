/*
  RelayControl

  The classic use case: switch a lamp (or relay, or shelly, or MOSFET) on
  when the sun sinks below the horizon in the evening and off again when it
  climbs back above the horizon in the morning - without any photocell.

  Two independent angles are used so the behaviour can be asymmetric:

      LIGHTS_ON_ELEVATION    when the sun reaches this angle in the evening
                             (sun going down) -> lights ON
      LIGHTS_OFF_ELEVATION   when the sun reaches this angle in the morning
                             (sun coming up)   -> lights OFF

  A later/higher ON angle and an earlier/lower OFF angle keep the lights on
  for longer, just like a real dawn-to-dusk switch.

  NOTE ON TIME
  -------------
  SunLocator never reads the clock itself. You must tell it today's date and
  you must know the current local time. This example ships with a *simulated*
  clock (one minute per second) so it runs on any board out of the box.
  Replace readLocalTime() with your real source: a DS3231/PCF8563 RTC, or
  NTP on an ESP32/ESP8266 (see the notes at the bottom of this file).

  Created by Clifford Farrugia, 2026.
  Released into the public domain / MIT.
*/

#include <SunLocator.h>

// ---------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------
const double LATITUDE  = 35.8989;  // Valletta, Malta
const double LONGITUDE = 14.5146;
const double TIME_ZONE = 2.0;      // CEST

const int  RELAY_PIN        = 2;             // change to your relay pin
const bool RELAY_ACTIVE_LOW = true;         // most relay modules are

const double LIGHTS_ON_ELEVATION  = -5.0;  // evening: sun 5 deg below horizon
const double LIGHTS_OFF_ELEVATION = -6.0;  // morning: sun 6 deg below horizon

// ---------------------------------------------------------------------
// State
// ---------------------------------------------------------------------
SunLocator sun(LATITUDE, LONGITUDE, TIME_ZONE);

SunEvent lightsOn;   // evening crossing (rising sun below horizon -> ON)
SunEvent lightsOff;  // morning crossing (sun climbs back above -> OFF)
int  loadedDayOfYear = -1;
bool relayState = false;

// Returns today's day-of-year and the local time as minutes after midnight.
bool readLocalTime(int& dayOfYear, int& minutesOfDay);

void setRelay(bool on) {
  if (on == relayState) return;
  relayState = on;
  bool level = RELAY_ACTIVE_LOW ? !on : on;
  digitalWrite(RELAY_PIN, level ? HIGH : LOW);
}

// (Re)compute the switching times. Called once per day.
void loadSunTimes(int dayOfYear) {
  sun.setDate(2026, dayOfYear);  // year, day-of-year overload

  lightsOn  = sun.set(LIGHTS_ON_ELEVATION);
  lightsOff = sun.rise(LIGHTS_OFF_ELEVATION);

  loadedDayOfYear = dayOfYear;

  Serial.print(F("Day "));
  Serial.print(dayOfYear);
  Serial.print(F("  lights ON at "));
  Serial.print(SunLocator::timeToString(lightsOn.minutes));
  Serial.print(F("  OFF at "));
  Serial.println(SunLocator::timeToString(lightsOff.minutes));
}

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 3000) {
  }

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);  // start switched off
  relayState = false;

  Serial.println(F("SunLocator - relay control example"));
}

void loop() {
  int dayOfYear = 0;
  int minutesOfDay = 0;
  if (!readLocalTime(dayOfYear, minutesOfDay)) {
    return;
  }

  if (dayOfYear != loadedDayOfYear) {
    loadSunTimes(dayOfYear);
  }

  // Decide whether the lights should be on right now.
  bool shouldBeOn = false;
  if (lightsOn.occurs && lightsOff.occurs) {
    // On after dusk OR before dawn (spans local midnight).
    shouldBeOn = (minutesOfDay >= lightsOn.minutes) ||
                 (minutesOfDay <= lightsOff.minutes);
  } else if (lightsOn.occurs) {
    shouldBeOn = minutesOfDay >= lightsOn.minutes;  // polar twilight edge case
  } else if (lightsOff.occurs) {
    shouldBeOn = minutesOfDay <= lightsOff.minutes;
  }

  setRelay(shouldBeOn);
}

// ---------------------------------------------------------------------
// Simulated clock: "21 June", one minute of the day per real second.
// Replace this with a real time source in your project.
// ---------------------------------------------------------------------
bool readLocalTime(int& dayOfYear, int& minutesOfDay) {
  static unsigned long startMs = 0;
  if (startMs == 0) startMs = millis();

  const unsigned long msPerMinute = 1000UL;  // simulation speed
  unsigned long elapsed = (millis() - startMs) / msPerMinute;

  dayOfYear = 172;                       // 21 June (non-leap year)
  minutesOfDay = (int)(elapsed % 1440UL);
  return true;
}

// ---------------------------------------------------------------------
// Using a real clock
// ---------------------------------------------------------------------
//
// ESP32 / ESP8266 (NTP):
//
//   #include <time.h>
//   configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
//   struct tm t;
//   if (getLocalTime(&t)) {
//     dayOfYear   = t.tm_yday + 1;
//     minutesOfDay = t.tm_hour * 60 + t.tm_min;
//   }
//
// Note: when you use configTzTime(), the *system* clock is already in local
// time, so set TIME_ZONE on the SunLocator to the same UTC offset (or a fixed
// value) and compare directly against minutesOfDay.
//
// A DS3231 RTC returns a struct tm as well:
//
//   DateTime now = rtc.now();
//   dayOfYear    = ...;                 // compute from now.month()/now.day()
//   minutesOfDay = now.hour() * 60 + now.minute();
