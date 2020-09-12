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

#include "RemoteSync.hpp"
#include "RemoteSyncDialog.hpp"

#include "SyncServer.hpp"
#include "SyncClient.hpp"

#include "CLIProcessor.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"

#include <QApplication>
#include <QDebug>
#include <QSettings>

#include <stdexcept>

Q_LOGGING_CATEGORY(remoteSync,"stel.plugin.remoteSync")

//! This method is the one called automatically by the StelModuleMgr just after loading the dynamic library
StelModule* RemoteSyncStelPluginInterface::getStelModule() const
{
	return new RemoteSync();
}

StelPluginInfo RemoteSyncStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(RemoteSync);

	StelPluginInfo info;
	info.id = "RemoteSync";
	info.displayedName = N_("Remote Sync");
	info.authors = "Florian Schaukowitsch and Georg Zotti";
	info.contact = "http://homepage.univie.ac.at/Georg.Zotti";
	info.description = N_("Provides state synchronization for multiple Stellarium instances running in a network. See manual for detailed description.");
	info.acknowledgements = N_("This plugin was created in the 2015/2016 campaigns of the ESA Summer of Code in Space programme.");
	info.version = REMOTESYNC_PLUGIN_VERSION;
	info.license = REMOTESYNC_PLUGIN_LICENSE;
	return info;
}


// A list that holds properties that cannot be sync'ed for technical reasons.
// Currently only HipsMgr.surveys cannot be synchronized.
QStringList RemoteSync::propertyBlacklist;


RemoteSync::RemoteSync()
	: clientServerPort(20180)
	, serverPort(20180)
	, connectionLostBehavior(ClientBehavior::RECONNECT)
	, quitBehavior(ClientBehavior::NONE)
	, state(IDLE)
	, server(Q_NULLPTR)
	, client(Q_NULLPTR)
{
	setObjectName("RemoteSync");

	configDialog = new RemoteSyncDialog();
	conf = StelApp::getInstance().getSettings();

	reconnectTimer.setSingleShot(true);
	connect(&reconnectTimer, SIGNAL(timeout()), this, SLOT(connectToServer()));

	// There are a few unsynchronizable properties. They must be listed here!
	propertyBlacklist.push_back("HipsMgr.surveys");
}

RemoteSync::~RemoteSync()
{
	delete configDialog;
}

bool RemoteSync::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

QVariant RemoteSync::argsGetOptionWithArg(const QStringList& args, QString shortOpt, QString longOpt, QVariant defaultValue)
{
	// Don't see anything after a -- as an option
	int lastOptIdx = args.indexOf("--");
	if (lastOptIdx == -1)
		lastOptIdx = args.size();

	for (int i=0; i<lastOptIdx; i++)
	{
		bool match(false);
		QString argStr;

		// form -n=arg
		if ((!shortOpt.isEmpty() && args.at(i).left(shortOpt.length()+1)==shortOpt+"="))
		{
			match=true;
			argStr=args.at(i).right(args.at(i).length() - shortOpt.length() - 1);
		}
		// form --number=arg
		else if (args.at(i).left(longOpt.length()+1)==longOpt+"=")
		{
			match=true;
			argStr=args.at(i).right(args.at(i).length() - longOpt.length() - 1);
		}
		// forms -n arg and --number arg
		else if ((!shortOpt.isEmpty() && args.at(i)==shortOpt) || args.at(i)==longOpt)
		{
			if (i+1>=lastOptIdx)
			{
				// i.e., option given as last option, but without arguments. Last chance: default value!
				if (defaultValue.isValid())
				{
					return defaultValue;
				}
				else
				{
					throw (std::runtime_error(qPrintable("optarg_missing ("+longOpt+")")));
				}
			}
			else
			{
				match=true;
				argStr=args.at(i+1);
				i++;  // skip option argument in next iteration
			}
		}

		if (match)
		{
			return QVariant(argStr);
		}
	}
	return defaultValue;
}

