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
#include "SatellitesListModel.hpp"
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
#include <QDir>

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
	info.authors = "Matthew Gates, Jose Luis Canales, Bogdan Marinov";
	info.contact = "http://stellarium.org/";
	info.description = N_("Prediction of artificial satellite positions in Earth orbit based on NORAD TLE data");
	return info;
}

Q_EXPORT_PLUGIN2(Satellites, SatellitesStelPluginInterface)

Satellites::Satellites()
    : satelliteListModel(0),
      hintTexture(NULL)
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
		// TODO: Compatibility with installation-dir modules? --BM
		// It seems that the original code couldn't handle them either.
		QString dirPath = StelFileMgr::getUserDir() + "/modules/Satellites";
		// TODO: Ideally, this should return a QDir object
		StelFileMgr::makeSureDirExistsAndIsWritable(dirPath);
		dataDir.setPath(dirPath);

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Satellites"))
		{
			//qDebug() << "Stellites: created section in config file.";
			restoreDefaultSettings();
		}

		// populate settings from main config file.
		loadSettings();

		catalogPath = dataDir.absoluteFilePath("satellites.json");

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
	connect(messageTimer, SIGNAL(timeout()), this, SLOT(hideMessages()));

	// If the json file does not already exist, create it from the resource in the QT resource
	if(QFileInfo(catalogPath).exists())
	{
		if (readCatalogVersion() != SATELLITES_PLUGIN_VERSION)
		{
			displayMessage(q_("The old satellites.json file is no longer compatible - using default file"), "#bb0000");
			restoreDefaultCatalog();
		}
	}
	else
	{
		qDebug() << "Satellites::init satellites.json does not exist - copying default file to " << QDir::toNativeSeparators(catalogPath);
		restoreDefaultCatalog();
	}

	qDebug() << "Satellites: loading catalog file:" << QDir::toNativeSeparators(catalogPath);

	// create satellites according to content os satellites.json file
	loadCatalog();

	// Set up download manager and the update schedule
	downloadMgr = new QNetworkAccessManager(this);
	connect(downloadMgr, SIGNAL(finished(QNetworkReply*)),
	        this, SLOT(saveDownloadedUpdate(QNetworkReply*)));
	updateState = CompleteNoUpdates;
	updateTimer = new QTimer(this);
	updateTimer->setSingleShot(false);   // recurring check for update
	updateTimer->setInterval(13000);     // check once every 13 seconds to see if it is time for an update
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
	updateTimer->start();

	earth = GETSTELMODULE(SolarSystem)->getEarth();
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	// Handle changes to the observer location:
	connect(StelApp::getInstance().getCore(),
	        SIGNAL(locationChanged(StelLocation)),
	        this,
	        SLOT(updateObserverLocation(StelLocation)));
	
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

bool Satellites::backupCatalog(bool deleteOriginal)
{
	QFile old(catalogPath);
	if (!old.exists())
	{
		qWarning() << "Satellites::backupJsonFile no file to backup";
		return false;
	}

	QString backupPath = catalogPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "Satellites: WARNING: unable to remove old catalog file!";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Satellites: WARNING: failed to back up catalog file as" 
			   << QDir::toNativeSeparators(backupPath);
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
		if (sat->initialized && sat->displayed)
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
		if (sat->initialized && sat->displayed)
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
		if (sat->initialized && sat->displayed)
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
			if (sat->initialized && sat->displayed)
			{
				if (sat->getCatalogNumberString() == numberString)
					return qSharedPointerCast<StelObject>(sat);
			}
		}
	}
	
	return StelObjectP();
}

QStringList Satellites::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
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
	bool find;
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->displayed)
		{
			find = false;
			if (useStartOfWords)
			{
				if (sat->getNameI18n().toUpper().left(objw.length()) == objw)
				{
					find = true;
				}
			}
			else
			{
				if (sat->getNameI18n().toUpper().contains(objw, Qt::CaseInsensitive))
				{
					find = true;
				}
			}

			if (find)
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

QStringList Satellites::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
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
	bool find;
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->displayed)
		{
			find = false;
			if (useStartOfWords)
			{
				if (sat->getEnglishName().toUpper().left(objw.length()) == objw)
				{
					find = true;
				}
			}
			else
			{
				if (sat->getEnglishName().toUpper().contains(objw, Qt::CaseInsensitive))
				{
					find = true;
				}
			}
			if (find)
			{
				result << sat->getEnglishName().toUpper();
			}
			else if (find==false && !numberPrefix.isEmpty() && sat->getCatalogNumberString().left(numberPrefix.length()) == numberPrefix)
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
	restoreDefaultSettings();
	restoreDefaultCatalog();
	loadCatalog();
	loadSettings();
}

