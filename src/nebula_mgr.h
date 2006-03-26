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

#ifndef _NEBULA_MGR_H_
#define _NEBULA_MGR_H_

#include <vector>
#include "nebula.h"
#include "s_font.h"
#include "loadingbar.h"
#include "fader.h"
#include "translator.h"
#include "grid.h"

using namespace std;

class NebulaMgr  
{
public:
	NebulaMgr();
	virtual ~NebulaMgr();
	
	// Read the Nebulas data from files
	bool read(float font_size, const string& font_name, const string& catNGC, const string& catNGCnames, const string& catTextures, LoadingBar& lb);
	
	// Draw all the Nebulas
	void draw(Projector *prj, const Navigator *nav, ToneReproductor *eye); 
	void update(int delta_time) {hintsFader.update(delta_time); flagShow.update(delta_time);}
	
	StelObject *search(const string& name);  // search by name M83, NGC 1123, IC 1234
	StelObject *search(Vec3f Pos);    // Search the Nebulae by position
	
	void setNebulaCircleScale(float scale) {Nebula::circleScale = scale;} 	
	float getNebulaCircleScale(void) {return Nebula::circleScale;} 
	
	void setHintsFadeDuration(float duration) {hintsFader.set_duration((int) (duration * 1000.f));}
	
	void setFlagHints(bool b) {hintsFader=b;}
	bool getFlagHints() {return hintsFader;}
	
	void setFlagShow(bool b) { flagShow = b; }
	bool getFlagShow() { return flagShow; }

	void set_label_color(const Vec3f& c) {Nebula::label_color = c;}
	const Vec3f& getLabelColor(void) const {return Nebula::label_color;}
	
	void set_circle_color(const Vec3f& c) {Nebula::circle_color = c;}

	// Return a stl vector containing the nebulas located inside the lim_fov circle around position v
	vector<StelObject*> search_around(Vec3d v, double lim_fov);
	
	//! @brief Update i18 names from english names according to passed translator
	//! The translation is done using gettext with translated strings defined in translations.h
	void translateNames(Translator& trans);
	
	//! Set flag for displaying Nebulae as bright
	void setFlagBright(bool b) {Nebula::flagBright = b;}
	//! Get flag for displaying Nebulae as bright
	bool getFlagBright(void) const {return Nebula::flagBright;}	
	
	//! Set maximum magnitude at which nebulae hints are displayed
	void setMaxMagHints(float f) {maxMagHints = f;}
	//! Get maximum magnitude at which nebulae hints are displayed
	float getMaxMagHints(void) {return maxMagHints;}
	
private:
	StelObject *searchNGC(unsigned int NGC);
	StelObject *searchIC(unsigned int NGC);
	bool loadNGC(const string& fileName, LoadingBar& lb);
	bool loadNGCNames(const string& fileName);
	bool loadTextures(const string& fileName, LoadingBar& lb);

	FILE *nebula_fic;
	vector<Nebula*> neb_array;		// The nebulas list
	LinearFader hintsFader;
	LinearFader flagShow;
	
	vector<Nebula*>* nebZones;		// array of nebula vector with the grid id as array rank
	Grid nebGrid;					// Grid for opimisation
	
	float maxMagHints;				// Define maximum magnitude at which nebulae hints are displayed
};

#endif // _NEBULA_MGR_H_
