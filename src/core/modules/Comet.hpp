/*
 * Stellarium
 * Copyright (C) 2010 Bogdan Marinov
 * Copyright (C) 2014 Georg Zotti (orbit fix, tails, speedup)
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
 
#ifndef COMET_HPP
#define COMET_HPP

#include "Planet.hpp"

/*! \class Comet
	\author Bogdan Marinov, Georg Zotti (orbit computation enhancements, tails)

	Some of the code in this class is re-used from the parent Planet class.
	\todo Implement better comet rendering (star-like objects, no physical body).
	\todo (Long-term) Photo realistic comet rendering, see https://blueprints.launchpad.net/stellarium/+spec/realistic-comet-rendering
	2013-12: GZ: New algorithms for position computation following Paul Heafner: Fundamental Ephemeris Computations (Willmann-Bell 1999).
	2014-01: GZ: Parabolic tails appropriately scaled/rotated. Much is currently empirical, leaving room for physics-based improvements.
	2014-08: GZ: speedup in case hundreds of comets are loaded.
	2014-11: GZ: tail extinction, better brightness balance.
	2017-03: GZ: added fields to infoMap
  */
class Comet : public Planet
{
public:
	friend class SolarSystem;               // Solar System initializes static constants.
	Comet(const QString& englishName,
	      double equatorialRadius,
	      double oblateness,
	      Vec3f halocolor,
	      float albedo,
	      float roughness,
	      float outgas_intensity,
	      float outgas_falloff,
	      const QString& texMapName,
	      const QString& objModelName,
	      posFuncType _coordFunc,
	      KeplerOrbit* orbitPtr,
	      OsculatingFunctType *osculatingFunc,
	      bool closeOrbit,
	      bool hidden,
	      const QString &pTypeStr,
	      float dustTailWidthFact=1.5f,
	      float dustTailLengthFact=0.4f,
	      float dustTailBrightnessFact=1.5f
	);

	virtual ~Comet() Q_DECL_OVERRIDE;

	//! In addition to Planet::getInfoMap(), Comets provides estimates for
	//! - tail-length-km
	//! - coma-diameter-km
	//! using the formula from Guide found by the GSoC2012 initiative at http://www.projectpluto.com/update7b.htm#comet_tail_formula
	virtual QVariantMap getInfoMap(const StelCore *core) const Q_DECL_OVERRIDE;
	//The Comet class inherits the "Planet" type because the SolarSystem class
	//was not designed to handle different types of objects.
	//virtual QString getType() const {return "Comet";}
	//! \todo Find better sources for the g,k system
	virtual float getVMagnitude(const StelCore* core) const Q_DECL_OVERRIDE;
	//! sets the nameI18 property with the appropriate translation.
	//! Function overriden to handle the problem with name conflicts.
	virtual void translateName(const StelTranslator& trans) Q_DECL_OVERRIDE;
	virtual QString getEnglishName(void) const Q_DECL_OVERRIDE {return englishName;}
	virtual QString getNameI18n(void) const Q_DECL_OVERRIDE {return nameI18;}

	//! \brief sets absolute magnitude and slope parameter.
	//! These are the parameters in the IAU's two-parameter magnitude system
	//! for comets. They are used to calculate the apparent magnitude at
	//! different distances from the Sun. They are not used in the same way
	//! as the same parameters in MinorPlanet.
	void setAbsoluteMagnitudeAndSlope(const float magnitude, const float slope);

	//! get sidereal period for comet, days, or returns 0 if not possible (parabolic, hyperbolic orbit)
	virtual double getSiderealPeriod() const Q_DECL_OVERRIDE;

	//! re-implementation of Planet's draw()
	virtual void draw(StelCore* core, float maxMagLabels, const QFont& planetNameFont) Q_DECL_OVERRIDE;

	// re-implementation of Planet's update() to prepare tails (extinction etc). @param deltaTime: ms (since last call)
	virtual void update(int deltaTime) Q_DECL_OVERRIDE;

protected:
	// components for Planet::getInfoString() that are overridden here:
	virtual QString getInfoStringAbsoluteMagnitude(const StelCore *core, const InfoStringGroup& flags) const Q_DECL_OVERRIDE;
	//! Any flag=Size information to be displayed
	virtual QString getInfoStringSize(const StelCore *core, const InfoStringGroup& flags) const Q_DECL_OVERRIDE;
	//! Any flag=Extra information to be displayed at the end
	virtual QString getInfoStringExtra(const StelCore *core, const InfoStringGroup& flags) const Q_DECL_OVERRIDE;

private:
	//! @returns estimates for (Coma diameter [AU], gas tail length [AU]).
	//! Using the formula from Guide found by the GSoC2012 initiative at http://www.projectpluto.com/update7b.htm#comet_tail_formula
	Vec2f getComaDiameterAndTailLengthAU() const;
	void drawTail(StelCore* core, StelProjector::ModelViewTranformP transfo, bool gas);
	void drawComa(StelCore* core, StelProjector::ModelViewTranformP transfo);

