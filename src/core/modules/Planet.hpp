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
#include "VecMath.hpp"
#include "GeomMath.hpp"
#include "StelFader.hpp"
#include "StelTextureTypes.hpp"
#include "StelProjectorType.hpp"

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

// Class used to store rotational elements, i.e. axis orientation for the planetary body.
class RotationElements
{
public:
	RotationElements(void) : period(1.), offset(0.), epoch(J2000), obliquity(0.), ascendingNode(0.), precessionRate(0.), siderealPeriod(0.) {}
	float period;			// (sidereal) rotation period [earth days]
	float offset;			// rotation at epoch  [degrees]
	double epoch;			// JDE (JD TT) of epoch for these elements
	float obliquity;			// tilt of rotation axis w.r.t. ecliptic [radians]
	float ascendingNode;		// long. of ascending node of equator on the ecliptic [radians]
	float precessionRate;		// rate of precession of rotation axis in [rads/JulianCentury(36525d)]
	double siderealPeriod;		// sidereal period (Planet year or a moon's sidereal month) [earth days]
};

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
		UndefinedAlgorithm,
		Generic                     // Visual magnitude based on phase angle and albedo. The formula source for this is totally unknown!
	};


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
	//! Get the value (1-f) for oblateness f.
	double getOneMinusOblateness(void) const {return oneMinusOblateness;}
	//! Get the polar radius of the planet in AU.
	//! @return the polar radius of the planet in astronomical units.
	double getPolarRadius(void) const {return equatorialRadius*oneMinusOblateness;}
	//! Get duration of sidereal day
	double getSiderealDay(void) const {return static_cast<double>(re.period);}
	//! Get duration of sidereal year
	// must be virtual for Comets.
	virtual double getSiderealPeriod(void) const { return re.siderealPeriod; }
	//! Get duration of mean solar day, in earth days.
	double getMeanSolarDay(void) const;
	//! Get albedo
	double getAlbedo(void) const { return static_cast<double>(albedo); }

	const QString& getTextMapName() const {return texMapName;}
	const QString getPlanetTypeString() const {return pTypeMap.value(pType);}
	PlanetType getPlanetType() const {return pType;}
	Orbit* getOrbit() const {return orbitPtr;}

	void setNativeName(QString planet) { nativeName = planet; }

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

	//! Compute the z rotation to use from equatorial to geographic coordinates. For general applicability we need both time flavours:
	//! @param JD is JD(UT) for Earth
	//! @param JDE is used for other locations
	double getSiderealTime(double JD, double JDE) const;
	Mat4d getRotEquatorialToVsop87(void) const;
	void setRotEquatorialToVsop87(const Mat4d &m);

	const RotationElements &getRotationElements(void) const {return re;}
	// Set the orbital elements
	void setRotationElements(float _period, float _offset, double _epoch,
				 float _obliquity, float _ascendingNode,
				 float _precessionRate, double _siderealPeriod);
	double getRotAscendingNode(void) const {return static_cast<double>(re.ascendingNode);}
	// return angle between axis and normal of ecliptic plane (or, for a moon, equatorial/reference plane defined by parent).
	// For Earth, this is the angle between axis and normal to current ecliptic of date, i.e. the ecliptic obliquity of date JDE.
	// TODO: decide if this is always angle between axis and J2000 ecliptic, or should be axis//current ecliptic!
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

	bool flagTranslatedName;
	void setFlagTranslatedName(bool b) { flagTranslatedName = b; }
	bool getFlagTranslatedName(void) const { return flagTranslatedName; }

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
	QString getSkyLabel(const StelCore* core) const;

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
	QString texMapName;              // Texture file path
	QString normalMapName;           // Texture file path
	//int flagLighting;              // Set whether light computation has to be proceed. NO LONGER USED (always on!)
	RotationElements re;             // Rotation param
	double equatorialRadius;         // Planet's equatorial radius in AU
	double oneMinusOblateness;       // (polar radius)/(equatorial radius)
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
	Mat4d rotLocalToParent;          // GZ2015: undocumented.
					 // Apparently this is the axis orientation with respect to the parent body.
					 // For planets, this is axis orientation w.r.t. VSOP87A/J2000 ecliptical system.
	float axisRotation;              // Rotation angle of the Planet on its axis, degrees.
					 // For Earth, this should be Greenwich Mean Sidereal Time GMST.
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
	// The callback for the calculation of the equatorial rect heliocentric position at time JDE.
	posFuncType coordFunc;
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

	static ApparentMagnitudeAlgorithm vMagAlgorithm;

	QOpenGLFunctions* gl;

	static bool shaderError;		// True if loading shaders caused errors

	static Vec3f labelColor;
	static StelTextureSP hintCircleTex;
	static const QMap<PlanetType, QString> pTypeMap; // Maps fast type to english name.
	static const QMap<ApparentMagnitudeAlgorithm, QString> vMagAlgorithmMap;
	static bool drawMoonHalo;
	//! If true, planet orbits will be drawn even if planet is off screen.
	static bool permanentDrawingOrbits;

	static bool flagCustomGrsSettings;	// Is enabled usage of custom settings for calculation of position of Great Red Spot?
	static double customGrsJD;		// Initial JD for calculation of position of Great Red Spot
	static int customGrsLongitude;		// Longitude of Great Red Spot (System II, degrees)
	static double customGrsDrift;		// Annual drift of Great Red Spot position (degrees)

	static int orbitsThickness;

private:
	QString iauMoonNumber;

	const QString getContextString() const;

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

