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

#include "SyncMessages.hpp"

using namespace SyncProtocol;

ErrorMessage::ErrorMessage()
{
}

ErrorMessage::ErrorMessage(const QString &msg)
	: message(msg)
{
}

void ErrorMessage::serialize(QDataStream &stream) const
{
	writeString(stream,message);
}

bool ErrorMessage::deserialize(QDataStream &stream, tPayloadSize dataSize)
{
	Q_UNUSED(dataSize)
	message = readString(stream);
	return !stream.status();
}

ServerChallenge::ServerChallenge()
	: protocolVersion(SYNC_PROTOCOL_VERSION),
	  remoteSyncVersion((REMOTESYNC_MAJOR<<16) | (REMOTESYNC_MINOR<<8) | REMOTESYNC_PATCH),
	  stellariumVersion((STELLARIUM_MAJOR<<16) | (STELLARIUM_MINOR<<8) | STELLARIUM_PATCH)
{
}

void ServerChallenge::serialize(QDataStream &stream) const
{
	//write the MAGIC value directly without encoding
	stream.writeRawData(SYNC_MAGIC_VALUE.constData(),SYNC_MAGIC_VALUE.size());
	//write protocol version, plugin + stellarium version
	stream<<protocolVersion;
	stream<<remoteSyncVersion;
	stream<<stellariumVersion;

	//write the client ID
	stream<<clientId;
}

bool ServerChallenge::deserialize(QDataStream &stream, tPayloadSize dataSize)
{
	//check if the size is what we expect
	if(dataSize != (SYNC_MAGIC_VALUE.size() + 9 + 16))
		return false;

	QByteArray data(SYNC_MAGIC_VALUE.size(),'\0');
	stream.readRawData(data.data(),data.size());

	//check if magic value matches
	if(data!=SYNC_MAGIC_VALUE)
	{
		qCWarning(syncProtocol)<<"[ServerHello] invalid magic value";
		return false;
	}

	stream>>protocolVersion;
	stream>>remoteSyncVersion;
	stream>>stellariumVersion;

	stream>>clientId;

	return !stream.status();
}

ClientChallengeResponse::ClientChallengeResponse()
	: remoteSyncVersion((REMOTESYNC_MAJOR<<16) | (REMOTESYNC_MINOR<<8) | REMOTESYNC_PATCH),
	  stellariumVersion((STELLARIUM_MAJOR<<16) | (STELLARIUM_MINOR<<8) | STELLARIUM_PATCH)
{
}

void ClientChallengeResponse::serialize(QDataStream &stream) const
{
	stream<<remoteSyncVersion;
	stream<<stellariumVersion;
	stream<<clientId;
}

bool ClientChallengeResponse::deserialize(QDataStream &stream, tPayloadSize dataSize)
{
	//check if the size is what we expect
	if(dataSize != (8 + 16))
		return false;

	stream>>remoteSyncVersion;
	stream>>stellariumVersion;
	stream>>clientId;

	return !stream.status();
}

void Time::serialize(QDataStream &stream) const
{
	stream<<lastTimeSyncTime;
	stream<<jDay;
	stream<<timeRate;
}

bool Time::deserialize(QDataStream &stream, tPayloadSize dataSize)
{
	if(dataSize != 24)
		return false;

	stream>>lastTimeSyncTime;
	stream>>jDay;
	stream>>timeRate;

	return !stream.status();
}

Location::Location()
	: totalDuration(0.0),timeToGo(0.0)
{
}

void Location::serialize(QDataStream &stream) const
{
	//! Serialize StelLocation directly
	//! Note: this introduces dependency on Stellarium version!
	stream<<stelLocation;
	stream<<totalDuration;
	stream<<timeToGo;
}

bool Location::deserialize(QDataStream &stream, tPayloadSize dataSize)
{
	Q_UNUSED(dataSize)
	stream>>stelLocation;
	stream>>totalDuration;
	stream>>timeToGo;
	return !stream.status();
}

void Selection::serialize(QDataStream &stream) const
{
	stream<<selectedObjects;
}

bool Selection::deserialize(QDataStream &stream, tPayloadSize dataSize)
{
	Q_UNUSED(dataSize)
	stream>>selectedObjects;
	return !stream.status();
}

void StelPropertyUpdate::serialize(QDataStream &stream) const
{
	writeString(stream,propId);
	stream<<value;
}

bool StelPropertyUpdate::deserialize(QDataStream &stream, tPayloadSize dataSize)
{
	Q_UNUSED(dataSize)
	propId = readString(stream);
	stream>>value;
	return !stream.status();
}

void View::serialize(QDataStream &stream) const
{
	stream<<viewAltAz;
}

bool View::deserialize(QDataStream &stream, tPayloadSize dataSize)
{
	if(dataSize != 3 * sizeof(double))
		return false;

	stream>>viewAltAz;

	return !stream.status();
}