	//! compute a coma, faked as simple disk to be tilted towards the observer.
	//! @param diameter Diameter of Coma [AU]
	void computeComa(const float diameter);

	//! compute tail shape. This is a paraboloid shell with triangular mesh (indexed vertices).
	//! Try to call not for every frame...
	//! To be more efficient, the arrays are only computed if they are empty.
	//! @param parameter the parameter p of the parabola. z=r²/2p (r²=x²+y²)
	//! @param lengthfactor The parabola will be lengthened. This shifts the visible focus, so it must be here.
	//! @param vertexArr vertex array, collects x0, y0, z0, x1, y1, z1, ...
	//! @param texCoordArr texture coordinates u0, v0, u1, v1, ...
	//! @param colorArr vertex colors (if not textured) r0, g0, b0, r1, g1, b1, ...
	//! @param indices into the former arrays (zero-starting), triplets forming triangles: t0,0, t0,1, t0,2, t1,0, t1,1, t1,2, ...
	//! @param xOffset for the dust tail, this may introduce a bend. Units are x per sqrt(z).
	void computeParabola(const float parameter, const float topradius, const float zshift, QVector<Vec3d>& vertexArr, QVector<Vec2f>& texCoordArr, QVector<unsigned short>& indices, const float xOffset=0.0f);

	float slopeParameter;
	bool isCometFragment;
	bool nameIsProvisionalDesignation;

	//GZ Tail additions
	Vec2f tailFactors; // result of latest call to getComaDiameterAndTailLengthAU(); Results cached here for infostring. [0]=Coma diameter, [1] gas tail length.
	bool tailActive;		//! true if there is a tail long enough to be worth drawing. Drawing tails is quite costly.
	bool tailBright;		//! true if tail is bright enough to draw.
	double deltaJDEtail;            //! like deltaJDE, but time difference between tail geometry updates.
	double lastJDEtail;             //! like lastJDE, but time of last tail geometry update.
	Mat4d gasTailRot;		//! rotation matrix for gas tail parabola
	Mat4d dustTailRot;		//! rotation matrix for the skewed dust tail parabola
	float dustTailWidthFactor;      //!< empirical individual broadening of the dust tail end, compared to the gas tail end. Actually, dust tail width=2*comaWidth*dustTailWidthFactor. Default 1.5
	float dustTailLengthFactor;     //!< empirical individual length of dust tail relative to gas tail. Taken from ssystem.ini, typical value 0.3..0.5, default 0.4
	float dustTailBrightnessFactor; //!< empirical individual brightness of dust tail relative to gas tail. Taken from ssystem.ini, default 1.5
	QVector<Vec3d> comaVertexArr;
	QVector<Vec2f> comaTexCoordArr; //  --> 2014-08: could also be declared static, but it is filled by StelPainter...

	float intensityFovScale; // like for constellations: reduce brightness when zooming in.
	float intensityMinFov;
	float intensityMaxFov;


	// These are static to avoid having index arrays for each comet when all are equal.
	static bool createTailIndices;
	static bool createTailTextureCoords;

	QVector<Vec3d> gastailVertexArr;  // computed frequently, describes parabolic shape (along z axis) of gas tail.
	QVector<Vec3d> dusttailVertexArr; // computed frequently, describes parabolic shape (along z axis) of dust tail.
	QVector<Vec3f> gastailColorArr;    // NEW computed for every 5 mins, modulates gas tail brightness for extinction
	QVector<Vec3f> dusttailColorArr;   // NEW computed for every 5 mins, modulates dust tail brightness for extinction
	static QVector<Vec2f> tailTexCoordArr; // computed only once for all comets!
	static QVector<unsigned short> tailIndices; // computed only once for all comets!
	static StelTextureSP comaTexture;
	static StelTextureSP tailTexture;      // it seems not really necessary to have different textures. gas tail is just painted blue.
};

#endif //COMET_HPP
