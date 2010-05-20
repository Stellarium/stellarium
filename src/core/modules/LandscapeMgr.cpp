/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#include <QDebug>
#include <QSettings>
#include <QString>

#ifdef USE_OPENGL_ES2
 #include "GLES2/gl2.h"
#else
 #include <QtOpenGL>
#endif

#include "LandscapeMgr.hpp"
#include "Landscape.hpp"
#include "Atmosphere.hpp"
#include "StelApp.hpp"
#include "SolarSystem.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "Planet.hpp"
#include "StelIniParser.hpp"
#include "StelSkyDrawer.hpp"
#include "StelPainter.hpp"

// Class which manages the cardinal points displaying
class Cardinals
{
public:
	Cardinals(float _radius = 1.);
	virtual ~Cardinals();
	void draw(const StelCore* core, double latitude) const;
	void setColor(const Vec3f& c) {color = c;}
	Vec3f get_color() {return color;}
	void updateI18n();
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void set_fade_duration(float duration) {fader.setDuration((int)(duration*1000.f));}
	void setFlagShow(bool b){fader = b;}
	bool getFlagShow() const {return fader;}
private:
	float radius;
	QFont font;
	Vec3f color;
	QString sNorth, sSouth, sEast, sWest;
	LinearFader fader;
};


Cardinals::Cardinals(float _radius) : radius(_radius), color(0.6,0.2,0.2)
{
	font.setPixelSize(30);
	// Default labels - if sky locale specified, loaded later
	// Improvement for gettext translation
	sNorth = "N";
	sSouth = "S";
	sEast = "E";
	sWest = "W";
}

Cardinals::~Cardinals()
{
}

// Draw the cardinals points : N S E W
// handles special cases at poles
void Cardinals::draw(const StelCore* core, double latitude) const
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	StelPainter sPainter(prj);
	sPainter.setFont(font);

	if (!fader.getInterstate()) return;

	// direction text
	QString d[4];

	d[0] = sNorth;
	d[1] = sSouth;
	d[2] = sEast;
	d[3] = sWest;

	// fun polar special cases
	if (latitude ==  90.0 ) d[0] = d[1] = d[2] = d[3] = sSouth;
	if (latitude == -90.0 ) d[0] = d[1] = d[2] = d[3] = sNorth;

	sPainter.setColor(color[0],color[1],color[2],fader.getInterstate());
	glEnable(GL_BLEND);
	sPainter.enableTexture2d(true);
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Vec3f pos;
	Vec3f xy;

	float shift = sPainter.getFontMetrics().width(sNorth)/2;

	// N for North
	pos.set(-1.f, 0.f, 0.f);
	if (prj->project(pos,xy)) sPainter.drawText(xy[0], xy[1], d[0], 0., -shift, -shift);

	// S for South
	pos.set(1.f, 0.f, 0.f);
	if (prj->project(pos,xy)) sPainter.drawText(xy[0], xy[1], d[1], 0., -shift, -shift);

	// E for East
	pos.set(0.f, 1.f, 0.f);
	if (prj->project(pos,xy)) sPainter.drawText(xy[0], xy[1], d[2], 0., -shift, -shift);

	// W for West
	pos.set(0.f, -1.f, 0.f);
	if (prj->project(pos,xy)) sPainter.drawText(xy[0], xy[1], d[3], 0., -shift, -shift);

}

// Translate cardinal labels with gettext to current sky language and update font for the language
void Cardinals::updateI18n()
{
	StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	sNorth = trans.qtranslate("N");
	sSouth = trans.qtranslate("S");
	sEast = trans.qtranslate("E");
	sWest = trans.qtranslate("W");
}


LandscapeMgr::LandscapeMgr() : atmosphere(NULL), cardinalsPoints(NULL), landscape(NULL), flagLandscapeSetsLocation(false)
{
	setObjectName("LandscapeMgr");
}

LandscapeMgr::~LandscapeMgr()
{
	delete atmosphere;
	delete cardinalsPoints;
	delete landscape;
	landscape = NULL;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double LandscapeMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MeteorMgr")->getCallOrder(actionName)+20;
	if (actionName==StelModule::ActionUpdate)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+10;
	return 0;
}

