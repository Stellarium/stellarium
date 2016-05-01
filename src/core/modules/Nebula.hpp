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
	enum CatalogGroupFlags
	{
		CatNGC		= 0x00000001, //!< New General Catalogue (NGC)
		CatIC		= 0x00000002, //!< Index Catalogue (IC)
		CatM		= 0x00000004, //!< Messier Catalog (M)
		CatC		= 0x00000008, //!< Caldwell Catalogue (C)
		CatB		= 0x00000010, //!< Barnard Catalogue (B)
		CatSh2		= 0x00000020, //!< Sharpless Catalogue (Sh 2)
		CatLBN		= 0x00000040, //!< Lynds' Catalogue of Bright Nebulae (LBN)
		CatLDN		= 0x00000080, //!< Lynds' Catalogue of Dark Nebulae (LDN)
		CatRCW		= 0x00000100, //!< A catalogue of Hα-emission regions in the southern Milky Way (RCW)
		CatVdB		= 0x00000200, //!< Van den Bergh Catalogue of reflection nebulae (VdB)
		CatCr		= 0x00000400, //!< Collinder Catalogue (Cr or Col)
		CatMel		= 0x00000800, //!< Melotte Catalogue of Deep Sky Objects (Mel)
		CatPGC		= 0x00001000, //!< HYPERLEDA. I. Catalog of galaxies (PGC)
		CatUGC		= 0x00002000, //!< The Uppsala General Catalogue of Galaxies
		CatCed		= 0x00004000 //!< Cederblad Catalog of bright diffuse Galactic nebulae (Ced)
	};

	enum TypeGroupFlags
	{
		TypeGalaxies		= 0x00000001, //!< Galaxies
		TypeActiveGalaxies	= 0x00000002, //!< Different Active Galaxies
		TypeInteractingGalaxies	= 0x00000004, //!< Different Active Galaxies
		TypeStarClusters	= 0x00000008, //!< Star Clusters
		TypeHydrogenRegions	= 0x00000010, //!< Hydrogen Regions
		TypeBrightNebulae	= 0x00000020, //!< Bright Nebulae
		TypeDarkNebulae		= 0x00000040, //!< Dark Nebulae
		TypePlanetaryNebulae	= 0x00000080, //!< Planetary Nebulae
		TypeSupernovaRemnants	= 0x00000100, //!< Planetary Nebulae
		TypeOther		= 0x00000200  //!< Other objects
	};


	typedef QFlags<CatalogGroupFlags> CatalogGroup;
	Q_FLAGS(CatalogGroup)
	typedef QFlags<TypeGroupFlags> TypeGroup;
	Q_FLAGS(TypeGroup)

	//! A pre-defined set of specifiers for the catalogs filter
	static const CatalogGroupFlags AllCatalogs = (CatalogGroupFlags)(CatNGC|CatIC|CatM|CatC|CatB|CatSh2|CatLBN|CatLDN|CatRCW|CatVdB|CatCr|CatMel|CatPGC|CatUGC|CatCed);
	static const TypeGroupFlags AllTypes = (TypeGroupFlags)(TypeGalaxies|TypeActiveGalaxies|TypeInteractingGalaxies|TypeStarClusters|TypeHydrogenRegions|TypeBrightNebulae|TypeDarkNebulae|TypePlanetaryNebulae|TypeSupernovaRemnants|TypeOther);

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
	virtual double getAngularSize(const StelCore*) const;
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

	//! Get the surface area.
	//! @return surface area in square degrees.
	float getSurfaceArea(void) const;

	void setProperName(QString name) { englishName = name; }

	//! Get designation for DSO (with priority: M, C, NGC, IC, B, Sh2, VdB, RCW, LDN, LBN, Cr, Mel, PGC, UGC, Ced)
	//! @return a designation
	QString getDSODesignation();

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
		NebSNR		= 18,	//!< Supernova remnant
		NebISM		= 19,	//!< Interstellar matter
		NebEMO		= 20,	//!< Emission object
		NebBLL		= 21,	//!< BL Lac object
		NebBLA		= 22,	//!< Blazar
		NebMolCld	= 23, 	//!< Molecular Cloud
		NebYSO		= 24, 	//!< Young Stellar Object
		NebPossQSO	= 25, 	//!< Possible Quasar
		NebPossPN	= 26, 	//!< Possible Planetary Nebula
		NebPPN		= 27, 	//!< Protoplanetary Nebula
		NebStar		= 28, 	//!< Star
		NebUnknown	= 29	//!< Unknown type, catalog errors, "Unidentified Southern Objects" etc.
	};

	//! Translate nebula name using the passed translator
	void translateName(const StelTranslator& trans) {nameI18 = trans.qtranslate(englishName);}

	void readDSO(QDataStream& in);

	void drawLabel(StelPainter& sPainter, float maxMagLabel);
	void drawHints(StelPainter& sPainter, float maxMagHints);

	bool objectInDisplayedType() const;

	//! Get the printable description of morphological nebula type.
	//! @return the nebula morphological type string.
	QString getMorphologicalTypeDescription() const;

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
	QString englishName;            // English name
	QString nameI18;                // Nebula name
	QString mTypeString;		// Morphological type of object (as string)	
	float bMag;                     // B magnitude
	float vMag;                     // V magnitude. For Dark Nebulae, opacity is stored here.	
	float majorAxisSize;		// Major axis size in degrees
	float minorAxisSize;		// Minor axis size in degrees
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

	static StelTextureSP texCircle;                    // The symbolic circle texture
	static StelTextureSP texGalaxy;                    // Type 0
	static StelTextureSP texOpenCluster;               // Type 1
	static StelTextureSP texGlobularCluster;           // Type 2
	static StelTextureSP texPlanetaryNebula;           // Type 3
	static StelTextureSP texDiffuseNebula;             // Type 4
	static StelTextureSP texDarkNebula;                // Type 5
	static StelTextureSP texOpenClusterWithNebulosity; // Type 7
	static float hintsBrightness;

	static Vec3f labelColor;			// The color of labels
	static Vec3f circleColor;			// The color of the symbolic circle texture (default marker; NebUnknown)
	static Vec3f galaxyColor;			// The color of galaxy marker texture (NebGx)
	static Vec3f radioGalaxyColor;			// The color of radio galaxy marker texture (NebRGx)
	static Vec3f activeGalaxyColor;			// The color of active galaxy marker texture (NebAGx)
	static Vec3f interactingGalaxyColor;		// The color of interacting galaxy marker texture (NebIGx)
	static Vec3f quasarColor;			// The color of quasar marker texture (NebQSO)
	static Vec3f nebulaColor;			// The color of nebula marker texture (NebN)
	static Vec3f planetaryNebulaColor;		// The color of planetary nebula marker texture (NebPn)
	static Vec3f reflectionNebulaColor;		// The color of reflection nebula marker texture (NebRn)
	static Vec3f bipolarNebulaColor;		// The color of bipolar nebula marker texture (NebBn)
	static Vec3f emissionNebulaColor;		// The color of emission nebula marker texture (NebEn)
	static Vec3f darkNebulaColor;			// The color of dark nebula marker texture (NebDn)
	static Vec3f hydrogenRegionColor;		// The color of hydrogen region marker texture (NebHII)
	static Vec3f supernovaRemnantColor;		// The color of supernova remnant marker texture (NebSNR)
	static Vec3f interstellarMatterColor;		// The color of interstellar matter marker texture (NebISM)
	static Vec3f clusterWithNebulosityColor;	// The color of cluster associated with nebulosity marker texture (NebCn)
	static Vec3f clusterColor;			// The color of star cluster marker texture (NebCl)
	static Vec3f openClusterColor;			// The color of open star cluster marker texture (NebOc)
	static Vec3f globularClusterColor;		// The color of globular star cluster marker texture (NebGc)
	static Vec3f stellarAssociationColor;		// The color of stellar association marker texture (NebSA)
	static Vec3f starCloudColor;			// The color of star cloud marker texture (NebSC)
	static Vec3f emissionObjectColor;		// The color of emission object marker texture (NebEMO)
	static Vec3f blLacObjectColor;			// The color of BL Lac object marker texture (NebBLL)
	static Vec3f blazarColor;			// The color of blazar marker texture (NebBLA)
	static Vec3f molecularCloudColor;		// The color of molecular cloud marker texture (NebMolCld)
	static Vec3f youngStellarObjectColor;		// The color of Young Stellar Object marker texture (NebYSO)
	static Vec3f possibleQuasarColor;		// The color of possible quasar marker texture (NebPossQSO)
	static Vec3f possiblePlanetaryNebulaColor;	// The color of possible planetary nebula marker texture (NebPossPN)
	static Vec3f protoplanetaryNebulaColor;		// The color of protoplanetary nebula marker texture (NebPPN)
	static Vec3f starColor;				// The color of star marker texture (NebStar)

	static float circleScale;             // Define the scaling of the hints circle. Unused in 0.15. TODO: Remove this? I see no use. (GZ)
	static bool drawHintProportional;     // scale hint with nebula size?
	static bool surfaceBrightnessUsage;

	static bool flagUsageTypeFilter;
	static CatalogGroup catalogFilters;
	static TypeGroup typeFilters;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Nebula::CatalogGroup)
Q_DECLARE_OPERATORS_FOR_FLAGS(Nebula::TypeGroup)

#endif // _NEBULA_HPP_

