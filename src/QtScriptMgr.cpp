/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "QtScriptMgr.hpp"

#include "fixx11h.h"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "Navigator.hpp"

QtScriptMgr::QtScriptMgr(QObject *parent) : QObject(parent)
{
	StelMainScriptAPI *mainAPI = new StelMainScriptAPI(this);
	QScriptValue objectValue = engine.newQObject(mainAPI);
	engine.globalObject().setProperty("main", objectValue);
	
	objectValue = engine.newQObject(&StelApp::getInstance().getModuleMgr());
	engine.globalObject().setProperty("modules", objectValue);
}


QtScriptMgr::~QtScriptMgr()
{
}

void QtScriptMgr::test()
{
	engine.evaluate("main.JDay = 152200.");
	//engine.evaluate("modules.ConstellationMgr.setFlagArt(true)");
	engine.evaluate("modules.ConstellationMgr.flagArt = true");
	if (engine.hasUncaughtException())
	{
		qWarning() << engine.uncaughtException().toString() << endl;
	}
}

StelMainScriptAPI::StelMainScriptAPI(QObject *parent) : QObject(parent)
{
}


StelMainScriptAPI::~StelMainScriptAPI()
{
}

//! Set the current date in Julian Day
//! @param JD the Julian Date
void StelMainScriptAPI::setJDay(double JD)
{
	StelApp::getInstance().getCore()->getNavigation()->setJDay(JD);
}

//! Get the current date in Julian Day
//! @return the Julian Date
double StelMainScriptAPI::getJDay(void) const
{
	return StelApp::getInstance().getCore()->getNavigation()->getJDay();
}

//! Set time speed in JDay/sec
//! @param ts time speed in JDay/sec
void StelMainScriptAPI::setTimeSpeed(double ts)
{
	StelApp::getInstance().getCore()->getNavigation()->setTimeSpeed(ts);
}

//! Get time speed in JDay/sec
//! @return time speed in JDay/sec
double StelMainScriptAPI::getTimeSpeed(void) const
{
	return StelApp::getInstance().getCore()->getNavigation()->getTimeSpeed();
}