void Satellites::restoreDefaultSettings()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// delete all existing Satellite settings...
	conf->remove("");

	conf->setValue("show_satellite_hints", false);
	conf->setValue("show_satellite_labels", true);
	conf->setValue("updates_enabled", true);
	conf->setValue("auto_add_enabled", true);
	conf->setValue("auto_remove_enabled", true);
	conf->setValue("hint_color", "0.0,0.4,0.6");
	conf->setValue("hint_font_size", 10);
	conf->setValue("update_frequency_hours", 72);
	conf->setValue("orbit_line_flag", true);
	conf->setValue("orbit_line_segments", 90);
	conf->setValue("orbit_fade_segments", 5);
	conf->setValue("orbit_segment_duration", 20);
	
	conf->endGroup(); // saveTleSources() opens it for itself
	
	// TLE update sources
	QStringList urls;
	urls << "1,http://celestrak.com/NORAD/elements/visual.txt" // Auto-add ON!
	     << "http://celestrak.com/NORAD/elements/tle-new.txt"
	     << "http://celestrak.com/NORAD/elements/science.txt"
	     << "http://celestrak.com/NORAD/elements/noaa.txt"
	     << "http://celestrak.com/NORAD/elements/goes.txt"
	     << "http://celestrak.com/NORAD/elements/amateur.txt"
	     << "http://celestrak.com/NORAD/elements/gps-ops.txt"
	     << "http://celestrak.com/NORAD/elements/galileo.txt"
	     << "http://celestrak.com/NORAD/elements/iridium.txt"
	     << "http://celestrak.com/NORAD/elements/geo.txt";
	saveTleSources(urls);
}

void Satellites::restoreDefaultCatalog()
{
	if (QFileInfo(catalogPath).exists())
		backupCatalog(true);

	QFile src(":/satellites/satellites.json");
	if (!src.copy(catalogPath))
	{
		qWarning() << "Satellites::restoreDefaultJsonFile cannot copy json resource to " + QDir::toNativeSeparators(catalogPath);
	}
	else
	{
		qDebug() << "Satellites::init copied default satellites.json to " << QDir::toNativeSeparators(catalogPath);
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(catalogPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is schreduled in a timely
		// manner
		StelApp::getInstance().getSettings()->remove("Satellites/last_update");
		lastUpdate = QDateTime::fromString("2001-05-25T12:00:00", Qt::ISODate);

	}
}

void Satellites::loadSettings()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// Load update sources list...
	updateUrls.clear();
	
	// Backward compatibility: try to detect and read an old-stlye array.
	// TODO: Assume that the user hasn't modified their conf in a stupid way?
//	if (conf->contains("tle_url0")) // This can skip some operations...
	QRegExp keyRE("^tle_url\\d+$");
	QStringList urls;
	foreach(const QString& key, conf->childKeys())
	{
		if (keyRE.exactMatch(key))
		{
			QString url = conf->value(key).toString();
			conf->remove(key); // Delete old-style keys
			if (url.isEmpty())
				continue;
			// NOTE: This URL is also hardcoded in restoreDefaultSettings().
			if (url == "http://celestrak.com/NORAD/elements/visual.txt")
				url.prepend("1,"); // Same as in the new default configuration
			urls << url;
		}
	}
	// If any have been read, save them in the new format.
	if (!urls.isEmpty())
	{
		conf->endGroup();
		setTleSources(urls);
		conf->beginGroup("Satellites");
	}
	else
	{
		int size = conf->beginReadArray("tle_sources");
		for (int i = 0; i < size; i++)
		{
			conf->setArrayIndex(i);
			QString url = conf->value("url").toString();
			if (!url.isEmpty())
			{
				if (conf->value("add_new").toBool())
					url.prepend("1,");
				updateUrls.append(url);
			}
		}
		conf->endArray();
	}
	
	// NOTE: Providing default values AND using restoreDefaultSettings() to create the section seems redundant. --BM 
	
	// updater related settings...
	updateFrequencyHours = conf->value("update_frequency_hours", 72).toInt();
	// last update default is the first Towell Day.  <3 DA
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2001-05-25T12:00:00").toString(), Qt::ISODate);
	setFlagHints(conf->value("show_satellite_hints", false).toBool());
	Satellite::showLabels = conf->value("show_satellite_labels", true).toBool();
	updatesEnabled = conf->value("updates_enabled", true).toBool();
	autoAddEnabled = conf->value("auto_add_enabled", true).toBool();
	autoRemoveEnabled = conf->value("auto_remove_enabled", true).toBool();

	// Get a font for labels
	labelFont.setPixelSize(conf->value("hint_font_size", 10).toInt());

	// orbit drawing params
	Satellite::orbitLinesFlag = conf->value("orbit_line_flag", true).toBool();
	Satellite::orbitLineSegments = conf->value("orbit_line_segments", 90).toInt();
	Satellite::orbitLineFadeSegments = conf->value("orbit_fade_segments", 5).toInt();
	Satellite::orbitLineSegmentDuration = conf->value("orbit_segment_duration", 20).toInt();

	conf->endGroup();
}

