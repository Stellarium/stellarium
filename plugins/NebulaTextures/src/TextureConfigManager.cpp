#include "TextureConfigManager.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

TextureConfigManager::TextureConfigManager(const QString& configFilePath)
	: filePath(configFilePath)
{}

bool TextureConfigManager::load()
{
	QFile file(filePath);
	if (!file.exists() || !file.open(QIODevice::ReadOnly))
		return false;

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();

	if (!doc.isObject())
		return false;

	rootObject = doc.object();
	return true;
}

bool TextureConfigManager::save() const
{
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;

	QJsonDocument doc(rootObject);
	file.write(doc.toJson(QJsonDocument::Indented));
	file.close();
	return true;
}

void TextureConfigManager::clearSubTiles()
{
	rootObject["subTiles"] = QJsonArray();
}

void TextureConfigManager::addSubTile(const QString& imageUrl, const QJsonArray& worldCoords, double minResolution, double maxBrightness)
{
	QJsonObject tile;
	tile["imageUrl"] = imageUrl;

	QJsonObject credits;
	credits["short"] = "Local User";
	tile["imageCredits"] = credits;

	QJsonArray outerWorldCoords;
	outerWorldCoords.append(worldCoords);
	tile["worldCoords"] = outerWorldCoords;

	QJsonArray texCoords = {
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

	QJsonArray subTiles = rootObject["subTiles"].toArray();
	subTiles.append(tile);
	rootObject["subTiles"] = subTiles;
}

void TextureConfigManager::addSubTileOverwrite(const QString& imageUrl, const QJsonArray& worldCoords, double minResolution, double maxBrightness)
{
	QJsonObject tile;
	tile["imageUrl"] = imageUrl;

	QJsonObject credits;
	credits["short"] = "Local User";
	tile["imageCredits"] = credits;

	QJsonArray outerWorldCoords;
	outerWorldCoords.append(worldCoords);
	tile["worldCoords"] = outerWorldCoords;

	QJsonArray texCoords = {
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

	QJsonArray subTiles;
	subTiles.append(tile);  // overwrite entire array with one tile
	rootObject["subTiles"] = subTiles;
}

bool TextureConfigManager::removeSubTileByImageUrl(const QString& imageUrl)
{
	if (!rootObject.contains("subTiles")) return false;
	QJsonArray subTiles = rootObject["subTiles"].toArray();
	QJsonArray updatedTiles;
	bool found = false;

	for (const QJsonValue& val : subTiles) {
		QJsonObject obj = val.toObject();
		if (obj["imageUrl"].toString() != imageUrl)
			updatedTiles.append(val);
		else
			found = true;
	}

	rootObject["subTiles"] = updatedTiles;
	return found;
}

QJsonArray TextureConfigManager::getSubTiles() const
{
	return rootObject.value("subTiles").toArray();
}

QJsonObject TextureConfigManager::getSubTileByImageUrl(const QString& imageUrl) const
{
	QJsonArray subTiles = rootObject.value("subTiles").toArray();
	for (const QJsonValue& val : subTiles) {
		QJsonObject obj = val.toObject();
		if (obj["imageUrl"].toString() == imageUrl)
			return obj;
	}
	return QJsonObject();
}

QString TextureConfigManager::getConfigPath() const
{
	return filePath;
}
