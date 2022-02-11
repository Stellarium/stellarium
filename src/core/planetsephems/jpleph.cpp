/* jpleph.cpp: JPL ephemeris functions

Copyright (C) 2011, Project Pluto

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.    */

/*****************************************************************************
*        *****    jpl planetary and lunar ephemerides    *****     C ver.1.2 *
******************************************************************************
*                                                                            *
*  This program was written in standard fortran-77 and it was manually       *
*  translated to C language by Piotr A. Dybczynski (dybol@phys.amu.edu.pl),  *
*  subsequently revised heavily by Bill J Gray (pluto@gwi.net),  just short  *
*  of a total re-write.                                                      *
*                                                                            *
******************************************************************************
*                 Last modified: July 23, 1997 by PAD                        *
******************************************************************************
21 Apr 2010:  Revised by Bill J. Gray.  The code now determines the kernel
size,  then allocates memory accordingly.  This should 'future-proof' us in
case JPL (or someone else) creates kernels that are larger than the previously
arbitrary MAX_KERNEL_SIZE parameter.  'swap_long' and 'swap_double' have
been replaced with 'swap_32_bit_val' and 'swap_64_bit_val'.  It also works
on 64-bit compiles now.

16 Mar 2001:  Revised by Bill J. Gray.  You can now use binary
ephemerides with either byte order ('big-endian' or 'small-endian');
the code checks to see if the data is in the "wrong" order for the
current platform,  and swaps bytes on-the-fly if needed.  (Yes,  this
can result in a slowdown... sometimes as much as 1%.  The function is
so mathematically intensive that the byte-swapping is the least of our
troubles.)  You can also use DE-200, 403, 404, 405,  or 406 without
recompiling (the constan() function now determines which ephemeris is
in use and its byte order).

Also,  I did some minor optimization of the interp() (Chebyshev
interpolation) function,  resulting in a bit of a speedup.

The code has been modified to be a separately linkable component,  with
details of the implementation encapsulated.
*****************************************************************************/
// Make Large File Support work also on ARM boards, explicitly.
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "StelUtils.hpp"
/**** include variable and type definitions, specific for this C version */

#include "jpleph.h"
#include "jpl_int.h"

#define TRUE 1
#define FALSE 0


// GZ patches for Large File Support for DE431 past AD10100...
#if defined(Q_OS_WIN)
#define FSeek(__FILE, __OFFSET, _MODE) _fseeki64(__FILE, __OFFSET, _MODE)
#else
#define FSeek(__FILE, __OFFSET, _MODE) fseeko(__FILE, __OFFSET, _MODE)
#endif


double DLL_FUNC jpl_get_double(const void *ephem, const int value)
{
	return(*reinterpret_cast<const double *>(static_cast<const char *>(ephem) + value));
}

long DLL_FUNC jpl_get_long(const void *ephem, const int value)
{
	return(*reinterpret_cast<const int32_t *>(static_cast<const char *>(ephem) + value));
}


