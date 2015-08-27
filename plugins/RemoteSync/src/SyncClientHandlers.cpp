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

#include "SyncClientHandlers.hpp"
#include "SyncClient.hpp"

#include "SyncMessages.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTranslator.hpp"

using namespace SyncProtocol;

ClientHandler::ClientHandler(SyncClient *client)
	: client(client)
{
	Q_ASSERT(client);
}


ClientErrorHandler::ClientErrorHandler(SyncClient *client)
	: ClientHandler(client)
{

}

bool ClientErrorHandler::handleMessage(QDataStream &stream, SyncRemotePeer &peer)
{
	ErrorMessage msg;
	bool ok = msg.deserialize(stream,peer.msgHeader.dataSize);
	peer.peerLog("Received error message from server: " + msg.message);

	client->emitError(msg.message);

	//we don't drop the connection here, we let the remote end do that
	return ok;
}

ClientAuthHandler::ClientAuthHandler(SyncClient *client)
	: ClientHandler(client)
{
	connect(this, SIGNAL(authenticated()),client,SIGNAL(connected()));
}


bool ClientAuthHandler::handleMessage(QDataStream &stream, SyncRemotePeer &peer)
{
	//get message type
	SyncMessageType type = SyncMessageType(peer.msgHeader.msgType);

	if(type == SERVER_CHALLENGE)
	{
		if(peer.isAuthenticated)
		{
			//we are already authenticated, another challenge is an error
			qWarning()<<"[SyncClient] received server challenge when not expecting one";
			return false;
		}

		ServerChallenge msg;
		bool ok = msg.deserialize(stream,peer.msgHeader.dataSize);

		if(!ok)
		{
			qWarning()<<"[SyncClient] invalid server challenge received";
			return false;
		}

		//check challenge for validity
		if(msg.protocolVersion != SYNC_PROTOCOL_VERSION)
		{
			qWarning()<<"[SyncClient] invalid protocol version, dropping connection";
			return false;
		}

		const quint32 expectedPluginVersion = (REMOTESYNC_MAJOR << 16) | (REMOTESYNC_MINOR << 8) | (REMOTESYNC_PATCH);
		const quint32 expectedStellariumVersion =(STELLARIUM_MAJOR << 16) | (STELLARIUM_MINOR<<8) | (STELLARIUM_PATCH);

		if(expectedPluginVersion != msg.remoteSyncVersion)
		{
			//This is only a warning here
			QString str("[SyncClient] RemoteSync plugin version mismatch! Expected: 0x%1, Got: 0x%2");
			qWarning()<<str.arg(expectedPluginVersion,0,16).arg(msg.remoteSyncVersion,0,16);
		}
		if(expectedStellariumVersion != msg.stellariumVersion)
		{
			//This is only a warning here
			QString str("[SyncClient] Stellarium version mismatch! Expected: 0x%1, Got: 0x%2");
			qWarning()<<str.arg(expectedStellariumVersion,0,16).arg(msg.stellariumVersion,0,16);
		}

		qDebug()<<"[SyncClient] Received server challenge, sending response";

		//we have to answer with the response
		ClientChallengeResponse response;
		//only need to set this
		response.clientId = msg.clientId;

		peer.authResponseSent = true;
		peer.writeMessage(response);

		return true;
	}
	else if (type == SERVER_CHALLENGERESPONSEVALID)
	{
		//this message has no data body, no need to deserialize
		if(peer.authResponseSent)
		{
			//we authenticated correctly, yay!
			peer.isAuthenticated = true;
			qDebug()<<"[SyncClient] Connection authenticated";
			emit authenticated();
			return true;
		}
		else
		{
			//we got a confirmation without sending a response, error
			qWarning()<<"[SyncClient] Got SERVER_CHALLENGERESPONSEVALID message without awaiting it";
			return false;
		}
	}
	else
	{
		//should never happen except the message type<-->handler config was somehow messed up
		Q_ASSERT(false);
		return false;
	}
}

bool ClientAliveHandler::handleMessage(QDataStream &stream, SyncRemotePeer &peer)
{
	Alive p;
	return p.deserialize(stream,peer.msgHeader.dataSize);
}

ClientTimeHandler::ClientTimeHandler()
{
	core = StelApp::getInstance().getCore();
}

bool ClientTimeHandler::handleMessage(QDataStream &stream, SyncRemotePeer &peer)
{
	Time msg;
	bool ok = msg.deserialize(stream,peer.msgHeader.dataSize);

	if(!ok)
		return false;

	//set time variables, time rate first because it causes a resetSync which we overwrite
	core->setTimeRate(msg.timeRate);
	core->setJDayOfLastJDayUpdate(msg.jDay);
	core->setMilliSecondsOfLastJDayUpdate(msg.lastTimeSyncTime);

	return true;
}
