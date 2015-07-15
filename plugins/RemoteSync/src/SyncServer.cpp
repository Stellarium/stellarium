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

void SyncServer::start(int port)
{
	if(qserver->isListening())
		stop();

	qserver->listen(QHostAddress::Any, port);
	qDebug()<<"[SyncServer] Started on port"<<port;
}

void SyncServer::stop()
{
	if(qserver->isListening())
	{
		qserver->close();
		qDebug()<<"[SyncServer] Stopped";
	}
}

void SyncServer::handleNewConnection()
{
	QTcpSocket* newConn = qserver->nextPendingConnection();

	qDebug()<<"[SyncServer] New connection:"<<(newConn->peerAddress().toString() + ":" + newConn->peerPort());
}

void SyncServer::connectionError(QAbstractSocket::SocketError err)
{
	qWarning()<<"[SyncServer] Could not accept an incoming connection, socket error is: "<<err;
}
