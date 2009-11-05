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
#include "StelLoadingBar.hpp"
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
#include "StelProjector.hpp"
#include "StelLocationMgr.hpp"
#include "StelDownloadMgr.hpp"

#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelScriptMgr.hpp"
#include "StelMainScriptAPIProxy.hpp"
#include "StelJsonParser.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelAudioMgr.hpp"
#include "StelMainWindow.hpp"
#include "StelStyle.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelGuiBase.hpp"
#include "StelPainter.hpp"

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
#include <QSysInfo>
#include <QNetworkProxy>
#include <QMessageBox>

#ifdef WIN32
#include <windows.h>
#endif

// Initialize static variables
StelApp* StelApp::singleton = NULL;
QTime* StelApp::qtime = NULL;
QFile StelApp::logFile;
QString StelApp::log;

void StelApp::initStatic()
{
	StelApp::qtime = new QTime();
	StelApp::qtime->start();
}

/*************************************************************************
 Create and initialize the main Stellarium application.
*************************************************************************/
StelApp::StelApp(int argc, char** argv, QObject* parent)
	: QObject(parent), core(NULL), stelGui(NULL), fps(0), maxfps(10000.f),
	  frame(0), timefr(0.), timeBase(0.), flagNightVision(false),
	  configFile("config.ini"), startupScript("startup.ssc"),
	  confSettings(NULL), initialized(false), saveProjW(-1), saveProjH(-1),
	  drawState(0)
{
	// Stat variables
	nbDownloadedFiles=0;
	totalDownloadedSize=0;
	nbUsedCache=0;
	totalUsedCacheSize=0;

	// Used for getting system date formatting
	setlocale(LC_TIME, "");
	// We need scanf()/printf() and friends to always work in the C locale,
	// otherwise configuration/INI file parsing will be erroneous.
	setlocale(LC_NUMERIC, "C");

	setObjectName("StelApp");

	skyCultureMgr=NULL;
	localeMgr=NULL;
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
	{
		*argList << argv[i];
		// qDebug() << "adding argument:\"" << argv[i] << "\"";
	}

	// Echo debug output to log file
	stelFileMgr = new StelFileMgr();
	logFile.setFileName(stelFileMgr->getUsersDataDirectoryName()+"/log.txt");
	QDir userDirTmp(stelFileMgr->getUserDir());
	if (!userDirTmp.exists())
	{
		// Try to create it
		StelFileMgr::mkDir(stelFileMgr->getUserDir());
	}
	if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text | QIODevice::Unbuffered))
		qInstallMsgHandler(StelApp::debugLogHandler);

	// Print system info to log file
	setupLog();

	// Parse for first set of CLI arguments - stuff we want to process before other
	// output, such as --help and --version, and if we want to set the configFile value.
	parseCLIArgsPreConfig();

	// Load language codes
	try
	{
		StelTranslator::init(stelFileMgr->findFile("data/iso639-1.utf8"));
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "ERROR while loading translations: " << e.what() << endl;
	}

	// OK, print the console splash and get on with loading the program
	QString versionLine = QString("This is %1 - http://www.stellarium.org").arg(StelApp::getApplicationName());
	QString copyrightLine = QString("Copyright (C) 2000-2009 Fabien Chereau et al");
	int maxLength = qMax(versionLine.size(), copyrightLine.size());
	qDebug() << qPrintable(QString(" %1").arg(QString().fill('-', maxLength+2)));
	qDebug() << qPrintable(QString("[ %1 ]").arg(versionLine.leftJustified(maxLength, ' ')));
	qDebug() << qPrintable(QString("[ %1 ]").arg(copyrightLine.leftJustified(maxLength, ' ')));
	qDebug() << qPrintable(QString(" %1").arg(QString().fill('-', maxLength+2)));
	if (logFile.isOpen())
		qDebug() << "Writing log file to:" << logFile.fileName();
	else
		qDebug() << "Unable to open log file:" << logFile.fileName() << ".";

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
	qDebug() << qPrintable(QString("Downloaded %1 files (%2 kbytes) in a session of %3 sec (average of %4 kB/s + %5 files from cache (%6 kB)).").arg(nbDownloadedFiles).arg(totalDownloadedSize/1024).arg(getTotalRunTime()).arg((double)(totalDownloadedSize/1024)/getTotalRunTime()).arg(nbUsedCache).arg(totalUsedCacheSize/1024));

	if (scriptMgr->scriptIsRunning())
		scriptMgr->stopScript();
	stelObjectMgr->unSelect();
	moduleMgr->unloadModule("StelSkyLayerMgr", false);  // We need to delete it afterward
	moduleMgr->unloadModule("StelObjectMgr", false);// We need to delete it afterward
	StelModuleMgr* tmp = moduleMgr;
	moduleMgr = new StelModuleMgr(); // Create a secondary instance to avoid crashes at other deinit
	delete tmp; tmp=NULL;
	delete loadingBar; loadingBar=NULL;
	delete core; core=NULL;
	delete skyCultureMgr; skyCultureMgr=NULL;
	delete localeMgr; localeMgr=NULL;
	delete audioMgr; audioMgr=NULL;
	delete stelObjectMgr; stelObjectMgr=NULL; // Delete the module by hand afterward
	delete stelFileMgr; stelFileMgr=NULL;
	delete textureMgr; textureMgr=NULL;
	delete planetLocationMgr; planetLocationMgr=NULL;
	delete moduleMgr; moduleMgr=NULL; // Delete the secondary instance
	delete argList; argList=NULL;

	delete currentStelStyle;

	logFile.close();

	Q_ASSERT(singleton);
	singleton = NULL;
}

