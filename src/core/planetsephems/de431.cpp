/*
Copyright (c) 2015 Holger Niessner
Copyright (c) 2016 Georg Zotti

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
#include "StelUtils.hpp"
#ifndef UNIT_TEST
#include "StelCore.hpp"
#include "StelApp.hpp"
#else
#include "VecMath.hpp"
#endif

#ifdef __cplusplus
  extern "C" {
#endif

static void * ephem;
   
static Vec3d tempECLpos = Vec3d(0,0,0);
static Vec3d tempECLspd = Vec3d(0,0,0);
static Vec3d tempICRFpos = Vec3d(0,0,0);
static Vec3d tempICRFspd = Vec3d(0,0,0);
static char nams[JPL_MAX_N_CONSTANTS][6];
static double vals[JPL_MAX_N_CONSTANTS];
static double tempXYZ[6];
#ifdef UNIT_TEST
// NOTE: Added hook for unit testing
static const Mat4d matJ2000ToVsop87(Mat4d::xrotation(-23.4392803055555555556*(M_PI/180)) * Mat4d::zrotation(0.0000275*(M_PI/180)));
#endif

static bool initDone = false;

void InitDE431(const char* filepath)
{
	ephem = jpl_init_ephemeris(filepath, nams, vals);

	if(jpl_init_error_code() != 0)
	{
		#ifndef UNIT_TEST
		StelApp::getInstance().getCore()->setDe431Active(false);
		#endif
		qDebug() << "Error "<< jpl_init_error_code() << "at DE431 init:" << jpl_init_error_message();
	}
	else
	{
		initDone = true;
		double jd1, jd2;
		jd1=jpl_get_double(ephem, JPL_EPHEM_START_JD);
		jd2=jpl_get_double(ephem, JPL_EPHEM_END_JD);
		qDebug() << "DE431 init successful. startJD=" << QString::number(jd1, 'f', 4) << "endJD=" << QString::number(jd2, 'f', 4);
	}
}

void TerminateDE431()
{
  jpl_close_ephemeris(ephem);
}

bool GetDe431Coor(const double jde, const int planet_id, double * xyz, const int centralBody_id)
{
    if(initDone)
    {
	// This may return some error code!
	int jplresult=jpl_pleph(ephem, jde, planet_id, centralBody_id, tempXYZ, 1);

	switch (jplresult)
	{
		case 0: // all OK.
			break;
		case JPL_EPH_OUTSIDE_RANGE:
			qDebug() << "GetDe431Coor: JPL_EPH_OUTSIDE_RANGE at jde" << jde << "for planet" << planet_id;
			return false;
		case JPL_EPH_READ_ERROR:
			qDebug() << "GetDe431Coor: JPL_EPH_READ_ERROR at jde" << jde << "for planet" << planet_id;
			return false;
		case JPL_EPH_QUANTITY_NOT_IN_EPHEMERIS:
			qDebug() << "GetDe431Coor: JPL_EPH_QUANTITY_NOT_IN_EPHEMERIS at jde" << jde << "for planet" << planet_id;
			return false;
		case JPL_EPH_INVALID_INDEX:
			qDebug() << "GetDe431Coor: JPL_EPH_INVALID_INDEX at jde" << jde << "for planet" << planet_id;
			return false;
		case JPL_EPH_FSEEK_ERROR:
			qDebug() << "GetDe431Coor: JPL_EPH_FSEEK_ERROR at jde" << jde << "for planet" << planet_id;
			return false;
		default: // Should never happen...
			qDebug() << "GetDe431Coor: unknown error" << jplresult << "at jde" << jde << "for planet" << planet_id;
			return false;
	}

	tempICRFpos = Vec3d(tempXYZ[0], tempXYZ[1], tempXYZ[2]);
	tempICRFspd = Vec3d(tempXYZ[3], tempXYZ[4], tempXYZ[5]);
	#ifdef UNIT_TEST
	tempECLpos = matJ2000ToVsop87 * tempICRFpos;
	tempECLspd = matJ2000ToVsop87 * tempICRFspd;
	#else
	tempECLpos = StelCore::matJ2000ToVsop87 * tempICRFpos;
	tempECLspd = StelCore::matJ2000ToVsop87 * tempICRFspd;
	#endif

	xyz[0] = tempECLpos[0];
	xyz[1] = tempECLpos[1];
	xyz[2] = tempECLpos[2];
	xyz[3] = tempECLspd[0];
	xyz[4] = tempECLspd[1];
	xyz[5] = tempECLspd[2];
	return true;
    }
    return false;
}


#ifdef __cplusplus
  }
#endif
