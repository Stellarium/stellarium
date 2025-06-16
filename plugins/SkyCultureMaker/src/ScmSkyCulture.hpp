/**
 * @file ScmSkyCulture.hpp
 * @author fhofer, lgrumbach
 * @brief Object holding all data of a sky culture during the creation process with the Sky Culture Maker.
 * @version 0.1
 * @date 2025-05-09
 *
 * The minimun information that the object should store can be found in any index.json file of an existing sky culture.
 * For example: /{userpath}/stellarium/skycultures/maya/index.json
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

	/// Sets the region of the sky culture
	void setRegion(const QString &region);

	/// Sets the classification of the sky culture
	void setClassificationType(ClassificationType classificationType);

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

	/// Sets the license of the sky culture
	void setLicense(scm::LicenseType license);

	/// Returns the license of the sky culture
	scm::LicenseType getLicense() const;

	/// Sets the authors of the sky culture
	void setAuthors(const QString authors);

	/// Returns the authors of the sky culture
	QString getAuthors() const;

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
	bool saveDescriptionAsMarkdown(QFile file);

private:
	/// Sky culture identifier
	QString id;

	/*! The name of region following the United Nations geoscheme UN~M49
	 *   https://unstats.un.org/unsd/methodology/m49/ For skycultures of worldwide applicability (mostly those
	 *   adhering to IAU constellation borders), use "World".
	 */
	QString region;

	/// Classification of the sky culture
	ClassificationType classificationType = ClassificationType::NONE;

	/// Whether to show common names in addition to the culture-specific ones
	bool fallbackToInternationalNames = false;

	/// The asterisms of the sky culture
	std::vector<ScmAsterism> asterisms;

	/// The constellations of the sky culture
	std::vector<ScmConstellation> constellations;

	/// The common names of the stars, planets and nonstellar objects
	std::vector<ScmCommonName> commonNames;

	/// The license of the sky culture
	scm::LicenseType license = scm::LicenseType::NONE;

	/// The authors of the sky culture
	QString authors;

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
