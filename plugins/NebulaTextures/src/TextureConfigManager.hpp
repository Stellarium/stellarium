/*
 * Nebula Textures plug-in for Stellarium
 *
 * Copyright (C) 2024-2025 WANG Siliang
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

#ifndef TEXTURECONFIGMANAGER_HPP
#define TEXTURECONFIGMANAGER_HPP

#include <QString>
#include <QJsonArray>
#include <QJsonObject>

//! Manages loading, saving, and modifying texture configuration files in JSON format.
class TextureConfigManager
{
public:
	//! Constructs with the given config file path.
	explicit TextureConfigManager(const QString& configFilePath, const QString& texname);

	//! Loads the texture configuration from the specified file.
	bool load();

	//! Saves the current configuration to the file.
	bool save() const;

	//! Clears all sub-tiles in the configuration.
	void clearSubTiles();

	//! Adds a sub-tile to the configuration without overwriting.
	void addSubTile(const QString& imageUrl, const QJsonArray& worldCoords, double minResolution, double maxBrightness);

	//! Adds a sub-tile and overwrites existing ones.
	void addSubTileOverwrite(const QString& imageUrl, const QJsonArray& worldCoords, double minResolution, double maxBrightness);

	//! Removes a sub-tile based on the image URL.
	bool removeSubTileByImageUrl(const QString& imageUrl);

	//! Retrieves all sub-tiles from the configuration.
	QJsonArray getSubTiles() const;

	//! Gets a sub-tile by its image URL.
	QJsonObject getSubTileByImageUrl(const QString& imageUrl) const;

	//! Returns the file path of the configuration.
	QString getConfigPath() const;

private:
	QString filePath;  //!< Path to the configuration file.
	QString texname;  //!< Group Name of configuration file.
	QJsonObject rootObject;  //!< The root JSON object for the config.
};

#endif // TEXTURECONFIGMANAGER_HPP