/*****************************************************************************
**           jpl_pleph(ephem,et,ntar,ncent,rrd,calc_velocity)              **
******************************************************************************
**                                                                          **
**    This subroutine reads the jpl planetary ephemeris                     **
**    and gives the position and velocity of the point 'ntarg'              **
**    with respect to 'ncent'.                                              **
**                                                                          **
**    Calling sequence parameters:                                          **
**                                                                          **
**      et = (double) julian ephemeris date at which interpolation          **
**           is wanted.                                                     **
**                                                                          **
**    ntarg = integer number of 'target' point.                             **
**                                                                          **
**    ncent = integer number of center point.                               **
**                                                                          **
**    The numbering convention for 'ntarg' and 'ncent' is:                  **
**                                                                          **
**            1 = mercury           8 = neptune                             **
**            2 = venus             9 = pluto                               **
**            3 = earth            10 = moon                                **
**            4 = mars             11 = sun                                 **
**            5 = jupiter          12 = solar-system barycenter             **
**            6 = saturn           13 = earth-moon barycenter               **
**            7 = uranus           14 = nutations (longitude and obliq)     **
**                                 15 = librations, if on eph. file         **
**                                 16 = lunar mantle omega_x,omega_y,omega_z**
**                                 17 = TT-TDB, if on eph. file             **
**                                                                          **
**            (If nutations are wanted, set ntarg = 14.                     **
**             For librations, set ntarg = 15. set ncent= 0.                **
**             For TT-TDB,  set ntarg = 17.  I've not actually              **
**             seen an ntarg = 16 case yet.)                                **
**                                                                          **
**     rrd = output 6-element, double array of position and velocity        **
**           of point 'ntarg' relative to 'ncent'. The units are au and     **
**           au/day. For librations the units are radians and radians       **
**           per day. In the case of nutations the first four words of      **
**           rrd will be set to nutations and rates, having units of        **
**           radians and radians/day.                                       **
**                                                                          **
**           The option is available to have the units in km and km/sec.    **
**           for this, set km=TRUE at the beginning of the program.         **
**                                                                          **
**     calc_velocity = integer flag;  if nonzero,  velocities will be       **
**           computed,  otherwise not.                                      **
**                                                                          **
*****************************************************************************/
int DLL_FUNC jpl_pleph(void *ephem, const double et, const int ntarg,
                      const int ncent, double rrd[], const int calc_velocity)
{
    struct jpl_eph_data *eph = static_cast<struct jpl_eph_data *>(ephem);
    double pv[13][6]={{0.}};/* pv is the position/velocity array
                             NUMBERED FROM ZERO: 0=Mercury,1=Venus,...
                             8=Pluto,9=Moon,10=Sun,11=SSBary,12=EMBary
                             First 10 elements (0-9) are affected by
                             jpl_state(), all are adjusted here.         */


    int rval = 0;
    const int list_val = (calc_velocity ? 2 : 1);
    unsigned i;
    int list[14];    /* list is a vector denoting, for which "body"
                            ephemeris values should be calculated by
                            jpl_state():  0=Mercury,1=Venus,2=EMBary,...,
                            8=Pluto,  9=geocentric Moon, 10=nutations in
                            long. & obliq.  11= lunar librations;
                            12 = TT-TDB, 13=lunar mantle omegas */

    for(i = 0; i < 6; ++i) rrd[i] = 0.0;

    if(ntarg == ncent) return(0);

    for(i = 0; i < sizeof(list) / sizeof(list[0]); i++)
      list[i] = 0;

         /* Because of the whacko indexing in JPL ephemerides,  we need */
         /* to work out way through the following indexing schemes :   */
         /* ntarg       ipt         list     */
         /*  14          11          10      Nutations */
         /*  15          12          11      Librations */
         /*  16          13          12      Lunar mantle angular vel */
         /*  17          14          13      TT - TDB */

    for(i = 0; i < 4; i++)
      if(ntarg == static_cast<int>(i) + 14)
      {
        if(eph->ipt[i + 11][1] > 0) /* quantity is in ephemeris */
        {
          list[i + 10] = list_val;

	  // GZ: Coverity Scan (travis) automatic test chokes on next line with:
	  // CID 134920:  Memory - corruptions  (OVERRUN)
	  //     Overrunning array "list" of 56 bytes by passing it to a function
	  //     which accesses it at byte offset 56.
	  // I see it does explicitly NOT access list[14].
	  // TODO: check again after next round of travis.
          rval = jpl_state(ephem, et, list, pv, rrd, 0);
        }
        else          /*  quantity doesn't exist in the ephemeris file  */
          rval = JPL_EPH_QUANTITY_NOT_IN_EPHEMERIS;
        return(rval);
      }
    if(ntarg > 13 || ncent > 13 || ntarg < 1 || ncent < 1)
      return(JPL_EPH_INVALID_INDEX);

/*  force barycentric output by 'state'     */

/*  set up proper entries in 'list' array for state call     */

    for(i = 0; i < 2; i++) /* list[] IS NUMBERED FROM ZERO ! */
    {
      const unsigned k = static_cast<unsigned int>(i ? ncent : ntarg) - 1;

      if(k <= 9) list[k] = list_val;   /* Major planets */
      if(k == 9) list[2] = list_val;   /* for moon,  earth state is needed */
      if(k == 2) list[9] = list_val;   /* for earth,  moon state is needed */
      if(k == 12) list[2] = list_val;  /* EMBary state additionally */
    }

  /*   make call to state   */
   rval = jpl_state(eph, et, list, pv, rrd, 1);
   /* Solar System barycentric Sun state goes to pv[10][] */
   if(ntarg == 11 || ncent == 11)
      for(i = 0; i < 6; i++)
         pv[10][i] = eph->pvsun[i];

   /* Solar System Barycenter coordinates & velocities equal to zero */
   if(ntarg == 12 || ncent == 12)
      for(i = 0; i < 6; i++)
         pv[11][i] = 0.0;

   /* Solar System barycentric EMBary state:  */
   if(ntarg == 13 || ncent == 13)
      for(i = 0; i < 6; i++)
         pv[12][i] = pv[2][i];

   /* if moon from earth or earth from moon ..... */
   if((ntarg*ncent) == 30 && (ntarg+ncent) == 13)
      for(i = 0; i < 6; ++i) pv[2][i]=0.0;
   else
      {
      if(list[2])           /* calculate earth state from EMBary */
	 for(i = 0; i < static_cast<unsigned int>(list[2]) * 3u; ++i)
            pv[2][i] -= pv[9][i]/(1.0+eph->emrat);

      if(list[9]) /* calculate Solar System barycentric moon state */
	 for(i = 0; i < static_cast<unsigned int>(list[9]) * 3u; ++i)
            pv[9][i] += pv[2][i];
      }

   for(i = 0; i < static_cast<unsigned int>(list_val) * 3u; ++i)
      rrd[i] = pv[ntarg-1][i] - pv[ncent-1][i];

   return(rval);
}