void Satellites::saveSettings()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// updater related settings...
	conf->setValue("update_frequency_hours", updateFrequencyHours);
	conf->setValue("show_satellite_hints", getFlagHints());
	conf->setValue("show_satellite_labels", Satellite::showLabels);
	conf->setValue("updates_enabled", updatesEnabled );
	conf->setValue("auto_add_enabled", autoAddEnabled);
	conf->setValue("auto_remove_enabled", autoRemoveEnabled);

	// Get a font for labels
	conf->setValue("hint_font_size", labelFont.pixelSize());

	// orbit drawing params
	conf->setValue("orbit_line_flag", Satellite::orbitLinesFlag);
	conf->setValue("orbit_line_segments", Satellite::orbitLineSegments);
	conf->setValue("orbit_fade_segments", Satellite::orbitLineFadeSegments);
	conf->setValue("orbit_segment_duration", Satellite::orbitLineSegmentDuration);

	conf->endGroup();
	
	// Update sources...
	saveTleSources(updateUrls);
}

void Satellites::loadCatalog()
{
	setDataMap(loadDataMap());
}

const QString Satellites::readCatalogVersion()
{
	QString jsonVersion("unknown");
	QFile satelliteJsonFile(catalogPath);
	if (!satelliteJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Satellites::init cannot open " << QDir::toNativeSeparators(catalogPath);
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
	//qDebug() << "Satellites: catalog version from file:" << jsonVersion;
	return jsonVersion;
}

bool Satellites::saveDataMap(const QVariantMap& map, QString path)
{
	if (path.isEmpty())
		path = catalogPath;

	QFile jsonFile(path);
	StelJsonParser parser;

	if (jsonFile.exists())
		jsonFile.remove();

	if (!jsonFile.open(QIODevice::WriteOnly))
	{
		qWarning() << "Satellites::saveTleMap() cannot open for writing:" << QDir::toNativeSeparators(path);
		return false;
	}
	else
	{
		qDebug() << "Satellites::saveTleMap() writing to:" << QDir::toNativeSeparators(path);
		parser.write(map, &jsonFile);
		jsonFile.close();
		return true;
	}
}

QVariantMap Satellites::loadDataMap(QString path)
{
	if (path.isEmpty())
		path = catalogPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "Satellites::loadTleMap cannot open " << QDir::toNativeSeparators(path);
	else
		map = StelJsonParser::parse(&jsonFile).toMap();

	jsonFile.close();
	return map;
}

void Satellites::setDataMap(const QVariantMap& map)
{
	int numReadOk = 0;
	QVariantList defaultHintColorMap;
	defaultHintColorMap << defaultHintColor[0] << defaultHintColor[1] << defaultHintColor[2];

	if (map.contains("hintColor"))
	{
		defaultHintColorMap = map.value("hintColor").toList();
		defaultHintColor.set(defaultHintColorMap.at(0).toDouble(), defaultHintColorMap.at(1).toDouble(), defaultHintColorMap.at(2).toDouble());
	}

	if (satelliteListModel)
		satelliteListModel->beginSatellitesChange();
	
	satellites.clear();
	groups.clear();
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
			groups.unite(sat->groups);
			numReadOk++;
		}
	}
	qSort(satellites);
	
	if (satelliteListModel)
		satelliteListModel->endSatellitesChange();
}

