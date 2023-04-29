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
#include "StelObjectMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelSkyDrawer.hpp"
#include "RefractionExtinction.hpp"
#include "StelLocation.hpp"
#include "SolarSystem.hpp"
#include "StelModuleMgr.hpp"
#include "LandscapeMgr.hpp"
#include "SpecificTimeMgr.hpp"
#include "planetsephems/sidereal_time.h"
#include "planetsephems/precession.h"

#include <QRegularExpression>
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
	const double phi=static_cast<double>(core->getCurrentLocation().getLatitude())*M_PI/180.0;
	const Vec3d siderealPos=getSiderealPosApparent(core);
	double delta, ha;
	StelUtils::rectToSphe(&ha, &delta, siderealPos);
	ha *= -1.0; // We must invert the orientation sense in case of sidereal positions!

	// A rare condition! Object exactly in zenith, avoid undefined result.
	if ((ha==0.0) && ((delta-phi)==0.0))
		return 0.0f;
	else
		return static_cast<float>(atan2(sin(ha), tan(phi)*cos(delta)-sin(delta)*cos(ha)));
}

// Checking position an object above mathematical horizon for current location
bool StelObject::isAboveHorizon(const StelCore *core) const
{
	float az, alt;
	StelUtils::rectToSphe(&az, &alt, getAltAzPosAuto(core));
	return (alt >= 0.f);
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
	Q_UNUSED(core)
	return 99.f;
}

