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
	virtual StarId getStarId() const {return s->getGaia();}
	virtual QString getID(void) const override
	{
		return QString("Gaia DR3 %1").arg(s->getGaia());
	}

	Vec3d getJ2000EquatorialPos(const StelCore* core) const override
	{
		Vec3d v;

		double RA, DEC, pmra, pmdec, Plx, RadialVel;
		float dyrs = static_cast<float>(core->getJDE()-STAR_CATALOG_JDEPOCH)/365.25;
		s->getFull6DSolution(RA, DEC, Plx, pmra, pmdec, RadialVel, dyrs);
		StelUtils::spheToRect(RA, DEC, v);

		// in case it is in a binary system
		s->getBinaryOrbit(core->getJDE(), v);
		double withParallax = core->getUseParallax() * core->getParallaxFactor();
		if (withParallax) {
			const Vec3d diffPos = core->getParallaxDiff(core->getJDE());
			s->getPlxEffect(withParallax * Plx, v, diffPos);
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
	float getBV(void) const override final {return s->getBV();}
	//QString getEnglishName(void) const override {return QString();}
	QString getNameI18n(void) const override final {return s->getNameI18n();}

	QString getVariabilityRangeInfoString(const StelCore *core, const InfoStringGroup& flags) const
	{
		StarId star_id=getStarId();
		if (flags&Extra) // variable range
		{
			const QString varType = StarMgr::getGcvsVariabilityType(star_id);

			if (!varType.isEmpty())
			{
				const float maxVMag = StarMgr::getGcvsMaxMagnitude(star_id);
				const float magFlag = StarMgr::getGcvsMagnitudeFlag(star_id);
				const float minVMag = StarMgr::getGcvsMinMagnitude(star_id);
				const float min2VMag = StarMgr::getGcvsMinMagnitude(star_id, false);
				const QString photoVSys = StarMgr::getGcvsPhotometricSystem(star_id);
				float minimumM1 = minVMag;
				float minimumM2 = min2VMag;
				if (magFlag==1.f) // Amplitude
				{
					minimumM1 += maxVMag;
					minimumM2 += maxVMag;
				}

				if (maxVMag!=99.f) // seems it is not eruptive variable star
				{
					QString minStr = QString::number(minimumM1, 'f', 2);
					if (min2VMag<99.f)
						minStr = QString("%1/%2").arg(QString::number(minimumM1, 'f', 2), QString::number(minimumM2, 'f', 2));

					return QString("%1: <b>%2</b>%3<b>%4</b> (%5: %6)").arg(q_("Magnitude range"), QString::number(maxVMag, 'f', 2), QChar(0x00F7), minStr, q_("Photometric system"), photoVSys) + "<br />";
				}
			}
		}
		return QString();
	}
	QString getVariabilityRangeNarration(const StelCore *core, const InfoStringGroup& flags) const
	{
		StarId star_id=getStarId();
		if (flags&Extra) // variable range
		{
			const QString varType = StarMgr::getGcvsVariabilityType(star_id);
			if (!varType.isEmpty())
			{
				const float maxVMag = StarMgr::getGcvsMaxMagnitude(star_id);
				const float magFlag = StarMgr::getGcvsMagnitudeFlag(star_id);
				const float minVMag = StarMgr::getGcvsMinMagnitude(star_id);
				const float min2VMag = StarMgr::getGcvsMinMagnitude(star_id, false);
				const QString photoVSys = StarMgr::getGcvsPhotometricSystem(star_id);

				float minimumM1 = minVMag;
				float minimumM2 = min2VMag;
				if (magFlag==1.f) // Amplitude
				{
					minimumM1 += maxVMag;
					minimumM2 += maxVMag;
				}

				if (maxVMag!=99.f) // seems it is not eruptive variable star
				{
					QString minStr = StelUtils::narrateDecimal(minimumM1,  2);
					if (min2VMag<99.f)
						minStr = QString(qc_("either %1 or %2", "object narration, alternatives")).arg(StelUtils::narrateDecimal(minimumM1, 2), StelUtils::narrateDecimal(minimumM2, 2));

					return QString(qc_("Its magnitude range goes from %1 to %2 in the Photometric system %3.", "object narration"))
					       .arg(StelUtils::narrateDecimal(maxVMag, 2), minStr, photoVSys) + " ";
				}
			}
		}
		return QString();
	}
	//! @return either a generalized variable star type or "star"
	QString getObjectType() const override
	{
		StarId star_id = getStarId();

		const QString varType = StarMgr::getGcvsVariabilityType(star_id);
		if(!varType.isEmpty())
		{
			QString varstartype = "";
			// see also http://www.sai.msu.su/gcvs/gcvs/vartype.htm
			if (QString("BE FU GCAS I IA IB IN INA INB INT IT IN(YY) IS ISA ISB RCB RS SDOR UV UVN WR").contains(varType))
				varstartype = N_("eruptive variable star");
			else if (QString("ACYG BCEP BCEPS BLBOO CEP CEP(B) CW CWA CWB DCEP DCEPS DSCT DSCTC GDOR L LB LC LPB M PVTEL RPHS RR RR(B) RRAB RRC RV RVA RVB SR SRA SRB SRC SRD SXPHE ZZ ZZA ZZB ZZO").contains(varType))
				varstartype = N_("pulsating variable star");
			else if (QString("ACV, ACVO, BY, ELL, FKCOM, PSR, SXARI").contains(varType))
				varstartype = N_("rotating variable star");
			else if (QString("N NA NB NC NL NR SN SNI SNII UG UGSS UGSU UGZ ZAND").contains(varType))
				varstartype = N_("cataclysmic variable star");
			else if (QString("E EA EB EP EW GS PN RS WD WR AR D DM DS DW K KE KW SD E: E:/WR E/D E+LPB: EA/D EA/D+BY EA/RS EA/SD EA/SD: EA/GS EA/GS+SRC EA/DM EA/WR EA+LPB EA+LPB: EA+DSCT EA+BCEP: EA+ZAND EA+ACYG EA+SRD EB/GS EB/DM EB/KE EB/KE: EW/KE EA/AR/RS EA/GS/D EA/D/WR").contains(varType))
				varstartype = N_("eclipsing binary system");
			else
			// XXX intense variable X-ray sources "AM, X, XB, XF, XI, XJ, XND, XNG, XP, XPR, XPRM, XM)"
			// XXX other symbols "BLLAC, CST, GAL, L:, QSO, S,"
				varstartype = N_("variable star");

			return varstartype;
		}
		else
			return N_("star");
	}
	QString getObjectTypeI18n() const override
	{
		QString stypefinal, stype = getObjectType();
		StarId star_id =  getStarId();

		const QString varType = StarMgr::getGcvsVariabilityType(star_id);
		if (!varType.isEmpty())
		{
			if (stype.contains(","))
			{
				const QStringList stypes = stype.split(",");
				QStringList stypesI18n;
				for (const auto &st: stypes) { stypesI18n << q_(st.trimmed()); }
				stypefinal = stypesI18n.join(", ");
			}
			else
				stypefinal = q_(stype);
		}
		else
			stypefinal = q_(stype);

		return stypefinal;
	}
	QVariantMap getInfoMap(const StelCore *core) const override
	{
		QVariantMap map = StelObject::getInfoMap(core);
		StarId star_id=getStarId();
		const int wdsObs = StarMgr::getWdsLastObservation(star_id);
		const float wdsPA = StarMgr::getWdsLastPositionAngle(star_id);
		const float wdsSep = StarMgr::getWdsLastSeparation(star_id);
		const double vPeriod = StarMgr::getGcvsPeriod(star_id);

		const QString objectType=StarWrapper::getObjectType();
		QMap<QString, QString>varmap={
			{N_("eruptive variable star")   , "eruptive"},
			{N_("pulsating variable star")  , "pulsating"},
			{N_("rotating variable star")   , "rotating"},
			{N_("cataclysmic variable star"), "cataclysmic"},
			{N_("eclipsing binary system")  , "eclipsing-binary"},
			{N_("variable star")            , "variable"},
			{N_("star")                     , "no"}};
		map.insert("variable-star", varmap.value(objectType, "no"));

		map.insert("bV", getBV());

		if (s->getPlx())
		{
			map.insert("parallax", 0.001*s->getPlx());
			map.insert("absolute-mag", getVMagnitude(core)+5.f*(std::log10(0.001*s->getPlx())));
			map.insert("distance-ly", (AU/(SPEED_OF_LIGHT*86400*365.25)) / (s->getPlx()*((0.001/3600)*(M_PI/180))));
		}

		if (vPeriod>0)
			map.insert("period", vPeriod);

		if (wdsObs>0)
		{
			map.insert("wds-year", wdsObs);
			map.insert("wds-position-angle", wdsPA);
			map.insert("wds-separation", wdsSep);
		}

		return map;
	}

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
	StarId getStarId() const override {return s->getHip() ?  s->getHip() : s->getGaia();}
	QString getEnglishName(void) const override;
	QString getID(void) const override;
	QString getObjectType() const override;
	//QString getObjectTypeI18n() const override;

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
	//! @return a QString containing an HTML encoded description of the StarWrapper1.
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
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	QString getNarration(const StelCore *core, const InfoStringGroup& flags=StelObject::AllInfo) const override;
#endif
};

class StarWrapper2 : public StarWrapper<Star2>
{
public:
	StarWrapper2(const SpecialZoneArray<Star2> *array,
			   const SpecialZoneData<Star2> *zone,
			   const Star2 *star) : StarWrapper<Star2>(array,zone,star) {}
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const override;
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	QString getNarration(const StelCore *core, const InfoStringGroup& flags=StelObject::AllInfo) const override;
#endif
};

class StarWrapper3 : public StarWrapper<Star3>
{
public:
	StarWrapper3(const SpecialZoneArray<Star3> *array,
			   const SpecialZoneData<Star3> *zone,
			   const Star3 *star) : StarWrapper<Star3>(array,zone,star) {}
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const override;
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	QString getNarration(const StelCore *core, const InfoStringGroup& flags=StelObject::AllInfo) const override;
#endif
};

#endif // STARWRAPPER_HPP
