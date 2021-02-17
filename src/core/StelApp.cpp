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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelApp.hpp"

#include "StelCore.hpp"
#include "StelMainView.hpp"
#include "StelSplashScreen.hpp"
#include "StelUtils.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "ConstellationMgr.hpp"
#include "AsterismMgr.hpp"
#include "HipsMgr.hpp"
#include "NebulaMgr.hpp"
#include "LandscapeMgr.hpp"
#include "CustomObjectMgr.hpp"
#include "HighlightMgr.hpp"
#include "GridLinesMgr.hpp"
#include "MilkyWay.hpp"
#include "ZodiacalLight.hpp"
#include "LabelMgr.hpp"
#include "MarkerMgr.hpp"
#include "SolarSystem.hpp"
#include "NomenclatureMgr.hpp"
#include "SporadicMeteorMgr.hpp"
#include "StarMgr.hpp"
#include "StelIniParser.hpp"
#include "StelProjector.hpp"
#include "StelLocationMgr.hpp"
#include "ToastMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelPropertyMgr.hpp"
#include "StelProgressController.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelAudioMgr.hpp"
#include "StelVideoMgr.hpp"
#include "SpecialMarkersMgr.hpp"
#include "StelViewportEffect.hpp"
#include "StelGuiBase.hpp"
#include "StelPainter.hpp"
#ifdef ENABLE_SCRIPTING
 #include "StelScriptMgr.hpp"
 #include "StelMainScriptAPIProxy.hpp"
#endif


#include <cstdlib>
#include <iostream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QString>
#include <QStringList>
#include <QSysInfo>
#include <QTextStream>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QDateTime>
#ifdef ENABLE_SPOUT
#include <QMessageBox>
#include "SpoutSender.hpp"
#endif