/* Some notes about the information stored in 'iinfo':  the posn_coeff[]
array contains the Chebyshev polynomials for tc,

posn_coeff[i]=T (tc).
               i

The vel_coeff[] array contains the derivatives of the same polynomials,

vel_coeff[i]=T'(tc).
              i

   Evaluating these polynomials is a little expensive,  and we don't want
to evaluate any more than we have to.  (Some planets require many more
Chebyshev polynomials than others.)  So if 'tc' is unchanged,  we can
rest assured that 'n_posn_avail' Chebyshev polynomials,  and 'n_vel_avail'
derivatives of Chebyshev polynomials,  have already been evaluated,  and
we start from there,  using the recurrence formulae

T (x) = 1      T (x) = x      T   (x) = 2xT (x) - T   (x)
 0              1              n+1         n       n-1

T'(x) = 0      T'(x) = 1      T'  (x) = 2xT'(x) + 2T (x) - T'   (x)
 0              1              n+1         n        n        n-1

   (the second set being just the derivatives of the first).  To get the
_acceleration_ of an object,  we just keep going and get the second
derivatives as

T"(x) = 0      T"(x) = 1      T"  (x) = 2xT"(x) + 4T'(x) - T"   (x)
 0              1              n+1         n        n        n-1

   At present, i can range from 0 to 17.  If future JPL ephems require
Chebyshev polynomials beyond T  ,  those arrays may need to be expanded.
                              17                                            */

/*****************************************************************************
**                     interp(buf,t,ncf,ncm,na,ifl,pv)                      **
******************************************************************************
**                                                                          **
**    this subroutine differentiates and interpolates a                     **
**    set of chebyshev coefficients to give position and velocity           **
**                                                                          **
**    calling sequence parameters:                                          **
**                                                                          **
**      input:                                                              **
**                                                                          **
**      iinfo   stores certain chunks of interpolation info,  in hopes      **
**              that if you call again with similar parameters,  the        **
**              function won't have to re-compute all coefficients/data.    **
**                                                                          **
**       coef   1st location of array of d.p. chebyshev coefficients        **
**              of position                                                 **
**                                                                          **
**          t   t[0] is double fractional time in interval covered by       **
**              coefficients at which interpolation is wanted               **
**              (0 <= t[0] <= 1).  t[1] is dp length of whole               **
**              interval in input time units.                               **
**                                                                          **
**        ncf   # of coefficients per component                             **
**                                                                          **
**        ncm   # of components per set of coefficients                     **
**                                                                          **
**         na   # of sets of coefficients in full array                     **
**              (i.e., # of sub-intervals in full interval)                 **
**                                                                          **
**         ifl  integer flag: =1 for positions only                         **
**                            =2 for pos and vel                            **
**                            =3 for pos, vel, accel (currently used for    **
**                               pvsun only)                                **
**                                                                          **
**      output:                                                             **
**                                                                          **
**    posvel   interpolated quantities requested.  dimension                **
**              expected is posvel[ncm*ifl], double precision.              **
**                                                                          **
*****************************************************************************/
static void interp(struct interpolation_info *iinfo,
        const double coef[], const double t[2], const unsigned ncf, const unsigned ncm,
        const unsigned na, const int velocity_flag, double posvel[])
{
    const double dna = static_cast<double>(na);
    const double temp = dna * t[0];
    unsigned l = static_cast<unsigned>(temp);
    double vfac, unused_temp1;
    double tc = 2.0 * modf(temp, &unused_temp1) - 1.0;
    unsigned i, j;

    assert(ncf < MAX_CHEBY);
    if(l == na)
    {
      l--;
      tc = 1.;
    }
    assert(tc >= -1.);
    assert(tc <=  1.);

/*  check to see whether chebyshev time has changed,  and compute new
    polynomial values if it has.
    (the element iinfo->posn_coeff[1] is the value of t1[tc] and hence
    contains the value of tc on the previous call.)     */


    if(tc != iinfo->posn_coeff[1])
    {
      iinfo->n_posn_avail = 2;
      iinfo->n_vel_avail = 2;
      iinfo->posn_coeff[1] = tc;
      iinfo->twot = tc+tc;
    }

/*  be sure that at least 'ncf' polynomials have been evaluated and are
    stored in the array 'iinfo->posn_coeff'.  Note that we start out with
    posn_coeff[0] = 1. and posn_coeff[1] = tc (see 'jpl_init_ephemeris'
    below),  and vel_coeff[0] and [1] are similarly preset.  We do that
    because you need the first two coeffs of those series to start the
    Chebyshev recurrence;  see the comments above this function.   */

    if(iinfo->n_posn_avail < ncf)
    {
      double *pc_ptr = iinfo->posn_coeff + iinfo->n_posn_avail;

      for(i=ncf - iinfo->n_posn_avail; i; i--, pc_ptr++)
         *pc_ptr = iinfo->twot * pc_ptr[-1] - pc_ptr[-2];
      iinfo->n_posn_avail=ncf;
    }

/*  interpolate to get position for each component  */

    for(i = 0; i < ncm; ++i)        /* ncm is a number of coordinates */
    {
      const double *coeff_ptr = coef + ncf * (i + l * ncm + 1);
      const double *pc_ptr = iinfo->posn_coeff + ncf;

      *posvel = 0.0;
      for(j = ncf; j; j--)
         *posvel += (*--pc_ptr) * (*--coeff_ptr);
      posvel++;
    }

    if(velocity_flag <= 1) return;

/*  if velocity interpolation is wanted, be sure enough
    derivative polynomials have been generated and stored.    */

    if(iinfo->n_vel_avail < ncf)
    {
      double *vc_ptr = iinfo->vel_coeff + iinfo->n_vel_avail;
      const double *pc_ptr = iinfo->posn_coeff + iinfo->n_vel_avail - 1;

      for(i = ncf - iinfo->n_vel_avail; i; i--, vc_ptr++, pc_ptr++)
         *vc_ptr = iinfo->twot * vc_ptr[-1] + *pc_ptr + *pc_ptr - vc_ptr[-2];
      iinfo->n_vel_avail = ncf;
    }

/*  interpolate to get velocity for each component    */

    vfac = (dna + dna) / t[1];
    for(i = 0; i < ncm; ++i)
    {
      double tval = 0.;
      const double *coeff_ptr = coef + ncf * (i + l * ncm + 1);
      const double *vc_ptr = iinfo->vel_coeff + ncf;

      for(j = ncf - 1; j; j--)
         tval += (*--vc_ptr) * (*--coeff_ptr);
      *posvel++ = tval * vfac;
    }

            /* Accelerations are rarely computed -- at present,  only */
            /* for pvsun -- so we don't get so tricky in optimizing.  */
            /* The accel_coeffs (the second derivative of the Chebyshev */
            /* polynomials) are not stored for repeated use,  for example. */
    if(velocity_flag == 3)
    {
      double accel_coeffs[MAX_CHEBY];

      accel_coeffs[0] = accel_coeffs[1] = 0.;
      for(i = 2; i < ncf; i++)              /* recurrence for T"(x) */
         accel_coeffs[i] = 4. * iinfo->vel_coeff[i - 1]
                        + iinfo->twot * accel_coeffs[i - 1]
                        - accel_coeffs[i - 2];

      for(i = 0; i < ncm; ++i)        /* ncm is a number of coordinates */
      {
         double tval = 0.;
         const double *coeff_ptr = coef + ncf * (i + l * ncm + 1);
         const double *ac_ptr = accel_coeffs + ncf;

         for(j = ncf; j; j--)
            tval += (*--ac_ptr) * (*--coeff_ptr);
         *posvel++ = tval * vfac * vfac;
      }
    }
    
    return;
}

