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
#include "StelLocaleMgr.hpp"
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
		oss << getB_VInfoString(getBV()) << "<br />";
	
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
	StarId star_id = getStarId();

	QStringList starTypes;
	// This returns varstartype or "star":
	QString varstartype=StarWrapper::getObjectType();

	const int wdsObs = StarMgr::getWdsLastObservation(star_id);

	if (s->getComponentIds() || wdsObs>0)
		starTypes.append(N_("double star"));
	if (varstartype!="star")
		starTypes.append(varstartype);
	if (starTypes.isEmpty())
		return N_("star");
	else return starTypes.join(", ");
}

QString StarWrapper1::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	// rtl tracks the right-to-left status of the text in the current position.
	const bool rtl = StelApp::getInstance().getLocaleMgr().isSkyRTL();
	QString str;
	QTextStream oss(&str);
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));
	Q_UNUSED(az_app)

	StarId star_id = getStarId();

	const QString varType = StarMgr::getGcvsVariabilityType(star_id);
	const QString objType = StarMgr::convertToOjectTypes(s->getObjType());
	const int wdsObs = StarMgr::getWdsLastObservation(star_id);
	const float wdsPA = StarMgr::getWdsLastPositionAngle(star_id);
	const float wdsSep = StarMgr::getWdsLastSeparation(star_id);

	if ((flags&Name) || (flags&CatalogNumber))
		oss << (rtl ? "<h2 dir=\"rtl\">" : "<h2 dir=\"ltr\">");

	const QString commonNameI18 = StarMgr::getCommonNameI18n(star_id);
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

	QString objectTypeI18nStr = getObjectTypeI18n();
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

	if (flags&Extra) // B-V, variable range
	{
		oss << getB_VInfoString(getBV());
		oss << getVariabilityRangeInfoString(core, flags);
	}

	oss << getCommonInfoString(core, flags);

	oss << getDistanceInfoString(core, flags, Plx, PlxErr);

	oss << getProperMotionInfoString(core, flags, pmra, pmdec);

	if ((flags&Velocity) && RadialVel)
	{
		// TRANSLATORS: Unit of measure for speed - kilometers per second
		QString kms = qc_("km/s", "speed");
		oss << QString("%1: %2 %3").arg(q_("Radial velocity"), QString::number(RadialVel, 'f', 1), kms) << "<br />";
	}

	if (flags&Extra)
	{

		const QString specSW1 = (s->getSpInt() ? StarMgr::convertToSpectralType(s->getSpInt()) : QString());
		oss << getGcvsDataInfoString(core, star_id, specSW1);

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
									    // TRANSLATORS: This is about the angular separation between components of a binary star.
									    q_("Component separation"),
									    QString::number((binary_sep>0.f) ? binary_sep: wdsSep, 'f', 3),
									    StelUtils::decDegToDmsStr(((binary_sep>0.f) ? binary_sep: wdsSep)/3600.f),
									    (binary_sep>0.f) ? qc_("on date", "coordinates for current epoch"): QString::number(wdsObs)) << "<br />";
				else
					// TRANSLATORS: This is about the angular separation between components of a binary star.
					oss << QString("%1 (%3): %2\"").arg(q_("Component separation"), QString::number(wdsSep, 'f', 3), QString::number(wdsObs)) << "<br />";
			}
		}
	}

	oss << getSolarLunarInfoString(core, flags);

	StelObject::postProcessInfoString(str, flags);

	return str;
}

QVariantMap StarWrapper1::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StarWrapper::getInfoMap(core);
	StarId star_id=getStarId();
	const int wdsObs = StarMgr::getWdsLastObservation(star_id);

	map.insert("star-type", (s->getComponentIds() || wdsObs>0) ? "double-star" : "star");

	if (s->getPlx())
	{
		map.insert("parallax", 0.001*s->getPlx());
		map.insert("absolute-mag", getVMagnitude(core)+5.f*(std::log10(0.001*s->getPlx())));
		map.insert("distance-ly", (AU/(SPEED_OF_LIGHT*86400*365.25)) / (s->getPlx()*((0.001/3600)*(M_PI/180))));
	}

	if (s->getSpInt())
		map.insert("spectral-class", StarMgr::convertToSpectralType(s->getSpInt()));

	return map;
}

