# SunLocator

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Arduino Library](https://img.shields.io/badge/Arduino-Library-teal.svg)](https://www.arduino.cc/reference/en/libraries/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-orange.svg)](https://platformio.org/lib)

**A lightweight, dependency-free sunrise, sunset and twilight calculator for Arduino.**

SunLocator tells you *the local time at which the sun reaches any elevation angle*
for a given date and location. That single idea covers everything:

* sunrise and sunset,
* civil, nautical and astronomical twilight,
* the moment the sun is 5° below the horizon so you can switch a lamp on,
* the middle of the night, the length of the day, and more.

There is **no WiFi, no RTC, no hardware and no dynamic memory** inside the library —
it is a pure, portable calculator. You give it a date; it gives you times.

```cpp
#include <SunLocator.h>

SunLocator sun(35.8989, 14.5146, 2.0);  // lat, lon, UTC+2

void setup() {
  Serial.begin(115200);
  sun.setDate(2026, 6, 21);

  Serial.print("Sunrise: ");
  Serial.println(SunLocator::timeToString(sun.sunrise().minutes));  // 05:46
  Serial.print("Sunset:  ");
  Serial.println(SunLocator::timeToString(sun.sunset().minutes));   // 20:22
}

void loop() {}
```

---

## Table of contents

- [Features](#features)
- [Supported boards](#supported-boards)
- [Installation](#installation)
- [Quick start](#quick-start)
- [Understanding the angles](#understanding-the-angles)
- [Which angle should I use?](#which-angle-should-i-use)
- [Time zones and daylight saving](#time-zones-and-daylight-saving)
- [Polar day and polar night](#polar-day-and-polar-night)
- [API reference](#api-reference)
- [Accuracy and the algorithm](#accuracy-and-the-algorithm)
- [Examples](#examples)
- [Contributing](#contributing)
- [License](#license)

---

## Features

- **Everything from one formula.** Sunrise, sunset and every twilight boundary are
  the same calculation with a different *elevation angle*.
- **Pure C++.** No `Wire`, `WiFi`, `Time` or RTC dependency. Just `<math.h>`.
- **Portable.** Works on 8-bit AVR, 32-bit ARM (SAMD, STM32, nRF), ESP8266,
  ESP32, RP2040 and every other Arduino-compatible core.
- **Robust at the poles.** Midnight sun and polar night are reported as explicit
  flags — never as a nonsense time.
- **Tiny footprint.** Two files, no heap use, no `String` requirement.
- **Well documented.** Doxygen-style comments and four ready-to-run examples.

## Supported boards

SunLocator only uses standard Arduino APIs and `math.h`, so it compiles for any
core that follows the Arduino API. It has been written with the following in mind:

| Architecture | Examples |
| --- | --- |
| AVR (8-bit) | UNO, Nano, Pro Mini, Leonardo, Mega |
| SAMD / SAM | MKR family, Zero, Nano 33 IoT |
| STM32 | Nucleo, Blue Pill |
| nRF52 | Nano 33 BLE, Adafruit Feather nRF52 |
| ESP8266 | NodeMCU, Wemos D1 mini |
| ESP32 | ESP32, ESP32-S2/S3/C3, ESP32-C6 |
| RP2040 | Raspberry Pi Pico, Nano RP2040 Connect |
| Renesas / Portenta | Portenta H7, UNO R4 |

## Installation

**Arduino Library Manager** (recommended, once published):
`Sketch → Include Library → Manage Libraries…` and search for **SunLocator**.

**Arduino IDE (manual / zip):**

1. Download this repository as a `.zip`.
2. `Sketch → Include Library → Add .ZIP Library…` and select the zip, **or**
   unzip it into your `Arduino/libraries/` folder so that the folder is named
   `SunLocator`.

**PlatformIO:**

```ini
lib_deps = clifarr/sunlocator
```

or

```ini
lib_deps = https://github.com/clifarr/sunlocator.git
```

**Manual (any build system):** copy `src/SunLocator.h` and `src/SunLocator.cpp`
into your project and add them to your build.

## Quick start

```cpp
#include <SunLocator.h>

// 1. Create a calculator with your location and UTC offset.
SunLocator sun(51.5074, -0.1278, 0.0);   // London, UTC+0

void setup() {
  Serial.begin(115200);

  // 2. Give it a date (year, month, day).
  sun.setDate(2026, 3, 20);

  // 3. Ask for the events you care about.
  SunEvent sunrise = sun.sunrise();   // == rise(SUN_ELEVATION_OFFICIAL)
  SunEvent sunset  = sun.sunset();

  char buffer[8];
  SunLocator::formatTime(sunrise.minutes, buffer, sizeof(buffer));
  Serial.print("Sunrise: ");
  Serial.println(buffer);

  // A SunEvent is false when the event does not happen that day.
  if (sunset.occurs) {
    Serial.println(SunLocator::timeToString(sunset.minutes));
  }
}

void loop() {}
```

The location, time zone and date can also be set after construction:

```cpp
SunLocator sun;
sun.setLocation(35.8989, 14.5146);
sun.setTimeZone(2.0);
sun.setDate(2026, 6, 21);          // or sun.setDate(2026, 172) by day-of-year
```

---

## Understanding the angles

### What "elevation angle" means

The **solar elevation angle** is how high the sun is above the horizon, in
degrees:

* `0°` — the sun's centre is exactly on the horizon.
* positive — the sun is **above** the horizon (daytime).
* negative — the sun is **below** the horizon (night, twilight).

> Some references talk about the *zenith angle* instead, which is simply
> `90° − elevation`. This library always asks for **elevation**, because it is
> the intuitive one: *"I want the lights to come on when the sun is 5° below the
> horizon"* → `-5.0`.

### The standard angles

| Elevation | Name | What is happening | Typical use |
| --- | --- | --- | --- |
| `0.0°` | Geometric horizon | Sun's centre on the horizon. | Scientific/geometric only; this is *not* the sunrise you see. |
| `-0.833°` | **Official sunrise / sunset** | The sun's upper limb touches the horizon, including atmospheric refraction. | The "published" sunrise/sunset time. `sun.sunrise()` / `sun.sunset()` use this. |
| `-2°` … `-4°` | "Lights" angle | Sun clearly below the horizon, but the sky is still bright. | A very common choice for switching garden/outdoor lights on. |
| `-6°` | **Civil twilight** | Enough natural light to see outdoors without a torch; street lights normally switch on here. | `sun.civilDawn()` / `sun.civilDusk()`. The classic "dusk". |
| `-12°` | **Nautical twilight** | The horizon is still (just) visible at sea; the first stars appear. | `sun.nauticalDawn()` / `sun.nauticalDusk()`. |
| `-18°` | **Astronomical twilight** | The sky is completely dark; deep-sky observing can start. | `sun.astronomicalDawn()` / `sun.astronomicalDusk()`. Beyond this it is fully night. |

`0°` is used above for reference. Values *below* the horizon are negative;
if you ever need an angle above the horizon (e.g. solar panels, `+10°`), just
pass a positive number.

### Why sunrise is `-0.833` and not `0`

The sun is not a point. Its disc is about 0.53° wide, so its upper edge appears
at the horizon roughly 0.27° *before* its centre does. On top of that, the
atmosphere bends (refracts) light, lifting the apparent sun by a further ~0.57°.
Adding these together gives the familiar **0.833°** used for official
sunrise/sunset. This is why telescopes and photographers prefer the computed
official time over the pure geometry.

---

## Which angle should I use?

Different projects want different thresholds. Some rules of thumb:

| Goal | Suggested angle |
| --- | --- |
| Match the sunrise/sunset printed in a newspaper or weather app | `SUN_ELEVATION_OFFICIAL` (`-0.833`) |
| Turn garden / security lights **on** in the evening | `-3` to `-6` (a very popular choice is **`-6`**, civil dusk) |
| Turn the same lights **off** in the morning | `-4` to `-6` |
| Keep lights on longer (e.g. a dusk-to-dawn lamp) | ON at `-4`, OFF at `-6` |
| Street lighting / billboards | `-6` (civil twilight) |
| Open a chicken coop door at first light | `-6` (civil dawn) |
| Close the coop / roll down blinds in the evening | `-3` to `-6` |
| Start or end astronomical observing | `-18` (astronomical twilight) |

There is no single "correct" value — it is a taste/energy decision. If you are
migrating from a light sensor, pick an angle that reproduces the switching point
you are used to, then tune it by a degree or two.

**Tip:** for a dusk-to-dawn lamp, use a *lower* (more negative) **ON** angle and
an *earlier* (less negative) **OFF** angle to avoid the lamp flickering near the
threshold at cloud edges. The library simply computes crossings; adding a
hysteresis (two different angles, or a time delay) is up to your sketch — see
the `RelayControl` example.

---

## Time zones and daylight saving

`setTimeZone()` takes the offset from UTC **in hours** and may be fractional:

```cpp
sun.setTimeZone(2.0);    // CEST (UTC+2)
sun.setTimeZone(1.0);    // CET  (UTC+1)
sun.setTimeZone(-5.0);   // EST  (UTC-5)
sun.setTimeZone(5.5);    // IST  (UTC+5:30)
sun.setTimeZone(5.75);   // Nepal (UTC+5:45)
```

The returned times are **local clock times** based on that fixed offset, so the
same date can give a different answer in winter and summer because of daylight
saving:

```cpp
// Valletta, 21 June - CEST = UTC+2
sun.setTimeZone(2.0);    // sunrise 05:46
// Valletta, 21 December - CET = UTC+1
sun.setTimeZone(1.0);    // sunrise 07:08
```

SunLocator does **not** know about DST rules. The offset you set is applied to
the date you give it, so pass the offset that is actually in force on that date
(2.0 in summer, 1.0 in winter, for the example above). Entering the wrong
season's offset shifts every result by an hour. You decide the offset:

* If you use NTP with a POSIX timezone string (`configTzTime(...)`), the ESP32/
  ESP8266 system clock is already local, so set the SunLocator time zone to the
  current numeric offset, or compute it as `(localEpoch − utcEpoch)/3600`.
* If you use an RTC that stores UTC, add the offset you want to apply.
* Simplest of all: work in UTC everywhere (`setTimeZone(0.0)`) and compare
  against a UTC `struct tm`, then DST never enters the calculation at all.

A simple way to derive the offset from a local and UTC `struct tm` is shown in the
`RelayControl` example.

---

## Polar day and polar night

Inside the polar circles the sun can fail to reach (or fail to leave) a given
angle on a whole day. SunLocator reports this instead of inventing a time:

```cpp
SunEvent rise = sun.sunrise();

if (rise.occurs) {
  // Normal case: rise.minutes is the time.
} else if (rise.alwaysAbove) {
  // The sun never sinks this low - e.g. midnight sun.
} else if (rise.alwaysBelow) {
  // The sun never climbs this high - e.g. polar night.
}
```

The same flags make `dayLength()` well defined: it returns `1440` minutes during
the midnight sun and `0` minutes during the polar night.

---

## API reference

### `class SunLocator`

| Method | Description |
| --- | --- |
| `SunLocator()` | Default calculator (0,0, UTC+0, 1 Jan 2000). |
| `SunLocator(lat, lon, timeZone)` | Construct and configure in one line. |
| `setLocation(lat, lon)` | Set latitude and longitude (degrees). |
| `setLatitude(lat)` / `setLongitude(lon)` | Set individually. East/north positive. |
| `setTimeZone(hours)` | UTC offset in hours; may be fractional. |
| `getLatitude()` / `getLongitude()` / `getTimeZone()` | Read back the configuration. |
| `setDate(year, month, day)` | Set the date. |
| `setDate(year, dayOfYear)` | Set the date from a 1-based day of year. |
| `getYear()` / `getDayOfYear()` | Read back the current date. |
| `rise(elevation)` | Morning crossing. Returns a `SunEvent`. |
| `set(elevation)` | Evening crossing. Returns a `SunEvent`. |
| `sunrise()` / `sunset()` | `rise`/`set` at `SUN_ELEVATION_OFFICIAL`. |
| `civilDawn()` / `civilDusk()` | `rise`/`set` at `SUN_ELEVATION_CIVIL`. |
| `nauticalDawn()` / `nauticalDusk()` | `rise`/`set` at `SUN_ELEVATION_NAUTICAL`. |
| `astronomicalDawn()` / `astronomicalDusk()` | `rise`/`set` at `SUN_ELEVATION_ASTRONOMICAL`. |
| `solarNoon()` | Local time of the sun's highest point, in minutes. |
| `dayLength()` | Length of the day in minutes (official angle). |
| `dayLength(elevation)` | Length of the day in minutes at a custom angle. |
| `formatTime(minutes, buffer, size)` | Static. Format as `"HH:MM"` (or `"--:--"`). |
| `timeToString(minutes)` | Static. Same, returning a `String`. |

### `struct SunEvent`

| Field | Description |
| --- | --- |
| `double minutes` | Time as minutes after local midnight (`0`…`1439.99`), or `NAN` if the event does not happen. |
| `bool occurs` | `true` if the sun crosses the requested angle that day. |
| `bool alwaysAbove` | `true` if the sun stays above the angle all day (polar day). |
| `bool alwaysBelow` | `true` if the sun stays below the angle all day (polar night). |
| `operator bool()` | Convenience: true when `occurs` is true. |

### Constants

| Constant | Value | Meaning |
| --- | --- | --- |
| `SUN_ELEVATION_OFFICIAL` | `-0.833` | Official sunrise/sunset. |
| `SUN_ELEVATION_CIVIL` | `-6.0` | Civil twilight. |
| `SUN_ELEVATION_NAUTICAL` | `-12.0` | Nautical twilight. |
| `SUN_ELEVATION_ASTRONOMICAL` | `-18.0` | Astronomical twilight. |

### Working with the result

Times are returned as **minutes after local midnight**, which is convenient for
comparison against a real-time clock:

```cpp
struct tm now;
getLocalTime(&now);
int minutesNow = now.tm_hour * 60 + now.tm_min;

if (minutesNow >= sun.set(-6.0).minutes) {
  // it is after civil dusk
}
```

Or, if you prefer structured time:

```cpp
char t[6];
SunLocator::formatTime(sun.sunrise().minutes, t, sizeof(t));  // "05:46"
```

---

## Accuracy and the algorithm

SunLocator is based on the sunrise/sunset algorithm from *Almanac for Computers*
(1990 USNO), as popularised by Ed Williams, generalised to an arbitrary solar
elevation angle.

The library was verified against published reference sunrise/sunset and twilight
data across 15 locations on 5 dates spanning the year (520 event comparisons):

| Latitude band | Mean absolute error | Max absolute error |
| --- | --- | --- |
| Below 60° | **0.6 min** | 2.8 min |
| 60°–66.5° | 1.7 min | 7.4 min |
| Above 66.5° (polar) | 3.3 min | 30 min* |

\* Near the polar circle on dates when the sun barely grazes the horizon, the
crossing time is extremely sensitive to a tiny angular error (a 0.1° error in
declination moves the crossing by tens of minutes). This is inherent to this
class of algorithm and affects every dawn/dusk calculator.

The declination is evaluated once per day, so `rise()` and `set()` always agree
on whether an event happens — you will never see "no sunrise" alongside a valid
sunset for the same angle. The algorithm ignores the observer's altitude above
sea level; the effect is only a few seconds for realistic altitudes.

The library is a *calculator*: it does not correct for the 1–2 minute
year-to-year drift of clock time, doesn't model leap seconds, and does not apply
daylight-saving rules (see [Time zones and daylight saving](#time-zones-and-daylight-saving)).
For the purpose of switching lights this is far more than enough.

---

## Examples

| Sketch | What it shows |
| --- | --- |
| [`BasicSunTimes`](examples/BasicSunTimes/BasicSunTimes.ino) | Print sunrise, sunset, solar noon, day length and twilight. |
| [`TwilightAngles`](examples/TwilightAngles/TwilightAngles.ino) | Compare the dawn/dusk time for every standard angle. |
| [`RelayControl`](examples/RelayControl/RelayControl.ino) | Switch a relay on at dusk and off at dawn (with a simulated clock). |
| [`PolarNightAndDay`](examples/PolarNightAndDay/PolarNightAndDay.ino) | Handle midnight sun and polar night correctly. |

## Contributing

Issues and pull requests are welcome at
<https://github.com/clifarr/sunlocator>. Please keep the code portable
(no platform-specific includes) and make sure the examples still compile.

## License

MIT © Clifford Farrugia. See [LICENSE](LICENSE).

## Acknowledgements

The core algorithm is adapted from *Almanac for Computers, 1990*, published by
the Nautical Almanac Office, United States Naval Observatory, via the widely
used formulation by Ed Williams.
