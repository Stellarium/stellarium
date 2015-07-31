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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelProjector.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelSkyDrawer.hpp"
#include "RefractionExtinction.hpp"
#include "StelLocation.hpp"
#include "SolarSystem.hpp"
#include "StelModuleMgr.hpp"

#include <QRegExp>
#include <QDebug>
#include <QSettings>

Vec3d StelObject::getEquinoxEquatorialPos(const StelCore* core) const
{
	return core->j2000ToEquinoxEqu(getJ2000EquatorialPos(core));
}

// Get observer local sidereal coordinate
Vec3d StelObject::getSiderealPosGeometric(const StelCore* core) const
{
	// Hour Angle corrected to Delta-T value
	// TODO: make code readable by calling siderealTime(JD_UT), this should not contain a deltaT in its algorithm.
	//double dt = (core->getDeltaT(core->getJDay())/240.)*M_PI/180.;
	//return Mat4d::zrotation(-core->getLocalSiderealTime()+dt)* getEquinoxEquatorialPos(core);
	// GZ JDfix for 0.14
	return Mat4d::zrotation(-core->getLocalSiderealTime())* getEquinoxEquatorialPos(core);
}

// Get observer local sidereal coordinates, deflected by refraction
Vec3d StelObject::getSiderealPosApparent(const StelCore* core) const
{
	Vec3d v=getAltAzPosApparent(core);
	v = core->altAzToEquinoxEqu(v, StelCore::RefractionOff);
	// Hour Angle corrected to Delta-T value
	// TODO: make code readable by calling siderealTime(JD_UT), this should not contain a deltaT in its algorithm.
	//double dt = (core->getDeltaT(core->getJDay())/240.)*M_PI/180.;
	//return Mat4d::zrotation(-core->getLocalSiderealTime()+dt)*v;
	// GZ JDfix for 0.14
	return Mat4d::zrotation(-core->getLocalSiderealTime())*v;
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
Vec3d StelObject::getGalacticPos(const StelCore *core) const
{
	return core->j2000ToGalactic(getJ2000EquatorialPos(core));
}

float StelObject::getVMagnitude(const StelCore* core) const 
{
	Q_UNUSED(core);
	return 99;
}

float StelObject::getSelectPriority(const StelCore* core) const
{
	float extMag = getVMagnitudeWithExtinction(core);
	if (extMag>15.f)
		extMag=15.f;
	return extMag;
}

float StelObject::getVMagnitudeWithExtinction(const StelCore* core) const
{
	Vec3d altAzPos = getAltAzPosGeometric(core);
	altAzPos.normalize();
	float vMag = getVMagnitude(core);
	// without the test, planets flicker stupidly in fullsky atmosphere-less view.
	if (core->getSkyDrawer()->getFlagHasAtmosphere())
		core->getSkyDrawer()->getExtinction().forward(altAzPos, &vMag);
	return vMag;
}

// Format the positional info string contain J2000/of date/altaz/hour angle positions for the object
QString StelObject::getPositionInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	bool withAtmosphere = core->getSkyDrawer()->getFlagHasAtmosphere();
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();;
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));
	Q_UNUSED(az_app);
	QString cepoch = qc_("on date", "coordinates for current epoch");
	QString res;
	if (flags&RaDecJ2000)
	{
		double dec_j2000, ra_j2000;
		StelUtils::rectToSphe(&ra_j2000,&dec_j2000,getJ2000EquatorialPos(core));
		if (withDecimalDegree)
			res += q_("RA/Dec") + QString(" (J%1): %2/%3").arg(QString::number(2000.f, 'f', 1), StelUtils::radToDecDegStr(ra_j2000,5,false,true), StelUtils::radToDecDegStr(dec_j2000)) + "<br>";
		else
			res += q_("RA/Dec") + QString(" (J%1): %2/%3").arg(QString::number(2000.f, 'f', 1), StelUtils::radToHmsStr(ra_j2000,true), StelUtils::radToDmsStr(dec_j2000,true)) + "<br>";
	}

	if (flags&RaDecOfDate)
	{
		double dec_equ, ra_equ;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,getEquinoxEquatorialPos(core));		
		if (withDecimalDegree)
			res += q_("RA/Dec") + QString(" (%1): %2/%3").arg(cepoch, StelUtils::radToDecDegStr(ra_equ,5,false,true), StelUtils::radToDecDegStr(dec_equ)) + "<br>";
		else
			res += q_("RA/Dec") + QString(" (%1): %2/%3").arg(cepoch, StelUtils::radToHmsStr(ra_equ,true), StelUtils::radToDmsStr(dec_equ,true)) + "<br>";
	}

	if (flags&HourAngle)
	{
		double dec_sidereal, ra_sidereal, ha_sidereal;
		QString hadec;
		StelUtils::rectToSphe(&ra_sidereal,&dec_sidereal,getSiderealPosGeometric(core));
		ra_sidereal = 2.*M_PI-ra_sidereal;
		if (withAtmosphere && (alt_app>-3.0*M_PI/180.0)) // Don't show refracted values much below horizon where model is meaningless.
		{
			StelUtils::rectToSphe(&ra_sidereal,&dec_sidereal,getSiderealPosApparent(core));
			ra_sidereal = 2.*M_PI-ra_sidereal;
			if (withDecimalDegree)
			{
				ha_sidereal = ra_sidereal*12/M_PI;
				if (ha_sidereal>24.)
					ha_sidereal -= 24.;
				hadec = QString("%1h").arg(ha_sidereal, 0, 'f', 5);
				res += q_("Hour angle/DE: %1/%2").arg(hadec, StelUtils::radToDecDegStr(dec_sidereal)) + " " + q_("(apparent)") + "<br>";
			}
			else
				res += q_("Hour angle/DE: %1/%2").arg(StelUtils::radToHmsStr(ra_sidereal,true), StelUtils::radToDmsStr(dec_sidereal,true)) + " " + q_("(apparent)") + "<br>";
		}
		else
		{
			if (withDecimalDegree)
			{
				ha_sidereal = ra_sidereal*12/M_PI;
				if (ha_sidereal>24.)
					ha_sidereal -= 24.;
				hadec = QString("%1h").arg(ha_sidereal, 0, 'f', 5);
				res += q_("Hour angle/DE: %1/%2").arg(hadec, StelUtils::radToDecDegStr(dec_sidereal)) + " " + "<br>";
			}
			else
				res += q_("Hour angle/DE: %1/%2").arg(StelUtils::radToHmsStr(ra_sidereal,true), StelUtils::radToDmsStr(dec_sidereal,true)) + " " + "<br>";
		}
	}

	if (flags&AltAzi)
	{
		// calculate alt az
		double az,alt;
		StelUtils::rectToSphe(&az,&alt,getAltAzPosGeometric(core));
		az = 3.*M_PI - az;  // N is zero, E is 90 degrees
		if (az > M_PI*2)
			az -= M_PI*2;
		if (withAtmosphere && (alt_app>-3.0*M_PI/180.0)) // Don't show refracted altitude much below horizon where model is meaningless.
		{
			if (withDecimalDegree)
				res += q_("Az/Alt: %1/%2").arg(StelUtils::radToDecDegStr(az), StelUtils::radToDecDegStr(alt_app)) + " " + q_("(apparent)") + "<br>";
			else
				res += q_("Az/Alt: %1/%2").arg(StelUtils::radToDmsStr(az,true), StelUtils::radToDmsStr(alt_app,true)) + " " + q_("(apparent)") + "<br>";
		}
		else
		{
			if (withDecimalDegree)
				res += q_("Az/Alt: %1/%2").arg(StelUtils::radToDecDegStr(az), StelUtils::radToDecDegStr(alt)) + " " + "<br>";
			else
				res += q_("Az/Alt: %1/%2").arg(StelUtils::radToDmsStr(az,true), StelUtils::radToDmsStr(alt,true)) + " " + "<br>";
		}
	}

	if (flags&EclipticCoord)
	{
		// N.B. Ecliptical coordinates are particularly earth-bound.
		// It may be OK to have terrestrial ecliptical coordinates of J2000.0 (standard epoch) because those are in practice linked with VSOP XY plane,
		// and because the ecliptical grid of J2000 is also shown for observers on other planets.
		// The formulation here has never computed the true position of any observer planet's orbital plane except for Earth,
		// or ever displayed the coordinates in the observer planet's equivalent to Earth's ecliptical coordinates.
		// As quick test you can observe if in any "Ecliptic coordinate" as seen from e.g. Mars or Jupiter the Sun was ever close to beta=0.

		//double ecl = core->getCurrentPlanet()->getRotObliquity(2451545.0);
		double ecl=GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(2451545.0);
		double ra_equ, dec_equ, lambda, beta;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,getJ2000EquatorialPos(core));
		StelUtils::equToEcl(ra_equ, dec_equ, ecl, &lambda, &beta);
		if (lambda<0) lambda+=2.0*M_PI;
		if (withDecimalDegree)
			res += q_("Ecliptic longitude/latitude") + QString(" (J%1): %2/%3").arg(QString::number(2000.f, 'f', 1), StelUtils::radToDecDegStr(lambda), StelUtils::radToDecDegStr(beta)) + "<br>";
		else
			res += q_("Ecliptic longitude/latitude") + QString(" (J%1): %2/%3").arg(QString::number(2000.f, 'f', 1), StelUtils::radToDmsStr(lambda, true), StelUtils::radToDmsStr(beta, true)) + "<br>";

		if (core->getCurrentPlanet()->getEnglishName()=="Earth")
		{
			ecl = GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(2451545.0);

			StelUtils::rectToSphe(&ra_equ,&dec_equ,getEquinoxEquatorialPos(core));
			StelUtils::equToEcl(ra_equ, dec_equ, ecl, &lambda, &beta);
			if (lambda<0) lambda+=2.0*M_PI;
			if (withDecimalDegree)
				res += q_("Ecliptic longitude/latitude") + QString(" (%1): %2/%3").arg(cepoch, StelUtils::radToDecDegStr(lambda), StelUtils::radToDecDegStr(beta)) + "<br>";
			else
				res += q_("Ecliptic longitude/latitude") + QString(" (%1): %2/%3").arg(cepoch, StelUtils::radToDmsStr(lambda, true), StelUtils::radToDmsStr(beta, true)) + "<br>";
			// GZ Only for now: display epsilon_A, angle between Earth's Axis and ecl. of date.
			if (withDecimalDegree)
				res += q_("Ecliptic obliquity") + QString(" (%1): %2").arg(cepoch, StelUtils::radToDecDegStr(ecl)) + "<br>";
			else
				res += q_("Ecliptic obliquity") + QString(" (%1): %2").arg(cepoch, StelUtils::radToDmsStr(ecl)) + "<br>";
		}
	}

	if (flags&GalacticCoord)
	{
		double glong, glat;
		StelUtils::rectToSphe(&glong, &glat, getGalacticPos(core));
		if (withDecimalDegree)
			res += q_("Galactic longitude/latitude: %1/%2").arg(StelUtils::radToDecDegStr(glong), StelUtils::radToDecDegStr(glat)) + "<br>";
		else
			res += q_("Galactic longitude/latitude: %1/%2").arg(StelUtils::radToDmsStr(glong,true), StelUtils::radToDmsStr(glat,true)) + "<br>";
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
		str.replace(QRegExp("<br(\\s*/)?>"), "\n");
	}
	else
	{
		Vec3f color = getInfoColor();
		StelCore* core = StelApp::getInstance().getCore();
		if (core->isBrightDaylight() && core->getSkyDrawer()->getFlagHasAtmosphere()==true)
		{
			// make info text more readable when atmosphere enabled at daylight.
			color = StelUtils::strToVec3f(StelApp::getInstance().getSettings()->value("color/daylight_text_color", "0.0,0.0,0.0").toString());
		}
		str.prepend(QString("<font color=%1>").arg(StelUtils::vec3fToHtmlColor(color)));
		str.append(QString("</font>"));
	}
}