Vec4d StelObject::getRTSTime(const StelCore *core, const double altitude) const
{
	const StelLocation loc=core->getCurrentLocation();
	if (loc.name.contains("->")) // a spaceship
		return Vec4d(0., 0., 0., -1000.);

	//StelObjectMgr* omgr=GETSTELMODULE(StelObjectMgr);
	double ho = 0.;
	if (core->getSkyDrawer()->getFlagHasAtmosphere())
	{
		// canonical" refraction at horizon is -34'. Replace by pressure-dependent value here!
		// This fixes 0-degree refraction. Not sure if we use refracted position throughout? Canonical ephemerides have -6/-12/-18 without refraction.
		Refraction refraction=core->getSkyDrawer()->getRefraction();
		Vec3d zeroAlt(1.0,0.0,0.0);
		refraction.backward(zeroAlt);
		ho += asin(zeroAlt[2]);
	}
	if (altitude != 0.)
		ho = altitude*M_PI_180; // Not sure if we use refraction for off-zero settings?
	const double phi = static_cast<double>(loc.getLatitude()) * M_PI_180;
	const double L = static_cast<double>(loc.getLongitude()) * M_PI_180; // OUR longitude. Meeus has it reversed
	PlanetP obsPlanet = core->getCurrentPlanet();
	const double rotRate = obsPlanet->getSiderealDay();
	const double currentJD=core->getJD();
	const double currentJDE=core->getJDE();
	const double utcShift = core->getUTCOffset(currentJD) / 24.;
	int year, month, day, currentdate;
	StelUtils::getDateFromJulianDay(currentJD+utcShift, &year, &month, &currentdate);

	// And convert to equatorial coordinates of date. We can also use this day's current aberration, given the other uncertainties/omissions.
	const Vec3d eq_2=getEquinoxEquatorialPos(core);
	double ra2, de2;
	StelUtils::rectToSphe(&ra2, &de2, eq_2);
	// Around ra~12 there may be a jump between 12h and -12h which could crash interpolation. We better make sure to have either negative RA or RA>24 in this case.
	if (cos(ra2)<0.)
	{
		ra2=StelUtils::fmodpos(ra2, 2*M_PI);
	}

	// 3. Approximate times:
	// we use Sidereal Time of Place!
	const double Theta2=obsPlanet->getSiderealTime(currentJD, currentJDE) * (M_PI/180.) + L;  // [radians]
	double cosH0=(sin(ho)-sin(phi)*sin(de2))/(cos(phi)*cos(de2));

	//omgr->removeExtraInfoStrings(StelObject::DebugAid);
	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("&alpha;<sub>2</sub>: %1=%2 &delta;<sub>2</sub>: %3<br/>").arg(QString::number(ra2, 'f', 4)).arg(StelUtils::radToHmsStr(ra2)).arg(StelUtils::radToDmsStr(de2)));
	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("h<sub>0</sub>= %1<br/>").arg(StelUtils::radToDmsStr(ho)));
	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("JD<sub>2</sub>= %1<br/>").arg(QString::number(currentJD, 'f', 5)));
	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("&Theta;<sub>2</sub>= %1<br/>").arg(StelUtils::radToHmsStr(Theta2)));
	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("cos H<sub>0</sub>= %1<br/>").arg(QString::number(cosH0, 'f', 4)));


	double h2=StelUtils::fmodpos(Theta2-ra2, 2.*M_PI); if (h2>M_PI) h2-=2.*M_PI; // Hour angle at currentJD. This should be [-pi, pi]
	// Find approximation of transit time
	//double JDt=currentJD-h2/(M_PI*2.)*rotRate;
	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("h<sub>2</sub>= %1<br/>").arg(QString::number(h2, 'f', 4)));
	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("JD<sub>t</sub>= %1<br/>").arg(QString::number(JDt, 'f', 4)));

	// In terms of chapter 15, where m0, m1 and m2 are fractions of day within the current day, we use mr, mt, ms as fractions of day from currentJD, and they lie within [-1...+1].

	double mr, ms, flag=0.;
	double mt=-h2*(0.5*rotRate/M_PI);
	// Transit should occur on current date
	StelUtils::getDateFromJulianDay(currentJD+mt+utcShift, &year, &month, &day);
	if (day != currentdate)
	{
		if (mt<0.)
			mt += rotRate;
		else
			mt -= rotRate;
	}

	// circumpolar: set rise and set times to lower culmination, i.e. 1/2 rotation from transit
	if (fabs(cosH0)>1.)
	{
		flag = (cosH0<-1.) ? 100 : -100; // circumpolar / never rises
		mr   = (cosH0<-1.) ? mt-0.5*rotRate : mt;
		ms   = (cosH0<-1.) ? mt+0.5*rotRate : mt;
	}
	else
	{
		const double H0 = acos(cosH0);
		//omgr->addToExtraInfoString(StelObject::DebugAid, QString("H<sub>0</sub>= %1<br/>").arg(QString::number(H0*M_180_PI, 'f', 6)));

		mr = mt - H0*rotRate/(2.*M_PI);
		ms = mt + H0*rotRate/(2.*M_PI);
	}

	// Rise should occur on current date
	StelUtils::getDateFromJulianDay(currentJD+mr+utcShift, &year, &month, &day);
	if (day != currentdate)
	{
		if (mr<0.)
			mr += rotRate;
		else
			mr -= rotRate;
	}

	// Set should occur on current date
	StelUtils::getDateFromJulianDay(currentJD+ms+utcShift, &year, &month, &day);
	if (day != currentdate)
	{
		if (ms<0.)
			ms += rotRate;
		else
			ms -= rotRate;
	}

	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("m<sub>t</sub>= %1<br/>").arg(QString::number(mt, 'f', 6)));
	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("m<sub>r</sub>= %1<br/>").arg(QString::number(mr, 'f', 6)));
	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("m<sub>s</sub>= %1<br/>").arg(QString::number(ms, 'f', 6)));

	// Not quite done! Do the final tweaks...
	if (fabs(cosH0)<1.)
	{
		// RISE
		int iterations=0; // add this to limit the loops, just in case.
		double Delta_mr=1.;
		while (Delta_mr > 1./8640.) // Do that until accurate to 10 seconds
		{
			const double theta_mr=obsPlanet->getSiderealTime(currentJD+mr, currentJDE+mr) * (M_PI/180.) + L;  // [radians]
			double hr=StelUtils::fmodpos(theta_mr-ra2, 2.*M_PI); if (hr>M_PI) hr-=2.*M_PI; // Hour angle of the rising RA at currentJD. This should be [-pi, pi]
			//omgr->addToExtraInfoString(StelObject::DebugAid, QString("h<sub>r</sub>': %1 = %2<br/>").arg(QString::number(hr, 'f', 6)).arg(StelUtils::radToHmsStr(hr, true)));

			double ar=asin(sin(phi)*sin(de2)+cos(phi)*cos(de2)*cos(hr)); // altitude at this hour angle

			Delta_mr= (ar-ho)/(cos(de2)*cos(phi)*sin(hr)) / (M_PI*2.);
			Delta_mr=StelUtils::fmodpos(Delta_mr+0.5, 1.0)-0.5; // ensure this is a small correction
			mr+=Delta_mr;

			//omgr->addToExtraInfoString(StelObject::DebugAid, QString("alt<sub>r</sub>': %1 = %2<br/>").arg(QString::number(ar, 'f', 6)).arg(StelUtils::radToDmsStr(ar)));
			//omgr->addToExtraInfoString(StelObject::DebugAid, QString("&Delta;<sub>mr</sub>'= %1<br/>").arg(QString::number(Delta_mr, 'f', 6)));
			//omgr->addToExtraInfoString(StelObject::DebugAid, QString("m<sub>r</sub>' = %1<br/>").arg(QString::number(mr, 'f', 6)));

			if (++iterations >= 5)
				break;
		}
		// SET
		iterations=0; // add this to limit the loops, just in case.
		double Delta_ms=1.;
		while (Delta_ms > 1./8640.) // Do that until accurate to 10 seconds
		{
			const double theta_ms=obsPlanet->getSiderealTime(currentJD+ms, currentJDE+ms) * (M_PI/180.) + L;  // [radians]
			double hs=StelUtils::fmodpos(theta_ms-ra2, 2.*M_PI); if (hs>M_PI) hs-=2.*M_PI; // Hour angle of the setting RA at currentJD. This should be [-pi, pi]
			//omgr->addToExtraInfoString(StelObject::DebugAid, QString("h<sub>s</sub>': %1 = %2<br/>").arg(QString::number(hs, 'f', 6)).arg(StelUtils::radToHmsStr(hs, true)));

			double as=asin(sin(phi)*sin(de2)+cos(phi)*cos(de2)*cos(hs)); // altitude at this hour angle

			Delta_ms= (as-ho)/(cos(de2)*cos(phi)*sin(hs)) / (M_PI*2.);
			Delta_ms=StelUtils::fmodpos(Delta_ms+0.5, 1.0)-0.5; // ensure this is a small correction
			ms+=Delta_ms;

			//omgr->addToExtraInfoString(StelObject::DebugAid, QString("alt<sub>s</sub>': %1 = %2<br/>").arg(QString::number(as, 'f', 6)).arg(StelUtils::radToDmsStr(as)));
			//omgr->addToExtraInfoString(StelObject::DebugAid, QString("&Delta;<sub>ms</sub>'= %1<br/>").arg(QString::number(Delta_ms, 'f', 6)));
			//omgr->addToExtraInfoString(StelObject::DebugAid, QString("m<sub>s</sub>' = %1<br/>").arg(QString::number(ms, 'f', 6)));

			if (++iterations >= 5)
				break;
		}
	}
	return Vec4d(currentJD+mr, currentJD+mt, currentJD+ms, flag);
}

