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

#include <utility>
#include <QPainter>

namespace
{

double sqr(const double x) { return x*x; }

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

double zetaFromQ(const double cosQ, const double sinQ, const double tf, const EclipseBesselParameters& bp)
{
	// Reference: Explanatory Supplement to the Astronomical Ephemeris
	// and the American Ephemeris and Nautical Almanac, 3rd Edition (2013)

	const double x = bp.elems.x, y = bp.elems.y, d = bp.elems.d,
	             mudot = bp.mudot, bdot = bp.bdot, cdot = bp.cdot, ddot = bp.ddot;
	const double cosd = cos(d);
	const double adot = -bp.ldot-mudot*x*tf*cosd + y*ddot*tf;

	// From equation (11.81), restoring the missing dots over a,b,c in the book (cf. eq. (11.78)).
	return (-adot + bdot * cosQ - cdot * sinQ) / ((1 + sqr(tf)) * (ddot * cosQ - mudot * cosd * sinQ));
}

struct ShadowLimitPoints
{
	EclipseBesselParameters bp;
	double JD;
	struct Point
	{
		double Q;
		double zeta;
	};
	QVarLengthArray<Point, 8> values;

	ShadowLimitPoints(const EclipseBesselParameters& bp, const double JD)
		: bp(bp)
		, JD(JD)
	{
	}
};

ShadowLimitPoints getShadowLimitQs(StelCore*const core, const double JD, const double e2, const bool penumbra)
{
	/*
	 * Reference: Explanatory Supplement to the Astronomical Ephemeris and the American Ephemeris and Nautical Almanac,
	 * 3rd Edition (2013).
	 *
	 * This function computes Q values for shadow limit points. The equation being solved here is obtained from the
	 * main identity (11.56):
	 *
	 *                                            xi^2+eta1^2+zeta1^2=1,
	 *
	 * and (11.60):
	 *
	 *                                  zeta=rho2*[zeta1*cos(d1-d2)-eta1*sin(d1-d2)].
	 *
	 * Solve the latter equation for zeta1 and substitute the result into the former equation.  Then multiply both
	 * sides of the result by rho1^2*rho2^2*cos(d1-d2)^2, and after a rearrangement you'll get:
	 *
	 *   zeta^2*rho1^2 + 2*zeta*eta*sin(d1-d2)*rho1*rho2 +
	 *                              + eta^2*rho2^2 - cos(d1-d2)^2*rho1^2*rho2^2 + xi^2*cos(d1-d2)^2*rho1^2*rho2^2 = 0.
	 *
	 * Now substitute xi and eta with (eq. (11.82))
	 *
	 *                                              xi = x - L*sin(Q),
	 *                                             eta = x - L*cos(Q),
	 *
	 * where (eq. (11.65))
	 *
	 *                                             L = l - zeta*tan(f).
	 *
	 * The result will be a bit more complicated:
	 *
	 *  zeta^2*rho1^2 - cos(d1-d2)^2*rho1^2*rho2^2 + 2*zeta*sin(d1-d2)*rho1*rho2*(y - cos(Q)*(l - zeta*tan(f))) +
	 *    + rho2^2*(y - cos(Q)*(l - zeta*tan(f)))^2 + cos(d1-d2)^2*rho1^2*rho2^2*(x - sin(Q)*(l - zeta*tan(f)))^2 = 0.
	 *
	 * Now replace zeta by the expression obtainable from (11.78),
	 *
	 *                                              -adot + bdot*cos(Q) - cdot*sin(Q)
	 *                               zeta = -------------------------------------------------,
	 *                                      (1 + tan(f)^2)*(ddot*cos(Q) - mudot*cos(d)*sin(Q)
	 *
	 * yielding, after multiplying both sides of the result by (1+tan(f))^2, the final equation:
	 *
	 * (-adot + bdot*cos(Q) - cdot*sin(Q))^2*(rho1^2 + 2*cos(Q)*sin(d1 - d2)*rho1*rho2*tan(f) +
	 *  + (cos(Q)^2 + cos(d1 - d2)^2*sin(Q)^2*rho1^2)*rho2^2*tan(f)^2) +
	 *  + 2*(-adot + bdot*cos(Q) - cdot*sin(Q))*rho2*(sin(d1 - d2)*(y - cos(Q)*l)*rho1 +
	 *    + cos(Q)*(y - cos(Q)*l)*rho2*tan(f) + cos(d1 - d2)^2*sin(Q)*(x - sin(Q)*l)*rho1^2*rho2*tan(f)) *
	 *       * (1 + tan(f)^2)*(cos(Q)*ddot - cos(d)*sin(Q)*mudot) +
	 *    + ((y - cos(Q)*l)^2 + cos(d1 - d2)^2*(-1 + x - sin(Q)*l)*(1 + x - sin(Q)*l)*rho1^2)*rho2^2 *
	 *       * (1 + tan(f)^2)^2*(cos(Q)*ddot - cos(d)*sin(Q)*mudot)^2 = 0.
	 *
	 * As we are using the Newton's method to solve it, we need to separate the different powers of sin(Q)^n*cos(Q)^m
	 * to be able to find a derivative. This will then yield the final result implemented in the code below.
	 */

	core->setJD(JD);
	core->update(0);
	const auto bp = calcBesselParameters(penumbra);
	const double x = bp.elems.x, y = bp.elems.y, d = bp.elems.d, tf1 = bp.elems.tf1,
	             bdot = bp.bdot, cdot = bp.cdot, ddot = bp.ddot, mudot = bp.mudot,
	             tf2 = bp.elems.tf2, L1 = bp.elems.L1, L2 = bp.elems.L2;
	const double tf = penumbra ? tf1 : tf2;
	const double L = penumbra ? L1 : L2;
	const double adot = -bp.ldot-mudot*x*tf*std::cos(d) + y*ddot*tf;

	using namespace std;
	const double sind = sin(d);
	const double cosd = cos(d);
	const double rho1 = sqrt(1-e2*sqr(cosd));
	const double rho2 = sqrt(1-e2*sqr(sind));
	const double sdd = e2*sind*cosd/(rho1*rho2);   // sin(d1-d2)
	const double cdd = sqrt(1-sqr(sdd));           // cos(d1-d2)

	// Some convenience variables to shorten the expressions below
	const double tfSp1 = 1 + sqr(tf);
	const double tfSp12 = sqr(tfSp1);
	const double x2 = x*x;
	const double y2 = y*y;
	const double adot2 = sqr(adot);
	const double bdot2 = sqr(bdot);
	const double cdot2 = sqr(cdot);
	const double rho12 = sqr(rho1);
	const double rho22 = sqr(rho2);
	const double cdd2 = sqr(cdd);

	// Coefficients of the LHS of the equation we are going to solve.

	//  * Constant term
	const double cC0S0 = adot2 * rho12;

	//  * coefficient for cos(Q)
	const double cC1S0 = 2*adot*rho1*(-(bdot*rho1) + rho2*sdd*(adot*tf - ddot*tfSp1*y));
	//  * coefficient for cos(Q)^2
	const double cC2S0 = bdot2*rho12 + 2*bdot*rho1*rho2*sdd*(-2*adot*tf + ddot*tfSp1*y) +
	                     rho2*(2*adot*ddot*tfSp1*(L*rho1*sdd - rho2*tf*y) + adot2*rho2*sqr(tf) +
	                           cdd2*rho12*rho2*tfSp12*sqr(ddot)*(-1 + x2) +
	                           rho2*tfSp12*sqr(ddot)*y2);
	//  * coefficient for cos(Q)^3
	const double cC3S0 = -2*rho2*(-(bdot*tf) + ddot*L*tfSp1)*(bdot*rho1*sdd - adot*rho2*tf +
	                                                          ddot*rho2*tfSp1*y);
	//  * coefficient for cos(Q)^4
	const double cC4S0 = rho22*sqr(bdot*tf - ddot*L*tfSp1);

	//  * coefficient for sin(Q)
	const double cC0S1 = 2*adot*rho1*(cdot*rho1 + cosd*mudot*rho2*sdd*tfSp1*y);
	//  * coefficient for sin(Q)^2
	const double cC0S2 = cdot2*rho12 + 2*cdot*cosd*mudot*rho1*rho2*sdd*tfSp1*y +
	                     rho22*(cdd2*rho12*(adot*tf + cosd*mudot*tfSp1*(-1 + x)) *
	                            (adot*tf + cosd*mudot*tfSp1*(1 + x)) +
	                            tfSp12*sqr(cosd)*sqr(mudot)*y2);
	//  * coefficient for sin(Q)^3
	const double cC0S3 = -2*cdd2*rho12*rho22*(-(cdot*tf) + cosd*L*mudot*tfSp1) *
	                     (adot*tf + cosd*mudot*tfSp1*x);
	//  * coefficient for sin(Q)^4
	const double cC0S4 = cdd2*rho12*rho22*sqr(cdot*tf - cosd*L*mudot*tfSp1);

	//  * coefficient for cos(Q)*sin(Q)
	const double cC1S1 = -2*bdot*rho1*(cdot*rho1 + cosd*mudot*rho2*sdd*tfSp1*y) -
	                     2*rho2*(ddot*tfSp1*y*(cdot*rho1*sdd + cosd*mudot*rho2*tfSp1*y) +
	                             adot*(-2*cdot*rho1*sdd*tf + cosd*mudot*tfSp1*(L*rho1*sdd - rho2*tf*y)) +
	                             cdd2*ddot*rho12*rho2*tfSp1*(adot*tf*x + cosd*mudot*tfSp1*(-1 + x2)));

	//  * coefficient for cos(Q)^2*sin(Q)^2
	const double cC2S2 = rho22*(-2*cdot*cosd*L*mudot*tf*tfSp1 + tfSp12*sqr(cosd)*sqr(L)*sqr(mudot) +
	                            cdot2*sqr(tf) + cdd2*rho12*sqr(bdot*tf - ddot*L*tfSp1));

	//  * coefficient for cos(Q)*sin(Q)^2
	const double cC1S2 = 2*rho2*(cdot2*rho1*sdd*tf + adot*cdd2*rho12*rho2*tf*(-(bdot*tf) + ddot*L*tfSp1) +
	                             cdot*tfSp1*(-(rho1*(cosd*L*mudot*sdd + cdd2*ddot*rho1*rho2*tf*x)) +
	                                         cosd*mudot*rho2*tf*y) + cosd*mudot*rho2*tfSp1*
	                             (cdd2*rho12*(-(bdot*tf) + 2*ddot*L*tfSp1)*x - cosd*L*mudot*tfSp1*y));
	//  * coefficient for cos(Q)^2*sin(Q)
	const double cC2S1 = 2*rho2*(tfSp1*(-(adot*cosd*L*mudot*rho2*tf) + bdot*cdd2*ddot*rho12*rho2*tf*x +
	                                    ddot*L*rho2*tfSp1*(-(cdd2*ddot*rho12*x) + 2*cosd*mudot*y) +
	                                    bdot*cosd*mudot*(L*rho1*sdd - rho2*tf*y)) +
	                             cdot*(tf*(-2*bdot*rho1*sdd + adot*rho2*tf) +
	                                   ddot*tfSp1*(L*rho1*sdd - rho2*tf*y)));

	//  * coefficient for cos(Q)^3*sin(Q)
	const double cC3S1 = -2*rho22*(-(bdot*tf) + ddot*L*tfSp1)*(-(cdot*tf) + cosd*L*mudot*tfSp1);
	//  * coefficient for cos(Q)*sin(Q)^3
	const double cC1S3 = -2*cdd2*rho12*rho22*(-(bdot*tf) + ddot*L*tfSp1)*(-(cdot*tf) + cosd*L*mudot*tfSp1);

	const double lhsScale = max({abs(cC0S0),
	                             abs(cC1S0),
	                             abs(cC2S0),
	                             abs(cC3S0),
	                             abs(cC4S0),
	                             abs(cC0S1),
	                             abs(cC0S2),
	                             abs(cC0S3),
	                             abs(cC0S4),
	                             abs(cC1S1),
	                             abs(cC2S2),
	                             abs(cC1S2),
	                             abs(cC2S1),
	                             abs(cC3S1),
	                             abs(cC1S3)});

	// LHS(Q) of the equation and its derivative
	const auto lhsAndDerivative = [=](const double Q) -> std::pair<double,double>
	{
		const auto sinQ = std::sin(Q);
		const auto cosQ = std::cos(Q);
		const auto sinQ_2 = sqr(sinQ);
		const auto cosQ_2 = sqr(cosQ);
		const auto sinQ_3 = sinQ_2 * sinQ;
		const auto cosQ_3 = cosQ_2 * cosQ;
		const auto sinQ_4 = sqr(sinQ_2);
		const auto cosQ_4 = sqr(cosQ_2);
		const auto lhs = cC0S0 +
		                 cC1S0*cosQ + cC2S0*cosQ_2 + cC3S0*cosQ_3 + cC4S0*cosQ_4 +
		                 cC0S1*sinQ + cC0S2*sinQ_2 + cC0S3*sinQ_3 + cC0S4*sinQ_4 +
		                 cC1S1*cosQ*sinQ + cC1S2*cosQ*sinQ_2 + cC1S3*cosQ*sinQ_3 +
		                 cC2S1*cosQ_2*sinQ + cC2S2*cosQ_2*sinQ_2 +
		                 cC3S1*cosQ_3*sinQ;
		const auto lhsPrime =
		    -cC1S0*sinQ -2*cC2S0*cosQ*sinQ - 3*cC3S0*cosQ_2*sinQ - 4*cC4S0*cosQ_3*sinQ +
		     cC0S1*cosQ + 2*cC0S2*sinQ*cosQ + 3*cC0S3*sinQ_2*cosQ + 4*cC0S4*sinQ_3*cosQ +
		     cC1S1*(cosQ_2-sinQ_2) + cC1S2*(2*cosQ_2*sinQ-sinQ_3) + cC1S3*(3*cosQ_2*sinQ_2-sinQ_4) +
		     cC2S1*(cosQ_3-2*cosQ*sinQ_2) + cC2S2*(2*cosQ_3*sinQ-2*cosQ*sinQ_3) +
		     cC3S1*(cosQ_4-3*cosQ_2*sinQ_2);
		return std::make_pair(lhs, lhsPrime);
	};

	// Find roots using Newton's method. The LHS of the equation is divided
	// by sin((Q-rootQ)/2) for all known roots to find subsequent roots.
	ShadowLimitPoints points(bp, JD);
	double Q = 0;
	bool rootFound = false;
	do
	{
		rootFound = false;
		bool finalIteration = false;
		// Max retry count is chosen with the idea that we'll scan the (periodic) domain [-pi,pi] with 4
		// extrema approximately evenly distributed over the domain, aiming to hit each slope, with an extra
		// sample just to make sure that we've not missed anything.
		// If we don't do these retries, we may lose roots when Newton's method gets stuck at an extremum that
		// doesn't reach zero.
		const double maxRetries = 9;
		for(double retryCount = 0; retryCount < maxRetries && !rootFound; ++retryCount)
		{
			Q = 2*M_PI*retryCount/maxRetries;

			for(int n = 0; n < 50; ++n)
			{
				const auto [lhs, lhsPrime] = lhsAndDerivative(Q);

				// Cancel the known roots to avoid finding them instead of the remaining ones
				auto newLHSPrime = lhsPrime;
				auto newLHS = lhs;
				for(const auto root : points.values)
				{
					const auto rootQ = root.Q;
					// We need a 2pi-periodic root-canceling function,
					// so take a sine of half the difference.
					const auto sinDiff = sin((Q - rootQ)/2);
					const auto cosDiff = cos((Q - rootQ)/2);

					newLHS /= sinDiff;
					newLHSPrime = (newLHSPrime - 0.5*cosDiff*newLHS)/sinDiff;
				}
				if(!isfinite(newLHS) || !isfinite(newLHSPrime))
				{
					qWarning().nospace() << "Hit infinite/NaN values: LHS = " << newLHS
					                     << ", LHS'(Q) = " << newLHSPrime << " at Q = " << Q;
					break;
				}

				if(abs(newLHS) < 1e-10*lhsScale)
					finalIteration = true;

				const auto deltaQ = newLHS / newLHSPrime;
				if(newLHSPrime==0 || abs(deltaQ) > 1000)
				{
					// We are shooting too far away, convergence may be too slow.
					// Let's try perturbing Q and retrying.
					Q += 0.01;
					finalIteration = false;
					continue;
				}
				Q -= deltaQ;
				Q = StelUtils::fmodpos(Q, 2*M_PI);

				if(finalIteration)
				{
					points.values.push_back({Q, zetaFromQ(cos(Q),sin(Q),tf,bp)});
					// Set a new initial value, but try to avoid setting it to 0 if the current
					// root is close to it (all values here are arbitrary in other respects).
					Q = abs(Q) > 0.5 ? 0 : -M_PI/2;

					rootFound = true;
					break;
				}
			}
		}
	}
	while(rootFound);

	std::sort(points.values.begin(), points.values.end(), [](auto& p1, auto& p2){ return p1.Q < p2.Q; });

	return points;
}

SolarEclipseComputer::GeoPoint computeTimePoint(const EclipseBesselParameters& bp, const double earthOblateness,
                                                const double Q, const double zeta, const bool penumbra)
{
	// Reference: Explanatory Supplement to the Astronomical Ephemeris
	// and the American Ephemeris and Nautical Almanac, 3rd Edition (2013)

	const double f = earthOblateness;
	const double e2 = f*(2.-f);
	const double ff = 1./(1.-f);
	const double x = bp.elems.x, y = bp.elems.y, d = bp.elems.d, tf1 = bp.elems.tf1,
	             tf2 = bp.elems.tf2, L1 = bp.elems.L1, L2 = bp.elems.L2, mu = bp.elems.mu;

	using namespace std;
	const double sind = sin(d);
	const double cosd = cos(d);
	const double rho1 = sqrt(1-e2*sqr(cosd));
	const double rho2 = sqrt(1-e2*sqr(sind));
	const double sd1 = sind/rho1;
	const double cd1 = sqrt(1-e2)*cosd/rho1;
	const double sdd = e2*sind*cosd/(rho1*rho2);   // sin(d1-d2)
	const double cdd = sqrt(1-sqr(sdd));           // cos(d1-d2)
	const double tf = penumbra ? tf1 : tf2;
	const double L = penumbra ? L1 : L2;

	SolarEclipseComputer::GeoPoint coordinates{99., 0.};

	const double sinQ = sin(Q);
	const double cosQ = cos(Q);

	const double Lz = L - zeta*tf; // radius of shadow at distance zeta from the fundamental plane
	const double xi  = x - Lz * sinQ;
	const double eta = y - Lz * cosQ;

	const double eta1 = eta / rho1;
	const double zeta1 = (zeta / rho2 + eta1 * sdd) / cdd;

	if(abs(eta) > 1.0001 || abs(xi) > 1.0001 || abs(zeta) > 1.0001)
	{
		qWarning().noquote() << QString("Unnormalized vector (xi,eta,zeta) found: (%1, %2, %3); Q = %4°")
		                            .arg(xi,0,'g',17).arg(eta,0,'g',17).arg(zeta,0,'g',17)
		                            .arg(StelUtils::fmodpos(Q*180/M_PI, 360), 0,'g',17);
	}

	const double b = -eta1*sd1+zeta1*cd1;
	const double theta = atan2(xi,b)*M_180_PI;
	double lngDeg = theta-mu;
	lngDeg = StelUtils::fmodpos(lngDeg, 360.);
	if (lngDeg > 180.) lngDeg -= 360.;
	const double sfn1 = eta1*cd1+zeta1*sd1;
	const double cfn1 = sqrt(1.-sfn1*sfn1);
	const double tanLat = ff*sfn1/cfn1;
	coordinates.latitude = atan(tanLat)*M_180_PI;
	coordinates.longitude = lngDeg;

	return coordinates;
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

bool SolarEclipseComputer::bothPenumbraLimitsPresent(const double JDMid) const
{
	core->setJD(JDMid);
	core->update(0);
	const auto ep = calcSolarEclipseBessel();
	// FIXME: can the rise-set line exist at greatest eclipse but not exist at some other phase?
	// It seems that ellipticity of the Earth could result in this, because greatest eclipse is
	// defined relative to Earth's center rather than to its rim.
	const auto coordinates = getRiseSetLineCoordinates(true, ep.x, ep.y, ep.d, ep.L1, ep.mu);
	return coordinates.latitude > 90;
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
	const bool bothPenumbralLimits = bothPenumbraLimitsPresent(JDMid);

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

	// Northern/southern limits of penumbra
	computeNSLimitsOfShadow(JDP1, JDP4, true, data.penumbraLimits);

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

		// Northern/southern limits of umbra
		computeNSLimitsOfShadow(JDP1, JDP4, false, data.umbraLimits);
	}

	core->setJD(currentJD);
	core->setUseTopocentricCoordinates(savedTopocentric);
	core->update(0);

	return data;
}

void SolarEclipseComputer::computeNSLimitsOfShadow(const double JDP1, const double JDP4, const bool penumbra,
                                                   std::vector<std::vector<EclipseMapData::GeoTimePoint>>& limits) const
{
	// Reference: Explanatory Supplement to the Astronomical Ephemeris
	// and the American Ephemeris and Nautical Almanac, 3rd Edition (2013)

	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);