#include <QNetworkDiskCache>

/*************************************************************************
 Return the full name of stellarium, i.e. "stellarium 0.9.0"
*************************************************************************/
QString StelApp::getApplicationName()
{
#ifdef SVN_REVISION
	return QString("Stellarium")+" "+PACKAGE_VERSION+" (SVN r"+SVN_REVISION+")";
#else
	return QString("Stellarium")+" "+PACKAGE_VERSION;
#endif
}

/*************************************************************************
 Return the version of stellarium, i.e. "0.9.0"
*************************************************************************/
QString StelApp::getApplicationVersion()
{
#ifdef SVN_REVISION
	return QString(PACKAGE_VERSION)+" (SVN r"+SVN_REVISION+")";
#else
	return QString(PACKAGE_VERSION);
#endif
}

void StelApp::debugLogHandler(QtMsgType type, const char* msg)
{
	fprintf(stderr, "%s\n", msg);
	StelApp::writeLog(QString(msg));
}

void StelApp::writeLog(QString msg)
{
	msg += "\n";
	logFile.write(qPrintable(msg), msg.size());
	log += msg;
}

void StelApp::init()
{
	networkAccessManager = new QNetworkAccessManager(this);
	// Activate http cache if Qt version >= 4.5
	QNetworkDiskCache* cache = new QNetworkDiskCache(networkAccessManager);
	QString cachePath = getFileMgr().getCacheDir();

	qDebug() << "Cache directory is: " << cachePath;
	cache->setCacheDirectory(cachePath);
	networkAccessManager->setCache(cache);
	connect(networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(reportFileDownloadFinished(QNetworkReply*)));

	// Stel Object Data Base manager
	stelObjectMgr = new StelObjectMgr();
	stelObjectMgr->init();
	getModuleMgr().registerModule(stelObjectMgr);

	core = new StelCore();
	if (saveProjW!=-1 && saveProjH!=-1)
		core->windowHasBeenResized(0, 0, saveProjW, saveProjH);

	setUseGLShaders(confSettings->value("main/use_glshaders", true).toBool());

	// Initialize AFTER creation of openGL context
	textureMgr = new StelTextureMgr();
	textureMgr->init();

	localeMgr = new StelLocaleMgr();
	skyCultureMgr = new StelSkyCultureMgr();
	planetLocationMgr = new StelLocationMgr();

#ifdef SVN_REVISION
	loadingBar = new StelLoadingBar(12., "logo24bits.png", QString("SVN r%1").arg(SVN_REVISION), 25, 320, 101);
#else
	loadingBar = new StelLoadingBar(12., "logo24bits.png", PACKAGE_VERSION, 45, 320, 121);
#endif // SVN_RELEASE

	downloadMgr = new StelDownloadMgr();

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
	skyImageMgr = new StelSkyLayerMgr();
	skyImageMgr->init();
	getModuleMgr().registerModule(skyImageMgr);

	// Init audio manager
	audioMgr = new StelAudioMgr();

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

	// Proxy Initialisation
	QString proxyName = confSettings->value("proxy/host_name").toString();
	QString proxyUser = confSettings->value("proxy/user").toString();
	QString proxyPassword = confSettings->value("proxy/password").toString();
	QVariant proxyPort = confSettings->value("proxy/port");

	if (proxyName!="" && !proxyPort.isNull()){

		QNetworkProxy proxy(QNetworkProxy::HttpProxy);
		proxy.setHostName(proxyName);
		proxy.setPort(proxyPort.toUInt());
		if(proxyUser!="" && proxyPassword!="") {
			proxy.setUser(proxyUser);
			proxy.setPassword(proxyPassword);
		}
		QNetworkProxy::setApplicationProxy(proxy);

	}

	updateI18n();

	scriptAPIProxy = new StelMainScriptAPIProxy(this);
	scriptMgr = new StelScriptMgr(this);
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
		StelModule* m = moduleMgr->loadPlugin(i.info.id);
		if (m!=NULL)
		{
			moduleMgr->registerModule(m, true);
			m->init();
		}
	}
}

