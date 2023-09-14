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

#ifndef SYNCCLIENTHANDLERS_HPP
#define SYNCCLIENTHANDLERS_HPP

#include "SyncProtocol.hpp"

#include <QRegularExpression>

class SyncClient;
class StelCore;

class ClientHandler : public QObject, public SyncMessageHandler
{
	Q_OBJECT
	Q_INTERFACES(SyncMessageHandler)

public:
	ClientHandler(SyncClient *client);
protected:
	SyncClient* client;
	StelCore* core;
};

class ClientErrorHandler : public ClientHandler
{
	Q_OBJECT
public:
	ClientErrorHandler(SyncClient* client);
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override;
};

//! Reacts to Server challenge and challenge OK on the client
class ClientAuthHandler : public ClientHandler
{
	Q_OBJECT
public:
	ClientAuthHandler(SyncClient* client);
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override;
signals:
	void authenticated();
};

class ClientAliveHandler : public SyncMessageHandler
{
public:
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override;
};

class ClientTimeHandler : public ClientHandler
{
public:
	ClientTimeHandler(SyncClient *client): ClientHandler(client){};
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override;
};

class ClientLocationHandler : public ClientHandler
{
public:
	ClientLocationHandler(SyncClient *client): ClientHandler(client){};
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override;
};

class StelObjectMgr;
class ClientSelectionHandler : public ClientHandler
{
public:
	ClientSelectionHandler(SyncClient *client);
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override;
private:
	StelObjectMgr* objMgr;
};

class StelPropertyMgr;
class ClientStelPropertyUpdateHandler : public ClientHandler
{
public:
	ClientStelPropertyUpdateHandler(SyncClient *client, bool skipGuiProps, const QStringList& excludeProps);
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override;
private:
	StelPropertyMgr* propMgr;
	QRegularExpression filter;
};

class StelMovementMgr;
class ClientViewHandler : public ClientHandler
{
public:
	ClientViewHandler(SyncClient *client);
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override;
private:
	StelMovementMgr* mvMgr;
};

#endif
