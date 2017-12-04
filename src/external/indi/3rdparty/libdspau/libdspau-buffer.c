/*
 *   libDSPAU - a digital signal processing library for astronomy usage
 *   Copyright (C) 2017  Ilia Platone <info@iliaplatone.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "libdspau.h"

int dspau_removemean(double* in, double* out, int len)
{
	double mean = dspau_mean(in, len);
	for(int k = 0; k < len; k++)
		out[k] = in[k] - mean;
	return 0;
}

int dspau_stretch(double* in, double* out, int len, double min, double max)
{
	double mn, mx;
	dspau_minmidmax(in, len, &mn, &mx);
	for(int k = 0; k < len; k++) {
		out[k] -= mn;
		out[k] *= (max - min) / (mx - mn);
		out[k] += min;
	}
	return 0;
}

int dspau_normalize(double* in, double* out, int len, double min, double max)
{
	for(int k = 0; k < len; k++) {
		out[k] = (in[k] < min ? min : (in[k] > max ? max : in[k]));
	}
	return 0;
}

int dspau_sub(double* in1, double* in2, double* out, int len)
{
	for(int k = 0; k < len; k++) {
		out[k] = in1[k] - in2[k];
	}
	return 0;
}

int dspau_sum(double* in1, double* in2, double* out, int len)
{
	for(int k = 0; k < len; k++) {
		out[k] = in1[k] + in2[k];
	}
	return 0;
}

int dspau_div(double* in1, double* in2, double* out, int len)
{
	for(int k = 0; k < len; k++) {
		out[k] = in1[k] / in2[k];
	}
	return 0;
}

int dspau_mul(double* in1, double* in2, double* out, int len)
{
	for(int k = 0; k < len; k++) {
		out[k] = in1[k] * in2[k];
	}
	return 0;
}

int dspau_sub1(double* in, double* out, int len, double val)
{
	for(int k = 0; k < len; k++) {
		out[k] = in[k] - val;
	}
	return 0;
}

int dspau_sum1(double* in, double* out, int len, double val)
{
	for(int k = 0; k < len; k++) {
		out[k] = in[k] + val;
	}
	return 0;
}

int dspau_div1(double* in, double* out, int len, double val)
{
	for(int k = 0; k < len; k++) {
		out[k] = in[k] / val;
	}
	return 0;
}

int dspau_mul1(double* in, double* out, int len, double val)
{
	for(int k = 0; k < len; k++) {
		out[k] = in[k] * val;
	}
	return 0;
}

static int compare( const void* a, const void* b)
{
     double int_a = * ( (double*) a );
     double int_b = * ( (double*) b );

     if ( int_a == int_b ) return 0;
     else if ( int_a < int_b ) return -1;
     else return 1;
}

int dspau_median(double* in, double* out, int len, int size, int median)
{
	int mid = (size / 2) + (size % 2);
	double* sorted = (double*)malloc(size * sizeof(double));
	for(int k = mid; k < len; k++) {
		memcpy (sorted, in + (k - mid), size * sizeof(double));
		qsort(sorted, size, sizeof(double), compare);
		out[k] = sorted[median];
	}
	return 0;
}

