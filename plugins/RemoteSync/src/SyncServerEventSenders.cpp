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

#include "SyncServerEventSenders.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "SyncServer.hpp"

SyncServerEventSender::SyncServerEventSender()
	: isDirty(false)
{

}

void SyncServerEventSender::broadcastMessage(const SyncMessage &msg)
{
	server->broadcastMessage(msg);
}

TimeEventSender::TimeEventSender()
{
	core = StelApp::getInstance().getCore();
	//this is the only event we need to listen to
	connect(core,SIGNAL(timeSyncOccurred(double)),this,SLOT(reactToStellariumEvent()));
}

Time TimeEventSender::constructMessage()
{
	Time msg;
	msg.lastTimeSyncTime = core->getMilliSecondsOfLastJDayUpdate();
	msg.jDay = core->getJDayOfLastJDayUpdate();
	msg.timeRate = core->getTimeRate();
	return msg;
}
