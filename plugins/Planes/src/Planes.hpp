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
#include "DatabaseDataSource.hpp"
#include "BSDataSource.hpp"
#include "PlanesDialog.hpp"
#include "DBCredentials.hpp"


Q_DECLARE_METATYPE(DBCredentials)


//! @class Planes
//! This class is the entry point for the plugin.
//! It handles the integration with Stellarium.
//! @author Felix Zeltner
class Planes : public StelObjectModule
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE enablePlanes)
    friend class FlightMgr;
public:
    Planes();
    virtual ~Planes();

    ///////////////////////////////////////////////////////////////////////////
    // Methods defined in the StelModule class
    void init();
    void deinit();
    void update(double deltaTime);
    void draw(StelCore* core);
    double getCallOrder(StelModuleActionName actionName) const;
    void handleKeys(class QKeyEvent* event);
    void handleMouseClicks(class QMouseEvent* event);
    bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
    bool configureGui(bool show = true);

    ////////////////////////////////////////////////////////////////////////////
    // Methods defined in StelObjectModule
    // Forward requests to FlightMgr
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

    QString getName() const
    {
        return "Planes";
    }

    bool isEnabled() const {return displayFader;}

    void loadSettings();
    void saveSettings();

    bool isUsingDB() const
    {
        return bsDataSource.isDatabaseEnabled();
    }
    bool isUsingBS() const
    {
        return bsDataSource.isSocketEnabled();
    }
    bool isDumpingOldFlights() const
    {
        return bsDataSource.isDumpOldFlightsEnabled();
    }
    DBCredentials getDBCreds() const
    {
        return dbc;
    }
    const FlightMgr *getFlightMgr() const
    {
        return &flightMgr;
    }
    QString getBSHost() const
    {
        return bsHost;
    }
    quint16 getBSPort() const
    {
        return bsPort;
    }
    bool isConnectOnStartupEnabled() const;

public slots:
    void enablePlanes(bool b);
    void updateLocation(StelLocation loc)
    {
        Flight::updateObserverPos(loc);
    }

    void setDBCreds(DBCredentials creds);
    void openBSRecording(QString file);
    void connectDBBS();
    void setBSHost(QString host)
    {
        bsHost = host;
    }
    void setBSPort(quint16 port)
    {
        bsPort = port;
    }
    void setConnectOnStartup(bool value);

private:
    static StelTextureSP planeTexture;
    FlightMgr flightMgr;
    LinearFader displayFader;

    PlanesDialog *settingsDialog;

    BSRecordingDataSource bsRecordingDataSource;
    BSDataSource bsDataSource;

    StelButton *settingsButton;
    StelButton *pathsButton;
    StelButton *enableButton;
    QPixmap *onPix;
    QPixmap *offPix;
    QPixmap *glowPix;
    QPixmap *onSettingsPix;
    QPixmap *offSettingsPix;

    DBCredentials dbc;
    QString bsHost;
    quint16 bsPort;
    bool connectOnStartup;
};



#include <QObject>
#include "StelPluginInterface.hpp"

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

