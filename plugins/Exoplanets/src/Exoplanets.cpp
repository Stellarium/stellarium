/*
 * Copyright (C) 2012 Alexander Wolf
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
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StarMgr.hpp"
#include "LabelMgr.hpp"
#include "Exoplanets.hpp"
#include "Exoplanet.hpp"
#include "ExoplanetsDialog.hpp"
#include "StelActionMgr.hpp"
#include "StelProgressController.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QTimer>
#include <QVariantMap>
#include <QVariant>
#include <QList>
#include <QSharedPointer>
#include <QStringList>
#include <QPixmap>
#include <QDir>
#include <QSettings>
#include <stdexcept>

#define CATALOG_FORMAT_VERSION 1 /* Version of format of catalog */

/*
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*/
StelModule* ExoplanetsStelPluginInterface::getStelModule() const
{
	return new Exoplanets();
}

StelPluginInfo ExoplanetsStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Exoplanets);

	StelPluginInfo info;
	info.id = "Exoplanets";
	info.displayedName = N_("Exoplanets");
	info.authors = "Alexander Wolf";
	info.contact = "https://github.com/Stellarium/stellarium";
	info.description = N_("This plugin plots the position of stars with exoplanets. Exoplanets data is derived from the 'Extrasolar Planets Encyclopaedia' at exoplanet.eu");
	info.version = EXOPLANETS_PLUGIN_VERSION;
	info.license = EXOPLANETS_PLUGIN_LICENSE;
	return info;
}

/*
 Constructor
*/
Exoplanets::Exoplanets()
	: PSCount(0)
	, EPCountAll(0)
	, EPCountPH(0)
	, updateState(CompleteNoUpdates)
	, networkManager(Q_NULLPTR)
	, downloadReply(Q_NULLPTR)
	, updateTimer(Q_NULLPTR)
	, updatesEnabled(false)
	, updateFrequencyHours(0)
	, enableAtStartup(false)
	, flagShowExoplanets(false)
	, flagShowExoplanetsButton(false)
	, toolbarButton(Q_NULLPTR)
	, progressBar(Q_NULLPTR)
{
	setObjectName("Exoplanets");
	exoplanetsConfigDialog = new ExoplanetsDialog();
	conf = StelApp::getInstance().getSettings();
	setFontSize(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSize(int)));
}

/*
 Destructor
*/
Exoplanets::~Exoplanets()
{
	StelApp::getInstance().getStelObjectMgr().unSelect();

	delete exoplanetsConfigDialog;
}

void Exoplanets::deinit()
{
	ep.clear();
	Exoplanet::markerTexture.clear();
	texPointer.clear();
}

void Exoplanets::update(double) //deltaTime
{
	//
}

