/*
  PolarNightAndDay

  Above the Arctic/Antarctic circles the sun may not cross the horizon at all
  on a given day: it can stay up all day (midnight sun) or stay down all day
  (polar night). The library reports this explicitly through the SunEvent
  flags instead of returning a bogus time.

    event.occurs       true if the sun crosses the requested angle that day
    event.alwaysAbove  sun stays above the angle all day  (polar day)
    event.alwaysBelow  sun stays below the angle all day  (polar night)

  This sketch reports the state for Tromso (Norway) at both solstices.

  Created by Clifford Farrugia, 2026.
  Released into the public domain / MIT.
*/

#include <SunLocator.h>

SunLocator sun(69.6492, 18.9553, 1.0);  // Tromso, Norway (CET)

void describe(const char* what, const SunEvent& event) {
  Serial.print(what);
  Serial.print(F(": "));
  if (event.occurs) {
    Serial.println(SunLocator::timeToString(event.minutes));
  } else if (event.alwaysAbove) {
    Serial.println(F("sun never goes this low (always above)"));
  } else if (event.alwaysBelow) {
    Serial.println(F("sun never gets this high (always below)"));
  } else {
    Serial.println(F("undefined"));
  }
}

void reportDay(int year, int month, int day) {
  sun.setDate(year, month, day);
  Serial.print(F("---- "));
  Serial.print(year);
  Serial.print('-');
  Serial.print(month);
  Serial.print('-');
  Serial.print(day);
  Serial.println(F(" ----"));
  describe("Sunrise", sun.sunrise());
  describe("Sunset", sun.sunset());
  describe("Civil dawn", sun.civilDawn());
  describe("Civil dusk", sun.civilDusk());
}

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 3000) {
  }

  Serial.println(F("SunLocator - polar regions"));
  reportDay(2026, 6, 21);   // midnight sun
  Serial.println();
  reportDay(2026, 12, 21);  // polar night
}

void loop() {
  // Nothing to do.
}
