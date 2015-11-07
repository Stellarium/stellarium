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
#include "StelUtils.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "ConstellationMgr.hpp"
#include "NebulaMgr.hpp"
#include "LandscapeMgr.hpp"
#include "GridLinesMgr.hpp"
#include "MilkyWay.hpp"
#include "ZodiacalLight.hpp"
#include "LabelMgr.hpp"
#include "SolarSystem.hpp"
#include "SporadicMeteorMgr.hpp"
#include "StarMgr.hpp"
#include "StelIniParser.hpp"
#include "StelProjector.hpp"
#include "StelLocationMgr.hpp"
#include "StelActionMgr.hpp"

#include "StelProgressController.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelAudioMgr.hpp"
#include "StelVideoMgr.hpp"
#include "StelViewportEffect.hpp"
#include "StelGuiBase.hpp"
#include "StelPainter.hpp"
#ifndef DISABLE_SCRIPTING
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
#include <QScreen>
#include <QDateTime>

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

#ifdef USE_STATIC_PLUGIN_COMPASSMARKS
Q_IMPORT_PLUGIN(CompassMarksStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_SATELLITES
Q_IMPORT_PLUGIN(SatellitesStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_TEXTUSERINTERFACE
Q_IMPORT_PLUGIN(TextUserInterfaceStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_LOGBOOK
Q_IMPORT_PLUGIN(LogBookStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_OCULARS
Q_IMPORT_PLUGIN(OcularsStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_TELESCOPECONTROL
Q_IMPORT_PLUGIN(TelescopeControlStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_SOLARSYSTEMEDITOR
Q_IMPORT_PLUGIN(SolarSystemEditorStelPluginInterface)
#endif

#ifdef USE_STATIC_PLUGIN_TIMEZONECONFIGURATION
Q_IMPORT_PLUGIN(TimeZoneConfigurationStelPluginInterface)
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

#ifdef USE_STATIC_PLUGIN_FOV
Q_IMPORT_PLUGIN(FOVStelPluginInterface)
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

// Initialize static variables
StelApp* StelApp::singleton = NULL;
qint64 StelApp::startMSecs = 0;

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
StelApp::StelApp(QObject* parent)
	: QObject(parent)
	, core(NULL)
	, planetLocationMgr(NULL)
	, audioMgr(NULL)
	, videoMgr(NULL)
	, skyImageMgr(NULL)
#ifndef DISABLE_SCRIPTING
	, scriptAPIProxy(NULL)
	, scriptMgr(NULL)
#endif
	, stelGui(NULL)
	, devicePixelsPerPixel(1.f)
	, globalScalingRatio(1.f)
	, fps(0)
	, frame(0)
	, timefr(0.)
	, timeBase(0.)
	, flagNightVision(false)
	, confSettings(NULL)
	, initialized(false)
	, saveProjW(-1)
	, saveProjH(-1)
	, baseFontSize(13)
	, renderBuffer(NULL)
	, viewportEffect(NULL)
	, flagShowDecimalDegrees(false)
	, flagUseAzimuthFromSouth(false)
{
	// Stat variables
	nbDownloadedFiles=0;
	totalDownloadedSize=0;
	nbUsedCache=0;
	totalUsedCacheSize=0;

	setObjectName("StelApp");

	skyCultureMgr=NULL;
	localeMgr=NULL;
	stelObjectMgr=NULL;
	textureMgr=NULL;
	moduleMgr=NULL;
	networkAccessManager=NULL;
	actionMgr = NULL;

	// Can't create 2 StelApp instances
	Q_ASSERT(!singleton);
	singleton = this;

	moduleMgr = new StelModuleMgr();

	wheelEventTimer = new QTimer(this);
	wheelEventTimer->setInterval(25);
	wheelEventTimer->setSingleShot(true);

	// Reset delta accumulators
	wheelEventDelta[0] = wheelEventDelta[1] = 0;
}

/*************************************************************************
 Deinitialize and destroy the main Stellarium application.
*************************************************************************/
StelApp::~StelApp()
{
	qDebug() << qPrintable(QString("Downloaded %1 files (%2 kbytes) in a session of %3 sec (average of %4 kB/s + %5 files from cache (%6 kB)).").arg(nbDownloadedFiles).arg(totalDownloadedSize/1024).arg(getTotalRunTime()).arg((double)(totalDownloadedSize/1024)/getTotalRunTime()).arg(nbUsedCache).arg(totalUsedCacheSize/1024));

	stelObjectMgr->unSelect();
	moduleMgr->unloadModule("StelSkyLayerMgr", false);  // We need to delete it afterward
	moduleMgr->unloadModule("StelObjectMgr", false);// We need to delete it afterward
	StelModuleMgr* tmp = moduleMgr;
	moduleMgr = new StelModuleMgr(); // Create a secondary instance to avoid crashes at other deinit
	delete tmp; tmp=NULL;
	delete skyImageMgr; skyImageMgr=NULL;
	delete core; core=NULL;
	delete skyCultureMgr; skyCultureMgr=NULL;
	delete localeMgr; localeMgr=NULL;
	delete audioMgr; audioMgr=NULL;
	delete videoMgr; videoMgr=NULL;
	delete stelObjectMgr; stelObjectMgr=NULL; // Delete the module by hand afterward
	delete textureMgr; textureMgr=NULL;
	delete planetLocationMgr; planetLocationMgr=NULL;
	delete moduleMgr; moduleMgr=NULL; // Delete the secondary instance
	delete actionMgr; actionMgr = NULL;

	Q_ASSERT(singleton);
	singleton = NULL;
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

#ifndef DISABLE_SCRIPTING
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

void StelApp::init(QSettings* conf)
{
	confSettings = conf;

	devicePixelsPerPixel = QOpenGLContext::currentContext()->screen()->devicePixelRatio();

	setBaseFontSize(confSettings->value("gui/base_font_size", 13).toInt());
	
	core = new StelCore();
	if (saveProjW!=-1 && saveProjH!=-1)
		core->windowHasBeenResized(0, 0, saveProjW, saveProjH);

	// Initialize AFTER creation of openGL context
	textureMgr = new StelTextureMgr();
	textureMgr->init();

	networkAccessManager = new QNetworkAccessManager(this);
	// Activate http cache if Qt version >= 4.5
	QNetworkDiskCache* cache = new QNetworkDiskCache(networkAccessManager);
	QString cachePath = StelFileMgr::getCacheDir();

	qDebug() << "Cache directory is: " << QDir::toNativeSeparators(cachePath);
	cache->setCacheDirectory(cachePath);
	networkAccessManager->setCache(cache);
	connect(networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(reportFileDownloadFinished(QNetworkReply*)));

	// Stel Object Data Base manager
	stelObjectMgr = new StelObjectMgr();
	stelObjectMgr->init();
	getModuleMgr().registerModule(stelObjectMgr);

	localeMgr = new StelLocaleMgr();
	skyCultureMgr = new StelSkyCultureMgr();
	planetLocationMgr = new StelLocationMgr();
	actionMgr = new StelActionMgr();

	localeMgr->init();

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

	// Init zodiacal light
	ZodiacalLight* zodiacal_light = new ZodiacalLight();
	zodiacal_light->init();
	getModuleMgr().registerModule(zodiacal_light);

	// Init sky image manager
	skyImageMgr = new StelSkyLayerMgr();
	skyImageMgr->init();
	getModuleMgr().registerModule(skyImageMgr);

	// Init audio manager
	audioMgr = new StelAudioMgr();

	// Init video manager
	videoMgr = new StelVideoMgr();

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

	// Sporadic Meteors
	SporadicMeteorMgr* meteors = new SporadicMeteorMgr(10, 72);
	meteors->init();
	getModuleMgr().registerModule(meteors);

	// User labels
	LabelMgr* skyLabels = new LabelMgr();
	skyLabels->init();
	getModuleMgr().registerModule(skyLabels);

	skyCultureMgr->init();

	//Create the script manager here, maybe some modules/plugins may want to connect to it
	//It has to be initialized later after all modules have been loaded by calling initScriptMgr
#ifndef DISABLE_SCRIPTING
	scriptAPIProxy = new StelMainScriptAPIProxy(this);
	scriptMgr = new StelScriptMgr(this);
#endif

	// Initialisation of the color scheme
	emit colorSchemeChanged("color");
	setVisionModeNight(confSettings->value("viewing/flag_night").toBool());

	// Enable viewport effect at startup if he set
	setViewportEffect(confSettings->value("video/viewport_effect", "none").toString());

	// Proxy Initialisation
	setupNetworkProxy();
	updateI18n();

	// Init actions.
	actionMgr->addAction("actionShow_Night_Mode", N_("Display Options"), N_("Night mode"), this, "nightMode", "Ctrl+N");

	setFlagShowDecimalDegrees(confSettings->value("gui/flag_show_decimal_degrees", false).toBool());
	setFlagOldAzimuthUsage(confSettings->value("gui/flag_use_azimuth_from_south", false).toBool());
	
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

void StelApp::deinit()
{
#ifndef DISABLE_SCRIPTING
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
	StelProgressController* p = new StelProgressController();
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

void StelApp::prepareRenderBuffer()
{
	if (!viewportEffect) return;
	if (!renderBuffer)
	{
		StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
		int w = params.viewportXywh[2];
		int h = params.viewportXywh[3];
		viewportEffect = new StelViewportDistorterFisheyeToSphericMirror(w, h);
		renderBuffer = new QOpenGLFramebufferObject(w, h, QOpenGLFramebufferObject::CombinedDepthStencil);
	}
	renderBuffer->bind();
}

void StelApp::applyRenderBuffer()
{
	if (!renderBuffer) return;
	renderBuffer->release();
	viewportEffect->paintViewportBuffer(renderBuffer);
}

//! Main drawing function called at each frame
void StelApp::draw()
{
	if (!initialized)
		return;
	prepareRenderBuffer();
	core->preDraw();

	const QList<StelModule*> modules = moduleMgr->getCallOrders(StelModule::ActionDraw);
	foreach(StelModule* module, modules)
	{
		module->draw(core);
	}
	core->postDraw();
	applyRenderBuffer();
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
	if (renderBuffer)
	{
		delete renderBuffer;
		renderBuffer = NULL;
	}
}

// Handle mouse clics
void StelApp::handleClick(QMouseEvent* inputEvent)
{
	QPointF pos = inputEvent->pos();
	float x, y;
	x = pos.x();
	y = pos.y();
	if (viewportEffect)
		viewportEffect->distortXY(x, y);

	QMouseEvent event(inputEvent->type(), QPoint(x*devicePixelsPerPixel, y*devicePixelsPerPixel), inputEvent->button(), inputEvent->buttons(), inputEvent->modifiers());
	event.setAccepted(false);
	
	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHandleMouseClicks))
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
// This deltaEvent is a work-around for QTBUG-22269
void StelApp::handleWheel(QWheelEvent* event)
{
	event->setAccepted(false);

	const int deltaIndex = event->orientation() == Qt::Horizontal ? 0 : 1;
	wheelEventDelta[deltaIndex] += event->delta();
	if (wheelEventTimer->isActive())
	{
		// Collect the values. If delta is small enough we wait for more values or the end
		// of the timer period to process them.
		if (qAbs(wheelEventDelta[deltaIndex]) < 120)
			return;
	}

	wheelEventTimer->start();

	// Create a new event with the accumulated delta
	QWheelEvent deltaEvent(QPoint(event->pos().x()*devicePixelsPerPixel, event->pos().y()*devicePixelsPerPixel),
	                       QPoint(event->globalPos().x()*devicePixelsPerPixel, event->globalPos().y()*devicePixelsPerPixel),
	                       wheelEventDelta[deltaIndex], event->buttons(), event->modifiers(), event->orientation());
	deltaEvent.setAccepted(false);
	// Reset the collected values
	wheelEventDelta[deltaIndex] = 0;

	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHandleMouseClicks)) {
		i->handleMouseWheel(&deltaEvent);
		if (deltaEvent.isAccepted()) {
			event->accept();
			break;
		}
	}
}

// Handle mouse move
void StelApp::handleMove(float x, float y, Qt::MouseButtons b)
{
	if (viewportEffect)
		viewportEffect->distortXY(x, y);
	// Send the event to every StelModule
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHandleMouseMoves))
	{
		if (i->handleMouseMoves(x*devicePixelsPerPixel, y*devicePixelsPerPixel, b))
			return;
	}
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
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHandleKeys))
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
	foreach (StelModule* i, moduleMgr->getCallOrders(StelModule::ActionHandleMouseMoves))
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


void StelApp::setFlagShowDecimalDegrees(bool b)
{
	flagShowDecimalDegrees = b;
}

// Update translations and font for sky everywhere in the program
void StelApp::updateI18n()
{
#ifdef ENABLE_NLS
	emit(languageChanged());
#endif
}

// Update and reload sky culture informations everywhere in the program
void StelApp::updateSkyCulture()
{
	QString skyCultureDir = getSkyCultureMgr().getCurrentSkyCultureID();
	emit(skyCultureChanged(skyCultureDir));
}

// Return the time since when stellarium is running in second.
double StelApp::getTotalRunTime()
{
	return (double)(QDateTime::currentMSecsSinceEpoch() - StelApp::startMSecs)/1000.;
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
	QCoreApplication::exit(0);
}

void StelApp::setDevicePixelsPerPixel(float dppp)
{
	// Check that the device-independent pixel size didn't change
	if (devicePixelsPerPixel!=dppp)
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
		delete renderBuffer;
		renderBuffer = NULL;
	}
	if (viewportEffect)
	{
		delete viewportEffect;
		viewportEffect = NULL;
	}
	if (name == "none") return;

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
