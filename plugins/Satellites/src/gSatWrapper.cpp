/***************************************************************************
 * Name: gSatWrapper.hpp
 *
 * Description: Wrapper over gSatTEME class.
 *              This class allow use Satellite orbit calculation module (gSAt) in
 *              Stellarium 'native' mode using Stellarium objects.
 *
 ***************************************************************************/
/***************************************************************************
 *   Copyright (C) 2006 by J.L. Canales                                    *
 *   jlcanales.gasco@gmail.com                                  *
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



#include "gsatellite/stdsat.h"
#include "gsatellite/mathUtils.hpp"

#include "gSatWrapper.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"

#include "SolarSystem.hpp"
#include "StelModuleMgr.hpp"

#include <QDebug>
#include <QByteArray>


gSatWrapper::gSatWrapper(QString designation, QString tle1,QString tle2)
{
	// The TLE library actually modifies the TLE strings, which is annoying (because
	// when we get updates, we want to check if there has been a change by using ==
	// with the original.  Thus we make a copy to send to the TLE library.
	QByteArray t1(tle1.toAscii().data()), t2(tle2.toAscii().data());

	// Also, the TLE library expects no more than 130 characters length input.  We
	// shouldn't have sane input with a TLE longer than about 80, but just in case
	// we have a mal-formed input, we will truncate here to be safe
	t1.truncate(130);
	t2.truncate(130);

	pSatellite = new gSatTEME(designation.toAscii().data(),
	                          t1.data(),
	                          t2.data());
	updateEpoch();
}


gSatWrapper::~gSatWrapper()
{
	if (pSatellite != NULL)
		delete pSatellite;
}


Vec3d gSatWrapper::getTEMEPos()
{
        gVector position;
	Vec3d returnedVector;
	if (pSatellite != NULL)
	{
                position = pSatellite->getPos();
                returnedVector.set(position[0], position[1], position[2]);
	}
	else
		qWarning() << "gSatWrapper::getTEMEPos Method called without pSatellite initialized";

	return returnedVector;

}


Vec3d gSatWrapper::getTEMEVel()
{
        gVector velocity;
	Vec3d returnedVector;
	if (pSatellite != NULL)
	{
                velocity = pSatellite->getVel();
                returnedVector.set(velocity[0], velocity[1], velocity[2]);
	}
	else
		qWarning() << "gSatWrapper::getTEMEVel Method called without pSatellite initialized";

	return returnedVector;

}


Vec3d gSatWrapper::getSubPoint()
{
        gVector satelliteSubPoint;
	Vec3d returnedVector;
	if (pSatellite != NULL)
	{
                satelliteSubPoint = pSatellite->getSubPoint();
                returnedVector.set(satelliteSubPoint[0], satelliteSubPoint[1], satelliteSubPoint[2]);
	}
	else
		qWarning() << "gSatWrapper::getTEMEVel Method called without pSatellite initialized";

	return returnedVector;
}


void gSatWrapper::updateEpoch()
{
	double jul_utc = StelApp::getInstance().getCore()->getJDay();
        epoch = jul_utc;

	if (pSatellite)
                pSatellite->setEpoch(epoch);
}

void gSatWrapper::setEpoch(double ai_julianDaysEpoch)
{
    epoch = ai_julianDaysEpoch;
    if (pSatellite)
		pSatellite->setEpoch(ai_julianDaysEpoch);
}


void gSatWrapper::calcObserverECIPosition(Vec3d& ao_position, Vec3d& ao_velocity)
{

	StelLocation loc   = StelApp::getInstance().getCore()->getCurrentLocation();

	double radLatitude = loc.latitude * KDEG2RAD;
        double theta       = epoch.toThetaLMST(loc.longitude * KDEG2RAD);
	double r;
	double c,sq;

	/* Reference:  Explanatory supplement to the Astronomical Almanac, page 209-210. */
	/* Elipsoid earth model*/
	/* c = Nlat/a */
	c = 1/sqrt(1 + __f*(__f - 2)*Sqr(sin(radLatitude)));
	sq = Sqr(1 - __f)*c;

	r = (KEARTHRADIUS*c + (loc.altitude/1000))*cos(radLatitude);
	ao_position[0] = r * cos(theta);/*kilometers*/
	ao_position[1] = r * sin(theta);
	ao_position[2] = (KEARTHRADIUS*sq + (loc.altitude/1000))*sin(radLatitude);
        ao_velocity[0] = -KMFACTOR*ao_position[1];/*kilometers/second*/
        ao_velocity[1] =  KMFACTOR*ao_position[0];
        ao_velocity[2] =  0;
}



