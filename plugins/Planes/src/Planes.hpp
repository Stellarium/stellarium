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
	Q_PROPERTY(bool enabled          READ isEnabled               WRITE enablePlanes            NOTIFY enabledChanged)
	Q_PROPERTY(bool connectOnStartup READ isConnectOnStartup      WRITE setConnectOnStartup     NOTIFY connectOnStartupChanged)
	Q_PROPERTY(bool showLabels       READ getFlagShowLabels       WRITE setFlagShowLabels       NOTIFY showLabelsChanged)                // From FlightMgr
	Q_PROPERTY(bool useInterpolation READ getFlagUseInterpolation WRITE setFlagUseInterpolation NOTIFY useInterpolationChanged)          // From FlightMgr
	Q_PROPERTY(Flight::PathColorMode pathColorMode READ getPathColorMode WRITE setPathColorMode NOTIFY pathColorModeChanged)
	Q_PROPERTY(Flight::PathDrawMode pathDrawMode   READ getPathDrawMode   WRITE setPathDrawMode   NOTIFY pathDrawModeChanged)
	Q_PROPERTY(double maxVertRate READ getMaxVertRate WRITE setMaxVertRate NOTIFY maxVertRateChanged)
	Q_PROPERTY(double minVertRate READ getMinVertRate WRITE setMinVertRate NOTIFY minVertRateChanged)
	Q_PROPERTY(double maxVelocity READ getMaxVelocity WRITE setMaxVelocity NOTIFY maxVelocityChanged)
	Q_PROPERTY(double minVelocity READ getMinVelocity WRITE setMinVelocity NOTIFY minVelocityChanged)
	Q_PROPERTY(double maxHeight   READ getMaxHeight   WRITE setMaxHeight   NOTIFY maxHeightChanged)
	Q_PROPERTY(double minHeight   READ getMinHeight   WRITE setMinHeight   NOTIFY minHeightChanged)
	Q_PROPERTY(Vec3f infoColor   READ getFlightInfoColor  WRITE setFlightInfoColor  NOTIFY flightInfoColorChanged)

	friend class FlightMgr;
