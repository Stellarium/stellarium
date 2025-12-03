/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Moritz Rätz
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

#ifndef SCM_TYPES_CULTURE_POLYGON_HPP
#define SCM_TYPES_CULTURE_POLYGON_HPP

#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qpolygon.h>

namespace scm
{
//! A settlement area describing polygon with a start and end time
struct CulturePolygon
{
	CulturePolygon() = default;

	CulturePolygon(int id, int startTime, QString endTime, QPolygonF polygon)
		: id(id), startTime(startTime), endTime(endTime), polygon(polygon)
	{
	}

	//! The identifier of the polygon.
	int id = 0;

	//! The start time of the polygon.
	int startTime = 0;

	//! The end time of the polygon.
	QString endTime = "0";

	//! The settlement area of the culture.
	QPolygonF polygon;

	/**
	 * @brief Converts the CulturePolygon to a JSON object.
     *
	 * @return QJsonObejct The JSON representation of the culture polygon.
     */
	QJsonObject toJson() const
	{
		QJsonObject jsonObject;

		jsonObject["startTime"] = startTime;
		jsonObject["endTime"] = endTime;

		QJsonArray poly;
		for (const auto &point : polygon)
		{
			QJsonArray currentPoint;
			currentPoint.append(point.x());
			currentPoint.append(point.y());
			poly.append(currentPoint);
		}
		jsonObject["geometry"] = poly;

		return jsonObject;
	}
};
} // namespace scm

#endif
