/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
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

#ifndef _STARWRAPPER_HPP_
#define _STARWRAPPER_HPP_

#include "StelObject.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StarMgr.hpp"
#include "Star.hpp"
#include "StelSkyDrawer.hpp"

#include <QString>

template <class Star> class SpecialZoneArray;
template <class Star> struct SpecialZoneData;


//! @class StarWrapperBase
//! A Star (Star1,Star2,Star3,...) cannot be a StelObject. The additional
//! overhead of having a dynamic type would simply be too much.
//! Therefore the StarWrapper is needed when returning Stars as StelObjects, e.g. for searching, and for constellations.
//! The StarWrapper is destroyed when it is not needed anymore, by utilizing reference counting.
//! So there is no chance that more than a few hundreds of StarWrappers are alive simultanousely.
//! Another reason for having the StarWrapper is to encapsulate the differences between the different kinds of Stars (Star1,Star2,Star3).
class StarWrapperBase : public StelObject
{
protected:
	StarWrapperBase(void) : ref_count(0) {;}
	virtual ~StarWrapperBase(void) {;}
	QString getType(void) const {return STAR_TYPE;}
	QString getID(void) const { return getEnglishName(); } //TODO: add a proper ID here (probably based on position?)

	QString getEnglishName(void) const {return "";}
	QString getNameI18n(void) const = 0;

	//! StarWrapperBase supports the following InfoStringGroup flags <ul>
	//! <li> Name
	//! <li> Magnitude
	//! <li> RaDecJ2000
	//! <li> RaDec
	//! <li> AltAzi
	//! <li> PlainText </ul>
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HTML encoded description of the StarWrapperBase.
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	virtual float getBV(void) const = 0;

private:
	int ref_count;
};

template <class Star> class StarWrapper : public StarWrapperBase
{
protected:
	StarWrapper(const SpecialZoneArray<Star> *a,
		const SpecialZoneData<Star> *z,
		const Star *s) : a(a), z(z), s(s) {;}
	Vec3d getJ2000EquatorialPos(const StelCore* core) const
	{
		static const double d2000 = 2451545.0;
		Vec3f v;
		s->getJ2000Pos(z, (M_PI/180.)*(0.0001/3600.) * ((core->getJDE()-d2000)/365.25) / a->star_position_scale, v);
		return Vec3d(v[0], v[1], v[2]);
	}
	Vec3f getInfoColor(void) const
	{
		return StelSkyDrawer::indexToColor(s->getBVIndex());
	}
	float getVMagnitude(const StelCore* core) const
	{
		Q_UNUSED(core);
		return 0.001f*a->mag_min + s->getMag()*(0.001f*a->mag_range)/a->mag_steps;
	}
	float getBV(void) const {return s->getBV();}
	QString getEnglishName(void) const {return QString();}
	QString getNameI18n(void) const {return s->getNameI18n();}
	virtual double getAngularSize(const StelCore*) const {return 0.;}	
protected:
	const SpecialZoneArray<Star> *const a;
	const SpecialZoneData<Star> *const z;
	const Star *const s;
};


class StarWrapper1 : public StarWrapper<Star1>
{
public:
	StarWrapper1(const SpecialZoneArray<Star1> *a,
		const SpecialZoneData<Star1> *z,
		const Star1 *s) : StarWrapper<Star1>(a,z,s) {;}

	//! StarWrapper1 supports the following InfoStringGroup flags: <ul>
	//! <li> Name
	//! <li> CatalogNumber
	//! <li> Magnitude
	//! <li> RaDecJ2000
	//! <li> RaDec
	//! <li> AltAzi
	//! <li> Extra (spectral type, parallax)
	//! <li> Distance	
	//! <li> PlainText </ul>
	//! @param core the StelCore object.
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the StarWrapper1.
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	//! In addition to the entries from StelObject::getInfoMap(), StarWrapper1 objects provide
	//! - variable-star (no|eruptive|pulsating|rotating|cataclysmic|eclipsing-binary)
	//! - star-type (star|double-star)
	//! - bV : B-V Color Index
	//! A few tags are only present if data known, or for variable or double stars from the WDS catalog
	//! - absolute-mag
	//! - distance-ly
	//! - parallax
	//! - spectral-class
	//! - period (days)
	//! - wds-year (year of validity of wds... fields)
	//! - wds-position-angle
	//! - wds-separation (arcseconds; 0 for spectroscopic binaries)
	virtual QVariantMap getInfoMap(const StelCore *core) const;
	QString getEnglishName(void) const;
};

class StarWrapper2 : public StarWrapper<Star2>
{
public:
	StarWrapper2(const SpecialZoneArray<Star2> *a,
			   const SpecialZoneData<Star2> *z,
			   const Star2 *s) : StarWrapper<Star2>(a,z,s) {;}
};

class StarWrapper3 : public StarWrapper<Star3>
{
public:
	StarWrapper3(const SpecialZoneArray<Star3> *a,
			   const SpecialZoneData<Star3> *z,
			   const Star3 *s) : StarWrapper<Star3>(a,z,s) {;}
};


#endif // _STARWRAPPER_HPP_
