/*
 * Stellarium
 * Copyright (C) 2010 Bogdan Marinov
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
 
#ifndef _MINOR_PLANET_HPP_
#define _MINOR_PLANET_HPP_

#include "Planet.hpp"

/*! \class MinorPlanet
	\author Bogdan Marinov

	This class implements a minor planet (an asteroid).

	There are two main reasons for having a separate class from Planet:
	 - visual magnitude calculation (asteroids use the H,G system;
       Stellarium's default algorithm is not easily applied to asteroids);
	 - support for minor planet numbers and provisional designations.

	Some of the code in this class is re-used from the parent Planet class.
  */
class MinorPlanet : public Planet
{
public:
	MinorPlanet(const QString& englishName,
		int flagLighting,
		double radius,
		double oblateness,
		Vec3f color,
		float albedo,
		const QString& texMapName,
		posFuncType _coordFunc,
		void* userDataPtr,
		OsculatingFunctType *osculatingFunc,
		bool closeOrbit,
		bool hidden,
		const QString &pType);

	~MinorPlanet();

	//Inherited from StelObject via Planet
	//! Get a string with data about the MinorPlanet.
	//! Asteroids support the following InfoStringGroup flags:
	//! - Name
	//! - Magnitude
	//! - RaDec
	//! - AltAzi
	//! - Distance
	//! - Size
	//! - PlainText
	//! \param core the StelCore object
	//! \param flags a set of InfoStringGroup items to include in the return value.
	//! \return a QString containing an HMTL encoded description of the MinorPlanet.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup &flags) const;
	//The Comet class inherits the "Planet" type because the SolarSystem class
	//was not designed to handle different types of objects.
	// \todo Decide if this is going to be "MinorPlanet" or "Asteroid"
	//virtual QString getType() const {return "MinorPlanet";}
	virtual float getVMagnitude(const StelCore* core, bool withExtinction=false) const;
	//! sets the nameI18 property with the appropriate translation.
	//! Function overriden to handle the problem with name conflicts.
	virtual void translateName(StelTranslator& trans);

	//! set the minor planet's number, if any.
	//! The number should be specified as an additional parameter, as
	//! englishName is passed as a constant to Planet's constructor.
	//! The number can be set only once. Any further calls of this function will
	//! have no result.
	void setMinorPlanetNumber(int number);

	//! sets a provisional designation.
	//! At the moment, the only role is for it to be displayed in the info
	//! field.
	//! \todo Support more than one provisional designations.
	//! \todo Include them in the search lists.
	void setProvisionalDesignation(QString designation);


	//! sets absolute magnitude (H) and slope parameter (G).
	//! These are the parameters in the IAU's two-parameter magnitude system
	//! for minor planets. They are used to calculate the apparent magnitude at
	//! different phase angles.
	void setAbsoluteMagnitudeAndSlope(double magnitude, double slope);

	//! renders the subscript in a minor planet provisional designation with HTML.
	//! \returns an emtpy string if the source string is not a provisional
	//! designation.
	static QString renderProvisionalDesignationinHtml(QString plainText);

	//! set value for semi-major axis in AU
	void setSemiMajorAxis(double value);

	//! get sidereal period for minor planet
	double getSiderealPeriod() const;

private:
	int minorPlanetNumber;
	double absoluteMagnitude;
	double slopeParameter;
	double semiMajorAxis;

	bool nameIsProvisionalDesignation;
	QString provisionalDesignationHtml;
	QString properName;
};

#endif //_MINOR_PLANET_HPP_