/* swap_32_bit_val() and swap_64_bit_val() are used when reading a binary
ephemeris that was created on a machine with 'opposite' byte order to
the currently-used machine (signalled by the 'swap_bytes' flag in the
jpl_eph_data structure).  In such cases,  every double and integer
value read from the ephemeris must be byte-swapped by these two functions. */

#define SWAP_MACRO(A, B, TEMP)   { TEMP = A;  A = B;  B = TEMP; }

static void swap_32_bit_val(void *ptr)
{
   char *tptr = static_cast<char *>(ptr), tchar;

   SWAP_MACRO(tptr[0], tptr[3], tchar)
   SWAP_MACRO(tptr[1], tptr[2], tchar)
}

static void swap_64_bit_val(void *ptr, long count)
{
    char *tptr = static_cast<char *>(ptr), tchar;
    
    while(count--)
    {
      SWAP_MACRO(tptr[0], tptr[7], tchar)
      SWAP_MACRO(tptr[1], tptr[6], tchar)
      SWAP_MACRO(tptr[2], tptr[5], tchar)
      SWAP_MACRO(tptr[3], tptr[4], tchar)

      tptr += 8;
    }
}

/* Most ephemeris quantities have a dimension of three.  Planet positions
have an x, y, and z;  librations and lunar mantle angles have three Euler
angles.  But TDT-TT is a single quantity,  and nutation is expressed as
two angles.   */

static unsigned int dimension(const unsigned int idx)
{
    unsigned int rval;

    if(idx == 11)             /* Nutations */
      rval = 2;
    else if(idx == 14)        /* TDT - TT */
      rval = 1;
    else                       /* planets, lunar mantle angles, librations */
      rval = 3;
    return(rval);
}

