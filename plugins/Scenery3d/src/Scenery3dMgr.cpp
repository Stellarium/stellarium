/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza
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

#include <QDebug>
#include <QSettings>
#include <QString>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QTimer>
#include <QOpenGLShaderProgram>
#include <QtConcurrent>

#include <stdexcept>

#include "Scenery3dMgr.hpp"
#include "Scenery3d.hpp"
#include "Scenery3dDialog.hpp"
#include "StoredViewDialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelActionMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelFileMgr.hpp"
#include "StelPainter.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "LandscapeMgr.hpp"

#define S3D_CONFIG_PREFIX QString("Scenery3d")

Scenery3dMgr::Scenery3dMgr() :
    scenery3d(NULL),
    flagEnabled(false),
    cleanedUp(false),
    progressBar(NULL),
    currentLoadScene(),
    currentLoadFuture(this)
{
    setObjectName("Scenery3dMgr");
    scenery3dDialog = new Scenery3dDialog();
    storedViewDialog = new StoredViewDialog();

    font.setPixelSize(16);
    messageFader.setDuration(500);
    messageTimer = new QTimer(this);
    messageTimer->setInterval(2000);
    messageTimer->setSingleShot(true);
    connect(messageTimer, &QTimer::timeout, this, &Scenery3dMgr::clearMessage);
    connect(&currentLoadFuture,&QFutureWatcherBase::finished, this, &Scenery3dMgr::loadSceneCompleted);

    connect(this, &Scenery3dMgr::progressReport, this, &Scenery3dMgr::progressReceive, Qt::QueuedConnection);

    //create scenery3d object
    scenery3d = new Scenery3d(this);
}

Scenery3dMgr::~Scenery3dMgr()
{
	qDebug()<<"Scenery3dMgr destructor";

	if(!cleanedUp)
		deinit();

	delete storedViewDialog;
	delete scenery3dDialog;
}

double Scenery3dMgr::getCallOrder(StelModuleActionName actionName) const
{
    if (actionName == StelModule::ActionDraw)
	return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName) + 5; // between Landscape and compass marks!
    if (actionName == StelModule::ActionUpdate)
        return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName) + 10;
    if (actionName == StelModule::ActionHandleKeys)
        return 3; // GZ: low number means high precedence!
    return 0;
}

void Scenery3dMgr::handleKeys(QKeyEvent* e)
{
	if (flagEnabled)
	{
		//pass event on to scenery3d obj
		scenery3d->handleKeys(e);
	}
}


void Scenery3dMgr::update(double deltaTime)
{
	if (flagEnabled)
	{
		scenery3d->update(deltaTime);
	}

	messageFader.update((int)(deltaTime*1000));
}

void Scenery3dMgr::draw(StelCore* core)
{
	if (flagEnabled)
	{
		scenery3d->draw(core);
	}

	//the message is always drawn
	if (messageFader.getInterstate() > 0.000001f)
	{

		const StelProjectorP prj = core->getProjection(StelCore::FrameEquinoxEqu);
		StelPainter painter(prj);
		painter.setFont(font);
		painter.setColor(textColor[0], textColor[1], textColor[2], messageFader.getInterstate());
		painter.drawText(83, 120, currentMessage);
	}

}

