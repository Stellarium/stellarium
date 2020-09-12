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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
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

#include "Scenery3d.hpp"
#include "Scenery3dRemoteControlService.hpp"
#include "S3DRenderer.hpp"
#include "S3DScene.hpp"
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
#include "StelMainView.hpp"

Q_LOGGING_CATEGORY(scenery3d,"stel.plugin.scenery3d")

#define S3D_CONFIG_PREFIX QString("Scenery3d")

Scenery3d::Scenery3d() :
	renderer(Q_NULLPTR),
	flagEnabled(false),
	cleanedUp(false),
	movementKeyInput(0.0,0.0,0.0),
	oldProjectionType(StelCore::ProjectionPerspective),
	forceHorizonPolyline(false),
	loadCancel(false),
	progressBar(Q_NULLPTR),
	currentLoadScene(),
	currentScene(Q_NULLPTR),
	currentLoadFuture(this)
{
	setObjectName("Scenery3d");
	scenery3dDialog = new Scenery3dDialog();
	storedViewDialog = new StoredViewDialog();

	font.setPixelSize(16);
	messageFader.setDuration(500);
	messageTimer = new QTimer(this);
	messageTimer->setInterval(2000);
	messageTimer->setSingleShot(true);
	connect(messageTimer, &QTimer::timeout, this, &Scenery3d::clearMessage);
	connect(&currentLoadFuture,&QFutureWatcherBase::finished, this, &Scenery3d::loadSceneCompleted);

	connect(this, &Scenery3d::progressReport, this, &Scenery3d::progressReceive, Qt::QueuedConnection);

	//get the global configuration object
	conf = StelApp::getInstance().getSettings();

	core = StelApp::getInstance().getCore();
	mvMgr = GETSTELMODULE(StelMovementMgr);

	//create scenery3d object
	renderer = new S3DRenderer();
	connect(renderer, SIGNAL(message(QString)), this, SLOT(showMessage(QString)));
}

Scenery3d::~Scenery3d()
{
	if(!cleanedUp)
		deinit();

	delete storedViewDialog;
	delete scenery3dDialog;
}

double Scenery3d::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName) + 5; // between Landscape and compass marks!
	if (actionName == StelModule::ActionUpdate)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName) + 10;
	if (actionName == StelModule::ActionHandleKeys)
		return 3; // GZ: low number means high precedence!
	return 0;
}

void Scenery3d::handleKeys(QKeyEvent* e)
{
	if (!flagEnabled)
		return;

	static const Qt::KeyboardModifier S3D_SPEEDBASE_MODIFIER = Qt::ShiftModifier;

	//on OSX, there is a still-unfixed bug which prevents the command key (=Qt's Control key) to be used here
	//see https://bugreports.qt.io/browse/QTBUG-36839
	//we have to use the option/ALT key instead to activate walking around, and CMD is used as multiplier.
#ifdef Q_OS_OSX
	static const Qt::KeyboardModifier S3D_CTRL_MODIFIER = Qt::AltModifier;
	static const Qt::KeyboardModifier S3D_SPEEDMUL_MODIFIER = Qt::ControlModifier;
#else
	static const Qt::KeyboardModifier S3D_CTRL_MODIFIER = Qt::ControlModifier;
	static const Qt::KeyboardModifier S3D_SPEEDMUL_MODIFIER = Qt::AltModifier;
#endif

	if ((e->type() == QKeyEvent::KeyPress) && (e->modifiers() & S3D_CTRL_MODIFIER))
	{
		// Pressing CTRL+ALT: 5x, CTRL+SHIFT: 10x speedup; CTRL+SHIFT+ALT: 50x!
		float speedup=((e->modifiers() & S3D_SPEEDBASE_MODIFIER)? 10.0f : 1.0f);
		speedup *= ((e->modifiers() & S3D_SPEEDMUL_MODIFIER)? 5.0f : 1.0f);

		switch (e->key())
		{
			case Qt::Key_Plus:      // or
			case Qt::Key_PageUp:    movementKeyInput[2] =  1.0f * speedup; e->accept(); break;
			case Qt::Key_Minus:     // or
			case Qt::Key_PageDown:  movementKeyInput[2] = -1.0f * speedup; e->accept(); break;
			case Qt::Key_Up:        movementKeyInput[1] =  1.0f * speedup; e->accept(); break;
			case Qt::Key_Down:      movementKeyInput[1] = -1.0f * speedup; e->accept(); break;
			case Qt::Key_Right:     movementKeyInput[0] =  1.0f * speedup; e->accept(); break;
			case Qt::Key_Left:      movementKeyInput[0] = -1.0f * speedup; e->accept(); break;
#ifdef QT_DEBUG
				//leave this out on non-debug builds to reduce conflict chance
			case Qt::Key_P:         renderer->saveFrusts(); e->accept(); break;
#endif
		}
	}
	// FS: No modifier here!? GZ: I want the lock feature. If this does not work for MacOS, it is not there, but only on that platform...
#ifdef Q_OS_OSX
	else if ((e->type() == QKeyEvent::KeyRelease) )
#else
	else if ((e->type() == QKeyEvent::KeyRelease) && (e->modifiers() & S3D_CTRL_MODIFIER))
#endif
	{
		//if a movement key is released, stop moving in that direction
		//we do not accept the event on MacOS to allow further handling the event in other modules. (Else the regular view motion stop does not work!)
		switch (e->key())
		{
			case Qt::Key_Plus:
			case Qt::Key_PageUp:
			case Qt::Key_Minus:
			case Qt::Key_PageDown:
				movementKeyInput[2] = 0.0f;
#ifndef Q_OS_OSX
				e->accept();
#endif
				break;
			case Qt::Key_Up:
			case Qt::Key_Down:
				movementKeyInput[1] = 0.0f;
#ifndef Q_OS_OSX
				e->accept();
#endif
				break;
			case Qt::Key_Right:
			case Qt::Key_Left:
				movementKeyInput[0] = 0.0f;
#ifndef Q_OS_OSX
				e->accept();
#endif
				break;
		}
	}
}