/*
 Reimplementation of the getCallOrder method
*/
double Exoplanets::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void Exoplanets::init()
{
	upgradeConfigIni();

	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Exoplanets");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Exoplanets"))
		{
			qDebug() << "[Exoplanets] No Exoplanets section exists in main config file - creating with defaults";
			resetConfiguration();
		}

		// populate settings from main config file.
		loadConfiguration();

		jsonCatalogPath = StelFileMgr::findFile("modules/Exoplanets", static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable)) + "/exoplanets.json";
		if (jsonCatalogPath.isEmpty())
			return;

		texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");
		Exoplanet::markerTexture = StelApp::getInstance().getTextureManager().createTexture(":/Exoplanets/exoplanet.png");

		// key bindings and other actions
		addAction("actionShow_Exoplanets", N_("Exoplanets"), N_("Show exoplanets"), "showExoplanets", "Ctrl+Alt+E");
		addAction("actionShow_Exoplanets_ConfigDialog", N_("Exoplanets"), N_("Exoplanets configuration window"), exoplanetsConfigDialog, "visible", "Alt+E");

		setFlagShowExoplanets(getEnableAtStartup());
		setFlagShowExoplanetsButton(flagShowExoplanetsButton);
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "[Exoplanets] init error: " << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(jsonCatalogPath).exists())
	{
		if (!checkJsonFileFormat() || getJsonFileFormatVersion()<CATALOG_FORMAT_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "[Exoplanets] exoplanets.json does not exist - copying default catalog to " << QDir::toNativeSeparators(jsonCatalogPath);
		restoreDefaultJsonFile();
	}

	qDebug() << "[Exoplanets] loading catalog file:" << QDir::toNativeSeparators(jsonCatalogPath);

	readJsonFile();

	// Set up download manager and the update schedule
	networkManager = StelApp::getInstance().getNetworkAccessManager();
	updateState = CompleteNoUpdates;
	updateTimer = new QTimer(this);
	updateTimer->setSingleShot(false);   // recurring check for update
	updateTimer->setInterval(13000);     // check once every 13 seconds to see if it is time for an update
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
	updateTimer->start();

	connect(this, SIGNAL(jsonUpdateComplete(void)), this, SLOT(reloadCatalog()));
	connect(StelApp::getInstance().getCore(), SIGNAL(configurationDataSaved()), this, SLOT(saveSettings()));

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

void Exoplanets::draw(StelCore* core)
{
	if (!flagShowExoplanets)
		return;

	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);
	
	for (const auto& eps : ep)
	{
		if (eps && eps->initialized)
			eps->draw(core, &painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);
}

void Exoplanets::drawPointer(StelCore* core, StelPainter& painter)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Exoplanet");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!painter.getProjector()->project(pos, screenpos))
			return;

		const Vec3f& c(obj->getInfoColor());
		painter.setColor(c[0],c[1],c[2]);
		texPointer->bind();
		painter.setBlending(true);
		painter.drawSprite2dMode(static_cast<float>(screenpos[0]), static_cast<float>(screenpos[1]), 13.f, static_cast<float>(StelApp::getInstance().getTotalRunTime())*40.f);
	}
}

QList<StelObjectP> Exoplanets::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	if (!flagShowExoplanets)
		return result;

	Vec3d v(av);
	v.normalize();
	const double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	for (const auto& eps : ep)
	{
		if (eps->initialized)
		{
			equPos = eps->XYZ;
			equPos.normalize();
			if (equPos.dot(v) >= cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(eps));
			}
		}
	}

	return result;
}

StelObjectP Exoplanets::searchByName(const QString& englishName) const
{
	if (!flagShowExoplanets)
		return Q_NULLPTR;

	for (const auto& eps : ep)
	{
		if (eps->getEnglishName().toUpper() == englishName.toUpper() || eps->getDesignation().toUpper() == englishName.toUpper())
			return qSharedPointerCast<StelObject>(eps);

		QStringList ppn = eps->getExoplanetsEnglishNames();
		if (!ppn.isEmpty())
		{
			for (const auto& str : ppn)
			{
				if (str.toUpper() == englishName.toUpper())
					return qSharedPointerCast<StelObject>(eps);
			}
		}

		ppn = eps->getExoplanetsDesignations();
		if (!ppn.isEmpty())
		{
			for (const auto& str : ppn)
			{
				if (str.toUpper() == englishName.toUpper())
					return qSharedPointerCast<StelObject>(eps);
			}
		}
	}

	return Q_NULLPTR;
}

StelObjectP Exoplanets::searchByID(const QString &id) const
{
	for (const auto& eps : ep)
	{
		if(eps->getID() == id)
			return qSharedPointerCast<StelObject>(eps);
	}
	return Q_NULLPTR;
}

StelObjectP Exoplanets::searchByNameI18n(const QString& nameI18n) const
{
	if (!flagShowExoplanets)
		return Q_NULLPTR;

	for (const auto& eps : ep)
	{
		if (eps->getNameI18n().toUpper() == nameI18n.toUpper() || eps->getDesignation().toUpper() == nameI18n.toUpper())
			return qSharedPointerCast<StelObject>(eps);

		QStringList ppn = eps->getExoplanetsNamesI18n();
		if (!ppn.isEmpty())
		{
			for (const auto& str : ppn)
			{
				if (str.toUpper() == nameI18n.toUpper())
					return qSharedPointerCast<StelObject>(eps);
			}
		}
	}

	return Q_NULLPTR;
}

