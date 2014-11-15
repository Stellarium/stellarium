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
#include "StelIniParser.hpp"
#include "StelPainter.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "LandscapeMgr.hpp"

const QString Scenery3dMgr::MODULE_PATH("scenery3d/");

Scenery3dMgr::Scenery3dMgr() :
    scenery3d(NULL),
    shadowShader(NULL),
    bumpShader(NULL),
    univShader(NULL),
    debugShader(NULL),
    cleanedUp(false),
    flagEnabled(false)
{
    setObjectName("Scenery3dMgr");
    scenery3dDialog = new Scenery3dDialog();

    font.setPixelSize(16);
    messageFader.setDuration(500);
    messageTimer = new QTimer(this);
    messageTimer->setInterval(2000);
    messageTimer->setSingleShot(true);
    connect(messageTimer, SIGNAL(timeout()), this, SLOT(clearMessage()));

    //create scenery3d object
    //TODO FS: only 1 scenery3d object should be created throughout the lifetime of the app!
    scenery3d = new Scenery3d();
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

	loadShaders();
	setEnableShadows(false);
	setEnableShadowsFilter(true);
	setEnableShadowsFilterHQ(false);

	//Initialize Shadow Mapping
	qWarning() << "init scenery3d object shadowmapping.";
	if(scenery3d->getShadowmapSize()){
	    scenery3d->initShadowMapping();
	}
	else
	    qWarning() << "Note: shadowmapping disabled by zero size.\n";

	qWarning() << "init scenery3d object shadowmapping...done\n";

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
	if(!shadowShader)
	    delete shadowShader;
	if(!bumpShader)
	    delete bumpShader;
	if(!univShader)
	    delete univShader;
	if(!debugShader)
	    delete debugShader;

	cleanedUp = true;
}

void Scenery3dMgr::loadConfig()
{
	QSettings* conf = StelApp::getInstance().getSettings();

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
}

