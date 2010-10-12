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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#ifndef _COMET_HPP_
#define _COMET_HPP_

#include "Planet.hpp"
 
class Comet : public Planet
{
public:
	Comet(const QString& englishName,
	       int flagLighting,
	       double radius,
	       double oblateness,
	       Vec3f color,
	       float albedo,
	       const QString& texMapName,
	       posFuncType _coordFunc,
	       void* userDataPtr,
	       OsulatingFunctType *osculatingFunc,
	       bool closeOrbit,
	       bool hidden);

	~Comet();

	//Inherited from StelObject via Planet
	//! Get a string with data about the Planet.
	//! Planets support the following InfoStringGroup flags:
	//! - Name
	//! - Magnitude
	//! - RaDec
	//! - AltAzi
	//! - Distance
	//! //- Size <- Size of what?
	//! - PlainText
	//! @param core the Stelore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the Planet.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup &flags) const;
	virtual QString getType() const {return "Comet";}
	virtual float getVMagnitude(const StelNavigator *nav) const;

	//! sets absolute magnitude and slope parameter
	//! These are the parameters in the IAU's two-parameter magnitude system
	//! for comets. They are used to calculate the apparent magnitude at
	//! different distances from the Sun. They are not used in the same way
	//! as the same parameters in MinorPlanet
	void setAbsoluteMagnitudeAndSlope(double magnitude, double slope);

private:
	double absoluteMagnitude;
	double slopeParameter;

	bool isCometFragment;
	bool nameIsProvisionalDesignation;
};

#endif //_COMET_HPP_
