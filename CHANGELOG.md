# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-10-12

### Added

- Initial release.
- `SunLocator` class computing the local time at which the sun reaches any
  elevation angle, for a given date and location.
- Convenience accessors for sunrise/sunset and civil, nautical and astronomical
  twilight.
- `solarNoon()` and `dayLength()`.
- `SunEvent` result type that distinguishes "event occurs", "sun always above"
  (midnight sun) and "sun always below" (polar night).
- `formatTime()` / `timeToString()` helpers.
- Support for setting the date either by year/month/day or by day of year.
- Examples: `BasicSunTimes`, `TwilightAngles`, `RelayControl`,
  `PolarNightAndDay`.
- Library metadata for the Arduino Library Manager and PlatformIO.

[1.0.0]: https://github.com/clifarr/sunlocator/releases/tag/v1.0.0
