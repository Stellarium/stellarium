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
	QByteArray t1(tle1.toLatin1().data()), t2(tle2.toLatin1().data());

	// Also, the TLE library expects no more than 130 characters length input.  We
	// shouldn't have sane input with a TLE longer than about 80, but just in case
	// we have a mal-formed input, we will truncate here to be safe
	t1.truncate(130);
	t2.truncate(130);

	pSatellite = new gSatTEME(designation.toLatin1().data(), t1.data(), t2.data());
	setEpoch(StelApp::getInstance().getCore()->getJD());
}

gSatWrapper::~gSatWrapper()
{
	if (pSatellite)
		delete pSatellite;
}

Vec3d gSatWrapper::getTEMEPos() const
{
	Vec3d returnedVector;
	if (pSatellite)
		returnedVector = pSatellite->getPos();
	else
		qWarning() << "gSatWrapper::getTEMEPos Method called without pSatellite initialized";

	return returnedVector;
}

Vec3d gSatWrapper::getTEMEVel() const
{
	Vec3d returnedVector;
	if (pSatellite)
		returnedVector = pSatellite->getVel();
	else
		qWarning() << "gSatWrapper::getTEMEVel Method called without pSatellite initialized";

	return returnedVector;
}

Vec3d gSatWrapper::getSubPoint() const
{
	Vec3d returnedVector;
	if (pSatellite)
		returnedVector = pSatellite->getSubPoint();
	else
		qWarning() << "gSatWrapper::getSubPoint Method called without pSatellite initialized";

	return returnedVector;
}

void gSatWrapper::setEpoch(double ai_julianDaysEpoch)
{
	epoch = ai_julianDaysEpoch;
	if (pSatellite)
		pSatellite->setEpoch(epoch);
}

void gSatWrapper::calcObserverECIPosition(Vec3d& ao_position, Vec3d& ao_velocity)
{
	if (epoch != lastCalcObserverECIPosition)
	{
		StelLocation loc   = StelApp::getInstance().getCore()->getCurrentLocation();

		double radLatitude	= loc.getLatitude() * KDEG2RAD;
		double theta		= epoch.toThetaLMST(loc.getLongitude() * KDEG2RAD);

		/* Reference:  Explanatory supplement to the Astronomical Almanac 1992, page 209-210. */
		/* Ellipsoid earth model*/
		/* C = Nlat/a */
		const double C = 1/std::sqrt(1 + __f*(__f - 2.)*Sqr(sin(radLatitude)));
		const double S = Sqr(1 - __f)*C;

		double r = (EARTH_RADIUS*C + (loc.altitude/1000.))*cos(radLatitude);
		ao_position[0] = r * cos(theta);/*kilometers*/
		ao_position[1] = r * sin(theta);
		ao_position[2] = (EARTH_RADIUS*S + (loc.altitude/1000.))*sin(radLatitude);
		ao_velocity[0] = -KMFACTOR*ao_position[1];/*kilometers/second*/
		ao_velocity[1] =  KMFACTOR*ao_position[0];
		ao_velocity[2] =  0;

		lastCalcObserverECIPosition=epoch;
	}
}

Vec3d gSatWrapper::getAltAz() const
{
	StelLocation loc   = StelApp::getInstance().getCore()->getCurrentLocation();
	Vec3d topoSatPos;

	const double  radLatitude	= loc.getLatitude() * KDEG2RAD;
	const double  theta		= epoch.toThetaLMST(loc.getLongitude() * KDEG2RAD);
	const double sinRadLatitude	= sin(radLatitude);
	const double cosRadLatitude	= cos(radLatitude);
	const double sinTheta	= sin(theta);
	const double cosTheta	= cos(theta);

	// This now only updates if required.
	calcObserverECIPosition(observerECIPos, observerECIVel);

	Vec3d satECIPos	= getTEMEPos();
	Vec3d slantRange	= satECIPos - observerECIPos;

	//top_s
	topoSatPos[0] = (sinRadLatitude * cosTheta*slantRange[0]
			 + sinRadLatitude* sinTheta*slantRange[1]
			 - cosRadLatitude* slantRange[2]);
	//top_e
	topoSatPos[1] = ((-1.0)* sinTheta*slantRange[0]
			 + cosTheta*slantRange[1]);

	//top_z
	topoSatPos[2] = (cosRadLatitude * cosTheta*slantRange[0]
			 + cosRadLatitude * sinTheta*slantRange[1]
			 + sinRadLatitude *slantRange[2]);

	return topoSatPos;
}

