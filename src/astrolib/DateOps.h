/*****************************************************************************\
 * DateOps.h
 *
 * DateOps contains misc. time and date operations
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

// uncomment the following line to include an assortment of non-western
// calendars:
/* #define CALENDARS_OF_THE_WORLD */


#if !defined( DATE_OPS__H )
#define DATE_OPS__H

typedef int MonthDays[13];

typedef long YearEndDays[2];

class DateOps {
public:
  enum CalendarType {
    T_GREGORIAN = 0,
    T_JULIAN = 1,
    // these are only used if CALENDARS_OF_THE_WORLD is defined:
    T_HEBREW = 2,
    T_ISLAMIC = 3,
    T_REVOLUTIONARY = 4,
    T_PERSIAN = 5
  };

  static long dmyToDay(
      int day, int month, long year, int calendar = T_GREGORIAN );

  static void dayToDmy(
      long jd, int& day, int& month, long& year, int calendar = T_GREGORIAN );

  static void dayToDmy(
      long jd, int* day, int* month, long* year, int calendar = T_GREGORIAN )
  {
    dayToDmy( jd, *day, *month, *year, calendar );
  }

  static long dstStart(int year);

  static long dstEnd(int year);

private:
  enum CalendarEpoch {
      E_JULIAN_GREGORIAN = 1721060L,
      // these are only used if CALENDARS_OF_THE_WORLD is defined:
      E_ISLAMIC = 1948086L,
      E_HEBREW = 347996L,
      E_REVOLUTIONARY = 2375475L
  };

  static void getJulGregYearData(
      long year, long& days, MonthDays& md, bool julian );

  static int getCalendarData(
      long year, YearEndDays& days, MonthDays& md, int calendar );

#if defined( CALENDARS_OF_THE_WORLD )

  enum {
      HALAKIM_IN_DAY = (24L * 1080L),
      JALALI_ZERO = 1947954L,
      LOWER_PERSIAN_YEAR = -1096,
      UPPER_PERSIAN_YEAR = 2327
  };

  static long mod( long x, long y );

  // Islamic calendar
  static void getIslamicYearData( long year, long& days, MonthDays& md );

  // Hebrew calendar
  static long lunationsToTishri1( long year );
  static void lunationsToDaysAndHalakim(
      long lunations, long& days, long& halakim);
  static void findTishri1( long year, long& days, long& halakim );
  static void getHebrewYearData( long year, YearEndDays& days, MonthDays& md );

  // French Revolutionary calendar
  static long jdOfFrenchRevYear( long year );
  static void getRevolutionaryYearData(
      long year, YearEndDays& days, MonthDays& md );

  // Persian (Jalaali) calendar
  static long jalaliJd0( long jalaliYear );
  static void getJalaliYearData(
      const long year, YearEndDays& days, MonthDays& md );

#endif
};

#endif  /* #if !defined( DATE_OPS__H ) */
