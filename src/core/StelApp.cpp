/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#include "StelApp.hpp"

#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelTextureMgr.hpp"
#include "LoadingBar.hpp"
#include "StelObjectMgr.hpp"

#include "TelescopeMgr.hpp"
#include "ConstellationMgr.hpp"
#include "NebulaMgr.hpp"
#include "LandscapeMgr.hpp"
#include "GridLinesMgr.hpp"
#include "MilkyWay.hpp"
#include "MeteorMgr.hpp"
#include "LabelMgr.hpp"
#include "ScreenImageMgr.hpp"
#include "StarMgr.hpp"
#include "SolarSystem.hpp"
#include "StelIniParser.hpp"
#include "Projector.hpp"
#include "LocationMgr.hpp"
#include "DownloadMgr.hpp"

#include "StelModuleMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "MovementMgr.hpp"
#include "StelFileMgr.hpp"
#include "QtScriptMgr.hpp"
#include "QtJsonParser.hpp"
#include "SkyImageMgr.hpp"

#include "StelStyle.hpp"

#include <iostream>
#include <QStringList>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QTextStream>
#include <QMouseEvent>
#include <QDebug>
#include <QNetworkAccessManager>

// Initialize static variables
StelApp* StelApp::singleton = NULL;
QTime* StelApp::qtime = NULL;

/*************************************************************************
 Create and initialize the main Stellarium application.
*************************************************************************/
StelApp::StelApp(int argc, char** argv, QObject* parent)
	: QObject(parent), core(NULL), fps(0), maxfps(10000.f), frame(0), 
	  timefr(0.), timeBase(0.), flagNightVision(false), 
	  configFile("config.ini"), startupScript("startup.ssc"), 
	  confSettings(NULL), initialized(false), saveProjW(-1), 
	  saveProjH(-1)
{
	// Used for getting system date formatting
	setlocale(LC_TIME, "");
	// We need scanf()/printf() and friends to always work in the C locale,
	// otherwise configuration/INI file parsing will be erroneous.
	setlocale(LC_NUMERIC, "C");
	
	setObjectName("StelApp");
	
	skyCultureMgr=NULL;
	localeMgr=NULL;
	fontManager=NULL;
	stelObjectMgr=NULL;
	textureMgr=NULL;
	moduleMgr=NULL;
	loadingBar=NULL;
	networkAccessManager=NULL;
	
	// Can't create 2 StelApp instances
	Q_ASSERT(!singleton);
	singleton = this;
	
	argList = new QStringList;
	for(int i=0; i<argc; i++)
		*argList << argv[i];
	
	stelFileMgr = new StelFileMgr();
	
	StelApp::qtime = new QTime();
	StelApp::qtime->start();
	
	// Parse for first set of CLI arguments - stuff we want to process before other
	// output, such as --help and --version, and if we want to set the configFile value.
	parseCLIArgsPreConfig();

	// Load language codes
	try
	{
		Translator::init(stelFileMgr->findFile("data/iso639-1.utf8"));
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "ERROR while loading translations: " << e.what() << endl;
	}
	
	// OK, print the console splash and get on with loading the program
	std::cout << " -------------------------------------------------------" << std::endl;
	std::cout << "[ This is " << qPrintable(StelApp::getApplicationName()) << " - http://www.stellarium.org ]" << std::endl;
	std::cout << "[ Copyright (C) 2000-2008 Fabien Chereau et al          ]" << std::endl;
	std::cout << " -------------------------------------------------------" << std::endl;
	
	QStringList p=stelFileMgr->getSearchPaths();
	qDebug() << "File search paths:";
	int n=0;
	foreach (QString i, p)
	{
		qDebug() << " " << n << ". " << i;
		++n;
	}
	qDebug() << "Config file is: " << configFile;
	
	// implement "restore default settings" feature.
	bool restoreDefaults = false;
	if (stelFileMgr->exists(configFile))
	{
		QSettings* tmpSettings = new QSettings(configFile, StelIniFormat);
		restoreDefaults = tmpSettings->value("main/restore_defaults", false).toBool();
		delete tmpSettings;
	}

	if (restoreDefaults)
	{
		QFile(configFile).remove();
		qDebug() << "DELETING old config.ini";
	}

	if (!stelFileMgr->exists(configFile))
	{
		qDebug() << "config file " << configFile << " does not exist - copying the default file.";
		copyDefaultConfigFile();
	}

	// Load the configuration file
	confSettings = new QSettings(getConfigFilePath(), StelIniFormat, this);
	
	// Main section
	QString version = confSettings->value("main/version").toString();
	
	if (version.isEmpty())
	{
		qWarning() << "Found an invalid config file. Overwrite with default.";
		delete confSettings;
		QFile::remove(getConfigFilePath());
		copyDefaultConfigFile();
		confSettings = new QSettings(getConfigFilePath(), StelIniFormat);
		// get the new version value from the updated config file
		version = confSettings->value("main/version").toString();
	}
	
	if (version!=QString(PACKAGE_VERSION))
	{
		QTextStream istr(&version);
		char tmp;
		int v1 =0;
		int v2 =0;
		istr >> v1 >> tmp >> v2;

		// Config versions less than 0.6.0 are not supported, otherwise we will try to use it
		if(v1==0 && v2<6)
		{
			// The config file is too old to try an importation
			qDebug() << "The current config file is from a version too old for parameters to be imported (" 
			         << (version.isEmpty() ? "<0.6.0" : version) << ").\n"
			         << "It will be replaced by the default config file.";

			delete confSettings;
			QFile::remove(getConfigFilePath());
			copyDefaultConfigFile();
			confSettings = new QSettings(getConfigFilePath(), StelIniFormat);
		}
		else
		{
			qDebug() << "Attempting to use an existing older config file.";
		}
	}
	
	parseCLIArgsPostConfig();
	moduleMgr = new StelModuleMgr();
	
	maxfps = confSettings->value("video/maximum_fps",10000.).toDouble();
	minfps = confSettings->value("video/minimum_fps",10000.).toDouble();
	
	// Init a default StelStyle, before loading modules, it will be overrided
	currentStelStyle = NULL;
	setColorScheme("color");
}

