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

#include "TextureConfigManager.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

/**
 * @brief Constructs a TextureConfigManager with the specified config file path.
 * @param configFilePath Path to the JSON configuration file.
 */
TextureConfigManager::TextureConfigManager(const QString& configFilePath, const QString& texname)
	: filePath(configFilePath), texname(texname)
{}

/**
 * @brief Loads the texture configuration from the JSON file.
 * @return true if the file is successfully read and parsed; false otherwise.
 */
bool TextureConfigManager::load()
{
	QFile file(filePath);
	// Check if file exists and can be opened for reading
	if (!file.exists())
	{
		rootObject = QJsonObject{ { "alphaBlend", true }, { "description", "User specified nebula texture set." }, { "shortName", texname }, { "minResolution", 0.05 }, { "subTiles", QJsonArray{} }};
		return true;
	}

	if (!file.open(QIODevice::ReadOnly)) return false;

	// Parse JSON content
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();

	// Check if root element is a JSON object
	if (!doc.isObject())
		return false;

	rootObject = doc.object();
	return true;
}

/**
 * @brief Saves the current JSON object to the config file in indented format.
 * @return true if the file is written successfully; false otherwise.
 */
bool TextureConfigManager::save() const
{
	QFile file(filePath);
	// Open file for writing
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;

	// Write JSON document to file
	QJsonDocument doc(rootObject);
	file.write(doc.toJson(QJsonDocument::Indented));
	file.close();
	return true;
}

/**
 * @brief Clears all sub-tile entries in the configuration.
 */
void TextureConfigManager::clearSubTiles()
{
	// Replace "subTiles" array with an empty array
	rootObject["subTiles"] = QJsonArray();
}

/**
 * @brief Adds a sub-tile entry to the configuration without clearing existing entries.
 * @param imageUrl Path or URL of the tile image.
 * @param worldCoords A QJsonArray of world coordinates (RA/Dec or similar).
 * @param minResolution Minimum resolution value for this tile.
 * @param maxBrightness Maximum brightness value for this tile.
 */
void TextureConfigManager::addSubTile(const QString& imageUrl, const QJsonArray& worldCoords, double minResolution, double maxBrightness)
{
	QJsonObject tile;
	tile["imageUrl"] = imageUrl;

	// Add static image credits
	QJsonObject credits;
	credits["short"] = "Local User";
	tile["imageCredits"] = credits;

	// Wrap world coordinates in outer array (GeoJSON-style)
	QJsonArray outerWorldCoords;
	outerWorldCoords.append(worldCoords);
	tile["worldCoords"] = outerWorldCoords;

	// Define default texture coordinates (normalized quad)
	QJsonArray texCoords =
	{
		QJsonArray({0, 0}),
		QJsonArray({1, 0}),
		QJsonArray({1, 1}),
		QJsonArray({0, 1})
	};
	QJsonArray outerTexCoords;
	outerTexCoords.append(texCoords);
	tile["textureCoords"] = outerTexCoords;

	tile["minResolution"] = minResolution;
	tile["maxBrightness"] = maxBrightness;

	// Append tile to the subTiles array
	QJsonArray subTiles = rootObject["subTiles"].toArray();
	subTiles.append(tile);
	rootObject["subTiles"] = subTiles;
}

/**
 * @brief Adds a sub-tile and clears existing ones (overwrite).
 * @param imageUrl Path or URL of the tile image.
 * @param worldCoords A QJsonArray of world coordinates (RA/Dec or similar).
 * @param minResolution Minimum resolution value for this tile.
 * @param maxBrightness Maximum brightness value for this tile.
 */
void TextureConfigManager::addSubTileOverwrite(const QString& imageUrl, const QJsonArray& worldCoords, double minResolution, double maxBrightness)
{
	QJsonObject tile;
	tile["imageUrl"] = imageUrl;

	// Static image credit info
	QJsonObject credits;
	credits["short"] = "Local User";
	tile["imageCredits"] = credits;

	// Wrap world coordinates
	QJsonArray outerWorldCoords;
	outerWorldCoords.append(worldCoords);
	tile["worldCoords"] = outerWorldCoords;

	// Default texture quad
	QJsonArray texCoords =
	{
		QJsonArray({0, 0}),
		QJsonArray({1, 0}),
		QJsonArray({1, 1}),
		QJsonArray({0, 1})
	};
	QJsonArray outerTexCoords;
	outerTexCoords.append(texCoords);
	tile["textureCoords"] = outerTexCoords;

	tile["minResolution"] = minResolution;
	tile["maxBrightness"] = maxBrightness;

	// Overwrite the entire subTiles array with a single entry
	QJsonArray subTiles;
	subTiles.append(tile);
	rootObject["subTiles"] = subTiles;
}

/**
 * @brief Removes a sub-tile entry matching the given imageUrl.
 * @param imageUrl The image URL or path to search for.
 * @return true if a tile was found and removed; false otherwise.
 */
bool TextureConfigManager::removeSubTileByImageUrl(const QString& imageUrl)
{
	if (!rootObject.contains("subTiles")) return false;
	QJsonArray subTiles = rootObject["subTiles"].toArray();
	QJsonArray updatedTiles;
	bool found = false;

	// Filter out the tile to be removed
	for (const QJsonValue& val : subTiles)
	{
		QJsonObject obj = val.toObject();
		if (obj["imageUrl"].toString() != imageUrl)
			updatedTiles.append(val);
		else
			found = true;
	}

	rootObject["subTiles"] = updatedTiles;
	return found;
}

/**
 * @brief Retrieves the array of all sub-tile entries.
 * @return QJsonArray containing all sub-tiles.
 */
QJsonArray TextureConfigManager::getSubTiles() const
{
	return rootObject.value("subTiles").toArray();
}

/**
 * @brief Searches and returns a sub-tile entry by its image URL.
 * @param imageUrl The image URL to look for.
 * @return QJsonObject of the matching sub-tile, or empty object if not found.
 */
QJsonObject TextureConfigManager::getSubTileByImageUrl(const QString& imageUrl) const
{
	QJsonArray subTiles = rootObject.value("subTiles").toArray();
	for (const QJsonValue& val : subTiles)
	{
		QJsonObject obj = val.toObject();
		if (obj["imageUrl"].toString() == imageUrl)
			return obj;
	}
	return QJsonObject();
}

/**
 * @brief Gets the full file path to the configuration file.
 * @return QString representing the config file path.
 */
QString TextureConfigManager::getConfigPath() const
{
	return filePath;
}