float StelObject::getSelectPriority(const StelCore* core) const
{
	return qMin(getVMagnitudeWithExtinction(core), 15.0f);
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

float StelObject::getAirmass(const StelCore *core) const
{
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app, &alt_app, getAltAzPosApparent(core));
	Q_UNUSED(az_app)
	if (core->getSkyDrawer()->getFlagHasAtmosphere() && (alt_app>-2.0*M_PI_180)) // Don't compute extinction much below horizon where model is meaningless.
	{
		const Extinction &extinction=core->getSkyDrawer()->getExtinction();
		return extinction.airmass(static_cast<float>(std::cos(M_PI_2-alt_app)), true);
	}
	else
		return -1.f;
}

// Format the magnitude info string for the object
QString StelObject::getMagnitudeInfoString(const StelCore *core, const InfoStringGroup& flags, const int decimals) const
{
	if (flags&Magnitude)
	{
		QString str = QString("%1: <b>%2</b>").arg(q_("Magnitude"), QString::number(getVMagnitude(core), 'f', decimals));
		const float airmass = getAirmass(core);
		if (airmass>-1.f) // Don't show extincted magnitude much below horizon where model is meaningless.
			str += QString(" (%1 <b>%2</b> %3 <b>%4</b> %5)").arg(q_("reduced to"), QString::number(getVMagnitudeWithExtinction(core), 'f', decimals), q_("by"), QString::number(airmass, 'f', 2), q_("Airmasses"));
		str += "<br/>" + getExtraInfoStrings(Magnitude).join("");
		return str;
	}
	else
		return QString();
}

