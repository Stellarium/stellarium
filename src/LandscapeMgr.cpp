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
 
#include "LandscapeMgr.h"
#include "landscape.h"
#include "stel_atmosphere.h"
#include "stelapp.h"
#include "stelmodulemgr.h"
#include "solarsystem.h"
#include "stellocalemgr.h"
#include "stelfontmgr.h"
#include "stel_core.h"

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

	prj->set_orthographic_projection();

	float shift = font.getStrLen(sNorth)/2;

	if (prj->getFlagGravityLabels())
	{
		// N for North
		pos.set(-1.f, 0.f, 0.22f);
		if (prj->project_local(pos,xy)) prj->print_gravity180(&font, xy[0], xy[1], d[0], -shift, -shift);

		// S for South
		pos.set(1.f, 0.f, 0.22f);
		if (prj->project_local(pos,xy)) prj->print_gravity180(&font, xy[0], xy[1], d[1], -shift, -shift);

		// E for East
		pos.set(0.f, 1.f, 0.22f);
		if (prj->project_local(pos,xy)) prj->print_gravity180(&font, xy[0], xy[1], d[2], -shift, -shift);

		// W for West
		pos.set(0.f, -1.f, 0.22f);
		if (prj->project_local(pos,xy)) prj->print_gravity180(&font, xy[0], xy[1], d[3], -shift, -shift);
	}
	else
	{
		// N for North
		pos.set(-1.f, 0.f, 0.f);
		if (prj->project_local(pos,xy)) font.print(xy[0]-shift, xy[1]-shift, d[0]);

		// S for South
		pos.set(1.f, 0.f, 0.f);
		if (prj->project_local(pos,xy)) font.print(xy[0]-shift, xy[1]-shift, d[1]);

		// E for East
		pos.set(0.f, 1.f, 0.f);
		if (prj->project_local(pos,xy)) font.print(xy[0]-shift, xy[1]-shift, d[2]);

		// W for West
		pos.set(0.f, -1.f, 0.f);
		if (prj->project_local(pos,xy)) font.print(xy[0]-shift, xy[1]-shift, d[3]);
	}

	prj->reset_perspective_projection();
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


LandscapeMgr::LandscapeMgr() : atmosphere(NULL), cardinals_points(NULL), landscape(NULL)
{
}

LandscapeMgr::~LandscapeMgr()
{
	delete atmosphere;
	delete cardinals_points;
	delete landscape;
	landscape = NULL;
}

void LandscapeMgr::update(double deltaTime)
{
	atmosphere->update(deltaTime);
	landscape->update(deltaTime);
	cardinals_points->update(deltaTime);
	
	// Compute the atmosphere color and intensity
	// Compute the sun position in local coordinate
	SolarSystem* ssystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("ssystem");
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	Projector* prj = StelApp::getInstance().getCore()->getProjection();
	const Observator* obs = StelApp::getInstance().getCore()->getObservatory();
	ToneReproductor* eye = StelApp::getInstance().getCore()->getToneReproductor();
	
	Vec3d sunPos = nav->helio_to_local(ssystem->getSun()->get_heliocentric_ecliptic_pos());
	// Compute the moon position in local coordinate
	Vec3d moonPos = nav->helio_to_local(ssystem->getMoon()->get_heliocentric_ecliptic_pos());
	atmosphere->compute_color(nav->getJDay(), sunPos, moonPos,
	                          ssystem->getMoon()->get_phase(ssystem->getEarth()->get_heliocentric_ecliptic_pos()),
	                          eye, prj, obs->get_latitude(), obs->get_altitude(),
	                          15.f, 40.f);	// Temperature = 15c, relative humidity = 40%
	eye->set_world_adaptation_luminance(atmosphere->get_world_adaptation_luminance());
	
	sunPos.normalize();
	moonPos.normalize();
	// compute global sky brightness TODO : make this more "scientifically"
	// TODO: also add moonlight illumination

	if(sunPos[2] < -0.1/1.5 )
		sky_brightness = 0.01;
	else
		sky_brightness = (0.01 + 1.5*(sunPos[2]+0.1/1.5));

	// TODO make this more generic for non-atmosphere planets
	if(atmosphere->get_fade_intensity() == 1)
	{
		// If the atmosphere is on, a solar eclipse might darken the sky
		// otherwise we just use the sun position calculation above
		sky_brightness *= (atmosphere->get_intensity()+0.1);
	}

	// TODO: should calculate dimming with solar eclipse even without atmosphere on
	landscape->set_sky_brightness(sky_brightness+0.05);
}

double LandscapeMgr::draw(Projector *prj, const Navigator *nav, ToneReproductor *eye)
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
	setLandscape(conf.get_str("init_location:landscape_name"));
	setFlagLandscape(conf.get_boolean("landscape", "flag_landscape", conf.get_boolean("landscape", "flag_ground", 1)));  // name change
	setFlagFog(conf.get_boolean("landscape:flag_fog"));
	setFlagAtmosphere(conf.get_boolean("landscape:flag_atmosphere"));
	setAtmosphereFadeDuration(conf.get_double("landscape","atmosphere_fade_duration",1.5));
	cardinals_points = new Cardinals();
	cardinals_points->setFlagShow(conf.get_boolean("viewing:flag_cardinal_points"));
}