/*************************************************************************
 Deinitialize and destroy the main Stellarium application.
*************************************************************************/
StelApp::~StelApp()
{
	delete scriptMgr; scriptMgr=NULL;
	delete loadingBar; loadingBar=NULL;
	delete core; core=NULL;
	delete skyCultureMgr; skyCultureMgr=NULL;
	delete localeMgr; localeMgr=NULL;
	delete fontManager; fontManager=NULL;
	delete stelObjectMgr; stelObjectMgr=NULL;
	delete stelFileMgr; stelFileMgr=NULL;
	delete moduleMgr; moduleMgr=NULL;	// Also delete all modules
	delete textureMgr; textureMgr=NULL;
	delete planetLocationMgr; planetLocationMgr=NULL;
	delete argList; argList=NULL;
	
	delete currentStelStyle;
	
	Q_ASSERT(singleton);
	singleton = NULL;
}

/*************************************************************************
 Return the full name of stellarium, i.e. "stellarium 0.9.0"
*************************************************************************/
QString StelApp::getApplicationName()
{
	return QString("Stellarium")+" "+PACKAGE_VERSION;
}


void StelApp::init()
{
	networkAccessManager = new QNetworkAccessManager(this);
	core = new StelCore();
	if (saveProjW!=-1 && saveProjH!=-1)
		core->windowHasBeenResized(saveProjW, saveProjH);
	textureMgr = new StelTextureMgr();
	localeMgr = new StelLocaleMgr();
	fontManager = new StelFontMgr();
	skyCultureMgr = new StelSkyCultureMgr();
	planetLocationMgr = new LocationMgr();
	
	timeMultiplier = 1;
	
	// Initialize AFTER creation of openGL context
	textureMgr->init();

	loadingBar = new LoadingBar(12., "logo24bitsbeta.png", PACKAGE_VERSION, 45, 320, 121);
	
	downloadMgr = new DownloadMgr();
	
	// Stel Object Data Base manager
	stelObjectMgr = new StelObjectMgr();
	stelObjectMgr->init();
	getModuleMgr().registerModule(stelObjectMgr);
	
	localeMgr->init();
	skyCultureMgr->init();
	
	// Init the solar system first
	SolarSystem* ssystem = new SolarSystem();
	ssystem->init();
	getModuleMgr().registerModule(ssystem);
	
	// Load hipparcos stars & names
	StarMgr* hip_stars = new StarMgr();
	hip_stars->init();
	getModuleMgr().registerModule(hip_stars);	
	
	core->init();

	// Init nebulas
	NebulaMgr* nebulas = new NebulaMgr();
	nebulas->init();
	getModuleMgr().registerModule(nebulas);
	
	// Init milky way
	MilkyWay* milky_way = new MilkyWay();
	milky_way->init();
	getModuleMgr().registerModule(milky_way);
	
	// Init sky image manager
	skyImageMgr = new SkyImageMgr();
	skyImageMgr->init();
	getModuleMgr().registerModule(skyImageMgr);
	
	// Telescope manager
	TelescopeMgr* telescope_mgr = new TelescopeMgr();
	telescope_mgr->init();
	getModuleMgr().registerModule(telescope_mgr);
	
	// Constellations
	ConstellationMgr* asterisms = new ConstellationMgr(hip_stars);
	asterisms->init();
	getModuleMgr().registerModule(asterisms);
	
	// Landscape, atmosphere & cardinal points section
	LandscapeMgr* landscape = new LandscapeMgr();
	landscape->init();
	getModuleMgr().registerModule(landscape);

	GridLinesMgr* gridLines = new GridLinesMgr();
	gridLines->init();
	getModuleMgr().registerModule(gridLines);
	
	// Meteors
	MeteorMgr* meteors = new MeteorMgr(10, 60);
	meteors->init();
	getModuleMgr().registerModule(meteors);

	// User labels
	LabelMgr* skyLabels = new LabelMgr();
	skyLabels->init();
	getModuleMgr().registerModule(skyLabels);

	// Scripting images
	ScreenImageMgr* scriptImages = new ScreenImageMgr();
	scriptImages->init();
	getModuleMgr().registerModule(scriptImages);

// ugly fix by Johannes: call skyCultureMgr->init twice so that
// star names are loaded again
	skyCultureMgr->init();
	
	// Initialisation of the color scheme
	flagNightVision=true;  // fool caching
	setVisionModeNight(false);
	setVisionModeNight(confSettings->value("viewing/flag_night").toBool());
	
	// Generate dependency Lists for all modules
	moduleMgr->generateCallingLists();
	
	updateI18n();
	
	scriptMgr = new QtScriptMgr(startupScript);
	initialized = true;
}