void Scenery3d::update(double deltaTime)
{
	if (flagEnabled && currentScene)
	{
		//update view direction
		Vec3d mainViewDir = mvMgr->getViewDirectionJ2000();
		mainViewDir = core->j2000ToAltAz(mainViewDir, StelCore::RefractionOff);
		currentScene->setViewDirection(mainViewDir);

		//perform movement
		//when zoomed in more than 5°, we slow down movement
		if(movementKeyInput.lengthSquared() > 0.00001)
			currentScene->moveViewer(movementKeyInput * deltaTime * 0.01 * std::max(5.0, mvMgr->getCurrentFov()));

		//update material fade info, if necessary
		double curTime = core->getJD();
		S3DScene::MaterialList& matList = currentScene->getMaterialList();
		for(int i = 0; i<matList.size();++i)
		{
			S3DScene::Material& mat = matList[i];
			if(mat.traits.hasTimeFade)
				mat.updateFadeInfo(curTime);
		}
	}

	messageFader.update(static_cast<int>(deltaTime*1000));
}

void Scenery3d::draw(StelCore* core)
{
	if (flagEnabled && currentScene)
	{
		renderer->draw(core,*currentScene);
	}

	if (forceHorizonPolyline)
		GETSTELMODULE(LandscapeMgr)->drawPolylineOnly(core); // Allow a check with background line.

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

void Scenery3d::init()
{
	qCDebug(scenery3d) << "Scenery3d plugin - press KGA button to toggle 3D scenery, KGA tool button for settings";

	//Initialize the renderer - this also finds out what features are supported
	qCDebug(scenery3d) << "Initializing Scenery3d renderer...";
	renderer->init();
	qCDebug(scenery3d) << "Initializing Scenery3d renderer...done";

	//make sure shadows are off if unsupported
	if(! renderer->areShadowsSupported())
		setEnableShadows(false);
	if(! renderer->isShadowFilteringSupported())
	{
		setShadowFilterQuality(S3DEnum::SFQ_OFF);
		setEnablePCSS(false);
	}

	//load config and create interface actions
	loadConfig();
	createActions();
	createToolbarButtons();

	connect(&StelMainView::getInstance(), SIGNAL(reloadShadersRequested()), this, SLOT(reloadShaders()));

	//finally, hook up the lightscape toggle event (external to this plugin) to cubemap redraw
	StelAction* action = StelApp::getInstance().getStelActionManager()->findAction("actionShow_LandscapeIllumination");
	Q_ASSERT(action);
	connect(action, &StelAction::toggled, this, &Scenery3d::forceCubemapRedraw);

#ifndef NDEBUG
	showMessage(q_("Scenery3d plugin loaded!"));
#endif
}

void Scenery3d::deinit()
{
	//wait until loading is finished. (If test added after hint from Coverity)
	if(renderer)
	{
		loadCancel = true;
		currentLoadFuture.waitForFinished();
	}
	//this is correct the place to delete all OpenGL related stuff, not the destructor
	delete renderer;
	renderer = Q_NULLPTR;
	delete currentScene;
	currentScene = Q_NULLPTR;

	cleanedUp = true;
}

void Scenery3d::loadConfig()
{
	conf->beginGroup(S3D_CONFIG_PREFIX);

	textColor = Vec3f(conf->value("text_color", "0.5,0.5,1").toString());
	renderer->setCubemappingMode( static_cast<S3DEnum::CubemappingMode>(conf->value("cubemap_mode",0).toInt()) );
	renderer->setCubemapSize(conf->value("cubemap_size",2048).toInt());
	renderer->setShadowmapSize(conf->value("shadowmap_size", 1024).toInt());
	renderer->setShadowFilterQuality( static_cast<S3DEnum::ShadowFilterQuality>(conf->value("shadow_filter_quality", 1).toInt()) );
	renderer->setPCSS(conf->value("flag_pcss").toBool());
	renderer->setTorchEnabled(conf->value("torch_enabled", false).toBool());
	renderer->setTorchBrightness(conf->value("torch_brightness", 0.5f).toFloat());
	renderer->setTorchRange(conf->value("torch_range",5.0f).toFloat());
	renderer->setBumpsEnabled(conf->value("flag_bumpmap", false).toBool());
	renderer->setShadowsEnabled(conf->value("flag_shadow", false).toBool());
	renderer->setUseSimpleShadows(conf->value("flag_shadow_simple", false).toBool());
	renderer->setUseFullCubemapShadows(conf->value("flag_cubemap_fullshadows", false).toBool());
	renderer->setLazyCubemapEnabled(conf->value("flag_lazy_cubemap", true).toBool());
	renderer->setLazyCubemapInterval(conf->value("cubemap_lazy_interval",1.0).toDouble());
	renderer->setPixelLightingEnabled(conf->value("flag_pixel_lighting", false).toBool());
	renderer->setLocationInfoEnabled(conf->value("flag_location_info", false).toBool());

	forceHorizonPolyline = conf->value("force_landscape_polyline", false).toBool();

	bool v1 = conf->value("flag_lazy_dominantface",false).toBool();
	bool v2 = conf->value("flag_lazy_seconddominantface",true).toBool();
	renderer->setLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,v2);

	defaultScenery3dID = conf->value("default_location_id","").toString();

	conf->endGroup();
}

