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



#include "gSatStelWrapper.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

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
}


gSatStelWrapper::~gSatStelWrapper()
{
	if(pSatellite != NULL)
		delete pSatellite;
}


Vec3d gSatStelWrapper::getTEMEPos()
{
	gVector tempPos;
	Vec3d returnedVector;
	if(pSatellite != NULL)
	{
		tempPos = pSatellite->getPos();
		returnedVector.set( tempPos[0], tempPos[1], tempPos[2]);
	}
	else
		qWarning() << "gSatStelWrapper::getTEMEPos Method called without pSatellite initialized";

	return returnedVector;

}


Vec3d gSatStelWrapper::getTEMEVel()
{
	gVector tempVel;
	Vec3d returnedVector;
	if(pSatellite != NULL)
	{
		tempVel = pSatellite->getVel();
		returnedVector.set( tempVel[0], tempVel[1], tempVel[2]);
	}
	else
		qWarning() << "gSatStelWrapper::getTEMEVel Method called without pSatellite initialized";

	return returnedVector;

}


Vec3d gSatStelWrapper::getSubPoint()
{
	gVector tempSubPoint;
	Vec3d returnedVector;
	if(pSatellite != NULL)
	{
		tempSubPoint = pSatellite->getSubPoint();
		returnedVector.set( tempSubPoint[0], tempSubPoint[1], tempSubPoint[2]);
	}
	else
		qWarning() << "gSatStelWrapper::getTEMEVel Method called without pSatellite initialized";

	return returnedVector;
}


void gSatStelWrapper::updateEpoch(){


	double jul_utc = StelApp::getInstance().getCore()->getNavigator()->getJDay();
	gTime epochTime( jul_utc);

	if (pSatellite)
		pSatellite->setEpoch( epochTime);
}