// Load and initialize external modules (plugins)
void StelApp::initPlugIns()
{
	// Load dynamically all the modules found in the modules/ directories
	// which are configured to be loaded at startup
	foreach (StelModuleMgr::PluginDescriptor i, moduleMgr->getPluginsList())
	{
		if (i.loadAtStartup==false)
			continue;
		StelModule* m = moduleMgr->loadPlugin(i.key);
		if (m!=NULL)
		{
			moduleMgr->registerModule(m, true);
			m->init();
		}
	}
}
	
void StelApp::parseCLIArgsPreConfig(void)
{	
	if (argsGetOption(argList, "-v", "--version"))
	{
		std::cout << qPrintable(getApplicationName()) << std::endl;
		exit(0);
	}

	if (argsGetOption(argList, "-h", "--help"))
	{
		// Get the basename of binary
		QString binName = argList->at(0);
		binName.remove(QRegExp("^.*[/\\\\]"));
		
		std::cout << "Usage:" << std::endl
		     << "  " 
		     << qPrintable(binName) << " [options]" << std::endl << std::endl
		     << "Options:" << std::endl
		     << "--version (or -v)       : Print program name and version and exit." << std::endl
		     << "--help (or -h)          : This cruft." << std::endl
		     << "--config-file (or -c)   : Use an alternative name for the config file" << std::endl
		     << "--user-dir (or -u)      : Use an alternative user data directory" << std::endl
		     << "--full-screen (or -f)   : With argument \"yes\" or \"no\" over-rides" << std::endl
		     << "                          the full screen setting in the config file" << std::endl
		     << "--screenshot-dir        : Specify directory to save screenshots" << std::endl
		     << "--startup-script        : Specify name of startup script" << std::endl
		     << "--home-planet           : Specify observer planet (English name)" << std::endl
		     << "--altitude              : Specify observer altitude in meters" << std::endl
		     << "--longitude             : Specify longitude, e.g. +53d58\\'16.65\\\"" << std::endl
		     << "--latitude              : Specify latitude, e.g. -1d4\\'27.48\\\"" << std::endl 
		     << "--list-landscapes       : Print a list of value landscape IDs" << std::endl 
		     << "--landscape             : Start using landscape whose ID (dir name)" << std::endl
		     << "                          is passed as parameter to option" << std::endl
		     << "--sky-date              : Specify sky date in format yyyymmdd" << std::endl
		     << "--sky-time              : Specify sky time in format hh:mm:ss" << std::endl
		     << "--fov                   : Specify the field of view (degrees)" << std::endl
		     << "--projection-type       : Specify projection type, e.g. stereographic" << std::endl
		     << "--restore-defaults      : Delete existing config.ini and use defaults" << std::endl;
		exit(0);
	}
	
	if (argsGetOption(argList, "", "--list-landscapes"))
	{
		QSet<QString> landscapeIds = stelFileMgr->listContents("landscapes", StelFileMgr::Directory);
		for(QSet<QString>::iterator i=landscapeIds.begin(); i!=landscapeIds.end(); ++i)
		{
			try 
			{
				// finding the file will throw an exception if it is not found
				// in that case we won't output the landscape ID as it canont work
				stelFileMgr->findFile("landscapes/" + *i + "/landscape.ini");
				std::cout << qPrintable(*i) << std::endl;
			}
			catch (std::runtime_error& e){}
		}
		exit(0);
	}

	try
	{
		QString newUserDir;
		newUserDir = argsGetOptionWithArg<QString>(argList, "-u", "--user-dir", "");
		if (newUserDir!="" && !newUserDir.isEmpty())
			stelFileMgr->setUserDir(newUserDir);
	}
	catch (std::runtime_error& e)
	{
		qCritical() << "ERROR: while processing --user-dir option: " << e.what();
		exit(1);
	}
	
	// If the chosen user directory does not exist we will create it
	if (!StelFileMgr::exists(stelFileMgr->getUserDir()))
	{
		if (!StelFileMgr::mkDir(stelFileMgr->getUserDir()))
		{
			qCritical() << "ERROR - cannot create non-existent user directory" << stelFileMgr->getUserDir();
			exit(1);
		}
	}
	
	bool restoreDefaultConfigFile = false;
	if (argsGetOption(argList, "", "--restore-defaults"))
		restoreDefaultConfigFile=true;
	
	try
	{
		setConfigFile(argsGetOptionWithArg<QString>(argList, "-c", "--config-file", "config.ini"), restoreDefaultConfigFile);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: while looking for --config-file option: " << e.what() << ". Using \"config.ini\"";
		setConfigFile("config.ini", restoreDefaultConfigFile);		
	}

	startupScript = argsGetOptionWithArg<QString>(argList, "", "--startup-script", "startup.ssc");
}

