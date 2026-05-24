/*
 * Copyright (C) 2013 Felix Zeltner
 * Copyright (C) 2026 Kamil Zaraś (astronow.pl)
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

#ifndef PLANES_AIRCRAFTRECORD_HPP
#define PLANES_AIRCRAFTRECORD_HPP

#include <QString>

struct AircraftRecord
{
	QString icao24;
	QString callsign;
	QString aircraftType;
	double latitude = 0.0;
	double longitude = 0.0;
	double altitudeMeters = 0.0;
	double groundSpeedMs = 0.0;
	double trackDegrees = 0.0;
	double verticalRateMs = 0.0;
	double snapshotJd = 0.0;
};

#endif
