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
 *              Conference, Keystone, CO, 2006 August 21–24.
 *              http://celestrak.com/publications/AIAA/2006-6753/
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

#ifndef _GSATTEME_HPP_
#define _GSATTEME_HPP_ 1

#include "gTime.hpp"
#include "gVector.hpp"

#include "sgp4unit.h"
#include "sgp4ext.h"


//! @class gSatTEME
//! @brief Sat position and velocity predictions over TEME reference system.
//! @details
//! Class to abstract all the SGP4 complexity.
//! It implementation wrap whit an object oriented class the revised David. A. Vallado
//! code for SGP4 Calculation. (Spacetrack report #6 revised AIAA-2006-6753-rev1)
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
	//! @return gVector
	//!   Satellite position vector meassured in Km.
	//!    x: position[0]
	//!    y: position[1]
	//!    z: position[2]
	gVector getPos()
	{
		return m_Position;
	}

	// Operation: getVel()
	//! @brief Get the TEME satellite Velocity Vector
	//! @return gVector Satellite Velocity Vector measured in Km/s
	//!    x: Vel[0]\n
	//!    y: Vel[1]\n
	//!    z: Vel[2]\n
	gVector getVel()
	{
		return m_Vel;
	}




	// Operation:  getSubPoint
	//! @brief Get the Geographic satellite subpoint Vector calculated by the method compute SubPoint
	//! @details To implement this operation, next references has been used:
	//!	   Orbital Coordinate Systems, Part III  By Dr. T.S. Kelso
	//!	   http://www.celestrak.com/columns/v02n03/
	//! @return gVector Geographical coordinates\n
	//!    Latitude:  Coord[0]  measured in degrees\n
	//!    Longitude: Coord[1]  measured in degrees\n
	//!	   Altitude:  Coord[2]  measured in Km.\n
	gVector getSubPoint()
	{
		return m_SubPoint;
	}

	int getErrorCode()
	{
		return satrec.error;
	}

private:
	// Operation:  computeSubPoint
	//! @brief Compute the Geographic satellite subpoint Vector
	//! @details To implement this operation, next references has been used:
	//!	   Orbital Coordinate Systems, Part III  By Dr. T.S. Kelso
	//!	   http://www.celestrak.com/columns/v02n03/
	//! @param[in] ai_Time Epoch time for subpoint calculation. (of course, this must be
	//!    refactorized to be computed in the setEpoch function)
	//! @return gVector Geographical coordinates\n
	//!    Latitude:  Coord[0]  measured in degrees\n
	//!    Longitude: Coord[1]  measured in degrees\n
	//!	   Altitude:  Coord[2]  measured in Km.\n
	gVector computeSubPoint( gTime ai_time);


	// sgp4 proceses variables
	double tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2;
	elsetrec satrec;

	std::string  m_SatName;
	gVector 	 m_Position;
	gVector 	 m_Vel;
	gVector		 m_SubPoint;
};

#endif // _GSATTEME_HPP_
