/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef PLANET_HPP
#define PLANET_HPP

#include "StelObject.hpp"
#include "StelProjector.hpp"
#include "StelPropertyMgr.hpp"
#include "VecMath.hpp"
#include "GeomMath.hpp"
#include "StelFader.hpp"
#include "StelTextureTypes.hpp"
#include "StelProjectorType.hpp"
#include "RotationElements.hpp"

#include <QCache>
#include <QString>

// The callback type for the external position computation function
// arguments are JDE, position[3], velocity[3].
// The last variable is the userData pointer, which is Q_NULLPTR for Planets, but used in derived classes. E.g. points to the KeplerOrbit for Comets.
typedef void (*posFuncType)(double, double*, double*, void*);

typedef void (OsculatingFunctType)(double jde0,double jde,double xyz[3], double xyzdot[3]);

// epoch J2000: 12 UT on 1 Jan 2000
#define J2000 2451545.0
#define ORBIT_SEGMENTS 360

class Orbit;
class KeplerOrbit;
class StelFont;
class StelPainter;
class StelTranslator;
class StelOBJ;
class StelOpenGLArray;
class HipsSurvey;
template <class T> class QFuture;
class QOpenGLBuffer;
class QOpenGLFunctions;
class QOpenGLShaderProgram;
class QOpenGLTexture;
#ifdef DEBUG_SHADOWMAP
class QOpenGLFramebufferObject;
#endif

typedef QSharedPointer<class HipsSurvey> HipsSurveyP;

// Class to manage rings for planets like Saturn
class Ring
{
public:
	Ring(float radiusMin, float radiusMax,const QString &texname);
	double getSize(void) const {return static_cast<double>(radiusMax);}
	const float radiusMin;
	const float radiusMax;
	StelTextureSP tex;
};


class Planet : public StelObject
{
public:
	static const QString PLANET_TYPE;
	friend class SolarSystem;

	Q_ENUMS(PlanetType)
	Q_ENUMS(PlanetOrbitColorStyle)
	Q_ENUMS(ApparentMagnitudeAlgorithm)
	//! numeric typecodes for the type descriptions in ssystem.ini
	// GZ: Until 0.13 QStrings were used for types.
	// GZ: Enums are slightly faster than string comparisons in time-critical comparisons.
	// GZ: If other types are introduced, add here and the string in init().
	// GZ TODO for 0.19: Preferably convert this into a bitfield and allow several bits set:
	// Cubewanos, SDO, OCO, Sednoids are Asteroids, Pluto is a Plutino and DwarfPlanet, Ceres is Asteroid and DwarfPlanet etc.!
	// Maybe even add queries like Planet::isAsteroid() { return (planetType & Planet::isAsteroid);}
	enum PlanetType
	{
		isStar,         // ssystem.ini: type="star"
		isPlanet,       // ssystem.ini: type="planet"
		isMoon,         // ssystem.ini: type="moon"
		isObserver,     // ssystem.ini: type="observer"
		isArtificial,   // Used in transitions from planet to planet.
		isAsteroid,     // ssystem.ini: type="asteroid". all types >= isAsteroid are "Minor Bodies".
				// Put other things (spacecraft etc) before isAsteroid.
		isPlutino,      // ssystem.ini: type="plutino"
		isComet,        // ssystem.ini: type="comet"
		isDwarfPlanet,  // ssystem.ini: type="dwarf planet"
		isCubewano,     // ssystem.ini: type="cubewano"
		isSDO,          // ssystem.ini: type="scattered disc object"
		isOCO,          // ssystem.ini: type="oco"
		isSednoid,      // ssystem.ini: type="sednoid"
		isInterstellar, // ssystem.ini: type="interstellar object"
		isUNDEFINED     // ssystem.ini: type=<anything else>. THIS IS ONLY IN CASE OF ERROR!
	};

	enum PlanetOrbitColorStyle
	{
		ocsOneColor,		// One color for all orbits
		ocsGroups,		// Separate colors for each group of Solar system bodies
		ocsMajorPlanets		// Separate colors for each of major planets of Solar system
	};

	enum ApparentMagnitudeAlgorithm
	{
		Mueller_1893,               // G. Mueller, based on visual observations 1877-91. [Explanatory Supplement to the Astronomical Almanac, 1961]
		AstronomicalAlmanac_1984,   // Astronomical Almanac 1984 and later. These give V (instrumental) magnitudes (allegedly from D.L. Harris, but this is wrong!)
		ExplanatorySupplement_1992, // Algorithm provided by Pere Planesas (Observatorio Astronomico Nacional) (Was called "Planesas")
		ExplanatorySupplement_2013, // Explanatory Supplement to the Astronomical Almanac, 3rd edition 2013
		MallamaHilton_2018,         // A. Mallama, J. L. Hilton: Computing apparent planetary magnitudes for the Astronomical Almanac. Astron.&Computing 25 (2018) 10-24
		UndefinedAlgorithm,
		Generic                     // Visual magnitude based on phase angle and albedo. The formula source for this is totally unknown!
	};

public:
	Planet(const QString& englishName,
	       double equatorialRadius,
	       double oblateness,
	       Vec3f halocolor,
	       float albedo,
	       float roughness,
	       const QString& texMapName,
	       const QString& normalMapName,
	       const QString& objModelName,
	       posFuncType _coordFunc,
	       Orbit *anOrbitPtr,
	       OsculatingFunctType *osculatingFunc,
	       bool closeOrbit,
	       bool hidden,
	       bool hasAtmosphere,
	       bool hasHalo,
	       const QString &pTypeStr);

