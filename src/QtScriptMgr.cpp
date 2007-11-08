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

Q_DECLARE_METATYPE(Vec3f);

QScriptValue vec3fToScriptValue(QScriptEngine *engine, const Vec3f& c)
{
	QScriptValue obj = engine->newObject();
	obj.setProperty("r", QScriptValue(engine, c[0]));
	obj.setProperty("g", QScriptValue(engine, c[1]));
	obj.setProperty("b", QScriptValue(engine, c[2]));
	return obj;
}

void vec3fFromScriptValue(const QScriptValue& obj, Vec3f& c)
{
	c[0] = obj.property("r").toNumber();
	c[1] = obj.property("g").toNumber();
	c[2] = obj.property("b").toNumber();
}

QScriptValue createVec3f(QScriptContext* context, QScriptEngine *engine)
{
	Vec3f c;
	c[0] = context->argument(0).toNumber();
	c[1] = context->argument(1).toNumber();
	c[2] = context->argument(2).toNumber();
	return vec3fToScriptValue(engine, c);
}

QtScriptMgr::QtScriptMgr(QObject *parent) : QObject(parent)
{
	// Allow Vec3f managment in scripts
	qScriptRegisterMetaType(&engine, vec3fToScriptValue, vec3fFromScriptValue);
	// Constructor
	QScriptValue ctor = engine.newFunction(createVec3f);
	engine.globalObject().setProperty("Vec3f", ctor);
	
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
	engine.evaluate("modules.ConstellationMgr.flagLines = true");
	engine.evaluate("modules.ConstellationMgr.linesColor = Vec3f(1.,0.,0.)");
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