Vec3d gSatWrapper::getAltAz()
{

	StelLocation loc   = StelApp::getInstance().getCore()->getCurrentLocation();
	Vec3d topoSatPos;
	Vec3d observerECIPos;
	Vec3d observerECIVel;

	double  radLatitude    = loc.latitude * KDEG2RAD;
        double  theta          = epoch.toThetaLMST(loc.longitude * KDEG2RAD);

	calcObserverECIPosition(observerECIPos, observerECIVel);

	Vec3d satECIPos  = getTEMEPos();
	Vec3d slantRange = satECIPos - observerECIPos;

	//top_s
	topoSatPos[0] = (sin(radLatitude) * cos(theta)*slantRange[0]
	                 + sin(radLatitude)* sin(theta)*slantRange[1]
                         - cos(radLatitude)* slantRange[2]);
	//top_e
	topoSatPos[1] = ((-1.0)* sin(theta)*slantRange[0]
                         + cos(theta)*slantRange[1]);

	//top_z
	topoSatPos[2] = (cos(radLatitude) * cos(theta)*slantRange[0]
	                 + cos(radLatitude) * sin(theta)*slantRange[1]
                         + sin(radLatitude) *slantRange[2]);

	return topoSatPos;
}

void  gSatWrapper::getSlantRange(double &ao_slantRange, double &ao_slantRangeRate)
{

	Vec3d observerECIPos;
	Vec3d observerECIVel;

	calcObserverECIPosition(observerECIPos, observerECIVel);


        Vec3d satECIPos            = getTEMEPos();
        Vec3d satECIVel            = getTEMEVel();
        Vec3d slantRange           = satECIPos - observerECIPos;
        Vec3d slantRangeVelocity   = satECIVel - observerECIVel;

	ao_slantRange     = slantRange.length();
        ao_slantRangeRate = slantRange.dot(slantRangeVelocity)/ao_slantRange;
}

// Operation getVisibilityPredict
// @brief This operation predicts the satellite visibility contidions.
int gSatWrapper::getVisibilityPredict()
{


	// All positions in ECI system are positions referenced in a StelCore::EquinoxEq system centered in the earth centre
	Vec3d observerECIPos;
	Vec3d observerECIVel;
	Vec3d satECIPos;
	Vec3d satAltAzPos;
	Vec3d sunECIPos;
	Vec3d sunEquinoxEqPos;
	Vec3d sunAltAzPos;


	double sunSatAngle, Dist;
	int   visibility;

	satAltAzPos = getAltAz();

	if (satAltAzPos[2] > 0)
	{
		calcObserverECIPosition(observerECIPos, observerECIVel);

		satECIPos = getTEMEPos();
		SolarSystem *solsystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
		sunEquinoxEqPos        = solsystem->getSun()->getEquinoxEquatorialPos(StelApp::getInstance().getCore());
		sunAltAzPos        = solsystem->getSun()->getAltAzPosGeometric(StelApp::getInstance().getCore());

		//sunEquinoxEqPos is measured in AU. we need meassure it in Km
		sunECIPos.set(sunEquinoxEqPos[0]*AU, sunEquinoxEqPos[1]*AU, sunEquinoxEqPos[2]*AU);
		sunECIPos = sunECIPos + observerECIPos; //Change ref system centre


		if (sunAltAzPos[2] > 0.0)
		{
			visibility = RADAR_SUN;
		}
		else
		{
			sunSatAngle = sunECIPos.angle(satECIPos);
			Dist = satECIPos.length()*cos(sunSatAngle - (M_PI/2));

			if (Dist > KEARTHRADIUS)
			{
				visibility = VISIBLE;
			}
			else
			{
				visibility = RADAR_NIGHT;
			}
		}
	}
	else
		visibility = NOT_VISIBLE;

	return visibility; //TODO: put correct return
}




