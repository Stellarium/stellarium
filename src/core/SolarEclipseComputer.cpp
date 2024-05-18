/*
 * Stellarium
 * Copyright (C) 2022 Worachate Boonplod
 * Copyright (C) 2022 Georg Zotti
 * Copyright (C) 2024 Ruslan Kabatsayev
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "SolarEclipseComputer.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"
#include "StelUtils.hpp"
#include "StelCore.hpp"
#include "sidereal_time.h"

#include <QPainter>

namespace
{

static const QColor hybridEclipseColor ("#800080");
static const QColor totalEclipseColor  ("#ff0000");
static const QColor annularEclipseColor("#0000ff");
static const QColor eclipseLimitsColor ("#00ff00");

QString toKMLColorString(const QColor& c)
{
	return QString("%1%2%3%4").arg(c.alpha(),2,16,QChar('0'))
	                          .arg(c.blue (),2,16,QChar('0'))
	                          .arg(c.green(),2,16,QChar('0'))
	                          .arg(c.red  (),2,16,QChar('0'));
}

void drawContinuousEquirectGeoLine(QPainter& painter, QPointF pointA, QPointF pointB)
{
	using namespace std;
	const auto lineLengthDeg = hypot(pointB.x() - pointA.x(), pointB.y() - pointA.y());
	// For short enough lines go the simple way
	if(lineLengthDeg < 2)
	{
		painter.drawLine(pointA, pointB);
		return;
	}

	// Order them west to east
	if(pointA.x() > pointB.x())
		swap(pointA, pointB);

	Vec3d dirA;
	StelUtils::spheToRect(M_PI/180 * pointA.x(), M_PI/180 * pointA.y(), dirA);

	Vec3d dirB;
	StelUtils::spheToRect(M_PI/180 * pointB.x(), M_PI/180 * pointB.y(), dirB);

	const auto cosAngleBetweenDirs = dot(dirA, dirB);
	const auto angleMax = acos(cosAngleBetweenDirs);

	// Create an orthonormal pair of vectors that will define a plane in which we'll
	// rotate from one of the vectors towards the second one, thus creating the
	// shortest line on a unit sphere between the previous and the current directions.
	const auto firstDir = dirA;
	const auto secondDir = normalize(dirB - cosAngleBetweenDirs*firstDir);

	auto prevPoint = firstDir;
	// Keep the step no greater than 2°
	const double numPoints = max(3., ceil(lineLengthDeg / 2.));
	for(double n = 1; n < numPoints; ++n)
	{
		const auto alpha = n / (numPoints-1) * angleMax;
		const auto currPoint = cos(alpha)*firstDir + sin(alpha)*secondDir;

		double lon1, lat1;
		StelUtils::rectToSphe(&lon1, &lat1, prevPoint);

		double lon2, lat2;
		StelUtils::rectToSphe(&lon2, &lat2, currPoint);

		// If current point happens to have wrapped around 180°, bring it back to
		// the eastern side (the check relies on the ordering of the endpoints).
		if(pointA.x() > 0 && lon2 < 0)
			lon2 += 2*M_PI;

		painter.drawLine(180/M_PI * QPointF(lon1,lat1), 180/M_PI * QPointF(lon2,lat2));
		prevPoint = currPoint;
	}
}

void drawGeoLinesForEquirectMap(QPainter& painter, const std::vector<QPointF>& points)
{
	if(points.empty()) return;

	using namespace std;

	Vec3d prevDir;
	StelUtils::spheToRect(M_PI/180 * points[0].x(), M_PI/180 * points[0].y(), prevDir);
	for(unsigned n = 1; n < points.size(); ++n)
	{
		const auto currLon = M_PI/180 * points[n].x();
		const auto currLat = M_PI/180 * points[n].y();
		Vec3d currDir;
		StelUtils::spheToRect(currLon, currLat, currDir);

		const auto cosAngleBetweenDirs = dot(prevDir, currDir);
		// Create an orthonormal pair of vectors that will define a plane in which we'll
		// rotate from one of the vectors towards the second one, thus creating the
		// shortest line on a unit sphere between the previous and the current directions.
		const auto firstDir = prevDir;
		const auto secondDir = normalize(currDir - cosAngleBetweenDirs*firstDir);
		// The parametric equation for the connecting line is:
		//  P(alpha) = cos(alpha)*firstDir+sin(alpha)*secondDir.
		// Here we assume alpha>0 (otherwise we'll go the longer route over the sphere).
		//
		// Now we need to find out if this line crosses the 180° meridian. This happens
		// if there exists an alpha such that P(alpha).y==0 && P(alpha).x<0.
		// These are the solutions of this equation for alpha.
		const auto alpha1 = atan2(firstDir[1], -secondDir[1]);
		const auto alpha2 = atan2(-firstDir[1], secondDir[1]);
		const bool firstSolutionBad  = alpha1 < 0 || cos(alpha1) < cosAngleBetweenDirs;
		const bool secondSolutionBad = alpha2 < 0 || cos(alpha2) < cosAngleBetweenDirs;
		// If the line doesn't cross 180°, we are not splitting it
		if(firstSolutionBad && secondSolutionBad)
		{
			drawContinuousEquirectGeoLine(painter, points[n-1], points[n]);
			prevDir = currDir;
			continue;
		}

		const auto alpha = firstSolutionBad ? alpha2 : alpha1;
		const auto P = cos(alpha)*firstDir + sin(alpha)*secondDir;
		// Ignore the crossing of 0°
		if(P[0] > 0)
		{
			drawContinuousEquirectGeoLine(painter, points[n-1], points[n]);
			prevDir = currDir;
			continue;
		}

		// So, we've found the crossing. Let's split our line by the crossing point.
		double crossLon, crossLat;
		StelUtils::rectToSphe(&crossLon, &crossLat, P);
		crossLon *= 180/M_PI;
		crossLat *= 180/M_PI;
		const bool sameSign = (crossLon < 0 && points[n-1].x() < 0) || (crossLon >= 0 && points[n-1].x() >= 0);
		drawContinuousEquirectGeoLine(painter, points[n-1], QPointF(sameSign ? crossLon : -crossLon, crossLat));
		drawContinuousEquirectGeoLine(painter, points[n], QPointF(sameSign ? -crossLon : crossLon, crossLat));

		prevDir = currDir;
	}
}

}

EclipseBesselElements calcSolarEclipseBessel()
{
	// Besselian elements
	// Source: Explanatory Supplement to the Astronomical Ephemeris
	// and the American Ephemeris and Nautical Almanac (1961)

	EclipseBesselElements out;

	StelCore* core = StelApp::getInstance().getCore();
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	const bool topocentricWereEnabled = core->getUseTopocentricCoordinates();
	if(topocentricWereEnabled)
	{
		core->setUseTopocentricCoordinates(false);
		core->update(0);
	}

	double raMoon, deMoon, raSun, deSun;
	StelUtils::rectToSphe(&raSun, &deSun, ssystem->getSun()->getEquinoxEquatorialPos(core));
	StelUtils::rectToSphe(&raMoon, &deMoon, ssystem->getMoon()->getEquinoxEquatorialPos(core));

	double sdistanceAu = ssystem->getSun()->getEquinoxEquatorialPos(core).norm();
	const double earthRadius = ssystem->getEarth()->getEquatorialRadius()*AU;
	// Moon's distance in Earth's radius
	double mdistanceER = ssystem->getMoon()->getEquinoxEquatorialPos(core).norm() * AU / earthRadius;
	// Greenwich Apparent Sidereal Time
	const double gast = get_apparent_sidereal_time(core->getJD(), core->getJDE());

	// Avoid bug for special cases happen around Vernal Equinox
	double raDiff = StelUtils::fmodpos(raMoon-raSun, 2.*M_PI);
	if (raDiff>M_PI) raDiff-=2.*M_PI;

	constexpr double SunEarth = 109.12278;
	// ratio of Sun-Earth radius : 109.12278 = 696000/6378.1366
	// Earth's equatorial radius = 6378.1366
	// Source: IERS Conventions (2003)
	// https://www.iers.org/IERS/EN/Publications/TechnicalNotes/tn32.html

	// NASA's solar eclipse predictions use solar radius of 696,000 km
	// calculated from arctan of IAU 1976 solar radius (959.63 arcsec at 1 au).

	const double rss = sdistanceAu * 23454.7925; // from 1 AU/Earth's radius : 149597870.8/6378.1366
	const double b = mdistanceER / rss;
	const double a = raSun - ((b * cos(deMoon) * raDiff) / ((1 - b) * cos(deSun)));
	out.d = deSun - (b * (deMoon - deSun) / (1 - b));
	out.x = cos(deMoon) * sin((raMoon - a));
	out.x *= mdistanceER;
	out.y = cos(out.d) * sin(deMoon);
	out.y -= cos(deMoon) * sin(out.d) * cos((raMoon - a));
	out.y *= mdistanceER;
	double z = sin(deMoon) * sin(out.d);
	z += cos(deMoon) * cos(out.d) * cos((raMoon - a));
	z *= mdistanceER;
	const double k = 0.2725076;
	const double s = 0.272281;
	// Ratio of Moon/Earth's radius 0.2725076 is recommended by IAU for both k & s
	// s = 0.272281 is used by Fred Espenak/NASA for total eclipse to eliminate extreme cases
	// when the Moon's apparent diameter is very close to the Sun but cannot completely cover it.
	// we will use two values (same with NASA), because durations seem to agree with NASA.
	// Source: Solar Eclipse Predictions and the Mean Lunar Radius
	// http://eclipsewise.com/solar/SEhelp/SEradius.html

	// Parameters of the shadow cone
	const double f1 = asin((SunEarth + k) / (rss * (1. - b)));
	out.tf1 = tan(f1);
	const double f2 = asin((SunEarth - s) / (rss * (1. - b)));
	out.tf2 = tan(f2);
	out.L1 = z * out.tf1 + (k / cos(f1));
	out.L2 = z * out.tf2 - (s / cos(f2));
	out.mu = gast - a * M_180_PI;
	out.mu = StelUtils::fmodpos(out.mu, 360.);

	if(topocentricWereEnabled)
	{
		core->setUseTopocentricCoordinates(true);
		core->update(0);
	}

	return out;
}

EclipseBesselParameters calcBesselParameters(bool penumbra)
{
	EclipseBesselParameters out;
	StelCore* core = StelApp::getInstance().getCore();
	double JD = core->getJD();
	core->setJD(JD - 5./1440.);
	core->update(0);
	const auto ep1 = calcSolarEclipseBessel();
	const double x1 = ep1.x, y1 = ep1.y, d1 = ep1.d, mu1 = ep1.mu, L11 = ep1.L1, L21 = ep1.L2;
	core->setJD(JD + 5./1440.);
	core->update(0);
	const auto ep2 = calcSolarEclipseBessel();
	const double x2 = ep2.x, y2 = ep2.y, d2 = ep2.d, mu2 = ep2.mu, L12 = ep2.L1, L22 = ep2.L2;

	out.xdot = (x2-x1)*6.;
	out.ydot = (y2-y1)*6.;
	out.ddot = (d2-d1)*6.*M_PI_180;
	out.mudot = (mu2-mu1);
	if (out.mudot<0.) out.mudot += 360.; // make sure it is positive in case mu2 < mu1
	out.mudot = out.mudot*6.* M_PI_180;
	core->setJD(JD);
	core->update(0);
	out.elems = calcSolarEclipseBessel();
	const auto& ep = out.elems;
	double tf,L;
	if (penumbra)
	{
		L = ep.L1;
		tf = ep.tf1;
		out.ldot = (L12-L11)*6.;
	}
	else
	{
		L = ep.L2;
		tf = ep.tf2;
		out.ldot = (L22-L21)*6.;
	}
	out.etadot = out.mudot * ep.x * std::sin(ep.d);
	out.bdot = -(out.ydot-out.etadot);
	out.cdot = out.xdot + out.mudot * ep.y * std::sin(ep.d) + out.mudot * L * tf * std::cos(ep.d);

	return out;
}

void calcSolarEclipseData(double JD, double &dRatio, double &latDeg, double &lngDeg, double &altitude,
                          double &pathWidth, double &duration, double &magnitude)
{
	StelCore* core = StelApp::getInstance().getCore();
	const double currentJD = core->getJDOfLastJDUpdate(); // save current JD
	const qint64 millis = core->getMilliSecondsOfLastJDUpdate(); // keep time in sync
	const bool saveTopocentric = core->getUseTopocentricCoordinates();

	core->setUseTopocentricCoordinates(false);
	core->setJD(JD);
	core->update(0);

	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double earthRadius = ssystem->getEarth()->getEquatorialRadius()*AU;
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	static const double ff = 1./(1.-f);

	const auto ep = calcSolarEclipseBessel();
	const double rho1 = sqrt(1. - e2 * cos(ep.d) * cos(ep.d));
	const double eta1 = ep.y / rho1;
	const double sd1 = sin(ep.d) / rho1;
	const double cd1 = sqrt(1. - e2) * cos(ep.d) / rho1;
	const double rho2 = sqrt(1.- e2 * sin(ep.d) * sin(ep.d));
	const double sd1d2 = e2*sin(ep.d)*cos(ep.d) / (rho1*rho2);
	const double cd1d2 = sqrt(1. - sd1d2 * sd1d2);
	const double p = 1. - ep.x * ep.x - eta1 * eta1;

	if (p > 0.) // Central eclipse : Moon's shadow axis is touching Earth
	{
		const double zeta1 = sqrt(p);
		const double zeta = rho2 * (zeta1 * cd1d2 - eta1 * sd1d2);
		const double L2a = ep.L2 - zeta * ep.tf2;
		const double b = -ep.y * sin(ep.d) + zeta * cos(ep.d);
		const double theta = atan2(ep.x, b) * M_180_PI;
		lngDeg = theta - ep.mu;
		lngDeg = StelUtils::fmodpos(lngDeg, 360.);
		if (lngDeg > 180.) lngDeg -= 360.;
		const double sfn1 = eta1 * cd1 + zeta1 * sd1;
		const double cfn1 = sqrt(1. - sfn1 * sfn1);
		latDeg = atan(ff * sfn1 / cfn1) / M_PI_180;
		const double L1a = ep.L1 - zeta * ep.tf1;
		magnitude = L1a / (L1a + L2a);
		dRatio = 1.+(magnitude-1.)*2.;

		core->setJD(JD - 5./1440.);
		core->update(0);

		const auto ep1 = calcSolarEclipseBessel();

		core->setJD(JD + 5./1440.);
		core->update(0);

		const auto ep2 = calcSolarEclipseBessel();

		// Hourly rate
		const double xdot = (ep2.x - ep1.x) * 6.;
		const double ydot = (ep2.y - ep1.y) * 6.;
		const double ddot = (ep2.d - ep1.d) * 6.;
		double mudot = (ep2.mu - ep1.mu);
		if (mudot<0.) mudot += 360.; // make sure it is positive in case ep2.mu < ep1.mu
		mudot = mudot * 6.* M_PI_180;

		// Duration of central eclipse in minutes
		const double etadot = mudot * ep.x * sin(ep.d) - ddot * zeta;
		const double xidot = mudot * (-ep.y * sin(ep.d) + zeta * cos(ep.d));
		const double n = sqrt((xdot - xidot) * (xdot - xidot) + (ydot - etadot) * (ydot - etadot));
		duration = L2a*120./n; // positive = annular eclipse, negative = total eclipse

		// Approximate altitude
		altitude = asin(cfn1*cos(ep.d)*cos(theta * M_PI_180)+sfn1*sin(ep.d)) / M_PI_180;

		// Path width in kilometers
		// Explanatory Supplement to the Astronomical Almanac
		// Seidelmann, P. Kenneth, ed. (1992). University Science Books. ISBN 978-0-935702-68-2
		// https://archive.org/details/131123ExplanatorySupplementAstronomicalAlmanac
		// Path width for central solar eclipses which only part of umbra/antumbra touches Earth
		// are too wide and could give a false impression, annular eclipse of 2003 May 31, for example.
		// We have to check this in the next step by calculating northern/southern limit of umbra/antumbra.
		// Don't show the path width if there is no northern limit or southern limit.
		// We will eventually have to calculate both limits, if we want to draw eclipse path on world map.
		const double p1 = zeta * zeta;
		const double p2 = ep.x * (xdot - xidot) / n;
		const double p3 = eta1 * (ydot - etadot) / n;
		const double p4 = (p2 + p3) * (p2 + p3);
		pathWidth = abs(earthRadius*2.*L2a/sqrt(p1+p4));
	}
	else  // Partial eclipse or non-central eclipse
	{
		const double yy1 = ep.y / rho1;
		double xi = ep.x / sqrt(ep.x * ep.x + yy1 * yy1);
		const double eta1 = yy1 / sqrt(ep.x * ep.x + yy1 * yy1);
		const double sd1 = sin(ep.d) / rho1;
		const double cd1 = sqrt(1.- e2) * cos(ep.d) / rho1;
		const double rho2 = sqrt(1.- e2 * sin(ep.d) * sin(ep.d));
		const double sd1d2 = e2 * sin(ep.d) * cos(ep.d) / (rho1 * rho2);
		double zeta = rho2 * (-(eta1) * sd1d2);
		const double b = -eta1 * sd1;
		double theta = atan2(xi, b);
		const double sfn1 = eta1*cd1;
		const double cfn1 = sqrt(1.- sfn1 * sfn1);
		double lat = ff * sfn1 / cfn1;
		lat = atan(lat);
		const double L1 = ep.L1 - zeta * ep.tf1;
		const double L2 = ep.L2 - zeta * ep.tf2;
		const double c = 1. / sqrt(1.- e2 * sin(lat) * sin(lat));
		const double s = (1.- e2) * c;
		const double rs = s * sin(lat);
		const double rc = c * cos(lat);
		xi = rc * sin(theta);
		const double eta = rs * cos(ep.d) - rc * sin(ep.d) * cos(theta);
		const double u = ep.x - xi;
		const double v = ep.y - eta;
		magnitude = (L1 - sqrt(u * u + v * v)) / (L1 + L2);
		dRatio = 1.+ (magnitude - 1.)* 2.;
		theta = theta / M_PI_180;
		lngDeg = theta - ep.mu;
		lngDeg = StelUtils::fmodpos(lngDeg, 360.);
		if (lngDeg > 180.) lngDeg -= 360.;
		latDeg = lat / M_PI_180;
		duration = 0.;
		pathWidth = 0.;
	}
	core->setJD(currentJD);
	core->setMilliSecondsOfLastJDUpdate(millis); // restore millis.
	core->setUseTopocentricCoordinates(saveTopocentric);
	core->update(0);
}

SolarEclipseComputer::SolarEclipseComputer(StelCore* core, StelLocaleMgr* localeMgr)
	: core(core)
	, localeMgr(localeMgr)
{
}

auto SolarEclipseComputer::generateEclipseMap(const double JDMid) const -> EclipseMapData
{
	const bool savedTopocentric = core->getUseTopocentricCoordinates();
	const double currentJD = core->getJD(); // save current JD
	core->setUseTopocentricCoordinates(false);
	core->update(0);

	EclipseMapData data;

	bool partialEclipse = false;
	bool nonCentralEclipse = false;
	double dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude;
	core->setJD(JDMid);
	core->update(0);
	const auto ep = calcSolarEclipseBessel();
	const double x = ep.x, y = ep.y;
	double gamma = std::sqrt(x*x+y*y);
	// Type of eclipse
	if (abs(gamma) > 0.9972 && abs(gamma) < (1.5433 + ep.L2))
	{
		if (abs(gamma) < 0.9972 + abs(ep.L2))
		{
			partialEclipse = false;
			nonCentralEclipse = true; // non-central total/annular eclipse
		}
		else
			partialEclipse = true;
	}
	const double JDP1 = getJDofContact(JDMid,true,true,true,true);
	const double JDP4 = getJDofContact(JDMid,false,true,true,true);

	double JDP2 = 0., JDP3 = 0.;
	GeoPoint coordinates;
	// Check northern/southern limits of penumbra at greatest eclipse
	bool bothPenumbralLimits = false;
	coordinates = getNSLimitOfShadow(JDMid,true,true);
	double latPL1 = coordinates.latitude;
	coordinates = getNSLimitOfShadow(JDMid,false,true);
	double latPL2 = coordinates.latitude;
	if (latPL1 <= 90. && latPL2 <= 90.)
		bothPenumbralLimits = true;

	if (bothPenumbralLimits)
	{
		JDP2 = getJDofContact(JDMid,true,true,true,false);
		JDP3 = getJDofContact(JDMid,false,true,true,false);
	}

	// Generate GE
	data.greatestEclipse.JD = JDMid;
	calcSolarEclipseData(JDMid,dRatio,data.greatestEclipse.latitude,data.greatestEclipse.longitude,altitude,pathWidth,duration,magnitude);

	// Generate P1
	data.firstContactWithEarth.JD = JDP1;
	calcSolarEclipseData(JDP1,dRatio,data.firstContactWithEarth.latitude,data.firstContactWithEarth.longitude,altitude,pathWidth,duration,magnitude);

	// Generate P4
	data.lastContactWithEarth.JD = JDP4;
	calcSolarEclipseData(JDP4,dRatio,data.lastContactWithEarth.latitude,data.lastContactWithEarth.longitude,altitude,pathWidth,duration,magnitude);

	// Northern/southern Limits of penumbra
	bool north = true;
	for (int j = 0; j < 2; j++)
	{
		if (j != 0) north = false;
		auto& points = data.penumbraLimits[j];
		double JD = JDP1;
		int i = 0;
		while (JD < JDP4)
		{
			JD = JDP1 + i/1440.0;
			coordinates = getNSLimitOfShadow(JD,north,true);
			points.emplace_back(JD, coordinates.longitude, coordinates.latitude);
			i++;
		}

		if(points.empty()) continue;

		// Refine at the beginning and the end of the line so as to find the precise endpoints

		// 1. Beginning of the line
		const auto firstValidIt = std::find_if(points.begin(), points.end(),
		                                       [](const auto& p){ return p.latitude <= 90; });
		if (firstValidIt == points.end()) continue;
		const int firstValidPos = firstValidIt - points.begin();
		if (firstValidPos > 0)
		{
			double lastInvalidTime = points[firstValidPos - 1].JD;
			double firstValidTime = points[firstValidPos].JD;
			// Bisect between these times. The sufficient number of iterations was found empirically.
			for (int n = 0; n < 15; ++n)
			{
				const auto currTime = (lastInvalidTime + firstValidTime) / 2;
				const auto coords = getNSLimitOfShadow(currTime,north,true);
				if (coords.latitude > 90)
				{
					lastInvalidTime = currTime;
				}
				else
				{
					firstValidTime = currTime;
					points.emplace_front(currTime, coords.longitude, coords.latitude);
				}
			}
		}

		// 2. End of the line
		const auto lastValidIt = std::find_if(points.rbegin(), points.rend(),
		                                      [](const auto& p){ return p.latitude <= 90; });
		if (lastValidIt == points.rend()) continue;
		const int lastValidPos = points.size() - 1 - (lastValidIt - points.rbegin());
		if (lastValidPos + 1u < points.size())
		{
			double firstInvalidTime = points[lastValidPos + 1].JD;
			double lastValidTime = points[lastValidPos].JD;
			// Bisect between these times. The sufficient number of iterations was found empirically.
			for (int n = 0; n < 15; ++n)
			{
				const auto currTime = (firstInvalidTime + lastValidTime) / 2;
				const auto coords = getNSLimitOfShadow(currTime,north,true);
				if (coords.latitude > 90)
				{
					firstInvalidTime = currTime;
				}
				else
				{
					lastValidTime = currTime;
					points.emplace_back(currTime, coords.longitude, coords.latitude);
				}
			}
		}

		// 3. Cleanup: remove invalid points, sort by time increase
		points.erase(std::remove_if(points.begin(), points.end(), [](const auto& p) { return p.latitude > 90; }),
		             points.end());
		std::sort(points.begin(), points.end(), [](const auto& a, const auto& b) { return a.JD < b.JD; });
	}

	// Eclipse begins/ends at sunrise/sunset curve
	if (bothPenumbralLimits)
	{
		double latP2, lngP2, latP3, lngP3;
		bool first = true;
		for (int j = 0; j < 2; j++)
		{
			if (j != 0) first = false;
			// P1 to P2 curve
			core->setJD(JDP2);
			core->update(0);
			auto ep = calcSolarEclipseBessel();
			coordinates = getContactCoordinates(ep.x, ep.y, ep.d, ep.mu);
			latP2 = coordinates.latitude;
			lngP2 = coordinates.longitude;
			auto& limit = data.riseSetLimits[j].emplace<EclipseMapData::TwoLimits>();

			limit.p12curve.emplace_back(data.firstContactWithEarth.longitude,
			                            data.firstContactWithEarth.latitude);
			double JD = JDP1;
			int i = 0;
			while (JD < JDP2)
			{
				JD = JDP1 + i/1440.0;
				core->setJD(JD);
				core->update(0);
				ep = calcSolarEclipseBessel();
				coordinates = getRiseSetLineCoordinates(first, ep.x, ep.y, ep.d, ep.L1, ep.mu);
				if (coordinates.latitude <= 90.)
					limit.p12curve.emplace_back(coordinates.longitude, coordinates.latitude);
				i++;
			}
			limit.p12curve.emplace_back(lngP2, latP2);

			// P3 to P4 curve
			core->setJD(JDP3);
			core->update(0);
			ep = calcSolarEclipseBessel();
			coordinates = getContactCoordinates(ep.x, ep.y, ep.d, ep.mu);
			latP3 = coordinates.latitude;
			lngP3 = coordinates.longitude;
			limit.p34curve.emplace_back(lngP3, latP3);
			JD = JDP3;
			i = 0;
			while (JD < JDP4)
			{
				JD = JDP3 + i/1440.0;
				core->setJD(JD);
				core->update(0);
				ep = calcSolarEclipseBessel();
				coordinates = getRiseSetLineCoordinates(first, ep.x, ep.y, ep.d, ep.L1, ep.mu);
				if (coordinates.latitude <= 90.)
					limit.p34curve.emplace_back(coordinates.longitude, coordinates.latitude);
				i++;
			}
			limit.p34curve.emplace_back(data.lastContactWithEarth.longitude,
			                            data.lastContactWithEarth.latitude);
		}
	}
	else
	{
		// Only northern or southern limit exists
		// Draw the curve between P1 to P4
		bool first = true;
		for (int j = 0; j < 2; j++)
		{
			if (j != 0) first = false;
			auto& limit = data.riseSetLimits[j].emplace<EclipseMapData::SingleLimit>();
			limit.curve.emplace_back(data.firstContactWithEarth.longitude,
			                         data.firstContactWithEarth.latitude);
			double JD = JDP1;
			int i = 0;
			while (JD < JDP4)
			{
				JD = JDP1 + i/1440.0;
				core->setJD(JD);
				core->update(0);
				const auto ep = calcSolarEclipseBessel();
				coordinates = getRiseSetLineCoordinates(first, ep.x, ep.y, ep.d, ep.L1, ep.mu);
				if (coordinates.latitude <= 90.)
					limit.curve.emplace_back(coordinates.longitude, coordinates.latitude);
				i++;
			}
			limit.curve.emplace_back(data.lastContactWithEarth.longitude,
			                         data.lastContactWithEarth.latitude);
		}
	}

	// Curve of maximum eclipse at sunrise/sunset
	// There are two parts of the curve
	bool first = true;
	for (int j = 0; j < 2; j++)
	{
		if (j!= 0) first = false;
		if(bothPenumbralLimits)
		{
			// The curve corresponding to P1-P2 time interval
			data.maxEclipseAtRiseSet.emplace_back();
			auto* curve = &data.maxEclipseAtRiseSet.back();
			auto JDmin = JDP1;
			auto JDmax = JDP2;
			int numPoints = 5;
			bool goodPointFound = false;
			do
			{
				curve->clear();
				numPoints = 2*numPoints+1;
				const auto step = (JDmax-JDmin)/numPoints;
				// We use an extended interval of n to include min and max values of JD. The internal
				// values of JD are chosen in such a way that after increasing numPoints at the next
				// iteration we'd check the points between the ones we checked in the previous
				// iteration, thus more efficiently searching for good points, avoiding rechecking
				// the same JD values.
				for(int n = -1; n < numPoints+1; ++n)
				{
					const auto JD = std::clamp(JDmin + step*(n+0.5), JDmin, JDmax);
					const auto coordinates = getMaximumEclipseAtRiseSet(first,JD);
					curve->emplace_back(JD, coordinates.longitude, coordinates.latitude);
					if (abs(coordinates.latitude) <= 90.)
						goodPointFound = true;
					else if(goodPointFound)
						break; // we've obtained a bad point after a good one, can refine now
				}
			}
			while(!goodPointFound && numPoints < 500);

			// We can't refine the curve if we have no usable points, so clear it (don't
			// remove because we need it to match first and second branches).
			if(!goodPointFound) curve->clear();

			// The curve corresponding to P3-P4 time interval
			data.maxEclipseAtRiseSet.emplace_back();
			curve = &data.maxEclipseAtRiseSet.back();
			JDmin = JDP3;
			JDmax = JDP4;
			numPoints = 5;
			goodPointFound = false;
			do
			{
				curve->clear();
				numPoints = 2*numPoints+1;
				const auto step = (JDmax-JDmin)/numPoints;
				// We use an extended interval of n to include min and max values of JD. The internal
				// values of JD are chosen in such a way that after increasing numPoints at the next
				// iteration we'd check the points between the ones we checked in the previous
				// iteration, thus more efficiently searching for good points, avoiding rechecking
				// the same JD values.
				for(int n = -1; n < numPoints+1; ++n)
				{
					const auto JD = std::clamp(JDmin + step*(n+0.5), JDmin, JDmax);
					const auto coordinates = getMaximumEclipseAtRiseSet(first,JD);
					curve->emplace_back(JD, coordinates.longitude, coordinates.latitude);
					if (abs(coordinates.latitude) <= 90.)
						goodPointFound = true;
					else if(goodPointFound)
						break; // we've obtained a bad point after a good one, can refine now
				}
			}
			while(!goodPointFound && numPoints < 500);

			// We can't refine the curve if we have no usable points, so clear it (don't
			// remove because we need it to match first and second branches).
			if(!goodPointFound) curve->clear();
		}
		else
		{
			// The curve corresponding to P1-P4 time interval
			data.maxEclipseAtRiseSet.emplace_back();
			auto*const curve = &data.maxEclipseAtRiseSet.back();
			const auto JDmin = JDP1;
			const auto JDmax = JDP4;
			int numPoints = 5;
			bool goodPointFound = false;
			do
			{
				curve->clear();
				numPoints = 2*numPoints+1;
				const auto step = (JDmax-JDmin)/numPoints;
				// We use an extended interval of n to include min and max values of JD. The internal
				// values of JD are chosen in such a way that after increasing numPoints at the next
				// iteration we'd check the points between the ones we checked in the previous
				// iteration, thus more efficiently searching for good points, avoiding rechecking
				// the same JD values.
				for(int n = -1; n < numPoints+1; ++n)
				{
					const auto JD = std::clamp(JDmin + step*(n+0.5), JDmin, JDmax);
					const auto coordinates = getMaximumEclipseAtRiseSet(first,JD);
					curve->emplace_back(JD, coordinates.longitude, coordinates.latitude);
					if (abs(coordinates.latitude) <= 90.)
						goodPointFound = true;
					else if(goodPointFound)
						break; // we've obtained a bad point after a good one, can refine now
				}
			}
			while(!goodPointFound && numPoints < 500);

			// We can't refine the curve if we have no usable points, so clear it (don't
			// remove because we need it to match first and second branches).
			if(!goodPointFound) curve->clear();
		}
	}
	// Refine at the beginnings and the ends of the lines so as to find the precise endpoints
	for(unsigned c = 0; c < data.maxEclipseAtRiseSet.size(); ++c)
	{
		auto& points = data.maxEclipseAtRiseSet[c];
		const bool first = c < data.maxEclipseAtRiseSet.size() / 2;

		// 1. Beginning of the line
		const auto firstValidIt = std::find_if(points.begin(), points.end(),
						       [](const auto& p){ return p.latitude <= 90; });
		if (firstValidIt == points.end()) continue;
		const int firstValidPos = firstValidIt - points.begin();
		if (firstValidPos > 0)
		{
			double lastInvalidTime = points[firstValidPos - 1].JD;
			double firstValidTime = points[firstValidPos].JD;
			// Bisect between these times. The sufficient number of iterations was found empirically.
			for (int n = 0; n < 15; ++n)
			{
				const auto currTime = (lastInvalidTime + firstValidTime) / 2;
				const auto coords = getMaximumEclipseAtRiseSet(first, currTime);
				if (coords.latitude > 90)
				{
					lastInvalidTime = currTime;
				}
				else
				{
					firstValidTime = currTime;
					points.emplace_front(currTime, coords.longitude, coords.latitude);
				}
			}
		}

		// 2. End of the line
		const auto lastValidIt = std::find_if(points.rbegin(), points.rend(),
						      [](const auto& p){ return p.latitude <= 90; });
		if (lastValidIt == points.rend()) continue;
		const int lastValidPos = points.size() - 1 - (lastValidIt - points.rbegin());
		if (lastValidPos + 1u < points.size())
		{
			double firstInvalidTime = points[lastValidPos + 1].JD;
			double lastValidTime = points[lastValidPos].JD;
			// Bisect between these times. The sufficient number of iterations was found empirically.
			for (int n = 0; n < 15; ++n)
			{
				const auto currTime = (firstInvalidTime + lastValidTime) / 2;
				const auto coords = getMaximumEclipseAtRiseSet(first, currTime);
				if (coords.latitude > 90)
				{
					firstInvalidTime = currTime;
				}
				else
				{
					lastValidTime = currTime;
					points.emplace_back(currTime, coords.longitude, coords.latitude);
				}
			}
		}

		// 3. Cleanup: remove invalid points, sort by time increase
		points.erase(std::remove_if(points.begin(), points.end(), [](const auto& p) { return p.latitude > 90; }),
			     points.end());
		std::sort(points.begin(), points.end(), [](const auto& a, const auto& b) { return a.JD < b.JD; });

		// Refine too long internal segments
		bool newPointsInserted;
		do
		{
			newPointsInserted = false;
			const unsigned origNumPoints = points.size();
			for(unsigned n = 1; n < origNumPoints; ++n)
			{
				const auto prevLat = points[n-1].latitude;
				const auto currLat = points[n].latitude;
				const auto prevLon = points[n-1].longitude;
				const auto currLon = points[n].longitude;
				auto lonDiff = currLon - prevLon;
				while(lonDiff >  180) lonDiff -= 360;
				while(lonDiff < -180) lonDiff += 360;
				constexpr double admissibleStepDeg = 5;
				constexpr double admissibleStepDegSqr = admissibleStepDeg*admissibleStepDeg;
				// We want to sample more densely near the pole, where the rise/set curve may have
				// more features, so using unscaled longitude as one of the coordinates is on purpose.
				if(Vec2d(prevLat - currLat, lonDiff).normSquared() < admissibleStepDegSqr)
					continue;

				const auto JD = (points[n-1].JD + points[n].JD) / 2;
				const auto coordinates = getMaximumEclipseAtRiseSet(first,JD);
				points.emplace_back(JD, coordinates.longitude, coordinates.latitude);
				newPointsInserted = true;
			}
			std::sort(points.begin(), points.end(), [](const auto& a, const auto& b) { return a.JD < b.JD; });
		} while(newPointsInserted);

		// Connect the first and second branches of the lines
		if(!first)
		{
			const auto firstBranchIndex = c - data.maxEclipseAtRiseSet.size() / 2;
			auto& firstBranch  = data.maxEclipseAtRiseSet[firstBranchIndex];
			auto& secondBranch = data.maxEclipseAtRiseSet[c];
			if(firstBranch.empty() || secondBranch.empty())
				continue;
			// Connect the points that are closer to each other by time
			if(std::abs(secondBranch.back().JD - firstBranch.back().JD) < std::abs(secondBranch.front().JD - firstBranch.front().JD))
				secondBranch.emplace_back(firstBranch.back());
			else
				secondBranch.emplace_front(firstBranch.front());
		}
	}

	if (!partialEclipse)
	{
		double JDC1 = JDMid, JDC2 = JDMid;
		const double JDU1 = getJDofContact(JDMid,true,false,true,true); // beginning of external (ant)umbral contact
		const double JDU4 = getJDofContact(JDMid,false,false,true,true); // end of external (ant)umbral contact
		calcSolarEclipseData(JDC1,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
		if (!nonCentralEclipse)
		{
			// C1
			double JD = getJDofContact(JDMid,true,false,false,true);
			JD = int(JD)+(int((JD-int(JD))*86400.)-1)/86400.;
			calcSolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			int steps = 0;
			while (pathWidth<0.0001 && steps<20)
			{
				JD += .1/86400.;
				calcSolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				steps += 1;
			}
			JDC1 = JD;
			core->setJD(JDC1);
			core->update(0);
			auto ep = calcSolarEclipseBessel();
			coordinates = getContactCoordinates(ep.x, ep.y, ep.d, ep.mu);
			double latC1 = coordinates.latitude;
			double lngC1 = coordinates.longitude;

			// Generate C1
			data.centralEclipseStart.JD = JDC1;
			data.centralEclipseStart.longitude = lngC1;
			data.centralEclipseStart.latitude = latC1;

			// C2
			JD = getJDofContact(JDMid,false,false,false,true);
			JD = int(JD)+(int((JD-int(JD))*86400.)+1)/86400.;
			calcSolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			steps = 0;
			while (pathWidth<0.0001 && steps<20)
			{
				JD -= .1/86400.;
				calcSolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				steps += 1;
			}
			JDC2 = JD;
			core->setJD(JDC2);
			core->update(0);
			ep = calcSolarEclipseBessel();
			coordinates = getContactCoordinates(ep.x, ep.y, ep.d, ep.mu);
			double latC2 = coordinates.latitude;
			double lngC2 = coordinates.longitude;

			// Generate C2
			data.centralEclipseEnd.JD = JDC2;
			data.centralEclipseEnd.longitude = lngC2;
			data.centralEclipseEnd.latitude = latC2;

			// Center line
			JD = JDC1;
			int i = 0;
			double dRatioC1 = dRatio;
			calcSolarEclipseData(JDMid,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			double dRatioMid = dRatio;
			calcSolarEclipseData(JDC2,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			double dRatioC2 = dRatio;
			if (dRatioC1 >= 1. && dRatioMid >= 1. && dRatioC2 >= 1.)
				data.eclipseType = EclipseMapData::EclipseType::Total;
			else if (dRatioC1 < 1. && dRatioMid < 1. && dRatioC2 < 1.)
				data.eclipseType = EclipseMapData::EclipseType::Annular;
			else
				data.eclipseType = EclipseMapData::EclipseType::Hybrid;
			data.centerLine.emplace_back(lngC1, latC1);
			while (JD+(1./1440.) < JDC2)
			{
				JD = JDC1 + i/1440.; // generate every one minute
				calcSolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				data.centerLine.emplace_back(lngDeg, latDeg);
				i++;
			}
			data.centerLine.emplace_back(lngC2, latC2);
		}
		else
		{
			JDC1 = JDMid;
			JDC2 = JDMid;
		}

		double dRatioC1 = dRatio;
		calcSolarEclipseData(JDMid,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
		double dRatioMid = dRatio;
		calcSolarEclipseData(JDC2,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
		double dRatioC2 = dRatio;
		// Umbra/antumbra outline
		// we want to draw (ant)umbral shadow on world map at exact times like 09:00, 09:10, 09:20, ...
		double beginJD = int(JDU1)+(10.*int(1440.*(JDU1-int(JDU1))/10.)+10.)/1440.;
		double endJD = int(JDU4)+(10.*int(1440.*(JDU4-int(JDU4))/10.))/1440.;
		double JD = beginJD;
		int i = 0;
		double lat0 = 0., lon0 = 0.;
		while (JD < endJD)
		{
			JD = beginJD + i/144.; // generate every 10 minutes
			core->setJD(JD);
			core->update(0);
			const auto ep = calcSolarEclipseBessel();
			calcSolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			double angle = 0.;
			bool firstPoint = false;
			auto& outline = data.umbraOutlines.emplace_back();
			outline.JD = JD;
			if (dRatio>=1.)
				outline.eclipseType = EclipseMapData::EclipseType::Total;
			else
				outline.eclipseType = EclipseMapData::EclipseType::Annular;
			int pointNumber = 0;
			while (pointNumber < 60)
			{
				angle = pointNumber*M_PI*2./60.;
				coordinates = getShadowOutlineCoordinates(angle, ep.x, ep.y, ep.d, ep.L2, ep.tf2, ep.mu);
				if (coordinates.latitude <= 90.)
				{
					outline.curve.emplace_back(coordinates.longitude, coordinates.latitude);
					if (!firstPoint)
					{
						lat0 = coordinates.latitude;
						lon0 = coordinates.longitude;
						firstPoint = true;
					}
				}
				pointNumber++;
			}
			outline.curve.emplace_back(lon0, lat0); // completing the circle
			i++;
		}

		// Extreme northern/southern limits of umbra/antumbra at C1
		const auto C1a = getExtremeNSLimitOfShadow(JDC1,true,false,true);
		const auto C1b = getExtremeNSLimitOfShadow(JDC1,false,false,true);

		// Extreme northern/southern limits of umbra/antumbra at C2
		const auto C2a = getExtremeNSLimitOfShadow(JDC2,true,false,false);
		const auto C2b = getExtremeNSLimitOfShadow(JDC2,false,false,false);

		double dRatio,altitude,pathWidth,duration,magnitude;
		calcSolarEclipseData(JDC1,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);

		auto& extremeLimit1 = data.extremeUmbraLimit1.emplace_back();
		if (dRatioC1 >= 1. && dRatioMid >= 1. && dRatioC2 >= 1.)
			extremeLimit1.eclipseType = EclipseMapData::EclipseType::Total;
		else if (dRatioC1 < 1. && dRatioMid < 1. && dRatioC2 < 1.)
			extremeLimit1.eclipseType = EclipseMapData::EclipseType::Annular;
		else
			extremeLimit1.eclipseType = EclipseMapData::EclipseType::Hybrid;
		// 1st extreme limit at C1
		if (C1a.latitude <= 90. || C1b.latitude <= 90.)
		{
			if (dRatio>=1.)
				extremeLimit1.curve.emplace_back(C1a.longitude, C1a.latitude);
			else
				extremeLimit1.curve.emplace_back(C1b.longitude, C1b.latitude);
		}
		JD = JDC1-20./1440.;
		i = 0;
		while (JD < JDC2+20./1440.)
		{
			JD = JDC1+(i-20.)/1440.;
			coordinates = getNSLimitOfShadow(JD,true,false);
			if (coordinates.latitude <= 90.)
				extremeLimit1.curve.emplace_back(coordinates.longitude, coordinates.latitude);
			i++;
		}

		calcSolarEclipseData(JDC2,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
		// 1st extreme limit at C2
		if (C2a.latitude <= 90. || C2b.latitude <= 90.)
		{
			if (dRatio>=1.)
				extremeLimit1.curve.emplace_back(C2a.longitude, C2a.latitude);
			else
				extremeLimit1.curve.emplace_back(C2b.longitude, C2b.latitude);
		}

		auto& extremeLimit2 = data.extremeUmbraLimit1.emplace_back();
		// 2nd extreme limit at C1
		if (dRatioC1 >= 1. && dRatioMid >= 1. && dRatioC2 >= 1.)
			extremeLimit2.eclipseType = EclipseMapData::EclipseType::Total;
		else if (dRatioC1 < 1. && dRatioMid < 1. && dRatioC2 < 1.)
			extremeLimit2.eclipseType = EclipseMapData::EclipseType::Annular;
		else
			extremeLimit2.eclipseType = EclipseMapData::EclipseType::Hybrid;
		calcSolarEclipseData(JDC1,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
		if (C1a.latitude <= 90. || C1b.latitude <= 90.)
		{
			if (dRatio>=1.)
				extremeLimit2.curve.emplace_back(C1b.longitude, C1b.latitude);
			else
				extremeLimit2.curve.emplace_back(C1a.longitude, C1a.latitude);
		}
		JD = JDC1-20./1440.;
		i = 0;
		while (JD < JDC2+20./1440.)
		{
			JD = JDC1+(i-20.)/1440.;
			coordinates = getNSLimitOfShadow(JD,false,false);
			if (coordinates.latitude <= 90.)
				extremeLimit2.curve.emplace_back(coordinates.longitude, coordinates.latitude);
			i++;
		}
		calcSolarEclipseData(JDC2,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
		// 2nd extreme limit at C2
		if (C2a.latitude <= 90. || C2b.latitude <= 90.)
		{
			if (dRatio>=1.)
				extremeLimit2.curve.emplace_back(C2b.longitude, C2b.latitude);
			else
				extremeLimit2.curve.emplace_back(C2a.longitude, C2a.latitude);
		}
	}

	core->setJD(currentJD);
	core->setUseTopocentricCoordinates(savedTopocentric);
	core->update(0);

	return data;
}

auto SolarEclipseComputer::getNSLimitOfShadow(double JD, bool northernLimit, bool penumbra) const -> GeoPoint
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	GeoPoint coordinates;
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	static const double ff = 1./(1.-f);
	core->setJD(JD);
	core->update(0);
	const auto bp = calcBesselParameters(penumbra);
	const double x = bp.elems.x, y = bp.elems.y, d = bp.elems.d, tf1 = bp.elems.tf1,
	             tf2 = bp.elems.tf2, L1 = bp.elems.L1, L2 = bp.elems.L2, mu = bp.elems.mu;
	const double rho1 = std::sqrt(1.-e2*std::cos(d)*std::cos(d));
	const double y1 = y/rho1;
	double eta1 = y1;
	const double sd1 = std::sin(d)/rho1;
	const double cd1 = std::sqrt(1.-e2)*std::cos(d)/rho1;
	const double rho2 = std::sqrt(1.-e2*std::sin(d)*std::sin(d));
	const double sd1d2 = e2*std::sin(d)*std::cos(d)/(rho1*rho2);
	const double cd1d2 = std::sqrt(1.-sd1d2*sd1d2);
	double zeta = rho2*(-eta1*sd1d2);
	double tf, L;
	if (penumbra)
	{
		L = L1;
		tf = tf1;
	}
	else
	{
		L = L2;
		tf = tf2;
	}
	double Lb = L-zeta*tf;
	double xidot = bp.mudot*(-y*std::sin(d)+zeta*std::cos(d));
	double tq = -(bp.ydot-bp.etadot)/(bp.xdot-xidot);
	double sq = std::sin(std::atan(tq));
	double cq = std::cos(std::atan(tq));
	if (!northernLimit)
	{
		sq *= -1.;
		cq *= -1.;
	}
	double xi = x-Lb*sq;
	eta1 = y1-Lb*cq/rho1;
	zeta = 1.-xi*xi-eta1*eta1;

	if (zeta < 0.)
	{
		coordinates.latitude = 99.;
		coordinates.longitude = 0.;
	}
	else
	{
		double zeta1 = std::sqrt(zeta);
		zeta = rho2*(zeta1*cd1d2-eta1*sd1d2);
		double adot = -bp.ldot-bp.mudot*x*tf*std::cos(d);
		double tq = (bp.bdot-zeta*bp.ddot-(adot/cq))/(bp.cdot-zeta*bp.mudot*std::cos(d));
		double Lb = L-zeta*tf;
		sq = std::sin(std::atan(tq));
		cq = std::cos(std::atan(tq));
		if (!northernLimit)
		{
			sq *= -1.;
			cq *= -1.;
		}
		xi = x-Lb*sq;
		eta1 = y1-Lb*cq/rho1;
		zeta = 1.-xi*xi-eta1*eta1;
		if (zeta < 0.)
		{
			coordinates.latitude = 99.;
			coordinates.longitude = 0.;
		}	
		else
		{
			zeta1 = std::sqrt(zeta);
			zeta = rho2*(zeta1*cd1d2-eta1*sd1d2);
			//tq = bp.bdot-zeta*ddot-adot/cq;
			//tq = tq/(bp.cdot-zeta*bp.mudot*std::cos(d));
			double b = -eta1*sd1+zeta1*cd1;
			double lngDeg = StelUtils::fmodpos(std::atan2(xi,b)*M_180_PI - mu + 180., 360.) - 180.;
			double sfn1 = eta1*cd1+zeta1*sd1;
			double cfn1 = std::sqrt(1.-sfn1*sfn1);
			double latDeg = ff*sfn1/cfn1;
			coordinates.latitude = std::atan(latDeg)*M_180_PI;
			coordinates.longitude = lngDeg;
		}
	}
	return coordinates;
}

auto SolarEclipseComputer::getExtremeNSLimitOfShadow(double JD, bool northernLimit, bool penumbra, bool begin) const -> GeoPoint
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	GeoPoint coordinates;
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	static const double ff = 1./(1.-f);

	core->setJD(JD+0.1);
	core->update(0);
	const auto bpPlus = calcBesselParameters(penumbra);
	const double bdot1 = bpPlus.bdot, cdot1 = bpPlus.cdot;

	core->setJD(JD-0.1);
	core->update(0);
	const auto bpMinus = calcBesselParameters(penumbra);
	const double bdot2 = bpMinus.bdot, cdot2 = bpMinus.cdot;

	const double bdd = 5.*(bdot1-bdot2);
	const double cdd = 5.*(cdot1-cdot2);

	core->setJD(JD);
	core->update(0);
	const auto bp = calcBesselParameters(penumbra);
	const double xdot = bp.xdot, ydot = bp.ydot, bdot = bp.bdot, cdot = bp.cdot;
	const double x = bp.elems.x, y = bp.elems.y, d = bp.elems.d, L1 = bp.elems.L1, L2 = bp.elems.L2;
	double e = std::sqrt(bdot*bdot+cdot*cdot);
	double rho1 = std::sqrt(1-e2*std::cos(d)*std::cos(d));
	double scq = e/cdot;
	double tq = bdot/cdot;
	double cq = 1./scq;
	const double L = penumbra ? L1 : L2;
	if (northernLimit)
	{
		if (L<0.)
			cq = std::abs(cq);
		else
			cq = -std::abs(cq);
	}
	else
	{
		if (L<0.)
			cq = -std::abs(cq);
		else
			cq = std::abs(cq);
	}
	double sq = tq*cq;
	double xidot, etadot;
	if (cq>0.)
	{
		xidot = xdot-L*bdd/e;
		etadot = (ydot-L*cdd/e)/rho1;
	}
	else
	{
		xidot = xdot+L*bdd/e;
		etadot = (ydot+L*cdd/e)/rho1;
	}
	const double n2 = xidot*xidot+etadot*etadot;
	double xi = x-L*sq;
	double eta = (y-L*cq)/rho1;
	double szi = (xi*etadot-xidot*eta)/std::sqrt(n2);
	if (std::abs(szi)<=1.)
	{
		double czi = -std::sqrt(1.-szi*szi);
		if (!begin) czi *= -1.;
		double tc = (czi/std::sqrt(n2))-(xi*xidot+eta*etadot)/n2;
		core->setJD(JD+tc/24.);
		core->update(0);
		const auto bp = calcBesselParameters(penumbra);
		const double bdot = bp.bdot, cdot = bp.cdot;
		const double x = bp.elems.x, y = bp.elems.y, d = bp.elems.d,
		             L1 = bp.elems.L1, L2 = bp.elems.L2, mu = bp.elems.mu;
		//tq = bdot/cdot;
		e = std::sqrt(bdot*bdot+cdot*cdot);
		rho1 = std::sqrt(1.-e2*std::cos(d)*std::cos(d));
		scq = e/cdot;
		tq = bdot/cdot;
		cq = 1./scq;
		const double L = penumbra ? L1 : L2;
		if (northernLimit)
			cq = (L<0.) ?  std::abs(cq) : -std::abs(cq);
		else
			cq = (L<0.) ? -std::abs(cq) :  std::abs(cq);
		sq = tq*cq;
		// clazy warns n2 below and therefore this all is unused!
		//if (cq>0.)
		//{
		//	xidot = xdot-L*bdd/e;
		//	etadot = (ydot-L*cdd/e)/rho1;
		//}
		//else
		//{
		//	xidot = xdot+L*bdd/e;
		//	etadot = (ydot+L*cdd/e)/rho1;
		//}
		//n2 = xidot*xidot+etadot*etadot;
		xi = x-L*sq;
		eta = (y-L*cq)/rho1;
		double sd1 = std::sin(d)/rho1;
		double cd1 = std::sqrt(1.-e2)*std::cos(d)/rho1;
		double b = -eta*sd1;
		double lngDeg = StelUtils::fmodpos(std::atan2(xi,b)*M_180_PI - mu +180., 360.) - 180.;
		double sfn1 = eta*cd1;
		double cfn1 = std::sqrt(1.-sfn1*sfn1);
		double latDeg = ff*sfn1/cfn1;
		coordinates.latitude = std::atan(latDeg)*M_180_PI;
		coordinates.longitude = lngDeg;
	}
	else
	{
		coordinates.latitude = 99.;
		coordinates.longitude = 0.;
	}
	return coordinates;
}

auto SolarEclipseComputer::getContactCoordinates(double x, double y, double d, double mu) const -> GeoPoint
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	GeoPoint coordinates;
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	static const double ff = 1./(1.-f);
	const double rho1 = std::sqrt(1.-e2*std::cos(d)*std::cos(d));
	const double yy1 = y/rho1;
	const double m1 = std::sqrt(x*x+yy1*yy1);
	const double eta1 = yy1/m1;
	const double sd1 = std::sin(d)/rho1;
	const double cd1 = std::sqrt(1.-e2)*std::cos(d)/rho1;
	const double theta = std::atan2(x/m1,-eta1*sd1)*M_180_PI;
	double lngDeg = StelUtils::fmodpos(theta - mu + 180., 360.) - 180.;
	double latDeg = ff*std::tan(std::asin(eta1*cd1));
	coordinates.latitude = std::atan(latDeg)*M_180_PI;
	coordinates.longitude = lngDeg;
	return coordinates;
}

auto SolarEclipseComputer::getRiseSetLineCoordinates(bool first, double x,double y,double d,double L,double mu) const -> GeoPoint
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double ff = 1./(1.-f);
	const double m2 = x*x+y*y;
	// Cosine of the angle between (0,0)-(x,y) line and the intersection
	// of the penumbra circle and the Earth circle in the fundamental plane.
	const double cgm = (m2+1.-L*L)/(2.*std::sqrt(m2));

	GeoPoint coordinates(99., 0.);
	if (std::abs(cgm)<=1.)
	{
		// Angle from Y axis clockwise to the direction towards one of the points of intersection
		const double gamma = first ? std::acos(cgm)+std::atan2(x,y)
		                           : M_PI*2.-std::acos(cgm)+std::atan2(x,y);

		const double xi = std::sin(gamma);
		const double eta = std::cos(gamma);
		const double b = -eta*std::sin(d);
		const double theta = std::atan2(xi,b)*M_180_PI;
		double lngDeg = theta-mu;
		lngDeg = StelUtils::fmodpos(lngDeg, 360.);
		if (lngDeg > 180.) lngDeg -= 360.;
		double sfn1 = eta*std::cos(d);
		double cfn1 = std::sqrt(1.-sfn1*sfn1);
		double tanLat = ff*sfn1/cfn1;
		coordinates.latitude = std::atan(tanLat)*M_180_PI;
		coordinates.longitude = lngDeg;
	}
	return coordinates;
}

auto SolarEclipseComputer::getShadowOutlineCoordinates(double angle,double x,double y,double d,double L,double tf,double mu) const -> GeoPoint
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	static const double ff = 1./(1.-f);
	const double sinAngle=std::sin(angle);
	const double cosAngle=std::cos(angle);
	const double cosD = std::cos(d);
	const double rho1 = std::sqrt(1.-e2*cosD*cosD);
	const double sd1 = std::sin(d)/rho1;
	const double cd1 = std::sqrt(1.-e2)*cosD/rho1;

	GeoPoint coordinates(99., 0.);

	double L1, xi, eta1, zeta1 = 0;
	for(int n = 0; n < 3; ++n)
	{
		L1 = L-zeta1*tf;
		xi = x-L1*sinAngle;
		eta1 = (y-L1*cosAngle)/rho1;
		const double zeta1sqr = 1.-xi*xi-eta1*eta1;
		if (zeta1sqr < 0) return coordinates;
		zeta1 = std::sqrt(zeta1sqr);
	}

	double b = -eta1*sd1+zeta1*cd1;
	double theta = std::atan2(xi,b)*M_180_PI;
	double lngDeg = theta-mu;
	lngDeg = StelUtils::fmodpos(lngDeg, 360.);
	if (lngDeg > 180.) lngDeg -= 360.;

	double sfn1 = eta1*cd1+zeta1*sd1;
	double cfn1 = std::sqrt(1.-sfn1*sfn1);
	double tanLat = ff*sfn1/cfn1;
	coordinates.latitude = std::atan(tanLat)*M_180_PI;
	coordinates.longitude = lngDeg;

	return coordinates;
}

auto SolarEclipseComputer::getMaximumEclipseAtRiseSet(bool first, double JD) const -> GeoPoint
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double ff = 1./(1.-f);
	core->setJD(JD);
	core->update(0);
	const auto bp = calcBesselParameters(true);
	const double bdot = bp.bdot, cdot = bp.cdot;
	const double x = bp.elems.x, y = bp.elems.y, d = bp.elems.d, L1 = bp.elems.L1, mu = bp.elems.mu;

	double qa = std::atan2(bdot,cdot);
	if (!first) // there are two parts of the curve
		qa += M_PI;
	const double sgqa = x*std::cos(qa)-y*std::sin(qa);

	GeoPoint coordinates(99., 0.);
	if (std::abs(sgqa) > 1.) return coordinates;

	const double gqa = std::asin(sgqa);
	const double gamma = gqa+qa;
	const double xi = std::sin(gamma);
	const double eta = std::cos(gamma);
	const double xxia = x-xi;
	const double yetaa = y-eta;
	if (xxia*xxia+yetaa*yetaa > L1*L1) return coordinates;

	const double b = -eta*std::sin(d);
	const double theta = std::atan2(xi,b)*M_180_PI;
	double lngDeg = StelUtils::fmodpos(theta-mu, 360.);
	if (lngDeg > 180.) lngDeg -= 360.;
	const double sfn1 = std::cos(gamma)*std::cos(d);
	const double cfn1 = std::sqrt(1.-sfn1*sfn1);
	const double tanLat = ff*sfn1/cfn1;
	coordinates.latitude = std::atan(tanLat)*M_180_PI;
	coordinates.longitude = lngDeg;

	return coordinates;
}

double SolarEclipseComputer::getDeltaTimeOfContact(double JD, bool beginning, bool penumbra, bool external, bool outerContact) const
{
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	const int sign = outerContact ? 1 : -1; // there are outer & inner contacts
	core->setJD(JD);
	core->update(0);
	const auto bp = calcBesselParameters(true);
	const double xdot = bp.xdot;
	double ydot = bp.ydot;
	const double x = bp.elems.x, y = bp.elems.y, d = bp.elems.d, L1 = bp.elems.L1, L2 = bp.elems.L2;
	const double rho1 = std::sqrt(1.-e2*std::cos(d)*std::cos(d));
	double s,dt;
	if (!penumbra)
		ydot = ydot/rho1;
	const double n = std::sqrt(xdot*xdot+ydot*ydot);
	const double y1 = y/rho1;
	const double m = std::sqrt(x*x+y*y);
	const double m1 = std::sqrt(x*x+y1*y1);
	const double rho = m/m1;
	const double L = penumbra ? L1 : L2;

	if (external)
	{
		s = (x*ydot-y*xdot)/(n*(L+sign*rho)); // shadow's limb
	}
	else
	{
		s = (x*ydot-xdot*y1)/n; // center of shadow
	}
	double cs = std::sqrt(1.-s*s);
	if (beginning)
		cs *= -1.;
	if (external)
	{
		dt = (L+sign*rho)*cs/n;
		if (outerContact)
			dt -= ((x*xdot+y*ydot)/(n*n));
		else
			dt = -((x*xdot+y*ydot)/(n*n))-dt;
	}
	else
		dt = cs/n-((x*xdot+y1*ydot)/(n*n));
	return dt;
}

double SolarEclipseComputer::getJDofContact(double JD, bool beginning, bool penumbra, bool external, bool outerContact) const
{
	double dt = 1.;
	int iterations = 0;
	while (std::abs(dt)>(0.1/86400.) && (iterations < 10))
	{
		dt = getDeltaTimeOfContact(JD,beginning,penumbra,external,outerContact);
		JD += dt/24.;
		iterations++;
	}
	return JD;
}

double SolarEclipseComputer::getJDofMinimumDistance(double JD) const
{
	const double currentJD = core->getJD(); // save current JD
	double dt = 1.;
	int iterations = 0;
	while (std::abs(dt)>(0.1/86400.) && (iterations < 20)) // 0.1 second of accuracy
	{
		core->setJD(JD);
		core->update(0);
		const auto bp = calcBesselParameters(true);
		const double xdot = bp.xdot, ydot = bp.ydot;
		const double x = bp.elems.x, y = bp.elems.y;
		double n2 = xdot*xdot + ydot*ydot;
		dt = -(x*xdot + y*ydot)/n2;
		JD += dt/24.;
		iterations++;
	}
	core->setJD(currentJD);
	core->update(0);
	return JD;
}

bool SolarEclipseComputer::generatePNGMap(const EclipseMapData& data, const QString& filePath) const
{
	QImage img(":/graphicGui/miscWorldMap.jpg");

	QPainter painter(&img);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.translate(img.width() / 2., img.height() / 2.);
	painter.scale(1,-1); // latitude grows upwards

	const double scale = img.width() / 360.;
	const double penWidth = std::lround(img.width() / 2048.) / scale;
	painter.scale(scale, scale);

	const auto updatePen = [&painter,penWidth](const EclipseMapData::EclipseType type =
	                                                   EclipseMapData::EclipseType::Undefined)
	{
		switch(type)
		{
		case EclipseMapData::EclipseType::Total:
			painter.setPen(QPen(totalEclipseColor, penWidth));
			break;
		case EclipseMapData::EclipseType::Annular:
			painter.setPen(QPen(annularEclipseColor, penWidth));
			break;
		case EclipseMapData::EclipseType::Hybrid:
			painter.setPen(QPen(hybridEclipseColor, penWidth));
			break;
		default:
			painter.setPen(QPen(eclipseLimitsColor, penWidth));
			break;
		}
	};

	updatePen();
	std::vector<QPointF> points;
	for(const auto& penumbraLimit : data.penumbraLimits)
	{
		points.clear();
		points.reserve(penumbraLimit.size());
		for(const auto& p : penumbraLimit)
			points.emplace_back(p.longitude, p.latitude);
		drawGeoLinesForEquirectMap(painter, points);
	}

	for(const auto& riseSetLimit : data.riseSetLimits)
	{
		struct RiseSetLimitVisitor
		{
			QPainter& painter;
			RiseSetLimitVisitor(QPainter& painter)
				: painter(painter)
			{
			}
			void operator()(const EclipseMapData::TwoLimits& limits) const
			{
				// P1-P2 curve
				std::vector<QPointF> points;
				points.reserve(limits.p12curve.size());
				for(const auto& p : limits.p12curve)
					points.emplace_back(p.longitude, p.latitude);
				drawGeoLinesForEquirectMap(painter, points);

				// P3-P4 curve
				points.clear();
				points.reserve(limits.p34curve.size());
				for(const auto& p : limits.p34curve)
					points.emplace_back(p.longitude, p.latitude);
				drawGeoLinesForEquirectMap(painter, points);
			}
			void operator()(const EclipseMapData::SingleLimit& limit) const
			{
				std::vector<QPointF> points;
				points.reserve(limit.curve.size());
				for(const auto& p : limit.curve)
					points.emplace_back(p.longitude, p.latitude);
				drawGeoLinesForEquirectMap(painter, points);
			}
		};
		std::visit(RiseSetLimitVisitor{painter}, riseSetLimit);
	}

	for(const auto& curve : data.maxEclipseAtRiseSet)
	{
		points.clear();
		points.reserve(curve.size());
		for(const auto& p : curve)
			points.emplace_back(p.longitude, p.latitude);
		drawGeoLinesForEquirectMap(painter, points);
	}

	if(!data.centerLine.empty())
	{
		updatePen(data.eclipseType);
		points.clear();
		points.reserve(data.centerLine.size());
		for(const auto& p : data.centerLine)
			points.emplace_back(p.longitude, p.latitude);
		drawGeoLinesForEquirectMap(painter, points);
	}

	for(const auto& outline : data.extremeUmbraLimit1)
	{
		updatePen(outline.eclipseType);
		points.clear();
		points.reserve(outline.curve.size());
		for(const auto& p : outline.curve)
			points.emplace_back(p.longitude, p.latitude);
		drawGeoLinesForEquirectMap(painter, points);
	}

	for(const auto& outline : data.extremeUmbraLimit2)
	{
		updatePen(outline.eclipseType);
		points.clear();
		points.reserve(outline.curve.size());
		for(const auto& p : outline.curve)
			points.emplace_back(p.longitude, p.latitude);
		drawGeoLinesForEquirectMap(painter, points);
	}

	for(const auto& outline : data.umbraOutlines)
	{
		updatePen(outline.eclipseType);
		points.clear();
		points.reserve(outline.curve.size());
		for(const auto& p : outline.curve)
			points.emplace_back(p.longitude, p.latitude);
		drawGeoLinesForEquirectMap(painter, points);
	}

	return img.save(filePath, "PNG");
}

void SolarEclipseComputer::generateKML(const EclipseMapData& data, const QString& dateString, QTextStream& stream) const
{
	using EclipseType = EclipseMapData::EclipseType;

	stream << "<?xml version='1.0' encoding='UTF-8'?>\n<kml xmlns='http://www.opengis.net/kml/2.2'>\n<Document>" << '\n';
	stream << "<name>"+q_("Solar Eclipse")+" "+dateString+"</name>\n<description>"+q_("Created by Stellarium")+"</description>\n";
	stream << "<Style id='Hybrid'>\n<LineStyle>\n<color>" << toKMLColorString(hybridEclipseColor) << "</color>\n<width>1</width>\n</LineStyle>\n";
	stream << "<PolyStyle>\n<color>" << toKMLColorString(hybridEclipseColor) << "</color>\n</PolyStyle>\n</Style>\n";
	stream << "<Style id='Total'>\n<LineStyle>\n<color>" << toKMLColorString(totalEclipseColor) << "</color>\n<width>1</width>\n</LineStyle>\n";
	stream << "<PolyStyle>\n<color>" << toKMLColorString(totalEclipseColor) << "</color>\n</PolyStyle>\n</Style>\n";
	stream << "<Style id='Annular'>\n<LineStyle>\n<color>" << toKMLColorString(annularEclipseColor) << "</color>\n<width>1</width>\n</LineStyle>\n";
	stream << "<PolyStyle>\n<color>" << toKMLColorString(annularEclipseColor) << "</color>\n</PolyStyle>\n</Style>\n";
	stream << "<Style id='PLimits'>\n<LineStyle>\n<color>" << toKMLColorString(eclipseLimitsColor) << "</color>\n<width>1</width>\n</LineStyle>\n";
	stream << "<PolyStyle>\n<color>" << toKMLColorString(eclipseLimitsColor) << "</color>\n</PolyStyle>\n</Style>\n";

	{
		const auto timeStr = localeMgr->getPrintableTimeLocal(data.greatestEclipse.JD);
		stream << "<Placemark>\n<name>"+q_("Greatest eclipse")+" ("+timeStr+")</name>\n<Point>\n<coordinates>";
		stream << data.greatestEclipse.longitude << "," << data.greatestEclipse.latitude << ",0.0\n";
		stream << "</coordinates>\n</Point>\n</Placemark>\n";
	}

	{
		const auto timeStr = localeMgr->getPrintableTimeLocal(data.firstContactWithEarth.JD);
		stream << "<Placemark>\n<name>"+q_("First contact with Earth")+" ("+timeStr+")</name>\n<Point>\n<coordinates>";
		stream << data.firstContactWithEarth.longitude << "," << data.firstContactWithEarth.latitude << ",0.0\n";
		stream << "</coordinates>\n</Point>\n</Placemark>\n";
	}

	{
		const auto timeStr = localeMgr->getPrintableTimeLocal(data.lastContactWithEarth.JD);
		stream << "<Placemark>\n<name>"+q_("Last contact with Earth")+" ("+timeStr+")</name>\n<Point>\n<coordinates>";
		stream << data.lastContactWithEarth.longitude << "," << data.lastContactWithEarth.latitude << ",0.0\n";
		stream << "</coordinates>\n</Point>\n</Placemark>\n";
	}

	const auto startLinePlaceMark = [&stream](const QString& name, const EclipseMapData::EclipseType type = EclipseType::Undefined)
	{
		switch(type)
		{
		case EclipseMapData::EclipseType::Total:
			stream << "<Placemark>\n<name>" << name << "</name>\n<styleUrl>#Total</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
			break;
		case EclipseMapData::EclipseType::Annular:
			stream << "<Placemark>\n<name>" << name << "</name>\n<styleUrl>#Annular</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
			break;
		case EclipseMapData::EclipseType::Hybrid:
			stream << "<Placemark>\n<name>" << name << "</name>\n<styleUrl>#Hybrid</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
			break;
		default:
			stream << "<Placemark>\n<name>" << name << "</name>\n<styleUrl>#PLimits</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
			break;
		}
	};

	for(const auto& penumbraLimit : data.penumbraLimits)
	{
		startLinePlaceMark("PenumbraLimit");
		stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
		for(const auto& p : penumbraLimit)
			stream << p.longitude << "," << p.latitude << ",0.0\n";
		stream << "</coordinates>\n</LineString>\n</Placemark>\n";
	}

	for(const auto& riseSetLimit : data.riseSetLimits)
	{
		struct RiseSetLimitVisitor
		{
			QTextStream& stream;
			std::function<void(const QString&, EclipseType)> startLinePlaceMark;
			RiseSetLimitVisitor(QTextStream& stream, const std::function<void(const QString&, EclipseType)>& startLinePlaceMark)
				: stream(stream), startLinePlaceMark(startLinePlaceMark)
			{
			}
			void operator()(const EclipseMapData::TwoLimits& limits) const
			{
				// P1-P2 curve
				startLinePlaceMark("RiseSetLimit", EclipseType::Undefined);
				stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
				for(const auto& p : limits.p12curve)
					stream << p.longitude << "," << p.latitude << ",0.0\n";
				stream << "</coordinates>\n</LineString>\n</Placemark>\n";

				// P3-P4 curve
				startLinePlaceMark("RiseSetLimit", EclipseType::Undefined);
				stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
				for(const auto& p : limits.p34curve)
					stream << p.longitude << "," << p.latitude << ",0.0\n";
				stream << "</coordinates>\n</LineString>\n</Placemark>\n";
			}
			void operator()(const EclipseMapData::SingleLimit& limit) const
			{
				startLinePlaceMark("RiseSetLimit", EclipseType::Undefined);
				stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
				for(const auto& p : limit.curve)
					stream << p.longitude << "," << p.latitude << ",0.0\n";
				stream << "</coordinates>\n</LineString>\n</Placemark>\n";
			}
		};
		std::visit(RiseSetLimitVisitor{stream, startLinePlaceMark}, riseSetLimit);
	}

	for(const auto& curve : data.maxEclipseAtRiseSet)
	{
		startLinePlaceMark("MaxEclipseSunriseSunset");
		stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
		for(const auto& p : curve)
			stream << p.longitude << "," << p.latitude << ",0.0\n";
		stream << "</coordinates>\n</LineString>\n</Placemark>\n";
	}

	if(data.centralEclipseStart.JD > 0)
	{
		const auto timeStr = localeMgr->getPrintableTimeLocal(data.centralEclipseStart.JD);
		stream << "<Placemark>\n<name>"+q_("Central eclipse begins")+" ("+timeStr+")</name>\n<Point>\n<coordinates>";
		stream << data.centralEclipseStart.longitude << "," << data.centralEclipseStart.latitude << ",0.0\n";
		stream << "</coordinates>\n</Point>\n</Placemark>\n";
	}

	if(data.centralEclipseEnd.JD > 0)
	{
		const auto timeStr = localeMgr->getPrintableTimeLocal(data.centralEclipseEnd.JD);
		stream << "<Placemark>\n<name>"+q_("Central eclipse ends")+" ("+timeStr+")</name>\n<Point>\n<coordinates>";
		stream << data.centralEclipseEnd.longitude << "," << data.centralEclipseEnd.latitude << ",0.0\n";
		stream << "</coordinates>\n</Point>\n</Placemark>\n";
	}

	if(!data.centerLine.empty())
	{
		startLinePlaceMark("Center line", data.eclipseType);
		stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
		for(const auto& p : data.centerLine)
			stream << p.longitude << "," << p.latitude << ",0.0\n";
		stream << "</coordinates>\n</LineString>\n</Placemark>\n";
	}

	for(const auto& outline : data.umbraOutlines)
	{
		const auto timeStr = localeMgr->getPrintableTimeLocal(outline.JD);
		startLinePlaceMark(timeStr, outline.eclipseType);
		stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
		for(const auto& p : outline.curve)
			stream << p.longitude << "," << p.latitude << ",0.0\n";
		stream << "</coordinates>\n</LineString>\n</Placemark>\n";
	}

	for(const auto& outline : data.extremeUmbraLimit1)
	{
		startLinePlaceMark("Limit", outline.eclipseType);
		stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
		for(const auto& p : outline.curve)
			stream << p.longitude << "," << p.latitude << ",0.0\n";
		stream << "</coordinates>\n</LineString>\n</Placemark>\n";
	}

	for(const auto& outline : data.extremeUmbraLimit2)
	{
		startLinePlaceMark("Limit", outline.eclipseType);
		stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
		for(const auto& p : outline.curve)
			stream << p.longitude << "," << p.latitude << ",0.0\n";
		stream << "</coordinates>\n</LineString>\n</Placemark>\n";
	}

	stream << "</Document>\n</kml>\n";
}
