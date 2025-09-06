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

#ifndef SCM_DESCRIPTION_HPP
#define SCM_DESCRIPTION_HPP

#include "Classification.hpp"
#include "License.hpp"
#include <map>
#include <vector>
#include <QMetaType>
#include <QString>

namespace scm
{

/**
 * @brief The Description struct represents a sky culture description.
 */
struct Description
{
	QString name;
	QString authors;
	scm::LicenseType license;
	QString cultureDescription;
	QString about;

	QString sky;
	QString moonAndSun;
	QString planets;
	QString zodiac;
	QString milkyWay;
	QString otherObjects;

	QString constellations;
	QString references;
	QString acknowledgements;
	scm::ClassificationType classification;

	/**
	 * @brief Check if the description is complete.
	 * @return true if all required fields are filled, false otherwise.
	 */
	bool isComplete() const
	{
		return !name.trimmed().isEmpty() &&
			   !authors.trimmed().isEmpty() &&
			   license != scm::LicenseType::NONE &&
			   !cultureDescription.trimmed().isEmpty() &&
			   !about.trimmed().isEmpty() &&
			   !sky.trimmed().isEmpty() &&
			   !moonAndSun.trimmed().isEmpty() &&
			   !planets.trimmed().isEmpty() &&
			   !zodiac.trimmed().isEmpty() &&
			   !milkyWay.trimmed().isEmpty() &&
			   !otherObjects.trimmed().isEmpty() &&
			   !constellations.trimmed().isEmpty() &&
			   !references.trimmed().isEmpty() &&
			   !acknowledgements.trimmed().isEmpty() &&
			   classification != scm::ClassificationType::NONE;
	}
};
} // namespace scm

Q_DECLARE_METATYPE(scm::Description);

#endif // SCM_DESCRIPTION_HPP
