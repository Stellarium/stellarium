/*
 * Copyright (C) 2009, 2012 Matthew Gates
 * Copyright (C) 2015 Nick Fedoseev (Iridium flares)
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
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocation.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelTextureMgr.hpp"
#include "Satellites.hpp"
#include "Satellite.hpp"
#include "SatellitesListModel.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StelJsonParser.hpp"
#include "SatellitesDialog.hpp"
#include "LabelMgr.hpp"
#include "StelTranslator.hpp"
#include "StelProgressController.hpp"
#include "StelUtils.hpp"
#include "StelActionMgr.hpp"

#include <private/qzipreader_p.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QTimer>
#include <QVariantMap>
#include <QVariant>
#include <QDir>
#include <QTemporaryFile>
#include <QRegularExpression>

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
	info.contact = STELLARIUM_DEV_URL;
	info.description = N_("Prediction of artificial satellite positions in Earth orbit based on NORAD TLE data");
	info.version = SATELLITES_PLUGIN_VERSION;
	info.license = SATELLITES_PLUGIN_LICENSE;
	return info;
}

// WARNING! Update also the version number in resources/satellites.json,
// otherwise the local copy of that file will be overwritten every time
// Stellarium starts. (Less of a problem if it manages to get one update.)
QString Satellites::SatellitesCatalogVersion = "0.12.0";

Satellites::Satellites()
	: satelliteListModel(nullptr)
	, toolbarButton(nullptr)
	, earth(nullptr)
	, defaultHintColor(0.7f, 0.7f, 0.7f)
	, updateState(CompleteNoUpdates)
	, downloadMgr(nullptr)
	, progressBar(nullptr)
	, numberDownloadsComplete(0)
	, updateTimer(nullptr)
	, updatesEnabled(false)
	, autoAddEnabled(false)
	, autoRemoveEnabled(false)
	, updateFrequencyHours(0)
	, flagUmbraVisible(false)
	, flagUmbraAtFixedDistance(false)
	, umbraColor(1.0f, 0.0f, 0.0f)
	, fixedUmbraDistance(1000.0)
	, flagPenumbraVisible(false)
	, penumbraColor(1.0f, 0.0f, 0.0f)
	, earthShadowEnlargementDanjon(false)
	, lastSelectedSatellite(QString())
	#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	, iridiumFlaresPredictionDepth(7)
	#endif
{
	setObjectName("Satellites");
	configDialog = new SatellitesDialog();
}

void Satellites::deinit()
{
	Satellite::hintTexture.clear();
	texPointer.clear();
	texCross.clear();
}

Satellites::~Satellites()
{
	delete configDialog;
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

		// load standard magnitudes and RCS data for satellites
		loadExtraData();

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Satellites"))
		{
			//qDebug() << "Satellites: created section in config file.";
			restoreDefaultSettings();
		}

		// populate settings from main config file.
		loadSettings();

		// absolute file name for inner catalogue of the satellites
		catalogPath = dataDir.absoluteFilePath("satellites.json");

		// Load and find resources used in the plugin
		texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur5.png");
		texCross = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/cross.png");
		Satellite::hintTexture = StelApp::getInstance().getTextureManager().createTexture(":/satellites/hint.png");

		// key bindings and other actions		
		QString satGroup = N_("Satellites");
		addAction("actionShow_Satellite_Hints", satGroup, N_("Artificial satellites"), "flagHintsVisible", "Ctrl+Z");
		addAction("actionShow_Satellite_Labels", satGroup, N_("Satellite labels"), "flagLabelsVisible", "Alt+Shift+Z");
		addAction("actionShow_Satellite_ConfigDialog_Global", satGroup, N_("Show settings dialog"), configDialog, "visible", "Alt+Z");

		// Gui toolbar button
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		if (gui)
		{
			toolbarButton = new StelButton(nullptr,
						       QPixmap(":/satellites/bt_satellites_on.png"),
						       QPixmap(":/satellites/bt_satellites_off.png"),
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       "actionShow_Satellite_Hints",
						       false,
						       "actionShow_Satellite_ConfigDialog_Global");
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "[Satellites] init error: " << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo::exists(catalogPath))
	{
		if (!checkJsonFileFormat() || readCatalogVersion() != SatellitesCatalogVersion)
		{
			displayMessage(q_("The old satellites.json file is no longer compatible - using default file"), "#bb0000");
			restoreDefaultCatalog();
		}
	}
	else
	{
		qDebug() << "[Satellites] satellites.json does not exist - copying default file to " << QDir::toNativeSeparators(catalogPath);
		restoreDefaultCatalog();
	}

	qDebug() << "[Satellites] loading catalogue file:" << QDir::toNativeSeparators(catalogPath);

	// create satellites according to content of satellites.json file
	loadCatalog();
	// create list of "supergroups" for satellites
	createSuperGroupsList();

	// Set up download manager and the update schedule
	downloadMgr = new QNetworkAccessManager(this);
	connect(downloadMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(saveDownloadedUpdate(QNetworkReply*)));
	updateState = CompleteNoUpdates;
	updateTimer = new QTimer(this);
	updateTimer->setSingleShot(false);   // recurring check for update
	updateTimer->setInterval(13000);     // check once every 13 seconds to see if it is time for an update
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
	updateTimer->start();

	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	earth = ssystem->getEarth();
	sun = ssystem->getSun();
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	// Handle changes to the observer location or wide range of dates:
	StelCore* core = StelApp::getInstance().getCore();
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(updateObserverLocation(StelLocation)));
	connect(core, SIGNAL(configurationDataSaved()), this, SLOT(saveSettings()));
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(translateData()));
	connect(ssystem, SIGNAL(earthShadowEnlargementDanjonChanged(bool)), this, SLOT(updateEarthShadowEnlargementFlag(bool)));

	connect(this, SIGNAL(satSelectionChanged(QString)), this, SLOT(changeSelectedSatellite(QString)));

	bindingGroups();
}

void Satellites::translateData()
{
	bindingGroups();
	for (const auto& sat : qAsConst(satellites))
	{
		if (sat->initialized)
			sat->recomputeSatData();
	}
}

void Satellites::updateSatellitesVisibility()
{
	if (getFlagHintsVisible())
		setFlagHintsVisible(false);
}

void Satellites::bindingGroups()
{
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	QStringList groups = getGroupIdList();
	QString satGroup = N_("Satellites");
	QString showSatGroup = q_("Show satellites from the group");
	QString hideSatGroup = q_("Hide satellites from the group");
	for (auto constIterator = groups.constBegin(); constIterator != groups.constEnd(); ++constIterator)
	{
		QString groupId = (*constIterator).toLocal8Bit().constData();
		QString actionShowName = QString("actionShow_Satellite_Group_%1").arg(groupId);
		QString actionShowDescription = QString("%1 \"%2\"").arg(showSatGroup, q_(groupId));
		StelAction* actionShow = actionMgr->findAction(actionShowName);
		if (actionShow)
			actionMgr->findAction(actionShowName)->setText(actionShowDescription);
		else
			addAction(actionShowName, satGroup, actionShowDescription, this, [=](){setSatGroupVisible(groupId, true);});

		QString actionHideName = QString("actionHide_Satellite_Group_%1").arg(groupId);
		QString actionHideDescription = QString("%1 \"%2\"").arg(hideSatGroup, q_(groupId));
		StelAction* actionHide = actionMgr->findAction(actionHideName);
		if (actionHide)
			actionMgr->findAction(actionHideName)->setText(actionHideDescription);
		else
			addAction(actionHideName, satGroup, actionHideDescription, this, [=](){setSatGroupVisible(groupId, false);});
	}
}

void Satellites::setSatGroupVisible(const QString& groupId, bool visible)
{
	for (const auto& sat : qAsConst(satellites))
	{
		if (sat->initialized && sat->groups.contains(groupId))
		{
			SatFlags flags = sat->getFlags();
			visible ? flags |= SatDisplayed : flags &= ~SatDisplayed;
			sat->setFlags(flags);
		}
	}
	emit satGroupVisibleChanged();
}

bool Satellites::backupCatalog(bool deleteOriginal)
{
	QFile old(catalogPath);
	if (!old.exists())
	{
		qWarning() << "[Satellites] no file to backup";
		return false;
	}

	QString backupPath = catalogPath + ".old";
	if (QFileInfo::exists(backupPath))
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "[Satellites] WARNING: unable to remove old catalogue file!";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "[Satellites] WARNING: failed to back up catalogue file as"
			   << QDir::toNativeSeparators(backupPath);
		return false;
	}

	return true;
}

double Satellites::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+1.;
	return 0;
}

QList<StelObjectP> Satellites::searchAround(const Vec3d& av, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;
	if (!hintFader)
		return result;

	if (qAbs(core->getTimeRate())>=Satellite::timeRateLimit) // Do not show satellites when time rate is over limit
		return result;

	if (core->getCurrentPlanet()!=earth || !isValidRangeDates(core))
		return result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	for (const auto& sat : satellites)
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
	if (!hintFader)
		return nullptr;

	StelCore* core = StelApp::getInstance().getCore();

	if (qAbs(core->getTimeRate())>=Satellite::timeRateLimit) // Do not show satellites when time rate is over limit
		return nullptr;

	if (core->getCurrentPlanet()!=earth || !isValidRangeDates(core))
		return nullptr;
	
	QString objw = nameI18n.toUpper();
	
	StelObjectP result = searchByNoradNumber(objw);
	if (result)
		return result;

	result = searchByInternationalDesignator(objw);
	if (result)
		return result;

	for (const auto& sat : satellites)
	{
		if (sat->initialized && sat->displayed)
		{
			if (sat->getNameI18n().toUpper() == objw)
				return qSharedPointerCast<StelObject>(sat);
		}
	}

	return nullptr;
}

StelObjectP Satellites::searchByName(const QString& englishName) const
{
	if (!hintFader)
		return nullptr;

	StelCore* core = StelApp::getInstance().getCore();

	if (qAbs(core->getTimeRate())>=Satellite::timeRateLimit) // Do not show satellites when time rate is over limit
		return nullptr;

	if (core->getCurrentPlanet()!=earth || !isValidRangeDates(core))
		return nullptr;

	QString objw = englishName.toUpper();
	
	StelObjectP result = searchByNoradNumber(objw);
	if (result)
		return result;
	
	result = searchByInternationalDesignator(objw);
	if (result)
		return result;

	for (const auto& sat : satellites)
	{
		if (sat->initialized && sat->displayed)
		{
			if (sat->getEnglishName().toUpper() == objw)
				return qSharedPointerCast<StelObject>(sat);
		}
	}

	return nullptr;
}

StelObjectP Satellites::searchByID(const QString &id) const
{
	for (const auto& sat : satellites)
	{
		if (sat->initialized && sat->getID() == id)
		{
			return qSharedPointerCast<StelObject>(sat);
		}
	}

	return nullptr;
}

StelObjectP Satellites::searchByNoradNumber(const QString &noradNumber) const
{
	if (!hintFader)
		return nullptr;

	StelCore* core = StelApp::getInstance().getCore();

	if (qAbs(core->getTimeRate())>=Satellite::timeRateLimit) // Do not show satellites when time rate is over limit
		return nullptr;

	if (core->getCurrentPlanet()!=earth || !isValidRangeDates(core))
		return nullptr;
	
	// If the search string is a catalogue number...
	static const QRegularExpression regExp("^(NORAD)\\s*(\\d+)\\s*$");
	QRegularExpressionMatch match=regExp.match(noradNumber);
	if (match.hasMatch())
	{
		QString numberString = match.captured(2);
		
		for (const auto& sat : satellites)
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

StelObjectP Satellites::searchByInternationalDesignator(const QString &intlDesignator) const
{
	if (!hintFader)
		return nullptr;

	StelCore* core = StelApp::getInstance().getCore();

	if (qAbs(core->getTimeRate())>=Satellite::timeRateLimit) // Do not show satellites when time rate is over limit
		return nullptr;

	if (core->getCurrentPlanet()!=earth || !isValidRangeDates(core))
		return nullptr;

	// If the search string is an international designator...
	static const QRegularExpression regExp("^(\\d+)-(\\w*)\\s*$");
	QRegularExpressionMatch match=regExp.match(intlDesignator);
	if (match.hasMatch())
	{
		QString designator = QString("%1-%2").arg(match.captured(1), match.captured(2));

		for (const auto& sat : satellites)
		{
			if (sat->initialized && sat->displayed)
			{
				if (sat->getInternationalDesignator() == designator)
					return qSharedPointerCast<StelObject>(sat);
			}
		}
	}

	return StelObjectP();
}


QStringList Satellites::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (!hintFader || maxNbItem <= 0)
		return result;

	StelCore* core = StelApp::getInstance().getCore();

	if (qAbs(core->getTimeRate())>=Satellite::timeRateLimit) // Do not show satellites when time rate is over limit
		return result;

	if (core->getCurrentPlanet()!=earth || !isValidRangeDates(core))
		return result;

	QString objw = objPrefix.toUpper();

	QString numberPrefix;
	static const QRegularExpression regExp("^(NORAD)\\s*(\\d+)\\s*$");
	QRegularExpressionMatch match=regExp.match(objw);
	if (match.hasMatch())
	{
		QString numberString = match.captured(2);
		bool ok;
		/* int number = */ numberString.toInt(&ok);
		if (ok)
			numberPrefix = numberString;
	}

	QString designatorPrefix;
	static const QRegularExpression regExp2("^(\\d+)-(\\w*)\\s*$");
	QRegularExpressionMatch match2=regExp2.match(objw);
	if (match2.hasMatch())
		designatorPrefix = QString("%1-%2").arg(match2.captured(1), match2.captured(2));

	QStringList names;
	for (const auto& sobj : satellites)
	{
		if (!sobj->initialized || !sobj->displayed)
			continue;

		names.append(sobj->getNameI18n());
		names.append(sobj->getEnglishName());
		if (!numberPrefix.isEmpty() && sobj->getCatalogNumberString().startsWith(numberPrefix))
			names.append(QString("NORAD %1").arg(sobj->getCatalogNumberString()));
		if (!designatorPrefix.isEmpty() && sobj->getInternationalDesignator().startsWith(designatorPrefix))
			names.append(sobj->getInternationalDesignator());
	}

	QString fullMatch = "";
	for (const auto& name : qAsConst(names))
	{
		if (!matchObjectName(name, objPrefix, useStartOfWords))
			continue;

		if (name==objPrefix)
			fullMatch = name;
		else
			result.append(name);

		if (result.size() >= maxNbItem)
			break;
	}

	result.sort();
	if (!fullMatch.isEmpty())
		result.prepend(fullMatch);

	return result;
}

QStringList Satellites::listAllObjects(bool inEnglish) const
{
	QStringList result;

	if (!hintFader)
		return result;

	StelCore* core = StelApp::getInstance().getCore();

	if (qAbs(core->getTimeRate())>=Satellite::timeRateLimit) // Do not show satellites when time rate is over limit
		return result;

	if (core->getCurrentPlanet()!=earth || !isValidRangeDates(core))
		return result;

	for (const auto& sat : satellites)
	{
		if (inEnglish)
			result << sat->getEnglishName();
		else
			result << sat->getNameI18n();
	}
	return result;
}

bool Satellites::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

void Satellites::restoreDefaults(void)
{
	restoreDefaultSettings();
	restoreDefaultTleSources();
	restoreDefaultCatalog();	
	loadCatalog();
	loadSettings();
}

