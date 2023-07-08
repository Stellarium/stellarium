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

#include "SyncServerHandlers.hpp"
#include "SyncMessages.hpp"
#include "SyncServer.hpp"

using namespace SyncProtocol;

ServerHandler::ServerHandler(SyncServer *server)
	: server(server)
{
}

bool ServerErrorHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peerData)
{
	ErrorMessage msg;
	bool ok = msg.deserialize(stream,dataSize);
	peerData.peerLog(QtWarningMsg, "Received error message from client: " + msg.message);

	//we don't drop the connection here, we let the remote end do that
	return ok;
}


ServerAuthHandler::ServerAuthHandler(SyncServer* server, bool allowDivergingAppVersions)
	: ServerHandler(server), allowDivergingAppVersions(allowDivergingAppVersions)
{
}

bool ServerAuthHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	ClientChallengeResponse msg;
	bool ok = msg.deserialize(stream, dataSize);

	if(!ok)
	{
		peer.peerLog(QtCriticalMsg, "invalid challenge response received");
		return false;
	}

	//check if ID is valid
	if(msg.clientId != peer.id)
	{
		peer.peerLog(QtCriticalMsg, "invalid ID in challenge response");
		peer.writeError("invalid id sent");
		return true;
	}

	//check version
	const quint32 expectedPluginVersion = (REMOTESYNC_MAJOR << 16) | (REMOTESYNC_MINOR << 8) | (REMOTESYNC_PATCH);
	const quint32 expectedStellariumVersion =(STELLARIUM_MAJOR << 16) | (STELLARIUM_MINOR<<8) | (STELLARIUM_PATCH);

	if(expectedPluginVersion != msg.remoteSyncVersion)
	{
		QString str("RemoteSync plugin version mismatch! Expected: 0x%1, Got: 0x%2");
		peer.peerLog(QtWarningMsg, str.arg(expectedPluginVersion,0,16).arg(msg.remoteSyncVersion,0,16));
		if(!allowDivergingAppVersions)
		{
			peer.writeError("plugin version mismatch not allowed");
			return true;
		}
	}
	if(expectedStellariumVersion != msg.stellariumVersion)
	{
		//This is only a warning here
		QString str("Stellarium version mismatch! Expected: 0x%1, Got: 0x%2");
		peer.peerLog(QtWarningMsg, str.arg(expectedStellariumVersion,0,16).arg(msg.stellariumVersion,0,16));
		if(!allowDivergingAppVersions)
		{
			peer.writeError("app version mismatch not allowed");
			return true;
		}
	}

	peer.peerLog(QtDebugMsg, "Authenticated client");

	//if we got here, peer is successfully authenticated!
	peer.authenticated = true;
	peer.writeMessage(ServerChallengeResponseValid());
	server->clientAuthenticated(peer);

	return true;
}

bool ServerAliveHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	Q_UNUSED(peer)
	Alive p;
	return p.deserialize(stream,dataSize);
}
