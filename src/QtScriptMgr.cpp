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
#include "AudioMgr.hpp"
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
#include "StelMainGraphicsView.hpp"
#include "SkyDrawer.hpp"
#include "SkyImageMgr.hpp"
#include "StarMgr.hpp"
#include "Projector.hpp"
#include "Location.hpp"
#include "Planet.hpp"
#include "LocationMgr.hpp"

#include <QFile>
#include <QSet>
#include <QDebug>
#include <QStringList>
#include <QDateTime>

Q_DECLARE_METATYPE(Vec3f);

class StelScriptThread : public QThread
{
	public:
		StelScriptThread(const QString& ascriptCode, QScriptEngine* aengine, QString fileName) : scriptCode(ascriptCode), engine(aengine), fname(fileName) {;}
		QString getFileName() {return fname;}
		
	protected:
		void run()
		{
			// seed the QT PRNG
			qsrand(QDateTime::currentDateTime().toTime_t());

			// Stop Phonon from complaining
			QCoreApplication::setApplicationName("stellariumscript");

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
	connect(this, SIGNAL(requestLoadSkyImage(const QString&, const QString&, double, double, double, double, double, double, double, double, double, double, bool)), &StelApp::getInstance().getSkyImageMgr(), SLOT(loadSkyImage(const QString&, const QString&, double, double, double, double, double, double, double, double, double, double, bool)));

	connect(this, SIGNAL(requestRemoveSkyImage(const QString&)), &StelApp::getInstance().getSkyImageMgr(), SLOT(removeSkyImage(const QString&)));

	connect(this, SIGNAL(requestLoadSound(const QString&, const QString&)), StelApp::getInstance().getAudioMgr(), SLOT(loadSound(const QString&, const QString&)));
	connect(this, SIGNAL(requestPlaySound(const QString&)), StelApp::getInstance().getAudioMgr(), SLOT(playSound(const QString&)));
	connect(this, SIGNAL(requestPauseSound(const QString&)), StelApp::getInstance().getAudioMgr(), SLOT(playSound(const QString&)));
	connect(this, SIGNAL(requestStopSound(const QString&)), StelApp::getInstance().getAudioMgr(), SLOT(playSound(const QString&)));
	connect(this, SIGNAL(requestDropSound(const QString&)), StelApp::getInstance().getAudioMgr(), SLOT(playSound(const QString&)));
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
	StelApp::getInstance().getCore()->getNavigation()->setJDay(jdFromDateString(dt, spec));
}

//! Set time speed in JDay/sec
//! @param ts time speed in JDay/sec
void StelMainScriptAPI::setTimeRate(double ts)
{
	// 1 second = .00001157407407407407 JDay
	StelApp::getInstance().getCore()->getNavigation()->setTimeRate(ts * 0.00001157407407407407);
}

//! Get time speed in JDay/sec
//! @return time speed in JDay/sec
double StelMainScriptAPI::getTimeRate(void) const
{
	return StelApp::getInstance().getCore()->getNavigation()->getTimeRate() / 0.00001157407407407407;
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
	MySleep::msleep((unsigned long )(t*1000));
}

void StelMainScriptAPI::waitFor(const QString& dt, const QString& spec)
{
	double JD = jdFromDateString(dt, spec);
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	Q_ASSERT(nav);
	double timeSpeed = nav->getTimeRate();

	if (timeSpeed == 0.)
	{
		qWarning() << "waitFor called with no time passing - would be infinite. not waiting!";
		return;
	}
	else if (timeSpeed > 0)
	{
		while(nav->getJDay() < JD)
			MySleep::msleep(50);
	}
	else
	{
		while(nav->getJDay() > JD)
			MySleep::msleep(50);
	}
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

void StelMainScriptAPI::screenshot(const QString& prefix, bool invert, const QString& dir)
{
	bool oldInvertSetting = StelMainGraphicsView::getInstance().getFlagInvertScreenShotColors();
	StelMainGraphicsView::getInstance().setFlagInvertScreenShotColors(invert);
	StelMainGraphicsView::getInstance().saveScreenShot(prefix, dir);
	StelMainGraphicsView::getInstance().setFlagInvertScreenShotColors(oldInvertSetting);
}

void StelMainScriptAPI::setHideGui(bool b)
{
	StelGui* gui = (StelGui*)GETSTELMODULE("StelGui");
	Q_ASSERT(gui);
	gui->setHideGui(b);
}

void StelMainScriptAPI::setMinFps(float m) 
{
	StelApp::getInstance().setMinFps(m);
}

float StelMainScriptAPI::getMinFps() 
{
	return StelApp::getInstance().getMinFps();
}

void StelMainScriptAPI::setMaxFps(float m) 
{
	StelApp::getInstance().setMaxFps(m);
}

float StelMainScriptAPI::getMaxFps() 
{
	return StelApp::getInstance().getMaxFps();
}

void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
			             double ra0, double dec0,
			             double ra1, double dec1,
			             double ra2, double dec2,
			             double ra3, double dec3,
	                             double minRes, double maxBright, bool visible)
{
	QString path = "scripts/" + filename;
	emit(requestLoadSkyImage(id, path, ra0, dec0, ra1, dec1, ra2, dec2, ra3, dec3, minRes, maxBright, visible));
}

void StelMainScriptAPI::removeSkyImage(const QString& id)
{
	emit(requestRemoveSkyImage(id));
}

void StelMainScriptAPI::loadSound(const QString& filename, const QString& id)
{
	QString path;
	try
	{
		path = StelApp::getInstance().getFileMgr().findFile("scripts/" + filename);
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "cannot play sound" << filename << ":" << e.what();
		return;
	}

	emit(requestLoadSound(path, id));
}

void StelMainScriptAPI::playSound(const QString& id)
{
	emit(requestPlaySound(id));
}

void StelMainScriptAPI::pauseSound(const QString& id)
{
	emit(requestPauseSound(id));
}

void StelMainScriptAPI::stopSound(const QString& id)
{
	emit(requestStopSound(id));
}

void StelMainScriptAPI::dropSound(const QString& id)
{
	emit(requestDropSound(id));
}

void StelMainScriptAPI::debug(const QString& s)
{
	qDebug() << "script: " << s;
}

double StelMainScriptAPI::jdFromDateString(const QString& dt, const QString& spec)
{
	QDateTime qdt;
	double JD;

	// 2008-03-24T13:21:01
	QRegExp isoRe("^\\d{4}[:\\-]\\d\\d[:\\-]\\d\\dT\\d\\d:\\d\\d:\\d\\d$");
	QRegExp nowRe("^(now)?(\\s*([+\\-])\\s*(\\d+(\\.\\d+)?)\\s*(second|seconds|minute|minutes|hour|hours|day|days|week|weeks))(\\s+(sidereal)?)?");

	if (dt == "now")
		return StelUtils::getJDFromSystem();
	else if (isoRe.exactMatch(dt))
	{
		qdt = QDateTime::fromString(dt, Qt::ISODate);

		if (spec=="local")
			JD = StelUtils::qDateTimeToJd(qdt.toUTC());
		else
			JD = StelUtils::qDateTimeToJd(qdt);
			
		return JD;
	}
	else if (nowRe.exactMatch(dt))
	{
		double delta;
		double unit;
		double dayLength = 1.0;

		if (nowRe.capturedTexts().at(1)=="now")
			JD = StelUtils::getJDFromSystem();
		else
			JD = StelApp::getInstance().getCore()->getNavigation()->getJDay();

		if (nowRe.capturedTexts().at(8) == "sidereal")
			dayLength = StelApp::getInstance().getCore()->getNavigation()->getHomePlanet()->getSiderealDay();

		QString unitString = nowRe.capturedTexts().at(6);
		if (unitString == "seconds" || unitString == "second")
			unit = dayLength / (24*3600.);
		else if (unitString == "minutes" || unitString == "minute")
			unit = dayLength / (24*60.);
		else if (unitString == "hours" || unitString == "hour")
			unit = dayLength / (24.);
		else if (unitString == "days" || unitString == "day")
			unit = dayLength;
		else if (unitString == "weeks" || unitString == "week")
			unit = dayLength * 7.;
		else
		{
			qWarning() << "StelMainScriptAPI::setDate - unknown time unit:" << nowRe.capturedTexts().at(4);
			unit = 0;
		}

		delta = nowRe.capturedTexts().at(4).toDouble();

		if (nowRe.capturedTexts().at(3) == "+")
			JD += (unit * delta);
		else if (nowRe.capturedTexts().at(3) == "-")
			JD -= (unit * delta);

		return JD;
	}
	else
	{
		qWarning() << "StelMainScriptAPI::jdFromDateString error - date string not recognised, returning \"now\"" << dt;
		return StelUtils::getJDFromSystem();
	}
}

void StelMainScriptAPI::selectObjectByName(const QString& name, bool pointer)
{
	StelApp::getInstance().getStelObjectMgr().setFlagSelectedObjectPointer(pointer);
	if (name=="")
		StelApp::getInstance().getStelObjectMgr().unSelect();
	else
		StelApp::getInstance().getStelObjectMgr().findAndSelect(name);
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
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	Q_ASSERT(nav);

	if (state.toLower() == "natural")
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
	else if (state.toLower() == "starchart")
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
	else if (state.toLower() == "deepspace")
	{
		nav->setEquatorialMount(true);
		skyd->setFlagTwinkle(false);
		skyd->setFlagLuminanceAdaptation(false);
		ssmgr->setFlagPlanets(false);
		ssmgr->setFlagHints(false);
		ssmgr->setFlagOrbits(false);
		ssmgr->setFlagMoonScale(false);
		ssmgr->setFlagTrails(false);
		mmgr->setZHR(0);
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

	// Add other classes which we want to be directly accessible from scripts
	objectValue = engine.newQObject(&StelApp::getInstance().getSkyImageMgr());
	
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
		QSet<QString> files = fileMan.listContents("scripts",StelFileMgr::File, true);
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

const QString QtScriptMgr::getHeaderSingleLineCommentText(const QString& s, const QString& id, const QString& notFoundText)
{
	try
	{
		QFile file(StelApp::getInstance().getFileMgr().findFile("scripts/" + s, StelFileMgr::File));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qWarning() << "script file " << s << " could not be opened for reading";
			return QString();
		}

		QRegExp nameExp("^\\s*//\\s*" + id + ":\\s*(.+)$");
		while (!file.atEnd()) {
			QString line(file.readLine());
			if (nameExp.exactMatch(line))
			{
				file.close();	
				return nameExp.capturedTexts().at(1);
			}
		}
		file.close();
		return notFoundText;
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "script file " << s << " could not be found:" << e.what();
		return QString();
	}
}

const QString QtScriptMgr::getName(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "Name", s);
}

const QString QtScriptMgr::getAuthor(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "Author");
}

const QString QtScriptMgr::getLicense(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "License", "");
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
		qWarning() << "Script error: " << engine.uncaughtException().toString() << "@ line" << engine.uncaughtExceptionLineNumber();
	}
	emit(scriptStopped());
}

