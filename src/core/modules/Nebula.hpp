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

	void setProperName(QString name) { englishName = name; }

private:
	friend struct DrawNebulaFuncObject;
	
	//! @enum NebulaType Nebula types
	enum NebulaType
	{
		NebGx		= 0,	//!< Galaxy
		NebAGx		= 1,	//!< Active galaxy
		NebRGx		= 2,	//!< Radio galaxy
		NebIGx		= 3,	//!< Interacting galaxy
		NebQSO		= 4,	//!< Quasar
		NebCl		= 5,	//!< Star cluster
		NebOc		= 6,	//!< Open star cluster
		NebGc		= 7,	//!< Globular star cluster, usually in the Milky Way Galaxy
		NebSA		= 8,	//!< Stellar association
		NebSC		= 9,	//!< Star cloud
		NebN		= 10,	//!< A nebula
		NebPn		= 11,	//!< Planetary nebula
		NebDn		= 12,	//!< Dark Nebula
		NebRn		= 13,	//!< Reflection nebula
		NebBn		= 14,	//!< Bipolar nebula
		NebEn		= 15,	//!< Emission nebula
		NebCn		= 16,	//!< Cluster associated with nebulosity
		NebHII		= 17,	//!< HII Region
		NebHa		= 18,	//!< H-α emission region
		NebSNR		= 19,	//!< Supernova remnant
		NebISM		= 20,	//!< Interstellar matter
		NebEMO		= 21,	//!< Emission object
		NebUnknown	= 22	//!< Unknown type, catalog errors, "Unidentified Southern Objects" etc.
	};

	//! Translate nebula name using the passed translator
	void translateName(const StelTranslator& trans) {nameI18 = trans.qtranslate(englishName);}

	void readDSO(QDataStream& in);

	void drawLabel(StelPainter& sPainter, float maxMagLabel);
	void drawHints(StelPainter& sPainter, float maxMagHints);

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
	unsigned int UGC_nb;            // UGC number (The Uppsala General Catalogue of Galaxies)
	QString Ced_nb;			// Ced number (Cederblad Catalog of bright diffuse Galactic nebulae)	
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
	float oDistance;		// distance (Mpc for galaxies, kpc for other objects)
	float oDistanceErr;		// Error of distance (Mpc for galaxies, kpc for other objects)
	float redshift;
	float redshiftErr;
	float parallax;
	float parallaxErr;
	Vec3d XYZ;                      // Cartesian equatorial position (J2000.0)
	Vec3d XY;                       // Store temporary 2D position
	NebulaType nType;

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

