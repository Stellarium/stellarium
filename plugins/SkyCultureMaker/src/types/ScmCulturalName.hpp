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

#ifndef SCM_CULTURAL_NAME_HPP
#define SCM_CULTURAL_NAME_HPP

#include "StelObject.hpp"
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

namespace scm
{

struct ScmCulturalName : public StelObject::CulturalName
{
	//! Integers that refer to the references list in description.md.
	QList<int> references;

	//! Clears all fields of the cultural name.
	void clear()
	{
		references.clear();

		native.clear();
		pronounce.clear();
		pronounceI18n.clear();
		transliteration.clear();
		translated.clear();
		translatedI18n.clear();
		IPA.clear();
		byname.clear();
		bynameI18n.clear();
		special = StelObject::CulturalNameSpecial::None;
	}

	//! Converts the cultural name to a JSON object.
	QJsonObject toJson() const
	{
		QJsonObject json;
		json["english"] = translated;

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
		addIfNotEmpty("IPA", IPA);

		switch (special)
		{
			case StelObject::CulturalNameSpecial::Morning:
				json["visible"] = "morning";
				break;
			case StelObject::CulturalNameSpecial::Evening:
				json["visible"] = "evening";
				break;
			default:
				break;
		}

		if (references.size() > 0)
		{
			QJsonArray refsArray;
			for (int ref : references)
			{
				refsArray.append(ref);
			}
			json["references"] = refsArray;
		}

		return json;
	}

	//! Returns a trimmed version of the cultural name, i.e. with all fields trimmed.
	ScmCulturalName trimmed() const
	{
		ScmCulturalName result = *this;
		auto trim = [](QString &field) { field = field.trimmed(); };
		trim(result.native);
		trim(result.pronounce);
		trim(result.pronounceI18n);
		trim(result.transliteration);
		trim(result.translated);
		trim(result.translatedI18n);
		trim(result.IPA);
		trim(result.byname);
		trim(result.bynameI18n);
		return result;
	}
};

} // namespace scm

#endif // SCM_CULTURAL_NAME_HPP
