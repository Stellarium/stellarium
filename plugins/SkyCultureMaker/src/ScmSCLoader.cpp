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

#include "ScmSCLoader.hpp"

#include "Constellation.hpp"
#include "ScmConstellation.hpp"
#include "StarMgr.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "types/Classification.hpp"
#include "types/CulturePolygon.hpp"
#include "types/License.hpp"
#include "types/Region.hpp"
#include "types/ScmCulturalName.hpp"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QRegularExpression>
#include <QTextStream>
#include <QWidget>

scm::ScmSkyCulture *ScmSCLoader::loadFromDirectory(const QDir &dir, QString *errorMsg)
{
	if (!dir.exists())
	{
		if (errorMsg) *errorMsg = QObject::tr("Directory does not exist: %1").arg(dir.absolutePath());
		qWarning() << "SkyCultureMaker: directory does not exist:" << dir.absolutePath();
		return nullptr;
	}

	auto *sc = new scm::ScmSkyCulture();

	if (!parseIndexJson(dir, sc, errorMsg))
	{
		delete sc;
		return nullptr;
	}

	parseTerritoryGeoJson(dir, sc);
	parseDescriptionMd(dir, sc);

	return sc;
}

scm::ScmSkyCulture *ScmSCLoader::selectAndLoad(QWidget *parent, const QString &defaultPath, QString *errorMsg)
{
	const QString chosen = QFileDialog::getExistingDirectory(parent, QObject::tr("Select Sky Culture Directory"),
	                                                         defaultPath.isEmpty() ? QDir::homePath() : defaultPath);

	if (chosen.isEmpty()) return nullptr;
	return loadFromDirectory(QDir(chosen), errorMsg);
}

bool ScmSCLoader::parseIndexJson(const QDir &dir, scm::ScmSkyCulture *sc, QString *errorMsg)
{
	const QString filePath = dir.absoluteFilePath("index.json");
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		if (errorMsg) *errorMsg = QObject::tr("Cannot open index.json: %1").arg(file.errorString());
		qWarning() << "SkyCultureMaker: cannot open" << filePath << ":" << file.errorString();
		return false;
	}

	QJsonParseError parseError;
	const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
	file.close();

	if (parseError.error != QJsonParseError::NoError)
	{
		if (errorMsg) *errorMsg = QObject::tr("Failed to parse index.json: %1").arg(parseError.errorString());
		qWarning() << "SkyCultureMaker: JSON parse error in" << filePath << ":" << parseError.errorString();
		return false;
	}
	if (!doc.isObject())
	{
		if (errorMsg) *errorMsg = QObject::tr("index.json does not contain a JSON object.");
		return false;
	}

	const QJsonObject root = doc.object();
	parseIndexJsonBasicFields(root, dir, sc);

	const QJsonArray constellationsArr = root["constellations"].toArray();
	if (constellationsArr.isEmpty())
	{
		qInfo() << "SkyCultureMaker: no constellations in" << filePath;
	}
	else
	{
		parseIndexJsonConstellations(constellationsArr, dir, sc);
	}

	parseIndexJsonCommonNames(root["common_names"].toObject(), sc);

	return true;
}

void ScmSCLoader::parseIndexJsonBasicFields(const QJsonObject &root, const QDir &dir, scm::ScmSkyCulture *sc)
{
	sc->setId(root["id"].toString(dir.dirName()));
	sc->setBeginTime(root["beginTime"].toInt());
	sc->setEndTime(root["endTime"].toInt());
	sc->setFallbackToInternationalNames(root["fallback_to_international_names"].toBool(false));

	scm::Description desc;
	const QString classStr = root["classification"].toArray().first().toString().toLower().trimmed();
	for (const auto &cl : scm::CLASSIFICATIONS)
	{
		if (cl.second.name.toLower() == classStr)
		{
			desc.classification = cl.first;
			break;
		}
	}

	const QString regionStr = root["region"].isArray() ? root["region"].toArray().first().toString()
	                                                   : root["region"].toString();
	for (const auto &rg : scm::REGIONS)
	{
		if (rg.second.name == regionStr)
		{
			desc.region = rg.first;
			break;
		}
	}
	sc->setDescription(desc);
}

