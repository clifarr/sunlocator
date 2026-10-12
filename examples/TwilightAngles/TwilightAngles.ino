/*
  TwilightAngles

  Shows how the same location produces different "dawn" and "dusk" times
  depending on the elevation angle you ask for. This is the key idea behind
  the library: every twilight boundary is just a different angle below the
  horizon.

  Created by Clifford Farrugia, 2026.
  Released into the public domain / MIT.
*/

#include <SunLocator.h>

SunLocator sun(35.8989, 14.5146, 2.0);  // Valletta, Malta (CEST)

struct Angle {
  const char* label;
  double elevation;  // degrees above the horizon
};

const Angle angles[] = {
    {"Horizon (0.0)", 0.0},
    {"Official (-0.83)", SUN_ELEVATION_OFFICIAL},
    {"Custom (-2.5)", -2.5},
    {"Civil (-6.0)", SUN_ELEVATION_CIVIL},
    {"Nautical (-12.0)", SUN_ELEVATION_NAUTICAL},
    {"Astronomical (-18.0)", SUN_ELEVATION_ASTRONOMICAL},
};
const size_t angleCount = sizeof(angles) / sizeof(angles[0]);

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 3000) {
  }

  sun.setDate(2026, 6, 21);

  Serial.println(F("SunLocator - twilight angles"));
  Serial.println(F("elevation               morning   evening"));
  Serial.println(F("-----------------------------------------"));

  for (size_t i = 0; i < angleCount; i++) {
    SunEvent morning = sun.rise(angles[i].elevation);
    SunEvent evening = sun.set(angles[i].elevation);

    char dawn[8];
    char dusk[8];
    SunLocator::formatTime(morning.minutes, dawn, sizeof(dawn));
    SunLocator::formatTime(evening.minutes, dusk, sizeof(dusk));

    Serial.print(angles[i].label);
    for (size_t pad = strlen(angles[i].label); pad < 22; pad++) {
      Serial.print(' ');
    }
    Serial.print(dawn);
    Serial.print(F("      "));
    Serial.println(dusk);
  }
}

void loop() {
  // Nothing to do.
}