	const int iMax = std::ceil((JDP4-JDP1)*1440);
	std::vector<ShadowLimitPoints> solutionsPerTime;
	solutionsPerTime.reserve(iMax);

	constexpr double minutesToDays = 1./(24*60);
	constexpr double secondsToDays = 1./(24*3600);

	// First sample the sets of Q values over all the time of the eclipse.
	for(int i = 0; i < iMax; ++i)
	{
		const double JD = JDP1 + i*minutesToDays;
		solutionsPerTime.emplace_back(getShadowLimitQs(core, JD, e2, penumbra));
	}

	// Check that each set of solutions has an even number of solutions.
	// If it's odd, the set is broken so should be removed.
	for(unsigned i = 0; i < solutionsPerTime.size(); )
	{
		if(solutionsPerTime[i].values.size() % 2)
		{
			qWarning() << "Found an odd number of values of Q:" << solutionsPerTime[i].values.size();
			solutionsPerTime.erase(solutionsPerTime.begin()+i);
		}
		else
		{
			++i;
		}
	}

	// Search for time points where number of Q values changes and
	// refine each case to get closer to the point of the jump.
	for(unsigned i = 1; i < solutionsPerTime.size(); ++i)
	{
		const auto& solA = solutionsPerTime[i-1];
		const auto& solB = solutionsPerTime[i];
		if(std::abs(solA.JD - solB.JD) <= 0.001*secondsToDays)
		{
			// Already fine enough.
			continue;
		}

		if(solA.values.size() != solB.values.size())
		{
			const auto midJD = (solA.JD + solB.JD)/2;
			solutionsPerTime.insert(solutionsPerTime.begin()+i,
			                        getShadowLimitQs(core, midJD, e2, penumbra));
			if(solutionsPerTime[i].values.size() % 2)
			{
				qWarning() << "Found an odd number of values of Q while searching for "
					      "JD of change in number of solutions:"
					   << solutionsPerTime[i].values.size();
			}
			// Retry with the first of the new intervals at the next iteration
			--i;
		}
	}

