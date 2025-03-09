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



Planes::Planes(): labelsVisible(true), displayFader(true), connectOnStartup(false)
{
	setObjectName("Planes");
	// Register metatypes so they can be passed on signals/slots across threads
	qRegisterMetaType<DBCredentials>();
	qRegisterMetaType<QList<ADSBFrame> >();
	qRegisterMetaType<QList<FlightID> >();
	settingsDialog = new PlanesDialog();
}

Planes::~Planes()
{
}

void Planes::init()
{
	//qDebug() << "Planes init()";
	loadSettings();
	planeTexture = StelApp::getInstance().getTextureManager().createTexture(":/planes/plane.png");

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	connect(StelApp::getInstance().getCore(), SIGNAL(locationChanged(StelLocation)), this, SLOT(updateLocation(StelLocation)));

	QString planesGroup = N_("Planes");
	addAction("actionShow_Planes",          planesGroup, N_("Show Planes"), "enabled", "Shift+P");
	addAction("actionShow_Planes_Settings", planesGroup, N_("Show Planes Settings"), settingsDialog, "visible", "Ctrl+P");

	// Add a toolbar button
	try
	{
		StelGui *gui = dynamic_cast<StelGui *>(StelApp::getInstance().getGui());
		if (gui)
		{
			toolbarButton = new StelButton(nullptr,
						      QPixmap(":/planes/planes_on_160.png"),
						      QPixmap(":/planes/planes_off_160.png"),
						      QPixmap(":/graphicGui/miscGlow32x32.png"),
						      "actionShow_Planes",
						      false,
						      "actionShow_Planes_Settings");
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to create toolbar button for Planes plugin: " << e.what();
	}


	// Settings
	// Appearance
	connect(settingsDialog, SIGNAL(pathColorModeChanged(Flight::PathColorMode)), this, SLOT(setPathColorMode(Flight::PathColorMode)));
	connect(settingsDialog, SIGNAL(pathDrawModeChanged(Flight::PathDrawMode)), this, SLOT(setPathDrawMode(Flight::PathDrawMode)));

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

	if (connectOnStartup)
	{
		connectDBBS();
	}
}

void Planes::deinit()
{
	//qDebug() << "Planes deinit()";
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
	conf->beginGroup("Planes");

	bsDataSource.setDatabaseEnabled(conf->value("use_db", false).toBool());
	bsDataSource.setSocketEnabled(conf->value("use_bs", false).toBool());
	bsDataSource.setReconnectOnConnectionLoss(conf->value("attempt_reconnects", false).toBool());

	dbc.type = conf->value("db_type", "QMYSQL").toString();
	dbc.host = conf->value("db_host", QString()).toString();
	dbc.port = conf->value("db_port", 3306).toInt();
	dbc.user = conf->value("db_user", QString()).toString();
	dbc.pass = conf->value("db_pass", QString()).toString();
	dbc.name = conf->value("db_name", QString()).toString();

	setFlagUseInterpolation(conf->value("use_interp", true).toBool());
	setFlagShowLabels(conf->value("show_labels", true).toBool());
	setPathDrawMode(static_cast<Flight::PathDrawMode>(conf->value("show_trails", 1).toInt()));
	setPathColorMode(static_cast<Flight::PathColorMode>(conf->value("trail_col", 0).toInt()));

	setMaxVertRate(conf->value("max_vert_rate", 50).toDouble());
	setMinVertRate(conf->value("min_vert_rate", -50).toDouble());
	setMaxHeight(conf->value("max_height", 20000).toDouble());
	setMinHeight(conf->value("min_height", 0).toDouble());
	setMaxVelocity(conf->value("max_velocity", 500).toDouble());
	setMinVelocity(conf->value("min_velocity", 0).toDouble());

	bsHost = conf->value("bs_host", "localhost").toString();
	bsPort = conf->value("bs_port", 30003).toUInt();
	bsDataSource.setDumpOldFlights(conf->value("bs_use_db", false).toBool());

	setConnectOnStartup(conf->value("connect_on_startup", false).toBool());

	setFlightInfoColor(Vec3f(conf->value("planes_color", "0.,1.,0.").toString()));

	conf->endGroup();
}

void Planes::saveSettings()
{
	QSettings *conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Planes");

	conf->setValue("use_db", bsDataSource.isDatabaseEnabled());
	conf->setValue("use_bs", bsDataSource.isSocketEnabled());
	conf->setValue("attempt_reconnects", isReconnectOnConnectionLossEnabled());

	conf->setValue("db_type", dbc.type);
	conf->setValue("db_host", dbc.host);
	conf->setValue("db_port", dbc.port);
	conf->setValue("db_user", dbc.user);
	conf->setValue("db_pass", dbc.pass);
	conf->setValue("db_name", dbc.name);

	conf->setValue("use_interp", getFlagUseInterpolation());
	conf->setValue("show_labels", getFlagShowLabels());
	conf->setValue("show_trails", static_cast<uint>(getPathDrawMode()));
	conf->setValue("trail_col", static_cast<uint>(getPathColorMode()));

	conf->setValue("max_vert_rate", getMaxVertRate());
	conf->setValue("min_vert_rate", getMinVertRate());
	conf->setValue("max_height", getMaxHeight());
	conf->setValue("min_height", getMinHeight());
	conf->setValue("max_velocity", getMaxVelocity());
	conf->setValue("min_velocity", getMinVelocity());

	conf->setValue("bs_host", bsHost);
	conf->setValue("bs_port", bsPort);
	conf->setValue("bs_use_db", bsDataSource.isDumpOldFlightsEnabled());

	conf->setValue("connect_on_startup", connectOnStartup);

	conf->setValue("planes_color", getFlightInfoColor().toStr());

	conf->endGroup();
}

void Planes::enablePlanes(bool b)
{
	if (displayFader != b)
	{
		displayFader = b;
	}
	//qDebug() << "Planes enabled " << b;
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

bool Planes::isConnectOnStartup() const
{
	return connectOnStartup;
}

void Planes::setConnectOnStartup(bool value)
{
	connectOnStartup = value;
	emit connectOnStartupChanged(value);
}

double Planes::getMaxVertRate() const
{
	return Flight::maxVertRate;
}
void Planes::setMaxVertRate(double value)
{
	Flight::maxVertRate = value;
	emit maxVertRateChanged(value);
}

double Planes::getMinVertRate() const
{
	return Flight::minVertRate;
}
void Planes::setMinVertRate(double value)
{
	Flight::minVertRate = value;
	emit minVertRateChanged(value);
}

double Planes::getMaxVelocity() const
{
	return Flight::maxVelocity;
}
void Planes::setMaxVelocity(double value)
{
	Flight::maxVelocity = value;
	Flight::velRange = Flight::maxVelocity - Flight::minVelocity;
	emit maxVelocityChanged(value);
}

double Planes::getMinVelocity() const
{
	return Flight::minVelocity;
}
void Planes::setMinVelocity(double value)
{
	Flight::minVelocity = value;
	Flight::velRange = Flight::maxVelocity - Flight::minVelocity;
	emit minVelocityChanged(value);
}

double Planes::getMaxHeight() const
{
	return Flight::maxHeight;
}
void Planes::setMaxHeight(double value)
{
	Flight::maxHeight = value;
	Flight::heightRange = Flight::maxHeight - Flight::minHeight;
	emit maxHeightChanged(value);
}

double Planes::getMinHeight() const
{
	return Flight::minHeight;
}
void Planes::setMinHeight(double value)
{
	Flight::minHeight = value;
	Flight::heightRange = Flight::maxHeight - Flight::minHeight;
	emit minHeightChanged(value);
}

Vec3f Planes::getFlightInfoColor() const
{
	return Flight::infoColor;
}
void Planes::setFlightInfoColor(const Vec3f &col)
{
	Flight::infoColor = col;
	emit flightInfoColorChanged(col);
}

void Planes::setFlagUseInterpolation(bool interp)
{
	ADSBData::useInterp = interp;
	emit useInterpolationChanged(interp);
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
