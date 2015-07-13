/*
Copyright (c) 2015 Holger Niessner

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "de431.hpp"
#include "jpleph.h"

#define JPL_MAX_N_CONSTANTS 1018
#define CALC_VELOCITY       0

static void * ephem;
static char nams[JPL_MAX_N_CONSTANTS][6], buff[102];
static double vals[JPL_MAX_N_CONSTANTS];
    
void InitDE431(const char* filepath)
{
    ephem = jpl_init_ephemeris(filepath, nams, vals);
}
void GetDe431Coor(double jd, int planet_id, double * xyz)
{

}

void GetDe431OsculatingCoor(double jd0, double jd, int planet_id, double *xyz)
{
	
}
