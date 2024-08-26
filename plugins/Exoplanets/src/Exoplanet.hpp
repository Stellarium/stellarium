/*
 * Copyright (C) 2012 Alexander Wolf
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

#ifndef EXOPLANET_HPP
#define EXOPLANET_HPP

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelTranslator.hpp"

//! @ingroup exoplanets
typedef struct
{
	QString planetName;		//! Exoplanet designation
	QString planetProperName;	//! Exoplanet proper name
	double mass;				//! Exoplanet mass (Mjup)
	double radius;				//! Exoplanet radius (Rjup)
	double period;				//! Exoplanet period (days)
	double semiAxis;			//! Exoplanet orbit semi-major axis (AU)
	double eccentricity;			//! Exoplanet orbit eccentricity
	double inclination;			//! Exoplanet orbit inclination
	double angleDistance;			//! Exoplanet angle distance
	int discovered;				//! Exoplanet discovered year
	QString pclass;				//! Exoplanet classification from host star spectral type (F, G, K, M), habitable zone (hot, warm, cold) and size (miniterran, subterran, terran, superterran, jovian, neptunian)
	int SurfTemp;				//! Exoplanet surface temperature in kelvins (K) assuming a 0.3 bond albedo (Earth = 255 K).
	int flux;				//! Average stellar flux of the planet in Earth fluxes (Earth = 1.0 SE).
	int ESI;				//! Exoplanet Earth Similarity Index
	QString detectionMethod;		//! Method of detection of exoplanet
	bool conservative;			//! Conservative sample
} exoplanetData;

class StelPainter;

//! @class Exoplanet
//! An exoplanet object represents one extrasolar planetary system in the sky.
//! Details about the exoplanets are passed using a QVariant which contains
//! a map of data from the json file.
//! @ingroup exoplanets

class Exoplanet : public StelObject
{
	friend class Exoplanets;

public:
	static const QString EXOPLANET_TYPE;

	//! @param id The official designation for a exoplanet, e.g. "Kepler-10 b"
	Exoplanet(const QVariantMap& map);
	~Exoplanet() override;

	//! Get a QVariantMap which describes the exoplanet. Could be used to
	//! create a duplicate.
	QVariantMap getMap(void) const;

	//! Get the type of object
	QString getType(void) const override
	{
		return EXOPLANET_TYPE;
	}

	//! Get the type of object
	QString getObjectType(void) const override
	{
		return N_("planetary system");
	}
	QString getObjectTypeI18n(void) const override
	{
		return q_(getObjectType());
	}

	QString getID(void) const override
	{
		return getDesignation();
	}

	float getSelectPriority(const StelCore* core) const override;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const override;
	//! Return a map like StelObject, but with a few extra tags also available in getMap().
	//! - distance = distance in pc
	//! - stype = Spectral type of star
	//! - smass = Mass of star in Msun
	//! - smetal = [Fe/H] of star
	//! - sradius = Radius of star in Rsun
	//! - effectiveTemp = Effective temperature of star in K
	//! - hasHabitablePlanets (true/false)
	QVariantMap getInfoMap(const StelCore *core) const override;
	Vec3f getInfoColor(void) const override;
	Vec3d getJ2000EquatorialPos(const StelCore* core) const override;
	//! Get the visual magnitude
	float getVMagnitude(const StelCore* core) const override;
	//! Get the localized name of host star
	QString getNameI18n(void) const override;
	//! Get the english name
	QString getEnglishName(void) const override;

	bool isVMagnitudeDefined() const;

	QString getDesignation(void) const;
	QStringList getDesignations(void) const;
	QStringList getExoplanetsEnglishNames(void) const;
	QStringList getExoplanetsNamesI18n(void) const;
	QStringList getExoplanetsDesignations(void) const;

	//! @returns whether system has been discovered by the current year.
	bool isDiscovered(const StelCore* core);

	int getCountExoplanets(void) const
	{
		return EPCount;
	}
	int getCountHabitableExoplanets(void) const
	{
		return PHEPCount;
	}

	QList<double> getData(int mode)
	{
		return QMap<int, QList<double>>{
			{1, semiAxisList},
			{2, massList},
			{3, radiusList},
			{4, periodList},
			{5, angleDistanceList},
			{6, effectiveTempHostStarList},
			{7, yearDiscoveryList},
			{8, metallicityHostStarList},
			{9, vMagHostStarList},
			{10, raHostStarList},
			{11, decHostStarList},
			{12, distanceHostStarList},
			{13, massHostStarList},
			{14, radiusHostStarList}}.value(mode,eccentricityList);
	}

private:
	QString getPlanetaryClassI18n(const QString &ptype) const;

	bool initialized;

	Vec3d XYZ;                         // holds J2000 position	

	static StelTextureSP markerTexture;
	static Vec3f habitableExoplanetMarkerColor;
	static Vec3f exoplanetMarkerColor;
	static bool distributionMode;
	static bool timelineMode;
	static bool habitableMode;
	static bool showDesignations;
	static bool showNumbers;
	static int temperatureScaleID; //!< Magic number. 0: Kelvin; 1: Celsius; 2: Fahrenheit

	static bool syncShowLabels;

	void draw(StelCore* core, StelPainter *painter);

	QString getTemperatureScaleUnit() const;
	//! convert input temperature to Kelvin temperature, depending on the setting of temperatureScaleID
	float getTemperature(float temperature) const;

	int EPCount;
	int PHEPCount;

	//! Variables for description of properties of exoplanets
	QString designation;			//! The designation of the host star
	QString starProperName;			//! The proper name of the host star
	QString starAltNames;			//! The alternative names of the host star
	double RA;				//! J2000 right ascension of host star // ALMOST USELESS AFTER CONSTRUCTOR!
	double DE;				//! J2000 declination of host star     // ALMOST USELESS AFTER CONSTRUCTOR!   use XYZ
	double distance;			//! Distance to star in pc
	QString stype;				//! Spectral type of star
	double smass;				//! Mass of star in Msun
	double smetal;				//! [Fe/H] of star
	double Vmag;				//! Visual magnitude of star
	double sradius;				//! Radius of star in Rsun
	int effectiveTemp;			//! Effective temperature of star in K
	bool hasHabitableExoplanets;		//! Has potential habitable exoplanets
	QList<exoplanetData> exoplanets;	//! List of exoplanets

	QStringList englishNames, translatedNames, exoplanetDesignations;

	// Lists with various data for fast creating a diagrams of relations
	QList<double> eccentricityList, semiAxisList, massList, radiusList, periodList, angleDistanceList,
		      effectiveTempHostStarList, yearDiscoveryList, metallicityHostStarList, vMagHostStarList,
		      raHostStarList, decHostStarList, distanceHostStarList, massHostStarList, radiusHostStarList;
};

#endif // EXOPLANET_HPP
