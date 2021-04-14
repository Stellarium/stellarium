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

#ifndef NEBULA_HPP
#define NEBULA_HPP

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

	//Required for the correct working of the Q_FLAGS macro (which requires a MOC pass)
	Q_GADGET
	Q_FLAGS(CatalogGroup)
	Q_FLAGS(TypeGroup)
	Q_ENUMS(NebulaType)
public:
	static const QString NEBULA_TYPE;

	enum CatalogGroupFlags
	{
		CatNGC		= 0x00000001, //!< New General Catalogue (NGC)
		CatIC			= 0x00000002, //!< Index Catalogue (IC)
		CatM			= 0x00000004, //!< Messier Catalog (M)
		CatC			= 0x00000008, //!< Caldwell Catalogue (C)
		CatB			= 0x00000010, //!< Barnard Catalogue (B)
		CatSh2		= 0x00000020, //!< Sharpless Catalogue (Sh 2)
		CatLBN		= 0x00000040, //!< Lynds' Catalogue of Bright Nebulae (LBN)
		CatLDN		= 0x00000080, //!< Lynds' Catalogue of Dark Nebulae (LDN)
		CatRCW		= 0x00000100, //!< A catalogue of Hα-emission regions in the southern Milky Way (RCW)
		CatVdB		= 0x00000200, //!< van den Bergh Catalogue of reflection nebulae (vdB)
		CatCr		= 0x00000400, //!< Collinder Catalogue (Cr or Col)
		CatMel		= 0x00000800, //!< Melotte Catalogue of Deep Sky Objects (Mel)
		CatPGC		= 0x00001000, //!< HYPERLEDA. I. Catalog of galaxies (PGC)
		CatUGC		= 0x00002000, //!< The Uppsala General Catalogue of Galaxies
		CatCed		= 0x00004000, //!< Cederblad Catalog of bright diffuse Galactic nebulae (Ced)
		CatArp		= 0x00008000, //!< Atlas of Peculiar Galaxies (Arp)
		CatVV		= 0x00010000, //!< Interacting galaxies catalogue by Vorontsov-Velyaminov (VV)
		CatPK		= 0x00020000, //!< Catalogue of Galactic Planetary Nebulae (PK)
		CatPNG		= 0x00040000, //!< Strasbourg-ESO Catalogue of Galactic Planetary Nebulae (Acker+, 1992) (PN G)
		CatSNRG		= 0x00080000, //!< A catalogue of Galactic supernova remnants (Green, 2014) (SNR G)
		CatACO		= 0x00100000, //!< A Catalog of Rich Clusters of Galaxies (Abell+, 1989) (ACO)
		CatHCG		= 0x00200000, //!< Hickson, Compact Group (Hickson+ 1982) (HCG)		
		CatESO		= 0x00400000, //!< ESO/Uppsala Survey of the ESO(B) Atlas (Lauberts, 1982) (ESO)
		CatVdBH		= 0x00800000, //!< Catalogue of southern stars embedded in nebulosity (van den Bergh+, 1975) (vdBH)
		CatDWB		= 0x01000000, //!< Catalogue and distances of optically visible H II regions (Dickel+, 1969) (DWB)
		CatTr			= 0x02000000, //!< Trumpler Catalogue (Tr)
		CatSt		= 0x04000000, //!< Stock Catalogue (St)
		CatRu		= 0x08000000, //!< Ruprecht Catalogue (Ru)
		CatVdBHa		= 0x10000000, //!< van den Bergh-Hagen Catalogue (vdB-Ha)
		CatOther		= 0x20000000  //!< without ID
	};
	Q_DECLARE_FLAGS(CatalogGroup, CatalogGroupFlags)

	enum TypeGroupFlags
	{
		TypeGalaxies			= 0x00000001, //!< Galaxies
		TypeActiveGalaxies		= 0x00000002, //!< Different Active Galaxies
		TypeInteractingGalaxies	= 0x00000004, //!< Interacting Galaxies
		TypeOpenStarClusters	= 0x00000008, //!< Open Star Clusters
		TypeGlobularStarClusters	= 0x00000010, //!< Globular Star Clusters
		TypeHydrogenRegions	= 0x00000020, //!< Hydrogen Regions
		TypeBrightNebulae		= 0x00000040, //!< Bright Nebulae
		TypeDarkNebulae		= 0x00000080, //!< Dark Nebulae
		TypePlanetaryNebulae	= 0x00000100, //!< Planetary Nebulae
		TypeSupernovaRemnants	= 0x00000200, //!< Supernova Remnants
		TypeGalaxyClusters		= 0x00000400, //!< Galaxy Clusters
		TypeOther				= 0x00000800  //!< Other objects
	};
	Q_DECLARE_FLAGS(TypeGroup, TypeGroupFlags)

	//! A pre-defined set of specifiers for the catalogs filter
	static const CatalogGroupFlags AllCatalogs = static_cast<CatalogGroupFlags>(CatNGC|CatIC|CatM|CatC|CatB|CatSh2|CatLBN|CatLDN|CatRCW|CatVdB|CatCr|CatMel|CatPGC|CatUGC|CatCed|CatArp|CatVV|CatPK|CatPNG|CatSNRG|CatACO|CatHCG|CatESO|CatVdBH|CatDWB|CatTr|CatSt|CatRu|CatOther);
	static const TypeGroupFlags AllTypes = static_cast<TypeGroupFlags>(TypeGalaxies|TypeActiveGalaxies|TypeInteractingGalaxies|TypeOpenStarClusters|TypeGlobularStarClusters|TypeHydrogenRegions|TypeBrightNebulae|TypeDarkNebulae|TypePlanetaryNebulae|TypeSupernovaRemnants|TypeGalaxyClusters|TypeOther);

	//! @enum NebulaType Nebula types
	enum NebulaType
	{
		NebGx			= 0,		//!< m Galaxy
		NebAGx			= 1,  	//!< Active galaxy
		NebRGx			= 2,		//!< m Radio galaxy
		NebIGx			= 3,		//!< Interacting galaxy
		NebQSO			= 4,		//!< Quasar
		NebCl			= 5,		//!< Star cluster
		NebOc			= 6,		//!< Open star cluster
		NebGc			= 7,		//!< Globular star cluster, usually in the Milky Way Galaxy
		NebSA			= 8,		//!< Stellar association
		NebSC			= 9,		//!< Star cloud
		NebN			= 10,	//!< A nebula
		NebPn			= 11,	//!< Planetary nebula
		NebDn			= 12,	//!< Dark Nebula
		NebRn			= 13,	//!< Reflection nebula
		NebBn			= 14,	//!< Bipolar nebula
		NebEn			= 15,	//!< Emission nebula
		NebCn			= 16,	//!< Cluster associated with nebulosity
		NebHII			= 17,	//!< HII Region
		NebSNR			= 18,	//!< Supernova remnant
		NebISM			= 19,	//!< Interstellar matter
		NebEMO			= 20,	//!< Emission object
		NebBLL			= 21,	//!< BL Lac object
		NebBLA			= 22,	//!< Blazar
		NebMolCld	        = 23, 	//!< Molecular Cloud
		NebYSO			= 24, 	//!< Young Stellar Object
		NebPossQSO		= 25, 	//!< Possible Quasar
		NebPossPN		= 26, 	//!< Possible Planetary Nebula
		NebPPN			= 27, 	//!< Protoplanetary Nebula
		NebStar			= 28, 	//!< Star
		NebSymbioticStar	= 29, 	//!< Symbiotic Star
		NebEmissionLineStar	= 30, 	//!< Emission-line Star
		NebSNC			= 31, 	//!< Supernova Candidate
		NebSNRC			= 32, 	//!< Supernova Remnant Candidate
		NebGxCl			= 33,	//!< Cluster of Galaxies
		NebPartOfGx		= 34,	//!< Part of a Galaxy
		NebRegion		= 35,	//!< Region of the sky
		NebUnknown		= 36		//!< m Unknown type, catalog errors, "Unidentified Southern Objects" etc.
	};

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
	//! In addition to the entries from StelObject::getInfoMap(), Nebula objects provide
	//! - bmag (photometric B magnitude. 99 if unknown)
	//! - morpho (longish description; translated!)
	//! - surface-brightness
	//! - designations (all designations of DSO)
	//! A few entries are optional
	//! - bV (B-V index)
	//! - redshift
	virtual QVariantMap getInfoMap(const StelCore *core) const;
	virtual QString getType() const {return NEBULA_TYPE;}
	virtual QString getID() const {return getDSODesignation(); } //this depends on the currently shown catalog flags, should this be changed?
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const {return XYZ;}
	virtual double getCloseViewFov(const StelCore* core = Q_NULLPTR) const;
	virtual float getVMagnitude(const StelCore* core) const;
	virtual float getSelectPriority(const StelCore* core) const;
	virtual Vec3f getInfoColor() const;
	virtual QString getNameI18n() const {return nameI18;}
	virtual QString getEnglishName() const {return englishName;}
	QString getEnglishAliases() const;
	QString getI18nAliases() const;
	virtual double getAngularSize(const StelCore*) const;
	virtual SphericalRegionP getRegion() const {return pointRegion;}

	// Methods specific to Nebula
	void setLabelColor(const Vec3f& v) {labelColor = v;}
	// void setCircleColor(const Vec3f& v) {hintColorMap.insert(NebUnknown, v);}

	//! Get the printable nebula Type.
	//! @return the nebula type code.
	QString getTypeString() const {return getTypeString(nType);}
	static QString getTypeString(Nebula::NebulaType nType);

	NebulaType getDSOType() const {return nType;}

	//! Get the printable morphological nebula Type.
	//! @return the nebula morphological type string.
	QString getMorphologicalTypeString() const;

	float getSurfaceBrightness(const StelCore* core, bool arcsec=false) const;
	float getSurfaceBrightnessWithExtinction(const StelCore* core, bool arcsec=false) const;
	//! Compute an extended object's contrast index
	float getContrastIndex(const StelCore* core) const;

	//! Return object's B magnitude as seen from observer, without including extinction.
	virtual float getBMagnitude(const StelCore* core) const;

	//! Return object's B magnitude as seen from observer including extinction.
	//! Extinction obviously only if atmosphere=on.
	float getBMagnitudeWithExtinction(const StelCore* core) const;

	//! Get the surface area.
	//! @return surface area in square degrees.
	float getSurfaceArea(void) const;

	void setProperName(QString name) { englishName = name; }
	void addNameAlias(QString name) { englishAliases.append(name); }
	void removeAllNames() { englishName=""; englishAliases.clear(); }

	//! Get designation for DSO (with priority: M, C, NGC, IC, B, Sh2, vdB, RCW, LDN, LBN, Cr, Mel, PGC, UGC, Ced, Arp, VV, PK, PN G, SNR G, ACO, HCG, ESO, vdBH, DWB, Tr, St, Ru, vdB-Ha)
	//! @return a designation
	QString getDSODesignation() const;
	//! Get designation for DSO with priority and ignorance of availability of catalogs
	//! @return a designation
	QString getDSODesignationWIC() const;	

	bool objectInDisplayedCatalog() const;

	bool objectInAllowedSizeRangeLimits() const;

