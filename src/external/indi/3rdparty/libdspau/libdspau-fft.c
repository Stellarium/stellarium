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
#include <math.h>
#include <float.h>
#include <fftw3.h>
#include "libdspau.h"

double complex_mag(fftw_complex n)
{
	return sqrt (n[0] * n[0] + n[1] * n[1]);
}

double complex_phi(fftw_complex n)
{
	double out = 0;
	if (n[0] != 0)
		out = atan (n[1] / n[0]);
	return out;
}

void complex2mag(fftw_complex* in, double* out, int len)
{
	for (int i = 0; i < len; i++) {
		out [i] = complex_mag(in [i]);
	}
}

void complex2magpow(fftw_complex* in, double* out, int len)
{
	for (int i = 0; i < len; i++) {
		out [i] = pow(complex_mag(in [i]), 2);
	}
}

void complex2magsqrt(fftw_complex* in, double* out, int len)
{
	for (int i = 0; i < len; i++) {
		out [i] = sqrt (complex_mag(in [i]));
	}
}

void complex2magdbv(fftw_complex* in, double* out, int len)
{
	for (int i = 0; i < len; i++) {
		double magVal = complex_mag(in [i]);

		if (magVal <= 0.0)
			magVal = DBL_EPSILON;

		out [i] = 20 * log10 (magVal);
	}
}

void complex2phideg(fftw_complex* in, double* out, int len)
{
	double sf = 180.0 / M_PI;
	for (int i = 0; i < len; i++) {
		out [i] = complex_phi(in [i]) * sf;
	}
}

void complex2phirad(fftw_complex* in, double* out, int len)
{
	for (int i = 0; i < len; i++) {
		out [i] = complex_phi(in [i]);
	}
}

int dspau_spectrum(double* in, double* out, int dims, int *sizes, int conversion)
{
	int i = 0;
	int len = sizes[0];
	int ret = 0;
	fftw_complex *fft_in, *fft_out;
	fftw_plan p;
	for(int d = 1; d < dims; d++)
	{
		len *= sizes[d];
	}
	fft_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * len);
	fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * len);
	for(i = 0; i < len; i++) {
		fft_in[i][0] = in[i];
		fft_in[i][1] = in[i];
	}
	p = fftw_plan_dft(dims, sizes, fft_in, fft_out, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(p);
	switch (conversion) {
	case magnitude:
		complex2mag(fft_out, out, len);
		break;
	case magnitude_dbv:
		complex2magdbv(fft_out, out, len);
		break;
	case magnitude_rooted:
		complex2magsqrt(fft_out, out, len);
		break;
	case magnitude_squared:
		complex2magpow(fft_out, out, len);
		break;
	case phase_degrees:
		complex2phideg(fft_out, out, len);
		break;
	case phase_radians:
		complex2phirad(fft_out, out, len);
		break;
	default:
		ret = -1;
		break;
	}
	fftw_destroy_plan(p);
	fftw_free(fft_in);
	fftw_free(fft_out);
	return ret;
}
