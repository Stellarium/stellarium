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

#ifndef TILEMANAGER_HPP
#define TILEMANAGER_HPP

#include <QString>
#include "StelSkyImageTile.hpp"
#include "TextureConfigManager.hpp"

//! Manages loading, visibility, conflict resolution, and file operations for sky tiles.
class TileManager
{
public:
	//! Constructor.
	TileManager();

	//! Returns the top-level tile by key.
	StelSkyImageTile* getTile(const QString& key);

	//! Sets visibility for the specified tile and its subtiles.
	bool setTileVisible(const QString& key, bool visible);

	//! Loads and inserts a tile from a configuration file.
	bool insertTileFromConfig(const QString& configFilePath, const QString& texName, bool show = true);

	//! Removes a tile from the sky layer manager.
	void removeTile(const QString& texName);

	//! Checks if two tiles have overlapping subtiles.
	bool hasConflict(StelSkyImageTile* tileA, StelSkyImageTile* tileB);

	//! Hides default tile regions that overlap with custom tiles.
	void resolveConflicts(const QString& defaultTexName, const QString& customTexName);

	//! Deletes image files listed in the configuration.
	void deleteImagesFromConfig(TextureConfigManager* configManager, const QString& pluginDir);
};

#endif // TILEMANAGER_HPP
