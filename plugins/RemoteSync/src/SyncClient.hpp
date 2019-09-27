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

#ifndef SYNCCLIENT_HPP
#define SYNCCLIENT_HPP

#include <QLoggingCategory>
#include <QObject>
#include <QTcpSocket>

Q_DECLARE_LOGGING_CATEGORY(syncClient)

class SyncMessageHandler;
class SyncRemotePeer;

//! A client which can connect to a SyncServer to receive state changes, and apply them
class SyncClient : public QObject
{
	Q_OBJECT
	Q_FLAGS(SyncOptions)
public:
	//! Bitflag-enum which determines the message types the client instance reacts to,
	//! and other boolean options
	enum SyncOption
	{
		NONE		= 0x0000,
		SyncTime	= 0x0001,
		SyncLocation	= 0x0002,
		SyncSelection	= 0x0004,
		SyncStelProperty= 0x0008,
		SyncView	= 0x0010,
		SyncFov		= 0x0020,
		SkipGUIProps	= 0x0040,
		ALL		= 0xFFFF
	};
	Q_DECLARE_FLAGS(SyncOptions, SyncOption)

	SyncClient(SyncOptions options, const QStringList& excludeProperties, QObject* parent = Q_NULLPTR);
	virtual ~SyncClient() Q_DECL_OVERRIDE;

	QString errorString() const { return errorStr; }

public slots:
	void connectToServer(const QString& host, const int port);
	void disconnectFromServer();

protected:
	void timerEvent(QTimerEvent* evt) Q_DECL_OVERRIDE;
signals:
	void connected();
	void disconnected(bool cleanExit);
private slots:
	void serverDisconnected(bool clean);
	void socketConnected();
	void emitServerError(const QString& errorStr);

private:
	void checkTimeout();

	SyncOptions options;
	QStringList stelPropFilter; // list of excluded properties
	QString errorStr;
	bool isConnecting;
	SyncRemotePeer* server;
	int timeoutTimerId;
	QVector<SyncMessageHandler*> handlerList;

	friend class ClientErrorHandler;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SyncClient::SyncOptions)

#endif
