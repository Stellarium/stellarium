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

#include <QDebug>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMetaType>


StelTextureSP Planes::planeTexture;



Planes::Planes()
{
	qDebug() << "Planes: CTOR";
	// Register metatypes so they can be passed on signals/slots across threads
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

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	connect(StelApp::getInstance().getCore(), SIGNAL(locationChanged(StelLocation)), this, SLOT(updateLocation(StelLocation)));

	QString planesGroup = N_("Planes");
	addAction("actionShow_planes",          planesGroup, N_("Show Planes"), "enabled", "Shift+P");
	addAction("actionShow_planes_settings", planesGroup, N_("Show Planes Settings"), settingsDialog, "visible", "Ctrl+P");

	// Add a toolbar button
	try
	{
		StelGui *gui = dynamic_cast<StelGui *>(StelApp::getInstance().getGui());
		if (gui!=Q_NULLPTR)
		{
			//onPix = new QPixmap(QStringLiteral(":/planes/planes_on.png"));
			//offPix = new QPixmap(QStringLiteral(":/planes/planes_off.png"));
			//glowPix = new QPixmap(QStringLiteral(":/graphicGui/miscGlow32x32.png"));
			//onSettingsPix = new QPixmap(QStringLiteral(":/planes/planes_settings.png"));
			//offSettingsPix = new QPixmap(QStringLiteral(":/planes/planes_settings_off.png"));
			toolbarButton = new StelButton(NULL,
						      QPixmap(QStringLiteral(":/planes/planes_on.png")),
						      QPixmap(QStringLiteral(":/planes/planes_off.png")),
						      QPixmap(QStringLiteral(":/graphicGui/miscGlow32x32.png")),
						      "actionShow_planes",
						      false,
						      "actionShow_planes_settings");

			//enableButton = new StelButton(NULL, *onPix, *offPix, *glowPix, "actionShow_planes", false, "actionShow_planes_settings");
			//settingsButton = new StelButton(NULL, *onSettingsPix, *offSettingsPix, *glowPix, "action_show_planes_settings");
			gui->getButtonBar()->addButton(toolbarButton, QStringLiteral("065-pluginsGroup"));
			//gui->getButtonBar()->addButton(settingsButton, QStringLiteral("065-pluginsGroup"));
		}

	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to create toolbar button for Planes plugin: " << e.what();
	}


	// Settings
	// Appearance
	connect(settingsDialog, SIGNAL(pathColourModeChanged(Flight::PathColour)), &flightMgr, SLOT(setPathColourMode(Flight::PathColour)));
	connect(settingsDialog, SIGNAL(pathDrawModeChanged(Flight::PathDrawMode)), &flightMgr, SLOT(setPathDrawMode(Flight::PathDrawMode)));
	connect(settingsDialog, SIGNAL(showLabelsChanged(bool)), &flightMgr, SLOT(setLabelsVisible(bool)));
	connect(settingsDialog, SIGNAL(useInterpChanged(bool)), &flightMgr, SLOT(setInterpEnabled(bool)));

	connect(settingsDialog, SIGNAL(fileSelected(QString)), this, SLOT(openBSRecording(QString)));

	connect(settingsDialog, SIGNAL(credentialsChanged(DBCredentials)), this, SLOT(setDBCreds(DBCredentials)));
	connect(settingsDialog, SIGNAL(bsHostChanged(QString)), this, SLOT(setBSHost(QString)));
	connect(settingsDialog, SIGNAL(bsPortChanged(quint16)), this, SLOT(setBSPort(quint16)));
	connect(settingsDialog, SIGNAL(bsUseDBChanged(bool)), &bsDataSource, SLOT(setDumpOldFlights(bool)));
	connect(settingsDialog, SIGNAL(dbUsedSet(bool)), &bsDataSource, SLOT(setDatabaseEnabled(bool)));
	connect(settingsDialog, SIGNAL(bsUsedSet(bool)), &bsDataSource, SLOT(setSocketEnabled(bool)));

	connect(settingsDialog, SIGNAL(connectDBRequested()), this, SLOT(connectDBBS()));
	connect(settingsDialog, SIGNAL(disconnectDBRequested()), &bsDataSource, SLOT(disconnectDatabase()));
	connect(settingsDialog, SIGNAL(connectBSRequested()), this, SLOT(connectDBBS()));
	connect(settingsDialog, SIGNAL(disconnectBSRequested()), &bsDataSource, SLOT(disconnectSocket()));
	connect(settingsDialog, SIGNAL(reconnectOnConnectionLossChanged(bool)), &bsDataSource, SLOT(setReconnectOnConnectionLoss(bool)));
	connect(&bsDataSource, SIGNAL(dbStatusChanged(QString)), settingsDialog, SLOT(setDBStatus(QString)));
	connect(&bsDataSource, SIGNAL(bsStatusChanged(QString)), settingsDialog, SLOT(setBSStatus(QString)));

	connect(settingsDialog, SIGNAL(connectOnStartupChanged(bool)), this, SLOT(setConnectOnStartup(bool)));

	if (connectOnStartup)
	{
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
}

void Planes::update(double deltaTime)
{
	if (!displayFader && displayFader.getInterstate() <= 0.)
	{
		return;
	}

	// Faders
	displayFader.update((int)(deltaTime * 1000));

	flightMgr.update(deltaTime);
}

void Planes::draw(StelCore *core)
{
	if (!displayFader && displayFader.getInterstate() <= 0.)
	{
		return;
	}

	flightMgr.setBrightness(displayFader.getInterstate());

	flightMgr.draw(core);
}

double Planes::getCallOrder(StelModule::StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
	{
		return StelApp::getInstance().getModuleMgr().getModule(QStringLiteral("SolarSystem"))->getCallOrder(actionName) + 1.;
	}
	return 0;
}

void Planes::handleKeys(QKeyEvent *event)
{
	event->ignore();
}

//void Planes::handleMouseClicks(QMouseEvent *event)
//{
//	event->ignore();
//}
//
//bool Planes::handleMouseMoves(int x, int y, Qt::MouseButtons b)
//{
//	Q_UNUSED(x);
//	Q_UNUSED(y);
//	Q_UNUSED(b);
//	return false;
//}

bool Planes::configureGui(bool show)
{
	if (show)
	{
		settingsDialog->setVisible(true);
	}
	return true;
}

void Planes::loadSettings()
{
	QSettings *conf = StelApp::getInstance().getSettings();
	conf->beginGroup(QStringLiteral("Planes"));

	bsDataSource.setDatabaseEnabled(conf->value(QStringLiteral("use_db"), false).toBool());
	bsDataSource.setSocketEnabled(conf->value(QStringLiteral("use_bs"), false).toBool());
	bsDataSource.setReconnectOnConnectionLoss(conf->value(QStringLiteral("attempt_reconnects"), false).toBool());

	dbc.type = conf->value(QStringLiteral("db_type"), QStringLiteral("QMYSQL")).toString();
	dbc.host = conf->value(QStringLiteral("db_host"), QString()).toString();
	dbc.port = conf->value(QStringLiteral("db_port"), 3306).toInt();
	dbc.user = conf->value(QStringLiteral("db_user"), QString()).toString();
	dbc.pass = conf->value(QStringLiteral("db_pass"), QString()).toString();
	dbc.name = conf->value(QStringLiteral("db_name"), QString()).toString();

	flightMgr.setInterpEnabled(conf->value(QStringLiteral("use_interp"), true).toBool());
	flightMgr.setLabelsVisible(conf->value(QStringLiteral("show_labels"), true).toBool());
	flightMgr.setPathDrawMode(static_cast<Flight::PathDrawMode>(conf->value(QStringLiteral("show_trails"), 1).toUInt()));
	flightMgr.setPathColourMode(static_cast<Flight::PathColour>(conf->value(QStringLiteral("trail_col"), 0).toUInt()));

	Flight::setMaxVertRate(conf->value(QStringLiteral("max_vert_rate"), 50).toDouble());
	Flight::setMinVertRate(conf->value(QStringLiteral("min_vert_rate"), -50).toDouble());
	Flight::setMaxHeight(conf->value(QStringLiteral("max_height"), 20000).toDouble());
	Flight::setMinHeight(conf->value(QStringLiteral("min_height"), 0).toDouble());
	Flight::setMaxVelocity(conf->value(QStringLiteral("max_velocity"), 500).toDouble());
	Flight::setMinVelocity(conf->value(QStringLiteral("min_velocity"), 0).toDouble());

	bsHost = conf->value(QStringLiteral("bs_host"), QStringLiteral("localhost")).toString();
	bsPort = conf->value(QStringLiteral("bs_port"), 30003).toUInt();
	bsDataSource.setDumpOldFlights(conf->value(QStringLiteral("bs_use_db"), false).toBool());

	connectOnStartup = conf->value(QStringLiteral("connect_on_startup"), false).toBool();

	Flight::setFlightInfoColour(Vec3f(conf->value(QStringLiteral("planes_colour"), QStringLiteral("0.,1.,0.")).toString()));

	conf->endGroup();
}

void Planes::saveSettings()
{
	QSettings *conf = StelApp::getInstance().getSettings();
	conf->beginGroup(QStringLiteral("Planes"));

	conf->setValue(QStringLiteral("use_db"), bsDataSource.isDatabaseEnabled());
	conf->setValue(QStringLiteral("use_bs"), bsDataSource.isSocketEnabled());
	conf->setValue(QStringLiteral("attempt_reconnects"), isReconnectOnConnectionLossEnabled());

	conf->setValue(QStringLiteral("db_type"), dbc.type);
	conf->setValue(QStringLiteral("db_host"), dbc.host);
	conf->setValue(QStringLiteral("db_port"), dbc.port);
	conf->setValue(QStringLiteral("db_user"), dbc.user);
	conf->setValue(QStringLiteral("db_pass"), dbc.pass);
	conf->setValue(QStringLiteral("db_name"), dbc.name);

	conf->setValue(QStringLiteral("use_interp"), flightMgr.isInterpEnabled());
	conf->setValue(QStringLiteral("show_labels"), flightMgr.isLabelsVisible());
	conf->setValue(QStringLiteral("show_trails"), static_cast<uint>(flightMgr.getPathDrawMode()));
	conf->setValue(QStringLiteral("trail_col"), static_cast<uint>(flightMgr.getPathColourMode()));

	conf->setValue(QStringLiteral("max_vert_rate"), Flight::getMaxVertRate());
	conf->setValue(QStringLiteral("min_vert_rate"), Flight::getMinVertRate());
	conf->setValue(QStringLiteral("max_height"), Flight::getMaxHeight());
	conf->setValue(QStringLiteral("min_height"), Flight::getMinHeight());
	conf->setValue(QStringLiteral("max_velocity"), Flight::getMaxVelocity());
	conf->setValue(QStringLiteral("min_velocity"), Flight::getMinVelocity());

	conf->setValue(QStringLiteral("bs_host"), bsHost);
	conf->setValue(QStringLiteral("bs_port"), bsPort);
	conf->setValue(QStringLiteral("bs_use_db"), bsDataSource.isDumpOldFlightsEnabled());

	conf->setValue(QStringLiteral("connect_on_startup"), connectOnStartup);

	conf->setValue(QStringLiteral("planes_colour"), QString(QStringLiteral("%1,%2,%3")).arg(Flight::getFlightInfoColour()[0])
			.arg(Flight::getFlightInfoColour()[1]).arg(Flight::getFlightInfoColour()[2]));

	conf->endGroup();
}

void Planes::enablePlanes(bool b)
{
	if (displayFader != b)
	{
		displayFader = b;
	}
	qDebug() << "Planes enabled " << b;
	emit enabledChanged(b);
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
	bsDataSource.connectDBBS(bsHost, bsPort, dbc);
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
	return new Planes();
}

StelPluginInfo PlanesStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(Planes);

	StelPluginInfo info;
	info.id = QStringLiteral("Planes");
	info.displayedName = N_("Planes");
	info.authors = N_("Felix Zeltner");
	info.contact = QString();
	info.description = N_("Display flight trajectories from ADS-B tracking data.");
	info.version = PLANES_PLUGIN_VERSION;
	info.license = PLANES_PLUGIN_LICENSE;
	return info;
}
