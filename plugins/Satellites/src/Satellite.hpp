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

#ifndef SATELLITE_HPP
#define SATELLITE_HPP

#include <QDateTime>
#include <QFont>
#include <QList>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelSphereGeometry.hpp"
#include "StelTranslator.hpp"
#include "gSatWrapper.hpp"
#include "SolarSystem.hpp"

class StelPainter;
class StelLocation;

//! Radio communication channel properties.
//! @ingroup satellites
typedef struct
{
	double frequency;	//!< Channel frequency in MHz.
	QString modulation;	//!< Signal modulation mode.
	QString description;	//!< Callsign with channel description.
} CommLink;

//! Description of the data roles used in SatellitesListModel.
//! @ingroup satellites
enum SatelliteDataRole {
	SatIdRole = Qt::UserRole,
	SatCosparIDRole,
	SatDescriptionRole,
	SatStdMagnitudeRole,
	SatRCSRole,
	SatPerigeeRole,
	SatApogeeRole,
	SatPeriodRole,
	SatTLEEpochRole,
	SatFlagsRole,
	SatGroupsRole,
	FirstLineRole,
	SecondLineRole
};

//! Type for sets of satellite group IDs.
//! @ingroup satellites
typedef QSet<QString> GroupSet;

//! Flag type reflecting internal flags of Satellite.
//! @ingroup satellites
enum SatFlag
{
	SatNoFlags				= 0x00000000,
	SatDisplayed				= 0x00000001,
	SatNotDisplayed			= 0x00000002,
	SatUser					= 0x00000004,
	SatOrbit					= 0x00000008,
	SatNew					= 0x00000010,
	SatError					= 0x00000020,
	SatSmallSize				= 0x00000040,
	SatMediumSize			= 0x00000080,
	SatLargeSize				= 0x00000100,
	SatLEO					= 0x00000200,
	SatMEO					= 0x00000400,
	SatGSO					= 0x00000800,
	SatHEO					= 0x00001000,
	SatHGSO					= 0x00002000,
	SatPolarOrbit				= 0x00004000,
	SatEquatOrbit				= 0x00008000,
	SatPSSO					= 0x00010000,
	SatHEarthO				= 0x00020000,
	SatOutdatedTLE			= 0x00040000,
	SatCustomFilter			= 0x00080000,
	SatCommunication			= 0x00100000,
	SatReentry				= 0x00200000,
	SatActiveOS				= 0x00400000,
	SatOperationalOS			= 0x00800000,
	SatNonoperationalOS		= 0x01000000,
	SatPartiallyOperationalOS	= 0x02000000,
	SatStandbyOS				= 0x04000000,
	SatSpareOS				= 0x08000000,
	SatExtendedMissionOS		= 0x10000000,
	SatDecayedOS			= 0x20000000
};
typedef QFlags<SatFlag> SatFlags;
Q_DECLARE_OPERATORS_FOR_FLAGS(SatFlags)

// Allows the type to be used by QVariant
Q_DECLARE_METATYPE(GroupSet)
Q_DECLARE_METATYPE(SatFlags)

//! @class Satellite
//! A representation of a satellite in Earth orbit.
//! Details about the satellite are passed with a JSON-representation structure
//! that contains a <b>Satellite Catalog</b> entry.
//! 
//! Thanks to operator<() overloading, container classes (QList, QMap, etc.)
//! with Satellite or SatelliteP objects can be sorted by satellite name/ID.
//! @ingroup satellites
class Satellite : public StelObject
{
	friend class Satellites;
	friend class SatellitesDialog;
	friend class SatellitesListModel;

public:
	static const QString SATELLITE_TYPE;

	//! @enum OptStatus operational statuses
	enum OptStatus
	{
		StatusOperational			= 1,
		StatusNonoperational		= 2,
		StatusPartiallyOperational	= 3,
		StatusStandby			= 4,
		StatusSpare				= 5,
		StatusExtendedMission		= 6,
		StatusDecayed			= 7,
		StatusUnknown			= 0
	};
	Q_ENUM(OptStatus)

	//! \param identifier unique identifier (currently the Catalog Number)
	//! \param data a QMap which contains the details of the satellite
	//! (TLE set, description etc.)
	Satellite(const QString& identifier, const QVariantMap& data);
	~Satellite() override;

	//! Get a QVariantMap which describes the satellite.  Could be used to
	//! create a duplicate.
	QVariantMap getMap(void);

	QString getType(void) const override
	{
		return SATELLITE_TYPE;
	}

	QString getObjectType(void) const override
	{
		return N_("artificial satellite");
	}
	QString getObjectTypeI18n(void) const override
	{
		return q_(getObjectType());
	}

	QString getID(void) const override
	{
		return id;
	}

