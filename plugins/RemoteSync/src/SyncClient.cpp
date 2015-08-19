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
#include <QTcpSocket>

SyncClient::SyncClient(QObject *parent)
	: QObject(parent), isConnecting(false)
{
	timeoutTimer = new QTimer(this);
	timeoutTimer->setSingleShot(true);
	qsocket = new QTcpSocket(this);
	connect(qsocket, SIGNAL(connected()), this, SLOT(socketConnected()));
	connect(qsocket, SIGNAL(disconnected()),this, SLOT(socketDisconnected()));
	connect(qsocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
	connect(qsocket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
	connect(timeoutTimer, SIGNAL(timeout()), this, SLOT(timeoutOccurred()));
}

SyncClient::~SyncClient()
{
	//try to disconnect gracefully
	if(qsocket->state() != QAbstractSocket::UnconnectedState)
	{
		qsocket->disconnectFromHost();
		qsocket->waitForDisconnected(2000);
	}
}

void SyncClient::connectToServer(const QString &host, const int port)
{
	if(qsocket->state() != QAbstractSocket::UnconnectedState)
	{
		qsocket->disconnectFromHost();
		if(qsocket->state() != QAbstractSocket::UnconnectedState && !qsocket->waitForDisconnected(5000))
		{
			qDebug()<<"[SyncClient] Error disconnecting, aborting socket:"<<qsocket->error();
			qsocket->abort();
		}
	}
	isConnecting = true;
	qDebug()<<"[SyncClient] Connecting to"<<(host + ":" + QString::number(port));
	qsocket->connectToHost(host,port);
	timeoutTimer->start(10000);
}

void SyncClient::timeoutOccurred()
{
	qDebug()<<"[SyncClient] Connect timeout occurred";
	disconnectFromServer();
	errorStr = "Connect timeout occurred";
	emit connectionError();
}

void SyncClient::disconnectFromServer()
{
	if(qsocket->state()!= QAbstractSocket::UnconnectedState)
	{
		qsocket->disconnectFromHost();
		if(qsocket->state() != QAbstractSocket::UnconnectedState && !qsocket->waitForDisconnected(5000))
		{
			qDebug()<<"[SyncClient] Error disconnecting, aborting socket:"<<qsocket->error();
			qsocket->abort();
		}
	}
	isConnecting = false;
}

bool SyncClient::isConnected() const
{
	return qsocket->state() == QAbstractSocket::ConnectedState;
}

void SyncClient::socketConnected()
{
	isConnecting = false;
	timeoutTimer->stop();
	qDebug()<<"[SyncClient] Connection successful";
	emit connected();
}

void SyncClient::socketDisconnected()
{
	qDebug()<<"[SyncClient] Disconnected from server";
	emit disconnected();
}

void SyncClient::socketError(QAbstractSocket::SocketError err)
{
	Q_UNUSED(err);
	errorStr = qsocket->errorString();
	qDebug()<<"[SyncClient] Socket error:"<<errorStr<<", connection state:"<<qsocket->state();

	if(isConnecting)
	{
		isConnecting = false;
		emit connectionError();
	}
}

void SyncClient::dataReceived()
{
	//a chunk of data is avaliable for reading
	qDebug()<<"[SyncClient] Received data:"<<qsocket->readAll();
}
