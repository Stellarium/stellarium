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
#include <QMetaType>
#include <QString>

namespace scm
{

/**
 * @brief The Description struct represents a sky culture description.
 */
struct Description
{                                                // MD equivalent:
	QString name;                            // name (will become L1 title)
	QString introduction;                    // content of Introduction section (L2)

	QString cultureDescription;              // content of Description section (L2)
	QString sky;                             // content of optional subsection (L3) in Description section
	QString moonAndSun;                      // content of optional subsection (L3) in Description section
	QString planets;                         // content of optional subsection (L3) in Description section
	QString zodiac;                          // content of optional subsection (L3) in Description section
	QString milkyWay;                        // content of optional subsection (L3) in Description section
	QString otherObjects;                    // content of optional subsection (L3) in Description section

	QString constellations;                  // Content of main section Constellations (L2): sequence of L5 markdown descriptions
	QString references;                      // Content of L2 section: List of references in format documented in the SUG. We trust our SCM users that they enter proper format!
	QString authors;                         // content of author section (L2)
	QString about;                           // content of subsection (L3) in author section
	QString acknowledgements;                // content of subsection (L3) in author section
	scm::LicenseType license;                // license code
	scm::ClassificationType classification;  // classification code

	/**
	 * @brief Check if the description is complete.
	 * @return true if all required fields are correctly filled, false otherwise.
	 */
	bool isComplete() const
	{
		return !name.trimmed().isEmpty() &&
			   !introduction.trimmed().isEmpty() &&
			   !cultureDescription.trimmed().isEmpty() &&
			   !references.trimmed().isEmpty() &&
			   !authors.trimmed().isEmpty() &&
			   !about.trimmed().isEmpty() &&
			   license != scm::LicenseType::NONE &&
			   classification != scm::ClassificationType::NONE;
	}
};
} // namespace scm

Q_DECLARE_METATYPE(scm::Description);

#endif // SCM_DESCRIPTION_HPP
