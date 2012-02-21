/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2010 Bogdan Marinov
 * 
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _INTERPOLATED_POSITION_HPP_
#define _INTERPOLATED_POSITION_HPP_

#ifndef INT64_MAX
#define INT64_MAX 0x7FFFFFFFFFFFFFFFLL
#endif

#include "VecMath.hpp"

//! A telescope's position at a given time.
//! This structure used to be defined inline in TelescopeTCP.
struct Position
{
	qint64 server_micros;
	qint64 client_micros;
	Vec3d pos;
	int status;
};

class InterpolatedPosition {
public:
	InterpolatedPosition();
	~InterpolatedPosition();
	
	void add(Vec3d& position, qint64 clientTime, qint64 serverTime, int status = 0);
	//! returns the current interpolated position
	Vec3d get(qint64 time) const;
	//! resets/initializes the array of positions kept for position interpolation
	void reset();
	bool isKnown() const {return (position_pointer->client_micros != INT64_MAX);}
	
private:
	Position positions[16];
	Position *position_pointer;
	Position *const end_position;
};
 
 #endif //_INTEPOLATED_POSITION_HPP_