/*****************************************************************************
**                        jpl_state(ephem,et2,list,pv,nut,bary)             **
******************************************************************************
** This subroutine reads and interpolates the jpl planetary ephemeris file  **
**                                                                          **
**    Calling sequence parameters:                                          **
**                                                                          **
**    Input:                                                                **
**                                                                          **
**        et2[] double, 2-element JED epoch at which interpolation          **
**              is wanted.  Any combination of et2[0]+et2[1] which falls    **
**              within the time span on the file is a permissible epoch.    **
**                                                                          **
**               a. for ease in programming, the user may put the           **
**                  entire epoch in et2[0] and set et2[1]=0.0               **
**                                                                          **
**               b. for maximum interpolation accuracy, set et2[0] =        **
**                  the most recent midnight at or before interpolation     **
**                  epoch and set et2[1] = fractional part of a day         **
**                  elapsed between et2[0] and epoch.                       **
**                                                                          **
**               c. as an alternative, it may prove convenient to set       **
**                  et2[0] = some fixed epoch, such as start of integration,**
**                  and et2[1] = elapsed interval between then and epoch.   **
**                                                                          **
**       list   13-element integer array specifying what interpolation      **
**              is wanted for each of the "bodies" on the file.             **
**                                                                          **
**                        list[i]=0, no interpolation for body i            **
**                               =1, position only                          **
**                               =2, position and velocity                  **
**                                                                          **
**              the designation of the astronomical bodies by i is:         **
**                                                                          **
**                        i = 0: mercury                                    **
**                          = 1: venus                                      **
**                          = 2: earth-moon barycenter                      **
**                          = 3: mars                                       **
**                          = 4: jupiter                                    **
**                          = 5: saturn                                     **
**                          = 6: uranus                                     **
**                          = 7: neptune                                    **
**                          = 8: pluto                                      **
**                          = 9: geocentric moon                            **
**                          =10: nutations in lon & obliq (if on file)      **
**                          =11: lunar librations (if on file)              **
**                          =12: lunar mantle omegas                        **
**                          =13: TT-TDB (if on file)                        **
**                                                                          **
** Note that I've not actually seen case 12 yet.  It probably doesn't work. **
**                                                                          **
**    output:                                                               **
**                                                                          **
**    pv[][6]   double array that will contain requested interpolated       **
**              quantities.  The body specified by list[i] will have its    **
**              state in the array starting at pv[i][0]  (on any given      **
**              call, only those words in 'pv' which are affected by the    **
**              first 10 'list' entries (and by list(11) if librations are  **
**              on the file) are set.  The rest of the 'pv' array           **
**              is untouched.)  The order of components in pv[][] is:       **
**              pv[][0]=x,....pv[][5]=dz.                                   **
**                                                                          **
**              All output vectors are referenced to the earth mean         **
**              equator and equinox of epoch. The moon state is always      **
**              geocentric; the other nine states are either heliocentric   **
**              or solar-system barycentric, depending on the setting of    **
**              global variables (see below).                               **
**                                                                          **
**              Lunar librations, if on file, are put into pv[10][k] if     **
**              list[11] is 1 or 2.                                         **
**                                                                          **
**        nut   dp 4-word array that will contain nutations and rates,      **
**              depending on the setting of list[10].  the order of         **
**              quantities in nut is:                                       **
**                                                                          **
**                       d psi  (nutation in longitude)                     **
**                       d epsilon (nutation in obliquity)                  **
**                       d psi dot                                          **
**                       d epsilon dot                                      **
**                                                                          **
*****************************************************************************/
int DLL_FUNC jpl_state(void *ephem, const double et, const int list[14],
                          double pv[][6], double nut[4], const int bary)
{
	struct jpl_eph_data *eph = static_cast<struct jpl_eph_data *>(ephem);
	unsigned i, j, n_intervals;
	uint32_t nr;
	double *buf = eph->cache;
	double t[2];
	const double block_loc = (et - eph->ephem_start) / eph->ephem_step;
	bool recompute_pvsun;
	const double aufac = 1.0 / eph->au;

	/*   error return for epoch out of range  */
	if(et < eph->ephem_start || et > eph->ephem_end)
		return(JPL_EPH_OUTSIDE_RANGE);

	/*   calculate record # and relative time in interval   */

	nr = static_cast<uint32_t>(block_loc);
	t[0] = block_loc - static_cast<double>(nr);
	if(t[0]==0.0 && nr)
	{
		t[0] = 1.;
		nr--;
	}

	/*   read correct record if not in core (static vector buf[])   */
	if(nr != eph->curr_cache_loc)
	{
		eph->curr_cache_loc = nr;
		/* Read two blocks ahead to account for header: */
		if(FSeek(eph->ifile, static_cast<unsigned long>(nr + 2) * eph->recsize, SEEK_SET)) // lgtm [cpp/integer-multiplication-cast-to-long]
		{
			// GZ: Make sure we will try again on next call...
			eph->curr_cache_loc=0;
			return(JPL_EPH_FSEEK_ERROR);
		}
		if(fread(buf, sizeof(double), static_cast<size_t>(eph->ncoeff), eph->ifile)
				!= static_cast<size_t>(eph->ncoeff))
			return(JPL_EPH_READ_ERROR);

		if(eph->swap_bytes)
			swap_64_bit_val(buf, eph->ncoeff);
	}
	t[1] = eph->ephem_step;

	if(eph->pvsun_t != et)   /* If several calls are made for the same et, */
	{                      /* don't recompute pvsun each time... only on */
		recompute_pvsun = true;   /* the first run through.                     */
		eph->pvsun_t = et;
	}
	else
		recompute_pvsun = false;

	/* Here, i loops through the "traditional" 14 listed items -- 10
	  solar system objects,  nutations,  librations,  lunar mantle angles,
	  and TT-TDT -- plus a fifteenth:  the solar system barycenter.  That
	  last is quite different:  it's computed 'as needed',  rather than
	  from list[];  the output goes to pvsun rather than the pv array;
	  and three quantities (position,  velocity,  acceleration) are
	  computed (nobody else gets accelerations at present.)  */
	for(n_intervals = 1; n_intervals <= 8; n_intervals *= 2)
		for(i = 0; i < 15; i++)
		{
			unsigned quantities;
			uint32_t *iptr = &eph->ipt[i + 1][0];

			if(i == 14)
			{
				quantities = (recompute_pvsun ? 3 : 0);
				iptr = &eph->ipt[10][0];
			}
			else
			{
				quantities = static_cast<unsigned int>(list[i]);
				iptr = &eph->ipt[i < 10 ? i : i + 1][0];
			}
			if(n_intervals == iptr[2] && quantities)
			{
				double *dest = ((i == 10) ? eph->pvsun : pv[i]);

				if(i < 10)
					dest = pv[i];
				else if(i == 14)
					dest = eph->pvsun;
				else
					dest = nut;

				interp(&eph->iinfo, &buf[iptr[0]-1], t, static_cast<unsigned int>(iptr[1]),
						dimension(i + 1),
						n_intervals, static_cast<int>(quantities), dest);

				if(i < 10 || i == 14)        /*  convert km to AU */
					for(j = 0; j < quantities * 3; j++)
						dest[j] *= aufac;
			}
		}
	if(!bary)                             /* gotta correct everybody for */
		for(i = 0; i < 9; i++)            /* the solar system barycenter */
			for(j = 0; j < static_cast<unsigned>(list[i]) * 3; j++)
				pv[i][j] -= eph->pvsun[j];
	return(0);
}

