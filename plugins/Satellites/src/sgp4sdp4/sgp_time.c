/*
 * Unit SGP_Time
 *       Author:  Dr TS Kelso
 * Original Version:  1992 Jun 02
 * Current Revision:  2000 Jan 22
 * Modified for Y2K:  1999 Mar 07
 *          Version:  2.05
 *        Copyright:  1992-1999, All Rights Reserved
 * Version 1.50 added Y2K support. Due to limitations in the current
 * format of the NORAD two-line element sets, however, only dates
 * through 2056 December 31/2359 UTC are valid.
 * Version 1.60 modifies Calendar_Date to ensure date matches time
 * resolution and modifies Time_of_Day to make it more robust.
 * Version 2.00 adds Julian_Date, Date_Time, and Check_Date to support
 * checking for valid date/times, permitting the use of Time_to_UTC and
 * Time_from_UTC for UTC/local time conversions.
 * Version 2.05 modifies UTC_offset to allow non-integer offsets.
 *
 *   Ported to C by: Neoklis Kyriazis  April 9  2001
 */

#include "sgp4sdp4.h"

/* The function Julian_Date_of_Epoch returns the Julian Date of     */
/* an epoch specified in the format used in the NORAD two-line      */
/* element sets. It has been modified to support dates beyond       */
/* the year 1999 assuming that two-digit years in the range 00-56   */
/* correspond to 2000-2056. Until the two-line element set format   */
/* is changed, it is only valid for dates through 2056 December 31. */

double
Julian_Date_of_Epoch(double epoch)
{ 
  double year,day;

  /* Modification to support Y2K */
  /* Valid 1957 through 2056     */
  day = modf(epoch*1E-3, &year)*1E3;
  if( year < 57 )
    year = year + 2000;
  else
    year = year + 1900;
  /* End modification */

  return( Julian_Date_of_Year(year) + day );
} /*Function Julian_Date_of_Epoch*/

/*------------------------------------------------------------------*/

/* Converts a Julian epoch to NORAD TLE epoch format */
double
Epoch_Time(double jd)
{  
  double yr,_time,epoch_time;
  struct tm edate;

  Calendar_Date(jd, &edate);
  yr = edate.tm_year - 100*(edate.tm_year/100) ;
  _time = Frac(jd + 0.5);
  epoch_time =  yr*1000
                + DOY(edate.tm_year, edate.tm_mon, edate.tm_mday)
                + _time;

  return( epoch_time );
} /*Function Epoch_Time*/

/*------------------------------------------------------------------*/

/* The function DOY calculates the day of the year for the specified */
/* date. The calculation uses the rules for the Gregorian calendar   */
/* and is valid from the inception of that calendar system.          */
int
DOY(int yr, int mo, int dy)
{
  const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  int i,day;

  day = 0;
  for( i = 0; i < mo-1; i++ )
    day += days[i];
  day = day + dy;

  /* Leap year correction */
  if( 
     (yr%4 == 0) && ((yr%100 != 0) || (yr%400 == 0)) && (mo>2)
     )
    day++;

  return( day );
} /*Function DOY*/

/*------------------------------------------------------------------*/

/* Fraction_of_Day calculates the fraction of */
/* a day passed at the specified input time.  */
double
Fraction_of_Day(int hr,int mi,int se)
{
  return( (hr + (mi + se/60.0)/60.0)/24.0 );
} /*Function Fraction_of_Day*/

/*------------------------------------------------------------------*/

/* The function Calendar_Date converts a Julian Date to a struct tm.   */
/* Only the members tm_year, tm_mon and tm_mday are calculated and set */
void
Calendar_Date(double jd, struct tm *cdate)
{
  /* Astronomical Formulae for Calculators, Jean Meeus, pages 26-27 */
  int Z,month;
  double A,B,C,D,E,F,alpha,day,year,factor;

  factor = 0.5/secday/1000;
  F = Frac(jd + 0.5);
  if (F + factor >= 1.0)
    {
      jd = jd + factor;
      F  = 0.0;
    } /*if*/
  Z = Round(jd);
  if( Z < 2299161 )
    A = Z;
  else
    {
      alpha = Int((Z - 1867216.25)/36524.25);
      A = Z + 1 + alpha - Int(alpha/4);
    } /*else*/
  B = A + 1524;
  C = Int((B - 122.1)/365.25);
  D = Int(365.25 * C);
  E = Int((B - D)/30.6001);
  day = B - D - Int(30.6001 * E) + F;

  if( E < 13.5 )
    month = Round(E - 1);
  else
    month = Round(E - 13);
  if( month > 2.5 )
    year = C - 4716;
  else
    year = C - 4715;

  cdate->tm_year = (int) year;
  cdate->tm_mon = month;
  cdate->tm_mday = (int) floor(day);

} /*Function Calendar_Date*/