void Scenery3d::createActions()
{
	QString groupName = N_("Scenery3d: 3D landscapes");

	//enable action will be set checkable if a scene was loaded
	addAction("actionShow_Scenery3d",                  groupName, N_("Toggle 3D landscape"),      this,          "enableScene",       "Ctrl+W");
	addAction("actionShow_Scenery3d_dialog",           groupName, N_("Show settings dialog"),  scenery3dDialog,  "visible",           "Ctrl+Shift+W");
	addAction("actionShow_Scenery3d_storedViewDialog", groupName, N_("Show viewpoint dialog"), storedViewDialog, "visible",           "Ctrl+Alt+W");
	addAction("actionShow_Scenery3d_shadows",          groupName, N_("Toggle shadows"),           this,          "enableShadows",     "Ctrl+R, S");
	addAction("actionShow_Scenery3d_debuginfo",        groupName, N_("Toggle debug information"), this,          "enableDebugInfo",   "Ctrl+R, D");
	addAction("actionShow_Scenery3d_locationinfo",     groupName, N_("Toggle location text"),     this,          "enableLocationInfo","Ctrl+R, T");
	addAction("actionShow_Scenery3d_torchlight",       groupName, N_("Toggle torchlight"),        this,          "enableTorchLight",  "Ctrl+R, L");
}

void Scenery3d::createToolbarButtons() const
{
	// Add 3 toolbar buttons (copy/paste widely from AngleMeasure): activate, settings, and viewpoints.
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());

		if (gui!=Q_NULLPTR)
		{
			StelButton* toolbarEnableButton =	new StelButton(Q_NULLPTR,
									       QPixmap(":/Scenery3d/bt_scenery3d_on.png"),
									       QPixmap(":/Scenery3d/bt_scenery3d_off.png"),
									       QPixmap(":/graphicGui/miscGlow32x32.png"),
									       "actionShow_Scenery3d");
			StelButton* toolbarSettingsButton =	new StelButton(Q_NULLPTR,
									       QPixmap(":/Scenery3d/bt_scenery3d_settings_on.png"),
									       QPixmap(":/Scenery3d/bt_scenery3d_settings_off.png"),
									       QPixmap(":/graphicGui/miscGlow32x32.png"),
									       "actionShow_Scenery3d_dialog");
			StelButton* toolbarStoredViewButton =	new StelButton(Q_NULLPTR,
									       QPixmap(":/Scenery3d/bt_scenery3d_eyepoint_on.png"),
									       QPixmap(":/Scenery3d/bt_scenery3d_eyepoint_off.png"),
									       QPixmap(":/graphicGui/miscGlow32x32.png"),
									       "actionShow_Scenery3d_storedViewDialog");

			gui->getButtonBar()->addButton(toolbarEnableButton, "065-pluginsGroup");
			gui->getButtonBar()->addButton(toolbarSettingsButton, "065-pluginsGroup");
			gui->getButtonBar()->addButton(toolbarStoredViewButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qCWarning(scenery3d) << "WARNING: unable to create toolbar buttons for Scenery3d plugin: " << e.what();
	}
}

