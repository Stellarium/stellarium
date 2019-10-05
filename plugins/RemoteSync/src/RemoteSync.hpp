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
 
#ifndef REMOTESYNC_HPP
#define REMOTESYNC_HPP

#include "StelModule.hpp"
#include "SyncClient.hpp"
#include "SyncServer.hpp"

#include <QFont>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QTimer>

class RemoteSyncDialog;

Q_DECLARE_LOGGING_CATEGORY(remoteSync)

/*! @defgroup remoteSync RemoteSync plug-in
@{
Provides state synchronization for multiple Stellarium instances
running in a network. See manual for detailed description.

@}
*/

//! @ingroup remoteSync
//! Main class of the %RemoteSync plug-in.
//! Provides a synchronization mechanism for multiple Stellarium instances in a network.
//! This plugin has been developed during ESA SoCiS 2015/2016.
//! @author Florian Schaukowitsch
//! @author Georg Zotti
class RemoteSync : public StelModule
{
	Q_OBJECT
	Q_ENUMS(SyncState ClientBehaviour)

public:
	enum SyncState
	{
		IDLE, //Plugin is disabled
		SERVER, //Plugin is running as server
		CLIENT, //Plugin is connected as a client to a server
		CLIENT_CONNECTING, //Plugin is currently trying to connect to a server
		CLIENT_CLOSING, //Plugin is disconnecting from the server
		CLIENT_WAIT_RECONNECT //Plugin is waiting to try reconnecting again
	};

	//! Defines behavior when client connection is lost/server quits
	enum ClientBehavior
	{
		NONE, //do nothing
		RECONNECT, //automatically try to reconnect
		QUIT //quit the client
	};

	RemoteSync();
	virtual ~RemoteSync() Q_DECL_OVERRIDE;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	virtual void update(double deltaTime) Q_DECL_OVERRIDE;

	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;

	virtual bool configureGui(bool show=true) Q_DECL_OVERRIDE;
	///////////////////////////////////////////////////////////////////////////

	QString getClientServerHost() const { return clientServerHost; }
	quint16 getClientServerPort() const { return clientServerPort; }
	quint16 getServerPort() const { return serverPort; }
	SyncClient::SyncOptions getClientSyncOptions() const { return syncOptions; }
	QStringList getStelPropFilter() const { return stelPropFilter; }
	ClientBehavior getConnectionLostBehavior() const { return connectionLostBehavior; }
	ClientBehavior getQuitBehavior() const { return quitBehavior; }

	SyncState getState() const { return state; }
	//! Very few propertries cannot be synchronized for technical reasons.
	static bool isPropertyBlacklisted(const QString &name);

public slots:
	void setClientServerHost(const QString& clientServerHost);
	void setClientServerPort(const int port);
	void setServerPort(const int port);
	void setClientSyncOptions(SyncClient::SyncOptions options);
	void setStelPropFilter(const QStringList& stelPropFilter);
	void setConnectionLostBehavior(const ClientBehavior bh);
	void setQuitBehavior(const ClientBehavior bh);

	//! Starts the plugin in server mode, on the port specified by the serverPort property.
	//! If currently in a state other than IDLE, this call has no effect.
	void startServer();

	//! Tries to disconnect all current clients and stops the server, returning to the IDLE state.
	//! Only has an effect in the SERVER state.
	void stopServer();

	//! Connects the plugin to the server specified by the clientServerHost and clientServerPort properties.
	//! If currently in a state other than IDLE or CLIENT_WAIT_RECONNECT, this call has no effect.
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
	void errorOccurred(const QString& errorString);
	void clientServerHostChanged(const QString& clientServerHost);
	void clientServerPortChanged(const int port);
	void serverPortChanged(const int port);
	void clientSyncOptionsChanged(const SyncClient::SyncOptions options);
	void stelPropFilterChanged(const QStringList& stelPropFilter);
	void connectionLostBehaviorChanged(const ClientBehavior bh);
	void quitBehaviorChanged(const ClientBehavior bh);

	void stateChanged(RemoteSync::SyncState state);

private slots:
	void clientDisconnected(bool clean);
	void clientConnected();
private:
	void setState(RemoteSync::SyncState state);
	void setError(const QString& errorString);
	QVariant argsGetOptionWithArg(const QStringList &args, QString shortOpt, QString longOpt, QVariant defaultValue);

	SyncState applyClientBehavior(ClientBehavior bh);

	static QString packStringList(const QStringList props);
	static QStringList unpackStringList(const QString packedProps);

	//The host string/IP addr to connect to
	QString clientServerHost;
	//The host port to connect to
	quint16 clientServerPort;
	//the port used in server mode
	quint16 serverPort;
	SyncClient::SyncOptions syncOptions;
	QStringList stelPropFilter;
	ClientBehavior connectionLostBehavior;
	ClientBehavior quitBehavior;

	QTimer reconnectTimer;

	SyncState state;
	SyncServer* server;
	SyncClient* client;

	// the last error that occurred
	QString errorString;

	QSettings* conf;

	// GUI
	RemoteSyncDialog* configDialog;
	// A stringlist which contains property names which cannot be synchronized.
	// The list currently is fixed.
	static QStringList propertyBlacklist;
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
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /*REMOTESYNC_HPP*/

