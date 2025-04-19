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

#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StarMgr.hpp"

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
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), getObjectTypeI18n()) << "<br />";

	oss << getMagnitudeInfoString(core, flags, 2);

	if (flags&Extra)
		oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(getBV(), 'f', 2)) << "<br />";
	
	oss << getCommonInfoString(core, flags);
	oss << getSolarLunarInfoString(core, flags);

	StelObject::postProcessInfoString(str, flags);

	return str;
}

QString StarWrapper1::getEnglishName(void) const
{
	if (s->getHip())
		return QString("HIP %1").arg(s->getHip());
	else
		return QString("Gaia DR3 %1").arg(s->getGaia());
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
	else
	{
		hip = QString("Gaia DR3 %1").arg(s->getGaia());
	}

	return hip;
}

QString StarWrapper1::getObjectType() const
{
	StarId star_id = s->getHip() ?  s->getHip() : s->getGaia();

	const QString varType = StarMgr::getGcvsVariabilityType(star_id);
	const int wdsObs = StarMgr::getWdsLastObservation(star_id);
	QString startype = (s->getComponentIds() || wdsObs>0) ? N_("double star") : N_("star");
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

		QString vtt = varstartype;
		if (s->getComponentIds() || wdsObs>0)
			vtt = QString("%1, %2").arg(startype, varstartype);
		return vtt;
	}
	else
		return startype;
}