void ScmSCLoader::parseIndexJsonConstellations(const QJsonArray &constellationsArr, const QDir &dir,
                                               scm::ScmSkyCulture *sc)
{
	StarMgr *starMgr = GETSTELMODULE(StarMgr);
	StelCore *core   = StelApp::getInstance().getCore();

	for (const QJsonValue &consVal : constellationsArr)
	{
		const QJsonObject consObj = consVal.toObject();
		Constellation tempCons;
		if (!tempCons.read(consObj, starMgr))
		{
			qWarning() << "SkyCultureMaker: failed to read constellation" << consObj["id"].toString();
			continue;
		}

		const bool isDark                        = tempCons.isDarkConstellation();
		const std::vector<StelObjectP> &rawLines = isDark ? tempCons.getDarkConstellationLines()
		                                                  : tempCons.getConstellationLines();

		// Convert StelObjectP pairs to ConstellationLine pairs
		std::vector<scm::ConstellationLine> lines;
		lines.reserve(rawLines.size() / 2);
		for (size_t i = 0; i + 1 < rawLines.size(); i += 2)
		{
			scm::ConstellationLine line;
			line.start.coordinate = rawLines[i]->getJ2000EquatorialPos(core);
			line.end.coordinate   = rawLines[i + 1]->getJ2000EquatorialPos(core);
			if (!isDark)
			{
				line.start.star = rawLines[i]->getID();
				line.end.star   = rawLines[i + 1]->getID();
			}
			lines.push_back(line);
		}

		if (lines.empty()) continue;

		scm::ScmConstellation &constellation = sc->addConstellation(tempCons.getShortName(), lines, isDark);
		scm::ScmCulturalName cn(tempCons.getCulturalName());

		const QJsonObject nameObj = consObj["common_name"].toObject();
		const QJsonArray refs     = nameObj["references"].toArray();
		for (const QJsonValue &ref : refs)
			cn.references.append(ref.toInt());

		constellation.setCulturalName(cn);

		const QJsonValue imgVal = consObj["image"];
		if (!imgVal.isObject()) continue;

		const QJsonObject imgObj    = imgVal.toObject();
		const QString imgFile       = imgObj["file"].toString();
		const QJsonArray anchorsArr = imgObj["anchors"].toArray();
		if (imgFile.isEmpty() || anchorsArr.size() < 3) continue;

		const QString imgPath = dir.absoluteFilePath(imgFile);
		const QImage image(imgPath);
		if (image.isNull())
		{
			qWarning() << "SkyCultureMaker: cannot load artwork image" << imgPath;
			continue;
		}
		std::array<scm::Anchor, 3> anchors;
		for (int a = 0; a < 3; ++a)
		{
			const QJsonObject aObj = anchorsArr[a].toObject();
			const QJsonArray pos   = aObj["pos"].toArray();
			anchors[a].position    = Vec2i(pos[0].toInt(), pos[1].toInt());
			anchors[a].hip         = aObj["hip"].toInt();
		}
		scm::ScmConstellationArtwork artwork(anchors, image);
		artwork.setupArt();
		constellation.setArtwork(artwork);
	}
}

void ScmSCLoader::parseIndexJsonCommonNames(const QJsonObject &commonNamesObj, scm::ScmSkyCulture *sc)
{
	if (commonNamesObj.isEmpty()) return;

	QMap<QString, QList<scm::ScmCulturalName>> culturalNames;
	for (auto it = commonNamesObj.constBegin(); it != commonNamesObj.constEnd(); ++it)
	{
		const QJsonArray namesArr = it.value().toArray();
		QList<scm::ScmCulturalName> names;
		for (const QJsonValue &nv : namesArr)
		{
			names.append(scm::ScmCulturalName::fromJson(nv.toObject()));
		}
		if (!names.isEmpty()) culturalNames.insert(it.key(), names);
	}
	sc->setCulturalNames(culturalNames);
}