	virtual ~Planet() Q_DECL_OVERRIDE;

	//! Initializes static vars. Must be called before creating first planet.
	// Currently ensured by SolarSystem::init()
	static void init();

	///////////////////////////////////////////////////////////////////////////
	// Methods inherited from StelObject
	//! Get a string with data about the Planet.
	//! Planets support the following InfoStringGroup flags:
	//! - Name
	//! - Magnitude
	//! - RaDec
	//! - AltAzi
	//! - Distance
	//! - Size
	//! - PlainText
	//! - Extra: Heliocentric Ecliptical Coordinates & Observer-planetocentric Ecliptical Coordinates, Phase, illumination, phase angle & elongation from the Sun
	//! @note subclasses should prefer to override only the component infostrings getInfoString...(), not this method!
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the Planet.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const Q_DECL_OVERRIDE;
	//! In addition to the entries from StelObject::getInfoMap(), Planet objects provide
	//! - distance
	//! - phase (result of getPhase)
	//! - illumination (=100*phase)
	//! - phase-angle
	//! - phase-angle-dms (formatted string)
	//! - phase-angle-deg (formatted string)
	//! - elongation
	//! - elongation-dms (formatted string)
	//! - elongation-deg (formatted string)
	//! - type (object type description)
	//! - velocity (formatted string)
	//! - heliocentric-velocity (formatted string)
	//! - scale
	//! - eclipse-obscuration (for Sun only)
	//! - eclipse-magnitude (for Sun only)
	//! - central_l (on Earth only; degrees)
	//! - central_b (on Earth only; degrees)
	//! - pa_axis (on Earth only; degrees)
	//! - subsolar_l (on Earth only; degrees)
	//! - subsolar_b (on Earth only; degrees)
	//! - libration_l (on Earth for Moon only; degrees)
	//! - libration_b (on Earth for Moon only; degrees)
	//! - colongitude (on Earth for Moon only; degrees)
	//! - penumbral-eclipse-magnitude (on Earth for Moon only)
	//! - umbral-eclipse-magnitude (on Earth for Moon only)
	virtual QVariantMap getInfoMap(const StelCore *core) const  Q_DECL_OVERRIDE;
	virtual double getCloseViewFov(const StelCore* core) const Q_DECL_OVERRIDE;
	virtual double getSatellitesFov(const StelCore* core) const Q_DECL_OVERRIDE;
	virtual double getParentSatellitesFov(const StelCore* core) const Q_DECL_OVERRIDE;
	virtual float getVMagnitude(const StelCore* core) const Q_DECL_OVERRIDE;
	virtual float getSelectPriority(const StelCore* core) const Q_DECL_OVERRIDE;
	virtual Vec3f getInfoColor(void) const Q_DECL_OVERRIDE;
	virtual QString getType(void) const Q_DECL_OVERRIDE {return PLANET_TYPE;}
	virtual QString getID(void) const Q_DECL_OVERRIDE { return englishName; }
	virtual Vec3d getJ2000EquatorialPos(const StelCore *core) const Q_DECL_OVERRIDE;
	virtual QString getEnglishName(void) const Q_DECL_OVERRIDE;
	virtual QString getNameI18n(void) const Q_DECL_OVERRIDE;
	QString getNativeName(void) const { return nativeName; }
	QString getNativeNameI18n(void) const { return nativeNameMeaningI18n; }
	QString getCommonEnglishName(void) const {return englishName;}
	QString getCommonNameI18n(void) const {return nameI18;}
	//! Get angular semidiameter, degrees. If planet display is artificially enlarged (e.g. Moon upscale), value will also be increased.
	virtual double getAngularSize(const StelCore* core) const Q_DECL_OVERRIDE;
	virtual bool hasAtmosphere(void) {return atmosphere;}
	virtual bool hasHalo(void) {return halo;}
	float getAxisRotation(void) { return axisRotation;} //! return axisRotation last computed in computeTransMatrix().

	///////////////////////////////////////////////////////////////////////////
	// Methods of SolarSystem object
	//! Translate planet name using the passed translator
	virtual void translateName(const StelTranslator &trans);

	// Draw the Planet
	// GZ Made that virtual to allow comets having their own draw().
	virtual void draw(StelCore* core, float maxMagLabels, const QFont& planetNameFont);

