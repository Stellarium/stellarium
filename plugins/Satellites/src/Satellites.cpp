/*
 * Copyright (C) 2009, 2012 Matthew Gates
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelProjector.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocation.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "Satellites.hpp"
#include "Satellite.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StelJsonParser.hpp"
#include "SatellitesDialog.hpp"
#include "LabelMgr.hpp"
#include "StelTranslator.hpp"
#include "renderer/StelRenderer.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QAction>
#include <QProgressBar>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QTimer>
#include <QVariantMap>
#include <QVariant>

StelModule* SatellitesStelPluginInterface::getStelModule() const
{
	return new Satellites();
}

StelPluginInfo SatellitesStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(Satellites);

	StelPluginInfo info;
	info.id = "Satellites";
	info.displayedName = N_("Satellites");
	info.authors = "Matthew Gates, Jose Luis Canales";
	info.contact = "http://stellarium.org/";
	info.description = N_("Prediction of artificial satellite positions in Earth orbit based on NORAD TLE data");
	return info;
}

Q_EXPORT_PLUGIN2(Satellites, SatellitesStelPluginInterface)

Satellites::Satellites()
	: hintTexture(NULL)
	, texPointer(NULL)
	, pxmapGlow(NULL)
	, pxmapOnIcon(NULL)
	, pxmapOffIcon(NULL)
	, toolbarButton(NULL)
	, earth(NULL)
	, defaultHintColor(0.0, 0.4, 0.6)
	, defaultOrbitColor(0.0, 0.3, 0.6)
    , progressBar(NULL)
{
	setObjectName("Satellites");
	configDialog = new SatellitesDialog();
}

void Satellites::deinit()
{
	if(NULL != hintTexture)
	{
		delete hintTexture;
	}
	if(NULL != texPointer)
	{
		delete texPointer;
	}
}

Satellites::~Satellites()
{
	delete configDialog;
	
	if (pxmapGlow)
		delete pxmapGlow;
	if (pxmapOnIcon)
		delete pxmapOnIcon;
	if (pxmapOffIcon)
		delete pxmapOffIcon;
}


void Satellites::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();

	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Satellites");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Satellites"))
		{
			qDebug() << "Stellites::init no Satellites section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		satellitesJsonPath = StelFileMgr::findFile("modules/Satellites", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/satellites.json";

		// Load and find resources used in the plugin

		// key bindings and other actions
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		gui->getGuiAction("actionShow_Satellite_Hints")->setChecked(getFlagHints());
		gui->getGuiAction("actionShow_Satellite_Labels")->setChecked(Satellite::showLabels);

		// Gui toolbar button
		pxmapGlow = new QPixmap(":/graphicGui/glow32x32.png");
		pxmapOnIcon = new QPixmap(":/satellites/bt_satellites_on.png");
		pxmapOffIcon = new QPixmap(":/satellites/bt_satellites_off.png");
		toolbarButton = new StelButton(NULL, *pxmapOnIcon, *pxmapOffIcon, *pxmapGlow, gui->getGuiAction("actionShow_Satellite_Hints"));
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");

		connect(gui->getGuiAction("actionShow_Satellite_ConfigDialog_Global"), SIGNAL(toggled(bool)), configDialog, SLOT(setVisible(bool)));
		connect(configDialog, SIGNAL(visibleChanged(bool)), gui->getGuiAction("actionShow_Satellite_ConfigDialog_Global"), SLOT(setChecked(bool)));
		connect(gui->getGuiAction("actionShow_Satellite_Hints"), SIGNAL(toggled(bool)), this, SLOT(setFlagHints(bool)));
		connect(gui->getGuiAction("actionShow_Satellite_Labels"), SIGNAL(toggled(bool)), this, SLOT(setFlagLabels(bool)));

	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Satellites::init error: " << e.what();
		return;
	}

	// A timer for hiding alert messages
	messageTimer = new QTimer(this);
	messageTimer->setSingleShot(true);   // recurring check for update
	messageTimer->setInterval(9000);      // 6 seconds should be enough time
	messageTimer->stop();
	connect(messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

	// If the json file does not already exist, create it from the resource in the QT resource
	if(QFileInfo(satellitesJsonPath).exists())
	{
		if (getJsonFileVersion() != SATELLITES_PLUGIN_VERSION)
		{
			displayMessage(q_("The old satellites.json file is no longer compatible - using default file"), "#bb0000");
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Satellites::init satellites.json does not exist - copying default file to " << satellitesJsonPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "Satellites::init using satellite.json file: " << satellitesJsonPath;

	// create satellites according to content os satellites.json file
	readJsonFile();

	// Set up download manager and the update schedule
	downloadMgr = new QNetworkAccessManager(this);
	connect(downloadMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(updateDownloadComplete(QNetworkReply*)));
	updateState = CompleteNoUpdates;
	updateTimer = new QTimer(this);
	updateTimer->setSingleShot(false);   // recurring check for update
	updateTimer->setInterval(13000);     // check once every 13 seconds to see if it is time for an update
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
	updateTimer->start();

	earth = GETSTELMODULE(SolarSystem)->getEarth();
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	// Handle changes to the observer location:
	connect(StelApp::getInstance().getCore(), SIGNAL(locationChanged(StelLocation)), this, SLOT(observerLocationChanged(StelLocation)));
	
	//Load the module's custom style sheets
	QFile styleSheetFile;
	styleSheetFile.setFileName(":/satellites/normalStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text))
	{
		normalStyleSheet = styleSheetFile.readAll();
	}
	styleSheetFile.close();
	styleSheetFile.setFileName(":/satellites/nightStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text))
	{
		nightStyleSheet = styleSheetFile.readAll();
	}
	styleSheetFile.close();

	connect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
}

bool Satellites::backupJsonFile(bool deleteOriginal)
{
	QFile old(satellitesJsonPath);
	if (!old.exists())
	{
		qWarning() << "Satellites::backupJsonFile no file to backup";
		return false;
	}

	QString backupPath = satellitesJsonPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "Satellites::backupJsonFile WARNING - could not remove old satellites.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Satellites::backupJsonFile WARNING - failed to copy satellites.json to satellites.json.old";
		return false;
	}

	return true;
}

void Satellites::setStelStyle(const QString& mode)
{
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized)
		{
			sat->setNightColors(mode=="night_color");
		}
	}
}

const StelStyle Satellites::getModuleStyleSheet(const StelStyle& style)
{
	StelStyle pluginStyle(style);
	if (style.confSectionName == "color")
	{
		pluginStyle.qtStyleSheet.append(normalStyleSheet);
	}
	else
	{
		pluginStyle.qtStyleSheet.append(nightStyleSheet);
	}
	return pluginStyle;
}



double Satellites::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+1.;
	return 0;
}

QList<StelObjectP> Satellites::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;
	if (!hintFader || StelApp::getInstance().getCore()->getCurrentLocation().planetName != earth->getEnglishName())
		return result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
		{
			equPos = sat->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(sat));
			}
		}
	}
	return result;
}

StelObjectP Satellites::searchByNameI18n(const QString& nameI18n) const
{
	if (!hintFader || StelApp::getInstance().getCore()->getCurrentLocation().planetName != earth->getEnglishName())
		return NULL;
	
	QString objw = nameI18n.toUpper();
	
	StelObjectP result = searchByNoradNumber(objw);
	if (result)
		return result;

	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
		{
			if (sat->getNameI18n().toUpper() == nameI18n)
				return qSharedPointerCast<StelObject>(sat);
		}
	}

	return NULL;
}

StelObjectP Satellites::searchByName(const QString& englishName) const
{
	if (!hintFader || StelApp::getInstance().getCore()->getCurrentLocation().planetName != earth->getEnglishName())
		return NULL;

	QString objw = englishName.toUpper();
	
	StelObjectP result = searchByNoradNumber(objw);
	if (result)
		return result;
	
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
		{
			if (sat->getEnglishName().toUpper() == englishName)
				return qSharedPointerCast<StelObject>(sat);
		}
	}

	return NULL;
}

StelObjectP Satellites::searchByNoradNumber(const QString &noradNumber) const
{
	if (!hintFader || StelApp::getInstance().getCore()->getCurrentLocation().planetName != earth->getEnglishName())
		return NULL;
	
	// If the search string is a catalog number...
	QRegExp regExp("^(NORAD)\\s*(\\d+)\\s*$");
	if (regExp.exactMatch(noradNumber))
	{
		QString numberString = regExp.capturedTexts().at(2);
		bool ok;
		/* int number = */ numberString.toInt(&ok);
		if (!ok)
			return StelObjectP();
		
		foreach(const SatelliteP& sat, satellites)
		{
			if (sat->initialized && sat->visible)
			{
				if (sat->getCatalogNumberString() == numberString)
					return qSharedPointerCast<StelObject>(sat);
			}
		}
	}
	
	return StelObjectP();
}

