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

// Get parallactic angle, which is the deviation between zenith angle and north angle.
// Meeus, Astronomical Algorithms, 2nd ed. (1998), p.98.
float StelObject::getParallacticAngle(const StelCore* core) const
{
	const double phi=core->getCurrentLocation().latitude*M_PI/180.0;
	const Vec3d siderealPos=getSiderealPosApparent(core);
	double delta, ha;
	StelUtils::rectToSphe(&ha, &delta, siderealPos);
	ha *= -1.0; // We must invert the orientation sense in case of sidereal positions!

	// A rare condition! Object exactly in zenith, avoid undefined result.
	if ((ha==0.0) && (delta==phi))
		return 0.0f;
	else
		return atan2(sin(ha), tan(phi)*cos(delta)-sin(delta)*cos(ha));
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

Vec3f StelObject::getRTSTime(StelCore *core) const
{
	return computeRTSTime(core);
}

Vec3f StelObject::computeRTSTime(StelCore *core) const
{
	float hz = 0.f;
	if ( (getEnglishName()=="Moon") && (core->getCurrentLocation().planetName=="Earth"))
		hz = +0.7275*0.95; // horizon parallax factor
	else if (getEnglishName()=="Sun")
		hz = - getAngularSize(core); // semidiameter; Canonical value 16', but this is accurate even from other planets...
	hz *= M_PI/180.;

	if (core->getSkyDrawer()->getFlagHasAtmosphere())
	{
		// canonical" refraction at horizon is -34'. Replace by pressure-dependent value here!
		Refraction refraction=core->getSkyDrawer()->getRefraction();
		Vec3d zeroAlt(1.0,0.0,0.0);
		refraction.backward(zeroAlt);
		hz += asin(zeroAlt[2]);
	}
	const float phi = core->getCurrentLocation().latitude * M_PI/180.f;
	PlanetP cp = core->getCurrentPlanet();
	const double coeff = cp->getMeanSolarDay() / cp->getSiderealDay();

	Vec3f rts = Vec3f(-100.f,-100.f,-100.f);  // init as "never rises" [abs must be larger than 24!]

	double dec, ra, ha, t;
	StelUtils::rectToSphe(&ra, &dec, getSiderealPosGeometric(core));
	ra = 2.*M_PI-ra;
	ha = ra*12./M_PI;
	if (ha>24.)
		ha -= 24.;
	// It seems necessary to have ha in [-12,12]!
	if (ha>12.)
		ha -= 24.;

	const double JD = core->getJD();
	const double ct = (JD - (int)JD)*24.;

	t = ct - ha*coeff; // earth: coeff=(360.985647/360.);
	if (ha>12. && ha<=24.)
		t += 24.;

	t += core->getUTCOffset(JD) + 12.;
	t = StelUtils::fmodpos(t, 24.0);
	rts[1] = t;

	const double cosH = (sin(hz) - sin(phi)*sin(dec))/(cos(phi)*cos(dec));
	if (cosH<-1.) // circumpolar
	{
		rts[0]=rts[2]=100.f;
	}
	else if (cosH>1.) // never rises
	{
		rts[0]=rts[2]=-100.f;
	}
	else
	{
		const double HC = acos(cosH)*12.*coeff/M_PI;

		rts[0] = StelUtils::fmodpos(t - HC, 24.);
		rts[2] = StelUtils::fmodpos(t + HC, 24.);
	}

	return rts;
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

// Format the magnitude info string for the object
QString StelObject::getMagnitudeInfoString(const StelCore *core, const InfoStringGroup& flags, const double alt_app, const int decimals) const
{
	if (flags&Magnitude)
	{
		QString emag = "";
		if (core->getSkyDrawer()->getFlagHasAtmosphere() && (alt_app>-2.0*M_PI/180.0)) // Don't show extincted magnitude much below horizon where model is meaningless.
		{
			const Extinction &extinction=core->getSkyDrawer()->getExtinction();
			float airmass=extinction.airmass(std::cos(M_PI/2.0-alt_app), true);

			emag = QString(" (%1 <b>%2</b> %3 <b>%4</b> %5)").arg(q_("reduced to"), QString::number(getVMagnitudeWithExtinction(core), 'f', decimals), q_("by"), QString::number(airmass, 'f', 2), q_("Airmasses"));
		}
		return QString("%1: <b>%2</b>%3<br />").arg(q_("Magnitude"), QString::number(getVMagnitude(core), 'f', decimals), emag);
	}
	else
		return QString();
}

// Format the positional info string contain J2000/of date/altaz/hour angle positions for the object
QString StelObject::getCommonInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	StelApp& app = StelApp::getInstance();
	bool withAtmosphere = core->getSkyDrawer()->getFlagHasAtmosphere();
	bool withDecimalDegree = app.getFlagShowDecimalDegrees();
	bool useSouthAzimuth = app.getFlagSouthAzimuthUsage();
	bool withTables = app.getFlagUseFormattingOutput();
	bool withDesignations = app.getFlagUseCCSDesignation();
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));
	Q_UNUSED(az_app);
	QString cepoch = qc_("on date", "coordinates for current epoch");
	QString res;
	QString currentPlanet = core->getCurrentPlanet()->getEnglishName();
	QString firstCoordinate, secondCoordinate, apparent = " ";
	if (withAtmosphere)
		apparent += q_("(apparent)");

	if (withTables)
		res += "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";

	// TRANSLATORS: Right ascension/Declination
	QString RADec = qc_("RA/Dec", "celestial coordinate system");
	if (withDesignations)
		RADec = QString("%1/%2").arg(QChar(0x03B1), QChar(0x03B4));

	if (flags&RaDecJ2000)
	{
		double dec_j2000, ra_j2000;
		StelUtils::rectToSphe(&ra_j2000,&dec_j2000,getJ2000EquatorialPos(core));
		if (withDecimalDegree)
		{
			firstCoordinate  = StelUtils::radToDecDegStr(ra_j2000,5,false,true);
			secondCoordinate = StelUtils::radToDecDegStr(dec_j2000);
		}
		else
		{
			firstCoordinate  = StelUtils::radToHmsStr(ra_j2000,true);
			secondCoordinate = StelUtils::radToDmsStr(dec_j2000,true);
		}

		if (withTables)
			res += QString("<tr><td>%1 (J2000.0):</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td></td></tr>").arg(RADec, firstCoordinate, secondCoordinate);
		else
			res += QString("%1 (J2000.0): %2/%3").arg(RADec, firstCoordinate, secondCoordinate) + "<br>";
	}

	if (flags&RaDecOfDate)
	{
		double dec_equ, ra_equ;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,getEquinoxEquatorialPos(core));
		if (withDecimalDegree)
		{
			firstCoordinate  = StelUtils::radToDecDegStr(ra_equ,5,false,true);
			secondCoordinate = StelUtils::radToDecDegStr(dec_equ);
		}
		else
		{
			firstCoordinate  = StelUtils::radToHmsStr(ra_equ,true);
			secondCoordinate = StelUtils::radToDmsStr(dec_equ,true);
		}

		if (withTables)
			res += QString("<tr><td>%1 (%4):</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td></td></tr>").arg(RADec, firstCoordinate, secondCoordinate, cepoch);
		else
			res += QString("%1 (%4): %2/%3").arg(RADec, firstCoordinate, secondCoordinate, cepoch) + "<br>";
	}

	if (flags&HourAngle)
	{
		double dec_sidereal, ra_sidereal, ha_sidereal;
		StelUtils::rectToSphe(&ra_sidereal,&dec_sidereal,getSiderealPosGeometric(core));
		ra_sidereal = 2.*M_PI-ra_sidereal;
		if (withAtmosphere && (alt_app>-2.0*M_PI/180.0)) // Don't show refracted values much below horizon where model is meaningless.
		{
			StelUtils::rectToSphe(&ra_sidereal,&dec_sidereal,getSiderealPosApparent(core));
			ra_sidereal = 2.*M_PI-ra_sidereal;
			if (withDecimalDegree)
			{
				ha_sidereal = ra_sidereal*12/M_PI;
				if (ha_sidereal>24.)
					ha_sidereal -= 24.;
				firstCoordinate  = QString("%1h").arg(ha_sidereal, 0, 'f', 5);
				secondCoordinate = StelUtils::radToDecDegStr(dec_sidereal);
			}
			else
			{
				firstCoordinate  = StelUtils::radToHmsStr(ra_sidereal,true);
				secondCoordinate = StelUtils::radToDmsStr(dec_sidereal,true);
			}
		}
		else
		{
			if (withDecimalDegree)
			{
				ha_sidereal = ra_sidereal*12/M_PI;
				if (ha_sidereal>24.)
					ha_sidereal -= 24.;
				firstCoordinate  = QString("%1h").arg(ha_sidereal, 0, 'f', 5);
				secondCoordinate = StelUtils::radToDecDegStr(dec_sidereal);
			}
			else
			{
				firstCoordinate  = StelUtils::radToHmsStr(ra_sidereal,true);
				secondCoordinate = StelUtils::radToDmsStr(dec_sidereal,true);
			}
		}

		// TRANSLATORS: Hour angle/Declination
		QString HADec = qc_("HA/Dec", "celestial coordinate system");
		if (withDesignations)
			HADec = QString("h/%1").arg(QChar(0x03B4));

		if (withTables)
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td>%4</td></tr>").arg(HADec, firstCoordinate, secondCoordinate, apparent);
		else
			res += QString("%1: %2/%3 %4").arg(HADec, firstCoordinate, secondCoordinate, apparent) + "<br>";
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
		if (withAtmosphere && (alt_app>-2.0*M_PI/180.0)) // Don't show refracted altitude much below horizon where model is meaningless.
		{
			if (withDecimalDegree)
			{
				firstCoordinate  = StelUtils::radToDecDegStr(az);
				secondCoordinate = StelUtils::radToDecDegStr(alt_app);
			}
			else
			{
				firstCoordinate  = StelUtils::radToDmsStr(az,true);
				secondCoordinate = StelUtils::radToDmsStr(alt_app,true);
			}
		}
		else
		{
			if (withDecimalDegree)
			{
				firstCoordinate  = StelUtils::radToDecDegStr(az);
				secondCoordinate = StelUtils::radToDecDegStr(alt);
			}
			else
			{
				firstCoordinate  = StelUtils::radToDmsStr(az,true);
				secondCoordinate = StelUtils::radToDmsStr(alt,true);
			}
		}

		// TRANSLATORS: Azimuth/Altitude
		QString AzAlt = qc_("Az./Alt.", "celestial coordinate system");
		if (withDesignations)
			AzAlt = "A/a";

		if (withTables)
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td>%4</td></tr>").arg(AzAlt, firstCoordinate, secondCoordinate, apparent);
		else
			res += QString("%1: %2/%3 %4").arg(AzAlt, firstCoordinate, secondCoordinate, apparent) + "<br>";
	}

	if (flags&GalacticCoord)
	{
		double glong, glat;
		StelUtils::rectToSphe(&glong, &glat, getGalacticPos(core));
		if (withDecimalDegree)
		{
			firstCoordinate  = StelUtils::radToDecDegStr(glong);
			secondCoordinate = StelUtils::radToDecDegStr(glat);
		}
		else
		{
			firstCoordinate  = StelUtils::radToDmsStr(glong,true);
			secondCoordinate = StelUtils::radToDmsStr(glat,true);
		}

		// TRANSLATORS: Galactic longitude/latitude
		QString GalLongLat = qc_("Gal. long./lat.", "celestial coordinate system");
		if (withDesignations)
			GalLongLat = "l/b";

		if (withTables)
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td></td></tr>").arg(GalLongLat, firstCoordinate, secondCoordinate);
		else
			res += QString("%1: %2/%3").arg(GalLongLat, firstCoordinate, secondCoordinate) + "<br>";
	}

	if (flags&SupergalacticCoord)
	{
		double sglong, sglat;
		StelUtils::rectToSphe(&sglong, &sglat, getSupergalacticPos(core));
		if (withDecimalDegree)
		{
			firstCoordinate  = StelUtils::radToDecDegStr(sglong);
			secondCoordinate = StelUtils::radToDecDegStr(sglat);
		}
		else
		{
			firstCoordinate  = StelUtils::radToDmsStr(sglong,true);
			secondCoordinate = StelUtils::radToDmsStr(sglat,true);
		}

		// TRANSLATORS: Supergalactic longitude/latitude
		QString SGalLongLat = qc_("Supergal. long./lat.", "celestial coordinate system");
		if (withDesignations)
			SGalLongLat = "SGL/SGB";

		if (withTables)
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td></td></tr>").arg(SGalLongLat, firstCoordinate, secondCoordinate);
		else
			res += QString("%1: %2/%3").arg(SGalLongLat, firstCoordinate, secondCoordinate) + "<br>";
	}

	// N.B. Ecliptical coordinates are particularly earth-bound.
	// It may be OK to have terrestrial ecliptical coordinates of J2000.0 (standard epoch) because those are in practice linked with VSOP XY plane,
	// and because the ecliptical grid of J2000 is also shown for observers on other planets.
	// The formulation here has never computed the true position of any observer planet's orbital plane except for Earth,
	// or ever displayed the coordinates in the observer planet's equivalent to Earth's ecliptical coordinates.
	// As quick test you can observe if in any "Ecliptic coordinate" as seen from e.g. Mars or Jupiter the Sun was ever close to beta=0 (except if crossing the node...).

	// TRANSLATORS: Ecliptic longitude/latitude
	QString EqlLongLat = qc_("Ecl. long./lat.", "celestial coordinate system");
	if (withDesignations)
		EqlLongLat = QString("%1/%2").arg(QChar(0x03BB), QChar(0x03B2));

	if (flags&EclipticCoordJ2000)
	{
		const double eclJ2000=GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(2451545.0);
		double ra_equ, dec_equ, lambda, beta;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,getJ2000EquatorialPos(core));
		StelUtils::equToEcl(ra_equ, dec_equ, eclJ2000, &lambda, &beta);
		if (lambda<0) lambda+=2.0*M_PI;
		if (withDecimalDegree)
		{
			firstCoordinate  = StelUtils::radToDecDegStr(lambda);
			secondCoordinate = StelUtils::radToDecDegStr(beta);
		}
		else
		{
			firstCoordinate  = StelUtils::radToDmsStr(lambda, true);
			secondCoordinate = StelUtils::radToDmsStr(beta, true);
		}

		if (withTables)
			res += QString("<tr><td>%1 (J2000.0):</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td></td></tr>").arg(EqlLongLat, firstCoordinate, secondCoordinate);
		else
			res += QString("%1 (J2000.0): %2/%3").arg(EqlLongLat, firstCoordinate, secondCoordinate) + "<br>";
	}

	if ((flags&EclipticCoordOfDate) && (QString("Earth Sun").contains(currentPlanet)))
	{
		const double jde=core->getJDE();
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
		{
			firstCoordinate  = StelUtils::radToDecDegStr(lambdaJDE);
			secondCoordinate = StelUtils::radToDecDegStr(betaJDE);
		}
		else
		{
			firstCoordinate  = StelUtils::radToDmsStr(lambdaJDE, true);
			secondCoordinate = StelUtils::radToDmsStr(betaJDE, true);
		}

		if (withTables)
			res += QString("<tr><td>%1 (%4):</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td></td></tr>").arg(EqlLongLat, firstCoordinate, secondCoordinate, cepoch) + "</table>";
		else
			res += QString("%1 (%4): %2/%3").arg(EqlLongLat, firstCoordinate, secondCoordinate, cepoch) + "<br>";

		// GZ Only for now: display epsilon_A, angle between Earth's Axis and ecl. of date.
		if (withDecimalDegree)
			firstCoordinate = StelUtils::radToDecDegStr(eclJDE);
		else
			firstCoordinate = StelUtils::radToDmsStr(eclJDE, true);

		QString eqlObl = q_("Ecliptic obliquity");
		if (withTables)
		{
			res += "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
			res += QString("<tr><td>%1 (%4):</td><td>%2</td></tr>").arg(eqlObl, firstCoordinate, cepoch);
		}
		else
			res += QString("%1 (%3): %2").arg(eqlObl, firstCoordinate, cepoch) + "<br>";
	}

	if (withTables)
		 res += "</table>";

	if ((flags&SiderealTime) && (currentPlanet==QStringLiteral("Earth")))
	{
		const double longitude=core->getCurrentLocation().longitude;
		double sidereal=(get_mean_sidereal_time(core->getJD(), core->getJDE())  + longitude) / 15.;
		sidereal=fmod(sidereal, 24.);
		if (sidereal < 0.) sidereal+=24.;
		QString STc = q_("Mean Sidereal Time");
		QString STd = StelUtils::hoursToHmsStr(sidereal);

		if (withTables)
		{
			res += "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(STc, STd);
		}
		else
			res += QString("%1: %2").arg(STc, STd) + "<br>";

		if (core->getUseNutation())
		{
			sidereal=(get_apparent_sidereal_time(core->getJD(), core->getJDE()) + longitude) / 15.;
			sidereal=fmod(sidereal, 24.);
			if (sidereal < 0.) sidereal+=24.;
			STc = q_("Apparent Sidereal Time");
			STd = StelUtils::hoursToHmsStr(sidereal);
			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(STc, STd);
			else
				res += QString("%1: %2").arg(STc, STd) + "<br>";
		}
		if (withTables && !(flags&RTSTime && getType()!=QStringLiteral("Satellite")))
			res += "</table>";
	}

	if (flags&RTSTime && getType()!=QStringLiteral("Satellite"))
	{
		Vec3f rts = getRTSTime(StelApp::getInstance().getCore()); // required not const StelCore!
		QString sTransit = qc_("Transit", "celestial event; passage across a meridian");
		QString sRise = qc_("Rise", "celestial event");
		QString sSet = qc_("Set", "celestial event");
		float sunrise = 0.f;
		float sunset = 24.f;
		bool isSun = false;
		if (getEnglishName()=="Sun")
			isSun = true;

		if (withTables && !(flags&SiderealTime && currentPlanet==QStringLiteral("Earth")))
			res += "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";

		if (rts[0]>-99.f && rts[0]<100.f)
		{
			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sRise, StelUtils::hoursToHmsStr(rts[0], true));
			else
				res += QString("%1: %2").arg(sRise, StelUtils::hoursToHmsStr(rts[0], true)) + "<br />";

			sunrise = rts[0];
		}

		if (withTables)
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sTransit, StelUtils::hoursToHmsStr(rts[1], true));
		else
			res += QString("%1: %2").arg(sTransit, StelUtils::hoursToHmsStr(rts[1], true)) + "<br />";

		if (rts[2]>-99.f && rts[2]<100.f)
		{
			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sSet, StelUtils::hoursToHmsStr(rts[2], true));
			else
				res += QString("%1: %2").arg(sSet, StelUtils::hoursToHmsStr(rts[2], true)) + "<br />";

			sunset = rts[2];
		}

		float day = sunset - sunrise;
		if (isSun && day<24.f)
		{
			QString sDay = q_("Daytime");
			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sDay, StelUtils::hoursToHmsStr(day, true));
			else
				res += QString("%1: %2").arg(sDay, StelUtils::hoursToHmsStr(day, true)) + "<br />";
		}

		if (withTables)
			res += "</table>";

		if (rts[0]<-99.f && rts[2]<-99.f )
		{
			if (isSun)
				res += q_("Polar night") + "<br />";
			else
				res += q_("This object never rises") + "<br />";
		}
		else if (rts[0]>99.f && rts[2]>99.f)
		{
			if (isSun)
				res += q_("Polar day") + "<br />";
			else
				res += q_("Circumpolar (never sets)") + "<br />";
		}
		else if (rts[0]>99.f && rts[2]<99.f)
			res += q_("Polar dawn") + "<br />";
		else if (rts[0]<99.f && rts[2]>99.f)
			res += q_("Polar dusk") + "<br />";
	}

	if (flags&HourAngle && getType()!=QStringLiteral("Star"))
	{
		const float par=getParallacticAngle(core) * 180.0/M_PI;
		res += QString("%1: %2%3").arg(q_("Parallactic Angle")).arg(par, 0, 'f', 2).arg(QChar(0x00B0)) + "<br>";
	}

	if (flags&IAUConstellation)
	{
		QString constel=core->getIAUConstellation(getEquinoxEquatorialPos(core));
		res += QString("%1: %2").arg(q_("IAU Constellation"), constel) + "<br>";
	}

	return res;
}

