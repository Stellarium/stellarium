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

#ifndef SYNCSERVEREVENTSENDERS_HPP
#define SYNCSERVEREVENTSENDERS_HPP

#include "SyncProtocol.hpp"
#include "SyncMessages.hpp"

class SyncServer;
class StelCore;
class StelObjectMgr;

//! Subclasses of this class notify clients of state changes.
class SyncServerEventSender : public QObject
{
	Q_OBJECT
public:
	SyncServerEventSender();
	~SyncServerEventSender() override {}

protected slots:

	//! This may be used to react to Stellarium application events and queue a broadcast or store the changed state.
	//! The general idea is to connect this to various signals in the constructor.
	//! It is not necessary to use this, but recommended for clarity.
	//! The default implementation sets isDirty to true.
	virtual void reactToStellariumEvent() { isDirty = true; }

	//! This is automatically called by the SyncServer whenever a new client connects.
	//! Use this to set clients to the current server state.
	//! The default implementation does nothing.
	virtual void newClientConnected(SyncRemotePeer& client) { Q_UNUSED(client) }
protected:
	//! This is guaranteed to be called once per frame (usually after all other StelModules have been updated).
	//! It is can be used to defer state broadcasts until the frame is finished to only send a single message.
	//! Default implementation does nothing.
	virtual void update() {}

	//! Subclasses can call this to broadcast a message to all valid connected clients
	void broadcastMessage(const SyncProtocol::SyncMessage& msg);
	//! Free to use by subclasses. Recommendation: use to track if update() should broadcast a message.
	bool isDirty;
	//! Direct access to StelCore
	StelCore* core;
private:
	SyncServer* server;
	friend class SyncServer;
};

//! This class provides a semi-automatic event sender using a specific message type.
template<class T>
class TypedSyncServerEventSender : public SyncServerEventSender
{
protected:
	//! This has to be used to construct a message using current app state.
	//! The default implementation constructs an error message.
	virtual T constructMessage() = 0;

	//! Uses constructMessage() to send a message to the new client.
	void newClientConnected(SyncRemotePeer& client) override;

	//! If isDirty is true, broadcasts a message to all using constructMessage() and broadcastMessage,
	//! and resets isDirty.
	void update() override;
};

template<class T>
void TypedSyncServerEventSender<T>::newClientConnected(SyncRemotePeer& client)
{
	client.writeMessage(constructMessage());
}

template<class T>
void TypedSyncServerEventSender<T>::update()
{
	if(isDirty)
	{
		broadcastMessage(constructMessage());
		isDirty = false;
	}
}

//! Notifies clients of simulation time jumps and time scale changes
class TimeEventSender : public TypedSyncServerEventSender<SyncProtocol::Time>
{
	Q_OBJECT

public:
	TimeEventSender();
protected:
	SyncProtocol::Time constructMessage() override;
};

class LocationEventSender : public TypedSyncServerEventSender<SyncProtocol::Location>
{
	Q_OBJECT
public:
	LocationEventSender();
protected:
	SyncProtocol::Location constructMessage() override;
};

class SelectionEventSender : public TypedSyncServerEventSender<SyncProtocol::Selection>
{
	Q_OBJECT
public:
	SelectionEventSender();
protected:
	SyncProtocol::Selection constructMessage() override;
private:
	StelObjectMgr* objMgr;
};

class StelProperty;
class StelPropertyMgr;
class StelPropertyEventSender : public SyncServerEventSender
{
	Q_OBJECT
public:
	StelPropertyEventSender();
protected slots:
	//! Sends all current StelProperties to the client
	void newClientConnected(SyncRemotePeer& client) override;
	void sendStelPropChange(StelProperty* prop, const QVariant& val);
private:
	StelPropertyMgr* propMgr;
};

class StelMovementMgr;
class ViewEventSender : public TypedSyncServerEventSender<SyncProtocol::View>
{
	Q_OBJECT
public:
	ViewEventSender();
protected:
	SyncProtocol::View constructMessage() override;

	void update() override;
private:
	StelMovementMgr* mvMgr;
	Vec3d lastView;
};

#endif