	// Search for time points where sign of zeta switches and
	// refine each case to get closer to the zero crossing.
	for(unsigned i = 1; i < solutionsPerTime.size(); ++i)
	{
		const auto& solA = solutionsPerTime[i-1];
		const auto& solB = solutionsPerTime[i];
		if(solA.values.size() != solB.values.size())
		{
			// Even if there's a sign change, we've already refined these
			// points, so it's not a problem that we skip this case here.
			continue;
		}

		if(std::abs(solA.JD - solB.JD) <= 0.001*secondsToDays)
		{
			// Already fine enough.
			continue;
		}

		for(int n = 0; n < solA.values.size(); ++n)
		{
			if(solA.values[n].zeta * solB.values[n].zeta < 0)
			{
				const auto midJD = (solA.JD + solB.JD)/2;
				solutionsPerTime.insert(solutionsPerTime.begin()+i,
				                        getShadowLimitQs(core, midJD, e2, penumbra));
				if(solutionsPerTime[i].values.size() % 2)
				{
					qWarning() << "Found an odd number of values of Q while searching for "
					              "JD of zeta sign change:"
					           << solutionsPerTime[i].values.size();
				}
				// Retry with the first of the new intervals at the next iteration
				--i;
			}
		}
	}

	if(solutionsPerTime.empty()) return;