void LandscapeMgr::update(double deltaTime)
{
	atmosphere->update(deltaTime);
	landscape->update(deltaTime);
	cardinalsPoints->update(deltaTime);

	// Compute the atmosphere color and intensity
	// Compute the sun position in local coordinate
	SolarSystem* ssystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();

	Vec3d sunPos = ssystem->getSun()->getAltAzPos(nav);
	// Compute the moon position in local coordinate
	Vec3d moonPos = ssystem->getMoon()->getAltAzPos(nav);
	atmosphere->computeColor(nav->getJDay(), sunPos, moonPos,
		ssystem->getMoon()->getPhase(ssystem->getEarth()->getHeliocentricEclipticPos()),
		StelApp::getInstance().getCore(), nav->getCurrentLocation().latitude, nav->getCurrentLocation().altitude,
		15.f, 40.f);	// Temperature = 15c, relative humidity = 40%

	StelApp::getInstance().getCore()->getSkyDrawer()->reportLuminanceInFov(3.75+atmosphere->getAverageLuminance()*3.5, true);

	// Compute the ground luminance based on every planets around
//	float groundLuminance = 0;
//	const vector<Planet*>& allPlanets = ssystem->getAllPlanets();
//	for (vector<Planet*>::const_iterator i=allPlanets.begin();i!=allPlanets.end();++i)
//	{
//		Vec3d pos = (*i)->getAltAzPos(nav);
//		pos.normalize();
//		if (pos[2] <= 0)
//		{
//			// No need to take this body into the landscape illumination computation
//			// because it is under the horizon
//		}
//		else
//		{
//			// Compute the Illuminance E of the ground caused by the planet in lux = lumen/m^2
//			float E = pow10(((*i)->get_mag(nav)+13.988)/-2.5);
//			//qDebug() << "mag=" << (*i)->get_mag(nav) << " illum=" << E;
//			// Luminance in cd/m^2
//			groundLuminance += E/0.44*pos[2]*pos[2]; // 1m^2 from 1.5 m above the ground is 0.44 sr.
//		}
//	}
//	groundLuminance*=atmosphere->getFadeIntensity();
//	groundLuminance=atmosphere->getAverageLuminance()/50;
//	qDebug() << "Atmosphere lum=" << atmosphere->getAverageLuminance() << " ground lum=" <<  groundLuminance;
//	qDebug() << "Adapted Atmosphere lum=" << eye->adaptLuminance(atmosphere->getAverageLuminance()) << " Adapted ground lum=" << eye->adaptLuminance(groundLuminance);

	// compute global ground brightness in a simplistic way, directly in RGB
	float landscapeBrightness = 0;
	sunPos.normalize();
	moonPos.normalize();

	// We define the brigthness zero when the sun is 8 degrees below the horizon.
	float sinSunAngleRad = sin(qMin(M_PI_2, asin(sunPos[2])+8.*M_PI/180.));

	if(sinSunAngleRad < -0.1/1.5 )
		landscapeBrightness = 0.01;
	else
		landscapeBrightness = (0.01 + 1.5*(sinSunAngleRad+0.1/1.5));
	if (moonPos[2] > -0.1/1.5)
		landscapeBrightness += qMax(0.2/-12.*ssystem->getMoon()->getVMagnitude(nav),0.)*moonPos[2];

	// TODO make this more generic for non-atmosphere planets
	if(atmosphere->getFadeIntensity() == 1)
	{
		// If the atmosphere is on, a solar eclipse might darken the sky
		// otherwise we just use the sun position calculation above
		landscapeBrightness *= (atmosphere->getRealDisplayIntensityFactor()+0.1);
	}

	// TODO: should calculate dimming with solar eclipse even without atmosphere on
	landscape->setBrightness(landscapeBrightness+0.05);
}

void LandscapeMgr::draw(StelCore* core)
{
	// Draw the atmosphere
	atmosphere->draw(core);

	// Draw the landscape
	landscape->draw(core);

	// Draw the cardinal points
	cardinalsPoints->draw(core, StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation().latitude);
}

void LandscapeMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	atmosphere = new Atmosphere();
	landscape = new LandscapeOldStyle();
	defaultLandscapeID = conf->value("init_location/landscape_name").toString();
	setCurrentLandscapeID(defaultLandscapeID);
	setFlagLandscape(conf->value("landscape/flag_landscape", conf->value("landscape/flag_ground", true).toBool()).toBool());
	setFlagFog(conf->value("landscape/flag_fog",true).toBool());
	setFlagAtmosphere(conf->value("landscape/flag_atmosphere").toBool());
	setAtmosphereFadeDuration(conf->value("landscape/atmosphere_fade_duration",0.5).toFloat());
	setAtmosphereLightPollutionLuminance(conf->value("viewing/light_pollution_luminance",0.0).toFloat());
	cardinalsPoints = new Cardinals();
	cardinalsPoints->setFlagShow(conf->value("viewing/flag_cardinal_points",true).toBool());
	setFlagLandscapeSetsLocation(conf->value("landscape/flag_landscape_sets_location",false).toBool());

	bool ok =true;
	setAtmosphereBortleLightPollution(conf->value("stars/init_bortle_scale",3).toInt(&ok));
	if (!ok)
	{
		conf->setValue("stars/init_bortle_scale",3);
		setAtmosphereBortleLightPollution(3);
		ok = true;
	}
}

