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

#ifndef SCM_SKYCULTURE_HPP
#define SCM_SKYCULTURE_HPP

#include <QString>
// #include <vector>
#include <optional>

#include "ScmAsterism.hpp"
#include "ScmCommonName.hpp"
#include "ScmConstellation.hpp"
#include "StelCore.hpp"
#include "StelSkyCultureMgr.hpp"
#include "types/Classification.hpp"
#include "types/CoordinateLine.hpp"
#include "types/Description.hpp"
#include "types/License.hpp"
#include "types/StarLine.hpp"

namespace scm
{

class ScmSkyCulture
{
public:
	/// Sets the id of the sky culture
	void setId(const QString &id);

	/**
	 * @brief Gets the id of the sky culture.
	 */
	const QString &getId() const;

	/// Sets whether to show common names in addition to the culture-specific ones
	void setFallbackToInternationalNames(bool fallback);

	/// Adds an asterism to the sky culture
	void addAsterism(const ScmAsterism &asterism);

	/// Removes an asterism from the sky culture by its ID
	void removeAsterism(const QString &id);

	/// Adds a constellation to the sky culture
	ScmConstellation &addConstellation(const QString &id, const std::vector<CoordinateLine> &coordinates,
	                                   const std::vector<StarLine> &stars);

	/// Removes a constellation from the sky culture by its ID
	void removeConstellation(const QString &id);

	/// Gets a constellation from the sky culture by its ID
	ScmConstellation *getConstellation(const QString &id);

	/// Adds a common name to the sky culture
	void addCommonName(ScmCommonName commonName);

	/// Removes a common name from the sky culture by its ID
	void removeCommonName(const QString &id);

	/// Returns the asterisms of the sky culture
	// TODO: QVector<ScmAsterism> getAsterisms() const;

	/// Returns a pointer to the constellations of the sky culture
	std::vector<ScmConstellation> *getConstellations();

	/// Returns the common names of the stars, planets and nonstellar objects
	std::vector<ScmCommonName> getCommonNames() const;

	/**
	* @brief Returns the sky culture as a JSON object
	*/
	QJsonObject toJson() const;

	/**
	* @brief Draws the sky culture.
	*/
	void draw(StelCore *core) const;

	/**
	 * @brief Sets the description of the sky culture.
	 * @param description The description to set.
	 */
	void setDescription(const scm::Description &description);

	/**
	 * @brief Saves the current sky culture description as markdown text.
	 * @param file The file to save the description to.
	 * @return true if the description was saved successfully, false otherwise.
	 */
	bool saveDescriptionAsMarkdown(QFile &file);

	/**
	 * @brief Saves all illustrations to the directory. No subdirectory is saved.
	 * 
	 * @param directory The directory the illustrations are saved in.
	 * @return true Successful saved.
	 * @return false Failed to save.
	 */
	bool saveIllustrations(const QString &directory);

private:
	/// Sky culture identifier
	QString id;

	/// Whether to show common names in addition to the culture-specific ones
	bool fallbackToInternationalNames = false;

	/// The asterisms of the sky culture
	std::vector<ScmAsterism> asterisms;

	/// The constellations of the sky culture
	std::vector<ScmConstellation> constellations;

	/// The common names of the stars, planets and nonstellar objects
	std::vector<ScmCommonName> commonNames;

	/// The description of the sky culture
	scm::Description description;

	// TODO: edges:
	/// Type of the edges. Can be one of "none", "iau" or "own". TODO: enum?
	// std::optional<QString> edgeType;

	/// Source of the edges.
	// std::optional<QString> edgeSource;

	/*! Describes the coordinate epoch. Allowed values:
	 *   "J2000" (default)
	 *   "B1875" used for the edge list defining the IAU borders.
	 *   "Byyyy.y" (a number with B prepended) Arbitrary epoch as Besselian year.
	 *   "Jyyyy.y" (a number with J prepended) Arbitrary epoch as Julian year.
	 *   "JDddddddd.ddd" (a number with JD prepended) Arbitrary epoch as Julian Day number.
	 *   "ddddddd.ddd"
	 */
	// std::optional<QString> edgeEpoch;

	/// Describes the edges of the constellations.
	// std::optional<QVector<QString>> edges;
};

} // namespace scm

#endif // SCM_SKYCULTURE_HPP
