#include <QDebug>
#include <QSettings>
#include <QString>
#include <QDir>
#include <QFile>

#include <stdexcept>

#include <GLee.h>

#include "Scenery3dMgr.hpp"
#include "Scenery3d.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelPainter.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "LandscapeMgr.hpp"

const QString Scenery3dMgr::MODULE_PATH("modules/scenery3d/");

Scenery3dMgr::Scenery3dMgr() : scenery3d(NULL)
{
    setObjectName("Scenery3dMgr");
    scenery3dDialog = new Scenery3dDialog();

    shadowShader = 0;
}

Scenery3dMgr::~Scenery3dMgr()
{
    delete scenery3d;
    scenery3d = NULL;
    delete scenery3dDialog;
    delete shadowShader;
}

void Scenery3dMgr::enableScenery3d(bool enable)
{
    flagEnabled=enable;
    if (cubemapSize==0)
    {
        if (flagEnabled)
        {
            oldProjectionType= StelApp::getInstance().getCore()->getCurrentProjectionType();
            StelApp::getInstance().getCore()->setCurrentProjectionType(StelCore::ProjectionPerspective);
        }
        else
            StelApp::getInstance().getCore()->setCurrentProjectionType(oldProjectionType);
     }
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
            // handle keys
        }
    }
}

void Scenery3dMgr::update(double deltaTime)
{
    if (flagEnabled) scenery3d->update(deltaTime);
}

void Scenery3dMgr::draw(StelCore* core)
{
    if (flagEnabled) scenery3d->draw(core);
    //scenery3d->drawCoordinatesText(core);
}

void Scenery3dMgr::init()
{
    qDebug() << "Scenery3d plugin - press KGA button to toggle 3D scenery, KGA tool button for settings";

    QSettings* conf = StelApp::getInstance().getSettings();
    Q_ASSERT(conf);
    cubemapSize=conf->value("Scenery3d/cubemapSize", 1024).toInt();
    shadowmapSize=conf->value("Scenery3d/shadowmapSize", 1024).toInt();

    // graphics hardware without FrameBufferObj extension cannot use the cubemap rendering.
    // In this case, set cubemapSize to 0 to signal auto-switch to perspective projection.
    if (! GLEE_EXT_framebuffer_object) cubemapSize=0;

    // create action for enable/disable & hook up signals
    StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
    Q_ASSERT(gui);

    qDebug() << "call gui->addGuiActions()";
    gui->addGuiActions("actionShow_Scenery3d",
                       N_("Scenery3d: 3D landscapes"),
                       "Ctrl+3", // hotkey
                       N_("Show astronomical alignments"), // help
                       true); // checkable; autorepeat=false.
    connect(gui->getGuiActions("actionShow_Scenery3d"), SIGNAL(toggled(bool)), this, SLOT(enableScenery3d(bool)));

    gui->addGuiActions("actionShow_Scenery3d_window",
                       N_("Scenery3d configuration window"),
                       "Ctrl+Shift+3", // hotkey
                       N_("Scenery3d Selection and Settings"), // help
                       true); // checkable; autorepeat=false.
    connect(gui->getGuiActions("actionShow_Scenery3d_window"), SIGNAL(toggled(bool)), scenery3dDialog, SLOT(setVisible(bool)));
    connect(scenery3dDialog, SIGNAL(visibleChanged(bool)), gui->getGuiActions("actionShow_Scenery3d_window"), SLOT(setChecked(bool)));


    // Add 2 toolbar buttons (copy/paste widely from AngleMeasure): activate, and settings.
    try
    {
        //qDebug() << "trying buttons\n";
        toolbarEnableButton = new StelButton(NULL, QPixmap(":/Scenery3d/bt_scenery3d_on.png"),
                                      QPixmap(":/Scenery3d/bt_scenery3d_off.png"),
                                      QPixmap(":/graphicGui/glow32x32.png"),
                                      gui->getGuiActions("actionShow_Scenery3d"));
        toolbarSettingsButton = new StelButton(NULL, QPixmap(":/Scenery3d/bt_scenery3d_settings_on.png"),
                                      QPixmap(":/Scenery3d/bt_scenery3d_settings_off.png"),
                                      QPixmap(":/graphicGui/glow32x32.png"),
                                      gui->getGuiActions("actionShow_Scenery3d_window"));

       //qDebug() << "call getButtonBar etc.\n";

       gui->getButtonBar()->addButton(toolbarEnableButton, "065-pluginsGroup");
       gui->getButtonBar()->addButton(toolbarSettingsButton, "065-pluginsGroup");
    }
    catch (std::runtime_error& e)
    {
        qDebug() << "catch exc.\n";
            qWarning() << "WARNING: unable to create toolbar buttons for Scenery3d plugin: " << e.what();
    }

    //qDebug() << "past exception.\n";


    //Load shadow shader
    shadowShader = new StelShader;
    QStringList lst =  QStringList(StelFileMgr::findFileInAllPaths("data/shaders/",
      (StelFileMgr::Flags)(StelFileMgr::Directory)));
    QByteArray vshader = (QString(lst.first()) + "smap.v.glsl").toLocal8Bit();
    QByteArray fshader = (QString(lst.first()) + "smap.f.glsl").toLocal8Bit();

    if (!(shadowShader->load(vshader.data(), fshader.data())))
    {
        qWarning() << "WARNING [Scenery3d]: unable to load shader files.\n";
        shadowShader = 0;
    }

    scenery3d = new Scenery3d(cubemapSize, shadowmapSize, shadowShader);
    scenery3d->setShadowsEnabled(enableShadows);
}

bool Scenery3dMgr::configureGui(bool show)
{
    if (show)
    {
            StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
            Q_ASSERT(gui);
            gui->getGuiActions("actionShow_Scenery3d_window")->setChecked(true);
    }

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
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    newScenery3d->loadModel();
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
    Scenery3d* newScenery3d = new Scenery3d(cubemapSize, shadowmapSize, shadowShader);
    newScenery3d->setShadowsEnabled(enableShadows);
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
	info.id = "Scenery3dMgr";
	info.displayedName = "Scenery3d";
        info.authors = "Simon Parzer, Peter Neubauer, Georg Zotti";
        info.contact = "Georg.Zotti@univie.ac.at";
	info.description = "OBJ landscape renderer.";
//	info.description = "OBJ landscape renderer. This Beta compiled 2011-06-06 for Maurizio Forte for collaboration on Copan.";
	return info;
}

Q_EXPORT_PLUGIN2(Scenery3dMgr, Scenery3dStelPluginInterface)
/////////////////////////////////////////////////////////////////////