void LandscapeMgr::setStelStyle(const QString& section)
{
	// Load colors from config file
	QSettings* conf = StelApp::getInstance().getSettings();

	QString defaultColor = conf->value(section+"/default_color").toString();
	setColorCardinalPoints(StelUtils::strToVec3f(conf->value(section+"/cardinal_color", defaultColor).toString()));
}

bool LandscapeMgr::setCurrentLandscapeID(const QString& id)
{
	if (id.isEmpty())
		return false;

	// We want to lookup the landscape ID (dir) from the name.
	Landscape* newLandscape = NULL;
	try
	{
		newLandscape = createFromFile(StelFileMgr::findFile("landscapes/" + id + "/landscape.ini"), id);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR while loading default landscape " << "landscapes/" + id + "/landscape.ini" << ", (" << e.what() << ")";
	}

	if (!newLandscape)
		return false;

	if (landscape)
	{
		// Copy display parameters from previous landscape to new one
		newLandscape->setFlagShow(landscape->getFlagShow());
		newLandscape->setFlagShowFog(landscape->getFlagShowFog());
		delete landscape;
		landscape = newLandscape;
	}
	currentLandscapeID = id;

	if (getFlagLandscapeSetsLocation())
	{
		StelApp::getInstance().getCore()->getNavigator()->moveObserverTo(landscape->getLocation());
	}
	return true;
}

bool LandscapeMgr::setCurrentLandscapeName(const QString& name)
{
	if (name.isEmpty())
		return false;
	
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
	if (nameToDirMap.find(name)!=nameToDirMap.end())
	{
		return setCurrentLandscapeID(nameToDirMap[name]);
	}
	else
	{
		qWarning() << "Can't find a landscape with name=" << name << endl;
		return false;
	}
}

// Change the default landscape to the landscape with the ID specified.
bool LandscapeMgr::setDefaultLandscapeID(const QString& id)
{
	if (id.isEmpty())
		return false;
	defaultLandscapeID = id;
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("init_location/landscape_name", id);
	return true;
}

void LandscapeMgr::updateI18n()
{
	// Translate all labels with the new language
	if (cardinalsPoints) cardinalsPoints->updateI18n();
}

void LandscapeMgr::setFlagLandscape(bool b)
{
	landscape->setFlagShow(b);
}

bool LandscapeMgr::getFlagLandscape() const
{
	return landscape->getFlagShow();
}

void LandscapeMgr::setFlagFog(bool b)
{
	landscape->setFlagShowFog(b);
}

bool LandscapeMgr::getFlagFog() const
{
	return landscape->getFlagShowFog();
}

/*********************************************************************
 Retrieve list of the names of all the available landscapes
 *********************************************************************/
QStringList LandscapeMgr::getAllLandscapeNames() const
{
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
	QStringList result;

	// We just look over the map of names to IDs and extract the keys
	foreach (QString i, nameToDirMap.keys())
	{
		result += i;
	}
	return result;
}

QStringList LandscapeMgr::getAllLandscapeIDs() const
{
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
	QStringList result;

	// We just look over the map of names to IDs and extract the keys
	foreach (QString i, nameToDirMap.values())
	{
		result += i;
	}
	return result;
}

QString LandscapeMgr::getCurrentLandscapeName() const
{
	return landscape->getName();
}

QString LandscapeMgr::getCurrentLandscapeHtmlDescription() const
{
	QString desc = QString("<h3>%1</h3>").arg(landscape->getName());
	desc += landscape->getDescription();
	desc+="<br><br>";
	desc+="<b>"+q_("Author: ")+"</b>";
	desc+=landscape->getAuthorName();
	desc+="<br>";
	desc+="<b>"+q_("Location: ")+"</b>";
	if (landscape->getLocation().longitude>-500.0 && landscape->getLocation().latitude>-500.0)
	{
		desc += StelUtils::radToDmsStrAdapt(landscape->getLocation().longitude * M_PI/180.);
		desc += "/" + StelUtils::radToDmsStrAdapt(landscape->getLocation().latitude *M_PI/180.);
		desc += QString(q_(", %1 m")).arg(landscape->getLocation().altitude);
		if (landscape->getLocation().planetName!="")
		{
			desc += "<br><b>"+q_("Planet: ")+"</b>"+landscape->getLocation().planetName;
		}
		desc += "<br><br>";
	}
	return desc;
}

//! Set flag for displaying Cardinals Points
void LandscapeMgr::setFlagCardinalsPoints(bool b)
{
	cardinalsPoints->setFlagShow(b);
}