	///////////////////////////////////////////////////////////////////////////
	// Methods specific to Planet
	//! Get the equator radius of the planet in AU.
	//! @return the equator radius of the planet in astronomical units.
	double getEquatorialRadius(void) const {return equatorialRadius;}
	//! Get the value (1-f) for oblateness f=polarRadius/equatorialRadius.
	double getOneMinusOblateness(void) const {return oneMinusOblateness;}
	//! Get the polar radius of the planet in AU.
	//! @return the polar radius of the planet in astronomical units.
	double getPolarRadius(void) const {return equatorialRadius*oneMinusOblateness;}
	//! Get duration of sidereal day (earth days, may come from rot_periode or orbit_period (for moons) from ssystem_*.ini)
	double getSiderealDay(void) const { if (re.W1!=0.) return 360.0/re.W1; else return static_cast<double>(re.period);} // I assume the more modern values are better.
	//! Get duration of sidereal year
	// must be virtual for Comets.
	virtual double getSiderealPeriod(void) const { return siderealPeriod; }
	//! set duration of sidereal year. Also sets deltaOrbitJDE and may set closeOrbit for Planet objects which have KeplerOrbits.
	//! siderealPeriod [earth days] orbital duration.
	void setSiderealPeriod(const double siderealPeriod);
	//! Get duration of mean solar day, in earth days.
	double getMeanSolarDay(void) const;
	//! Get albedo
	double getAlbedo(void) const { return static_cast<double>(albedo); }

	const QString& getTextMapName() const {return texMapName;}
	const QString getPlanetTypeString() const {return pTypeMap.value(pType);}
	PlanetType getPlanetType() const {return pType;}
	Orbit* getOrbit() const {return orbitPtr;}

	void setNativeName(QString planet) { nativeName = planet; }
	void setNativeNameMeaning(QString planet) { nativeNameMeaning = planet; }

	//! set the IAU moon number (designation of the moon), if any.
	void setIAUMoonNumber(QString designation);

	//! Return the absolute magnitude (read from file ssystem.ini)
	float getAbsoluteMagnitude() const {return absoluteMagnitude;}
	//! Return the mean opposition magnitude, defined as V(1,0)+5log10(a(a-1))
	//! A return value of 100 signals invalid result.
	float getMeanOppositionMagnitude() const;
	//
	static ApparentMagnitudeAlgorithm getApparentMagnitudeAlgorithm()  { return vMagAlgorithm; }
	static const QString getApparentMagnitudeAlgorithmString()  { return vMagAlgorithmMap.value(vMagAlgorithm); }
	static void setApparentMagnitudeAlgorithm(QString algorithm);
	static void setApparentMagnitudeAlgorithm(ApparentMagnitudeAlgorithm algorithm){ vMagAlgorithm=algorithm; }

	//! Compute the axial z rotation (daily rotation around the polar axis) [degrees] to use from equatorial to hour angle based coordinates.
	//! On Earth, sidereal time on the other hand is the angle along the planet equator from RA0 to the meridian, i.e. hour angle of the first point of Aries.
	//! For Earth (of course) it is sidereal time at Greenwich.
	//! V0.21+ update:
	//! For planets and Moons, in this context this is the rotation angle W of the Prime meridian from the ascending node of the planet equator on the ICRF equator.
	//! The usual WGCCRE model is W=W0+d*W1. Some planets/moons have more complicated rotations though, these are also handled in here.
	//! The planet objects with old-style data are computed like in earlier versions of Stellarium. Their computational model is however questionable.
	//! For general applicability we need both time flavours:
	//! @param JD is JD(UT) for Earth
	//! @param JDE is used for other locations
	double getSiderealTime(double JD, double JDE) const;

	//! return a rotation matrix from the planet's equatorial coordinate frame to the VSOP87 (ecliptical J2000) frame.
	//! For planets/moons with WGCCRE rotation elements, this is just a single matrix.
	//! For objects with traditional elements, this builds up the matrix from the parents.
	Mat4d getRotEquatorialToVsop87(void) const;
	//! set a rotation matrix from the planet's equatorial coordinate frame to the VSOP87 (ecliptical J2000) frame.
	//! For planets/moons with WGCCRE rotation elements, this just sets this matrix.
	//! For objects with traditional elements, this builds up the matrix from the parents.
	void setRotEquatorialToVsop87(const Mat4d &m);

	const RotationElements &getRotationElements(void) const {return re;}

	//! Set the rotational elements.
	//! Given two data models, we must support both: the traditional elements relative to the parent object:
	//! name: English name of the object. A corrective function may be attached which depends on the name.
	//! _period: duration of sidereal rotation [Julian days]
	//! _offset: [angle at _epoch. ]
	//! _epoch: [JDE]
	//! _obliquity [rad]
	//! _ascendingNode of equator on ecliptic[rad]
	//! The more modern way to specify these elements are relative to the ICRF:
	//! ra_pole=_ra0 + T*_ra1. ra_pole and de_pole must be computed more than for initialisation for J2000
	//! de_pole=_de0 + T*_de1. ra and de values to be stored in [rad]
	//! _w0, _w1 to be given in degrees!
	//! If _ra0 is not zero, we understand WGCCRE data ra0, ra1, de0, de1, w0, w1 are used.
	void setRotationElements(const QString name, const double _period, const double _offset, const double _epoch,
				 const double _obliquity, const double _ascendingNode,
				 const double _ra0, const double _ra1,
				 const double _de0, const double _de1,
				 const double _w0,  const double _w1);

