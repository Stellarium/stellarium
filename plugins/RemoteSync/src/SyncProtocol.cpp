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

#include "SyncProtocol.hpp"
#include "SyncMessages.hpp"

#include <QDateTime>
#include <QHostAddress>
#include <QVector>

Q_LOGGING_CATEGORY(syncProtocol,"stel.plugin.remoteSync.protocol")

using namespace SyncProtocol;

namespace SyncProtocol
{

QDataStream& operator<<(QDataStream& out, const SyncHeader& header)
{
	out<<header.msgType;
	out<<header.dataSize;
	return out;
}

QDataStream& operator>>(QDataStream& in, SyncHeader& header)
{
	in>>header.msgType;
	in>>header.dataSize;
	return in;
}

}

qint64 SyncMessage::createFullMessage(QByteArray &target) const
{
	//we serialize into a byte buffer first so that we can get message size easily
	QDataStream tmpStream(&target, QIODevice::WriteOnly);
	tmpStream.setVersion(SYNC_DATASTREAM_VERSION);

	//write the payload after the header first
	tmpStream.device()->seek(SYNC_HEADER_SIZE);
	serialize(tmpStream);

	//Important: we do not rely on the buffer's size to find the actual amount of data written
	//because that may depend on previous messages.
	//Because the msgWriteBuffer is persistently kept to avoid unnecessary allocations, we use the current pos for that
	qint64 totalSize = tmpStream.device()->pos();
	qint64 writtenSize = totalSize - SYNC_HEADER_SIZE;

	if(writtenSize > SYNC_MAX_PAYLOAD_SIZE)
	{
		//crash here when message is too large in debugging
		Q_ASSERT(true);
		return 0;
	}
	else
	{
		//write header in front
		SyncHeader header = { static_cast<quint8>(getMessageType()), static_cast<tPayloadSize>(writtenSize) };
		tmpStream.device()->seek(0);
		tmpStream<<header;

		return totalSize;
	}
}

void SyncMessage::serialize(QDataStream &stream) const
{
	Q_UNUSED(stream)
}

bool SyncMessage::deserialize(QDataStream &stream, tPayloadSize dataSize)
{
	Q_UNUSED(stream)
	return dataSize == 0;
}

void SyncMessage::writeString(QDataStream &stream, const QString &str)
{
	stream<<str.toUtf8();
}

QString SyncMessage::readString(QDataStream &stream)
{
	QByteArray arr;
	stream>>arr;
	return QString::fromUtf8(arr);
}

SyncRemotePeer::SyncRemotePeer(QAbstractSocket *socket, bool isServer, const QHash<SyncMessageType, SyncMessageHandler *> &handlerHash)
	: sock(socket), stream(sock), expectDisconnect(false), isPeerAServer(isServer), authenticated(false), authResponseSent(false), waitingForBody(false),
	  handlerHash(handlerHash)
{
	Q_ASSERT(sock);
	sock->setParent(this); //reparent
	sock->setSocketOption(QAbstractSocket::LowDelayOption, 1);
	stream.setVersion(SYNC_DATASTREAM_VERSION);
	connect(sock, SIGNAL(readyRead()), this, SLOT(receiveMessage()));
	connect(sock, SIGNAL(disconnected()), this, SLOT(sockDisconnected()));
#if (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
	connect(sock, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(sockError(QAbstractSocket::SocketError)));
#else
	connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(sockError(QAbstractSocket::SocketError)));
#endif
	connect(sock, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(sockStateChanged(QAbstractSocket::SocketState)));

	// silence CoverityScan...
        msgHeader.msgType=SyncProtocol::SYNC_ERROR;
	msgHeader.dataSize=0;

	lastReceiveTime = QDateTime::currentMSecsSinceEpoch();
	lastSendTime = lastReceiveTime;
	msgWriteBuffer.reserve(SYNC_MAX_MESSAGE_SIZE);

	if(!isServer)
		id = QUuid::createUuid();
}

SyncRemotePeer::~SyncRemotePeer()
{
	peerLog(QtDebugMsg, "Destroyed");
	delete sock;
}