void RemoteSync::init()
{
	if (!conf->childGroups().contains("RemoteSync"))
		restoreDefaultSettings();

	loadSettings();

	qCDebug(remoteSync)<<"Plugin initialized";

	//parse command line args
	QStringList args = StelApp::getCommandlineArguments();
	QString syncMode = argsGetOptionWithArg(args,"","--syncMode","").toString();
	QString syncHost = argsGetOptionWithArg(args,"","--syncHost","").toString();
	int syncPort = argsGetOptionWithArg(args,"","--syncPort",0).toInt();

	if(syncMode=="server")
	{
		if(syncPort!=0)
			setServerPort(syncPort);
		qCDebug(remoteSync)<<"Starting server from command line";
		startServer();
	}
	else if(syncMode=="client")
	{
		if(!syncHost.isEmpty())
			setClientServerHost(syncHost);
		if(syncPort!=0)
			setClientServerPort(syncPort);
		qCDebug(remoteSync)<<"Connecting to server from command line";
		connectToServer();
	}

	connect(StelApp::getInstance().getCore(), SIGNAL(configurationDataSaved()), this, SLOT(saveSettings()));
}

void RemoteSync::update(double deltaTime)
{
	Q_UNUSED(deltaTime)
	if(server)
	{
		//pass update on to server, client does not need this
		server->update();
	}
}

double RemoteSync::getCallOrder(StelModuleActionName actionName) const
{
	//we want update() to be called as late as possible
	if(actionName == ActionUpdate)
		return 100000.0;

	return StelModule::getCallOrder(actionName);
}


void RemoteSync::setClientServerHost(const QString &clientServerHost)
{
	if(clientServerHost != this->clientServerHost)
	{
		this->clientServerHost = clientServerHost;
		emit clientServerHostChanged(clientServerHost);
	}
}

void RemoteSync::setClientServerPort(const int port)
{
	if(port != this->clientServerPort)
	{
		this->clientServerPort = static_cast<quint16>(port);
		emit clientServerPortChanged(port);
	}
}

void RemoteSync::setServerPort(const int port)
{
	if(port!= serverPort)
	{
		serverPort = static_cast<quint16>(port);
		emit serverPortChanged(port);
	}
}

void RemoteSync::setClientSyncOptions(SyncClient::SyncOptions options)
{
	if(options!=syncOptions)
	{
		syncOptions = options;
		emit clientSyncOptionsChanged(options);
	}
}

void RemoteSync::setStelPropFilter(const QStringList &stelPropFilter)
{
	if(stelPropFilter!=this->stelPropFilter)
	{
		this->stelPropFilter = stelPropFilter;
		emit stelPropFilterChanged(stelPropFilter);
	}
}

void RemoteSync::setConnectionLostBehavior(const ClientBehavior bh)
{
	if(connectionLostBehavior!=bh)
	{
		connectionLostBehavior = bh;
		emit connectionLostBehaviorChanged(bh);
	}
}

void RemoteSync::setQuitBehavior(const ClientBehavior bh)
{
	if(quitBehavior!=bh)
	{
		quitBehavior = bh;
		emit quitBehaviorChanged(bh);
	}
}

void RemoteSync::startServer()
{
	if(state == IDLE)
	{
		server = new SyncServer(this);
		if(server->start(serverPort))
			setState(SERVER);
		else
		{
			setError(server->errorString());
			delete server;
			server = Q_NULLPTR;
		}
	}
	else
		qCWarning(remoteSync)<<"startServer: invalid state";
}

void RemoteSync::stopServer()
{
	if(state == SERVER)
	{
		connect(server, SIGNAL(serverStopped()), server, SLOT(deleteLater()));
		server->stop();
		server = Q_NULLPTR;
		setState(IDLE);
	}
	else
		qCWarning(remoteSync)<<"stopServer: invalid state";
}

void RemoteSync::connectToServer()
{
	if(state == IDLE || state == CLIENT_WAIT_RECONNECT)
	{
		client = new SyncClient(syncOptions, stelPropFilter, this);
		connect(client, SIGNAL(connected()), this, SLOT(clientConnected()));
		connect(client, SIGNAL(disconnected(bool)), this, SLOT(clientDisconnected(bool)));
		setState(CLIENT_CONNECTING);
		client->connectToServer(clientServerHost,clientServerPort);
	}
	else
		qCWarning(remoteSync)<<"connectToServer: invalid state";
}

void RemoteSync::clientConnected()
{
	Q_ASSERT(state == CLIENT_CONNECTING);
	setState(CLIENT);
}