void Scenery3dMgr::loadShaders()
{
    qDebug()<<"(Re)loading Scenery3d shaders";
    //create shader objects, if not existing
    //the shaders get this manager as parent, so we dont have to manage deletion ourselves, it is done by Qt.
    if(!shadowShader)
	shadowShader = new QOpenGLShaderProgram(this);
    if(!bumpShader)
	bumpShader = new QOpenGLShaderProgram(this);
    if(!univShader)
	univShader = new QOpenGLShaderProgram(this);
    if(!debugShader)
	debugShader = new QOpenGLShaderProgram(this);


    loadShader(*shadowShader,"smap.v.glsl","smap.f.glsl");
    loadShader(*bumpShader,"bmap.v.glsl","bmap.f.glsl");
    loadShader(*univShader,"univ.v.glsl","univ.f.glsl");
    loadShader(*debugShader,"debug.v.glsl","debug.f.glsl");

    scenery3d->setShaders(shadowShader,bumpShader,univShader,debugShader);
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

bool Scenery3dMgr::setCurrentScenery3dID(const QString& id)
{
    if (id.isEmpty())
	return false;

    Scenery3d* newScenery3d = NULL;
    try
    {
	newScenery3d = createFromFile(StelFileMgr::findFile(MODULE_PATH + id + "/scenery3d.ini"), id);
    }
    catch (std::runtime_error& e)
    {
	qCritical() << "ERROR while loading 3D scenery " << MODULE_PATH + id + "/scenery3d.ini" << ", (" << e.what() << ")";
    }

    if (!newScenery3d)
    {
	    showMessage("Could not load scene, please check log for error messages!");
	return false;
    }

    LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
    bool landscapeSetsLocation=lmgr->getFlagLandscapeSetsLocation();
    lmgr->setFlagLandscapeSetsLocation(true);
    lmgr->setCurrentLandscapeName(newScenery3d->getLandscapeName(), 0.); // took a second, implicitly.
    // Switched to immediate landscape loading: Else,
    // Landscape and Navigator at this time have old coordinates! But it should be possible to
    // delay rot_z computation up to this point and live without an own location section even
    // with meridian_convergence=from_grid.
    lmgr->setFlagLandscapeSetsLocation(landscapeSetsLocation); // restore

    if (scenery3d)
    {
	delete scenery3d;
	scenery3d = NULL;
    }
    // Loading may take a while...
    showMessage(QString(N_("Loading scenery3d. Please be patient!")));
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    newScenery3d->loadModel();
    clearMessage();
    QApplication::restoreOverrideCursor();

    if (newScenery3d->hasLocation())
    {
	qDebug() << "Scenery3D: Setting location to given coordinates.";
	StelApp::getInstance().getCore()->moveObserverTo(newScenery3d->getLocation(), 0., 0.);
    }
    else qDebug() << "Scenery3D: No coordinates given in scenery3d.";

    if (newScenery3d->hasLookat())
    {
	qDebug() << "Scenery3D: Setting orientation.";
	StelMovementMgr* mm=StelApp::getInstance().getCore()->getMovementMgr();
	Vec3f lookat=newScenery3d->getLookat();
	// This vector is (az_deg, alt_deg, fov_deg)
	Vec3d v;
	StelUtils::spheToRect(lookat[0]*M_PI/180.0, lookat[1]*M_PI/180.0, v);
	mm->setViewDirectionJ2000(StelApp::getInstance().getCore()->altAzToJ2000(v, StelCore::RefractionOff));
	mm->zoomTo(lookat[2], 3.);
    } else qDebug() << "Scenery3D: Not setting orientation, no data.";

    scenery3d = newScenery3d;
    currentScenery3dID = id;

    enableAction->setCheckable(true);
    setEnableScene(true);

    return true;
}

bool Scenery3dMgr::setCurrentScenery3dName(const QString& name)
{
    if (name.isEmpty())
	return false;

    QMap<QString, QString> nameToDirMap = getNameToDirMap();
    if (nameToDirMap.find(name) != nameToDirMap.end())
    {
	return setCurrentScenery3dID(nameToDirMap[name]);
    }
    else
    {
	qWarning() << "Can't find a 3D scenery with name=" << name;
	return false;
    }
}

bool Scenery3dMgr::setDefaultScenery3dID(const QString& id)
{
    if (id.isEmpty())
	return false;
    defaultScenery3dID = id;
    QSettings* conf = StelApp::getInstance().getSettings();
    conf->setValue("init_location/scenery3d_name", id);
    return true;
}

QStringList Scenery3dMgr::getAllScenery3dNames() const
{
    QMap<QString, QString> nameToDirMap = getNameToDirMap();
    QStringList result;

    foreach (QString i, nameToDirMap.keys())
    {
	result += i;
    }
    return result;
}

QStringList Scenery3dMgr::getAllScenery3dIDs() const
{
    QMap<QString, QString> nameToDirMap = getNameToDirMap();
    QStringList result;

    foreach (QString i, nameToDirMap.values())
    {
	result += i;
    }
    return result;
}

QString Scenery3dMgr::getCurrentScenery3dName() const
{
    return scenery3d->getName();
}

Scenery3d* Scenery3dMgr::createFromFile(const QString& scenery3dFile, const QString& scenery3dID)
{
    QSettings scenery3dIni(scenery3dFile, StelIniFormat);
    if (scenery3dIni.status() != QSettings::NoError)
    {
	qCritical() << "ERROR parsing scenery3d.ini file: " << scenery3dFile;
	return NULL;
    }

    //TODO FS: remove the creation of a new scenery3d object
    //TODO FS: make the scene metadata a struct so that the loading of it is independent of the Scenery3d class.
    Scenery3d* newScenery3d = new Scenery3d();
    newScenery3d->setCubemapSize(scenery3d->getCubemapSize());
    newScenery3d->setShadowmapSize(scenery3d->getShadowmapSize());
    newScenery3d->setTorchBrightness(scenery3d->getTorchBrightness());
    newScenery3d->setShaders(shadowShader, bumpShader, univShader, debugShader);
    newScenery3d->setShadowsEnabled(scenery3d->getShadowsEnabled());
    newScenery3d->setBumpsEnabled(scenery3d->getBumpsEnabled());
    if(scenery3d->getShadowmapSize()) newScenery3d->initShadowMapping();
	newScenery3d->loadConfig(scenery3dIni, scenery3dID);
    return newScenery3d;
}

QString Scenery3dMgr::nameToID(const QString& name)
{
    QMap<QString, QString> nameToDirMap = getNameToDirMap();

    if (nameToDirMap.find(name) != nameToDirMap.end())
    {
	Q_ASSERT(0);
	return "error";
    }
    else
    {
	return nameToDirMap[name];
    }
}

QMap<QString, QString> Scenery3dMgr::getNameToDirMap() const
{
    qDebug() << "Scenery3dMgr::getNameToDirMap(): ";
    QSet<QString> scenery3dDirs;
    QMap<QString, QString> result;
    try
    {
	scenery3dDirs = StelFileMgr::listContents(MODULE_PATH, StelFileMgr::Directory);
	qDebug() << "dirs " << scenery3dDirs;
    }
    catch (std::runtime_error& e)
    {
	qDebug() << "ERROR while trying to list 3D sceneries:" << e.what();
    }

    foreach (const QString& dir, scenery3dDirs)
    {
	try
	{
	    QSettings scenery3dIni(StelFileMgr::findFile(MODULE_PATH + dir + "/scenery3d.ini"), StelIniFormat);
	    QString k = scenery3dIni.value("model/name").toString();
	    result[k] = dir;
	}
	catch (std::runtime_error& e)
	{
	}
    }
    return result;
}

QString Scenery3dMgr::getScenery3dPath(QString scenery3dID)
{
    QString result;
    if (scenery3dID.isEmpty())
    {
	return result;
    }

    try
    {
	result = StelFileMgr::findFile(MODULE_PATH + scenery3dID, StelFileMgr::Directory);
    }
    catch (std::runtime_error &e)
    {
	qWarning() << "Scenery3dMgr: Error! Unable to find " << scenery3dID << ": " << e.what();
	return result;
    }

    return result;
}

QString Scenery3dMgr::loadScenery3dName(QString scenery3dID)
{
    QString scenery3dName;
    if (scenery3dID.isEmpty())
    {
	qWarning() << "Scenery3dMgr: Error! No scenery3d ID passed to loadScenery3dName().";
	return scenery3dName;
    }

    QString scenery3dPath = getScenery3dPath(scenery3dID);
    if (scenery3dPath.isEmpty())
	return scenery3dName;

    QDir scenery3dDir(scenery3dPath);
    if (scenery3dDir.exists("scenery3d.ini"))
    {
	QString scenery3dSettingsPath = scenery3dDir.filePath("scenery3d.ini");
	QSettings scenery3dSettings(scenery3dSettingsPath, StelIniFormat);
	scenery3dName = scenery3dSettings.value("model/name").toString();
    }
    else
    {
	qWarning() << "Scenery3dMgr: Error! Scenery3d directory" << scenery3dPath << "does not contain a 'scenery3d.ini' file";
    }

    return scenery3dName;
}

quint64 Scenery3dMgr::loadScenery3dSize(QString scenery3dID)
{
    quint64 scenery3dSize = 0;
    if (scenery3dID.isEmpty())
    {
	qWarning() << "Scenery3dMgr: Error! No scenery3d ID passed to loadScenery3dSize().";
	return scenery3dSize;
    }

    QString scenery3dPath = getScenery3dPath(scenery3dID);
    if (scenery3dPath.isEmpty())
	return scenery3dSize;

    QDir scenery3dDir(scenery3dPath);
    foreach (QFileInfo file, scenery3dDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot))
    {
	scenery3dSize += file.size();
    }
    return scenery3dSize;
}

QString Scenery3dMgr::getCurrentScenery3dHtmlDescription() const
{
    QString desc = QString("<h3>%1</h3>").arg(scenery3d->getName());
    desc += scenery3d->getDescription();
    desc+="<br><br>";
    desc+="<b>"+q_("Author: ")+"</b>";
    desc+= scenery3d->getAuthorName();
    return desc;
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
		showMessage(QString(N_("Filtering shadows %1")).arg(enableShadowsFilter? N_("on") : N_("off")));
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
		showMessage(QString(N_("Shadow filtering quality %1")).arg(enableShadowsFilterHQ? N_("high") : N_("low")));
		scenery3d->setShadowsFilterHQEnabled(enableShadowsFilterHQ);
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