QStringList Satellites::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (!hintFader || StelApp::getInstance().getCore()->getCurrentLocation().planetName != earth->getEnglishName())
		return result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	QString numberPrefix;
	QRegExp regExp("^(NORAD)\\s*(\\d+)\\s*$");
	if (regExp.exactMatch(objw))
	{
		QString numberString = regExp.capturedTexts().at(2);
		bool ok;
		/* int number = */ numberString.toInt(&ok);
		if (ok)
			numberPrefix = numberString;
	}
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
		{
			if (sat->getNameI18n().toUpper().left(objw.length()) == objw)
			{
				result << sat->getNameI18n().toUpper();
			}
			else if (!numberPrefix.isEmpty() && sat->getCatalogNumberString().left(numberPrefix.length()) == numberPrefix)
			{
				result << QString("NORAD %1").arg(sat->getCatalogNumberString());
			}
		}
	}

	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList Satellites::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		foreach(const SatelliteP& sat, satellites)
		{
			result << sat->getEnglishName();
		}
	}
	else
	{
		foreach(const SatelliteP& sat, satellites)
		{
			result << sat->getNameI18n();
		}
	}
	return result;
}

bool Satellites::configureGui(bool show)
{
	if (show)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		gui->getGuiAction("actionShow_Satellite_ConfigDialog_Global")->setChecked(true);
	}

	return true;
}