QString StarWrapper1::getObjectTypeI18n() const
{
	QString stypefinal, stype = getObjectType();
	StarId star_id = s->getHip() ?  s->getHip() : s->getGaia();

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

QString StarWrapper1::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));
	Q_UNUSED(az_app)

	StarId star_id = s->getHip() ?  s->getHip() : s->getGaia();

	const QString varType = StarMgr::getGcvsVariabilityType(star_id);
	const QString objType = StarMgr::convertToOjectTypes(s->getObjType());
	const int wdsObs = StarMgr::getWdsLastObservation(star_id);
	const float wdsPA = StarMgr::getWdsLastPositionAngle(star_id);
	const float wdsSep = StarMgr::getWdsLastSeparation(star_id);
	const float maxVMag = StarMgr::getGcvsMaxMagnitude(star_id);
	const float magFlag = StarMgr::getGcvsMagnitudeFlag(star_id);
	const float minVMag = StarMgr::getGcvsMinMagnitude(star_id);
	const float min2VMag = StarMgr::getGcvsMinMagnitude(star_id, false);
	const QString photoVSys = StarMgr::getGcvsPhotometricSystem(star_id);
	const double vEpoch = StarMgr::getGcvsEpoch(star_id);
	const double vPeriod = StarMgr::getGcvsPeriod(star_id);
	const int vMm = StarMgr::getGcvsMM(star_id);

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "<h2>";

	const QString commonNameI18 = StarMgr::getCommonNameI18n(star_id);
	//const QString additionalNameI18 = StarMgr::getAdditionalNames(star_id);
	const QString culturalInfoName=StarMgr::getCulturalInfoLabel(star_id);
	const QString sciName = StarMgr::getSciDesignation(star_id);
	const QString sciExtraName = StarMgr::getSciExtraDesignation(star_id);
	const QString varSciName = StarMgr::getGcvsDesignation(star_id);
	const QString wdsSciName = StarMgr::getWdsDesignation(star_id);
	QStringList designations;
	if (!sciName.isEmpty())
		designations.append(sciName);
	if (!sciExtraName.isEmpty())
		designations.append(sciExtraName);
	if (!varSciName.isEmpty() && !sciName.contains(varSciName, Qt::CaseInsensitive))
		designations.append(varSciName);

	QString hip, hipq;
	if (s->getHip())
	{
		if (s->hasComponentID())
		{
			hip = QString("HIP %1 %2").arg(s->getHip()).arg(StarMgr::convertToComponentIds(s->getComponentIds()));
			hipq = QString("%1%2").arg(s->getHip()).arg(StarMgr::convertToComponentIds(s->getComponentIds()));
		}
		else
		{
			hip = QString("HIP %1").arg(s->getHip());
			hipq = QString::number(s->getHip());
		}
		designations.append(hip);
	}
	else
		hipq = QString::number(s->getGaia());  // need to look up with Gaia number

	const QString crossIndexData = StarMgr::getCrossIdentificationDesignations(hipq);
	if (!crossIndexData.isEmpty())
		designations.append(crossIndexData);

	if (s->getGaia())
		designations.append(QString("Gaia DR3 %1").arg(s->getGaia()));

	if (!wdsSciName.isEmpty() && !sciName.contains(wdsSciName, Qt::CaseInsensitive))
		designations.append(wdsSciName);

	QString designationsList = designations.join(" - ");
	designations.clear();
	designations = designationsList.split(" - ");
	designations.removeDuplicates();
	int asize = designations.size();
	if (asize>6) // Special case for many designations (max - 7 items per line); NOTE: Should we add size to the config data for skyculture?
	{
		designationsList = "";
		for(int i=0; i<asize; i++)
		{
			designationsList.append(designations.at(i));
			if (i<asize-1)
				designationsList.append(" - ");

			if (i>0 && (i % 6)==0 && i<(asize-1))
				designationsList.append("<br />");
		}
	}
	else
		designationsList = designations.join(" - ");

	if (flags&Name)
	{
		QString allNames = culturalInfoName;

		// TODO: Finalize appearance of cultural Names
		if (culturalInfoName.isEmpty())
			allNames = commonNameI18;

		if (!allNames.isEmpty())
			oss << StelUtils::wrapText(allNames, 80);

		if (!allNames.isEmpty() && !designationsList.isEmpty() && flags&CatalogNumber)
			oss << "<br />";
	}

	if (flags&CatalogNumber)
		oss << designationsList;

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "</h2>";

	if (flags&Name)
	{
		QStringList extraNames=getExtraInfoStrings(Name);
		if (!extraNames.isEmpty())
			oss << q_("Additional names: ") << extraNames.join(", ") << "<br/>";
	}
	if (flags&CatalogNumber)
	{
		QStringList extraCat=getExtraInfoStrings(CatalogNumber);
		if (!extraCat.isEmpty())
			oss << q_("Additional catalog numbers: ") << extraCat.join(", ") << "<br/>";
	}

	QString stype = getObjectType();
	QString objectTypeI18nStr = getObjectTypeI18n();
	bool ebsFlag = stype.contains("eclipsing binary system");
	if (flags&ObjectType)
	{
		QStringList stypes;
		if (!objType.isEmpty())
			stypes.append(objType);
		if (!varType.isEmpty())
			stypes.append(varType);
		if (stypes.size()>0)
			oss << QString("%1: <b>%2</b> (%3)").arg(q_("Type"), objectTypeI18nStr, stypes.join(", ")) << "<br />";
		else
			oss << QString("%1: <b>%2</b>").arg(q_("Type"), objectTypeI18nStr) << "<br />";

		oss << getExtraInfoStrings(flags&ObjectType).join("");
	}

	double RA, DEC, pmra, pmdec, Plx, RadialVel;
	double PlxErr = s->getPlxErr();
	float dyrs = static_cast<float>(core->getJDE()-STAR_CATALOG_JDEPOCH)/365.25;
	s->getFull6DSolution(RA, DEC, Plx, pmra, pmdec, RadialVel, dyrs);
	Vec3d v;
	double binary_sep = 0.0, binary_pa = 0.0;  // binary star separation and position angle
	s->getBinaryOrbit(core->getJDE(), v, RA, DEC, Plx, pmra, pmdec, RadialVel, binary_sep, binary_pa);
	binary_pa *= M_180_PIf;

	float magOffset = 0.f;
	if (Plx && s->getPlx())
	{
		magOffset = 5.f * log10(s->getPlx()/Plx);
	} 
	oss << getMagnitudeInfoString(core, flags, 2, magOffset);

	// should use Plx from getPlx because Plx can change with time, but not absolute magnitude
	if ((flags&AbsoluteMagnitude) && s->getPlx())
		oss << QString("%1: %2").arg(q_("Absolute Magnitude")).arg(getVMagnitude(core)+5.*(1.+std::log10(0.001*s->getPlx())), 0, 'f', 2) << "<br />";
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
					minStr = QString("%1/%2").arg(QString::number(minimumM1, 'f', 2), QString::number(minimumM2, 'f', 2));

				oss << QString("%1: <b>%2</b>%3<b>%4</b> (%5: %6)").arg(q_("Magnitude range"), QString::number(maxVMag, 'f', 2), QChar(0x00F7), minStr, q_("Photometric system"), photoVSys) << "<br />";
			}
		}
	}

	oss << getCommonInfoString(core, flags);

	// kinda impossible for both parallax and parallax_err to be exactly 0, so they must just be missing
	if (flags&Distance)
	{
		// do parallax SNR cut because we are calculating distance, inverse parallax is bad for >20% uncertainty
		if ((Plx!=0) && (PlxErr!=0) && (Plx/PlxErr>5))
		{
			//TRANSLATORS: Unit of measure for distance - Light Years
			QString ly = qc_("ly", "distance");
			double distance = PARSEC_LY * 1000. / Plx;
			oss << QString("%1: %2%3%4 %5").arg(q_("Distance"), QString::number(distance, 'f', 2), QChar(0x00B1), QString::number(distance * PlxErr / Plx, 'f', 2), ly) << "<br />";
		}
		oss << getExtraInfoStrings(Distance).join("");
	}

	// kinda impossible for both pm to be exactly 0, so they must just be missing
	if ((flags&ProperMotion) && (pmra || pmdec))
	{
		float pa = std::atan2(pmra, pmdec)*M_180_PIf;
		if (pa<0)
			pa += 360.f;
		oss << QString("%1: %2 %3 %4 %5°").arg(q_("Proper motion"),
							QString::number(std::sqrt(pmra * pmra + pmdec * pmdec), 'f', 2),
							qc_("mas/yr", "milliarc second per year"),
							qc_("towards", "into the direction of"),
							QString::number(pa, 'f', 1)) << "<br />";
		oss << QString("%1: %2 %3 (%4)").arg(q_("Proper motions by axes"),
							QString::number(pmra, 'f', 2),
							QString::number(pmdec, 'f', 2),
							qc_("mas/yr", "milliarc second per year")) << "<br />";
	}

	if (flags&Velocity)
	{
		if (RadialVel)
		{
			// TRANSLATORS: Unit of measure for speed - kilometers per second
			QString kms = qc_("km/s", "speed");
			oss << QString("%1: %2 %3").arg(q_("Radial velocity"), QString::number(RadialVel, 'f', 1), kms) << "<br />";
		}
	}

	if (flags&Extra)
	{
		if (Plx!=0)
		{
			QString plx = q_("Parallax");
			if (PlxErr>0.f)
				oss <<  QString("%1: %2%3%4 ").arg(plx, QString::number(Plx, 'f', 3), QChar(0x00B1), QString::number(PlxErr, 'f', 3));
			else
				oss << QString("%1: %2 ").arg(plx, QString::number(Plx, 'f', 3));
			oss  << qc_("mas", "parallax") << "<br />";
		}

		if (s->getSpInt())
			oss << QString("%1: %2").arg(q_("Spectral Type"), StarMgr::convertToSpectralType(s->getSpInt())) << "<br />";

		if (vPeriod>0.)
			oss << QString("%1: %2 %3").arg(q_("Period")).arg(vPeriod).arg(qc_("days", "duration")) << "<br />";

		if (vEpoch>0. && vPeriod>0.)
		{
			// Calculate next minimum or maximum light
			double vsEpoch = 2400000+vEpoch;
			double npDate = vsEpoch + vPeriod * ::floor(1.0 + (core->getJD() - vsEpoch)/vPeriod);
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

		if ((wdsObs>0) || (binary_sep>0.f))  // either have a WDS observation or a separation modelled by the binary orbit
		{
			// use separation and position angle from the binary orbit if available
			oss << QString("%1 (%3): %2°").arg(q_("Position angle"),
							QString::number((binary_sep>0.f) ? binary_pa: wdsPA, 'f', 2),
							(binary_sep>0.f) ? qc_("on date", "coordinates for current epoch"): QString::number(wdsObs)) << "<br />";
			if (wdsSep>0.f && wdsSep<999.f) // A spectroscopic binary or not?
			{
				if (wdsSep>60.f) // A wide binary star?
					oss << QString("%1 (%4): %2\" (%3)").arg(
									    q_("Separation"),
									    QString::number((binary_sep>0.f) ? binary_sep: wdsSep, 'f', 3),
									    StelUtils::decDegToDmsStr(((binary_sep>0.f) ? binary_sep: wdsSep)/3600.f),
									    (binary_sep>0.f) ? qc_("on date", "coordinates for current epoch"): QString::number(wdsObs)) << "<br />";
				else
					oss << QString("%1 (%3): %2\"").arg(q_("Separation"), QString::number(wdsSep, 'f', 3), QString::number(wdsObs)) << "<br />";
			}
		}
	}

	oss << getSolarLunarInfoString(core, flags);

	StelObject::postProcessInfoString(str, flags);

	return str;
}