protected:
	//! Format the magnitude info string for the object
	virtual QString getMagnitudeInfoString(const StelCore *core, const InfoStringGroup& flags, const int decimals=1) const;

private:
	friend struct DrawNebulaFuncObject;

	//! Translate nebula name using the passed translator
	void translateName(const StelTranslator& trans)
	{
		nameI18 = trans.qtranslate(englishName);
		nameI18Aliases.clear();
		for (auto alias : englishAliases)
			nameI18Aliases.append(trans.qtranslate(alias));
	}	

	void readDSO(QDataStream& in);

	void drawLabel(StelPainter& sPainter, float maxMagLabel) const;
	void drawHints(StelPainter& sPainter, float maxMagHints) const;
	void drawOutlines(StelPainter& sPainter, float maxMagHints) const;

	bool objectInDisplayedType() const;

	static Vec3f getHintColor(Nebula::NebulaType nType);
	float getVisibilityLevelByMagnitude() const;

	//! Get the printable description of morphological nebula type.
	//! @return the nebula morphological type string.
	QString getMorphologicalTypeDescription() const;

	unsigned int DSO_nb;
	unsigned int M_nb;			// Messier Catalog number
	unsigned int NGC_nb;		// New General Catalog number
	unsigned int IC_nb;			// Index Catalog number
	unsigned int C_nb;			// Caldwell Catalog number
	unsigned int B_nb;			// Barnard Catalog number (Dark Nebulae)
	unsigned int Sh2_nb;		// Sharpless Catalog number (Catalogue of HII Regions (Sharpless, 1959))
	unsigned int VdB_nb;		// van den Bergh Catalog number (Catalogue of Reflection Nebulae (van den Bergh, 1966))
	unsigned int RCW_nb;		// RCW Catalog number (H-α emission regions in Southern Milky Way (Rodgers+, 1960))
	unsigned int LDN_nb;		// LDN Catalog number (Lynds' Catalogue of Dark Nebulae (Lynds, 1962))
	unsigned int LBN_nb;		// LBN Catalog number (Lynds' Catalogue of Bright Nebulae (Lynds, 1965))
	unsigned int Cr_nb;			// Collinder Catalog number
	unsigned int Mel_nb;		// Melotte Catalog number
	unsigned int PGC_nb;		// PGC number (Catalog of galaxies)
	unsigned int UGC_nb;		// UGC number (The Uppsala General Catalogue of Galaxies)
	unsigned int Arp_nb;		// Arp number (Atlas of Peculiar Galaxies (Arp, 1966))
	unsigned int VV_nb;			// VV number (The Catalogue of Interacting Galaxies (Vorontsov-Velyaminov+, 2001))	
	unsigned int DWB_nb;		// DWB number (Catalogue and distances of optically visible H II regions (Dickel+, 1969))
	unsigned int Tr_nb;			// Tr number (Trumpler Catalogue)
	unsigned int St_nb;			// St number (Stock Catalogue)
	unsigned int Ru_nb;			// Ru number (Ruprecht Catalogue)
	unsigned int VdBHa_nb;		// vdB-Ha number (van den Bergh-Hagen Catalogue)
	QString Ced_nb;			// Ced number (Cederblad Catalog of bright diffuse Galactic nebulae)	
	QString PK_nb;				// PK number (Catalogue of Galactic Planetary Nebulae)
	QString PNG_nb;			// PN G number (Strasbourg-ESO Catalogue of Galactic Planetary Nebulae (Acker+, 1992))
	QString SNRG_nb;			// SNR G number (A catalogue of Galactic supernova remnants (Green, 2014))
	QString ACO_nb;			// ACO number (Rich Clusters of Galaxies (Abell+, 1989))
	QString HCG_nb;			// HCG number (Hickson Compact Group (Hickson, 1989))
	QString ESO_nb;			// ESO number (ESO/Uppsala Survey of the ESO(B) Atlas (Lauberts, 1982))
	QString VdBH_nb;			// VdBH number (Southern Stars embedded in nebulosity (van den Bergh+, 1975))
	bool withoutID;
	QString englishName;		// English name
	QStringList englishAliases;	// English aliases
	QString nameI18;			// Nebula name
	QStringList nameI18Aliases;	// Nebula aliases
	QString mTypeString;		// Morphological type of object (as string)
	float bMag;				// B magnitude
	float vMag;				// V magnitude. For Dark Nebulae, opacity is stored here.
	float majorAxisSize;			// Major axis size in degrees
	float minorAxisSize;			// Minor axis size in degrees
	int orientationAngle;			// Orientation angle in degrees
	float oDistance;				// distance (kpc)
	float oDistanceErr;			// Error of distance (kpc)
	float redshift;
	float redshiftErr;
	float parallax;
	float parallaxErr;
	Vec3d XYZ;			// Cartesian equatorial position (J2000.0)
	Vec3d XY;				// Store temporary 2D position
	NebulaType nType;

	SphericalRegionP pointRegion;
	QStringList designations;

	static StelTextureSP texCircle;				// The symbolic circle texture
	static StelTextureSP texCircleLarge;			// The symbolic circle texture for large objects
	static StelTextureSP texRegion;				// The symbolic dashed shape texture
	static StelTextureSP texGalaxy;				// Type 0
	static StelTextureSP texGalaxyLarge;			// Type 0_large
	static StelTextureSP texOpenCluster;			// Type 1
	static StelTextureSP texOpenClusterLarge;		// Type 1_large
	static StelTextureSP texOpenClusterXLarge;	// Type 1_extralarge
	static StelTextureSP texGlobularCluster;		// Type 2
	static StelTextureSP texGlobularClusterLarge;	// Type 2_large
	static StelTextureSP texPlanetaryNebula;		// Type 3
	static StelTextureSP texDiffuseNebula;		// Type 4
	static StelTextureSP texDiffuseNebulaLarge;	// Type 4_large
	static StelTextureSP texDiffuseNebulaXLarge;	// Type 4_extralarge
	static StelTextureSP texDarkNebula;			// Type 5
	static StelTextureSP texDarkNebulaLarge;		// Type 5_large
	static StelTextureSP texOpenClusterWithNebulosity;	// Type 6
	static StelTextureSP texOpenClusterWithNebulosityLarge;	// Type 6_large
	static float hintsBrightness;

	static Vec3f labelColor;				// The color of labels
	static QMap<Nebula::NebulaType, Vec3f>hintColorMap;	// map for rapid lookup. Updated by NebulaMgr whenever a color changes.
	static QMap<Nebula::NebulaType, QString> typeStringMap; // map that keeps type strings for NebulaType. Must be retranslated on language change.
	static void buildTypeStringMap();			// (Re-)Fills typeStringMap. Called by NebulaMgr when required.

	static bool drawHintProportional;     // scale hint with nebula size?
	static bool surfaceBrightnessUsage;
	static bool designationUsage;

	static bool flagUseTypeFilters;
	static CatalogGroup catalogFilters;
	static TypeGroup typeFilters;

	static bool flagUseArcsecSurfaceBrightness;
	static bool flagUseShortNotationSurfaceBrightness;
	static bool flagUseOutlines;
	static bool flagShowAdditionalNames;

	static bool flagUseSizeLimits;
	static double minSizeLimit;
	static double maxSizeLimit;

	std::vector<std::vector<Vec3d> *> outlineSegments;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Nebula::CatalogGroup)
Q_DECLARE_OPERATORS_FOR_FLAGS(Nebula::TypeGroup)

#endif // NEBULA_HPP