void Scenery3dMgr::init()
{
    qDebug() << "Scenery3d plugin - press KGA button to toggle 3D scenery, KGA tool button for settings";

	//load config and create interface actions
	loadConfig();
	createActions();

	// graphics hardware without FrameBufferObj extension cannot use the cubemap rendering and shadow mapping.
	// In this case, set cubemapSize to 0 to signal auto-switch to perspective projection.
	if ( !QOpenGLContext::currentContext()->hasExtension("GL_EXT_framebuffer_object")) {

		//TODO FS: it seems like the current stellarium requires a working framebuffer extension anyway, so skip this check?

		qWarning() << "Scenery3d: Your hardware does not support EXT_framebuffer_object.";
		qWarning() << "           Shadow mapping disabled, and display limited to perspective projection.";

		scenery3d->setCubemapSize(0);
		scenery3d->setShadowmapSize(0);
	}

	reloadShaders();

	//Initialize Shadow Mapping
	qWarning() << "init scenery3d object.";
	scenery3d->init();
	qWarning() << "init scenery3d object...done\n";

	emit isGeometryShaderSupportedChanged(getIsGeometryShaderSupported());

	// Add 2 toolbar buttons (copy/paste widely from AngleMeasure): activate, and settings.
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());

		StelButton* toolbarEnableButton = new StelButton(NULL, QPixmap(":/Scenery3d/bt_scenery3d_on.png"),
								 QPixmap(":/Scenery3d/bt_scenery3d_off.png"),
								 QPixmap(":/graphicGui/glow32x32.png"),
								 "actionShow_Scenery3d");
		StelButton* toolbarSettingsButton = new StelButton(NULL, QPixmap(":/Scenery3d/bt_scenery3d_settings_on.png"),
								   QPixmap(":/Scenery3d/bt_scenery3d_settings_off.png"),
								   QPixmap(":/graphicGui/glow32x32.png"),
								   "actionShow_Scenery3d_dialog");
		StelButton* toolbarStoredViewButton = new StelButton(NULL, QPixmap(":/graphicGui/btNightView-on.png"),
								   QPixmap(":/graphicGui/btNightView-off.png"),
								   QPixmap(":/graphicGui/glow32x32.png"),
								   "actionShow_Scenery3d_storedViewDialog");

		gui->getButtonBar()->addButton(toolbarEnableButton, "065-pluginsGroup");
		gui->getButtonBar()->addButton(toolbarSettingsButton, "065-pluginsGroup");
		gui->getButtonBar()->addButton(toolbarStoredViewButton, "065-pluginsGroup");
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to create toolbar buttons for Scenery3d plugin: " << e.what();
	}

	//finally, hook up the lightscape toggle event to cubemap redraw
	StelAction* action = StelApp::getInstance().getStelActionManager()->findAction("actionShow_LandscapeIllumination");
	Q_ASSERT(action);
	connect(action, &StelAction::toggled, this, &Scenery3dMgr::forceCubemapRedraw);

	showMessage("Scenery3d plugin loaded!");
}

void Scenery3dMgr::deinit()
{
	qDebug()<<"Scenery3dMgr deinit";

	//wait until loading is finished
	scenery3d->setLoadCancel(true);
	currentLoadFuture.waitForFinished();

	//this is correct the place to delete all OpenGL related stuff, not the destructor
	if(scenery3d != NULL)
	{
		delete scenery3d;
		scenery3d = NULL;
	}

	cleanedUp = true;
}

void Scenery3dMgr::loadConfig()
{
	conf = StelApp::getInstance().getSettings();

	conf->beginGroup(S3D_CONFIG_PREFIX);

	textColor = StelUtils::strToVec3f(conf->value("text_color", "0.5,0.5,1").toString());
	scenery3d->setCubemappingMode( static_cast<S3DEnum::CubemappingMode>(conf->value("cubemap_mode",1).toInt()) );
	scenery3d->setCubemapShadowMode( static_cast<S3DEnum::CubemapShadowMode>(conf->value("cubemap_shadow_mode",0).toInt()) );
	scenery3d->setCubemapSize(conf->value("cubemap_size",2048).toInt());
	scenery3d->setShadowmapSize(conf->value("shadowmap_size", 1024).toInt());
	scenery3d->setShadowFilterQuality( static_cast<S3DEnum::ShadowFilterQuality>(conf->value("shadow_filter_quality", 1).toInt()) );
	scenery3d->setPCSS(conf->value("flag_pcss").toBool());
	scenery3d->setTorchBrightness(conf->value("torch_brightness", 0.5f).toFloat());
	scenery3d->setTorchRange(conf->value("torch_range",5.0f).toFloat());
	scenery3d->setBumpsEnabled(conf->value("flag_bumpmap").toBool());
	scenery3d->setShadowsEnabled(conf->value("flag_shadow").toBool());
	scenery3d->setLazyCubemapEnabled(conf->value("flag_lazy_cubemap").toBool());
	scenery3d->setLazyCubemapInterval(conf->value("cubemap_lazy_interval",1.0).toDouble());
	scenery3d->setPixelLightingEnabled(conf->value("flag_pixel_lighting").toBool());

	bool v1 = conf->value("flag_lazy_dominantface",false).toBool();
	bool v2 = conf->value("flag_lazy_seconddominantface",true).toBool();
	scenery3d->setLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,v2);

	defaultScenery3dID = conf->value("default_location_id","").toString();

	conf->endGroup();
}