	float getSelectPriority(const StelCore* core) const override;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @param flags a set of flags with information types to include.
	//! Supported types for Satellite objects:
	//! - Name: designation in large type with the description underneath
	//! - RaDecJ2000, RaDecOfDate, HourAngle, AltAzi
	//! - Extra: range, range rate and altitude of satellite above the Earth, comms frequencies, modulation types and so on.
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const override;
	//! Return a map like StelObject::getInfoMap(), but with a few extra tags also available in getInfoString().
	//! - description
	//! - catalog
	//! - international-designator
	//! - type
	//! - range (distance in km)
	//! - rangerate (distance change in km/s)
	//! - height (height in km)
	//! - subpoint-lat (latitude of subpoint, decimal degrees)
	//! - subpoint-long (longitude of subpoint, decimal degrees)
	//! - inclination (decimal degrees)
	//! - period (minutes)
	//! - perigee-altitude (height in km)
	//! - apogee-altitude (height in km)
	//! - TEME-km-X
	//! - TEME-km-Y
	//! - TEME-km-Z
	//! - TEME-speed-X
	//! - TEME-speed-Y
	//! - TEME-speed-Z
	//! - sun-reflection-angle (if available)
	//! - operational-status
	//! - visibility (descriptive string)
	//! - comm (Radio information, optional, if available. There may be several comm entries!)
	QVariantMap getInfoMap(const StelCore *core) const override;
	Vec3f getInfoColor(void) const override;
	Vec3d getJ2000EquatorialPos(const StelCore*) const override;
	float getVMagnitude(const StelCore* core) const override;
	//! Get angular half-size, degrees
	double getAngularRadius(const StelCore*) const override;
	QString getNameI18n(void) const override;
	QString getEnglishName(void) const override
	{
		return name;
	}
	//! Returns the (NORAD) catalog number. (For now, the ID string.)
	QString getCatalogNumberString() const {return id;}
	//! Returns the (COSPAR) International Designator.
	QString getInternationalDesignator() const {return internationalDesignator;}

	//! Set new tleElements.  This assumes the designation is already set, populates
	//! the tleElements values and configures internal orbit parameters.
	void setNewTleElements(const QString& tle1, const QString& tle2);

	// calculate faders, new position
	void update(double deltaTime);

	double getDoppler(double freq) const;
	static bool showLabels;
	static double roundToDp(float n, int dp);

	// when the observer location changes we need to
	void recalculateOrbitLines(void);
	
	void setNew() {newlyAdded = true;}
	bool isNew() const {return newlyAdded;}

	void setCommData(QList<CommLink> comm) { comms = comm; }
	
	//! Get internal flags as a single value.
	SatFlags getFlags() const;
	//! Sets the internal flags in one operation (only display flags)!
	void setFlags(const SatFlags& flags);
	
	//! Needed for sorting lists (if this ever happens...).
	//! Compares #name fields. If equal, #id fields, which can't be.
	bool operator<(const Satellite& another) const;

	//! Calculation of illuminated fraction of the satellite.
	float calculateIlluminatedFraction() const;

	//! Get radii and geocentric antisolar distances of shadow circles in km from the geocenter.
	//! @return Vec4d(umbraDistance, umbraRadius, penumbraDistance, penumbraRadius);
	//! umbraDistance    : geocentric distance (km) of the umbra circle centre in the antisolar direction
	//! umbraRadius      : radius (km) of the umbra circle
	//! penumbraDistance : geocentric distance (km) of the penumbra circle in the antisolar direction
	//! penumbraRadius   : radius (km) of the penumbra circle
	Vec4d getUmbraData();

	//! Get radii and geocentric antisolar distances of shadow circles in km for a hypothetical object in distance rhoE [km] from the geocenter.
	//! @param rhoE distance [km] from the geocenter.
	//! @return Vec4d(umbraDistance, umbraRadius, penumbraDistance, penumbraRadius);
	//! umbraDistance    : geocentric distance (km) of the umbra circle centre in the antisolar direction
	//! umbraRadius      : radius (km) of the umbra circle
	//! penumbraDistance : geocentric distance (km) of the penumbra circle in the antisolar direction
	//! penumbraRadius   : radius (km) of the penumbra circle
	//! @note To gain data for an object in dist_km above (spherical) earth, use
	//!    rhoE=earth->getEquatorialRadius()*AU+dist_km;
	static Vec4d getUmbraData(const double rhoE);

	//! Get operational status of satellite
	QString getOperationalStatus() const;

	void recomputeSatData();

private:
	//draw orbits methods
	void computeOrbitPoints();
	void drawOrbit(StelCore* core, StelPainter& painter);
	//! returns 0 - 1.0 for the DRAWORBIT_FADE_NUMBER segments at
	//! each end of an orbit, with 1 in the middle.
	float calculateOrbitSegmentIntensity(int segNum);
	void calculateSatDataFromLine2(QString tle);
	//! Parse TLE line to extract International Designator and launch year.
	//! Sets #internationalDesignator and #jdLaunchYearJan1.
	void parseInternationalDesignator(const QString& tle1);
	void calculateEpochFromLine1(QString tle);

	bool getCustomFiltersFlag() const;
	QString getCommLinkInfo(CommLink comm) const;

