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

#include "StelScriptMgr.hpp"
#include "StelAudioMgr.hpp"
#include "ConstellationMgr.hpp"
#include "GridLinesMgr.hpp"
#include "LandscapeMgr.hpp"
#include "MeteorMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelNavigator.hpp"
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
#include "StelMainWindow.hpp"
#include "StelSkyDrawer.hpp"
#include "StelSkyImageMgr.hpp"
#include "StarMgr.hpp"
#include "StelProjector.hpp"
#include "StelLocation.hpp"
#include "Planet.hpp"
#include "StelLocationMgr.hpp"

#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QDebug>
#include <QStringList>
#include <QDateTime>
#include <QRegExp>
#include <QDir>
#include <QTemporaryFile>

#include <cmath>

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

			// For startup scripts, the gui object might not 
			// have completed init when we run. Wait for that.
			StelGui* gui = GETSTELMODULE(StelGui);
			while(!gui)
			{
				msleep(200);
				gui = GETSTELMODULE(StelGui);
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

ScriptSleeper::ScriptSleeper()
{
	scriptRate = 1.0;
}

ScriptSleeper::~ScriptSleeper()
{
}

void ScriptSleeper::setRate(double newRate)
{
	scriptRate = newRate;
}

double ScriptSleeper::getRate(void) const
{
	return scriptRate;
}

void ScriptSleeper::sleep(int ms)
{
	scriptRateOnSleep = scriptRate;
	sleptTime.start();
	int sleepForTime = ms/scriptRate;
	while(sleepForTime > sleptTime.elapsed())
	{
		msleep(10);
		if (scriptRateOnSleep!=scriptRate)
		{
			int sleepLeft = sleepForTime - sleptTime.elapsed();
			sleepForTime = sleptTime.elapsed() + (sleepLeft / scriptRate);
			scriptRateOnSleep = scriptRate;
		}
	}
}

StelMainScriptAPI::StelMainScriptAPI(QObject *parent) : QObject(parent)
{
	if(StelSkyImageMgr* smgr = GETSTELMODULE(StelSkyImageMgr))
	{
		connect(this, SIGNAL(requestLoadSkyImage(const QString&, const QString&, double, double, double, double, double, double, double, double, double, double, bool)), smgr, SLOT(loadSkyImage(const QString&, const QString&, double, double, double, double, double, double, double, double, double, double, bool)));
		connect(this, SIGNAL(requestRemoveSkyImage(const QString&)), smgr, SLOT(removeSkyImage(const QString&)));
	}
	
	connect(this, SIGNAL(requestLoadSound(const QString&, const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(loadSound(const QString&, const QString&)));
	connect(this, SIGNAL(requestPlaySound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(playSound(const QString&)));
	connect(this, SIGNAL(requestPauseSound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(pauseSound(const QString&)));
	connect(this, SIGNAL(requestStopSound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(stopSound(const QString&)));
	connect(this, SIGNAL(requestDropSound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(dropSound(const QString&)));
	connect(this, SIGNAL(requestExit()), this->parent(), SLOT(stopScript()));
}

StelMainScriptAPI::~StelMainScriptAPI()
{
}

ScriptSleeper& StelMainScriptAPI::getScriptSleeper(void)
{
	return scriptSleeper;
}

//! Set the current date in Julian Day
//! @param JD the Julian Date
void StelMainScriptAPI::setJDay(double JD)
{
	StelApp::getInstance().getCore()->getNavigator()->setJDay(JD);
}

//! Get the current date in Julian Day
//! @return the Julian Date
double StelMainScriptAPI::getJDay(void) const
{
	return StelApp::getInstance().getCore()->getNavigator()->getJDay();
}

void StelMainScriptAPI::setDate(const QString& dt, const QString& spec)
{
	StelApp::getInstance().getCore()->getNavigator()->setJDay(jdFromDateString(dt, spec));
}

//! Set time speed in JDay/sec
//! @param ts time speed in JDay/sec
void StelMainScriptAPI::setTimeRate(double ts)
{
	// 1 second = .00001157407407407407 JDay
	StelApp::getInstance().getCore()->getNavigator()->setTimeRate(ts * 0.00001157407407407407 * scriptSleeper.getRate());
}

//! Get time speed in JDay/sec
//! @return time speed in JDay/sec
double StelMainScriptAPI::getTimeRate(void) const
{
	return StelApp::getInstance().getCore()->getNavigator()->getTimeRate() / (0.00001157407407407407 * scriptSleeper.getRate());
}

void StelMainScriptAPI::wait(double t)
{
	scriptSleeper.sleep(t*1000);
}

void StelMainScriptAPI::waitFor(const QString& dt, const QString& spec)
{
	double JD = jdFromDateString(dt, spec);
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();
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
			scriptSleeper.sleep(200);
	}
	else
	{
		while(nav->getJDay() > JD)
			scriptSleeper.sleep(200);
	}
}

void StelMainScriptAPI::setObserverLocation(double longitude, double latitude, double altitude, double duration, const QString& name, const QString& planet)
{
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();
	Q_ASSERT(nav);
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	Q_ASSERT(ssmgr);

	StelLocation loc = nav->getCurrentLocation();
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
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();
	Q_ASSERT(nav);
	StelLocation loc = StelApp::getInstance().getLocationMgr().locationForSmallString(id);
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
	GETSTELMODULE(StelGui)->getGuiActions("actionToggle_GuiHidden_Global")->setChecked(b);
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

void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
                                     const QString& ra0, const QString& dec0,
                                     const QString& ra1, const QString& dec1,
                                     const QString& ra2, const QString& dec2,
                                     const QString& ra3, const QString& dec3,
                                     double minRes, double maxBright, bool visible)
{
	loadSkyImage(id, filename,
	             StelUtils::getDecAngle(ra0) *180./M_PI, StelUtils::getDecAngle(dec0)*180./M_PI,
	             StelUtils::getDecAngle(ra1) *180./M_PI, StelUtils::getDecAngle(dec1)*180./M_PI,
	             StelUtils::getDecAngle(ra2) *180./M_PI, StelUtils::getDecAngle(dec2)*180./M_PI,
	             StelUtils::getDecAngle(ra3) *180./M_PI, StelUtils::getDecAngle(dec3)*180./M_PI,
	             minRes, maxBright, visible);
}

void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
                                     double ra, double dec, double angSize, double rotation,
                                     double minRes, double maxBright, bool visible)
{
	Vec3d XYZ;
	const double RADIUS_NEB = 1.;
	StelUtils::spheToRect(ra*M_PI/180., dec*M_PI/180., XYZ);
	XYZ*=RADIUS_NEB;
	double texSize = RADIUS_NEB * sin(angSize/2/60*M_PI/180);
	Mat4f matPrecomp = Mat4f::translation(XYZ) *
	                   Mat4f::zrotation(ra*M_PI/180.) *
	                   Mat4f::yrotation(-dec*M_PI/180.) *
	                   Mat4f::xrotation(rotation*M_PI/180.);

	Vec3d corners[4];
        corners[0] = matPrecomp * Vec3d(0.,-texSize,-texSize); 
        corners[1] = matPrecomp * Vec3d(0., texSize,-texSize);
        corners[2] = matPrecomp * Vec3d(0.,-texSize, texSize);
        corners[3] = matPrecomp * Vec3d(0., texSize, texSize);

	// convert back to ra/dec (radians)
	Vec3d cornersRaDec[4];
	for(int i=0; i<4; i++)
		StelUtils::rectToSphe(&cornersRaDec[i][0], &cornersRaDec[i][1], corners[i]);

	loadSkyImage(id, filename, 
	             cornersRaDec[0][0]*180./M_PI, cornersRaDec[0][1]*180./M_PI,
	             cornersRaDec[1][0]*180./M_PI, cornersRaDec[1][1]*180./M_PI,
	             cornersRaDec[3][0]*180./M_PI, cornersRaDec[3][1]*180./M_PI,
	             cornersRaDec[2][0]*180./M_PI, cornersRaDec[2][1]*180./M_PI,
	             minRes, maxBright, visible);
}

void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
                                     const QString& ra, const QString& dec, double angSize, double rotation,
                                     double minRes, double maxBright, bool visible)
{
	loadSkyImage(id, filename, StelUtils::getDecAngle(ra)*180./M_PI, 
	             StelUtils::getDecAngle(dec)*180./M_PI, angSize, 
	             rotation, minRes, maxBright, visible);
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

int StelMainScriptAPI::getScreenWidth(void)
{
	return StelMainWindow::getInstance().size().width();
}

int StelMainScriptAPI::getScreenHeight(void)
{
	return StelMainWindow::getInstance().size().height();
}

double StelMainScriptAPI::getScriptRate(void)
{
	return scriptSleeper.getRate();
}

void StelMainScriptAPI::setScriptRate(double r)
{
	return scriptSleeper.setRate(r);
}

void StelMainScriptAPI::exit(void)
{
	emit(requestExit());
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
	QRegExp isoRe("^\\d{4}[:\\-]\\d\\d[:\\-]\\d\\dT\\d?\\d:\\d\\d:\\d\\d$");
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
			JD = StelApp::getInstance().getCore()->getNavigator()->getJDay();

		if (nowRe.capturedTexts().at(8) == "sidereal")
			dayLength = StelApp::getInstance().getCore()->getNavigator()->getHomePlanet()->getSiderealDay();

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
		qWarning() << "StelMainScriptAPI::jdFromDateString error - date string" << dt << "not recognised, returning \"now\"";
		return StelUtils::getJDFromSystem();
	}
}

void StelMainScriptAPI::selectObjectByName(const QString& name, bool pointer)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	omgr->setFlagSelectedObjectPointer(pointer);
	if (name=="")
		omgr->unSelect();
	else
		omgr->findAndSelect(name);
}

void StelMainScriptAPI::clear(const QString& state)
{
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	Q_ASSERT(lmgr);
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	Q_ASSERT(ssmgr);
	MeteorMgr* mmgr = GETSTELMODULE(MeteorMgr);
	Q_ASSERT(mmgr);
	StelSkyDrawer* skyd = StelApp::getInstance().getCore()->getSkyDrawer();
	Q_ASSERT(skyd);
	ConstellationMgr* cmgr = GETSTELMODULE(ConstellationMgr);
	Q_ASSERT(cmgr);
	StarMgr* smgr = GETSTELMODULE(StarMgr);
	Q_ASSERT(smgr);
	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	Q_ASSERT(nmgr);
	GridLinesMgr* glmgr = GETSTELMODULE(GridLinesMgr);
	Q_ASSERT(glmgr);
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();
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

double StelMainScriptAPI::getViewAltitudeAngle()
{
	Vec3d current = StelApp::getInstance().getCore()->getNavigator()->getAltAzVisionDirection();
	double alt, azi;
	StelUtils::rectToSphe(&azi, &alt, current);
	return alt*180/M_PI; // convert to degrees from radians
}

double StelMainScriptAPI::getViewAzimuthAngle()
{
	Vec3d current = StelApp::getInstance().getCore()->getNavigator()->getAltAzVisionDirection();
	double alt, azi;
	StelUtils::rectToSphe(&azi, &alt, current);
	// The returned azimuth angle is in radians and set up such that:
	// N=+/-PI; E=PI/2; S=0; W=-PI/2;
	// But we want compass bearings, i.e. N=0, E=90, S=180, W=270
	return std::fmod(((azi*180/M_PI)*-1)+180., 360.);
}

double StelMainScriptAPI::getViewRaAngle()
{
	Vec3d current = StelApp::getInstance().getCore()->getNavigator()->getEquinoxEquVisionDirection();
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, current);
	// returned RA angle is in range -PI .. PI, but we want 0 .. 360
	return std::fmod((ra*180/M_PI)+360., 360.); // convert to degrees from radians
}

double StelMainScriptAPI::getViewDecAngle()
{
	Vec3d current = StelApp::getInstance().getCore()->getNavigator()->getEquinoxEquVisionDirection();
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, current);
	return dec*180/M_PI; // convert to degrees from radians
}

double StelMainScriptAPI::getViewRaJ2000Angle()
{
	Vec3d current = StelApp::getInstance().getCore()->getNavigator()->getJ2000EquVisionDirection();
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, current);
	// returned RA angle is in range -PI .. PI, but we want 0 .. 360
	return std::fmod((ra*180/M_PI)+360., 360.); // convert to degrees from radians
}

double StelMainScriptAPI::getViewDecJ2000Angle()
{
	Vec3d current = StelApp::getInstance().getCore()->getNavigator()->getJ2000EquVisionDirection();
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, current);
	return dec*180/M_PI; // convert to degrees from radians
}

