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
#include <QTcpServer>
#include <QTcpSocket>

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
		qDebug()<<"[SyncServer] Started on port"<<port;
	else
		qDebug()<<"[SyncServer] Error while starting:"<<errorString();
	return ok;
}

void SyncServer::stop()
{
	if(qserver->isListening())
	{
		qserver->close();
		foreach(QAbstractSocket* c , clients)
		{
			c->disconnectFromHost();
			c->deleteLater();
		}
		clients.clear();

		qDebug()<<"[SyncServer] Stopped";
	}
}

QString SyncServer::errorString() const
{
	return qserver->errorString();
}

void SyncServer::handleNewConnection()
{
	QTcpSocket* newConn = qserver->nextPendingConnection();

	clients.append(newConn);
	connect(newConn, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
	connect(newConn, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(clientError(QAbstractSocket::SocketError)));
	clientLog(newConn,"New Connection");
	qDebug()<<"[SyncServer] "<<clients.size()<<" connections";

	newConn->write("Hello you!");
	newConn->waitForBytesWritten(500);
}

void SyncServer::clientError(QAbstractSocket::SocketError)
{
	QAbstractSocket* sock = qobject_cast<QAbstractSocket*>(sender());
	clientLog(sock, "Socket error: " + sock->errorString());
}

void SyncServer::clientDisconnected()
{
	QAbstractSocket* sock = qobject_cast<QAbstractSocket*>(sender());
	clientLog(sock, "Socket disconnected");
	clients.removeOne(sock);
	sock->deleteLater();
}

void SyncServer::connectionError(QAbstractSocket::SocketError err)
{
	qWarning()<<"[SyncServer] Could not accept an incoming connection, socket error is: "<<err;
}

void SyncServer::clientLog(QAbstractSocket *cl, const QString &msg)
{
	qDebug()<<"[SyncServer][Client"<<(cl->peerAddress().toString() + ":" + QString::number(cl->peerPort()))<<"]:"<<msg;
}