public:
	Planes();
	virtual ~Planes();

	///////////////////////////////////////////////////////////////////////////
	//!@{
	//! Methods defined in the StelModule class
	void init() override;
	void deinit() override;
	void update(double deltaTime) override;
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;
	void handleKeys(class QKeyEvent* event) override;
	//void handleMouseClicks(class QMouseEvent* event) override;
	//bool handleMouseMoves(int x, int y, Qt::MouseButtons b) override;
	bool configureGui(bool show = true) override;
	//!@}

	////////////////////////////////////////////////////////////////////////////
	//!@{
	//! Methods defined in StelObjectModule
	//! Forward requests to FlightMgr
	QList<StelObjectP> searchAround(const Vec3d &v, double limitFov, const StelCore *core) const override
	{
		return flightMgr.searchAround(v, limitFov , core);
	}

	StelObjectP searchByNameI18n(const QString &nameI18n) const override
	{
		return flightMgr.searchByNameI18n(nameI18n);
	}

	StelObjectP searchByName(const QString &name) const override
	{
		return flightMgr.searchByName(name);
	}

	//! Return the StelObject with the given ID if exists or the empty StelObject if not found
	//! @param name the english object name
	//! @todo for now this is equal to searchByName(). Maybe this is wrong.
	virtual StelObjectP searchByID(const QString& id) const override
	{
		return searchByName(id);
	}

	QStringList listMatchingObjectsI18n(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const
	{
		return flightMgr.listMatchingObjectsI18n(objPrefix, maxNbItem, useStartOfWords);
	}

	QStringList listMatchingObjects(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const override
	{
		return flightMgr.listMatchingObjects(objPrefix, maxNbItem, useStartOfWords);
	}

	QStringList listAllObjects(bool inEnglish) const override
	{
		return flightMgr.listAllObjects(inEnglish);
	}
	//!@}

	//! Return the name of this StelObject.
	//! Name is the class name.
	QString getName() const override
	{
		return QStringLiteral("Planes");
	}
	//! Returns the name that will be returned by StelObject::getType() for the objects this module manages
	virtual QString getStelObjectType() const override
	{
		return QStringLiteral("Flight");
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


	//! Should the data port attempt to reconnect on connection loss?
	bool isReconnectOnConnectionLossEnabled() const
	{
		return bsDataSource.isReconnectOnConnectionLossEnabled();
	}

signals:
	void enabledChanged(bool b);
	void connectOnStartupChanged(bool b);
	void showLabelsChanged(bool b);
	void useInterpolationChanged(bool b);
	void pathColorModeChanged(Flight::PathColorMode mode);
	void pathDrawModeChanged(Flight::PathDrawMode mode);
	void maxVertRateChanged(double d);
	void minVertRateChanged(double d);
	void maxVelocityChanged(double d);
	void minVelocityChanged(double d);
	void maxHeightChanged(double d);
	void minHeightChanged(double d);
	void flightInfoColorChanged(Vec3f c);


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

	// Property getters/setters. Some moved over from Flight class
	//!@{

	//! Should we autoconnect on startup?
	bool isConnectOnStartup() const;
	//! User changed connect on startup setting
	void setConnectOnStartup(bool value);

	//! Personalise path coloring
	double getMaxVertRate() const;
	void setMaxVertRate(double value);

	double getMinVertRate() const;
	void setMinVertRate(double value);

	double getMaxVelocity() const;
	void setMaxVelocity(double value);

	double getMinVelocity() const;
	void setMinVelocity(double value);

	double getMaxHeight() const;
	void setMaxHeight(double value);

	double getMinHeight() const;
	void setMinHeight(double value);

	//! The color used for drawing info text and icons
	Vec3f getFlightInfoColor() const;
	//! Set the color used for drawing info text and icons
	void setFlightInfoColor(const Vec3f &col);
	//!@}
	//! Check whether labels are drawn
	bool getFlagShowLabels() const
	{
		return labelsVisible;
	}
	//! Turn label drawing on or off
	void setFlagShowLabels(bool visible)
	{
		labelsVisible = visible;
		emit showLabelsChanged(visible);
	}

	//! Check whether interp is enabled
	bool getFlagUseInterpolation() const
	{
		return ADSBData::useInterp;
	}
	//! Turn interpolation on or off
	void setFlagUseInterpolation(bool interp);

	//! Get the path drawing mode
	Flight::PathDrawMode getPathDrawMode() const
	{
		return Flight::pathDrawMode;
	}
	//! Change the path drawing mode
	void setPathDrawMode(Flight::PathDrawMode mode)
	{
		Flight::pathDrawMode = mode;
		emit pathDrawModeChanged(mode);
	}

	//! Get the path coloring mode
	Flight::PathColorMode getPathColorMode() const
	{
		return Flight::pathColorMode;
	}
	//! Change the path coloring mode
	void setPathColorMode(Flight::PathColorMode mode)
	{
		Flight::setPathColorMode(mode);
		emit pathColorModeChanged(mode);
	}

private:
	bool labelsVisible; //!< are labels shown

	static StelTextureSP planeTexture; //!< the texture used for drawing the plane icons
	FlightMgr flightMgr; //!< The FlightMgr
	LinearFader displayFader; //!< Fader to fade in and out on enable/disable

	PlanesDialog *settingsDialog; //!< Configuration window

	BSRecordingDataSource bsRecordingDataSource; //!< data source for loading files
	BSDataSource bsDataSource; //!< data source for database and data port

	StelButton *toolbarButton; //!< Button to enable/disable plugin

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
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule()    const override;
	virtual StelPluginInfo getPluginInfo() const override;
	virtual QObjectList getExtensionList() const override { return QObjectList(); }
};

#endif /*PLANES_HPP_*/

