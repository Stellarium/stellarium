/*     ----------------------------------------------------------------
*
*                               sgp4ext.cpp
*
*    this file contains extra routines needed for the main test program for sgp4.
*    these routines are derived from the astro libraries.
*
*                            companion code for
*               fundamentals of astrodynamics and applications
*                                    2007
*                              by david vallado
*
*       (w) 719-573-2600, email dvallado@agi.com
*
*    current :
*               7 may 08  david vallado
*                           fix sgn
*    changes :
*               2 apr 07  david vallado
*                           fix jday floor and str lengths
*                           updates for constants
*              14 aug 06  david vallado
*                           original baseline
*       ----------------------------------------------------------------      */

#include "sgp4ext.h"
#include "StelUtils.hpp"

/* -----------------------------------------------------------------------------
*
*                           function mag
*
*  this procedure finds the magnitude of a vector.  the tolerance is set to
*    0.000001, thus the 1.0e-12 for the squared test of underflows.
*
*  author        : david vallado                  719-573-2600    1 mar 2001
*
*  inputs          description                    range / units
*    vec         - vector
*
*  outputs       :
*    vec         - answer stored in fourth component
*
*  locals        :
*    none.
*
*  coupling      :
*    none.
* --------------------------------------------------------------------------- */

double mag(double x[3])
{
	return std::sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
}  // end mag

/* -----------------------------------------------------------------------------
*
*                           procedure cross
*
*  this procedure crosses two vectors.
*
*  author        : david vallado                  719-573-2600    1 mar 2001
*
*  inputs          description                    range / units
*    vec1        - vector number 1
*    vec2        - vector number 2
*
*  outputs       :
*    outvec      - vector result of a x b
*
*  locals        :
*    none.
*
*  coupling      :
*    mag           magnitude of a vector
 ---------------------------------------------------------------------------- */

void cross(double vec1[3], double vec2[3], double outvec[3])
{
	outvec[0]= vec1[1]*vec2[2] - vec1[2]*vec2[1];
	outvec[1]= vec1[2]*vec2[0] - vec1[0]*vec2[2];
	outvec[2]= vec1[0]*vec2[1] - vec1[1]*vec2[0];
}  // end cross


/* -----------------------------------------------------------------------------
*
*                           function dot
*
*  this function finds the dot product of two vectors.
*
*  author        : david vallado                  719-573-2600    1 mar 2001
*
*  inputs          description                    range / units
*    vec1        - vector number 1
*    vec2        - vector number 2
*
*  outputs       :
*    dot         - result
*
*  locals        :
*    none.
*
*  coupling      :
*    none.
*
* --------------------------------------------------------------------------- */

double dot(double x[3], double y[3])
{
	return (x[0]*y[0] + x[1]*y[1] + x[2]*y[2]);
}  // end dot

/* -----------------------------------------------------------------------------
*
*                           procedure angle
*
*  this procedure calculates the angle between two vectors.  the output is
*    set to 999999.1 to indicate an undefined value.  be sure to check for
*    this at the output phase.
*
*  author        : david vallado                  719-573-2600    1 mar 2001
*
*  inputs          description                    range / units
*    vec1        - vector number 1
*    vec2        - vector number 2
*
*  outputs       :
*    theta       - angle between the two vectors  -pi to pi
*
*  locals        :
*    temp        - temporary real variable
*
*  coupling      :
*    dot           dot product of two vectors
* --------------------------------------------------------------------------- */

double angle(double vec1[3], double vec2[3])
{
	double sv, undefined, magv1, magv2, temp;
	sv        = 0.00000001;
	undefined = 999999.1;

	magv1 = mag(vec1);
	magv2 = mag(vec2);

	if (magv1*magv2 > sv*sv)
	{
		temp= dot(vec1,vec2) / (magv1*magv2);
		if ( std::fabs( temp ) > 1.0)
			temp = StelUtils::sign(temp) * 1.0;

		return std::acos( temp );
	}

	return undefined;
}  // end angle