static int init_err_code = JPL_INIT_NOT_CALLED;

int DLL_FUNC jpl_init_error_code(void)
{
   return(init_err_code);
}

const char * jpl_init_error_message(void)
{
  switch(init_err_code)
  {
    case 0:
      return "JPL_INIT_NO_ERROR";
    case -1:
      return "JPL_INIT_FILE_NOT_FOUND";
    case -2:
      return "JPL_INIT_FSEEK_FAILED";
    case -3:
      return "JPL_INIT_FREAD_FAILED";
    case -4:
      return "JPL_INIT_FREAD2_FAILED";
    case -5:
      return "JPL_INIT_FILE_CORRUPT";
    case -6:
      return "JPL_INIT_MEMORY_FAILURE";
    case -7:
      return "JPL_INIT_FREAD3_FAILED";
    case -8:
      return "JPL_INIT_FREAD4_FAILED";
    case -9:
      return "JPL_INIT_NOT_CALLED";
    case -10:
      return "JPL_INIT_FREAD5_FAILED";
    default:
      return "ERROR_NOT_RECOGNIZED";
  }
}

   /* DE-430 has 572 constants.  That's more than the 400 constants */
   /* originally expected.  The remaining 172 are stored after the  */
   /* other header data :                                           */

#define START_400TH_CONSTANT_NAME   (84 * 3 + 400 * 6 + 5 * sizeof(double) \
                                                    + 41 * sizeof(int32_t))

   /* ...which comes out to 2856.  See comments in 'jpl_int.h'.   */

/****************************************************************************
**    jpl_init_ephemeris(ephemeris_filename, nam, val, n_constants)       **
*****************************************************************************
**                                                                         **
**    this function does the initial prep work for use of binary JPL       **
**    ephemerides.                                                         **
**      const char *ephemeris_filename = full path/filename of the binary  **
**          ephemeris (on the Willmann-Bell CDs,  this is UNIX.200, 405,   **
**          or 406)                                                        **
**      char nam[][6] = array of constant names (max 6 characters each)    **
**          You can pass nam=NULL if you don't care about the names        **
**      double *val = array of values of constants                         **
**          You can pass val=NULL if you don't care about the constants    **
**      Return value is a pointer to the jpl_eph_data structure            **
**      NULL is returned if the file isn't opened or memory isn't alloced  **
**      Errors can be determined with the above jpl_init_error_code()     **
****************************************************************************/