#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
QString StarWrapper1::getNarration(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	StarId star_id = getStarId();

	const QString varType = StarMgr::getGcvsVariabilityType(star_id);
	const QString objType = StarMgr::convertToOjectTypes(s->getObjType());
	const int wdsObs = StarMgr::getWdsLastObservation(star_id);
	const float wdsPA = StarMgr::getWdsLastPositionAngle(star_id);
	const float wdsSep = StarMgr::getWdsLastSeparation(star_id);

	const QString commonNameI18 = StarMgr::getCommonNameI18n(star_id);
	const QString culturalInfoName=StarMgr::getCulturalScreenLabel(star_id);
	const QString sciName = StarMgr::getSciDesignation(star_id);
	const QString sciExtraName = StarMgr::getSciExtraDesignation(star_id);
	const QString varSciName = StarMgr::getGcvsDesignation(star_id);
	const QString wdsSciName = StarMgr::getWdsDesignation(star_id);

	QStringList designations;
	if (!sciName.isEmpty())
		designations.append(sciName.split(" - "));
	if (!sciExtraName.isEmpty())
		designations.append(sciExtraName.split(" - "));
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
		designations.append(crossIndexData.split(" - "));

	if (s->getGaia())
		designations.append(QString("Gaia DR3 %1").arg(s->getGaia()));

	if (!wdsSciName.isEmpty() && !sciName.contains(wdsSciName, Qt::CaseInsensitive))
		designations.append(wdsSciName);

	designations.removeDuplicates();

	if (flags&Name) // NAME
	{
		QString allNames = culturalInfoName;

		// TODO: Finalize appearance of cultural Names
		if (culturalInfoName.isEmpty())
			allNames = commonNameI18;

		oss << allNames << ". ";
	}

	if (flags&CatalogNumber) // Designations. TBD: KEEP HOW MANY? For now, two:
	{
		oss << StelUtils::narrateGreekChars(designations.sliced(0, qMin(2, designations.length())) .join(", ")) << ". ";

		QStringList extraCat=getExtraInfoStrings(CatalogNumber);
		if (!extraCat.isEmpty())
			oss << q_("Additional catalog numbers: ") << extraCat.sliced(0, qMin(2, designations.length())) .join(", ") << ". ";
	}

	QString objectTypeI18nStr = getObjectTypeI18n();

	if (flags&ObjectType)
	{
		QString descType=qc_("This star is described as ", "object narration");
		QStringList stypes;
		if (!objType.isEmpty())
			stypes.append(objType);
		if (!varType.isEmpty())
			stypes.append(varType);
		if (stypes.size()>0)
			oss << QString("%1: %2 (%3). ").arg(descType, objectTypeI18nStr, stypes.join(", "));
		else
			oss << QString("%1: %2. ").arg(descType, objectTypeI18nStr);

		//	oss << getExtraInfoStrings(flags&ObjectType).join("");
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
	if (flags&Magnitude)
		oss << getMagnitudeNarration(core, InfoStringGroupFlags::Magnitude, 2, magOffset);

	if (flags&AbsoluteMagnitude && (s->getPlx())) // should use Plx from getPlx because Plx can change with time, but not absolute magnitude
		oss << QString(" %1  %2. ").arg(qc_("with absolute Magnitude of", "object narration"), StelUtils::narrateDecimal(getVMagnitude(core)+5.*(1.+std::log10(0.001*s->getPlx())), 2));
	else
		oss << ". ";

	if (flags&Extra)
	{
		oss << getB_VNarration(getBV()) << ". ";
		oss << getVariabilityRangeNarration(core, flags);
	}

	//InfoStringGroup alreadyProcessed=StelObject::IAUConstellation | StelObject::CulturalConstellation;
	oss << getCommonNarration(core, flags); // & (~alreadyProcessed));

	oss << getDistanceNarration(core, flags, Plx, PlxErr);

	oss << getProperMotionNarration(core, flags, pmra, pmdec);

	if ((flags&Velocity) && RadialVel)
		oss << QString(qc_("Its radial velocity is %1 kilometers per second.", "object narration")).arg(StelUtils::narrateDecimal(RadialVel, 1)) + " ";

	if (flags&Extra)
	{
		const QString specSW1 = (s->getSpInt() ? StarMgr::convertToSpectralType(s->getSpInt()) : QString());
		oss << getGcvsDataNarration(core, star_id, specSW1);

		if ((wdsObs>0) || (binary_sep>0.f))  // either have a WDS observation or a separation modelled by the binary orbit
		{
			// use separation and position angle from the binary orbit if available
			oss << QString(qc_("Its position angle as given for %1 is %2 degrees.", "object narration"))
			       .arg((binary_sep>0.f) ? qc_("current date", "coordinates for current epoch"): StelUtils::narrateDecimal(wdsObs),
					StelUtils::narrateDecimal((binary_sep>0.f) ? binary_pa : wdsPA, 2)) + " ";
			if (wdsSep>0.f && wdsSep<999.f) // A spectroscopic binary or not?
			{
				if (wdsSep>60.f) // A wide binary star?
					// TRANSLATORS: This is about the angular separation between components of a binary star.
					oss << QString(qc_("Its component separation for %1 is %2 arc-seconds (%3). ", "object narration"))
					       .arg((binary_sep>0.f) ? qc_("current date", "coordinates for current epoch"): StelUtils::narrateDecimal(wdsObs, 1),
						    StelUtils::narrateDecimal((binary_sep>0.f) ? binary_sep: wdsSep, 2),
						    StelUtils::decDegToDmsNarration(((binary_sep>0.f) ? binary_sep: wdsSep)/3600.f));
				else
					// TRANSLATORS: This is about the angular separation between components of a binary star.
					oss << QString(qc_("Its component separation for %1 is %2 arc-seconds. ", "object narration"))
					       .arg(StelUtils::narrateDecimal(wdsObs, 1), StelUtils::narrateDecimal(wdsSep, 2));
				oss << " ";
			}
		}
	}

	oss << getSolarLunarNarration(core, flags);

	return str;
}
#endif

QString StarWrapper2::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	StarId star_id = getStarId();
	const QString commonNameI18 = StarMgr::getCommonNameI18n(star_id);
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

	if ((flags&AbsoluteMagnitude) && s->getPlx())
		// should use Plx from getPlx because Plx can change with time, but not absolute magnitude
		oss << QString("%1: %2").arg(q_("Absolute Magnitude")).arg(getVMagnitude(core)+5.*(1.+std::log10(0.001*s->getPlx())), 0, 'f', 2) << "<br />";

	if (flags&Extra)
	{
		oss << getB_VInfoString(getBV());
		oss << getVariabilityRangeInfoString(core, flags);
	}
	
	oss << getCommonInfoString(core, flags);

	oss << getProperMotionInfoString(core, flags, pmra, pmdec);

	oss << getDistanceInfoString(core, flags, Plx, PlxErr);

	if (flags&Extra)
		oss << getGcvsDataInfoString(core, star_id);

	oss << getSolarLunarInfoString(core, flags);

	StelObject::postProcessInfoString(str, flags);

	return str;
}