void Satellites::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	restoreDefaultJsonFile();
	readJsonFile();
	readSettingsFromConfig();
}

void Satellites::restoreDefaultConfigIni(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// delete all existing Satellite settings...
	conf->remove("");

	conf->setValue("show_satellite_hints", false);
	conf->setValue("show_satellite_labels", true);
	conf->setValue("updates_enabled", true);
	conf->setValue("hint_color", "0.0,0.4,0.6");
	conf->setValue("hint_font_size", 10);
	conf->setValue("tle_url0", "http://celestrak.com/NORAD/elements/noaa.txt");
	conf->setValue("tle_url1", "http://celestrak.com/NORAD/elements/goes.txt");
	conf->setValue("tle_url2", "http://celestrak.com/NORAD/elements/gps-ops.txt");
	conf->setValue("tle_url3", "http://celestrak.com/NORAD/elements/galileo.txt");
	conf->setValue("tle_url4", "http://celestrak.com/NORAD/elements/visual.txt");
	conf->setValue("tle_url5", "http://celestrak.com/NORAD/elements/amateur.txt");
	conf->setValue("tle_url6", "http://celestrak.com/NORAD/elements/iridium.txt");
	conf->setValue("tle_url7", "http://celestrak.com/NORAD/elements/geo.txt");
	conf->setValue("tle_url8", "http://celestrak.com/NORAD/elements/tle-new.txt");
	conf->setValue("tle_url9", "http://celestrak.com/NORAD/elements/science.txt");
	//TODO: Better? See http://doc.qt.nokia.com/4.7/qsettings.html#beginWriteArray --BM
	conf->setValue("update_frequency_hours", 72);
	conf->setValue("orbit_line_flag", true);
	conf->setValue("orbit_line_segments", 90);
	conf->setValue("orbit_fade_segments", 5);
	conf->setValue("orbit_segment_duration", 20);
	conf->endGroup();
}