void Scenery3d::relativeMove(const Vec3d &move)
{
	if(currentScene)
	{
		currentScene->moveViewer(move);
	}
}

void Scenery3d::reloadShaders()
{
	showMessage(q_("Scenery3d shaders reloaded"));
	qCDebug(scenery3d)<<"Reloading Scenery3d shaders";

	renderer->getShaderManager().clearCache();
}

bool Scenery3d::configureGui(bool show)
{
	if (show)
		scenery3dDialog->setVisible(show);
	return true;
}

void Scenery3d::showStoredViewDialog()
{
	storedViewDialog->setVisible(true);
}

void Scenery3d::updateProgress(const QString &str, int val, int min, int max) const
{
	emit progressReport(str,val,min,max);
}

void Scenery3d::progressReceive(const QString &str, int val, int min, int max)
{
	//update progress bar
	if(progressBar)
	{
		progressBar->setFormat(str);
		progressBar->setRange(min,max);
		progressBar->setValue(val);
	}
}

void Scenery3d::loadScene(const SceneInfo& scene)
{
	loadCancel = true;

	//If currently loading, we have to wait until it is finished
	//This currently blocks the GUI thread until the loading can be canceled
	//  (which is for now rather rough-grained and so may take a while)
	currentLoadFuture.waitForFinished();
	loadCancel = false;

	if(progressBar)
	{
		//kinda hack: it can be that this here is executed before loadSceneBackground is called
		//so we push the call back into the queue to ensure correct execution order
		QMetaObject::invokeMethod(this,"loadScene",Qt::QueuedConnection,Q_ARG(SceneInfo, scene));
		return;
	}

	// Loading may take a while...
	showMessage(QString(q_("Loading scene. Please be patient!")));
	progressBar = StelApp::getInstance().addProgressBar();
	progressBar->setFormat(QString(q_("Loading scene '%1'")).arg(scene.name));
	progressBar->setValue(0);

	currentLoadScene = scene;
	emit loadingSceneIDChanged(currentLoadScene.id);

	QFuture<S3DScene*> future = QtConcurrent::run(this,&Scenery3d::loadSceneBackground,scene);
	currentLoadFuture.setFuture(future);
}

S3DScene* Scenery3d::loadSceneBackground(const SceneInfo& scene) const
{
	//the scoped pointer ensures this scene is deleted when errors occur
	QScopedPointer<S3DScene> newScene(new S3DScene(scene));

	if(loadCancel)
		return Q_NULLPTR;

	updateProgress(q_("Loading model..."),1,0,6);

	//load model
	StelOBJ modelOBJ;
	QString modelFile = StelFileMgr::findFile( scene.fullPath+ "/" + scene.modelScenery);
	qCDebug(scenery3d)<<"Loading scene from "<<modelFile;
	if(!modelOBJ.load(modelFile, scene.vertexOrderEnum))
	{
	    qCCritical(scenery3d)<<"Failed to load OBJ file"<<modelFile;
	    return Q_NULLPTR;
	}

	if(loadCancel)
		return Q_NULLPTR;

	updateProgress(q_("Transforming model..."),2,0,6);
	newScene->setModel(modelOBJ);

	if(loadCancel)
		return Q_NULLPTR;

	if(scene.modelGround.isEmpty())
	{
		updateProgress(q_("Calculating collision map..."),5,0,6);
		newScene->setGround(modelOBJ);
	}
	else if (scene.modelGround != "NULL")
	{
		updateProgress(q_("Loading ground..."),3,0,6);

		StelOBJ groundOBJ;
		modelFile = StelFileMgr::findFile(scene.fullPath + "/" + scene.modelGround);
		qCDebug(scenery3d)<<"Loading ground from"<<modelFile;
		if(!groundOBJ.load(modelFile, scene.vertexOrderEnum))
		{
			qCCritical(scenery3d)<<"Failed to load ground model"<<modelFile;
			return Q_NULLPTR;
		}

		updateProgress(q_("Transforming ground..."),4,0,6);
		if(loadCancel)
			return Q_NULLPTR;

		updateProgress(q_("Calculating collision map..."),5,0,6);
		newScene->setGround(groundOBJ);
	}

	if(loadCancel)
		return Q_NULLPTR;

	updateProgress(q_("Finalizing load..."),6,0,6);

	return newScene.take();
}