QStringList Exoplanets::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (!flagShowExoplanets || maxNbItem <= 0)
	{
		return result;
	}

	QStringList names;
	for (const auto& eps : ep)
	{
		names.append(eps->getNameI18n());
		names.append(eps->getExoplanetsNamesI18n());
		names.append(eps->getEnglishName());
		names.append(eps->getExoplanetsEnglishNames());
	}

	QString fullMatch = "";
	for (const auto& name : names)
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

QStringList Exoplanets::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (!flagShowExoplanets)
		return result;

	for (const auto& planet : ep)
		result << planet->getExoplanetsDesignations();

	if (inEnglish)
	{
		for (const auto& planet : ep)
			result << planet->getExoplanetsEnglishNames();
	}
	else
	{
		for (const auto& planet : ep)
			result << planet->getExoplanetsNamesI18n();
	}
	return result;
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void Exoplanets::restoreDefaultJsonFile(void)
{
	if (QFileInfo(jsonCatalogPath).exists())
		backupJsonFile(true);

	QFile src(":/Exoplanets/exoplanets.json");
	if (!src.copy(jsonCatalogPath))
	{
		qWarning() << "[Exoplanets] Cannot copy JSON resource to " + QDir::toNativeSeparators(jsonCatalogPath);
	}
	else
	{
		qDebug() << "[Exoplanets] Default exoplanets.json to " << QDir::toNativeSeparators(jsonCatalogPath);
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(jsonCatalogPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is schreduled in a timely
		// manner
		conf->remove("Exoplanets/last_update");
		lastUpdate = QDateTime::fromString("2012-05-24T12:00:00", Qt::ISODate);
	}
}

/*
  Creates a backup of the exoplanets.json file called exoplanets.json.old
*/
bool Exoplanets::backupJsonFile(bool deleteOriginal) const
{
	QFile old(jsonCatalogPath);
	if (!old.exists())
	{
		qWarning() << "[Exoplanets] No file to backup";
		return false;
	}

	QString backupPath = jsonCatalogPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "[Exoplanets] WARNING - could not remove old exoplanets.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "[Exoplanets] WARNING - failed to copy exoplanets.json to exoplanets.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of exoplanets.
*/
void Exoplanets::readJsonFile(void)
{
	setEPMap(loadEPMap());
}

void Exoplanets::reloadCatalog(void)
{
	bool hasSelection = false;
	StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);
	// Whether any exoplanet are selected? Save the current selection...
	const QList<StelObjectP> selectedObject = objMgr->getSelectedObject("Exoplanet");
	if (!selectedObject.isEmpty())
	{
		// ... unselect current exoplanet.
		hasSelection = true;
		objMgr->unSelect();
	}

	readJsonFile();

	if (hasSelection)
	{
		// Restore selection...
		objMgr->setSelectedObject(selectedObject);
	}
}

/*
  Parse JSON file and load exoplanets to map
*/
QVariantMap Exoplanets::loadEPMap(QString path)
{
	if (path.isEmpty())
	    path = jsonCatalogPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "[Exoplanets] Cannot open " << QDir::toNativeSeparators(path);
	else
	{
		try
		{
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();
		}
		catch (std::runtime_error &e)
		{
			qDebug() << "[Exoplanets] File format is wrong! Error: " << e.what();
			return QVariantMap();
		}
	}
	return map;
}

/*
  Set items for list of struct from data map
*/
void Exoplanets::setEPMap(const QVariantMap& map)
{
	StelCore* core = StelApp::getInstance().getCore();
	StarMgr* smgr = GETSTELMODULE(StarMgr);
	double ra, dec;
	StelObjectP star;
	ep.clear();
	PSCount = EPCountAll = EPCountPH = 0;
	EPEccentricityAll.clear();
	EPSemiAxisAll.clear();
	EPMassAll.clear();
	EPRadiusAll.clear();
	EPPeriodAll.clear();
	EPAngleDistanceAll.clear();
	QVariantMap epsMap = map.value("stars").toMap();
	for (auto epsKey : epsMap.keys())
	{
		QVariantMap epsData = epsMap.value(epsKey).toMap();
		epsData["designation"] = epsKey;

		PSCount++;

		// Let's check existence the star (by designation) in our catalog...
		star = smgr->searchByName(epsKey.trimmed());
		if (!star.isNull())
		{
			// ...if exists, let's use our coordinates of star instead exoplanets.eu website data
			StelUtils::rectToSphe(&ra, &dec, star->getJ2000EquatorialPos(core));
			epsData["RA"] = StelUtils::radToDecDegStr(ra, 6);
			epsData["DE"] = StelUtils::radToDecDegStr(dec, 6);
		}

		ExoplanetP eps(new Exoplanet(epsData));
		if (eps->initialized)
		{
			ep.append(eps);
			EPEccentricityAll.append(eps->getData(0));
			EPSemiAxisAll.append(eps->getData(1));
			EPMassAll.append(eps->getData(2));
			EPRadiusAll.append(eps->getData(3));
			EPPeriodAll.append(eps->getData(4));
			EPAngleDistanceAll.append(eps->getData(5));
			EPEffectiveTempHostStarAll.append(eps->getData(6));
			EPYearDiscoveryAll.append(eps->getData(7));
			EPMetallicityHostStarAll.append(eps->getData(8));
			EPVMagHostStarAll.append(eps->getData(9));
			EPRAHostStarAll.append(eps->getData(10));
			EPDecHostStarAll.append(eps->getData(11));
			EPDistanceHostStarAll.append(eps->getData(12));
			EPMassHostStarAll.append(eps->getData(13));
			EPRadiusHostStarAll.append(eps->getData(14));
			EPCountAll += eps->getCountExoplanets();
			EPCountPH += eps->getCountHabitableExoplanets();
		}
	}
}

int Exoplanets::getJsonFileFormatVersion(void) const
{
	int jsonVersion = -1;
	QFile jsonEPCatalogFile(jsonCatalogPath);
	if (!jsonEPCatalogFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Exoplanets] Cannot open " << QDir::toNativeSeparators(jsonCatalogPath);
		return jsonVersion;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&jsonEPCatalogFile).toMap();
		jsonEPCatalogFile.close();
	}
	catch (std::runtime_error &e)
	{
		qDebug() << "[Exoplanets] File format is wrong! Error: " << e.what();
		return jsonVersion;
	}
	if (map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}

	qDebug() << "[Exoplanets] Version of the format of the catalog:" << jsonVersion;
	return jsonVersion;
}

