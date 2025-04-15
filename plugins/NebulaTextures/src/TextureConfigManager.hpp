#ifndef TEXTURECONFIGMANAGER_HPP
#define TEXTURECONFIGMANAGER_HPP

#include <QString>
#include <QJsonArray>
#include <QJsonObject>

class TextureConfigManager
{
public:
	explicit TextureConfigManager(const QString& configFilePath);

	bool load();
	bool save() const;
	void clearSubTiles();
	void addSubTile(const QString& imageUrl, const QJsonArray& worldCoords, double minResolution, double maxBrightness);
	void addSubTileOverwrite(const QString& imageUrl, const QJsonArray& worldCoords, double minResolution, double maxBrightness);
	bool removeSubTileByImageUrl(const QString& imageUrl);
	QJsonArray getSubTiles() const;
	QJsonObject getSubTileByImageUrl(const QString& imageUrl) const;
	QString getConfigPath() const;

private:
	QString filePath;
	QJsonObject rootObject;
};


#endif // TEXTURECONFIGMANAGER_HPP