void StelApp::setupLog()
{
	// write timestamp
	StelApp::writeLog(QString("%1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));

	// write command line arguments
	QString args;
	foreach(QString arg, *argList)
		args += QString("%1 ").arg(arg);
	StelApp::writeLog(args);

	// write OS version
#ifdef Q_WS_WIN
	switch(QSysInfo::WindowsVersion)
	{
		case QSysInfo::WV_95:
			StelApp::writeLog("Windows 95");
			break;
		case QSysInfo::WV_98:
			StelApp::writeLog("Windows 98");
			break;
		case QSysInfo::WV_Me:
			StelApp::writeLog("Windows Me");
			break;
		case QSysInfo::WV_NT:
			StelApp::writeLog("Windows NT");
			break;
		case QSysInfo::WV_2000:
			StelApp::writeLog("Windows 2000");
			break;
		case QSysInfo::WV_XP:
			StelApp::writeLog("Windows XP");
			break;
		case QSysInfo::WV_2003:
			StelApp::writeLog("Windows Server 2003");
			break;
		case QSysInfo::WV_VISTA:
			StelApp::writeLog("Windows Vista");
			break;
		default:
			StelApp::writeLog("Unsupported Windows version");
			break;
	}

	// somebody writing something useful for Macs would be great here
#elif defined Q_WS_MAC
	switch(QSysInfo::MacintoshVersion)
	{
		case QSysInfo::MV_10_3:
			StelApp::writeLog("Mac OS X 10.3");
			break;
		case QSysInfo::MV_10_4:
			StelApp::writeLog("Mac OS X 10.4");
			break;
		case QSysInfo::MV_10_5:
			StelApp::writeLog("Mac OS X 10.5");
			break;
		default:
			StelApp::writeLog("Unsupported Mac version");
			break;
	}

#elif defined Q_OS_LINUX
	QFile procVersion("/proc/version");
	if(!procVersion.open(QIODevice::ReadOnly | QIODevice::Text))
		StelApp::writeLog("Unknown Linux version");
	else
	{
		QString version = procVersion.readAll();
		if(version.right(1) == "\n")
			version.chop(1);
		StelApp::writeLog(version);
		procVersion.close();
	}
#else
	StelApp::writeLog("Unsupported operating system");
#endif

	// write GCC version
#ifndef __GNUC__
	StelApp::writeLog("Non-GCC compiler");
#else
	StelApp::writeLog(QString("Compiled with GCC %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__));
#endif

	// write Qt version
	StelApp::writeLog(QString("Qt runtime version: %1").arg(qVersion()));
	StelApp::writeLog(QString("Qt compilation version: %1").arg(QT_VERSION_STR));

	// write addressing mode
#ifdef __LP64__
	StelApp::writeLog("Addressing mode: 64-bit");
#else
	StelApp::writeLog("Addressing mode: 32-bit");
#endif

	// write memory and CPU info
#ifdef Q_OS_LINUX
	QFile infoFile("/proc/meminfo");
	if(!infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
		StelApp::writeLog("Could not get memory info.");
	else
	{
		while(!infoFile.peek(1).isEmpty())
		{
			QString line = infoFile.readLine();
			line.chop(1);
			if(line.startsWith("Mem") || line.startsWith("SwapTotal"))
				StelApp::writeLog(line);
		}
		infoFile.close();
	}

	infoFile.setFileName("/proc/cpuinfo");
	if(!infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
		StelApp::writeLog("Could not get CPU info.");
	else
	{
		while(!infoFile.peek(1).isEmpty())
		{
			QString line = infoFile.readLine();
			line.chop(1);
			if(line.startsWith("model name") || line.startsWith("cpu MHz"))
				StelApp::writeLog(line);
		}
		infoFile.close();
	}

	QProcess lspci;
	lspci.start("lspci -v", QIODevice::ReadOnly);
	lspci.waitForFinished(100);
	QString pciData(lspci.readAll());
	QStringList pciLines = pciData.split('\n', QString::SkipEmptyParts);
	for(int i = 0; i < pciLines.size(); i++)
	{
		if(pciLines.at(i).contains("VGA compatible controller"))
		{
			StelApp::writeLog(pciLines.at(i));
			i++;
			while(i < pciLines.size() && pciLines.at(i).startsWith('\t'))
			{
				if(pciLines.at(i).contains("Kernel driver in use"))
					StelApp::writeLog(pciLines.at(i).trimmed());
				else if(pciLines.at(i).contains("Kernel modules"))
					StelApp::writeLog(pciLines.at(i).trimmed());
				i++;
			}
		}
	}

	// Aargh Windows API
#elif defined Q_WS_WIN
	// Hopefully doesn't throw a linker error on earlier systems. Not like
	// I'm gonna test it or anything.
	if(QSysInfo::WindowsVersion >= QSysInfo::WV_2000)
	{
#ifdef __LP64__
		MEMORYSTATUSEX statex;
		GlobalMemoryStatusEx(&statex);
		StelApp::writeLog(QString("Total physical memory: %1 MB (unreliable)").arg(statex.ullTotalPhys/(1024<<10)));
		StelApp::writeLog(QString("Total virtual memory: %1 MB (unreliable)").arg(statex.ullTotalVirtual/(1024<<10)));
		StelApp::writeLog(QString("Physical memory in use: %1%").arg(statex.dwMemoryLoad));
#else
		MEMORYSTATUS statex;
		GlobalMemoryStatus(&statex);
		StelApp::writeLog(QString("Total memory: %1 MB (unreliable)").arg(statex.dwTotalPhys/(1024<<10)));
		StelApp::writeLog(QString("Total virtual memory: %1 MB (unreliable)").arg(statex.dwTotalVirtual/(1024<<10)));
		StelApp::writeLog(QString("Physical memory in use: %1%").arg(statex.dwMemoryLoad));
#endif
	}
	else
		StelApp::writeLog("Windows version too old to get memory info.");

	HKEY hKey = NULL;
	DWORD dwType = REG_DWORD;
	DWORD numVal = 0;
	DWORD dwSize = sizeof(numVal);

	// iterate over the processors listed in the registry
	QString procKey = "Hardware\\Description\\System\\CentralProcessor";
	LONG lRet = ERROR_SUCCESS;
	int i;
	for(i = 0; lRet == ERROR_SUCCESS; i++)
	{
		lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					TEXT(qPrintable(QString("%1\\%2").arg(procKey).arg(i))),
					0, KEY_QUERY_VALUE, &hKey);

		if(lRet == ERROR_SUCCESS)
		{
			if(RegQueryValueEx(hKey, "~MHz", NULL, &dwType, (LPBYTE)&numVal, &dwSize) == ERROR_SUCCESS)
				StelApp::writeLog(QString("Processor speed: %1 MHz").arg(numVal));
			else
				StelApp::writeLog("Could not get processor speed.");
		}

		// can you believe this trash?
		dwType = REG_SZ;
		char nameStr[512];
		DWORD nameSize = sizeof(nameStr);

		if(lRet == ERROR_SUCCESS)
		{
			if(RegQueryValueEx(hKey, "ProcessorNameString", NULL, &dwType, (LPBYTE)&nameStr, &nameSize) == ERROR_SUCCESS)
				StelApp::writeLog(QString("Processor name: %1").arg(nameStr));
			else
				StelApp::writeLog("Could not get processor name.");
		}

		RegCloseKey(hKey);
	}
	if(i == 0)
		StelApp::writeLog("Could not get processor info.");

#elif defined Q_WS_MAC
	StelApp::writeLog("You look like a Mac user. How would you like to write some system info code here? That would help a lot.");

#endif
}

void StelApp::parseCLIArgsPreConfig(void)
{
	if (argsGetOption(argList, "-v", "--version"))
	{
		std::cout << qPrintable(getApplicationName());
		exit(0);
	}

	if (argsGetOption(argList, "-h", "--help"))
	{
		// Get the basename of binary
		QString binName = argList->at(0);
		binName.remove(QRegExp("^.*[/\\\\]"));

		std::cout << "Usage:\n"
			 << "  "
			 << qPrintable(binName) << " [options]\n\n"
			 << "Options:\n"
			 << "--version (or -v)       : Print program name and version and exit.\n"
			 << "--help (or -h)          : This cruft.\n"
			 << "--config-file (or -c)   : Use an alternative name for the config file\n"
			 << "--user-dir (or -u)      : Use an alternative user data directory\n"
			 << "--full-screen (or -f)   : With argument \"yes\" or \"no\" over-rides\n"
			 << "                          the full screen setting in the config file\n"
			 << "--graphics-system       : With argument \"native\", \"raster\", or\n"
			 << "                          \"opengl\"; choose graphics backend \n"
			 << "                          default is " DEFAULT_GRAPHICS_SYSTEM "\n"
			 << "--screenshot-dir        : Specify directory to save screenshots\n"
			 << "--startup-script        : Specify name of startup script\n"
			 << "--home-planet           : Specify observer planet (English name)\n"
			 << "--altitude              : Specify observer altitude in meters\n"
			 << "--longitude             : Specify longitude, e.g. +53d58\\'16.65\\\"\n"
			 << "--latitude              : Specify latitude, e.g. -1d4\\'27.48\\\"\n"
			 << "--list-landscapes       : Print a list of value landscape IDs\n"
			 << "--landscape             : Start using landscape whose ID (dir name)\n"
			 << "                          is passed as parameter to option\n"
			 << "--sky-date              : Specify sky date in format yyyymmdd\n"
			 << "--sky-time              : Specify sky time in format hh:mm:ss\n"
			 << "--fov                   : Specify the field of view (degrees)\n"
			 << "--projection-type       : Specify projection type, e.g. stereographic\n"
			 << "--restore-defaults      : Delete existing config.ini and use defaults\n"
			 << "--multires-image        : With filename / URL argument,specify a\n"
				 << "                          multi-resolution image to load\n";
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
				// in that case we won't output the landscape ID as it cannot work
				stelFileMgr->findFile("landscapes/" + *i + "/landscape.ini");
				std::cout << qPrintable(*i);
			}
			catch (std::runtime_error& e){}
		}
		exit(0);
	}

	try
	{
		QString newUserDir;
		newUserDir = argsGetOptionWithArg(argList, "-u", "--user-dir", "").toString();
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
		setConfigFile(argsGetOptionWithArg(argList, "-c", "--config-file", "config.ini").toString(), restoreDefaultConfigFile);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: while looking for --config-file option: " << e.what() << ". Using \"config.ini\"";
		setConfigFile("config.ini", restoreDefaultConfigFile);
	}

	startupScript = argsGetOptionWithArg(argList, "", "--startup-script", "startup.ssc").toString();
}

void StelApp::parseCLIArgsPostConfig()
{
	// Over-ride config file options with command line options
	// We should catch exceptions from argsGetOptionWithArg...
	int fullScreen, altitude;
	float fov;
	QString landscapeId, homePlanet, longitude, latitude, skyDate, skyTime, projectionType, screenshotDir, multiresImage;

	try
	{
		fullScreen = argsGetYesNoOption(argList, "-f", "--full-screen", -1);
		landscapeId = argsGetOptionWithArg(argList, "", "--landscape", "").toString();
		homePlanet = argsGetOptionWithArg(argList, "", "--home-planet", "").toString();
		altitude = argsGetOptionWithArg(argList, "", "--altitude", -1).toInt();
		longitude = argsGetOptionWithArg(argList, "", "--longitude", "").toString();
		latitude = argsGetOptionWithArg(argList, "", "--latitude", "").toString();
		skyDate = argsGetOptionWithArg(argList, "", "--sky-date", "").toString();
		skyTime = argsGetOptionWithArg(argList, "", "--sky-time", "").toString();
		fov = argsGetOptionWithArg(argList, "", "--fov", -1.0).toDouble();
		projectionType = argsGetOptionWithArg(argList, "", "--projection-type", "").toString();
		screenshotDir = argsGetOptionWithArg(argList, "", "--screenshot-dir", "").toString();
		multiresImage = argsGetOptionWithArg(argList, "", "--multires-image", "").toString();
	}
	catch (std::runtime_error& e)
	{
		qCritical() << "ERROR while checking command line options: " << e.what();
		exit(0);
	}

	// This option is handled by main.cpp, but it's nice to have some debugging output in the log, which is
	// only available after the processing in main.cpp...
	qDebug() << "Using graphics system: " << argsGetOptionWithArg(argList, "", "--graphics-system", DEFAULT_GRAPHICS_SYSTEM).toString();

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

		// Over-ride the skyDatePart if the user specified the date using --sky-date
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

	if (!multiresImage.isEmpty())
		confSettings->setValue("skylayers/clilayer", multiresImage);
	else
	{
		confSettings->remove("skylayers/clilayer");
	}

	if (fov > 0.0) confSettings->setValue("navigation/init_fov", fov);

	if (projectionType != "") confSettings->setValue("projection/type", projectionType);

	if (screenshotDir!="")
	{
		try
		{
			QString newShotDir = argsGetOptionWithArg(argList, "", "--screenshot-dir", "").toString();
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

	core->update(deltaTime);

	moduleMgr->update();

	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionUpdate))
	{
		i->update(deltaTime);
	}

	stelObjectMgr->update(deltaTime);
}

//#include "StelPainter.hpp"
//#include "StelTextureTypes.hpp"


//! Iterate through the drawing sequence.
bool StelApp::drawPartial()
{
	if (drawState == 0)
	{
		if (!initialized)
			return false;
		core->preDraw();
		drawState = 1;
		return true;
	}

	const QList<StelModule*> modules = moduleMgr->getCallOrders(StelModule::ActionDraw);
	int index = drawState - 1;
	if (index < modules.size())
	{
		if (modules[index]->drawPartial(core))
			return true;
		drawState++;
		return true;
	}
	core->postDraw();
	drawState = 0;
	return false;
}

//! Main drawing function called at each frame
void StelApp::draw()
{
	Q_ASSERT(drawState == 0);
	while (drawPartial()) {}
	Q_ASSERT(drawState == 0);
}

/*************************************************************************
 Call this when the size of the GL window has changed
*************************************************************************/
void StelApp::glWindowHasBeenResized(float x, float y, float w, float h)
{
	if (core)
		core->windowHasBeenResized(x, y, w, h);
	else
	{
		saveProjW = w;
		saveProjH = h;
	}
}

// Handle mouse clics
void StelApp::handleClick(QMouseEvent* event)
{
	event->setAccepted(false);
	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHandleMouseClicks))
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
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHandleMouseClicks))
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

	if(stelGui)
		stelGui->setStelStyle(*currentStelStyle);

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
	StelMainWindow::getInstance().initTitleI18n();
	// Send the event to every StelModule
	foreach (StelModule* iter, moduleMgr->getAllModules())
	{
		iter->updateI18n();
	}
	if (getGui())
		getGui()->updateI18n();
}

