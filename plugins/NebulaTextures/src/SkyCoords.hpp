/*
 * Nebula Textures plug-in for Stellarium
 *
 * Copyright (C) 2024-2025 WANG Siliang
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SKYCOORDS_HPP
#define SKYCOORDS_HPP

#include "StelUtils.hpp"
#include <QPair>
#include <QJsonArray>

constexpr double D2R = M_PI / 180.0;
constexpr double R2D = 180.0 / M_PI;

class SkyCoords
{
public:
	//! Converts image pixel coordinates to celestial coordinates (RA, Dec).
	static QPair<double, double> pixelToRaDec(int X, int Y,
		double CRPIX1, double CRPIX2,
		double CRVAL1, double CRVAL2,
		double CD1_1, double CD1_2,
		double CD2_1, double CD2_2);

	//! Calculates the center point (RA, Dec) from a set of celestial corner coordinates.
	static QPair<double, double> calculateCenter(const QJsonArray& corners);
};

#endif // SKYCOORDS_HPP
