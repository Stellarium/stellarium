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
#include "StelFileMgr.hpp"

#include <QFile>

Q_DECLARE_METATYPE(Vec3f);

class StelScriptThread : public QThread
{
	public:
		StelScriptThread(const QString& ascriptCode, QScriptEngine* aengine) : scriptCode(ascriptCode), engine(aengine) {;}
		
	protected:
		void run()
		{
			engine->evaluate(scriptCode);
		}
    
	private:
		QString scriptCode;
		QScriptEngine* engine;
};


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

QtScriptMgr::QtScriptMgr(QObject *parent) : QObject(parent), thread(NULL)
{
	// Allow Vec3f managment in scripts
	qScriptRegisterMetaType(&engine, vec3fToScriptValue, vec3fFromScriptValue);
	// Constructor
	QScriptValue ctor = engine.newFunction(createVec3f);
	engine.globalObject().setProperty("Vec3f", ctor);
	
	// Add the core object to access methods related to core
	StelMainScriptAPI *mainAPI = new StelMainScriptAPI(this);
	QScriptValue objectValue = engine.newQObject(mainAPI);
	engine.globalObject().setProperty("core", objectValue);
	
	// Add all the StelModules into the script engine
	StelModuleMgr* mmgr = &StelApp::getInstance().getModuleMgr();
	foreach (StelModule* m, mmgr->getAllModules())
	{
		objectValue = engine.newQObject(m);
		engine.globalObject().setProperty(m->objectName(), objectValue);
	}
	
	runScript("scripts/test.sts");
}


QtScriptMgr::~QtScriptMgr()
{
}

// Run the script located at the given location
void QtScriptMgr::runScript(const QString& fileName)
{
	if (thread!=NULL)
	{
		qWarning() << "ERROR: there is already a script running, please wait that it's over.";
		return;
	}
	QString absPath;
	try
	{
		absPath = StelApp::getInstance().getFileMgr().findFile(fileName);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: could not find script file " << fileName << ": " << e.what();
		return;
	}
	QFile fic(absPath);
	fic.open(QIODevice::ReadOnly);
	thread = new StelScriptThread(QTextStream(&fic).readAll(), &engine);
	fic.close();
	
	connect(thread, SIGNAL(finished()), this, SLOT(scriptEnded()));
	thread->start();
}
	
void QtScriptMgr::scriptEnded()
{
	delete thread;
	thread=NULL;
	if (engine.hasUncaughtException())
	{
		qWarning() << "Error while running script: " << engine.uncaughtException().toString() << endl;
	}
}
	
// void QtScriptMgr::test()
// {
// 	engine.evaluate("core.JDay = 152200.; ConstellationMgr.setFlagArt(true)");
// 	engine.evaluate("ConstellationMgr.flagArt = true");
// 	engine.evaluate("ConstellationMgr.flagLines = true");
// 	engine.evaluate("ConstellationMgr.linesColor = Vec3f(1.,0.,0.)");
// 	if (engine.hasUncaughtException())
// 	{
// 		qWarning() << engine.uncaughtException().toString() << endl;
// 	}
// }

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

// This class let's us sleep in milleseconds
class MySleep : public QThread
{
	public:
		static void msleep(unsigned long msecs) 
		{
			QThread::msleep(msecs);
		}
};

void StelMainScriptAPI::sleep(double t)
{
	MySleep::msleep(t*1000);
}