void StelMainScriptAPI::moveToAltAzi(const QString& alt, const QString& azi, float duration)
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);

	GETSTELMODULE(StelObjectMgr)->unSelect();

	Vec3d aim;
	double dAlt = StelUtils::getDecAngle(alt); 
	double dAzi = M_PI - StelUtils::getDecAngle(azi);

	StelUtils::spheToRect(dAzi,dAlt,aim);
	mvmgr->moveTo(aim, duration, true);
}

void StelMainScriptAPI::moveToRaDec(const QString& ra, const QString& dec, float duration)
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);

	GETSTELMODULE(StelObjectMgr)->unSelect();

	Vec3d aim;
	double dRa = StelUtils::getDecAngle(ra); 
	double dDec = StelUtils::getDecAngle(dec);

	StelUtils::spheToRect(dRa,dDec,aim);
	mvmgr->moveTo(aim, duration, false);
}

StelScriptMgr::StelScriptMgr(QObject *parent) 
	: QObject(parent), 
	  thread(NULL)
{
	// Allow Vec3f managment in scripts
	qScriptRegisterMetaType(&engine, vec3fToScriptValue, vec3fFromScriptValue);
	// Constructor
	QScriptValue ctor = engine.newFunction(createVec3f);
	engine.globalObject().setProperty("Vec3f", ctor);
	
	// Add the core object to access methods related to core
	mainAPI = new StelMainScriptAPI(this);
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
	if(StelSkyImageMgr* smgr = GETSTELMODULE(StelSkyImageMgr))
		objectValue = engine.newQObject(smgr);
}