bool ScmSCLoader::parseTerritoryGeoJson(const QDir &dir, scm::ScmSkyCulture *sc)
{
	const QString filePath = dir.absoluteFilePath("territory.geojson");
	if (!QFile::exists(filePath)) return true;

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		qWarning() << "SkyCultureMaker: cannot open" << filePath;
		return false;
	}

	QJsonParseError parseError;
	const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
	file.close();

	if (parseError.error != QJsonParseError::NoError || !doc.isObject())
	{
		qWarning() << "SkyCultureMaker: failed to parse territory.geojson:" << parseError.errorString();
		return false;
	}

	const QJsonObject root    = doc.object();
	const QJsonArray features = root["features"].toArray();

	for (const QJsonValue &fv : features)
	{
		const QJsonObject feature    = fv.toObject();
		const QJsonObject properties = feature["properties"].toObject();
		const QJsonObject geometry   = feature["geometry"].toObject();

		if (geometry["type"].toString() != "Polygon") continue;

		const int id        = properties["id"].toInt();
		const int beginTime = properties["beginTime"].toInt();
		const int endTime   = properties["endTime"].toInt();

		// coordinates: [[[lon, lat], [lon, lat], ...]]
		const QJsonArray coordsOuter = geometry["coordinates"].toArray();
		if (coordsOuter.isEmpty()) continue;
		const QJsonArray ring = coordsOuter[0].toArray(); // first (outer) ring

		QPolygonF polygon;
		polygon.reserve(ring.size());
		for (const QJsonValue &ptVal : ring)
		{
			const QJsonArray pt = ptVal.toArray();
			if (pt.size() < 2) continue;
			// GeoJSON spec requires the ring to be closed (last == first), so we skip the duplicate
			const QPointF point(pt[0].toDouble(), pt[1].toDouble());
			if (!polygon.isEmpty() && polygon.last() == point) continue;
			polygon.append(point);
		}

		if (polygon.size() >= 3) sc->addLocation(scm::CulturePolygon(id, beginTime, endTime, polygon));
	}

	return true;
}

bool ScmSCLoader::parseDescriptionMd(const QDir &dir, scm::ScmSkyCulture *sc)
{
	const QString filePath = dir.absoluteFilePath("description.md");
	if (!QFile::exists(filePath)) return true;

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "SkyCultureMaker: cannot open" << filePath;
		return false;
	}

	QTextStream in(&file);
	const QString text = in.readAll();
	file.close();

	Section current = Section::None;
	QMap<Section, QString> sectionContent;
	QString name;

	// scans for markdown headings with depth 1-6
	static QRegularExpression headingRe("^#{1,6}\\s+(.+)$");

	for (const QString &rawLine : text.split('\n'))
	{
		const QRegularExpressionMatch m = headingRe.match(rawLine);
		if (m.hasMatch())
		{
			const QString heading = m.captured(1).trimmed();
			const Section s       = sectionOf(heading);

			if (s != Section::None)
			{
				// Known section heading -> switch to that section
				current = s;
				continue;
			}
			if (name.isEmpty())
			{
				// Heuristic: first unrecognized heading becomes the sky culture name
				name    = heading;
				current = Section::None;
				continue;
			}
			// Any other unrecognized heading: route its line and all
			// subsequent content into Description until a known section
			// heading is encountered.
			current = Section::Description;
		}
		if (current != Section::None) sectionContent[current] += rawLine + "\n";
	}

	auto trimContent = [&sectionContent](Section s) { return sectionContent.value(s).trimmed(); };

	// Start from the existing description so that fields already loaded from
	// index.json (classification, region) are preserved
	scm::Description desc   = sc->getDescription();
	desc.name               = name;
	desc.introduction       = trimContent(Section::Introduction);
	desc.cultureDescription = trimContent(Section::Description);
	desc.sky                = trimContent(Section::Sky);
	desc.moonAndSun         = trimContent(Section::MoonAndSun);
	desc.planets            = trimContent(Section::Planets);
	desc.zodiac             = trimContent(Section::Zodiac);
	desc.milkyWay           = trimContent(Section::MilkyWay);
	desc.otherObjects       = trimContent(Section::OtherObjects);
	desc.constellations     = trimContent(Section::Constellations);
	desc.references         = trimContent(Section::References);
	desc.authors            = trimContent(Section::Authors);
	desc.about              = trimContent(Section::About);
	desc.acknowledgements   = trimContent(Section::Acknowledgements);

	const QString licenseName = trimContent(Section::License);
	desc.license              = scm::LicenseType::NONE;
	for (const auto &lc : scm::LICENSES)
	{
		if (lc.second.name == licenseName)
		{
			desc.license = lc.first;
			break;
		}
	}

	sc->setDescription(desc);
	return true;
}