QVariantMap StarWrapper1::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);
	StarId star_id;
	if (s->getHip())
	{
		star_id = s->getHip();
	}
	else
	{
		star_id = s->getGaia();
	}
	const QString varType = StarMgr::getGcvsVariabilityType(star_id);
	const int wdsObs = StarMgr::getWdsLastObservation(star_id);
	const float wdsPA = StarMgr::getWdsLastPositionAngle(star_id);
	const float wdsSep = StarMgr::getWdsLastSeparation(star_id);
	const double vPeriod = StarMgr::getGcvsPeriod(star_id);

	QString varstartype = "no";
	QString startype = "star";
	if(!varType.isEmpty())
	{
		if (QString("BE FU GCAS I IA IB IN INA INB INT IT IN(YY) IS ISA ISB RCB RS SDOR UV UVN WR").contains(varType))
			varstartype = "eruptive";
		else if (QString("ACYG BCEP BCEPS BLBOO CEP CEP(B) CW CWA CWB DCEP DCEPS DSCT DSCTC GDOR L LB LC LPB M PVTEL RPHS RR RR(B) RRAB RRC RV RVA RVB SR SRA SRB SRC SRD SXPHE ZZ ZZA ZZB ZZO").contains(varType))
			varstartype = "pulsating";
		else if (QString("ACV, ACVO, BY, ELL, FKCOM, PSR, SXARI").contains(varType))
			varstartype = "rotating";
		else if (QString("N NA NB NC NL NR SN SNI SNII UG UGSS UGSU UGZ ZAND").contains(varType))
			varstartype = "cataclysmic";
		else if (QString("E EA EB EP EW GS PN RS WD WR AR D DM DS DW K KE KW SD E: E:/WR E/D E+LPB: EA/D EA/D+BY EA/RS EA/SD EA/SD: EA/GS EA/GS+SRC EA/DM EA/WR EA+LPB EA+LPB: EA+DSCT EA+BCEP: EA+ZAND EA+ACYG EA+SRD EB/GS EB/DM EB/KE EB/KE: EW/KE EA/AR/RS EA/GS/D EA/D/WR").contains(varType))
			varstartype = "eclipsing-binary";
		else
			varstartype = "variable";
	}
	map.insert("variable-star", varstartype);

	if (s->getComponentIds() || wdsObs>0)
		startype = "double-star";

	map.insert("star-type", startype);

	map.insert("bV", s->getBV());

	if (s->getPlx())
	{
		map.insert("parallax", 0.001*s->getPlx());
		map.insert("absolute-mag", getVMagnitude(core)+5.f*(std::log10(0.001*s->getPlx())));
		map.insert("distance-ly", (AU/(SPEED_OF_LIGHT*86400*365.25)) / (s->getPlx()*((0.001/3600)*(M_PI/180))));
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

QString StarWrapper2::getObjectType() const
{
	StarId star_id =  s->getGaia();

	const QString varType = StarMgr::getGcvsVariabilityType(star_id);
	QString startype = N_("star");
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
		return startype;
}

QString StarWrapper2::getID(void) const
{
	return QString("Gaia DR3 %1").arg(s->getGaia());
}

QString StarWrapper2::getObjectTypeI18n() const
{
	QString stypefinal, stype = getObjectType();
	StarId star_id =  s->getGaia();

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

QString StarWrapper2::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	StarId star_id = s->getGaia();
	const QString commonNameI18 = StarMgr::getCommonNameI18n(star_id);
	//const QString additionalNameI18 = StarMgr::getAdditionalNames(star_id);
	const QString culturalInfoName=StarMgr::getCulturalInfoLabel(star_id);
	const QString sciName = StarMgr::getSciDesignation(star_id);
	const QString sciExtraName = StarMgr::getSciExtraDesignation(star_id);
	const QString varSciName = StarMgr::getGcvsDesignation(star_id);
	const QString varType = StarMgr::getGcvsVariabilityType(star_id);

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "<h2>";

	QStringList designations;
	if (!sciName.isEmpty())
		designations.append(sciName);
	if (!sciExtraName.isEmpty())
		designations.append(sciExtraName);
	if (!varSciName.isEmpty() && !sciName.contains(varSciName, Qt::CaseInsensitive))
		designations.append(varSciName);

	const QString crossIndexData = StarMgr::getCrossIdentificationDesignations(QString::number(s->getGaia()));
	if (!crossIndexData.isEmpty())
		designations.append(crossIndexData);

	designations.append(QString("Gaia DR3 %1").arg(s->getGaia()));

	QString designationsList = designations.join(" - ");
	designations.clear();
	designations = designationsList.split(" - ");
	designations.removeDuplicates();
	int asize = designations.size();
	if (asize>6) // Special case for many designations (max - 7 items per line); NOTE: Should we add size to the config data for skyculture?
	{
		designationsList = "";
		for(int i=0; i<asize; i++)
		{
			designationsList.append(designations.at(i));
			if (i<asize-1)
				designationsList.append(" - ");

			if (i>0 && (i % 6)==0 && i<(asize-1))
				designationsList.append("<br />");
		}
	}
	else
		designationsList = designations.join(" - ");

	if (flags&Name)
	{
		QString commonNames;
		if (!commonNameI18.isEmpty())
			commonNames = commonNameI18;

		//if (!additionalNameI18.isEmpty() && StarMgr::getFlagAdditionalNames())
		//{
		//	QStringList additionalNames = additionalNameI18.split(" - ");
		//	additionalNames.removeDuplicates();

		//	commonNames.append(QString(" (%1)").arg(additionalNames.join(" - ")));
		//}
		// TODO: Finalize appearance of cultural Names
		if (!culturalInfoName.isEmpty() && StarMgr::getFlagAdditionalNames())
			oss << culturalInfoName << "<br />";

		if (!commonNames.isEmpty())
			oss << StelUtils::wrapText(commonNames, 80);

		if (!commonNameI18.isEmpty() && !designationsList.isEmpty() && flags&CatalogNumber)
			oss << "<br />";
	}

	if (flags&CatalogNumber)
		oss << designationsList;

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "</h2>";

	if (flags&ObjectType)
	{
		const QString objectTypeI18nStr = getObjectTypeI18n();
		if (!varType.isEmpty())
			oss << QString("%1: <b>%2</b> (%3)").arg(q_("Type"), objectTypeI18nStr, varType) << "<br />";
		else
			oss << QString("%1: <b>%2</b>").arg(q_("Type"), objectTypeI18nStr) << "<br />";
	}

	oss << getMagnitudeInfoString(core, flags, 2);

	double RA, DEC, pmra, pmdec, Plx, RadialVel;
	double PlxErr = s->getPlxErr();
	float dyrs = static_cast<float>(core->getJDE()-STAR_CATALOG_JDEPOCH)/365.25;
	s->getFull6DSolution(RA, DEC, Plx, pmra, pmdec, RadialVel, dyrs);
	bool computeAstrometryFlag = (flags&ProperMotion) && (pmra || pmdec);

	if ((flags&AbsoluteMagnitude) && s->getPlx())
		// should use Plx from getPlx because Plx can change with time, but not absolute magnitude
		oss << QString("%1: %2").arg(q_("Absolute Magnitude")).arg(getVMagnitude(core)+5.*(1.+std::log10(0.001*s->getPlx())), 0, 'f', 2) << "<br />";

	if (flags&Extra)
	{
		oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(getBV(), 'f', 2)) << "<br />";

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

				oss << QString("%1: <b>%2</b>%3<b>%4</b> (%5: %6)").arg(q_("Magnitude range"), QString::number(maxVMag, 'f', 2), QChar(0x00F7), minStr, q_("Photometric system"), photoVSys) << "<br />";
			}
		}
	}
	
	oss << getCommonInfoString(core, flags);

	// kinda impossible for both pm to be exactly 0, so they must just be missing
	if (computeAstrometryFlag)
	{
		float pa = std::atan2(pmra, pmdec)*M_180_PIf;
		if (pa<0)
			pa += 360.f;
		oss << QString("%1: %2 %3 %4 %5°").arg(q_("Proper motion"),
							QString::number(std::sqrt(pmra * pmra + pmdec * pmdec), 'f', 2),
							qc_("mas/yr", "milliarc second per year"),
							qc_("towards", "into the direction of"),
							QString::number(pa, 'f', 1)) << "<br />";
		oss << QString("%1: %2 %3 (%4)").arg(q_("Proper motions by axes"),
							QString::number(pmra, 'f', 2),
							QString::number(pmdec, 'f', 2),
							qc_("mas/yr", "milliarc second per year")) << "<br />";
	}

	// kinda impossible for both parallax and parallax_err to be exactly 0, so they must just be missing
	if (flags&Distance)
	{
		// do parallax SNR cut because we are calculating distance, inverse parallax is bad for >20% uncertainty
		if ((Plx!=0) && (PlxErr!=0) & (Plx/PlxErr > 5.))
		{
			//TRANSLATORS: Unit of measure for distance - Light Years
			QString ly = qc_("ly", "distance");
			double distance = PARSEC_LY * 1000. / Plx;
			oss << QString("%1: %2%3%4 %5").arg(q_("Distance"), QString::number(distance, 'f', 2), QChar(0x00B1), QString::number(distance * PlxErr / Plx, 'f', 2), ly) << "<br />";
		}
		if ((Plx!=0) && (PlxErr!=0))  // as long as having parallax, display it (but not neccessarily displaying inverse parallax)
		{
			QString plx = q_("Parallax");
			if (PlxErr>0.f)
				oss <<  QString("%1: %2%3%4 ").arg(plx, QString::number(Plx, 'f', 3), QChar(0x00B1), QString::number(PlxErr, 'f', 3));
			else
				oss << QString("%1: %2 ").arg(plx, QString::number(Plx, 'f', 3));
			oss  << qc_("mas", "parallax") << "<br />";
		}
		oss << getExtraInfoStrings(Distance).join("");
	}

	if (flags&Extra)
	{
		const double vEpoch = StarMgr::getGcvsEpoch(star_id);
		const double vPeriod = StarMgr::getGcvsPeriod(star_id);
		const int vMm = StarMgr::getGcvsMM(star_id);
		const QString sType = StarMgr::getGcvsSpectralType(star_id);

		QString stype = getObjectType();
		bool ebsFlag = stype.contains("eclipsing binary system");

		if (!sType.isEmpty())
			oss << QString("%1: %2").arg(q_("Spectral Type"), sType) << "<br />";

		if (vPeriod>0.)
			oss << QString("%1: %2 %3").arg(q_("Period")).arg(vPeriod).arg(qc_("days", "duration")) << "<br />";

		if (vEpoch>0. && vPeriod>0.)
		{
			// Calculate next minimum or maximum light
			double vsEpoch = 2400000+vEpoch;
			double npDate = vsEpoch + vPeriod * ::floor(1.0 + (core->getJD() - vsEpoch)/vPeriod);
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
	}

	oss << getSolarLunarInfoString(core, flags);

	StelObject::postProcessInfoString(str, flags);

	return str;
}

