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

// see general calendarical comments in DateOps.cpp

#if defined( CALENDARS_OF_THE_WORLD )

/* The following mod( ) function returns the _positive_ remainder after */
/* a division.  Annoyingly,  if x < 0,  then x % y <= 0;  thus,  this   */
/* function is needed for things such as determining a day of the week. */

long DateOps::mod( long x, long y )
{
   long rval = x % y;

   if( rval < 0L )
      rval += y;

   return rval;
}

void DateOps::getIslamicYearData( long year, long& daysInYear, MonthDays& md )
      // long *days, char *month_data)
{
   static const long THIRTY_ISLAMIC_YEARS = 10631L;

   static const int isIslamicLeapYear[30] = {
           0, 0, 1, 0, 0, 1, 0, 1, 0, 0,
           1, 0, 0, 1, 0, 0, 1, 0, 1, 0,
           0, 1, 0, 0, 1, 0, 1, 0, 0, 1 };

   long yearWithinCycle = mod( year, 30L );
   long thirtyYearCycles = (year - yearWithinCycle) / 30L;
   long rval = E_ISLAMIC +
               thirtyYearCycles * THIRTY_ISLAMIC_YEARS +
               yearWithinCycle * 354L;

   md[12] = 0;
   md[11] = 29 + isIslamicLeapYear[yearWithinCycle];
   while( yearWithinCycle-- )
      rval += long( isIslamicLeapYear[yearWithinCycle] );

   daysInYear = rval;

   /* The Islamic calendar alternates between 30-day and 29-day */
   /* months for the first eleven months;  the twelfth is 30    */
   /* days in a leap year,  29 otherwise (see above).           */

   for( int i=0; i<11; i++ )
      md[i] = 30 - (i % 2);
}

/* End:  Islamic calendar */


/* Begin:  Hebrew calendar */

/* See p 586,  _Explanatory Supplement_,  for explanation. */
/* There are 1080 Halakim,  or 'parts',  each 3.33 seconds long,  in  */
/* an hour.  So: */

static const int isHebrewLeapYear[19] = { 0, 0, 1, 0, 0, 1,
           0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1 };

long DateOps::lunationsToTishri1( long year )
{
   long yearWithinCycle = mod( year - 1, 19L );

   long fullNineteenYearCycles = ( year - 1 - yearWithinCycle ) / 19L;

   long rval = fullNineteenYearCycles * 235L + yearWithinCycle * 12L;

   for( int y=0; y<yearWithinCycle; y++ )
      rval += isHebrewLeapYear[y];

   return rval;
}

void DateOps::lunationsToDaysAndHalakim( long lunations, long& days, long& halakim )
{
   /*****
   One lunation is 29 days, 13753 halakim long.  To evade integer
   overflow for immense numbers of lunations,  we can use the fact
   that 25920 lunations is exactly 765433 days.  This cycle has no
   name that I know of,  and no real significance outside of this
   particular function... but it _does_ let us write simpler code
   that won't get wrong answers for large or negative numbers of
   lunations.  Let's call 25920 lunations a "glumph."  We figure
   out how many glumphs have passed and our location within that
   glumph,  and the rest is easy.
   *****/

   long lunationWithinGlumph = mod( lunations, 25920L );

   long currGlumph = ( lunations - lunationWithinGlumph ) / 25920L;

   days += currGlumph * 765433L + lunationWithinGlumph * 29L;
   halakim += lunationWithinGlumph * 13753L;

   // Now make sure excess halakim carry over correctly
   days += halakim / HALAKIM_IN_DAY;
   halakim %= HALAKIM_IN_DAY;
}

/* Set days and halakim to the 'epoch':  1 Tishri 1 = 2 5604 */
void DateOps::findTishri1( long year, long& days, long& halakim )
{
   days = 2L;
   halakim = 5604L;
   lunationsToDaysAndHalakim( lunationsToTishri1( year), days, halakim );
}

/* Warning: Certain aspects of getHebrewYearData( ) will definitely fail for
   years before zero... something will have to be done about that.
*/

void DateOps::getHebrewYearData( long year, YearEndDays& daysInYear, MonthDays& md )
{
   for( int i=0; i<2; i++ ) {
      long day, halakim;

      findTishri1( year + i, day, halakim );

      /* Check dehiyyah (c): */
      if( 3 == mod( day, 7L ) &&
          halakim >= 9L * 1080L + 204L &&
          !isHebrewLeapYear[ mod( year - 1 + i, 19L) ]
        )
      {
         day += 2;
      }
      /* Check dehiyyah (d): */
      else if( mod( day, 7L) == 2 &&
               halakim >= 15L * 1080L + 589L &&
               isHebrewLeapYear[ mod( year - 2 + i, 19L) ]
              )
      {
         day++;
      }
      else
      {
         if( halakim > 18L * 1080L )
            day++;

         if( mod( day, 7L ) == 1 ||
             mod( day, 7L ) == 4 ||
             mod( day, 7L ) == 6L
           )
         {
            day++;
         }
      }
      daysInYear[i] = day + E_HEBREW;
   }

   int yearLength = int( daysInYear[1] - daysInYear[0] );

   if( 0 != md[0] ) {
      for( int i=0; i<6; i++ )                 /* "normal" lengths */
         md[i] = md[i + 7] = (char)( 30 - (i & 1));

      if( isHebrewLeapYear[ mod( year - 1, 19L) ] ) {
         md[5] = 30;     /* Adar I is bumped up a day in leap years */
         md[6] = 29;
      }
      else                       /* In non-leap years,  Adar II doesn't    */
         md[6] = 0;      /* exist at all;  set it to zero days     */

      if( yearLength == 353 || yearLength == 383 )      /* deficient year */
         md[2] = 29;

      if( yearLength == 355 || yearLength == 385 )      /* complete year  */
         md[1] = 30;
   }
}

