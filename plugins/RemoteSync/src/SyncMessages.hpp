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

#ifndef SYNCMESSAGES_HPP_
#define SYNCMESSAGES_HPP_

#include "SyncProtocol.hpp"

namespace SyncProtocol
{

class ErrorMessage : public SyncMessage
{
public:
	ErrorMessage();
	ErrorMessage(const QString& msg);

	SyncMessageType getMessageType() const Q_DECL_OVERRIDE { return ERROR; }
	void serialize(QDataStream& stream) const Q_DECL_OVERRIDE;
	bool deserialize(QDataStream& stream, tPayloadSize dataSize) Q_DECL_OVERRIDE;

	QString message;
};

class ServerChallenge : public SyncMessage
{
public:
	//! Sets all values except clientId to the compile-time values
	ServerChallenge();

	SyncMessageType getMessageType() const Q_DECL_OVERRIDE { return SERVER_CHALLENGE; }
	void serialize(QDataStream &stream) const Q_DECL_OVERRIDE;
	bool deserialize(QDataStream &stream, tPayloadSize dataSize) Q_DECL_OVERRIDE;

	quint8 protocolVersion;
	quint32 remoteSyncVersion;
	quint32 stellariumVersion;
	QUuid clientId; //The ID the server has assigned to the client, working as a very basic challenge value
};

class ClientChallengeResponse : public SyncMessage
{
public:
	//! Sets all values except clientId to the compile-time values
	ClientChallengeResponse();

	SyncMessageType getMessageType() const Q_DECL_OVERRIDE { return CLIENT_CHALLENGE_RESPONSE; }
	void serialize(QDataStream &stream) const Q_DECL_OVERRIDE;
	bool deserialize(QDataStream &stream, tPayloadSize dataSize) Q_DECL_OVERRIDE;

	//basically the same as the challenge, without magic string and proto version
	quint32 remoteSyncVersion;
	quint32 stellariumVersion;
	QUuid clientId; //Must match the server challenge ID
};

//! This is just a notify message with no data, so no serialize/deserialize
class ServerChallengeResponseValid : public SyncMessage
{
public:
	SyncMessageType getMessageType() const Q_DECL_OVERRIDE { return SERVER_CHALLENGERESPONSEVALID; }
};

class Alive : public SyncMessage
{
public:
	SyncMessageType getMessageType() const Q_DECL_OVERRIDE  { return ALIVE; }
};


}

#endif
