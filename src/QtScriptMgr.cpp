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

#include "fixx11h.h"

#include "QtScriptMgr.hpp"
#include "ConstellationMgr.hpp"
#include "GridLinesMgr.hpp"
#include "LandscapeMgr.hpp"
#include "MeteorMgr.hpp"
#include "MovementMgr.hpp"
#include "Navigator.hpp"
#include "NebulaMgr.hpp"
#include "SolarSystem.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "SkyDrawer.hpp"
#include "StarMgr.hpp"
#include "Projector.hpp"
#include "Location.hpp"
#include "LocationMgr.hpp"

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
			// For startup scripts, the gui object might not 
			// have completed init when we run. Wait for that.
			StelGui* gui = (StelGui*)GETSTELMODULE("StelGui");
			while(!gui)
			{
				msleep(200);
				gui = (StelGui*)GETSTELMODULE("StelGui");
			}
			while(!gui->initComplete())
				msleep(200);

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

void StelMainScriptAPI::setDate(const QString& dt, const QString& spec)
{
	QDateTime qdt;
	double JD;
	qdt = QDateTime::fromString(dt, Qt::ISODate);

	if (spec=="local")
		JD = StelUtils::qDateTimeToJd(qdt.toUTC());
	else
		JD = StelUtils::qDateTimeToJd(qdt);
		
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

void StelMainScriptAPI::setObserverLocation(double longitude, double latitude, double altitude, double duration, const QString& name, const QString& planet)
{
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	Q_ASSERT(nav);
	SolarSystem* ssmgr = (SolarSystem*)GETSTELMODULE("SolarSystem");
	Q_ASSERT(ssmgr);

	Location loc = nav->getCurrentLocation();
	if (longitude < 180 || longitude > 180)
		loc.longitude = longitude;
	if (latitude < 180 || latitude > 180)
		loc.latitude = latitude;
	if (altitude < -1000)
		loc.altitude = altitude;
	if (ssmgr->searchByName(planet))
		loc.planetName = planet;
	loc.name = name;

	nav->moveObserverTo(loc, duration);
}

void StelMainScriptAPI::setObserverLocation(const QString id, double duration)
{
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	Q_ASSERT(nav);
	Location loc = StelApp::getInstance().getLocationMgr().locationForSmallString(id);
	// How best to test to see if the lookup of the name was a success?
	// On failure, it returns Paris, but maybe we _want_ Paris.
	// Ugly. -MNG
	if (id!="Paris, France" && (loc.name=="Paris" && loc.country=="France"))
		return;	// location find fail

	nav->moveObserverTo(loc, duration);
}

void StelMainScriptAPI::debug(const QString& s)
{
	qDebug() << "script: " << s;
}

void StelMainScriptAPI::selectObjectByName(const QString& name, bool pointer)
{
	StelApp::getInstance().getStelObjectMgr().findAndSelect(name);
	StelApp::getInstance().getStelObjectMgr().setFlagSelectedObjectPointer(pointer);
}

void StelMainScriptAPI::clear(const QString& state)
{
	LandscapeMgr* lmgr = (LandscapeMgr*)GETSTELMODULE("LandscapeMgr");
	Q_ASSERT(lmgr);
	SolarSystem* ssmgr = (SolarSystem*)GETSTELMODULE("SolarSystem");
	Q_ASSERT(ssmgr);
	MeteorMgr* mmgr = (MeteorMgr*)GETSTELMODULE("MeteorMgr");
	Q_ASSERT(mmgr);
	SkyDrawer* skyd = StelApp::getInstance().getCore()->getSkyDrawer();
	Q_ASSERT(skyd);
	ConstellationMgr* cmgr = (ConstellationMgr*)GETSTELMODULE("ConstellationMgr");
	Q_ASSERT(cmgr);
	StarMgr* smgr = (StarMgr*)GETSTELMODULE("StarMgr");
	Q_ASSERT(smgr);
	NebulaMgr* nmgr = (NebulaMgr*)GETSTELMODULE("NebulaMgr");
	Q_ASSERT(nmgr);
	GridLinesMgr* glmgr = (GridLinesMgr*)GETSTELMODULE("GridLinesMgr");
	Q_ASSERT(glmgr);
	StelGui* gui = (StelGui*)GETSTELMODULE("StelGui");
	Q_ASSERT(gui);
	MovementMgr* mvmgr = (MovementMgr*)GETSTELMODULE("MovementMgr");
	Q_ASSERT(mvmgr);
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	Q_ASSERT(nav);
	Projector* proj = StelApp::getInstance().getCore()->getProjection();
	Q_ASSERT(proj);

	if (state == "natural")
	{
		nav->setEquatorialMount(false);
		skyd->setFlagTwinkle(true);
		skyd->setFlagLuminanceAdaptation(true);
		ssmgr->setFlagPlanets(true);
		ssmgr->setFlagHints(false);
		ssmgr->setFlagOrbits(false);
		ssmgr->setFlagMoonScale(false);
		ssmgr->setFlagTrails(false);
		mmgr->setZHR(10);
		glmgr->setFlagAzimuthalGrid(false);
		glmgr->setFlagEquatorGrid(false);
		glmgr->setFlagEquatorLine(false);
		glmgr->setFlagEclipticLine(false);
		glmgr->setFlagMeridianLine(false);
		glmgr->setFlagEquatorJ2000Grid(false);
		lmgr->setFlagCardinalsPoints(false);
		cmgr->setFlagLines(false);
		cmgr->setFlagLabels(false);
		cmgr->setFlagBoundaries(false);
		cmgr->setFlagArt(false);
		smgr->setFlagLabels(false);
		ssmgr->setFlagLabels(false);
		nmgr->setFlagHints(false);
		lmgr->setFlagLandscape(true);
		lmgr->setFlagAtmosphere(true);
		lmgr->setFlagFog(true);
	}
	else if (state == "starchart")
	{
		nav->setEquatorialMount(true);
		skyd->setFlagTwinkle(false);
		skyd->setFlagLuminanceAdaptation(false);
		ssmgr->setFlagPlanets(true);
		ssmgr->setFlagHints(false);
		ssmgr->setFlagOrbits(false);
		ssmgr->setFlagMoonScale(false);
		ssmgr->setFlagTrails(false);
		mmgr->setZHR(0);
		glmgr->setFlagAzimuthalGrid(false);
		glmgr->setFlagEquatorGrid(true);
		glmgr->setFlagEquatorLine(false);
		glmgr->setFlagEclipticLine(false);
		glmgr->setFlagMeridianLine(false);
		glmgr->setFlagEquatorJ2000Grid(false);
		lmgr->setFlagCardinalsPoints(false);
		cmgr->setFlagLines(true);
		cmgr->setFlagLabels(true);
		cmgr->setFlagBoundaries(true);
		cmgr->setFlagArt(false);
		smgr->setFlagLabels(true);
		ssmgr->setFlagLabels(true);
		nmgr->setFlagHints(true);
		lmgr->setFlagLandscape(false);
		lmgr->setFlagAtmosphere(false);
		lmgr->setFlagFog(false);
	}
	else
	{
		qWarning() << "WARNING clear(" << state << ") - state not known";
	}
}

void StelMainScriptAPI::moveToAltAzi(const QString& alt, const QString& azi, float duration)
{
	MovementMgr* mvmgr = (MovementMgr*)GETSTELMODULE("MovementMgr");
	Q_ASSERT(mvmgr);

	StelApp::getInstance().getStelObjectMgr().unSelect();

	Vec3d aim;
	double dAlt = StelUtils::getDecAngle(alt); 
	double dAzi = M_PI - StelUtils::getDecAngle(azi);

	StelUtils::spheToRect(dAzi,dAlt,aim);
	mvmgr->moveTo(aim, duration, true);
}

void StelMainScriptAPI::moveToRaDec(const QString& ra, const QString& dec, float duration)
{
	MovementMgr* mvmgr = (MovementMgr*)GETSTELMODULE("MovementMgr");
	Q_ASSERT(mvmgr);

	StelApp::getInstance().getStelObjectMgr().unSelect();

	Vec3d aim;
	double dRa = StelUtils::getDecAngle(ra); 
	double dDec = StelUtils::getDecAngle(dec);

	StelUtils::spheToRect(dRa,dDec,aim);
	mvmgr->moveTo(aim, duration, false);
}

QtScriptMgr::QtScriptMgr(const QString& startupScript, QObject *parent) 
	: QObject(parent), 
	  thread(NULL)
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
	
	runScript(startupScript);
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
		QRegExp descExp("^\\s*//\\s*Description:\\s*([^\\s].+)\\s*$");
		QRegExp descNewlineExp("^\\s*//\\s*$");
		QRegExp descContExp("^\\s*//\\s*([^\\s].*)\\s*$");
		while (!file.atEnd()) {
			QString line(file.readLine());
			
			if (!inDesc && descExp.exactMatch(line))
			{
				inDesc = true;
				desc = descExp.capturedTexts().at(1) + " ";
				desc.replace("\n","");
			}
			else if (inDesc)
			{
				QString d("");
				if (descNewlineExp.exactMatch(line))
					d = "\n";
				else if (descContExp.exactMatch(line))
				{
					d = descContExp.capturedTexts().at(1) + " ";
					d.replace("\n","");
				}
				else
				{
					file.close();	
					return desc;
				}
				desc += d;
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

