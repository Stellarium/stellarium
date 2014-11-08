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
    debugShader(NULL)
{
    setObjectName("Scenery3dMgr");
    scenery3dDialog = new Scenery3dDialog();
    enableShadows=false;
    enableBumps=false;
    enableShadowsFilter=true;
    enableShadowsFilterHQ=false;
    // taken from AngleMeasure:
    textColor = StelUtils::strToVec3f(StelApp::getInstance().getSettings()->value("options/text_color", "0,0.5,1").toString());
    font.setPixelSize(16);
    messageTimer = new QTimer(this);
    messageTimer->setInterval(2000);
    messageTimer->setSingleShot(true);
    connect(messageTimer, SIGNAL(timeout()), this, SLOT(clearMessage()));
}

Scenery3dMgr::~Scenery3dMgr()
{
    //the deletion of many objects can be handled by Qt automatically, if a parent of QObject is set
    //scenery3d is no QObject
    delete scenery3d;
    scenery3d = NULL;

	delete scenery3dDialog;
}

void Scenery3dMgr::setScenery3dEnabled(const bool enable)
{
    if(enable!=flagEnabled)
    {
        flagEnabled=enable;
        if (cubemapSize==0)
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

        emit scenery3dEnabledChanged(flagEnabled);
    }
}

bool Scenery3dMgr::isScenery3dEnabled() const
{
    return flagEnabled;
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
        scenery3d->handleKeys(e);
        if (!e->isAccepted()) {
            // handle keys for CTRL-SPACE and CTRL-B. Allows easier interaction with GUI.

            if ((e->type() == QKeyEvent::KeyPress) && (e->modifiers() & Qt::ControlModifier))
            {
                switch (e->key())
                {
                    case Qt::Key_Space:
                        if (shadowmapSize)
                        {
                            enableShadows = !enableShadows;
                            scenery3d->setShadowsEnabled(enableShadows);
                            showMessage(QString(N_("Shadows %1")).arg(enableShadows? N_("on") : N_("off")));
                        } else
                        {
                            showMessage(QString(N_("Shadows deactivated or not possible.")));
                        }
                        e->accept();
                        break;
                    case Qt::Key_B:
                        enableBumps   = !enableBumps;
                        scenery3d->setBumpsEnabled(enableBumps);
                        showMessage(QString(N_("Surface bumps %1")).arg(enableBumps? N_("on") : N_("off")));
                        e->accept();
                        break;

                    case Qt::Key_I:
                        if(enableShadows)
                        {
                            enableShadowsFilter = !enableShadowsFilter;
                            scenery3d->setShadowsFilterEnabled(enableShadowsFilter);
                            showMessage(QString(N_("Filtering shadows %1")).arg(enableShadowsFilter? N_("on") : N_("off")));
                        } else
                        {
                            showMessage(QString(N_("Please activate shadows first.")));
                        }
                        e->accept();
                        break;
                    case Qt::Key_U:
                        if(enableShadows)
                        {
                            if(enableShadowsFilter)
                            {
                                enableShadowsFilterHQ = !enableShadowsFilterHQ;
                                scenery3d->setShadowsFilterHQEnabled(enableShadowsFilterHQ);
                                showMessage(QString(N_("Improved shadows filtering %1")).arg(enableShadowsFilterHQ? N_("on") : N_("off")));
                            } else
                            {
                                showMessage(QString(N_("Please activate shadows filtering first.")));
                            }
                        } else
                        {
                            showMessage(QString(N_("Please activate shadows first.")));
                        }
                        e->accept();
                        break;

                }
            }
        }
    }
}


void Scenery3dMgr::update(double deltaTime)
{
    if (flagEnabled)
    {
        scenery3d->update(deltaTime);
        messageFader.update((int)(deltaTime*1000));
    }
}

void Scenery3dMgr::draw(StelCore* core)
{
    if (flagEnabled)
    {
        scenery3d->draw(core);
        if (messageFader.getInterstate() > 0.000001f)
        {

            const StelProjectorP prj = core->getProjection(StelCore::FrameEquinoxEqu);
            StelPainter painter(prj);
            painter.setFont(font);
            painter.setColor(textColor[0], textColor[1], textColor[2], messageFader.getInterstate());
            painter.drawText(83, 120, currentMessage);
        }
    }
}