// Update and reload sky culture informations everywhere in the program
void StelApp::updateSkyCulture()
{
	QString skyCultureDir = getSkyCultureMgr().getCurrentSkyCultureID();
	// Send the event to every StelModule
	foreach (StelModule* iter, moduleMgr->getAllModules())
	{
		iter->updateSkyCulture(skyCultureDir);
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

QVariant StelApp::argsGetOptionWithArg(QStringList* args, QString shortOpt, QString longOpt, QVariant defaultValue)
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
			return QVariant(argStr);
		}
	}
	return defaultValue;
}

int StelApp::argsGetYesNoOption(QStringList* args, QString shortOpt, QString longOpt, int defaultValue)
{
	QString strArg = argsGetOptionWithArg(args, shortOpt, longOpt, "").toString();
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
	return (double)(StelApp::qtime->elapsed())/1000.;
}


void StelApp::reportFileDownloadFinished(QNetworkReply* reply)
{
	bool fromCache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
	if (fromCache)
	{
		++nbUsedCache;
		totalUsedCacheSize+=reply->bytesAvailable();
	}
	else
	{
		++nbDownloadedFiles;
		totalDownloadedSize+=reply->bytesAvailable();
	}
}

void StelApp::makeMainGLContextCurrent()
{
	StelMainGraphicsView::getInstance().makeGLContextCurrent();
}

void StelApp::quitStellarium()
{
	if (getDownloadMgr().blockQuit())
	{
		QMessageBox::StandardButtons b = QMessageBox::question(0, q_("Download in progress"), q_("Stellarium is still downloading the file %1. Would you like to cancel the download and quit Stellarium?").arg(getDownloadMgr().name()), QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
		if (b==QMessageBox::Yes)
		{
			getDownloadMgr().abort();
		}
		else
		{
			return;	// Cancel quit
		}
	}
	QCoreApplication::exit();
}