	//! Note: The only place where this is used is to build up orbits for planet moons w.r.t. the parent planet orientation.
	double getRotAscendingNode(void) const {return re.ascendingNode; }
	//! return angle between axis and normal of ecliptic plane (or, for a moon, equatorial/reference plane defined by parent).
	//! For Earth, this is the angle between axis and normal to current ecliptic of date, i.e. the ecliptic obliquity of date JDE.
	//! Note: The only place where this is not used for Earth is to build up orbits for planet moons w.r.t. the parent planet orientation.
	double getRotObliquity(double JDE) const;

	//! Compute the position in the parent Planet coordinate system
	virtual void computePosition(const double dateJDE);

	//! Compute the transformation matrix from the local Planet coordinate to the parent Planet coordinate.
	//! This requires both flavours of JD in cases involving Earth.
	void computeTransMatrix(double JD, double JDE);

	//! Get the phase angle (radians) for an observer at pos obsPos in heliocentric coordinates (in AU)
	double getPhaseAngle(const Vec3d& obsPos) const;
	//! Get the elongation angle (radians) for an observer at pos obsPos in heliocentric coordinates (in AU)
	double getElongation(const Vec3d& obsPos) const;
	//! Get the angular radius (degrees) of the planet spheroid (i.e. without the rings)
	double getSpheroidAngularSize(const StelCore* core) const;
	//! Get the planet phase (illuminated fraction of the planet disk, [0=dark..1=full]) for an observer at pos obsPos in heliocentric coordinates (in AU)
	float getPhase(const Vec3d& obsPos) const;
	//! Get the position angle of the illuminated limb of a planet
	//! The result depends on the arguments' coordinate system which must be identical.
	//! E.g. if both are equatorial for equinox of date or J2000, the angle is zero when the bright limb is towards the north of the disk.
	//! An angle of 90° indicates a bright limb on the eastern limb, like an old moon.
	//! Source: Meeus, Astr.Algorithms (2nd ed.), 48.5.
	static float getPAsun(const Vec3d &sunPos, const Vec3d &objPos);

	//! Get planetographic coordinates of subsolar and sub-observer points.
	//! Only meaningful for earth-bound observers.
	//! Source: Explanatory Supplement 2013, 10.4.1
	//! first[0]  = 10.26 phi_e     [rad] Planetocentric latitude of sub-earth point
	//! first[1]  = 10.26 phi'_e	[rad] Planetographic latitude of sub-earth point
	//! first[2]  = 10.26 lambda'_e	[rad] Planetographic longitude of sub-earth point (0..2pi)
	//! first[3]  = 10.29 P_n	[rad] Position angle of axis north pole in equatorial coordinates of date
	//! second[0] = 10.26 phi_s	[rad] Planetocentric latitude of sub-solar point
	//! second[1] = 10.26 phi'_s	[rad] Planetographic latitude of sub-solar point
	//! second[2] = 10.26 lambda'_s	[rad] Planetographic longitude of sub-solar point (0..2pi)
	//! Note: For the Moon, it is more common to give Libration angles, where L=-lambda'_e, B=phi'_e.
	//! Note: For Jupiter, this returns central meridian in L_II.
	//! Note: For Saturn, this returns central meridian in L_III (rotation of magnetic field).
	QPair<Vec4d, Vec3d> getSubSolarObserverPoints(const StelCore *core) const;

	//! returns if planet has retrograde rotation
	bool isRotatingRetrograde() const { return re.W1<0.; }

	//! Get the Planet position in the parent Planet ecliptic coordinate in AU
	Vec3d getEclipticPos(double dateJDE) const;
	Vec3d getEclipticPos() const {return getEclipticPos(lastJDE);}

	//! Return the heliocentric ecliptical position
	Vec3d getHeliocentricEclipticPos() const {return getHeliocentricPos(eclipticPos);}
	Vec3d getHeliocentricEclipticPos(double dateJDE) const;

	//! Return the heliocentric transformation for local (parentocentric) coordinate
	//! @arg p planetocentric rectangular ecliptical coordinate (J2000)
	//! @return heliocentric rectangular ecliptical coordinates (J2000)
	Vec3d getHeliocentricPos(Vec3d p) const;
	//! Propagate the heliocentric coordinates to parentocentric coordinates
	//! @arg pos heliocentric rectangular ecliptical coordinate (J2000)
	void setHeliocentricEclipticPos(const Vec3d &pos);

	//! Get the planet velocity around the parent planet in ecliptical coordinates in AU/d
	Vec3d getEclipticVelocity() const {return eclipticVelocity;}