void Satellites::restoreDefaultJsonFile(void)
{
	if (QFileInfo(satellitesJsonPath).exists())
		backupJsonFile(true);

	QFile src(":/satellites/satellites.json");
	if (!src.copy(satellitesJsonPath))
	{
		qWarning() << "Satellites::restoreDefaultJsonFile cannot copy json resource to " + satellitesJsonPath;
	}
	else
	{
		qDebug() << "Satellites::init copied default satellites.json to " << satellitesJsonPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(satellitesJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is schreduled in a timely
		// manner
		StelApp::getInstance().getSettings()->remove("Satellites/last_update");
		lastUpdate = QDateTime::fromString("2001-05-25T12:00:00", Qt::ISODate);

	}
}

void Satellites::readSettingsFromConfig(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// populate updateUrls from tle_url? keys
	QRegExp keyRE("^tle_url\\d+$");
	updateUrls.clear();
	foreach(const QString& key, conf->childKeys())
	{
		if (keyRE.exactMatch(key))
		{
			QString s = conf->value(key, "").toString();
			if (!s.isEmpty() && s!="")
				updateUrls << s;
		}
	}

	// updater related settings...
	updateFrequencyHours = conf->value("update_frequency_hours", 72).toInt();
	// last update default is the first Towell Day.  <3 DA
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2001-05-25T12:00:00").toString(), Qt::ISODate);
	setFlagHints(conf->value("show_satellite_hints", false).toBool());
	Satellite::showLabels = conf->value("show_satellite_labels", true).toBool();
	updatesEnabled = conf->value("updates_enabled", true).toBool();

	// Get a font for labels
	labelFont.setPixelSize(conf->value("hint_font_size", 10).toInt());

	// orbit drawing params
	Satellite::orbitLinesFlag = conf->value("orbit_line_flag", true).toBool();
	Satellite::orbitLineSegments = conf->value("orbit_line_segments", 90).toInt();
	Satellite::orbitLineFadeSegments = conf->value("orbit_fade_segments", 5).toInt();
	Satellite::orbitLineSegmentDuration = conf->value("orbit_segment_duration", 20).toInt();

	conf->endGroup();
}

void Satellites::saveSettingsToConfig(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// update tle urls... first clear the existing ones in the file
	QRegExp keyRE("^tle_url\\d+$");
	foreach(const QString& key, conf->childKeys())
	{
		if (keyRE.exactMatch(key))
			conf->remove(key);
	}


	// populate updateUrls from tle_url? keys
	int n=0;
	foreach(const QString& url, updateUrls)
	{
		QString key = QString("tle_url%1").arg(n++);
		conf->setValue(key, url);
	}

	// updater related settings...
	conf->setValue("update_frequency_hours", updateFrequencyHours);
	conf->setValue("show_satellite_hints", getFlagHints());
	conf->setValue("show_satellite_labels", Satellite::showLabels);
	conf->setValue("updates_enabled", updatesEnabled );

	// Get a font for labels
	conf->setValue("hint_font_size", labelFont.pixelSize());

	// orbit drawing params
	conf->setValue("orbit_line_flag", Satellite::orbitLinesFlag);
	conf->setValue("orbit_line_segments", Satellite::orbitLineSegments);
	conf->setValue("orbit_fade_segments", Satellite::orbitLineFadeSegments);
	conf->setValue("orbit_segment_duration", Satellite::orbitLineSegmentDuration);

	conf->endGroup();
}

void Satellites::readJsonFile(void)
{
	setTleMap(loadTleMap());
}

const QString Satellites::getJsonFileVersion(void)
{
	QString jsonVersion("unknown");
	QFile satelliteJsonFile(satellitesJsonPath);
	if (!satelliteJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Satellites::init cannot open " << satellitesJsonPath;
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&satelliteJsonFile).toMap();
	if (map.contains("creator"))
	{
		QString creator = map.value("creator").toString();
		QRegExp vRx(".*(\\d+\\.\\d+\\.\\d+).*");
		if (vRx.exactMatch(creator))
		{
			jsonVersion = vRx.capturedTexts().at(1);
		}
	}

	satelliteJsonFile.close();
	qDebug() << "Satellites::getJsonFileVersion() version from file:" << jsonVersion;
	return jsonVersion;
}

bool Satellites::saveTleMap(const QVariantMap& map, QString path)
{
	if (path.isEmpty())
		path = satellitesJsonPath;

	QFile jsonFile(path);
	StelJsonParser parser;

	if (jsonFile.exists())
		jsonFile.remove();

	if (!jsonFile.open(QIODevice::WriteOnly))
	{
		qWarning() << "Satellites::saveTleMap() cannot open for writing:" << path;
		return false;
	}
	else
	{
		qDebug() << "Satellites::saveTleMap() writing to:" << path;
		parser.write(map, &jsonFile);
		jsonFile.close();
		return true;
	}
}

QVariantMap Satellites::loadTleMap(QString path)
{
	if (path.isEmpty())
		path = satellitesJsonPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "Satellites::loadTleMap cannot open " << path;
	else
		map = StelJsonParser::parse(&jsonFile).toMap();

	jsonFile.close();
	return map;
}

void Satellites::setTleMap(const QVariantMap& map)
{
	int numReadOk = 0;
	QVariantList defaultHintColorMap;
	defaultHintColorMap << defaultHintColor[0] << defaultHintColor[1] << defaultHintColor[2];

	if (map.contains("hintColor"))
	{
		defaultHintColorMap = map.value("hintColor").toList();
		defaultHintColor.set(defaultHintColorMap.at(0).toDouble(), defaultHintColorMap.at(1).toDouble(), defaultHintColorMap.at(2).toDouble());
	}

	satellites.clear();
	QVariantMap satMap = map.value("satellites").toMap();
	foreach(const QString& satId, satMap.keys())
	{
		QVariantMap satData = satMap.value(satId).toMap();

		if (!satData.contains("hintColor"))
			satData["hintColor"] = defaultHintColorMap;

		if (!satData.contains("orbitColor"))
			satData["orbitColor"] = satData["hintColor"];

		SatelliteP sat(new Satellite(satId, satData));
		if (sat->initialized)
		{
			satellites.append(sat);
			numReadOk++;
		}
	}
}

QVariantMap Satellites::getTleMap(void)
{
	QVariantMap map;
	QVariantList defHintCol;
	defHintCol << Satellite::roundToDp(defaultHintColor[0],3)
						 << Satellite::roundToDp(defaultHintColor[1],3)
						 << Satellite::roundToDp(defaultHintColor[2],3);

	map["creator"] = QString("Satellites plugin version %1 (updated)").arg(SATELLITES_PLUGIN_VERSION);
	map["hintColor"] = defHintCol;
	map["shortName"] = "satellite orbital data";
	QVariantMap sats;
	foreach(const SatelliteP& sat, satellites)
	{
		QVariantMap satMap = sat->getMap();

		if (satMap["orbitColor"] == satMap["hintColor"])
			satMap.remove("orbitColor");

		if (satMap["hintColor"].toList() == defHintCol)
			satMap.remove("hintColor");

		sats[sat->id] = satMap;
	}
	map["satellites"] = sats;
	return map;
}

QStringList Satellites::getGroups(void) const
{
	QStringList groups;
	foreach (const SatelliteP& sat, satellites)
	{
		if (sat->initialized)
		{
			foreach(const QString& group, sat->groupIDs)
			{
				if (!groups.contains(group))
					groups << group;
			}
		}
	}
	return groups;
}

QHash<QString,QString> Satellites::getSatellites(const QString& group, Status vis)
{
	QHash<QString,QString> result;

	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized)
		{
			if ((group.isEmpty() || sat->groupIDs.contains(group)) && ! result.contains(sat->id))
			{
				if (vis==Both ||
						(vis==Visible && sat->visible) ||
						(vis==NotVisible && !sat->visible) ||
						(vis==OrbitError && !sat->orbitValid) ||
						(vis==NewlyAdded && sat->isNew()))
					result.insert(sat->id, sat->name);
			}
		}
	}
	return result;
}

