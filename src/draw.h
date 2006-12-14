/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef __DRAW_H__
#define __DRAW_H__

#include <string>
#include <fstream>
#include "stellarium.h"
#include "s_font.h"
#include "projector.h"
#include "fader.h"
#include "translator.h"
#include "stelmodule.h"

class STexture;
class Navigator;
class ToneReproductor;
class LoadingBar;

// Class which manages a grid to display in the sky
class SkyGrid
{
public:
	enum SKY_GRID_TYPE
	{
		EQUATORIAL,
		ALTAZIMUTAL
	};
	// Create and precompute positions of a SkyGrid
	SkyGrid(SKY_GRID_TYPE grid_type = EQUATORIAL, unsigned int _nb_meridian = 24, unsigned int _nb_parallel = 17,
	 double _radius = 1., unsigned int _nb_alt_segment = 18, unsigned int _nb_azi_segment = 50);
    virtual ~SkyGrid();
	void draw(const Projector* prj) const;
	void setFontSize(double newFontSize);
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(int delta_time) {fader.update(delta_time);}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void setFlagshow(bool b){fader = b;}
	bool getFlagshow(void) const {return fader;}
	void set_top_transparancy(bool b) { transparent_top= b; }
private:
	unsigned int nb_meridian;
	unsigned int nb_parallel;
	double radius;
	unsigned int nb_alt_segment;
	unsigned int nb_azi_segment;
	bool transparent_top;
	Vec3f color;
	Vec3f** alt_points;
	Vec3f** azi_points;
	bool (Projector::*proj_func)(const Vec3d&, Vec3d&) const;
	double fontSize;
	SFont& font;
	SKY_GRID_TYPE gtype;
	LinearFader fader;
};


// Class which manages a line to display around the sky like the ecliptic line
class SkyLine
{
public:
	enum SKY_LINE_TYPE
	{
		EQUATOR,
		ECLIPTIC,
		LOCAL,
		MERIDIAN
	};
	// Create and precompute positions of a SkyGrid
	SkyLine(SKY_LINE_TYPE _line_type = EQUATOR, double _radius = 1., unsigned int _nb_segment = 48);
    virtual ~SkyLine();
	void draw(const Projector *prj,const Navigator *nav) const;
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(int delta_time) {fader.update(delta_time);}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void setFlagshow(bool b){fader = b;}
	bool getFlagshow(void) const {return fader;}
	void setFontSize(double newSize);
private:
	double radius;
	unsigned int nb_segment;
	SKY_LINE_TYPE line_type;
	Vec3f color;
	Vec3f* points;
	bool (Projector::*proj_func)(const Vec3d&, Vec3d&) const;
	LinearFader fader;
	double fontSize;
	SFont& font;
};


// Class which manages the displaying of the Milky Way
class MilkyWay : public StelModule
{
public:
	MilkyWay();
    virtual ~MilkyWay();
  
 	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init(const InitParser& conf, LoadingBar& lb);
	virtual string getModuleID() const {return "milkyway";}
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproductor *eye);
	virtual void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	virtual void updateI18n() {;}
	virtual void updateSkyCulture(LoadingBar& lb) {;}
	
	//! Get Milky Way intensity
	float getIntensity() {return intensity;}
	//! Set Milky Way intensity
	void setIntensity(float aintensity) {intensity = aintensity;}
	void setTexture(const string& tex_file);
	void setColor(const Vec3f& c) { color=c;}
	void setFlagShow(bool b){fader = b;}
	bool getFlagShow(void) const {return fader;}
private:
	float radius;
	STexture* tex;
	Vec3f color;
	float intensity;
	float tex_avg_luminance;
	LinearFader fader;
};

// Class which manages the displaying of the Milky Way
class Draw
{
public:
	// Draw a point... (used for tests)
	static void drawPoint(float X,float Y,float Z);
};

#endif // __DRAW_H__
