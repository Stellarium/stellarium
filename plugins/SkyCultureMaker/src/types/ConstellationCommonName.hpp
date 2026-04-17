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

#include <QJsonObject>
#include <QString>

namespace scm
{

struct ConstellationCommonName
{
	QString english;
	QString byname;
	QString native;
	QString pronounce;
	QString transliteration;
	QString ipa;

	//! Clears all fields of the common name.
	void clear()
	{
		english.clear();
		byname.clear();
		native.clear();
		pronounce.clear();
		transliteration.clear();
		ipa.clear();
	}

	//! Converts the common name to a JSON object.
	QJsonObject toJson() const
	{
		QJsonObject json;
		json["english"] = english;

		auto addIfNotEmpty = [&json](const char *key, const QString &value)
		{
			if (!value.isEmpty())
			{
				json[key] = value;
			}
		};

		addIfNotEmpty("byname", byname);
		addIfNotEmpty("native", native);
		addIfNotEmpty("pronounce", pronounce);
		addIfNotEmpty("transliteration", transliteration);
		addIfNotEmpty("ipa", ipa);

		return json;
	}

	//! Returns a trimmed version of the common name, i.e. with all fields trimmed.
	ConstellationCommonName trimmed() const
	{
		ConstellationCommonName result = *this;
		result.english = result.english.trimmed();
		result.byname = result.byname.trimmed();
		result.native = result.native.trimmed();
		result.pronounce = result.pronounce.trimmed();
		result.transliteration = result.transliteration.trimmed();
		result.ipa = result.ipa.trimmed();
		return result;
	}
};

} // namespace scm

#endif // SCM_CONSTELLATION_COMMON_NAME_HPP