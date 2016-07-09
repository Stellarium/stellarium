/*
 * Stellarium Remote Sync plugin
 * Copyright (C) 2015 Florian Schaukowitsch
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
 
#ifndef REMOTESYNC_HPP_
#define REMOTESYNC_HPP_

#include <QFont>
#include <QKeyEvent>

#include "StelModule.hpp"

class RemoteSyncDialog;
class SyncServer;
class SyncClient;

//! Main class of the RemoteSync plug-in.
//! Provides a synchronization mechanism for multiple Stellarium instances in a network.
//! This plugin has been developed during ESA SoCiS 2015.
class RemoteSync : public StelModule
{
	Q_OBJECT
	Q_ENUMS(SyncState)

public:
	enum SyncState
	{
		IDLE, //Plugin is disabled
		SERVER, //Plugin is running as server
		CLIENT, //Plugin is connected as a client to a server
		CLIENT_CONNECTING //Plugin is currently trying to connect to a server
	};

	RemoteSync();
	virtual ~RemoteSync();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	virtual void update(double deltaTime) Q_DECL_OVERRIDE;

	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;

	virtual bool configureGui(bool show=true) Q_DECL_OVERRIDE;
	///////////////////////////////////////////////////////////////////////////

	QString getClientServerHost() const { return clientServerHost; }
	int getClientServerPort() const { return clientServerPort; }
	int getServerPort() const { return serverPort; }
	SyncState getState() const { return state; }

public slots:
	void setClientServerHost(const QString& clientServerHost);
	void setClientServerPort(const int port);
	void setServerPort(const int port);

	//! Starts the plugin in server mode, on the port specified by the serverPort property.
	//! If currently in a state other than IDLE, this call has no effect.
	void startServer();

	//! Tries to disconnect all current clients and stops the server, returning to the IDLE state.
	//! Only has an effect in the SERVER state.
	void stopServer();

	//! Connects the plugin to the server specified by the clientServerHost and clientServerPort properties.
	//! If currently in a state other than IDLE, this call has no effect.
	void connectToServer();

	//! Disconnects from the server and returns to the IDLE state.
	//! Only has an effect in the CLIENT state.
	void disconnectFromServer();

	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "RemoteSync" section in Stellarium's
	//! configuration file. If no such section exists, it will load default
	//! values.
	//! @see saveSettings(), restoreSettings()
	void loadSettings();

	//! Save the plug-in's settings to the configuration file.
	//! @see loadSettings(), restoreSettings()
	void saveSettings();

	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings() and saveSettings().
	void restoreDefaultSettings();

signals:
	void errorOccurred(const QString errorString);
	void clientServerHostChanged(const QString clientServerHost);
	void clientServerPortChanged(const int port);
	void serverPortChanged(const int port);
	void stateChanged(RemoteSync::SyncState state);

private slots:
	void clientDisconnected();
	void clientConnected();
	void clientConnectionFailed();
private:
	void setState(RemoteSync::SyncState state);
	void setError(const QString& errorString);

	//The host string/IP addr to connect to
	QString clientServerHost;
	//The host port to connect to
	int clientServerPort;
	//the port used in server mode
	int serverPort;
	SyncState state;

	SyncServer* server;
	SyncClient* client;

	// the last error that occurred
	QString errorString;

	QSettings* conf;

	// GUI
	RemoteSyncDialog* configDialog;
};

Q_DECLARE_METATYPE(RemoteSync::SyncState)

//! Stream overload for debugging
QDebug operator<<(QDebug, RemoteSync::SyncState);

#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class RemoteSyncStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*REMOTESYNC_HPP_*/

