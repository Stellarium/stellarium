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

#ifndef SYNCMESSAGES_HPP
#define SYNCMESSAGES_HPP

#include "SyncProtocol.hpp"
#include "StelLocation.hpp"
#include "VecMath.hpp"

namespace SyncProtocol
{

class ErrorMessage : public SyncMessage
{
public:
	ErrorMessage();
	ErrorMessage(const QString& msg);

	SyncProtocol::SyncMessageType getMessageType() const override { return SyncProtocol::SYNC_ERROR; }
	void serialize(QDataStream& stream) const override;
	bool deserialize(QDataStream& stream, SyncProtocol::tPayloadSize dataSize) override;

	QString message;
};

class ServerChallenge : public SyncMessage
{
public:
	//! Sets all values except clientId to the compile-time values
	ServerChallenge();

	SyncProtocol::SyncMessageType getMessageType() const override { return SyncProtocol::SERVER_CHALLENGE; }
	void serialize(QDataStream &stream) const override;
	bool deserialize(QDataStream &stream, SyncProtocol::tPayloadSize dataSize) override;

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

	SyncProtocol::SyncMessageType getMessageType() const override { return SyncProtocol::CLIENT_CHALLENGE_RESPONSE; }
	void serialize(QDataStream &stream) const override;
	bool deserialize(QDataStream &stream, SyncProtocol::tPayloadSize dataSize) override;

	//basically the same as the challenge, without magic string and proto version
	quint32 remoteSyncVersion;
	quint32 stellariumVersion;
	QUuid clientId; //Must match the server challenge ID
};

//! This is just a notify message with no data, so no serialize/deserialize
class ServerChallengeResponseValid : public SyncMessage
{
public:
	SyncProtocol::SyncMessageType getMessageType() const override { return SyncProtocol::SERVER_CHALLENGERESPONSEVALID; }
};

class Time : public SyncMessage
{
public:
	SyncProtocol::SyncMessageType getMessageType() const override { return SyncProtocol::TIME; }

	void serialize(QDataStream &stream) const override;
	bool deserialize(QDataStream &stream, SyncProtocol::tPayloadSize dataSize) override;

	//TODO implement network delay compensation (also for other message types where it makes sense)
	//TODO maybe split up so that each message is only for 1 thing?
	qint64 lastTimeSyncTime; //corresponds to StelCore::milliSecondsOfLastJDayUpdate
	double jDay; //current jDay, without any time zone/deltaT adjustments
	double timeRate; //current time rate
};

class Location : public SyncMessage
{
public:
	Location();

	SyncProtocol::SyncMessageType getMessageType() const override { return SyncProtocol::LOCATION; }

	void serialize(QDataStream &stream) const override;
	bool deserialize(QDataStream &stream, SyncProtocol::tPayloadSize dataSize) override;

	StelLocation stelLocation;
	double totalDuration;
	double timeToGo;
};

class Selection : public SyncMessage
{
public:
	SyncProtocol::SyncMessageType getMessageType() const override { return SyncProtocol::SELECTION; }

	void serialize(QDataStream &stream) const override;
	bool deserialize(QDataStream &stream, SyncProtocol::tPayloadSize dataSize) override;

	QString toString() const override
	{
		QString s("{");
		foreach (auto &o, selectedObjects)
		{
			s.append(QString("[%1, %2], ").arg(o.first, o.second));
		}
		s.chop(2); // remove last comma/space
		s.append("}");
		return s;
	}

	//list of type/ID pairs
	QList< QPair<QString,QString> > selectedObjects;
};

class Alive : public SyncMessage
{
public:
	SyncProtocol::SyncMessageType getMessageType() const override  { return SyncProtocol::ALIVE; }
};

class StelPropertyUpdate : public SyncMessage
{
public:
	SyncMessageType getMessageType() const override { return SyncProtocol::STELPROPERTY; }

	void serialize(QDataStream &stream) const override;
	bool deserialize(QDataStream &stream, SyncProtocol::tPayloadSize dataSize) override;

	QString toString() const override
	{
		return 	QString("[%1, %2]").arg(propId, value.toString());
	}
	QString propId;
	QVariant value;
};

class View : public SyncMessage
{
public:
	SyncMessageType getMessageType() const override { return SyncProtocol::VIEW; }

	void serialize(QDataStream& stream) const override;
	bool deserialize(QDataStream &stream, tPayloadSize dataSize) override;

	Vec3d viewAltAz;
};

}

#endif
