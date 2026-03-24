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

	CulturePolygon(int id, int beginTime, QString endTime, QPolygonF polygon)
		: id(id), beginTime(beginTime), endTime(endTime), polygon(polygon)
	{
	}

	//! The identifier of the polygon.
	int id = 0;

	//! The start time of the polygon.
	int beginTime = 0;

	//! The end time of the polygon.
	QString endTime = "0";

	//! The settlement area of the culture.
	QPolygonF polygon;

	/**
	 * @brief Converts the CulturePolygon to a JSON object.
     *
	 * @return QJsonObject The JSON representation of the culture polygon.
     */
	QJsonObject toJson() const
	{
		QJsonObject jsonObject;

		jsonObject["beginTime"] = beginTime;
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

	/**
	 * @brief Converts the CulturePolygon to a (Geo)JSON object.
	 *
	 * @return QJsonObejct The (Geo)JSON representation of the culture polygon.
	 */
	QJsonObject toGeoJson() const
	{
		QJsonObject geoJsonObject;
		QJsonObject propertiesObject;
		QJsonObject geometryObject;

		geoJsonObject["type"] = "Feature";

		propertiesObject["id"] = id;
		propertiesObject["beginTime"] = beginTime;
		propertiesObject["endTime"] = endTime;
		geoJsonObject["properties"] = propertiesObject;

		geometryObject["type"] = "Polygon";
		QJsonArray coordinatesArray;
		QJsonArray polygonArray;
		for (const auto &point : polygon)
		{
			QJsonArray currentPoint;
			currentPoint.append(point.x());
			currentPoint.append(point.y());
			polygonArray.append(currentPoint);
		}
		// according to the GeoJSON standard, the last point must match with the first point of the polygon
		QJsonArray currentPoint;
		currentPoint.append(polygon[0].x());
		currentPoint.append(polygon[0].y());
		polygonArray.append(currentPoint);

		// append other arrays to coordinatesArray for holes in the main Polygon (for now SCM does not support those, but may in the future)
		coordinatesArray.append(polygonArray);
		geometryObject["coordinates"] = coordinatesArray;
		geoJsonObject["geometry"] = geometryObject;

		return geoJsonObject;
	}
};
} // namespace scm

#endif
