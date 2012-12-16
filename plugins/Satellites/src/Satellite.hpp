/*
 * Stellarium
 * Copyright (C) 2009, 2012 Matthew Gates
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

#ifndef _SATELLITE_HPP_
#define _SATELLITE_HPP_ 1

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelSphereGeometry.hpp"
#include "StelProjectorType.hpp"

#include "gSatWrapper.hpp"


class StelLocation;

typedef struct
{
	double frequency;
	QString modulation;
	QString description;
} commLink;

//! @class Satellite
//! A Satellite object represents one satellite in Earth orbit.
//! Details about the satellite are passed using a QVariant which contains
//! a map of data from the json file.
class Satellite : public StelObject
{
	friend class Satellites;
	friend class SatellitesDialog;
public:
	//! \param identifier unique identifier (currently the Catalog Number)
	//! \param data a QMap which contains the details of the satellite
	//! (TLE set, description etc.)
	Satellite(const QString& identifier, const QVariantMap& data);
	~Satellite();

	//! Get a QVariantMap which describes the satellite.  Could be used to
	//! create a duplicate.
	QVariantMap getMap(void);

	virtual QString getType(void) const
	{
		return "Satellite";
	}
	virtual float getSelectPriority(const StelCore* core) const;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	//! Supported types for Satellite objects:
	//! - Name: designation in large type with the description underneath
	//! - RaDecJ2000, RaDecOfDate, HourAngle, AltAzi
	//! - Extra1: range, rage rate and altitude of satellite above the Earth
	//! - Extra2: Comms frequencies, modulation types and so on.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	virtual Vec3f getInfoColor(void) const;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const;
	virtual float getVMagnitude(const StelCore* core=NULL, bool withExtinction=false) const;
	virtual double getAngularSize(const StelCore* core) const;
	virtual QString getNameI18n(void) const
	{
		return name;
	}
	virtual QString getEnglishName(void) const
	{
		return name;
	}
	//! Returns the (NORAD) catalog number. (For now, the ID string.)
	QString getCatalogNumberString() const {return id;}

	//! Set new tleElements.  This assumes the designation is already set, populates
	//! the tleElements values and configures internal orbit parameters.
	void setNewTleElements(const QString& tle1, const QString& tle2);

	// calculate faders, new position
	void update(double deltaTime);

	double getDoppler(double freq) const;
	static float showLabels;
	static double roundToDp(float n, int dp);

	// when the observer location changes we need to
	void recalculateOrbitLines(void);
	
	void setNew() {newlyAdded = true;}
	bool isNew() const {return newlyAdded;}
	
	static QString extractInternationalDesignator(const QString& tle1);
	static int extractLaunchYear(const QString& tle1);

public:
	void enableDrawOrbit(bool b);

private:
	//draw orbits methods
	void computeOrbitPoints();
	void drawOrbit(class StelRenderer* renderer, StelProjectorP projector);
	//! returns 0 - 1.0 for the DRAWORBIT_FADE_NUMBER segments at
	//! each end of an orbit, with 1 in the middle.
	float calculateOrbitSegmentIntensity(int segNum);
	void setNightColors(bool night);

private:
	bool initialized;
	bool visible;
	bool orbitVisible;  // draw orbit enabled/disabled
	bool newlyAdded;
	bool orbitValid;

	//! Identifier of the satellite, must be unique within the list.
	//! Currently, the Satellite Catalog Number is used. It is contained in both
	//! numbered lines of TLE sets.
	QString id;
	//! Human-readable name of the satellite.
	//! Usually the string in the "Title line" of TLE sets.
	QString name;
	//! Longer description of the satellite.
	QString description;
	//! International Designator / COSPAR designation / NSSDC ID
	QString internationalDesignator;
	//! JD for Jan 1st of launch year, extracted from TLE (will be for 1957-1-1 if extraction fails). Used to hide objects before launch year.
	double jdLaunchYearJan1;
	//! Contains the J2000 position 
	Vec3d XYZ;
	QPair< QByteArray, QByteArray > tleElements;
	double height, range, rangeRate;
	QList<commLink> comms;
	Vec3f hintColor;
	QStringList groupIDs;
	QDateTime lastUpdated;

	static SphericalCap  viewportHalfspace;
	static float hintBrightness;
	static float hintScale;
	static int   orbitLineSegments;
	static int   orbitLineFadeSegments;
	static int   orbitLineSegmentDuration; //measured in seconds
	static bool  orbitLinesFlag;

	void draw(const StelCore* core, class StelRenderer* renderer, 
	          StelProjectorP projector, class StelTextureNew* hintTexture);

	//Satellite Orbit Position calculation
	gSatWrapper *pSatWrapper;
	Vec3d position;
	Vec3d velocity;
	Vec3d latLongSubPointPosition;
	Vec3d elAzPosition;
	int   visibility;

	//Satellite Orbit Draw
	QFont     font;
	Vec3f     orbitColorNormal;
	Vec3f     orbitColorNight;
	Vec3f*    orbitColor;
	double    lastEpochCompForOrbit; //measured in Julian Days
	double    epochTime;  //measured in Julian Days
	QList<Vec3d> orbitPoints; //orbit points represented by ElAzPos vectors

};

#endif // _SATELLITE_HPP_ 

