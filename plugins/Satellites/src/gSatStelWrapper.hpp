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
#include "gsatellite/gTime.hpp"

class gSatStelWrapper
{

	public:
		gSatStelWrapper(QString designation, QString tle1,QString tle2);
		~gSatStelWrapper();

		// Operation updateEpoch
		//! @brief This operation update Epoch timestamp for gSatTEME object
		//! from Stellarium Julian Date.
		//! @return void
		void updateEpoch();

		// Operation getTEMEPos
		//! @brief This operation isolate gSatTEME getPos operation.
		//! @return Vec3d with TEME position. Units measured in Km.
		Vec3d getTEMEPos();

		// Operation getTEMEVel
		//! @brief This operation isolate gSatTEME getVel operation.
		//! @return Vec3d with TEME speed. Units measured in Km/s.
		Vec3d getTEMEVel();

		// Operation:  getSubPoint
		//! @brief This operation isolate getSubPoint method of gSatTEME object.
		//! @return Vec3d Geographical coordinates\n
		//!    Latitude:  Coord[0]  measured in degrees\n
		//!    Longitude: Coord[1]  measured in degrees\n
		//!	   Altitude:  Coord[2]  measured in Km.\n
		Vec3d getSubPoint();

		// Operation calculateLook
		//! @brief This operation compute the coordinates in StelCore::FrameAltAz
		//! @return Vect3d Vector with coordinates
		//! References:
		//!  Orbital Coordinate Systems, Part II
		//!   Dr. T.S. Kelso
		//!   http://www.celestrak.com/columns/v02n02/
		Vec3d getAltAz();


		void  getSlantRange(double &ao_slantRange, double &ao_slantRangeRate); //meassured in km and km/s
		Vec3d getEquinoxEqu();
		Vec3d getJ2000();


	private:
		// Operation calcObserverTEMEPosition
		//! @brief This operation compute the observer ECI coordinates in TEME framework
		//! for the ai_epoch time
		//! @details
		//! References:
		//!  Orbital Coordinate Systems, Part II
		//!   Dr. T.S. Kelso
		//!   http://www.celestrak.com/columns/v02n02/
		//! @param[out] ao_position Observer TEME position vector measured in Km
		//! @param[out] ao_vel Observer TEME velocity vector measured in Km/s
		void calcObserverTEMEPosition(Vec3d& ao_position, Vec3d& ao_vel);


	private:
		gSatTEME *pSatellite;
		gTime	 Epoch;

};





#endif