SatelliteP Satellites::getByID(const QString& id)
{
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->id == id)
			return sat;
	}
	return SatelliteP();
}

QStringList Satellites::getAllIDs()
{
	QStringList result;
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized)
			result.append(sat->id);
	}
	return result;
}

void Satellites::add(const TleDataList& newSatellites)
{
	int numAdded = 0;
	QVariantList defaultHintColorMap;
	defaultHintColorMap << defaultHintColor[0] << defaultHintColor[1]
											<< defaultHintColor[2];
	
	foreach (const TleData& tleSet, newSatellites)
	{
		//TODO: Duplicates check? --BM
		
		if (tleSet.id.isEmpty() ||
				tleSet.name.isEmpty() ||
				tleSet.first.isEmpty() ||
				tleSet.second.isEmpty())
			continue;
		
		QVariantMap satProperties;
		satProperties.insert("name", tleSet.name);
		satProperties.insert("tle1", tleSet.first);
		satProperties.insert("tle2", tleSet.second);
		satProperties.insert("hintColor", defaultHintColorMap);
		//TODO: Decide if newly added satellites are visible by default --BM
		satProperties.insert("visible", true);
		satProperties.insert("orbitVisible", false);
		
		SatelliteP sat(new Satellite(tleSet.id, satProperties));
		if (sat->initialized)
		{
			qDebug() << "Satellites: added" << tleSet.id << tleSet.name;
			satellites.append(sat);
			sat->setNew();
			numAdded++;
		}
	}
	qDebug() << "Satellites: "
					 << newSatellites.count() << "satellites proposed for addition, "
					 << numAdded << " added, "
					 << satellites.count() << " total after the operation.";
}

