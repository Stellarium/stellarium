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

#ifndef _NEBULA_H_
#define _NEBULA_H_

#include <QString>
#include "StelObject.hpp"
#include "Projector.hpp"
#include "Navigator.hpp"
#include "StelCore.hpp"
#include "Translator.hpp"
#include "STextureTypes.hpp"

class SFont;

class Nebula : public StelObject
{
friend class NebulaMgr;
public:
	Nebula();
	~Nebula();

	//! Get a textual description of the object
	//! @param core the StelCore object
	//! @param flags StelObject::InfoStringGroups to be included in the return value.
	//! The StelObject::InfoStringGroups Extra1 is the "type" - e.g. "Galaxy" or "Open Cluster".
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	virtual QString getShortInfoString(const StelCore *core) const;
	virtual QString getType(void) const {return "Nebula";}
	virtual Vec3d getObsJ2000Pos(const Navigator *nav) const {return XYZ;}
	virtual double getCloseViewFov(const Navigator * nav = NULL) const;
	virtual float getMagnitude(const Navigator * nav = NULL) const {return mag;}
	virtual float getSelectPriority(const Navigator *nav) const;
	virtual Vec3f getInfoColor(void) const;
	virtual QString getNameI18n(void) const {return nameI18;}
	virtual QString getEnglishName(void) const {return englishName;}
	virtual double getAngularSize(const StelCore *core) const {return angularSize/2;}
	
	// Methods specific to Nebula
	void setLabelColor(const Vec3f& v) {label_color = v;}
	void setCircleColor(const Vec3f& v) {circle_color = v;}

	//! Get the printable nebula Type.
	//! @return the nebula type code.
	QString getTypeString(void) const;
	
private:
	
	//!	NEB_GX Galaxy
	//! NEB_OC Open star cluster
	//! NEB_GC Globular star cluster, usually in the Milky Way Galaxy
	//! NEB_N Bright emission or reflection nebula
	//! NEB_PN Planetary nebula
	//! CN Cluster associated with nebulosity
	//! Other Undocumented type
	enum nebula_type { NEB_GX, NEB_OC, NEB_GC, NEB_N, NEB_PN, NEB_DN, NEB_IG, NEB_CN, NEB_UNKNOWN };
	
	//! Translate nebula name using the passed translator
	void translateName(Translator& trans) {nameI18 = trans.qtranslate(englishName);}

	bool readNGC(char *record);
	
	void drawLabel(const StelCore* core, float maxMagLabel);
	void drawHints(const StelCore* core, float maxMagHints);
    
	unsigned int M_nb;				// Messier Catalog number
	unsigned int NGC_nb;			// New General Catalog number
	unsigned int IC_nb;				// Index Catalog number
	QString englishName;			// English name
	QString nameI18;                // Nebula name
	float mag;                      // Apparent magnitude
	float angularSize;				// Angular size in degree
	Vec3f XYZ;                      // Cartesian equatorial position
	Vec3d XY;                       // Store temporary 2D position
	nebula_type nType;

	static STextureSP tex_circle;   // The symbolic circle texture
	static SFont* nebula_font;      // Font used for names printing
	static float hints_brightness;

	static Vec3f label_color, circle_color;
	static float circleScale;       // Define the scaling of the hints circle
};

#endif // _NEBULA_H_