StelScriptMgr::~StelScriptMgr()
{
}

QStringList StelScriptMgr::getScriptList(void)
{
	QStringList scriptFiles;
	try
	{
		StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
		QSet<QString> files = fileMan.listContents("scripts",StelFileMgr::File, true);
		foreach(QString f, files)
		{
#ifdef ENABLE_STRATOSCRIPT_COMPAT
			QRegExp fileRE("^.*\\.(ssc|sts)$");
#else // ENABLE_STRATOSCRIPT_COMPAT
			QRegExp fileRE("^.*\\.ssc$");
#endif // ENABLE_STRATOSCRIPT_COMPAT
			if (fileRE.exactMatch(f))
				scriptFiles << f;
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: could not list scripts:" << e.what();
	}
	return scriptFiles;
}

bool StelScriptMgr::scriptIsRunning(void)
{
	return (thread != NULL);
}

QString StelScriptMgr::runningScriptId(void)
{
	if (thread)
		return thread->getFileName();
	else
		return QString();
}

const QString StelScriptMgr::getHeaderSingleLineCommentText(const QString& s, const QString& id, const QString& notFoundText)
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

const QString StelScriptMgr::getName(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "Name", s);
}

const QString StelScriptMgr::getAuthor(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "Author");
}

const QString StelScriptMgr::getLicense(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "License", "");
}

