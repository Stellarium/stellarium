/*****************************************************************************\
 * DateOps.cpp
 *
 * DateOps contains misc. time and date operations
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#include "DateOps.h"

#include <string.h>

/* General calendrical comments:

This code supports conversions between JD and five calendrical systems:
Julian,  Gregorian,  Hebrew,  Islamic,  and (French) revolutionary.
Comments pertaining to specific calendars are found near the code for
those calendars.

For each calendar,  there is a "get_(calendar_name)_year_data( )" function,
used only within this source code. This function takes a particular year
number,  and computes the JD corresponding to "new years day" (first day of
the first month) in that calendar in that year.  It also figures out the
number of days in each month of that year,  returned in the array
month_data[].  There can be up to 13 months,  because the Hebrew calendar
(and some others that may someday be added) can include an "intercalary
month".  If a month doesn't exist,  then the month_data[] entry for it
will be zero (thus,  in the Gregorian and Julian and Islamic calendars,
month_data[12] is always zero,  since these calendars have only 12 months.)

The next level up is the get_calendar_data( ) function,  which (through
the wonders of a switch statement) can get the JD of New Years Day and
the array of months for any given year for any calendar.  Above this
point,  all calendars can be treated in a common way;  one is shielded
from the oddities of individual calendrical systems.

Finally,  at the top level,  we reach the only two functions that are
exported for the rest of the world to use:  dmy_to_day( ) and day_to_dmy( ).
The first takes a day,  month,  year,  and calendar system.  It calls
get_calendar_data( ) for the given year,  adds in the days in the months
intervening New Years Day and the desired month,  and adds in the day
of the month,  and returns the resulting Julian Day.

day_to_dmy( ) reverses this process.  It finds an "approximate" year
corresponding to an input JD,  and calls get_calendar_data( ) for
that year.  By adding all the month_data[] values for that year,  it
can also find the JD for the _end_ of that year;  if the input JD is
outside that range,  it may have to back up a year or add in a year.
Once it finds "JD of New Years Day < JD < JD of New Years Eve",  it's
a simple matter to track down which month and day of the month corresponds
to the input JD.
*/

const char* monthNames[12] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/*
 *  Begin:  Gregorian and Julian calendars (combined for simplicity)
 *
 *  It's common to implement Gregorian/Julian calendar code with the
 * aid of cryptic formulae,  rather than through simple lookup tables.
 * For example,  consider this formula from Fliegel and Van Flandern,
 * to convert Gregorian (D)ay, (M)onth, (Y)ear to JD:
 *
 * JD = (1461*(Y+4800+(M-14)/12))/4+(367*(M-2-12*((M-14)/12)))/12
 *       -(3*((Y+4900+(M-14)/12)/100))/4+D-32075
 *
 * The only way to verify that they work is to feed through all possible
 * cases.  Personally,  I like to be able to look at a chunk of code and
 * see what it means.  It should resemble the Reformation view of the
 * Bible:  anyone can read it and witness the truth thereof.
 */

//----------------------------------------------------------------------------
void DateOps::getJulGregYearData(
    long year, long& daysInYear, MonthDays& md, bool julian )
{
  static const MonthDays months =
                   { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0 };

  if( year >= 0L ) {
    daysInYear = year * 365L + year / 4L;
    if( !julian )
      daysInYear += -year / 100L + year / 400L;
  }
  else {
   daysInYear = (year * 365L) + (year - 3L) / 4L;
   if( !julian )
     daysInYear += -(year - 99L) / 100L + (year - 399L) / 400L;
  }

  if( julian )
    daysInYear -= 2L;

  memcpy( &md, months, sizeof(MonthDays) );

  if( !(year % 4)) {
    if( (year % 100L) || !(year % 400L) || julian ) {
      md[1] = 29;
      daysInYear--;
    }
  }
  daysInYear += E_JULIAN_GREGORIAN + 1;
}

