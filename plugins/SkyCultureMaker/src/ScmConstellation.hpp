/**
 * @file ScmConstellation.hpp
 * @author lgrumbach
 * @brief Represents a constellation in a sky culture.
 * @version 0.1
 * @date 2025-05-09
 *
 */

#ifndef SCM_CONSTELLATION_HPP
#define SCM_CONSTELLATION_HPP

#include <QString>
#include <vector>
#include <optional>
#include <variant>
#include <StelCore.hpp>
#include "types/CoordinateLine.hpp"
#include "types/StarLine.hpp"
#include "VecMath.hpp"
#include <QJsonObject>
#include <QJsonArray>

namespace scm
{

class ScmConstellation
{
public:
	ScmConstellation(std::vector<CoordinateLine> coordinates, std::vector<StarLine> stars);

	/// The frame that is used for calculation and is drawn on.
	static const StelCore::FrameType drawFrame = StelCore::FrameJ2000;

    /**
    * @brief Sets the id of the constellation
    * 
    * @param id id
    */
    void setId(QString id);

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
	void setEnglishName(QString name);

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
	void setNativeName(std::optional<QString> name);

	/**
    * @brief Sets the pronounciation of the constellation
    * 
    * @param pronounce The pronounciation
    */
	void setPronounce(std::optional<QString> pronounce);

	/**
    * @brief Sets the IPA.
    * 
    * @param ipa The optional ipa
	 */
	void setIPA(std::optional<QString> ipa);

	/**
    * @brief Sets the coordinate lines and star lines of the constellation.
    * 
    * @param coordinates The coordinates of the constellation. 
	* @param stars The equivalent stars to the coordinates.
    */
	void setConstellation(std::vector<CoordinateLine> coordinates, std::vector<StarLine> stars);

	/**
	 * @brief Draws the constellation based on the coordinates.
	 *
	 * @param core The core used for drawing.
	 * @param color The color to use for drawing the constellation.
	 */
	void drawConstellation(StelCore *core, Vec3f color);

	/**
	 * @brief Draws the constellation based on the coordinates using the default color.
	 *
	 * @param core The core used for drawing.
	 */
	void drawConstellation(StelCore *core);

	/**
	 * @brief Draws the label of the constellation.
	 * 
	 * @param core The core used for drawing.
	 * @param painter The painter used for drawing.
	 * @param labelColor The color of the label.
	 */
	void drawNames(StelCore *core, StelPainter painter, Vec3f labelColor);

	/**
	 * @brief Draws the label of the constellation using the default color.
	 * 
	 * @param core The core used for drawing.
	 * @param painter The painter used for drawing.
	 */
	void drawNames(StelCore *core, StelPainter painter);

	/**
	 * @brief Returns the constellation data as a JSON object.
	 */
	QJsonObject toJson(QString &skyCultureName) const;

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
	std::vector<CoordinateLine> constellationCoordinates;

	/// List of stars forming the segments. Might be empty.
	std::vector<StarLine> constellationStars;

	/// Direction vector pointing on constellation name drawing position
	Vec3d XYZname;
	Vec3d XYname;

	/// The font used for constellation labels
	QFont constellationLabelFont;

	/// The default color used for drawing the constellation
	Vec3f colorDrawDefault = Vec3f(0.0f, 0.0f, 0.0f);

	/// The default color used for drawing the constellation label
	Vec3f colorLabelDefault = Vec3f(0.0f, 0.0f, 0.0f);
};

}  // namespace scm

#endif	// SCM_CONSTELLATION_HPP
