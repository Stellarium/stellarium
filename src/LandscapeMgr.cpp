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

#include "LandscapeMgr.hpp"
#include "Landscape.hpp"
#include "Atmosphere.hpp"
#include "StelApp.hpp"
#include "SolarSystem.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "InitParser.hpp"
#include "Planet.hpp"
#include "Observer.hpp"

// Class which manages the cardinal points displaying
class Cardinals
{
public:
	Cardinals(float _radius = 1.);
    virtual ~Cardinals();
	void draw(const Projector* prj, double latitude, bool gravityON = false) const;
	void setColor(const Vec3f& c) {color = c;}
	Vec3f get_color() {return color;}
	void updateI18n();
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void setFlagShow(bool b){fader = b;}
	bool getFlagShow(void) const {return fader;}
private:
	float radius;
	double fontSize;
	SFont& font;
	Vec3f color;
	wstring sNorth, sSouth, sEast, sWest;
	LinearFader fader;
};


Cardinals::Cardinals(float _radius) : radius(_radius), fontSize(30),
font(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize)), color(0.6,0.2,0.2)
{
	// Default labels - if sky locale specified, loaded later
	// Improvement for gettext translation
	sNorth = L"N";
	sSouth = L"S";
	sEast = L"E";
	sWest = L"W";
}

Cardinals::~Cardinals()
{
}

// Draw the cardinals points : N S E W
// handles special cases at poles
void Cardinals::draw(const Projector* prj, double latitude, bool gravityON) const
{
	if (!fader.getInterstate()) return;

	// direction text
	wstring d[4];
	
	d[0] = sNorth;
	d[1] = sSouth;
	d[2] = sEast;
	d[3] = sWest;
	
	// fun polar special cases
	if(latitude ==  90.0 ) d[0] = d[1] = d[2] = d[3] = sSouth;
	if(latitude == -90.0 ) d[0] = d[1] = d[2] = d[3] = sNorth;

	glColor4f(color[0],color[1],color[2],fader.getInterstate());
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Vec3f pos;
	Vec3d xy;

	prj->setCurrentFrame(Projector::FRAME_LOCAL);

	float shift = font.getStrLen(sNorth)/2;

	// N for North
	pos.set(-1.f, 0.f, 0.f);
	if (prj->project(pos,xy)) prj->drawText(&font, xy[0], xy[1], d[0], 0., -shift, -shift);

	// S for South
	pos.set(1.f, 0.f, 0.f);
	if (prj->project(pos,xy)) prj->drawText(&font, xy[0], xy[1], d[1], 0., -shift, -shift);

	// E for East
	pos.set(0.f, 1.f, 0.f);
	if (prj->project(pos,xy)) prj->drawText(&font, xy[0], xy[1], d[2], 0., -shift, -shift);

	// W for West
	pos.set(0.f, -1.f, 0.f);
	if (prj->project(pos,xy)) prj->drawText(&font, xy[0], xy[1], d[3], 0., -shift, -shift);

}

// Translate cardinal labels with gettext to current sky language and update font for the language
void Cardinals::updateI18n()
{
	Translator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	sNorth = trans.translate("N");
	sSouth = trans.translate("S");
	sEast = trans.translate("E");
	sWest = trans.translate("W");	
	font = StelApp::getInstance().getFontManager().getStandardFont(trans.getTrueLocaleName(), fontSize);
}


LandscapeMgr::LandscapeMgr() : atmosphere(NULL), cardinals_points(NULL), landscape(NULL), flagLandscapeSetsLocation(false)
{
	setObjectName("LandscapeMgr");
}

LandscapeMgr::~LandscapeMgr()
{
	delete atmosphere;
	delete cardinals_points;
	delete landscape;
	landscape = NULL;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double LandscapeMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ACTION_DRAW)
		return StelApp::getInstance().getModuleMgr().getModule("TelescopeMgr")->getCallOrder(actionName)+10;
	if (actionName==StelModule::ACTION_UPDATE)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+10;
	return 0;
}