void StelApp::parseCLIArgsPostConfig()
{
	// Over-ride config file options with command line options
	// We should catch exceptions from argsGetOptionWithArg...
	int fullScreen, altitude;
	float fov;
	QString landscapeId, homePlanet, longitude, latitude, skyDate, skyTime, projectionType, screenshotDir;
	
	try
	{
		fullScreen = argsGetYesNoOption(argList, "-f", "--full-screen", -1);
		landscapeId = argsGetOptionWithArg<QString>(argList, "", "--landscape", "");
		homePlanet = argsGetOptionWithArg<QString>(argList, "", "--home-planet", "");
		altitude = argsGetOptionWithArg<int>(argList, "", "--altitude", -1);
		longitude = argsGetOptionWithArg<QString>(argList, "", "--longitude", "");
		latitude = argsGetOptionWithArg<QString>(argList, "", "--latitude", "");
		skyDate = argsGetOptionWithArg<QString>(argList, "", "--sky-date", "");
		skyTime = argsGetOptionWithArg<QString>(argList, "", "--sky-time", "");
		fov = argsGetOptionWithArg<float>(argList, "", "--fov", -1.0);
		projectionType = argsGetOptionWithArg<QString>(argList, "", "--projection-type", "");
		screenshotDir = argsGetOptionWithArg<QString>(argList, "", "--screenshot-dir", "");
	}
	catch (std::runtime_error& e)
	{
		qCritical() << "ERROR while checking command line options: " << e.what();
		exit(0);
	}

	// Will be -1 if option is not found, in which case we don't change anything.
	if (fullScreen == 1) confSettings->setValue("video/fullscreen", true);
	else if (fullScreen == 0) confSettings->setValue("video/fullscreen", false);
	
	if (landscapeId != "") confSettings->setValue("init_location/landscape_name", landscapeId);
	
	if (homePlanet != "") confSettings->setValue("init_location/home_planet", homePlanet);
	
	if (altitude != -1) confSettings->setValue("init_location/altitude", altitude);
	
	QRegExp longLatRx("[\\-+]?\\d+d\\d+\\'\\d+(\\.\\d+)?\"");
	if (longitude != "")
	{
		if (longLatRx.exactMatch(longitude))
			confSettings->setValue("init_location/longitude", longitude);
		else
			qWarning() << "WARNING: --longitude argument has unrecognised format";
	}
	
	if (latitude != "")
	{
		if (longLatRx.exactMatch(latitude))
			confSettings->setValue("init_location/latitude", latitude);
		else
			qWarning() << "WARNING: --latitude argument has unrecognised format";
	}
	
	if (skyDate != "" || skyTime != "")
	{
		// Get the Julian date for the start of the current day
		// and the extra necessary for the time of day as separate
		// components.  Then if the --sky-date and/or --sky-time flags
		// are set we over-ride the component, and finally add them to 
		// get the full julian date and set that.
		
		// First, lets determine the Julian day number and the part for the time of day
		QDateTime now = QDateTime::currentDateTime();
		double skyDatePart = now.date().toJulianDay();
		double skyTimePart = StelUtils::qTimeToJDFraction(now.time());
		
		// Over-ride the sktDatePart if the user specified the date using --sky-date
		if (skyDate != "")
		{
			// validate the argument format, we will tolerate yyyy-mm-dd by removing all -'s
			QRegExp dateRx("\\d{8}");
			if (dateRx.exactMatch(skyDate.remove("-")))
				skyDatePart = QDate::fromString(skyDate, "yyyyMMdd").toJulianDay();
			else
				qWarning() << "WARNING: --sky-date argument has unrecognised format  (I want yyyymmdd)";
		}
		
		if (skyTime != "")
		{
			QRegExp timeRx("\\d{1,2}:\\d{2}:\\d{2}");
			if (timeRx.exactMatch(skyTime))
				skyTimePart = StelUtils::qTimeToJDFraction(QTime::fromString(skyTime, "hh:mm:ss"));
			else
				qWarning() << "WARNING: --sky-time argument has unrecognised format (I want hh:mm:ss)";
		}

		confSettings->setValue("navigation/startup_time_mode", "preset");
		confSettings->setValue("navigation/preset_sky_time", skyDatePart + skyTimePart);
	}

	if (fov > 0.0) confSettings->setValue("navigation/init_fov", fov);
	
	if (projectionType != "") confSettings->setValue("projection/type", projectionType);

	if (screenshotDir!="")
	{
		try
		{
			QString newShotDir = argsGetOptionWithArg<QString>(argList, "", "--screenshot-dir", "");
			if (!newShotDir.isEmpty() && newShotDir!="")
				stelFileMgr->setScreenshotDir(newShotDir);
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "WARNING: problem while setting screenshot directory for --screenshot-dir option: " << e.what();
		}
	}
	else
	{
		QString confScreenshotDir = confSettings->value("main/screenshot_dir", "").toString();
		if (confScreenshotDir!="")
		{
			try
			{
				stelFileMgr->setScreenshotDir(confScreenshotDir);
			}
			catch (std::runtime_error& e)
			{
				qWarning() << "WARNING: problem while setting screenshot from config file setting: " << e.what();
			}
		}
	}
}