void Satellites::restoreDefaultTleSources()
{
	// Format: group name, auto-add flag
	const QMap<QString, bool> celestrak = {
		{ "visual", true },      { "stations", true },      { "last-30-days", false }, { "active", false },
		{ "analyst", false },    { "science", true },       { "noaa", false },         { "goes", false },
		{ "amateur", true },     { "gnss", true },          { "gps-ops", true },       { "galileo", true },
		{ "iridium", false },    { "iridium-NEXT", false }, { "geo", false },          { "weather", false },
		{ "resource", false },   { "sarsat", false },       { "dmc", false },          { "tdrss", false },
		{ "argos", false },      { "intelsat", false },     { "gorizont", false },     { "raduga", false },
		{ "molniya", false },    { "orbcomm", false },      { "globalstar", false },   { "x-comm", false },
		{ "other-comm", false }, { "glo-ops", true },       { "beidou", true },        { "sbas", false },
		{ "nnss", false },       { "engineering", false },  { "education", false },    { "geodetic", false },
		{ "radar", false },      { "cubesat", false },      { "other", false },        { "oneweb", true },
		{ "starlink", true },    { "planet", false },       { "spire", false },        { "swarm", false }
	};
	// Details: https://celestrak.org/NORAD/documentation/gp-data-formats.php
	QString celestrackBaseURL = "https://celestrak.org/NORAD/elements/gp.php?GROUP=%1&FORMAT=TLE";
	QStringList urls;
	// TLE sources from Celestrak
	for (auto group = celestrak.begin(); group != celestrak.end(); ++group)
	{
		QString url = celestrackBaseURL.arg(group.key());
		if (group.value()) // Auto-add ON!
			urls << QString("1,%1").arg(url);
		else
			urls << url;
	}
	// Other sources and supplemental data from Celestrack
	urls << "1,https://celestrak.org/NORAD/elements/supplemental/starlink.txt"
	     << "https://www.amsat.org/amsat/ftp/keps/current/nasabare.txt"
	     << "https://www.prismnet.com/~mmccants/tles/classfd.zip";

	saveTleSources(urls);
}

void Satellites::restoreDefaultSettings()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// delete all existing Satellite settings...
	conf->remove("");

	conf->setValue("show_satellite_hints", true);
	conf->setValue("show_satellite_labels", false);
	conf->setValue("updates_enabled", true);
	conf->setValue("auto_add_enabled", true);
	conf->setValue("auto_remove_enabled", true);
	conf->setValue("hint_color", "0.7,0.7,0.7");
	conf->setValue("invisible_satellite_color", "0.2,0.2,0.2");
	conf->setValue("transit_satellite_color", "0.0,0.0,0.0");
	conf->setValue("hint_font_size", 10);
	conf->setValue("update_frequency_hours", 72);
	conf->setValue("orbit_line_flag", false);
	conf->setValue("orbit_line_segments", 90);
	conf->setValue("orbit_fade_segments", 5);
	conf->setValue("orbit_segment_duration", 20);
	conf->setValue("valid_epoch_age", 30);
	conf->setValue("iconic_mode_enabled", false);
	conf->setValue("hide_invisible_satellites", false);
	conf->setValue("colored_invisible_satellites", true);
	conf->setValue("umbra_flag", false);
	conf->setValue("umbra_fixed_distance_flag", false);
	conf->setValue("umbra_color", "1.0,0.0,0.0");
	conf->setValue("umbra_fixed_distance", 1000.0);
	conf->setValue("penumbra_flag", false);
	conf->setValue("penumbra_color", "1.0,0.0,0.0");
	conf->setValue("cf_magnitude_flag", false);
	conf->setValue("cf_apogee_flag", false);
	conf->setValue("cf_apogee_min", 20000.);
	conf->setValue("cf_apogee_max", 55000.);
	conf->setValue("cf_perigee_flag", false);
	conf->setValue("cf_perigee_min", 200.);
	conf->setValue("cf_perigee_max", 1500.);
	conf->setValue("cf_eccentricity_flag", false);
	conf->setValue("cf_eccentricity_min", 0.3);
	conf->setValue("cf_eccentricity_max", 0.9);
	conf->setValue("cf_period_flag", false);
	conf->setValue("cf_period_min", 0.);
	conf->setValue("cf_period_max", 150.);
	conf->setValue("cf_inclination_flag", false);
	conf->setValue("cf_inclination_min", 120.);
	conf->setValue("cf_inclination_max", 180.);
	conf->setValue("cf_rcs_flag", false);
	conf->setValue("cf_rcs_min", 0.1);
	conf->setValue("cf_rcs_max", 100.);
	conf->setValue("vf_altitude_flag", false);
	conf->setValue("vf_altitude_min", 200.);
	conf->setValue("vf_altitude_max", 1500.);
	
	conf->endGroup(); // saveTleSources() opens it for itself
}

void Satellites::restoreDefaultCatalog()
{
    if (QFileInfo::exists(catalogPath))
		backupCatalog(true);

	QFile src(":/satellites/satellites.json");
	if (!src.copy(catalogPath))
	{
		qWarning() << "[Satellites] cannot copy json resource to " + QDir::toNativeSeparators(catalogPath);
	}
	else
	{
		qDebug() << "[Satellites] copied default satellites.json to " << QDir::toNativeSeparators(catalogPath);
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writeable by the Stellarium process so that updates can be done.
		QFile dest(catalogPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is scheduled in a timely
		// manner
		StelApp::getInstance().getSettings()->remove("Satellites/last_update");
		lastUpdate = QDateTime::fromString("2015-05-01T12:00:00", Qt::ISODate);
	}
}

void Satellites::loadSettings()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// Load update sources list...
	updateUrls.clear();
	
	// Backward compatibility: try to detect and read an old-style array.
	// TODO: Assume that the user hasn't modified their conf in a stupid way?
//	if (conf->contains("tle_url0")) // This can skip some operations...
	static const QRegularExpression keyRE("^tle_url\\d+$");
	QStringList urls;
	for (const auto& key : conf->childKeys())
	{
		if (keyRE.match(key).hasMatch())
		{
			QString url = conf->value(key).toString();
			conf->remove(key); // Delete old-style keys
			if (url.isEmpty())
				continue;

			// celestrak.com moved to celestrak.org
			if (url.contains("celestrak.com", Qt::CaseInsensitive))
				url.replace("celestrak.com", "celestrak.org", Qt::CaseInsensitive);

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
				// celestrak.com moved to celestrak.org
				if (url.contains("celestrak.com", Qt::CaseInsensitive))
					url.replace("celestrak.com", "celestrak.org", Qt::CaseInsensitive);

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
	// last update default is the first Towel Day.  <3 DA
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2001-05-25T12:00:00").toString(), Qt::ISODate);
	setFlagHintsVisible(conf->value("show_satellite_hints", true).toBool());
	Satellite::showLabels = conf->value("show_satellite_labels", false).toBool();
	updatesEnabled = conf->value("updates_enabled", true).toBool();
	autoAddEnabled = conf->value("auto_add_enabled", true).toBool();
	autoRemoveEnabled = conf->value("auto_remove_enabled", true).toBool();
#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	iridiumFlaresPredictionDepth = conf->value("flares_prediction_depth", 7).toInt();
#endif

	// Get a font for labels
	labelFont.setPixelSize(conf->value("hint_font_size", 10).toInt());

	// orbit drawing params
	Satellite::orbitLinesFlag = conf->value("orbit_line_flag", false).toBool();
	Satellite::orbitLineSegments = conf->value("orbit_line_segments", 180).toInt();
	Satellite::orbitLineFadeSegments = conf->value("orbit_fade_segments", 5).toInt();
	Satellite::orbitLineSegmentDuration = conf->value("orbit_segment_duration", 5).toInt();
	Satellite::orbitLineThickness = conf->value("orbit_line_thickness", 1).toInt();
	setInvisibleSatelliteColor(Vec3f(conf->value("invisible_satellite_color", "0.2,0.2,0.2").toString()));
	setTransitSatelliteColor(Vec3f(conf->value("transit_satellite_color", "0.0,0.0,0.0").toString()));
	Satellite::timeRateLimit = conf->value("time_rate_limit", 1.0).toDouble();
	Satellite::tleEpochAge = conf->value("valid_epoch_age", 30).toInt();

	// umbra/penumbra
	setFlagUmbraVisible(conf->value("umbra_flag", false).toBool());
	// FIXME: Repair the functionality!
	setFlagUmbraAtFixedDistance(false);
	//setFlagUmbraAtFixedDistance(conf->value("umbra_fixed_distance_flag", false).toBool());
	setUmbraColor(Vec3f(conf->value("umbra_color", "1.0,0.0,0.0").toString()));
	setUmbraDistance(conf->value("umbra_fixed_distance", 1000.0).toDouble());
	setFlagPenumbraVisible(conf->value("penumbra_flag", false).toBool());
	setPenumbraColor(Vec3f(conf->value("penumbra_color", "1.0,0.0,0.0").toString()));

	// iconic mode
	setFlagIconicMode(conf->value("iconic_mode_enabled", false).toBool());
	setFlagHideInvisible(conf->value("hide_invisible_satellites", false).toBool());
	setFlagColoredInvisible(conf->value("colored_invisible_satellites", true).toBool());

	// custom filter
	setFlagCFKnownStdMagnitude(conf->value("cf_magnitude_flag", false).toBool());
	setFlagCFApogee(conf->value("cf_apogee_flag", false).toBool());
	setMinCFApogee(conf->value("cf_apogee_min", 20000.).toDouble());
	setMaxCFApogee(conf->value("cf_apogee_max", 55000.).toDouble());
	setFlagCFPerigee(conf->value("cf_perigee_flag", false).toBool());
	setMinCFPerigee(conf->value("cf_perigee_min", 200.).toDouble());
	setMaxCFPerigee(conf->value("cf_perigee_max", 1500.).toDouble());
	setFlagCFEccentricity(conf->value("cf_eccentricity_flag", false).toBool());
	setMinCFEccentricity(conf->value("cf_eccentricity_min", 0.3).toDouble());
	setMaxCFEccentricity(conf->value("cf_eccentricity_max", 0.9).toDouble());
	setFlagCFPeriod(conf->value("cf_period_flag", false).toBool());
	setMinCFPeriod(conf->value("cf_period_min", 0.).toDouble());
	setMaxCFPeriod(conf->value("cf_period_max", 150.).toDouble());
	setFlagCFInclination(conf->value("cf_inclination_flag", false).toBool());
	setMinCFInclination(conf->value("cf_inclination_min", 120.).toDouble());
	setMaxCFInclination(conf->value("cf_inclination_max", 180.).toDouble());
	setFlagCFRCS(conf->value("cf_rcs_flag", false).toBool());
	setMinCFRCS(conf->value("cf_rcs_min", 0.1).toDouble());
	setMaxCFRCS(conf->value("cf_rcs_max", 100.).toDouble());

	// visual filter
	setFlagVFAltitude(conf->value("vf_altitude_flag", false).toBool());
	setMinVFAltitude(conf->value("vf_altitude_min", 200.).toDouble());
	setMaxVFAltitude(conf->value("vf_altitude_max", 500.).toDouble());
	setFlagVFMagnitude(conf->value("vf_magnitude_flag", false).toBool());
	setMinVFMagnitude(conf->value("vf_magnitude_min", 8.).toDouble());
	setMaxVFMagnitude(conf->value("vf_magnitude_max", -8.).toDouble());

	// custom filter
	setFlagCFKnownStdMagnitude(conf->value("cf_magnitude_flag", false).toBool());
	setFlagCFApogee(conf->value("cf_apogee_flag", false).toBool());
	setMinCFApogee(conf->value("cf_apogee_min", 20000.).toDouble());
	setMaxCFApogee(conf->value("cf_apogee_max", 55000.).toDouble());
	setFlagCFPerigee(conf->value("cf_perigee_flag", false).toBool());
	setMinCFPerigee(conf->value("cf_perigee_min", 200.).toDouble());
	setMaxCFPerigee(conf->value("cf_perigee_max", 1500.).toDouble());
	setFlagCFEccentricity(conf->value("cf_eccentricity_flag", false).toBool());
	setMinCFEccentricity(conf->value("cf_eccentricity_min", 0.3).toDouble());
	setMaxCFEccentricity(conf->value("cf_eccentricity_max", 0.9).toDouble());
	setFlagCFPeriod(conf->value("cf_period_flag", false).toBool());
	setMinCFPeriod(conf->value("cf_period_min", 0.).toDouble());
	setMaxCFPeriod(conf->value("cf_period_max", 150.).toDouble());
	setFlagCFInclination(conf->value("cf_inclination_flag", false).toBool());
	setMinCFInclination(conf->value("cf_inclination_min", 120.).toDouble());
	setMaxCFInclination(conf->value("cf_inclination_max", 180.).toDouble());
	setFlagCFRCS(conf->value("cf_rcs_flag", false).toBool());
	setMinCFRCS(conf->value("cf_rcs_min", 0.1).toDouble());
	setMaxCFRCS(conf->value("cf_rcs_max", 100.).toDouble());

	conf->endGroup();
}

void Satellites::saveSettingsToConfig()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// updater related settings...
	conf->setValue("update_frequency_hours", updateFrequencyHours);
	conf->setValue("show_satellite_hints", getFlagHintsVisible());
	conf->setValue("show_satellite_labels", Satellite::showLabels);
	conf->setValue("updates_enabled", updatesEnabled );
	conf->setValue("auto_add_enabled", autoAddEnabled);
	conf->setValue("auto_remove_enabled", autoRemoveEnabled);
#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	conf->setValue("flares_prediction_depth", iridiumFlaresPredictionDepth);
#endif

	// Get a font for labels
	conf->setValue("hint_font_size", labelFont.pixelSize());

	// orbit drawing params
	conf->setValue("orbit_line_flag", Satellite::orbitLinesFlag);
	conf->setValue("orbit_line_segments", Satellite::orbitLineSegments);
	conf->setValue("orbit_fade_segments", Satellite::orbitLineFadeSegments);
	conf->setValue("orbit_segment_duration", Satellite::orbitLineSegmentDuration);
	conf->setValue("orbit_line_thickness", Satellite::orbitLineThickness);

	conf->setValue("valid_epoch_age", Satellite::tleEpochAge);

	// umbra/penumbra
	conf->setValue("umbra_flag", getFlagUmbraVisible());
	// FIXME: Fix and re-enable this function
	//conf->setValue("umbra_fixed_distance_flag", getFlagUmbraAtFixedDistance());
	conf->setValue("umbra_fixed_distance_flag", false); // Preliminary
	conf->setValue("umbra_color", getUmbraColor().toStr());
	conf->setValue("umbra_fixed_distance", getUmbraDistance());
	conf->setValue("penumbra_flag", getFlagPenumbraVisible());
	conf->setValue("penumbra_color", getPenumbraColor().toStr());

	// iconic mode
	conf->setValue("iconic_mode_enabled", getFlagIconicMode());
	conf->setValue("hide_invisible_satellites", getFlagHideInvisible());
	conf->setValue("colored_invisible_satellites", getFlagColoredInvisible());

	// custom filter
	conf->setValue("cf_magnitude_flag", getFlagCFKnownStdMagnitude());
	conf->setValue("cf_apogee_flag", getFlagCFApogee());
	conf->setValue("cf_apogee_min", getMinCFApogee());
	conf->setValue("cf_apogee_max", getMaxCFApogee());
	conf->setValue("cf_perigee_flag", getFlagCFPerigee());
	conf->setValue("cf_perigee_min", getMinCFPerigee());
	conf->setValue("cf_perigee_max", getMaxCFPerigee());
	conf->setValue("cf_eccentricity_flag", getFlagCFEccentricity());
	conf->setValue("cf_eccentricity_min", getMinCFEccentricity());
	conf->setValue("cf_eccentricity_max", getMaxCFEccentricity());
	conf->setValue("cf_period_flag", getFlagCFPeriod());
	conf->setValue("cf_period_min", getMinCFPeriod());
	conf->setValue("cf_period_max", getMaxCFPeriod());
	conf->setValue("cf_inclination_flag", getFlagCFInclination());
	conf->setValue("cf_inclination_min", getMinCFInclination());
	conf->setValue("cf_inclination_max", getMaxCFInclination());
	conf->setValue("cf_rcs_flag", getFlagCFRCS());
	conf->setValue("cf_rcs_min", getMinCFRCS());
	conf->setValue("cf_rcs_max", getMaxCFRCS());

	// visual filter
	conf->setValue("vf_altitude_flag", getFlagVFAltitude());
	conf->setValue("vf_altitude_min", getMinVFAltitude());
	conf->setValue("vf_altitude_max", getMaxVFAltitude());
	conf->setValue("vf_magnitude_flag", getFlagVFMagnitude());
	conf->setValue("vf_magnitude_min", getMinVFMagnitude());
	conf->setValue("vf_magnitude_max", getMaxVFMagnitude());

	// custom filter
	conf->setValue("cf_magnitude_flag", getFlagCFKnownStdMagnitude());
	conf->setValue("cf_apogee_flag", getFlagCFApogee());
	conf->setValue("cf_apogee_min", getMinCFApogee());
	conf->setValue("cf_apogee_max", getMaxCFApogee());
	conf->setValue("cf_perigee_flag", getFlagCFPerigee());
	conf->setValue("cf_perigee_min", getMinCFPerigee());
	conf->setValue("cf_perigee_max", getMaxCFPerigee());
	conf->setValue("cf_eccentricity_flag", getFlagCFEccentricity());
	conf->setValue("cf_eccentricity_min", getMinCFEccentricity());
	conf->setValue("cf_eccentricity_max", getMaxCFEccentricity());
	conf->setValue("cf_period_flag", getFlagCFPeriod());
	conf->setValue("cf_period_min", getMinCFPeriod());
	conf->setValue("cf_period_max", getMaxCFPeriod());
	conf->setValue("cf_inclination_flag", getFlagCFInclination());
	conf->setValue("cf_inclination_min", getMinCFInclination());
	conf->setValue("cf_inclination_max", getMaxCFInclination());
	conf->setValue("cf_rcs_flag", getFlagCFRCS());
	conf->setValue("cf_rcs_min", getMinCFRCS());
	conf->setValue("cf_rcs_max", getMaxCFRCS());

	conf->endGroup();
	
	// Update sources...
	saveTleSources(updateUrls);
}

Vec3f Satellites::getInvisibleSatelliteColor()
{
	return Satellite::invisibleSatelliteColor;
}

void Satellites::setInvisibleSatelliteColor(const Vec3f &c)
{
	Satellite::invisibleSatelliteColor = c;
	emit invisibleSatelliteColorChanged(c);
}

Vec3f Satellites::getTransitSatelliteColor()
{
	return Satellite::transitSatelliteColor;
}

void Satellites::setTransitSatelliteColor(const Vec3f &c)
{
	Satellite::transitSatelliteColor = c;
	emit transitSatelliteColorChanged(c);
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
		qWarning() << "[Satellites] cannot open " << QDir::toNativeSeparators(catalogPath);
		return jsonVersion;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&satelliteJsonFile).toMap();
		satelliteJsonFile.close();
	}
	catch (std::runtime_error &e)
	{
		qDebug() << "[Satellites] File format is wrong! Error: " << e.what();
		return jsonVersion;
	}

	if (map.contains("version"))
	{
		QString version = map.value("version").toString();
		static const QRegularExpression vRx("(\\d+\\.\\d+\\.\\d+)");
		QRegularExpressionMatch match=vRx.match(version);
		if (match.hasMatch())
			jsonVersion = match.captured(1);
	}
	else if (map.contains("creator"))
	{
		QString creator = map.value("creator").toString();
		static const QRegularExpression vRx(".*(\\d+\\.\\d+\\.\\d+).*");
		QRegularExpressionMatch match=vRx.match(creator);
		if (match.hasMatch())
			jsonVersion = match.captured(1);
	}

	//qDebug() << "[Satellites] catalogue version from file:" << jsonVersion;
	return jsonVersion;
}

