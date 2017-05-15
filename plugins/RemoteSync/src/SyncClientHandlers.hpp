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

#ifndef SYNCCLIENTHANDLERS_HPP_
#define SYNCCLIENTHANDLERS_HPP_

#include "SyncProtocol.hpp"

class SyncClient;
class StelCore;

class ClientHandler : public QObject, public SyncMessageHandler
{
	Q_OBJECT
	Q_INTERFACES(SyncMessageHandler)

public:
	ClientHandler();
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
	bool handleMessage(QDataStream &stream, SyncRemotePeer &peer) Q_DECL_OVERRIDE;
};

//! Reacts to Server challenge and challenge OK on the client
class ClientAuthHandler : public ClientHandler
{
	Q_OBJECT
public:
	ClientAuthHandler(SyncClient* client);
	bool handleMessage(QDataStream &stream, SyncRemotePeer &peer) Q_DECL_OVERRIDE;
signals:
	void authenticated();
};

class ClientAliveHandler : public SyncMessageHandler
{
public:
	bool handleMessage(QDataStream &stream, SyncRemotePeer &peer) Q_DECL_OVERRIDE;
};

class ClientTimeHandler : public ClientHandler
{
public:
	bool handleMessage(QDataStream &stream, SyncRemotePeer &peer) Q_DECL_OVERRIDE;
};

class ClientLocationHandler : public ClientHandler
{
public:
	bool handleMessage(QDataStream &stream, SyncRemotePeer &peer) Q_DECL_OVERRIDE;
};

class StelObjectMgr;

class ClientSelectionHandler : public ClientHandler
{
public:
	ClientSelectionHandler();
	bool handleMessage(QDataStream &stream, SyncRemotePeer &peer) Q_DECL_OVERRIDE;
private:
	StelObjectMgr* objMgr;
};

#endif
