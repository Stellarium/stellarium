/* rocks.hpp: coordinates of inner moons of Mars through Pluto

Copyright (C) 2010, Project Pluto

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

/* ROCKS.HPP

   This code generates positions for "rocks" (faint inner satellites
that can be modelled,  at least passably well,  as precessing ellipses).
The currently listed "rocks" are:

  505:         Amalthea     706: 1986U7 Cordelia   801: Triton (Neptune I)
  514: 1979J2  Thebe        707: 1986U8 Ophelia    803: 1989 N6 Naiad
  515: 1979J1  Adrastea     708: 1986U9 Bianca     804: 1989 N5 Thalassa
  516: 1979J3  Metis        709: 1986U3 Cressida   805: 1989 N3 Despina
                            710: 1986U6 Desdemona  806: 1989 N4 Galatea
  610: Janus      (+)       711: 1986U2 Juliet     807: 1989 N2 Larissa
  611: Epimetheus (+)       712: 1986U1 Portia     808: 1989 N1 Proteus
  612: Helene     (+)       713: 1986U4 Rosalind
  613: Telesto    (+)       714: 1986U5 Belinda
  614: Calypso    (+)       715: 1986U1 Puck
  615: 1980S28 Atlas        725: 1986U10 = Perdita       401: Phobos (+)
  616: 1980S27 Prometheus   726: Mab (+)                 402: Deimos (+)
  617: 1980S26 Pandora      727: Cupid (+)
  618: 1981S13 Pan
  635: Daphnis (+)

   The orbital elements were provided by Mark Showalter (some updates from
Bob Jacobson),  along with the following comments:

(1) The listing refers to JPL ephemeris IDs JUP120, SAT080, URA039, and
NEP022(*).  If you select the SPICE ephemerides of the same name in
Horizons, then you really should get almost identical results, because
the SPICE files were generated using these exact elements.

(2) Be aware that Saturn's moons Prometheus and Pandora are exhibiting
peculiar variations in longitude that are not currently understood,
probably related to interactions with the other nearby moons and rings.
So don't expect your results for these bodies to be particularly
accurate.

Regards, Mark Showalter

  -------------------------

(+) Data is provided for this object,  but I've not tried it out in Guide yet.

(*) replaced with JUP250 data 3 Nov 2007,  NEP050 data 8 Jan 2008
(and Triton added),  URA086X on 8 Jan 2008, MAR080 on 13 Feb 2009.
Some others updated as noted in comments.  All Uranians replaced
with URA091 on 22 Dec 2009.

2012 Mar 9:  updated Pan & Daphnis with SAT353.

2013 May 8:  updated Pan & Daphnis with SAT357eq.

2015 Apr 15:  updated Pan & Daphnis with SAT363eq_b.  Flipped signs on
      p and q for Triton,  which caused me to get values within 300 to
      500 km of Horizons.  Added comments converting epoch JDs to
      calendar dates.  Improved the test routine.        */

#include <math.h>

#define ROCK struct rock

ROCK
   {
   int jpl_id;
   double epoch_jd, a, h, k, mean_lon0, p, q, apsis_rate, mean_motion;
   double node_rate, laplacian_pole_ra, laplacian_pole_dec;
   };

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923
#define N_ROCKS 36

#define ROCKS_AMALTHEA 505
#define ROCKS_THEBE 514

#ifdef __cplusplus
extern "C" {
#endif

int evaluate_rock( const double jde, const int jpl_id, double *output_vect);

#ifdef __cplusplus
}
#endif