void Scenery3d::loadSceneCompleted()
{
	S3DScene* result = currentLoadFuture.result();

	progressBar->setValue(100);
	StelApp::getInstance().removeProgressBar(progressBar);
	progressBar=Q_NULLPTR;

	if(!result)
	{
		showMessage(q_("Could not load scene, please check log for error messages!"));
		return;
	}
	else
		showMessage(q_("Scene successfully loaded."));

	//do stuff that requires the main thread
	const SceneInfo& info = result->getSceneInfo();

	//move to the location specified by the scene
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	const bool landscapeSetsLocation=lmgr->getFlagLandscapeSetsLocation();
	lmgr->setFlagLandscapeSetsLocation(true);
	lmgr->setCurrentLandscapeName(info.landscapeName, 0.); // took a second, implicitly.
	// Switched to immediate landscape loading: Else,
	// Landscape and Navigator at this time have old coordinates! But it should be possible to
	// delay rot_z computation up to this point and live without an own location section even
	// with meridian_convergence=from_grid.
	lmgr->setFlagLandscapeSetsLocation(landscapeSetsLocation); // restore


	if (info.hasLocation())
	{
		qCDebug(scenery3d) << "Setting location to given coordinates";
		StelApp::getInstance().getCore()->moveObserverTo(*info.location, 0., 0.);
	}
	else qCDebug(scenery3d) << "No coordinates given in scenery3d.ini";

	if (info.hasLookAtFOV())
	{
		qCDebug(scenery3d) << "Setting orientation";
		Vec3f lookat=currentLoadScene.lookAt_fov;
		// This vector is (az_deg, alt_deg, fov_deg)
		Vec3d v;
		StelUtils::spheToRect(lookat[0]*M_PI/180.0, lookat[1]*M_PI/180.0, v);
		mvMgr->setViewDirectionJ2000(StelApp::getInstance().getCore()->altAzToJ2000(v, StelCore::RefractionOff));
		mvMgr->zoomTo(lookat[2]);
	} else qCDebug(scenery3d) << "No orientation given in scenery3d.ini";

	//clear loading scene
	currentLoadScene = SceneInfo();
	emit loadingSceneIDChanged(QString());

	//switch scenes
	delete currentScene;
	currentScene = result;

	//show the scene
	setEnableScene(true);

	emit currentSceneChanged(info);
	emit currentSceneIDChanged(info.id);
}

SceneInfo Scenery3d::loadScenery3dByID(const QString& id)
{
	if (id.isEmpty())
		return SceneInfo();

	SceneInfo scene;
	try
	{
		if(!SceneInfo::loadByID(id,scene))
		{
			showMessage(q_("Could not load scene info, please check log for error messages!"));
			return SceneInfo();
		}
	}
	catch (std::runtime_error& e)
	{
		//TODO do away with the exceptions if possible
		qCCritical(scenery3d) << "ERROR while loading 3D scenery with id " <<  id  << ", (" << e.what() << ")";
		return SceneInfo();
	}

	loadScene(scene);
	return scene;
}

SceneInfo Scenery3d::loadScenery3dByName(const QString& name)
{
	if (name.isEmpty())
	    return SceneInfo();

	QString id = SceneInfo::getIDFromName(name);

	if(id.isEmpty())
	{
		showMessage(QString(q_("Could not find scene ID for %1")).arg(name));
		return SceneInfo();
	}
	return loadScenery3dByID(id);
}

SceneInfo Scenery3d::getCurrentScene() const
{
	if(currentScene)
		return currentScene->getSceneInfo();
	return SceneInfo();
}

QString Scenery3d::getCurrentSceneID() const
{
	return currentScene ? currentScene->getSceneInfo().id : QString();
}

QString Scenery3d::getLoadingSceneID() const
{
	return currentLoadScene.id;
}

void Scenery3d::setDefaultScenery3dID(const QString& id)
{
	defaultScenery3dID = id;

	conf->setValue(S3D_CONFIG_PREFIX + "/default_location_id", id);
}

