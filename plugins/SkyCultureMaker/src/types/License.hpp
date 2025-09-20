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

#ifndef SCM_LICENSE_HPP
#define SCM_LICENSE_HPP

#include <map>
#include <vector>
#include <QString>
#include <QMetaType>

namespace scm
{

//! Struct to represent a license
struct License
{
	QString name;
	QString description;

	License(const QString& name, const QString& description)
		: name(name)
		, description(description)
	{
	}
};

//! Enum class to represent different types of licenses
enum class LicenseType
{
	NONE = 0,
	CC0,
	CC_BY,
	CC_BY_NC,
	CC_BY_ND,
	CC_BY_NC_ND
};

//! Map of license types to their corresponding name and description
const std::map<LicenseType, License> LICENSES = {
	{LicenseType::NONE, License("None", "Please select a valid license.")},
	{LicenseType::CC0, License("CC0 1.0",
        "This file is made available under the Creative Commons CC0 1.0 Universal Public Domain Dedication. "
        "The person who associated a work with this deed has dedicated the work to the public domain by "
        "waiving all of his or her rights to the work worldwide under copyright law, including all related "
        "and neighboring rights, to the extent allowed by law. You can copy, modify, distribute and perform "
        "the work, even for commercial purposes, all without asking permission.")},
	{LicenseType::CC_BY, License("CC BY 4.0", 
        "This work is licensed under the Creative Commons Attribution 4.0 License.")},
	{LicenseType::CC_BY_NC, License("CC BY-NC 4.0", 
		"This work is licensed under the Creative Commons Attribution-NonCommercial 4.0 License.")},
	{LicenseType::CC_BY_ND, License("CC BY-ND 4.0", 
		"This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 License.")},
	{LicenseType::CC_BY_NC_ND, License("CC BY-NC-ND 4.0", 
		"This work is licensed under the Creative Commons Attribution-NoDerivatives 4.0 License.")}
};

} // namespace scm

Q_DECLARE_METATYPE(scm::LicenseType);

#endif // SCM_LICENSE_HPP
