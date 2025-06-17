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

// TileManager.cpp
#include "TileManager.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelSkyImageTile.hpp"
#include "StelTranslator.hpp"

#include <QFile>
#include <QDir>
#include <QDebug>

//! Constructor for TileManager.
//! Initializes an empty TileManager instance.
TileManager::TileManager() {}


//! Retrieve a StelSkyImageTile object by its registered key.
//! @param key The identifier of the tile (usually the texture group name).
//! @return Pointer to the StelSkyImageTile if found, or nullptr otherwise.
StelSkyImageTile* TileManager::getTile(const QString& key)
{
	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
	QString keyname = qc_(key, "dataset short name");
	auto it = skyLayerMgr->allSkyLayers.find(keyname);
	if (it == skyLayerMgr->allSkyLayers.end())
		return Q_NULLPTR;

	StelSkyLayerMgr::SkyLayerElem* elem = it.value();
	if (!elem || !elem->layer)
		return Q_NULLPTR;

	return dynamic_cast<StelSkyImageTile*>(elem->layer.data());
}


//! Set visibility for all subtiles under a specific tile.
//! @param key Tile name (group name) to modify.
//! @param visible True to show the tile, false to hide.
//! @return True if tile exists and visibility was set; false otherwise.
bool TileManager::setTileVisible(const QString& key, bool visible)
{
	StelSkyImageTile* tile = getTile(key);
	if (!tile) return false;

	for (auto subTile : tile->getSubTiles())
	{
		if (auto imageTile = dynamic_cast<StelSkyImageTile*>(subTile))
		{
			imageTile->setVisible(visible);
		}
	}
	return true;
}


//! Load and insert a sky image tile from a configuration file.
//! @param configFilePath Relative path to the JSON configuration file.
//! @param texName Group name (tile key) to register this tile under.
//! @param show Whether the tile should be shown immediately after loading.
//! @return True if insertion succeeded; false otherwise.
bool TileManager::insertTileFromConfig(const QString& configFilePath, const QString& texName, bool show)
{
	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
	QString fullPath = StelFileMgr::getUserDir() + configFilePath;

	if (fullPath.isEmpty())
	{
		qWarning() << "[TileManager] Invalid config path:" << configFilePath;
		return false;
	}

	// Remove existing tile with the same name if present
	if (skyLayerMgr->getAllKeys().contains(texName))
		skyLayerMgr->removeSkyLayer(texName);

	// Insert new tile using provided config
	skyLayerMgr->insertSkyImage(fullPath, QString(), show, 1);
	return true;
}


//! Remove a tile from the Stellarium sky layer manager by its name.
//! @param texName Name of the texture group to remove.
void TileManager::removeTile(const QString& texName)
{
	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
	skyLayerMgr->removeSkyLayer(texName);
}


//! Check whether two sky tiles have visible overlapping regions.
//! @param tileA First tile.
//! @param tileB Second tile.
//! @return True if a conflict (intersection) exists; false otherwise.
bool TileManager::hasConflict(StelSkyImageTile* tileA, StelSkyImageTile* tileB)
{
	if (!tileA || !tileB) return false;

	for (auto subA : tileA->getSubTiles())
	{
		auto imageA = dynamic_cast<StelSkyImageTile*>(subA);
		if (!imageA || !imageA->getVisible() || imageA->getSkyConvexPolygons().isEmpty())
			continue;

		auto polyA = imageA->getSkyConvexPolygons()[0];

		for (auto subB : tileB->getSubTiles())
		{
			auto imageB = dynamic_cast<StelSkyImageTile*>(subB);
			if (!imageB || !imageB->getVisible() || imageB->getSkyConvexPolygons().isEmpty())
				continue;

			auto polyB = imageB->getSkyConvexPolygons()[0];
			if (polyA->intersects(polyB))
				return true;
		}
	}
	return false;
}


//! Hide parts of the default texture that conflict with custom texture overlays.
//! @param defaultTexName Name of the default texture group.
//! @param customTexName Name of the custom texture group.
void TileManager::resolveConflicts(const QString& defaultTexName, const QString& customTexName)
{
	StelSkyImageTile* defTile = getTile(defaultTexName);
	StelSkyImageTile* cusTile = getTile(customTexName);

	if (!defTile || !cusTile) return;

	for (auto subDef : defTile->getSubTiles())
	{
		auto tileDef = dynamic_cast<StelSkyImageTile*>(subDef);
		if (!tileDef || !tileDef->getVisible() || tileDef->getSkyConvexPolygons().isEmpty())
			continue;

		auto polyDef = tileDef->getSkyConvexPolygons()[0];

		for (auto subCus : cusTile->getSubTiles())
		{
			auto tileCus = dynamic_cast<StelSkyImageTile*>(subCus);
			if (!tileCus || !tileCus->getVisible() || tileCus->getSkyConvexPolygons().isEmpty())
				continue;

			auto polyCus = tileCus->getSkyConvexPolygons()[0];
			if (polyCus->intersects(polyDef))
			{
				// Hide the default tile if it overlaps with custom tile
				tileDef->setVisible(false);
				break;
			}
		}
	}
}


//! Delete all image files listed in a configuration manager's JSON file.
//! @param configManager Pointer to the loaded configuration manager.
//! @param pluginDir Directory path prefix where the image files are located.
void TileManager::deleteImagesFromConfig(TextureConfigManager* configManager, const QString& pluginDir)
{
	if (!configManager || !configManager->load())
	{
		qWarning() << "[TileManager] Failed to load configManager";
		return;
	}

	QJsonArray subTiles = configManager->getSubTiles();
	for (const QJsonValue& tileVal : subTiles)
	{
		if (!tileVal.isObject()) continue;
		QJsonObject obj = tileVal.toObject();
		QString imageUrl = obj.value("imageUrl").toString();
		QString imagePath = StelFileMgr::getUserDir() + pluginDir + imageUrl;

		if (QFile::exists(imagePath))
		{
			// Try deleting the image file
			if (!QFile::remove(imagePath))
			{
				qWarning() << "[TileManager] Failed to delete:" << imagePath;
			}
		}
		else
		{
			qDebug() << "[TileManager] Image not found:" << imagePath;
		}
	}
}
