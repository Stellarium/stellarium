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
#include <QTimer>
#include <QOpenGLShaderProgram>
#include <QtConcurrent>

#include <stdexcept>

#include "Scenery3dMgr.hpp"
#include "Scenery3d.hpp"
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

Scenery3dMgr::Scenery3dMgr() :
    scenery3d(NULL),
    flagEnabled(false),
    cleanedUp(false),
    debugShader(NULL),
    progressBar(NULL),
    currentLoadFuture(this)
{
    setObjectName("Scenery3dMgr");
    scenery3dDialog = new Scenery3dDialog();

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

	//on startup, make this not checkable because no scene is loaded
	this->enableAction->setCheckable(false);

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
	setEnableShadows(false);
	setEnableShadowsFilter(true);
	setEnableShadowsFilterHQ(false);

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

		gui->getButtonBar()->addButton(toolbarEnableButton, "065-pluginsGroup");
		gui->getButtonBar()->addButton(toolbarSettingsButton, "065-pluginsGroup");
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to create toolbar buttons for Scenery3d plugin: " << e.what();
	}
}

void Scenery3dMgr::deinit()
{
	qDebug()<<"Scenery3dMgr deinit";

	//this is correct the place to delete all OpenGL related stuff, not the destructor
	if(scenery3d != NULL)
	{
		delete scenery3d;
		scenery3d = NULL;
	}

	//delete shaders
	if(!debugShader)
	{
		delete debugShader;
		debugShader = NULL;
	}

	cleanedUp = true;
}

void Scenery3dMgr::loadConfig()
{
	conf = StelApp::getInstance().getSettings();

	conf->beginGroup("Scenery3d");

	textColor = StelUtils::strToVec3f(conf->value("text_color", "0.5,0.5,1").toString());
	scenery3d->setCubemapSize(conf->value("cubemapSize",2048).toInt());
	scenery3d->setShadowmapSize(conf->value("shadowmapSize", 1024).toInt());
	scenery3d->setTorchBrightness(conf->value("extralight_brightness", 0.5f).toFloat());

	conf->endGroup();
}

void Scenery3dMgr::createActions()
{
	QString groupName = N_("Scenery3d: 3D landscapes");

	//TODO make some of these a GUI setting instead of a switch (quality options, etc)
	//also add cubemap/shadowmap size (=quality) options to GUI?

	//enable action will be set checkable if a scene was loaded
	this->enableAction = addAction("actionShow_Scenery3d", groupName, N_("Toggle 3D landscape"),this,"enableScene","Ctrl+3");
	addAction("actionShow_Scenery3d_dialog", groupName, N_("Show settings dialog"), scenery3dDialog, "visible", "Ctrl+Shift+3");
	addAction("actionShow_Scenery3d_shadows", groupName, N_("Toggle shadows"), this, "enableShadows","Ctrl+R, S");
	addAction("actionSwitch_Scenery3d_shadowfilter", groupName, N_("Toggle shadow filtering"), this, "enableShadowsFilter","Ctrl+R, F");
	addAction("actionSwitch_Scenery3d_shadowfilterhq", groupName, N_("Toggle shadow filtering quality"), this, "enableShadowsFilterHQ","Ctrl+R, Q");
	addAction("actionShow_Scenery3d_debuginfo", groupName, N_("Toggle debug information"), this, "enableDebugInfo","Ctrl+R, D");
	addAction("actionShow_Scenery3d_locationinfo", groupName, N_("Toggle location text"), this, "enableLocationInfo","Ctrl+R, T");
	addAction("actionShow_Scenery3d_torchlight", groupName, N_("Toggle torchlight"), this, "enableTorchLight", "Ctrl+R, L");
	addAction("actionReload_Scenery3d_shaders", groupName, N_("Reload shaders"), this, "reloadShaders()", "Ctrl+R, P");
}

void Scenery3dMgr::reloadShaders()
{
	showMessage(N_("Scenery3d shaders reloaded"));

	qDebug()<<"(Re)loading Scenery3d shaders";
	//create shader objects, if not existing
	//the shaders get this manager as parent, so we dont have to manage deletion ourselves, it is done by Qt.
	if(!debugShader)
		debugShader = new QOpenGLShaderProgram(this);

	loadShader(*debugShader,"debug.v.glsl","debug.f.glsl");

	scenery3d->setShaders(debugShader);
	scenery3d->getShaderManager().clearCache();
}

