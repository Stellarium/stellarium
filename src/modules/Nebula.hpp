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

#ifndef _NEBULA_HPP_
#define _NEBULA_HPP_

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

	//! Nebula support the following InfoStringGroup flags:
	//! - Name
	//! - CatalogNumber
	//! - Magnitude
	//! - RaDec
	//! - AltAzi
	//! - Distance
	//! - Size
	//! - Extra1 (contains the Nebula type, which might be "Galaxy", "Cluster" or similar)
	//! - PlainText
	//! @param core the Stelore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the Nebula.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	virtual QString getType(void) const {return "Nebula";}
	virtual Vec3d getJ2000EquatorialPos(const Navigator *nav) const {return XYZ;}
	virtual double getCloseViewFov(const Navigator * nav = NULL) const;
	virtual float getVMagnitude(const Navigator * nav = NULL) const {return mag;}
	virtual float getSelectPriority(const Navigator *nav) const;
	virtual Vec3f getInfoColor(void) const;
	virtual QString getNameI18n(void) const {return nameI18;}
	virtual QString getEnglishName(void) const {return englishName;}
	virtual double getAngularSize(const StelCore *core) const {return angularSize/2;}
	
	// Methods specific to Nebula
	void setLabelColor(const Vec3f& v) {labelColor = v;}
	void setCircleColor(const Vec3f& v) {circleColor = v;}

	//! Get the printable nebula Type.
	//! @return the nebula type code.
	QString getTypeString(void) const;
	
private:
	
	//! @enum NebulaType Nebula types
	enum NebulaType 
	{ 
		NebGx,     //!< Galaxy
		NebOc,     //!< Open star cluster
		NebGc,     //!< Globular star cluster, usually in the Milky Way Galaxy
		NebN,      //!< Bright emission or reflection nebula
		NebPn,     //!< Planetary nebula
		NebDn,     //!< ???
		NebIg,     //!< ???
		NebCn,     //!< Cluster associated with nebulosity
		NebUnknown //!< Unknown type
	};
	
	//! Translate nebula name using the passed translator
	void translateName(Translator& trans) {nameI18 = trans.qtranslate(englishName);}

	bool readNGC(char *record);
	
	void drawLabel(const StelCore* core, float maxMagLabel);
	void drawHints(const StelCore* core, float maxMagHints);
    
	unsigned int M_nb;              // Messier Catalog number
	unsigned int NGC_nb;            // New General Catalog number
	unsigned int IC_nb;             // Index Catalog number
	QString englishName;            // English name
	QString nameI18;                // Nebula name
	float mag;                      // Apparent magnitude
	float angularSize;              // Angular size in degree
	Vec3f XYZ;                      // Cartesian equatorial position
	Vec3d XY;                       // Store temporary 2D position
	NebulaType nType;

	static STextureSP texCircle;   // The symbolic circle texture
	static SFont* nebulaFont;      // Font used for names printing
	static float hintsBrightness;

	static Vec3f labelColor, circleColor;
	static float circleScale;       // Define the scaling of the hints circle
};

#endif // _NEBULA_HPP_

