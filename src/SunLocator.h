/*
  SunLocator.h - Sunrise, sunset and twilight calculator for Arduino.

  Computes the local time at which the sun reaches any given elevation angle
  (its height above the horizon, in degrees) for a given date and location.

  Part of the SunLocator library.
  MIT License - see LICENSE file for details.
*/

#ifndef SUNLOCATOR_H
#define SUNLOCATOR_H

#include <Arduino.h>

/*
  Standard solar elevation angles, in degrees, measured from the horizon.
  Negative values are *below* the horizon. Pass any of these (or any custom
  angle) to rise()/set() or dayLength().

    SUN_ELEVATION_OFFICIAL     upper limb of the sun touches the horizon
                               (accounts for refraction + solar radius)
    SUN_ELEVATION_CIVIL        civil twilight  (sun 6 degrees below horizon)
    SUN_ELEVATION_NAUTICAL     nautical twilight (sun 12 degrees below horizon)
    SUN_ELEVATION_ASTRONOMICAL astronomical twilight (sun 18 degrees below)
*/
#define SUN_ELEVATION_OFFICIAL      (-0.833)
#define SUN_ELEVATION_CIVIL         (-6.0)
#define SUN_ELEVATION_NAUTICAL      (-12.0)
#define SUN_ELEVATION_ASTRONOMICAL  (-18.0)

/**
 * Result of a single solar event calculation.
 *
 * `minutes` is the time of the event expressed as minutes after local
 * midnight (0..1439.99). When the event does not happen on the requested
 * day, `minutes` is NAN and exactly one of `alwaysAbove` / `alwaysBelow`
 * is set.
 */
struct SunEvent {
  double minutes;      //!< minutes after local midnight, or NAN
  bool   occurs;       //!< true if the sun crosses the requested elevation
  bool   alwaysAbove;  //!< sun stays above the elevation all day (polar day)
  bool   alwaysBelow;  //!< sun stays below the elevation all day (polar night)

  //! Convenience: a SunEvent is "true" when the event occurs.
  explicit operator bool() const { return occurs; }
};

/**
 * SunLocator - computes sunrise, sunset and twilight times.
 *
 * The class is a pure calculator: it never reads the clock, the network or
 * any hardware. You supply the location, the time zone and the date, and it
 * returns times as "minutes after local midnight". Compare that against the
 * current time (from an RTC, NTP, ...) to drive lamps, curtains, relays, etc.
 */
class SunLocator {
 public:
  //! Construct with default (unset) location, time zone 0 and date 1 Jan 2000.
  SunLocator();

  //! Construct and initialise location and time zone in one call.
  SunLocator(double latitude, double longitude, double timeZone = 0.0);

  // ---------------------------------------------------------------------
  // Location
  // ---------------------------------------------------------------------

  //! Set latitude (deg, north positive) and longitude (deg, east positive).
  void setLocation(double latitude, double longitude);

  //! Set latitude in degrees; north is positive, south is negative.
  void setLatitude(double latitude);

  //! Set longitude in degrees; east is positive, west is negative.
  void setLongitude(double longitude);

  //! Set the UTC offset in hours (e.g. 2.0 for CEST, 5.5 for IST).
  void setTimeZone(double timeZone);

  double getLatitude() const;
  double getLongitude() const;
  double getTimeZone() const;

  // ---------------------------------------------------------------------
  // Date
  // ---------------------------------------------------------------------

  //! Set the date. Month is 1..12, day is 1..31.
  void setDate(int year, int month, int day);

  //! Set the date directly from a day of year (1..365, or 366 in leap years).
  void setDate(int year, int dayOfYear);

  int getYear() const;
  //! Day of year, 1-based.
  int getDayOfYear() const;

  // ---------------------------------------------------------------------
  // Core calculations
  // ---------------------------------------------------------------------

  /**
   * Morning crossing of the given elevation angle.
   * @param elevation degrees above the horizon (use a negative value below).
   */
  SunEvent rise(double elevation) const;

  /**
   * Evening crossing of the given elevation angle.
   * @param elevation degrees above the horizon (use a negative value below).
   */
  SunEvent set(double elevation) const;

  //! Local time of solar noon (sun at its highest point), in minutes.
  double solarNoon() const;

  //! Length of the day in minutes, using the official sunrise/sunset angle.
  double dayLength() const;

  //! Length of the day in minutes for a custom elevation angle.
  double dayLength(double elevation) const;

  // ---------------------------------------------------------------------
  // Convenience accessors (all use the standard angles above)
  // ---------------------------------------------------------------------

  SunEvent sunrise() const;
  SunEvent sunset() const;
  SunEvent civilDawn() const;
  SunEvent civilDusk() const;
  SunEvent nauticalDawn() const;
  SunEvent nauticalDusk() const;
  SunEvent astronomicalDawn() const;
  SunEvent astronomicalDusk() const;

  // ---------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------

  /**
   * Format minutes-after-midnight as "HH:MM" into a caller supplied buffer.
   * A non-occurring event (NAN) is rendered as "--:--".
   */
  static void formatTime(double minutes, char* buffer, size_t length);

  //! Convenience wrapper around formatTime() that returns a String.
  static String timeToString(double minutes);

 private:
  double _latitude;
  double _longitude;
  double _timeZone;
  int    _year;
  int    _dayOfYear;

  //! Core solver. Returns minutes after local midnight, or NAN.
  double _event(double elevation, bool morning,
                bool& alwaysAbove, bool& alwaysBelow) const;

  static double _deg2rad(double degrees);
  static double _rad2deg(double radians);
  static double _fixAngle(double degrees);  //!< wrap into [0, 360)
  static double _fixHour(double hours);     //!< wrap into [0, 24)
  static bool   _isLeapYear(int year);
  static int    _daysInMonth(int year, int month);
  static int    _dayOfYearFor(int year, int month, int day);
};

#endif  // SUNLOCATOR_H
