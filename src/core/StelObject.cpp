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
#include "LandscapeMgr.hpp"
#include "planetsephems/sidereal_time.h"
#include "planetsephems/precession.h"

#include <QRegExp>
#include <QDebug>
#include <QSettings>

int StelObject::stelObjectPMetaTypeID = qRegisterMetaType<StelObjectP>();

Vec3d StelObject::getEquinoxEquatorialPos(const StelCore* core) const
{
	return core->j2000ToEquinoxEqu(getJ2000EquatorialPos(core), StelCore::RefractionOff);
}

Vec3d StelObject::getEquinoxEquatorialPosApparent(const StelCore* core) const
{
	return core->j2000ToEquinoxEqu(getJ2000EquatorialPos(core), StelCore::RefractionOn);
}

Vec3d StelObject::getEquinoxEquatorialPosAuto(const StelCore* core) const
{
	return core->j2000ToEquinoxEqu(getJ2000EquatorialPos(core), StelCore::RefractionAuto);
}


// Get observer local sidereal coordinate
Vec3d StelObject::getSiderealPosGeometric(const StelCore* core) const
{
	return Mat4d::zrotation(-core->getLocalSiderealTime())* getEquinoxEquatorialPos(core);
}

// Get observer local sidereal coordinates, deflected by refraction
Vec3d StelObject::getSiderealPosApparent(const StelCore* core) const
{
	Vec3d v=getAltAzPosApparent(core); // These already come with refraction!
	v = core->altAzToEquinoxEqu(v, StelCore::RefractionOff);
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
	return core->j2000ToAltAz(getJ2000EquatorialPos(core), StelCore::RefractionAuto);
}

// Get observer-centered galactic position
Vec3d StelObject::getGalacticPos(const StelCore *core) const
{
	return core->j2000ToGalactic(getJ2000EquatorialPos(core));
}

// Get observer-centered supergalactic position
Vec3d StelObject::getSupergalacticPos(const StelCore *core) const
{
	return core->j2000ToSupergalactic(getJ2000EquatorialPos(core));
}

// Checking position an object above mathematical horizon for current location
bool StelObject::isAboveHorizon(const StelCore *core) const
{
	bool r = true;
	float az, alt;
	StelUtils::rectToSphe(&az, &alt, getAltAzPosAuto(core));
	if (alt < 0.f)
		r = false;

	return r;
}

