#include <QDebug>
#include <QSettings>
#include <QString>
#include <QDir>
#include <QFile>

#include <stdexcept>

#include "Scenery3dMgr.hpp"
#include "Scenery3d.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
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
    useCubeMap = false;
}

Scenery3dMgr::~Scenery3dMgr()
{
    delete scenery3d;
    scenery3d = NULL;
    delete scenery3dDialog;
}

void Scenery3dMgr::enableScenery3d(bool enable)
{
    flagEnabled=enable;

}


double Scenery3dMgr::getCallOrder(StelModuleActionName actionName) const
{
    if (actionName == StelModule::ActionDraw)
        return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName) + 5; // between Landscape and compass marks!
    if (actionName == StelModule::ActionUpdate)
        return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName) + 10;
    if (actionName == StelModule::ActionHandleKeys)
        //return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName) + 100;
        // GZ: low number means high precedence!
        return 3;
    return 0;
}

void Scenery3dMgr::handleKeys(QKeyEvent* e)
{
    scenery3d->handleKeys(e);
    if (!e->isAccepted()) {
        // handle keys
    }
}

void Scenery3dMgr::update(double deltaTime)
{
    scenery3d->update(deltaTime);
}

void Scenery3dMgr::draw(StelCore* core)
{
    scenery3d->draw(core, useCubeMap);
}

void Scenery3dMgr::init()
{
    qDebug() << "Scenery3d plugin - press KGA button to toggle 3D scenery";

    QSettings* conf = StelApp::getInstance().getSettings();
    Q_ASSERT(conf);
    if (conf->contains("Scenery3d/cubemapSize"))
        cubemapSize=conf->value("Scenery3d/cubemapSize").toInt();
    else cubemapSize=1024;

    // create action for enable/disable & hook up signals
    StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
    Q_ASSERT(gui);

    qDebug() << "call gui->addGuiActions()";
    gui->addGuiActions("actionShow_Scenery3d_window",
                       N_("Scenery3d configuration window"),
                       NULL, // no hotkey
                       NULL, // no help
                       true); // checkable; autorepeat=false.
    // GZ: DOES NOT WORK YET...
    //gui->getGuiActions("actionShow_Scenery3d_window")->setChecked(flagEnabled);

    connect(gui->getGuiActions("actionShow_Scenery3d_window"), SIGNAL(toggled(bool)), scenery3dDialog, SLOT(setVisible(bool)));
    connect(scenery3dDialog, SIGNAL(visibleChanged(bool)), gui->getGuiActions("actionShow_Scenery3d_window"), SLOT(setChecked(bool)));

    /*
    // GZ: Add a toolbar button (copy/paste widely from AngleMeasure) -- DOES NOT WORK YET
    try
    {
        qDebug() << "trying button\n"; // HERE IS THE PROBLEM

        toolbarButton = new StelButton(NULL, QPixmap(":/Scenery3d/bt_scenery3d_on.png"),
                                      QPixmap(":/Scenery3d/bt_scenery3d_off.png"),
                                      QPixmap(":/graphicGui/glow32x32.png"),
                                      gui->getGuiActions("actionShow_Scenery3d_window"));

       qDebug() << "call getButtonBar etc.\n";

       gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
    }
    catch (std::runtime_error& e)
    {
        qDebug() << "catch exc.\n";
            qWarning() << "WARNING: unable create toolbar button for Scenery3d plugin: " << e.what();
    }

    qDebug() << "past exception.\n";
    */

    scenery3d = new Scenery3d(cubemapSize);
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
    lmgr->setCurrentLandscapeName(newScenery3d->getLandscapeName());

    if (scenery3d)
    {
        delete scenery3d;
        scenery3d = NULL;
    }

    newScenery3d->loadModel();

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
    Scenery3d* newScenery3d = new Scenery3d(cubemapSize);
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
    qDebug() << "Scenery3dMgr::getNameToDirMap()";
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

/////////////////////////////////////////////////////////////////////
StelModule* Scenery3dStelPluginInterface::getStelModule() const
{
	return new Scenery3dMgr();
}

StelPluginInfo Scenery3dStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	//Q_INIT_RESOURCE(Scenery3dMgr);
	
	StelPluginInfo info;
	info.id = "Scenery3dMgr";
	info.displayedName = "Scenery3d";
	info.authors = "Simon Parzer, Peter Neubauer";
	info.contact = "/dev/null";
	info.description = "Test scene renderer.";
	return info;
}

Q_EXPORT_PLUGIN2(Scenery3dMgr, Scenery3dStelPluginInterface)
/////////////////////////////////////////////////////////////////////

