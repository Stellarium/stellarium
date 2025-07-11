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

#ifndef STELOBJECT_HPP
#define STELOBJECT_HPP

#include "VecMath.hpp"
// Ignore clangd warning! Inclusion is OK.
#include "StelObjectType.hpp"
#include "StelRegionObject.hpp"

#include <QFlags>
#include <QString>

class StelCore;

//! The base abstract class for sky objects used in Stellarium like Stars, Planets, Constellations etc...
//! Normally you should use StelObjectP instead of StelObject* which have by default the same behaviour,
//! but which can be added reference counting if needed.
//! @sa StelObjectP
class StelObject : public StelRegionObject
{
	//Required for Q_FLAG macro, this requires this header to be MOC'ed
	Q_GADGET
public:
	//! Used as named bitfield flags as specifiers to
	//! filter results of getInfoString. The precise definition of these should
	//! be documented in the getInfoString documentation for the derived classes
	//! for all specifiers which are defined in that derivative.
	//! Use InfoStringGroup instead.
	enum InfoStringGroupFlags
	{
		None			= 0x00000000, //!< Show Nothing
		Name			= 0x00000001, //!< An object's name as further defined by CulturalDisplayStyle found in SkyCultureMgr.
		CatalogNumber		= 0x00000002, //!< Catalog numbers
		Magnitude		= 0x00000004, //!< Magnitude related data
		RaDecJ2000		= 0x00000008, //!< The equatorial position (J2000 ref)
		RaDecOfDate		= 0x00000010, //!< The equatorial position (of date)
		AltAzi			= 0x00000020, //!< The position (Altitude/Azimuth)
		Distance		= 0x00000040, //!< Info about an object's distance
		Elongation		= 0x00000080, //!< Info about elongation, phase angle etc. Most useful for Planets, but possible for all objects.
		Size			= 0x00000100, //!< Info about an object's size
		Velocity		= 0x00000200, //!< Info about object's velocity
		ProperMotion		= 0x00000400, //!< Annual proper motion (for stars) or hourly motion (for Planets)
		Extra			= 0x00000800, //!< Derived class-specific extra fields
		HourAngle		= 0x00001000, //!< The hour angle + DE (of date)
		AbsoluteMagnitude	= 0x00002000, //!< The absolute magnitude
		GalacticCoord		= 0x00004000, //!< The galactic position
		SupergalacticCoord	= 0x00008000, //!< The supergalactic position
		OtherCoord		= 0x00010000, //!< Unspecified additional coordinates. These can be "injected" into the extraInfoStrings by plugins.
		ObjectType		= 0x00020000, //!< The type of the object (star, planet, etc.)
		EclipticCoordJ2000	= 0x00040000, //!< The ecliptic position (J2000.0 ref) [+ XYZ of VSOP87A (used mainly for debugging, not public)]
		EclipticCoordOfDate	= 0x00080000, //!< The ecliptic position (of date)
		IAUConstellation        = 0x00100000, //!< Three-letter constellation code (And, Boo, Cas, ...), and Zodiacal sign and Lunar station/mansion where defined
		SiderealTime		= 0x00200000, //!< Mean and Apparent Sidereal Time
		RTSTime			= 0x00400000, //!< Time of rise, transit and set of celestial object
		SolarLunarPosition      = 0x00800000, //!< Show Solar and Lunar horizontal position (on Earth location only)
		Script                  = 0x01000000, //!< Should be used by Scripts only which can inject extraInfoStrings.
		DebugAid                = 0x02000000, //!< Can be used for development only, place messages into extraInfoStrings. Comment them away or delete for releases.
		NoFont			= 0x04000000,
		PlainText		= 0x08000000  //!< Strip HTML tags from output
	};
	Q_DECLARE_FLAGS(InfoStringGroup, InfoStringGroupFlags)
	Q_FLAG(InfoStringGroup)