void  gSatWrapper::getSlantRange(double &ao_slantRange, double &ao_slantRangeRate) const
{
	calcObserverECIPosition(observerECIPos, observerECIVel);

	Vec3d satECIPos		= getTEMEPos();
	Vec3d satECIVel		= getTEMEVel();
	Vec3d slantRange		= satECIPos - observerECIPos;
	Vec3d slantRangeVelocity = satECIVel - observerECIVel;

	ao_slantRange		= slantRange.norm();
	ao_slantRangeRate	= slantRange.dot(slantRangeVelocity)/ao_slantRange;
}

// Does the actual computation only if necessary (0.1s apart) and caches the result in a static variable.
void gSatWrapper::updateSunECIPos()
{
	// All positions in ECI system are positions referenced in a StelCore::EquinoxEq system centered in the earth centre
	calcObserverECIPosition(observerECIPos, observerECIVel);

	static const SolarSystem *solsystem = GETSTELMODULE(SolarSystem);
	Vec3d sunEquinoxEqPos = solsystem->getSun()->getEquinoxEquatorialPos(StelApp::getInstance().getCore());

	//sunEquinoxEqPos is measured in AU. we need measure it in Km
	sunECIPos=sunEquinoxEqPos*AU + observerECIPos; //Change ref system centre
}

Vec3d gSatWrapper::getSunECIPos()
{
	if (epoch != lastSunECIepoch)
	{
		updateSunECIPos();
		lastSunECIepoch=epoch;
	}
	return sunECIPos;
}

// Operation getVisibilityPredict
// @brief This operation predicts the satellite visibility conditions.
gSatWrapper::Visibility gSatWrapper::getVisibilityPredict() const
{
	gSatWrapper::Visibility rval = BELOW_HORIZON;
	Vec3d satAltAzPos = getAltAz();
	if (satAltAzPos[2] > 0)
	{
		static const SolarSystem* solsystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
		Vec3d sunAltAzPos = solsystem->getSun()->getAltAzPosGeometric(StelApp::getInstance().getCore());
		if (sunAltAzPos[2] > 0.0)
		{
			rval = RADAR_SUN;
		}
		else
		{
			Vec3d sunECIPos = getSunECIPos();
			Vec3d satECIPos = getTEMEPos();
			rval = VISIBLE;
			/*
			Satellites in umbra/penumbra based on:-
			Visually Observing Earth Satellites By Dr. T.S. Kelso
			https://celestrak.org/columns/v03n01/
			*/
			double psun = std::sqrt(pow(satECIPos[0] - sunECIPos[0], 2) + pow(satECIPos[1] - sunECIPos[1], 2) + pow(satECIPos[2] - sunECIPos[2], 2) );
			double pearth = std::sqrt(std::pow(satECIPos[0], 2) + std::pow(satECIPos[1], 2) + std::pow(satECIPos[2], 2));
			double theta_e = std::asin((12742. / 2.) / pearth);
			double theta_s = std::asin((1391000. / 2.) / psun);
			double dotproduct_peps = std::abs((satECIPos[0] * sunECIPos[0]) + (satECIPos[1] * sunECIPos[1]) + (satECIPos[2] * sunECIPos[2]));
			double theta = std::acos(dotproduct_peps / (psun * pearth));
			if (theta_e > theta_s && theta < (theta_e - theta_s))
			{
				rval = RADAR_NIGHT;
			}
			else if (std::abs(theta_e - theta_s) < theta && theta < (theta_e + theta_s))
			{
				rval = PENUMBRAL;
			}
			else if (theta_s > theta_e && theta < (theta_s - theta_e))
			{
				rval = ANNULAR;
			}
		}
	}

	return rval;
}

double gSatWrapper::getPhaseAngle() const
{
	Vec3d sunECIPos = getSunECIPos();
	return sunECIPos.angle(getTEMEPos());
}

double gSatWrapper::getOrbitalPeriod() const
{
	return pSatellite->getPeriod();
}

double gSatWrapper::getOrbitalInclination() const
{
	return pSatellite->getInclination();
}

Vec2d gSatWrapper::getPerigeeApogeeAltitudes() const
{
	return pSatellite->getPerigeeApogee();
}

gTime gSatWrapper::epoch;
gTime gSatWrapper::lastSunECIepoch=0.0; // store last time of computation to avoid all-1 computations.
gTime gSatWrapper::lastCalcObserverECIPosition;

Vec3d gSatWrapper::sunECIPos; // enough to have this once.
Vec3d gSatWrapper::observerECIPos;
Vec3d gSatWrapper::observerECIVel;
