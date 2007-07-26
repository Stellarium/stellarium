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
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Return a stl vector containing the stars located inside the lim_fov circle around position v
	virtual vector<StelObjectP > searchAround(const Vec3d& v, double limitFov, const Navigator * nav, const Projector * prj) const;
	
	//! Return the matching Stars object's pointer if exists or NULL
    //! @param nameI18n The case sensistive star common name or HP
    //! catalog name (format can be HP1234 or HP 1234) or sci name
	virtual StelObjectP searchByNameI18n(const wstring& nameI18n) const;	

	//! Return the matching star if exists or NULL
	//! @param name The case sensistive standard program planet name
	virtual StelObjectP searchByName(const string& name) const;

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

    StelObjectP search(Vec3d Pos) const;     // Search the star by position
    StelObjectP search(const string&) const; // Search the star by string (incl catalog prefix)
    StelObjectP searchHP(int) const;         // Search the star by HP number
    
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
    
    /** Set stars limiting display magnitude at 60 degree fov */
//    void setLimitingMag(float f) {setMagConverterMaxScaled60DegMag(f);}
    /** Get stars limiting display magnitude at 60 degree fov */
//    float getLimitingMag(void) const
//      {return getMagConverterMaxScaled60DegMag();}


      /** Set MagConverter maximum FOV
          Usually stars/planet halos are drawn fainter when FOV gets larger.
          But when FOV gets larger than this value, the stars do not become
          fainter any more. Must be >= 60.0 */
    void setMagConverterMaxFov(float x) {mag_converter->setMaxFov(x);}
      /** Set MagConverter minimum FOV
          Usually stars/planet halos are drawn brighter when FOV gets smaller.
          But when FOV gets smaller than this value, the stars do not become
          brighter any more. Must be <= 60.0 */
    void setMagConverterMinFov(float x) {mag_converter->setMinFov(x);}
      /** Set MagConverter magnitude shift
          draw the stars/planet halos as if they were brighter of fainter
          by this amount of magnitude */
    void setMagConverterMagShift(float x) {mag_converter->setMagShift(x);}
      /** Set MagConverter maximum magnitude
          stars/planet halos, whose original (unshifted) magnitude is greater
          than this value will not be drawn */
    void setMagConverterMaxMag(float mag) {mag_converter->setMaxMag(mag);}
      /** Set MagConverter maximum scaled magnitude wrt 60 degree FOV
          stars/planet halos, whose original (unshifted) magnitude is greater
          than this value will not be drawn at 60 degree FOV */
    void setMagConverterMaxScaled60DegMag(float mag)
      {mag_converter->setMaxScaled60DegMag(mag);}

      /** Get MagConverter maximum FOV */
    float getMagConverterMaxFov(void) const {return mag_converter->getMaxFov();}
      /** Get MagConverter minimum FOV */
    float getMagConverterMinFov(void) const {return mag_converter->getMinFov();}
      /** Get MagConverter magnitude shift */
    float getMagConverterMagShift(void) const {return mag_converter->getMagShift();}
      /** Get MagConverter maximum magnitude */
    float getMagConverterMaxMag(void) const {return mag_converter->getMaxMag();}
      /** Get MagConverter maximum scaled magnitude wrt 60 degree FOV */
    float getMagConverterMaxScaled60DegMag(void) const
      {return mag_converter->getMaxScaled60DegMag();}

      /** Compute RMag and CMag from magnitude.
          Useful for conststent drawing of Planet halos */
    int computeRCMag(float mag,bool point_star,float fov,
                     const ToneReproducer *eye,float rc_mag[2]) const
      {mag_converter->setFov(fov);
       mag_converter->setEye(eye);
       return mag_converter->computeRCMag(mag,point_star,eye,rc_mag);}

        
    //! Define font size to use for star names display
	void setFontSize(double newFontSize);
    
    //! show scientific or catalog names on stars without common names
    static void setFlagSciNames(bool f) {flagSciNames = f;}
    static bool getFlagSciNames(void) {return flagSciNames;}

    int drawStar(const Projector *prj, const Vec3d &XY,
                 const float rc_mag[2],const Vec3f &color) const;

    static wstring getCommonName(int hip);
    static wstring getSciName(int hip);
private:
 
    // Load all the stars from the files
    void load_data(const InitParser &conf,LoadingBar &lb);
    
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

    class MagConverter {
    public:
      MagConverter(const StarMgr &mgr) : mgr(mgr) {
        setMaxFov(180.f);
        setMinFov(0.1f);
        setFov(180.f);
        setMagShift(0.f);
        setMaxMag(30.f);
        min_rmag = 0.01f;
      }
      void setMaxFov(float fov) {max_fov = (fov<60.f)?60.f:fov;}
      void setMinFov(float fov) {min_fov = (fov>60.f)?60.f:fov;}
      void setMagShift(float d) {mag_shift = d;}
      void setMaxMag(float mag) {max_mag = mag;}
      void setMaxScaled60DegMag(float mag) {max_scaled_60deg_mag = mag;}
      float getMaxFov(void) const {return max_fov;}
      float getMinFov(void) const {return min_fov;}
      float getMagShift(void) const {return mag_shift;}
      float getMaxMag(void) const {return max_mag;}
      float getMaxScaled60DegMag(void) const {return max_scaled_60deg_mag;}
      void setFov(float fov);
      void setEye(const ToneReproducer *eye);
      int computeRCMag(float mag,bool point_star,
                       const ToneReproducer *eye,float rc_mag[2]) const;
    private:
      const StarMgr &mgr;
      float max_fov,min_fov,mag_shift,max_mag,max_scaled_60deg_mag,
            min_rmag,fov_factor;
    };
    MagConverter *mag_converter;

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
    
    STextureSP texPointer;			// The selection pointer texture
};


#endif // _STAR_MGR_H_
