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
#include "StelObjectType.hpp"
#include "Fader.hpp"
#include "TreeGrid.hpp"
//#include "SimpleGrid.hpp"
#include "StelObjectModule.hpp"
#include "STextureTypes.hpp"

using namespace std;

class Nebula;
class LoadingBar;
class Translator;
class ToneReproducer;

//! @class NebulaMgr
//! Manage a collection of nebulae. This class is used 
//! to display the NGC catalog with information, and textures for some of them.
class NebulaMgr : public StelObjectModule
{
	Q_OBJECT
			
public:
	NebulaMgr();
	virtual ~NebulaMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the NebulaMgr object.
	//!  - Load the font into the Nebula class, which is used to draw Nebula labels.
	//!  - Load the texture used to draw nebula locations into the Nebula class (for
	//!     those with no individual texture).
	//!  - Load the pointer texture.
	//!  - Set flags values from ini parser which relate to nebula display.
	//!  - call updateI18n() to translate names.
	//! @param conf the ini parser object which contains configuration information
	//! pertaining to nebulae.
	//! @param lb the loading bar object used to display loading progress and status
	virtual void init(const InitParser& conf, LoadingBar& lb);
	
	//! Draws all nebula objects.
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	
	//! Update state which is time dependent.
	virtual void update(double deltaTime) {hintsFader.update((int)(deltaTime*1000)); flagShow.update((int)(deltaTime*1000));}
	
	//! Update i18 names from English names according to passed translator.
	//! The translation is done using gettext with translated strings defined 
	//! in translations.h
	virtual void updateI18n();
	
	//! Called when the sky culture is updated, so that the module can respond
	//! as appropriate.  Does nothing as there are no SkyCulture specific features 
	//! in the current nebula implementation.
	virtual void updateSkyCulture(LoadingBar& lb);
	
	//! Sets the colors of the Nebula labels and markers according to the
	//! values in the ini parser object.
	//! @param conf The ini parser object.
	//! @param section The section of the parser object from which to take the 
	//! new color values.
	virtual void setColorScheme(const InitParser& conf, const QString& section);
	
	//! Determines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Used to get a vector of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which 
	//! to search for nebulae.
	//! @param limitFov the field of view around the position v in which to
	//! search for nebulae.
	//! @param nav the Navigator object.
	//! @param prj the Projector object.
	//! @return an stl vector containing the nebulae located inside the
	//! limitFov circle around position v.
	virtual vector<StelObjectP> searchAround(const Vec3d& v, double limitFov, const Navigator * nav, const Projector * prj) const;
	
	//! Return the matching nebula object's pointer if exists or NULL.
	//! @param nameI18n The case sensistive nebula name or NGC M catalog name : format can be M31, M 31, NGC31 NGC 31
	virtual StelObjectP searchByNameI18n(const wstring& nameI18n) const;
	
	//! Return the matching nebula if exists or NULL.
	//! @param name The case sensistive standard program name
	virtual StelObjectP searchByName(const string& name) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
	virtual vector<wstring> listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem=5) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
public slots:
	//! Set Nebulae Hints circle scale.
	void setCircleScale(float scale);
	//! Get Nebulae Hints circle scale.
	float getCircleScale(void) const;
	
	//! Set how long it takes for nebula hints to fade in and out when turned on and off.
	void setHintsFadeDuration(float duration) {hintsFader.set_duration((int) (duration * 1000.f));}
	
	//! Set flag for displaying Nebulae Hints.
	void setFlagHints(bool b) {hintsFader=b;}
	//! Get flag for displaying Nebulae Hints.
	bool getFlagHints(void) const {return hintsFader;}
	
	//! Set flag used to turn on and off Nebula rendering.
	void setFlagShow(bool b) { flagShow = b; }
	//! Get value of flag used to turn on and off Nebula rendering.
	bool getFlagShow(void) const { return flagShow; }
	
	//! Set the color used to draw nebula labels.
	void setNamesColor(const Vec3f& c);
	//! Get current value of the nebula label color.
	const Vec3f &getNamesColor(void) const;
	
	//! Set the color used to draw the nebula circles.
	void setCirclesColor(const Vec3f& c);
	//! Get current value of the nebula circle color.
	const Vec3f &getCirclesColor(void) const;
	
	//! Set flag for displaying nebulae as bright to show all details.
	//! Turning on the bright flag turns off magnitude compensation when rendering nebulae.
	void setFlagBright(bool b);
	//! Get whether we display nebulae as bright to show all details.
	//! Turning on the bright flag turns off magnitude compensation when rendering nebulae.
	bool getFlagBright(void) const;
	
	//! Set flag for displaying nebulae even without textures.
	void setFlagDisplayNoTexture(bool b) {displayNoTexture = b;}
	//! Get flag for displaying nebulae without textures.
	bool getFlagDisplayNoTexture(void) const {return displayNoTexture;}	
	
	//! Display textures for nebulae which have one.
	void setFlagShowTexture(bool b) {flagShowTexture = b;}
	//! Get whether textures are displayed for nebulas which have one.
	bool getFlagShowTexture(void) const {return flagShowTexture;}	
	
	//! Set maximum magnitude at which nebulae hints are displayed.
	void setMaxMagHints(float f) {maxMagHints = f;}
	//! Get maximum magnitude at which nebulae hints are displayed.
	float getMaxMagHints(void) const {return maxMagHints;}
	
private:
	
	//! Search for a nebula object by name. e.g. M83, NGC 1123, IC 1234.
	StelObject* search(const string& name);
	
	//! Load a set of nebula images.
	//! Each sub-directory of the INSTALLDIR/nebulae directory contains a set of
	//! nebula textures.  The sub-directory is the setName.  Each set has its
	//! own nebula_textures.fab file and corresponding image files.
	//! This function loads a set of textures.
	//! @param setName a string which corresponds to the directory where the set resides
	void loadNebulaSet(const string& setName, LoadingBar& lb);
	
	StelObject* search(Vec3f Pos);    // Search the Nebulae by position	
		
	//! Draw a nice animated pointer around the object
	void drawPointer(const Projector* prj, const Navigator * nav);
		
	Nebula *searchM(unsigned int M);
	Nebula *searchNGC(unsigned int NGC);
	Nebula *searchIC(unsigned int IC);
	bool loadNGC(const string& fileName, LoadingBar& lb);
	bool loadNGCNames(const string& fileName);
	
	//! loads the textures for a specified nebula texture setName
	//! @param setName The name of the sub-directory in .../nebulae in which the set resides
	//! @param lb the loading progress bar object
	bool loadTextures(const string& setName, LoadingBar& lb);

	vector<Nebula*> neb_array;		// The nebulas list
	LinearFader hintsFader;
	LinearFader flagShow;
	
	TreeGrid nebGrid;
	//SimpleGrid nebGrid;
	
	float maxMagHints;			// Define maximum magnitude at which nebulae hints are displayed
	bool displayNoTexture;			// Define if nebulas without textures are to be displayed
	bool flagShowTexture;			// Define if nebula textures are displayed
	
	STextureSP texPointer;			// The selection pointer texture
};

#endif // _NEBULA_MGR_H_
