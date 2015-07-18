/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2011 Alexander Wolf
 * Copyright (C) 2015 Georg Zotti
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

#include "StelObject.hpp"
#include "StelTranslator.hpp"
#include "StelTextureTypes.hpp"

#include <QString>

class StelPainter;
class QDataStream;

// This only draws nebula icons. For the DSO images, see StelSkylayerMgr and StelSkyImageTile.
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
	//! - Extra (contains the Nebula type, which might be "Galaxy", "Cluster" or similar)
	//! - PlainText
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the Nebula.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	virtual QString getType() const {return "Nebula";}
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const {return XYZ;}
	virtual double getCloseViewFov(const StelCore* core = NULL) const;
	virtual float getVMagnitude(const StelCore* core) const;
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

	//! Get the printable morphological nebula Type.
	//! @return the nebula morphological type string.
	QString getMorphologicalTypeString() const;

	float getSurfaceBrightness(const StelCore* core) const;
	float getSurfaceBrightnessWithExtinction(const StelCore* core) const;

private:
	friend struct DrawNebulaFuncObject;
	
	//! @enum NebulaType Nebula types
	enum NebulaType
	{
		NebGx		= 0,	//!< Galaxy
		NebSy2		= 1,	//!< Seyfert 2 Galaxy
		NebOc		= 2,	//!< Open star cluster
		NebGc		= 3,	//!< Globular star cluster, usually in the Milky Way Galaxy
		NebN		= 4,	//!< A nebula
		NebPn		= 5,	//!< Planetary nebula
		NebDn		= 6,	//!< Dark Nebula
		NebRn		= 7,	//!< Reflection nebula
		NebBn		= 8,	//!< Bipolar nebula
		NebEn		= 9,	//!< Emission nebula
		NebCn		= 10,	//!< Cluster associated with nebulosity
		NebHII		= 11,	//!< HII Region
		NebHa		= 12,	//!< H-α emission region
		NebSNR		= 13,	//!< Supernova remnant
		NebSA		= 14,	//!< Stellar association
		NebSC		= 15,	//!< Star cloud
		NebUnknown	= 16	//!< Unknown type, catalog errors, "Unidentified Southern Objects" etc.
	};

	//! @enum HIIFormType HII region form types
	enum HIIFormType
	{
		FormCir=1,   //!< circular form
		FormEll=2,   //!< elliptical form
		FormIrr=3    //!< irregular form
	};

	//! @enum HIIStructureType HII region structure types
	enum HIIStructureType
	{
		StructureAmo=1,   //!< amorphous structure
		StructureCon=2,   //!< conventional structure
		StructureFil=3    //!< filamentary structure
	};

	//! @enum HIIBrightnessType HII region brightness types
	enum HIIBrightnessType
	{
		Faintest=1,
		Moderate=2,
		Brightest=3
	};

	//! @enum HaBrightnessType H-α emission region brightness types
	enum HaBrightnessType
	{
		HaFaint=1,
		HaMedium=2,
		HaBright=3,
		HaVeryBright=4
	};

	//! Translate nebula name using the passed translator
	void translateName(const StelTranslator& trans) {nameI18 = trans.qtranslate(englishName);}

	bool readNGC(char *record);
	void readNGC(QDataStream& in);
	bool readBarnard(QString record);
	bool readSharpless(QString record);
	bool readVandenBergh(QString record);
	bool readRCW(QString record);
	bool readLDN(QString record);
	bool readLBN(QString record);

	// ----------------------------------------------
	void readDSO(QDataStream& in);
	// ----------------------------------------------

	void drawLabel(StelPainter& sPainter, float maxMagLabel);
	void drawHints(StelPainter& sPainter, float maxMagHints);


	//! Get the printable HII region form type.
	QString getHIIFormTypeString() const;
	//! Get the printable HII region structure type.
	QString getHIIStructureTypeString() const;
	//! Get the printable HII region brightness type.
	QString getHIIBrightnessTypeString() const;
	//! Get the printable H-α emission region brightness type.
	QString getHaBrightnessTypeString() const;

	unsigned int DSO_nb;
	unsigned int M_nb;              // Messier Catalog number
	unsigned int NGC_nb;            // New General Catalog number
	unsigned int IC_nb;             // Index Catalog number
	unsigned int C_nb;              // Caldwell Catalog number
	unsigned int B_nb;              // Barnard Catalog number (Dark Nebulae)
	unsigned int Sh2_nb;            // Sharpless Catalog number (Catalogue of HII Regions (Sharpless, 1959))
	unsigned int VdB_nb;            // Van den Bergh Catalog number (Catalogue of Reflection Nebulae (Van den Bergh, 1966))
	unsigned int RCW_nb;            // RCW Catalog number (H-α emission regions in Southern Milky Way (Rodgers+, 1960))
	unsigned int LDN_nb;            // LDN Catalog number (Lynds' Catalogue of Dark Nebulae (Lynds, 1962))
	unsigned int LBN_nb;            // LBN Catalog number (Lynds' Catalogue of Bright Nebulae (Lynds, 1965))
	unsigned int Cr_nb;             // Collinder Catalog number
	unsigned int Mel_nb;            // Melotte Catalog number
	unsigned int PGC_nb;            // PGC number (Catalog of galaxies)
	unsigned int Ced_nb;		// Ced number (Cederblad Catalog of bright diffuse Galactic nebulae)
	QString PK_nb;			// PK number (Catalogue of galactic planetary nebulae (Perek-Kohoutek))
	QString since;			// JD of the nebula formation
	QString englishName;            // English name
	QString nameI18;                // Nebula name
	QString mTypeString;		// Morphological type of object (as string)
	float mag;                      // Apparent magnitude. For Dark Nebulae, opacity is stored here. -- OUTDATED!
	float bMag;                     // B magnitude
	float vMag;                     // V magnitude. For Dark Nebulae, opacity is stored here.
	float angularSize;              // Angular size in degree -- OUTDATED!
	float majorAxisSize;		// Major axis size in arcmin
	float minorAxisSize;		// Minor axis size in arcmin
	int orientationAngle;		// Orientation angle in degrees
	float radialVelocity;		// Radial velocity in km/s
	float radialVelocityErr;	// Error of radial velocity in km/s
	float redshift;			// Redshift
	float redshiftErr;		// Error of redshift
	float parallax;			// Parallax in mas
	float parallaxErr;		// Error of parallax in mas
	Vec3d XYZ;                      // Cartesian equatorial position (J2000.0)
	Vec3d XY;                       // Store temporary 2D position
	NebulaType nType;

	HIIFormType formType;
	HIIStructureType structureType;
	HIIBrightnessType brightnessType;
	HaBrightnessType rcwBrightnessType;

	int brightnessClass;

	SphericalRegionP pointRegion;

	static StelTextureSP texCircle;   // The symbolic circle texture
	static StelTextureSP texGalaxy;	                   // Type 0
	static StelTextureSP texOpenCluster;               // Type 1
	static StelTextureSP texGlobularCluster;           // Type 2
	static StelTextureSP texPlanetaryNebula;           // Type 3
	static StelTextureSP texDiffuseNebula;             // Type 4
	static StelTextureSP texDarkNebula;                // Type 5
	static StelTextureSP texOpenClusterWithNebulosity; // Type 7
	static float hintsBrightness;

	static Vec3f labelColor, circleColor, galaxyColor, brightNebulaColor, darkNebulaColor, clusterColor;
	static float circleScale;       // Define the scaling of the hints circle
	static bool drawHintProportional; // scale hint with nebula size?
};

#endif // _NEBULA_HPP_

