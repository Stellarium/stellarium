/*
 * Stellarium
 * Copyright (C) 2010 Bogdan Marinov
 * Copyright (C) 2013-14 Georg Zotti (accuracy&speedup)
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
 
#ifndef MINORPLANET_HPP
#define MINORPLANET_HPP

#include "Planet.hpp"
#include <QVector>

//! One snapshot of Keplerian orbital elements at a specific epoch,
//! used by the extended asteroid ephemeris pack.
//! Angles are in radians, distances in AU, time in JDE.
//!
//! meanAnomalyAtEpoch is stored instead of timeAtPericenter (Tp) because
//! Tp from JPL Horizons always refers to the nearest perihelion to the query
//! date and is therefore not a stable value to store per-epoch.  We derive
//! the equivalent t0 on demand as:
//!   t0 = epochJDE - meanAnomalyAtEpoch / meanMotion
struct AsteroidEpochElements
{
	double epochJDE;            //!< JDE epoch of these elements
	double pericenterDistance;  //!< AU
	double eccentricity;
	double inclination;         //!< radians
	double ascendingNode;       //!< radians
	double argOfPericenter;     //!< radians
	double meanAnomalyAtEpoch;  //!< radians at epochJDE (replaces timeAtPericenter)
	double meanMotion;          //!< radians/day
};

/*! \class MinorPlanet
	\author Bogdan Marinov

	This class implements a minor planet (an asteroid).

	There are two main reasons for having a separate class from Planet:
	 - visual magnitude calculation (asteroids use the H,G system;
       Stellarium's default algorithm is not easily applied to asteroids);
	 - support for minor planet numbers and IAU designations.

	Some of the code in this class is re-used from the parent Planet class.
  */
class MinorPlanet : public Planet
{
public:
	MinorPlanet(const QString& englishName,
		    double equatorialRadius,
		    double oblateness,
		    Vec3f halocolor,
		    float albedo,
		    float roughness,
		    const QString& texMapName,
		    const QString& normalMapName,
			const QString& horizonMapName,
		    const QString& objModelName,
		    posFuncType _coordFunc,
		    KeplerOrbit *orbitPtr,
		    OsculatingFunctType *osculatingFunc,
		    bool closeOrbit,
		    bool hidden,
		    const QString &pTypeStr);

	~MinorPlanet() override;

	//The MinorPlanet class inherits the "Planet" type because the SolarSystem class
	//was not designed to handle different types of objects.
	// \todo Decide if this is going to be "MinorPlanet" or "Asteroid"
	//virtual QString getType() const {return "MinorPlanet";}
	//! Override for minor planets.
	//! When observer is on an "Observer" planet, the magnitude reported is heliocentric (seen from the Sun).
	//! This allows observing the asteroid belt freom the side without nearer objects being obscured by phase angle.
	float getVMagnitude(const StelCore* core) const override;
	//! Convenience method, necessary override. @param eclipseFactor is ignored.
	float getVMagnitude(const StelCore* core, const double eclipseFactor) const override;
	//! sets the nameI18 property with the appropriate translation.
	//! Function overridden to handle the problem with name conflicts.
	void translateName(const StelTranslator& trans) override;
	QString getEnglishName(void) const override;
	QString getNameI18n(void) const override;
	//! gets an IAU provisional designation.
	QString getIAUDesignation() const override { return iauDesignationText; }
	//! get english name without the minor planet's number
	QString getCommonEnglishName(void) const { return englishName; }

	//! set the minor planet's number, if any.
	//! The number should be specified as an additional parameter, as
	//! englishName is passed as a constant to Planet's constructor.
	//! The number can be set only once. Any further calls of this function will
	//! have no result.
	void setMinorPlanetNumber(int number);

	//! sets an IAU provisional designation.
	void setIAUDesignation(const QString& designation);

