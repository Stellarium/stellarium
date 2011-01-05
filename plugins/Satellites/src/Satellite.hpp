/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#include "StelTextureTypes.hpp"
#include "StelSphereGeometry.hpp"

#include "StelPainter.hpp"
#include "gsatellite/gSatTEME.hpp"
#include "gsatellite/gObserver.hpp"
#include "gsatellite/gTime.hpp"
#include "gsatellite/gVector.hpp"


class StelPainter;
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
	//! @param id The official designation for a satellite, e.g. "ZARYA"
	//! @param conf a pointer to a QSettings object which contains the
	//! details of the satellite (TLEs, description etc.)
	Satellite(const QVariantMap& map);
	~Satellite();

	//! Get a QVariantMap which describes the satellite.  Could be used to
	//! create a duplicate.
	QVariantMap getMap(void);

	virtual QString getType(void) const {return "Satellite";}
	virtual float getSelectPriority(const StelNavigator *nav) const;

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
	virtual Vec3d getJ2000EquatorialPos(const StelNavigator *) const {return XYZ;}
	virtual float getVMagnitude(const StelNavigator* nav=NULL) const;
	virtual double getAngularSize(const StelCore* core) const;
	virtual QString getNameI18n(void) const {return designation;}
	virtual QString getEnglishName(void) const {return designation;}
	
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

public:
	void enableDrawOrbit(bool b);

private:
	//draw orbits methods
	void computeOrbitPoints();
	void drawOrbit(StelPainter& painter);
	//! returns 0 - 1.0 for the DRAWORBIT_FADE_NUMBER segments at
	//! each end of an orbit, with 1 in the middle.
	float calculateOrbitSegmentIntensity(int segNum);
	void setNightColors(bool night);

private:
	bool initialized;
	bool visible;
	bool orbitVisible;  //draw orbit enabled/disabled

	QString designation;               // The ID of the satllite
	QString description;               // longer description of spacecraft
	Vec3d XYZ;                         // holds J2000 position
	QPair< QByteArray, QByteArray > tleElements;
	double height, velocity, azimuth, elevation, range, rangeRate;
	QList<commLink> comms;
	Vec3f hintColor;
	QStringList groupIDs;
	QDateTime lastUpdated;
	
	static StelTextureSP hintTexture;
	static float hintBrightness;
	static float hintScale;
	static SphericalCap viewportHalfspace;
	static int orbitLineSegments;
	static int orbitLineFadeSegments;
	static int orbitLineSegmentDuration;
	static bool orbitLinesFlag;

	void draw(const StelCore* core, StelPainter& painter, float maxMagHints);
	void setObserverLocation(StelLocation* loc=NULL);


	//gsatellite objects
	gSatTEME *pSatellite;
	gObserver observer;
	gTime     epochTime;
	gVector   Position;
	gVector   Vel;
	gVector   LatLong;
	gVector   azElPos;

	//Satellite Orbit Draw
	QFont     font;
	Vec3f     orbitColorNormal;
	Vec3f     orbitColorNight;
	Vec3f*    orbitColor;
	gTime     lastEpochCompForOrbit;
	QList<gVector> orbitPoints; //orbit points represented by azElPos vectors

};

#endif // _SATELLITE_HPP_ 

