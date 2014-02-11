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

#ifndef PLANES_HPP_
#define PLANES_HPP_

#include "StelObjectModule.hpp"
#include "FlightMgr.hpp"
#include "StelGui.hpp"
#include "BSRecordingDataSource.hpp"
#include "BSDataSource.hpp"
#include "PlanesDialog.hpp"
#include "DBCredentials.hpp"


Q_DECLARE_METATYPE(DBCredentials)


//! @class Planes
//! This class is the entry point for the plugin.
//! It handles the integration with Stellarium.
class Planes : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled READ isEnabled WRITE enablePlanes)
	friend class FlightMgr;
public:
	Planes();
	virtual ~Planes();

	///////////////////////////////////////////////////////////////////////////
	//!@{
	//! Methods defined in the StelModule class
	void init();
	void deinit();
	void update(double deltaTime);
	void draw(StelCore* core);
	double getCallOrder(StelModuleActionName actionName) const;
	void handleKeys(class QKeyEvent* event);
	void handleMouseClicks(class QMouseEvent* event);
	bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	bool configureGui(bool show = true);
	//!@}

	////////////////////////////////////////////////////////////////////////////
	//!@{
	//! Methods defined in StelObjectModule
	//! Forward requests to FlightMgr
	QList<StelObjectP> searchAround(const Vec3d &v, double limitFov, const StelCore *core) const
	{
		return flightMgr.searchAround(v, limitFov , core);
	}

	StelObjectP searchByNameI18n(const QString &nameI18n) const
	{
		return flightMgr.searchByNameI18n(nameI18n);
	}

	StelObjectP searchByName(const QString &name) const
	{
		return flightMgr.searchByName(name);
	}

	QStringList listMatchingObjectsI18n(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const
	{
		return flightMgr.listMatchingObjectsI18n(objPrefix, maxNbItem, useStartOfWords);
	}

	QStringList listMatchingObjects(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const
	{
		return flightMgr.listMatchingObjects(objPrefix, maxNbItem, useStartOfWords);
	}

	QStringList listAllObjects(bool inEnglish) const
	{
		return flightMgr.listAllObjects(inEnglish);
	}
	//!@}

	//! Return the name of this StelObject.
	//! Name is the class name.
	QString getName() const
	{
		return QStringLiteral("Planes");
	}

	//! Is the plugin enabled?
	bool isEnabled() const
	{
		return displayFader;
	}

	//! Load settings from the stellarium config file
	void loadSettings();

	//! Save the settings to the stellarium config file
	void saveSettings();

	//! Is the database source enabled?
	bool isUsingDB() const
	{
		return bsDataSource.isDatabaseEnabled();
	}

	//! Is the BaseStation port source enabled?
	bool isUsingBS() const
	{
		return bsDataSource.isSocketEnabled();
	}

	//! Is saving of Flight objects received from the data port enabled?
	bool isDumpingOldFlights() const
	{
		return bsDataSource.isDumpOldFlightsEnabled();
	}

	//! Get the credentials to connect to the database
	DBCredentials getDBCreds() const
	{
		return dbc;
	}

	//! Get the FlightMgr object
	const FlightMgr *getFlightMgr() const
	{
		return &flightMgr;
	}

	//! Get the hostname for the BaseStation data port
	QString getBSHost() const
	{
		return bsHost;
	}

	//! Get the port for the BaseStation data port
	quint16 getBSPort() const
	{
		return bsPort;
	}

	//! Should we autoconnect on startup?
	bool isConnectOnStartupEnabled() const;

	//! Should the data port attempt to reconnect on connection loss?
	bool isReconnectOnConnectionLossEnabled() const
	{
		return bsDataSource.isReconnectOnConnectionLossEnabled();
	}

public slots:
	//! Turn this plugin on or off
	void enablePlanes(bool b);

	//! The user changed the observer position in stellarium
	void updateLocation(StelLocation loc)
	{
		Flight::updateObserverPos(loc);
	}

	//! User updated db credentials in the settings
	void setDBCreds(DBCredentials creds);

	//! User wants to open a BaseStation recording file
	//! @param file the filename and path to the file
	void openBSRecording(QString file);

	//! The user wants to connect to the database and/or the data port
	void connectDBBS();

	//! User changed the data port hostname
	void setBSHost(QString host)
	{
		bsHost = host;
	}

	//! User changed the data port port
	void setBSPort(quint16 port)
	{
		bsPort = port;
	}

	//! User changed connect on startup setting
	void setConnectOnStartup(bool value);

private:
	static StelTextureSP planeTexture; //!< the texture used for drawing the plane icons
	FlightMgr flightMgr; //!< The FlightMgr
	LinearFader displayFader; //!< Fader to fade in and out on enable/disable

	PlanesDialog *settingsDialog; //!< Configuration window

	BSRecordingDataSource bsRecordingDataSource; //!< data source for loading files
	BSDataSource bsDataSource; //!< data source for database and data port

	StelButton *settingsButton; //!< Button to show settings
	StelButton *enableButton; //!< Button to enable/disable plugin

	//!@{
	//! Icons for the buttons
	QPixmap *onPix;
	QPixmap *offPix;
	QPixmap *glowPix;
	QPixmap *onSettingsPix;
	QPixmap *offSettingsPix;
	//!@}

	DBCredentials dbc; //!< Database credentials
	QString bsHost; //!< data port hostname
	quint16 bsPort; //!< data port port
	bool connectOnStartup; //!< connect on startup setting
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! @class PlanesStelPluginInterface
//! This class is used by Qt to manage a plug-in interface
class PlanesStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "stellarium.StelGuiPluginInterface/1.0")
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*PLANES_HPP_*/

