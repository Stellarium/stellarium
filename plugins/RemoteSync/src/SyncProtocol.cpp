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
		SyncHeader header = { getMessageType(), static_cast<tPayloadSize>(writtenSize) };
		tmpStream.device()->seek(0);
		tmpStream<<header;

		return totalSize;
	}
}

void SyncMessage::serialize(QDataStream &stream) const
{
	Q_UNUSED(stream);
}

bool SyncMessage::deserialize(QDataStream &stream, tPayloadSize dataSize)
{
	Q_UNUSED(stream);
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

SyncRemotePeer::SyncRemotePeer()
	: isValid(false)
{

}

SyncRemotePeer::SyncRemotePeer(QAbstractSocket *socket, bool isServer, const QVector<SyncMessageHandler *> &handlerList)
	: isValid(true), sock(socket), isPeerAServer(isServer), isAuthenticated(false), authResponseSent(false), waitingForBody(false),
	  handlerList(handlerList)
{
	lastReceiveTime = QDateTime::currentMSecsSinceEpoch();
	lastSendTime = lastReceiveTime;
	msgWriteBuffer.reserve(SYNC_MAX_MESSAGE_SIZE);
}

void SyncRemotePeer::receiveMessage()
{
	QDataStream dataStream(sock);
	dataStream.setVersion(SYNC_DATASTREAM_VERSION);

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

			dataStream>>msgHeader;
			//check if msgtype is valid
			if(msgHeader.msgType>MSGTYPE_MAX)
			{
				writeError("invalid message type " + QString::number(msgHeader.msgType));
				return;
			}
			if(!isAuthenticated && msgHeader.msgType > SERVER_CHALLENGERESPONSEVALID)
			{
				//if not fully authenticated, it is an error to send messages other than auth messages
				writeError("not authenticated");
				return;
			}

			qDebug()<<"receive header for "<<msgHeader.msgType;
		}

		if(sock->bytesAvailable() < msgHeader.dataSize)
		{
			waitingForBody = true;
			return;
		}
		else
		{
			waitingForBody = false;
			qDebug()<<"received body, processing";

			//full packet available, pass to handler
			SyncMessageHandler* handler = handlerList[msgHeader.msgType];
			if(!handler)
			{
				//no handler registered on this end for this msgtype
				writeError("unregistered message type " + QString::number(msgHeader.msgType));
				return;
			}
			if(!handlerList[msgHeader.msgType]->handleMessage(dataStream,*this))
			{
				writeError("last message of type " + QString::number(msgHeader.msgType) + " was rejected");
			}
		}
	}

	qDebug()<<"Available bytes after receiveMessage"<<sock->bytesAvailable();
}

void SyncRemotePeer::peerLog(const QString &msg)
{
	qDebug()<<"[Sync][Peer"<<(sock->peerAddress().toString() + ":" + QString::number(sock->peerPort()))<<"]:"<<msg;
}

void SyncRemotePeer::writeMessage(const SyncMessage &msg)
{
	qint64 size = msg.createFullMessage(msgWriteBuffer);
	qDebug()<<"[SyncPeer] Send message"<<msg.getMessageType();

	if(!size)
	{
		//crash here when message is too large in debugging
		Q_ASSERT(true);
		qCritical()<<"[SyncPlugin] A message is too large for sending! Message buffer contents follow...";
		qCritical()<<msgWriteBuffer.toHex();
		//disconnect the client
		writeError("next pending message too large");
	}
	else
	{
		writeData(msgWriteBuffer,size);
	}
}

void SyncRemotePeer::writeData(const QByteArray &data, int size)
{
	//Only write if connected
	if(sock->state() == QAbstractSocket::ConnectedState)
	{
		sock->write(data.constData(),size>0?size:data.size());
		//flush immediately if possible to reduce delay
		sock->flush();
		lastSendTime = QDateTime::currentMSecsSinceEpoch();
	}
	else
		peerLog("Can't write message, not connected");
}

void SyncRemotePeer::writeError(const QString &err)
{
	qWarning()<<"[SyncPlugin] Disconnecting with error:"<<err;
	writeMessage(ErrorMessage(err));
	sock->disconnectFromHost();
}