	//! Get the planet's heliocentric velocity in the solar system in ecliptical coordinates in AU/d. Required for aberration!
	Vec3d getHeliocentricEclipticVelocity() const;

	//! Compute and return the distance to the given position in heliocentric ecliptical (J2000) coordinates (in AU)
	double computeDistance(const Vec3d& obsHelioPos);
	//! Return the last computed distance to the given position in heliocentric ecliptical (J2000) coordinates (in AU)
	double getDistance(void) const {return distance;}

	void setRings(Ring* r) {rings = r;}

	void setSphereScale(double s) { if(!fuzzyEquals(s, sphereScale)) { sphereScale = s; if(objModel) objModel->needsRescale=true; } }
	double getSphereScale() const { return sphereScale; }

	const QSharedPointer<Planet> getParent(void) const {return parent;}

	static void setLabelColor(const Vec3f& lc) {labelColor = lc;}
	static const Vec3f& getLabelColor(void) {return labelColor;}

	// update displayed elements. @param deltaTime: ms (since last call)
	virtual void update(int deltaTime);

	void setFlagHints(bool b){hintFader = b;}
	bool getFlagHints(void) const {return hintFader;}

	void setFlagLabels(bool b){flagLabels = b;}
	bool getFlagLabels(void) const {return flagLabels;}

	bool flagNativeName;
	void setFlagNativeName(bool b) { flagNativeName = b; }
	bool getFlagNativeName(void) const { return flagNativeName; }

	///////////////////////////////////////////////////////////////////////////
	// DEPRECATED
	///// Orbit related code
	// Should move to an OrbitPath class which works on a SolarSystemObject, not a Planet
	void setFlagOrbits(bool b){orbitFader = b;}
	bool getFlagOrbits(void) const {return orbitFader;}
	LinearFader orbitFader;
	// draw orbital path of Planet
	void drawOrbit(const StelCore*);
	Vec3d orbit[ORBIT_SEGMENTS+1];  // store heliocentric coordinates for drawing the orbit
	double deltaJDE;                // time difference between positional updates.
	double deltaOrbitJDE;
	bool closeOrbit;                // whether to connect the beginning of the orbit line to
					// the end: good for elliptical orbits, bad for parabolic
					// and hyperbolic orbits

	static Vec3f orbitColor;
	static void setOrbitColor(const Vec3f& oc) {orbitColor = oc;}
	static const Vec3f& getOrbitColor() {return orbitColor;}

	static Vec3f orbitMajorPlanetsColor;
	static void setMajorPlanetOrbitColor(const Vec3f& oc) { orbitMajorPlanetsColor = oc;}
	static const Vec3f& getMajorPlanetOrbitColor() {return orbitMajorPlanetsColor;}

	static Vec3f orbitMoonsColor;
	static void setMoonOrbitColor(const Vec3f& oc) { orbitMoonsColor = oc;}
	static const Vec3f& getMoonOrbitColor() {return orbitMoonsColor;}

	static Vec3f orbitMinorPlanetsColor;
	static void setMinorPlanetOrbitColor(const Vec3f& oc) { orbitMinorPlanetsColor = oc;}
	static const Vec3f& getMinorPlanetOrbitColor() {return orbitMinorPlanetsColor;}

	static Vec3f orbitDwarfPlanetsColor;
	static void setDwarfPlanetOrbitColor(const Vec3f& oc) { orbitDwarfPlanetsColor = oc;}
	static const Vec3f& getDwarfPlanetOrbitColor() {return orbitDwarfPlanetsColor;}

	static Vec3f orbitCubewanosColor;
	static void setCubewanoOrbitColor(const Vec3f& oc) { orbitCubewanosColor = oc;}
	static const Vec3f& getCubewanoOrbitColor() {return orbitCubewanosColor;}

	static Vec3f orbitPlutinosColor;
	static void setPlutinoOrbitColor(const Vec3f& oc) { orbitPlutinosColor = oc;}
	static const Vec3f& getPlutinoOrbitColor() {return orbitPlutinosColor;}

	static Vec3f orbitScatteredDiscObjectsColor;
	static void setScatteredDiscObjectOrbitColor(const Vec3f& oc) { orbitScatteredDiscObjectsColor = oc;}
	static const Vec3f& getScatteredDiscObjectOrbitColor() {return orbitScatteredDiscObjectsColor;}

	static Vec3f orbitOortCloudObjectsColor;
	static void setOortCloudObjectOrbitColor(const Vec3f& oc) { orbitOortCloudObjectsColor = oc;}
	static const Vec3f& getOortCloudObjectOrbitColor() {return orbitOortCloudObjectsColor;}

	static Vec3f orbitCometsColor;
	static void setCometOrbitColor(const Vec3f& oc) { orbitCometsColor = oc;}
	static const Vec3f& getCometOrbitColor() {return orbitCometsColor;}

	static Vec3f orbitSednoidsColor;
	static void setSednoidOrbitColor(const Vec3f& oc) { orbitSednoidsColor = oc;}
	static const Vec3f& getSednoidOrbitColor() {return orbitSednoidsColor;}