void Scenery3dMgr::createActions()
{
	QString groupName = N_("Scenery3d: 3D landscapes");

	//enable action will be set checkable if a scene was loaded
	addAction("actionShow_Scenery3d", groupName, N_("Toggle 3D landscape"),this,"enableScene","Ctrl+3");
	addAction("actionShow_Scenery3d_dialog", groupName, N_("Show settings dialog"), scenery3dDialog, "visible", "Ctrl+Shift+3");
	addAction("actionShow_Scenery3d_storedViewDialog", groupName, N_("Show viewpoint dialog"), storedViewDialog, "visible", "Ctrl+Shift+4");
	addAction("actionShow_Scenery3d_shadows", groupName, N_("Toggle shadows"), this, "enableShadows","Ctrl+R, S");
	addAction("actionShow_Scenery3d_debuginfo", groupName, N_("Toggle debug information"), this, "enableDebugInfo","Ctrl+R, D");
	addAction("actionShow_Scenery3d_locationinfo", groupName, N_("Toggle location text"), this, "enableLocationInfo","Ctrl+R, T");
	addAction("actionShow_Scenery3d_torchlight", groupName, N_("Toggle torchlight"), this, "enableTorchLight", "Ctrl+R, L");
	addAction("actionReload_Scenery3d_shaders", groupName, N_("Reload shaders"), this, "reloadShaders()", "Ctrl+R, P");
}

void Scenery3dMgr::reloadShaders()
{
	showMessage(N_("Scenery3d shaders reloaded"));

	qDebug()<<"(Re)loading Scenery3d shaders";

	scenery3d->getShaderManager().clearCache();
}

bool Scenery3dMgr::configureGui(bool show)
{
	if (show)
		scenery3dDialog->setVisible(show);
    return true;
}

void Scenery3dMgr::showStoredViewDialog()
{
	storedViewDialog->setVisible(true);
}

void Scenery3dMgr::updateProgress(const QString &str, int val, int min, int max)
{
	emit progressReport(str,val,min,max);
}

void Scenery3dMgr::progressReceive(const QString &str, int val, int min, int max)
{
	//update progress bar
	if(progressBar)
	{
		progressBar->setFormat(str);
		progressBar->setRange(min,max);
		progressBar->setValue(val);
	}
}

void Scenery3dMgr::loadScene(const SceneInfo& scene)
{
	scenery3d->setLoadCancel(true);

	//If currently loading, we have to wait until it is finished
	//This currently blocks the GUI thread until the loading can be canceled
	//  (which is for now rather rough-grained and so may take a while)
	currentLoadFuture.waitForFinished();
	scenery3d->setLoadCancel(false);

	if(progressBar)
	{
		//kinda hack: it can be that this here is executed before loadSceneBackground is called
		//so we push the call back into the queue to ensure correct execution order
		QMetaObject::invokeMethod(this,"loadScene",Qt::QueuedConnection,Q_ARG(SceneInfo, scene));
		return;
	}

	currentLoadScene = scene;

	// Loading may take a while...
	showMessage(QString(N_("Loading scene. Please be patient!")));
	progressBar = StelApp::getInstance().addProgressBar();
	progressBar->setFormat(QString(N_("Loading scene '%1'")).arg(scene.name));
	progressBar->setValue(0);

	QFuture<bool> future = QtConcurrent::run(this,&Scenery3dMgr::loadSceneBackground);
	currentLoadFuture.setFuture(future);
}

bool Scenery3dMgr::loadSceneBackground()
{
	bool val = scenery3d->loadScene(currentLoadScene);
	return val;
}

