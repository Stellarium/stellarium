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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "StarWrapper.hpp"
#include "ZoneArray.hpp"

#include "StelUtils.hpp"
#include "Translator.hpp"

#include <QTextStream>

namespace BigStarCatalogExtension {

wstring StarWrapperBase::getInfoString(const Navigator *nav) const {
  const Vec3d j2000_pos = getObsJ2000Pos(nav);
  double dec_j2000, ra_j2000;
  StelUtils::rect_to_sphe(&ra_j2000,&dec_j2000,j2000_pos);
  const Vec3d equatorial_pos = nav->j2000_to_earth_equ(j2000_pos);
  double dec_equ, ra_equ;
  StelUtils::rect_to_sphe(&ra_equ,&dec_equ,equatorial_pos);
  QString str;
  QTextStream oss(&str);
  oss.setRealNumberNotation(QTextStream::FixedNotation);
  oss.setRealNumberPrecision(2);
  oss << q_("Magnitude: ") << getMagnitude(nav) << " B-V: " << getBV() << endl;
  oss << q_("J2000") << " " << q_("RA/DE: ") << StelUtils::radToHmsStr(ra_j2000,true)
		  << "/" << StelUtils::radToDmsStr(dec_j2000,true) << endl;
  oss << q_("Equ of date") << " " << q_("RA/DE: ") << StelUtils::radToHmsStr(ra_equ)
		  << "/" << StelUtils::radToDmsStr(dec_equ) << endl;

    // calculate alt az
  double az,alt;
  StelUtils::rect_to_sphe(&az,&alt,nav->earth_equ_to_local(equatorial_pos));
  az = 3*M_PI - az;  // N is zero, E is 90 degrees
  if(az > M_PI*2) az -= M_PI*2;    
  oss << q_("Az/Alt: ") << StelUtils::radToDmsStr(az) << "/" << StelUtils::radToDmsStr(alt) << endl;
  
  return str.toStdWString();
}

wstring StarWrapperBase::getShortInfoString(const Navigator *nav) const
{
	QString str;
	QTextStream oss(&str);
	oss.setRealNumberNotation(QTextStream::FixedNotation);
	oss.setRealNumberPrecision(2);
	oss << q_("Magnitude: ") << getMagnitude(nav);
	return str.toStdWString();
}


string StarWrapper1::getEnglishName(void) const {
  if (s->hip) {
    char buff[64];
    sprintf(buff,"HP %d",s->hip);
    return buff;
  }
  return StarWrapperBase::getEnglishName();
}

wstring StarWrapper1::getInfoString(const Navigator *nav) const {
  const Vec3d j2000_pos = getObsJ2000Pos(nav);
  double dec_j2000, ra_j2000;
  StelUtils::rect_to_sphe(&ra_j2000,&dec_j2000,j2000_pos);
  const Vec3d equatorial_pos = nav->j2000_to_earth_equ(j2000_pos);
  double dec_equ, ra_equ;
  StelUtils::rect_to_sphe(&ra_equ,&dec_equ,equatorial_pos);
  QString str;
  QTextStream oss(&str);
  if (s->hip)
  {
    const wstring commonNameI18 = StarMgr::getCommonName(s->hip);
    const wstring sciName = StarMgr::getSciName(s->hip);
    if (commonNameI18!=L"" || sciName!=L"")
	{
		oss << QString::fromStdWString(commonNameI18) << (commonNameI18 == L"" ? "" : " ");
		if (commonNameI18!=L"" && sciName!=L"") oss << "(";
		oss << (sciName==L"" ? "" : QString::fromStdWString(sciName));
		if (commonNameI18!=L"" && sciName!=L"") oss << ")";
		oss << endl;
    }
    oss << "HP " << s->hip;
    if (s->component_ids)
	{
		oss << " " << StarMgr::convertToComponentIds(s->component_ids).c_str();
    }
    oss << endl;
  }

  oss.setRealNumberNotation(QTextStream::FixedNotation);
  oss.setRealNumberPrecision(2);
  oss << q_("Magnitude: ") << getMagnitude(nav) << " B-V: " << s->getBV() << endl;
  oss << q_("J2000") << " " << q_("RA/DE: ") << StelUtils::radToHmsStr(ra_j2000,true)
		  << "/" << StelUtils::radToDmsStr(dec_j2000,true) << endl;
  
  oss << q_("Equ of date") << " " << q_("RA/DE: ") << StelUtils::radToHmsStr(ra_equ)
		  << "/" << StelUtils::radToDmsStr(dec_equ) << endl;

    // calculate alt az
  double az,alt;
  StelUtils::rect_to_sphe(&az,&alt,nav->earth_equ_to_local(equatorial_pos));
  az = 3*M_PI - az;  // N is zero, E is 90 degrees
  if (az > M_PI*2)
	  az -= M_PI*2;    
  oss << q_("Az/Alt: ") << StelUtils::radToDmsStr(az) << "/" << StelUtils::radToDmsStr(alt) << endl;

  if (s->plx)
  {
		oss.setRealNumberPrecision(5);
		oss << q_("Parallax: ") << (0.00001*s->plx) << endl;
		oss.setRealNumberPrecision(2);
		oss << q_("Distance: ") << (AU/(SPEED_OF_LIGHT*86400*365.25)) / (s->plx*((0.00001/3600)*(M_PI/180)))
        << q_(" Light Years") << endl;
  }

  if (s->sp_int)
  {
    oss << q_("Spectral Type: ") << StarMgr::convertToSpectralType(s->sp_int).c_str() << endl;
  }
  return str.toStdWString();
}


wstring StarWrapper1::getShortInfoString(const Navigator *nav) const
{
	QString str;
	QTextStream oss(&str);
	if (s->hip)
	{
		const wstring commonNameI18 = StarMgr::getCommonName(s->hip);
		const wstring sciName = StarMgr::getSciName(s->hip);
		if (commonNameI18!=L"" || sciName!=L"")
		{
			oss << QString::fromStdWString(commonNameI18) << (commonNameI18 == L"" ? "" : " ");
			if (commonNameI18!=L"" && sciName!=L"") oss << "(";
			oss << (sciName==L"" ? "" : QString::fromStdWString(sciName));
			if (commonNameI18!=L"" && sciName!=L"") oss << ")";
			oss << "  ";
		}
		oss << "HP " << s->hip;
		if (s->component_ids)
		{
			oss << " " << StarMgr::convertToComponentIds(s->component_ids).c_str();
		}
		oss << "  ";
	}
	
	oss.setRealNumberNotation(QTextStream::FixedNotation);
	oss.setRealNumberPrecision(2);
	oss << q_("Magnitude: ") << getMagnitude(nav) << "  ";

	if (s->plx)
	{
		oss << q_("Distance: ") << (AU/(SPEED_OF_LIGHT*86400*365.25)) / (s->plx*((0.00001/3600)*(M_PI/180)))
			<< q_(" Light Years") << "  ";
	}
	
	if (s->sp_int)
	{
		oss << q_("Spectral Type: ") << StarMgr::convertToSpectralType(s->sp_int).c_str();
	}
	return str.toStdWString();
}


StelObjectP Star1::createStelObject(const SpecialZoneArray<Star1> *a,
                                    const SpecialZoneData<Star1> *z) const {
  return StelObjectP(new StarWrapper1(a,z,this));
}

StelObjectP Star2::createStelObject(const SpecialZoneArray<Star2> *a,
                                    const SpecialZoneData<Star2> *z) const {
  return StelObjectP(new StarWrapper2(a,z,this));
}

StelObjectP Star3::createStelObject(const SpecialZoneArray<Star3> *a,
                                    const SpecialZoneData<Star3> *z) const {
  return StelObjectP(new StarWrapper3(a,z,this));
}


} // namespace BigStarCatalogExtension