//! Get flag for displaying Cardinals Points
bool LandscapeMgr::getFlagCardinalsPoints() const
{
	return cardinalsPoints->getFlagShow();
}

//! Set Cardinals Points color
void LandscapeMgr::setColorCardinalPoints(const Vec3f& v)
{
	cardinalsPoints->setColor(v);
}

//! Get Cardinals Points color
Vec3f LandscapeMgr::getColorCardinalPoints() const
{
	return cardinalsPoints->get_color();
}

///////////////////////////////////////////////////////////////////////////////////////
// Atmosphere
//! Set flag for displaying Atmosphere
void LandscapeMgr::setFlagAtmosphere(bool b)
{
	atmosphere->setFlagShow(b);
	StelApp::getInstance().getCore()->getSkyDrawer()->setFlagHasAtmosphere(b);
}

//! Get flag for displaying Atmosphere
bool LandscapeMgr::getFlagAtmosphere() const
{
	return atmosphere->getFlagShow();
}

//! Set atmosphere fade duration in s
void LandscapeMgr::setAtmosphereFadeDuration(float f)
{
	atmosphere->setFadeDuration(f);
}

//! Get atmosphere fade duration in s
float LandscapeMgr::getAtmosphereFadeDuration() const
{
	return atmosphere->getFadeDuration();
}

//! Set light pollution luminance level
void LandscapeMgr::setAtmosphereLightPollutionLuminance(float f)
{
	atmosphere->setLightPollutionLuminance(f);
}

//! Get light pollution luminance level
float LandscapeMgr::getAtmosphereLightPollutionLuminance() const
{
	return atmosphere->getLightPollutionLuminance();
}

//! Set the light pollution following the Bortle Scale
void LandscapeMgr::setAtmosphereBortleLightPollution(int bIndex)
{
	// This is an empirical formula
	setAtmosphereLightPollutionLuminance(qMax(0.,0.0020*std::pow(bIndex-1, 2.1)));
}

//! Get the light pollution following the Bortle Scale
int LandscapeMgr::getAtmosphereBortleLightPollution()
{
	return (int)std::pow(getAtmosphereLightPollutionLuminance()/0.0020, 1./2.1) + 1;
}

void LandscapeMgr::setZRotation(float d)
{
	if (landscape)
		landscape->setZRotation(d);
}

float LandscapeMgr::getLuminance()
{
	return atmosphere->getRealDisplayIntensityFactor();
}

Landscape* LandscapeMgr::createFromFile(const QString& landscapeFile, const QString& landscapeId)
{
	QSettings landscapeIni(landscapeFile, StelIniFormat);
	QString s;
	if (landscapeIni.status() != QSettings::NoError)
	{
		qWarning() << "ERROR parsing landscape.ini file: " << landscapeFile;
		s = "";
	}
	else
		s = landscapeIni.value("landscape/type").toString();

	Landscape* ldscp = NULL;
	if (s=="old_style")
		ldscp = new LandscapeOldStyle();
	else if (s=="spherical")
		ldscp = new LandscapeSpherical();
	else if (s=="fisheye")
		ldscp = new LandscapeFisheye();
	else
	{
		qDebug() << "Unknown landscape type: \"" << s << "\"";

		// to avoid making this a fatal error, will load as a fisheye
		// if this fails, it just won't draw
		ldscp = new LandscapeFisheye();
	}

	ldscp->load(landscapeIni, landscapeId);
	return ldscp;
}


QString LandscapeMgr::nameToID(const QString& name)
{
	QMap<QString,QString> nameToDirMap = getNameToDirMap();

	if (nameToDirMap.find(name)!=nameToDirMap.end())
	{
		Q_ASSERT(0);
		return "error";
	}
	else
	{
		return nameToDirMap[name];
	}
}

/****************************************************************************
 get a map of landscape name (from landscape.ini name field) to ID (dir name)
 ****************************************************************************/
QMap<QString,QString> LandscapeMgr::getNameToDirMap() const
{
	QSet<QString> landscapeDirs;
	QMap<QString,QString> result;
	try
	{
		landscapeDirs = StelFileMgr::listContents("landscapes",StelFileMgr::Directory);
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "ERROR while trying list landscapes:" << e.what();
	}

	foreach (const QString& dir, landscapeDirs)
	{
		try
		{
			QSettings landscapeIni(StelFileMgr::findFile("landscapes/" + dir + "/landscape.ini"), StelIniFormat);
			QString k = landscapeIni.value("landscape/name").toString();
			result[k] = dir;
		}
		catch (std::runtime_error& e)
		{
			//qDebug << "WARNING: unable to successfully read landscape.ini file from landscape " << dir;
		}
	}
	return result;
}