/*------------------------------------------------------------------*/

/* Time_of_Day takes a Julian Date and calculates the clock time */
/* portion of that date. Only tm_hour, tm_min and tm_sec are set */
void
Time_of_Day(double jd, struct tm *cdate)
{
  int hr,mn,sc;
  double _time;

  _time = Frac(jd - 0.5)*secday;
  _time = Round(_time);
  hr = floor(_time/3600.0);
  _time = _time - 3600.0*hr;
  if( hr == 24 ) hr = 0;
  mn = floor(_time/60.0);
  sc = _time - 60.0*mn;
  cdate->tm_hour = hr;
  cdate->tm_min = mn;
  cdate->tm_sec = sc;

} /*Function Time_of_Day*/

/*------------------------------------------------------------------*/

/* The function Julian_Date converts a standard calendar   */
/* date and time to a Julian Date. The procedure Date_Time */
/* performs the inverse of this function. */
double
Julian_Date(struct tm *cdate)
{
  double julian_date;

  julian_date = Julian_Date_of_Year(cdate->tm_year) + 
                DOY(cdate->tm_year,cdate->tm_mon,cdate->tm_mday) +
                Fraction_of_Day(cdate->tm_hour,cdate->tm_min,cdate->tm_sec);

  return( julian_date );
} /*Function Julian_Date */

/*------------------------------------------------------------------*/


/*  Date_Time()
 *
 *  The function Date_Time() converts a Julian Date to
 *  standard calendar date and time. The function
 *  Julian_Date() performs the inverse of this function.
 */

void
Date_Time(double julian_date, struct tm *cdate)
{
  time_t jtime;

  jtime = (julian_date - 2440587.5)*86400.;
  *cdate = *gmtime( &jtime );

} /* End of Date_Time() */


/*------------------------------------------------------------------*/

/* The procedure Check_Date can be used as a check to see if a calendar    */
/* date and time are valid. It works by first converting the calendar      */
/* date and time to a Julian Date (which allows for irregularities, such   */
/* as a time greater than 24 hours) and then converting back and comparing.*/
int
Check_Date(struct tm *cdate)
{
  double jt;
  struct tm chkdate;

  jt = Julian_Date(cdate);
  Date_Time(jt, &chkdate);

  if( (cdate->tm_year == chkdate.tm_year) &&
      (cdate->tm_mon  == chkdate.tm_mon ) &&
      (cdate->tm_mday == chkdate.tm_mday) &&
      (cdate->tm_hour == chkdate.tm_hour) &&
      (cdate->tm_min  == chkdate.tm_min ) &&
      (cdate->tm_sec  == chkdate.tm_sec ) )
    return ( 1 );
  else
    return( 0 );

} /*Procedure Check_Date*/

/*------------------------------------------------------------------*/

/* Procedures Time_to_UTC and Time_from_UTC are used to  */
/* convert 'struct tm' dates between UTC and local time. */
/* The procedures JD_to_UTC and JD_from_UTC are used to  */
/* do the same thing working directly with Julian dates. */

struct tm
Time_to_UTC(struct tm *cdate)
{
  time_t tdate;

  tdate = mktime(cdate);
  return( *gmtime(&tdate) );
} /*Procedure Time_to_UTC*/

/*------------------------------------------------------------------*/

struct tm 
Time_from_UTC(struct tm *cdate)
{
  time_t tdate;

  tdate = mktime(cdate);
  return( *localtime(&tdate) );
} /*Procedure Time_from_UTC*/

/*------------------------------------------------------------------*/

/*
 BSD systems don't define the timezone variable, so the following two
 routines won't work.  They're not used anyway in the example main(),
 so we might as well comment them out.  
*/

#if 0

double
JD_to_UTC(double jt)
{
  extern long timezone;
  struct tm cdate;

  time_t t = 0;

  cdate = *localtime( &t );
  jt = jt - timezone/secday;
  if( cdate.tm_isdst )
    jt= jt - 1.0/24.0;

  return( jt );
} /*Procedure JD_to_UTC*/