QVariantMap Satellites::createDataMap(void)
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

void Satellites::markLastUpdate()
{
	lastUpdate = QDateTime::currentDateTime();
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("Satellites/last_update",
	               lastUpdate.toString(Qt::ISODate));
}

QSet<QString> Satellites::getGroups() const
{
	return groups;
}

QStringList Satellites::getGroupIdList() const
{
	QStringList groupList(groups.values());
	groupList.sort();
	return groupList;
}

void Satellites::addGroup(const QString& groupId)
{
	if (groupId.isEmpty())
		return;
	groups.insert(groupId);
}

QHash<QString,QString> Satellites::getSatellites(const QString& group, Status vis)
{
	QHash<QString,QString> result;

	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized)
		{
			if ((group.isEmpty() || sat->groups.contains(group)) && ! result.contains(sat->id))
			{
				if (vis==Both ||
						(vis==Visible && sat->displayed) ||
						(vis==NotVisible && !sat->displayed) ||
						(vis==OrbitError && !sat->orbitValid) ||
						(vis==NewlyAdded && sat->isNew()))
					result.insert(sat->id, sat->name);
			}
		}
	}
	return result;
}

SatellitesListModel* Satellites::getSatellitesListModel()
{
	if (!satelliteListModel)
		satelliteListModel = new SatellitesListModel(&satellites, this);
	return satelliteListModel;
}

SatelliteP Satellites::getById(const QString& id)
{
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->id == id)
			return sat;
	}
	return SatelliteP();
}

QStringList Satellites::listAllIds()
{
	QStringList result;
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized)
			result.append(sat->id);
	}
	return result;
}

bool Satellites::add(const TleData& tleData)
{
	//TODO: Duplicates check!!! --BM
	
	// More validation?
	if (tleData.id.isEmpty() ||
	        tleData.name.isEmpty() ||
	        tleData.first.isEmpty() ||
	        tleData.second.isEmpty())
		return false;
	
	QVariantList hintColor;
	hintColor << defaultHintColor[0]
	          << defaultHintColor[1]
	          << defaultHintColor[2];
	
	QVariantMap satProperties;
	satProperties.insert("name", tleData.name);
	satProperties.insert("tle1", tleData.first);
	satProperties.insert("tle2", tleData.second);
	satProperties.insert("hintColor", hintColor);
	//TODO: Decide if newly added satellites are visible by default --BM
	satProperties.insert("visible", true);
	satProperties.insert("orbitVisible", false);
	
	SatelliteP sat(new Satellite(tleData.id, satProperties));
	if (sat->initialized)
	{
		qDebug() << "Satellite added:" << tleData.id << tleData.name;
		satellites.append(sat);
		sat->setNew();
		return true;
	}
	return false;
}

void Satellites::add(const TleDataList& newSatellites)
{
	if (satelliteListModel)
		satelliteListModel->beginSatellitesChange();
	
	int numAdded = 0;
	foreach (const TleData& tleSet, newSatellites)
	{
		if (add(tleSet))
		{
			numAdded++;
		}
	}
	if (numAdded > 0)
		qSort(satellites);
	
	if (satelliteListModel)
		satelliteListModel->endSatellitesChange();
	
	qDebug() << "Satellites: "
					 << newSatellites.count() << "satellites proposed for addition, "
					 << numAdded << " added, "
					 << satellites.count() << " total after the operation.";
}

void Satellites::remove(const QStringList& idList)
{
	if (satelliteListModel)
		satelliteListModel->beginSatellitesChange();
	
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
	// As the satellite list is kept sorted, no need for re-sorting.
	
	if (satelliteListModel)
		satelliteListModel->endSatellitesChange();

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
	saveTleSources(updateUrls);
}

void Satellites::saveTleSources(const QStringList& urls)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// clear old source list
	conf->remove("tle_sources");

	int index = 0;
	conf->beginWriteArray("tle_sources");
	foreach (QString url, urls)
	{
		conf->setArrayIndex(index++);
		if (url.startsWith("1,"))
		{
			conf->setValue("add_new", true);
			url.remove(0, 2);
		}
		else if (url.startsWith("0,"))
			url.remove(0, 2);
		conf->setValue("url", url);
	}
	conf->endArray();

	conf->endGroup();
}

bool Satellites::getFlagLabels()
{
	return Satellite::showLabels;
}