const QString StelScriptMgr::getDescription(const QString& s)
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
bool StelScriptMgr::runScript(const QString& fileName)
{
	if (thread!=NULL)
	{
		qWarning() << "ERROR: there is already a script running, please wait that it's over.";
		return false;
	}
	QString absPath;
	QString scriptDir;
	try
	{
		absPath = StelApp::getInstance().getFileMgr().findFile("scripts/" + fileName);
		scriptDir = QFileInfo(absPath).dir().path();
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: could not find script file " << fileName << ": " << e.what();
		return false;
	}
	// pre-process the script into a temporary file
	QTemporaryFile tmpFile;
	bool ok = false;
	if (!tmpFile.open())
	{
		qWarning() << "WARNING: cannot create temporary file for script pre-processing";
		return false;
	}
	QFile fic(absPath);
	if (!fic.open(QIODevice::ReadOnly))
	{
		qWarning() << "WARNING: cannot open script:" << fileName;
		tmpFile.close();
		return false;
	}

	if (fileName.right(4) == ".ssc")
		ok = preprocessScript(fic, tmpFile, scriptDir);
#ifdef ENABLE_STRATOSCRIPT_COMPAT
	else if (fileName.right(4) == ".sts")
		ok = preprocessStratoScript(fic, tmpFile, scriptDir);
#endif

	fic.close();

	if (ok==false)
	{
		tmpFile.close();
		return false;
	}

	tmpFile.seek(0);
	thread = new StelScriptThread(QTextStream(&tmpFile).readAll(), &engine, fileName);
	tmpFile.close();
	
	GETSTELMODULE(StelGui)->setScriptKeys(true);
	connect(thread, SIGNAL(finished()), this, SLOT(scriptEnded()));
	thread->start();
	emit(scriptRunning());
	return true;
}

bool StelScriptMgr::stopScript(void)
{
	if (thread)
	{
		qDebug() << "asking running script to exit";
		thread->terminate();
		return true;
	}
	else
	{
		qWarning() << "StelScriptMgr::stopScript - no script is running";
		return false;
	}
}

void StelScriptMgr::setScriptRate(double r)
{
	// pre-calculate the new time rate in an effort to prevent there being much latency
	// between setting the script rate and the time rate.
	double factor = r / mainAPI->getScriptSleeper().getRate();
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();
	double newTimeRate = nav->getTimeRate() * factor;

	mainAPI->getScriptSleeper().setRate(r);
	if (scriptIsRunning()) nav->setTimeRate(newTimeRate);
}

double StelScriptMgr::getScriptRate(void)
{
	return mainAPI->getScriptSleeper().getRate();
}

void StelScriptMgr::scriptEnded()
{
	GETSTELMODULE(StelGui)->setScriptKeys(false);
	mainAPI->getScriptSleeper().setRate(1);
	thread->deleteLater();
	thread=NULL;
	if (engine.hasUncaughtException())
	{
		qWarning() << "Script error: " << engine.uncaughtException().toString() << "@ line" << engine.uncaughtExceptionLineNumber();
	}
	emit(scriptStopped());
}