bool LandscapeMgr::setLandscape(const string& new_landscape_name)
{
	if (new_landscape_name.empty())
		return 0;
	Landscape* newLandscape = create_from_file(StelApp::getInstance().getDataFilePath("landscapes.ini"), new_landscape_name);

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
	landscapeSectionName = new_landscape_name;
	return 1;
}


//! Load a landscape based on a hash of parameters mirroring the landscape.ini file
//! and make it the current landscape
bool LandscapeMgr::loadLandscape(stringHash_t& param)
{
	Landscape* newLandscape = create_from_hash(param);
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
	landscapeSectionName = param["name"];
	// probably not particularly useful, as not in landscape.ini file

	return 1;
}

void LandscapeMgr::updateI18n()
{
	// Translate all labels with the new language
	if (cardinals_points) cardinals_points->updateI18n();
}

	//! Set flag for displaying Landscape
	void LandscapeMgr::setFlagLandscape(bool b) {landscape->setFlagShow(b);}
	//! Get flag for displaying Landscape
	bool LandscapeMgr::getFlagLandscape(void) const {return landscape->getFlagShow();}

	//! Set flag for displaying Fog
	void LandscapeMgr::setFlagFog(bool b) {landscape->setFlagShowFog(b);}
	//! Get flag for displaying Fog
	bool LandscapeMgr::getFlagFog(void) const {return landscape->getFlagShowFog();}

    wstring LandscapeMgr::getLandscapeName(void) {return landscape->getName();}
    wstring LandscapeMgr::getLandscapeAuthorName(void) {return landscape->getAuthorName();}
    wstring LandscapeMgr::getLandscapeDescription(void) {return landscape->getDescription();}
    
	//! Set flag for displaying Cardinals Points
	void LandscapeMgr::setFlagCardinalsPoints(bool b) {cardinals_points->setFlagShow(b);}
	//! Get flag for displaying Cardinals Points
	bool LandscapeMgr::getFlagCardinalsPoints(void) const {return cardinals_points->getFlagShow();}

	//! Set Cardinals Points color
	void LandscapeMgr::setColorCardinalPoints(const Vec3f& v) {cardinals_points->setColor(v); }
	//! Get Cardinals Points color
	Vec3f LandscapeMgr::getColorCardinalPoints(void) const {return cardinals_points->get_color();}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Atmosphere
	//! Set flag for displaying Atmosphere
	void LandscapeMgr::setFlagAtmosphere(bool b) {atmosphere->setFlagShow(b);}
	//! Get flag for displaying Atmosphere
	bool LandscapeMgr::getFlagAtmosphere(void) const {return atmosphere->getFlagShow();}

	//! Set atmosphere fade duration in s
	void LandscapeMgr::setAtmosphereFadeDuration(float f) {atmosphere->setFadeDuration(f);}
	//! Get atmosphere fade duration in s
	float LandscapeMgr::getAtmosphereFadeDuration(void) const {return atmosphere->getFadeDuration();}
	
	float LandscapeMgr::getLuminance(void) {return atmosphere->get_intensity();}

Landscape* LandscapeMgr::create_from_file(const string& landscape_file, const string& section_name)
{
	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);
	string s;
	s = pd.get_str(section_name, "type");
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
	
	ldscp->load(landscape_file, section_name);
	return ldscp;
}

// create landscape from parameters passed in a hash (same keys as with ini file)
// NOTE: maptex must be full path and filename
Landscape* LandscapeMgr::create_from_hash(stringHash_t & param)
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
		ldscp->create(StelUtils::stringToWstring(param["name"]), 1, param["path"] + param["maptex"]);
		return ldscp;
	}
	else
	{   //	if (s=="fisheye")
		LandscapeFisheye* ldscp = new LandscapeFisheye();
		ldscp->create(StelUtils::stringToWstring(param["name"]), 1, param["path"] + param["maptex"], StelUtils::str_to_double(param["texturefov"]));
		return ldscp;
	}
}


string LandscapeMgr::getFileContent(const string& landscape_file)
{
	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	string result;

	for (int i=0; i<pd.get_nsec();i++)
	{
		result += pd.get_secname(i) + '\n';
	}
	return result;
}

string LandscapeMgr::getLandscapeNames(const string& landscape_file)
{
    InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	string result;

	for (int i=0; i<pd.get_nsec();i++)
	{
        result += pd.get_str(pd.get_secname(i), "name") + '\n';
	}
	return result;
}

string LandscapeMgr::nameToKey(const string& landscape_file, const string & name)
{
    InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	for (int i=0; i<pd.get_nsec();i++)
	{
        if (name==pd.get_str(pd.get_secname(i), "name")) return pd.get_secname(i);
	}
	assert(0);
	return "error";
}
