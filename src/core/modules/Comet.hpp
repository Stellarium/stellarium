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
 
#ifndef _COMET_HPP_
#define _COMET_HPP_

#include "Planet.hpp"

/*! \class Comet
	\author Bogdan Marinov, Georg Zotti (orbit computation enhancements, tails)

	Some of the code in this class is re-used from the parent Planet class.
	\todo Implement better comet rendering (star-like objects, no physical body).
	\todo (Long-term) Photo realistic comet rendering, see https://blueprints.launchpad.net/stellarium/+spec/realistic-comet-rendering
	2013-12: New algorithms for position computation following Paul Heafner: Fundamental Ephemeris Computations (Willmann-Bell 1999).
  */
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
	       OsculatingFunctType *osculatingFunc,
	       bool closeOrbit,
	       bool hidden,
	       const QString &pType);

	virtual ~Comet();

	//Inherited from StelObject via Planet
	//! Get a string with data about the Comet.
	//! Comets support the following InfoStringGroup flags:
	//! - Name
	//! - Magnitude
	//! - RaDec
	//! - AltAzi
	//! - Distance
	//! - PlainText
	//  - Size <- Size of what?
	//! \param core the StelCore object
	//! \param flags a set of InfoStringGroup items to include in the return value.
	//! \return a QString containing an HMTL encoded description of the Comet.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup &flags) const;
	//The Comet class inherits the "Planet" type because the SolarSystem class
	//was not designed to handle different types of objects.
	//virtual QString getType() const {return "Comet";}
	//! \todo Find better sources for the g,k system
	virtual float getVMagnitude(const StelCore* core) const;

	//! \brief sets absolute magnitude and slope parameter.
	//! These are the parameters in the IAU's two-parameter magnitude system
	//! for comets. They are used to calculate the apparent magnitude at
	//! different distances from the Sun. They are not used in the same way
	//! as the same parameters in MinorPlanet.
	void setAbsoluteMagnitudeAndSlope(double magnitude, double slope);

	//! set value for semi-major axis in AU
	void setSemiMajorAxis(double value);

	//! get sidereal period for comet, days, or returns 0 if not possible (paraboloid, hyperboloid orbit)
	virtual double getSiderealPeriod() const;

	//! GZ new: re-implementation of Planet's draw()
	virtual void draw(StelCore* core, float maxMagLabels, const QFont& planetNameFont);

private:
//	float getTailLengthAU() const; //! return estimate for tail length
//	float getComaDiameterAU() const; //! return estimate for Coma diameter
	//! @returns estimates for (Coma diameter, gas tail length, dust tail length).
	//! Using a formula found at http://www.projectpluto.com/update7b.htm#comet_tail_formula
	//! @param dustFactor estimate of dust tail length in relation to gas tail length. Default: 0.7.
	Vec3f getComaTailLengthsAU(const float dustFactor=0.7f) const;
	void drawTail(StelCore* core, StelProjector::ModelViewTranformP transfo, float screenSz, bool gas);
	//! compute tail shape. This is a paraboloid shell with triangular mesh (indexed vertices).
	//! Try to call not for every frame...
	//! @param parameter the parameter p of the parabola. z=r²/2p (r²=x²+y²)
	//! @param lengthfactor The parabola will be lengthened. This shifts the visible focus, so it must be here.
	//! @param slices segments around the perimeter
	//! @param stacks cust along the rotational axis
	//! @param vertexArr vertex array, collects x0, y0, z0, x1, y1, z1, ...
	//! @param texCoordArr texture coordinates u0, v0, u1, v1, ...
	//! @param colorArr vertex colors (if not textured) r0, g0, b0, r1, g1, b1, ...
	//! @param indices into the former arrays (zero-starting), triplets forming triangles: t0,0, t0,1, t0,2, t1,0, t1,1, t1,2, ...
	void computeParabola(const float parameter, const float topradius, const float zshift, const int slices, const int stacks,
								QVector<double>& vertexArr, QVector<float>& texCoordArr, QVector<float>& colorArr, QVector<unsigned short>& indices);
	double absoluteMagnitude;
	double slopeParameter;
	double semiMajorAxis;

	bool isCometFragment;
	bool nameIsProvisionalDesignation;

	//GZ Tail additions
//	StelVertexArray *dustTail;    //!< a thin textured paraboloid that represents the dust tail
//	StelVertexArray *gasTail;     //!< an even thinner textured paraboloid that represents the gas tail
	QVector<double> dusttailVertexArr; // TBD: Maybe only one array is required, used with different scalings. Let's see.
	QVector<float> dusttailTexCoordArr;
	QVector<float> dusttailColorArr;
	QVector<unsigned short> dusttailIndices;
	QVector<double> gastailVertexArr;
	QVector<float> gastailTexCoordArr;
	QVector<float> gastailColorArr;
	QVector<unsigned short> gastailIndices;


	StelTextureSP dustTexture;
	StelTextureSP gasTexture;
	Mat4d scaleRotDust;           //!< rotation and scale matrix for dust tail
	Mat4d scaleRotGas;            //!< rotation and scale matrix for gas tail
	Vec3d eclipticSpeed;          //!< Speed in the solar system. Together with eclipticPos, this is the state vector. Required for rotating the tails.
};

#endif //_COMET_HPP_