/* -----------------------------------------------------------------------------
*
*                           function newtonnu
*
*  this function solves keplers equation when the true anomaly is known.
*    the mean and eccentric, parabolic, or hyperbolic anomaly is also found.
*    the parabolic limit at 168 degree is arbitrary. the hyperbolic anomaly is also
*    limited. the hyperbolic sine is used because it's not double valued.
*
*  author        : david vallado                  719-573-2600   27 may 2002
*
*  revisions
*    vallado     - fix small                                     24 sep 2002
*
*  inputs          description                    range / units
*    ecc         - eccentricity                   0.0  to
*    nu          - true anomaly                   -2pi to 2pi rad
*
*  outputs       :
*    e0          - eccentric anomaly              0.0  to 2pi rad       153.02 degree
*    m           - mean anomaly                   0.0  to 2pi rad       151.7425 degree
*
*  locals        :
*    e1          - eccentric anomaly, next value  rad
*    sine        - sine of e
*    cose        - cosine of e
*    ktr         - index
*
*  coupling      :
*    asinh       - arc hyperbolic sine
*
*  references    :
*    vallado       2007, 85, alg 5
* --------------------------------------------------------------------------- */

void newtonnu(double ecc, double nu, double& e0, double& m)
{
	double sv, sine, cose;

	// ---------------------  implementation   ---------------------
	e0 = 999999.9;
	m  = 999999.9;
	sv = 0.00000001;

	if ( std::fabs( ecc ) < sv  ) // circular
	{
		m  = nu;
		e0 = nu;
	}
	else if ( ecc < 1.0-sv  ) // elliptical
	{
		sine = ( std::sqrt( 1.0 -ecc*ecc ) * std::sin(nu) ) / ( 1.0 +ecc*std::cos(nu) );
		cose = ( ecc + std::cos(nu) ) / ( 1.0  + ecc*std::cos(nu) );
		e0   = std::atan2( sine,cose );
		m    = e0 - ecc*std::sin(e0);
	}
	else if ( ecc > 1.0 + sv  ) // hyperbolic
	{
		if (std::fabs(nu)+0.00001 < M_PI-std::acos(1.0 /ecc))
		{
			sine = ( std::sqrt( ecc*ecc-1.0  ) * std::sin(nu) ) / ( 1.0  + ecc*std::cos(nu) );
			#ifdef _MSC_BUILD
			e0   = StelUtils::asinh( sine );
			#else
			e0   = std::asinh( sine );
			#endif
			m    = ecc*std::sinh(e0) - e0;
		}
	}
	else if ( std::fabs(nu) < 168.0*M_PI/180.0  ) // parabolic
	{
		e0 = std::tan( nu*0.5  );
		m  = e0 + (e0*e0*e0)/3.0;
	}

	if ( ecc < 1.0  )
	{
		m = std::fmod( m,2.0 *M_PI );
		if ( m < 0.0  )
			m = m + 2.0 *M_PI;

		e0 = std::fmod( e0,2.0 *M_PI );
	}
}  // end newtonnu