bool Exoplanets::checkJsonFileFormat() const
{
	QFile jsonEPCatalogFile(jsonCatalogPath);
	if (!jsonEPCatalogFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Exoplanets] Cannot open " << QDir::toNativeSeparators(jsonCatalogPath);
		return false;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&jsonEPCatalogFile).toMap();
		jsonEPCatalogFile.close();
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "[Exoplanets] File format is wrong! Error:" << e.what();
		return false;
	}

	return true;
}

ExoplanetP Exoplanets::getByID(const QString& id) const
{
	for (const auto& eps : ep)
	{
		if (eps->initialized && eps->designation == id)
			return eps;
	}
	return ExoplanetP();
}

bool Exoplanets::configureGui(bool show)
{
	if (show)
		exoplanetsConfigDialog->setVisible(true);
	return true;
}

void Exoplanets::restoreDefaults(void)
{
	resetConfiguration();
	restoreDefaultJsonFile();
	readJsonFile();
	loadConfiguration();
}

void Exoplanets::resetConfiguration(void)
{
	conf->beginGroup("Exoplanets");

	// delete all existing Exoplanets settings...
	conf->remove("");
	conf->endGroup();
	// Load the default values...
	loadConfiguration();
	// ... then save them.
	saveConfiguration();
}

void Exoplanets::loadConfiguration(void)
{
	conf->beginGroup("Exoplanets");

	updateUrl = conf->value("url", "https://www.stellarium.org/json/exoplanets.json").toString();
	updateFrequencyHours = conf->value("update_frequency_hours", 72).toInt();
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2012-05-24T12:00:00").toString(), Qt::ISODate);
	updatesEnabled = conf->value("updates_enabled", true).toBool();
	setDisplayMode(conf->value("distribution_enabled", false).toBool());
	setTimelineMode(conf->value("timeline_enabled", false).toBool());
	setHabitableMode(conf->value("habitable_enabled", false).toBool());
	enableAtStartup = conf->value("enable_at_startup", false).toBool();
	flagShowExoplanetsButton = conf->value("flag_show_exoplanets_button", true).toBool();
	setFlagShowExoplanetsDesignations(conf->value("flag_show_designations", true).toBool());
	setMarkerColor(Vec3f(conf->value("exoplanet_marker_color", "0.4,0.9,0.5").toString()));
	setHabitableColor(Vec3f(conf->value("habitable_exoplanet_marker_color", "1.0,0.5,0.0").toString()));
	setCurrentTemperatureScaleKey(conf->value("temperature_scale", "Celsius").toString());

	conf->endGroup();
}

