/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
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

#ifndef SCM_TYPES_COORDINATE_LINE_HPP
#define SCM_TYPES_COORDINATE_LINE_HPP

#include "VecMath.hpp"
#include <QJsonArray>
#include "StelUtils.hpp"

namespace scm
{
//! The pair of start and end coordinate
struct CoordinateLine
{
	//! The start coordinate of the line.
	Vec3d start;

	//! The end coordinate of the line.
	Vec3d end;	
    
    /**
    * @brief Converts the ConstellationLine to a JSON array.
    * 
    * @return QJsonArray The JSON representation of the coordinate line.
    */
	QJsonArray toJson() const
    {
		QJsonArray json;

        // Only if both start and end points do not have names, we save the coordinates
        QJsonArray startCoordinateArray;
        double RA, DE;
        convertToSphereCoords(RA, DE, start);
        startCoordinateArray.append(RA);
        startCoordinateArray.append(DE);
        json.append(startCoordinateArray);

        QJsonArray endCoordinateArray;
        convertToSphereCoords(RA, DE, end);
        endCoordinateArray.append(RA);
        endCoordinateArray.append(DE);
        json.append(endCoordinateArray);

		return json;
	}

private:
    static void convertToSphereCoords(double &RA, double &DE, const Vec3d &vec)
    {
        double longitude;
        double latitude;
        StelUtils::rectToSphe(&longitude, &latitude, vec);
        RA = longitude * M_180_PI / 15.0;
        DE = latitude * M_180_PI;
    }
};
}  // namespace scm

#endif