	//! A 6-bit code with all options for displaying relevant CulturalName parts.
	//! Describes how to display culture aware labels for constellation, planets, star names, ....
	//! The viewDialog GUI has checkboxes which corresponds to these values.
	//! It is necessary to have different settings for screen labels (usually shorter) and InfoString labels (may be set to more complete)

	//! This setting is handled by methods getScreenLabel() and getInfoLabel() in StelObject and descendants.

	//! The names are explicitly long and descriptive, usable in config.ini.
	//! Example: Native_Pronounce_Translit_Translated_IPA_Modern shows everything:
	//! - native name,
	//! - simple translatable pronunciation
	//! - non-translatable scientific transliteration (in rare cases only)
	//! - translated meaning
	//! - IPA reading/pronunciation aid in International Phonetic Alphabet
	//! - Modern name (useful for stars and planets only). Helpful to see the modern name in context.
	enum class CulturalDisplayStyle
	{
		NONE                                            = 0x00,
		Modern                                          = 0x01,
		IPA                                             = 0x02,
		IPA_Modern                                      = 0x03,
		Translated                                      = 0x04,
		Translated_Modern                               = 0x05,
		Translated_IPA                                  = 0x06,
		Translated_IPA_Modern                           = 0x07,
		Translit                                        = 0x08,
		Translit_Modern                                 = 0x09,
		Translit_IPA                                    = 0x0A,
		Translit_IPA_Modern                             = 0x0B,
		Translit_Translated                             = 0x0C,
		Translit_Translated_Modern                      = 0x0D,
		Translit_Translated_IPA                         = 0x0E,
		Translit_Translated_IPA_Modern                  = 0x0F,
		Pronounce                                       = 0x10,
		Pronounce_Modern                                = 0x11,
		Pronounce_IPA                                   = 0x12,
		Pronounce_IPA_Modern                            = 0x13,
		Pronounce_Translated                            = 0x14,
		Pronounce_Translated_Modern                     = 0x15,
		Pronounce_Translated_IPA                        = 0x16,
		Pronounce_Translated_IPA_Modern                 = 0x17,
		Pronounce_Translit                              = 0x18,
		Pronounce_Translit_Modern                       = 0x19,
		Pronounce_Translit_IPA                          = 0x1A,
		Pronounce_Translit_IPA_Modern                   = 0x1B,
		Pronounce_Translit_Translated                   = 0x1C,
		Pronounce_Translit_Translated_Modern            = 0x1D,
		Pronounce_Translit_Translated_IPA               = 0x1E,
		Pronounce_Translit_Translated_IPA_Modern        = 0x1F,
		Native                                          = 0x20,
		Native_Modern                                   = 0x21,
		Native_IPA                                      = 0x22,
		Native_IPA_Modern                               = 0x23,
		Native_Translated                               = 0x24,
		Native_Translated_Modern                        = 0x25,
		Native_Translated_IPA                           = 0x26,
		Native_Translated_IPA_Modern                    = 0x27,
		Native_Translit                                 = 0x28,
		Native_Translit_Modern                          = 0x29,
		Native_Translit_IPA                             = 0x2A,
		Native_Translit_IPA_Modern                      = 0x2B,
		Native_Translit_Translated                      = 0x2C,
		Native_Translit_Translated_Modern               = 0x2D,
		Native_Translit_Translated_IPA                  = 0x2E,
		Native_Translit_Translated_IPA_Modern           = 0x2F,
		Native_Pronounce                                = 0x30,
		Native_Pronounce_Modern                         = 0x31,
		Native_Pronounce_IPA                            = 0x32,
		Native_Pronounce_IPA_Modern                     = 0x33,
		Native_Pronounce_Translated                     = 0x34,
		Native_Pronounce_Translated_Modern              = 0x35,
		Native_Pronounce_Translated_IPA                 = 0x36,
		Native_Pronounce_Translated_IPA_Modern          = 0x37,
		Native_Pronounce_Translit                       = 0x38,
		Native_Pronounce_Translit_Modern                = 0x39,
		Native_Pronounce_Translit_IPA                   = 0x3A,
		Native_Pronounce_Translit_IPA_Modern            = 0x3B,
		Native_Pronounce_Translit_Translated            = 0x3C,
		Native_Pronounce_Translit_Translated_Modern     = 0x3D,
		Native_Pronounce_Translit_Translated_IPA        = 0x3E,
		Native_Pronounce_Translit_Translated_IPA_Modern = 0x3F
	};
	Q_ENUM(CulturalDisplayStyle)