void Satellites::enableInternetUpdates(bool enabled)
{
	if (enabled != updatesEnabled)
	{
		updatesEnabled = enabled;
		emit settingsChanged();
	}
}

void Satellites::enableAutoAdd(bool enabled)
{
	if (autoAddEnabled != enabled)
	{
		autoAddEnabled = enabled;
		emit settingsChanged();
	}
}

void Satellites::enableAutoRemove(bool enabled)
{
	if (autoRemoveEnabled != enabled)
	{
		autoRemoveEnabled = enabled;
		emit settingsChanged();
	}
}

void Satellites::setFlagHints(bool b)
{
	if (hintFader != b)
	{
		hintFader = b;
		emit settingsChanged();
	}
}

void Satellites::setFlagLabels(bool b)
{
	if (Satellite::showLabels != b)
	{
		Satellite::showLabels = b;
		emit settingsChanged();
	}
}

void Satellites::setLabelFontSize(int size)
{
	if (labelFont.pixelSize() != size)
	{
		labelFont.setPixelSize(size);
		emit settingsChanged();
	}
}

void Satellites::setUpdateFrequencyHours(int hours)
{
	if (updateFrequencyHours != hours)
	{
		updateFrequencyHours = hours;
		emit settingsChanged();
	}
}

void Satellites::checkForUpdate(void)
{
	if (updatesEnabled && updateState != Updating
	    && lastUpdate.addSecs(updateFrequencyHours * 3600) <= QDateTime::currentDateTime())
		updateFromOnlineSources();
}

void Satellites::updateFromOnlineSources()
{
	if (updateState==Satellites::Updating)
	{
		qWarning() << "Satellites: Internet update already in progress!";
		return;
	}
	else
	{
		qDebug() << "Satellites: starting Internet update...";
	}

	// Setting lastUpdate should be done only when the update is finished. -BM

	// TODO: Perhaps tie the emptyness of updateUrls to updatesEnabled... --BM
	if (updateUrls.isEmpty())
	{
		qWarning() << "Satellites: update failed."
		           << "No update sources are defined!";
		
		// Prevent from re-entering this method on the next check:
		markLastUpdate();
		// TODO: Do something saner, such as disabling internet updates,
		// or stopping the timer. --BM
		emit updateStateChanged(OtherError);
		emit tleUpdateComplete(0, satellites.count(), 0, 0);
		return;
	}

	updateState = Satellites::Updating;
	emit(updateStateChanged(updateState));
	updateSources.clear();
	numberDownloadsComplete = 0;

	if (progressBar==NULL)
		progressBar = StelApp::getInstance().getGui()->addProgressBar();

	progressBar->setValue(0);
	progressBar->setMaximum(updateUrls.size());
	progressBar->setVisible(true);
	progressBar->setFormat("TLE download %v/%m");

	foreach (QString url, updateUrls)
	{
		TleSource source;
		source.file = 0;
		source.addNew = false;
		if (url.startsWith("1,"))
		{
			// Also prevents inconsistent behavior if the user toggles the flag
			// while an update is in progress.
			source.addNew = autoAddEnabled;
			url.remove(0, 2);
		}
		else if (url.startsWith("0,"))
			url.remove(0, 2);
		
		source.url.setUrl(url);
		if (source.url.isValid())
		{
			updateSources.append(source);
			downloadMgr->get(QNetworkRequest(source.url));
		}
	}
}

