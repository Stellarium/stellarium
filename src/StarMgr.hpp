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
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "Fader.hpp"
#include "StelObjectModule.hpp"
#include "STextureTypes.hpp"

using namespace std ;

class StelObject;
class ToneReproducer;
class Projector;
class Navigator;
class LoadingBar;

class ZoneArray;
class HipIndexStruct;

class StarMgr : public StelObjectModule
{
public:
    StarMgr(void);
    ~StarMgr(void);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init(const InitParser& conf, LoadingBar& lb);
	virtual string getModuleID() const {return "stars";}
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye); //! Draw all the stars
	virtual void update(double deltaTime) {names_fader.update((int)(deltaTime*1000)); starsFader.update((int)(deltaTime*1000));}
	virtual void updateI18n();
	virtual void updateSkyCulture(LoadingBar& lb);
	virtual void setColorScheme(const InitParser& conf, const std::string& section);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Return a stl vector containing the stars located inside the lim_fov circle around position v
	virtual vector<boost::intrusive_ptr<StelObject> > searchAround(const Vec3d& v, double limitFov, const Navigator * nav, const Projector * prj) const;
	
	//! Return the matching Stars object's pointer if exists or NULL
    //! @param nameI18n The case sensistive star common name or HP
    //! catalog name (format can be HP1234 or HP 1234) or sci name
	virtual boost::intrusive_ptr<StelObject> searchByNameI18n(const wstring& nameI18n) const;
	
	//! @brief Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
	virtual vector<wstring> listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem=5) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
    int getMaxGridLevel(void) const {return max_geodesic_grid_level;}
    void setGrid(void);

    int getMaxSearchLevel(const ToneReproducer *eye,
                          const Projector *prj) const;
   
    void set_names_fade_duration(float duration) {names_fader.set_duration((int) (duration * 1000.f));}
    int load_common_names(const string& commonNameFile);
    void load_sci_names(const string& sciNameFile);

    boost::intrusive_ptr<StelObject> search(Vec3d Pos) const;     // Search the star by position
    boost::intrusive_ptr<StelObject> search(const string&) const; // Search the star by string (incl catalog prefix)
    boost::intrusive_ptr<StelObject> searchHP(int) const;         // Search the star by HP number
    
    void setLabelColor(const Vec3f& c) {label_color = c;}
    Vec3f getLabelColor(void) const {return label_color;}

    void setCircleColor(const Vec3f& c) {circle_color = c;}
    Vec3f getCircleColor(void) const {return circle_color;}
    
    /** Set display flag for Stars */
    void setFlagStars(bool b) {starsFader=b;}
    /** Get display flag for Stars */
    bool getFlagStars(void) const {return starsFader==true;}    
    
    /** Set display flag for Star names */
    void setFlagNames(bool b) {names_fader=b;}
    /** Get display flag for Star names */
    bool getFlagNames(void) const {return names_fader==true;}
    
    /** Set display flag for Star Scientific names */
    void setFlagStarsSciNames(bool b) {flagStarSciName=b;}
    /** Get display flag for Star Scientific names */
    bool getFlagStarsSciNames(void) const {return flagStarSciName;}    
    
    /** Set flag for Star twinkling */
    void setFlagTwinkle(bool b) {flagStarTwinkle=b;}
    /** Get flag for Star twinkling */
    bool getFlagTwinkle(void) const {return flagStarTwinkle;}    
        
    /** Set flag for displaying Star as GLpoints (faster but not so nice) */
    void setFlagPointStar(bool b) {flagPointStar=b;}
    /** Get flag for displaying Star as GLpoints (faster but not so nice) */
    bool getFlagPointStar(void) const {return flagPointStar;}    
    
    /** Set maximum magnitude at which stars names are displayed */
    void setMaxMagName(float b) {maxMagStarName=b;}
    /** Get maximum magnitude at which stars names are displayed */
    float getMaxMagName(void) const {return maxMagStarName;} 
        
    /** Set base stars display scaling factor */
    void setScale(float b) {starScale=b;}
    /** Get base stars display scaling factor */
    float getScale(void) const {return starScale;}            
    
    /** Set stars display scaling factor wrt magnitude */
    void setMagScale(float b) {starMagScale=b;}
    /** Get base stars display scaling factor wrt magnitude */
    float getMagScale(void) const {return starMagScale;}

    /** Set stars twinkle amount */
    void setTwinkleAmount(float b) {twinkleAmount=b;}
    /** Get stars twinkle amount */
    float getTwinkleAmount(void) const {return twinkleAmount;}
    
    /** Set stars limiting display magnitude */
    void setLimitingMag(float f) {limitingMag = f;}
    /** Get stars limiting display magnitude */
    float getLimitingMag(void) const {return limitingMag;}
        
    //! Define font size to use for star names display
	void setFontSize(double newFontSize);
    
    //! show scientific or catalog names on stars without common names
    static void setFlagSciNames(bool f) {flagSciNames = f;}
    static bool getFlagSciNames(void) {return flagSciNames;}

    int drawStar(const Projector *prj, const Vec3d &XY,float rmag,const Vec3f &color) const;
    //! Draw the star rendered as GLpoint. This may be faster but it is not so nice
    int drawPointStar(const Projector *prj, const Vec3d &XY,float rmag,const Vec3f &color) const;

    static wstring getCommonName(int hip);
    static wstring getSciName(int hip);
private:
 
    // Load all the stars from the files
    void load_data(LoadingBar& lb);
    
	//! Draw a nice animated pointer around the object
	void drawPointer(const Projector* prj, const Navigator * nav);
	
    LinearFader names_fader;
    LinearFader starsFader;

    float starScale;
    float starMagScale;
    bool flagStarName;
    bool flagStarSciName;
    float maxMagStarName;
    bool flagStarTwinkle;
    float twinkleAmount;
    bool flagPointStar;
    bool gravityLabel;
    float limitingMag;                  // limiting magnitude at 60 degree fov
    
    STextureSP starTexture;                // star texture

    int max_geodesic_grid_level;
    int last_max_search_level;
    typedef map<int,ZoneArray*> ZoneArrayMap;
    ZoneArrayMap zone_arrays; // index is the grid level
    static void initTriangleFunc(int lev,int index,
                                 const Vec3d &c0,
                                 const Vec3d &c1,
                                 const Vec3d &c2,
                                 void *context) {
      reinterpret_cast<StarMgr*>(context)->initTriangle(lev,index,c0,c1,c2);
    }
    void initTriangle(int lev,int index,
                      const Vec3d &c0,
                      const Vec3d &c1,
                      const Vec3d &c2);
    HipIndexStruct *hip_index; // array of hiparcos stars

    static map<int,string> common_names_map;
    static map<int,wstring> common_names_map_i18n;
    static map<string,int> common_names_index;
    static map<wstring,int> common_names_index_i18n;

    static map<int,wstring> sci_names_map_i18n;
    static map<wstring,int> sci_names_index_i18n;
    
	double fontSize;
    SFont *starFont;
    static bool flagSciNames;
    Vec3f label_color,circle_color;
    float twinkle_amount;
    float star_scale;
    
    STextureSP texPointer;			// The selection pointer texture
};


#endif // _STAR_MGR_H_