bool Scenery3dMgr::loadShader(QOpenGLShaderProgram& program, const QString& vShader, const QString& fShader)
{
    qDebug()<<"Loading Scenery3d shader: '"<<vShader<<"', '"<<fShader<<"'";

    //clear old shader data, if exists
    program.removeAllShaders();

    QDir dir("data/shaders/");
    QString vs = StelFileMgr::findFile(dir.filePath(vShader),StelFileMgr::File);

    if(!program.addShaderFromSourceFile(QOpenGLShader::Vertex,vs))
    {
	qCritical() << "Scenery3d: unable to compile " << vs << " vertex shader file";
	qCritical() << program.log();
	return false;
    }
    else
    {
	QString log = program.log().trimmed();
	if(!log.isEmpty())
	{
	    qWarning()<<vShader<<" warnings:";
	    qWarning()<<log;
	}
    }

    QString fs = StelFileMgr::findFile(dir.filePath(fShader),StelFileMgr::File);
    if(!program.addShaderFromSourceFile(QOpenGLShader::Fragment,fs))
    {
	qCritical() << "Scenery3d: unable to compile " << fs << " fragment shader file";
	qCritical() << program.log();
	return false;
    }
    else
    {
	QString log = program.log().trimmed();
	if(!log.isEmpty())
	{
	    qWarning()<<fShader<<" warnings:";
	    qWarning()<<log;
	}
    }

    //link program
    if(!program.link())
    {
	qCritical()<<"Scenery3d: unable to link shader files "<<vShader<<", "<<fShader;
	qCritical()<<program.log();
	return false;
    }

    return true;
}

bool Scenery3dMgr::configureGui(bool show)
{
	if (show)
		scenery3dDialog->setVisible(show);
    return true;
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
		mm->zoomTo(lookat[2], 0.);
	} else qDebug() << "Scenery3D: Not setting orientation, no data.";


	//perform GL upload + other calculations that require the location to be set
	scenery3d->finalizeLoad();

	enableAction->setCheckable(true);
	setEnableScene(true);
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

bool Scenery3dMgr::setDefaultScenery3dID(const QString& id)
{
    if (id.isEmpty())
	return false;
    defaultScenery3dID = id;

    conf->setValue("init_location/scenery3d_name", id);
    return true;
}

void Scenery3dMgr::setEnableScene(const bool enable)
{
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
		if (scenery3d->getShadowmapSize())
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
		emit enableBumpsChanged(enableBumps);
	}
}

bool Scenery3dMgr::getEnableShadowsFilter() const
{
	return scenery3d->getShadowsFilterEnabled();
}

void Scenery3dMgr::setEnableShadowsFilter(const bool enableShadowsFilter)
{
	if(enableShadowsFilter != getEnableShadowsFilter())
	{
		showMessage(QString(N_("Shadow filter quality: %1")).arg(enableShadowsFilter? (getEnableShadowsFilterHQ() ? N_("high"):N_("low")) : N_("off")));
		scenery3d->setShadowsFilterEnabled(enableShadowsFilter);
		emit enableShadowsFilterChanged(enableShadowsFilter);
	}
}

bool Scenery3dMgr::getEnableShadowsFilterHQ() const
{
	return scenery3d->getShadowsFilterHQEnabled();
}

void Scenery3dMgr::setEnableShadowsFilterHQ(const bool enableShadowsFilterHQ)
{
	if(enableShadowsFilterHQ != getEnableShadowsFilterHQ())
	{
		scenery3d->setShadowsFilterHQEnabled(enableShadowsFilterHQ);
		showMessage(QString(N_("Shadow filter quality: %1")).arg(getEnableShadowsFilter()? (enableShadowsFilterHQ ? N_("high"):N_("low")) : N_("off")));
		if(enableShadowsFilterHQ) // turn on normal filtering
			setEnableShadowsFilter(true);
		emit enableShadowsFilterHQChanged(enableShadowsFilterHQ);
	}
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

bool Scenery3dMgr::getEnableGeometryShader() const
{
	return scenery3d->getGeometryShaderCubemapEnabled();
}

void Scenery3dMgr::setEnableGeometryShader(const bool enableGeometryShader)
{
	if(enableGeometryShader != getEnableGeometryShader())
	{
		bool val = enableGeometryShader;
		if( ! scenery3d->isGeometryShaderCubemapSupported())
		{
			val = false;
		}
		scenery3d->setGeometryShaderCubemapEnabled(val);
		showMessage(QString(N_("Geometry shader support: %1")).arg(val?"enabled":"disabled"));
		emit enableGeometryShaderChanged(val);
	}
}

bool Scenery3dMgr::getIsGeometryShaderSupported() const
{
	return scenery3d->isGeometryShaderCubemapSupported();
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

