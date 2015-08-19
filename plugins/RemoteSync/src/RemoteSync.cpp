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

#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"

#include <QDebug>
#include <QSettings>

//! This method is the one called automatically by the StelModuleMgr just after loading the dynamic library
StelModule* RemoteSyncStelPluginInterface::getStelModule() const
{
	return new RemoteSync();
}

StelPluginInfo RemoteSyncStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	//Q_INIT_RESOURCE(RemoteSync);

	StelPluginInfo info;
	info.id = "RemoteSync";
	info.displayedName = N_("Remote Sync");
	info.authors = "Florian Schaukowitsch and Georg Zotti";
	info.contact = "http://homepage.univie.ac.at/Georg.Zotti";
	info.description = N_("<p>Provides state synchronization for multiple Stellarium instances running in a network.</p> "
			      "<p>This can be used, for example, to create multi-screen/panorama setups using multiple physical PCs.</p>"
			      "<p>See manual for detailed description.</p>"
			      "<p>This plugin was developed during ESA SoCiS 2015.</p>");
	info.version = REMOTESYNC_VERSION;
	return info;
}

RemoteSync::RemoteSync() : state(IDLE), server(NULL), client(NULL)
{
	setObjectName("RemoteSync");

	configDialog = new RemoteSyncDialog();
	conf = StelApp::getInstance().getSettings();
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

void RemoteSync::init()
{
	if (!conf->childGroups().contains("RemoteSync"))
		restoreDefaultSettings();

	loadSettings();

	qDebug()<<"[RemoteSync] Plugin initialized";

	// TODO create actions/buttons, if required
}

void RemoteSync::update(double deltaTime)
{
	//include update logic here
	Q_UNUSED(deltaTime);
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
		this->clientServerPort = port;
		emit clientServerPortChanged(port);
	}
}

void RemoteSync::setServerPort(const int port)
{
	if(port!= serverPort)
	{
		serverPort = port;
		emit serverPortChanged(port);
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
		}
	}
	else
		qWarning()<<"[RemoteSync] startServer: invalid state";
}

void RemoteSync::stopServer()
{
	if(state == SERVER)
	{
		server->stop();
		delete server;
		setState(IDLE);
	}
	else
		qWarning()<<"[RemoteSync] stopServer: invalid state";
}

void RemoteSync::connectToServer()
{
	if(state == IDLE)
	{
		client = new SyncClient(this);
		connect(client, SIGNAL(connectionError()), this, SLOT(clientConnectionFailed()));
		connect(client, SIGNAL(connected()), this, SLOT(clientConnected()));
		connect(client, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
		client->connectToServer(clientServerHost,clientServerPort);
		setState(CLIENT_CONNECTING);
	}
	else
		qWarning()<<"[RemoteSync] connectToServer: invalid state";
}

void RemoteSync::clientConnected()
{
	setState(CLIENT);
}

void RemoteSync::clientDisconnected()
{
	setState(IDLE);
	client->deleteLater();
}

void RemoteSync::clientConnectionFailed()
{
	setState(IDLE);
	setError(client->errorString());
	client->deleteLater();
}

void RemoteSync::disconnectFromServer()
{
	if(state == CLIENT)
	{
		client->disconnectFromServer();
	}
	else
		qWarning()<<"[RemoteSync] disconnectFromServer: invalid state";
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
	setClientServerPort(conf->value("clientServerPort",20180).toInt());
	setServerPort(conf->value("serverPort",20180).toInt());
	conf->endGroup();
}

void RemoteSync::saveSettings()
{
	conf->beginGroup("RemoteSync");
	conf->setValue("clientServerHost",clientServerHost);
	conf->setValue("clientServerPort",clientServerPort);
	conf->setValue("serverPort",serverPort);
	conf->endGroup();
}

void RemoteSync::setState(RemoteSync::SyncState state)
{
	if(state != this->state)
	{
		this->state = state;
		qDebug()<<"[RemoteSync] New state:"<<state;
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
		default:
			deb<<"RemoteSync::SyncState(" <<int(state)<<')';
			break;
	}

	return deb;
}
