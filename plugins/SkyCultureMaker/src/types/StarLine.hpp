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

#ifndef SCM_TYPES_STAR_LINE_HPP
#define SCM_TYPES_STAR_LINE_HPP

#include <optional>
#include <QJsonArray>
#include <QRegularExpression>
#include <QString>

namespace scm
{
//! The pair of optional start and end stars
struct StarLine
{
	//! The start star of the line.
	std::optional<QString> start;

	//! The end star of the line.
	std::optional<QString> end;

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
     * @brief Converts the StarLine to a JSON array.
     *
     * @return QJsonArray The JSON representation of the star line.
     */
	QJsonArray toJson() const
	{
		QJsonArray json;

		if (start.has_value())
		{
			QString formattedStarId = getFormattedStarId(start.value());

			if (start.value().contains("HIP"))
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
			json.append("-1");
		}

		if (end.has_value())
		{
			QString formattedStarId = getFormattedStarId(end.value());

			if (end.value().contains("HIP"))
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
			json.append("-1");
		}

		return json;
	}
};
} // namespace scm

#endif
