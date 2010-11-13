/*
 * gObserver.cpp
 *
 *  Created on: 05/08/2010
 *      Author: cdeveloper
 */

#include "stdsat.h"
#include "gVector.hpp"
#include "gObserver.hpp"
#include "gTime.hpp"
#include "mathUtils.hpp"
#include <math.h>

//! Definition: This operation compute the observer ECI coordinates for the ai_epoch time
void gObserver::getECIPosition(gTime ai_epoch, gVector& ao_position, gVector& ao_vel)
{
	ao_position.resize(3);
	ao_vel.resize(3);
	double radLatitude = m_latitude * KDEG2RAD;
	double theta = ai_epoch.toThetaLMST(m_longitude * KDEG2RAD);
	double r;
	double c,sq;

	/* Reference:  The 1992 Astronomical Almanac, page K11. */
	c = 1/sqrt(1 + __f*(__f - 2)*Sqr(sin(radLatitude)));
	sq = Sqr(1 - __f)*c;

	r = (KEARTHRADIUS*c + m_attitude)*cos(radLatitude);
	ao_position[0] = r * cos(theta);/*kilometers*/
	ao_position[1] = r * sin(theta);
	ao_position[2] = (KEARTHRADIUS*sq + m_attitude)*sin(radLatitude);
	ao_vel[0] = -KMFACTOR*ao_position[1];/*kilometers/second*/
	ao_vel[1] =  KMFACTOR*ao_position[0];
	ao_vel[2] =  0;
}

gVector gObserver::calculateLook(gSatTEME ai_Sat, gTime ai_epoch)
{

	gVector returnVector(4);
	gVector topoSatPos(3);
	gVector observerECIPos;
	gVector observerECIVel;


	double  radLatitude    = m_latitude * KDEG2RAD;
	double  theta          = ai_epoch.toThetaLMST(m_longitude * KDEG2RAD);

	getECIPosition(ai_epoch, observerECIPos, observerECIVel);
	ai_Sat.setEpoch(ai_epoch);

	gVector satECIPos = ai_Sat.getPos();
	gVector satECIVel = ai_Sat.getVel();
	gVector slantRange = satECIPos - observerECIPos;
	gVector rangeVel   = satECIVel - observerECIVel;

	//top_s
	topoSatPos[0] = sin(radLatitude) * cos(theta)*slantRange[0]
	                + sin(radLatitude)* sin(theta)*slantRange[1]
	                - cos(radLatitude)* slantRange[2];
	//top_e
	topoSatPos[1] = (-1.0)* sin(theta)*slantRange[0]
	                + cos(theta)*slantRange[1];

	//top_z
	topoSatPos[2] = cos(radLatitude) * cos(theta)*slantRange[0]
	                + cos(radLatitude) * sin(theta)*slantRange[1]
	                + sin(radLatitude) *slantRange[2];

	returnVector[ AZIMUTH] = atan((-1.0)*topoSatPos[1]/topoSatPos[0]);
	if(topoSatPos[0] > 0)
		returnVector[ AZIMUTH] += KPI;
	if(returnVector[ AZIMUTH] < 0)
		returnVector[ AZIMUTH] += K2PI;

	returnVector[ RANGE]     = slantRange.Magnitude();
	returnVector[ ELEVATION] = asin(topoSatPos[2]/returnVector[ RANGE]);
	returnVector[ RANGERATE] = slantRange.Dot(rangeVel)/returnVector[ RANGE];
	return returnVector;


	// Corrections for atmospheric refraction
	// Reference:  Astronomical Algorithms by Jean Meeus, pp. 101-104
	// Correction is meaningless when apparent elevation is below horizon
	// obs_set->y = obs_set->y + Radians((1.02/tan(Radians(Degrees(el)+
	//              10.3/(Degrees(el)+5.11))))/60);
}