void * DLL_FUNC jpl_init_ephemeris(const char *ephemeris_filename,
                          char nam[][6], double *val)
{
    unsigned i, j;
    unsigned long de_version;
    char title[84];
    FILE *ifile = fopen(ephemeris_filename, "rb");

    struct jpl_eph_data *rval;
    struct jpl_eph_data temp_data;

    init_err_code = 0;
    temp_data.ifile = ifile;
    if(!ifile)
      init_err_code = JPL_INIT_FILE_NOT_FOUND;
    else if(fread(title, 84, 1, ifile) != 1)
      init_err_code = JPL_INIT_FREAD_FAILED;
    else if(FSeek(ifile, 2652L, SEEK_SET))
      init_err_code = JPL_INIT_FSEEK_FAILED;
    else if(fread(&temp_data, JPL_HEADER_SIZE, 1, ifile) != 1)
      init_err_code = JPL_INIT_FREAD2_FAILED;

    if(init_err_code)
    {
      if(ifile)
        fclose(ifile);
      return(Q_NULLPTR);
    }

    de_version = strtoul(title+26, Q_NULLPTR, 10);
    
    /* A small piece of trickery:  in the binary file,  data is stored */
    /* for ipt[0...11],  then the ephemeris version,  then the         */
    /* remaining ipt[12] data.  A little switching is required to get  */
    /* the correct order. */
    temp_data.ipt[12][0] = temp_data.ipt[12][1];
    temp_data.ipt[12][1] = temp_data.ipt[12][2];
    temp_data.ipt[12][2] = temp_data.ipt[13][0];
    temp_data.ephemeris_version = de_version;

    //qDebug() << "DE_Version: " << de_version;


    temp_data.swap_bytes = (temp_data.ncon > 65536L);
    if(temp_data.swap_bytes)     /* byte order is wrong for current platform */
    {
      qDebug() << "Byte order is wrong for current platform";
      
      swap_64_bit_val(&temp_data.ephem_start, 1);
      swap_64_bit_val(&temp_data.ephem_end, 1);
      swap_64_bit_val(&temp_data.ephem_step, 1);
      swap_32_bit_val(&temp_data.ncon);
      swap_64_bit_val(&temp_data.au, 1);
      swap_64_bit_val(&temp_data.emrat, 1);
    }

            /* It's a little tricky to tell if an ephemeris really has  */
            /* TT-TDB data (with offsets in ipt[13][] and ipt[14][]).   */
            /* Essentially,  we read the data and sanity-check it,  and */
            /* zero it if it "doesn't add up" correctly.                */
            /*    Also:  certain ephems I've generated with ncon capped */
            /* at 400 have no TT-TDB data.  So if ncon == 400,  don't   */
            /* try to read such data;  you may get garbage.             */
    if(de_version >= 430 && temp_data.ncon != 400)
    {
         /* If there are 400 or fewer constants,  data for ipt[13][0...2] */
         /* immediately follows that for ipt[12][0..2];  i.e.,  we don't  */
         /* need to fseek().  Otherwise,  we gotta skip 6*(n_constants-400) */
         /* bytes. */
      if(temp_data.ncon > 400)
      {
	if ( FSeek(ifile, static_cast<long long>(static_cast<size_t>(temp_data.ncon - 400) * 6), SEEK_CUR) != 0)
	{
	      qWarning() << "jpl_init_ephemeris(): Cannot seek in file. Result will be undefined.";
	}
      }
      if(fread(&temp_data.ipt[13][0], sizeof(int32_t), 6, ifile) != 6)
         init_err_code = JPL_INIT_FREAD5_FAILED;
    }
    else                 /* mark header data as invalid */
      temp_data.ipt[13][0] = static_cast<uint32_t>(-1);

    if(temp_data.swap_bytes)     /* byte order is wrong for current platform */
    {  
        for(j = 0; j < 3; j++)
        {
            for(i = 0; i < 15; i++)
            { 
                swap_32_bit_val(&temp_data.ipt[i][j]);
            }
        }
    }

    if(temp_data.ipt[13][0] !=       /* if these don't add up correctly, */
          temp_data.ipt[12][0] + temp_data.ipt[12][1] * temp_data.ipt[12][2] * 3
    || temp_data.ipt[14][0] !=       /* zero them out (they've garbage data) */
          temp_data.ipt[13][0] + temp_data.ipt[13][1] * temp_data.ipt[13][2] * 3)
    {     /* not valid pointers to TT-TDB data */
      memset(&temp_data.ipt[13][0], 0, 6 * sizeof(int32_t));
    }

         /* A sanity check:  if the earth-moon ratio is outside reasonable */
         /* bounds,  we must be looking at a wrong or corrupted file.      */
         /* In DE-102,  emrat = 81.3007;  in DE-405/406, emrat = 81.30056. */
         /* Those are the low and high ranges.  We'll allow some slop in   */
         /* case the earth/moon mass ratio changes:                        */
    if(temp_data.emrat > 81.3008 || temp_data.emrat < 81.30055)
    {   
      init_err_code = JPL_INIT_FILE_CORRUPT;
      qWarning() << "temp_data: " << temp_data.emrat << "(should have been =~81). JPL_INIT_FILE_CORRUPT!";
    }

    if(init_err_code)
    {
      fclose(ifile);
      return(Q_NULLPTR);
    }

         /* Once upon a time,  the kernel size was determined from the */
         /* DE version.  This was not a terrible idea,  except that it */
         /* meant that when the code faced a new version,  it broke.   */
         /* Now we use some logic to compute the kernel size.          */
    temp_data.kernel_size = 4;
    for(i = 0; i < 15; i++)
      temp_data.kernel_size +=
	     2 * temp_data.ipt[i][1] * temp_data.ipt[i][2] * dimension(i);
// for(i = 0; i < 13; i++)
//    temp_data.kernel_size +=
//                     temp_data.ipt[i][1] * temp_data.ipt[i][2] * ((i == 11) ? 4 : 6);
//       /* ...and then add in space required for the TT-TDB data : */
// temp_data.kernel_size += temp_data.ipt[14][1] * temp_data.ipt[14][2] * 2;
    temp_data.recsize = temp_data.kernel_size * 4L;
    temp_data.ncoeff = temp_data.kernel_size / 2L;

               /* Rather than do two separate allocations,  everything     */
               /* we need is allocated in _one_ chunk,  then parceled out. */
               /* This looks a little weird,  but it simplifies error      */
               /* handling and cleanup.                                    */
    rval = static_cast<struct jpl_eph_data *>(calloc(sizeof(struct jpl_eph_data)
			+ temp_data.recsize, 1));
    if(!rval)
    {
      init_err_code = JPL_INIT_MEMORY_FAILURE;
      fclose(ifile);
      return(Q_NULLPTR);
    }
    memcpy(rval, &temp_data, sizeof(struct jpl_eph_data));
    rval->iinfo.posn_coeff[0] = 1.0;
            /* Seed a bogus value here.  The first and subsequent calls to */
            /* 'interp' will correct it to a value between -1 and +1.      */
    rval->iinfo.posn_coeff[1] = -2.0;
    rval->iinfo.vel_coeff[0] = 0.0;
    rval->iinfo.vel_coeff[1] = 1.0;
    rval->curr_cache_loc = static_cast<uint32_t>(-1);
    
              /* The 'cache' data is right after the 'jpl_eph_data' struct: */
    rval->cache = reinterpret_cast<double *>(rval + 1);
               /* If there are more than 400 constants,  the names of       */
               /* the extra constants are stored in what would normally     */
               /* be zero-padding after the header record.  However,        */
               /* older ephemeris-reading software won't know about that.   */
               /* So we store ncon=400,  then actually check the names to   */
               /* see how many constants there _really_ are.  Older readers */
               /* will just see 400 names and won't know about the others.  */
               /* But on the upside, they won't crash.                      */

    if(rval->ncon == 400)
    {
      char buff[7];

      buff[6] = '\0';
      FSeek(ifile, START_400TH_CONSTANT_NAME, SEEK_SET);
      while(fread(buff, 6, 1, ifile) && strlen(buff) == 6)
      {
         rval->ncon++;
      }
    }

    if(val)
    {
      FSeek(ifile, rval->recsize, SEEK_SET);
      if(fread(val, sizeof(double), static_cast<size_t>(rval->ncon), ifile)
			!= static_cast<size_t>(rval->ncon))
         init_err_code = JPL_INIT_FREAD3_FAILED;
      else if(rval->swap_bytes)     /* gotta swap the constants,  too */
         swap_64_bit_val(val, rval->ncon);
      }

   if(!init_err_code && nam)
      {
      FSeek(ifile, 84L * 3L, SEEK_SET);   /* just after the 3 'title' lines */
      for(i = 0; i < rval->ncon && !init_err_code; i++)
      {
        if(i == 400)
	  FSeek(ifile, START_400TH_CONSTANT_NAME, SEEK_SET);
        if(fread(nam[i], 6, 1, ifile) != 1)
          init_err_code = JPL_INIT_FREAD4_FAILED;
        }
      }
  return(rval);
}

