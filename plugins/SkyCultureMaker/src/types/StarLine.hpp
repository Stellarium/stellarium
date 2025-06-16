/**
 * @file StarLine.hpp
 * @author vgerlach, lgrumbach
 * @brief Type describing a line between two stars.
 * @version 0.1
 * @date 2025-06-02
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
	 * @brief Gets the star ID from the name.
	 * 
	 * @param starId The ID of the star, which may contain identifiers like "HIP" or "Gaia DR3".
	 */
	static QString getStarIdNumber(QString starId)
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
		return "-1";
	}

	/**
     * @brief Converts the StartLine to a JSON array.
     *
     * @return QJsonArray The JSON representation of the start line.
     */
	QJsonArray toJson() const
	{
		QJsonArray json;

		if (start.has_value())
		{
			QString number = getStarIdNumber(start.value());

			if (start.value().contains("HIP"))
			{
				// HIP are required as int
				json.append(number.toInt());
			}
			else
			{
				// Gaia is required as string
				json.append(number);
			}
		}
		else
		{
			json.append("-1");
		}

		if (end.has_value())
		{
			QString number = getStarIdNumber(end.value());

			if (end.value().contains("HIP"))
			{
				// HIP are required as int
				json.append(number.toInt());
			}
			else
			{
				// Gaia is required as string
				json.append(number);
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
