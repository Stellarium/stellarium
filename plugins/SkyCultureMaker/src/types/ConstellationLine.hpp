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

#ifndef SCM_TYPES_CONSTELLATION_LINE_HPP
#define SCM_TYPES_CONSTELLATION_LINE_HPP

#include "StelUtils.hpp"
#include "VecMath.hpp"
#include <optional>
#include <QJsonArray>
#include <QRegularExpression>
#include <QString>
#include "SkyPoint.hpp"

namespace scm
{
struct ConstellationLine
{
	//! The start point of the line.
	SkyPoint start;

	//! The end point of the line.
	SkyPoint end;

	/**
	 * @brief Gets the formatted star Id from the name.
	 * 
	 * @param starId The Id of the star or DSO, which may contain identifiers like "HIP" or "Gaia DR3".
	 * @return QString The formatted star Id, either as plain number for HIP or Gaia, or as "DSO:<name>" for others.
	 * 
	 */
	static QString getFormattedStarId(QString starId)
	{
		QRegularExpression hipExpression(R"(HIP\s+(\d+))");
		QRegularExpression gaiaExpression(R"(Gaia DR3\s+(\d+))");

		QRegularExpressionMatch hipMatch = hipExpression.match(starId);
		if (hipMatch.hasMatch())
		{
			return hipMatch.captured(1);
		}
		QRegularExpressionMatch gaiaMatch = gaiaExpression.match(starId);
		if (gaiaMatch.hasMatch())
		{
			return gaiaMatch.captured(1);
		}
		// Neither HIP nor Gaia, return as DSO
		return "DSO:" + starId.remove(' ');
	}

	/**
     * @brief Converts the stars of the line to a JSON array.
     *
     * @return QJsonArray The JSON representation of the stars of the line.
     */
	QJsonArray starsToJson() const
	{
		QJsonArray json;

		if (!start.star.isEmpty())
		{
			QString formattedStarId = getFormattedStarId(start.star);

			if (start.star.contains("HIP"))
			{
				// HIP are required as number
				json.append(formattedStarId.toLongLong());
			}
			else
			{
				// Other ids are required as string
				json.append(formattedStarId);
			}
		}
		else
		{
			json.append("null");
		}

		if (!end.star.isEmpty())
		{
			QString formattedStarId = getFormattedStarId(end.star);

			if (end.star.contains("HIP"))
			{
				// HIP are required as number
				json.append(formattedStarId.toLongLong());
			}
			else
			{
				// Other ids are required as string
				json.append(formattedStarId);
			}
		}
		else
		{
			json.append("null");
		}

		return json;
	}

	/**
    * @brief Converts the coordinates of the line to a JSON array.
    * 
    * @return QJsonArray The JSON representation of the coordinates of the line.
    */
	QJsonArray coordinatesToJson() const
    {
		QJsonArray json;
        QJsonArray startCoordinateArray;
        double RA, DE;
        convertToSphereCoords(RA, DE, start.coordinate);
        startCoordinateArray.append(RA);
        startCoordinateArray.append(DE);
        json.append(startCoordinateArray);

        QJsonArray endCoordinateArray;
        convertToSphereCoords(RA, DE, end.coordinate);
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
} // namespace scm

#endif
