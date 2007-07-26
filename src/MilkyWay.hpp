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

#ifndef __MILKYWAY_H__
#define __MILKYWAY_H__

#include <string>
#include "StelModule.hpp"
#include "vecmath.h"
#include "STextureTypes.hpp"

class Navigator;
class ToneReproducer;
class LoadingBar;

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
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	virtual void update(double deltaTime);
	virtual void updateI18n() {;}
	virtual void updateSkyCulture(LoadingBar& lb) {;}
	virtual double getCallOrder(StelModuleActionName actionName) const {return 1.;}
	
	//! Get Milky Way intensity
	float getIntensity() {return intensity;}
	//! Set Milky Way intensity
	void setIntensity(float aintensity) {intensity = aintensity;}
	void setTexture(const string& tex_file);
	void setColor(const Vec3f& c) { color=c;}
	void setFlagShow(bool b);
	bool getFlagShow(void) const;
	
private:
	float radius;
	STextureSP tex;
	Vec3f color;
	float intensity;
	float tex_avg_luminance;
	class LinearFader* fader;
};

#endif // __MILKYWAY_H__