void StelApp::update(double deltaTime)
{
     if (!initialized)
        return;
	
	++frame;
	timefr+=deltaTime;
	if (timefr-timeBase > 1.)
	{
		// Calc the FPS rate every seconds
		fps=(double)frame/(timefr-timeBase);
		frame = 0;
		timeBase+=1.;
	}

	// change time rate if needed to fast forward scripts
	deltaTime *= timeMultiplier;

	core->update(deltaTime);
	
	moduleMgr->update();
	
	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionUpdate))
	{
		i->update(deltaTime);
	}
	
	stelObjectMgr->update(deltaTime);
}

//! Main drawing function called at each frame
void StelApp::draw()
{
	if (!initialized)
		return;

	core->preDraw();

	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionDraw))
	{
		i->draw(core);
	}

	core->postDraw();
}

/*************************************************************************
 Call this when the size of the GL window has changed
*************************************************************************/
void StelApp::glWindowHasBeenResized(int w, int h)
{
	if (core)
		core->windowHasBeenResized(w, h);
	else
	{
		saveProjW = w;
		saveProjH = h;
	}
	// Send the event to every StelModule
	foreach (StelModule* iter, moduleMgr->getAllModules())
	{
		iter->glWindowHasBeenResized(w, h);
	}
}

// Handle mouse clics
void StelApp::handleClick(QMouseEvent* event)
{
	event->setAccepted(false);
	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHangleMouseClicks))
	{
		i->handleMouseClicks(event);
		if (event->isAccepted())
			return;
	}
}

