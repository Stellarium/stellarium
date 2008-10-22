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
#include "StelUtils.hpp"
#include "StelObjectMgr.hpp"

#include <QFile>
#include <QSet>
#include <QDebug>
#include <QStringList>

Q_DECLARE_METATYPE(Vec3f);

class StelScriptThread : public QThread
{
	public:
		StelScriptThread(const QString& ascriptCode, QScriptEngine* aengine, QString fileName) : scriptCode(ascriptCode), engine(aengine), fname(fileName) {;}
		QString getFileName() {return fname;}
		
	protected:
		void run()
		{
			engine->evaluate(scriptCode);
		}
    
	private:
		QString scriptCode;
		QScriptEngine* engine;
		QString fname;
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

void StelMainScriptAPI::setDate(QString dt)
{
	double JD = StelUtils::qDateTimeToJd(QDateTime::fromString(dt, Qt::ISODate));
	StelApp::getInstance().getCore()->getNavigation()->setJDay(JD);
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

void StelMainScriptAPI::wait(double t)
{
	MySleep::msleep(t*1000);
}

void StelMainScriptAPI::debug(const QString& s)
{
	qDebug() << "script: " << s;
}

void StelMainScriptAPI::selectObjectByName(QString name, bool pointer)
{
	StelApp::getInstance().getStelObjectMgr().findAndSelect(name);
	StelApp::getInstance().getStelObjectMgr().setFlagSelectedObjectPointer(pointer);
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
	
	runScript("startup.ssc");
}


QtScriptMgr::~QtScriptMgr()
{
}

QStringList QtScriptMgr::getScriptList(void)
{
	QStringList scriptFiles;
	try
	{
		StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
		QSet<QString> files = fileMan.listContents("scripts",StelFileMgr::File);
		foreach(QString f, files)
		{
			if (QRegExp("^.*\\.ssc$").exactMatch(f))
				scriptFiles << f;
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: could not list scripts:" << e.what();
	}
	return scriptFiles;
}

bool QtScriptMgr::scriptIsRunning(void)
{
	return (thread != NULL);
}

QString QtScriptMgr::runningScriptId(void)
{
	if (thread)
		return thread->getFileName();
	else
		return QString();
}

const QString QtScriptMgr::getName(const QString& s)
{
	try
	{
		QFile file(StelApp::getInstance().getFileMgr().findFile("scripts/" + s, StelFileMgr::File));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qWarning() << "script file " << s << " could not be opened for reading";
			return QString();
		}

		QRegExp nameExp("^\\s*//\\s*Name:\\s*(.+)$");
		while (!file.atEnd()) {
			QString line(file.readLine());
			if (nameExp.exactMatch(line))
			{
				file.close();	
				return nameExp.capturedTexts().at(1);
			}
		}
		file.close();
		return s;
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "script file " << s << " could not be found:" << e.what();
		return QString();
	}
}

const QString QtScriptMgr::getDescription(const QString& s)
{
	try
	{
		QFile file(StelApp::getInstance().getFileMgr().findFile("scripts/" + s, StelFileMgr::File));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qWarning() << "script file " << s << " could not be opened for reading";
			return QString();
		}

		QString desc = "";
		bool inDesc = false;
		QRegExp descExp("^\\s*//\\s*Description:\\s*(.+)$");
		QRegExp descContExp("^\\s*//\\s*(.*)$");
		while (!file.atEnd()) {
			QString line(file.readLine());
			
			if (!inDesc && descExp.exactMatch(line))
			{
				inDesc = true;
				desc = descExp.capturedTexts().at(1) + "\n";
			}
			else if (inDesc)
			{
				if (descContExp.exactMatch(line))
					desc += descContExp.capturedTexts().at(1) + "\n";
				else
				{
					file.close();	
					return desc;
				}
			}
			
		}
		file.close();
		return desc;
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "script file " << s << " could not be found:" << e.what();
		return QString();
	}
}

// Run the script located at the given location
bool QtScriptMgr::runScript(const QString& fileName)
{
	if (thread!=NULL)
	{
		qWarning() << "ERROR: there is already a script running, please wait that it's over.";
		return false;
	}
	QString absPath;
	try
	{
		absPath = StelApp::getInstance().getFileMgr().findFile("scripts/" + fileName);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: could not find script file " << fileName << ": " << e.what();
		return false;
	}
	QFile fic(absPath);
	fic.open(QIODevice::ReadOnly);
	thread = new StelScriptThread(QTextStream(&fic).readAll(), &engine, fileName);
	fic.close();
	
	connect(thread, SIGNAL(finished()), this, SLOT(scriptEnded()));
	thread->start();
	emit(scriptRunning());
	return true;
}

bool QtScriptMgr::stopScript(void)
{
	if (thread)
	{
		qDebug() << "asking running script to exit";
		thread->terminate();
		return true;
	}
	else
	{
		qWarning() << "QtScriptMgr::stopScript - no script is running";
		return false;
	}
}

void QtScriptMgr::scriptEnded()
{
	delete thread;
	thread=NULL;
	if (engine.hasUncaughtException())
	{
		qWarning() << "Error while running script: " << engine.uncaughtException().toString() << endl;
	}
	emit(scriptStopped());
}

