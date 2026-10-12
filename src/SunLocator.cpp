/*
  SunLocator.cpp - Sunrise, sunset and twilight calculator for Arduino.

  Implements the "Almanac for Computers" sunrise/sunset algorithm published by
  the United States Naval Observatory, generalised so that *any* solar
  elevation angle can be requested (not just the official sunrise/sunset).

  Part of the SunLocator library.
  MIT License - see LICENSE file for details.
*/

#include "SunLocator.h"

#include <math.h>
#include <stdio.h>

// Some cores build with a strict-ANSI configuration that hides M_PI.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

//! Portable NaN test that does not depend on <cmath> macros.
inline bool isNan(double value) { return value != value; }

}  // namespace

// =======================================================================
// Construction
// =======================================================================

SunLocator::SunLocator()
    : _latitude(0.0),
      _longitude(0.0),
      _timeZone(0.0),
      _year(2000),
      _dayOfYear(1) {}

SunLocator::SunLocator(double latitude, double longitude, double timeZone)
    : _latitude(latitude),
      _longitude(longitude),
      _timeZone(timeZone),
      _year(2000),
      _dayOfYear(1) {}

// =======================================================================
// Location
// =======================================================================

void SunLocator::setLocation(double latitude, double longitude) {
  setLatitude(latitude);
  setLongitude(longitude);
}

void SunLocator::setLatitude(double latitude) {
  if (latitude > 90.0) latitude = 90.0;
  if (latitude < -90.0) latitude = -90.0;
  _latitude = latitude;
}

void SunLocator::setLongitude(double longitude) {
  // Normalise to [-180, 180) so callers may pass either convention.
  longitude = _fixAngle(longitude);
  if (longitude >= 180.0) longitude -= 360.0;
  _longitude = longitude;
}

void SunLocator::setTimeZone(double timeZone) { _timeZone = timeZone; }

double SunLocator::getLatitude() const { return _latitude; }
double SunLocator::getLongitude() const { return _longitude; }
double SunLocator::getTimeZone() const { return _timeZone; }

// =======================================================================
// Date
// =======================================================================

void SunLocator::setDate(int year, int month, int day) {
  if (month < 1) month = 1;
  if (month > 12) month = 12;
  if (day < 1) day = 1;
  const int lastDay = _daysInMonth(year, month);
  if (day > lastDay) day = lastDay;

  _year = year;
  _dayOfYear = _dayOfYearFor(year, month, day);
}

void SunLocator::setDate(int year, int dayOfYear) {
  const int lastDay = _isLeapYear(year) ? 366 : 365;
  if (dayOfYear < 1) dayOfYear = 1;
  if (dayOfYear > lastDay) dayOfYear = lastDay;

  _year = year;
  _dayOfYear = dayOfYear;
}

int SunLocator::getYear() const { return _year; }
int SunLocator::getDayOfYear() const { return _dayOfYear; }

// =======================================================================
// Core calculations
// =======================================================================

double SunLocator::_event(double elevation, bool morning,
                          bool& alwaysAbove, bool& alwaysBelow) const {
  alwaysAbove = false;
  alwaysBelow = false;

  const double zenith = 90.0 - elevation;          // angle below zenith
  const double lonHour = _longitude / 15.0;        // longitude in hours
  const double dayNumber = static_cast<double>(_dayOfYear);

  // 1. First approximation of the time of the event (local solar hours).
  const double t = dayNumber + (((morning ? 6.0 : 18.0) - lonHour) / 24.0);

  // 2. Sun's mean anomaly.
  const double M = (0.9856 * t) - 3.289;

  // 3. Sun's true longitude.
  double L = M + (1.916 * sin(_deg2rad(M))) +
             (0.020 * sin(2.0 * _deg2rad(M))) + 282.634;
  L = _fixAngle(L);

  // 4. Sun's right ascension, matched to the quadrant of L.
  double RA = _rad2deg(atan(0.91764 * tan(_deg2rad(L))));
  const double Lquadrant = floor(L / 90.0) * 90.0;
  const double RAquadrant = floor(RA / 90.0) * 90.0;
  RA = (RA + (Lquadrant - RAquadrant)) / 15.0;

  // 5. Sun's declination. This is evaluated once at local solar noon rather
  //    than at the event time, so that the morning and evening crossings use
  //    the *same* declination. Otherwise a location just inside the polar
  //    circle could report "sunrise never happens" while simultaneously
  //    reporting a sunset time for the very same elevation angle.
  const double tNoon = dayNumber + ((12.0 - lonHour) / 24.0);
  const double mNoon = (0.9856 * tNoon) - 3.289;
  const double lNoon = _fixAngle(mNoon + (1.916 * sin(_deg2rad(mNoon))) +
                                 (0.020 * sin(2.0 * _deg2rad(mNoon))) + 282.634);
  const double sinDec = 0.39782 * sin(_deg2rad(lNoon));
  const double cosDec = cos(asin(sinDec));

  // 6. Sun's local hour angle.
  double cosH = (cos(_deg2rad(zenith)) - (sinDec * sin(_deg2rad(_latitude)))) /
                (cosDec * cos(_deg2rad(_latitude)));

  // cosH > 1  => the sun never climbs to the requested elevation (polar night).
  // cosH < -1 => the sun never sinks to the requested elevation (polar day).
  if (cosH > 1.0) {
    if (cosH > 1.0 + 1e-12) {
      alwaysBelow = true;
      return NAN;
    }
    cosH = 1.0;
  } else if (cosH < -1.0) {
    if (cosH < -1.0 - 1e-12) {
      alwaysAbove = true;
      return NAN;
    }
    cosH = -1.0;
  }

  double H = morning ? (360.0 - _rad2deg(acos(cosH))) : _rad2deg(acos(cosH));
  H /= 15.0;

  // 7. Local mean time of the event.
  const double localMean = H + RA - (0.06571 * t) - 6.622;

  // 8. Convert to UTC, then to local clock time.
  const double utc = _fixHour(localMean - lonHour);
  return _fixHour(utc + _timeZone) * 60.0;
}

