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

#ifndef _STAR_MGR_H_
#define _STAR_MGR_H_

#include <vector>
#include <map>
#include "Fader.hpp"
#include "StelObjectModule.hpp"
#include "STextureTypes.hpp"

class StelObject;
class ToneReproducer;
class Projector;
class Navigator;
class LoadingBar;
class SFont;
class QSettings;

namespace BigStarCatalogExtension {
  class ZoneArray;
  class HipIndexStruct;
}

//! @class StarMgr 
//! Stores the star catalogue data.
//! Used to render the stars themselves, as well as determine the color table
//! and render the labels of those stars with names for a given SkyCulture.
//! 
//! The celestial sphere is split into zones, which correspond to the
//! triangular faces of a geodesic sphere. The number of zones (faces)
//! depends on the level of sub-division of this sphere. The lowest
//! level, 0, is an icosahedron (20 faces), subsequent levels, L,
//! of sub-division give the number of zones, n as:
//!
//! n=20 x 4^L
//!
//! Stellarium uses levels 0 to 7 in the existing star catalogues.
//! Star Data Records contain the position of a star as an offset from
//! the central position of the zone in which that star is located,
//! thus it is necessary to determine the vector from the observer
//! to the centre of a zone, and add the star's offsets to find the
//! absolute position of the star on the celestial sphere.
//!
//! This position for a star is expressed as a 3-dimensional vector
//! which points from the observer (at the centre of the geodesic sphere)
//! to the position of the star as observed on the celestial sphere.
class StarMgr : public StelObjectModule
{	
	Q_OBJECT;
	
public:
	StarMgr(void);
	~StarMgr(void);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the StarMgr.
	//! - Loads the star catalogue data into memory
	//! - Sets up the star color table
	//! - Loads the star texture
	//! - Loads the star font (for labels on named stars)
	//! - Loads the texture of the sar selection indicator
	//! - Lets various display flags from the ini parser object
	virtual void init();
	
	
	//! Draw the stars and the star selection indicator if necessary.
	virtual double draw(StelCore* core); //! Draw all the stars
	
	//! Update any time-dependent features.
	//! Includes fading in and out stars and labels when they are turned on and off.
	virtual void update(double deltaTime) {names_fader.update((int)(deltaTime*1000)); starsFader.update((int)(deltaTime*1000));}
	
	//! Translate text.
	virtual void updateI18n();
	
	//! Called when the sky culture is updated.
	//! Loads common and scientific names of stars for a given sky culture.
	virtual void updateSkyCulture();
	
	//! Load a color scheme from a configration object
	//! @param conf the configuration object containing the color scheme
	//! @param section of conf containing the color scheme
	virtual void setColorScheme(const QSettings* conf, const QString& section);
	
	//! Used to determine the order in which the various StelModules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Return a stl vector containing the stars located inside the lim_fov circle around position v
	virtual vector<StelObjectP > searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;
	
	//! Return the matching Stars object's pointer if exists or NULL
	//! @param nameI18n The case in-sensistive star common name or HP
	//! catalog name (format can be HP1234 or HP 1234) or sci name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;	

	//! Return the matching star if exists or NULL
	//! @param name The case in-sensistive standard program planet name
	virtual StelObjectP searchByName(const QString& name) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;
	
public slots:
	///////////////////////////////////////////////////////////////////////////
	// Methods callable from script and GUI
	//! Set the color used to label bright stars.
	void setLabelColor(const Vec3f& c) {label_color = c;}
	//! Get the current color used to label bright stars.
	Vec3f getLabelColor(void) const {return label_color;}
	
	//! Set display flag for Stars.
	void setFlagStars(bool b) {starsFader=b;}
	//! Get display flag for Stars 
	bool getFlagStars(void) const {return starsFader==true;}
	
	//! Set display flag for Star names (labels).
	void setFlagNames(bool b) {names_fader=b;}
	//! Get display flag for Star names (labels).
	bool getFlagNames(void) const {return names_fader==true;}
	
	//! Set display flag for Star Scientific names.
	void setFlagStarsSciNames(bool b) {flagStarSciName=b;}
	//! Get display flag for Star Scientific names.
	bool getFlagStarsSciNames(void) const {return flagStarSciName;}
	