void Satellites::remove(const QStringList& idList)
{
	StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);
	int numRemoved = 0;
	for (int i = 0; i < satellites.size(); i++)
	{
		const SatelliteP& sat = satellites.at(i);
		if (idList.contains(sat->id))
		{
			QList<StelObjectP> selected = objMgr->getSelectedObject("Satellite");
			if (selected.contains(sat.staticCast<StelObject>()))
				objMgr->unSelect();
			
			qDebug() << "Satellite removed:" << sat->id << sat->name;
			satellites.removeAt(i);
			i--; //Compensate for the change in the array's indexing
			numRemoved++;
		}
	}

	qDebug() << "Satellites: "
					 << idList.count() << "satellites proposed for removal, "
					 << numRemoved << " removed, "
					 << satellites.count() << " remain.";
}

int Satellites::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyHours * 3600);
	return QDateTime::currentDateTime().secsTo(nextUpdate);
}

void Satellites::setTleSources(QStringList tleSources)
{
	updateUrls = tleSources;
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// clear old source list
	QRegExp keyRE("^tle_url\\d+$");
	foreach(const QString& key, conf->childKeys())
	{
		if (keyRE.exactMatch(key))
			conf->remove(key);
	}

	// set the new sources list
	int i=0;
	foreach (const QString& url, updateUrls)
	{
		conf->setValue(QString("tle_url%1").arg(i++), url);
	}

	conf->endGroup();
}

bool Satellites::getFlagLabels(void)
{
	return Satellite::showLabels;
}

void Satellites::setFlagLabels(bool b)
{
	Satellite::showLabels = b;
}

void Satellites::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyHours * 3600) <= QDateTime::currentDateTime())
		updateTLEs();
}

void Satellites::updateTLEs(void)
{
	if (updateState==Satellites::Updating)
	{
		qWarning() << "Satellites: already updating...  will not start again current update is complete.";
		return;
	}
	else
	{
		qDebug() << "Satellites: starting update...";
	}

	lastUpdate = QDateTime::currentDateTime();
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("Satellites/last_update", lastUpdate.toString(Qt::ISODate));

	if (updateUrls.size() == 0)
	{
		qWarning() << "Satellites::updateTLEs no update URLs are defined... nothing to do.";
		emit(tleUpdateComplete(0,satellites.count(),satellites.count()));
		return;
	}

	updateState = Satellites::Updating;
	emit(updateStateChanged(updateState));
	updateFiles.clear();
	numberDownloadsComplete = 0;

	if (progressBar==NULL)
		progressBar = StelApp::getInstance().getGui()->addProgressBar();

	progressBar->setValue(0);
	progressBar->setMaximum(updateUrls.size());
	progressBar->setVisible(true);
	progressBar->setFormat("TLE download %v/%m");

	// set off the downloads
	for(int i=0; i<updateUrls.size(); i++)
	{
		downloadMgr->get(QNetworkRequest(QUrl(updateUrls.at(i))));
	}
}

