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

#include <QTextStream>
#include <limits>

template<typename T> inline bool isNan(T value)
{
	return value != value;
}

template<typename T> inline bool isInf(T value)
{
	return std::numeric_limits<T>::has_infinity && value == std::numeric_limits<T>::infinity();
}

namespace BigStarCatalogExtension {

QString StarWrapperBase::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Extra1)
	{
		oss << q_("Type: <b>%1</b>").arg(q_("star")) << "<br />";
	}

	if (flags&Magnitude)
	{
	    oss << q_("Magnitude: <b>%1</b> (B-V: %2)").arg(QString::number(getVMagnitude(core, false), 'f', 2), QString::number(getBV(), 'f', 2)) << "<br>";
            if (core->getSkyDrawer()->getFlagHasAtmosphere())
            {
                oss << q_("Apparent Magnitude: <b>%1</b> (by extinction)").arg(QString::number(getVMagnitude(core, true), 'f', 2)) << "<br>";
            }
        }
        oss << getPositionInfoString(core, flags);

	StelObject::postProcessInfoString(str, flags);

	return str;
}


QString StarWrapper1::getEnglishName(void) const
{
	if (s->hip)
		return QString("HIP %1").arg(s->hip);
	return StarWrapperBase::getEnglishName();
}

QString StarWrapper1::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;

	QTextStream oss(&str);
	if (s->hip)
	{
		if ((flags&Name) || (flags&CatalogNumber))
			oss << "<h2>";

		const QString commonNameI18 = StarMgr::getCommonName(s->hip);
		const QString sciName = StarMgr::getSciName(s->hip);

		bool nameWasEmpty=true;
		if (flags&Name)
		{
			if (commonNameI18!="" || sciName!="")
			{
				oss << commonNameI18 << (commonNameI18 == "" ? "" : " ");
				if (commonNameI18!="" && sciName!="")
					oss << "(";
				oss << (sciName=="" ? "" : sciName);
				if (commonNameI18!="" && sciName!="")
					oss << ")";
				nameWasEmpty=false;
			}
		}
		if ((flags&CatalogNumber) && (flags&Name) && !nameWasEmpty)
			oss << " - ";

		if (flags&CatalogNumber || (nameWasEmpty && (flags&Name)))
			oss << "HIP " << s->hip;
		if (s->componentIds)
			oss << " " << StarMgr::convertToComponentIds(s->componentIds);

		if ((flags&Name) || (flags&CatalogNumber))
			oss << "</h2>";
	}

	if (flags&Extra1)
	{
		if (s->componentIds)
			oss << q_("Type: <b>%1</b>").arg(q_("double star")) << "<br />";
		else
			oss << q_("Type: <b>%1</b>").arg(q_("star")) << "<br />";
	}

	if (flags&Magnitude)
	{
	    if (core->getSkyDrawer()->getFlagHasAtmosphere())
                oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>. B-V: <b>%3</b>)").arg(QString::number(getVMagnitude(core, false), 'f', 2),
											      QString::number(getVMagnitude(core, true), 'f', 2),
											      QString::number(s->getBV(), 'f', 2)) << "<br>";
	    else
		oss << q_("Magnitude: <b>%1</b> (B-V: <b>%2</b>)").arg(QString::number(getVMagnitude(core, false), 'f', 2),
								       QString::number(s->getBV(), 'f', 2)) << "<br>";
	}

	if ((flags&AbsoluteMagnitude) && s->plx && !isNan(s->plx) && !isInf(s->plx))
		oss << q_("Absolute Magnitude: %1").arg(getVMagnitude(core, false)+5.*(1.+std::log10(0.00001*s->plx)), 0, 'f', 2) << "<br>";

	oss << getPositionInfoString(core, flags);

	if (s->spInt && flags&Extra1)
	{
		oss << q_("Spectral Type: %1").arg(StarMgr::convertToSpectralType(s->spInt)) << "<br>";
	}

	if ((flags&Distance) && s->plx && !isNan(s->plx) && !isInf(s->plx))
		oss << q_("Distance: %1 Light Years").arg((AU/(SPEED_OF_LIGHT*86400*365.25)) / (s->plx*((0.00001/3600)*(M_PI/180))), 0, 'f', 2) << "<br>";

	if (s->plx && flags&Extra2)
		oss << q_("Parallax: %1\"").arg(0.00001*s->plx, 0, 'f', 5) << "<br>";

	StelObject::postProcessInfoString(str, flags);

	return str;
}

StelObjectP Star1::createStelObject(const SpecialZoneArray<Star1> *a,
									const SpecialZoneData<Star1> *z) const {
  return StelObjectP(new StarWrapper1(a,z,this), true);
}

StelObjectP Star2::createStelObject(const SpecialZoneArray<Star2> *a,
									const SpecialZoneData<Star2> *z) const {
  return StelObjectP(new StarWrapper2(a,z,this), true);
}

StelObjectP Star3::createStelObject(const SpecialZoneArray<Star3> *a,
									const SpecialZoneData<Star3> *z) const {
  return StelObjectP(new StarWrapper3(a,z,this), true);
}


} // namespace BigStarCatalogExtension

