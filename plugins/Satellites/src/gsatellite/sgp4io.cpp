/*     ----------------------------------------------------------------
*
*                               sgp4io.cpp
*
*    this file contains a function to read two line element sets. while 
*    not formerly part of the sgp4 mathematical theory, it is 
*    required for practical implemenation.
*
*                            companion code for
*               fundamentals of astrodynamics and applications
*                                    2007
*                              by david vallado
*
*       (w) 719-573-2600, email dvallado@agi.com
*
*    Note: Deleted code related with command line inputs. That implementation
*          is not a god way to build a library. It's better to aisle logic and interface.
*          J.L. Canales 20/11/2010
*
*    current :
*               3 sep 08  david vallado
*                           add operationmode for afspc (a) or improved (i)
*    changes :
*               9 may 07  david vallado
*                           fix year correction to 57
*              27 mar 07  david vallado
*                           misc fixes to manual inputs
*              14 aug 06  david vallado
*                           original baseline
*       ----------------------------------------------------------------      */

#include "sgp4io.h"

/* -----------------------------------------------------------------------------
*
*                           function twoline2rv
*
*  this function converts the two line element set character string data to
*    variables and initializes the sgp4 variables. several intermediate varaibles
*    and quantities are determined. note that the result is a structure so multiple
*    satellites can be processed simultaneously without having to reinitialize. the
*    verification mode is an important option that permits quick checks of any
*    changes to the underlying technical theory. this option works using a
*    modified tle file in which the start, stop, and delta time values are
*    included at the end of the second line of data. this only works with the
*    verification mode. the catalog mode simply propagates from -1440 to 1440 min
*    from epoch and is useful when performing entire catalog runs.
*
*  author        : david vallado                  719-573-2600    1 mar 2001
*
*  inputs        :
*    longstr1    - first line of the tle
*    longstr2    - second line of the tle
*    typerun     - type of run                    verification 'v', catalog 'c', 
*                                                 manual 'm'
*    typeinput   - type of manual input           mfe 'm', epoch 'e', dayofyr 'd'
*    opsmode     - mode of operation afspc or improved 'a', 'i'
*    whichconst  - which set of constants to use  72, 84
*
*  outputs       :
*    satrec      - structure containing all the sgp4 satellite information
*
*  coupling      :
*    getgravconst-
*    days2mdhms  - conversion of days to month, day, hour, minute, second
*    jday        - convert day month year hour minute second into julian date
*    sgp4init    - initialize the sgp4 variables
*
*  references    :
*    norad spacetrack report #3
*    vallado, crawford, hujsak, kelso  2006
  --------------------------------------------------------------------------- */

