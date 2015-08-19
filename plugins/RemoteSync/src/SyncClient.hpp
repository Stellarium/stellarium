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

#ifndef SYNCCLIENT_HPP_
#define SYNCCLIENT_HPP_

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

//! A client which can connect to a SyncServer to receive state changes, and apply them
class SyncClient : public QObject
{
	Q_OBJECT
public:
	SyncClient(QObject* parent = 0);
	virtual ~SyncClient();

	//! True if the connection has been established completely
	bool isConnected() const;
	QString errorString() const { return errorStr; }

public slots:
	void connectToServer(const QString& host, const int port);
	void disconnectFromServer();
signals:
	void connected();
	void disconnected();
	void connectionError();
private slots:
	void dataReceived();
	void timeoutOccurred();
	void socketConnected();
	void socketDisconnected();
	void socketError(QAbstractSocket::SocketError err);
private:
	QString errorStr;
	bool isConnecting;
	QTcpSocket* qsocket;
	QTimer* timeoutTimer;
};


#endif