/****************************************************************************
**    jpl_close_ephemeris(ephem)                                          **
*****************************************************************************
**                                                                         **
**    this function closes files and frees up memory allocated by the      **
**    jpl_init_ephemeris() function.                                      **
****************************************************************************/
void DLL_FUNC jpl_close_ephemeris(void *ephem)
{
   struct jpl_eph_data *eph = static_cast<struct jpl_eph_data *>(ephem);

   fclose(eph->ifile);
   free(ephem);
}

/* Added 2011 Jan 18:  random access to any desired JPL constant */


double DLL_FUNC jpl_get_constant(const int idx, void *ephem, char *constant_name)
{
	struct jpl_eph_data *eph = static_cast<struct jpl_eph_data *>(ephem);
	double rval = 0.;

	*constant_name = '\0';
	if(idx >= 0 && idx < static_cast<int>(eph->ncon))
	{
		// GZ extended from const long to const long long
		const long long seek_loc = (idx < 400 ? 84L * 3L + static_cast<long>(idx) * 6 :
							static_cast<long long>(START_400TH_CONSTANT_NAME) + (idx - 400) * 6);

		if (FSeek(eph->ifile, seek_loc, SEEK_SET) != 0)
		{
			qWarning() << "jpl_get_constant(): Cannot seek in file. Result will be undefined.";
		}
		if(fread(constant_name, 1, 6, eph->ifile) == 6 )
		{
			constant_name[6] = '\0';
			if (FSeek(eph->ifile, static_cast<long long>(eph->recsize) + static_cast<long long>(idx) * static_cast<long long>(sizeof(double)), SEEK_SET) != 0)
			{
				qWarning() << "jpl_get_constant(): Cannot seek in file (call2). Result will be undefined.";
			}

			// GZ added tests to make travis test suite (Coverity Scan) happier
			if( fread(&rval, 1, sizeof(double), eph->ifile) == sizeof(double) )
			{
				if(eph->swap_bytes)     /* gotta swap the constants,  too */
					swap_64_bit_val(&rval, 1);
			}
			else
				qWarning() << "jpl_get_constant(): fread() failed to read a double. Result will be undefined.";
		}
		else
			qWarning() << "jpl_get_constant(): fread() failed to read a constant name. Result will be 0, likely wrong.";
	}
	return(rval);
}
/*************************** THE END ***************************************/
