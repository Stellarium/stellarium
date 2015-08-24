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

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimerEvent>


using namespace SyncProtocol;

SyncServer::SyncServer(QObject* parent)
	: QObject(parent)
{
	qserver = new QTcpServer(this);
	connect(qserver,SIGNAL(newConnection()), this, SLOT(handleNewConnection()));
	connect(qserver,SIGNAL(acceptError(QAbstractSocket::SocketError)),this,SLOT(connectionError(QAbstractSocket::SocketError)));
}

SyncServer::~SyncServer()
{
	stop();
}

bool SyncServer::start(int port)
{
	if(qserver->isListening())
		stop();

	bool ok = qserver->listen(QHostAddress::Any, port);
	if(ok)
	{
		qDebug()<<"[SyncServer] Started on port"<<port;

		//create message handlers
		handlerList.resize(MSGTYPE_SIZE);
		handlerList[ERROR] =  new ServerErrorHandler();
		handlerList[CLIENT_CHALLENGE_RESPONSE] = new ServerAuthHandler(true);
		handlerList[ALIVE] = new ServerAliveHandler();

		timeoutTimerId = startTimer(5000,Qt::VeryCoarseTimer);
	}
	else
		qDebug()<<"[SyncServer] Error while starting:"<<errorString();
	return ok;
}

void SyncServer::stop()
{
	if(qserver->isListening())
	{
		killTimer(timeoutTimerId);

		qserver->close();

		for(tClientMap::iterator it = clients.begin();it!=clients.end(); )
		{
			//this may cause disconnected signal, which will remove the client
			QAbstractSocket* sock = it.key();
			sock->disconnectFromHost();
			if(sock->state() != QAbstractSocket::UnconnectedState)
			{
				if(!sock->waitForDisconnected(500))
				{
					sock->abort();
					sock->deleteLater();
					++it;
				}
				else
				{
					//restart iterator because it is most likely invalid
					it = clients.begin();
				}
			}
			else
			{
				//restart iterator because it is most likely invalid
				it = clients.begin();
			}
		}
		clients.clear();

		//delete handlers
		foreach(SyncMessageHandler* h, handlerList)
		{
			if(h)
				delete h;
		}

		handlerList.clear();

		qDebug()<<"[SyncServer] Stopped";
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
	qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

	//iterate over the connected clients
	for(tClientMap::iterator it = clients.begin(); it!=clients.end(); )
	{
		qint64 writeDiff = currentTime - it.value().lastSendTime;
		qint64 readDiff = currentTime - it.value().lastReceiveTime;

		if(writeDiff > 5000)
		{
			//no data sent to this client for some time, send a ALIVE
			Alive msg;
			it.value().writeMessage(msg);
		}


		if(readDiff > 15000)
		{
			if(it.key()->state() == QAbstractSocket::ConnectedState)
			{
				//no data received for some time, assume client timed out
				clientLog(it.key(),QString("No data received for %1ms, timing out").arg(readDiff));
				it.key()->disconnectFromHost();

				//restart iterator, disconnect may have modified client list
				it = clients.begin();
			}
		}
		else
			++it;
	}
}

QString SyncServer::errorString() const
{
	return qserver->errorString();
}

void SyncServer::handleNewConnection()
{
	QTcpSocket* newConn = qserver->nextPendingConnection();
	clientLog(newConn,"New Connection");

	//add to client list
	clients.insert(newConn,SyncRemotePeer(newConn,false,handlerList));
	SyncRemotePeer& peer = clients.last();
	//assign an ID to the client
	peer.id = QUuid::createUuid();

	qDebug()<<"[SyncServer] "<<clients.size()<<" current connections";

	//hook up disconnect, error and data signals
	connect(newConn, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
	connect(newConn, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(clientError(QAbstractSocket::SocketError)));
	connect(newConn, SIGNAL(readyRead()), this, SLOT(clientDataReceived()));

	//set low delay option
	newConn->setSocketOption(QAbstractSocket::LowDelayOption,1);

	//write challenge
	ServerChallenge msg;
	msg.clientId = peer.id;
	peer.writeMessage(msg);
}

void SyncServer::clientDataReceived()
{
	qDebug()<<"[SyncServer] client data received";
	QAbstractSocket* sock = qobject_cast<QAbstractSocket*>(sender());
	tClientMap::iterator it = clients.find(sock);
	if(it!=clients.end())
		(*it).receiveMessage();
	else
	{
		Q_ASSERT(false);
		qCritical()<<"Received data from socket without client";
		sock->disconnectFromHost();
		sock->deleteLater();
	}
}

void SyncServer::clientError(QAbstractSocket::SocketError)
{
	//Note: we also get an error if the client has disconnected, handle it differently?
	QAbstractSocket* sock = qobject_cast<QAbstractSocket*>(sender());
	clientLog(sock, "Socket error: " + sock->errorString());
}

void SyncServer::clientDisconnected()
{
	QAbstractSocket* sock = qobject_cast<QAbstractSocket*>(sender());
	clientLog(sock, "Socket disconnected");
	clients.remove(sock);
	sock->deleteLater();
	qDebug()<<"[SyncServer] "<<clients.size()<<" current connections";
}

void SyncServer::connectionError(QAbstractSocket::SocketError err)
{
	qWarning()<<"[SyncServer] Could not accept an incoming connection, socket error is: "<<err;
}

void SyncServer::clientLog(QAbstractSocket *cl, const QString &msg)
{
	qDebug()<<"[SyncServer][Client"<<(cl->peerAddress().toString() + ":" + QString::number(cl->peerPort()))<<"]:"<<msg;
}
