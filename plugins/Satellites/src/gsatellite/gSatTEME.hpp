/***************************************************************************
 * Name: gSatTeme.hpp
 *
 * Description: gSatTEME classes declaration.
 *              This class abstract all the SGP4 complexity. It uses the
 *              revised David. A. Vallado code for SGP4 Calculation.
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

#ifndef GSATTEME_HPP
#define GSATTEME_HPP

#include "gTime.hpp"
#include "VecMath.hpp"
#include "StelUtils.hpp"

#include "OMM.hpp"
#include "SGP4.h" // Upgraded to SGP4 Version 2020-07-13

#include "stdsat.h"

struct elsetrec_data
{
	double epoch;
	double bstar;
	double ndot;
	double nndot;
	double ecco;
	double argpo;
	double inclo;
	double mo;
	double no_kozai;
	double nodeo;
	double jdsatepoch;
	double jdsatepochF;

	void set(elsetrec* p) {
		epoch = p->epochdays;
		bstar = p->bstar;
		ndot = p->ndot;
		nndot = p->nddot;
		ecco = p->ecco;
		argpo = p->argpo;
		inclo = p->inclo;
		mo = p->mo;
		no_kozai = p->no_kozai;
		nodeo = p->nodeo;
		jdsatepoch = p->jdsatepoch;
		jdsatepochF = p->jdsatepochF;
	}
};

//! @class gSatTEME
//! @brief Sat position and velocity predictions over TEME reference system.
//! @details
//! Class to abstract all the SGP4 complexity.
//! It implementation wrap whit an object oriented class the revised David. A. Vallado
//! code for SGP4 Calculation. (Spacetrack report #6 revised AIAA-2006-6753-rev1)
//! @ingroup satellites
class gSatTEME
{
public:
	// Operation: gSatTEME(const char *pstrName, char *pstrTleLine1, char *pstrTleLine2)
	//! @brief Default class gSatTEME constructor
	//! @param[in] 	pstrName Pointer to a null end string with the Sat. Name
	//! @param[in] 	pstrTleLine1 Pointer to a null end string with the
	//!             first TLE Kep. data line
	//! @param[in] 	pstrTleLine2 Pointer to a null end string with the
	//!             second TLE Kep. data line
	gSatTEME(const char *pstrName, char *pstrTleLine1, char *pstrTleLine2);

	gSatTEME(const OMM& omm);

	// Operation: setEpoch( gTime ai_time)
	//! @brief Set compute epoch for prediction
	//! @param[in] 	ai_time gTime object storing the compute epoch time.
	void setEpoch(gTime ai_time);

	// Operation: setEpoch( double ai_time)
	//! @brief Set compute epoch for prediction
	//! @param[in] 	ai_time double variable storing the compute epoch time in Julian Days.
	void setEpoch(double ai_time)
	{
		gTime time(ai_time);
		return setEpoch( time);
	}

	// Operation: setMinSinceKepEpoch( double ai_minSinceKepEpoch)
	//! @brief Set compute epoch for prediction in minutes since Keplerian data Epoch
	//! @param[in] 	ai_minSinceKepEpoch Time since Keplerian Epoch measured in minutes
	//! and fraction of minutes.
	void setMinSinceKepEpoch(double ai_minSinceKepEpoch);

	// Operation: getPos()
	//! @brief Get the TEME satellite position Vector
	//! @return Vec3d
	//!   Satellite position vector measured in Km.
	//!    x: position[0]
	//!    y: position[1]
	//!    z: position[2]
	const Vec3d& getPos() const
	{
		return m_Position;
	}

	// Operation: getVel()
	//! @brief Get the TEME satellite Velocity Vector
	//! @return Vec3d Satellite Velocity Vector measured in Km/s
	//!    x: Vel[0]\n
	//!    y: Vel[1]\n
	//!    z: Vel[2]\n
	const Vec3d& getVel() const
	{
		return m_Vel;
	}

	// Operation:  getSubPoint
	//! @brief Get the Geographic satellite subpoint Vector calculated by the method compute SubPoint
	//! @details To implement this operation, next references has been used:
	//!	   Orbital Coordinate Systems, Part III  By Dr. T.S. Kelso
	//!	   https://celestrak.org/columns/v02n03/
	//! @return Vec3d Geographical coordinates\n
	//!    Latitude:  Coord[0]  measured in degrees\n
	//!    Longitude: Coord[1]  measured in degrees\n
	//!	   Altitude:  Coord[2]  measured in Km.\n
	const Vec3d& getSubPoint() const
	{
		return m_SubPoint;
	}

	int getErrorCode() const
	{
		return satrec.error;
	}

	double getPeriod() const
	{
		// Get orbital period from mean motion (rad/min)
		double mm = satrec.no_kozai;
		if (mm > 0.0) {
			return 2 * M_PI / mm;
		}

		return 0.0;
	}

	// Get orbital inclination in degrees
	double getInclination() const
	{
		return satrec.inclo*180./M_PI;
	}

	Vec2d getPerigeeApogee() const
	{
		double semiMajorAxis = std::cbrt((xke/satrec.no_kozai)*(xke/satrec.no_kozai));
		return Vec2d((semiMajorAxis*(1.0 - satrec.ecco) - 1.0)*EARTH_RADIUS, (semiMajorAxis*(1.0 + satrec.ecco) - 1.0)*EARTH_RADIUS);
	}

private:
	// Operation:  computeSubPoint
	//! @brief Compute the Geographic satellite subpoint Vector
	//! @details To implement this operation, next references has been used:
	//!	   Orbital Coordinate Systems, Part III  By Dr. T.S. Kelso
	//!	   https://celestrak.org/columns/v02n03/
	//! @param[in] ai_Time Epoch time for subpoint calculation. (of course, this must be
	//!    refactorized to be computed in the setEpoch function)
	//! @return Vec3d Geographical coordinates\n
	//!    Latitude:  Coord[0]  measured in degrees\n
	//!    Longitude: Coord[1]  measured in degrees\n
	//!	   Altitude:  Coord[2]  measured in Km.\n
	Vec3d computeSubPoint(gTime ai_time);

	// sgp4 processes variables
	double tumin{}, mu{}, radiusearthkm{}, xke{}, j2{}, j3{}, j4{}, j3oj2{};
	elsetrec satrec{};
	elsetrec_data debug_data;

	std::string  m_SatName{};
	Vec3d m_Position{};
	Vec3d m_Vel{};
	Vec3d m_SubPoint{};
};

#endif // GSATTEME_HPP