void SyncRemotePeer::checkTimeout()
{
	if(sock->state() == QAbstractSocket::UnconnectedState || sock->state() == QAbstractSocket::ClosingState)
		return;

	qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
	qint64 writeDiff = currentTime - lastSendTime;
	qint64 readDiff = currentTime - lastReceiveTime;

	if(writeDiff > 5000 && authenticated) //only send ALIVE to authenticated peers
	{
		//no data sent to this peer for some time, send a ALIVE
		Alive msg;
		writeMessage(msg);
	}

	if(readDiff > 15000)
	{
		//no data received for some time, assume client timed out
		peerLog(QtWarningMsg, QString("No data received for %1ms, timing out").arg(readDiff));
		errorString = "Connection timed out";

		if(sock->state()==QAbstractSocket::ConnectedState)
			sock->disconnectFromHost();
		else
		{
			sock->abort();
			sockDisconnected();
		}
	}
}

void SyncRemotePeer::disconnectPeer()
{
	expectDisconnect = true;
	if(sock->state()==QAbstractSocket::ConnectedState)
		sock->disconnectFromHost();
	else if(sock->state()!=QAbstractSocket::UnconnectedState)
	{
		sock->abort();
		sockDisconnected();
	}
}

void SyncRemotePeer::sockDisconnected()
{
	peerLog(QtDebugMsg, "Socket disconnected");
	emit disconnected(expectDisconnect);
}

void SyncRemotePeer::sockError(QAbstractSocket::SocketError err)
{
	errorString = sock->errorString();
	peerLog(QtWarningMsg, "Socket error:" + errorString);

	if(err == QAbstractSocket::RemoteHostClosedError) //handle remote close as normal disconnect
		expectDisconnect = true;

	if(sock->state()==QAbstractSocket::ConnectedState) // it is still connected, wait for automatic disconnect
		sock->disconnectFromHost();
	else if(sock->state()==QAbstractSocket::UnconnectedState) //in this case, we have to emit the signal manually
		sockDisconnected();
}

void SyncRemotePeer::sockStateChanged(QAbstractSocket::SocketState state)
{
	peerLog(QtDebugMsg, "Socket state:" + QVariant::fromValue(state).toString());
}

void SyncRemotePeer::receiveMessage()
{
	//to debug read buffer contents, uncomment
	//QByteArray peekData = sock->peek(SYNC_MAX_MESSAGE_SIZE);

	lastReceiveTime = QDateTime::currentMSecsSinceEpoch();

	//This loop is required to make sure all pending data is read (i.e. multiple messages may be queued up)
	while(sock->bytesAvailable()>0)
	{
		if(!waitingForBody)
		{
			//we use the socket's read buffer to wait until a full packet is available
			if(sock->bytesAvailable() < SYNC_HEADER_SIZE)
				return;

			stream>>msgHeader;
			//check if msgtype is valid
			if(msgHeader.msgType>MSGTYPE_MAX)
			{
				writeError("invalid message type " + QString::number(msgHeader.msgType));
				return;
			}
			if(!authenticated && msgHeader.msgType > SERVER_CHALLENGERESPONSEVALID)
			{
				//if not fully authenticated, it is an error to send messages other than auth messages
				writeError("not authenticated");
				return;
			}

			peerLog(QtDebugMsg, "received header for " + SyncMessage::toString(SyncMessageType(msgHeader.msgType)));
		}

		if(sock->bytesAvailable() < msgHeader.dataSize)
		{
			waitingForBody = true;
			return;
		}
		else
		{
			waitingForBody = false;
			peerLog(QtDebugMsg, "received body, processing");

			//full packet available, pass to handler
			SyncMessageHandler* handler = handlerHash[static_cast<SyncMessageType>(msgHeader.msgType)];
			if(!handler)
			{
				//no handler registered on this end for this msgtype
				writeError("unregistered message type " + QString::number(msgHeader.msgType));
				return;
			}
			if(!handlerHash[static_cast<SyncMessageType>(msgHeader.msgType)]->handleMessage(stream, msgHeader.dataSize, *this))
			{
				writeError("last message of type " + QString::number(msgHeader.msgType) + " was rejected");
			}
		}
	}
}