void Scenery3dMgr::loadSceneCompleted()
{
	bool ok = currentLoadFuture.result();

	progressBar->setValue(100);
	StelApp::getInstance().removeProgressBar(progressBar);
	progressBar=NULL;

	if(!ok)
	{
		showMessage(N_("Could not load scene, please check log for error messages!"));
		return;
	}
	else
		showMessage(N_("Scene successfully loaded"));

	//do stuff that requires the main thread

	//move to the location specified by the scene
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	bool landscapeSetsLocation=lmgr->getFlagLandscapeSetsLocation();
	lmgr->setFlagLandscapeSetsLocation(true);
	lmgr->setCurrentLandscapeName(currentLoadScene.landscapeName, 0.); // took a second, implicitly.
	// Switched to immediate landscape loading: Else,
	// Landscape and Navigator at this time have old coordinates! But it should be possible to
	// delay rot_z computation up to this point and live without an own location section even
	// with meridian_convergence=from_grid.
	lmgr->setFlagLandscapeSetsLocation(landscapeSetsLocation); // restore


	if (currentLoadScene.hasLocation())
	{
		qDebug() << "Scenery3D: Setting location to given coordinates.";
		StelApp::getInstance().getCore()->moveObserverTo(*(currentLoadScene.location.data()), 0., 0.);
	}
	else qDebug() << "Scenery3D: No coordinates given in scenery3d.";

	if (currentLoadScene.hasLookAtFOV())
	{
		qDebug() << "Scenery3D: Setting orientation.";
		StelMovementMgr* mm=StelApp::getInstance().getCore()->getMovementMgr();
		Vec3f lookat=currentLoadScene.lookAt_fov;
		// This vector is (az_deg, alt_deg, fov_deg)
		Vec3d v;
		StelUtils::spheToRect(lookat[0]*M_PI/180.0, lookat[1]*M_PI/180.0, v);
		mm->setViewDirectionJ2000(StelApp::getInstance().getCore()->altAzToJ2000(v, StelCore::RefractionOff));
		mm->zoomTo(lookat[2]);
	} else qDebug() << "Scenery3D: Not setting orientation, no data.";


	//perform GL upload + other calculations that require the location to be set
	scenery3d->finalizeLoad();

	//clear loading scene
	SceneInfo s = currentLoadScene;
	currentLoadScene = SceneInfo();

	//show the scene
	setEnableScene(true);

	emit currentSceneChanged(s);
}

SceneInfo Scenery3dMgr::loadScenery3dByID(const QString& id)
{
	if (id.isEmpty())
		return SceneInfo();

	SceneInfo scene;
	try
	{
		if(!SceneInfo::loadByID(id,scene))
		{
			showMessage(N_("Could not load scene info, please check log for error messages!"));
			return SceneInfo();
		}
	}
	catch (std::runtime_error& e)
	{
		//TODO do away with the exceptions if possible
		qCritical() << "ERROR while loading 3D scenery with id " <<  id  << ", (" << e.what() << ")";
	}

	loadScene(scene);
	return scene;
}

SceneInfo Scenery3dMgr::loadScenery3dByName(const QString& name)
{
	if (name.isEmpty())
	    return SceneInfo();

	QString id = SceneInfo::getIDFromName(name);

	if(id.isEmpty())
	{
		showMessage(QString(N_("Could not find scene ID for %1")).arg(name));
		return SceneInfo();
	}
	return loadScenery3dByID(id);
}

SceneInfo Scenery3dMgr::getCurrentScene() const
{
	return scenery3d->getCurrentScene();
}

void Scenery3dMgr::setDefaultScenery3dID(const QString& id)
{
    defaultScenery3dID = id;

    conf->setValue(S3D_CONFIG_PREFIX + "/default_location_id", id);
}

void Scenery3dMgr::setEnableScene(const bool enable)
{
	if(enable && ! getCurrentScene().isValid)
	{
		//check if a default scene is set and load that
		QString id = getDefaultScenery3dID();
		if(!id.isEmpty())
		{
			if(!currentLoadScene.isValid)
				loadScenery3dByID(id);
			return;
		}
		else
		{
			flagEnabled = false;
			showMessage("Please load a scene first!");
			emit enableSceneChanged(false);
		}
	}
	if(enable!=flagEnabled)
	{
		flagEnabled=enable;
		if (scenery3d->getCubemapSize()==0)
		{
			//TODO FS: remove this?
			if (flagEnabled)
			{
				oldProjectionType= StelApp::getInstance().getCore()->getCurrentProjectionType();
				StelApp::getInstance().getCore()->setCurrentProjectionType(StelCore::ProjectionPerspective);
			}
			else
				StelApp::getInstance().getCore()->setCurrentProjectionType(oldProjectionType);
		}

		emit enableSceneChanged(flagEnabled);
	}
}

