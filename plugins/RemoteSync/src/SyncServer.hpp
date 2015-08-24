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

#ifndef SYNCSERVER_HPP_
#define SYNCSERVER_HPP_

#include "SyncProtocol.hpp"
#include <QObject>
#include <QAbstractSocket>
#include <QDateTime>
#include <QUuid>

class QTcpServer;

//! Implements a server to which SyncClients can connect and receive state changes
class SyncServer : public QObject
{
	Q_OBJECT

public:
	SyncServer(QObject* parent = 0);
	virtual ~SyncServer();

	//! Broadcasts this message to all connected and authenticated clients
	void broadcastMessage(const SyncProtocol::SyncMessage& msg);
public slots:
	//! Starts the SyncServer on the specified port. If the server is already running, stops it first.
	//! Returns true if successful (false usually means port was in use, use getErrorString)
	bool start(int port);
	//! Disconnects all clients, and stops listening
	void stop();
	//! Returns a string of the last server error.
	QString errorString() const;

protected:
	void timerEvent(QTimerEvent* evt) Q_DECL_OVERRIDE;
private slots:
	void handleNewConnection();
	void connectionError(QAbstractSocket::SocketError err);

	//! On receipt of new client data
	void clientDataReceived();
	void clientError(QAbstractSocket::SocketError);
	void clientDisconnected();

private:
	void checkTimeouts();
	void clientLog(QAbstractSocket* cl, const QString& msg);
	//use composition instead of inheritance, cleaner interfaace this way
	//for now, we use TCP, but will test multicast UDP later if the basic setup is working
	QTcpServer* qserver;
	QVector<SyncProtocol::SyncMessageHandler*> handlerList;

	// a map to associate sockets with the client data
	typedef QMap<QAbstractSocket*,SyncProtocol::SyncRemotePeer> tClientMap;
	tClientMap clients;

	int timeoutTimerId;
};

#endif