/*  Some test cases:  16 Av 5748 AM (16 12 5748) = 30 Jul 1988 Gregorian */
/*                 14 Nisan 5730 AM (14 8 5730) = 20 Apr 1970 Gregorian */
/*                  1 Tishri 5750 AM (1 1 5750) = 30 Sep 1989 Gregorian */

/* End:  Hebrew calendar */

/* Begin:  (French) Revolutionary calendar */

/*
The French Revolutionary calendar is simplest,  in some respects;
you just have twelve months,  each of 30 days,  with five or six
"unattached" days at the end of the year.  Leap years are those
divisible by four,  except for those divisible by 128.  This slight
deviation from the Gregorian scheme,  of "divisible by four,  unless
divisible by 100,  unless divisible by 400",  is slightly simpler
and gives a calendar that is _much_ closer to the true tropical year.

Unfortunately,  when first devised,  the French attempted to have
New Years Day line up with the autumn equinox,  which is not
particularly regular.  Thus, between 1 AR and 20 AR,  leap years
occurred a year early;  i.e, years 3, 7, 11,  and 15 AR were leap
years;  after that,  they were supposed to revert to the rule
described above.  (There are also claims that leap years were to
follow the Gregorian "4, 100, 400" rule.  I have no real evidence
to support one scheme over the other.  But I suspect that a revolution
so devoted to revising every aspect of human existence that it
changed names of all months,  "regularized" each to be 30 days,  and
made a week ten days long,  probably went out of its way not to
resemble earlier calendars proposed by a Pope.  Also,  the fact that
it would be an almost perfect match to the tropical year would lend
support to the scheme.  Finally,  the irony of the Republic creating
a calendar that would be good for a hundred thousand years appeals
to me,  considering the short life of the Republic itself.  Those
objecting to my choice are free to #define GREGORIAN_REVOLUTIONARY.)

   A 'proleptic' calendar wasn't defined,  to my knowledge...
I've added one based on the logical assumption that zero,  and all
"BR" (Before the Revolution) years divisible by four and not by 128,
are leap;  so (as with all other calendars in this code) negative
years are correctly supported.

*/

long DateOps::jdOfFrenchRevYear( long year )
{
   long rval = E_REVOLUTIONARY + year * 365L;

   if( year >= 20)
      year--;

#ifdef GREGORIAN_REVOLUTIONARY
   rval += long(year / 4 - year / 100 + year / 400);
#else
   rval += long(year / 4 - year / 128);
#endif

   if( year <= 0L )
      rval--;

   return rval;
}

void DateOps::getRevolutionaryYearData( long year, YearEndDays& daysInYear,
                                        MonthDays& md )
{
   daysInYear[0] = jdOfFrenchRevYear( year );
   daysInYear[1] = jdOfFrenchRevYear( year + 1 );

   /* There are twelve months of 30 days each,  followed by
    * five (leap years,  six) days;  call 'em an extra
    * thirteenth "month",  containing all remaining days:
    */
   for( int i=0; i<12; i++ )
      md[i] = 30;

   md[12] = ( daysInYear[1] - daysInYear[0] - 360L );
}

/* End:  (French) Revolutionary calendar */

/* Begin:  Persian (Jalaali) calendar */

long DateOps::jalaliJd0( long jalaliYear )
{
   static const int breaks[12] = { -708, -221,   -3,    6,  394,  720,
                                    786, 1145, 1635, 1701, 1866, 2328 };
   static const int deltas[12] = { 1108, 1047,  984, 1249,  952,  891,
                                   930,  866,  869,  844,  848,  852 };
   long rval = -1L;

   if( jalaliYear >= LOWER_PERSIAN_YEAR ) {

      for( int i = 0; i < 12; i++ ) {
         if( jalaliYear < breaks[i] )
         {
            rval = JALALI_ZERO + jalaliYear * 365L +
                   ( deltas[i] + jalaliYear * 303L ) / 1250L;

            if( i < 3 )  /* zero point drops one day in first three blocks */
               rval--;

            break;
         }
      }
   }
   return rval;
}

void DateOps::getJalaliYearData( long year, YearEndDays& daysInYear, MonthDays& md )
{
   if( year < LOWER_PERSIAN_YEAR || year > UPPER_PERSIAN_YEAR)
      return;

   daysInYear[0] = jalaliJd0( year) + 1L;
   daysInYear[1] = jalaliJd0( year + 1L ) + 1L;

   /* The first six months have 31 days.
    * The next five have 30 days.
    * The last month has 29 days in ordinary years,  30 in leap years.
    */
   for( int i=0; i<6; i++ )
      md[i] = 31;

   for( int i=6; i<11; i++ )
      md[i] = 30;

   md[11] = ( daysInYear[1] - daysInYear[0] - 336L );
}

/* End:  Persian (Jalali) calendar */

#endif  /* #if defined( CALENDARS_OF_THE_WORLD ) */