/* -----------------------------------------------------------------------------
*
*                           function rv2coe
*
*  this function finds the classical orbital elements given the geocentric
*    equatorial position and velocity vectors.
*
*  author        : david vallado                  719-573-2600   21 jun 2002
*
*  revisions
*    vallado     - fix special cases                              5 sep 2002
*    vallado     - delete extra check in inclination code        16 oct 2002
*    vallado     - add constant file use                         29 jun 2003
*    vallado     - add mu                                         2 apr 2007
*
*  inputs          description                    range / units
*    r           - ijk position vector            km
*    v           - ijk velocity vector            km / s
*    mu          - gravitational parameter        km3 / s2
*
*  outputs       :
*    p           - semilatus rectum               km
*    a           - semimajor axis                 km
*    ecc         - eccentricity
*    incl        - inclination                    0.0  to pi rad
*    omega       - longitude of ascending node    0.0  to 2pi rad
*    argp        - argument of perigee            0.0  to 2pi rad
*    nu          - true anomaly                   0.0  to 2pi rad
*    m           - mean anomaly                   0.0  to 2pi rad
*    arglat      - argument of latitude      (ci) 0.0  to 2pi rad
*    truelon     - true longitude            (ce) 0.0  to 2pi rad
*    lonper      - longitude of periapsis    (ee) 0.0  to 2pi rad
*
*  locals        :
*    hbar        - angular momentum h vector      km2 / s
*    ebar        - eccentricity     e vector
*    nbar        - line of nodes    n vector
*    c1          - v**2 - u/r
*    rdotv       - r dot v
*    hk          - hk unit vector
*    sme         - specfic mechanical energy      km2 / s2
*    i           - index
*    e           - eccentric, parabolic,
*                  hyperbolic anomaly             rad
*    temp        - temporary variable
*    typeorbit   - type of orbit                  ee, ei, ce, ci
*
*  coupling      :
*    mag         - magnitude of a vector
*    cross       - cross product of two vectors
*    angle       - find the angle between two vectors
*    newtonnu    - find the mean anomaly
*
*  references    :
*    vallado       2007, 126, alg 9, ex 2-5
* --------------------------------------------------------------------------- */

void rv2coe(double r[3], double v[3], double mu,
	    double& p, double& a, double& ecc, double& incl, double& omega, double& argp,
	    double& nu, double& m, double& arglat, double& truelon, double& lonper)
{
	double undefined, sv, hbar[3], nbar[3], magr, magv, magn, ebar[3], sme,
	       rdotv, infinite, temp, c1, hk, twopi, magh, halfpi, e;

	int i;
	char typeorbit[3];

	twopi     = 2.0 * M_PI;
	halfpi    = 0.5 * M_PI;
	sv        = 0.00000001;
	undefined = 999999.1;
	infinite  = 999999.9;

	// -------------------------  implementation   -----------------
	magr = mag( r );
	magv = mag( v );

	// ------------------  find h n and e vectors   ----------------
	cross( r,v, hbar );
	magh = mag( hbar );
	if ( magh > sv )
	{
		nbar[0] = -hbar[1];
		nbar[1] =  hbar[0];
		nbar[2] =  0.0;
		magn    = mag( nbar );
		c1      = magv*magv - mu /magr;
		rdotv   = dot( r,v );
		for (i= 0; i <= 2; i++)
			ebar[i] = (c1*r[i] - rdotv*v[i])/mu;

		ecc = mag( ebar );

		// ------------  find a e and semi-latus rectum   ----------
		sme = ( magv*magv*0.5  ) - ( mu /magr );
		if ( std::fabs( sme ) > sv )
			a = -mu  / (2.0 *sme);
		else
			a = infinite;

		p = magh*magh/mu;

		// -----------------  find inclination   -------------------
		hk   = hbar[2]/magh;
		incl = std::acos( hk );

		// --------  determine type of orbit for later use  --------
		std::strcpy(typeorbit,"ei"); // elliptical, parabolic, hyperbolic inclined
		if ( ecc < sv )
		{
			if  ((incl<sv) | (std::fabs(incl-M_PI)<sv))
				std::strcpy(typeorbit,"ce"); // circular equatorial
			else
				std::strcpy(typeorbit,"ci"); // circular inclined
		}
		else
		{
			if  ((incl<sv) | (std::fabs(incl-M_PI)<sv))
				std::strcpy(typeorbit,"ee"); // elliptical, parabolic, hyperbolic equatorial
		}

		// ----------  find longitude of ascending node ------------
		if ( magn > sv )
		{
			temp  = nbar[0] / magn;
			if ( std::fabs(temp) > 1.0  )
				temp = StelUtils::sign(temp);
			omega = std::acos( temp );
			if ( nbar[1] < 0.0  )
				omega = twopi - omega;
		}
		else
			omega = undefined;

		// ---------------- find argument of perigee ---------------
		if ( std::strcmp(typeorbit,"ei") == 0 )
		{
			argp = angle( nbar,ebar);
			if ( ebar[2] < 0.0  )
				argp = twopi - argp;
		}
		else
			argp = undefined;

		// ------------  find true anomaly at epoch    -------------
		if ( typeorbit[0] == 'e' )
		{
			nu =  angle( ebar,r);
			if ( rdotv < 0.0  )
				nu= twopi - nu;
		}
		else
			nu = undefined;

		// ----  find argument of latitude - circular inclined -----
		if ( std::strcmp(typeorbit,"ci") == 0 )
		{
			arglat = angle( nbar,r );
			if ( r[2] < 0.0  )
				arglat= twopi - arglat;
			m = arglat;
		}
		else
			arglat = undefined;

		// -- find longitude of perigee - elliptical equatorial ----
		if  (( ecc>sv ) && (std::strcmp(typeorbit,"ee") == 0))
		{
			temp = ebar[0]/ecc;
			if ( std::fabs(temp) > 1.0  )
				temp = StelUtils::sign(temp);
			lonper = std::acos( temp );
			if ( ebar[1] < 0.0  )
				lonper = twopi - lonper;
			if ( incl > halfpi )
				lonper = twopi - lonper;
		}
		else
			lonper = undefined;

		// -------- find true longitude - circular equatorial ------
		if  (( magr>sv ) && ( std::strcmp(typeorbit,"ce") == 0 ))
		{
			temp = r[0]/magr;
			if ( std::fabs(temp) > 1.0  )
				temp = StelUtils::sign(temp);
			truelon = std::acos( temp );
			if ( r[1] < 0.0  )
				truelon = twopi - truelon;
			if ( incl > halfpi )
				truelon = twopi - truelon;
			m = truelon;
		}
		else
			truelon = undefined;

		// ------------ find mean anomaly for all orbits -----------
		if ( typeorbit[0] == 'e' )
			newtonnu(ecc, nu, e, m);
	}
	else
	{
		p       = undefined;
		a       = undefined;
		ecc     = undefined;
		incl    = undefined;
		omega   = undefined;
		argp    = undefined;
		nu      = undefined;
		m       = undefined;
		arglat  = undefined;
		truelon = undefined;
		lonper  = undefined;
	}
}  // end rv2coe