//----------------------------------------------------------------------------
int DateOps::getCalendarData(
    long year, YearEndDays& days, MonthDays& md, int calendar)
{
  int rval = 0;

  memset( &md, 0, sizeof(MonthDays) );
  switch( calendar)
  {
    case T_GREGORIAN:
    case T_JULIAN:
        getJulGregYearData( year, days[0], md, (T_JULIAN == calendar) );
        break;
#if defined( CALENDARS_OF_THE_WORLD )
    case T_ISLAMIC:
        getIslamicYearData( year, days[0], md );
        break;
    case T_HEBREW:
        getHebrewYearData( year, days, md );
        break;
    case T_REVOLUTIONARY:
        getRevolutionaryYearData( year, days, md );
        break;
    case T_PERSIAN:
        if( year >= LOWER_PERSIAN_YEAR && year <= UPPER_PERSIAN_YEAR)
          getJalaliYearData( year, days, md );
        else
          rval = -1;
        break;
#endif
    default:
        rval = -1;
        break;
  }
  if( 0 == rval ) {
    //
    // days[1] = JD of "New Years Eve" + 1;  that is, New Years Day of the
    // following year.  If you have days[0] <= JD < days[1],  JD is in the
    // current year.
    //
    days[1] = days[0];
    for( int i=0; i<13; i++ )
      days[1] += md[i];
  }
  return( rval );
}

/*
 * dmy_to_day( )
 *
 * just gets calendar data for the current year,  including the JD of New Years
 * Day for that year.  After that,  all it has to do is add up the days in
 * intervening months, plus the day of the month, and it's done:
 */

// calendar: 0 = gregorian, 1 = julian

long DateOps::dmyToDay( int day, int month, long year, int calendar )
{
  long jd = 0;
  MonthDays md;
  YearEndDays yed;

  if( 0 == getCalendarData( year, yed, md, calendar ) ) {
    jd = yed[0];
    for( int i=0; i<(month-1); i++ ) {
      jd += md[i];
    }
    jd += long(day - 1);
  }
  return jd;
}

/*
 * day_to_dmy( )
 *
 * Estimates the year corresponding to an input JD and calls get_calendar_data();
 * for that year.  Occasionally,  it will find that the guesstimate was off;
 * in such cases,  it moves ahead or back a year and tries again.  Once it's
 * done,  jd - year_ends[0] gives the number of days since New Years Day;
 * by subtracting month_data[] values,  we quickly determine which month and
 * day of month we're in.
 */

// calendar: 0 = gregorian, 1 = julian

void DateOps::dayToDmy( long jd, int& day, int& month, long& year, int calendar )
{
  day = -1;           /* to signal an error */
  switch( calendar) {
  case T_GREGORIAN:
  case T_JULIAN:
    year = (jd - E_JULIAN_GREGORIAN) / 365L;
    break;
#if defined( CALENDARS_OF_THE_WORLD )
  case T_HEBREW:
    year = (jd - E_HEBREW) / 365L;
    break;
  case T_ISLAMIC:
    year = (jd - E_ISLAMIC) / 354L;
    break;
  case T_REVOLUTIONARY:
    year = (jd - E_REVOLUTIONARY) / 365L;
    break;
  case T_PERSIAN:
    year = (jd - JALALI_ZERO) / 365L;
    if( year < LOWER_PERSIAN_YEAR)
      year = LOWER_PERSIAN_YEAR;
    if( year > UPPER_PERSIAN_YEAR)
      year = UPPER_PERSIAN_YEAR;
    break;
#endif
  default:       /* undefined calendar */
    return;
    break;
  }  // end switch()

  YearEndDays yed;
  MonthDays md;
  do {
    if( 0 != getCalendarData( year, yed, md, calendar ) )
      return;

    if( yed[0] > jd)
      year--;

    if( yed[1] <= jd)
      year++;

  } while( yed[0] > jd || yed[1] <= jd );

  long currJd = yed[0];
  month = -1;
  for( int i = 0; i < 13; i++) {
    day = int( jd - currJd );
    if( day < md[i] ) {
      month = i + 1;
      day++;
      return;
    }
    currJd += long( md[i] );
  }
  return;
}


//----------------------------------------------------------------------------
// Determine DST start JD (first Sunday in April)
//
long DateOps::dstStart(int year)
{
  long jdStart = dmyToDay( 1, 4, year, T_GREGORIAN );
  while ( 6 != (jdStart % 7 ) ) // Sunday
    jdStart++;

  return jdStart;
}

//----------------------------------------------------------------------------
// Determine DST end JD (last Sunday in October)
//
long DateOps::dstEnd(int year)
{
  long jdEnd = dmyToDay( 31, 10, year, T_GREGORIAN );
  while ( 6 != (jdEnd % 7 ) ) // Sunday
    jdEnd--;

  return jdEnd;
}

//----------------------------------------------------------------------------