// Handle mouse wheel.
void StelApp::handleWheel(QWheelEvent* event)
{
	event->setAccepted(false);
	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHangleMouseClicks))
	{
		i->handleMouseWheel(event);
		if (event->isAccepted())
			return;
	}
}
	
// Handle mouse move
void StelApp::handleMove(int x, int y, Qt::MouseButtons b)
{
	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHandleMouseMoves))
	{
		if (i->handleMouseMoves(x, y, b))
			return;
	}
}

// Handle key press and release
void StelApp::handleKeys(QKeyEvent* event)
{
	event->setAccepted(false);
	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHandleKeys))
	{
		i->handleKeys(event);
		if (event->isAccepted())
			return;
	}
}


void StelApp::setConfigFile(const QString& configName, bool restoreDefaults)
{
	try
	{
		configFile = stelFileMgr->findFile(configName, StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::File));
		if (restoreDefaults == true)
		{
			QString backupFile(configFile.left(configFile.length()-3) + QString("old"));

			if (QFileInfo(backupFile).exists())
				QFile(backupFile).remove();

			QFile(configFile).rename(backupFile);
			qDebug() << "setting defaults - old config is backed up in " << backupFile;
		}
		return;
	}
	catch (std::runtime_error& e)
	{
		//qDebug() << "DEBUG StelApp::setConfigFile could not locate writable config file " << configName;
	}
	
	try
	{
		configFile = stelFileMgr->findFile(configName, StelFileMgr::File);	
		return;
	}
	catch (std::runtime_error& e)
	{
		//qDebug() << "DEBUG StelApp::setConfigFile could not find read only config file " << configName;
	}		
	
	try
	{
		configFile = stelFileMgr->findFile(configName, StelFileMgr::New);
		//qDebug() << "DEBUG StelApp::setConfigFile found NEW file path: " << configFile;
		return;
	}
	catch (std::runtime_error& e)
	{
		qCritical() << "ERROR StelApp::setConfigFile could not find or create configuration file " << configName;
		exit(1);
	}
}

