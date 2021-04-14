/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
 * The implementation of most functions in this file
 * (getInfoString,getShortInfoString,...) is taken from
 * Stellarium, Copyright (C) 2002 Fabien Chereau,
 * and therefore the copyright of these belongs to Fabien Chereau.
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

#include "StarWrapper.hpp"
#include "ZoneArray.hpp"
#include "SolarSystem.hpp"

#include "StelUtils.hpp"
#include "StelTranslator.hpp"

#include <QTextStream>
#include <limits>

template<typename T> inline bool isNan(T value)
{
	return value != value; // lgtm [cpp/comparison-of-identical-expressions]
}

template<typename T> inline bool isInf(T value)
{
	return std::numeric_limits<T>::has_infinity && value == std::numeric_limits<T>::infinity();
}

QString StarWrapperBase::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_("star")) << "<br />";

	oss << getMagnitudeInfoString(core, flags, 2);

	if (flags&Extra)
		oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(getBV(), 'f', 2)) << "<br />";
	
	oss << getCommonInfoString(core, flags);

	StelObject::postProcessInfoString(str, flags);

	return str;
}

QString StarWrapper1::getEnglishName(void) const
{
	if (s->getHip())
		return QString("HIP %1").arg(s->getHip());
	return StarWrapperBase::getEnglishName();
}

QString StarWrapper1::getID(void) const
{
	QString hip;
	if (s->getHip())
	{
		if (s->hasComponentID())
			hip = QString("HIP %1 %2").arg(s->getHip()).arg(StarMgr::convertToComponentIds(s->getComponentIds()));
		else
			hip = QString("HIP %1").arg(s->getHip());
	}

	return hip;
}