/* -----------------------------------------------------------------------------
*
*                           procedure jday
*
*  this procedure finds the julian date given the year, month, day, and time.
*    the julian date is defined by each elapsed day since noon, jan 1, 4713 bc.
*
*  algorithm     : calculate the answer in one step for efficiency
*
*  author        : david vallado                  719-573-2600    1 mar 2001
*
*  inputs          description                    range / units
*    year        - year                           1900 .. 2100
*    mon         - month                          1 .. 12
*    day         - day                            1 .. 28,29,30,31
*    hr          - universal time hour            0 .. 23
*    min         - universal time min             0 .. 59
*    sec         - universal time sec             0.0 .. 59.999
*
*  outputs       :
*    jd          - julian date                    days from 4713 bc
*
*  locals        :
*    none.
*
*  coupling      :
*    none.
*
*  references    :
*    vallado       2007, 189, alg 14, ex 3-14
*
* --------------------------------------------------------------------------- */

void jday(int year, int mon, int day, int hr, int minute, double sec, double& jd)
{
	jd = 367.0 * year -
	     std::floor((7 * (year + floor((mon + 9) / 12.0))) * 0.25) +
	     std::floor( 275 * mon / 9.0 ) +
	     day + 1721013.5 +
	     ((sec / 60.0 + minute) / 60.0 + hr) / 24.0;  // ut in days
	// - 0.5*StelUtils::sign(100.0*year + mon - 190002.5) + 0.5;
}  // end jday