void Satellites::saveDownloadedUpdate(QNetworkReply* reply)
{
	// check the download worked, and save the data to file if this is the case.
	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Satellites: FAILED to download"
		           << reply->url().toString(QUrl::RemoveUserInfo)
		           << "Error:" << reply->errorString();
	}
	else
	{
		// download completed successfully.
		QString name = QString("tle%1.txt").arg(numberDownloadsComplete);
		QString path = dataDir.absoluteFilePath(name);
		// QFile as a child object to the plugin to ease memory management
		QFile* tmpFile = new QFile(path, this);
		if (tmpFile->exists())
			tmpFile->remove();
		
		if (tmpFile->open(QIODevice::WriteOnly | QIODevice::Text))
		{
			tmpFile->write(reply->readAll());
			tmpFile->close();
			
			// The reply URL can be different form the requested one...
			QUrl url = reply->request().url();
			for (int i = 0; i < updateSources.count(); i++)
			{
				if (updateSources[i].url == url)
				{
					updateSources[i].file = tmpFile;
					tmpFile = 0;
					break;
				}
			}
			if (tmpFile) // Something strange just happened...
				delete tmpFile; // ...so we have to clean.
		}
		else
		{
			qWarning() << "Satellites: cannot save update file:"
			           << tmpFile->error()
			           << tmpFile->errorString();
		}
	}
	numberDownloadsComplete++;
	if (progressBar)
		progressBar->setValue(numberDownloadsComplete);

	// Check if all files have been downloaded.
	// TODO: It's better to keep track of the network requests themselves. --BM 
	if (numberDownloadsComplete < updateSources.size())
		return;
	
	if (progressBar)
	{
		delete progressBar;
		progressBar = 0;
	}
	
	// All files have been downloaded, finish the update
	TleDataHash newData;
	for (int i = 0; i < updateSources.count(); i++)
	{
		if (!updateSources[i].file)
			continue;
		if (updateSources[i].file->open(QFile::ReadOnly|QFile::Text))
		{
			parseTleFile(*updateSources[i].file,
			             newData,
			             updateSources[i].addNew);
			updateSources[i].file->close();
			delete updateSources[i].file;
			updateSources[i].file = 0;
		}
	}	
	updateSources.clear();
	updateSatellites(newData);
}

void Satellites::updateObserverLocation(StelLocation)
{
	recalculateOrbitLines();
}

void Satellites::setOrbitLinesFlag(bool b)
{
	Satellite::orbitLinesFlag = b;
}

bool Satellites::getOrbitLinesFlag()
{
	return Satellite::orbitLinesFlag;
}

void Satellites::recalculateOrbitLines(void)
{
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->displayed && sat->orbitDisplayed)
			sat->recalculateOrbitLines();
	}
}

void Satellites::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor);
	messageTimer->start();
}

void Satellites::hideMessages()
{
	foreach(const int& id, messageIDs)
	{
		GETSTELMODULE(LabelMgr)->deleteLabel(id);
	}
}

void Satellites::saveCatalog(QString path)
{
	saveDataMap(createDataMap(), path);
}

void Satellites::updateFromFiles(QStringList paths, bool deleteFiles)
{
	// Container for the new data.
	TleDataHash newTleSets;
	foreach(const QString& tleFilePath, paths)
	{
		QFile tleFile(tleFilePath);
		if (tleFile.open(QIODevice::ReadOnly|QIODevice::Text))
		{
			parseTleFile(tleFile, newTleSets);
			tleFile.close();

			if (deleteFiles)
				tleFile.remove();
		}
	}

	updateSatellites(newTleSets);
}

void Satellites::updateSatellites(TleDataHash& newTleSets)
{
	// Save the update time.
	// One of the reasons it's here is that lastUpdate is used below.
	markLastUpdate();
	
	if (newTleSets.isEmpty())
	{
		qWarning() << "Satellites: update files contain no TLE sets!";
		updateState = OtherError;
		emit(updateStateChanged(updateState));
		return;
	}
	
	if (satelliteListModel)
		satelliteListModel->beginSatellitesChange();
	
	// Right, we should now have a map of all the elements we downloaded.  For each satellite
	// which this module is managing, see if it exists with an updated element, and update it if so...
	int sourceCount = newTleSets.count(); // newTleSets is modified below
	int updatedCount = 0;
	int totalCount = 0;
	int addedCount = 0;
	int missingCount = 0; // Also the number of removed sats, if any.
	QStringList toBeRemoved;
	foreach(const SatelliteP& sat, satellites)
	{
		totalCount++;
		
		// Satellites marked as "user-defined" are protected from updates and
		// removal.
		if (sat->userDefined)
		{
			qDebug() << "Satellite ignored (user-protected):"
			         << sat->id << sat->name;
			continue;
		}
		
		QString id = sat->id;
		TleData newTle = newTleSets.take(id);
		if (!newTle.name.isEmpty())
		{
			if (sat->tleElements.first != newTle.first ||
			    sat->tleElements.second != newTle.second ||
			    sat->name != newTle.name)
			{
				// We have updated TLE elements for this satellite
				sat->setNewTleElements(newTle.first, newTle.second);
				
				// Update the name if it has been changed in the source list
				sat->name = newTle.name;

				// we reset this to "now" when we started the update.
				sat->lastUpdated = lastUpdate;
				updatedCount++;
			}
		}
		else
		{
			if (autoRemoveEnabled)
				toBeRemoved.append(sat->id);
			else
				qWarning() << "Satellites:" << sat->id << sat->name
				           << "is missing in the update lists.";
			missingCount++;
		}
	}
	
	// Only those not in the loaded collection have remained
	// (autoAddEnabled is not checked, because it's already in the flags)
	QHash<QString, TleData>::const_iterator i;
	for (i = newTleSets.begin(); i != newTleSets.end(); ++i)
	{
		if (i.value().addThis)
		{
			// Add the satellite...
			if (add(i.value()))
				addedCount++;
		}
	}
	if (addedCount)
		qSort(satellites);
	
	if (autoRemoveEnabled && !toBeRemoved.isEmpty())
	{
		qWarning() << "Satellites: purging objects that were not updated...";
		remove(toBeRemoved);
	}
	
	if (updatedCount > 0 ||
	        (autoRemoveEnabled && missingCount > 0))
	{
		saveDataMap(createDataMap());
		updateState = CompleteUpdates;
	}
	else
		updateState = CompleteNoUpdates;
	
	if (satelliteListModel)
		satelliteListModel->endSatellitesChange();

	qDebug() << "Satellites: update finished."
	         << updatedCount << "/" << totalCount << "updated,"
	         << addedCount << "added,"
	         << missingCount << "missing or removed."
	         << sourceCount << "source entries parsed.";

	emit(updateStateChanged(updateState));
	emit(tleUpdateComplete(updatedCount, totalCount, addedCount, missingCount));
}