#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
QString StarWrapper2::getNarration(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	StarId star_id = getStarId();
	const QString commonNameI18 = StarMgr::getCommonNameI18n(star_id);
	const QString culturalInfoName=StarMgr::getCulturalInfoLabel(star_id);
	const QString sciName = StarMgr::getSciDesignation(star_id);
	const QString sciExtraName = StarMgr::getSciExtraDesignation(star_id);
	const QString varSciName = StarMgr::getGcvsDesignation(star_id);
	const QString varType = StarMgr::getGcvsVariabilityType(star_id);

	QStringList designations;
	if (!sciName.isEmpty())
		designations.append(sciName.split(" - "));
	if (!sciExtraName.isEmpty())
		designations.append(sciExtraName.split(" - "));
	if (!varSciName.isEmpty() && !sciName.contains(varSciName, Qt::CaseInsensitive))
		designations.append(varSciName);

	const QString crossIndexData = StarMgr::getCrossIdentificationDesignations(QString::number(s->getGaia()));
	if (!crossIndexData.isEmpty())
		designations.append(crossIndexData.split(" - "));

	designations.append(QString("Gaia DR3 %1").arg(s->getGaia()));

	designations.removeDuplicates();
	if (flags&Name)
	{
		QString commonNames;
		if (!commonNameI18.isEmpty())
			commonNames = commonNameI18;

		// TODO: Finalize appearance of cultural Names
		if (!culturalInfoName.isEmpty() && StarMgr::getFlagAdditionalNames())
			oss << culturalInfoName << ". ";

		if (!commonNames.isEmpty())
			oss << commonNames << ". ";
	}
	if (flags&CatalogNumber) // Use only 2 designations
		oss << StelUtils::narrateGreekChars(designations.sliced(0, qMin(2, designations.length())) .join(", "));

	if (flags&ObjectType)
	{
		const QString objectTypeI18nStr = getObjectTypeI18n();
		if (!varType.isEmpty())
			oss << QString(qc_("Its type is %1 with variability type %2.", "object narration")).arg(objectTypeI18nStr, varType);
		else
			oss << QString(qc_("Its type is %1.", "object narration")).arg(objectTypeI18nStr);
		oss << ". ";
	}

	if (flags&Magnitude)
		oss << getMagnitudeNarration(core, InfoStringGroupFlags::Magnitude, 2);

	double RA, DEC, pmra, pmdec, Plx, RadialVel;
	double PlxErr = s->getPlxErr();
	float dyrs = static_cast<float>(core->getJDE()-STAR_CATALOG_JDEPOCH)/365.25;
	s->getFull6DSolution(RA, DEC, Plx, pmra, pmdec, RadialVel, dyrs);

	if ((flags&AbsoluteMagnitude) && s->getPlx())
		// should use Plx from getPlx because Plx can change with time, but not absolute magnitude
		oss << QString("%1: %2. ").arg(q_("Absolute Magnitude")).arg(getVMagnitude(core)+5.*(1.+std::log10(0.001*s->getPlx())), 0, 'f', 2);

	if (flags&Extra)
	{
		oss << getB_VNarration(getBV()) << ". ";
		oss << getVariabilityRangeNarration(core, flags);
	}

	//InfoStringGroup alreadyProcessed=StelObject::IAUConstellation | StelObject::CulturalConstellation;
	oss << getCommonNarration(core, flags); // & (~alreadyProcessed));

	oss << getProperMotionNarration(core, flags, pmra, pmdec);

	oss << getDistanceNarration(core, flags, Plx, PlxErr);

	if (flags&Extra)
		oss << getGcvsDataNarration(core, star_id);

	oss << getSolarLunarNarration(core, flags);

	return str;
}
#endif

