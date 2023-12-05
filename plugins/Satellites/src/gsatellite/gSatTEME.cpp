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
 *              Conference, Keystone, CO, 2006 August 21-24.
 *              https://celestrak.org/publications/AIAA/2006-6753/
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
#include <math.h>

#include "mathUtils.hpp"

#define CONSTANTS_SET wgs84
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
	m_SatName = pstrName;
	SGP4Funcs::twoline2rv(pstrTleLine1, pstrTleLine2, TYPERUN_SET, TYPEINPUT_SET, OPSMODE_SET, CONSTANTS_SET,
	           startmfe, stopmfe, deltamin, satrec);
	SGP4Funcs::sgp4(satrec, 0.0, m_Position.v, m_Vel.v);
}

gSatTEME::gSatTEME(const OMM& omm)
{
	// No use is made of the satnum within the SGP4 propergator
	// so we just fake it here.
	char objectId[] = "ABCDE";
	
	// The newer interface to sgp4() without using a TLE but
	// instead using direct element placement within the elsetrec
	// is done in this constructor (because XML and JSON do not
	// have two lines :)
	// Because we do not call twoline2rv() we must convert the 
	// SGP4 parameters when calling sgp4init() to ensure the 
	// terms are in their correct base units, (radians, etc).
	// The comments to the right are the parameter names for
	// the sgp4init() function call.
	SGP4Funcs::sgp4init(wgs84, 'c',                         // sgp4init(args) below
		objectId,                                           // satn[5]
		omm.getEpochJD() - 2433281.5,                       // epoch
		omm.getBstar(),                                     // xbstar
		omm.getMeanMotionDot() / (XPDOTP * 1440.0),         // xndot
		omm.getMeanMotionDDot() / (XPDOTP * 1440.0 * 1440), // xnndot
		omm.getEccentricity(),                              // xecco
		omm.getArgumentOfPerigee() * KDEG2RAD,              // xargpo
		omm.getInclination() * KDEG2RAD,                    // xinclo
		omm.getMeanAnomoly() * KDEG2RAD,                    // xmo
		omm.getMeanMotion() / XPDOTP,                       // xno_kozai
		omm.getAscendingNode() * KDEG2RAD,                  // xnodeo
		satrec);

	// Despite passing EpochJD to the sgp4init() function 
	// is does not setup the following like twoline2rv() does.
	satrec.jdsatepoch  = omm.getEpochJDW();
	satrec.jdsatepochF = omm.getEpochJDF();

	// Call the propagator to get the initial state vector value.
	SGP4Funcs::sgp4(satrec, 0.0, m_Position.v, m_Vel.v);
	m_SubPoint = computeSubPoint(omm.getEpochJD());
}

void gSatTEME::setEpoch(gTime ai_time)
{
	gTime kepEpoch(satrec.jdsatepoch + satrec.jdsatepochF);
	gTimeSpan tSince = ai_time - kepEpoch;
	double dtsince = tSince.getDblSeconds()/KSEC_PER_MIN;
	SGP4Funcs::sgp4(satrec, dtsince, m_Position.v, m_Vel.v);
	m_SubPoint = computeSubPoint(ai_time);
}

void gSatTEME::setMinSinceKepEpoch(double ai_minSinceKepEpoch)
{
	gTimeSpan tSince( ai_minSinceKepEpoch/KMIN_PER_DAY);
	gTime     Epoch(satrec.jdsatepoch);
	Epoch += tSince;
	// call the propagator to get the initial state vector value
	SGP4Funcs::sgp4(satrec, ai_minSinceKepEpoch, m_Position.v, m_Vel.v);
	m_SubPoint = computeSubPoint( Epoch);
}

Vec3d gSatTEME::computeSubPoint(gTime ai_Time)
{
	Vec3d resultVector; // (0) Latitude, (1) Longitude, (2) altitude
	double theta, r, e2, phi, c;

	theta = atan2(m_Position[1], m_Position[0]); // radians
	resultVector[ LONGITUDE] = fmod((theta - ai_Time.toThetaGMST()), K2PI);  //radians


	r = std::sqrt(Sqr(m_Position[0]) + Sqr(m_Position[1]));
	e2 = __f*(2. - __f);
	resultVector[ LATITUDE] = atan2(m_Position[2],r); /*radians*/

	do
	{
		phi = resultVector[ LATITUDE];
		c = 1./std::sqrt(1. - e2*Sqr(sin(phi)));
		resultVector[ LATITUDE] = atan2(m_Position[2] + EARTH_RADIUS*c*e2*sin(phi),r);
	}
	while(fabs(resultVector[ LATITUDE] - phi) >= 1E-10);

	resultVector[ ALTITUDE] = r/cos(resultVector[ LATITUDE]) - EARTH_RADIUS*c;/*kilometers*/

	if(resultVector[ LATITUDE] > (KPI/2.0)) resultVector[ LATITUDE] -= K2PI;

	resultVector[LATITUDE]  = resultVector[LATITUDE]/KDEG2RAD;
	resultVector[LONGITUDE] = resultVector[LONGITUDE]/KDEG2RAD;
	if(resultVector[LONGITUDE] < -180.0) resultVector[LONGITUDE] +=360;
	else if(resultVector[LONGITUDE] > 180.0) resultVector[LONGITUDE] -= 360;

	return resultVector;
}
