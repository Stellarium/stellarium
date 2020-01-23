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

#ifndef GSATWRAPPER_HPP
#define GSATWRAPPER_HPP

#include <QString>

#include "VecMath.hpp"

#include "gsatellite/gSatTEME.hpp"
#include "gsatellite/gTime.hpp"

//! Wrapper allowing compatibility between gsat and Stellarium/Qt.
//! @ingroup satellites
class gSatWrapper
{
public:
	enum Visibility
	{
		//constants for visibility prediction
		UNKNOWN=0,
		RADAR_SUN=1,
		VISIBLE=2,
		RADAR_NIGHT=3,
		NOT_VISIBLE=4
	};
        gSatWrapper(QString designation, QString tle1,QString tle2);
        ~gSatWrapper();

	// Operation setEpoch
	//! @brief This operation update Epoch timestamp for gSatTEME object
	//! from Stellarium Julian Date.
	void setEpoch(double ai_julianDaysEpoch);

	// Operation getTEMEPos
	//! @brief This operation isolate gSatTEME getPos operation.
	//! @return Vec3d with TEME position. Units measured in Km.
	Vec3d getTEMEPos() const;

	// Operation getSunECIPos
	//! @brief Get Sun positions in ECI system.
	//! @return Vec3d with ECI position.
	static Vec3d getSunECIPos();

	// Operation getTEMEVel
	//! @brief This operation isolate gSatTEME getVel operation.
	//! @return Vec3d with TEME speed. Units measured in Km/s.
	Vec3d getTEMEVel() const;

	// Operation:  getSubPoint
	//! @brief This operation isolate getSubPoint method of gSatTEME object.
	//! @return Vec3d Geographical coordinates\n
	//!    Latitude:  Coord[0]  measured in degrees\n
	//!    Longitude: Coord[1]  measured in degrees\n
        //!    Altitude:  Coord[2]  measured in Km.\n
	Vec3d getSubPoint() const;

	// Operation getAltAz
	//! @brief This operation compute the coordinates in StelCore::FrameAltAz
	//! @return Vect3d Vector with coordinates (meassured in km)
	//! @par References
	//!  Orbital Coordinate Systems, Part II
	//!   Dr. T.S. Kelso
	//!   http://www.celestrak.com/columns/v02n02/
	Vec3d getAltAz() const;

        // Operation getSlantRange
        //! @brief This operation compute the slant range (distance between the
        //! satellite and the observer) and its variation/seg
        //! @param &ao_slantRange Reference to a output variable where the method store the slant range measured in Km
        //! @param &ao_slantRangeRate Reference to a output variable where the method store the slant range variation in Km/s
        //! @return void
	void  getSlantRange(double &ao_slantRange, double &ao_slantRangeRate) const; //measured in km and km/s

        // Operation getVisibilityPredict
        //! @brief This operation predicts the satellite visibility contidions.
        //! This prediction can return 4 different states
	//!   RADAR_SUN when satellite and observer are in the sunlight
	//!   VISIBLE   when satellite is in sunlight and observer is in the dark. Satellite could be visible in the sky.
        //!   RADAR_NIGHT when satellite is eclipsed by the earth shadow.
        //!   NOT_VISIBLE The satellite is under the observer horizon
        //! @return
        //!     1 if RADAR_SUN
        //!     2 if VISIBLE
	//!     3 if RADAR_NIGHT
        //!     3 if NOT_VISIBLE
        //! @par References
        //!   Fundamentals of Astrodynamis and Applications (Third Edition) pg 898
        //!   David A. Vallado
	Visibility getVisibilityPredict() const;

	double getPhaseAngle() const;
	//! Get orbital period in minutes
	double getOrbitalPeriod() const;
	//! Get orbital inclination in degrees
	double getOrbitalInclination() const;
	//! Get perigee/apogee altitudes in kilometers for equatorial radius of Earth
	Vec2d getPerigeeApogeeAltitudes() const;
	static gTime getEpoch() { return epoch; }

	// Operation calcObserverECIPosition
	//! @brief This operation computes the observer ECI coordinates in Geocentric
	//! Equatorial Coordinate System (IJK) for the ai_epoch time.
        //! This position can be asumed as observer position in TEME framework without an appreciable error.
        //! ECI axis (IJK) are parallel to StelCore::EquinoxEQ Framework but centered in the earth centre
        //! instead the observer position.
	//! @par References
	//!  Orbital Coordinate Systems, Part II
	//!   Dr. T.S. Kelso
	//!   http://www.celestrak.com/columns/v02n02/
        //! @param[out] ao_position Observer ECI position vector measured in Km
        //! @param[out] ao_vel Observer ECI velocity vector measured in Km/s
	static void calcObserverECIPosition(Vec3d& ao_position, Vec3d& ao_vel) ;

private:
	//! do the actual work to compute a cached value.
	static void updateSunECIPos();

	gSatTEME *pSatellite;
	static gTime	 epoch;

	// GZ We can avoid many computations (solar and observer positions for every satellite) by computing them only once for all objects.
	static gTime lastSunECIepoch; // store last time of computation to avoid all-1 computations.
	static Vec3d sunECIPos;       // enough to have these once.
	static Vec3d observerECIPos;
	static Vec3d observerECIVel;
	static gTime lastCalcObserverECIPosition;
};

#endif
