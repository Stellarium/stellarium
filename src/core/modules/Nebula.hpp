/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2011 Alexander Wolf
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _NEBULA_HPP_
#define _NEBULA_HPP_

#include <QString>
#include "StelObject.hpp"
#include "StelTranslator.hpp"
#include "StelProjectorType.hpp"

class QDataStream;

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
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the Nebula.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	virtual QString getType() const {return "Nebula";}
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const {return XYZ;}
	virtual double getCloseViewFov(const StelCore* core = NULL) const;
	virtual float getVMagnitude(const StelCore* core, bool withExtinction=false) const;
	virtual float getSelectPriority(const StelCore* core) const;
	virtual Vec3f getInfoColor() const;
	virtual QString getNameI18n() const {return nameI18;}
	virtual QString getEnglishName() const {return englishName;}
	virtual double getAngularSize(const StelCore*) const {return angularSize*0.5;}
	virtual SphericalRegionP getRegion() const {return pointRegion;}

	// Methods specific to Nebula
	void setLabelColor(const Vec3f& v) {labelColor = v;}
	void setCircleColor(const Vec3f& v) {circleColor = v;}

	//! Get the printable nebula Type.
	//! @return the nebula type code.
	QString getTypeString() const;

private:
	friend struct DrawNebulaFuncObject;

	//! Textures used to draw nebula hints.
	struct NebulaHintTextures
	{
		//! The symbolic circle texture.
		class StelTextureNew* texCircle;  
		//! The open cluster marker texture.
		class StelTextureNew* texOpenCluster;
		//! The globular cluster marker texture.
		class StelTextureNew* texGlobularCluster;
		//! The planetary nebula marker texture.
		class StelTextureNew* texPlanetaryNebula;
		//! Are we initialized yet?
		bool initialized;

		//! Default constructor - construct uninitialized NebulaHintTextures.
		NebulaHintTextures(): initialized(false){}
		//! Destructor - frees resources if initialized.
		~NebulaHintTextures();
		//! Lazily initialize the data, using given renderer to create textures/shader.
		void lazyInit(class StelRenderer* renderer);
	};
	
	//! @enum NebulaType Nebula types
	enum NebulaType
	{
		NebGx=0,     //!< Galaxy
		NebOc=1,     //!< Open star cluster
		NebGc=2,     //!< Globular star cluster, usually in the Milky Way Galaxy
		NebN=3,      //!< Bright emission or reflection nebula
		NebPn=4,     //!< Planetary nebula
		NebDn=5,     //!< ??? 
		NebIg=6,     //!< ??? 
		NebCn=7,     //!< Cluster associated with nebulosity
		NebUnknown=8 //!< Unknown type
	};

	//! Translate nebula name using the passed translator
	void translateName(StelTranslator& trans) {nameI18 = trans.qtranslate(englishName);}

	bool readNGC(char *record);
	void readNGC(QDataStream& in);
			
	void drawHints(StelRenderer* renderer, float maxMagHints, NebulaHintTextures& hintTextures);
	void drawLabel(StelRenderer* renderer, StelProjectorP projector, float maxMagLabel);

	unsigned int M_nb;              // Messier Catalog number
	unsigned int NGC_nb;            // New General Catalog number
	unsigned int IC_nb;             // Index Catalog number
	QString englishName;            // English name
	QString nameI18;                // Nebula name
	float mag;                      // Apparent magnitude
	float angularSize;              // Angular size in degree
	Vec3d XYZ;                      // Cartesian equatorial position
	Vec3d XY;                       // Store temporary 2D position
	NebulaType nType;

	SphericalRegionP pointRegion;

	static float hintsBrightness;

	static Vec3f labelColor, circleColor;
	static float circleScale;       // Define the scaling of the hints circle
};

#endif // _NEBULA_HPP_

