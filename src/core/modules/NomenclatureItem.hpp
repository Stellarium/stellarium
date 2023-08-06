/*
 * Copyright (C) 2017 Alexander Wolf
 * Copyright (C) 2017 Teresa Huertas Roldán
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

#ifndef NOMENCLATUREITEM_HPP
#define NOMENCLATUREITEM_HPP

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>

#include "StelObject.hpp"
#include "StelFader.hpp"
#include "StelTranslator.hpp"
#include "SolarSystem.hpp"

class StelPainter;

//! Class which contains data about one Nomenclature entry from the IAU database at https://planetarynames.wr.usgs.gov/
//! There is a confusing variety of planetographic vs. planetocentric coordinate systems,
//! counting longitudes in eastern or western direction, or counting from 0...360 degrees or from -180...+180 degrees.
//! The actual data were taken from https://planetarynames.wr.usgs.gov/GIS_Downloads
//! and include eastward-positive planetocentric coordinates, i.a. all bodies are treated as spheres. However, according to
//! https://planetarynames.wr.usgs.gov/TargetCoordinates on some objects at least the longitudes should be counted west-positive,
//! a convention which we should follow closely. Note that for Mars the traditional west-positive counting has recently been inverted,
//! and longitudes are now counted east-positive like on earth.
class NomenclatureItem : public StelObject
{
	friend class NomenclatureMgr;
public:
	static const QString NOMENCLATURE_TYPE;

	// Details: https://planetarynames.wr.usgs.gov/DescriptorTerms
	enum NomenclatureItemType
	{
		niUNDEFINED		=  0, // Undefined type of feature. THIS IS ONLY IN CASE OF ERROR!
		niAlbedoFeature		=  1, // type="albedo feature"
		niArcus			=  2, // type="arcus"
		niAstrum		=  3, // type="astrum"
		niCatena		=  4, // type="catena"
		niCavus			=  5, // type="cavus"
		niChaos			=  6, // type="chaos"
		niChasma		=  7, // type="chasma"
		niCollis		=  8, // type="collis"
		niCorona		=  9, // type="corona"
		niCrater		= 10, // type="crater"
		niDorsum		= 11, // type="dorsum"
		niEruptiveCenter	= 12, // type="eruptive center"
		niFacula		= 13, // type="facula"
		niFarrum		= 14, // type="farrum"
		niFlexus		= 15, // type="flexus"
		niFluctus		= 16, // type="fluctus"
		niFlumen		= 17, // type="flumen"
		niFretum		= 18, // type="fretum"
		niFossa			= 19, // type="fossa"
		niInsula		= 20, // type="insula"
		niLabes			= 21, // type="labes"
		niLabyrinthus		= 22, // type="labyrinthus"
		niLacuna		= 23, // type="lacuna"
		niLacus			= 24, // type="lacus"
		niLargeRingedFeature	= 25, // type="large ringed feature"
		niLinea			= 26, // type="linea"
		niLingula		= 27, // type="lingula"
		niMacula		= 28, // type="macula"
		niMare			= 29, // type="mare"
		niMensa			= 30, // type="mensa"
		niMons			= 31, // type="mons"
		niOceanus		= 32, // type="oceanus"
		niPalus			= 33, // type="palus"
		niPatera		= 34, // type="patera"
		niPlanitia		= 35, // type="planitia"
		niPlanum		= 36, // type="planum"
		niPlume			= 37, // type="plume"
		niPromontorium		= 38, // type="promontorium"
		niRegio			= 39, // type="regio"
		niRima			= 40, // type="rima"
		niRupes			= 41, // type="rupes"
		niScopulus		= 42, // type="scopulus"
		niSerpens		= 43, // type="serpens"
		niSulcus		= 44, // type="sulcus"
		niSinus			= 45, // type="sinus"
		niTerra			= 46, // type="terra"
		niTholus		= 47, // type="tholus"
		niUnda			= 48, // type="unda"
		niVallis		= 49, // type="vallis"
		niVastitas		= 50, // type="vastitas"
		niVirga			= 51, // type="virga"
		niLandingSite		= 52, // type="landing site"
		niLenticula		= 53, // type="lenticula"
		niReticulum		= 54, // type="reticulum"
		niSatelliteFeature	= 55, // type="satellite feature"
		niTessera		= 56, // type="tessera"
		niSaxum			= 57, // type="saxum"
		niSpecialPointPole	= 100,// North and South Poles; special type
		niSpecialPointEast	= 101,// East point; special type
		niSpecialPointWest	= 102,// West point; special type
		niSpecialPointCenter    = 103,// Central point; special type
		niSpecialPointSubSolar	= 104 // Subsolar point; special type
	};
	Q_ENUM(NomenclatureItemType)

	// Describes the orientation of the given feature coordinates
	enum PlanetCoordinateOrientation
	{
		pcoPlanetographicWest360 = 0x000,
		pcoPlanetographicWest180 = 0x001,
		pcoPlanetographicEast360 = 0x010,
		pcoPlanetographicEast180 = 0x011,
		pcoPlanetocentricWest360 = 0x100,
		pcoPlanetocentricWest180 = 0x101,
		pcoPlanetocentricEast360 = 0x110,
		pcoPlanetocentricEast180 = 0x111
	};
	Q_ENUM(PlanetCoordinateOrientation)

	NomenclatureItem(PlanetP nPlanet, int nId, const QString& nName, const QString& nContext, NomenclatureItemType nItemType, double nLatitude, double nLongitude, double nSize);
	virtual ~NomenclatureItem() Q_DECL_OVERRIDE;

	//! Get the type of object
	virtual QString getType(void) const Q_DECL_OVERRIDE
	{
		return NOMENCLATURE_TYPE;
	}
	virtual QString getObjectType(void) const Q_DECL_OVERRIDE
	{
		return getNomenclatureTypeLatinString(nType);
	}
	virtual QString getObjectTypeI18n(void) const Q_DECL_OVERRIDE
	{
		return getNomenclatureTypeString(nType);
	}

	virtual QString getID(void) const Q_DECL_OVERRIDE
	{
		return QString("%1").arg(identificator);
	}

	virtual float getSelectPriority(const StelCore* core) const Q_DECL_OVERRIDE;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const Q_DECL_OVERRIDE;
	virtual Vec3f getInfoColor(void) const Q_DECL_OVERRIDE;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const Q_DECL_OVERRIDE;
	//! Return the angular radius of a circle containing the feature as seen from the observer
	//! with the circle center assumed to be at getJ2000EquatorialPos().
	//! @return radius in degree. This value is half of the apparent angular size of the object, and is independent of the current FOV.
	virtual double getAngularRadius(const StelCore* core) const Q_DECL_OVERRIDE;
	//! Get the localized name of nomenclature item.
	virtual QString getNameI18n(void) const Q_DECL_OVERRIDE;
	//! Get the english name of nomenclature item.
	virtual QString getEnglishName(void) const Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	//! Translate feature name using the passed translator
	virtual void translateName(const StelTranslator &trans);

	//! Compute times of nearest rise, transit and set of the item's current Planet.
	//! @param core the currently active StelCore object
	//! @param altitude (optional; default=0) altitude of the object, degrees.
	//! @return Vec4d - time of rise, transit and set closest to current time; JD.
	//! @note The fourth element flags particular conditions:
	//!       *  +100. for circumpolar objects. Rise and set give lower culmination times.
	//!       *  -100. for objects never rising. Rise and set give transit times.
	//!       * -1000. is used as "invalid" value. The result should then not be used.
	virtual Vec4d getRTSTime(const StelCore* core, const double altitude=0.) const Q_DECL_OVERRIDE;

	void draw(StelCore* core, StelPainter *painter);
	NomenclatureItemType getNomenclatureType() const { return nType;}

	static void setFlagLabels(bool b){ labelsFader = b; }
	static bool getFlagLabels(void){ return labelsFader;}
	void setFlagHideLocalNomenclature(bool b) { hideLocalNomenclature=b; }
	bool getFlagHideLocalNomenclature() const { return hideLocalNomenclature; }
	void setFlagShowSpecialNomenclatureOnly(bool b) { showSpecialNomenclatureOnly=b; }
	bool getFlagShowSpecialNomenclatureOnly() const { return showSpecialNomenclatureOnly; }
	//QString getEnglishPlanetName(void) const {return planet->getEnglishName();}
	PlanetP getPlanet(void) const { return planet;}
	//! get latitude [degrees]
	float getLatitude(void) const {return static_cast<float>(latitude);}
	//! get longitude [degrees]
	float getLongitude(void) const {return static_cast<float>(longitude);}
	//! return solar altitude in degrees (Meeus, Astr.Alg.1998 53.3)
	double getSolarAltitude(const StelCore *core) const;

protected:
	//! returns a type enum for a given 2-letter code
	static NomenclatureItemType getNomenclatureItemType(const QString abbrev);
	//! Should be triggered once at start and then whenever program language setting changes.
	static void createNameLists();

private:
	mutable Vec3d XYZpc;   // holds planetocentric direction vector (unit length, from longitude/latitude). One-time conversion for fixed features, but mutable for special points.
	mutable Vec3d XYZ;     // holds J2000 position in space (i.e. including planet position, offset from planetocenter)
	mutable double jde;    // jde time of XYZ value
	static Vec3f color;
	static bool flagOutlineCraters; // draw craters and satellite features as ellipses?
	static bool hideLocalNomenclature;
	static bool showTerminatorZoneOnly;
	static int terminatorMinAltitude;
	static int terminatorMaxAltitude;
	static bool showSpecialNomenclatureOnly;

	// ratio of angular size of feature to the FOV
	float getAngularDiameterRatio(const StelCore *core) const;

	static QString getNomenclatureTypeLatinString(NomenclatureItemType nType);
	static QString getNomenclatureTypeString(NomenclatureItemType nType);
	static QString getNomenclatureTypeDescription(NomenclatureItemType nType, QString englishName);
	//! Returns the description of the feature coordinates where available, or pcoPlanetographicWest360.
	//! The default value ensures valid central meridian data for the gas giants.
	static PlanetCoordinateOrientation getPlanetCoordinateOrientation(QString planetName);
	PlanetCoordinateOrientation getPlanetCoordinateOrientation() const;
	//! return whether counting sense of the coordinates is positive towards the east
	static bool isEastPositive(PlanetCoordinateOrientation pco);
	//! return whether coordinates are planetocentric (and not planetographic)
	//! Given that the source data are planetocentric, this returns always true.
	static bool isPlanetocentric(PlanetCoordinateOrientation pco);
	//! return whether longitudes shall be counted from -180 to +180 degrees (and not 0...360 degrees)
	static bool is180(PlanetCoordinateOrientation pco);
	//! return whether counting sense of the coordinates is positive towards the east
	bool isEastPositive() const;
	//! return whether coordinates are planetocentric (and not planetographic).
	//! Given that the source data are planetocentric, this returns always true.
	bool isPlanetocentric() const;
	//! return whether longitudes shall be counted from -180 to +180 degrees (and not 0...360 degrees)
	bool is180() const;
	static QMap<NomenclatureItemType, QString>niTypeStringMap;
	static QMap<NomenclatureItemType, QString>niTypeDescriptionMap;

	PlanetP planet;
	int identificator;
	QString englishName, context, nameI18n;
	NomenclatureItemType nType; // Type of nomenclature item
	mutable double latitude;    // degrees
	mutable double longitude;   // degrees. Declared mutable to allow change in otherwise const methods (for special points)
	double size;                //  km

	static LinearFader labelsFader;
};

#endif // NOMENCLATUREITEM_HPP