void Satellites::updateDownloadComplete(QNetworkReply* reply)
{
	// check the download worked, and save the data to file if this is the case.
	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Satellites::updateDownloadComplete FAILED to download" << reply->url() << " Error: " << reply->errorString();
	}
	else
	{
		// download completed successfully.
		try
		{
			QString partialName = QString("tle%1.txt").arg(numberDownloadsComplete);
			QString tleTmpFilePath = StelFileMgr::findFile("modules/Satellites", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/" + partialName;
			QFile tmpFile(tleTmpFilePath);
			if (tmpFile.exists())
				tmpFile.remove();

			tmpFile.open(QIODevice::WriteOnly | QIODevice::Text);
			tmpFile.write(reply->readAll());
			tmpFile.close();
			updateFiles << tleTmpFilePath;
		}
		catch (std::runtime_error &e)
		{
			qWarning() << "Satellites::updateDownloadComplete: cannot write TLE data to file:" << e.what();
		}
	}
	numberDownloadsComplete++;
	if (progressBar)
		progressBar->setValue(numberDownloadsComplete);

	// all downloads are complete...  do the update.
	if (numberDownloadsComplete >= updateUrls.size())
	{
		updateFromFiles(updateFiles, true);
	}
}

void Satellites::observerLocationChanged(StelLocation)
{
	recalculateOrbitLines();
}

void Satellites::setOrbitLinesFlag(bool b)
{
	Satellite::orbitLinesFlag = b;
}

bool Satellites::getOrbitLinesFlag(void)
{
	return Satellite::orbitLinesFlag;
}

void Satellites::recalculateOrbitLines(void)
{
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible && sat->orbitVisible)
			sat->recalculateOrbitLines();
	}
}

void Satellites::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor);
	messageTimer->start();
}

void Satellites::messageTimeout(void)
{
	foreach(const int& id, messageIDs)
	{
		GETSTELMODULE(LabelMgr)->deleteLabel(id);
	}
}

void Satellites::saveTleData(QString path)
{
	saveTleMap(getTleMap(), path);
}

void Satellites::updateFromFiles(QStringList paths, bool deleteFiles)
{
	// Container for the new data.
	TleDataHash newTleSets;

	if (progressBar)
	{
		progressBar->setValue(0);
		progressBar->setMaximum(paths.size() + 1);
		progressBar->setFormat("TLE updating %v/%m");
	}

	foreach(const QString& tleFilePath, paths)
	{
		QFile tleFile(tleFilePath);
		if (tleFile.open(QIODevice::ReadOnly|QIODevice::Text))
		{
			parseTleFile(tleFile, newTleSets);
			tleFile.close();

			if (deleteFiles)
				tleFile.remove();

			if (progressBar)
				progressBar->setValue(progressBar->value() + 1);
		}
	}

	// Right, we should now have a map of all the elements we downloaded.  For each satellite
	// which this module is managing, see if it exists with an updated element, and update it if so...
	int numUpdated = 0;
	int totalSats = 0;
	int numMissing = 0;
	foreach(const SatelliteP& sat, satellites)
	{
		totalSats++;
		QString id = sat->id;
		if (newTleSets.contains(id))
		{
			TleData newTle = newTleSets.value(id);
			if (sat->tleElements.first  != newTle.first ||
					sat->tleElements.second != newTle.second ||
					sat->name != newTle.name)
			{
				// We have updated TLE elements for this satellite
				sat->setNewTleElements(newTle.first, newTle.second);
				
				// Update the name if it has been changed in the source list
				sat->name = newTle.name;

				// we reset this to "now" when we started the update.
				sat->lastUpdated = lastUpdate;
				numUpdated++;
			}
		}
		else
		{
			qWarning() << "Satellites: could not update orbital elements for"
								 << sat->name
								 << sat->id
								 << ": no entry found in the source TLE lists.";
			numMissing++;
		}
	}

	if (numUpdated>0)
	{
		saveTleMap(getTleMap());
	}

	delete progressBar;
	progressBar = NULL;

	qDebug() << "Satellites: updated" << numUpdated << "/" << totalSats
					 << "satellites.  Update URLs contained" << newTleSets.size() << "objects. "
					 << "There were" << numMissing << "satellies missing from the update URLs";

	if (numUpdated==0)
		updateState = CompleteNoUpdates;
	else
		updateState = CompleteUpdates;

	emit(updateStateChanged(updateState));
	emit(tleUpdateComplete(numUpdated, totalSats, numMissing));
}