	//! @struct CulturalName
	//! Contains name components belonging to an object.
	//!
	enum class CulturalNameSpecial
	{
		None = 0,        //!< Nothing special
		Morning = 1,     //!< This name is used for Mercury or Venus in Western elongation, i.e., "morning star"
		Evening = 2      //!< This name is used for Mercury or Venus in Eastern elongation, i.e., "evening star"
	};
	Q_ENUM(CulturalNameSpecial)
	struct CulturalName
	{
		QString native;           //!< native name in native glyphs
		QString pronounce;        //!< native name in a Latin-based transliteration usable as pronunciation aid for English
		QString pronounceI18n;    //!< native name in a transliteration scheme in user-language usable as pronunciation aid
		QString transliteration;  //!< native name in a science-based transliteration scheme not geared at pronunciation (e.g. Tibetan Wylie; rarely used).
		QString translated;       //!< Native name translated to English. NOT the same as the usual object's englishName!
		QString translatedI18n;   //!< Translated name (user language)
		QString IPA;              //!< native name expressed in International Phonetic Alphabet
		StelObject::CulturalNameSpecial special;          //!< any particular extra application?
		CulturalName(): special(StelObject::CulturalNameSpecial::None){};
		CulturalName(QString nat, QString pr, QString prI18n, QString trl,
			     QString tra, QString traI18n, QString ipa, StelObject::CulturalNameSpecial sp=StelObject::CulturalNameSpecial::None):
			native(nat), pronounce(pr), pronounceI18n(prI18n), transliteration(trl),
			translated(tra), translatedI18n(traI18n), IPA(ipa),
			special(sp){};
	};

	//! A pre-defined "all available" set of specifiers for the getInfoString flags argument to getInfoString
	static constexpr InfoStringGroup AllInfo = static_cast<InfoStringGroup>(Name|CatalogNumber|Magnitude|RaDecJ2000|RaDecOfDate|AltAzi|
									   Distance|Elongation|Size|Velocity|ProperMotion|Extra|HourAngle|AbsoluteMagnitude|
									   GalacticCoord|SupergalacticCoord|OtherCoord|ObjectType|EclipticCoordJ2000|
									   EclipticCoordOfDate|IAUConstellation|SiderealTime|RTSTime|SolarLunarPosition);
	//! A pre-defined "default" set of specifiers for the getInfoString flags argument to getInfoString
	//! It appears useful to propose this set as post-install settings and let users configure more on demand.
	static constexpr InfoStringGroup DefaultInfo = static_cast<InfoStringGroup>(Name|CatalogNumber|Magnitude|RaDecOfDate|HourAngle|AltAzi|OtherCoord|
											  Distance|Elongation|Size|Velocity|Extra|IAUConstellation|SiderealTime|RTSTime);
	//! A pre-defined "shortest useful" set of specifiers for the getInfoString flags argument to getInfoString
	static constexpr InfoStringGroup ShortInfo = static_cast<InfoStringGroup>(Name|CatalogNumber|Magnitude|RaDecJ2000);

	~StelObject() override {}

