/*
 * TileManager - manages sky image tiles for Stellarium NebulaTextures plugin
 *
 * Handles loading, visibility, conflict resolution, and file cleanup
 *
 * Copyright (C) 2025 WANG Siliang
 * Released under the GNU General Public License
 */

#ifndef TILEMANAGER_HPP
#define TILEMANAGER_HPP

#include <QString>
#include "StelSkyImageTile.hpp"
#include "TextureConfigManager.hpp"

// 管理所有 tile 的加载、显示、冲突处理与图像文件操作
class TileManager
{
public:
	TileManager();

	// 获取贴图图层（顶层）
	StelSkyImageTile* getTile(const QString& key);

	// 设置贴图图层及其所有子贴图的可见性
	bool setTileVisible(const QString& key, bool visible);

	// 从配置文件加载贴图（自动插入）
	bool insertTileFromConfig(const QString& configFilePath, const QString& texName, bool show = true);

	// 从 SkyLayer 中移除图层
	void removeTile(const QString& texName);

	// 判断两个图层是否冲突（任意子贴图相交）
	bool hasConflict(StelSkyImageTile* tileA, StelSkyImageTile* tileB);

	// 解决默认贴图与自定义贴图之间的区域冲突（隐藏重叠部分）
	void resolveConflicts(const QString& defaultTexName, const QString& customTexName);

	// 删除配置文件中所有 imageUrl 对应的图像文件
	void deleteImagesFromConfig(TextureConfigManager* configManager, const QString& pluginDir);
};

#endif // TILEMANAGER_HPP