// Format the positional info string contain J2000/of date/altaz/hour angle positions for the object
QString StelObject::getCommonInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	StelApp& app = StelApp::getInstance();
	StelObjectMgr* omgr=GETSTELMODULE(StelObjectMgr);
	const bool withAtmosphere = core->getSkyDrawer()->getFlagHasAtmosphere();
	const bool withDecimalDegree = app.getFlagShowDecimalDegrees();
	const bool useSouthAzimuth = app.getFlagSouthAzimuthUsage();
	const bool withTables = app.getFlagUseFormattingOutput();
	const bool withDesignations = app.getFlagUseCCSDesignation();
	const QString cepoch = qc_("on date", "coordinates for current epoch");
	const QString currentPlanet = core->getCurrentPlanet()->getEnglishName();
	const QString apparent = " " + (withAtmosphere ? q_("(apparent)") : "");
	QString res, firstCoordinate, secondCoordinate;
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));
	Q_UNUSED(az_app)

	if (withTables)
		res += "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";

	// TRANSLATORS: Right ascension/Declination
	const QString RADec = withDesignations ? QString("&alpha;/&delta;") : qc_("RA/Dec", "celestial coordinate system");

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
			res += QString("%1 (J2000.0): %2/%3<br/>").arg(RADec, firstCoordinate, secondCoordinate);
		res += getExtraInfoStrings(RaDecJ2000).join("");
		res += omgr->getExtraInfoStrings(RaDecJ2000).join("");
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
			res += QString("%1 (%4): %2/%3<br/>").arg(RADec, firstCoordinate, secondCoordinate, cepoch);
		res += getExtraInfoStrings(RaDecOfDate).join("");
		res += omgr->getExtraInfoStrings(RaDecOfDate).join("");
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
		const QString HADec = withDesignations ? QString("h/&delta;") : qc_("HA/Dec", "celestial coordinate system");

		if (withTables)
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td>%4</td></tr>").arg(HADec, firstCoordinate, secondCoordinate, apparent);
		else
			res += QString("%1: %2/%3 %4<br/>").arg(HADec, firstCoordinate, secondCoordinate, apparent);
		res += getExtraInfoStrings(HourAngle).join("");
		res += omgr->getExtraInfoStrings(HourAngle).join("");
	}

	if (flags&AltAzi)
	{
		// calculate alt az
		double az,alt;
		StelUtils::rectToSphe(&az,&alt,getAltAzPosGeometric(core));
		double direction = 3.; // N is zero, E is 90 degrees
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
		const QString AzAlt = (withDesignations ? "A/a" : qc_("Az./Alt.", "celestial coordinate system"));

		if (withTables)
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td>%4</td></tr>").arg(AzAlt, firstCoordinate, secondCoordinate, apparent);
		else
			res += QString("%1: %2/%3 %4<br/>").arg(AzAlt, firstCoordinate, secondCoordinate, apparent);
		res += getExtraInfoStrings(AltAzi).join("");
	}

	if (flags&GalacticCoord)
	{
		double glong, glat;
		StelUtils::rectToSphe(&glong, &glat, getGalacticPos(core));
		if (glong<0.) glong += 2.0*M_PI;
		if (withDecimalDegree)
		{
			firstCoordinate  = StelUtils::radToDecDegStr(glong);
			secondCoordinate = StelUtils::radToDecDegStr(glat);
		}
		else
		{
			firstCoordinate  = StelUtils::radToDmsStr(glong, true);
			secondCoordinate = StelUtils::radToDmsStr(glat, true);
		}

		// TRANSLATORS: Galactic longitude/latitude
		const QString GalLongLat = (withDesignations ? "l/b" : qc_("Gal. long./lat.", "celestial coordinate system"));
		if (withTables)
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td></td></tr>").arg(GalLongLat, firstCoordinate, secondCoordinate);
		else
			res += QString("%1: %2/%3<br/>").arg(GalLongLat, firstCoordinate, secondCoordinate);
		res += getExtraInfoStrings(GalacticCoord).join("");
	}

	if (flags&SupergalacticCoord)
	{
		double sglong, sglat;
		StelUtils::rectToSphe(&sglong, &sglat, getSupergalacticPos(core));
		if (sglong<0.) sglong += 2.0*M_PI;
		if (withDecimalDegree)
		{
			firstCoordinate  = StelUtils::radToDecDegStr(sglong);
			secondCoordinate = StelUtils::radToDecDegStr(sglat);
		}
		else
		{
			firstCoordinate  = StelUtils::radToDmsStr(sglong, true);
			secondCoordinate = StelUtils::radToDmsStr(sglat, true);
		}

		// TRANSLATORS: Supergalactic longitude/latitude
		const QString SGalLongLat = (withDesignations ? "SGL/SGB" : qc_("Supergal. long./lat.", "celestial coordinate system"));

		if (withTables)
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td><td></td></tr>").arg(SGalLongLat, firstCoordinate, secondCoordinate);
		else
			res += QString("%1: %2/%3<br/>").arg(SGalLongLat, firstCoordinate, secondCoordinate);
		res += getExtraInfoStrings(SupergalacticCoord).join("");
	}

	// N.B. Ecliptical coordinates are particularly earth-bound.
	// It may be OK to have terrestrial ecliptical coordinates of J2000.0 (standard epoch) because those are in practice linked with VSOP XY plane,
	// and because the ecliptical grid of J2000 is also shown for observers on other planets.
	// The formulation here has never computed the true position of any observer planet's orbital plane except for Earth,
	// or ever displayed the coordinates in the observer planet's equivalent to Earth's ecliptical coordinates.
	// As quick test you can observe if in any "Ecliptic coordinate" as seen from e.g. Mars or Jupiter the Sun was ever close to beta=0 (except if crossing the node...).

	// TRANSLATORS: Ecliptic longitude/latitude
	const QString EqlLongLat = (withDesignations ? QString("&lambda;/&beta;") :
						       qc_("Ecl. long./lat.", "celestial coordinate system") );

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
			res += QString("%1 (J2000.0): %2/%3<br/>").arg(EqlLongLat, firstCoordinate, secondCoordinate);
		res += getExtraInfoStrings(EclipticCoordJ2000).join("");
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
			res += QString("%1 (%4): %2/%3<br/>").arg(EqlLongLat, firstCoordinate, secondCoordinate, cepoch);
		res += getExtraInfoStrings(EclipticCoordOfDate).join("");

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
			res += QString("%1 (%3): %2<br/>").arg(eqlObl, firstCoordinate, cepoch);
	}

	if (withTables)
		 res += "</table>";

	// Specialized plugins (e.g. Astro Navigation or ethno-astronomical specialties) may want to provide additional types of coordinates here.
	if (flags&OtherCoord)
	{
		if (withTables)
			res += "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
		res += getExtraInfoStrings(OtherCoord).join("");
		res += omgr->getExtraInfoStrings(OtherCoord).join("");
		if (withTables)
			 res += "</table>";
	}

	if ((flags&SiderealTime) && (currentPlanet==QStringLiteral("Earth")))
	{
		const double longitude=static_cast<double>(core->getCurrentLocation().getLongitude());
		double sidereal=(get_mean_sidereal_time(core->getJD(), core->getJDE())  + longitude) / 15.;
		sidereal=StelUtils::fmodpos(sidereal, 24.);
		QString STc = q_("Mean Sidereal Time");
		QString STd = StelUtils::hoursToHmsStr(sidereal);

		if (withTables)
		{
			res += "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
			res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(STc, STd);
		}
		else
			res += QString("%1: %2<br/>").arg(STc, STd);

		if (core->getUseNutation())
		{
			sidereal=(get_apparent_sidereal_time(core->getJD(), core->getJDE()) + longitude) / 15.;
			sidereal=StelUtils::fmodpos(sidereal, 24.);
			STc = q_("Apparent Sidereal Time");
			STd = StelUtils::hoursToHmsStr(sidereal);
			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(STc, STd);
			else
				res += QString("%1: %2<br/>").arg(STc, STd);
		}
		res += getExtraInfoStrings(flags&SiderealTime).join("");
		res += omgr->getExtraInfoStrings(flags&SiderealTime).join("");
		if (withTables && !(flags&RTSTime && getType()!=QStringLiteral("Satellite")))
			res += "</table>";
	}

	if (flags&RTSTime && getType()!=QStringLiteral("Satellite") && !currentPlanet.contains("observer", Qt::CaseInsensitive) && !(core->getCurrentLocation().name.contains("->")))
	{
		const double currentJD = core->getJD();
		const double utcShift = core->getUTCOffset(currentJD) / 24.; // Fix DST shift...
		Vec4d rts = getRTSTime(core);
		QString sTransit = qc_("Transit", "celestial event; passage across a meridian");
		QString sRise = qc_("Rise", "celestial event");
		QString sSet = qc_("Set", "celestial event");
		const QString dash = QChar(0x2014);
		double sunrise = 0.;
		double sunset = 24.;
		const bool isSun = (getEnglishName()=="Sun");
		double hour(0);

		int year, month, day, currentdate;
		StelUtils::getDateFromJulianDay(currentJD+utcShift, &year, &month, &currentdate);

		if (withTables && !(flags&SiderealTime && currentPlanet==QStringLiteral("Earth")))
			res += "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";

		// Rise
		StelUtils::getDateFromJulianDay(rts[0]+utcShift, &year, &month, &day);
		if (rts[3]==30 || rts[3]<0 || rts[3]>50 || day != currentdate) // no rise
		{
			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sRise, dash);
			else
				res += QString("%1: %2<br/>").arg(sRise, dash);
		}
		else
		{
			hour = StelUtils::getHoursFromJulianDay(rts[0]+utcShift);
			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sRise, StelUtils::hoursToHmsStr(hour, true));
			else
				res += QString("%1: %2<br/>").arg(sRise, StelUtils::hoursToHmsStr(hour, true));

			sunrise = hour;
		}

		// Transit
		StelUtils::getDateFromJulianDay(rts[1]+utcShift, &year, &month, &day);
		if (rts[3]==20 || day != currentdate) // no transit
		{
			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sTransit, dash);
			else
				res += QString("%1: %2<br/>").arg(sTransit, dash);
		}
		else {
			hour = StelUtils::getHoursFromJulianDay(rts[1]+utcShift);

			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sTransit, StelUtils::hoursToHmsStr(hour, true));
			else
				res += QString("%1: %2<br/>").arg(sTransit, StelUtils::hoursToHmsStr(hour, true));
		}

		// Set
		StelUtils::getDateFromJulianDay(rts[2]+utcShift, &year, &month, &day);
		if (rts[3]==40 || rts[3]<0 || rts[3]>50 || day != currentdate) // no set
		{
			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sSet, dash);
			else
				res += QString("%1: %2<br/>").arg(sSet, dash);
		}
		else {
			hour = StelUtils::getHoursFromJulianDay(rts[2]+utcShift);
			if (withTables)
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sSet, StelUtils::hoursToHmsStr(hour, true));
			else
				res += QString("%1: %2<br/>").arg(sSet, StelUtils::hoursToHmsStr(hour, true));

			sunset = hour;
		}

		if (isSun)
		{
			QString sMTwilight = qc_("Morning twilight", "celestial event");
			QString sETwilight = qc_("Evening twilight", "celestial event");
			const double twilightAltitude = GETSTELMODULE(SpecificTimeMgr)->getTwilightAltitude();
			QString alt = QString::number(twilightAltitude, 'f', 1);
			Vec4d twilight = getRTSTime(core, twilightAltitude);
			if (twilight[3]==0.)
			{
				hour = StelUtils::getHoursFromJulianDay(twilight[0]+utcShift);
				if (withTables)
					res += QString("<tr><td>%1 (h=%2°):</td><td style='text-align:right;'>%3</td></tr>").arg(sMTwilight, alt, StelUtils::hoursToHmsStr(hour, true));
				else
					res += QString("%1 (h=%2°): %3<br/>").arg(sMTwilight, alt, StelUtils::hoursToHmsStr(hour, true));

				hour = StelUtils::getHoursFromJulianDay(twilight[2]+utcShift);
				if (withTables)
					res += QString("<tr><td>%1 (h=%2°):</td><td style='text-align:right;'>%3</td></tr>").arg(sETwilight, alt, StelUtils::hoursToHmsStr(hour, true));
				else
					res += QString("%1 (h=%2°): %3<br/>").arg(sETwilight, alt, StelUtils::hoursToHmsStr(hour, true));
			}
			double lengthOfDay = StelUtils::fmodpos(sunset - sunrise, 24.); // prevent negative value
			if (lengthOfDay<24. && rts[3]==0.) // avoid showing Daytime: 0h 00m
			{
				QString sDay = q_("Daytime");
				if (withTables)
					res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(sDay, StelUtils::hoursToHmsStr(lengthOfDay, true));
				else
					res += QString("%1: %2<br/>").arg(sDay, StelUtils::hoursToHmsStr(lengthOfDay, true));
			}
		}

		if (withTables)
			res += "</table>";

		if (rts[3]<0.)
		{
			if (isSun)
				res += q_("Polar night") + "<br />";
			else
				res += q_("This object never rises") + "<br />";
		}
		else if (rts[3]>50.)
		{
			if (isSun)
				res += q_("Polar day") + "<br />";
			else
				res += q_("Circumpolar (never sets)") + "<br />";
		}
		// These never could have been seen before (??)
		//else if (rts[0]>99. && rts[2]<99.)
		//	res += q_("Polar dawn") + "<br />";
		//else if (rts[0]<99. && rts[2]>99.)
		//	res += q_("Polar dusk") + "<br />";


		// Greatest Digression: limiting azimuth and hour angles for stars with upper culmination between pole and zenith
		double dec_equ, ra_equ;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,getEquinoxEquatorialPos(core));
		const double latitude=static_cast<double>(core->getCurrentLocation().getLatitude())*M_PI_180;
		if (((latitude>0.) && (dec_equ>=latitude)) || ((latitude<0.) && (dec_equ<=latitude)))
		{
			const double theta=acos(tan(latitude)/tan(dec_equ)); // hour angle
			double az=asin(cos(dec_equ)/cos(latitude)); // azimuth (eastern)
			// TRANSLATORS: Greatest Eastern Digression is the maximum azimuth for stars with upper culmination between pole and zenith
			QString event(q_("Max. E. Digression"));
			// TRANSLATORS: azimuth (abbrev.)
			QString azStr=(withDesignations? qc_("A", "celestial coordinate system") : qc_("Az.", "celestial coordinate system"));
			// Translators: hour angle  (abbrev.)
			QString haStr=(withDesignations? qc_("h", "celestial coordinate system") : qc_("HA", "celestial coordinate system"));
			if (latitude<0.)
				az=M_PI-az;
			if (StelApp::getInstance().getFlagSouthAzimuthUsage())
				az+=M_PI;

			if (withDecimalDegree)
			{
				firstCoordinate  = StelUtils::radToDecDegStr(az,5,false,true);
				secondCoordinate = StelUtils::radToDecDegStr(-theta);
			}
			else
			{
				firstCoordinate  = StelUtils::radToDmsStr(az,true);
				secondCoordinate = StelUtils::radToHmsStr(-theta,true);
			}

			if (withTables)
			{
				res += "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2:</td><td style='text-align:right;'>%3</td><td style='text-align:right;'>/%4:</td><td style='text-align:right;'>%5</td></tr>").arg(event,  azStr,  firstCoordinate, haStr, secondCoordinate);
			}
			else
				res += QString("%1: %2=%3, %4=%5<br/>").arg(event,  azStr,  firstCoordinate, haStr, secondCoordinate);

			// TRANSLATORS: Greatest Western Digression is the maximum western azimuth for stars with upper culmination between pole and zenith
			event=q_("Max. W. Digression");
			if (withDecimalDegree)
			{
				firstCoordinate  = StelUtils::radToDecDegStr(StelUtils::fmodpos(-az, 2.*M_PI),5,false,true);
				secondCoordinate = StelUtils::radToDecDegStr(theta);
			}
			else
			{
				firstCoordinate  = StelUtils::radToDmsStr(StelUtils::fmodpos(-az, 2.*M_PI),true);
				secondCoordinate = StelUtils::radToHmsStr(theta,true);
			}

			if (withTables)
			{
				res += QString("<tr><td>%1:</td><td style='text-align:right;'>%2:</td><td style='text-align:right;'>%3</td><td style='text-align:right;'>/%4:</td><td style='text-align:right;'>%5</td></tr>").arg(event,  azStr,  firstCoordinate, haStr, secondCoordinate);
				res += QString("</table>");
			}
			else
				res += QString("%1: %2=%3, %4=%5<br/>").arg(event, azStr,  firstCoordinate, haStr, secondCoordinate);
		}
		res += getExtraInfoStrings(flags&RTSTime).join(' ');
		res += omgr->getExtraInfoStrings(flags&RTSTime).join(' ');
	}

	if (flags&Extra)
	{
		if (getType()!=QStringLiteral("Star"))
		{
			QString pa;
			const double par = static_cast<double>(getParallacticAngle(core));
			if (withDecimalDegree)
				pa = StelUtils::radToDecDegStr(par);
			else
				pa = StelUtils::radToDmsStr(par, true);

			res += QString("%1: %2<br/>").arg(q_("Parallactic Angle"), pa);
		}
		res += getExtraInfoStrings(Extra).join("");
		res += omgr->getExtraInfoStrings(Extra).join("");
	}

	if (flags&IAUConstellation)
	{
		QString constel=core->getIAUConstellation(getEquinoxEquatorialPos(core));
		res += QString("%1: %2<br/>").arg(q_("IAU Constellation"), constel);
		res += getExtraInfoStrings(flags&IAUConstellation).join("");
		res += omgr->getExtraInfoStrings(flags&IAUConstellation).join("");
	}

	return res;
}

