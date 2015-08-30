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
#include "SyncMessages.hpp"

#include "StelTranslator.hpp"

#include <QDateTime>
#include <QTcpSocket>
#include <QTimerEvent>

using namespace SyncProtocol;

SyncClient::SyncClient(QObject *parent)
	: QObject(parent), isConnecting(false), server(NULL), timeoutTimerId(-1)
{
}

SyncClient::~SyncClient()
{
	disconnectFromServer();
}

void SyncClient::connectToServer(const QString &host, const int port)
{
	if(server)
	{
		disconnectFromServer();
	}

	handlerList.resize(MSGTYPE_SIZE);
	handlerList[ERROR] = new ClientErrorHandler(this);
	handlerList[SERVER_CHALLENGE] = new ClientAuthHandler(this);
	handlerList[SERVER_CHALLENGERESPONSEVALID] = new ClientAuthHandler(this);
	handlerList[ALIVE] = new ClientAliveHandler();

	//these are the actual sync handlers
	handlerList[TIME] = new ClientTimeHandler();
	handlerList[LOCATION] = new ClientLocationHandler();
	handlerList[SELECTION] = new ClientSelectionHandler();

	server = new SyncRemotePeer(new QTcpSocket(this), true, handlerList );
	connect(server->sock, SIGNAL(connected()), this, SLOT(socketConnected()));
	connect(server->sock, SIGNAL(disconnected()),this, SLOT(socketDisconnected()));
	connect(server->sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
	connect(server->sock, SIGNAL(readyRead()), this, SLOT(dataReceived()));

	isConnecting = true;
	qDebug()<<"[SyncClient] Connecting to"<<(host + ":" + QString::number(port));
	server->sock->connectToHost(host,port);
	timeoutTimerId = startTimer(2000,Qt::VeryCoarseTimer); //the connection is checked all 5 seconds
}

void SyncClient::disconnectFromServer()
{
	if(server)
	{
		if(server->sock->state()!= QAbstractSocket::UnconnectedState)
		{
			server->sock->disconnectFromHost();
			if(!server)
				return;

			if(server->sock->state() != QAbstractSocket::UnconnectedState && !server->sock->waitForDisconnected(500))
			{
				qDebug()<<"[SyncClient] Error disconnecting, aborting socket:"<<server->sock->error();
				server->sock->abort();
			}
		}

		//this must be tested AGAIN because waitForDisconnected might re-enter here trough the disconnected signal
		if(server)
		{
			killTimer(timeoutTimerId);

			isConnecting = false;
			server->sock->deleteLater();
			delete server;
			server = NULL;

			//delete handlers
			foreach(SyncMessageHandler* h, handlerList)
			{
				if(h)
					delete h;
			}
			handlerList.clear();
		}
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

	qint64 curTime = QDateTime::currentMSecsSinceEpoch();
	qint64 diff = curTime - server->lastReceiveTime;
	qint64 writeDiff = curTime - server->lastSendTime;


	if(writeDiff>5000)
	{
		//send an ALIVE message
		Alive msg;
		server->writeMessage(msg);
	}


	if(diff > 15000)
	{
		qDebug()<<"[SyncClient] No data received for"<<diff<<"ms, timing out";
		emitError(q_("Connection timed out"));
		emit connectionError();
	}
}


bool SyncClient::isConnected() const
{
	return server->sock->state() == QAbstractSocket::ConnectedState;
}

void SyncClient::socketConnected()
{
	isConnecting = false;
	qDebug()<<"[SyncClient] Socket connection established, waiting for challenge";
	//set low delay option
	server->sock->setSocketOption(QAbstractSocket::LowDelayOption,1);
}

void SyncClient::socketDisconnected()
{
	qDebug()<<"[SyncClient] Socket disconnected";
	disconnectFromServer();
	emit disconnected();
}

void SyncClient::socketError(QAbstractSocket::SocketError err)
{
	Q_UNUSED(err);
	emitError(server->sock->errorString());
}

void SyncClient::emitError(const QString &msg)
{
	errorStr = msg;
	qDebug()<<"[SyncClient] Connection error:"<<msg<<", connection state:"<<server->sock->state();
	disconnectFromServer();
	emit connectionError();
}

void SyncClient::dataReceived()
{
	qDebug()<<"[SyncClient] server data received";
	//a chunk of data is avaliable for reading
	server->receiveMessage();
}