void twoline2rv
     (
      char      longstr1[130], char longstr2[130],
      char      typerun,  char typeinput, char opsmode,
      gravconsttype       whichconst,
      double& startmfe, double& stopmfe, double& deltamin,
      elsetrec& satrec
     )
     {
       const double deg2rad  =   M_PI / 180.0;         //   0.0174532925199433
       const double xpdotp   =  1440.0 / (2.0 *M_PI);  // 229.1831180523293

       double sec, mu, radiusearthkm, tumin, xke, j2, j3, j4, j3oj2;
       int cardnumb, numb, j;
       long revnum = 0, elnum = 0;
       char classification, intldesg[11];
       int year = 0;
       int mon, day, hr, minute, nexp, ibexp;

       getgravconst( whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );

       satrec.error = 0;

       // set the implied decimal points since doing a formated read
       // fixes for bad input data values (missing, ...)
       for (j = 10; j <= 15; j++)
           if (longstr1[j] == ' ')
               longstr1[j] = '_';

       if (longstr1[44] != ' ')
           longstr1[43] = longstr1[44];
       longstr1[44] = '.';
       if (longstr1[7] == ' ')
           longstr1[7] = 'U';
       if (longstr1[9] == ' ')
           longstr1[9] = '.';
       for (j = 45; j <= 49; j++)
           if (longstr1[j] == ' ')
               longstr1[j] = '0';
       if (longstr1[51] == ' ')
           longstr1[51] = '0';
       if (longstr1[53] != ' ')
           longstr1[52] = longstr1[53];
       longstr1[53] = '.';
       longstr2[25] = '.';
       for (j = 26; j <= 32; j++)
           if (longstr2[j] == ' ')
               longstr2[j] = '0';
       if (longstr1[62] == ' ')
           longstr1[62] = '0';
       if (longstr1[68] == ' ')
           longstr1[68] = '0';

       sscanf(longstr1,"%2d %5ld %1c %10s %2d %12lf %11lf %7lf %2d %7lf %2d %2d %6ld ",
                       &cardnumb,&satrec.satnum,&classification, intldesg, &satrec.epochyr,
                       &satrec.epochdays,&satrec.ndot, &satrec.nddot, &nexp, &satrec.bstar,
                       &ibexp, &numb, &elnum );

       if (typerun == 'v')  // run for specified times from the file
         {
           if (longstr2[52] == ' ')
               sscanf(longstr2,"%2d %5ld %9lf %9lf %8lf %9lf %9lf %10lf %6ld %lf %lf %lf \n",
                       &cardnumb,&satrec.satnum, &satrec.inclo,
                       &satrec.nodeo,&satrec.ecco, &satrec.argpo, &satrec.mo, &satrec.no,
                       &revnum, &startmfe, &stopmfe, &deltamin );
             else
               sscanf(longstr2,"%2d %5ld %9lf %9lf %8lf %9lf %9lf %11lf %6ld %lf %lf %lf \n",
                       &cardnumb,&satrec.satnum, &satrec.inclo,
                       &satrec.nodeo,&satrec.ecco, &satrec.argpo, &satrec.mo, &satrec.no,
                       &revnum, &startmfe, &stopmfe, &deltamin );
         }
         else  // simply run -1 day to +1 day or user input times
         {
           if (longstr2[52] == ' ')
               sscanf(longstr2,"%2d %5ld %9lf %9lf %8lf %9lf %9lf %10lf %6ld \n",
                       &cardnumb,&satrec.satnum, &satrec.inclo,
                       &satrec.nodeo,&satrec.ecco, &satrec.argpo, &satrec.mo, &satrec.no,
                       &revnum );
             else
               sscanf(longstr2,"%2d %5ld %9lf %9lf %8lf %9lf %9lf %11lf %6ld \n",
                       &cardnumb,&satrec.satnum, &satrec.inclo,
                       &satrec.nodeo,&satrec.ecco, &satrec.argpo, &satrec.mo, &satrec.no,
                       &revnum );
         }

       // ---- find no, ndot, nddot ----
       satrec.no   = satrec.no / xpdotp; //* rad/min
       satrec.nddot= satrec.nddot * pow(10.0, nexp);
       satrec.bstar= satrec.bstar * pow(10.0, ibexp);

       // ---- convert to sgp4 units ----
       satrec.a    = pow( satrec.no*tumin , (-2.0/3.0) );
       satrec.ndot = satrec.ndot  / (xpdotp*1440.0);  //* ? * minperday
       satrec.nddot= satrec.nddot / (xpdotp*1440.0*1440);

       // ---- find standard orbital elements ----
       satrec.inclo = satrec.inclo  * deg2rad;
       satrec.nodeo = satrec.nodeo  * deg2rad;
       satrec.argpo = satrec.argpo  * deg2rad;
       satrec.mo    = satrec.mo     * deg2rad;

       satrec.alta = satrec.a*(1.0 + satrec.ecco) - 1.0;
       satrec.altp = satrec.a*(1.0 - satrec.ecco) - 1.0;

       // ----------------------------------------------------------------
       // find sgp4epoch time of element set
       // remember that sgp4 uses units of days from 0 jan 1950 (sgp4epoch)
       // and minutes from the epoch (time)
       // ----------------------------------------------------------------

       // ---------------- temp fix for years from 1957-2056 -------------------
       // --------- correct fix will occur when year is 4-digit in tle ---------
       if (satrec.epochyr < 57)
           year= satrec.epochyr + 2000;
         else
           year= satrec.epochyr + 1900;

       days2mdhms ( year,satrec.epochdays, mon,day,hr,minute,sec );
       jday( year,mon,day,hr,minute,sec, satrec.jdsatepoch );

        // ------------ perform complete catalog evaluation, -+ 1 day -----------
       if (typerun == 'c')
         {
           startmfe = -1440.0;
           stopmfe  =  1440.0;
           deltamin =    10.0;
         }

       // ---------------- initialize the orbit at sgp4epoch -------------------
       sgp4init( whichconst, opsmode, satrec.satnum, satrec.jdsatepoch-2433281.5, satrec.bstar,
                 satrec.ecco, satrec.argpo, satrec.inclo, satrec.mo, satrec.no,
                 satrec.nodeo, satrec);
    } // end twoline2rv


