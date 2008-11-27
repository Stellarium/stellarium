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
#include "StelFader.hpp"
#include "vecmath.h"
#include "StelTextureTypes.hpp"
#include "StelProjectorType.hpp"

#include <QString>
#include <QStringList>
#include <QMap>

class StelProjector;
class Navigator;
class StelFont;
class StelObject;
class Telescope;
class StelPainter;

class TelescopeMgr : public StelObjectModule
{
	Q_OBJECT;

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
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;
	virtual StelObjectP searchByName(const QString& name) const;
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to TelescopeMgr
	//! send a J2000-goto-command to the specified telescope
	//! @param telescopeNr the number of the telescope
	//! @param j2000Pos the direction in equatorial J2000 frame
	void telescopeGoto(int telescopeNr, const Vec3d &j2000Pos);
	
	//! Remove all currently registered telescopes
	void deleteAllTelescopes();

public slots:
	//! Set display flag for Telescopes
	void setFlagTelescopes(bool b) {telescopeFader=b;}
	//! Get display flag for Telescopes
	bool getFlagTelescopes() const {return (bool)telescopeFader;}  
	
	//! Set display flag for Telescope names
	void setFlagTelescopeName(bool b) {nameFader=b;}
	//! Get display flag for Telescope names
	bool getFlagTelescopeName() const {return nameFader==true;}
	
	//! Set the telescope circle color
	void setCircleColor(const Vec3f &c) {circleColor = c;}
	//! Get the telescope circle color
	const Vec3f& getCircleColor() const {return circleColor;}
	
	//! Get the telescope labels color
	const Vec3f& getLabelColor() const {return labelColor;}
	//! Set the telescope labels color
	void setLabelColor(const Vec3f &c) {labelColor = c;}
	
	//! Define font size to use for telescope names display
	void setFontSize(float fontSize);
	
	//! For use from the GUI.  The telescope number will be
	//! chosen according to the action which triggered the slot to be
	//! triggered.
	void moveTelescopeToSelected(void);
	
private:
	//! Draw a nice animated pointer around the object
	void drawPointer(const StelProjectorP& prj, const Navigator* nav, const StelPainter& sPainter);

	//! Perform the communication with the telescope servers
	void communicate(void);
	
	LinearFader nameFader;
	LinearFader telescopeFader;
	Vec3f circleColor;
	Vec3f labelColor;
	StelFont *telescope_font;
	StelTextureSP telescopeTexture;

	QMap<int, Telescope*> telescope_map;
	
	// The selection pointer texture
	StelTextureSP texPointer;
};


#endif // _TELESCOPEMGR_HPP_