	// Now treat all runs of the same solution counts as simultaneous runs of multiple lines, where for each
	// JD the point belonging to line n is the solution number n (the solutions are already sorted by Q).

	// 1. Compute the lines ignoring the sign of zeta
	struct Point
	{
		GeoPoint geo;
		double JD;
		double zeta;
		Point(const GeoPoint& geo, double JD, double zeta) : geo(geo), JD(JD), zeta(zeta) {}
	};
	std::vector<std::vector<Point>> lines;
	lines.resize(solutionsPerTime[0].values.size());
	for(unsigned i = 0, startN = 0; i < solutionsPerTime.size(); ++i)
	{
		const auto& sol = solutionsPerTime[i];
		if(i > 0 && sol.values.size() != solutionsPerTime[i-1].values.size())
		{
			startN = lines.size();
			lines.resize(lines.size() + sol.values.size());
		}
		for(unsigned n = 0; n < sol.values.size(); ++n)
		{
			const double JD = sol.JD;
			const double Q = sol.values[n].Q;
			const double zeta = sol.values[n].zeta;
			const auto tp = computeTimePoint(sol.bp, f, Q, zeta, penumbra);
			lines[startN + n].emplace_back(tp, JD, zeta);
		}
	}

	// 2. Remove the points under the horizon (i.e. where zeta < 0)
	for(unsigned n = 0; n < lines.size(); ++n)
	{
		auto& line = lines[n];
		if(line.empty()) continue;

		const auto negZetaIt = std::find_if(line.begin(), line.end(), [](auto& p){ return p.zeta < 0; });
		if(negZetaIt == line.end()) continue; // whole line is visible

		const auto nonNegZetaIt = std::find_if(line.begin(), line.end(),
		                                       [](auto& p){ return p.zeta >= 0; });
		if(nonNegZetaIt == line.end())
		{
			// Whole line is under the horizon
			line.clear();
			continue;
		}

		if(nonNegZetaIt == line.begin())
		{
			// Line starts with non-negative zetas, then gets under the horizon.  Move the second
			// part of the line to a new line, skipping all leading points with negative zeta.
			const auto nextNonNegZetaIt = std::find_if(negZetaIt, line.end(),
			                                           [](auto& p){ return p.zeta >= 0; });
			if(nextNonNegZetaIt != line.end())
				lines.emplace_back(nextNonNegZetaIt, line.end());
			line.erase(negZetaIt, line.end());
		}
		else
		{
			// Line starts with negative zetas and then there appear positive-zeta points.
			// Remove the negative-zeta head.
			const auto nextNonNegZetaIt = std::find_if(negZetaIt, line.end(),
			                                           [](auto& p){ return p.zeta >= 0; });
			line.erase(negZetaIt, nextNonNegZetaIt);
			if(!line.empty())
			{
				// The remaining points may still contain negative zetas,
				// so restart processing from the same line.
				--n;
			}
		}
	}