/* -----------------------------------------------------------------------------
*
*                           procedure days2mdhms
*
*  this procedure converts the day of the year, days, to the equivalent month
*    day, hour, minute and second.
*
*  algorithm     : set up array for the number of days per month
*                  find leap year - use 1900 because 2000 is a leap year
*                  loop through a temp value while the value is < the days
*                  perform int conversions to the correct day and month
*                  convert remainder into h m s using type conversions
*
*  author        : david vallado                  719-573-2600    1 mar 2001
*
*  inputs          description                    range / units
*    year        - year                           1900 .. 2100
*    days        - julian day of the year         0.0  .. 366.0
*
*  outputs       :
*    mon         - month                          1 .. 12
*    day         - day                            1 .. 28,29,30,31
*    hr          - hour                           0 .. 23
*    min         - minute                         0 .. 59
*    sec         - second                         0.0 .. 59.999
*
*  locals        :
*    dayofyr     - day of year
*    temp        - temporary extended values
*    inttemp     - temporary int value
*    i           - index
*    lmonth[12]  - int array containing the number of days per month
*
*  coupling      :
*    none.
* --------------------------------------------------------------------------- */

void days2mdhms(int year, double days, int& mon, int& day, int& hr, int& minute, double& sec)
{
	int i, inttemp, dayofyr;
	double    temp;
	int lmonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	dayofyr = static_cast<int>(std::floor(days));
	/* ----------------- find month and day of month ---------------- */
	if ( (year % 4) == 0 )
		lmonth[1] = 29;

	i = 1;
	inttemp = 0;
	while ((dayofyr > inttemp + lmonth[i-1]) && (i < 12))
	{
		inttemp = inttemp + lmonth[i-1];
		i++;
	}
	mon = i;
	day = dayofyr - inttemp;

	/* ----------------- find hours minutes and seconds ------------- */
	temp = (days - dayofyr) * 24.0;
	hr   = static_cast<int>(std::floor(temp));
	temp = (temp - hr) * 60.0;
	minute  = static_cast<int>(std::floor(temp));
	sec  = (temp - minute) * 60.0;
}  // end days2mdhms

/* -----------------------------------------------------------------------------
*
*                           procedure invjday
*
*  this procedure finds the year, month, day, hour, minute and second
*  given the julian date. tu can be ut1, tdt, tdb, etc.
*
*  algorithm     : set up starting values
*                  find leap year - use 1900 because 2000 is a leap year
*                  find the elapsed days through the year in a loop
*                  call routine to find each individual value
*
*  author        : david vallado                  719-573-2600    1 mar 2001
*
*  inputs          description                    range / units
*    jd          - julian date                    days from 4713 bc
*
*  outputs       :
*    year        - year                           1900 .. 2100
*    mon         - month                          1 .. 12
*    day         - day                            1 .. 28,29,30,31
*    hr          - hour                           0 .. 23
*    min         - minute                         0 .. 59
*    sec         - second                         0.0 .. 59.999
*
*  locals        :
*    days        - day of year plus fractional
*                  portion of a day               days
*    tu          - julian centuries from 0 h
*                  jan 0, 1900
*    temp        - temporary double values
*    leapyrs     - number of leap years from 1900
*
*  coupling      :
*    days2mdhms  - finds month, day, hour, minute and second given days and year
*
*  references    :
*    vallado       2007, 208, alg 22, ex 3-13
* --------------------------------------------------------------------------- */

void invjday(double jd, int& year, int& mon, int& day, int& hr, int& minute, double& sec)
{
	int leapyrs;
	double    days, tu, temp;

	/* --------------- find year and days of the year --------------- */
	temp    = jd - 2415019.5;
	tu      = temp / 365.25;
	year    = 1900 + static_cast<int>(std::floor(tu));
	leapyrs = static_cast<int>(std::floor((year - 1901) * 0.25));

	// optional nudge by 8.64x10-7 sec to get even outputs
	days    = temp - ((year - 1900) * 365.0 + leapyrs) + 0.00000000001;

	/* ------------ check for case of beginning of a year ----------- */
	if (days < 1.0)
	{
		year    = year - 1;
		leapyrs = static_cast<int>(std::floor((year - 1901) * 0.25));
		days    = temp - ((year - 1900) * 365.0 + leapyrs);
	}

	/* ----------------- find remaing data  ------------------------- */
	days2mdhms(year, days, mon, day, hr, minute, sec);
	sec = sec - 0.00000086400;
}  // end invjday
