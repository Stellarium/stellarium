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
#include "StelObject.hpp"
#include "Fader.hpp"
#include "grid.h"
#include "StelObjectMgr.hpp"

using namespace std;

class Nebula;
class LoadingBar;
class Translator;
class ToneReproducer;

//! Manage a collection of nebula. It display the NGC catalog with informations 
//! and textures for some of them.
class NebulaMgr : public StelObjectMgr
{
public:
	NebulaMgr();
	virtual ~NebulaMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init(const InitParser& conf, LoadingBar& lb);
	virtual string getModuleID() const {return "nebulas";}
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	virtual void update(double deltaTime) {hintsFader.update((int)(deltaTime*1000)); flagShow.update((int)(deltaTime*1000));}
	virtual void updateI18n();
	virtual void updateSkyCulture(LoadingBar& lb);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	virtual vector<StelObject> searchAround(const Vec3d& v, double limitFov, const Navigator * nav, const Projector * prj) const;
	//! Return the matching Nebula object's pointer if exists or NULL
	//! @param nameI18n The case sensistive nebula name or NGC M catalog name : format can be M31, M 31, NGC31 NGC 31
	virtual StelObject searchByNameI18n(const wstring& nameI18n) const;
	
	//! @brief Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
	virtual vector<wstring> listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem=5) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
	//! Set Nebulae Hints circle scale
	void setCircleScale(float scale);
	//! Get Nebulae Hints circle scale
	float getCircleScale(void) const;
	
	void setHintsFadeDuration(float duration) {hintsFader.set_duration((int) (duration * 1000.f));}
	
	//! Set flag for displaying Nebulae Hints
	void setFlagHints(bool b) {hintsFader=b;}
	//! Get flag for displaying Nebulae Hints
	bool getFlagHints(void) const {return hintsFader;}
	
	void setFlagShow(bool b) { flagShow = b; }
	bool getFlagShow(void) const { return flagShow; }

	void setNamesColor(const Vec3f& c);
	const Vec3f &getNamesColor(void) const;
	
	void setCirclesColor(const Vec3f& c);
	const Vec3f &getCirclesColor(void) const;
	
	//! Set flag for displaying nebulae as bright to show all details
	void setFlagBright(bool b);
	//! Get whether we display nebulae as bright to show all details
	bool getFlagBright(void) const;
	
	//! Set flag for displaying Nebulae even without textures
	void setFlagDisplayNoTexture(bool b) {displayNoTexture = b;}
	//! Get flag for displaying Nebulae without textures
	bool getFlagDisplayNoTexture(void) const {return displayNoTexture;}	
	
	//! Display textures for nebulas which have one
	void setFlagShowTexture(bool b) {flagShowTexture = b;}
	//! Get whether textures are displayed for nebulas which have one
	bool getFlagShowTexture(void) const {return flagShowTexture;}	
	
	//! Set maximum magnitude at which nebulae hints are displayed
	void setMaxMagHints(float f) {maxMagHints = f;}
	//! Get maximum magnitude at which nebulae hints are displayed
	float getMaxMagHints(void) const {return maxMagHints;}

	StelObject search(const string& name);  // search by name M83, NGC 1123, IC 1234
		
private:
	StelObject search(Vec3f Pos);    // Search the Nebulae by position	
		
	Nebula *searchM(unsigned int M);
	Nebula *searchNGC(unsigned int NGC);
	Nebula *searchIC(unsigned int IC);
	bool loadNGC(const string& fileName, LoadingBar& lb);
	bool loadNGCNames(const string& fileName);
	bool loadTextures(const string& fileName, LoadingBar& lb);

	vector<Nebula*> neb_array;		// The nebulas list
	LinearFader hintsFader;
	LinearFader flagShow;
	
	vector<Nebula*>* nebZones;		// array of nebula vector with the grid id as array rank
	Grid nebGrid;					// Grid for opimisation
	
	float maxMagHints;				// Define maximum magnitude at which nebulae hints are displayed
	bool displayNoTexture;			// Define if nebulas without textures are to be displayed
	bool flagShowTexture;			// Define if nebula textures are displayed
};

#endif // _NEBULA_MGR_H_