	// Finally, fill in the limits
	limits.clear();
	for(const auto& line : lines)
	{
		if(line.empty()) continue;
		auto& limit = limits.emplace_back();
		for(const auto& p : line)
		{
			limit.emplace_back(p.JD, p.geo.longitude, p.geo.latitude);
		}
	}
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
	// Reference for terminology and variable naming:
	// Explanatory Supplement to the Astronomical Ephemeris and the
	// American Ephemeris and Nautical Almanac, 3rd Edition (2013).
	//
	// The computation of the intersection is my own derivation, I haven't found it in the book.
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double ff = 1./(1.-f);

	GeoPoint coordinates(99., 0.);
	using namespace std;
	const double sind = sin(d);
	const double cosd = cos(d);
	static const double e2 = f*(2.-f);
	const double rho1 = sqrt(1-e2*sqr(cosd));
	const double rho2 = sqrt(1-e2*sqr(sind));
	const double sd1 = sind/rho1;
	const double cd1 = sqrt(1-e2)*cosd/rho1;
	const double sdd = e2*sind*cosd/(rho1*rho2);   // sin(d1-d2)
	const double cdd = sqrt(1-sqr(sdd));           // cos(d1-d2)

	// Semi-minor axis of the elliptic cross section of the
	// Earth in the fundamental plane (in Earth radii).
	const double k = 1/sqrt(sqr(sind)+sqr(cosd)/(1-e2));
	/*
	   We solve simultaneous equations: one for the ellipse of the Earth's border as
	   crossed by the fundamental plane, and the other is the circle of the shadow edge:

	                        ⎧ xi^2+eta^2/k^2=1
	                        ⎨
	                        ⎩ (xi-x)^2+(eta-y)^2=L^2

	    If we parametrize the solution for the Earth's border equation as

	                              xi=cos(t),
	                              eta=k*sin(t),

	    we'll get a single equation for the shadow border in terms of t:

	                    (cos(t)-x)^2+(k*sin(t)-y)^2=L^2.

	    This equation can be solved using Newton's method, which we'll now do.
	*/
	const auto lhsAndDerivative = [=](const double t) -> std::pair<double,double>
	{
		const auto cost = cos(t);
		const auto sint = sin(t);
		const auto lhs = sqr(cost-x) + sqr(k*sint-y) - sqr(L);
		const auto lhsPrime = 2*x*sint + 2*cost*((k*k-1)*sint - k*y);
		return std::make_pair(lhs,lhsPrime);
	};

