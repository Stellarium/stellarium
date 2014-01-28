/*
 * Copyright (C) 2013 Felix Zeltner
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


#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "Planes.hpp"
#include "FlightPainter.hpp"

#include <QDebug>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMetaType>


StelTextureSP Planes::planeTexture;



Planes::Planes()
{
    qRegisterMetaType<DBCredentials>();
    qRegisterMetaType<QList<ADSBFrame> >();
    qRegisterMetaType<QList<FlightID> >();
    displayFader = true;
    setObjectName("Planes");
    settingsDialog = new PlanesDialog();
    connectOnStartup = false;
}

Planes::~Planes()
{
}

void Planes::init()
{
    qDebug() << "Planes init()";
    loadSettings();
    planeTexture = StelApp::getInstance().getTextureManager().createTexture(":/planes/plane.png");

    //FlightPainter::initShaders();

    GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

    connect(StelApp::getInstance().getCore(), SIGNAL(locationChanged(StelLocation)), this, SLOT(updateLocation(StelLocation)));

    QString planesGroup = N_("Planes");
    addAction("action_enable_planes", planesGroup, N_("Show Planes"), "enabled", "Shift+P");
    //addAction("action_show_paths", planesGroup, N_("Show Plane Paths"), "pathsVisible");
    //addAction("action_load_planes", planesGroup, N_("Load planes"), "loadFile()");
    addAction("action_show_planes_settings", planesGroup, N_("Show Planes Settings"), settingsDialog, "visible", "Ctrl+P");

    onPix = new QPixmap(":/planes/planes_on.png");
    offPix = new QPixmap(":/planes/planes_off.png");
    glowPix = new QPixmap(":/graphicGui/glow32x32.png");
    onSettingsPix = new QPixmap(":/planes/planes_settings.png");
    offSettingsPix = new QPixmap(":/planes/planes_settings_off.png");
    enableButton = new StelButton(NULL, *onPix, *offPix, *glowPix, "action_enable_planes");
    settingsButton = new StelButton(NULL, *onSettingsPix, *offSettingsPix, *glowPix, "action_show_planes_settings");
    StelGui *gui = dynamic_cast<StelGui *>(StelApp::getInstance().getGui());
    gui->getButtonBar()->addButton(enableButton, "065-pluginsGroup");
    gui->getButtonBar()->addButton(settingsButton, "065-pluginsGroup");

    // Settings
    // Appearance
    connect(settingsDialog, SIGNAL(changePathColourMode(Flight::PathColour)), &flightMgr, SLOT(setPathColourMode(Flight::PathColour)));
    connect(settingsDialog, SIGNAL(changePathDrawModw(Flight::PathDrawMode)), &flightMgr, SLOT(setPathDrawMode(Flight::PathDrawMode)));
    connect(settingsDialog, SIGNAL(showLabels(bool)), &flightMgr, SLOT(setLabelsVisible(bool)));
    connect(settingsDialog, SIGNAL(useInterp(bool)), &flightMgr, SLOT(setInterpEnabled(bool)));

    connect(settingsDialog, SIGNAL(fileSelected(QString)), this, SLOT(openBSRecording(QString)));

    connect(settingsDialog, SIGNAL(credentialsChanged(DBCredentials)), this, SLOT(setDBCreds(DBCredentials)));
    connect(settingsDialog, SIGNAL(bsHostChanged(QString)), this, SLOT(setBSHost(QString)));
    connect(settingsDialog, SIGNAL(bsPortChanged(quint16)), this, SLOT(setBSPort(quint16)));
    connect(settingsDialog, SIGNAL(bsUseDBChanged(bool)), &bsDataSource, SLOT(setDumpOldFlights(bool)));
    connect(settingsDialog, SIGNAL(useDB(bool)), &bsDataSource, SLOT(setDatabaseEnabled(bool)));
    connect(settingsDialog, SIGNAL(useBS(bool)), &bsDataSource, SLOT(setSocketEnabled(bool)));

    connect(settingsDialog, SIGNAL(connectDB()), this, SLOT(connectDBBS()));
    connect(settingsDialog, SIGNAL(disconnectDB()), &bsDataSource, SLOT(disconnectDatabase()));
    connect(settingsDialog, SIGNAL(connectBS()), this, SLOT(connectDBBS()));
    connect(settingsDialog, SIGNAL(disconnectBS()), &bsDataSource, SLOT(disconnectSocket()));
    connect(&bsDataSource, SIGNAL(dbStatus(QString)), settingsDialog, SLOT(setDBStatus(QString)));
    connect(&bsDataSource, SIGNAL(bsStatus(QString)), settingsDialog, SLOT(setBSStatus(QString)));

    connect(settingsDialog, SIGNAL(connectOnStartup(bool)), this, SLOT(setConnectOnStartup(bool)));

    if (connectOnStartup) {
        connectDBBS();
    }
}

void Planes::deinit()
{
    qDebug() << "Planes deinit()";
    saveSettings();
    planeTexture.clear();
    bsDataSource.deinit();
    bsRecordingDataSource.deinit();
    //FlightPainter::deinitShaders();
}

void Planes::update(double deltaTime)
{
    if (!displayFader && displayFader.getInterstate() <= 0.) {
        return;
    }

    // Faders
    displayFader.update((int)(deltaTime * 1000));

    flightMgr.update(deltaTime);
}

void Planes::draw(StelCore *core)
{
    if (!displayFader && displayFader.getInterstate() <= 0.) {
        return;
    }

    flightMgr.setBrightness(displayFader.getInterstate());

    flightMgr.draw(core);
}

double Planes::getCallOrder(StelModule::StelModuleActionName actionName) const
{
    if (actionName == StelModule::ActionDraw) {
        return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName) + 1.;
    }
    return 0;
}

void Planes::handleKeys(QKeyEvent *event)
{
    event->ignore();
}

void Planes::handleMouseClicks(QMouseEvent *event)
{
    event->ignore();
}

bool Planes::handleMouseMoves(int x, int y, Qt::MouseButtons b)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(b);
    return false;
}

bool Planes::configureGui(bool show)
{
    if (show) {
        settingsDialog->setVisible(true);
    }
    return true;
}

void Planes::loadSettings()
{
    QSettings *conf = StelApp::getInstance().getSettings();
    conf->beginGroup("Planes");

    bsDataSource.setDatabaseEnabled(conf->value("use_db", false).toBool());
    bsDataSource.setSocketEnabled(conf->value("use_bs", false).toBool());

    dbc.type = conf->value("db_type", "QMYSQL").toString();
    dbc.host = conf->value("db_host", "").toString();
    dbc.port = conf->value("db_port", 3306).toInt();
    dbc.user = conf->value("db_user", "").toString();
    dbc.pass = conf->value("db_pass", "").toString();
    dbc.name = conf->value("db_name", "").toString();

    flightMgr.setInterpEnabled(conf->value("use_interp", true).toBool());
    flightMgr.setLabelsVisible(conf->value("show_labels", true).toBool());
    flightMgr.setPathDrawMode(static_cast<Flight::PathDrawMode>(conf->value("show_trails", 1).toUInt()));
    flightMgr.setPathColourMode(static_cast<Flight::PathColour>(conf->value("trail_col", 0).toUInt()));

    Flight::setMaxVertRate(conf->value("max_vert_rate", 50).toDouble());
    Flight::setMinVertRate(conf->value("min_vert_rate", -50).toDouble());
    Flight::setMaxHeight(conf->value("max_height", 20000).toDouble());
    Flight::setMinHeight(conf->value("min_height", 0).toDouble());
    Flight::setMaxVelocity(conf->value("max_velocity", 500).toDouble());
    Flight::setMinVelocity(conf->value("min_velocity", 0).toDouble());

    bsHost = conf->value("bs_host", "localhost").toString();
    bsPort = conf->value("bs_port", 30003).toUInt();
    bsDataSource.setDumpOldFlights(conf->value("bs_use_db", false).toBool());

    connectOnStartup = conf->value("connect_on_startup", false).toBool();

    conf->endGroup();
}

void Planes::saveSettings()
{
    QSettings *conf = StelApp::getInstance().getSettings();
    conf->beginGroup("Planes");

    conf->setValue("use_db", bsDataSource.isDatabaseEnabled());
    conf->setValue("use_bs", bsDataSource.isSocketEnabled());

    conf->setValue("db_type", dbc.type);
    conf->setValue("db_host", dbc.host);
    conf->setValue("db_port", dbc.port);
    conf->setValue("db_user", dbc.user);
    conf->setValue("db_pass", dbc.pass);
    conf->setValue("db_name", dbc.name);

    conf->setValue("use_interp", flightMgr.isInterpEnabled());
    conf->setValue("show_labels", flightMgr.isLabelsVisible());
    conf->setValue("show_trails", static_cast<uint>(flightMgr.getPathDrawMode()));
    conf->setValue("trail_col", static_cast<uint>(flightMgr.getPathColourMode()));

    conf->setValue("max_vert_rate", Flight::getMaxVertRate());
    conf->setValue("min_vert_rate", Flight::getMinVertRate());
    conf->setValue("max_height", Flight::getMaxHeight());
    conf->setValue("min_height", Flight::getMinHeight());
    conf->setValue("max_velocity", Flight::getMaxVelocity());
    conf->setValue("min_velocity", Flight::getMinVelocity());

    conf->setValue("bs_host", bsHost);
    conf->setValue("bs_port", bsPort);
    conf->setValue("bs_use_db", bsDataSource.isDumpOldFlightsEnabled());

    conf->setValue("connect_on_startup", connectOnStartup);

    conf->endGroup();
}

void Planes::enablePlanes(bool b)
{
    if (displayFader != b) {
        displayFader = b;
    }
    qDebug() << "Planes enabled " << b;
}

void Planes::setDBCreds(DBCredentials creds)
{
    dbc = creds;
}

void Planes::openBSRecording(QString file)
{
    flightMgr.setDataSource(&bsRecordingDataSource);
    bsRecordingDataSource.loadFile(file);
}

void Planes::connectDBBS()
{
    flightMgr.setDataSource(&bsDataSource);
    bsDataSource.connectClicked(bsHost, bsPort, dbc);
}

bool Planes::isConnectOnStartupEnabled() const
{
    return connectOnStartup;
}

void Planes::setConnectOnStartup(bool value)
{
    connectOnStartup = value;
}


StelModule *PlanesStelPluginInterface::getStelModule() const
{
    qDebug() << "getStelModule() Planes";
    return new Planes();
}

StelPluginInfo PlanesStelPluginInterface::getPluginInfo() const
{
    qDebug() << "getPluginInfo() Planes";
    // Allow to load the resources when used as a static plugin
    Q_INIT_RESOURCE(Planes);

    StelPluginInfo info;
    info.id = "Planes";
    info.displayedName = N_("Planes");
    info.authors = "Felix Zeltner";
    info.contact = "";
    info.description = N_("Display ADS-B plane tracking data");
    return info;
}