bool Scenery3dMgr::getEnablePixelLighting() const
{
	return scenery3d->getPixelLightingEnabled();
}

void Scenery3dMgr::setEnablePixelLighting(const bool val)
{
	if(val != getEnablePixelLighting())
	{
		if(!val)
		{
			setEnableBumps(false);
			setEnableShadows(false);
		}
		showMessage(QString(N_("Per-Pixel shading %1")).arg(val? N_("on") : N_("off")));

		scenery3d->setPixelLightingEnabled(val);
		conf->setValue(S3D_CONFIG_PREFIX + "/flag_pixel_lighting", val);
		emit enablePixelLightingChanged(val);
	}
}

bool Scenery3dMgr::getEnableShadows() const
{
	return scenery3d->getShadowsEnabled();
}

void Scenery3dMgr::setEnableShadows(const bool enableShadows)
{
	if(enableShadows != getEnableShadows())
	{
		if (scenery3d->getShadowmapSize() && getEnablePixelLighting())
		{
			showMessage(QString(N_("Shadows %1")).arg(enableShadows? N_("on") : N_("off")));
			scenery3d->setShadowsEnabled(enableShadows);
			emit enableShadowsChanged(enableShadows);
		} else
		{
			showMessage(QString(N_("Shadows deactivated or not possible.")));
			scenery3d->setShadowsEnabled(false);
			emit enableShadowsChanged(false);
		}

		conf->setValue(S3D_CONFIG_PREFIX + "/flag_shadow",getEnableShadows());
	}
}

bool Scenery3dMgr::getEnableBumps() const
{
	return scenery3d->getBumpsEnabled();
}

void Scenery3dMgr::setEnableBumps(const bool enableBumps)
{
	if(enableBumps != getEnableBumps())
	{
		showMessage(QString(N_("Surface bumps %1")).arg(enableBumps? N_("on") : N_("off")));
		scenery3d->setBumpsEnabled(enableBumps);

		conf->setValue(S3D_CONFIG_PREFIX + "/flag_bumpmap", enableBumps);
		emit enableBumpsChanged(enableBumps);
	}
}

S3DEnum::ShadowFilterQuality Scenery3dMgr::getShadowFilterQuality() const
{
    return scenery3d->getShadowFilterQuality();
}

void Scenery3dMgr::setShadowFilterQuality(const S3DEnum::ShadowFilterQuality val)
{
	S3DEnum::ShadowFilterQuality oldVal = getShadowFilterQuality();
	if(oldVal == val)
		return;

	QString msg(N_("Shadow filter quality: %1"));
	QString type;

	switch (val) {
		case S3DEnum::SFQ_HIGH:
			type = N_("high");
			break;
		case S3DEnum::SFQ_LOW:
			type = N_("low");
			break;
		default:
			type = N_("off");
	}

	showMessage(msg.arg(type));

	conf->setValue(S3D_CONFIG_PREFIX + "/shadow_filter_quality",val);
	scenery3d->setShadowFilterQuality(val);
	emit shadowFilterQualityChanged(val);
}

bool Scenery3dMgr::getEnablePCSS() const
{
	return scenery3d->getPCSS();
}

void Scenery3dMgr::setEnablePCSS(const bool val)
{
	scenery3d->setPCSS(val);
	conf->setValue(S3D_CONFIG_PREFIX + "/flag_pcss",val);

	emit enablePCSSChanged(val);
}

S3DEnum::CubemappingMode Scenery3dMgr::getCubemappingMode() const
{
	return scenery3d->getCubemappingMode();
}

void Scenery3dMgr::setCubemappingMode(const S3DEnum::CubemappingMode val)
{
	scenery3d->setCubemappingMode(val);

	conf->setValue(S3D_CONFIG_PREFIX + "/cubemap_mode",val);
	emit cubemappingModeChanged(val);
}

S3DEnum::CubemapShadowMode Scenery3dMgr::getCubemapShadowMode() const
{
	return scenery3d->getCubemapShadowMode();
}

void Scenery3dMgr::setCubemapShadowMode(const S3DEnum::CubemapShadowMode val)
{
	scenery3d->setCubemapShadowMode(val);

	conf->setValue(S3D_CONFIG_PREFIX + "/cubemap_shadow_mode",val);
	emit cubemapShadowModeChanged(val);
}

