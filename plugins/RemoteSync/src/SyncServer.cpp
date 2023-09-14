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

#include "SyncServer.hpp"
#include "SyncMessages.hpp"
#include "SyncServerHandlers.hpp"
#include "SyncServerEventSenders.hpp"

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimerEvent>


Q_LOGGING_CATEGORY(syncServer,"stel.plugin.remoteSync.server")

using namespace SyncProtocol;

SyncServer::SyncServer(QObject* parent, bool allowVersionMismatch)
	: QObject(parent), stopping(false), timeoutTimerId(-1)
{
	qserver = new QTcpServer(this);
	connect(qserver,SIGNAL(newConnection()), this, SLOT(handleNewConnection()));
	connect(qserver,SIGNAL(acceptError(QAbstractSocket::SocketError)),this,SLOT(connectionError(QAbstractSocket::SocketError)));

	//create message handlers
	handlerHash.clear();
	handlerHash[SYNC_ERROR] =  new ServerErrorHandler();
	handlerHash[CLIENT_CHALLENGE_RESPONSE] = new ServerAuthHandler(this, allowVersionMismatch);
	handlerHash[ALIVE] = new ServerAliveHandler();
}

SyncServer::~SyncServer()
{
	stop();

	//delete handlers
	for (auto* h : qAsConst(handlerHash))
	{
		if(h)
			delete h;
	}
	handlerHash.clear();

	qCDebug(syncServer)<<"Destroyed";
}

bool SyncServer::start(quint16 port)
{
	if(qserver->isListening())
		stop();

	bool ok = qserver->listen(QHostAddress::Any, port);
	if(ok)
	{
		qCDebug(syncServer)<<"Started on port"<<port;

		timeoutTimerId = startTimer(5000,Qt::VeryCoarseTimer);

		//create senders
		addSender(new TimeEventSender());
		addSender(new LocationEventSender());
		addSender(new SelectionEventSender());
		addSender(new StelPropertyEventSender());
		addSender(new ViewEventSender());
	}
	else
		qCCritical(syncServer)<<"Error while starting:"<<qserver->errorString();
	return ok;
}

void SyncServer::addSender(SyncServerEventSender *snd)
{
	snd->server = this;
	senderList.append(snd);
}

void SyncServer::broadcastMessage(const SyncMessage &msg)
{
	qCDebug(syncServer)<<"Broadcast message"<<msg.toString();
	qint64 size = msg.createFullMessage(broadcastBuffer);

	if(!size)
	{
		//crash here when message is too large in debugging
		Q_ASSERT(true);
		qCCritical(syncServer)<<"A message is too large for broadcast! Message buffer contents follow...";
		qCCritical(syncServer)<<broadcastBuffer.toHex();
		//stop server
		stop();
		return;
	}

	for (auto* client : qAsConst(clients))
	{
		if(client->isAuthenticated())
		{
			client->writeData(broadcastBuffer,size);
		}
	}
}

void SyncServer::stop()
{
	if(qserver->isListening())
	{
		stopping = true;
		killTimer(timeoutTimerId);

		qserver->close();

		//delete senders
		for (auto* s : qAsConst(senderList))
		{
			if(s)
				delete s;
		}
		senderList.clear();

		for (auto it = clients.begin(); it!=clients.end();)
		{
			//this may cause disconnected signal, which will remove the client
			SyncRemotePeer* peer = *it;
			peer->disconnectPeer();
		}

		qCDebug(syncServer)<<"Stopped listening";

		checkStopState();
	}
}

void SyncServer::update()
{
	for (auto* s : qAsConst(senderList))
	{
		s->update();
	}
}

void SyncServer::timerEvent(QTimerEvent *evt)
{
	if(evt->timerId() == timeoutTimerId)
	{
		checkTimeouts();
		evt->accept();
	}
}

void SyncServer::checkTimeouts()
{
	//iterate over the connected clients
	for (auto* client : qAsConst(clients))
	{
		client->checkTimeout();
	}
}

void SyncServer::checkStopState()
{
	if(stopping)
	{
		if(clients.isEmpty())
		{
			qCDebug(syncServer)<<"All clients disconnected";
			stopping = false;
			emit serverStopped();
		}
	}
}

QString SyncServer::errorString() const
{
	return qserver->errorString();
}

void SyncServer::handleNewConnection()
{
	QTcpSocket* newConn = qserver->nextPendingConnection();

	SyncRemotePeer* newClient = new SyncRemotePeer(newConn,false,handlerHash);
	newClient->peerLog(QtDebugMsg, "New client connection");
	//add to client list
	clients.append(newClient);

	qCDebug(syncServer)<<clients.size()<<"current connections";

	//hook up disconnect signal
	connect(newClient, SIGNAL(disconnected(bool)), this, SLOT(clientDisconnected(bool)));

	//write challenge
	ServerChallenge msg;
	msg.clientId = newClient->getID();
	newClient->writeMessage(msg);
}

void SyncServer::clientAuthenticated(SyncRemotePeer &peer)
{
	//we have to send the client the current app state
	for (auto* s : qAsConst(senderList))
	{
		s->newClientConnected(peer);
	}
}

void SyncServer::clientDisconnected(bool clean)
{
	SyncRemotePeer* peer = qobject_cast<SyncRemotePeer*>(sender());

	if(!clean)
	{
		qCWarning(syncServer)<<"Client disconnected with error"<<peer->getError();
	}
	clients.removeAll(peer);
	peer->deleteLater();
	qCDebug(syncServer)<<clients.size()<<"current connections";
	checkStopState();
}

void SyncServer::connectionError(QAbstractSocket::SocketError err)
{
	Q_UNUSED(err)
	qCWarning(syncServer)<<"Could not accept an incoming connection, socket error is: "<<qserver->errorString();
}
