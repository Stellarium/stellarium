/*
 * Author and Copyright of this file and of the stellarium telescope feature:
 * Johannes Gajdosik, 2006
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

#ifndef _TELESCOPEMGR_HPP_
#define _TELESCOPEMGR_HPP_

#include "StelObjectModule.hpp"
#include "Fader.hpp"
#include "vecmath.h"
#include "STextureTypes.hpp"

#include <vector>
#include <map>
#include <QString>
#include <QStringList>

class Projector;
class Navigator;
class SFont;
class StelObject;
class Telescope;
class QSettings;

class TelescopeMgr : public StelObjectModule {
public:
  TelescopeMgr(void);
  virtual ~TelescopeMgr(void);
  
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void draw(StelCore *core);
	virtual void update(double deltaTime);
	virtual void setStelStyle(const StelStyle& style);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelObjectModule class
	virtual vector<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;
	virtual StelObjectP searchByName(const QString& name) const;

	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;


  void communicate(void);
   
  void set_names_fade_duration(float duration)
    {nameFader.setDuration((int) (duration * 1000.f));}
  
  void setLabelColor(const Vec3f &c) {labelColor = c;}
  const Vec3f &getLabelColor(void) const {return labelColor;}

  void set_circleColor(const Vec3f &c) {circleColor = c;}
  const Vec3f &getCircleColor(void) const {return circleColor;}
  
  //! Set display flag for Telescopes
  void setFlagTelescopes(bool b) {telescopeFader=b;}
  //! Get display flag for Telescopes
  bool getFlagTelescopes(void) const {return (bool)telescopeFader;}  
  
  //! Set display flag for Telescope names
  void setFlagTelescopeName(bool b) {nameFader=b;}
  //! Get display flag for Telescope names
  bool getFlagTelescopeName(void) const {return nameFader==true;}
  
  //! Define font size to use for telescope names display
  void setFontSize(float fontSize);

  //! send a J2000-goto-command to the specified telescope
  void telescopeGoto(int telescope_nr,const Vec3d &j2000Pos);
private:
#ifdef WIN32
  bool wsaOk;
#endif
	//! Draw a nice animated pointer around the object
	void drawPointer(const Projector* prj, const Navigator * nav);

  LinearFader nameFader;
  LinearFader telescopeFader;
  Vec3f circleColor;
  Vec3f labelColor;
  SFont *telescope_font;
  STextureSP telescopeTexture;

  class TelescopeMap : public std::map<int,Telescope*> {
  public:
    ~TelescopeMap(void) {clear();}
    void clear(void);
  };
  TelescopeMap telescope_map;
  STextureSP texPointer;   // The selection pointer texture
};


#endif // _TELESCOPEMGR_HPP_
