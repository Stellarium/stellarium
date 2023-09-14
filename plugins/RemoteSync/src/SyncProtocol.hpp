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

#ifndef SYNCPROTOCOL_HPP
#define SYNCPROTOCOL_HPP

#include <QLoggingCategory>
#include <QByteArray>
#include <QDataStream>
#include <QAbstractSocket>
#include <QUuid>

Q_DECLARE_LOGGING_CATEGORY(syncProtocol)

//! Contains sync protocol data definitions shared between client and server
namespace SyncProtocol
{

//Important: All data should use the sized typedefs provided by Qt (i.e. qint32 instead of 4 byte int on x86)

//! Should be changed with every breaking change
const quint8 SYNC_PROTOCOL_VERSION = 2;
const QDataStream::Version SYNC_DATASTREAM_VERSION = QDataStream::Qt_5_0;
//! Magic value for protocol used during connection. Should NEVER change.
const QByteArray SYNC_MAGIC_VALUE = "StellariumSyncPluginProtocol";

typedef quint16 tPayloadSize;

//! All messages are preceded by this
struct SyncHeader
{
	quint8 msgType; //The SyncMessageType of the data
	SyncProtocol::tPayloadSize dataSize; //The size of the data part
};

//! Write a SyncHeader to a DataStream
QDataStream& operator<<(QDataStream& out, const SyncHeader& header);
//! Read a SyncHeader from a DataStream
QDataStream& operator>>(QDataStream& in, SyncHeader& header);

const qint64 SYNC_HEADER_SIZE = sizeof(quint8) + sizeof(tPayloadSize); //3 byte
const qint64 SYNC_MAX_PAYLOAD_SIZE = (2<<15) - 1; // 65535
const qint64 SYNC_MAX_MESSAGE_SIZE = SYNC_HEADER_SIZE + SYNC_MAX_PAYLOAD_SIZE;

//! Contains the possible message types. The enum value is used as an ID to identify the message type over the network.
//! The classes handling these messages are defined in SyncMessages.hpp
enum SyncMessageType
{
	SYNC_ERROR, //sent to the other party on protocol/auth error with a message, connection will be dropped afterwards
	SERVER_CHALLENGE, //sent as a challenge to the client on establishment of connection
	CLIENT_CHALLENGE_RESPONSE, //sent as a reply to the challenge
	SERVER_CHALLENGERESPONSEVALID, //sent from the server to the client after valid client hello was received.
	ALIVE, //sent from a peer after no data was sent for about 5 seconds to indicate it is still alive

	//all messages below here can only be sent from authenticated peers
	TIME, //time jumps + time scale updates
	LOCATION, //location changes
	SELECTION, //current selection changed
	STELPROPERTY, //stelproperty updates
	VIEW, //view change

	MSGTYPE_MAX = VIEW,
	MSGTYPE_SIZE = MSGTYPE_MAX+1
};

//! Base interface for the messages themselves, allowing to serialize/deserialize them
class SyncMessage
{
public:
	virtual ~SyncMessage() {}

	//! Subclasses must return the message type this message represents
	virtual SyncProtocol::SyncMessageType getMessageType() const = 0;

	//! Writes a full message (with header) to the specified byte array.
	//! Returns the total message size. If zero, the payload is too large for a SyncMessage.
	qint64 createFullMessage(QByteArray& target) const;

	//! Subclasses should override this to serialize their contents to the data stream.
	//! The default implementation writes nothing.
	virtual void serialize(QDataStream& stream) const;
	//! Subclasses should override this to load their contents from the data stream.
	//! The default implementation expects a zero dataSize, and reads nothing.
	virtual bool deserialize(QDataStream& stream, SyncProtocol::tPayloadSize dataSize);

	//! Subclasses can override this to provide proper debug output.
	//! The default just prints the message type.
	virtual QString toString() const;
	//! Print enum name for SyncMessageType type.
	static QString toString(SyncMessageType type);

protected:
	static void writeString(QDataStream& stream, const QString& str);
	static QString readString(QDataStream& stream);
};

}

class SyncMessageHandler;

//! Handling the connection to a remote peer (i.e. all clients on the server, and the server on the client)
class SyncRemotePeer : public QObject
{
	Q_OBJECT
public:
	SyncRemotePeer(QAbstractSocket* socket, bool isServer, const QHash<SyncProtocol::SyncMessageType, SyncMessageHandler*> &handlerHash);
	~SyncRemotePeer() override;

	//! Sends a message to this peer
	void writeMessage(const SyncProtocol::SyncMessage& msg);
	//! Writes this data packet to the socket without processing
	void writeData(const QByteArray& data, qint64 size=-1);
	//! Can be used to write an error message to the peer and drop the connection
	void writeError(const QString& err);

	//! Log a message for this peer via qCDebug or the respective other messages
	//! type={QtDebugMsg|QtInfoMsg|QtWarningMsg|QtCriticalMsg|QtFatalMsg}
	//! If in doubt, use QtDebugMsg
	void peerLog(const QtMsgType type, const QString& msg) const;

	bool isAuthenticated() const { return authenticated; }
	QUuid getID() const { return id; }

	void checkTimeout();
	void disconnectPeer();

	QString getError() const { return errorString; }
signals:
	void disconnected(bool cleanDisconnect);
private slots:
	void sockDisconnected();
	void sockError(QAbstractSocket::SocketError err);
	void sockStateChanged(QAbstractSocket::SocketState state);

	//! Call this to try to read message data from the socket.
	//! If a fully formed message is currently buffered, it is processed.
	void receiveMessage();
private:
	QAbstractSocket* sock; // The socket for communication with this peer
	QDataStream stream;
	QString errorString;
	bool expectDisconnect;
	bool isPeerAServer; // True if this identifies a server
	QUuid id; // An ID value, currently not used for anything else than auth. The server always has a NULL UUID.
	bool authenticated; // True if the peer ran through the HELLO process and can receive/send all message types
	bool authResponseSent; //only for client use, tracks if the client has sent a response to the server challenge
	bool waitingForBody; //True if waiting for full message body (after header was received)
	SyncProtocol::SyncHeader msgHeader; //the last message header read/currently being processed
	qint64 lastReceiveTime; // The time the last data of this peer was received
	qint64 lastSendTime; //The time the last data was written to this peer
	QHash<SyncProtocol::SyncMessageType, SyncMessageHandler*> handlerHash;

	QByteArray msgWriteBuffer; //Byte array used to construct messages before writing them

	friend class ServerAuthHandler;
	friend class ClientAuthHandler;
};

//! Base interface for message handlers, i.e. reacting to messages
class SyncMessageHandler
{
public:
	virtual ~SyncMessageHandler() {}

	//! Read a message directly from the stream. SyncMessage::deserialize of the correct class should be used to deserialize the message.
	//! @param stream The stream to be used to read data. The current position is after the header.
        //! @param dataSize The data size from the message header
	//! @param peer The remote peer this message originated from. Can be used to send replies through SyncRemotePeer::writeMessage
	//! @return return false if the message is found to be invalid. The connection to the client/server will be dropped.
	virtual bool handleMessage(QDataStream& stream, SyncProtocol::tPayloadSize dataSize,  SyncRemotePeer& peer) = 0;
};

//! Skips the message, and does nothing else.
class DummyMessageHandler : public SyncMessageHandler
{
public:
	bool handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer) override
	{
		Q_UNUSED(peer)
		stream.skipRawData(dataSize);
		return !stream.status();
	}
};

Q_DECLARE_INTERFACE(SyncMessageHandler,"Stellarium/RemoteSync/SyncMessageHandler/1.0")


#endif
