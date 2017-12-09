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
#include "libdspau.h"

struct Coefficient {
	double e;
	double p;
	double d0;
	double d1;
	double d2;
	double x0;
	double x1;
	double x2;
	double y0;
	double y1;
	double y2;
};

int dspau_squarelawfilter(double* in, double* out, int len)
{
	double mean = dspau_mean(in, len);
	int val = 0;
	for (int i = 0; i < len; i++) {
		val = in [i] - mean;
		out [i] = (abs (val) + mean);
	}
	return 0;
}

int dspau_lowpassfilter(double* in, double* out, int len, double SamplingFrequency, double Frequency, double Q)
{
	double RC = 1.0 / (Frequency * 2.0 * M_PI); 
	double dt = 1.0 / (SamplingFrequency * 2.0 * M_PI); 
	double alpha = dt / (RC + dt) / Q;
	out [0] = in [0];
	for (int i = 1; i < len; ++i) { 
		out [i] = (out [i - 1] + (alpha * (in [i] - out [i - 1])));
	}
	return 0;
}

int dspau_highpassfilter(double* in, double* out, int len, double SamplingFrequency, double Frequency, double Q)
{
	double RC = 1.0 / (Frequency * 2.0 * M_PI);
	double dt = 1.0 / (SamplingFrequency * 2.0 * M_PI);
	double alpha = dt / (RC + dt) / Q;
	out [0] = in [0];
	for (int i = 1; i < len; ++i) { 
		out [i] = (in[i] - (out [i - 1] + (alpha * (in [i] - out [i - 1]))));
	}
	return 0;
}

double dspau_singlefilter(double yin, struct Coefficient *coefficient) {
	double yout = 0.0;
	coefficient->x0 = coefficient->x1; 
	coefficient->x1 = coefficient->x2; 
	coefficient->x2 = yin;

	coefficient->y0 = coefficient->y1; 
	coefficient->y1 = coefficient->y2; 
	coefficient->y2 = coefficient->d0 * coefficient->x2 - coefficient->d1 * coefficient->x1 + coefficient->d0 * coefficient->x0 + coefficient->d1 * coefficient->y1 - coefficient->d2 * coefficient->y0;

	yout = coefficient->y2;
	return yout;
}

int dspau_bandrejectfilter(double* in, double* out, int len, double SamplingFrequency, double Frequency, double Q) {
	double wo = 2.0 * M_PI * Frequency / SamplingFrequency;

	struct Coefficient coefficient;
	coefficient.e = 1 / (1 + tan (wo / (Q * 2)));
	coefficient.p = cos (wo);
	coefficient.d0 = coefficient.e;
	coefficient.d1 = 2 * coefficient.e * coefficient.p;
	coefficient.d2 = (2 * coefficient.e - 1);
	for (int x = 0; x < len; x ++) {
		out [x] = dspau_singlefilter (in [x], &coefficient);
	}
	return 0;
}

int dspau_bandpassfilter(double* in, double* out, int len, double SamplingFrequency, double Frequency, double Q) {
	double wo = 2.0 * M_PI * Frequency / SamplingFrequency;

	struct Coefficient coefficient;
	coefficient.e = 1 / (1 + tan (wo / (Q * 2)));
	coefficient.p = cos (wo);
	coefficient.d0 = coefficient.e;
	coefficient.d1 = 2 * coefficient.e * coefficient.p;
	coefficient.d2 = (2 * coefficient.e - 1);
	for (int x = 0; x < len; x ++) {
		out [x] = in [x] - dspau_singlefilter (in [x], &coefficient);
	}
	return 0;
}


