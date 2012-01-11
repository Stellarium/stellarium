/***************************************************************************
 * Name: gSatTEME.cpp
 *
 * Description: gSatTEME class implementation.
 *              This class abstract all the SGP4 complexity. It uses the
 *              David. A. Vallado code for SGP4 Calculation.
 *
 * Reference:
 *              Revisiting Spacetrack Report #3 AIAA 2006-6753
 *              Vallado, David A., Paul Crawford, Richard Hujsak, and T.S.
 *              Kelso, "Revisiting Spacetrack Report #3,"
 *              presented at the AIAA/AAS Astrodynamics Specialist
 *              Conference, Keystone, CO, 2006 August 21–24.
 *              http://celestrak.com/publications/AIAA/2006-6753/
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2004 by J.L. Canales                                    *
 *   ph03696@homeserver                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.             *
 ***************************************************************************/

// GKepFile
#include "gSatTEME.hpp"
#include <iostream>
#include <iomanip>

#include "stdsat.h"
#include "mathUtils.hpp"

#include "sgp4io.h"

#define CONSTANTS_SET wgs72
#define TYPERUN_SET   'c'
#define OPSMODE_SET   'i'
#define TYPEINPUT_SET 'm'

#define LATITUDE  0
#define LONGITUDE 1
#define ALTITUDE  2

// Constructors
gSatTEME::gSatTEME(const char *pstrName, char *pstrTleLine1, char *pstrTleLine2)
{

	double startmfe, stopmfe, deltamin;
	double ro[3];
	double vo[3];

	m_Position.resize(3);
	m_Vel.resize(3);

	m_SatName = pstrName;

	//set gravitational constants
	getgravconst(CONSTANTS_SET, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2);

	//Parsing TLE_Files and sat variables setting
	twoline2rv(pstrTleLine1, pstrTleLine2, TYPERUN_SET, TYPEINPUT_SET, OPSMODE_SET, CONSTANTS_SET,
	           startmfe, stopmfe, deltamin, satrec);

	// call the propagator to get the initial state vector value
	sgp4(CONSTANTS_SET, satrec,  0.0, ro,  vo);

	m_Position[ 0]= ro[ 0];
	m_Position[ 1]= ro[ 1];
	m_Position[ 2]= ro[ 2];
	m_Vel[ 0]     = vo[ 0];
	m_Vel[ 1]     = vo[ 1];
	m_Vel[ 2]     = vo[ 2];
}

void gSatTEME::setEpoch(gTime ai_time)
{

	gTime     kepEpoch(satrec.jdsatepoch);
	gTimeSpan tSince = ai_time - kepEpoch;

	double ro[3];
	double vo[3];
	double dtsince = tSince.getDblSeconds()/KSEC_PER_MIN;
	// call the propagator to get the initial state vector value
	sgp4(CONSTANTS_SET, satrec,  dtsince, ro,  vo);

	m_Position[ 0]= ro[ 0];
	m_Position[ 1]= ro[ 1];
	m_Position[ 2]= ro[ 2];
	m_Vel[ 0]     = vo[ 0];
	m_Vel[ 1]     = vo[ 1];
	m_Vel[ 2]     = vo[ 2];
	m_SubPoint    = computeSubPoint( ai_time);
}

void gSatTEME::setMinSinceKepEpoch(double ai_minSinceKepEpoch)
{

	double ro[3];
	double vo[3];
	gTimeSpan tSince( ai_minSinceKepEpoch/KMIN_PER_DAY);
	gTime     Epoch(satrec.jdsatepoch);
	Epoch += tSince;
	// call the propagator to get the initial state vector value
	sgp4(CONSTANTS_SET, satrec,  ai_minSinceKepEpoch, ro,  vo);

	m_Position[ 0]= ro[ 0];
	m_Position[ 1]= ro[ 1];
	m_Position[ 2]= ro[ 2];
	m_Vel[ 0]     = vo[ 0];
	m_Vel[ 1]     = vo[ 1];
	m_Vel[ 2]     = vo[ 2];
	m_SubPoint    = computeSubPoint( Epoch);
}

gVector gSatTEME::computeSubPoint(gTime ai_Time)
{

	gVector resultVector(3); // (0) Latitude, (1) Longitude, (2) altitude
	double theta, r, e2, phi, c;

	theta = AcTan(m_Position[1], m_Position[0]); // radians
	resultVector[ LONGITUDE] = fmod((theta - ai_Time.toThetaGMST()), K2PI);  //radians


	r = sqrt(Sqr(m_Position[0]) + Sqr(m_Position[1]));
	e2 = __f*(2 - __f);
	resultVector[ LATITUDE] = AcTan(m_Position[2],r); /*radians*/

	do
	{
		phi = resultVector[ LATITUDE];
		c = 1/sqrt(1 - e2*Sqr(sin(phi)));
		resultVector[ LATITUDE] = AcTan(m_Position[2] + KEARTHRADIUS*c*e2*sin(phi),r);
	}
	while(fabs(resultVector[ LATITUDE] - phi) >= 1E-10);

	resultVector[ ALTITUDE] = r/cos(resultVector[ LATITUDE]) - KEARTHRADIUS*c;/*kilometers*/

	if(resultVector[ LATITUDE] > (KPI/2.0)) resultVector[ LATITUDE] -= K2PI;

	resultVector[LATITUDE]  = resultVector[LATITUDE]/KDEG2RAD;
	resultVector[LONGITUDE] = resultVector[LONGITUDE]/KDEG2RAD;
	if(resultVector[LONGITUDE] < -180.0) resultVector[LONGITUDE] +=360;
	else if(resultVector[LONGITUDE] > 180.0) resultVector[LONGITUDE] -= 360;


	return resultVector;
}