void Scenery3d::setEnableScene(const bool enable)
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
			showMessage(q_("Please load a scene first!"));
			emit enableSceneChanged(false);
		}
	}
	if(enable!=flagEnabled)
	{
		flagEnabled=enable;
		if (renderer->getCubemapSize()==0)
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

bool Scenery3d::getEnablePixelLighting() const
{
	return renderer->getPixelLightingEnabled();
}

void Scenery3d::setEnablePixelLighting(const bool val)
{
	if(val != getEnablePixelLighting())
	{
		if(!val)
		{
			setEnableBumps(false);
			setEnableShadows(false);
		}
		showMessage(QString(q_("Per-Pixel shading %1.")).arg(val? qc_("on","enable") : qc_("off","disable")));

		renderer->setPixelLightingEnabled(val);
		conf->setValue(S3D_CONFIG_PREFIX + "/flag_pixel_lighting", val);
		emit enablePixelLightingChanged(val);
	}
}

bool Scenery3d::getEnableShadows() const
{
	return renderer->getShadowsEnabled();
}

void Scenery3d::setEnableShadows(const bool enableShadows)
{
	if(enableShadows != getEnableShadows())
	{
		if (renderer->getShadowmapSize() && getEnablePixelLighting())
		{
			showMessage(QString(q_("Shadows %1.")).arg(enableShadows? qc_("on","enable") : qc_("off","disable")));
			renderer->setShadowsEnabled(enableShadows);
			emit enableShadowsChanged(enableShadows);
		} else
		{
			showMessage(QString(q_("Shadows deactivated or not possible.")));
			renderer->setShadowsEnabled(false);
			emit enableShadowsChanged(false);
		}

		conf->setValue(S3D_CONFIG_PREFIX + "/flag_shadow",getEnableShadows());
	}
}

bool Scenery3d::getUseSimpleShadows() const
{
	return renderer->getUseSimpleShadows();
}

void Scenery3d::setUseSimpleShadows(const bool simpleShadows)
{
	renderer->setUseSimpleShadows(simpleShadows);

	conf->setValue(S3D_CONFIG_PREFIX + "/flag_shadow_simple",simpleShadows);
	emit useSimpleShadowsChanged(simpleShadows);
}

bool Scenery3d::getEnableBumps() const
{
	return renderer->getBumpsEnabled();
}

void Scenery3d::setEnableBumps(const bool enableBumps)
{
	if(enableBumps != getEnableBumps())
	{
		showMessage(QString(q_("Surface bumps %1.")).arg(enableBumps? qc_("on","enable") : qc_("off","disable")));
		renderer->setBumpsEnabled(enableBumps);

		conf->setValue(S3D_CONFIG_PREFIX + "/flag_bumpmap", enableBumps);
		emit enableBumpsChanged(enableBumps);
	}
}

S3DEnum::ShadowFilterQuality Scenery3d::getShadowFilterQuality() const
{
	return renderer->getShadowFilterQuality();
}

void Scenery3d::setShadowFilterQuality(const S3DEnum::ShadowFilterQuality val)
{
	S3DEnum::ShadowFilterQuality oldVal = getShadowFilterQuality();
	if(oldVal == val)
		return;

	conf->setValue(S3D_CONFIG_PREFIX + "/shadow_filter_quality",val);
	renderer->setShadowFilterQuality(val);
	emit shadowFilterQualityChanged(val);
}

bool Scenery3d::getEnablePCSS() const
{
	return renderer->getPCSS();
}

void Scenery3d::setEnablePCSS(const bool val)
{
	renderer->setPCSS(val);
	conf->setValue(S3D_CONFIG_PREFIX + "/flag_pcss",val);

	emit enablePCSSChanged(val);
}

S3DEnum::CubemappingMode Scenery3d::getCubemappingMode() const
{
	return renderer->getCubemappingMode();
}

void Scenery3d::setCubemappingMode(const S3DEnum::CubemappingMode val)
{
	renderer->setCubemappingMode(val);
	S3DEnum::CubemappingMode realVal = renderer->getCubemappingMode();

	if(val!=realVal)
		showMessage(q_("Selected cubemap mode not supported, falling back to '6 Textures'"));

	conf->setValue(S3D_CONFIG_PREFIX + "/cubemap_mode",realVal);
	emit cubemappingModeChanged(realVal);
}

bool Scenery3d::getUseFullCubemapShadows() const
{
	return renderer->getUseFullCubemapShadows();
}

void Scenery3d::setUseFullCubemapShadows(const bool useFullCubemapShadows)
{
	renderer->setUseFullCubemapShadows(useFullCubemapShadows);

	conf->setValue(S3D_CONFIG_PREFIX + "/flag_cubemap_fullshadows",useFullCubemapShadows);
	emit useFullCubemapShadowsChanged(useFullCubemapShadows);
}

bool Scenery3d::getEnableDebugInfo() const
{
	return renderer->getDebugEnabled();
}

void Scenery3d::setEnableDebugInfo(const bool debugEnabled)
{
	if(debugEnabled != getEnableDebugInfo())
	{
		renderer->setDebugEnabled(debugEnabled);
		emit enableDebugInfoChanged(debugEnabled);
	}
}

bool Scenery3d::getEnableLocationInfo() const
{
	return renderer->getLocationInfoEnabled();
}

void Scenery3d::setEnableLocationInfo(const bool enableLocationInfo)
{
	if(enableLocationInfo != getEnableLocationInfo())
	{
		renderer->setLocationInfoEnabled(enableLocationInfo);

		conf->setValue(S3D_CONFIG_PREFIX + "/flag_location_info",enableLocationInfo);

		emit enableLocationInfoChanged(enableLocationInfo);
	}
}

void Scenery3d::setForceHorizonPolyline(const bool forcePolyline)
{
	if(forcePolyline != getForceHorizonPolyline())
	{
		forceHorizonPolyline=forcePolyline;

		conf->setValue(S3D_CONFIG_PREFIX + "/force_landscape_polyline",forcePolyline);

		emit forceHorizonPolylineChanged(forcePolyline);
	}
}

bool Scenery3d::getForceHorizonPolyline() const
{
	return forceHorizonPolyline;
}

bool Scenery3d::getEnableTorchLight() const
{
	return renderer->getTorchEnabled();
}

void Scenery3d::setEnableTorchLight(const bool enableTorchLight)
{
	if(enableTorchLight != getEnableTorchLight())
	{
		renderer->setTorchEnabled(enableTorchLight);

		conf->setValue(S3D_CONFIG_PREFIX + "/torch_enabled",enableTorchLight);

		emit enableTorchLightChanged(enableTorchLight);
	}
}

float Scenery3d::getTorchStrength() const
{
	return renderer->getTorchBrightness();
}

void Scenery3d::setTorchStrength(const float torchStrength)
{
	renderer->setTorchBrightness(torchStrength);

	conf->setValue(S3D_CONFIG_PREFIX + "/torch_brightness",torchStrength);

	emit torchStrengthChanged(torchStrength);
}

float Scenery3d::getTorchRange() const
{
	return renderer->getTorchRange();
}

void Scenery3d::setTorchRange(const float torchRange)
{
	renderer->setTorchRange(torchRange);

	conf->setValue(S3D_CONFIG_PREFIX+ "/torch_range",torchRange);

	emit torchRangeChanged(torchRange);
}

bool Scenery3d::getEnableLazyDrawing() const
{
	return renderer->getLazyCubemapEnabled();
}

void Scenery3d::setEnableLazyDrawing(const bool val)
{
	showMessage(QString(q_("Lazy cubemapping: %1")).arg(val?q_("enabled"):q_("disabled")));
	renderer->setLazyCubemapEnabled(val);

	conf->setValue(S3D_CONFIG_PREFIX + "/flag_lazy_cubemap",val);
	emit enableLazyDrawingChanged(val);
}

double Scenery3d::getLazyDrawingInterval() const
{
	return renderer->getLazyCubemapInterval();
}

void Scenery3d::setLazyDrawingInterval(const double val)
{
	renderer->setLazyCubemapInterval(val);

	conf->setValue(S3D_CONFIG_PREFIX + "/cubemap_lazy_interval",val);
	emit lazyDrawingIntervalChanged(val);
}

bool Scenery3d::getOnlyDominantFaceWhenMoving() const
{
	bool v1,v2;
	renderer->getLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,v2);
	return v1;
}

