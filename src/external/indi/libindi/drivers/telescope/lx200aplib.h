/*
    LX200 AP Driver
    Copyright (C) 2007 Markus Wildi

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#pragma once

#define ATA   0
#define ATR   1
#define ARTT  2
#define ARTTO 3 /* not yet there, requires a pointing model */

double LDRAtoHA(double RA, double longitude);
int LDEqToEqT(double ra_h, double dec_d, double *hxt, double *rat_h, double *dect_d);
int LDCartToSph(double *vec, double *ra, double *dec);
int LDAppToX(int trans_to, double *star_cat, double tjd, double *loc, double *hxt, double *star_trans);
