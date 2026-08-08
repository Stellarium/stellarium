/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2026 Luca-Philipp Grumbach
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

#ifndef SCMSCLOADER_HPP
#define SCMSCLOADER_HPP

#include "ScmSkyCulture.hpp"
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class QWidget;

/**
 * @brief Utility class for loading an existing sky culture.
 */
class ScmSCLoader
{
public:
	/**
	 * @brief Loads a sky culture from a given directory.
	 *
	 * The directory must contain at least an index.json file.
	 * The files territory.geojson and description.md are loaded when present.
	 *
	 * @param dir       Sky culture directory to load.
	 * @param errorMsg  If non-null, receives a human-readable error description
	 *                  on failure.
	 * @return Newly heap-allocated ScmSkyCulture on success, nullptr on failure.
	 */
	static scm::ScmSkyCulture *loadFromDirectory(const QDir &dir, QString *errorMsg = nullptr);

	/**
	 * @brief Opens a directory picker and loads the selected sky culture.
	 *
	 * @param parent       Parent widget for the file dialog (may be nullptr).
	 * @param defaultPath  Initial directory shown in the picker.
	 * @param errorMsg     If non-null, receives a human-readable error on failure.
	 * @return Newly heap-allocated ScmSkyCulture, or nullptr if the user
	 *         cancelled or loading failed.
	 */
	static scm::ScmSkyCulture *selectAndLoad(QWidget *parent = nullptr, const QString &defaultPath = {},
	                                         QString *errorMsg = nullptr);

private:
	enum class Section
	{
		None,
		Introduction,
		Description,
		Sky,
		MoonAndSun,
		Planets,
		Zodiac,
		MilkyWay,
		OtherObjects,
		Constellations,
		References,
		Authors,
		About,
		Acknowledgements,
		License
	};

	static inline Section sectionOf(const QString &heading)
	{
		const QString h = heading.toLower().trimmed();
		if (h == "introduction") return Section::Introduction;
		if (h == "description") return Section::Description;
		if (h == "sky") return Section::Sky;
		if (h == "moon and sun") return Section::MoonAndSun;
		if (h == "planets") return Section::Planets;
		if (h == "zodiac") return Section::Zodiac;
		if (h == "milky way") return Section::MilkyWay;
		if (h == "other celestial objects") return Section::OtherObjects;
		if (h == "constellations") return Section::Constellations;
		if (h == "references") return Section::References;
		if (h == "authors") return Section::Authors;
		if (h == "about") return Section::About;
		if (h == "acknowledgements") return Section::Acknowledgements;
		if (h == "license") return Section::License;
		return Section::None;
	}

	static bool parseIndexJson(const QDir &dir, scm::ScmSkyCulture *sc, QString *errorMsg);
	static void parseIndexJsonBasicFields(const QJsonObject &root, const QDir &dir, scm::ScmSkyCulture *sc);
	static void parseIndexJsonConstellations(const QJsonArray &constellationsArr, const QDir &dir, scm::ScmSkyCulture *sc);
	static void parseIndexJsonCommonNames(const QJsonObject &commonNamesObj, scm::ScmSkyCulture *sc);
	static bool parseTerritoryGeoJson(const QDir &dir, scm::ScmSkyCulture *sc);
	static bool parseDescriptionMd(const QDir &dir, scm::ScmSkyCulture *sc);
};

#endif // SCMSCLOADER_HPP