	std::vector<double> ts;
	double t = 0;
	bool rootFound = false;
	do
	{
		if(ts.size() == 2) break; // there can't be more than 2 roots, so don't spend time uselessly

		rootFound = false;
		bool finalIteration = false;
		for(int n = 0; n < 50; ++n)
		{
			const auto [lhs, lhsPrime] = lhsAndDerivative(t);

			// Cancel the known roots to avoid finding them instead of the remaining ones
			auto newLHSPrime = lhsPrime;
			auto newLHS = lhs;
			for(const auto rootT : ts)
			{
				// We need a 2pi-periodic root-canceling function,
				// so take a sine of half the difference.
				const auto sinDiff = sin((t - rootT)/2);
				const auto cosDiff = cos((t - rootT)/2);

				newLHS /= sinDiff;
				newLHSPrime = (newLHSPrime - 0.5*cosDiff*newLHS)/sinDiff;
			}

			if(abs(newLHS) < 1e-10)
				finalIteration = true;

			const auto deltaT = newLHS / newLHSPrime;
			if(newLHSPrime==0 || abs(deltaT) > 1000)
			{
				// We are shooting too far away, convergence may be too slow.
				// Let's try perturbing t and retrying.
				t += 0.01;
				finalIteration = false;
				continue;
			}
			t -= deltaT;
			t = StelUtils::fmodpos(t, 2*M_PI);

			if(finalIteration)
			{
				ts.emplace_back(t);
				// Set a new initial value, but try to avoid setting it to 0 if the current
				// root is close to it (all values here are arbitrary in other respects).
				t = abs(t) > 0.5 ? 0 : -M_PI/2;

				rootFound = true;
				break;
			}
		}
	}
	while(rootFound);

