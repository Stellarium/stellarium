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
#include "StelMovementMgr.hpp"
#include "StelObserver.hpp"
#include "StelObjectMgr.hpp"
#include "StelPropertyMgr.hpp"

using namespace SyncProtocol;

ClientHandler::ClientHandler()
	: client(Q_NULLPTR)
{
	core = StelApp::getInstance().getCore();
}

ClientHandler::ClientHandler(SyncClient *client)
	: client(client)
{
	Q_ASSERT(client);
	core = StelApp::getInstance().getCore();
}


ClientErrorHandler::ClientErrorHandler(SyncClient *client)
	: ClientHandler(client)
{
}

bool ClientErrorHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	ErrorMessage msg;
	bool ok = msg.deserialize(stream,dataSize);
	peer.peerLog("Received error message from server: " + msg.message);

	client->emitServerError(msg.message);

	//we don't drop the connection here, we let the remote end do that
	return ok;
}

ClientAuthHandler::ClientAuthHandler(SyncClient *client)
	: ClientHandler(client)
{
	connect(this, SIGNAL(authenticated()),client,SIGNAL(connected()));
}


bool ClientAuthHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	//get message type
	SyncMessageType type = SyncMessageType(peer.msgHeader.msgType);

	if(type == SERVER_CHALLENGE)
	{
		if(peer.isAuthenticated())
		{
			//we are already authenticated, another challenge is an error
			qWarning()<<"[SyncClient] received server challenge when not expecting one";
			return false;
		}

		ServerChallenge msg;
		bool ok = msg.deserialize(stream,dataSize);

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
			peer.authenticated = true;
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

bool ClientAliveHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	Q_UNUSED(peer)
	Alive p;
	return p.deserialize(stream,dataSize);
}

bool ClientTimeHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	Q_UNUSED(peer)
	Time msg;
	bool ok = msg.deserialize(stream, dataSize);

	if(!ok)
		return false;

	//set time variables, time rate first because it causes a resetSync which we overwrite
	core->setTimeRate(msg.timeRate);
	core->setJD(msg.jDay);
	//This is needed for compensation of network delay. Requires system clocks of client/server to be calibrated to the same values.
	core->setMilliSecondsOfLastJDUpdate(msg.lastTimeSyncTime);

	return true;
}

bool ClientLocationHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	Q_UNUSED(peer)
	Location msg;
	bool ok = msg.deserialize(stream,dataSize);

	if(!ok)
		return false;

	//replicated from StelCore::moveObserverTo
	if(msg.totalDuration>0.0)
	{
		//for optimal results, the network latency should be subtracted from the timeToGo...

		StelLocation curLoc = core->getCurrentLocation();
		if (core->getCurrentObserver()->isTraveling())
		{
			// Avoid using a temporary location name to create another temporary one (otherwise it looks like loc1 -> loc2 -> loc3 etc..)
			curLoc.name = ".";
		}

		//create a spaceship observer
		SpaceShipObserver* newObs = new SpaceShipObserver(curLoc, msg.stelLocation, msg.totalDuration,msg.timeToGo);
		core->setObserver(newObs);
		newObs->update(0);
	}
	else
	{
		//create a normal observer
		core->setObserver(new StelObserver(msg.stelLocation));
	}
	emit core->targetLocationChanged(msg.stelLocation, QString());
	emit core->locationChanged(core->getCurrentLocation());


	return true;
}

ClientSelectionHandler::ClientSelectionHandler()
{
	objMgr = &StelApp::getInstance().getStelObjectMgr();
}

bool ClientSelectionHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	Q_UNUSED(peer)
	Selection msg;
	bool ok = msg.deserialize(stream, dataSize);

	if(!ok)
		return false;

	qDebug()<<msg;

	//lookup the objects from their names
	//this might cause problems if 2 objects of different types have the same name!
	QList<StelObjectP> selection;

	for (const auto& selectedObject : qAsConst(msg.selectedObjects))
	{
		StelObjectP obj = objMgr->searchByID(selectedObject.first, selectedObject.second);
		if(obj)
			selection.append(obj);
		else
			qWarning() << "Object not found" << selectedObject.first << selectedObject.second;
	}

	if(selection.isEmpty())
		objMgr->unSelect();
	else
	{
		//set selection
		objMgr->setSelectedObject(selection,StelModule::ReplaceSelection);
	}

	return true;
}

ClientStelPropertyUpdateHandler::ClientStelPropertyUpdateHandler(bool skipGuiProps, const QStringList &excludeProps)
{
	propMgr = StelApp::getInstance().getStelPropertyManager();

	QString pattern("^(");
	//construct a regular expression for the excludes
	bool first = true;
	for (auto &str : excludeProps)
	{
		QString tmp = QRegularExpression::escape(str);
		// replace escaped asterisks with the regex "all"
		tmp.replace("\\*",".*");
		if(!first)
		{
			pattern += '|';
		}
		first = false;
		pattern += tmp;
	}

	if(skipGuiProps)
	{
		if(!first)
		{
			pattern += '|';
		}
		first = false;

		//this is an attempt to filter out the GUI related properties
		pattern += "(actionShow_.*(Window_Global|_dialog))"; //most dialogs follow one of these patterns
		pattern += "|actionShow_Scenery3d_storedViewDialog"; //add other dialogs here
	}

	//finish the pattern
	pattern += ")$";
	filter.setPattern(pattern);

	if(!filter.isValid())
		qWarning()<<"Invalid StelProperty filter:"<<filter.errorString();
	else
		qDebug()<<"Constructed regex"<<filter;

	filter.optimize();
}

bool ClientStelPropertyUpdateHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	Q_UNUSED(peer)
	StelPropertyUpdate msg;
	bool ok = msg.deserialize(stream, dataSize);

	if(!ok)
		return false;

	qDebug()<<msg;

	QRegularExpressionMatch match = filter.match(msg.propId);
	if(match.hasMatch())
	{
		//filtered property
		qDebug()<<"Filtered"<<msg;
		return true;
	}
	propMgr->setStelPropertyValue(msg.propId,msg.value);
	return true;
}

ClientViewHandler::ClientViewHandler()
{
	mvMgr = core->getMovementMgr();
}

bool ClientViewHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	Q_UNUSED(peer)
	View msg;
	bool ok = msg.deserialize(stream, dataSize);
	if(!ok) return false;

	mvMgr->setViewDirectionJ2000(core->altAzToJ2000(msg.viewAltAz, StelCore::RefractionOff));
	return true;
}

ClientFovHandler::ClientFovHandler()
{
	mvMgr = core->getMovementMgr();
}

bool ClientFovHandler::handleMessage(QDataStream &stream, SyncProtocol::tPayloadSize dataSize, SyncRemotePeer &peer)
{
	Q_UNUSED(peer)
	Fov msg;
	bool ok = msg.deserialize(stream, dataSize);
	if(!ok) return false;

	mvMgr->zoomTo(msg.fov, 0.0f);
	return true;
}