void Scenery3dMgr::init()
{
    qDebug() << "Scenery3d plugin - press KGA button to toggle 3D scenery, KGA tool button for settings";

    QSettings* conf = StelApp::getInstance().getSettings();
    Q_ASSERT(conf);
    cubemapSize=conf->value("Scenery3d/cubemapSize", 2048).toInt();
    shadowmapSize=conf->value("Scenery3d/shadowmapSize", 1024).toInt();
    torchBrightness=conf->value("Scenery3d/extralight_brightness", 0.5f).toFloat();

    // graphics hardware without FrameBufferObj extension cannot use the cubemap rendering and shadow mapping.
    // In this case, set cubemapSize to 0 to signal auto-switch to perspective projection.
    if ( !QOpenGLContext::currentContext()->hasExtension("GL_EXT_framebuffer_object")) {

        //TODO FS: it seems like the current stellarium requires a working framebuffer extension anyway, so skip this check?

        qWarning() << "Scenery3d: Your hardware does not support EXT_framebuffer_object.";
        qWarning() << "           Shadow mapping disabled, and display limited to perspective projection.";
        cubemapSize=0;
        shadowmapSize=0;
    }    
    //cubemapSize = 0;
    // create action for enable/disable & hook up signals

    //hook up some signals
    StelApp *app = &StelApp::getInstance();
    connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
    connect(app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
    connect(this, SIGNAL(enabledChanged(bool)), this, SLOT(setEnabled(bool)));

    qDebug() << "call addActions()";

    addAction("actionShow_Scenery3d",
              N_("Scenery3d: 3D landscapes"),
              N_("Show 3D landscape"),
              this,
              "scenery3dEnabled",
              "Ctrl+3");

    addAction("actionShow_Scenery3d_dialog",       // ID
              N_("Scenery3d: 3D landscapes"),        // Group ID (for help)
              N_("Show settings dialog"),                 // help text
              scenery3dDialog,                       // target
              "visible",          // slot
              "Ctrl+Shift+3");                       // shortcut1

    // Add 2 toolbar buttons (copy/paste widely from AngleMeasure): activate, and settings.
    try
    {
        StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());

        toolbarEnableButton = new StelButton(NULL, QPixmap(":/Scenery3d/bt_scenery3d_on.png"),
                                             QPixmap(":/Scenery3d/bt_scenery3d_off.png"),
                                             QPixmap(":/graphicGui/glow32x32.png"),
                                             "actionShow_Scenery3d");
        toolbarSettingsButton = new StelButton(NULL, QPixmap(":/Scenery3d/bt_scenery3d_settings_on.png"),
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

    loadShaders();



    qWarning() << "init scenery3d object...";
    scenery3d = new Scenery3d(cubemapSize, shadowmapSize, torchBrightness);
    scenery3d->setShaders(shadowShader, bumpShader, univShader, debugShader);

    qWarning() << "init scenery3d object...done.\n";
    scenery3d->setShadowsEnabled(enableShadows);
    scenery3d->setBumpsEnabled(enableBumps);

    //Initialize Shadow Mapping
    qWarning() << "init scenery3d object shadowmapping.";
    if(shadowmapSize){
        scenery3d->initShadowMapping();
    }
    else
        qWarning() << "Note: shadowmapping disabled by zero size.\n";

    qWarning() << "init scenery3d object shadowmapping...done\n";
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

void Scenery3dMgr::setStelStyle(const QString& section)
{
	(void)section; // disable compiler warning (unused var)
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
        qWarning() << "ERROR while loading 3D scenery " << MODULE_PATH + id + "/scenery3d.ini" << ", (" << e.what() << ")";
    }

    if (!newScenery3d)
        return false;

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

void Scenery3dMgr::updateI18n()
{
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
    QString s;
    Scenery3d* newScenery3d = new Scenery3d(cubemapSize, shadowmapSize, torchBrightness);
    newScenery3d->setShaders(shadowShader, bumpShader, univShader, debugShader);
    newScenery3d->setShadowsEnabled(enableShadows);
    newScenery3d->setBumpsEnabled(enableBumps);
    if(shadowmapSize) newScenery3d->initShadowMapping();
    if (scenery3dIni.status() != QSettings::NoError)
    {
        qWarning() << "ERROR parsing scenery3d.ini file: " << scenery3dFile;
        s = "";
    }
    else
    {
        newScenery3d->loadConfig(scenery3dIni, scenery3dID);
    }
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

void Scenery3dMgr::setEnableShadows(bool enableShadows)
{
    this->enableShadows = enableShadows;
    if (scenery3d != NULL) {
        scenery3d->setShadowsEnabled(enableShadows);
    }
}

void Scenery3dMgr::setEnableBumps(bool enableBumps)
{
    this->enableBumps = enableBumps;
    if(scenery3d != NULL)
    {
        scenery3d->setBumpsEnabled(enableBumps);
    }
}

void Scenery3dMgr::setEnableShadowsFilter(bool enableShadowsFilter)
{
    this->enableShadowsFilter = enableShadowsFilter;
    if (scenery3d != NULL) {
        scenery3d->setShadowsFilterEnabled(enableShadowsFilter);
    }
}

void Scenery3dMgr::setEnableShadowsFilterHQ(bool enableShadowsFilterHQ)
{
    this->enableShadowsFilterHQ = enableShadowsFilterHQ;
    if (scenery3d != NULL) {
        scenery3d->setShadowsFilterHQEnabled(enableShadowsFilterHQ);
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
    info.displayedName = N_("3D Sceneries");
    info.authors = "Georg Zotti, Simon Parzer, Peter Neubauer, Andrei Borza";
    info.contact = "Georg.Zotti@univie.ac.at";
    info.description = N_("3D foreground renderer. Walk around, find and avoid obstructions in your garden, find and demonstrate possible astronomical alignments in temples, see shadows on sundials etc.");
    return info;
}
/////////////////////////////////////////////////////////////////////