	if(ts.empty()) return coordinates;

	double xi, eta;
	if(ts.size() == 1)
	{
		const auto t = ts[0];
		xi = cos(t);
		eta = k*sin(t);
	}
	else
	{
		// Whether a solution is "first" or "second" depends on which side from the
		// (0,0)-(x,y) line it is. To find this out we'll use the z component of
		// the vector product (x,y,0)×(xi,eta,0).
		const auto sin_t0 = sin(ts[0]);
		const auto cos_t0 = cos(ts[0]);
		const auto sin_t1 = sin(ts[1]);
		const auto cos_t1 = cos(ts[1]);

		const auto xi0 = cos_t0;
		const auto xi1 = cos_t1;
		const auto eta0 = k*sin_t0;
		const auto eta1 = k*sin_t1;

		const auto vecProdZ0 = x * eta0 - y * xi0;

		const bool use0 = first ? vecProdZ0 < 0 : vecProdZ0 > 0;

		xi  = use0 ? xi0  : xi1;
		eta = use0 ? eta0 : eta1;
	}

	const double eta1 = eta / rho1;
	const double zeta1 = (0 + eta1 * sdd) / cdd;

	const double b = -eta1*sd1+zeta1*cd1;
	const double theta = atan2(xi,b)*M_180_PI;
	double lngDeg = theta-mu;
	lngDeg = StelUtils::fmodpos(lngDeg, 360.);
	if (lngDeg > 180.) lngDeg -= 360.;
	const double sfn1 = eta1*cd1+zeta1*sd1;
	const double cfn1 = sqrt(1.-sfn1*sfn1);
	const double tanLat = ff*sfn1/cfn1;
	coordinates.latitude = atan(tanLat)*M_180_PI;
	coordinates.longitude = lngDeg;
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
	// Reference: Explanatory Supplement to the Astronomical Ephemeris
	// and the American Ephemeris and Nautical Almanac, 3rd Edition (2013)
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	static const double ff = 1./(1.-f);
	core->setJD(JD);
	core->update(0);
	const auto bp = calcBesselParameters(true);
	const double bdot = bp.bdot, cdot = bp.cdot;
	const double x = bp.elems.x, y = bp.elems.y, d = bp.elems.d, L1 = bp.elems.L1, mu = bp.elems.mu;