// Apply post processing on the info string
void StelObject::postProcessInfoString(QString& str, const InfoStringGroup& flags) const
{
	StelObjectMgr* omgr;
	omgr=GETSTELMODULE(StelObjectMgr);
	str.append(getExtraInfoStrings(Script).join(' '));
	str.append(omgr->getExtraInfoStrings(Script).join(' '));
	str.append(getExtraInfoStrings(DebugAid).join(' ')); // TBD: Remove for Release builds?
	str.append(omgr->getExtraInfoStrings(DebugAid).join(' ')); // TBD: Remove for Release builds?

	// hack for avoiding an empty line before table
	static const QRegularExpression tableRe("<br(\\s*/)?><table");
	static const QRegularExpression brRe("<br(\\s*/)?>\\s*$");
	static const QRegularExpression brRe2("<br(\\s*/)?>\\s*$");
	static const QRegularExpression tdRe("<td(\\w*)?>");
	static const QRegularExpression tableRe2("<table(\\w*)?>");
	str.replace(tableRe, "<table");
	// chomp trailing line breaks
	str.replace(brRe, "");

	if (flags&PlainText)
	{
		str.replace("<b>", "");
		str.replace("</b>", "");
		str.replace("<h2>", "");
		str.replace("</h2>", "\n");
		str.replace(brRe2, "\n");
		str.replace("<tr>", "");
		str.replace(tdRe, "");
		str.replace("<td>", "");
		str.replace("</tr>", "\n");
		str.replace(tableRe2, "");
		str.replace("</table>", "");
	}
	else if(!(flags&NoFont))
	{
		Vec3f color = getInfoColor();
		StelCore* core = StelApp::getInstance().getCore();
		if (StelApp::getInstance().getFlagOverwriteInfoColor())
		{
			// make info text more readable...
			color = StelApp::getInstance().getOverwriteInfoColor();
		}
		if (core->isBrightDaylight() && !StelApp::getInstance().getVisionModeNight())
		{
			// make info text more readable when atmosphere enabled at daylight.
			color = StelApp::getInstance().getDaylightInfoColor();
		}
		str.prepend(QString("<font color=%1>").arg(color.toHtmlColor()));
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
	map.insert("object-type", getObjectType());
	// ra/dec
	pos = getEquinoxEquatorialPos(core);
	StelUtils::rectToSphe(&ra, &dec, pos);
	map.insert("ra", ra*M_180_PI);
	map.insert("dec", dec*M_180_PI);
	map.insert("iauConstellation", core->getIAUConstellation(pos));

	if (getType()!=QStringLiteral("Star"))
		map.insert("parallacticAngle", static_cast<double>(getParallacticAngle(core))*M_180_PI);

	// Sidereal Time and hour angle
	if (core->getCurrentLocation().planetName=="Earth")
	{
		const double longitude=static_cast<double>(core->getCurrentLocation().getLongitude());
		double sidereal=(get_mean_sidereal_time(core->getJD(), core->getJDE())  + longitude) / 15.;
		sidereal=fmod(sidereal, 24.);
		if (sidereal < 0.) sidereal+=24.;
		map.insert("meanSidTm", StelUtils::hoursToHmsStr(sidereal));

		sidereal=(get_apparent_sidereal_time(core->getJD(), core->getJDE()) + longitude) / 15.;
		sidereal=fmod(sidereal, 24.);
		if (sidereal < 0.) sidereal+=24.;
		map.insert("appSidTm", StelUtils::hoursToHmsStr(sidereal));

		double ha = sidereal * 15.0 - ra * M_180_PI;
		ha=fmod(ha, 360.0);
		if (ha < 0.) ha+=360.0;
		map.insert("hourAngle-dd", ha);
		map.insert("hourAngle-hms", StelUtils::hoursToHmsStr(ha/15.0));
	}

	// ra/dec in J2000
	pos = getJ2000EquatorialPos(core);
	StelUtils::rectToSphe(&ra, &dec, pos);
	map.insert("raJ2000", ra*M_180_PI);
	map.insert("decJ2000", dec*M_180_PI);

	// apparent altitude/azimuth
	pos = getAltAzPosApparent(core);
	StelUtils::rectToSphe(&az, &alt, pos);
	double direction = 3.; // N is zero, E is 90 degrees
	if (useOldAzimuth)
		direction = 2.;
	az = direction*M_PI - az;
	if (az > M_PI*2)
		az -= M_PI*2;

	map.insert("altitude", alt*M_180_PI);
	map.insert("azimuth", az*M_180_PI);
	map.insert("airmass", getAirmass(core));

	// geometric altitude/azimuth
	pos = getAltAzPosGeometric(core);
	StelUtils::rectToSphe(&az, &alt, pos);
	az = direction*M_PI - az;
	if (az > M_PI*2)
		az -= M_PI*2;

	map.insert("altitude-geometric", alt*M_180_PI);
	map.insert("azimuth-geometric", az*M_180_PI);

	// galactic long/lat
	pos = getGalacticPos(core);
	StelUtils::rectToSphe(&glong, &glat, pos);
	if (glong<0.) glong += 2.0*M_PI;
	map.insert("glong", glong*M_180_PI);
	map.insert("glat", glat*M_180_PI);

	// supergalactic long/lat
	pos = getSupergalacticPos(core);
	StelUtils::rectToSphe(&glong, &glat, pos);
	if (glong<0.) glong += 2.0*M_PI;
	map.insert("sglong", glong*M_180_PI);
	map.insert("sglat", glat*M_180_PI);

	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	double ra_equ, dec_equ, lambda, beta;
	// J2000
	double eclJ2000 = ssmgr->getEarth()->getRotObliquity(2451545.0);
	double ecl = ssmgr->getEarth()->getRotObliquity(core->getJDE());

	// ecliptic longitude/latitude (J2000 frame)
	StelUtils::rectToSphe(&ra_equ,&dec_equ, getJ2000EquatorialPos(core));
	StelUtils::equToEcl(ra_equ, dec_equ, eclJ2000, &lambda, &beta);
	if (lambda<0) lambda+=2.0*M_PI;
	map.insert("elongJ2000", lambda*M_180_PI);
	map.insert("elatJ2000", beta*M_180_PI);

	if (QString("Earth Sun").contains(core->getCurrentLocation().planetName))
	{
		// ecliptic longitude/latitude
		StelUtils::rectToSphe(&ra_equ,&dec_equ, getEquinoxEquatorialPos(core));
		StelUtils::equToEcl(ra_equ, dec_equ, ecl, &lambda, &beta);
		if (lambda<0) lambda+=2.0*M_PI;
		map.insert("elong", lambda*M_180_PI);
		map.insert("elat", beta*M_180_PI);
	}

	// magnitude
	map.insert("vmag", getVMagnitude(core));
	map.insert("vmage", getVMagnitudeWithExtinction(core));

	// angular size
	double angularSize = getAngularRadius(core)*(2.*M_PI_180);
	Q_ASSERT(angularSize>=0.);
	map.insert("size", angularSize);
	map.insert("size-dd", angularSize*M_180_PI);
	map.insert("size-deg", StelUtils::radToDecDegStr(angularSize, 5));
	map.insert("size-dms", StelUtils::radToDmsPStr(angularSize, 2));

	// english name or designation & localized name
	map.insert("name", getEnglishName());
	map.insert("localized-name", getNameI18n());

	// 'above horizon' flag
	map.insert("above-horizon", isAboveRealHorizon(core));

	Vec4d rts = getRTSTime(core);
	if (rts[3]>-1000.)
	{
		const double utcShift = core->getUTCOffset(core->getJD()) / 24.; // Fix DST shift...
		int hr, min, sec;
		StelUtils::getTimeFromJulianDay(rts[1]+utcShift, &hr, &min, &sec);
		double hours=hr+static_cast<double>(min)/60. + static_cast<double>(sec)/3600.;

		int year, month, day, currentdate;
		StelUtils::getDateFromJulianDay(core->getJD()+utcShift, &year, &month, &currentdate);
		StelUtils::getDateFromJulianDay(rts[1]+utcShift, &year, &month, &day);
		if (rts[3]==20 || day != currentdate) // no transit
		{
			map.insert("transit", "---");
		}
		else {
			map.insert("transit", StelUtils::hoursToHmsStr(hours, true));
			map.insert("transit-dhr", hours);
		}

		StelUtils::getDateFromJulianDay(rts[0]+utcShift, &year, &month, &day);
		if (rts[3]==30 || rts[3]<0 || rts[3]>50 || day != currentdate) // no rise
		{
			map.insert("rise", "---");
		}
		else {
			StelUtils::getTimeFromJulianDay(rts[0]+utcShift, &hr, &min, &sec);
			hours=hr+static_cast<double>(min)/60. + static_cast<double>(sec)/3600.;
			map.insert("rise", StelUtils::hoursToHmsStr(hours, true));
			map.insert("rise-dhr", hours);
		}

		StelUtils::getDateFromJulianDay(rts[2]+utcShift, &year, &month, &day);
		if (rts[3]==40 || rts[3]<0 || rts[3]>50 || day != currentdate) // no set
		{
			map.insert("set", "---");
		}
		else {
			StelUtils::getTimeFromJulianDay(rts[2]+utcShift, &hr, &min, &sec);
			hours=hr+static_cast<double>(min)/60. + static_cast<double>(sec)/3600.;
			map.insert("set", StelUtils::hoursToHmsStr(hours, true));
			map.insert("set-dhr", hours);
		}
	}
	return map;
}

void StelObject::setExtraInfoString(const InfoStringGroup& flags, const QString &str)
{
	extraInfoStrings.remove(flags); // delete all entries with these flags
	if (!str.isEmpty())
		extraInfoStrings.insert(flags, str);
}
void StelObject::addToExtraInfoString(const StelObject::InfoStringGroup &flags, const QString &str)
{
	// Avoid insertion of full duplicates!
	if (!extraInfoStrings.contains(flags, str))
		extraInfoStrings.insert(flags, str);
}

QStringList StelObject::getExtraInfoStrings(const InfoStringGroup& flags) const
{
	QStringList list;
	QMultiMap<InfoStringGroup, QString>::const_iterator i = extraInfoStrings.constBegin();
	while (i != extraInfoStrings.constEnd())
	{
		if (i.key() & flags)
		{
			QString val=i.value();
			if (flags&DebugAid)
				val.prepend("DEBUG: ");
			// For unclear reasons the sequence of entries can be preserved by *pre*pending in the returned list.
			list.prepend(val);
		}
		++i;
	}
	return list;
}

void StelObject::removeExtraInfoStrings(const InfoStringGroup& flags)
{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	QMutableMultiMapIterator<InfoStringGroup, QString> i(extraInfoStrings);
#else
	QMutableMapIterator<InfoStringGroup, QString> i(extraInfoStrings);
#endif
	while (i.hasNext())
	{
		i.next();
		if (i.key() & flags)
			i.remove();
	}
}

// Add horizontal coordinates of Sun and Moon where useful
QString StelObject::getSolarLunarInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
    QString str;
    QTextStream oss(&str);
    static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
    PlanetP earth = ssystem->getEarth();
    if ((core->getCurrentPlanet()==earth) && (flags&SolarLunarPosition))
    {
	const bool withTables = StelApp::getInstance().getFlagUseFormattingOutput();
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

	if (withTables)
	    oss << "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
	const bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
	const bool withDesignations = StelApp::getInstance().getFlagUseCCSDesignation();
	double az, alt;
	QString azStr, altStr;

	if (getEnglishName()!="Sun")
	{
	    StelUtils::rectToSphe(&az,&alt,ssystem->getSun()->getAltAzPosAuto(core));
	    az = (useSouthAzimuth? 2. : 3.)*M_PI - az;
	    if (az > M_PI*2)
		az -= M_PI*2;
	    azStr  = (withDecimalDegree ? StelUtils::radToDecDegStr(az, 2)  : StelUtils::radToDmsStr(az,false));
	    altStr = (withDecimalDegree ? StelUtils::radToDecDegStr(alt, 2) : StelUtils::radToDmsStr(alt,false));

	    // TRANSLATORS: Azimuth/Altitude
	    const QString SolarAzAlt = (withDesignations ? qc_("Solar A/a", "celestial coordinate system") : qc_("Solar Az./Alt.", "celestial coordinate system"));
	    if (withTables)
	    {
		oss << QString("<tr><td>%1:</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td></tr>").arg(SolarAzAlt, azStr, altStr);
	    }
	    else
		oss << QString("%1: %2/%3<br/>").arg(SolarAzAlt, azStr, altStr);
	}
	if (getEnglishName()!="Moon")
	{
	    StelUtils::rectToSphe(&az,&alt,ssystem->getMoon()->getAltAzPosAuto(core));
	    az = (useSouthAzimuth? 2. : 3.)*M_PI - az;
	    if (az > M_PI*2)
		az -= M_PI*2;
	    azStr  = (withDecimalDegree ? StelUtils::radToDecDegStr(az, 2)  : StelUtils::radToDmsStr(az,false));
	    altStr = (withDecimalDegree ? StelUtils::radToDecDegStr(alt, 2) : StelUtils::radToDmsStr(alt,false));

	    // TRANSLATORS: Azimuth/Altitude
	    const QString LunarAzAlt = (withDesignations ? qc_("Lunar A/a", "celestial coordinate system") : qc_("Lunar Az./Alt.", "celestial coordinate system"));
	    if (withTables)
	    {
		oss << QString("<tr><td>%1:</td><td style='text-align:right;'>%2/</td><td style='text-align:right;'>%3</td></tr>").arg(LunarAzAlt, azStr, altStr);
	    }
	    else
		oss << QString("%1: %2/%3<br/>").arg(LunarAzAlt, azStr, altStr);
	}
	if (withTables)
	    oss << "</table>";
    }
    return str;
}
