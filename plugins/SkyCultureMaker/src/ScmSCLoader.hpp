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
	static bool parseIndexJson(const QDir &dir, scm::ScmSkyCulture *sc, QString *errorMsg);
	static bool parseTerritoryGeoJson(const QDir &dir, scm::ScmSkyCulture *sc);
	static bool parseDescriptionMd(const QDir &dir, scm::ScmSkyCulture *sc);
};

#endif // SCMSCLOADER_HPP
