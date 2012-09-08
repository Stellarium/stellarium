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

#ifndef _STELOBJECT_HPP_
#define _STELOBJECT_HPP_

#include <QFlags>
#include <QString>
#include "VecMath.hpp"
#include "StelObjectType.hpp"
#include "StelRegionObject.hpp"

class StelCore;

//! The base abstract class for sky objects used in Stellarium like Stars, Planets, Constellations etc...
//! Normally you should use StelObjectP instead of StelObject* which have by default the same behaviour,
//! but which can be added reference counting if needed.
//! @sa StelObjectP
class StelObject : public StelRegionObject
{
public:
	//! @enum InfoStringGroupFlags used as named bitfield flags as specifiers to
	//! filter results of getInfoString. The precise definition of these should
	//! be documented in the getInfoString documentation for the derived classes
	//! for all specifiers which are defined in that derivative.
	//! Use InfoStringGroup instead.
	enum InfoStringGroupFlags
	{
		Name          = 0x00000001, //!< An object's name
		CatalogNumber = 0x00000002, //!< Catalog numbers
		Magnitude     = 0x00000004, //!< Magnitude related data
		RaDecJ2000    = 0x00000008, //!< The equatorial position (J2000 ref)
		RaDecOfDate   = 0x00000010, //!< The equatorial position (of date)
		AltAzi        = 0x00000020, //!< The position (Altitude/Azimuth)
		Distance      = 0x00000040, //!< Info about an object's distance
		Size          = 0x00000080, //!< Info about an object's size
		Extra1        = 0x00000100, //!< Derived class-specific extra fields
		Extra2        = 0x00000200, //!< Derived class-specific extra fields
		Extra3        = 0x00000400, //!< Derived class-specific extra fields
		PlainText     = 0x00000800, //!< Strip HTML tags from output
		HourAngle     = 0x00001000,  //!< The hour angle + DE (of date)
		AbsoluteMagnitude = 0x00002000,  //!< The absolute magnitude
		GalCoordJ2000 = 0x00004000	//!< The galactic position (J2000 ref)
	};
	typedef QFlags<InfoStringGroupFlags> InfoStringGroup;
	Q_FLAGS(InfoStringGroup)

	//! A pre-defined set of specifiers for the getInfoString flags argument to getInfoString
	static const InfoStringGroupFlags AllInfo = (InfoStringGroupFlags)(Name|CatalogNumber|Magnitude|RaDecJ2000|RaDecOfDate|AltAzi|Distance|Size|Extra1|Extra2|Extra3|HourAngle|AbsoluteMagnitude|GalCoordJ2000);
	//! A pre-defined set of specifiers for the getInfoString flags argument to getInfoString
	static const InfoStringGroupFlags ShortInfo = (InfoStringGroupFlags)(Name|CatalogNumber|Magnitude|RaDecJ2000);

	virtual ~StelObject() {}

	//! Default implementation of the getRegion method.
	//! Return the spatial region of the object.
	virtual SphericalRegionP getRegion() const {return SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));}

	//! Write I18n information about the object in QString.
	//! @param core the StelCore object to use
	//! @param flags a set of InfoStringGroup flags which are used to
	//! filter the return value - including specified types of information
	//! and altering the output format.
	//! @return an HTML string containing information about the StelObject.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags=StelObject::AllInfo) const = 0;

	//! Return object's type. It should be the name of the class.
	virtual QString getType() const = 0;

	//! Return object's name in english
	virtual QString getEnglishName() const = 0;

	//! Return translated object's name
	virtual QString getNameI18n() const = 0;

	//! Get observer-centered equatorial coordinates at equinox J2000
	virtual Vec3d getJ2000EquatorialPos(const StelCore* core) const = 0;

	//! Get observer-centered equatorial coordinate at the current equinox
	//! The frame has it's Z axis at the planet's current rotation axis
	//! At time 2000-01-01 this frame is almost the same as J2000, but ONLY if the observer is on earth
	Vec3d getEquinoxEquatorialPos(const StelCore* core) const;

	//! Get observer-centered galactic coordinates at equinox J2000
	Vec3d getJ2000GalacticPos(const StelCore* core) const;

	//! Get observer-centered hour angle + declination (at current equinox)
	//! It is the geometric position, i.e. without taking refraction effect into account.
	//! The frame has its Z axis at the planet's current rotation axis
	Vec3d getSideralPosGeometric(const StelCore* core) const;

	//! Get observer-centered hour angle + declination (at current equinox)
	//! It is the apparent position, i.e. taking the refraction effect into account.
	//! The frame has its Z axis at the planet's current rotation axis
	Vec3d getSideralPosApparent(const StelCore* core) const;

	//! Get observer-centered alt/az position
	//! It is the geometric position, i.e. without taking refraction effect into account.
	//! The frame has it's Z axis at the zenith
	Vec3d getAltAzPosGeometric(const StelCore* core) const;

	//! Get observer-centered alt/az position
	//! It is the apparent position, i.e. taking the refraction effect into account.
	//! The frame has it's Z axis at the zenith
	Vec3d getAltAzPosApparent(const StelCore* core) const;

	//! Get observer-centered alt/az position
	//! It is the automatic position, i.e. taking the refraction effect into account if atmosphere is on.
	//! The frame has it's Z axis at the zenith
	Vec3d getAltAzPosAuto(const StelCore* core) const;

	//! Return object's apparent V magnitude as seen from observer
	virtual float getVMagnitude(const StelCore* core, bool withExtinction=false) const;

	//! Return a priority value which is used to discriminate objects by priority
	//! As for magnitudes, the lower is the higher priority
	virtual float getSelectPriority(const StelCore*) const {return 99;}

	//! Get a color used to display info about the object
	virtual Vec3f getInfoColor() const {return Vec3f(1,1,1);}

	//! Return the best FOV in degree to use for a close view of the object
	virtual double getCloseViewFov(const StelCore*) const {return 10.;}

	//! Return the best FOV in degree to use for a global view of the object satellite system (if there are satellites)
	virtual double getSatellitesFov(const StelCore*) const {return -1.;}
	virtual double getParentSatellitesFov(const StelCore*) const {return -1.;}

	//! Return the angular radius of a circle containing the object as seen from the observer
	//! with the circle center assumed to be at getJ2000EquatorialPos().
	//! @return radius in degree. This value is the apparent angular size of the object, and is independent of the current FOV.
	virtual double getAngularSize(const StelCore* core) const = 0;

protected:

	//! Format the positional info string contain J2000/of date/altaz/hour angle positions for the object
	QString getPositionInfoString(const StelCore *core, const InfoStringGroup& flags) const;

	//! Apply post processing on the info string
	void postProcessInfoString(QString& str, const InfoStringGroup& flags) const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(StelObject::InfoStringGroup)

#endif // _STELOBJECT_HPP_