	static Vec3f orbitInterstellarColor;
	static void setInterstellarOrbitColor(const Vec3f& oc) { orbitInterstellarColor = oc;}
	static const Vec3f& getInterstellarOrbitColor() {return orbitInterstellarColor;}

	static Vec3f orbitMercuryColor;
	static void setMercuryOrbitColor(const Vec3f& oc) { orbitMercuryColor = oc;}
	static const Vec3f& getMercuryOrbitColor() {return orbitMercuryColor;}

	static Vec3f orbitVenusColor;
	static void setVenusOrbitColor(const Vec3f& oc) { orbitVenusColor = oc;}
	static const Vec3f& getVenusOrbitColor() {return orbitVenusColor;}

	static Vec3f orbitEarthColor;
	static void setEarthOrbitColor(const Vec3f& oc) { orbitEarthColor = oc;}
	static const Vec3f& getEarthOrbitColor() {return orbitEarthColor;}

	static Vec3f orbitMarsColor;
	static void setMarsOrbitColor(const Vec3f& oc) { orbitMarsColor = oc;}
	static const Vec3f& getMarsOrbitColor() {return orbitMarsColor;}

	static Vec3f orbitJupiterColor;
	static void setJupiterOrbitColor(const Vec3f& oc) { orbitJupiterColor = oc;}
	static const Vec3f& getJupiterOrbitColor() {return orbitJupiterColor;}

	static Vec3f orbitSaturnColor;
	static void setSaturnOrbitColor(const Vec3f& oc) { orbitSaturnColor = oc;}
	static const Vec3f& getSaturnOrbitColor() {return orbitSaturnColor;}

	static Vec3f orbitUranusColor;
	static void setUranusOrbitColor(const Vec3f& oc) { orbitUranusColor = oc;}
	static const Vec3f& getUranusOrbitColor() {return orbitUranusColor;}

	static Vec3f orbitNeptuneColor;
	static void setNeptuneOrbitColor(const Vec3f& oc) { orbitNeptuneColor = oc;}
	static const Vec3f& getNeptuneOrbitColor() {return orbitNeptuneColor;}

	static PlanetOrbitColorStyle orbitColorStyle;

	//! Return the list of planets which project some shadow on this planet
	QVector<const Planet*> getCandidatesForShadow() const;
	
protected:
	// These components for getInfoString() can be overridden in subclasses
	virtual QString getInfoStringName(const StelCore *core, const InfoStringGroup& flags) const;
	virtual QString getInfoStringAbsoluteMagnitude(const StelCore *core, const InfoStringGroup& flags) const;
	//! Any flag=Extra information to be displayed after the magnitude strings
	virtual QString getInfoStringExtraMag(const StelCore *core, const InfoStringGroup& flags) const;
	//! Any flag=Size information to be displayed
	virtual QString getInfoStringSize(const StelCore *core, const InfoStringGroup& flags) const;
	//! Return elongation and phase angle when flags=Elongation
	virtual QString getInfoStringEloPhase(const StelCore *core, const InfoStringGroup& flags, const bool withIllum) const;
	//! Return sidereal and synodic periods when flags=Extra
	virtual QString getInfoStringPeriods(const StelCore *core, const InfoStringGroup& flags) const;
	//! Any flag=Extra information to be displayed at the end
	virtual QString getInfoStringExtra(const StelCore *core, const InfoStringGroup& flags) const;

protected:
	struct PlanetOBJModel
	{
		PlanetOBJModel();
		~PlanetOBJModel();

		//! Loads the data from the StelOBJ into the StelOpenGLArray
		bool loadGL();

		void performScaling(double scale);

		//! The BBox of the original model before any transformations
		AABBox bbox;
		//! Contains the original positions in model space in km, they need scaling and projection
		QVector<Vec3f> posArray;
		//! True when the positions need to be rescaled before drawing
		bool needsRescale;
		//! Contains the scaled positions (sphere scale in AU), need StelProjector transformation for display
		QVector<Vec3f> scaledArray;
		//! Used to store the projected array data, avoids re-allocation each frame
		QVector<Vec3f> projectedPosArray;
		//! An OpenGL buffer for the projected positions
		QOpenGLBuffer* projPosBuffer;
		//! The single texture to use
		StelTextureSP texture;
		//! The original StelOBJ data, deleted after loading to GL
		StelOBJ* obj;
		//! The opengl array, created by loadObjModel() but filled later in main thread
		StelOpenGLArray* arr;
	};

	static StelTextureSP texEarthShadow;     // for lunar eclipses

	void computeModelMatrix(Mat4d &result) const;

	//! Update the orbit position values.
	void computeOrbit();

	Vec3f getCurrentOrbitColor() const;
	
	// Return the information string "ready to print" :)
	QString getPlanetLabel() const;

	// Draw the 3d model. Call the proper functions if there are rings etc..
	void draw3dModel(StelCore* core, StelProjector::ModelViewTranformP transfo, float screenSz, bool drawOnlyRing=false);

