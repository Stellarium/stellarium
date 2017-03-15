/* jpl_int.cpp: internal definitions for JPL ephemeris functions

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

            /* A JPL binary ephemeris header contains five doubles and */
            /* (up to) 41 int32_t integers,  so:                          */
#define JPL_HEADER_SIZE (5 * sizeof( double) + 41 * sizeof( int32_t))
            /* ...also known as 5 * 8 + 41 * 4 = 204 bytes.   */


            /* Thus far,  no DE ephems use eighteen terms in the Chebyshev */
            /* expansion.  There's an assert to catch it if this changes.. */
#define MAX_CHEBY          18

#pragma pack(1)

struct interpolation_info
   {
   double posn_coeff[MAX_CHEBY], vel_coeff[MAX_CHEBY], twot;
   unsigned n_posn_avail, n_vel_avail;
   };

struct jpl_eph_data {
   double ephem_start, ephem_end, ephem_step;
   uint32_t ncon;
   double au;
   double emrat;
   uint32_t ipt[15][3];
   uint32_t ephemeris_version;
               /* This is the end of the file header.  Following are */
               /* items computed within my code.                     */
   uint32_t kernel_size, recsize, ncoeff;
   uint32_t swap_bytes;
   uint32_t curr_cache_loc;
   double pvsun[9];
   double pvsun_t;
   double *cache;
   struct interpolation_info iinfo;
   FILE *ifile;
   };
#pragma pack()

/* 2014 Mar 25:  notes about the file structure :

Bytes 0- 83:  first line ("JPL Planetary Ephemeris DExxx/LExxx")
     84-167:  second line ("Start Epoch: JED = ttttttt.t yyyy-MMM-dd 00:00:00")
    168-251:  third line ("Final Epoch: JED = ttttttt.t yyyy-MMM-dd 00:00:00")
    252-257:  first constant name ("DENUM ")
    252+6n to 257+6n:   nth constant name (0 <= n < 399)
    2646-2651:  400th constant name
    2652-2659:   ephem_start
    2660-2667:   ephem_end
    2668-2675:   ephem_step
    2676-2679:   ncon
    2680-2687:   AU in km: close to 149597870.700000
    2688-2695:   Earth/moon mass ratio: about 81.300569
         ...followed by 36 32-bit ints for the above 'ipt' array [12][3]...
    2840-2843:   ephemeris version (405,  430,  etc.)
         ...followed by three 32-bit ints for ipt[12][0...2].
    2844-2855:   ipt[12][0], ipt[12][1], ipt[12][2]
         Note that in the JPL FORTRAN code,
         ipt[12][0..2] become lpt[0..2] (the lunar libration offsets);
         ipt[13][0..2] become rpt[0..2] (the lunar euler angle rate offsets);
         ipt[14][0..2] become tpt[0..2] (the TT-TDB offsets)
         Further note that ipt[13] and [14] are new to DE-430t.  In older
         versions,  they're just zeroes.
    2856-2861:  name of 401th constant
    2862-2867:  name of 402st constant
         ...and after the last constant,  ipt[13][0..2] and ipt[14][0..2].
         If n_constants <= 400,  these will occupy bytes 2856-2879.  Otherwise,
         add (n_constants - 400) * 6 to get the byte offset.

         Notice that _most_ of the data in the jpl_eph_data struct is stored
         between bytes 2652 to 2839.  That gets us up to ipt[11][2].  In a
         sensible world,  this would be followed by ipt[12][0].  However,  it
         used to be that DE just gave eleven variables;  librations and
         TT-TDB are sort of an afterthought.  And furthermore,  having more
         than 400 constants was something of an afterthought,  too.  So you
         have ipts[0] to [11] stored contiguously;  then four bytes for the
         ephemeris version are stuck in there;  then twelve bytes for ipt[12],
         which may be zero;  then any "extra" (past 400) constant names;
         and _then_ 24 bytes for ipt[13] and ipt[14].

   recsize-recsize+8*ncon: actual values of the constants.
         recsize = 8 * ncoeff;  ncoeff,  thus far,  has been...
         ncoeff = 773 for DE-102
         ncoeff = 826 for DE-200 & 202
         ncoeff = 1018 for DE-403, 405, 406, 410, 414, 421, 422, 423, 424, 430, 431,
         ncoeff = 728 for DE-406
         ncoeff = 982 for DE-432, DE-432t
*/