SunEvent SunLocator::rise(double elevation) const {
  SunEvent event;
  event.minutes = _event(elevation, true, event.alwaysAbove, event.alwaysBelow);
  event.occurs = !isNan(event.minutes);
  return event;
}

SunEvent SunLocator::set(double elevation) const {
  SunEvent event;
  event.minutes = _event(elevation, false, event.alwaysAbove, event.alwaysBelow);
  event.occurs = !isNan(event.minutes);
  return event;
}

double SunLocator::solarNoon() const {
  const double lonHour = _longitude / 15.0;
  const double dayNumber = static_cast<double>(_dayOfYear);

  // Approximate the transit time as local solar noon (12:00).
  const double t = dayNumber + ((12.0 - lonHour) / 24.0);
  const double M = (0.9856 * t) - 3.289;

  double L = M + (1.916 * sin(_deg2rad(M))) +
             (0.020 * sin(2.0 * _deg2rad(M))) + 282.634;
  L = _fixAngle(L);

  double RA = _rad2deg(atan(0.91764 * tan(_deg2rad(L))));
  const double Lquadrant = floor(L / 90.0) * 90.0;
  const double RAquadrant = floor(RA / 90.0) * 90.0;
  RA = (RA + (Lquadrant - RAquadrant)) / 15.0;

  // At transit the hour angle H is zero.
  const double localMean = RA - (0.06571 * t) - 6.622;
  const double utc = _fixHour(localMean - lonHour);
  return _fixHour(utc + _timeZone) * 60.0;
}

double SunLocator::dayLength() const { return dayLength(SUN_ELEVATION_OFFICIAL); }

double SunLocator::dayLength(double elevation) const {
  const SunEvent morning = rise(elevation);
  const SunEvent evening = set(elevation);

  if (morning.occurs && evening.occurs) {
    return evening.minutes - morning.minutes;
  }
  if (morning.alwaysAbove && evening.alwaysAbove) return 1440.0;  // polar day
  if (morning.alwaysBelow && evening.alwaysBelow) return 0.0;     // polar night
  return NAN;
}

// =======================================================================
// Convenience accessors
// =======================================================================

SunEvent SunLocator::sunrise() const { return rise(SUN_ELEVATION_OFFICIAL); }
SunEvent SunLocator::sunset() const { return set(SUN_ELEVATION_OFFICIAL); }

SunEvent SunLocator::civilDawn() const { return rise(SUN_ELEVATION_CIVIL); }
SunEvent SunLocator::civilDusk() const { return set(SUN_ELEVATION_CIVIL); }

SunEvent SunLocator::nauticalDawn() const {
  return rise(SUN_ELEVATION_NAUTICAL);
}
SunEvent SunLocator::nauticalDusk() const {
  return set(SUN_ELEVATION_NAUTICAL);
}

SunEvent SunLocator::astronomicalDawn() const {
  return rise(SUN_ELEVATION_ASTRONOMICAL);
}
SunEvent SunLocator::astronomicalDusk() const {
  return set(SUN_ELEVATION_ASTRONOMICAL);
}

// =======================================================================
// Helpers
// =======================================================================

void SunLocator::formatTime(double minutes, char* buffer, size_t length) {
  if (buffer == NULL || length == 0) return;

  if (isNan(minutes)) {
    snprintf(buffer, length, "--:--");
    return;
  }

  long total = static_cast<long>(minutes + 0.5);  // floor(x + 0.5)
  total = ((total % 1440) + 1440) % 1440;         // wrap into [0, 1440)
  snprintf(buffer, length, "%02ld:%02ld", total / 60, total % 60);
}

String SunLocator::timeToString(double minutes) {
  char buffer[8];
  formatTime(minutes, buffer, sizeof(buffer));
  return String(buffer);
}

double SunLocator::_deg2rad(double degrees) { return degrees * (M_PI / 180.0); }

double SunLocator::_rad2deg(double radians) { return radians * (180.0 / M_PI); }

double SunLocator::_fixAngle(double degrees) {
  degrees = fmod(degrees, 360.0);
  if (degrees < 0.0) degrees += 360.0;
  return degrees;
}

double SunLocator::_fixHour(double hours) {
  hours = fmod(hours, 24.0);
  if (hours < 0.0) hours += 24.0;
  return hours;
}

bool SunLocator::_isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int SunLocator::_daysInMonth(int year, int month) {
  static const int days[] = {31, 28, 31, 30, 31, 30,
                             31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && _isLeapYear(year)) return 29;
  return days[month - 1];
}

int SunLocator::_dayOfYearFor(int year, int month, int day) {
  static const int cumulative[] = {0,  0,  31, 59,  90,  120,
                                   151, 181, 212, 243, 273, 304, 334};
  int dayOfYear = cumulative[month] + day;
  if (month > 2 && _isLeapYear(year)) dayOfYear++;
  return dayOfYear;
}