bool Scenery3dMgr::getEnableDebugInfo() const
{
	return scenery3d->getDebugEnabled();
}

void Scenery3dMgr::setEnableDebugInfo(const bool debugEnabled)
{
	if(debugEnabled != getEnableDebugInfo())
	{
		scenery3d->setDebugEnabled(debugEnabled);
		emit enableDebugInfoChanged(debugEnabled);
	}
}

bool Scenery3dMgr::getEnableLocationInfo() const
{
	return scenery3d->getLocationInfoEnabled();
}

void Scenery3dMgr::setEnableLocationInfo(const bool enableLocationInfo)
{
	if(enableLocationInfo != getEnableLocationInfo())
	{
		scenery3d->setLocationInfoEnabled(enableLocationInfo);
		emit enableLocationInfoChanged(enableLocationInfo);
	}
}

bool Scenery3dMgr::getEnableTorchLight() const
{
	return scenery3d->getTorchEnabled();
}

void Scenery3dMgr::setEnableTorchLight(const bool enableTorchLight)
{
	if(enableTorchLight != getEnableTorchLight())
	{
		scenery3d->setTorchEnabled(enableTorchLight);
		emit enableTorchLightChanged(enableTorchLight);
	}
}

float Scenery3dMgr::getTorchStrength() const
{
	return scenery3d->getTorchBrightness();
}

void Scenery3dMgr::setTorchStrength(const float torchStrength)
{
	scenery3d->setTorchBrightness(torchStrength);

	conf->setValue(S3D_CONFIG_PREFIX + "/torch_brightness",torchStrength);

	emit torchStrengthChanged(torchStrength);
}

float Scenery3dMgr::getTorchRange() const
{
	return scenery3d->getTorchRange();
}

void Scenery3dMgr::setTorchRange(const float torchRange)
{
	scenery3d->setTorchRange(torchRange);

	conf->setValue(S3D_CONFIG_PREFIX+ "/torch_range",torchRange);

	emit torchRangeChanged(torchRange);
}

bool Scenery3dMgr::getEnableLazyDrawing() const
{
	return scenery3d->getLazyCubemapEnabled();
}

void Scenery3dMgr::setEnableLazyDrawing(const bool val)
{
	showMessage(QString(N_("Lazy cubemapping: %1")).arg(val?N_("enabled"):N_("disabled")));
	scenery3d->setLazyCubemapEnabled(val);

	conf->setValue(S3D_CONFIG_PREFIX + "/flag_lazy_cubemap",val);
	emit enableLazyDrawingChanged(val);
}

double Scenery3dMgr::getLazyDrawingInterval() const
{
	return scenery3d->getLazyCubemapInterval();
}

void Scenery3dMgr::setLazyDrawingInterval(const double val)
{
	scenery3d->setLazyCubemapInterval(val);

	conf->setValue(S3D_CONFIG_PREFIX + "/cubemap_lazy_interval",val);
	emit lazyDrawingIntervalChanged(val);
}

bool Scenery3dMgr::getOnlyDominantFaceWhenMoving() const
{
	bool v1,v2;
	scenery3d->getLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,v2);
	return v1;
}

void Scenery3dMgr::setOnlyDominantFaceWhenMoving(const bool val)
{
	bool v1,v2;
	scenery3d->getLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,v2);
	scenery3d->setLazyCubemapUpdateOnlyDominantFaceOnMoving(val,v2);

	conf->setValue(S3D_CONFIG_PREFIX + "/flag_lazy_dominantface",val);
	emit onlyDominantFaceWhenMovingChanged(val);
}

bool Scenery3dMgr::getSecondDominantFaceWhenMoving() const
{
	bool v1,v2;
	scenery3d->getLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,v2);
	return v2;
}

void Scenery3dMgr::setSecondDominantFaceWhenMoving(const bool val)
{
	bool v1,v2;
	scenery3d->getLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,v2);
	scenery3d->setLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,val);

	conf->setValue(S3D_CONFIG_PREFIX + "/flag_lazy_seconddominantface",val);
	emit secondDominantFaceWhenMovingChanged(val);
}

void Scenery3dMgr::forceCubemapRedraw()
{
	scenery3d->invalidateCubemap();
}