QString StarWrapper1::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));
	Q_UNUSED(az_app)

	const QString varType = StarMgr::getGcvsVariabilityType(s->getHip());
	const int wdsObs = StarMgr::getWdsLastObservation(s->getHip());
	const float wdsPA = StarMgr::getWdsLastPositionAngle(s->getHip());
	const float wdsSep = StarMgr::getWdsLastSeparation(s->getHip());
	const float maxVMag = StarMgr::getGcvsMaxMagnitude(s->getHip());
	const float magFlag = StarMgr::getGcvsMagnitudeFlag(s->getHip());
	const float minVMag = StarMgr::getGcvsMinMagnitude(s->getHip());
	const float min2VMag = StarMgr::getGcvsMinMagnitude(s->getHip(), false);
	const QString photoVSys = StarMgr::getGcvsPhotometricSystem(s->getHip());
	const double vEpoch = StarMgr::getGcvsEpoch(s->getHip());
	const double vPeriod = StarMgr::getGcvsPeriod(s->getHip());
	const int vMm = StarMgr::getGcvsMM(s->getHip());
	const float plxErr = StarMgr::getPlxError(s->getHip());
	if (s->getHip())
	{
		if ((flags&Name) || (flags&CatalogNumber))
			oss << "<h2>";

		const QString commonNameI18 = StarMgr::getCommonName(s->getHip());
		const QString additionalNameI18 = StarMgr::getAdditionalNames(s->getHip());
		const QString sciName = StarMgr::getSciName(s->getHip());
		const QString addSciName = StarMgr::getSciAdditionalName(s->getHip());
		const QString dblSciName = StarMgr::getSciAdditionalDblName(s->getHip());
		const QString varSciName = StarMgr::getGcvsName(s->getHip());
		const QString wdsSciName = StarMgr::getWdsName(s->getHip());
		QStringList designations;
		if (!sciName.isEmpty())
			designations.append(sciName);
		if (!addSciName.isEmpty())
			designations.append(addSciName);
		if (!dblSciName.isEmpty())
			designations.append(dblSciName);
		if (!varSciName.isEmpty() && varSciName!=addSciName && varSciName!=dblSciName && varSciName!=sciName)
			designations.append(varSciName);

		QString hip, hipq;
		if (s->hasComponentID())
		{
			hip = QString("HIP %1 %2").arg(s->getHip()).arg(StarMgr::convertToComponentIds(s->getComponentIds()));
			hipq = QString("%1%2").arg(s->getHip()).arg(StarMgr::convertToComponentIds(s->getComponentIds()));
		}
		else
		{
			hip = QString("HIP %1").arg(s->getHip());
			hipq = QString("%1").arg(s->getHip());
		}

		designations.append(hip);

		const QString crossIndexData = StarMgr::getCrossIdentificationDesignations(hipq);
		if (!crossIndexData.isEmpty())
			designations.append(crossIndexData);

		if (!wdsSciName.isEmpty() && wdsSciName!=addSciName && wdsSciName!=sciName)
			designations.append(wdsSciName);

		const QString designationsList = designations.join(" - ");

		if (flags&Name)
		{
			if (!commonNameI18.isEmpty())
				oss << commonNameI18;

			if (!additionalNameI18.isEmpty() && StarMgr::getFlagAdditionalNames())
				oss << " (" << additionalNameI18 << ")";

			if (!commonNameI18.isEmpty() && !designationsList.isEmpty() && flags&CatalogNumber)
				oss << "<br />";
		}

		if (flags&CatalogNumber)
			oss << designationsList;

		if ((flags&Name) || (flags&CatalogNumber))
			oss << "</h2>";
	}
	if (flags&Name)
	{
		QStringList extraNames=getExtraInfoStrings(Name);
		if (extraNames.length()>0)
			oss << q_("Additional names: ") << extraNames.join(", ") << "<br/>";
	}
	if (flags&CatalogNumber)
	{
		QStringList extraCat=getExtraInfoStrings(CatalogNumber);
		if (extraCat.length()>0)
			oss << q_("Additional catalog numbers: ") << extraCat.join(", ") << "<br/>";
	}

	bool ebsFlag = false;
	if (flags&ObjectType)
	{
		QString varstartype = "";
		QString startype = "";
		if(!varType.isEmpty())
		{
			// see also http://www.sai.msu.su/gcvs/gcvs/vartype.htm
			if (QString("BE FU GCAS I IA IB IN INA INB INT IT IN(YY) IS ISA ISB RCB RS SDOR UV UVN WR").contains(varType))
				varstartype = q_("eruptive variable star");
			else if (QString("ACYG BCEP BCEPS BLBOO CEP CEP(B) CW CWA CWB DCEP DCEPS DSCT DSCTC GDOR L LB LC LPB M PVTEL RPHS RR RR(B) RRAB RRC RV RVA RVB SR SRA SRB SRC SRD SXPHE ZZ ZZA ZZB ZZO").contains(varType))
				varstartype = q_("pulsating variable star");
			else if (QString("ACV, ACVO, BY, ELL, FKCOM, PSR, SXARI").contains(varType))
				varstartype = q_("rotating variable star");
			else if (QString("N NA NB NC NL NR SN SNI SNII UG UGSS UGSU UGZ ZAND").contains(varType))
				varstartype = q_("cataclysmic variable star");
			else if (QString("E EA EB EP EW GS PN RS WD WR AR D DM DS DW K KE KW SD E: E:/WR E/D E+LPB: EA/D EA/D+BY EA/RS EA/SD EA/SD: EA/GS EA/GS+SRC EA/DM EA/WR EA+LPB EA+LPB: EA+DSCT EA+BCEP: EA+ZAND EA+ACYG EA+SRD EB/GS EB/DM EB/KE EB/KE: EW/KE EA/AR/RS EA/GS/D EA/D/WR").contains(varType))
			{
				varstartype = q_("eclipsing binary system");
				ebsFlag = true;
			}
			else
			// XXX intense variable X-ray sources "AM, X, XB, XF, XI, XJ, XND, XNG, XP, XPR, XPRM, XM)"
			// XXX other symbols "BLLAC, CST, GAL, L:, QSO, S,"
				varstartype = q_("variable star");
		}

		if (s->getComponentIds() || wdsObs>0)
			startype = q_("double star");
		else
			startype = q_("star");

		if (!varType.isEmpty())
		{
			QString vtt = varstartype;
			if (s->getComponentIds() || wdsObs>0)
				vtt = QString("%1, %2").arg(varstartype, startype);
			oss << QString("%1: <b>%2</b> (%3)").arg(q_("Type"), vtt, varType) << "<br />";
		}
		else
			oss << QString("%1: <b>%2</b>").arg(q_("Type"), startype) << "<br />";
		oss << getExtraInfoStrings(flags&ObjectType).join("");
	}

	oss << getMagnitudeInfoString(core, flags, 2);

	if ((flags&AbsoluteMagnitude) && s->getPlx ()&& !isNan(s->getPlx()) && !isInf(s->getPlx()))
		oss << QString("%1: %2").arg(q_("Absolute Magnitude")).arg(getVMagnitude(core)+5.*(1.+std::log10(0.00001*s->getPlx())), 0, 'f', 2) << "<br />";
	if (flags&AbsoluteMagnitude)
		oss << getExtraInfoStrings(AbsoluteMagnitude).join("");

	if (flags&Extra)
	{
		oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(s->getBV(), 'f', 2)) << "<br />";

		if (!varType.isEmpty())
		{
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
					minStr = QString("%1/%2").arg(QString::number(minimumM1, 'f', 2)).arg(QString::number(minimumM2, 'f', 2));

				oss << QString("%1: <b>%2</b>%3<b>%4</b> (%5: %6)").arg(q_("Magnitude range"), QString::number(maxVMag, 'f', 2), QChar(0x00F7), minStr, q_("Photometric system"), photoVSys) << "<br />";
			}
		}
	}

	oss << getCommonInfoString(core, flags);

	if (flags&Distance)
	{
		if (s->getPlx() && !isNan(s->getPlx()) && !isInf(s->getPlx()))
		{
			//TRANSLATORS: Unit of measure for distance - Light Years
			QString ly = qc_("ly", "distance");
			double k = AU/(SPEED_OF_LIGHT*86400*365.25);
			double d = ((0.00001/3600.)*(M_PI/180));
			double distance = k/(s->getPlx()*d);
			if (plxErr>0.f && (0.01f*s->getPlx())>plxErr) // No distance when error of parallax is bigger than parallax!
				oss << QString("%1: %2%3%4 %5").arg(q_("Distance"), QString::number(distance, 'f', 2), QChar(0x00B1), QString::number(qAbs(k/((100*plxErr + s->getPlx())*d) - distance), 'f', 2), ly) << "<br />";
			else
				oss << QString("%1: %2 %3").arg(q_("Distance"), QString::number(distance, 'f', 2), ly) << "<br />";
		}
		oss << getExtraInfoStrings(Distance).join("");
	}

	if (flags&ProperMotion)
	{
		float dx = 0.1f*s->getDx0();
		float dy = 0.1f*s->getDx1();
		float pa = std::atan2(dx, dy)*M_180_PIf;
		if (pa<0)
			pa += 360.f;
		oss << QString("%1: %2 %3 %4 %5%6").arg(q_("Proper motion"))
		       .arg(QString::number(std::sqrt(dx*dx + dy*dy), 'f', 1)).arg(qc_("mas/yr", "milliarc second per year"))
		       .arg(qc_("towards", "into the direction of")).arg(QString::number(pa, 'f', 1)).arg(QChar(0x00B0)) << "<br />";
		oss << QString("%1: %2 %3 (%4)").arg(q_("Proper motions by axes")).arg(QString::number(dx, 'f', 1)).arg(QString::number(dy, 'f', 1)).arg(qc_("mas/yr", "milliarc second per year")) << "<br />";
	}

	if (flags&Extra)
	{
		if (s->getPlx())
		{
			QString plx = q_("Parallax");
			if (plxErr>0.f)
				oss <<  QString("%1: %2%3%4 ").arg(plx, QString::number(0.01*s->getPlx(), 'f', 3), QChar(0x00B1), QString::number(plxErr, 'f', 3));
			else
				oss << QString("%1: %2 ").arg(plx, QString::number(0.01*s->getPlx(), 'f', 3));
			oss  << qc_("mas", "parallax") << "<br />";
		}

		if (s->getSpInt())
			oss << QString("%1: %2").arg(q_("Spectral Type"), StarMgr::convertToSpectralType(s->getSpInt())) << "<br />";

		if (vPeriod>0)
			oss << QString("%1: %2 %3").arg(q_("Period")).arg(vPeriod).arg(qc_("days", "duration")) << "<br />";

		if (vEpoch>0 && vPeriod>0)
		{
			// Calculate next minimum or maximum light
			double vsEpoch = 2400000+vEpoch;
			double npDate = vsEpoch + vPeriod * ::floor(1.0 + (core->getJDE() - vsEpoch)/vPeriod);
			QString nextDate = StelUtils::julianDayToISO8601String(npDate).replace("T", " ");
			QString dateStr = q_("Next maximum light");
			if (ebsFlag)
				dateStr = q_("Next minimum light");

			oss << QString("%1: %2 UTC").arg(dateStr, nextDate) << "<br />";
		}

		if (vMm>0)
		{
			QString mmStr = q_("Rising time");
			if (ebsFlag)
				mmStr = q_("Duration of eclipse");

			float daysFraction = vPeriod * vMm / 100.f;
			auto dms = StelUtils::daysFloatToDHMS(daysFraction);
			oss << QString("%1: %2% (%3)").arg(mmStr).arg(vMm).arg(dms) << "<br />";
		}

		if (wdsObs>0)
		{
			oss << QString("%1 (%4): %2%3").arg(q_("Position angle")).arg(QString::number(wdsPA, 'f', 2)).arg(QChar(0x00B0)).arg(wdsObs) << "<br />";
			if (wdsSep>0.f) // A spectroscopic binary or not?
			{
				if (wdsSep>60.f) // A wide binary star?
					oss << QString("%1 (%4): %2\" (%3)").arg(q_("Separation")).arg(QString::number(wdsSep, 'f', 3)).arg(StelUtils::decDegToDmsStr(wdsSep/3600.f)).arg(wdsObs) << "<br />";
				else
					oss << QString("%1 (%3): %2\"").arg(q_("Separation")).arg(QString::number(wdsSep, 'f', 3)).arg(wdsObs) << "<br />";
			}
		}
	}

	StelObject::postProcessInfoString(str, flags);

	return str;
}

