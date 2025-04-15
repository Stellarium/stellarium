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

TileManager::TileManager() {}

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

bool TileManager::setTileVisible(const QString& key, bool visible)
{
	StelSkyImageTile* tile = getTile(key);
	if (!tile) return false;

	for (auto subTile : tile->getSubTiles()) {
		if (auto imageTile = dynamic_cast<StelSkyImageTile*>(subTile)) {
			imageTile->setVisible(visible);
		}
	}
	return true;
}

bool TileManager::insertTileFromConfig(const QString& configFilePath, const QString& texName, bool show)
{
	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
	QString fullPath = StelFileMgr::getUserDir() + configFilePath;

	if (fullPath.isEmpty()) {
		qWarning() << "[TileManager] Invalid config path:" << configFilePath;
		return false;
	}

	// Remove existing if already there
	skyLayerMgr->removeSkyLayer(texName);
	skyLayerMgr->insertSkyImage(fullPath, QString(), show, 1);
	return true;
}

void TileManager::removeTile(const QString& texName)
{
	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
	skyLayerMgr->removeSkyLayer(texName);
}

bool TileManager::hasConflict(StelSkyImageTile* tileA, StelSkyImageTile* tileB)
{
	if (!tileA || !tileB) return false;

	for (auto subA : tileA->getSubTiles()) {
		auto imageA = dynamic_cast<StelSkyImageTile*>(subA);
		if (!imageA || !imageA->getVisible() || imageA->getSkyConvexPolygons().isEmpty())
			continue;

		auto polyA = imageA->getSkyConvexPolygons()[0];

		for (auto subB : tileB->getSubTiles()) {
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

void TileManager::resolveConflicts(const QString& defaultTexName, const QString& customTexName)
{
	StelSkyImageTile* defTile = getTile(defaultTexName);
	StelSkyImageTile* cusTile = getTile(customTexName);

	if (!defTile || !cusTile) return;

	for (auto subDef : defTile->getSubTiles()) {
		auto tileDef = dynamic_cast<StelSkyImageTile*>(subDef);
		if (!tileDef || !tileDef->getVisible() || tileDef->getSkyConvexPolygons().isEmpty())
			continue;

		auto polyDef = tileDef->getSkyConvexPolygons()[0];

		for (auto subCus : cusTile->getSubTiles()) {
			auto tileCus = dynamic_cast<StelSkyImageTile*>(subCus);
			if (!tileCus || !tileCus->getVisible() || tileCus->getSkyConvexPolygons().isEmpty())
				continue;

			auto polyCus = tileCus->getSkyConvexPolygons()[0];
			if (polyCus->intersects(polyDef)) {
				tileDef->setVisible(false);
				break;
			}
		}
	}
}

void TileManager::deleteImagesFromConfig(TextureConfigManager* configManager, const QString& pluginDir)
{
	if (!configManager || !configManager->load()) {
		qWarning() << "[TileManager] Failed to load configManager";
		return;
	}

	QJsonArray subTiles = configManager->getSubTiles();
	for (const QJsonValue& tileVal : subTiles) {
		if (!tileVal.isObject()) continue;
		QJsonObject obj = tileVal.toObject();
		QString imageUrl = obj.value("imageUrl").toString();
		QString imagePath = StelFileMgr::getUserDir() + pluginDir + imageUrl;

		if (QFile::exists(imagePath)) {
			if (!QFile::remove(imagePath)) {
				qWarning() << "[TileManager] Failed to delete:" << imagePath;
			}
		} else {
			qDebug() << "[TileManager] Image not found:" << imagePath;
		}
	}
}