	bool initialized;
	//! Flag indicating whether the satellite should be displayed.
	//! Should not be confused with the pedicted visibility of the 
	//! actual satellite to the observer.
	bool displayed;
	//! Flag indicating whether an orbit section should be displayed.
	bool orbitDisplayed;  // draw orbit enabled/disabled
	//! Flag indicating that the satellite is user-defined.
	//! This means that its TLE set shouldn't be updated and the satellite
	//! itself shouldn't be removed if auto-remove is enabled.
	bool userDefined;
	//! Flag indicating that the satellite was added during the current session.
	bool newlyAdded;
	bool orbitValid;

	//! Identifier of the satellite, must be unique within the list.
	//! Currently, the Satellite Catalog Number/NORAD Number is used,
	//! as it is unique and it is contained in both lines of TLE sets.
	QString id;
	//! Human-readable name of the satellite.
	//! Usually the string in the "Title line" of TLE sets.
	QString name;
	//! Longer description of the satellite.
	QString description;
	//! International Designator / COSPAR designation / NSSDC ID.
	QString internationalDesignator;
	//! Epoch of the TLE
	QString tleEpoch;
	//! Epoch of the TLE (JD)
	double tleEpochJD;
	//! Julian date of Jan 1st of the launch year.
	//! Used to hide satellites before their launch date.
	//! Extracted from TLE set with parseInternationalDesignator().
	//! It defaults to 1 Jan 1957 if extraction fails.
	double jdLaunchYearJan1;
	//! Standard visual magnitude of the satellite.
	double stdMag;
	//! Radar cross-section value of the satellite (in meters squared).
	double RCS;
	double perigee;
	double apogee;
	double inclination;
	double eccentricity;
	//! Operational status code
	int status;
	//! Contains the J2000 position.
	Vec3d XYZ;
	QPair< QString, QString> tleElements;
	double height, range, rangeRate;
	QList<CommLink> comms;
	Vec3f hintColor;	
	//! Identifiers of the groups to which the satellite belongs.
	//! See @ref groups.
	GroupSet groups;
	QDateTime lastUpdated;

	bool isISS;
	bool isStarlink;
	PlanetP moon;
	PlanetP sun;

	static StelTextureSP hintTexture;
	static SphericalCap  viewportHalfspace;
	static float hintBrightness;
	static float hintScale;
	static int   orbitLineSegments;
	static int   orbitLineFadeSegments;
	static int   orbitLineSegmentDuration; //measured in seconds
	static int   orbitLineThickness;
	static bool  orbitLinesFlag;
	static bool  iconicModeFlag;
	static bool  hideInvisibleSatellitesFlag;
	static bool  coloredInvisibleSatellitesFlag;
	//! Mask controlling which info display flags should be honoured.
	static StelObject::InfoStringGroupFlags flagsMask;
	static Vec3f invisibleSatelliteColor;
	static Vec3f transitSatelliteColor;

	static double timeRateLimit;
	static int tleEpochAge;

	static bool flagCFKnownStdMagnitude;
	static bool flagCFApogee;
	static double minCFApogee;
	static double maxCFApogee;
	static bool flagCFPerigee;
	static double minCFPerigee;
	static double maxCFPerigee;
	static bool flagCFEccentricity;
	static double minCFEccentricity;
	static double maxCFEccentricity;
	static bool flagCFPeriod;
	static double minCFPeriod;
	static double maxCFPeriod;
	static bool flagCFInclination;
	static double minCFInclination;
	static double maxCFInclination;
	static bool flagCFRCS;
	static double minCFRCS;
	static double maxCFRCS;
	static bool flagVFAltitude;
	static double minVFAltitude;
	static double maxVFAltitude;
	static bool flagVFMagnitude;
	static double minVFMagnitude;
	static double maxVFMagnitude;

	void draw(StelCore *core, StelPainter& painter);

	//Satellite Orbit Position calculation
	gSatWrapper *pSatWrapper;
	Vec3d	position; //< holds TEME coordinates [km]
	Vec3d	velocity; //< holds TEME velocity [km/s]
	Vec3d	latLongSubPointPosition; //< holds WGS84 satellite coordinates: long[degrees]/lat[degrees]/altitude[km]_over_wgs84
	Vec3d	elAzPosition;

	gSatWrapper::Visibility	visibility;
	double	phaseAngle; // phase angle for the satellite
#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	static double sunReflAngle; // for Iridium satellites
	//static double timeShift; // for Iridium satellites UNUSED
#endif
	Vec3f    infoColor;
	//Satellite Orbit Draw
	Vec3f    orbitColor;
	double    lastEpochCompForOrbit; //measured in Julian Days
	double    epochTime;  //measured in Julian Days
	QList<Vec4d> orbitPoints; //orbit points represented by ElAzPos vectors and altitudes
	QList<gSatWrapper::Visibility> visibilityPoints; //orbit visibility points
	QMap<gSatWrapper::Visibility, QString> visibilityDescription;
};

typedef QSharedPointer<Satellite> SatelliteP;
bool operator<(const SatelliteP& left, const SatelliteP& right);

#endif // SATELLITE_HPP