void Satellites::parseTleFile(QFile& openFile, TleDataHash& tleList)
{
	if (!openFile.isOpen() || !openFile.isReadable())
		return;
	
	// Code mostly re-used from updateFromFiles()
	int lineNumber = 0;
	TleData lastData;
	
	while (!openFile.atEnd())
	{
		QString line = QString(openFile.readLine()).trimmed();
		if (line.length() < 65) // this is title line
		{
			// New entry in the list, so reset all fields
			lastData = TleData();
			
			//TODO: We need to think of some kind of ecaping these
			//characters in the JSON parser. --BM
			line.replace(QRegExp("\\s*\\[([^\\]])*\\]\\s*$"),"");  // remove things in square brackets
			lastData.name = line;
		}
		else
		{
			if (QRegExp("^1 .*").exactMatch(line))
				lastData.first = line;
			else if (QRegExp("^2 .*").exactMatch(line))
			{
				lastData.second = line;
				// The Satellite Catalog Number is the second number
				// on the second line.
				QString id = line.split(' ').at(1).trimmed();
				if (id.isEmpty())
					continue;
				lastData.id = id;
				
				// This is the second line and there will be no more,
				// so if everything is OK, save the elements.
				if (!lastData.name.isEmpty() &&
						!lastData.first.isEmpty())
				{
					//TODO: This overwrites duplicates. Display warning? --BM
					tleList.insert(id, lastData);
				}
				//TODO: Error warnings? --BM
			}
			else
				qDebug() << "Satellites: unprocessed line " << lineNumber <<  " in file " << openFile.fileName();
		}
	}
}

void Satellites::update(double deltaTime)
{
	if (StelApp::getInstance().getCore()->getCurrentLocation().planetName != earth->getEnglishName() || (!hintFader && hintFader.getInterstate() <= 0.))
		return;

	hintFader.update((int)(deltaTime*1000));

	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
			sat->update(deltaTime);
	}
}

void Satellites::draw(StelCore* core, StelRenderer* renderer)
{
	if (core->getCurrentLocation().planetName != earth->getEnglishName() ||
			(core->getJDay()<2436116.3115)                               || // do not draw anything before Oct 4, 1957, 19:28:34GMT ;-)
			(!hintFader && hintFader.getInterstate() <= 0.))
		return;

	StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	renderer->setFont(labelFont);
	Satellite::hintBrightness = hintFader.getInterstate();

	renderer->setBlendMode(BlendMode_Alpha);

	if(NULL == hintTexture)
	{
		hintTexture = renderer->createTexture(":/satellites/hint.png");
	}
	hintTexture->bind();
	Satellite::viewportHalfspace = prj->getBoundingCap();
	foreach (const SatelliteP& sat, satellites)
	{
		if (sat && sat->initialized && sat->visible)
		{
			sat->draw(core, renderer, prj, hintTexture);
		}
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	{
		drawPointer(core, renderer);
	}
}

void Satellites::drawPointer(StelCore* core, StelRenderer* renderer)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Satellite");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);
		Vec3d screenpos;

		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos))
		{
			return;
		}
		if(NULL == texPointer)
		{
			texPointer = renderer->createTexture("textures/pointeur5.png");
		}
		if (StelApp::getInstance().getVisionModeNight())
			renderer->setGlobalColor(0.8f,0.0f,0.0f);
		else
			renderer->setGlobalColor(0.4f,0.5f,0.8f);
		texPointer->bind();

		renderer->setBlendMode(BlendMode_Alpha);

		// Size on screen
		float size = obj->getAngularSize(core)*M_PI/180.*prj->getPixelPerRadAtCenter();
		size += 12.f + 3.f*std::sin(2.f * StelApp::getInstance().getTotalRunTime());
		const float halfSize = size * 0.5;
		const float left   = screenpos[0] - halfSize - 20;
		const float right  = screenpos[0] + halfSize - 20;
		const float top    = screenpos[1] - halfSize - 20;
		const float bottom = screenpos[1] + halfSize - 20;
		renderer->drawTexturedRect(left,  top,    40, 40, 90);
		renderer->drawTexturedRect(left,  bottom, 40, 40, 0);
		renderer->drawTexturedRect(right, bottom, 40, 40, -90);
		renderer->drawTexturedRect(right, top,    40, 40, -180);
	}
}


