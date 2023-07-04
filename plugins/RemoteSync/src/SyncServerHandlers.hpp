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

#ifndef SYNCSERVERHANDLERS_HPP
#define SYNCSERVERHANDLERS_HPP

#include "SyncProtocol.hpp"

class SyncServer;

class ServerHandler : public QObject, public SyncMessageHandler
{
	Q_OBJECT
	Q_INTERFACES(SyncMessageHandler)

public:
	ServerHandler(SyncServer *server);
protected:
	SyncServer* server;
};

class ServerErrorHandler : public SyncMessageHandler
{
public:
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peerData) override;
};

//! Server-side auth handler
class ServerAuthHandler : public ServerHandler
{
	Q_OBJECT
public:
	ServerAuthHandler(SyncServer* server, bool allowDivergingAppVersions);
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override;
private:
	bool allowDivergingAppVersions;
};

class ServerAliveHandler : public SyncMessageHandler
{
public:
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override;
};

#endif