void Satellites::parseTleFile(QFile& openFile,
                              TleDataHash& tleList,
                              bool addFlagValue)
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
			lastData.addThis = addFlagValue;
			
			//TODO: We need to think of some kind of ecaping these
			//characters in the JSON parser. --BM
			// The thing in square brackets after the name is actually
			// Celestrak's "status code". Parse automatically? --BM
			line.replace(QRegExp("\\s*\\[([^\\]])*\\]\\s*$"),"");  // remove things in square brackets
			lastData.name = line;
		}
		else
		{
			// TODO: Yet another place suitable for a standard TLE regex. --BM
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
					// Some satellites can be listed in multiple files,
					// and only some of those files may be marked for adding,
					// so try to preserve the flag - if it's set,
					// feel free to overwrite the existing value.
					// If not, overwrite only if it's not in the list already.
					// NOTE: Second case overwrite may need to check which TLE set is newer. 
					if (lastData.addThis || !tleList.contains(id))
						tleList.insert(id, lastData); // Overwrite if necessary
				}
				//TODO: Error warnings? --BM
			}
			else
				qDebug() << "Satellites: unprocessed line " << lineNumber <<  " in file " << QDir::toNativeSeparators(openFile.fileName());
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
		if (sat->initialized && sat->displayed)
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
		if (sat && sat->initialized && sat->displayed)
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

void Satellites::translations()
{
#if 0
	// Satellite groups
	// TRANSLATORS: Satellite group: Bright/naked-eye-visible satellites
	N_("visual");
	// TRANSLATORS: Satellite group: Scientific satellites
	N_("scientific");
	// TRANSLATORS: Satellite group: Communication satellites
	N_("communications");
	// TRANSLATORS: Satellite group: Navigation satellites
	N_("navigation");
	// TRANSLATORS: Satellite group: Amateur radio (ham) satellites
	N_("amateur");
	// TRANSLATORS: Satellite group: Weather (meteorological) satellites
	N_("weather");
	// TRANSLATORS: Satellite group: Satellites in geostationary orbit
	N_("geostationary");
	// TRANSLATORS: Satellite group: Satellites that are no longer functioning
	N_("non-operational");
	// TRANSLATORS: Satellite group: Satellites belonging to the GPS constellation (the Global Positioning System)
	N_("gps");
	// TRANSLATORS: Satellite group: Satellites belonging to the Iridium constellation (Iridium is a proper name)
	N_("iridium");
	
	/* For copy/paste:
	// TRANSLATORS: Satellite group: 
	N_("");
	*/
	
	
	// Satellite descriptions - bright and/or famous objects
	// Just A FEW objects please! (I'm looking at you, Alex!)
	// TRANSLATORS: Satellite description. "Hubble" is a person's name.
	N_("The Hubble Space Telescope");
	// TRANSLATORS: Satellite description.
	N_("The International Space Station");
#endif
}
