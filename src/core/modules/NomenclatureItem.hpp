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

#ifndef _NOMENCLATUREITEM_HPP_
#define _NOMENCLATUREITEM_HPP_ 1

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
public:
	static const QString NOMENCLATURE_TYPE;

	Q_ENUMS(NomenclatureItemType)
	enum NomenclatureItemType
	{
		niAlbedoFeature,	// type="albedo feature"
		niArcus,		// type="arcus"
		niCatena,		// type="catena"
		niCavus,		// type="cavus"
		niChaos,		// type="chaos"
		niChasma,		// type="chasma"
		niCollis,		// type="collis"
		niCorona,		// type="corona"
		niCrater,		// type="crater"
		niDorsum,		// type="dorsum"
		niEruptiveCenter,	// type="eruptive center"
		niFlexus,		// type="flexus"
		niFluctus,		// type="fluctus"
		niFlumen,		// type="flumen"
		niFretum,		// type="fretum"
		niFossa,		// type="fossa"
		niInsula,		// type="insula"
		niLabes,		// type="labes"
		niLabyrinthus,		// type="labyrinthus"
		niLacuna,		// type="lacuna"
		niLacus,		// type="lacus"
		niLargeRingedFeature,	// type="large ringed feature"
		niLinea,		// type="linea"
		niLingula,		// type="lingula"
		niMacula,		// type="macula"
		niMare,			// type="mare"
		niMensa,		// type="mensa"
		niMons,			// type="mons"
		niOceanus,		// type="oceanus"
		niPalus,		// type="palus"
		niPatera,		// type="patera"
		niPlanitia,		// type="planitia"
		niPlanum,		// type="planum"
		niPlume,		// type="plume"
		niPromontorium,		// type="promontorium"
		niRegio,		// type="regio"
		niRima,			// type="rima"
		niRupes,		// type="rupes"
		niScopulus,		// type="scopulus"
		niSerpens,		// type="serpens"
		niSulcus,		// type="sulcus"
		niSinus,		// type="sinus"
		niTerra,		// type="terra"
		niTholus,		// type="tholus"
		niUnda,			// type="unda"
		niVallis,		// type="vallis"
		niVastitas,		// type="vastitas"
		niVirga,		// type="virga"
		niLandingSite,		// type="landing site"
		niUNDEFINED		// type=<anything else>. THIS IS ONLY IN CASE OF ERROR!
	};

	NomenclatureItem(PlanetP nPlanet, const QString& nId, const QString& nName, const QString& nItemType, float nLatitude, float nLongitude, float nSize);
	virtual ~NomenclatureItem();

	static void init();

	//! Get the type of object
	virtual QString getType(void) const
	{
		return NOMENCLATURE_TYPE;
	}

	virtual QString getID(void) const
	{
		return identificator;
	}

	virtual float getSelectPriority(const StelCore* core) const;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	virtual Vec3f getInfoColor(void) const;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const
	{
		return XYZ;
	}
	//! Get the visual magnitude of pulsar
	virtual float getVMagnitude(const StelCore* core) const;
	//! Get the angular size of pulsar
	virtual double getAngularSize(const StelCore* core) const;
	//! Get the localized name of pulsar
	virtual QString getNameI18n(void) const;
	//! Get the english name of pulsar
	virtual QString getEnglishName(void) const;

	///////////////////////////////////////////////////////////////////////////
	//! Translate planet name using the passed translator
	virtual void translateName(const StelTranslator &trans);

	void draw(StelCore* core, StelPainter *painter);

	const QString getNomenclatureTypeString() const {return niTypeMap.value(niType);}
	NomenclatureItemType getNomenclatureType() const {return niType;}

	void update(double deltaTime);

	void setFlagLabels(bool b){ labelsFader = b; }
	bool getFlagLabels(void) const { return labelsFader==true;}

private:
	bool initialized;
	Vec3d XYZ;                         // holds J2000 position
	static Vec3f color;

	PlanetP planet;
	QString identificator, englishName, nameI18n, nType;
	float latitude, longitude, size;

	LinearFader labelsFader;	

	NomenclatureItemType niType;       // Type of nomenclature item
	static QMap<NomenclatureItemType, QString> niTypeMap; // Maps fast type to english name.
};

#endif // _NOMENCLATUREITEM_HPP_
