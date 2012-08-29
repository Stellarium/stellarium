/*
 * Stellarium
 * Copyright (C) 2006 Johannes Gajdosik
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


#include "StelObject.hpp"

#include "StelCore.hpp"
#include "StelProjector.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelSkyDrawer.hpp"
#include "RefractionExtinction.hpp"
#include "StelLocation.hpp"

#include <QRegExp>
#include <QDebug>

Vec3d StelObject::getEquinoxEquatorialPos(const StelCore* core) const
{
	return core->j2000ToEquinoxEqu(getJ2000EquatorialPos(core));
}

// Get observer local sideral coordinate
Vec3d StelObject::getSideralPosGeometric(const StelCore* core) const
{
	return Mat4d::zrotation(-core->getLocalSideralTime())* getEquinoxEquatorialPos(core);
}

// Get observer local sidereal coordinates, deflected by refraction
Vec3d StelObject::getSideralPosApparent(const StelCore* core) const
{
	Vec3d v=getAltAzPosApparent(core);
	v = core->altAzToEquinoxEqu(v, StelCore::RefractionOff);
	return Mat4d::zrotation(-core->getLocalSideralTime())*v;
}

Vec3d StelObject::getAltAzPosGeometric(const StelCore* core) const
{
	return core->j2000ToAltAz(getJ2000EquatorialPos(core), StelCore::RefractionOff);
}

// Get observer-centered alt/az position
Vec3d StelObject::getAltAzPosApparent(const StelCore* core) const
{
	return core->j2000ToAltAz(getJ2000EquatorialPos(core), StelCore::RefractionOn);
}

// Get observer-centered alt/az position
Vec3d StelObject::getAltAzPosAuto(const StelCore* core) const
{
	return core->j2000ToAltAz(getJ2000EquatorialPos(core));
}

// Get observer-centered galactic position
Vec3d StelObject::getJ2000GalacticPos(const StelCore *core) const
{
	return core->j2000ToGalactic(getJ2000EquatorialPos(core));
}

float StelObject::getVMagnitude(const StelCore* core, bool withExtinction) const 
{
	Q_UNUSED(core);
	Q_UNUSED(withExtinction);
	return 99;
}

// Format the positional info string contain J2000/of date/altaz/hour angle positions for the object
QString StelObject::getPositionInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	bool withAtmosphere=core->getSkyDrawer()->getFlagHasAtmosphere();
	QString res;
	if (flags&RaDecJ2000)
	{
		double dec_j2000, ra_j2000;
		StelUtils::rectToSphe(&ra_j2000,&dec_j2000,getJ2000EquatorialPos(core));
		res += q_("RA/DE (J2000): %1/%2").arg(StelUtils::radToHmsStr(ra_j2000,true), StelUtils::radToDmsStr(dec_j2000,true)) + "<br>";
	}

	if (flags&RaDecOfDate)
	{
		double dec_equ, ra_equ;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,getEquinoxEquatorialPos(core));
		res += q_("RA/DE (of date): %1/%2").arg(StelUtils::radToHmsStr(ra_equ), StelUtils::radToDmsStr(dec_equ)) + "<br>";
	}

	if (flags&GalCoordJ2000)
	{
		double glong, glat;
		StelUtils::rectToSphe(&glong, &glat, getJ2000GalacticPos(core));
		res += q_("Galactic longitude/latitude (J2000): %1/%2").arg(StelUtils::radToDmsStr(glong,true), StelUtils::radToDmsStr(glat,true)) + "<br>";
	}

	if (flags&HourAngle)
	{
		double dec_sidereal, ra_sidereal;
		StelUtils::rectToSphe(&ra_sidereal,&dec_sidereal,getSideralPosGeometric(core));
		ra_sidereal = 2.*M_PI-ra_sidereal;
		if (withAtmosphere)
		{
		    res += q_("Hour angle/DE: %1/%2").arg(StelUtils::radToHmsStr(ra_sidereal), StelUtils::radToDmsStr(dec_sidereal)) + " " + q_("(geometric)") + "<br>";
		    StelUtils::rectToSphe(&ra_sidereal,&dec_sidereal,getSideralPosApparent(core));
		    ra_sidereal = 2.*M_PI-ra_sidereal;
		    res += q_("Hour angle/DE: %1/%2").arg(StelUtils::radToHmsStr(ra_sidereal), StelUtils::radToDmsStr(dec_sidereal)) + " " + q_("(apparent)") + "<br>";
		}
		else
		    res += q_("Hour angle/DE: %1/%2").arg(StelUtils::radToHmsStr(ra_sidereal), StelUtils::radToDmsStr(dec_sidereal)) + " " + "<br>";
	}

	if (flags&AltAzi)
	{
		// calculate alt az
		double az,alt;
		StelUtils::rectToSphe(&az,&alt,getAltAzPosGeometric(core));
		az = 3.*M_PI - az;  // N is zero, E is 90 degrees
		if (az > M_PI*2)
			az -= M_PI*2;
		if (withAtmosphere)
		{
		    res += q_("Az/Alt: %1/%2").arg(StelUtils::radToDmsStr(az), StelUtils::radToDmsStr(alt)) + " " + q_("(geometric)") + "<br>";

		    StelUtils::rectToSphe(&az,&alt,getAltAzPosApparent(core));
		    az = 3.*M_PI - az;  // N is zero, E is 90 degrees
		    if (az > M_PI*2)
			    az -= M_PI*2;
		    res += q_("Az/Alt: %1/%2").arg(StelUtils::radToDmsStr(az), StelUtils::radToDmsStr(alt)) + " " + q_("(apparent)") + "<br>";
		}
		else
		    res += q_("Az/Alt: %1/%2").arg(StelUtils::radToDmsStr(az), StelUtils::radToDmsStr(alt)) + " " + "<br>";
	}
	return res;
}

// Apply post processing on the info string
void StelObject::postProcessInfoString(QString& str, const InfoStringGroup& flags) const
{
	// chomp trailing line breaks
	str.replace(QRegExp("<br(\\s*/)?>\\s*$"), "");

	if (flags&PlainText)
	{
		str.replace("<b>", "");
		str.replace("</b>", "");
		str.replace("<h2>", "");
		str.replace("</h2>", "\n");
		str.replace("<br>", "\n");
	}
	else
	{
		str.prepend(QString("<font color=%1>").arg(StelUtils::vec3fToHtmlColor(getInfoColor())));
		str.append(QString("</font>"));
	}
}