QString StarWrapper3::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	StarId star_id = getStarId();
	const QString varType = StarMgr::getGcvsVariabilityType(star_id);

	if (flags&CatalogNumber) // Use only 2 designations
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

	if (flags&Extra) // B-V, variable range
	{
		oss << getB_VInfoString(getBV());
		oss << getVariabilityRangeInfoString(core, flags);
	}

	oss << getCommonInfoString(core, flags);

	if (flags&Extra)  // Spectral Type, period
		oss << getGcvsDataInfoString(core, star_id);

	oss << getSolarLunarInfoString(core, flags);

	StelObject::postProcessInfoString(str, flags);

	return str;
}

#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
QString StarWrapper3::getNarration(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	StarId star_id = getStarId();
	const QString varType = StarMgr::getGcvsVariabilityType(star_id);

	if (flags&CatalogNumber)
	{
		QStringList designations;

		const QString varSciName = StarMgr::getGcvsDesignation(star_id);
		if (!varSciName.isEmpty())
			designations.append(varSciName);

		designations.append(QString("Gaia DR3 %1").arg(star_id));

		oss << StelUtils::narrateGreekChars(designations.sliced(0, qMax(2, designations.length())) .join(", "));
	}

	if (flags&ObjectType)
	{
		const QString objectTypeI18nStr = getObjectTypeI18n();
		if (!varType.isEmpty())
			oss << QString(qc_("Its type is %1 with variability type %2.", "object narration")).arg(objectTypeI18nStr, varType);
		else
			oss << QString(qc_("Its type is %1.", "object narration")).arg(objectTypeI18nStr);
		oss << ". ";
	}

	if (flags&Magnitude)
		oss << getMagnitudeNarration(core, InfoStringGroupFlags::Magnitude, 2);

	if (flags&Extra) // B-V, variable range
	{
		oss << getB_VNarration(getBV()) << ". ";
		oss << getVariabilityRangeNarration(core, flags);
	}
	
	oss << getCommonNarration(core, flags);

	if (flags&Extra) // Spectral Type, period
		oss << getGcvsDataNarration(core, star_id);

	oss << getSolarLunarNarration(core, flags);

	return str;
}
#endif

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
