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
 *   51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.             *
 ***************************************************************************/

#ifndef _GSATWRAPPER_HPP_
#define _GSATWRAPPER_HPP_ 1

#include <QString>

#include "VecMath.hpp"

#include "gsatellite/gSatTEME.hpp"
#include "gsatellite/gTime.hpp"

//constants for predict visibility
#define  RADAR_SUN   1
#define  VISIBLE     2
#define  RADAR_NIGHT 3
#define  NOT_VISIBLE 4

class gSatWrapper
{

public:
        gSatWrapper(QString designation, QString tle1,QString tle2);
        ~gSatWrapper();

	// Operation updateEpoch
	//! @brief This operation update Epoch timestamp for gSatTEME object
	//! from Stellarium Julian Date.
	//! @return void
	void updateEpoch();

	void setEpoch(double ai_julianDaysEpoch);

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
        //!    Altitude:  Coord[2]  measured in Km.\n
	Vec3d getSubPoint();

        // Operation getAltAz
	//! @brief This operation compute the coordinates in StelCore::FrameAltAz
        //! @return Vect3d Vector with coordinates (Meassured in Km)
        //! @ref
	//!  Orbital Coordinate Systems, Part II
	//!   Dr. T.S. Kelso
	//!   http://www.celestrak.com/columns/v02n02/
	Vec3d getAltAz();

        // Operation getSlantRange
        //! @brief This operation compute the slant range (distance between the
        //! satellite and the observer) and its variation/seg
        //! @param &ao_slantRange Reference to a output variable where the method store the slant range measured in Km
        //! @param &ao_slantRangeRate Reference to a output variable where the method store the slant range variation in Km/s
        //! @return void
	void  getSlantRange(double &ao_slantRange, double &ao_slantRangeRate); //meassured in km and km/s


        // Operation getVisibilityPredict
        //! @brief This operation predicts the satellite visibility contidions.
        //! This prediction can return 4 different states
        //!   RADAR_SUN when satellite an observer are in the sunlit
        //!   VISIBLE   when satellite is in sunlit and observer is in the dark. Satellite could be visible in the sky.
        //!   RADAR_NIGHT when satellite is eclipsed by the earth shadow.
        //!   NOT_VISIBLE The satellite is under the observer horizon
        //! @return
        //!     1 if RADAR_SUN
        //!     2 if VISIBLE
        //!     3 if RADAR_NIGHt
        //!     3 if NOT_VISIBLE
        //! @ref
        //!   Fundamentals of Astrodynamis and Applications (Third Edition) pg 898
        //!   David A. Vallado
        int getVisibilityPredict();


private:
        // Operation calcObserverECIPosition
        //! @brief This operation compute the observer ECI coordinates in Geocentric
        //! Ecuatorial Coordinate System (IJK) for the ai_epoch time.
        //! This position can be asumed as observer position in TEME framework without an appreciable error.
        //! ECI axis (IJK) are parallel to StelCore::EquinoxEQ Framework but centered in the earth centre
        //! instead the observer position.
	//! @details
	//! References:
	//!  Orbital Coordinate Systems, Part II
	//!   Dr. T.S. Kelso
	//!   http://www.celestrak.com/columns/v02n02/
        //! @param[out] ao_position Observer ECI position vector measured in Km
        //! @param[out] ao_vel Observer ECI velocity vector measured in Km/s
        void calcObserverECIPosition(Vec3d& ao_position, Vec3d& ao_vel);


private:
	gSatTEME *pSatellite;
        gTime	 epoch;

};

#endif
