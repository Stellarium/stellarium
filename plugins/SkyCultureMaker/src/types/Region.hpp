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

#ifndef SCM_REGION_HPP
#define SCM_REGION_HPP

#include <map>
#include <QString>
#include <QMetaType>

namespace scm
{

//! Struct to represent a region
struct Region
{
	QString name;
	QString description;

	Region(const QString& name, const QString& description)
		: name(name)
		, description(description)
	{
	}
};

//! Enum class to represent different types of regions
enum class RegionType
{
	GLOBAL = 0,
	EASTERN_EUROPE,
	NORTHERN_EUROPE,
	SOUTHERN_EUROPE,
	WESTERN_EUROPE,
	EASTERN_AFRICA,
	MIDDLE_AFRICA,
	NORTHERN_AFRICA,
	SOUTHERN_AFRICA,
	WESTERN_AFRICA,
	CARIBBEAN,
	CENTRAL_AMERICA,
	NORTHERN_AMERICA,
	SOUTH_AMERICA,
	CENTRAL_ASIA,
	EASTERN_ASIA,
	SOUTH_EASTERN_ASIA,
	SOUTHERN_ASIA,
	WESTERN_ASIA,
	AUSTRALASIA,
	MELANASIA,
	MICRONESIA,
	POLYNESIA
};

//! Map of region types to their corresponding name and description
const std::map<RegionType, Region> REGIONS = {
	{RegionType::GLOBAL, Region("Global", "Unofficial region intended only for cultures that operate globally and cannot be assigned to a single location. (e.g. Modern)")},
	{RegionType::EASTERN_EUROPE, Region("Eastern Europe", "This region includes the whole of russia (even the parts on the asian continent).")},
	{RegionType::NORTHERN_EUROPE, Region("Northern Europe", "")},
	{RegionType::SOUTHERN_EUROPE, Region("Southern Europe", "")},
	{RegionType::WESTERN_EUROPE, Region("Western Europe", "")},
	{RegionType::EASTERN_AFRICA, Region("Eastern Africa", "")},
	{RegionType::MIDDLE_AFRICA, Region("Middle Africa", "")},
	{RegionType::NORTHERN_AFRICA, Region("Northern Africa", "")},
	{RegionType::SOUTHERN_AFRICA, Region("Southern Africa", "")},
	{RegionType::WESTERN_AFRICA, Region("Western Africa", "")},
	{RegionType::CARIBBEAN, Region("Caribbean", "")},
	{RegionType::CENTRAL_AMERICA, Region("Central America", "")},
	{RegionType::NORTHERN_AMERICA, Region("Northern America", "This region includes USA, Canada and Greenland.")},
	{RegionType::SOUTH_AMERICA, Region("South America", "")},
	{RegionType::CENTRAL_ASIA, Region("Central Asia", "")},
	{RegionType::EASTERN_ASIA, Region("Eastern Asia", "")},
	{RegionType::SOUTH_EASTERN_ASIA, Region("South-eastern Asia", "")},
	{RegionType::SOUTHERN_ASIA, Region("Southern Asia", "")},
	{RegionType::WESTERN_ASIA, Region("Western Asia", "")},
	{RegionType::AUSTRALASIA, Region("Australasia", "")},
	{RegionType::MELANASIA, Region("Melanesia", "")},
	{RegionType::MICRONESIA, Region("Micronesia", "")},
	{RegionType::POLYNESIA, Region("Polynesia", "")}
};

} // namespace scm

Q_DECLARE_METATYPE(scm::RegionType);

#endif // SCM_REGION_HPP