void RemoteSync::clientDisconnected(bool clean)
{
	QString errStr = client->errorString();
	client->deleteLater();
	client = Q_NULLPTR;

	if(!clean)
	{
		setError(errStr);
	}

	setState(applyClientBehavior(clean ? quitBehavior : connectionLostBehavior));
}

RemoteSync::SyncState RemoteSync::applyClientBehavior(ClientBehavior bh)
{
	if(state!=CLIENT_CLOSING) //when client closes we do nothing
	{
		switch (bh) {
			case RECONNECT:
				reconnectTimer.start();
				return CLIENT_WAIT_RECONNECT;
			case QUIT:
				StelApp::getInstance().quit();
				break;
			default:
				break;
		}
	}

	return IDLE;
}

void RemoteSync::disconnectFromServer()
{
	if(state == CLIENT)
	{
		setState(CLIENT_CLOSING);
		client->disconnectFromServer();
	}
	else if(state == CLIENT_WAIT_RECONNECT)
	{
		reconnectTimer.stop();
		setState(IDLE);
	}
	else
		qCWarning(remoteSync)<<"disconnectFromServer: invalid state"<<state;
}

void RemoteSync::restoreDefaultSettings()
{
	Q_ASSERT(conf);
	// Remove the old values...
	conf->remove("RemoteSync");
	// ...load the default values...
	loadSettings();
	// ...and then save them.
	saveSettings();
}

void RemoteSync::loadSettings()
{
	conf->beginGroup("RemoteSync");
	setClientServerHost(conf->value("clientServerHost","127.0.0.1").toString());
	setClientServerPort(static_cast<quint16>(conf->value("clientServerPort",20180).toUInt()));
	setServerPort(static_cast<quint16>(conf->value("serverPort",20180).toUInt()));
	setClientSyncOptions(SyncClient::SyncOptions(conf->value("clientSyncOptions", SyncClient::ALL).toInt()));
	setStelPropFilter(unpackStringList(conf->value("stelPropFilter").toString()));
	setConnectionLostBehavior(static_cast<ClientBehavior>(conf->value("connectionLostBehavior",1).toInt()));
	setQuitBehavior(static_cast<ClientBehavior>(conf->value("quitBehavior").toInt()));
	reconnectTimer.setInterval(conf->value("clientReconnectInterval", 5000).toInt());
	conf->endGroup();
}

void RemoteSync::saveSettings()
{
	conf->beginGroup("RemoteSync");
	conf->setValue("clientServerHost",clientServerHost);
	conf->setValue("clientServerPort",clientServerPort);
	conf->setValue("serverPort",serverPort);
	conf->setValue("clientSyncOptions",static_cast<int>(syncOptions));
	conf->setValue("stelPropFilter", packStringList(stelPropFilter));
	conf->setValue("connectionLostBehavior", connectionLostBehavior);
	conf->setValue("quitBehavior", quitBehavior);
	conf->setValue("clientReconnectInterval", reconnectTimer.interval());
	conf->endGroup();
}

QString RemoteSync::packStringList(const QStringList props)
{
	return props.join("|");
}

QStringList RemoteSync::unpackStringList(const QString packedProps)
{
	return packedProps.split("|");
}

void RemoteSync::setState(RemoteSync::SyncState state)
{
	if(state != this->state)
	{
		this->state = state;
		qCDebug(remoteSync)<<"New state:"<<state;
		emit stateChanged(state);
	}
}

void RemoteSync::setError(const QString &errorString)
{
	this->errorString = errorString;
	emit errorOccurred(errorString);
}

QDebug operator<<(QDebug deb, RemoteSync::SyncState state)
{
	switch (state) {
		case RemoteSync::IDLE:
			deb<<"IDLE";
			break;
		case RemoteSync::SERVER:
			deb<<"SERVER";
			break;
		case RemoteSync::CLIENT:
			deb<<"CLIENT";
			break;
		case RemoteSync::CLIENT_CONNECTING:
			deb<<"CLIENT_CONNECTING";
			break;
		case RemoteSync::CLIENT_WAIT_RECONNECT:
			deb<<"CLIENT_WAIT_RECONNECT";
			break;
		default:
			deb<<"RemoteSync::SyncState(" <<int(state)<<')';
			break;
	}

	return deb;
}

bool RemoteSync::isPropertyBlacklisted(const QString &name)
{
	return propertyBlacklist.contains(name);
}