void LandscapeMgr::update(double deltaTime)
{
	atmosphere->update(deltaTime);
	landscape->update(deltaTime);
	cardinals_points->update(deltaTime);
	
	// Compute the atmosphere color and intensity
	// Compute the sun position in local coordinate
	SolarSystem* ssystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	Projector* prj = StelApp::getInstance().getCore()->getProjection();
	const Observer* obs = StelApp::getInstance().getCore()->getObservatory();
	ToneReproducer* eye = StelApp::getInstance().getCore()->getToneReproducer();
	
	Vec3d sunPos = nav->helio_to_local(ssystem->getSun()->get_heliocentric_ecliptic_pos());
	// Compute the moon position in local coordinate
	Vec3d moonPos = nav->helio_to_local(ssystem->getMoon()->get_heliocentric_ecliptic_pos());
	atmosphere->compute_color(nav->getJDay(), sunPos, moonPos,
	                          ssystem->getMoon()->get_phase(ssystem->getEarth()->get_heliocentric_ecliptic_pos()),
	                          eye, prj, obs->get_latitude(), obs->get_altitude(),
	                          15.f, 40.f);	// Temperature = 15c, relative humidity = 40%
	
	eye->setWorldAdaptationLuminance(3.75+atmosphere->getAverageLuminance()*3.5);
	
	// Compute the ground luminance based on every planets around
//	float groundLuminance = 0;
//	const vector<Planet*>& allPlanets = ssystem->getAllPlanets();
//	for (vector<Planet*>::const_iterator i=allPlanets.begin();i!=allPlanets.end();++i)
//	{
//		Vec3d pos = nav->helio_to_local((*i)->get_heliocentric_ecliptic_pos());
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
//			//cerr << "mag=" << (*i)->get_mag(nav) << " illum=" << E << endl;
//			// Luminance in cd/m^2
//			groundLuminance += E/0.44*pos[2]*pos[2]; // 1m^2 from 1.5 m above the ground is 0.44 sr.
//		}
//	}
//	groundLuminance*=atmosphere->getFadeIntensity();
//	groundLuminance=atmosphere->getAverageLuminance()/50;
//	cout << "Atmosphere lum=" << atmosphere->getAverageLuminance() << " ground lum=" <<  groundLuminance << endl;
	//cout << "Adapted Atmosphere lum=" << eye->adaptLuminance(atmosphere->getAverageLuminance()) << " Adapted ground lum=" << eye->adaptLuminance(groundLuminance) << endl;
	
	// compute global ground brightness in a simplistic way, directly in RGB
	float landscapeBrightness = 0;
	sunPos.normalize();
	moonPos.normalize();
	if(sunPos[2] < -0.1/1.5 )
		landscapeBrightness = 0.01;
	else
		landscapeBrightness = (0.01 + 1.5*(sunPos[2]+0.1/1.5));
	if (moonPos[2] > -0.1/1.5)
		landscapeBrightness += MY_MAX(0.2/-12.*ssystem->getMoon()->getMagnitude(nav),0)*moonPos[2];
//
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

double LandscapeMgr::draw(Projector *prj, const Navigator *nav, ToneReproducer *eye)
{
	// Draw the atmosphere
	atmosphere->draw(prj);

	// Draw the landscape
	landscape->draw(eye, prj, nav);

	// Draw the cardinal points
	cardinals_points->draw(prj, StelApp::getInstance().getCore()->getObservatory()->get_latitude());
	
	return 0;
}

void LandscapeMgr::init(const InitParser& conf, LoadingBar& lb)
{
	atmosphere = new Atmosphere();
	landscape = new LandscapeOldStyle();
	setLandscapeByID(conf.get_str("init_location:landscape_name").c_str());
	setFlagLandscape(conf.get_boolean("landscape", "flag_landscape", conf.get_boolean("landscape", "flag_ground", 1)));  // name change
	setFlagFog(conf.get_boolean("landscape:flag_fog"));
	setFlagAtmosphere(conf.get_boolean("landscape:flag_atmosphere"));
	setAtmosphereFadeDuration(conf.get_double("landscape","atmosphere_fade_duration",1.5));
	setAtmosphereLightPollutionLuminance(conf.get_double("viewing","light_pollution_luminance",0.0));
	cardinals_points = new Cardinals();
	cardinals_points->setFlagShow(conf.get_boolean("viewing:flag_cardinal_points"));
	setFlagLandscapeSetsLocation(conf.get_boolean("landscape:flag_landscape_sets_location"));
}

void LandscapeMgr::setColorScheme(const InitParser& conf, const std::string& section)
{
	// Load colors from config file
	string defaultColor = conf.get_str(section,"default_color");
	setColorCardinalPoints(StelUtils::str_to_vec3f(conf.get_str(section,"cardinal_color", defaultColor)));
}


bool LandscapeMgr::setLandscapeByName(const QString& newLandscapeName)
{
	if (newLandscapeName.isEmpty())
		return 0;
	
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
	if (nameToDirMap.find(newLandscapeName)!=nameToDirMap.end())
	{
		return setLandscapeByID(nameToDirMap[newLandscapeName]);
	}
	else
	{
		qWarning() << "Can't find a landscape with name=" << newLandscapeName << endl;
		return false;
	}
}


bool LandscapeMgr::setLandscapeByID(const QString& newLandscapeID)
{
	if (newLandscapeID.isEmpty())
		return 0;
	
	// we want to lookup the landscape ID (dir) from the name.
	StelFileMgr& fileMan = StelApp::getInstance().getFileMgr();
	Landscape* newLandscape = NULL;
	
	try
	{
		newLandscape = createFromFile(fileMan.qfindFile("landscapes/" + newLandscapeID + "/landscape.ini"), newLandscapeID);
	}
	catch(exception& e)
	{
		qWarning() << "ERROR while loading landscape " << "landscapes/" + newLandscapeID + "/landscape.ini" << ", (" << e.what() << ")" << endl;
	}

	if(!newLandscape)
		return 0;

	if (landscape)
	{
		// Copy parameters from previous landscape to new one
		newLandscape->setFlagShow(landscape->getFlagShow());
		newLandscape->setFlagShowFog(landscape->getFlagShowFog());
		delete landscape;
		landscape = newLandscape;
	}
	currentLandscapeID = newLandscapeID;
	
	if (getFlagLandscapeSetsLocation())
	{
		// Set the planet and moveto the right location
		if (landscape->getPlanet()!="") 
			StelApp::getInstance().getCore()->setHomePlanet(landscape->getPlanet().c_str());
	
		if (landscape->getLongitude() > -500 && landscape->getLatitude() > -500) 
		{
			StelApp::getInstance().getCore()->getObservatory()->moveTo(
				landscape->getLatitude(),
				landscape->getLongitude(),
				landscape->getAltitude(),
				0,
    				landscape->getName());
		}
	
	}
	return 1;
}


//! Load a landscape based on a hash of parameters mirroring the landscape.ini file
//! and make it the current landscape
bool LandscapeMgr::loadLandscape(map<string, string>& param)
{
	Landscape* newLandscape = createFromHash(param);
	if(!newLandscape)
		return 0;

	if (landscape)
	{
		// Copy parameters from previous landscape to new one
		newLandscape->setFlagShow(landscape->getFlagShow());
		newLandscape->setFlagShowFog(landscape->getFlagShowFog());
		delete landscape;
		landscape = newLandscape;
	}
	currentLandscapeID = param["name"].c_str();
	// probably not particularly useful, as not in landscape.ini file

	return 1;
}

void LandscapeMgr::updateI18n()
{
	// Translate all labels with the new language
	if (cardinals_points) cardinals_points->updateI18n();
}

void LandscapeMgr::setFlagLandscape(bool b) 
{
	landscape->setFlagShow(b);
}
	
bool LandscapeMgr::getFlagLandscape(void) const 
{
	return landscape->getFlagShow();
}

void LandscapeMgr::setFlagFog(bool b) 
{
	landscape->setFlagShowFog(b);
}
	
bool LandscapeMgr::getFlagFog(void) const 
{
	return landscape->getFlagShowFog();
}

wstring LandscapeMgr::getLandscapeName(void)
{
	return landscape->getName();
}

wstring LandscapeMgr::getLandscapeAuthorName(void)
{
	return landscape->getAuthorName();
}

wstring LandscapeMgr::getLandscapeDescription(void)
{
	return landscape->getDescription();
}

wstring LandscapeMgr::getLandscapePlanetName(void) 
{
	string desc("");
	if (landscape->getPlanet() != "")
	{
		desc = landscape->getPlanet();
	}
	return StelUtils::stringToWstring(desc);
}
    
wstring LandscapeMgr::getLandscapeLocationDescription(void) 
{
	string desc;
//cerr << landscape->getLongitude() << " " << landscape->getLatitude() << endl;
	if (landscape->getLongitude()>-500.0 && landscape->getLatitude()>-500.0)
	{
		desc = "lon " + StelUtils::radToDmsStrAdapt(landscape->getLongitude() * M_PI/180.);
		desc += ", lat " + StelUtils::radToDmsStrAdapt(landscape->getLatitude() *M_PI/180.);
		desc += ", " + StelUtils::doubleToString(landscape->getAltitude()) + " m";
	}
	return StelUtils::stringToWstring(desc);
}

//! Set flag for displaying Cardinals Points
void LandscapeMgr::setFlagCardinalsPoints(bool b) 
{
	cardinals_points->setFlagShow(b);
}

//! Get flag for displaying Cardinals Points
bool LandscapeMgr::getFlagCardinalsPoints(void) const 
{
	return cardinals_points->getFlagShow();
}

//! Set Cardinals Points color
void LandscapeMgr::setColorCardinalPoints(const Vec3f& v) 
{
	cardinals_points->setColor(v); 
}

//! Get Cardinals Points color
Vec3f LandscapeMgr::getColorCardinalPoints(void) const 
{
	return cardinals_points->get_color();
}
	
///////////////////////////////////////////////////////////////////////////////////////
// Atmosphere
//! Set flag for displaying Atmosphere
void LandscapeMgr::setFlagAtmosphere(bool b) 
{
	atmosphere->setFlagShow(b);
}

//! Get flag for displaying Atmosphere
bool LandscapeMgr::getFlagAtmosphere(void) const 
{
	return atmosphere->getFlagShow();
}

//! Set atmosphere fade duration in s
void LandscapeMgr::setAtmosphereFadeDuration(float f) 
{
	atmosphere->setFadeDuration(f);
}

//! Get atmosphere fade duration in s
float LandscapeMgr::getAtmosphereFadeDuration(void) const 
{
	return atmosphere->getFadeDuration();
}

//! Set light pollution luminance level
void LandscapeMgr::setAtmosphereLightPollutionLuminance(float f) 
{
	atmosphere->setLightPollutionLuminance(f);
}

//! Get light pollution luminance level
float LandscapeMgr::getAtmosphereLightPollutionLuminance(void) const 
{
	return atmosphere->getLightPollutionLuminance();
}

float LandscapeMgr::getLuminance(void) 
{
	return atmosphere->getRealDisplayIntensityFactor();
}

Landscape* LandscapeMgr::createFromFile(const QString& landscapeFile, const QString& landscapeId)
{
	InitParser pd;	// The landscape data ini file parser
	pd.load(landscapeFile);
	string s;
	s = pd.get_str("landscape", "type");
	Landscape* ldscp = NULL;
	if (s=="old_style")
	{
		ldscp = new LandscapeOldStyle();
	}
	else if (s=="spherical")
	{
		ldscp = new LandscapeSpherical();
	}
	else if (s=="fisheye")
	{
		ldscp = new LandscapeFisheye();
	}
	else
	{
		cerr << "Unknown landscape type: " << s << endl;

		// to avoid making this a fatal error, will load as a fisheye
		// if this fails, it just won't draw
		ldscp = new LandscapeFisheye();
	}
	
	ldscp->load(landscapeFile, landscapeId);
	return ldscp;
}

Landscape* LandscapeMgr::createFromHash(map<string, string> & param)
{
	// NOTE: textures should be full filename (and path)
	if (param["type"]=="old_style")
	{
		LandscapeOldStyle* ldscp = new LandscapeOldStyle();
		ldscp->create(1, param);
		return ldscp;
	}
	else if (param["type"]=="spherical")
	{
		LandscapeSpherical* ldscp = new LandscapeSpherical();
		ldscp->create(StelUtils::stringToWstring(param["name"]), 1, (param["path"] + param["maptex"]).c_str(),
                      StelUtils::stringToDouble(param["angle_rotatez"]));
		return ldscp;
	}
	else
	{   //	if (s=="fisheye")
		LandscapeFisheye* ldscp = new LandscapeFisheye();
		ldscp->create(StelUtils::stringToWstring(param["name"]), 1, (param["path"] + param["maptex"]).c_str(),
		              StelUtils::stringToDouble(param["texturefov"]),
                      StelUtils::stringToDouble(param["angle_rotatez"]));
		return ldscp;
	}
}

/*********************************************************************
 return a list of distinct landscape names (the name field from each 
 landscape.ini file).  The result is a string with each name separated
 by a '\n' character.
 *********************************************************************/
QString LandscapeMgr::getAllLandscapeNames()
{
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
 	QString result;
	
	// We just look over the map of names to IDs and extract the keys
	foreach (QString i, nameToDirMap.keys())
	{
		result += i + '\n';
	}

	return result;
}

QString LandscapeMgr::nameToID(const QString& name)
{
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
	
	if (nameToDirMap.find(name)!=nameToDirMap.end())
	{
 		assert(0);
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
QMap<QString,QString> LandscapeMgr::getNameToDirMap(void)
{
	QSet<QString> landscapeDirs;
	QMap<QString,QString> result;
	StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
	try
	{
		landscapeDirs = fileMan.listContents("landscapes",StelFileMgr::DIRECTORY);
	}
	catch(exception& e)
	{
		cerr << "ERROR while trying list list landscapes:" << e.what() << endl;	
	}
	
	foreach (QString dir, landscapeDirs)
	{
		try
		{
			InitParser pd;
			pd.load(fileMan.qfindFile("landscapes/" + dir + "/landscape.ini"));
			QString k = pd.get_str("landscape", "name").c_str();
			result[k] = dir;
		}
		catch (exception& e)
		{
			//cerr << "WARNING: unable to successfully read landscape.ini file from landscape " << *dir << endl;
		}
	}

	return result;	
}