QVariantMap StarWrapper1::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	const QString varType = StarMgr::getGcvsVariabilityType(s->getHip());
	const int wdsObs = StarMgr::getWdsLastObservation(s->getHip());
	const float wdsPA = StarMgr::getWdsLastPositionAngle(s->getHip());
	const float wdsSep = StarMgr::getWdsLastSeparation(s->getHip());
	const double vPeriod = StarMgr::getGcvsPeriod(s->getHip());

	QString varstartype = "no";
	QString startype = "star";
	if(!varType.isEmpty())
	{
		if (QString("FU GCAS I IA IB IN INA INB INT IT IN(YY) IS ISA ISB RCB RS SDOR UV UVN WR").contains(varType))
			varstartype = "eruptive";
		else if (QString("ACYG BCEP BCEPS CEP CEP(B) CW CWA CWB DCEP DCEPS DSCT DSCTC GDOR L LB LC M PVTEL RPHS RR RR(B) RRAB RRC RV RVA RVB SR SRA SRB SRC SRD SXPHE ZZ ZZA ZZB").contains(varType))
			varstartype = "pulsating";
		else if (QString("ACV, ACVO, BY, ELL, FKCOM, PSR, SXARI").contains(varType))
			varstartype = "rotating";
		else if (QString("N NA NB NC NL NR SN SNI SNII UG UGSS UGSU UGZ ZAND").contains(varType))
			varstartype = "cataclysmic";
		else if (QString("E EA EB EW GS PN RS WD WR AR D DM DS DW K KE KW SD E: E:/WR E/D E+LPB: EA/D EA/D+BY EA/RS EA/SD EA/SD: EA/GS EA/GS+SRC EA/DM EA/WR EA+LPB EA+LPB: EA+DSCT EA+BCEP: EA+ZAND EA+ACYG EA+SRD EB/GS EB/DM EB/KE EB/KE: EW/KE EA/AR/RS EA/GS/D EA/D/WR").contains(varType))
		{
			varstartype = "eclipsing-binary";
		}
		else
			varstartype = "variable";
	}
	map.insert("variable-star", varstartype);

	if (s->getComponentIds() || wdsObs>0)
		startype = "double-star";

	map.insert("star-type", startype);

	map.insert("bV", s->getBV());

	if (s->getPlx ()&& !isNan(s->getPlx()) && !isInf(s->getPlx()))
	{
		map.insert("parallax", 0.00001*s->getPlx());
		map.insert("absolute-mag", getVMagnitude(core)+5.f*(1.f+std::log10(0.00001f*s->getPlx())));
		map.insert("distance-ly", (AU/(SPEED_OF_LIGHT*86400*365.25)) / (s->getPlx()*((0.00001/3600)*(M_PI/180))));
	}

	if (s->getSpInt())
		map.insert("spectral-class", StarMgr::convertToSpectralType(s->getSpInt()));

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

StelObjectP Star1::createStelObject(const SpecialZoneArray<Star1> *a, const SpecialZoneData<Star1> *z) const
{
	return StelObjectP(new StarWrapper1(a,z,this), true);
}

StelObjectP Star2::createStelObject(const SpecialZoneArray<Star2> *a, const SpecialZoneData<Star2> *z) const
{
	return StelObjectP(new StarWrapper2(a,z,this), true);
}

StelObjectP Star3::createStelObject(const SpecialZoneArray<Star3> *a, const SpecialZoneData<Star3> *z) const
{
	return StelObjectP(new StarWrapper3(a,z,this), true);
}