void StelApp::copyDefaultConfigFile()
{
	QString defaultConfigFilePath;
	try
	{
		defaultConfigFilePath = stelFileMgr->findFile("data/default_config.ini");
	}
	catch (std::runtime_error& e)
	{
		qCritical() << "ERROR StelApp::copyDefaultConfigFile failed to locate data/default_config.ini.  Please check your installation.";
		exit(1);
	}
	
	QFile::copy(defaultConfigFilePath, configFile);
	if (!stelFileMgr->exists(configFile))
	{
		qCritical() << "ERROR StelApp::copyDefaultConfigFile failed to copy file " << defaultConfigFilePath 
		         << " to " << configFile << ". You could try to copy it by hand.";
		exit(1);
	}
}

// Set the colorscheme for all the modules
void StelApp::setColorScheme(const QString& section)
{
	if (!currentStelStyle)
		currentStelStyle = new StelStyle;
	
	currentStelStyle->confSectionName = section;
	
	QString qtStyleFileName;
	QString htmlStyleFileName;
	
	if (section=="night_color")
	{	
		qtStyleFileName = "data/gui/nightStyle.css";
		htmlStyleFileName = "data/gui/nightHtml.css";
	}
	else if (section=="color")
	{
		qtStyleFileName = "data/gui/normalStyle.css";
		htmlStyleFileName = "data/gui/normalHtml.css";
	}
	
	// Load Qt style sheet
	StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
	QString styleFilePath;
	try
	{
		styleFilePath = fileMan.findFile(qtStyleFileName);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: can't find Qt style sheet:" << qtStyleFileName;
	}
	QFile styleFile(styleFilePath);
	styleFile.open(QIODevice::ReadOnly);
	currentStelStyle->qtStyleSheet = styleFile.readAll();
	
	// Load HTML style sheet
	try
	{
		styleFilePath = fileMan.findFile(htmlStyleFileName);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: can't find css:" << htmlStyleFileName;
	}
	QFile htmlStyleFile(styleFilePath);
	htmlStyleFile.open(QIODevice::ReadOnly);
	currentStelStyle->htmlStyleSheet = htmlStyleFile.readAll();
	
	// Send the event to every StelModule
	foreach (StelModule* iter, moduleMgr->getAllModules())
	{
		iter->setStelStyle(*currentStelStyle);
	}
}

//! Set flag for activating night vision mode
void StelApp::setVisionModeNight(bool b)
{
	if (flagNightVision!=b)
	{
		flagNightVision=b;
		setColorScheme(b ? "night_color" : "color");
	}
}