	//! Default implementation of the getRegion method.
	//! Return the spatial region of the object.
	SphericalRegionP getRegion() const override {return SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(Q_NULLPTR)));}

	//! Default implementation of the getPointInRegion method.
	//! Return the J2000 Equatorial Position of the object.
	Vec3d getPointInRegion() const override {return getJ2000EquatorialPos(Q_NULLPTR);}
	
	//! Write I18n information about the object in QString.
	//! @param core the StelCore object to use
	//! @param flags a set of InfoStringGroup flags which are used to
	//! filter the return value - including specified types of information
	//! and altering the output format.
	//! @return an HTML string containing information about the StelObject.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags=StelObject::AllInfo) const = 0;

	//! Return a key/value map with data about an object's position, magnitude and so on. Useful in a context like scripting.
	//! Derived objects can add their own special information tags.
	//! @param core the current StelCore
	//! @return a map of object data.  Keys:
	//! - above-horizon : true, if celestial body is above horizon
	//! - altitude : apparent altitude angle in decimal degrees
	//! - azimuth : apparent azimuth angle in decimal degrees
	//! - altitude-geometric : geometric altitude angle in decimal degrees
	//! - azimuth-geometric : geometric azimuth angle in decimal degrees
	//! - airmass : number of airmasses the object's light had to pass through the atmosphere. For negative altitudes this number may be meaningless.
	//! - ra : right ascension angle (current date frame) in decimal degrees
	//! - dec : declination angle (current date frame) in decimal degrees
	//! - raJ2000 : right ascension angle (J2000 frame) in decimal degrees
	//! - decJ2000 : declination angle (J2000 frame) in decimal degrees
	//! - parallacticAngle : parallactic angle in decimal degrees (for non-star objects only)
	//! - hourAngle-dd : hour angle in decimal degrees
	//! - hourAngle-hms : hour angle in HMS format (formatted string)
	//! - iauConstellation : 3-letter abbreviation of IAU constellation (string)
	//! - meanSidTm : mean sidereal time, as time string (on Earth only!)
	//! - meanSidTm-dd : mean sidereal time, in decimal degrees (on Earth only!)
	//! - appSidTm : mean sidereal time, as time string (on Earth only!)
	//! - appSidTm-dd : mean sidereal time, in decimal degrees (on Earth only!)
	//! - glong : galactic longitude in decimal degrees
	//! - glat : galactic latitude in decimal degrees
	//! - sglong : supergalactic longitude in decimal degrees
	//! - sglat : supergalactic latitude in decimal degrees
	//! - ecliptic-obliquity : mean ecliptic obliquity of date in decimal degrees
	//! - elong : ecliptic longitude in decimal degrees (on Earth only!)
	//! - elat : ecliptic latitude in decimal degrees (on Earth only!)
	//! - elongJ2000 : ecliptic longitude (Earth's J2000 frame) in decimal degrees
	//! - elatJ2000 : ecliptic latitude (Earth's J2000 frame) in decimal degrees
	//! - vmag : visual magnitude
	//! - vmage : visual magnitude (after atmospheric extinction)
	//! - size: angular size (diameter) in radians
	//! - size-dd : angular size (diameter) in decimal degrees
	//! - size-deg : angular size (diameter) in decimal degrees (formatted string)
	//! - size-dms : angular size (diameter) in DMS format
	//! - rise : time of rise in HM format
	//! - rise-dhr : time of rise in decimal hours
	//! - transit : time of transit in HM format
	//! - transit-dhr : time of transit in decimal hours
	//! - set : time of set in HM format
	//! - set-dhr : time of set in decimal hours
	//! - name : english name of the object
	//! - localized-name : localized name
	//! - type: type of object' class
	//! - object-type: English lowercase name of the type of the object
	//! @note Coordinate values may need modulo operation to bring them into ranges [0..360].
	virtual QVariantMap getInfoMap(const StelCore *core) const;

	//! Return object's type. It should be the name of the class.
	virtual QString getType() const = 0;

	//! Return object's type. It should be English lowercase name of the astronomical type of the object.
	//! The purpose of this string is a distinction or further refinement over object class name retrieved with getType():
	//! Planet objects can be planets, moons, or even the Sun. The Sun should however return "star".
	//! Nebula objects should return their actual type like "open cluster", "galaxy", "nebula", ...
	virtual QString getObjectType() const = 0;
	//! Return object's type. It should be translated lowercase name of the type of the object.
	virtual QString getObjectTypeI18n() const = 0;

	//! Returns a unique identifier for this object.
	//! The ID should be unique for all objects of the same type,
	//! but may freely conflict with IDs of other types, so getType() must also be tested.
	//!
	//! With this it should be possible to at least identify the same object
	//! in a different instance of Stellarium running the same version, but
	//! it would even be better if the ID provides some degree of forward-compatibility.
	//! For some object types (e.g. planets) this may simply return getEnglishName(),
	//! but better candidates may be official designations or at least (stable) internal IDs.
	//!
	//! An object may have multiple IDs (different catalog numbers, etc). StelObjectMgr::searchByID()
	//! should search through all ID variants, but this method only returns one of them.
	virtual QString getID() const = 0;

	//! Return object's name in english.
	//! For non-default skycultures, this is the english translation of the native name.
	virtual QString getEnglishName() const = 0;

	//! Return translated object's name
	//! For non-default skycultures, this is the user language translation of the english name (which should be native translated to english).
	virtual QString getNameI18n() const = 0;

	//! Return object's native name in the glyphs as written in skyculture descriptions (index.json).
	//! For non-default skycultures, this is as close to the original as possible.
	virtual QString getNameNative() const {return QString();}

	//! Return a Latin-letter based transliteration geared at english pronounciation of the native name.
	//! This is optional but essential for all skycultures in languages which use non-Latin glyphs.
	//! When user language is English, this is the string from index.json.
	//! TBD: When user language is different, this may appear adapted to user language.
	virtual QString getNamePronounce() const {return QString();}

	//! Return a secondary scientific transliteration of the native name.
	//! This is optional and in fact rarely used. An example would be Wylie-transliteration of Tibetan.
	virtual QString getNameTransliteration() const {return QString();}

	//! Return native name in International Phonetic Alphabet. Optional.
	virtual QString getNameIPA() const {return QString();}

	//! Return screen label (to be used in the sky display. Most users will use some short label)
	virtual QString getScreenLabel() const {return QString();}

	//! Return InfoString label (to be used in the InfoString).
	//! When dealing with foreign skycultures, many users will want this to be longer, with more name components.
	virtual QString getInfoLabel() const {return QString();}

	//! Get observer-centered equatorial coordinates at equinox J2000, including aberration
	virtual Vec3d getJ2000EquatorialPos(const StelCore* core) const = 0;

	//! Get observer-centered equatorial coordinate at the current equinox
	//! The frame has its Z axis at the planet's current rotation axis
	//! At time 2000-01-01 this frame is almost the same as J2000, but ONLY if the observer is on earth
	Vec3d getEquinoxEquatorialPos(const StelCore* core) const;
	//! Like getEquinoxEquatorialPos(core), but always adds refraction correction to the position.
	Vec3d getEquinoxEquatorialPosApparent(const StelCore* core) const;
	//! Like getEquinoxEquatorialPos(core), but adds refraction correction to the position if atmosphere is active.
	Vec3d getEquinoxEquatorialPosAuto(const StelCore* core) const;

	//! Get observer-centered galactic coordinates
	Vec3d getGalacticPos(const StelCore* core) const;

	//! Get observer-centered supergalactic coordinates
	Vec3d getSupergalacticPos(const StelCore* core) const;

	//! Get observer-centered hour angle + declination (at current equinox)
	//! It is the geometric position, i.e. without taking refraction effect into account.
	//! The frame has its Z axis at the planet's current rotation axis
	Vec3d getSiderealPosGeometric(const StelCore* core) const;

	//! Get observer-centered hour angle + declination (at current equinox)
	//! It is the apparent position, i.e. taking the refraction effect into account.
	//! The frame has its Z axis at the planet's current rotation axis
	Vec3d getSiderealPosApparent(const StelCore* core) const;

	//! Get observer-centered alt/az position
	//! It is the geometric position, i.e. without taking refraction effect into account.
	//! The frame has its Z axis at the zenith
	Vec3d getAltAzPosGeometric(const StelCore* core) const;

	//! Get observer-centered alt/az position
	//! It is the apparent position, i.e. taking the refraction effect into account.
	//! The frame has its Z axis at the zenith
	Vec3d getAltAzPosApparent(const StelCore* core) const;

	//! Get observer-centered alt/az position
	//! It is the automatic position, i.e. taking the refraction effect into account if atmosphere is on.
	//! The frame has its Z axis at the zenith
	Vec3d getAltAzPosAuto(const StelCore* core) const;

	//! Get parallactic angle, which is the deviation between zenith angle and north angle. [radians]
	float getParallacticAngle(const StelCore* core) const;

	//! Checking position an object above mathematical horizon for current location.
	//! @return true if object an above mathematical horizon
	bool isAboveHorizon(const StelCore* core) const;

	//! Checking position an object above real horizon for current location.
	//! @return true if object an above real horizon (uses test for landscapes)
	bool isAboveRealHorizon(const StelCore* core) const;

	//! Compute time of rise, transit and set for celestial object for current location.
	//! @param core the currently active StelCore object
	//! @param altitude (optional; default=0) altitude of the object, degrees.
	//!        Setting this to -6. for the Sun will find begin and end for civil twilight.
	//! @return Vec4d - time of rise, transit and set closest to current time; JD.
	//! @note The fourth element flags particular conditions:
	//!       *  +100. for circumpolar objects. Rise and set give lower culmination times.
	//!       *  -100. for objects never rising. Rise and set give transit times.
	//!       * -1000. is used as "invalid" value. The result should then not be used.
	//!       *   +20. (Planet objects only) no transit time on current date.
	//!       *   +30. (Planet objects only) no rise time on current date.
	//!       *   +40. (Planet objects only) no set time on current date.
	//! @note This is an abbreviated version of the method implemented in the Planet class.

	virtual Vec4d getRTSTime(const StelCore* core, const double altitude=0.) const;

	//! Return object's apparent V magnitude as seen from observer, without including extinction.
	virtual float getVMagnitude(const StelCore* core) const;
	
	//! Return object's apparent V magnitude as seen from observer including extinction.
	//! Extinction obviously only if atmosphere=on.
	//! If you already know vMag, it is wise to provide it in the optional @param knownVMag.
	//! Else it is called from getVMagnitude() which may be costly.
	float getVMagnitudeWithExtinction(const StelCore* core, const float knownVMag=-1000.f, const float& magOffset=0.f) const;

	//! Return a priority value which is used to discriminate objects by priority
	//! As for magnitudes, the lower is the higher priority
	virtual float getSelectPriority(const StelCore*) const;

	//! Get a color used to display info about the object
	virtual Vec3f getInfoColor() const {return Vec3f(1.f,1.f,1.f);}

	//! Return the best FOV in degree to use for a close view of the object
	virtual double getCloseViewFov(const StelCore*) const {return 10.;}

	//! Return the best FOV in degree to use for a global view of the object satellite system (if there are satellites)
	virtual double getSatellitesFov(const StelCore*) const {return -1.;}
	virtual double getParentSatellitesFov(const StelCore*) const {return -1.;}

	//! Return the angular radius of a circle containing the object as seen from the observer
	//! with the circle center assumed to be at getJ2000EquatorialPos().
	//! @return radius in degree. This value is the apparent angular size of the object, and is independent of the current FOV.
	//! @note The default implementation just returns zero.
	virtual double getAngularRadius(const StelCore* core) const { Q_UNUSED(core) return 0.; }

	//! Return airmass value for the object (for atmosphere-dependent calculations)
	//! @param core
	//! @return airmass value or -1.f if calculations are not applicable or meaningless
	virtual float getAirmass(const StelCore *core) const;