	//! sets absolute magnitude (H) and slope parameter (G).
	//! These are the parameters in the IAU's two-parameter magnitude system
	//! for minor planets. They are used to calculate the apparent magnitude at
	//! different phase angles.
	//! @param magnitude Absolute magnitude H
	//! @param slope Slope parameter G. This is usually [0..1], sometimes slightly outside. Allowed here [-1..2].
	void setAbsoluteMagnitudeAndSlope(const float magnitude, const float slope);

	//! sets radius based on absolute magnitude and albedo.
	void updateEquatorialRadius(void);

	//! renders the subscript in a minor planet IAU provisional designation with HTML.
	static QString renderIAUDesignationinHtml(const QString& plainText);

	//! set values for spectral types
	void setSpectralType(const QString& sT, const QString& sB);

	//! set value for color index B-V
	void setColorIndexBV(float bv=99.f);

	//! set the discovery circumstances of minor body
	//! @param date of discovery
	//! @param name of discoverer
	void setDiscoveryData(const QString& date, const QString& name) { discoveryDate = date; discoverer = name; }

	//! sets a possible date, discovery and perihelion codes of the minor planets (A/ and I/ objects).
	void setExtraDesignations(QStringList codes) { extraDesignations = codes; }

	//! get list of comet codes
	QStringList getExtraDesignations() const { return extraDesignations; }

	//! get sidereal period for minor planet
	double getSiderealPeriod() const override;

	// Multi-epoch ephemeris support (extended ephemeris pack)-

	//! Load a table of orbital element snapshots from the extended ephemeris pack.
	//! @param elements  Vector of epoch snapshots, must be sorted ascending by epochJDE.
	//! @param parentRotObliquity      [radians] VSOP87 frame rotation parameter for the Sun.
	//! @param parentRotAscendingNode  [radians]
	//! @param parentRotJ2000Longitude [radians]
	void setEpochElements(const QVector<AsteroidEpochElements>& elements,
	                      double parentRotObliquity,
	                      double parentRotAscendingNode,
	                      double parentRotJ2000Longitude);

	//! Returns true if this object has a multi-epoch ephemeris table loaded.
	bool hasEpochElements() const { return !epochElements.isEmpty(); }

	//! Update the active KeplerOrbit to the nearest epoch snapshot for @p jde.
	//! A cheap guard prevents redundant rebuilds during normal playback.
	//! Pass @p force = true to bypass the guard — used by AstroCalc so that
	//! each ephemeris step gets the correct snapshot regardless of step size.
	void updateEpochOrbit(double jde, bool force = false);

protected:
	// components for Planet::getInfoString() that are overridden here:
	QString getInfoStringName(const StelCore *core, const InfoStringGroup& flags) const override;
	QString getNarrationName(const StelCore *core, const InfoStringGroup& flags) const override;
	//! Any flag=Extra information to be displayed at the end
	QString getInfoStringExtra(const StelCore *core, const InfoStringGroup& flags) const override;
	QString getNarrationExtra(const StelCore *core, const InfoStringGroup& flags) const override;

private:
	int minorPlanetNumber;
	float  slopeParameter; // This is G from the H, G system for computation of apparent magnitude.

	bool nameIsIAUDesignation;
	QString iauDesignationHtml;
	QString iauDesignationText;
	QStringList extraDesignations;
	QString properName;

	float b_v;
	QString specT, specB;

	// Multi-epoch ephemeris (empty unless extended pack is loaded)

	//! Epoch snapshots loaded from the ephemeris pack, sorted by epochJDE.
	QVector<AsteroidEpochElements> epochElements;

	//! VSOP87-frame rotation parameters, same for all epochs of this object.
	double epochParentRotObliquity      = 0.0;
	double epochParentRotAscendingNode  = 0.0;
	double epochParentRotJ2000Longitude = 0.0;

	//! JDE at the last updateEpochOrbit() call — used to skip redundant rebuilds.
	double lastEpochUpdateJDE = -1e100;

	//! The interpolated KeplerOrbit currently installed as the active orbit.
	//! This object owns it; Planet::orbitPtr points at the same instance.
	//! nullptr until the first updateEpochOrbit() call.
	KeplerOrbit* activeEpochOrbit = nullptr;
};

#endif // MINORPLANET_HPP
