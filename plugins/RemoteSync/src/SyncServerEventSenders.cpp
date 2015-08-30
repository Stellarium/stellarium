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
#include "SyncServer.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelObserver.hpp"
#include "StelObjectMgr.hpp"


SyncServerEventSender::SyncServerEventSender()
	: isDirty(false)
{
	core = StelApp::getInstance().getCore();
}

void SyncServerEventSender::broadcastMessage(const SyncMessage &msg)
{
	server->broadcastMessage(msg);
}

TimeEventSender::TimeEventSender()
{
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

LocationEventSender::LocationEventSender()
{
	connect(core,SIGNAL(locationChanged(StelLocation)), this, SLOT(reactToStellariumEvent()));
}

Location LocationEventSender::constructMessage()
{
	Location loc;
	const StelObserver* obs = core->getCurrentObserver();

	//test if the observer is moving (spaceship)
	const SpaceShipObserver* sso = qobject_cast<const SpaceShipObserver*>(obs);

	if(sso)
	{
		loc.timeToGo = sso->getRemainingTime();
		loc.totalDuration = sso->getTransitTime();
		loc.stelLocation = sso->getTargetLocation();
	}
	else
	{
		loc.stelLocation = obs->getCurrentLocation();
	}

	return loc;
}

SelectionEventSender::SelectionEventSender()
{
	objMgr = &StelApp::getInstance().getStelObjectMgr();
	connect(objMgr,SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)),this,SLOT(reactToStellariumEvent()));
}

Selection SelectionEventSender::constructMessage()
{
	Selection msg;
	const QList<StelObjectP>& selObj = objMgr->getSelectedObject();

	//note: there is no current way to uniquely identify an object
	//maybe identify by type+name? but it seems this needs some changes
	//(StelObject::getType does not correspond directly to StelObjectModule::getName)
	//even then, some objects (e.g. Nebulas) dont seem to have a name at all!

	for (QList<StelObjectP>::const_iterator iter=selObj.constBegin();iter!=selObj.constEnd();++iter)
	{
		msg.selectedObjectNames.append((*iter)->getEnglishName());
	}

	return msg;
}
