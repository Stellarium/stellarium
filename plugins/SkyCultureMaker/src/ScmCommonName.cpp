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

#include "ScmCommonName.hpp"

scm::ScmCommonName::ScmCommonName(const QString &id)
{
	ScmCommonName::id = id;
}

void scm::ScmCommonName::setEnglishName(const QString &name)
{
	englishName = name;
}

void scm::ScmCommonName::setNativeName(const QString &name)
{
	nativeName = name;
}

void scm::ScmCommonName::setPronounce(const QString &pronounce)
{
	ScmCommonName::pronounce = pronounce;
}

void scm::ScmCommonName::setIpa(const QString &ipa)
{
	ScmCommonName::ipa = ipa;
}

void scm::ScmCommonName::setReferences(const std::vector<int> &refs)
{
	references = refs;
}

QString scm::ScmCommonName::getEnglishName() const
{
	return englishName;
}

std::optional<QString> scm::ScmCommonName::getIpa() const
{
	return ipa;
}

std::optional<std::vector<int>> scm::ScmCommonName::getReferences() const
{
	return references;
}