public slots:
	//! Allow additions to the Info String. Can be used by plugins to show extra info for the selected object, or for debugging.
	//! Hard-set this string group to a single str, or delete all messages when str==""
	//! @note This should be used with caution. Usually you want to use addToExtraInfoString().
	//! @note: If this breaks some const declaration, you can use StelObjectMgr::setExtraInfoString() instead.
	virtual void setExtraInfoString(const InfoStringGroup& flags, const QString &str);
	//! Add str to the extra string. This should be preferable over hard setting.
	//! Can be used by plugins to show extra info for the selected object, or for debugging.
	//! The strings will be shown in the InfoString for the selected object, below the default fields per-flag.
	//! Additional coordinates not fitting into one of the predefined coordinate sets should be flagged with OtherCoords,
	//! and must be adapted to table or non-table layout as required.
	//! The line ending must be given explicitly, usually just end a line with "<br/>", except when it may end up in a Table or appended to a line.
	//! See getCommonInfoString() or the respective getInfoString() in the subclasses for details of use.
	//! @note: If this breaks some const declaration, you can use StelObjectMgr::addToExtraInfoString() instead.
	virtual void addToExtraInfoString(const StelObject::InfoStringGroup& flags, const QString &str);
	//! Retrieve an (unsorted) QStringList of all extra info strings that match flags.
	//! Normally the order matches the order of addition, but this cannot be guaranteed.
	//! @note: Usually objects should keep their extraInfoStrings to themselves. But there are cases where StelObjectMgr::setExtraInfoString() has been set.
	QStringList getExtraInfoStrings(const InfoStringGroup& flags) const;
	//! Remove the extraInfoStrings with the given flags.
	//! This is a finer-grained removal than just extraInfoStrings.remove(flags), as it allows a combination of flags.
	//! After display, InfoPanel::setTextFromObjects() auto-clears the strings of the selected object using the AllInfo constant.
	//! extraInfoStrings having been set with the DebugAid and Script flags have to be removed by separate calls of this method.
	//! Those which have been set by scripts have to persist at least as long as the selection remains active.
	//! The behaviour of DebugAid texts depends on the use case.
	//! @note: Usually objects should keep their extraInfoStrings to themselves. But there are cases where StelObjectMgr::setExtraInfoString() has been set.
	void removeExtraInfoStrings(const InfoStringGroup& flags);

