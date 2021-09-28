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
#include "StelTextureTypes.hpp"
#include "StelFader.hpp"
#include "StelTranslator.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"

class StelPainter;

class NomenclatureItem : public StelObject
{
	friend class NomenclatureMgr;
	Q_ENUMS(NomenclatureItemType)
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
		niSaxum			= 57  // type="saxum"
	};

	NomenclatureItem(PlanetP nPlanet, int nId, const QString& nName, const QString& nContext, NomenclatureItemType nItemType, float nLatitude, float nLongitude, float nSize);
	virtual ~NomenclatureItem();

	//! Get the type of object
	virtual QString getType(void) const
	{
		return NOMENCLATURE_TYPE;
	}

	virtual QString getID(void) const
	{
		return QString("%1").arg(identificator);
	}

	virtual float getSelectPriority(const StelCore* core) const;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	virtual Vec3f getInfoColor(void) const;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const;
	//! Get the visual magnitude of a nomenclature item. Dummy method, returns 99.
	virtual float getVMagnitude(const StelCore* core) const;
	//! Get the angular size of nomenclature item.
	virtual double getAngularSize(const StelCore* core) const;
	//! Get the localized name of nomenclature item.
	virtual QString getNameI18n(void) const;
	//! Get the english name of nomenclature item.
	virtual QString getEnglishName(void) const;

	///////////////////////////////////////////////////////////////////////////
	//! Translate planet name using the passed translator
	virtual void translateName(const StelTranslator &trans);

	void draw(StelCore* core, StelPainter *painter);
	NomenclatureItemType getNomenclatureType() const { return nType;}
	void update(double deltaTime);

	void setFlagLabels(bool b){ labelsFader = b; }
	bool getFlagLabels(void) const { return labelsFader==true;}
	void setFlagHideLocalNomenclature(bool b) { hideLocalNomenclature=b; }
	bool getFlagHideLocalNomenclature() const { return hideLocalNomenclature; }
	//QString getEnglishPlanetName(void) const {return planet->getEnglishName();}
	PlanetP getPlanet(void) const { return planet;}
	//! get latitude [degrees]
	float getLatitude(void) const {return latitude;}
	//! get longitude [degrees]
	float getLongitude(void) const {return longitude;}
	//! return solar altitude in degrees (Meeus, Astr.Alg.1998 53.3)
	double getSolarAltitude(const StelCore *core) const;

protected:
	//! returns a type enum for a given 2-letter code
	static NomenclatureItemType getNomenclatureItemType(const QString abbrev);
	//! Should be triggered once at start and then whenever program language setting changes.
	static void createNameLists();

private:
	Vec3d XYZpc;                         // holds planetocentric position (from longitude/latitude)
	mutable Vec3d XYZ;                   // holds J2000 position
	mutable double jde;                  // jde time of XYZ value
	static Vec3f color;
	static bool hideLocalNomenclature;

	static QString getNomenclatureTypeLatinString(NomenclatureItemType nType);
	static QString getNomenclatureTypeString(NomenclatureItemType nType);
	static QString getNomenclatureTypeDescription(NomenclatureItemType nType, QString englishName);
	static QMap<NomenclatureItemType, QString>niTypeStringMap;
	static QMap<NomenclatureItemType, QString>niTypeDescriptionMap;

	PlanetP planet;
	int identificator;
	QString englishName, context, nameI18n;
	NomenclatureItemType nType;       // Type of nomenclature item
	float latitude, longitude, size;  // degrees, degrees, km

	LinearFader labelsFader;
};

#endif // NOMENCLATUREITEM_HPP