bool Satellites::saveDataMap(const QVariantMap& map, QString path)
{
	if (path.isEmpty())
		path = catalogPath;

	QFile jsonFile(path);

	if (jsonFile.exists())
		jsonFile.remove();

	if (!jsonFile.open(QIODevice::WriteOnly))
	{
		qWarning() << "[Satellites] cannot open for writing:" << QDir::toNativeSeparators(path);
		return false;
	}
	else
	{
		//qDebug() << "[Satellites] writing to:" << QDir::toNativeSeparators(path);
		StelJsonParser::write(map, &jsonFile);
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
		qWarning() << "[Satellites] cannot open " << QDir::toNativeSeparators(path);
	else
	{
		try
		{
			map = StelJsonParser::parse(&jsonFile).toMap();
			jsonFile.close();
		}
		catch (std::runtime_error &e)
		{
			qDebug() << "[Satellites] File format is wrong! Error: " << e.what();
			return QVariantMap();
		}
	}
	return map;
}

void Satellites::setDataMap(const QVariantMap& map)
{
	QVariantList defaultHintColorMap;
	defaultHintColorMap << defaultHintColor[0] << defaultHintColor[1] << defaultHintColor[2];

	if (map.contains("hintColor"))
	{
		defaultHintColorMap = map.value("hintColor").toList();
		defaultHintColor.set(defaultHintColorMap.at(0).toFloat(), defaultHintColorMap.at(1).toFloat(), defaultHintColorMap.at(2).toFloat());
	}

	if (satelliteListModel)
		satelliteListModel->beginSatellitesChange();
	
	satellites.clear();
	groups.clear();
	QVariantMap satMap = map.value("satellites").toMap();
	QMap<QString, QVariant>::const_iterator smit=satMap.constBegin();
	while (smit != satMap.constEnd())
	{
		QVariantMap satData = smit.value().toMap();

		if (!satData.contains("hintColor"))
			satData["hintColor"] = defaultHintColorMap;

		if (!satData.contains("orbitColor"))
			satData["orbitColor"] = satData["hintColor"];

		if (!satData.contains("infoColor"))
			satData["infoColor"] = satData["hintColor"];

		int sid = smit.key().toInt();
		if (!satData.contains("stdMag") && qsMagList.contains(sid))
			satData["stdMag"] = qsMagList[sid];

		if (!satData.contains("rcs") && rcsList.contains(sid))
			satData["rcs"] = rcsList[sid];

		SatelliteP sat(new Satellite(smit.key(), satData));
		if (sat->initialized)
		{
			satellites.append(sat);
			groups.unite(sat->groups);			
		}
		smit++;
	}
	std::sort(satellites.begin(), satellites.end());
	
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

	map["creator"] = QString("Satellites plugin version %1").arg(SATELLITES_PLUGIN_VERSION);
	map["version"] = QString("%1").arg(SatellitesCatalogVersion);
	map["hintColor"] = defHintCol;
	map["shortName"] = "satellite orbital data";
	QVariantMap sats;
	for (const auto& sat : qAsConst(satellites))
	{
		QVariantMap satMap = sat->getMap();

		if (satMap["orbitColor"] == satMap["hintColor"])
			satMap.remove("orbitColor");

		if (satMap["hintColor"].toList() == defHintCol)
			satMap.remove("hintColor");

		if (satMap["infoColor"].toList() == defHintCol)
			satMap.remove("infoColor");

		if (satMap["stdMag"].toFloat() > 98.f)
			satMap.remove("stdMag");

		if (satMap["rcs"].toFloat() < 0.f)
			satMap.remove("rcs");

		if (satMap["status"].toInt() == Satellite::StatusUnknown)
			satMap.remove("status");

		sats[sat->id] = satMap;		
	}
	map["satellites"] = sats;
	return map;
}

void Satellites::markLastUpdate()
{
	lastUpdate = QDateTime::currentDateTime();
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("Satellites/last_update", lastUpdate.toString(Qt::ISODate));
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

QHash<QString,QString> Satellites::getSatellites(const QString& group, Status vis) const
{
	QHash<QString,QString> result;

	for (const auto& sat : satellites)
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

SatelliteP Satellites::getById(const QString& id) const
{
	for (const auto& sat : satellites)
	{
		if (sat->initialized && sat->id == id)
			return sat;
	}
	return SatelliteP();
}

QStringList Satellites::listAllIds() const
{
	QStringList result;
	for (const auto& sat : satellites)
	{
		if (sat->initialized)
			result.append(sat->id);
	}
	return result;
}

bool Satellites::add(const TleData& tleData)
{
	// More validation?
	if (tleData.id.isEmpty() || tleData.name.isEmpty() || tleData.first.isEmpty() || tleData.second.isEmpty())
		return false;

	// Duplicates check
	if (searchByID(getSatIdFromLine2(tleData.second.trimmed()))!=nullptr)
		return false;

	QVariantList hintColor;
	hintColor << defaultHintColor[0] << defaultHintColor[1] << defaultHintColor[2];
	
	QVariantMap satProperties;
	satProperties.insert("name", tleData.name);
	satProperties.insert("tle1", tleData.first);
	satProperties.insert("tle2", tleData.second);
	satProperties.insert("hintColor", hintColor);	
	//TODO: Decide if newly added satellites are visible by default --BM
	satProperties.insert("visible", true);
	satProperties.insert("orbitVisible", false);

	QPair<double, double> stdMagRCS = getStdMagRCS(tleData);
	if (stdMagRCS.first < 99.)
		satProperties.insert("stdMag", stdMagRCS.first);
	if (stdMagRCS.second > 0.)
		satProperties.insert("rcs", stdMagRCS.second);

	if (tleData.status != Satellite::StatusUnknown)
		satProperties.insert("status", tleData.status);

	// Add the description for newly added satellites	
	QString description = getSatelliteDescription(tleData.id.toInt());
	if (!satProperties.contains("description") && !description.isEmpty())
		satProperties.insert("description", description);

	// Guessing the groups for newly added satellites only
	QVariantList groupList =  satProperties.value("groups", QVariantList()).toList();
	if (groupList.isEmpty())
	{
		QStringList satGroups = guessGroups(tleData);
		if (!satGroups.isEmpty())
		{
			satProperties.insert("groups", satGroups);
			for (const auto& str : qAsConst(satGroups))
			{
				if (!getGroupIdList().contains(str))
					addGroup(str);
			}
		}
	}

	QList<CommLink> comms = getCommunicationData(tleData);
	if (!comms.isEmpty())
	{
		QVariantList communications;
		for (const auto& comm : comms)
		{
			QVariantMap commData;
			commData.insert("frequency", comm.frequency);
			commData.insert("description", comm.description);
			if (!comm.modulation.isEmpty()) // can be empty
				commData.insert("modulation", comm.modulation);

			communications.append(commData);
		}
		satProperties.insert("comms", communications);
	}
	
	SatelliteP sat(new Satellite(tleData.id, satProperties));
	if (sat->initialized)
	{
		qDebug() << "[Satellites] satellite added:" << tleData.id << tleData.name;
		satellites.append(sat);
		sat->setNew();		
		return true;
	}
	return false;
}

QString Satellites::getSatelliteDescription(int satID)
{
	// Format: NORAD (SATCAT) ID, description
	const QMap<int, QString> descriptions = {
		// TRANSLATORS: Satellite description. "Hubble" is a person's name.
		{ 20580, N_("The Hubble Space Telescope") },
		// TRANSLATORS: Satellite description.
		{ 25544, N_("The International Space Station") },
		// TRANSLATORS: Satellite description.
		{ 25867, N_("The Chandra X-ray Observatory") },
		// TRANSLATORS: Satellite description.
		{ 25989, N_("X-ray Multi-Mirror Mission") },
		// TRANSLATORS: Satellite description.
		{ 27370, N_("Reuven Ramaty High Energy Solar Spectroscopic Imager") },
		// TRANSLATORS: Satellite description.
		{ 27540, N_("International Gamma-Ray Astrophysics Laboratory") },
		// TRANSLATORS: Satellite description.
		{ 27843, N_("The Microvariability and Oscillations of Stars telescope") },
		// TRANSLATORS: Satellite description.
		{ 28485, N_("Neil Gehrels Swift Observatory (Swift Gamma-Ray Burst Explorer)") },
		// TRANSLATORS: Satellite description.
		{ 31135, N_("Astro‐Rivelatore Gamma a Immagini Leggero") },
		// TRANSLATORS: Satellite description.
		{ 33053, N_("Fermi Gamma-ray Space Telescope") },
		// TRANSLATORS: Satellite description.
		{ 36119, N_("Wide-field Infrared Survey Explorer") },
		// TRANSLATORS: Satellite description.
		{ 36577, N_("Interplanetary Kite-craft Accelerated by Radiation Of the Sun") },
		// TRANSLATORS: Satellite description.
		{ 37755, N_("The russian space radio telescope RadioAstron") },
		// TRANSLATORS: Satellite description.
		{ 38358, N_("Nuclear Spectroscopic Telescope Array") },
		// TRANSLATORS: Satellite description.
		{ 39089, N_("Near Earth Object Surveillance Satellite") },
		// TRANSLATORS: Satellite description.
		{ 39197, N_("The Interface Region Imaging Spectrograph") },
		// TRANSLATORS: Satellite description.
		{ 39253, N_("The Spectroscopic Planet Observatory for Recognition of Interaction of Atmosphere") },
		// TRANSLATORS: Satellite description.
		{ 41173, N_("The Dark Matter Particle Explorer") },
		// TRANSLATORS: Satellite description.
		{ 42758, N_("Hard X-ray Modulation Telescope") },
		// TRANSLATORS: Satellite description.
		{ 43020, N_("Arcsecond Space Telescope Enabling Research in Astrophysics") },
		// TRANSLATORS: Satellite description.
		{ 43435, N_("Transiting Exoplanet Survey Satellite") },
		// TRANSLATORS: Satellite description.
		{ 44432, N_("Spectrum-X-Gamma") },
		// TRANSLATORS: Satellite description.
		{ 44874, N_("Characterising Exoplanets Satellite") },
		// TRANSLATORS: Satellite description.
		{ 48274, N_("Tiangong space station (Chinese large modular space station)") },
		// TRANSLATORS: Satellite description.
		{ 49954, N_("Imaging X-ray Polarimetry Explorer") },
		// TRANSLATORS: Satellite description. "James Webb" is a person's name.
		{ 50463, N_("James Webb Space Telescope") }
	};
	return descriptions.value(satID, QString());
}

QPair<double, double> Satellites::getStdMagRCS(const TleData& tleData)
{
	QPair<double, double> result;
	double stdMag = 99., RCS = -1.;
	int sid = tleData.id.toInt();
	if (qsMagList.contains(sid))
		stdMag = qsMagList[sid];
	if (rcsList.contains(sid))
		RCS = rcsList[sid];

	// special case: chinese space station / CSS (TIANHE1)
	if (sid==48274 && stdMag > 90.)
		stdMag = 0.87; // see details: http://www.satobs.org/seesat/Aug-2022/0030.html

	// special case: starlink satellites; details: http://satobs.org/seesat/Apr-2020/0174.html
	if (!rcsList.contains(sid) && tleData.name.startsWith("STARLINK"))
	{
		RCS = 22.68; // Starlink's solar array is 8.1 x 2.8 metres.
		// Source: Anthony Mallama. Starlink Satellite Brightness -- Characterized From 100,000 Visible Light Magnitudes; https://arxiv.org/abs/2111.09735
		if (tleData.name.contains("VISORSAT", Qt::CaseInsensitive))
			stdMag = 7.21; // stdMag=6.84 previously: https://arxiv.org/abs/2109.07345
		else
			stdMag = 5.89;
	}

	// special case: oneweb satellites
	// Source: Anthony Mallama. OneWeb Satellite Brightness -- Characterized From 80,000 Visible Light Magnitudes; https://arxiv.org/pdf/2203.05513.pdf
	if (!qsMagList.contains(sid) && tleData.name.startsWith("ONEWEB"))
		stdMag = 7.05;

	result.first	= stdMag;
	result.second	= RCS;
	return result;
}

QList<CommLink> Satellites::getCommunicationData(const QString &id)
{
	QList<CommLink> comms;

	for (const auto& sat : qAsConst(satellites))
	{
		if (sat->initialized && sat->getID() == id)
			comms = sat->comms;
	}

	return comms;
}

QList<CommLink> Satellites::getCommunicationData(const TleData& tleData)
{
	QList<CommLink> comms;

	// Communication data for individual satellites
	QVariantMap communications = satComms.value(tleData.id.toInt(), QVariantMap());
	if (!communications.isEmpty())
	{
		for (const auto& comm : communications.value("comms").toList())
		{
			CommLink c;
			QVariantMap commMap = comm.toMap();
			c.frequency = commMap.value("frequency").toDouble();
			c.description = commMap.value("description").toString();
			if (commMap.contains("modulation")) // can be empty
				c.modulation = commMap.value("modulation").toString();
			comms.append(c);
		}
	}

	// Communication data for groups of satellites
	const QMap<QString, QString> startsWith = {
		{ "GPS",	"gps" },
		{ "BEIDOU",	"beidou" },
		{ "IRNSS",	"irnss" },
		{ "ORBCOMM",	"orbcomm" },
		{ "TEVEL",	"tevel" },
		{ "QZS",	"qzss" },
		{ "FORMOSAT",	"formosat" },
		{ "FOSSASAT",	"fossasat"},
		{ "NETSAT",	"netsat" },
		{ "GONETS-M",	"gonets" },
		{ "SOYUZ-MS",	"soyuz-ms" },
		{ "PROGRESS-MS","progress-ms" },
		{ "IRIDIUM",	"iridium" },
		{ "STARLINK",	"starlink" },
		{ "NOAA",	"noaa" },
		{ "METEOR 1",	"meteor-1" },
		{ "METEOR 3",	"meteor-2" },
		{ "METEOR 2",	"meteor-3" },
		{ "GLOBALSTAR",	"globalstar" },
		{ "COSMO-SKYMED", "cosmo-skymed" }
	};

	QStringList groups;
	for (auto& satgr: startsWith.keys())
	{
		if (tleData.name.startsWith(satgr))
			groups << startsWith.value(satgr);
	}

	if (tleData.name.startsWith("COSMOS") && tleData.name.contains("("))
		groups << "glonass";

	if (tleData.name.startsWith("GSAT") && (tleData.name.contains("PRN") || tleData.name.contains("GALILEO")))
		groups << "galileo";

	for (const auto& name : qAsConst(groups))
	{
		communications.clear();
		communications = groupComms.value(name, QVariantMap());
		if (!communications.isEmpty())
		{
			for (const auto& comm : communications.value("comms").toList())
			{
				CommLink c;
				QVariantMap commMap = comm.toMap();
				c.frequency = commMap.value("frequency").toDouble();
				c.description = commMap.value("description").toString();
				if (commMap.contains("modulation")) // can be empty
					c.modulation = commMap.value("modulation").toString();
				comms.append(c);
			}
		}
	}

	return comms;
}

QStringList Satellites::guessGroups(const TleData& tleData)
{
	QStringList satGroups;
	// Special case: ISS
	if (tleData.id == "25544" || tleData.id == "49044")
	{
		satGroups.append("stations");
		satGroups.append("scientific");
		satGroups.append("amateur");
		satGroups.append("visual");
		satGroups.append("tdrss");
	}

	// Guessing the groups from the names of satellites
	if (tleData.name.startsWith("STARLINK"))
	{
		satGroups.append("starlink");
		satGroups.append("communications");
	}
	if (tleData.name.startsWith("IRIDIUM"))
	{
		QStringList d = tleData.name.split(" ");
		if (d.at(1).toInt()>=100)
			satGroups.append("iridium-NEXT");
		else
			satGroups.append("iridium");
		satGroups.append("communications");
	}
	if (tleData.name.startsWith("FLOCK") || tleData.name.startsWith("SKYSAT"))
		satGroups.append("earth resources");
	if (tleData.name.startsWith("ONEWEB"))
	{
		satGroups.append("oneweb");
		satGroups.append("communications");
	}
	if (tleData.name.startsWith("LEMUR"))
	{
		satGroups.append("spire");
		satGroups.append("earth resources");
	}
	if (tleData.name.startsWith("GPS"))
	{
		satGroups.append("gps");
		satGroups.append("gnss");
		satGroups.append("navigation");
	}
	if (tleData.name.startsWith("IRNSS"))
	{
		satGroups.append("irnss");
		satGroups.append("navigation");
	}
	if (tleData.name.startsWith("QZS"))
	{
		satGroups.append("qzss");
	}
	if (tleData.name.startsWith("TDRS"))
	{
		satGroups.append("tdrss");
		satGroups.append("communications");
		satGroups.append("geostationary");
	}
	if (tleData.name.startsWith("BEIDOU"))
	{
		satGroups.append("beidou");
		satGroups.append("gnss");
		satGroups.append("navigation");
	}
	if (tleData.name.startsWith("COSMOS"))
	{
		satGroups.append("cosmos");
		if (tleData.name.contains("("))
		{
			satGroups.append("glonass");
			satGroups.append("gnss");
			satGroups.append("navigation");
		}
	}
	if (tleData.name.startsWith("GSAT") && (tleData.name.contains("PRN") || tleData.name.contains("GALILEO")))
	{
		satGroups.append("galileo");
		satGroups.append("gnss");
		satGroups.append("navigation");
	}
	if (tleData.name.startsWith("GONETS-M") || tleData.name.startsWith("INTELSAT") || tleData.name.startsWith("GLOBALSTAR") || tleData.name.startsWith("ORBCOMM") || tleData.name.startsWith("GORIZONT") || tleData.name.startsWith("RADUGA") || tleData.name.startsWith("MOLNIYA") || tleData.name.startsWith("DIRECTV") || tleData.name.startsWith("CHINASAT") || tleData.name.startsWith("YAMAL"))
	{
		QString satName = tleData.name.split(" ").at(0).toLower();
		if (satName.contains("-"))
			satName = satName.split("-").at(0);
		satGroups.append(satName);
		satGroups.append("communications");
		if (satName.startsWith("INTELSAT") || satName.startsWith("RADUGA") || satName.startsWith("GORIZONT") || satName.startsWith("DIRECTV") || satName.startsWith("CHINASAT") || satName.startsWith("YAMAL"))
			satGroups.append("geostationary");
		if (satName.startsWith("INTELSAT") || satName.startsWith("DIRECTV") || satName.startsWith("YAMAL"))
			satGroups.append("tv");
	}
	if (tleData.name.contains(" DEB"))
		satGroups.append("debris");
	if (tleData.name.startsWith("SOYUZ-MS"))
		satGroups.append("crewed");
	if (tleData.name.startsWith("PROGRESS-MS") || tleData.name.startsWith("CYGNUS NG"))
		satGroups.append("resupply");
	if (tleData.status==Satellite::StatusNonoperational)
		satGroups.append("non-operational");

	// Guessing the groups from CelesTrak's groups (a "supergroups")
	if (tleData.sourceURL.contains("celestrak.org", Qt::CaseInsensitive))
	{
		// add groups, based on CelesTrak's groups
		QString groupName;
		if (tleData.sourceURL.contains(".txt", Qt::CaseInsensitive))
			groupName = QUrl(tleData.sourceURL).fileName().toLower().replace(".txt", "");
		else
		{
			// New format of source: https://celestrak.org/NORAD/documentation/gp-data-formats.php
			groupName = QUrl(tleData.sourceURL).query().toLower().split("&").filter("group=").join("").replace("group=", "");
		}
		groupName.remove("gp.php");
		if (!satGroups.contains(groupName) && !groupName.isEmpty())
			satGroups.append(groupName);

		// add "supergroups", based on CelesTrak's groups
		QStringList superGroup = satSuperGroupsMap.values(groupName);
		if (!superGroup.isEmpty())
		{
			for (int i=0; i<superGroup.size(); i++)
			{
				QString groupId = superGroup.at(i).toLocal8Bit().constData();
				if (!satGroups.contains(groupId))
					satGroups.append(groupId);
			}
		}
	}

	return satGroups;
}

void Satellites::add(const TleDataList& newSatellites)
{
	if (satelliteListModel)
		satelliteListModel->beginSatellitesChange();
	
	int numAdded = 0;
	for (const auto& tleSet : newSatellites)
	{
		if (add(tleSet))
		{
			numAdded++;
		}
	}
	if (numAdded > 0)
		std::sort(satellites.begin(), satellites.end());
	
	if (satelliteListModel)
		satelliteListModel->endSatellitesChange();
	
	qDebug() << "[Satellites] "
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

	qDebug() << "[Satellites] "
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
	for (auto url : urls)
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

bool Satellites::getFlagLabelsVisible()
{
	return Satellite::showLabels;
}

void Satellites::setUpdatesEnabled(bool enabled)
{
	if (enabled != updatesEnabled)
	{
		updatesEnabled = enabled;
		emit settingsChanged();
		emit updatesEnabledChanged(enabled);
	}
}

void Satellites::setAutoAddEnabled(bool enabled)
{
	if (autoAddEnabled != enabled)
	{
		autoAddEnabled = enabled;
		emit autoAddEnabledChanged(enabled);
		emit settingsChanged();
	}
}

void Satellites::setAutoRemoveEnabled(bool enabled)
{
	if (autoRemoveEnabled != enabled)
	{
		autoRemoveEnabled = enabled;
		emit autoRemoveEnabledChanged(enabled);
		emit settingsChanged();
	}
}

bool Satellites::getFlagIconicMode()
{
	return Satellite::iconicModeFlag;
}

void Satellites::setFlagIconicMode(bool b)
{
	if (Satellite::iconicModeFlag != b)
	{
		Satellite::iconicModeFlag = b;
		emit settingsChanged();
		emit flagIconicModeChanged(b);
	}
}

bool Satellites::getFlagHideInvisible()
{
	return Satellite::hideInvisibleSatellitesFlag;
}

void Satellites::setFlagHideInvisible(bool b)
{
	if (Satellite::hideInvisibleSatellitesFlag != b)
	{
		Satellite::hideInvisibleSatellitesFlag = b;
		emit settingsChanged();
		emit flagHideInvisibleChanged(b);
	}
}

bool Satellites::getFlagColoredInvisible()
{
	return Satellite::coloredInvisibleSatellitesFlag;
}

void Satellites::setFlagColoredInvisible(bool b)
{
	if (Satellite::coloredInvisibleSatellitesFlag != b)
	{
		Satellite::coloredInvisibleSatellitesFlag = b;
		emit settingsChanged();
		emit flagColoredInvisibleChanged(b);
	}
}

void Satellites::setFlagHintsVisible(bool b)
{
	if (hintFader != b)
	{
		hintFader = b;
		emit settingsChanged(); // GZ IS THIS REQUIRED/USEFUL??
		emit flagHintsVisibleChanged(b);
		emit StelApp::getInstance().getCore()->updateSearchLists();
	}
}

void Satellites::setFlagLabelsVisible(bool b)
{
	if (Satellite::showLabels != b)
	{
		Satellite::showLabels = b;
		emit settingsChanged(); // GZ IS THIS REQUIRED/USEFUL??
		emit flagLabelsVisibleChanged(b);
	}
}

void Satellites::setFlagUmbraVisible(bool b)
{
	if (flagUmbraVisible != b)
	{
		flagUmbraVisible = b;
		emit settingsChanged(); // GZ IS THIS REQUIRED/USEFUL??
		emit flagUmbraVisibleChanged(b);
	}
}

void Satellites::setFlagUmbraAtFixedDistance(bool b)
{
	flagUmbraAtFixedDistance = false;
	qDebug() << "setFlagUmbraAtFixedDistance() currently not possible";

	// TO BE FIXED
	//if (flagUmbraAtFixedDistance != b)
	//{
	//	flagUmbraAtFixedDistance = b;
	//	emit settingsChanged(); // GZ IS THIS REQUIRED/USEFUL??
	//	emit flagUmbraAtFixedDistanceChanged(b);
	//}
}

void Satellites::setUmbraColor(const Vec3f &c)
{
	umbraColor = c;
	emit umbraColorChanged(c);
}

void Satellites::setUmbraDistance(double d)
{
	fixedUmbraDistance = d;
	emit umbraDistanceChanged(d);
}

void Satellites::setFlagPenumbraVisible(bool b)
{
	if (flagPenumbraVisible != b)
	{
		flagPenumbraVisible = b;
		emit settingsChanged(); // GZ IS THIS REQUIRED/USEFUL??
		emit flagPenumbraVisibleChanged(b);
	}
}

void Satellites::setPenumbraColor(const Vec3f &c)
{
	penumbraColor = c;
	emit penumbraColorChanged(c);
}

void Satellites::setFlagCFKnownStdMagnitude(bool b)
{
	if (Satellite::flagCFKnownStdMagnitude != b)
	{
		Satellite::flagCFKnownStdMagnitude = b;
		emit customFilterChanged();
		emit flagCFKnownStdMagnitudeChanged(b);
	}
}

void Satellites::setFlagCFApogee(bool b)
{
	if (Satellite::flagCFApogee != b)
	{
		Satellite::flagCFApogee = b;
		emit customFilterChanged();
		emit flagCFApogeeChanged(b);
	}
}

void Satellites::setMinCFApogee(double v)
{
	Satellite::minCFApogee = v;
	emit customFilterChanged();
	emit minCFApogeeChanged(v);
}

void Satellites::setMaxCFApogee(double v)
{
	Satellite::maxCFApogee = v;
	emit customFilterChanged();
	emit maxCFApogeeChanged(v);
}

void Satellites::setFlagCFPerigee(bool b)
{
	if (Satellite::flagCFPerigee != b)
	{
		Satellite::flagCFPerigee = b;
		emit customFilterChanged();
		emit flagCFPerigeeChanged(b);
	}
}

void Satellites::setMinCFPerigee(double v)
{
	Satellite::minCFPerigee = v;
	emit customFilterChanged();
	emit minCFPerigeeChanged(v);
}

void Satellites::setMaxCFPerigee(double v)
{
	Satellite::maxCFPerigee = v;
	emit customFilterChanged();
	emit maxCFPerigeeChanged(v);
}

void Satellites::setFlagVFAltitude(bool b)
{
	if (Satellite::flagVFAltitude != b)
	{
		Satellite::flagVFAltitude = b;
		emit flagVFAltitudeChanged(b);
	}
}

void Satellites::setMinVFAltitude(double v)
{
	Satellite::minVFAltitude = v;
	emit minVFAltitudeChanged(v);
}

void Satellites::setMaxVFAltitude(double v)
{
	Satellite::maxVFAltitude = v;
	emit maxVFAltitudeChanged(v);
}

void Satellites::setFlagVFMagnitude(bool b)
{
	if (Satellite::flagVFMagnitude != b)
	{
		Satellite::flagVFMagnitude = b;
		emit flagVFMagnitudeChanged(b);
	}
}

void Satellites::setMinVFMagnitude(double v)
{
	Satellite::minVFMagnitude = v;
	emit minVFMagnitudeChanged(v);
}

void Satellites::setMaxVFMagnitude(double v)
{
	Satellite::maxVFMagnitude = v;
	emit maxVFMagnitudeChanged(v);
}

void Satellites::setFlagCFEccentricity(bool b)
{
	if (Satellite::flagCFEccentricity != b)
	{
		Satellite::flagCFEccentricity = b;
		emit customFilterChanged();
		emit flagCFEccentricityChanged(b);
	}
}

void Satellites::setMinCFEccentricity(double v)
{
	Satellite::minCFEccentricity = v;
	emit customFilterChanged();
	emit minCFEccentricityChanged(v);
}

void Satellites::setMaxCFEccentricity(double v)
{
	Satellite::maxCFEccentricity = v;
	emit customFilterChanged();
	emit maxCFEccentricityChanged(v);
}

void Satellites::setFlagCFPeriod(bool b)
{
	if (Satellite::flagCFPeriod != b)
	{
		Satellite::flagCFPeriod = b;
		emit customFilterChanged();
		emit flagCFPeriodChanged(b);
	}
}

void Satellites::setMinCFPeriod(double v)
{
	Satellite::minCFPeriod = v;
	emit customFilterChanged();
	emit minCFPeriodChanged(v);
}

void Satellites::setMaxCFPeriod(double v)
{
	Satellite::maxCFPeriod = v;
	emit customFilterChanged();
	emit maxCFPeriodChanged(v);
}

void Satellites::setFlagCFInclination(bool b)
{
	if (Satellite::flagCFInclination != b)
	{
		Satellite::flagCFInclination = b;
		emit customFilterChanged();
		emit flagCFInclinationChanged(b);
	}
}

void Satellites::setMinCFInclination(double v)
{
	Satellite::minCFInclination = v;
	emit customFilterChanged();
	emit minCFInclinationChanged(v);
}

void Satellites::setMaxCFInclination(double v)
{
	Satellite::maxCFInclination = v;
	emit customFilterChanged();
	emit maxCFInclinationChanged(v);
}

void Satellites::setFlagCFRCS(bool b)
{
	if (Satellite::flagCFRCS != b)
	{
		Satellite::flagCFRCS = b;
		emit customFilterChanged();
		emit flagCFRCSChanged(b);
	}
}

void Satellites::setMinCFRCS(double v)
{
	Satellite::minCFRCS = v;
	emit customFilterChanged();
	emit minCFRCSChanged(v);
}

void Satellites::setMaxCFRCS(double v)
{
	Satellite::maxCFRCS = v;
	emit customFilterChanged();
	emit maxCFRCSChanged(v);
}

void Satellites::setLabelFontSize(int size)
{
	if (labelFont.pixelSize() != size)
	{
		labelFont.setPixelSize(size);
		emit labelFontSizeChanged(size);
		emit settingsChanged();
	}
}

void Satellites::setOrbitLineSegments(int s)
{
	if (s != Satellite::orbitLineSegments)
	{
		Satellite::orbitLineSegments=s;
		emit orbitLineSegmentsChanged(s);
		recalculateOrbitLines();
	}
}

void Satellites::setOrbitLineFadeSegments(int s)
{
	if (s != Satellite::orbitLineFadeSegments)
	{
		Satellite::orbitLineFadeSegments=s;
		emit orbitLineFadeSegmentsChanged(s);
		recalculateOrbitLines();
	}
}

void Satellites::setOrbitLineThickness(int s)
{
	if (s != Satellite::orbitLineThickness)
	{
		Satellite::orbitLineThickness=s;
		emit orbitLineThicknessChanged(s);
		recalculateOrbitLines();
	}
}

void Satellites::setOrbitLineSegmentDuration(int s)
{
	if (s != Satellite::orbitLineSegmentDuration)
	{
		Satellite::orbitLineSegmentDuration=s;
		emit orbitLineSegmentDurationChanged(s);
		recalculateOrbitLines();
	}
}

void Satellites::setTleEpochAgeDays(int age)
{
	if (age != Satellite::tleEpochAge)
	{
		Satellite::tleEpochAge=age;
		emit tleEpochAgeDaysChanged(age);
	}
}

void Satellites::setUpdateFrequencyHours(int hours)
{
	if (updateFrequencyHours != hours)
	{
		updateFrequencyHours = hours;
		emit updateFrequencyHoursChanged(hours);
		emit settingsChanged();
	}
}

void Satellites::checkForUpdate(void)
{
	if (updatesEnabled && (updateState != Updating)
	    && (lastUpdate.addSecs(updateFrequencyHours * 3600) <= QDateTime::currentDateTime()))
	{
		updateFromOnlineSources();
	}
}

void Satellites::updateFromOnlineSources()
{
	// never update TLE's for any date before Oct 4, 1957, 19:28:34GMT ;-)
	if (StelApp::getInstance().getCore()->getJD()<2436116.3115)
		return;

	if (updateState==Satellites::Updating)
	{
		qWarning() << "[Satellites] Internet update already in progress!";
		return;
	}
	else
	{
		qDebug() << "[Satellites] starting Internet update...";
	}

	// Setting lastUpdate should be done only when the update is finished. -BM

	// TODO: Perhaps tie the emptiness of updateUrls to updatesEnabled... --BM
	if (updateUrls.isEmpty())
	{
		qWarning() << "[Satellites] update failed."
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
	emit updateStateChanged(updateState);
	updateSources.clear();
	numberDownloadsComplete = 0;

	if (progressBar==nullptr)
		progressBar = StelApp::getInstance().addProgressBar();

	progressBar->setValue(0);
	progressBar->setRange(0, updateUrls.size());
	// TRANSLATORS: The full phrase is 'Loading TLE %VALUE%/%MAX%' in progress bar
	progressBar->setFormat(QString("%1 %v/%m").arg(q_("Loading TLE")));

	for (auto url : qAsConst(updateUrls))
	{
		TleSource source;
		source.file = nullptr;
		source.addNew = false;
		if (url.startsWith("1,"))
		{
			// Also prevents inconsistent behaviour if the user toggles the flag
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
	if (reply->error() == QNetworkReply::NoError && reply->bytesAvailable()>0)
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
			QByteArray fd = reply->readAll();
			// qWarning() << "[Satellites] Processing an URL:" << reply->url().toString();
			if (reply->url().toString().contains(".zip", Qt::CaseInsensitive))
			{
				QTemporaryFile zip;
				if (zip.open())
				{
					// qWarning() << "[Satellites] Processing a ZIP archive...";
					zip.write(fd);
					zip.close();
					QString archive = zip.fileName();
					QByteArray data;

					QZipReader reader(archive);
					if (reader.status() != QZipReader::NoError)
						qWarning() << "[Satellites] Unable to open as a ZIP archive";
					else
					{
						for (const auto& info : reader.fileInfoList())
						{
							// qWarning() << "[Satellites] Processing:" << info.filePath;
							if (info.isFile)
								data.append(reader.fileData(info.filePath));
						}
						// qWarning() << "[Satellites] Extracted data:" << data;
						fd = data;
					}
					reader.close();
					zip.remove();
				}
				else
					qWarning() << "[Satellites] Unable to open a temporary file";
			}
			tmpFile->write(fd);
			tmpFile->close();
			
			// The reply URL can be different form the requested one...
			QUrl url = reply->request().url();
			for (int i = 0; i < updateSources.count(); i++)
			{
				if (updateSources[i].url == url)
				{
					updateSources[i].file = tmpFile;
					tmpFile = nullptr;
					break;
				}
			}
			if (tmpFile) // Something strange just happened...
				delete tmpFile; // ...so we have to clean.
		}
		else
		{
			qWarning() << "[Satellites] cannot save update file:"
			           << tmpFile->error()
			           << tmpFile->errorString();
		}
	}
	else
	{
		qWarning() << "[Satellites] FAILED to download" << reply->url().toString(QUrl::RemoveUserInfo) << "Error:" << reply->errorString();
		emit updateStateChanged(DownloadError);
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
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = nullptr;
	}

	// All files have been downloaded, finish the update
	TleDataHash newData;
	for (int i = 0; i < updateSources.count(); i++)
	{
		if (!updateSources[i].file)
			continue;
		if (updateSources[i].file->open(QFile::ReadOnly|QFile::Text))
		{
			parseTleFile(*updateSources[i].file, newData, updateSources[i].addNew, updateSources[i].url.toString(QUrl::None));
			updateSources[i].file->close();
			delete updateSources[i].file;
			updateSources[i].file = nullptr;
		}
	}
	updateSources.clear();
	if (!newData.isEmpty())
		updateSatellites(newData);
	else
		emit updateStateChanged(OtherError);
}

void Satellites::updateObserverLocation(const StelLocation &loc)
{
	Q_UNUSED(loc)
	recalculateOrbitLines();
}

void Satellites::setFlagOrbitLines(bool b)
{
	Satellite::orbitLinesFlag = b;
}

bool Satellites::getFlagOrbitLines()
{
	return Satellite::orbitLinesFlag;
}

void Satellites::recalculateOrbitLines(void)
{
	for (const auto& sat : qAsConst(satellites))
	{
		if (sat->initialized && sat->displayed && sat->orbitDisplayed)
			sat->recalculateOrbitLines();
	}
}

void Satellites::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor, false, 9000);
}


void Satellites::saveCatalog(QString path)
{
	saveDataMap(createDataMap(), path);
}

void Satellites::updateFromFiles(QStringList paths, bool deleteFiles)
{
	// Container for the new data.
	TleDataHash newTleSets;
	for (const auto& tleFilePath : paths)
	{
		QFile tleFile(tleFilePath);
		if (tleFile.open(QIODevice::ReadOnly|QIODevice::Text))
		{
			parseTleFile(tleFile, newTleSets, autoAddEnabled);
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
		qWarning() << "[Satellites] update files contain no TLE sets!";
		updateState = OtherError;
		emit updateStateChanged(updateState);
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
	for (const auto& sat : qAsConst(satellites))
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

				// Update operational status
				sat->status = newTle.status;

				// we reset this to "now" when we started the update.
				sat->lastUpdated = lastUpdate;

				QPair<double, double> stdMagRCS = getStdMagRCS(newTle);
				if (stdMagRCS.first < 99.)
					sat->stdMag = stdMagRCS.first;
				if (stdMagRCS.second > 0.)
					sat->RCS = stdMagRCS.second;

				QList<CommLink> comms = getCommunicationData(newTle);
				if (!comms.isEmpty())
					sat->comms = comms;

				updatedCount++;
			}

			if (sat->status==Satellite::StatusNonoperational && !sat->groups.contains("non-operational"))
				sat->groups.insert("non-operational");
			if (sat->status!=Satellite::StatusNonoperational && sat->groups.contains("non-operational"))
				sat->groups.remove("non-operational");
		}
		else
		{
			if (autoRemoveEnabled)
				toBeRemoved.append(sat->id);
			else
				qWarning() << "[Satellites]" << sat->id << sat->name
				           << "is missing in the update lists.";
			missingCount++;
		}
		// All satellites, who has invalid orbit should be removed.
		if (!sat->orbitValid)
		{
			toBeRemoved.append(sat->id);
			qWarning() << "[Satellites]" << sat->id << sat->name
				   << "has invalid orbit and will be removed.";
			missingCount++;
		}
	}
	
	// Only those not in the loaded collection have remained
	// (autoAddEnabled is not checked, because it's already in the flags)
	for (const auto& tleData : newTleSets)
	{
		if (tleData.addThis)
		{
			// Add the satellite...
			if (add(tleData))
				addedCount++;
		}
	}
	if (addedCount)
		std::sort(satellites.begin(), satellites.end());
	
	if (autoRemoveEnabled && !toBeRemoved.isEmpty())
	{
		qWarning() << "[Satellites] purging objects that were not updated...";
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

	qDebug() << "[Satellites] update finished."
	         << updatedCount << "/" << totalCount << "updated,"
	         << addedCount << "added,"
	         << missingCount << "missing or removed."
	         << sourceCount << "source entries parsed.";

	emit updateStateChanged(updateState);
	emit tleUpdateComplete(updatedCount, totalCount, addedCount, missingCount);
}

void Satellites::parseTleFile(QFile& openFile, TleDataHash& tleList, bool addFlagValue, const QString &tleURL)
{
	if (!openFile.isOpen() || !openFile.isReadable())
		return;
	
	// Code mostly re-used from updateFromFiles()
	int lineNumber = 0;
	TleData lastData;

	// Celestrak's "status code" list
	const QMap<QString, Satellite::OptStatus> satOpStatusMap = {
		{ "+", Satellite::StatusOperational },
		{ "-", Satellite::StatusNonoperational },
		{ "P", Satellite::StatusPartiallyOperational },
		{ "B", Satellite::StatusStandby },
		{ "S", Satellite::StatusSpare },
		{ "X", Satellite::StatusExtendedMission },
		{ "D", Satellite::StatusDecayed },
		{ "?", Satellite::StatusUnknown }
	};
	
	while (!openFile.atEnd())
	{
		QString line = QString(openFile.readLine()).trimmed();
		if (line.length() < 65) // this is title line
		{
			// New entry in the list, so reset all fields
			lastData = TleData();
			lastData.addThis = addFlagValue;
			lastData.sourceURL = tleURL;
			
			// The thing in square brackets after the name is actually
			// Celestrak's "status code". Parse it!
			static const QRegularExpression statusRx("\\s*\\[(\\D{1})\\]\\s*$", QRegularExpression::InvertedGreedinessOption );
			QRegularExpressionMatch match;
			if (line.indexOf(statusRx, 0, &match)>-1)
				lastData.status = satOpStatusMap.value(match.captured(1).toUpper(), Satellite::StatusUnknown);

			//TODO: We need to think of some kind of escaping these
			//characters in the JSON parser. --BM
			static const QRegularExpression statusCodeRx("\\s*\\[([^\\]])*\\]\\s*$");
			line.replace(statusCodeRx,"");  // remove "status code" from name
			lastData.name = line;
		}
		else
		{
			// TODO: Yet another place suitable for a standard TLE regex. --BM
			static const QRegularExpression reL1("^1 .*");
			static const QRegularExpression reL2("^2 .*");
			if (reL1.match(line).hasMatch())
				lastData.first = line;
			else if (reL2.match(line).hasMatch())
			{
				lastData.second = line;
				// The Satellite Catalogue Number is the second number
				// on the second line.
				QString id = getSatIdFromLine2(line);
				if (id.isEmpty())
				{
					qDebug() << "[Satellites] failed to extract SatId from \"" << line << "\"";
					continue;
				}
				lastData.id = id;
				
				// This is the second line and there will be no more,
				// so if everything is OK, save the elements.
				if (!lastData.name.isEmpty() && !lastData.first.isEmpty())
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
				qDebug() << "[Satellites] unprocessed line " << lineNumber <<  " in file " << QDir::toNativeSeparators(openFile.fileName());
		}
	}
}

QString Satellites::getSatIdFromLine2(const QString& line)
{
	#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
	QString id = line.split(' ',  Qt::SkipEmptyParts).at(1).trimmed();
	#else
	QString id = line.split(' ',  QString::SkipEmptyParts).at(1).trimmed();
	#endif
	if (!id.isEmpty())
	{
		// Strip any leading zeros as they should be unique ints as strings.
		static const QRegularExpression re("^[0]*\\B");
		id.remove(re);
	}
	return id;
}

void Satellites::loadExtraData()
{
	// Description of file and some additional information you can find here:
	// 1) http://www.prismnet.com/~mmccants/tles/mccdesc.html
	// 2) http://www.prismnet.com/~mmccants/tles/intrmagdef.html
	QFile qsmFile(":/satellites/qs.mag");	
	qsMagList.clear();	
	if (qsmFile.open(QFile::ReadOnly))
	{
		while (!qsmFile.atEnd())
		{
			QString line = QString(qsmFile.readLine());
			int id   = line.mid(0,5).trimmed().toInt();
			QString smag = line.mid(33,4).trimmed();
			if (!smag.isEmpty())
				qsMagList.insert(id, smag.toDouble());
		}
		qsmFile.close();
	}

	QFile rcsFile(":/satellites/rcs");
	rcsList.clear();
	if (rcsFile.open(QFile::ReadOnly))
	{
		while (!rcsFile.atEnd())
		{
			QString line = QString(rcsFile.readLine());
			int id   = line.mid(0,5).trimmed().toInt();
			QString srcs = line.mid(5,5).trimmed();
			if (!srcs.isEmpty())
				rcsList.insert(id, srcs.toDouble());
		}
		rcsFile.close();
	}

	QFile commFile(":/satellites/communications.json");
	if (commFile.open(QFile::ReadOnly))
	{
		satComms.clear();
		groupComms.clear();
		QVariantMap commMap = StelJsonParser::parse(&commFile).toMap();
		commFile.close();

		// Communications data for individual satellites
		QVariantMap satellitesCommLink = commMap.value("satellites").toMap();
		QMap<QString, QVariant>::const_iterator cit=satellitesCommLink.constBegin();
		while (cit != satellitesCommLink.constEnd())
		{
			satComms.insert(cit.key().toInt(), cit.value().toMap());
			cit++;
		}
		// Communications data for groups of satellites
		QVariantMap groupsCommLink = commMap.value("groups").toMap();
		QMap<QString, QVariant>::const_iterator grit=groupsCommLink.constBegin();
		while (grit != groupsCommLink.constEnd())
		{
			groupComms.insert(grit.key(), grit.value().toMap());
			grit++;
		}
	}
}

void Satellites::update(double deltaTime)
{
	// Separated because first test should be very fast.
	if (!hintFader && hintFader.getInterstate() <= 0.f)
		return;

	StelCore *core = StelApp::getInstance().getCore();

	if (qAbs(core->getTimeRate())>=Satellite::timeRateLimit) // Do not show satellites when time rate is over limit
		return;

	if (core->getCurrentPlanet() != earth || !isValidRangeDates(core))
		return;

	hintFader.update(static_cast<int>(deltaTime*1000));

	for (const auto& sat : qAsConst(satellites))
	{
		if (sat->initialized && sat->displayed)
			sat->update(deltaTime);
	}
}

void Satellites::draw(StelCore* core)
{
	// Separated because first test should be very fast.
	if (!hintFader && hintFader.getInterstate() <= 0.f)
		return;

	if (qAbs(core->getTimeRate())>=Satellite::timeRateLimit) // Do not show satellites when time rate is over limit
		return;

	if (core->getCurrentPlanet()!=earth || !isValidRangeDates(core))
		return;

	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(labelFont);
	Satellite::hintBrightness = hintFader.getInterstate();

	painter.setBlending(true);
	Satellite::hintTexture->bind();
	Satellite::viewportHalfspace = painter.getProjector()->getBoundingCap();
	for (const auto& sat : qAsConst(satellites))
	{
		if (sat && sat->initialized && sat->displayed)
			sat->draw(core, painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);

	if (getFlagUmbraVisible())
		drawCircles(core, painter);
}

void Satellites::drawPointer(StelCore* core, StelPainter& painter)
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
			return;
		painter.setColor(0.4f,0.5f,0.8f);
		texPointer->bind();

		painter.setBlending(true);

		// Size on screen
		double size = obj->getAngularRadius(core)*(2.*M_PI_180)*static_cast<double>(prj->getPixelPerRadAtCenter());
		size += 12. + 3.*std::sin(2. * StelApp::getInstance().getTotalRunTime());
		// size+=20.f + 10.f*std::sin(2.f * StelApp::getInstance().getTotalRunTime());
		painter.drawSprite2dMode(static_cast<float>(screenpos[0]-size/2), static_cast<float>(screenpos[1]-size/2), 20, 90);
		painter.drawSprite2dMode(static_cast<float>(screenpos[0]-size/2), static_cast<float>(screenpos[1]+size/2), 20, 0);
		painter.drawSprite2dMode(static_cast<float>(screenpos[0]+size/2), static_cast<float>(screenpos[1]+size/2), 20, -90);
		painter.drawSprite2dMode(static_cast<float>(screenpos[0]+size/2), static_cast<float>(screenpos[1]-size/2), 20, -180);
	}
}

void Satellites::drawCircles(StelCore* core, StelPainter &painter)
{
	StelProjectorP saveProj = painter.getProjector();
	painter.setProjector(core->getProjection(StelCore::FrameHeliocentricEclipticJ2000, StelCore::RefractionAuto));
	painter.setBlending(true, GL_ONE, GL_ONE);
	painter.setLineSmooth(true);
	painter.setFont(labelFont);

	double lambda, beta, umbraDistance_AU, umbraRadius_AU, penumbraDistance_AU, penumbraRadius_AU;
	const Vec3d pos = earth->getEclipticPos();
	const Vec3d dir = - sun->getAberrationPush() + pos;
	StelUtils::rectToSphe(&lambda, &beta, dir);
	const Mat4d rot=Mat4d::zrotation(lambda)*Mat4d::yrotation(-beta);

	SatelliteP sat = nullptr;
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Satellite");
	if (!newSelected.empty())
		sat = getById(newSelected[0].staticCast<Satellite>()->getCatalogNumberString());

	if (flagUmbraAtFixedDistance)
	{
		umbraDistance_AU = penumbraDistance_AU = fixedUmbraDistance/AU+earth->getEquatorialRadius(); // geocentric circle distance [AU]
		const double earthDistance=earth->getHeliocentricEclipticPos().norm(); // Earth distance [AU]
		const double sunHP = asin(earth->getEquatorialRadius()/earthDistance)    * (M_180_PI*3600.); // arcsec.
		const double satHP = asin(earth->getEquatorialRadius()/umbraDistance_AU) * (M_180_PI*3600.); // arcsec.
		const double sunSD = atan(sun->getEquatorialRadius()/earthDistance)      * (M_180_PI*3600.); // arcsec.

		//Classical Bessel elements
		double f1, f2;
		if (earthShadowEnlargementDanjon)
		{
			static const double danjonScale=1+1./85.-1./594.; // ~1.01, shadow magnification factor (see Espenak 5000 years Canon)
			f1=danjonScale*satHP + sunHP + sunSD; // penumbra radius, arcsec
			f2=danjonScale*satHP + sunHP - sunSD; // umbra radius, arcsec
		}
		else
		{
			const double mHP1=0.998340*satHP;
			f1=1.02*(mHP1 + sunHP + sunSD); // penumbra radius, arcsec
			f2=1.02*(mHP1 + sunHP - sunSD); // umbra radius, arcsec
		}
		penumbraRadius_AU=tan(f1/3600.*M_PI_180)*umbraDistance_AU;
		umbraRadius_AU=tan(f2/3600.*M_PI_180)*umbraDistance_AU;
	}
	else if (!sat.isNull())
	{
		Vec4d umbraData=sat->getUmbraData();
		umbraDistance_AU    = umbraData[0]/AU;
		umbraRadius_AU      = umbraData[1]/AU;
		penumbraDistance_AU = umbraData[2]/AU;
		penumbraRadius_AU   = umbraData[3]/AU;
	}
	else
		return;


	StelVertexArray umbra(StelVertexArray::LineLoop);
	for (int i=0; i<360; ++i)
	{
		Vec3d point(umbraDistance_AU, cos(i*M_PI_180)*umbraRadius_AU, sin(i*M_PI_180)*umbraRadius_AU);
		rot.transfo(point);
		umbra.vertex.append(pos+point);
	}
	painter.setColor(getUmbraColor(), 1.f);
	painter.drawStelVertexArray(umbra, false);

	// plot a center cross mark
	texCross->bind();
	Vec3d point(umbraDistance_AU, 0.0, 0.0);
	rot.transfo(point);
	Vec3d coord = pos+point;
	painter.drawSprite2dMode(coord, 5.f);
	QString cuLabel = QString("%1 (h=%2 %3)").arg(q_("C.U."), QString::number(AU*(umbraDistance_AU - earth->getEquatorialRadius()), 'f', 1), qc_("km","distance"));
	const float shift = 8.f;
	painter.drawText(coord, cuLabel, 0, shift, shift, false);

	if (getFlagPenumbraVisible())
	{
		StelVertexArray penumbra(StelVertexArray::LineLoop);
		for (int i=0; i<360; ++i)
		{
			Vec3d point(penumbraDistance_AU, cos(i*M_PI_180)*penumbraRadius_AU, sin(i*M_PI_180)*penumbraRadius_AU);
			rot.transfo(point);
			penumbra.vertex.append(pos+point);
		}

		painter.setColor(getPenumbraColor(), 1.f);
		painter.drawStelVertexArray(penumbra, false);
	}
	painter.setProjector(saveProj);
}

bool Satellites::checkJsonFileFormat()
{
	QFile jsonFile(catalogPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Satellites] cannot open " << QDir::toNativeSeparators(catalogPath);
		return false;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&jsonFile).toMap();
		jsonFile.close();
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "[Satellites] File format is wrong! Error:" << e.what();
		return false;
	}

	return true;
}

bool Satellites::isValidRangeDates(const StelCore *core) const
{
	bool ok;
	double tJD = core->getJD();
	double uJD = StelUtils::getJulianDayFromISO8601String(lastUpdate.toString(Qt::ISODate), &ok);
	if (lastUpdate.isNull()) // No updates yet?
		uJD = tJD;
	// do not draw anything before Oct 4, 1957, 19:28:34GMT ;-)
	// upper limit for drawing is +5 years after latest update of TLE
	if ((tJD<2436116.3115) || (tJD>(uJD+1825)))
		return false;
	else
		return true;
}

#if(SATELLITES_PLUGIN_IRIDIUM == 1)
#ifdef _OLD_IRIDIUM_PREDICTIONS
IridiumFlaresPredictionList Satellites::getIridiumFlaresPrediction()
{
	StelCore* pcore = StelApp::getInstance().getCore();
	double currentJD = pcore->getJD(); // save current JD
	bool isTimeNow = pcore->getIsTimeNow();
	long iJD =currentJD;
	double JDMidnight = iJD + 0.5 - pcore->getCurrentLocation().longitude / 360.f;
	double delta = currentJD - JDMidnight;

	pcore->setJD(iJD + 0.5 - pcore->getCurrentLocation().longitude / 360.f);
	pcore->update(10); // force update to get new coordinates

	IridiumFlaresPredictionList predictions;
	predictions.clear();
	for (const auto& sat : satellites)
	{
		if (sat->initialized && sat->getEnglishName().startsWith("IRIDIUM"))
		{
			double dt  = 0;
			double angle0 = 100;
			double lat0 = -1;
			while (dt<1)
			{
				Satellite::timeShift = dt+delta;
				sat->update(0);

				Vec3d pos = sat->getAltAzPosApparent(pcore);
				double lat = pos.latitude();
				double lon = M_PI - pos.longitude();
				double v =   sat->getVMagnitude(pcore);
				double angle =  sat->sunReflAngle;

				if (angle>0 && angle<2)
				{
					if (angle>angle0 && (v<1) && lat>5*M_PI/180)
					{
						IridiumFlaresPrediction flare;
						flare.datetime = StelUtils::julianDayToISO8601String(currentJD+dt+StelApp::getInstance().getCore()->getUTCOffset(currentJD+dt)/24.f);
						flare.satellite = sat->getEnglishName();
						flare.azimuth   = lon;
						flare.altitude  = lat;
						flare.magnitude = v;

						predictions.append(flare);

						dt +=0.005;
					}
					angle0 = angle;
				}
				if (angle>0)
					dt+=0.000002*angle*angle;
				else
				{
					if (lat0>0 && lat<0)
						dt+=0.05;
					else
						dt+=0.0002;
				}
				lat0 = lat;
			}
		}
	}
	Satellite::timeShift = 0.;
	if (isTimeNow)
		pcore->setTimeNow();
	else
		pcore->setJD(currentJD);

	return predictions;
}
#else

struct SatDataStruct {
	double nextJD;
	double angleToSun;
	double altitude;
	double azimuth;
	double v;
};

IridiumFlaresPredictionList Satellites::getIridiumFlaresPrediction()
{
	StelCore* pcore = StelApp::getInstance().getCore();
	SolarSystem* ssystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");

	double currentJD = pcore->getJD(); // save current JD
	bool isTimeNow = pcore->getIsTimeNow();

	double predictionJD = currentJD - 1.;  //  investigate what's seen recently// yesterday
	double predictionEndJD = currentJD + getIridiumFlaresPredictionDepth(); // 7 days interval by default
	pcore->setJD(predictionJD);

	bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();

	IridiumFlaresPredictionList predictions;

	// create a lіst of Iridiums
	QMap<SatelliteP,SatDataStruct> iridiums;
	SatDataStruct sds;
	double nextJD = predictionJD + 1./24;

	for (const auto& sat : satellites)
	{
		if (sat->initialized && sat->getEnglishName().startsWith("IRIDIUM"))
		{
			Vec3d pos = sat->getAltAzPosApparent(pcore);
			sds.angleToSun = sat->sunReflAngle;
			sds.altitude = pos.latitude();
			sds.azimuth = pos.longitude();
			sds.v = sat->getVMagnitude(pcore);
			double t;
			if (sds.altitude<0)
				t = qMax(-sds.altitude, 1.) / 2880;
			else
			if (sds.angleToSun>0)
				t = qMax(sds.angleToSun, 1.) / (2*86400);
			else
			{
				//qDebug() << "IRIDIUM warn: alt, angleToSun = " <<sds.altitude << sds.angleToSun;
				t = 0.25/1440; // we should never be here, but assuming 1/4 minute to leave this
			}

			sds.nextJD = predictionJD + t;
			iridiums.insert(sat, sds);
			if (nextJD>sds.nextJD)
				nextJD = sds.nextJD;
		}
	}
	predictionJD = nextJD;

	while (predictionJD<predictionEndJD)
	{
		nextJD = predictionJD + 1./24;
		pcore->setJD(predictionJD);

		ssystem->getEarth()->computePosition(predictionJD, Vec3d(0.));
		pcore->update(0);

		for (auto i = iridiums.begin(); i != iridiums.end(); ++i)
		{
			if ( i.value().nextJD<=predictionJD)
			{
				i.key()->update(0);

				double v = i.key()->getVMagnitude(pcore);
				bool flareFound = false;
				if (v > i.value().v)
				{
					if (i.value().v < 1. // brightness limit
					 && i.value().angleToSun>0.
					 && i.value().angleToSun<2.)
					{
						IridiumFlaresPrediction flare;
						flare.datetime = StelUtils::julianDayToISO8601String(predictionJD+StelApp::getInstance().getCore()->getUTCOffset(predictionJD)/24.f);
						flare.satellite = i.key()->getEnglishName();
						flare.azimuth   = i.value().azimuth;
						if (useSouthAzimuth)
						{
							flare.azimuth += M_PI;
							if (flare.azimuth > M_PI*2)
								flare.azimuth -= M_PI*2;
						}
						flare.altitude  = i.value().altitude;
						flare.magnitude = i.value().v;

						predictions.append(flare);
						flareFound = true;
						//qDebug() << "Flare:" << flare.datetime << flare.satellite;
					}
				}

				Vec3d pos = i.key()->getAltAzPosApparent(pcore);

				i.value().v = flareFound ?  17 : v; // block extra report
				i.value().altitude = pos.latitude();
				i.value().azimuth = M_PI - pos.longitude();
				i.value().angleToSun = i.key()->sunReflAngle;

				double t;
				if (flareFound)
					t = 1./24;
				else
				if (i.value().altitude<0)
					t = qMax((-i.value().altitude)*57,1.) / 5600;
				else
				if (i.value().angleToSun>0)
					t = qMax(i.value().angleToSun,1.) / (4*86400);
				else
				{
					//qDebug() << "IRIDIUM warn2: alt, angleToSun = " <<i.value().altitude << i.value().angleToSun;
					t = 0.25/1440; // we should never be here, but assuming 1/4 minute to leave this
				}
				i.value().nextJD = predictionJD + t;
				if (nextJD>i.value().nextJD)
					nextJD = i.value().nextJD;
			}
		}
		predictionJD = nextJD;
	}

	//Satellite::timeShift = 0.;
	if (isTimeNow)
		pcore->setTimeNow();
	else
		pcore->setJD(currentJD);

	return predictions;
}
#endif
// close SATELLITES_PLUGIN_IRIDIUM
#endif

void Satellites::createSuperGroupsList()
{
	QString communications = "communications", navigation = "navigation", scientific = "scientific",
		earthresources = "earth resources", gps = "gps", glonass = "glonass", geostationary = "geostationary";
	satSuperGroupsMap = {
		{ "geo", communications },
		{ "geo", geostationary },
		{ "gpz", communications },
		{ "gpz", geostationary },
		{ "gpz-plus", communications },
		{ "gpz-plus", geostationary },
		{ "intelsat", communications },
		{ "ses", communications },
		{ "iridium", communications },
		{ "iridium-NEXT", communications },
		{ "starlink", communications },
		{ "oneweb", communications },
		{ "orbcomm", communications },
		{ "globalstar", communications },
		{ "swarm", communications },
		{ "amateur", communications },
		{ "x-comm", communications },
		{ "other-comm", communications },
		{ "satnogs", communications },
		{ "gorizont", communications },
		{ "raduga", communications },
		{ "raduga", geostationary },
		{ "molniya", communications },
		{ "gnss", navigation },
		{ "gps", navigation },
		{ "gps-ops", navigation },
		{ "gps-ops", gps },
		{ "glonass", navigation },
		{ "glo-ops", navigation },
		{ "glo-ops", glonass },
		{ "galileo", navigation },
		{ "beidou", navigation },
		{ "sbas", navigation },
		{ "nnss", navigation },
		{ "musson", navigation },
		{ "science", scientific },
		{ "geodetic", scientific },
		{ "engineering", scientific },
		{ "education", scientific },
		{ "goes", scientific },
		{ "goes", earthresources },
		{ "resource", earthresources },
		{ "sarsat", earthresources },
		{ "dmc", earthresources },
		{ "tdrss", earthresources },
		{ "argos", earthresources },
		{ "planet", earthresources },
		{ "spire", earthresources }
	};
}

void Satellites::translations()
{
#if 0
	// Satellite groups
	//
	// *** Generic groups (not related to CelesTrak groups)
	//
	// TRANSLATORS: Satellite group: Communication satellites
	N_("communications");
	// TRANSLATORS: Satellite group: Navigation satellites
	N_("navigation");
	// TRANSLATORS: Satellite group: Scientific satellites
	N_("scientific");
	// TRANSLATORS: Satellite group: Earth Resources satellites
	N_("earth resources");
	// TRANSLATORS: Satellite group: Satellites in geostationary orbit
	N_("geostationary");
	// TRANSLATORS: Satellite group: Satellites that are no longer functioning
	N_("non-operational");
	// TRANSLATORS: Satellite group: Satellites belonging to the space observatories
	N_("observatory");	
	// TRANSLATORS: Satellite group: The Indian Regional Navigation Satellite System (IRNSS) is an autonomous regional satellite navigation system being developed by the Indian Space Research Organisation (ISRO) which would be under complete control of the Indian government.
	N_("irnss");
	// TRANSLATORS: Satellite group: The Quasi-Zenith Satellite System (QZSS), is a proposed three-satellite regional time transfer system and Satellite Based Augmentation System for the Global Positioning System, that would be receivable within Japan.
	N_("qzss");
	// TRANSLATORS: Satellite group: Satellites belonging to the COSMOS satellites
	N_("cosmos");
	// TRANSLATORS: Satellite group: Debris of satellites
	N_("debris");
	// TRANSLATORS: Satellite group: Crewed satellites
	N_("crewed");
	// TRANSLATORS: Satellite group: Satellites of ISS resupply missions
	N_("resupply");
	// TRANSLATORS: Satellite group: are known to broadcast TV signals
	N_("tv");
	// TRANSLATORS: Satellite group: Satellites belonging to the GONETS satellites
	N_("gonets");
	//
	// *** Special-Interest Satellites [CelesTrak groups]
	//
	// TRANSLATORS: Satellite group: Last 30 Days' Launches
	// TRANSLATORS: CelesTrak source [Last 30 Days' Launches]: https://celestrak.org/NORAD/elements/tle-new.txt
	N_("tle-new");
	// TRANSLATORS: Satellite group: Last 30 Days' Launches
	// TRANSLATORS: CelesTrak source [Last 30 Days' Launches]: https://celestrak.org/NORAD/elements/gp.php?GROUP=last-30-days&FORMAT=tle
	N_("last-30-days");
	// TRANSLATORS: Satellite group: Space stations	
	// TRANSLATORS: CelesTrak source [Space Stations]: https://celestrak.org/NORAD/elements/stations.txt
	// TRANSLATORS: CelesTrak source [Space Stations]: https://celestrak.org/NORAD/elements/gp.php?GROUP=stations&FORMAT=tle
	N_("stations");
	// TRANSLATORS: Satellite group: Bright/naked-eye-visible satellites
	// TRANSLATORS: CelesTrak source [100 (or so) Brightest]: https://celestrak.org/NORAD/elements/visual.txt
	// TRANSLATORS: CelesTrak source [100 (or so) Brightest]: https://celestrak.org/NORAD/elements/gp.php?GROUP=visual&FORMAT=tle
	N_("visual");
	// TRANSLATORS: Satellite group: Active Satellites
	// TRANSLATORS: CelesTrak source [Active Satellites]: https://celestrak.org/NORAD/elements/active.txt
	// TRANSLATORS: CelesTrak source [Active Satellites]: https://celestrak.org/NORAD/elements/gp.php?GROUP=active&FORMAT=tle
	N_("active");
	// TRANSLATORS: Satellite group: Analyst Satellites
	// TRANSLATORS: CelesTrak source [Analyst Satellites]: https://celestrak.org/NORAD/elements/analyst.txt
	// TRANSLATORS: CelesTrak source [Analyst Satellites]: https://celestrak.org/NORAD/elements/gp.php?GROUP=analyst&FORMAT=tle
	N_("analyst");
	//
	// *** Weather & Earth Resources Satellites [CelesTrak groups]
	//
	// TRANSLATORS: Satellite group: Weather (meteorological) satellites
	// TRANSLATORS: CelesTrak source [Weather]: https://celestrak.org/NORAD/elements/weather.txt
	// TRANSLATORS: CelesTrak source [Weather]: https://celestrak.org/NORAD/elements/gp.php?GROUP=weather&FORMAT=tle
	N_("weather");
	// TRANSLATORS: Satellite group: Satellites belonging to the NOAA satellites
	// TRANSLATORS: CelesTrak source [NOAA]: https://celestrak.org/NORAD/elements/noaa.txt
	// TRANSLATORS: CelesTrak source [NOAA]: https://celestrak.org/NORAD/elements/gp.php?GROUP=noaa&FORMAT=tle
	N_("noaa");
	// TRANSLATORS: Satellite group: Satellites belonging to the GOES satellites
	// TRANSLATORS: CelesTrak source [GOES]: https://celestrak.org/NORAD/elements/goes.txt
	// TRANSLATORS: CelesTrak source [GOES]: https://celestrak.org/NORAD/elements/gp.php?GROUP=goes&FORMAT=tle
	N_("goes");
	// TRANSLATORS: Satellite group: Earth Resources satellites
	// TRANSLATORS: CelesTrak source [Earth Resources]: https://celestrak.org/NORAD/elements/resource.txt
	// TRANSLATORS: CelesTrak source [Earth Resources]: https://celestrak.org/NORAD/elements/gp.php?GROUP=resource&FORMAT=tle
	N_("resource");
	// TRANSLATORS: Satellite group: Search & Rescue (SARSAT) satellites
	// TRANSLATORS: CelesTrak source [Search & Rescue (SARSAT)]: https://celestrak.org/NORAD/elements/sarsat.txt
	// TRANSLATORS: CelesTrak source [Search & Rescue (SARSAT)]: https://celestrak.org/NORAD/elements/gp.php?GROUP=sarsat&FORMAT=tle
	N_("sarsat");
	// TRANSLATORS: Satellite group: Disaster Monitoring satellites
	// TRANSLATORS: CelesTrak source [Disaster Monitoring]: https://celestrak.org/NORAD/elements/dmc.txt
	// TRANSLATORS: CelesTrak source [Disaster Monitoring]: https://celestrak.org/NORAD/elements/gp.php?GROUP=dmc&FORMAT=tle
	N_("dmc");
	// TRANSLATORS: Satellite group: The Tracking and Data Relay Satellite System (TDRSS) is a network of communications satellites and ground stations used by NASA for space communications.
	// TRANSLATORS: CelesTrak source [Tracking and Data Relay Satellite System (TDRSS)]: https://celestrak.org/NORAD/elements/tdrss.txt
	// TRANSLATORS: CelesTrak source [Tracking and Data Relay Satellite System (TDRSS)]: https://celestrak.org/NORAD/elements/gp.php?GROUP=tdrss&FORMAT=tle
	N_("tdrss");
	// TRANSLATORS: Satellite group: ARGOS Data Collection System satellites
	// TRANSLATORS: CelesTrak source [ARGOS Data Collection System]: https://celestrak.org/NORAD/elements/argos.txt
	// TRANSLATORS: CelesTrak source [ARGOS Data Collection System]: https://celestrak.org/NORAD/elements/gp.php?GROUP=argos&FORMAT=tle
	N_("argos");
	// TRANSLATORS: Satellite group: Satellites belonging to the Planet satellites
	// TRANSLATORS: CelesTrak source [Planet]: https://celestrak.org/NORAD/elements/planet.txt
	// TRANSLATORS: CelesTrak source [Planet]: https://celestrak.org/NORAD/elements/gp.php?GROUP=planet&FORMAT=tle
	// TRANSLATORS: CelesTrak supplemental source [Planet TLEs]: https://celestrak.org/NORAD/elements/supplemental/planet.txt
	N_("planet");
	// TRANSLATORS: Satellite group: Satellites belonging to the Spire constellation (LEMUR satellites)
	// TRANSLATORS: CelesTrak source [Spire]: https://celestrak.org/NORAD/elements/spire.txt
	// TRANSLATORS: CelesTrak source [Spire]: https://celestrak.org/NORAD/elements/gp.php?GROUP=spire&FORMAT=tle
	N_("spire");	
	//
	// *** Communications Satellites [CelesTrak groups]
	//
	// TRANSLATORS: Satellite group: Active Geosynchronous Satellites
	// TRANSLATORS: CelesTrak source [Active Geosynchronous]: https://celestrak.org/NORAD/elements/geo.txt
	// TRANSLATORS: CelesTrak source [Active Geosynchronous]: https://celestrak.org/NORAD/elements/gp.php?GROUP=geo&FORMAT=tle
	N_("geo");
	// TRANSLATORS: Satellite group: GEO Protected Zone
	// TRANSLATORS: CelesTrak source [GEO Protected Zone]: https://celestrak.org/satcat/gpz.php
	// TRANSLATORS: CelesTrak source [GEO Protected Zone]: https://celestrak.org/NORAD/elements/gp.php?SPECIAL=gpz&FORMAT=tle
	N_("gpz");
	// TRANSLATORS: Satellite group: GEO Protected Zone Plus
	// TRANSLATORS: CelesTrak source [GEO Protected Zone Plus]: https://celestrak.org/satcat/gpz-plus.php
	// TRANSLATORS: CelesTrak source [GEO Protected Zone Plus]: https://celestrak.org/NORAD/elements/gp.php?SPECIAL=gpz-plus&FORMAT=tle
	N_("gpz-plus");
	// TRANSLATORS: Satellite group: Satellites belonging to the INTELSAT satellites
	// TRANSLATORS: CelesTrak source [Intelsat]: https://celestrak.org/NORAD/elements/intelsat.txt
	// TRANSLATORS: CelesTrak source [Intelsat]: https://celestrak.org/NORAD/elements/gp.php?GROUP=intelsat&FORMAT=tle
	// TRANSLATORS: CelesTrak supplemental source [INTELSAT TLEs]: https://celestrak.org/NORAD/elements/supplemental/intelsat.txt
	N_("intelsat");
	// TRANSLATORS: Satellite group: Satellites belonging to the SES satellites
	// TRANSLATORS: CelesTrak source [SES]: https://celestrak.org/NORAD/elements/ses.txt
	// TRANSLATORS: CelesTrak source [SES]: https://celestrak.org/NORAD/elements/gp.php?GROUP=ses&FORMAT=tle
	// TRANSLATORS: CelesTrak supplemental source [SES TLEs]: https://celestrak.org/NORAD/elements/supplemental/ses.txt
	N_("ses");
	// TRANSLATORS: Satellite group: Satellites belonging to the Iridium constellation (Iridium is a proper name)
	// TRANSLATORS: CelesTrak source [Iridium]: https://celestrak.org/NORAD/elements/iridium.txt
	// TRANSLATORS: CelesTrak source [Iridium]: https://celestrak.org/NORAD/elements/gp.php?GROUP=iridium&FORMAT=tle
	N_("iridium");
	// TRANSLATORS: Satellite group: Satellites belonging to the Iridium NEXT constellation (Iridium is a proper name)
	// TRANSLATORS: CelesTrak source [Iridium NEXT]: https://celestrak.org/NORAD/elements/iridium-NEXT.txt
	// TRANSLATORS: CelesTrak source [Iridium NEXT]: https://celestrak.org/NORAD/elements/gp.php?GROUP=iridium-NEXT&FORMAT=tle
	N_("iridium-NEXT");
	// TRANSLATORS: Satellite group: Satellites belonging to the Starlink constellation (Starlink is a proper name)
	// TRANSLATORS: CelesTrak source [Starlink]: https://celestrak.org/NORAD/elements/starlink.txt
	// TRANSLATORS: CelesTrak source [Starlink]: https://celestrak.org/NORAD/elements/gp.php?GROUP=starlink&FORMAT=tle
	// TRANSLATORS: CelesTrak supplemental source [Starlink TLEs]: https://celestrak.org/NORAD/elements/supplemental/starlink.txt
	N_("starlink");
	// TRANSLATORS: Satellite group: Satellites belonging to the OneWeb constellation (OneWeb is a proper name)
	// TRANSLATORS: CelesTrak source [OneWeb]: https://celestrak.org/NORAD/elements/oneweb.txt
	// TRANSLATORS: CelesTrak source [OneWeb]: https://celestrak.org/NORAD/elements/gp.php?GROUP=oneweb&FORMAT=tle
	// TRANSLATORS: CelesTrak supplemental source [OneWeb TLEs]: https://celestrak.org/NORAD/elements/supplemental/oneweb.txt
	N_("oneweb");
	// TRANSLATORS: Satellite group: Satellites belonging to the ORBCOMM satellites
	// TRANSLATORS: CelesTrak source [Orbcomm]: https://celestrak.org/NORAD/elements/orbcomm.txt
	// TRANSLATORS: CelesTrak source [Orbcomm]: https://celestrak.org/NORAD/elements/gp.php?GROUP=orbcomm&FORMAT=tle
	// TRANSLATORS: CelesTrak supplemental source [ORBCOMM TLEs]: https://celestrak.org/NORAD/elements/supplemental/orbcomm.txt
	N_("orbcomm");
	// TRANSLATORS: Satellite group: Satellites belonging to the GLOBALSTAR satellites
	// TRANSLATORS: CelesTrak source [Globalstar]: https://celestrak.org/NORAD/elements/globalstar.txt
	// TRANSLATORS: CelesTrak source [Globalstar]: https://celestrak.org/NORAD/elements/gp.php?GROUP=globalstar&FORMAT=tle
	N_("globalstar");
	// TRANSLATORS: Satellite group: Satellites belonging to the SWARM satellites
	// TRANSLATORS: CelesTrak source [Swarm]: https://celestrak.org/NORAD/elements/swarm.txt
	// TRANSLATORS: CelesTrak source [Swarm]: https://celestrak.org/NORAD/elements/gp.php?GROUP=swarm&FORMAT=tle
	N_("swarm");
	// TRANSLATORS: Satellite group: Amateur radio (ham) satellites
	// TRANSLATORS: CelesTrak source [Amateur Radio]: https://celestrak.org/NORAD/elements/amateur.txt
	// TRANSLATORS: CelesTrak source [Amateur Radio]: https://celestrak.org/NORAD/elements/gp.php?GROUP=amateur&FORMAT=tle
	N_("amateur");
	// TRANSLATORS: Satellite group: Experimental communication satellites
	// TRANSLATORS: CelesTrak source [Experimental]: https://celestrak.org/NORAD/elements/x-comm.txt
	// TRANSLATORS: CelesTrak source [Experimental]: https://celestrak.org/NORAD/elements/gp.php?GROUP=x-comm&FORMAT=tle
	N_("x-comm");
	// TRANSLATORS: Satellite group: Other communication satellites
	// TRANSLATORS: CelesTrak source [Other Comm]: https://celestrak.org/NORAD/elements/other-comm.txt
	// TRANSLATORS: CelesTrak source [Other Comm]: https://celestrak.org/NORAD/elements/gp.php?GROUP=other-comm&FORMAT=tle
	N_("other-comm");
	// TRANSLATORS: Satellite group: Satellites belonging to the SatNOGS satellites
	// TRANSLATORS: CelesTrak source [SatNOGS]: https://celestrak.org/NORAD/elements/satnogs.txt
	// TRANSLATORS: CelesTrak source [SatNOGS]: https://celestrak.org/NORAD/elements/gp.php?GROUP=satnogs&FORMAT=tle
	N_("satnogs");
	// TRANSLATORS: Satellite group: Satellites belonging to the GORIZONT satellites
	// TRANSLATORS: CelesTrak source [Gorizont]: https://celestrak.org/NORAD/elements/gorizont.txt
	// TRANSLATORS: CelesTrak source [Gorizont]: https://celestrak.org/NORAD/elements/gp.php?GROUP=gorizont&FORMAT=tle
	N_("gorizont");
	// TRANSLATORS: Satellite group: Satellites belonging to the RADUGA satellites
	// TRANSLATORS: CelesTrak source [Raduga]: https://celestrak.org/NORAD/elements/raduga.txt
	// TRANSLATORS: CelesTrak source [Raduga]: https://celestrak.org/NORAD/elements/gp.php?GROUP=raduga&FORMAT=tle
	N_("raduga");
	// TRANSLATORS: Satellite group: Satellites belonging to the MOLNIYA satellites
	// TRANSLATORS: CelesTrak source [Molniya]: https://celestrak.org/NORAD/elements/molniya.txt
	// TRANSLATORS: CelesTrak source [Molniya]: https://celestrak.org/NORAD/elements/gp.php?GROUP=molniya&FORMAT=tle
	N_("molniya");	
	//
	// *** Navigation Satellites [CelesTrak groups]
	//
	// TRANSLATORS: Satellite group: Satellites belonging to the GNSS satellites
	// TRANSLATORS: CelesTrak source [GNSS]: https://celestrak.org/NORAD/elements/gnss.txt
	// TRANSLATORS: CelesTrak source [GNSS]: https://celestrak.org/NORAD/elements/gp.php?GROUP=gnss&FORMAT=tle
	N_("gnss");
	// TRANSLATORS: Satellite group: Satellites belonging to the GPS constellation (the Global Positioning System)
	// TRANSLATORS: CelesTrak supplemental source [GPS TLEs]: https://celestrak.org/NORAD/elements/supplemental/gps.txt
	N_("gps");
	// TRANSLATORS: Satellite group: Satellites belonging to the GPS constellation (the Global Positioning System)
	// TRANSLATORS: CelesTrak source [GPS Operational]: https://celestrak.org/NORAD/elements/gps-ops.txt
	// TRANSLATORS: CelesTrak source [GPS Operational]: https://celestrak.org/NORAD/elements/gp.php?GROUP=gps-ops&FORMAT=tle
	N_("gps-ops");
	// TRANSLATORS: Satellite group: Satellites belonging to the GLONASS constellation (GLObal NAvigation Satellite System)
	// TRANSLATORS: CelesTrak supplemental source [GLONASS TLEs]: https://celestrak.org/NORAD/elements/supplemental/glonass.txt
	N_("glonass");
	// TRANSLATORS: Satellite group: Satellites belonging to the GLONASS constellation (GLObal NAvigation Satellite System)
	// TRANSLATORS: CelesTrak supplemental source [GLONASS Operational]: https://celestrak.org/NORAD/elements/glo-ops.txt
	// TRANSLATORS: CelesTrak supplemental source [GLONASS Operational]: https://celestrak.org/NORAD/elements/gp.php?GROUP=glo-ops&FORMAT=tle
	N_("glo-ops");
	// TRANSLATORS: Satellite group: Satellites belonging to the Galileo constellation (global navigation satellite system by the European Union)
	// TRANSLATORS: CelesTrak source [Galileo]: https://celestrak.org/NORAD/elements/galileo.txt
	// TRANSLATORS: CelesTrak source [Galileo]: https://celestrak.org/NORAD/elements/gp.php?GROUP=galileo&FORMAT=tle
	N_("galileo");
	// TRANSLATORS: Satellite group: Satellites belonging to the BeiDou constellation (BeiDou Navigation Satellite System)
	// TRANSLATORS: CelesTrak source [Beidou]: https://celestrak.org/NORAD/elements/beidou.txt
	// TRANSLATORS: CelesTrak source [Beidou]: https://celestrak.org/NORAD/elements/gp.php?GROUP=beidou&FORMAT=tle
	N_("beidou");
	// TRANSLATORS: Satellite group: Satellite-Based Augmentation System (WAAS/EGNOS/MSAS)
	// TRANSLATORS: CelesTrak source [Satellite-Based Augmentation System (WAAS/EGNOS/MSAS)]: https://celestrak.org/NORAD/elements/sbas.txt
	// TRANSLATORS: CelesTrak source [Satellite-Based Augmentation System (WAAS/EGNOS/MSAS)]: https://celestrak.org/NORAD/elements/gp.php?GROUP=sbas&FORMAT=tle
	N_("sbas");
	// TRANSLATORS: Satellite group: Navy Navigation Satellite System (NNSS)
	// TRANSLATORS: CelesTrak source [Navy Navigation Satellite System (NNSS)]: https://celestrak.org/NORAD/elements/nnss.txt
	// TRANSLATORS: CelesTrak source [Navy Navigation Satellite System (NNSS)]: https://celestrak.org/NORAD/elements/gp.php?GROUP=nnss&FORMAT=tle
	N_("nnss");
	// TRANSLATORS: Satellite group: Russian LEO Navigation Satellites
	// TRANSLATORS: CelesTrak source [Russian LEO Navigation]: https://celestrak.org/NORAD/elements/musson.txt
	// TRANSLATORS: CelesTrak source [Russian LEO Navigation]: https://celestrak.org/NORAD/elements/gp.php?GROUP=musson&FORMAT=tle
	N_("musson");
	//
	// *** Scientific Satellites [CelesTrak groups]
	//
	// TRANSLATORS: Satellite group: Space & Earth Science satellites
	// TRANSLATORS: CelesTrak source [Space & Earth Science]: https://celestrak.org/NORAD/elements/science.txt
	// TRANSLATORS: CelesTrak source [Space & Earth Science]: https://celestrak.org/NORAD/elements/gp.php?GROUP=science&FORMAT=tle
	N_("science");
	// TRANSLATORS: Satellite group: Geodetic satellites
	// TRANSLATORS: CelesTrak source [Geodetic]: https://celestrak.org/NORAD/elements/geodetic.txt
	// TRANSLATORS: CelesTrak source [Geodetic]: https://celestrak.org/NORAD/elements/gp.php?GROUP=geodetic&FORMAT=tle
	N_("geodetic");
	// TRANSLATORS: Satellite group: Engineering satellites
	// TRANSLATORS: CelesTrak source [Engineering]: https://celestrak.org/NORAD/elements/engineering.txt
	// TRANSLATORS: CelesTrak source [Engineering]: https://celestrak.org/NORAD/elements/gp.php?GROUP=engineering&FORMAT=tle
	N_("engineering");
	// TRANSLATORS: Satellite group: Education satellites
	// TRANSLATORS: CelesTrak source [Education]: https://celestrak.org/NORAD/elements/education.txt
	// TRANSLATORS: CelesTrak source [Education]: https://celestrak.org/NORAD/elements/gp.php?GROUP=education&FORMAT=tle
	N_("education");
	//
	// *** Miscellaneous Satellites [CelesTrak groups]
	//
	// TRANSLATORS: Satellite group: Military satellites
	// TRANSLATORS: CelesTrak source [Miscellaneous Military]: https://celestrak.org/NORAD/elements/military.txt
	// TRANSLATORS: CelesTrak source [Miscellaneous Military]: https://celestrak.org/NORAD/elements/gp.php?GROUP=military&FORMAT=tle
	N_("military");
	// TRANSLATORS: Satellite group: Radar Calibration satellites
	// TRANSLATORS: CelesTrak source [Radar Calibration]: https://celestrak.org/NORAD/elements/radar.txt
	// TRANSLATORS: CelesTrak source [Radar Calibration]: https://celestrak.org/NORAD/elements/gp.php?GROUP=radar&FORMAT=tle
	N_("radar");
	// TRANSLATORS: Satellite group: CubeSats (Cube Satellites)
	// TRANSLATORS: CelesTrak source [CubeSats]: https://celestrak.org/NORAD/elements/cubesat.txt
	// TRANSLATORS: CelesTrak source [CubeSats]: https://celestrak.org/NORAD/elements/gp.php?GROUP=cubesat&FORMAT=tle
	N_("cubesat");
	// TRANSLATORS: Satellite group: Other satellites
	// TRANSLATORS: CelesTrak source [Other]: https://celestrak.org/NORAD/elements/other.txt
	// TRANSLATORS: CelesTrak source [Other]: https://celestrak.org/NORAD/elements/gp.php?GROUP=other&FORMAT=tle
	N_("other");
	//
	// *** Supplemental Two-Line Element Sets [CelesTrak groups]
	//
	// Probably the meteorological satellites
	// TRANSLATORS: Satellite group: Meteosat
	// TRANSLATORS: CelesTrak supplemental source [METEOSAT TLEs]: https://celestrak.org/NORAD/elements/supplemental/meteosat.txt
	N_("meteosat");
	// Probably the communications satellites
	// TRANSLATORS: Satellite group: Telesat
	// TRANSLATORS: CelesTrak supplemental source [Telesat TLEs]: https://celestrak.org/NORAD/elements/supplemental/telesat.txt
	N_("telesat");
	// TRANSLATORS: Satellite group: ISS Segments
	// TRANSLATORS: CelesTrak supplemental source [ISS TLEs]: https://celestrak.org/NORAD/elements/supplemental/iss.txt
	N_("iss");
	// TRANSLATORS: Satellite group: CPF
	// TRANSLATORS: CelesTrak supplemental source [CPF TLEs]: https://celestrak.org/NORAD/elements/supplemental/cpf.txt
	N_("cpf");

	// Satellite names - a few famous objects only
	// TRANSLATORS: Satellite name: International Space Station
	N_("ISS (ZARYA)");
	// TRANSLATORS: Satellite name: International Space Station
	N_("ISS (NAUKA)");
	// TRANSLATORS: Satellite name: International Space Station
	N_("ISS");
	// TRANSLATORS: Satellite name: Hubble Space Telescope
	N_("HST");
	// TRANSLATORS: Satellite name: Spektr-R Space Observatory (or RadioAstron)
	N_("SPEKTR-R");
	// TRANSLATORS: Satellite name: Spektr-RG Space Observatory
	N_("SPEKTR-RG");
	// TRANSLATORS: Satellite name: International Gamma-Ray Astrophysics Laboratory (INTEGRAL)
	N_("INTEGRAL");
	// TRANSLATORS: Satellite name: China's first space station name
	N_("TIANGONG 1");
	// TRANSLATORS: Satellite name: name of China's space station module
	N_("TIANHE");
	// TRANSLATORS: Satellite name: China's space station name (with name of base module)
	N_("TIANGONG (TIANHE)");

	// Special terms for communications
	// TRANSLATORS: An uplink (UL or U/L) is the link from a ground station to a satellite
	N_("uplink");
	// TRANSLATORS: A downlink (DL) is the link from a satellite to a ground station
	N_("downlink");
	// TRANSLATORS: The beacon (or radio beacon) is a device in the satellite, which emit one or more signals (normally on a fixed frequency) whose purpose is twofold: station-keeping information (telemetry) and locates the satellite (determines its azimuth and elevation) in the sky
	N_("beacon");
	// TRANSLATORS: Telemetry is the collection of measurements or other data at satellites and their automatic transmission to receiving equipment (telecommunication) for monitoring
	N_("telemetry");
	// TRANSLATORS: The channel for transmission of video data
	N_("video");
	// TRANSLATORS: The broadband is wide bandwidth data transmission which transports multiple signals at a wide range of frequencies
	N_("broadband");
	// TRANSLATORS: The channel for transmission of commands
	N_("command");

#endif
}
