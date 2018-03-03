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

#ifndef _EXOPLANET_HPP_
#define _EXOPLANET_HPP_ 1

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelFader.hpp"

//! @ingroup exoplanets
typedef struct
{
	QString planetName;			//! Exoplanet designation
	QString planetProperName;	//! Exoplanet proper name
	float mass;				//! Exoplanet mass (Mjup)
	float radius;				//! Exoplanet radius (Rjup)
	float period;				//! Exoplanet period (days)
	float semiAxis;				//! Exoplanet orbit semi-major axis (AU)
	float eccentricity;			//! Exoplanet orbit eccentricity
	float inclination;				//! Exoplanet orbit inclination
	float angleDistance;			//! Exoplanet angle distance
	int discovered;				//! Exoplanet discovered year
	QString pclass;				//! Exoplanet classification from host star spectral type (F, G, K, M), habitable zone (hot, warm, cold) and size (miniterran, subterran, terran, superterran, jovian, neptunian)
	int EqTemp;				//! Exoplanet equilibrium temperature in kelvins (K) assuming a 0.3 bond albedo (Earth = 255 K).
	int flux;					//! Average stellar flux of the planet in Earth fluxes (Earth = 1.0 SE).
	int ESI;					//! Exoplanet Earth Similarity Index
	QString detectionMethod;		//! Method of detection of exoplanet
	bool conservative;			//! Conservative sample
} exoplanetData;

class StelPainter;

//! @class Exoplanet
//! A exoplanet object represents one planetary system on the sky.
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
	~Exoplanet();

	//! Get a QVariantMap which describes the exoplanet. Could be used to
	//! create a duplicate.
	QVariantMap getMap(void) const;

	//! Get the type of object
	virtual QString getType(void) const
	{
		return EXOPLANET_TYPE;
	}

	virtual QString getID(void) const
	{
		return getDesignation();
	}

	virtual float getSelectPriority(const StelCore* core) const;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	//! Return a map like StelObject, but with a few extra tags also available in getMap().
	//! - distance = distance in pc
	//! - stype = Spectral type of star
	//! - smass = Mass of star in Msun
	//! - smetal = [Fe/H] of star
	//! - sradius = Radius of star in Rsun
	//! - effectiveTemp = Effective temperature of star in K
	//! - hasHabitablePlanets (true/false)
	virtual QVariantMap getInfoMap(const StelCore *core) const;
	virtual Vec3f getInfoColor(void) const;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const
	{
		return XYZ;
	}
	//! Get the visual magnitude
	virtual float getVMagnitude(const StelCore* core) const;
	//! Get the angular size of host star
	virtual double getAngularSize(const StelCore* core) const;
	//! Get the localized name of host star
	virtual QString getNameI18n(void) const;
	//! Get the english name
	virtual QString getEnglishName(void) const;

	QString getDesignation(void) const;
	QStringList getExoplanetsEnglishNames(void) const;
	QStringList getExoplanetsNamesI18n(void) const;
	QStringList getExoplanetsDesignations(void) const;

	bool isDiscovered(const StelCore* core);

	void update(double deltaTime);

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
		switch(mode)
		{
			case 1:
				return semiAxisList;
				break;
			case 2:
				return massList;
				break;
			case 3:
				return radiusList;
				break;
			case 4:
				return periodList;
				break;
			case 5:
				return angleDistanceList;
				break;
			case 6:
				return effectiveTempHostStarList;
				break;
			case 7:
				return yearDiscoveryList;
				break;
			case 8:
				return metallicityHostStarList;
				break;
			case 9:
				return vMagHostStarList;
				break;
			case 10:
				return raHostStarList;
				break;
			case 11:
				return decHostStarList;
				break;
			case 12:
				return distanceHostStarList;
				break;
			case 13:
				return massHostStarList;
				break;
			case 14:
				return radiusHostStarList;
				break;
			default:
				return eccentricityList;
		}
	}

private:

	QString getPlanetaryClassI18n(QString ptype) const;

	bool initialized;

	Vec3d XYZ;                         // holds J2000 position	

	static StelTextureSP markerTexture;
	static Vec3f habitableExoplanetMarkerColor;
	static Vec3f exoplanetMarkerColor;
	static bool distributionMode;
	static bool timelineMode;
	static bool habitableMode;
	static bool showDesignations;	
	static int temperatureScaleID;

	void draw(StelCore* core, StelPainter *painter);

	QString getTemperatureScaleUnit() const;
	float getTemperature(float temperature) const;

	int EPCount;
	int PHEPCount;

	//! Variables for description of properties of exoplanets
	QString designation;			//! The designation of the host star
	QString starProperName;			//! The proper name of the host star
	float RA;				//! J2000 right ascension of host star
	float DE;				//! J2000 declination of host star
	float distance;				//! Distance to star in pc
	QString stype;				//! Spectral type of star
	float smass;				//! Mass of star in Msun
	float smetal;				//! [Fe/H] of star
	float Vmag;				//! Visual magnitude of star
	float sradius;				//! Radius of star in Rsun
	int effectiveTemp;			//! Effective temperature of star in K
	bool hasHabitableExoplanets;		//! Has potential habitable exoplanets
	QList<exoplanetData> exoplanets;	//! List of exoplanets

	QStringList englishNames, translatedNames, exoplanetDesignations;

	QList<double> eccentricityList, semiAxisList, massList, radiusList, periodList, angleDistanceList,
		      effectiveTempHostStarList, yearDiscoveryList, metallicityHostStarList, vMagHostStarList,
		      raHostStarList, decHostStarList, distanceHostStarList, massHostStarList, radiusHostStarList;

	LinearFader labelsFader;
};

#endif // _EXOPLANET_HPP_