void SyncRemotePeer::peerLog(const QtMsgType type, const QString &msg) const
{
	switch (type)
	{
		case QtInfoMsg:
			qCInfo(syncProtocol)<<"[Peer"<<(sock->peerAddress().toString() + ":" + QString::number(sock->peerPort()))<<"]:" << msg;
			break;
		case QtWarningMsg:
			qCWarning(syncProtocol)<<"[Peer"<<(sock->peerAddress().toString() + ":" + QString::number(sock->peerPort()))<<"]:" << msg;
			break;
		case QtCriticalMsg:
			qCCritical(syncProtocol)<<"[Peer"<<(sock->peerAddress().toString() + ":" + QString::number(sock->peerPort()))<<"]:" << msg;
			break;
#if (QT_VERSION>=QT_VERSION_CHECK(6,5,0))
		case QtFatalMsg:
			qCFatal(syncProtocol)<<"[Peer"<<(sock->peerAddress().toString() + ":" + QString::number(sock->peerPort()))<<"]:" << msg;
			break;
#else
		case QtFatalMsg:
#endif
		case QtDebugMsg:
		default:
			qCDebug(syncProtocol)<<"[Peer"<<(sock->peerAddress().toString() + ":" + QString::number(sock->peerPort()))<<"]:" << msg;
			break;
	}
}

void SyncRemotePeer::writeMessage(const SyncMessage &msg)
{
	qint64 size = msg.createFullMessage(msgWriteBuffer);
	peerLog(QtDebugMsg, "Send message" + msg.toString());

	if(!size)
	{
		//crash here when message is too large in debugging
		Q_ASSERT(true);
		qCCritical(syncProtocol)<<"A message is too large for sending! Message buffer contents follow...";
		qCCritical(syncProtocol)<<msgWriteBuffer.toHex();
		//disconnect the client
		writeError("next pending message too large");
	}
	else
	{
		writeData(msgWriteBuffer,size);
	}
}

void SyncRemotePeer::writeData(const QByteArray &data, qint64 size)
{
	//Only write if connected
	if(sock->state() == QAbstractSocket::ConnectedState)
	{
		stream.writeRawData(data.constData(),static_cast<int>(size>0ll?size:data.size()));
		lastSendTime = QDateTime::currentMSecsSinceEpoch();
	}
	else
		qCWarning(syncProtocol) << "Can't write message, not connected";
}

void SyncRemotePeer::writeError(const QString &err)
{
	qCWarning(syncProtocol)<<"Disconnecting with error:"<<err;
	writeMessage(ErrorMessage(err));
	errorString = err;
	sock->disconnectFromHost();
}

QString SyncMessage::toString() const
{
	SyncProtocol::SyncMessageType type=getMessageType();
	return toString(type);
}
QString SyncMessage::toString(SyncProtocol::SyncMessageType type)
{
	switch (type) {
            case SyncProtocol::SYNC_ERROR:
			return "ERROR";
			break;
		case SyncProtocol::SERVER_CHALLENGE:
			return"SERVER_CHALLENGE";
			break;
		case SyncProtocol::CLIENT_CHALLENGE_RESPONSE:
			return"CLIENT_CHALLENGE_RESPONSE";
			break;
		case SyncProtocol::SERVER_CHALLENGERESPONSEVALID:
			return"SERVER_CHALLENGERESPONSEVALID";
			break;
		case SyncProtocol::TIME:
			return"TIME";
			break;
		case SyncProtocol::LOCATION:
			return"LOCATION";
			break;
		case SyncProtocol::SELECTION:
			return"SELECTION";
			break;
		case SyncProtocol::STELPROPERTY:
			return"STELPROPERTY";
			break;
		case SyncProtocol::VIEW:
			return "VIEW";
			break;
		case SyncProtocol::ALIVE:
			return "ALIVE";
			break;
		default:
			return QString("UNKNOWN(%1").arg(int(type));
			break;
	}
}