	//! Set maximum magnitude at which stars names are displayed.
	void setMaxMagName(float b) {maxMagStarName=b;}
	//! Get maximum magnitude at which stars names are displayed.
	float getMaxMagName(void) const {return maxMagStarName;} 
	
	//! Define font size to use for star names display.
	void setFontSize(double newFontSize);
	
	//! Show scientific or catalog names on stars without common names.
	static void setFlagSciNames(bool f) {flagSciNames = f;}
	static bool getFlagSciNames(void) {return flagSciNames;}

public:
	///////////////////////////////////////////////////////////////////////////
	// Other methods
	//! Search for the nearest star to some position.
	//! @param Pos the 3d vector representing the direction to search.
	//! @return the nearest star from the specified position, or an 
	//! empty StelObjectP if none were found close by.
	StelObjectP search(Vec3d Pos) const;
	
	//! Search for a star by catalogue number (including catalogue prefix).
	//! @param id the catalogue identifier for the required star.
	//! @return the requested StelObjectP or an empty objecy if the requested
	//! one was not found.
	StelObjectP search(const QString& id) const;
	
	//! Search bu Hipparcos catalogue number.
	//! @param num the Hipparcos catalogue number of the star which is required.
	//! @return the requested StelObjectP or an empty objecy if the requested
	//! one was not found.
	StelObjectP searchHP(int num) const;
	
	//! Get the (translated) common name for a star with a specified 
	//! Hipparcos catalogue number.
	static QString getCommonName(int hip);
	
	//! Get the (translated) scientific name for a star with a specified 
	//! Hipparcos catalogue number.
	static QString getSciName(int hip);
	
	// TODO: the StarMgr it self should call core()->getGeodesicGrid()->setMaxlevel etc..
	//! Get the maximum level of the geodesic sphere used.
	//! See the class description for a short introduction to the meaning of this value.
	int getMaxGridLevel(void) const {return max_geodesic_grid_level;}
	//! Initializes each triangular face of the geodesic grid.
	void setGrid(class GeodesicGrid* grid);
	
	static double getCurrentJDay(void) {return current_JDay;}
	
	static QString convertToSpectralType(int index);
	static QString convertToComponentIds(int index);
	
private:
	
	//! Loads common names for stars from a file.
	//! Called when the SkyCulture is updated.
	//! @param the path to a file containing the common names for bright stars.
	int load_common_names(const QString& commonNameFile);
	
	//! Loads scientific names for stars from a file.
	//! Called when the SkyCulture is updated.
	//! @param the path to a file containing the scientific names for bright stars.
	void load_sci_names(const QString& sciNameFile);
	
	//! Gets the maximum search level.
	// TODO: add a non-lame description - what is the purpose of the max search level?
	int getMaxSearchLevel() const;
	
	//! Load all the stars from the files.
	void load_data();
	
	//! Draw a nice animated pointer around the object.
	void drawPointer(const Projector* prj, const Navigator * nav);
	
	LinearFader names_fader;
	LinearFader starsFader;
	
	bool flagStarName;
	bool flagStarSciName;
	float maxMagStarName;
	bool gravityLabel;
	
	int max_geodesic_grid_level;
	int last_max_search_level;
	typedef map<int,BigStarCatalogExtension::ZoneArray*> ZoneArrayMap;
	ZoneArrayMap zone_arrays; // index is the grid level
	static void initTriangleFunc(int lev, int index,
				const Vec3d &c0,
				const Vec3d &c1,
				const Vec3d &c2,
				void *context)
	{
		reinterpret_cast<StarMgr*>(context)->initTriangle(lev, index, c0, c1, c2);
	}
	
	void initTriangle(int lev, int index,
			const Vec3d &c0,
			const Vec3d &c1,
			const Vec3d &c2);
	
	BigStarCatalogExtension::HipIndexStruct *hip_index; // array of hiparcos stars
	
	static map<int, QString> common_names_map;
	static map<int, QString> common_names_map_i18n;
	static map<QString, int> common_names_index;
	static map<QString, int> common_names_index_i18n;
	
	static map<int, QString> sci_names_map_i18n;
	static map<QString, int> sci_names_index_i18n;

	static double current_JDay;
	
	double fontSize;
	SFont *starFont;
	static bool flagSciNames;
	Vec3f label_color;
	
	STextureSP texPointer;		// The selection pointer texture
};


#endif // _STAR_MGR_H_