void Exoplanets::saveConfiguration(void)
{
	conf->beginGroup("Exoplanets");

	conf->setValue("url", updateUrl);
	conf->setValue("update_frequency_hours", updateFrequencyHours);
	conf->setValue("updates_enabled", updatesEnabled );
	conf->setValue("distribution_enabled", getDisplayMode());
	conf->setValue("timeline_enabled", getTimelineMode());
	conf->setValue("habitable_enabled", getHabitableMode());
	conf->setValue("enable_at_startup", enableAtStartup);
	conf->setValue("flag_show_exoplanets_button", flagShowExoplanetsButton);
	conf->setValue("flag_show_designations", getFlagShowExoplanetsDesignations());
	conf->setValue("habitable_exoplanet_marker_color", getHabitableColor().toStr());
	conf->setValue("exoplanet_marker_color", getMarkerColor().toStr());
	conf->setValue("temperature_scale", getCurrentTemperatureScaleKey());

	conf->endGroup();
}

int Exoplanets::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyHours * 3600);
	return static_cast<int>(QDateTime::currentDateTime().secsTo(nextUpdate));
}

void Exoplanets::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyHours * 3600) <= QDateTime::currentDateTime() && networkManager->networkAccessible()==QNetworkAccessManager::Accessible)
		updateJSON();
}

void Exoplanets::updateJSON(void)
{
	if (updateState==Exoplanets::Updating)
	{
		qWarning() << "[Exoplanets] Already updating...  will not start again current update is complete.";
		return;
	}
	qDebug() << "[Exoplanets] Updating exoplanets catalog...";
	startDownload(updateUrl);
}

void Exoplanets::displayMessage(const QString& message, const QString hexColor)
{
	GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30, true, 16, hexColor, false, 9000);
}

void Exoplanets::upgradeConfigIni(void)
{
	// Upgrade settings for Exoplanets plugin
	if (conf->contains("Exoplanets/flag_show_exoplanets"))
	{
		bool b = conf->value("Exoplanets/flag_show_exoplanets", false).toBool();
		if (!conf->contains("Exoplanets/enable_at_startup"))
			conf->setValue("Exoplanets/enable_at_startup", b);
		conf->remove("Exoplanets/flag_show_exoplanets");
	}
}