/*------------------------------------------------------------------*/

double
JD_from_UTC(double jt)
{
  extern long timezone;
  struct tm cdate;
  time_t t = 0;

  cdate = *localtime( &t );
  jt = jt + timezone/secday;
  if( cdate.tm_isdst )
    jt= jt + 1.0/24.0;

  return( jt );
} /*Procedure JD_from_UTC*/

#endif

/*------------------------------------------------------------------*/

/* The function Delta_ET has been added to allow calculations on   */
/* the position of the sun.  It provides the difference between UT */
/* (approximately the same as UTC) and ET (now referred to as TDT).*/
/* This function is based on a least squares fit of data from 1950 */
/* to 1991 and will need to be updated periodically. */

double
Delta_ET(double year)
{
  /* Values determined using data from 1950-1991 in the 1990 
     Astronomical Almanac.  See DELTA_ET.WQ1 for details. */

  double delta_et;

  delta_et = 26.465 + 0.747622*(year - 1950) +
             1.886913*sin(twopi*(year - 1975)/33);

  return( delta_et );
} /*Function Delta_ET*/

/*------------------------------------------------------------------*/

/* The function Julian_Date_of_Year calculates the Julian Date  */
/* of Day 0.0 of {year}. This function is used to calculate the */
/* Julian Date of any date by using Julian_Date_of_Year, DOY,   */
/* and Fraction_of_Day. */

double
Julian_Date_of_Year(double year)
{
  /* Astronomical Formulae for Calculators, Jean Meeus, */
  /* pages 23-25. Calculate Julian Date of 0.0 Jan year */

  long A,B,i;
  double jdoy;

  year = year-1;
  i = year/100;
  A = i;
  i = A/4;
  B = 2-A+i;
  i = 365.25*year;
  i += 30.6001*14;
  jdoy = i+1720994.5+B;

  return (jdoy);
}  /*Function Julian_Date_of_Year*/

/*------------------------------------------------------------------*/

/* The function ThetaG calculates the Greenwich Mean Sidereal Time */
/* for an epoch specified in the format used in the NORAD two-line */
/* element sets. It has now been adapted for dates beyond the year */
/* 1999, as described above. The function ThetaG_JD provides the   */
/* same calculation except that it is based on an input in the     */
/* form of a Julian Date. */

double
ThetaG(double epoch, deep_arg_t *deep_arg)
{
/* Reference:  The 1992 Astronomical Almanac, page B6. */

  double year,day,UT,jd,TU,GMST,_ThetaG;

/* Modification to support Y2K */
/* Valid 1957 through 2056     */
  day = modf(epoch*1E-3,&year)*1E3;
  if(year < 57)
    year += 2000;
  else
    year += 1900;
  /* End modification */

  UT   = modf(day,&day);
  jd   = Julian_Date_of_Year(year)+day;
  TU   = (jd-2451545.0)/36525;
  GMST = 24110.54841+TU*(8640184.812866+TU*(0.093104-TU* 6.2E-6));
  GMST = Modulus(GMST+secday*omega_E*UT,secday);
  _ThetaG = twopi*GMST/secday;
  deep_arg->ds50 = jd-2433281.5+UT;
  _ThetaG = FMod2p(6.3003880987*deep_arg->ds50+1.72944494);

  return (_ThetaG);
} /* Function ThetaG */

/*------------------------------------------------------------------*/

double
ThetaG_JD(double jd)
{
/* Reference:  The 1992 Astronomical Almanac, page B6. */

  double UT,TU,GMST;

  UT   = Frac(jd + 0.5);
  jd   = jd - UT;
  TU   = (jd - 2451545.0)/36525;
  GMST = 24110.54841 + TU * (8640184.812866 + TU * (0.093104 - TU * 6.2E-6));
  GMST = Modulus(GMST + secday*omega_E*UT,secday);

  return( twopi * GMST/secday );
} /*Function ThetaG_JD*/

/*------------------------------------------------------------------*/

/* Gets calendar time from time() and produces a UTC calendar date */
void
UTC_Calendar_Now( struct tm *cdate )
{
  time_t t;

  t = time(0);
  *cdate = *gmtime(&t);
  cdate->tm_year += 1900;
  cdate->tm_mon += 1;

} /* End UTC_Calendar_Now */
/*------------------------------------------------------------------*/
