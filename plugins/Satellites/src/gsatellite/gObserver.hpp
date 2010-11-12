/***************************************************************************
 * Name: gObserver.hpp
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2006 by J. L. Canales                                   *
 *   jlcanales@users.sourceforge.net                                       *
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
#ifndef _GOBSERVER_HPP_
#define _GOBSERVER_HPP_ 1

// stdsat
#include "stdsat.h"
#include "gVector.hpp"
#include "gTime.hpp"
#include "gSatTEME.hpp"

#define AZIMUTH   0
#define ELEVATION 1
#define RANGE     2
#define RANGERATE 3

//! @class gObserver
//! This class implements the need functionality to change
//! coordinates from Geographic Reference System to Observer Reference System
class gObserver
{
public:
	//## Constructors (generated)
	//! @brief Default class gObserver constructor
	//! @param[in] 	ai_latitude Observer Geographic latitude
	//! @param[in] 	ai_longitude Observer Geographic longitude
	//! @param[in] 	ai_attitude Observer Geographic attitude
	gObserver(double ai_latitude=0, double ai_longitude=0 , double ai_attitude=0):
		m_latitude(ai_latitude),m_longitude(ai_longitude), m_attitude(ai_attitude)
	{
		;
	}

	//!	Method used to get observer latitude.
	//!
	double getLatitude()
	{
		return m_latitude;
	}

	//!	Method used to get observer object longitude.
	//!
	double getLongitude()
	{
		return m_longitude;
	}

	//!	Method used to set Observer geographic position.
	//!
	void setPosition(double ai_latitude, double ai_longitude, double ai_attitude)
	{
		m_latitude  = ai_latitude;
		m_longitude = ai_longitude;
		m_attitude  = ai_attitude;
	}



	// Operation getECIPosition
	//! @brief This operation compute the observer ECI coordinates for the
	//! ai_epoch time
	//! @details
	//! References:
	//!  Orbital Coordinate Systems, Part II
	//!   Dr. T.S. Kelso
	//!   http://www.celestrak.com/columns/v02n02/
	//! @param[in] ai_epoch  Epoch for the ECI reference system calculation
	//! @param[out] ao_position Observer position vector
	//! @param[out] ao_vel Observer velocity vector
	//!   gVector  Vector including X,Y,Z ECI Coordinates
	void getECIPosition(gTime ai_epoch, gVector& ao_position, gVector& ao_vel);

	// Operation calculateLook
	//! @brief This operation compute the Azimuth, Elevation and Range of
	//! a satellite from the observer site.
	//! @param[in] ai_Sat   Sat TEME object to be looked.
	//! @param[in] ai_epoch  Epoch for the ECI reference system calculation
	//! @return gVector Vector including Az, El, Range coordinates
	//! References:
	//!  Orbital Coordinate Systems, Part II
	//!   Dr. T.S. Kelso
	//!   http://www.celestrak.com/columns/v02n02/
	gVector calculateLook(gSatTEME ai_Sat, gTime ai_epoch);

private: //## implementation

	double m_latitude;  //meassured in degrees
	double m_longitude; //meassured in degrees
	double m_attitude;

};

#endif // _GOBSERVER_HPP_