void Scenery3d::setOnlyDominantFaceWhenMoving(const bool val)
{
	bool v1,v2;
	renderer->getLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,v2);
	renderer->setLazyCubemapUpdateOnlyDominantFaceOnMoving(val,v2);

	conf->setValue(S3D_CONFIG_PREFIX + "/flag_lazy_dominantface",val);
	emit onlyDominantFaceWhenMovingChanged(val);
}

bool Scenery3d::getSecondDominantFaceWhenMoving() const
{
	bool v1,v2;
	renderer->getLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,v2);
	return v2;
}

void Scenery3d::setSecondDominantFaceWhenMoving(const bool val)
{
	bool v1,v2;
	renderer->getLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,v2);
	renderer->setLazyCubemapUpdateOnlyDominantFaceOnMoving(v1,val);

	conf->setValue(S3D_CONFIG_PREFIX + "/flag_lazy_seconddominantface",val);
	emit secondDominantFaceWhenMovingChanged(val);
}

void Scenery3d::forceCubemapRedraw()
{
	renderer->invalidateCubemap();
}

uint Scenery3d::getCubemapSize() const
{
	return renderer->getCubemapSize();
}

void Scenery3d::setCubemapSize(const uint val)
{
	if(val != getCubemapSize())
	{
		renderer->setCubemapSize(val);

		//hardware may not support the value, get real value set
		uint realVal = renderer->getCubemapSize();
		if(realVal==val)
			showMessage(q_("Cubemap size changed"));
		else
			showMessage(QString(q_("Cubemap size not supported, set to %1")).arg(realVal));

		conf->setValue(S3D_CONFIG_PREFIX + "/cubemap_size",realVal);
		emit cubemapSizeChanged(realVal);
	}
}

uint Scenery3d::getShadowmapSize() const
{
	return renderer->getShadowmapSize();
}

void Scenery3d::setShadowmapSize(const uint val)
{
	if(val != getShadowmapSize())
	{
		renderer->setShadowmapSize(val);

		//hardware may not support the value, get real value set
		uint realVal = renderer->getShadowmapSize();
		if(realVal==val)
			showMessage(q_("Shadowmap size changed"));
		else
			showMessage(QString(q_("Shadowmap size not supported, set to %1")).arg(realVal));

		conf->setValue(S3D_CONFIG_PREFIX + "/shadowmap_size",realVal);
		emit shadowmapSizeChanged(realVal);
	}
}