// Checking position an object above real horizon for current location
bool StelObject::isAboveRealHorizon(const StelCore *core) const
{
	bool r = true;
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	if (lmgr->getFlagLandscape())
	{
		if (lmgr->getLandscapeOpacity(getAltAzPosAuto(core))>0.85f) // landscape displayed
			r = false;
	}
	else
		r = isAboveHorizon(core); // check object is below mathematical horizon

	return r;
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
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));
	Q_UNUSED(az_app);
	QString cepoch = qc_("on date", "coordinates for current epoch");
	QString res;
	QString currentPlanet = core->getCurrentPlanet()->getEnglishName();
	if (flags&RaDecJ2000)
	{
		double dec_j2000, ra_j2000;
		StelUtils::rectToSphe(&ra_j2000,&dec_j2000,getJ2000EquatorialPos(core));
		if (withDecimalDegree)
			res += q_("RA/Dec") + QString(" (J2000.0): %1/%2").arg(StelUtils::radToDecDegStr(ra_j2000,5,false,true), StelUtils::radToDecDegStr(dec_j2000)) + "<br>";
		else
			res += q_("RA/Dec") + QString(" (J2000.0): %1/%2").arg(StelUtils::radToHmsStr(ra_j2000,true), StelUtils::radToDmsStr(dec_j2000,true)) + "<br>";
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
		float direction = 3.; // N is zero, E is 90 degrees
		if (useSouthAzimuth)
			direction = 2.;
		az = direction*M_PI - az;
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

	// N.B. Ecliptical coordinates are particularly earth-bound.
	// It may be OK to have terrestrial ecliptical coordinates of J2000.0 (standard epoch) because those are in practice linked with VSOP XY plane,
	// and because the ecliptical grid of J2000 is also shown for observers on other planets.
	// The formulation here has never computed the true position of any observer planet's orbital plane except for Earth,
	// or ever displayed the coordinates in the observer planet's equivalent to Earth's ecliptical coordinates.
	// As quick test you can observe if in any "Ecliptic coordinate" as seen from e.g. Mars or Jupiter the Sun was ever close to beta=0 (except if crossing the node...).

	if (flags&EclipticCoordJ2000)
	{
		double eclJ2000=GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(2451545.0);
		double ra_equ, dec_equ, lambda, beta;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,getJ2000EquatorialPos(core));
		StelUtils::equToEcl(ra_equ, dec_equ, eclJ2000, &lambda, &beta);
		if (lambda<0) lambda+=2.0*M_PI;
		if (withDecimalDegree)
			res += q_("Ecliptic longitude/latitude") + QString(" (J2000.0): %1/%2").arg(StelUtils::radToDecDegStr(lambda), StelUtils::radToDecDegStr(beta)) + "<br>";
		else
			res += q_("Ecliptic longitude/latitude") + QString(" (J2000.0): %1/%2").arg(StelUtils::radToDmsStr(lambda, true), StelUtils::radToDmsStr(beta, true)) + "<br>";

	}

	if ((flags&EclipticCoordOfDate) && (QString("Earth Sun").contains(currentPlanet)))
	{
		double jde=core->getJDE();
		double eclJDE = GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(jde);
		if (StelApp::getInstance().getCore()->getUseNutation())
		{
			double deltaEps, deltaPsi;
			getNutationAngles(jde, &deltaPsi, &deltaEps);
			eclJDE+=deltaEps;
		}
		double ra_equ, dec_equ, lambdaJDE, betaJDE;

		StelUtils::rectToSphe(&ra_equ,&dec_equ,getEquinoxEquatorialPos(core));
		StelUtils::equToEcl(ra_equ, dec_equ, eclJDE, &lambdaJDE, &betaJDE);
		if (lambdaJDE<0) lambdaJDE+=2.0*M_PI;
		if (withDecimalDegree)
			res += q_("Ecliptic longitude/latitude") + QString(" (%1): %2/%3").arg(cepoch, StelUtils::radToDecDegStr(lambdaJDE), StelUtils::radToDecDegStr(betaJDE)) + "<br>";
		else
			res += q_("Ecliptic longitude/latitude") + QString(" (%1): %2/%3").arg(cepoch, StelUtils::radToDmsStr(lambdaJDE, true), StelUtils::radToDmsStr(betaJDE, true)) + "<br>";
		// GZ Only for now: display epsilon_A, angle between Earth's Axis and ecl. of date.
		if (withDecimalDegree)
			res += q_("Ecliptic obliquity") + QString(" (%1): %2").arg(cepoch, StelUtils::radToDecDegStr(eclJDE)) + "<br>";
		else
			res += q_("Ecliptic obliquity") + QString(" (%1): %2").arg(cepoch, StelUtils::radToDmsStr(eclJDE, true)) + "<br>";
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

	if (flags&SupergalacticCoord)
	{
		double sglong, sglat;
		StelUtils::rectToSphe(&sglong, &sglat, getSupergalacticPos(core));
		if (withDecimalDegree)
			res += q_("Supergalactic longitude/latitude: %1/%2").arg(StelUtils::radToDecDegStr(sglong), StelUtils::radToDecDegStr(sglat)) + "<br>";
		else
			res += q_("Supergalactic longitude/latitude: %1/%2").arg(StelUtils::radToDmsStr(sglong,true), StelUtils::radToDmsStr(sglat,true)) + "<br>";
	}

	return res;
}

QString StelObject::getAdditionalInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString res;
	QString currentPlanet = core->getCurrentPlanet()->getEnglishName();

	if (flags&IAUConstellation)
	{
		QString constel=core->getIAUConstellation(getEquinoxEquatorialPos(core));
		res += q_("IAU Constellation: %1").arg(constel) + "<br>";
	}

	if ((flags&SiderealTime) && (currentPlanet=="Earth"))
	{
		double longitude=core->getCurrentLocation().longitude;
		double sidereal=(get_mean_sidereal_time(core->getJD(), core->getJDE())  + longitude) / 15.;
		sidereal=fmod(sidereal, 24.);
		if (sidereal < 0.) sidereal+=24.;
		res += q_("Mean Sidereal Time: %1").arg(StelUtils::hoursToHmsStr(sidereal)) + "<br>";
		if (core->getUseNutation())
		{
			sidereal=(get_apparent_sidereal_time(core->getJD(), core->getJDE()) + longitude) / 15.;
			sidereal=fmod(sidereal, 24.);
			if (sidereal < 0.) sidereal+=24.;
			res += q_("Apparent Sidereal Time: %1").arg(StelUtils::hoursToHmsStr(sidereal)) + "<br>";
		}
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
	else if(!(flags&NoFont))
	{
		Vec3f color = getInfoColor();
		StelCore* core = StelApp::getInstance().getCore();
		if (core->isBrightDaylight() && core->getSkyDrawer()->getFlagHasAtmosphere()==true && !StelApp::getInstance().getVisionModeNight())
		{
			// make info text more readable when atmosphere enabled at daylight.
			color = StelUtils::strToVec3f(StelApp::getInstance().getSettings()->value("color/daylight_text_color", "0.0,0.0,0.0").toString());
		}
		str.prepend(QString("<font color=%1>").arg(StelUtils::vec3fToHtmlColor(color)));
		str.append(QString("</font>"));
	}
}