	using namespace std;

	const double sind = sin(d);
	const double cosd = cos(d);
	const double rho1 = sqrt(1-e2*sqr(cosd));
	const double rho2 = sqrt(1-e2*sqr(sind));
	const double sd1 = sind/rho1;
	const double cd1 = sqrt(1-e2)*cosd/rho1;
	const double sdd = e2*sind*cosd/(rho1*rho2);   // sin(d1-d2)
	const double cdd = sqrt(1-sqr(sdd));           // cos(d1-d2)

	double qa = atan2(bdot,cdot);
	if (!first) // there are two parts of the curve
		qa += M_PI;
	const double sgqa = x*cos(qa)-y*sin(qa);

	GeoPoint coordinates(99., 0.);

	// Iteration as described in equations (11.89) and (11.94) in the reference book
	double rho = 1, gamma;
	for(int n = 0; n < 3; ++n)
	{
		if(abs(sgqa / rho) > 1) return coordinates;
		const double gqa = asin(sgqa / rho);
		gamma = gqa+qa;
		const double cosGamma = cos(gamma);
		const double rho1sinGamma = rho1 * sin(gamma);
		// simplified sin(atan2(rho1 * sin(gamma), cos(gamma)))
		const double sinGammaPrime = rho1sinGamma / sqrt(sqr(rho1sinGamma)+sqr(cosGamma));
		rho = sinGammaPrime / sin(gamma);
	}

	const double xi = rho * sin(gamma);
	const double eta = rho * cos(gamma);

	if (sqr(x-xi)+sqr(y-eta) > sqr(L1)) return coordinates;

	const double eta1 = eta / rho1;
	const double zeta1 = (0 + eta1 * sdd) / cdd;

	const double b = -eta1*sd1+zeta1*cd1;
	const double theta = atan2(xi,b)*M_180_PI;
	double lngDeg = theta-mu;
	lngDeg = StelUtils::fmodpos(lngDeg, 360.);
	if (lngDeg > 180.) lngDeg -= 360.;
	const double sfn1 = eta1*cd1+zeta1*sd1;
	const double cfn1 = sqrt(1.-sfn1*sfn1);
	const double tanLat = ff*sfn1/cfn1;
	coordinates.latitude = atan(tanLat)*M_180_PI;
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

	for(const auto& umbraLimit : data.umbraLimits)
	{
		updatePen(data.eclipseType);
		points.clear();
		points.reserve(umbraLimit.size());
		for(const auto& p : umbraLimit)
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

	for(const auto& umbraLimit : data.umbraLimits)
	{
		startLinePlaceMark("Limit", data.eclipseType);
		stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
		for(const auto& p : umbraLimit)
			stream << p.longitude << "," << p.latitude << ",0.0\n";
		stream << "</coordinates>\n</LineString>\n</Placemark>\n";
	}

	stream << "</Document>\n</kml>\n";
}
