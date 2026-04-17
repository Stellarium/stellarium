/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2026 Luca-Philipp Grumbach
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

#ifndef SCM_CONSTELLATION_COMMON_NAME_HPP
#define SCM_CONSTELLATION_COMMON_NAME_HPP

#include <optional>
#include <QJsonObject>
#include <QString>

namespace scm
{

struct ConstellationCommonName
{
	QString english;
	std::optional<QString> byname;
	std::optional<QString> native;
	std::optional<QString> pronounce;
	std::optional<QString> transliteration;
	std::optional<QString> ipa;

	void clear()
	{
		english.clear();
		byname = std::nullopt;
		native = std::nullopt;
		pronounce = std::nullopt;
		transliteration = std::nullopt;
		ipa = std::nullopt;
	}

	QJsonObject toJson() const
	{
		QJsonObject json;
		json["english"] = english;

		auto addIfPresent = [&json](const char *key, const std::optional<QString> &value)
		{
			if (value.has_value())
			{
				json[key] = value.value();
			}
		};

		addIfPresent("byname", byname);
		addIfPresent("native", native);
		addIfPresent("pronounce", pronounce);
		addIfPresent("transliteration", transliteration);
		addIfPresent("ipa", ipa);

		return json;
	}
};

} // namespace scm

#endif // SCM_CONSTELLATION_COMMON_NAME_HPP