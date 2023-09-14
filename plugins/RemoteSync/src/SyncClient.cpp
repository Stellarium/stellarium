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

#include "SyncClient.hpp"
#include "SyncClientHandlers.hpp"

#include <QDateTime>
#include <QTcpSocket>
#include <QTimerEvent>

Q_LOGGING_CATEGORY(syncClient,"stel.plugin.remoteSync.client")

using namespace SyncProtocol;

SyncClient::SyncClient(SyncOptions options, const QStringList &excludeProperties, QObject *parent)
	: QObject(parent),
	  options(options),
	  stelPropFilter(excludeProperties),
	  isConnecting(false),
	  server(nullptr),
	  timeoutTimerId(-1)
{
	handlerHash.clear();
        handlerHash.insert(SYNC_ERROR,  new ClientErrorHandler(this));
	handlerHash.insert(SERVER_CHALLENGE, new ClientAuthHandler(this));
	handlerHash.insert(SERVER_CHALLENGERESPONSEVALID, new ClientAuthHandler(this));
	handlerHash.insert(ALIVE, new ClientAliveHandler());

	//these are the actual sync handlers
	if(options.testFlag(SyncTime))
		handlerHash[TIME] = new ClientTimeHandler(this);
	if(options.testFlag(SyncLocation))
		handlerHash[LOCATION] = new ClientLocationHandler(this);
	if(options.testFlag(SyncSelection))
		handlerHash[SELECTION] = new ClientSelectionHandler(this);
	if(options.testFlag(SyncStelProperty))
		handlerHash[STELPROPERTY] = new ClientStelPropertyUpdateHandler(this, options.testFlag(SkipGUIProps), stelPropFilter);
	if(options.testFlag(SyncView))
		handlerHash[VIEW] = new ClientViewHandler(this);

	//fill unused handlers with dummies
	for(int t = TIME;t<MSGTYPE_SIZE;++t)
	{
		if(!handlerHash.value(static_cast<SyncMessageType>(t)))
		{
			qCDebug(syncClient) << "RemoteSyncClient: Adding dummy for message " << SyncMessage::toString(static_cast<SyncMessageType>(t));
			handlerHash[static_cast<SyncMessageType>(t)] = new DummyMessageHandler();
		}
	}
}

SyncClient::~SyncClient()
{
	disconnectFromServer();
	delete server;

	//delete handlers
	for (auto* h : qAsConst(handlerHash))
	{
		if(h)
			delete h;
	}
	handlerHash.clear();

	qCDebug(syncClient)<<"Destroyed";
}

void SyncClient::connectToServer(const QString &host, const int port)
{
	if(server)
	{
		disconnectFromServer();
	}

	QTcpSocket* sock = new QTcpSocket();
	connect(sock, SIGNAL(connected()), this, SLOT(socketConnected()));
	server = new SyncRemotePeer(sock, true, handlerHash );
	connect(server, SIGNAL(disconnected(bool)), this, SLOT(serverDisconnected(bool)));

	isConnecting = true;
	qCDebug(syncClient)<<"Connecting to"<<(host + ":" + QString::number(port))<<", with options"<<options;
	timeoutTimerId = startTimer(2000,Qt::VeryCoarseTimer); //the connection is checked all 5 seconds
	sock->connectToHost(host,static_cast<quint16>(port));
}

void SyncClient::disconnectFromServer()
{
	if(server)
	{
		server->disconnectPeer();
	}
}

void SyncClient::timerEvent(QTimerEvent *evt)
{
	if(evt->timerId() == timeoutTimerId)
	{
		checkTimeout();
		evt->accept();
	}
}

void SyncClient::checkTimeout()
{
	if(!server)
		return;

	server->checkTimeout();
}

void SyncClient::serverDisconnected(bool clean)
{
	qCDebug(syncClient)<<"Disconnected from server";
	if(!clean)
		errorStr = server->getError();
	server->deleteLater();
	server = nullptr;
	emit disconnected(errorStr.isEmpty());
}

void SyncClient::socketConnected()
{
	qCDebug(syncClient)<<"Socket connected";
}

void SyncClient::emitServerError(const QString &errorStr)
{
	this->errorStr = errorStr;
}

bool SyncClient::isPropertyFilteredAway(QString property) const
{
	return stelPropFilter.contains(property);
}