uint Scenery3dMgr::getCubemapSize() const
{
	return scenery3d->getCubemapSize();
}

void Scenery3dMgr::setCubemapSize(const uint val)
{
	if(val != getCubemapSize())
	{
		scenery3d->setCubemapSize(val);
		showMessage(N_("Cubemap size changed"));

		conf->setValue(S3D_CONFIG_PREFIX + "/cubemap_size",val);
		emit cubemapSizeChanged(val);
	}
}

uint Scenery3dMgr::getShadowmapSize() const
{
	return scenery3d->getShadowmapSize();
}

void Scenery3dMgr::setShadowmapSize(const uint val)
{
	if(val != getShadowmapSize())
	{
		scenery3d->setShadowmapSize(val);
		showMessage(N_("Shadowmap size changed"));

		conf->setValue(S3D_CONFIG_PREFIX + "/shadowmap_size",val);
		emit shadowmapSizeChanged(val);
	}
}

bool Scenery3dMgr::getIsGeometryShaderSupported() const
{
	return scenery3d->isGeometryShaderCubemapSupported();
}

void Scenery3dMgr::setView(const StoredView &view)
{
	//update position
	//important: set eye height first
	scenery3d->setEyeHeight(view.position[3]);
	//then, set grid position
	scenery3d->setGridPosition(Vec3d(view.position[0],view.position[1],view.position[2]));

	//update view vector
	StelMovementMgr* mm=StelApp::getInstance().getCore()->getMovementMgr();

	// This vector is (az_deg, alt_deg, fov_deg)
	Vec3d v;
	StelUtils::spheToRect(view.view_fov[0]*M_PI/180.0, view.view_fov[1]*M_PI/180.0, v);
	mm->setViewDirectionJ2000(StelApp::getInstance().getCore()->altAzToJ2000(v, StelCore::RefractionOff));
	mm->zoomTo(view.view_fov[2]);
}

StoredView Scenery3dMgr::getCurrentView()
{
	StoredView view;
	view.isGlobal = false;
	StelCore* core = StelApp::getInstance().getCore();
	StelMovementMgr* mm = core->getMovementMgr();

	//get view vec
	Vec3d vd = mm->getViewDirectionJ2000();
	//convert to alt/az format
	vd = core->j2000ToAltAz(vd, StelCore::RefractionOff);
	//convert to spherical angles
	StelUtils::rectToSphe(&view.view_fov[0],&view.view_fov[1],vd);
	//convert to degrees
	view.view_fov[0]*=180.0/M_PI;
	view.view_fov[1]*=180.0/M_PI;
	//3rd comp is fov
	view.view_fov[2] = mm->getAimFov();

	//get current grid pos + eye height
	Vec3d pos = scenery3d->getCurrentGridPosition();
	view.position[0] = pos[0];
	view.position[1] = pos[1];
	view.position[2] = pos[2];
	view.position[3] = scenery3d->getEyeHeight();

	return view;
}

void Scenery3dMgr::showMessage(const QString& message)
{
    currentMessage=message;
    messageFader=true;
    messageTimer->start();
}

void Scenery3dMgr::clearMessage()
{
	qDebug() << "Scenery3dMgr::clearMessage";
	messageFader = false;
}

/////////////////////////////////////////////////////////////////////
StelModule* Scenery3dStelPluginInterface::getStelModule() const
{
	return new Scenery3dMgr();
}

StelPluginInfo Scenery3dStelPluginInterface::getPluginInfo() const
{
    // Allow to load the resources when used as a static plugin
    Q_INIT_RESOURCE(Scenery3d);

    StelPluginInfo info;
    info.id = "Scenery3dMgr"; // TBD: Find way to call it just Scenery3d? [cosmetic]
    info.version = SCENERY3D_PLUGIN_VERSION;
    info.displayedName = N_("3D Sceneries");
    info.authors = "Georg Zotti, Simon Parzer, Peter Neubauer, Andrei Borza";
    info.contact = "Georg.Zotti@univie.ac.at";
    info.description = N_("3D foreground renderer. Walk around, find and avoid obstructions in your garden, find and demonstrate possible astronomical alignments in temples, see shadows on sundials etc.");
    return info;
}
/////////////////////////////////////////////////////////////////////