// Define whether the button toggling exoplanets should be visible
void Exoplanets::setFlagShowExoplanetsButton(bool b)
{
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui!=Q_NULLPTR)
	{
		if (b==true)
		{
			if (toolbarButton==Q_NULLPTR) {
				// Create the exoplanets button
				toolbarButton = new StelButton(Q_NULLPTR,
							       QPixmap(":/Exoplanets/btExoplanets-on.png"),
							       QPixmap(":/Exoplanets/btExoplanets-off.png"),
							       QPixmap(":/graphicGui/miscGlow32x32.png"),
							       "actionShow_Exoplanets");
			}
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		} else {
			gui->getButtonBar()->hideButton("actionShow_Exoplanets");
		}
	}
	flagShowExoplanetsButton = b;
}

bool Exoplanets::getDisplayMode() const
{
	return Exoplanet::distributionMode;
}

void Exoplanets::setDisplayMode(bool b)
{
	Exoplanet::distributionMode=b;
}

bool Exoplanets::getFlagShowExoplanetsDesignations() const
{
	return Exoplanet::showDesignations;
}

void Exoplanets::setFlagShowExoplanetsDesignations(bool b)
{
	Exoplanet::showDesignations=b;
}

bool Exoplanets::getTimelineMode() const
{
	return Exoplanet::timelineMode;
}

void Exoplanets::setTimelineMode(bool b)
{
	Exoplanet::timelineMode=b;
}

bool Exoplanets::getHabitableMode() const
{
	return Exoplanet::habitableMode;
}

void Exoplanets::setHabitableMode(bool b)
{
	Exoplanet::habitableMode=b;
}

Vec3f Exoplanets::getMarkerColor() const
{
	return Exoplanet::exoplanetMarkerColor;
}

void Exoplanets::setMarkerColor(const Vec3f &c)
{
	Exoplanet::exoplanetMarkerColor = c;
	emit markerColorChanged(c);
}

Vec3f Exoplanets::getHabitableColor() const
{
	return Exoplanet::habitableExoplanetMarkerColor;
}

void Exoplanets::setHabitableColor(const Vec3f &c)
{
	Exoplanet::habitableExoplanetMarkerColor = c;
	emit habitableColorChanged(c);
}

void Exoplanets::setFlagShowExoplanets(bool b)
{
	if (b!=flagShowExoplanets)
	{
		flagShowExoplanets=b;
		emit flagExoplanetsVisibilityChanged(b);
		emit StelApp::getInstance().getCore()->updateSearchLists();
	}
}

void Exoplanets::setCurrentTemperatureScaleKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("TemperatureScale"));
	TemperatureScale ts = static_cast<TemperatureScale>(en.keyToValue(key.toLatin1().data()));
	if (ts<0)
	{
		qWarning() << "Unknown temperature scale:" << key << "setting \"Celsius\" instead";
		ts = Celsius;
	}
	setCurrentTemperatureScale(ts);
}

QString Exoplanets::getCurrentTemperatureScaleKey() const
{
	return metaObject()->enumerator(metaObject()->indexOfEnumerator("TemperatureScale")).key(Exoplanet::temperatureScaleID);
}

void Exoplanets::deleteDownloadProgressBar()
{
	disconnect(this, SLOT(updateDownloadProgress(qint64,qint64)));

	if (progressBar)
	{
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = Q_NULLPTR;
	}
}

void Exoplanets::startDownload(QString urlString)
{
	QUrl url(urlString);
	if (!url.isValid() || url.isRelative() || !url.scheme().startsWith("http", Qt::CaseInsensitive))
	{
		qWarning() << "[Exoplanets] Invalid URL:" << urlString;
		return;
	}

	if (progressBar == Q_NULLPTR)
		progressBar = StelApp::getInstance().addProgressBar();
	progressBar->setValue(0);
	progressBar->setRange(0, 0);

	connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	QNetworkRequest request;
	request.setUrl(QUrl(updateUrl));
	request.setRawHeader("User-Agent", StelUtils::getUserAgentString().toUtf8());
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
	downloadReply = networkManager->get(request);
	connect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDownloadProgress(qint64,qint64)));

	updateState = Exoplanets::Updating;
	emit(updateStateChanged(updateState));
}

