/***************************************************************************
 * Name: gSatStelWrapper.hpp
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/



#include "gsatellite/stdsat.h"
#include "gsatellite/mathUtils.hpp"

#include "gSatStelWrapper.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QByteArray>


gSatStelWrapper::gSatStelWrapper(QString designation, QString tle1,QString tle2)
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


gSatStelWrapper::~gSatStelWrapper()
{
	if (pSatellite != NULL)
		delete pSatellite;
}


Vec3d gSatStelWrapper::getTEMEPos()
{
	gVector tempPos;
	Vec3d returnedVector;
	if (pSatellite != NULL)
	{
		tempPos = pSatellite->getPos();
		returnedVector.set(tempPos[0], tempPos[1], tempPos[2]);
	}
	else
		qWarning() << "gSatStelWrapper::getTEMEPos Method called without pSatellite initialized";

	return returnedVector;

}


Vec3d gSatStelWrapper::getTEMEVel()
{
	gVector tempVel;
	Vec3d returnedVector;
	if (pSatellite != NULL)
	{
		tempVel = pSatellite->getVel();
		returnedVector.set(tempVel[0], tempVel[1], tempVel[2]);
	}
	else
		qWarning() << "gSatStelWrapper::getTEMEVel Method called without pSatellite initialized";

	return returnedVector;

}


Vec3d gSatStelWrapper::getSubPoint()
{
	gVector tempSubPoint;
	Vec3d returnedVector;
	if (pSatellite != NULL)
	{
		tempSubPoint = pSatellite->getSubPoint();
		returnedVector.set(tempSubPoint[0], tempSubPoint[1], tempSubPoint[2]);
	}
	else
		qWarning() << "gSatStelWrapper::getTEMEVel Method called without pSatellite initialized";

	return returnedVector;
}


void gSatStelWrapper::updateEpoch()
{
	double jul_utc = StelApp::getInstance().getCore()->getNavigator()->getJDay();
	Epoch = jul_utc;

	if (pSatellite)
		pSatellite->setEpoch(Epoch);
}

void gSatStelWrapper::setEpoch(double ai_julianDaysEpoch)
{
	if (pSatellite)
		pSatellite->setEpoch(ai_julianDaysEpoch);
}


void gSatStelWrapper::calcObserverTEMEPosition(Vec3d& ao_position, Vec3d& ao_vel)
{

	StelLocation loc   = StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation();
	double radLatitude = loc.latitude * KDEG2RAD;
	double theta       = Epoch.toThetaLMST(loc.longitude * KDEG2RAD);
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
	ao_vel[0] = -KMFACTOR*ao_position[1];/*kilometers/second*/
	ao_vel[1] =  KMFACTOR*ao_position[0];
	ao_vel[2] =  0;
}



Vec3d gSatStelWrapper::getAltAz()
{

	StelLocation loc   = StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation();
	Vec3d topoSatPos;
	Vec3d observerECIPos;
	Vec3d observerECIVel;

	double  radLatitude    = loc.latitude * KDEG2RAD;
	double  theta          = Epoch.toThetaLMST(loc.longitude * KDEG2RAD);

	calcObserverTEMEPosition(observerECIPos, observerECIVel);

	Vec3d satECIPos  = getTEMEPos();
	Vec3d slantRange = satECIPos - observerECIPos;

	//top_s
	topoSatPos[0] = (sin(radLatitude) * cos(theta)*slantRange[0]
	                 + sin(radLatitude)* sin(theta)*slantRange[1]
	                 - cos(radLatitude)* slantRange[2])/AU;
	//top_e
	topoSatPos[1] = ((-1.0)* sin(theta)*slantRange[0]
	                 + cos(theta)*slantRange[1])/AU;

	//top_z
	topoSatPos[2] = (cos(radLatitude) * cos(theta)*slantRange[0]
	                 + cos(radLatitude) * sin(theta)*slantRange[1]
	                 + sin(radLatitude) *slantRange[2])/AU;

	return topoSatPos;
}

void  gSatStelWrapper::getSlantRange(double &ao_slantRange, double &ao_slantRangeRate)
{

	Vec3d observerECIPos;
	Vec3d observerECIVel;

	calcObserverTEMEPosition(observerECIPos, observerECIVel);


	Vec3d satECIPos = getTEMEPos();
	Vec3d satECIVel = getTEMEVel();
	Vec3d slantRange = satECIPos - observerECIPos;
	Vec3d rangeVel   = satECIVel - observerECIVel;

	ao_slantRange     = slantRange.length();
	ao_slantRangeRate = slantRange.dot(rangeVel)/ao_slantRange;
}





