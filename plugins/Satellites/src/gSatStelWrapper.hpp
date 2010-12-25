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

#ifndef _GSATSTELWRAPPER_HPP_
#define _GSATSTELWRAPPER_HPP_ 1

#include <QString>

#include "VecMath.hpp"
#include "StelNavigator.hpp"

#include "gsatellite/gSatTEME.hpp"


class gSatStelWrapper
{

public:
	gSatStelWrapper(QString designation, QString tle1,QString tle2);
	~gSatStelWrapper();


	void setEpoch(const StelNavigator*);

	Vec3d getPos();
	Vec3d getVel();
	Vec3d getSubPoint();

private:
	gSatTEME *pSatellite;

};





#endif