QVariantMap StelObject::getInfoMap(const StelCore *core) const
{
	QVariantMap map;

	Vec3d pos;
	double ra, dec, alt, az, glong, glat;
	bool useOldAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();

	map.insert("type", getType());
	// ra/dec
	pos = getEquinoxEquatorialPos(core);
	StelUtils::rectToSphe(&ra, &dec, pos);
	map.insert("ra", ra*180./M_PI);
	map.insert("dec", dec*180./M_PI);

	// ra/dec in J2000
	pos = getJ2000EquatorialPos(core);
	StelUtils::rectToSphe(&ra, &dec, pos);
	map.insert("raJ2000", ra*180./M_PI);
	map.insert("decJ2000", dec*180./M_PI);

	// apparent altitude/azimuth
	pos = getAltAzPosApparent(core);
	StelUtils::rectToSphe(&az, &alt, pos);
	float direction = 3.; // N is zero, E is 90 degrees
	if (useOldAzimuth)
		direction = 2.;
	az = direction*M_PI - az;
	if (az > M_PI*2)
		az -= M_PI*2;

	map.insert("altitude", alt*180./M_PI);
	map.insert("azimuth", az*180./M_PI);

	// geometric altitude/azimuth
	pos = getAltAzPosGeometric(core);
	StelUtils::rectToSphe(&az, &alt, pos);
	az = direction*M_PI - az;
	if (az > M_PI*2)
		az -= M_PI*2;

	map.insert("altitude-geometric", alt*180./M_PI);
	map.insert("azimuth-geometric", az*180./M_PI);

	// galactic long/lat
	pos = getGalacticPos(core);
	StelUtils::rectToSphe(&glong, &glat, pos);
	map.insert("glong", glong*180./M_PI);
	map.insert("glat", glat*180./M_PI);

	// supergalactic long/lat
	pos = getSupergalacticPos(core);
	StelUtils::rectToSphe(&glong, &glat, pos);
	map.insert("sglong", glong*180./M_PI);
	map.insert("sglat", glat*180./M_PI);

	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	double ra_equ, dec_equ, lambda, beta;
	// J2000
	double eclJ2000 = ssmgr->getEarth()->getRotObliquity(2451545.0);
	double ecl = ssmgr->getEarth()->getRotObliquity(core->getJDE());

	// ecliptic longitude/latitude (J2000 frame)
	StelUtils::rectToSphe(&ra_equ,&dec_equ, getJ2000EquatorialPos(core));
	StelUtils::equToEcl(ra_equ, dec_equ, eclJ2000, &lambda, &beta);
	if (lambda<0) lambda+=2.0*M_PI;
	map.insert("elongJ2000", lambda*180./M_PI);
	map.insert("elatJ2000", beta*180./M_PI);

	if (QString("Earth Sun").contains(core->getCurrentLocation().planetName))
	{
		// ecliptic longitude/latitude
		StelUtils::rectToSphe(&ra_equ,&dec_equ, getEquinoxEquatorialPos(core));
		StelUtils::equToEcl(ra_equ, dec_equ, ecl, &lambda, &beta);
		if (lambda<0) lambda+=2.0*M_PI;
		map.insert("elong", lambda*180./M_PI);
		map.insert("elat", beta*180./M_PI);
	}

	// magnitude
	map.insert("vmag", getVMagnitude(core));
	map.insert("vmage", getVMagnitudeWithExtinction(core));

	// angular size
	double angularSize = 2.*getAngularSize(core)*M_PI/180.;
	bool sign;
	double deg;
	StelUtils::radToDecDeg(angularSize, sign, deg);
	if (!sign)
		deg *= -1;
	map.insert("size", angularSize);
	map.insert("size-dd", deg);
	map.insert("size-deg", StelUtils::radToDecDegStr(angularSize, 5));
	map.insert("size-dms", StelUtils::radToDmsStr(angularSize, true));

	// english name or designation & localized name
	map.insert("name", getEnglishName());
	map.insert("localized-name", getNameI18n());

	// 'above horizon' flag
	map.insert("above-horizon", isAboveRealHorizon(core));

	return map;
}
