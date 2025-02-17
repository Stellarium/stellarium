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

#ifndef STARWRAPPER_HPP
#define STARWRAPPER_HPP

#include "StelObject.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StarMgr.hpp"
#include "Star.hpp"
#include "StelSkyDrawer.hpp"
#include "Planet.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

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
	StarWrapperBase(void) {}
	~StarWrapperBase(void) override {}
	QString getType(void) const override {return STAR_TYPE;}
	QString getObjectType(void) const override {return N_("star"); }
	QString getObjectTypeI18n(void) const override {return q_(getObjectType()); }

	QString getEnglishName(void) const override {return QString();}
	QString getNameI18n(void) const override = 0;

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
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const override;
	virtual float getBV(void) const = 0;
};

template <class Star> class StarWrapper : public StarWrapperBase
{
protected:
	StarWrapper(const SpecialZoneArray<Star> *array,
		const SpecialZoneData<Star> *zone,
		const Star *star) : a(array), z(zone), s(star) {}
	Vec3d getJ2000EquatorialPos(const StelCore* core) const override
	{
		Vec3d v;
		s->getJ2000Pos((core->getJDE()-STAR_CATALOG_JDEPOCH)/365.25, v);

		double withParallax = core->getUseParallax() * core->getParallaxFactor();
		if (withParallax) {
			const Vec3d diffPos = core->getParallaxDiff(core->getJDE());
			s->getPlxEffect(withParallax * s->getPlx(), v, diffPos);
			v.normalize();
		}

		// Aberration: Explanatory Supplement 2013, (7.38). We must get the observer planet speed vector in Equatorial J2000 coordinates.
		if (core->getUseAberration())
		{
			const Vec3d vel = core->getAberrationVec(core->getJDE());
			v+=vel;
			v.normalize();
		}

		return v;
	}
	Vec3f getInfoColor(void) const override
	{
		return StelSkyDrawer::indexToColor(s->getBVIndex());
	}
	float getVMagnitude(const StelCore* core) const override
	{
		Q_UNUSED(core)
		return s->getMag() / 1000.f;
	}
	float getBV(void) const  override {return s->getBV();}
	//QString getEnglishName(void) const override {return QString();}
	QString getNameI18n(void) const override {return s->getNameI18n();}
protected:
	const SpecialZoneArray<Star> *const a;
	const SpecialZoneData<Star> *const z;
	const Star *const s;
};


class StarWrapper1 : public StarWrapper<Star1>
{
public:
	StarWrapper1(const SpecialZoneArray<Star1> *array,
		const SpecialZoneData<Star1> *zone,
		const Star1 *star) : StarWrapper<Star1>(array,zone,star) {}

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
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const override;
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
	QVariantMap getInfoMap(const StelCore *core) const override;
	QString getEnglishName(void) const override;
	QString getID(void) const override;
	QString getObjectType() const override;
	QString getObjectTypeI18n() const override;
};

class StarWrapper2 : public StarWrapper<Star2>
{
public:
	StarWrapper2(const SpecialZoneArray<Star2> *array,
			   const SpecialZoneData<Star2> *zone,
			   const Star2 *star) : StarWrapper<Star2>(array,zone,star) {}
	QString getID(void) const override;
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const override;
	QString getObjectType() const override;
	QString getObjectTypeI18n() const override;
};

class StarWrapper3 : public StarWrapper<Star3>
{
public:
	StarWrapper3(const SpecialZoneArray<Star3> *array,
			   const SpecialZoneData<Star3> *zone,
			   const Star3 *star) : StarWrapper<Star3>(array,zone,star) {}
	QString getID(void) const override;
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const override;
	QString getObjectType() const override;
	QString getObjectTypeI18n() const override;
};


#endif // STARWRAPPER_HPP