// Apply post processing on the info string
void StelObject::postProcessInfoString(QString& str, const InfoStringGroup& flags) const
{
	// hack for avoiding an empty line before table
	str.replace(QRegExp("<br(\\s*/)?><table"), "<table");
	// chomp trailing line breaks
	str.replace(QRegExp("<br(\\s*/)?>\\s*$"), "");

	if (flags&PlainText)
	{
		str.replace("<b>", "");
		str.replace("</b>", "");
		str.replace("<h2>", "");
		str.replace("</h2>", "\n");
		str.replace(QRegExp("<br(\\s*/)?>"), "\n");
		str.replace("<tr>", "");
		str.replace(QRegExp("<td(\\w*)?>"), "");
		str.replace("<td>", "");
		str.replace("</tr>", "\n");
		str.replace(QRegExp("<table(\\w*)?>"), "");
		str.replace("</table>", "");
	}
	else if(!(flags&NoFont))
	{
		Vec3f color = getInfoColor();
		StelCore* core = StelApp::getInstance().getCore();
		if (core->isBrightDaylight() && !StelApp::getInstance().getVisionModeNight())
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

	if (getType()!=QStringLiteral("Star"))
		map.insert("parallacticAngle", getParallacticAngle(core)*180.0/M_PI);

	// Sidereal Time and hour angle
	if (core->getCurrentLocation().planetName=="Earth")
	{
		const double longitude=core->getCurrentLocation().longitude;
		double sidereal=(get_mean_sidereal_time(core->getJD(), core->getJDE())  + longitude) / 15.;
		sidereal=fmod(sidereal, 24.);
		if (sidereal < 0.) sidereal+=24.;
		map.insert("meanSidTm", StelUtils::hoursToHmsStr(sidereal));

		sidereal=(get_apparent_sidereal_time(core->getJD(), core->getJDE()) + longitude) / 15.;
		sidereal=fmod(sidereal, 24.);
		if (sidereal < 0.) sidereal+=24.;
		map.insert("appSidTm", StelUtils::hoursToHmsStr(sidereal));

		double ha = sidereal * 15.0 - ra * 180.0/M_PI;
		ha=fmod(ha, 360.0);
		if (ha < 0.) ha+=360.0;
		map.insert("hourAngle-dd", ha);
		map.insert("hourAngle-hms", StelUtils::hoursToHmsStr(ha/15.0));
	}

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

	const Extinction &extinction=core->getSkyDrawer()->getExtinction();
	map.insert("airmass", extinction.airmass(cos(M_PI/2.0-alt), true));

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
	map.insert("size-dms", StelUtils::radToDmsPStr(angularSize, 2));

	// english name or designation & localized name
	map.insert("name", getEnglishName());
	map.insert("localized-name", getNameI18n());

	// 'above horizon' flag
	map.insert("above-horizon", isAboveRealHorizon(core));

	Vec3f rts = getRTSTime(StelApp::getInstance().getCore());
	map.insert("rise", StelUtils::hoursToHmsStr(rts[0], true));
	map.insert("rise-dhr", rts[0]);
	map.insert("transit", StelUtils::hoursToHmsStr(rts[1], true));
	map.insert("transit-dhr", rts[1]);
	map.insert("set", StelUtils::hoursToHmsStr(rts[2], true));
	map.insert("set-dhr", rts[2]);

	return map;
}
