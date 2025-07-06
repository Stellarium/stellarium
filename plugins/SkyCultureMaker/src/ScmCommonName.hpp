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

#ifndef SCM_COMMONNAME_HPP
#define SCM_COMMONNAME_HPP

#include <optional>
#include <vector>
#include <QJsonObject>
#include <QString>

namespace scm
{

class ScmCommonName
{
public:
	ScmCommonName(const QString &id);

	/// Sets the english name
	void setEnglishName(const QString &name);

	/// Sets the native name
	void setNativeName(const QString &name);

	/// Sets the native name in European glyphs or Pinyin for Chinese.
	void setPronounce(const QString &pronounce);

	/// Sets the native name in IPA (International Phonetic Alphabet)
	void setIpa(const QString &name);

	/// Sets the references to the sources of the name spellings
	void setReferences(const std::vector<int> &refs);

	/// Returns the english name
	QString getEnglishName() const;

	/// Returns the native name
	std::optional<QString> getNativeName() const;

	/// Returns the native name in European glyphs or Pinyin for Chinese.
	std::optional<QString> getPronounce() const;

	/// Returns the native name in IPA (International Phonetic Alphabet)
	std::optional<QString> getIpa() const;

	/// Returns the references to the sources of the name spellings
	std::optional<std::vector<int>> getReferences() const;

	/// Returns the common name as a JSON object
	QJsonObject toJson() const;

private:
	/*! Identifier of the common name
	 * 	Example:
	 *    Star  : "HIP  <code>"
	 *    Planet: "NAME <name>"
	 */
	QString id;

	/// The english name
	QString englishName;

	/// The native name
	std::optional<QString> nativeName;

	/// Native name in European glyphs, if needed. For Chinese, expect Pinyin here.
	std::optional<QString> pronounce;

	/// The native name in IPA (International Phonetic Alphabet)
	std::optional<QString> ipa;

	/// References to the sources of the name spellings
	std::optional<std::vector<int>> references;
};

} // namespace scm

#endif // SCM_COMMONNAME_HPP