	// Draws the OBJ model, assuming it is available
	// @return false if the model can currently not be drawn (not loaded)
	bool drawObjModel(StelPainter* painter, float screenSz);

	bool drawObjShadowMap(StelPainter* painter, QMatrix4x4 &shadowMatrix);

	//! Starts the OBJ loading process, if it has not been done yet.
	//! Returns true when the OBJ is ready to draw
	bool ensureObjLoaded();

	// Draw the 3D sphere
	void drawSphere(StelPainter* painter, float screenSz, bool drawOnlyRing=false);

	// Draw the Hips survey.
	void drawSurvey(StelCore* core, StelPainter* painter);

	// Draw the circle and name of the Planet
	void drawHints(const StelCore* core, const QFont& planetNameFont);
    
	PlanetOBJModel* loadObjModel() const;

	QString englishName;             // english planet name
	QString nameI18;                 // International translated name
	QString nativeName;              // Can be used in a skyculture
	QString nativeNameMeaning;       // Can be used in a skyculture
	QString nativeNameMeaningI18n;   // Can be used in a skyculture
	QString texMapName;              // Texture file path
	QString normalMapName;           // Texture file path
	RotationElements re;             // Rotation and axis orientation parameters
	double siderealPeriod;           // sidereal period (Planet year or a moon's sidereal month) [earth days]
	double equatorialRadius;         // Planet's equatorial radius in AU
	double oneMinusOblateness;       // OneMinusOblateness=(polar radius)/(equatorial radius). Geometric flattening f=1-oneMinusOblateness (ExplanSup2013 10.1)
	Vec3d eclipticPos;               // Position in AU in the rectangular ecliptic coordinate system (J2000) centered on the parent body.
					 // To get heliocentric coordinates, use getHeliocentricEclipticPos()
	Vec3d eclipticVelocity;          // Speed in AU/d in the rectangular ecliptic coordinate system (J2000) around the parent body.
					 // NEW FEATURE in late 2017. For now, this may be 0/0/0 when we are not yet able to compute it.
					 // to get velocity, preferrably read getEclipticVelocity() and getHeliocentricEclipticVelocity()
					 // The "State Vector" [Heafner 1999] can be formed from (JDE, eclipticPos, eclipticVelocity)
	Vec3d screenPos;                 // Used to store temporarily the 2D position on screen. We need double for moons. Observe Styx from Pluto w/o atmosphere to see that.
	Vec3f haloColor;                 // used for drawing the planet halo. Also, when non-spherical (OBJ) model without texture is used, its color is derived from haloColour*albedo.

	float absoluteMagnitude;         // since 2017 this moved to the Planet class: V(1,0) from Explanatory Supplement or WGCCRE2009 paper for the planets, H in the H,G magnitude system for Minor planets, H10 for comets.
					 // This is the apparent visual magnitude when 1AU from sun and observer, with zero phase angle.
	float albedo;                    // Planet albedo. Used for magnitude computation when no other formula in use. Also, when non-spherical (OBJ) model without texture is used, its color is derived from haloColour*albedo.
	float roughness;                 // Oren-Nayar roughness for Moon and OBJ-based models
	float outgas_intensity;          // The intensity of a pseudo-outgas effect, based on an inverse exponential Lambert shading, with the light at the viewing position
					 // Non-null only for Comets, but we use one shader for all Planets and derivatives, so we need a placeholder here.
	float outgas_falloff;            // Exponent for falloff of outgas effect, should probably be < 1
					 // Non-null only for Comets, but we use one shader for all Planets and derivatives, so we need a placeholder here.
	Mat4d rotLocalToParent;          // retro-documented:
					 // rotation matrix of axis orientation with respect to the rotation axes of the parent body.
					 // For planets, this is the axis orientation w.r.t. VSOP87A/J2000 ecliptical system.
					 // For planets' satellites, this used to be a rotation into the planet's equatorial system.
					 // 0.21+: if rot_pole... data available in ssystem_*.ini (and therefore re.method==WGCCRE), this is not the rotation from planet axes over ICRF to the VSOP frame on which Stellarium is defined.
					 //
	float axisRotation;              // Rotation angle of the Planet on its axis, degrees.
					 // For Earth, this should be Greenwich Mean Sidereal Time GMST.
					 // For V0.21+, and for planets computed after the IAU2009/WGCCRE papers this is angle W (rotDeg),
					 // i.e. angle between ascending node of body equator w.r.t. ICRF equator and its prime meridian.
	StelTextureSP texMap;            // Planet map texture
	StelTextureSP normalMap;         // Planet normal map texture

	PlanetOBJModel* objModel;               // Planet model (when it has been loaded)
	QFuture<PlanetOBJModel*>* objModelLoader;// For async loading of the OBJ file
	QString objModelPath;

	HipsSurveyP survey;

	Ring* rings;                     // Planet rings
	double distance;                 // Temporary variable used to store the distance to a given point
					 // it is used for sorting while drawing
	double sphereScale;              // Artificial scaling for better viewing.
	double lastJDE;                  // caches JDE of last positional computation