void Exoplanets::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (progressBar == Q_NULLPTR)
		return;

	int currentValue = 0;
	int endValue = 0;

	if (bytesTotal > -1 && bytesReceived <= bytesTotal)
	{
		//Round to the greatest possible derived unit
		while (bytesTotal > 1024)
		{
			bytesReceived = static_cast<qint64>(std::floor(bytesReceived / 1024.));
			bytesTotal    = static_cast<qint64>(std::floor(bytesTotal / 1024.));
		}
		currentValue = static_cast<int>(bytesReceived);
		endValue = static_cast<int>(bytesTotal);
	}

	progressBar->setValue(currentValue);
	progressBar->setRange(0, endValue);
}

void Exoplanets::downloadComplete(QNetworkReply *reply)
{
	if (reply == Q_NULLPTR)
		return;

	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	deleteDownloadProgressBar();

	if (reply->error() || reply->bytesAvailable()==0)
	{
		qWarning() << "[Exoplanets] Download error: While trying to access"
			   << reply->url().toString()
			   << "the following error occured:"
			   << reply->errorString();

		reply->deleteLater();
		downloadReply = Q_NULLPTR;
		return;
	}

	// download completed successfully.
	try
	{
		QString jsonFilePath = StelFileMgr::findFile("modules/Exoplanets", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/exoplanets.json";
		QFile jsonFile(jsonFilePath);
		if (jsonFile.exists())
			jsonFile.remove();

		if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			jsonFile.write(reply->readAll());
			jsonFile.close();
		}

		updateState = Exoplanets::CompleteUpdates;

		lastUpdate = QDateTime::currentDateTime();
		conf->setValue("Exoplanets/last_update", lastUpdate.toString(Qt::ISODate));
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "[Exoplanets] Cannot write JSON data to file:" << e.what();
		updateState = Exoplanets::DownloadError;
	}

	emit(updateStateChanged(updateState));
	emit(jsonUpdateComplete());

	reply->deleteLater();
	downloadReply = Q_NULLPTR;

	readJsonFile();
}

void Exoplanets::translations()
{
#if 0
	// TRANSLATORS: Habitable zone
	N_("Hot");
	// TRANSLATORS: Habitable zone
	N_("Warm");
	// TRANSLATORS: Habitable zone
	N_("Cold");

	// TRANSLATORS: Planet size
	N_("Miniterran");
	// TRANSLATORS: Planet size
	N_("Subterran");
	// TRANSLATORS: Planet size
	N_("Terran");
	// TRANSLATORS: Planet size
	N_("Superterran");
	// TRANSLATORS: Planet size
	N_("Jovian");
	// TRANSLATORS: Planet size
	N_("Neptunian");

	// TRANSLATORS: Exoplanet detection method
	N_("Primary Transit");
	// TRANSLATORS: Exoplanet detection method
	N_("Microlensing");
	// TRANSLATORS: Exoplanet detection method
	N_("Radial Velocity");
	// TRANSLATORS: Exoplanet detection method
	N_("Imaging");
	// TRANSLATORS: Exoplanet detection method
	N_("Pulsar");
	// TRANSLATORS: Exoplanet detection method
	N_("Other");
	// TRANSLATORS: Exoplanet detection method
	N_("Astrometry");
	// TRANSLATORS: Exoplanet detection method. TTV=Transit Timing Variation
	N_("TTV");
	// TRANSLATORS: Exoplanet detection method
	N_("Timing");
	// TRANSLATORS: Exoplanet detection method. TTV=Transit Timing Variation
	N_("Primary Transit, TTV");
	// TRANSLATORS: Exoplanet detection method
	N_("Default");

	/* For copy/paste:
	// TRANSLATORS:
	N_("");
	*/

#endif
}