#ifdef USE_STATIC_PLUGIN_HELLOSTELMODULE
Q_IMPORT_PLUGIN(HelloStelModuleStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_SIMPLEDRAWLINE
Q_IMPORT_PLUGIN(SimpleDrawLineStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_ANGLEMEASURE
Q_IMPORT_PLUGIN(AngleMeasureStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_ARCHAEOLINES
Q_IMPORT_PLUGIN(ArchaeoLinesStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_CALENDARS
Q_IMPORT_PLUGIN(CalendarsStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_SATELLITES
Q_IMPORT_PLUGIN(SatellitesStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_TEXTUSERINTERFACE
Q_IMPORT_PLUGIN(TextUserInterfaceStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_OCULARS
Q_IMPORT_PLUGIN(OcularsStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_OCULUS
Q_IMPORT_PLUGIN(OculusStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_TELESCOPECONTROL
Q_IMPORT_PLUGIN(TelescopeControlStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_SOLARSYSTEMEDITOR
Q_IMPORT_PLUGIN(SolarSystemEditorStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_METEORSHOWERS
Q_IMPORT_PLUGIN(MeteorShowersStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_NAVSTARS
Q_IMPORT_PLUGIN(NavStarsStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_NOVAE
Q_IMPORT_PLUGIN(NovaeStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_SUPERNOVAE
Q_IMPORT_PLUGIN(SupernovaeStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_QUASARS
Q_IMPORT_PLUGIN(QuasarsStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_PULSARS
Q_IMPORT_PLUGIN(PulsarsStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_EXOPLANETS
Q_IMPORT_PLUGIN(ExoplanetsStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_EQUATIONOFTIME
Q_IMPORT_PLUGIN(EquationOfTimeStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_POINTERCOORDINATES
Q_IMPORT_PLUGIN(PointerCoordinatesStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_OBSERVABILITY
Q_IMPORT_PLUGIN(ObservabilityStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_SCENERY3D
Q_IMPORT_PLUGIN(Scenery3dStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_REMOTECONTROL
Q_IMPORT_PLUGIN(RemoteControlStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_REMOTESYNC
Q_IMPORT_PLUGIN(RemoteSyncStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_VTS
Q_IMPORT_PLUGIN(VtsStelPluginInterface)
#endif

// Initialize static variables
StelApp* StelApp::singleton = Q_NULLPTR;
qint64 StelApp::startMSecs = 0;
double StelApp::animationScale = 1.;

void StelApp::initStatic()
{
	StelApp::startMSecs = QDateTime::currentMSecsSinceEpoch();
}

void StelApp::deinitStatic()
{
	StelApp::startMSecs = 0;
}

/*************************************************************************
 Create and initialize the main Stellarium application.
*************************************************************************/
StelApp::StelApp(StelMainView *parent)
	: QObject(parent)
	, mainWin(parent)
	, core(Q_NULLPTR)
	, moduleMgr(Q_NULLPTR)
	, localeMgr(Q_NULLPTR)
	, skyCultureMgr(Q_NULLPTR)
	, actionMgr(Q_NULLPTR)
	, propMgr(Q_NULLPTR)
	, textureMgr(Q_NULLPTR)
	, stelObjectMgr(Q_NULLPTR)
	, planetLocationMgr(Q_NULLPTR)
	, networkAccessManager(Q_NULLPTR)
	, audioMgr(Q_NULLPTR)
	, videoMgr(Q_NULLPTR)
	, skyImageMgr(Q_NULLPTR)
#ifdef ENABLE_SCRIPTING
	, scriptAPIProxy(Q_NULLPTR)
	, scriptMgr(Q_NULLPTR)
#endif
	, stelGui(Q_NULLPTR)
	, devicePixelsPerPixel(1.)
	, globalScalingRatio(1.f)
	, fps(0)
	, frame(0)
	, frameTimeAccum(0.)
	, flagNightVision(false)
	, confSettings(Q_NULLPTR)
	, initialized(false)
	, saveProjW(-1.)
	, saveProjH(-1.)
	, nbDownloadedFiles(0)
	, totalDownloadedSize(0)
	, nbUsedCache(0)
	, totalUsedCacheSize(0)
	, screenFontSize(13)
	, renderBuffer(Q_NULLPTR)
	, viewportEffect(Q_NULLPTR)
	, gl(Q_NULLPTR)
	, flagShowDecimalDegrees(false)
	, flagUseAzimuthFromSouth(false)
	, flagUseFormattingOutput(false)
	, flagUseCCSDesignation(false)
	, flagOverwriteInfoColor(false)
	, overwriteInfoColor(Vec3f(1.f))
	, daylightInfoColor(Vec3f(0.f))
	#ifdef ENABLE_SPOUT
	, spoutSender(Q_NULLPTR)
	#endif
	, currentFbo(0)
{
	setObjectName("StelApp");

	// Can't create 2 StelApp instances
	Q_ASSERT(!singleton);
	singleton = this;

	moduleMgr = new StelModuleMgr();
}

/*************************************************************************
 Deinitialize and destroy the main Stellarium application.
*************************************************************************/
StelApp::~StelApp()
{
	qDebug() << qPrintable(QString("Downloaded %1 files (%2 kbytes) in a session of %3 sec (average of %4 kB/s + %5 files from cache (%6 kB)).").arg(nbDownloadedFiles).arg(totalDownloadedSize/1024).arg(getTotalRunTime()).arg(static_cast<double>(totalDownloadedSize/1024)/getTotalRunTime()).arg(nbUsedCache).arg(totalUsedCacheSize/1024));

	stelObjectMgr->unSelect();
	moduleMgr->unloadModule("StelVideoMgr", false);  // We need to delete it afterward
	moduleMgr->unloadModule("StelSkyLayerMgr", false);  // We need to delete it afterward
	moduleMgr->unloadModule("StelObjectMgr", false);// We need to delete it afterward
	StelModuleMgr* tmp = moduleMgr;
	moduleMgr = new StelModuleMgr(); // Create a secondary instance to avoid crashes at other deinit
	delete tmp; tmp=Q_NULLPTR;
	delete skyImageMgr; skyImageMgr=Q_NULLPTR;
	delete core; core=Q_NULLPTR;
	delete skyCultureMgr; skyCultureMgr=Q_NULLPTR;
	delete localeMgr; localeMgr=Q_NULLPTR;
	delete audioMgr; audioMgr=Q_NULLPTR;
	delete videoMgr; videoMgr=Q_NULLPTR;
	delete stelObjectMgr; stelObjectMgr=Q_NULLPTR; // Delete the module by hand afterward
	delete textureMgr; textureMgr=Q_NULLPTR;
	delete planetLocationMgr; planetLocationMgr=Q_NULLPTR;
	delete moduleMgr; moduleMgr=Q_NULLPTR; // Delete the secondary instance
	delete actionMgr; actionMgr = Q_NULLPTR;
	delete propMgr; propMgr = Q_NULLPTR;
	delete renderBuffer; renderBuffer = Q_NULLPTR;

	Q_ASSERT(singleton);
	singleton = Q_NULLPTR;
}

void StelApp::setupNetworkProxy()
{
	QString proxyHost = confSettings->value("proxy/host_name").toString();
	QString proxyPort = confSettings->value("proxy/port").toString();
	QString proxyUser = confSettings->value("proxy/user").toString();
	QString proxyPass = confSettings->value("proxy/password").toString();
	QString proxyType = confSettings->value("proxy/type").toString();

	bool useSocksProxy = proxyType.contains("socks", Qt::CaseInsensitive);

	// If proxy settings not found in config, use environment variable
	// if it is defined.  (Config file over-rides environment).
	if (proxyHost.isEmpty() && proxyUser.isEmpty() && proxyPass.isEmpty() && proxyPort.isEmpty())
	{
		char *httpProxyEnv;
		httpProxyEnv = std::getenv("http_proxy");
		if (!httpProxyEnv)
		{
			httpProxyEnv = std::getenv("HTTP_PROXY");
		}
		if (httpProxyEnv)
		{
			QString proxyString = QString(httpProxyEnv);
			if (!proxyString.isEmpty())
			{
				// Handle http_proxy of the form
				// proto://username:password@fqdn:port
				// e.g.:
				// http://usr:pass@proxy.loc:3128/
				// http://proxy.loc:3128/
				// http://2001:62a:4:203:6ab5:99ff:fef2:560b:3128/
				// http://foo:bar@2001:62a:4:203:6ab5:99ff:fef2:560b:3128/
				QRegExp pre("^([^:]+://)?(?:([^:]+):([^@]*)@)?(.+):([\\d]+)");
				if (pre.indexIn(proxyString) >= 0)
				{
					proxyUser = pre.cap(2);
					proxyPass = pre.cap(3);
					proxyHost = pre.cap(4);
					proxyPort = pre.cap(5);
				}
				else
				{
					qDebug() << "indecipherable environment variable http_proxy:" << proxyString;
					return;
				}
			}
		}
	}

	if (!proxyHost.isEmpty())
	{
		QNetworkProxy proxy;
		if (useSocksProxy)
			proxy.setType(QNetworkProxy::Socks5Proxy);
		else
			proxy.setType(QNetworkProxy::HttpProxy);
		proxy.setHostName(proxyHost);
		if (!proxyPort.isEmpty())
			proxy.setPort(proxyPort.toUShort());

		if (!proxyUser.isEmpty())
			proxy.setUser(proxyUser);

		if (!proxyPass.isEmpty())
			proxy.setPassword(proxyPass);

		QString ppDisp = proxyPass;
		ppDisp.fill('*');
		if (useSocksProxy)
			qDebug() << "Using SOCKS proxy:" << proxyUser << ppDisp << proxyHost << proxyPort;
		else
			qDebug() << "Using HTTP proxy:" << proxyUser << ppDisp << proxyHost << proxyPort;
		QNetworkProxy::setApplicationProxy(proxy);
	}
}

#ifdef ENABLE_SCRIPTING
void StelApp::initScriptMgr()
{
	scriptMgr->addModules();
	QString startupScript;
	if (qApp->property("onetime_startup_script").isValid())
		startupScript = qApp->property("onetime_startup_script").toString();
	else
		startupScript = confSettings->value("scripts/startup_script", "startup.ssc").toString();
	// Use a queued slot call to start the script only once the main qApp event loop is running...
	QMetaObject::invokeMethod(scriptMgr,
				  "runScript",
				  Qt::QueuedConnection,
				  Q_ARG(QString, startupScript));
}
#else
void StelApp::initScriptMgr() {}
#endif

QStringList StelApp::getCommandlineArguments()
{
	return qApp->property("stelCommandLine").toStringList();
}

void StelApp::init(QSettings* conf)
{
	gl = QOpenGLContext::currentContext()->functions();
	confSettings = conf;

	devicePixelsPerPixel = QOpenGLContext::currentContext()->screen()->devicePixelRatio();
	if (devicePixelsPerPixel>1)
		qDebug() << "Detected a high resolution device! Device pixel ratio:" << devicePixelsPerPixel;

	setScreenFontSize(confSettings->value("gui/screen_font_size", 13).toInt());
	setGuiFontSize(confSettings->value("gui/gui_font_size", 13).toInt());

	core = new StelCore();
	if (!fuzzyEquals(saveProjW, -1.) && !fuzzyEquals(saveProjH, -1.))
		core->windowHasBeenResized(0, 0, saveProjW, saveProjH);

	SplashScreen::showMessage(q_("Initializing textures..."));
	// Initialize AFTER creation of openGL context
	textureMgr = new StelTextureMgr();

	SplashScreen::showMessage(q_("Initializing network access..."));
	networkAccessManager = new QNetworkAccessManager(this);
	#if QT_VERSION >= 0x050900
	networkAccessManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
	#endif
	SplashScreen::showMessage(q_("Initializing network disk cache..."));
	// Activate http cache if Qt version >= 4.5
	QNetworkDiskCache* cache = new QNetworkDiskCache(networkAccessManager);
	//make maximum cache size configurable (in MB)
	//the default Qt value (50 MB) is quite low, especially for DSS
	cache->setMaximumCacheSize(confSettings->value("main/network_cache_size",300).toInt() * 1024 * 1024);
	QString cachePath = StelFileMgr::getCacheDir();

	qDebug() << "Cache directory is: " << QDir::toNativeSeparators(cachePath);
	cache->setCacheDirectory(cachePath);
	networkAccessManager->setCache(cache);	
	connect(networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(reportFileDownloadFinished(QNetworkReply*)));

	// Proxy Initialisation
	SplashScreen::showMessage(q_("Initializing network proxy..."));
	setupNetworkProxy();

	//create non-StelModule managers
	propMgr = new StelPropertyMgr();
	localeMgr = new StelLocaleMgr();
	skyCultureMgr = new StelSkyCultureMgr();
	propMgr->registerObject(skyCultureMgr);
	planetLocationMgr = new StelLocationMgr();
	actionMgr = new StelActionMgr();

	// register non-modules for StelProperty tracking
	propMgr->registerObject(this);
	propMgr->registerObject(mainWin);

	// Stel Object Data Base manager
	SplashScreen::showMessage(q_("Initializing Object Database..."));
	stelObjectMgr = new StelObjectMgr();
	stelObjectMgr->init();
	getModuleMgr().registerModule(stelObjectMgr);	

	SplashScreen::showMessage(q_("Initializing locales..."));
	localeMgr->init();

	// Hips surveys
	SplashScreen::showMessage(q_("Initializing HiPS survey..."));
	HipsMgr* hipsMgr = new HipsMgr();
	hipsMgr->init();
	getModuleMgr().registerModule(hipsMgr);

	// Init the solar system first
	SplashScreen::showMessage(q_("Initializing Solar System objects..."));
	SolarSystem* ssystem = new SolarSystem();
	ssystem->init();
	getModuleMgr().registerModule(ssystem);

	// Init the nomenclature for Solar system bodies
	SplashScreen::showMessage(q_("Initializing planetary nomenclature..."));
	NomenclatureMgr* nomenclature = new NomenclatureMgr();
	nomenclature->init();
	getModuleMgr().registerModule(nomenclature);

	// Load stars & their names
	SplashScreen::showMessage(q_("Initializing stars..."));
	StarMgr* hip_stars = new StarMgr();
	hip_stars->init();
	getModuleMgr().registerModule(hip_stars);

	SplashScreen::showMessage(q_("Initializing core..."));
	core->init();

	// Init nebulas
	SplashScreen::showMessage(q_("Initializing deep-sky objects..."));
	NebulaMgr* nebulas = new NebulaMgr();
	nebulas->init();
	getModuleMgr().registerModule(nebulas);

	// Init milky way
	SplashScreen::showMessage(q_("Initializing Milky Way..."));
	MilkyWay* milky_way = new MilkyWay();
	milky_way->init();
	getModuleMgr().registerModule(milky_way);

	// Init zodiacal light
	SplashScreen::showMessage(q_("Initializing zodiacal light..."));
	ZodiacalLight* zodiacal_light = new ZodiacalLight();
	zodiacal_light->init();
	getModuleMgr().registerModule(zodiacal_light);

	// Init sky image manager
	SplashScreen::showMessage(q_("Initializing sky image layer..."));
	skyImageMgr = new StelSkyLayerMgr();
	skyImageMgr->init();
	getModuleMgr().registerModule(skyImageMgr);

	// Toast surveys
	SplashScreen::showMessage(q_("Initializing TOAST surveys..."));
	ToastMgr* toasts = new ToastMgr();
	toasts->init();
	getModuleMgr().registerModule(toasts);

	// Init audio manager
	SplashScreen::showMessage(q_("Initializing audio..."));
	audioMgr = new StelAudioMgr();

	// Init video manager
	SplashScreen::showMessage(q_("Initializing video..."));
	videoMgr = new StelVideoMgr();
	videoMgr->init();
	getModuleMgr().registerModule(videoMgr);

	// Constellations
	SplashScreen::showMessage(q_("Initializing constellations..."));
	ConstellationMgr* constellations = new ConstellationMgr(hip_stars);
	constellations->init();
	getModuleMgr().registerModule(constellations);

	// Asterisms
	SplashScreen::showMessage(q_("Initializing asterisms..."));
	AsterismMgr* asterisms = new AsterismMgr(hip_stars);
	asterisms->init();
	getModuleMgr().registerModule(asterisms);

	// Landscape, atmosphere & cardinal points section
	SplashScreen::showMessage(q_("Initializing landscape..."));
	LandscapeMgr* landscape = new LandscapeMgr();
	landscape->init();
	getModuleMgr().registerModule(landscape);

	SplashScreen::showMessage(q_("Initializing grid lines..."));
	GridLinesMgr* gridLines = new GridLinesMgr();
	gridLines->init();
	getModuleMgr().registerModule(gridLines);
	
	SplashScreen::showMessage(q_("Initializing special markers..."));
	SpecialMarkersMgr* specialMarkers = new SpecialMarkersMgr();
	specialMarkers->init();
	getModuleMgr().registerModule(specialMarkers);

	// Sporadic Meteors
	SplashScreen::showMessage(q_("Initializing sporadic meteors..."));
	SporadicMeteorMgr* meteors = new SporadicMeteorMgr(10, 72);
	meteors->init();
	getModuleMgr().registerModule(meteors);

	// User labels
	SplashScreen::showMessage(q_("Initializing user labels..."));
	LabelMgr* skyLabels = new LabelMgr();
	skyLabels->init();
	getModuleMgr().registerModule(skyLabels);

	SplashScreen::showMessage(q_("Initializing sky cultures..."));
	skyCultureMgr->init();

	// User markers
	SplashScreen::showMessage(q_("Initializing user markers..."));
	MarkerMgr* skyMarkers = new MarkerMgr();
	skyMarkers->init();
	getModuleMgr().registerModule(skyMarkers);

	// Init custom objects
	SplashScreen::showMessage(q_("Initializing custom objects..."));
	CustomObjectMgr* custObj = new CustomObjectMgr();
	custObj->init();
	getModuleMgr().registerModule(custObj);

	// Init hightlights
	SplashScreen::showMessage(q_("Initializing highlights..."));
	HighlightMgr* hlMgr = new HighlightMgr();
	hlMgr->init();
	getModuleMgr().registerModule(hlMgr);

	//Create the script manager here, maybe some modules/plugins may want to connect to it
	//It has to be initialized later after all modules have been loaded by calling initScriptMgr
#ifdef ENABLE_SCRIPTING
	SplashScreen::showMessage(q_("Initializing scripting..."));
	scriptAPIProxy = new StelMainScriptAPIProxy(this);
	scriptMgr = new StelScriptMgr(this);
#endif

	SplashScreen::showMessage(q_("Initializing color scheme..."));
	// Initialisation of the color scheme
	emit colorSchemeChanged("color");
	setVisionModeNight(confSettings->value("viewing/flag_night", false).toBool());

	// Enable viewport effect at startup if he set
	setViewportEffect(confSettings->value("video/viewport_effect", "none").toString());

	SplashScreen::clearMessage();
	updateI18n();

	// Init actions.
	actionMgr->addAction("actionShow_Night_Mode", N_("Display Options"), N_("Night mode"), this, "nightMode", "Ctrl+N");

	setFlagShowDecimalDegrees(confSettings->value("gui/flag_show_decimal_degrees", false).toBool());
	setFlagSouthAzimuthUsage(confSettings->value("gui/flag_use_azimuth_from_south", false).toBool());
	setFlagUseFormattingOutput(confSettings->value("gui/flag_use_formatting_output", false).toBool());
	setFlagUseCCSDesignation(confSettings->value("gui/flag_use_ccs_designations", false).toBool());
	setFlagOverwriteInfoColor(confSettings->value("gui/flag_overwrite_info_color", false).toBool());	
	setOverwriteInfoColor(Vec3f(confSettings->value("color/info_text_color", "1.0,1.0,1.0").toString()));
	setDaylightInfoColor(Vec3f(confSettings->value("color/daylight_text_color", "0.0,0.0,0.0").toString()));

	// Animation
	animationScale = confSettings->value("gui/pointer_animation_speed", 1.).toDouble();
	
#ifdef ENABLE_SPOUT
	//qDebug() << "Property spout is" << qApp->property("spout").toString();
	//qDebug() << "Property spoutName is" << qApp->property("spoutName").toString();
	if (qApp->property("spout").toString() != "none")
	{
		//if we are on windows and we have GLES, we are most likely on ANGLE
		bool isANGLE=QOpenGLContext::currentContext()->isOpenGLES();

		if (isANGLE)
		{
			qCritical() << "SPOUT: Does not run in ANGLE/OpenGL ES mode!";
		}
		else
		{
			SplashScreen::showMessage(q_("Initializing SPOUT sender..."));
			// Create the SpoutSender object.
			QString spoutName = qApp->property("spoutName").toString();
			if(spoutName.isEmpty())
				spoutName = "stellarium";

			spoutSender = new SpoutSender(spoutName);

			if (!spoutSender->isValid())
			{
				QMessageBox::warning(&StelMainView::getInstance(), "Stellarium SPOUT", q_("Cannot create Spout sender. See log for details."), QMessageBox::Ok);
				delete spoutSender;
				spoutSender = Q_NULLPTR;
				qApp->setProperty("spout", "");
			}
			SplashScreen::clearMessage();
		}
	}
	else
	{
		qApp->setProperty("spout", "");
	}
#endif

	initialized = true;
}

// Load and initialize external modules (plugins)
void StelApp::initPlugIns()
{
	// Load dynamically all the modules found in the modules/ directories
	// which are configured to be loaded at startup
	for (const auto& i : moduleMgr->getPluginsList())
	{
		if (i.loadAtStartup==false)
			continue;
		SplashScreen::showMessage(QString("%1 \"%2\"...").arg(q_("Loading plugin"), q_(i.info.displayedName)));
		StelModule* m = moduleMgr->loadPlugin(i.info.id);
		if (m!=Q_NULLPTR)
		{
			moduleMgr->registerModule(m, true);
			//load extensions after the module is registered
			moduleMgr->loadExtensions(i.info.id);
			m->init();
		}
	}
	SplashScreen::clearMessage();
}

void StelApp::deinit()
{
#ifdef 	ENABLE_SPOUT
	delete spoutSender;
	spoutSender = Q_NULLPTR;
#endif
#ifdef ENABLE_SCRIPTING
	if (scriptMgr->scriptIsRunning())
		scriptMgr->stopScript();
#endif
	QCoreApplication::processEvents();
	getModuleMgr().unloadAllPlugins();
	QCoreApplication::processEvents();
	StelPainter::deinitGLShaders();
}


StelProgressController* StelApp::addProgressBar()
{
	StelProgressController* p = new StelProgressController(this);
	progressControllers.append(p);
	emit(progressBarAdded(p));
	return p;
}

void StelApp::removeProgressBar(StelProgressController* p)
{
	progressControllers.removeOne(p);	
	emit(progressBarRemoved(p));
	delete p;
}


void StelApp::update(double deltaTime)
{
	if (!initialized)
		return;

	++frame;
	frameTimeAccum+=deltaTime;
	if (frameTimeAccum > 1.)
	{
		// Calc the FPS rate every seconds
		fps=static_cast<float>(frame)/static_cast<float>(frameTimeAccum);
		frame = 0;
		frameTimeAccum=0.;
	}
		
	core->update(deltaTime);

	moduleMgr->update();

	// Send the event to every StelModule
	for (auto* i : moduleMgr->getCallOrders(StelModule::ActionUpdate))
	{
		i->update(deltaTime);
	}

	stelObjectMgr->update(deltaTime);
}

void StelApp::prepareRenderBuffer()
{
	if (!viewportEffect) return;
	if (!renderBuffer)
	{
		StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
		int w = params.viewportXywh[2];
		int h = params.viewportXywh[3];
		renderBuffer = new QOpenGLFramebufferObject(w, h, QOpenGLFramebufferObject::Depth); // we only need depth here
	}
	renderBuffer->bind();
}

void StelApp::applyRenderBuffer(GLuint drawFbo)
{
	if (!renderBuffer) return;
	GL(gl->glBindFramebuffer(GL_FRAMEBUFFER, drawFbo));
	viewportEffect->paintViewportBuffer(renderBuffer);
}

//! Main drawing function called at each frame
void StelApp::draw()
{
	if (!initialized)
		return;

	//find out which framebuffer is the current one
	//this is usually NOT the "zero" FBO, but one provided by QOpenGLWidget
	GLint drawFbo;
	GL(gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &drawFbo));

	prepareRenderBuffer();
	currentFbo = renderBuffer ? renderBuffer->handle() : static_cast<GLuint>(drawFbo);

	core->preDraw();

	const QList<StelModule*> modules = moduleMgr->getCallOrders(StelModule::ActionDraw);
	for (auto* module : modules)
	{
		module->draw(core);
	}
	core->postDraw();
#ifdef ENABLE_SPOUT
	// At this point, the sky scene has been drawn, but no GUI panels.
	if(spoutSender)
		spoutSender->captureAndSendFrame(static_cast<GLuint>(drawFbo));
#endif
	applyRenderBuffer(static_cast<GLuint>(drawFbo));
}

/*************************************************************************
 Call this when the size of the GL window has changed
*************************************************************************/
void StelApp::glWindowHasBeenResized(const QRectF& rect)
{
	// Remove the effect before resizing the core, or things get messy.
	QString effect = getViewportEffect();
	setViewportEffect("none");
	if (core)
	{
		core->windowHasBeenResized(rect.x(), rect.y(), rect.width(), rect.height());
	}
	else
	{
		saveProjW = rect.width();
		saveProjH = rect.height();
	}
	// Force to recreate the viewport effect if any.
	setViewportEffect(effect);
#ifdef ENABLE_SPOUT
	if (spoutSender)
		spoutSender->resize(static_cast<uint>(rect.width()),static_cast<uint>(rect.height()));
#endif
}

// Handle mouse clics
void StelApp::handleClick(QMouseEvent* inputEvent)
{
	QPointF pos = inputEvent->pos();
	qreal x, y;
	x = pos.x();
	y = pos.y();
	if (viewportEffect)
		viewportEffect->distortXY(x, y);

	QMouseEvent event(inputEvent->type(), QPoint(qRound(x*devicePixelsPerPixel), qRound(y*devicePixelsPerPixel)), inputEvent->button(), inputEvent->buttons(), inputEvent->modifiers());
	event.setAccepted(false);
	
	// Send the event to every StelModule
	for (auto* i : moduleMgr->getCallOrders(StelModule::ActionHandleMouseClicks))
	{
		i->handleMouseClicks(&event);
		if (event.isAccepted())
		{
			inputEvent->setAccepted(true);
			return;
		}
	}
}

// Handle mouse wheel.
void StelApp::handleWheel(QWheelEvent* event)
{
	event->setAccepted(false);
	QWheelEvent deltaEvent(QPoint(qRound(event->pos().x()*devicePixelsPerPixel), qRound(event->pos().y()*devicePixelsPerPixel)),
			       QPoint(qRound(event->globalPos().x()*devicePixelsPerPixel), qRound(event->globalPos().y()*devicePixelsPerPixel)),
	                       event->delta(), event->buttons(), event->modifiers(), event->orientation());
	deltaEvent.setAccepted(false);
	// Send the event to every StelModule
	for (auto* i : moduleMgr->getCallOrders(StelModule::ActionHandleMouseClicks)) {
		i->handleMouseWheel(&deltaEvent);
		if (deltaEvent.isAccepted()) {
			event->accept();
			break;
		}
	}
}

// Handle mouse move
bool StelApp::handleMove(qreal x, qreal y, Qt::MouseButtons b)
{
	if (viewportEffect)
		viewportEffect->distortXY(x, y);
	// Send the event to every StelModule
	for (auto* i : moduleMgr->getCallOrders(StelModule::ActionHandleMouseMoves))
	{
		if (i->handleMouseMoves(qRound(x*devicePixelsPerPixel), qRound(y*devicePixelsPerPixel), b))
			return true;
	}
	return false;
}

// Handle key press and release
void StelApp::handleKeys(QKeyEvent* event)
{
	event->setAccepted(false);
	// First try to trigger a shortcut.
	if (event->type() == QEvent::KeyPress)
	{
		if (getStelActionManager()->pushKey(event->key() + event->modifiers()))
		{
			event->setAccepted(true);
			return;
		}
	}
	// Send the event to every StelModule
	for (auto* i : moduleMgr->getCallOrders(StelModule::ActionHandleKeys))
	{
		i->handleKeys(event);
		if (event->isAccepted())
			return;
	}
}

// Handle pinch on multi touch devices
void StelApp::handlePinch(qreal scale, bool started)
{
	// Send the event to every StelModule
	for (auto* i : moduleMgr->getCallOrders(StelModule::ActionHandleMouseMoves))
	{
		if (i->handlePinch(scale, started))
			return;
	}
}

//! Set flag for activating night vision mode
void StelApp::setVisionModeNight(bool b)
{
	if (flagNightVision!=b)
	{
		flagNightVision=b;
		emit(visionNightModeChanged(b));
	}
}

void StelApp::setFlagOverwriteInfoColor(bool b)
{
	if (flagOverwriteInfoColor!=b)
	{
		flagOverwriteInfoColor=b;
		emit(flagOverwriteInfoColorChanged(b));
	}
}

void StelApp::setFlagShowDecimalDegrees(bool b)
{
	if (flagShowDecimalDegrees!=b)
	{
		flagShowDecimalDegrees = b;
		emit flagShowDecimalDegreesChanged(b);
	}
}

void StelApp::setFlagUseFormattingOutput(bool b)
{
	if (flagUseFormattingOutput!=b)
	{
		flagUseFormattingOutput = b;
		emit flagUseFormattingOutputChanged(b);
	}
}

void StelApp::setFlagUseCCSDesignation(bool b)
{
	if (flagUseCCSDesignation!=b)
	{
		flagUseCCSDesignation = b;
		emit flagUseCCSDesignationChanged(b);
	}
}

void StelApp::setOverwriteInfoColor(const Vec3f& color)
{
	if (color != overwriteInfoColor)
	{
		overwriteInfoColor = color;
		emit overwriteInfoColorChanged(color);
	}
}

Vec3f StelApp::getOverwriteInfoColor() const
{
	return overwriteInfoColor;
}

void StelApp::setDaylightInfoColor(const Vec3f& color)
{
	if (color != daylightInfoColor)
	{
		daylightInfoColor = color;
		emit daylightInfoColorChanged(color);
	}
}

Vec3f StelApp::getDaylightInfoColor() const
{
	return daylightInfoColor;
}

// Update translations and font for sky everywhere in the program
void StelApp::updateI18n()
{
#ifdef ENABLE_NLS
	emit(languageChanged());
#endif
}

void StelApp::ensureGLContextCurrent()
{
	mainWin->glContextMakeCurrent();
}

// Return the time since when stellarium is running in second.
double StelApp::getTotalRunTime()
{
	return static_cast<double>(QDateTime::currentMSecsSinceEpoch() - StelApp::startMSecs)/1000.;
}

// Return the scaled time since when stellarium is running in second.
double StelApp::getAnimationTime()
{
	return static_cast<double>(QDateTime::currentMSecsSinceEpoch() - StelApp::startMSecs)*StelApp::animationScale/1000.;
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

void StelApp::quit()
{
	emit aboutToQuit();
	// Let's allow exit from Stellarium via startup script!
	QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
}

void StelApp::setDevicePixelsPerPixel(qreal dppp)
{
	// Check that the device-independent pixel size didn't change
	if (!viewportEffect && !fuzzyEquals(devicePixelsPerPixel, dppp))
	{
		devicePixelsPerPixel = dppp;
		StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
		params.devicePixelsPerPixel = devicePixelsPerPixel;
		core->setCurrentStelProjectorParams(params);
	}
}

void StelApp::setViewportEffect(const QString& name)
{
	if (name == getViewportEffect()) return;
	if (renderBuffer)
	{
		ensureGLContextCurrent();
		delete renderBuffer;
		renderBuffer = Q_NULLPTR;
	}
	if (viewportEffect)
	{
		delete viewportEffect;
		viewportEffect = Q_NULLPTR;
	}
	if (name == "none") return;

	if (!core)
	{
		qDebug() << "No core to set viewport effect";
		return;
	}
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
	int w = params.viewportXywh[2];
	int h = params.viewportXywh[3];
	if (name == "sphericMirrorDistorter")
	{
		viewportEffect = new StelViewportDistorterFisheyeToSphericMirror(w, h);
	}
	else
	{
		qDebug() << "unknown viewport effect name:" << name;
		Q_ASSERT(false);
	}
}

QString StelApp::getViewportEffect() const
{
	if (viewportEffect)
		return viewportEffect->getName();
	return "none";
}

// Diagnostics
void StelApp::dumpModuleActionPriorities(StelModule::StelModuleActionName actionName) const
{
	const QList<StelModule*> modules = moduleMgr->getCallOrders(actionName);
	#if QT_VERSION >= 0x050500
	QMetaEnum me = QMetaEnum::fromType<StelModule::StelModuleActionName>();
	qDebug() << "Module Priorities for action named" << me.valueToKey(actionName);
	#else
	qDebug() << "Module Priorities for action named" << actionName;
	#endif

	for (auto* module : modules)
	{
		module->draw(core);
		qDebug() << " -- " << module->getCallOrder(actionName) << "Module: " << module->objectName();
	}
}

StelModule* StelApp::getModule(const QString& moduleID) const
{
	return getModuleMgr().getModule(moduleID);
}
void StelApp::setScreenFontSize(int s)
{
	if (screenFontSize!=s)
	{
		screenFontSize=s;
		emit screenFontSizeChanged(s);
	}
}
void StelApp::setGuiFontSize(int s)
{
	if (getGuiFontSize()!=s)
	{
		QFont font=QGuiApplication::font();
		font.setPixelSize(s);
		QGuiApplication::setFont(font);
		emit guiFontSizeChanged(s);
	}
}
int StelApp::getGuiFontSize() const
{
	return QGuiApplication::font().pixelSize();
}

void StelApp::setAppFont(QFont font)
{
	int oldSize=QGuiApplication::font().pixelSize();
	font.setPixelSize(oldSize);
	font.setStyleHint(QFont::AnyStyle, QFont::OpenGLCompatible);
	QGuiApplication::setFont(font);
	emit fontChanged(font);
}

QString StelApp::getVersion() const
{
	QStringList ver = StelUtils::getApplicationVersion().split(".");
	return QString("%1.%2.%3").arg(ver[0]).arg(ver[1]).arg(ver[2]);
}