QString StarWrapper3::getID(void) const
{
	return QString("Gaia DR3 %1").arg(s->getGaia());
}

QString StarWrapper3::getObjectType() const
{
	StarId star_id =  s->getGaia();

	const QString varType = StarMgr::getGcvsVariabilityType(star_id);
	QString startype = N_("star");
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
		return startype;
}

QString StarWrapper3::getObjectTypeI18n() const
{
	QString stypefinal, stype = getObjectType();
	StarId star_id =  s->getGaia();

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

QString StarWrapper3::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	StarId star_id = s->getGaia();
	const QString varType = StarMgr::getGcvsVariabilityType(star_id);

	if (flags&CatalogNumber)
	{
		oss << "<h2>";

		QStringList designations;

		const QString varSciName = StarMgr::getGcvsDesignation(star_id);
		if (!varSciName.isEmpty())
			designations.append(varSciName);

		designations.append(QString("Gaia DR3 %1").arg(star_id));

		oss << designations.join(" - ");
		oss << "</h2>";
	}

	if (flags&ObjectType)
	{
		const QString objectTypeI18nStr = getObjectTypeI18n();
		if (!varType.isEmpty())
			oss << QString("%1: <b>%2</b> (%3)").arg(q_("Type"), objectTypeI18nStr, varType) << "<br />";
		else
			oss << QString("%1: <b>%2</b>").arg(q_("Type"), objectTypeI18nStr) << "<br />";
	}

	oss << getMagnitudeInfoString(core, flags, 2);

	if (flags&Extra)
	{
		oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(getBV(), 'f', 2)) << "<br />";

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

				oss << QString("%1: <b>%2</b>%3<b>%4</b> (%5: %6)").arg(q_("Magnitude range"), QString::number(maxVMag, 'f', 2), QChar(0x00F7), minStr, q_("Photometric system"), photoVSys) << "<br />";
			}
		}
	}
	
	oss << getCommonInfoString(core, flags);

	if (flags&Extra)
	{
		const double vEpoch = StarMgr::getGcvsEpoch(star_id);
		const double vPeriod = StarMgr::getGcvsPeriod(star_id);
		const int vMm = StarMgr::getGcvsMM(star_id);
		const QString sType = StarMgr::getGcvsSpectralType(star_id);

		QString stype = getObjectType();
		bool ebsFlag = stype.contains("eclipsing binary system");

		if (!sType.isEmpty())
			oss << QString("%1: %2").arg(q_("Spectral Type"), sType) << "<br />";

		if (vPeriod>0.)
			oss << QString("%1: %2 %3").arg(q_("Period")).arg(vPeriod).arg(qc_("days", "duration")) << "<br />";

		if (vEpoch>0. && vPeriod>0.)
		{
			// Calculate next minimum or maximum light
			double vsEpoch = 2400000+vEpoch;
			double npDate = vsEpoch + vPeriod * ::floor(1.0 + (core->getJD() - vsEpoch)/vPeriod);
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
	}

	oss << getSolarLunarInfoString(core, flags);

	StelObject::postProcessInfoString(str, flags);

	return str;
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