protected:
	//! Format the positional info string containing J2000/of date/altaz/hour angle positions and constellation, sidereal time, etc. for the object
	//! FIXME: We should split this and provide shorter virtual methods for various parts of the InfoString.
	//! The ExtraInfoStrings should be placed per flag, where they best fit.
	QString getCommonInfoString(const StelCore *core, const InfoStringGroup& flags) const;

	//! Format the magnitude info string for the object
	//! @param core
	//! @param flags
	//! @param decimals significant digits after the comma.
	//! @param magOffset magnitude offset to apply to display final apparent magnitude, are used if a star distance has changed in the past/future
	virtual QString getMagnitudeInfoString(const StelCore *core, const InfoStringGroup& flags, const int decimals=1, const float& magOffset=0.f) const;

	//! Add a section to the InfoString with just horizontal data for the Sun and Moon, when observed from Earth.
	//! The application of this is to have quick info while observing other objects.
	QString getSolarLunarInfoString(const StelCore *core, const InfoStringGroup& flags) const;

	//! Apply post processing on the info string.
	//! This also removes all extraInfoStrings possibly injected by modules (plugins) etc., except for Script and DebugAid types.
	void postProcessInfoString(QString& str, const InfoStringGroup& flags) const;

private:
	//! Location for additional object info that can be set for special purposes (at least for debugging, but maybe others), even via scripting.
	//! Modules are allowed to add new strings to be displayed in the various getInfoString() methods of subclasses.
	//! This helps avoiding screen collisions if a plugin wants to display some additional object information.
	//! This string map gets cleared by InfoPanel::setTextFromObjects(), with the exception of strings with Script or DebugAid flags,
	//! which have been injected by scripts or for debugging (take care of those yourself!).
	QMultiMap<InfoStringGroup, QString> extraInfoStrings;

	static int stelObjectPMetaTypeID;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(StelObject::InfoStringGroup)

#endif // STELOBJECT_HPP