	posFuncType coordFunc;		// callback for the calculation of the equatorial rectangular heliocentric position at time JDE.
	Orbit* orbitPtr;		// Usually a KeplerOrbit for positional computations of Minor Planets, Comets and Moons.
					// For an "observer", it is GimbalOrbit.
					// For the major planets, it is Q_NULLPTR.

	OsculatingFunctType *const osculatingFunc;
	QSharedPointer<Planet> parent;           // Planet parent i.e. sun for earth
	QList<QSharedPointer<Planet> > satellites;      // satellites of the Planet
	LinearFader hintFader;
	LinearFader labelsFader;         // Store the current state of the label for this planet
	bool flagLabels;                 // Define whether labels should be displayed
	bool hidden;                     // useful for fake planets used as observation positions - not drawn or labeled
	bool atmosphere;                 // Does the planet have an atmosphere?
	bool halo;                       // Does the planet have a halo?
	PlanetType pType;                // Type of body
	bool multisamplingEnabled_;

	static ApparentMagnitudeAlgorithm vMagAlgorithm;

	QOpenGLFunctions* gl;

	static Vec3f labelColor;
	static StelTextureSP hintCircleTex;
	static const QMap<PlanetType, QString> pTypeMap; // Maps fast type to english name.
	static const QMap<ApparentMagnitudeAlgorithm, QString> vMagAlgorithmMap;
	static bool drawMoonHalo;
	static bool drawSunHalo;
	//! If true, planet orbits will be drawn even if planet is off screen.
	static bool permanentDrawingOrbits;
	static int orbitsThickness;

private:
	class StelPropertyMgr* propMgr;
	QString iauMoonNumber;

	const QString getContextString() const;
	QPair<double, double> getLunarEclipseMagnitudes() const;

	// Shader-related variables
	struct PlanetShaderVars {
		// Vertex attributes
		int texCoord;
		int unprojectedVertex;
		int vertex;
		int normalIn;

		// Common uniforms
		int projectionMatrix;
		int tex;
		int lightDirection;
		int eyeDirection;
		int diffuseLight;
		int ambientLight;
		int shadowCount;
		int shadowData;
		int sunInfo;
		int skyBrightness;
		int orenNayarParameters;
		int outgasParameters;

		// Moon-specific variables
		int earthShadow;
		int eclipsePush; // apparent brightness push for partial Lunar Eclipse (make bright rim overbright)
		int normalMap;

		// Rings-specific variables
		int isRing;
		int ring;
		int outerRadius;
		int innerRadius;
		int ringS;

		// Shadowmap variables
		int shadowMatrix;
		int shadowTex;
		int poissonDisk;
		
		void initLocations(QOpenGLShaderProgram*);
	};

	//! Encapsulates some calculated information used for rendering
	struct RenderData
	{
		Mat4d modelMatrix;
		Mat4d mTarget;
		QVector<const Planet*> shadowCandidates;
		QMatrix4x4 shadowCandidatesData;
		Vec3d eyePos;
	};

	//! Calculates and uploads the common shader uniforms (projection matrix, texture, lighting&shadow data)
	RenderData setCommonShaderUniforms(const StelPainter &painter, QOpenGLShaderProgram* shader, const PlanetShaderVars& shaderVars) const;

	static PlanetShaderVars planetShaderVars;
	static QOpenGLShaderProgram* planetShaderProgram;

	static PlanetShaderVars ringPlanetShaderVars;
	static QOpenGLShaderProgram* ringPlanetShaderProgram;
	
	static PlanetShaderVars moonShaderVars;
	static QOpenGLShaderProgram* moonShaderProgram;

	static PlanetShaderVars objShaderVars;
	static QOpenGLShaderProgram* objShaderProgram;

	static PlanetShaderVars objShadowShaderVars;
	static QOpenGLShaderProgram* objShadowShaderProgram;

	static PlanetShaderVars transformShaderVars;
	static QOpenGLShaderProgram* transformShaderProgram;

	static bool shaderError;		// True if loading shaders caused errors

	static bool shadowInitialized;
	static Vec2f shadowPolyOffset;
#ifdef DEBUG_SHADOWMAP
	static QOpenGLFramebufferObject* shadowFBO;
#else
	static unsigned int shadowFBO;
#endif
	static unsigned int shadowTex;


	static bool initShader();
	static void deinitShader();
	static bool initFBO();
	static void deinitFBO();

	static QOpenGLShaderProgram* createShader(const QString& name,
						  PlanetShaderVars& vars,
						  const QByteArray& vSrc,
						  const QByteArray& fSrc,
						  const QByteArray& prefix=QByteArray(),
						  const QMap<QByteArray,int>& fixedAttributeLocations=QMap<QByteArray,int>());

	// Cache of positions in the parent ecliptic coordinates in AU.
	mutable QCache<double, Vec3d> positionsCache;
};

#endif // PLANET_HPP
