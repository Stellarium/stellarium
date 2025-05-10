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

protected:
	// components for Planet::getInfoString() that are overridden here:
	QString getInfoStringName(const StelCore *core, const InfoStringGroup& flags) const override;
	//! Any flag=Extra information to be displayed at the end
	QString getInfoStringExtra(const StelCore *core, const InfoStringGroup& flags) const override;

	//QString getDiscoveryCircumstances() const override; // nothing new.

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
};

#endif // MINORPLANET_HPP
