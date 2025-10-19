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

#ifndef SCM_CONSTELLATION_HPP
#define SCM_CONSTELLATION_HPP

#include "ScmConstellationArtwork.hpp"
#include "VecMath.hpp"
#include "types/CoordinateLine.hpp"
#include "types/StarLine.hpp"
#include <StelCore.hpp>
#include <optional>
#include <variant>
#include <vector>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace scm
{

class ScmConstellation
{
public:
	ScmConstellation(const QString &id, const std::vector<CoordinateLine> &coordinates,
	                 const std::vector<StarLine> &stars, const bool isDarkConstellation);

	/// The frame that is used for calculation and is drawn on.
	static const StelCore::FrameType drawFrame = StelCore::FrameJ2000;

	/**
    * @brief Gets the id of the constellation
    * 
    * @return id
    */
	QString getId() const;

	/**
    * @brief Sets the english name of the constellation
    * 
    * @param name The english name
    */
	void setEnglishName(const QString &name);

	/**
	* @brief Gets the english name of the constellation
	* 
	* @return The english name
	*/
	QString getEnglishName() const;

	/**
    * @brief Sets the native name of the constellation
    * 
    * @param name The native name
    */
	void setNativeName(const std::optional<QString> &name);

	/**
	* @brief Gets the native name of the constellation
	* 
	* @return The native name
	*/
	std::optional<QString> getNativeName() const;

	/**
    * @brief Sets the pronounciation of the constellation
    * 
    * @param pronounce The pronounciation
    */
	void setPronounce(const std::optional<QString> &pronounce);

	/**
	 * @brief Gets the pronounciation of the constellation
	 * 
	 * @return The pronounciation
	 */
	std::optional<QString> getPronounce() const;

	/**
    * @brief Sets the IPA.
    * 
    * @param ipa The optional ipa
	 */
	void setIPA(const std::optional<QString> &ipa);

	/**
	 * @brief Gets the IPA.
	 * 
	 * @return The optional ipa
	 */
	std::optional<QString> getIPA() const;

	void setDescription(const QString& description) { this->description = description; }

	QString getDescription() const { return description; }

	/**
	 * @brief Sets the artwork.
	 * 
	 * @param artwork The artwork.
	 */
	void setArtwork(const ScmConstellationArtwork &artwork);

	/**
	 * @brief Gets the artwork.
	 * 
	 * @return The artwork.
	 */
	const ScmConstellationArtwork &getArtwork() const;

	/**
    * @brief Sets the coordinate lines and star lines of the constellation.
    * 
    * @param coordinates The coordinates of the constellation. 
	* @param stars The equivalent stars to the coordinates.
    */
	void setConstellation(const std::vector<CoordinateLine> &coordinates, const std::vector<StarLine> &stars);

	/**
	 * @brief Gets the coordinates of the constellation.
	 * 
	 * @return The coordinates of the constellation.
	 */
	const std::vector<CoordinateLine> &getCoordinates() const;

	/**
	 * @brief Gets the stars of the constellation.
	 * 
	 * @return The stars of the constellation.
	 */
	const std::vector<StarLine> &getStars() const;

	/**
	 * @brief Draws the constellation based on the coordinates.
	 *
	 * @param core The core used for drawing.
	 * @param color The color to use for drawing the constellation.
	 */
	void drawConstellation(StelCore *core, const Vec3f &lineColor, const Vec3f &labelColor) const;

	/**
	 * @brief Draws the constellation based on the coordinates using the default color.
	 *
	 * @param core The core used for drawing.
	 */
	void drawConstellation(StelCore *core) const;

	/**
	 * @brief Draws the label of the constellation.
	 * 
	 * @param core The core used for drawing.
	 * @param painter The painter used for drawing.
	 * @param labelColor The color of the label.
	 */
	void drawNames(StelCore *core, StelPainter &painter, const Vec3f &labelColor) const;

	/**
	 * @brief Draws the label of the constellation using the default color.
	 * 
	 * @param core The core used for drawing.
	 * @param painter The painter used for drawing.
	 */
	void drawNames(StelCore *core, StelPainter &painter) const;

	/**
	  * @brief Returns the constellation data as a JSON object.
	  * 
	  * @param skyCultureId The ID of the sky culture to which this constellation belongs.
	  * @param mergeLines Whether to merge lines into polylines where possible.
	  * @return QJsonObject 
	  */
	QJsonObject toJson(const QString &skyCultureId, const bool mergeLines) const;

	/**
	 * @brief Saves the artwork of this constellation, if art is attached, to the give filepath.
	 * 
	 * @param directory The directory to the illustrations.
	 * @return true Successful saved.
	 * @return false Failed to save.
	 */
	bool saveArtwork(const QString &directory);

	/**
	 * @brief Hides the constellation from being drawn.
	 */
	void hide();

	/**
	 * @brief Enables the constellation to be drawn.
	 */
	void show();

	/** 
	 * @brief Returns whether the constellation is a dark constellation.
	 * 
	 * @return true If the constellation is a dark constellation, false otherwise.
	 */
	bool getIsDarkConstellation() const { return isDarkConstellation; }

private:
	/// Identifier of the constellation
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
	std::optional<QVector<int>> references;

	/// List of coordinates forming the segments.
	std::vector<CoordinateLine> coordinates;

	/// List of stars forming the segments. Might be empty.
	std::vector<StarLine> stars;

	/// A short description of the constellation that could be shown in e.g. an info block.
	QString description;

	/// Direction vector pointing on constellation name drawing position
	Vec3d XYZname;

	/// The font used for constellation names
	QFont constellationNameFont;

	/// The default color used for drawing the constellation
	Vec3f defaultConstellationLineColor = Vec3f(0.0f, 0.0f, 0.0f);

	/// The default color used for drawing the constellation names
	Vec3f defaultConstellationNameColor = Vec3f(0.0f, 0.0f, 0.0f);

	/// Holds the artwork of this constellation.
	ScmConstellationArtwork artwork;

	/// Holds the path the artwork was saved to.
	QString artworkPath;

	/// Whether the constellation should be drawn or not.
	bool isHidden = false;

	/// Indicates if the constellation is a dark constellation.
	bool isDarkConstellation = false;

	/**
	 * @brief Updates the XYZname that is used for the text position.
	 */
	void updateTextPosition();

	/**
	 * @brief Merges individual star lines into polylines where possible.
	 *
	 * Merging is done in a two step process, where first the ends of lines are merged
	 * with the starts of other lines if they match, and secondly the starts of lines are
	 * merged with the ends of other lines if they match. The lines are processed in the order
	 * they have been drawn. However, lines which point to opposite directions are not merged.
	 * 
	 * Example:
	 * 
	 * Input 1: [[1,2], [2,3], [0,1]]
	 * 
	 * Output 1: [[0,1,2,3]]
	 * 
	 * Input 2: [[1,2], [2,3], [1,0]]
	 * 
	 * Output 2: [[1,2,3], [1,0]]
	 *
	 * @param lines The individual star lines to merge.
	 */
	void mergeLinesIntoPolylines(QJsonArray &lines) const;
};

} // namespace scm

#endif // SCM_CONSTELLATION_HPP