// Update translations and font for sky everywhere in the program
void StelApp::updateI18n()
{
	// Send the event to every StelModule
	foreach (StelModule* iter, moduleMgr->getAllModules())
	{
		iter->updateI18n();
	}
}

// Update and reload sky culture informations everywhere in the program
void StelApp::updateSkyCulture()
{
	// Send the event to every StelModule
	foreach (StelModule* iter, moduleMgr->getAllModules())
	{
		iter->updateSkyCulture();
	}
}

bool StelApp::argsGetOption(QStringList* args, QString shortOpt, QString longOpt)
{
	bool result=false;

        // Don't see anything after a -- as an option
	int lastOptIdx = args->indexOf("--");
	if (lastOptIdx == -1)
		lastOptIdx = args->size();

	for(int i=0; i<lastOptIdx; i++)
	{
		if ((shortOpt!="" && args->at(i) == shortOpt) || args->at(i) == longOpt)
		{
			result = true;
			i=args->size();
		}
	}

	return result;
}

template<class T>
T StelApp::argsGetOptionWithArg(QStringList* args, QString shortOpt, QString longOpt, T defaultValue)
{
	// Don't see anything after a -- as an option
	int lastOptIdx = args->indexOf("--");
	if (lastOptIdx == -1)
		lastOptIdx = args->size();

	for(int i=0; i<lastOptIdx; i++)
	{
		bool match(false);
		QString argStr("");

		// form -n=arg
		if ((shortOpt!="" && args->at(i).left(shortOpt.length()+1)==shortOpt+"="))
		{
			match=true;
			argStr=args->at(i).right(args->at(i).length() - shortOpt.length() - 1);
		}
		// form --number=arg
		else if (args->at(i).left(longOpt.length()+1)==longOpt+"=")
		{
			match=true;
			argStr=args->at(i).right(args->at(i).length() - longOpt.length() - 1);
		}
		// forms -n arg and --number arg
		else if ((shortOpt!="" && args->at(i)==shortOpt) || args->at(i)==longOpt)
		{
			if (i+1>=lastOptIdx)
			{
				throw (std::runtime_error(qPrintable("optarg_missing ("+longOpt+")")));
			}
			else
			{
				match=true;
				argStr=args->at(i+1);
				i++;  // skip option argument in next iteration 
			}
		}

		if (match)
		{
			T retVal;
			QTextStream converter(qPrintable(argStr));
			converter >> retVal;
			if (converter.status() != QTextStream::Ok)
				throw (std::runtime_error(qPrintable("optarg_type ("+longOpt+")")));
			else
				return retVal;
		}
	}

	return defaultValue;
}

int StelApp::argsGetYesNoOption(QStringList* args, QString shortOpt, QString longOpt, int defaultValue)
{
	QString strArg = argsGetOptionWithArg<QString>(args, shortOpt, longOpt, "");
	if (strArg == "")
	{
		return defaultValue;
	}
	if (strArg.compare("yes", Qt::CaseInsensitive)==0
		   || strArg.compare("y", Qt::CaseInsensitive)==0
		   || strArg.compare("true", Qt::CaseInsensitive)==0
		   || strArg.compare("t", Qt::CaseInsensitive)==0
		   || strArg.compare("on", Qt::CaseInsensitive)==0
		   || strArg=="1")
	{
		return 1;
	}
	else if (strArg.compare("no", Qt::CaseInsensitive)==0
			|| strArg.compare("n", Qt::CaseInsensitive)==0
			|| strArg.compare("false", Qt::CaseInsensitive)==0
			|| strArg.compare("f", Qt::CaseInsensitive)==0
			|| strArg.compare("off", Qt::CaseInsensitive)==0
			|| strArg=="0")
	{
		return 0;
	}
	else
	{
		throw (std::runtime_error("optarg_type"));
	}
}

// Return the time since when stellarium is running in second.
double StelApp::getTotalRunTime()
{
	return (double)StelApp::qtime->elapsed()/1000;
}