bool Scenery3d::getIsGeometryShaderSupported() const
{
	return renderer->isGeometryShaderCubemapSupported();
}

bool Scenery3d::getAreShadowsSupported() const
{
	return renderer->areShadowsSupported();
}

bool Scenery3d::getIsShadowFilteringSupported() const
{
	return renderer->isShadowFilteringSupported();
}

bool Scenery3d::getIsANGLE() const
{
	return renderer->isANGLEContext();
}

uint Scenery3d::getMaximumFramebufferSize() const
{
	return renderer->getMaximumFramebufferSize();
}

void Scenery3d::setView(const StoredView &view, const bool setDate)
{
	if(!currentScene)
	{
		qCWarning(scenery3d)<<"Can't set current view, no scene loaded!";
		return;
	}

	//update position
	//important: set eye height first
	currentScene->setEyeHeight(view.position[3]);
	//then, set grid position
	currentScene->setGridPosition(Vec3d(view.position[0],view.position[1],view.position[2]));

	//update time, if relevant and wanted.
	if (view.jdIsRelevant && setDate)
	{
		StelCore *core=StelApp::getInstance().getCore();
		core->setJD(view.jd);
	}

	//update view vector
	// This vector is (az_deg, alt_deg, fov_deg)
	Vec3d v;
	StelUtils::spheToRect((180.0-view.view_fov[0])*M_PI/180.0, view.view_fov[1]*M_PI/180.0, v);
	mvMgr->setViewDirectionJ2000(StelApp::getInstance().getCore()->altAzToJ2000(v, StelCore::RefractionOff));
	mvMgr->zoomTo(view.view_fov[2]);
}

StoredView Scenery3d::getCurrentView()
{
	if(!currentScene)
	{
		qCWarning(scenery3d)<<"Can't return current view, no scene loaded!";
		return StoredView();
	}

	StoredView view;
	view.isGlobal = false;
	StelCore* core = StelApp::getInstance().getCore();

	//get view vec
	Vec3d vd = mvMgr->getViewDirectionJ2000();
	//convert to alt/az format
	vd = core->j2000ToAltAz(vd, StelCore::RefractionOff);
	//convert to spherical angles
	StelUtils::rectToSphe(&view.view_fov[0],&view.view_fov[1],vd);
	//convert to degrees
	view.view_fov[0]*=180.0/M_PI;
	view.view_fov[1]*=180.0/M_PI;
	// we must patch azimuth
	view.view_fov[0]=180.0-view.view_fov[0];
	//3rd comp is fov
	view.view_fov[2] = mvMgr->getAimFov();

	//get current grid pos + eye height
	Vec3d pos = currentScene->getGridPosition();
	view.position[0] = pos[0];
	view.position[1] = pos[1];
	view.position[2] = pos[2];
	view.position[3] = currentScene->getEyeHeight();

	return view;
}

void Scenery3d::showMessage(const QString& message)
{
	currentMessage=message;
	messageFader=true;
	messageTimer->start();
}

void Scenery3d::clearMessage()
{
	messageFader = false;
}

/////////////////////////////////////////////////////////////////////
StelModule* Scenery3dStelPluginInterface::getStelModule() const
{
	return new Scenery3d();
}

StelPluginInfo Scenery3dStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(Scenery3d);

	StelPluginInfo info;
	info.id = "Scenery3d";
	info.version = SCENERY3D_PLUGIN_VERSION;
	info.license = SCENERY3D_PLUGIN_LICENSE;
	info.displayedName = N_("3D Sceneries");
	info.authors = "Georg Zotti, Simon Parzer, Peter Neubauer, Andrei Borza, Florian Schaukowitsch";
	info.contact = "https://homepage.univie.ac.at/Georg.Zotti";
	info.description = N_("<p>3D foreground renderer. Walk around, find and avoid obstructions in your garden, "
			      "find and demonstrate possible astronomical alignments in temples, see shadows on sundials etc.</p>"
			      "<p>To move around, press Ctrl+cursor keys. To lift eye height, use Ctrl+PgUp/PgDn. "
			      "Movement speed is linked to field of view (i.e. zoom in for fine adjustments). "
			      "You can even keep moving by releasing Ctrl before cursor key.</p>");
	info.acknowledgements=N_("Development of this plugin was in parts supported by the Austrian Science Fund (FWF) project ASTROSIM (P 21208-G19; https://astrosim.univie.ac.at/). <br/>"
				 "Further development is in parts supported by the Ludwig Boltzmann Institute for Archaeological Prospection and Virtual Archaeology, Vienna, Austria (http://archpro.lbg.ac.at/).");
	return info;
}

QObjectList Scenery3dStelPluginInterface::getExtensionList() const
{
	QObjectList ret;
	ret.append(new Scenery3dRemoteControlService());
	return ret;
}
/////////////////////////////////////////////////////////////////////

