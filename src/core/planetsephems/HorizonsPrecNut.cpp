/*
 * Stellarium
 * Copyright (C) 2026 Sylvain Simard
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

#include "HorizonsPrecNut.hpp"
#include "precession.h"

#include <erfa.h>

namespace
{
const double J2000 = 2451545.0;

// Obliquity of J2000 of the Owen (1990) Chebyshev fit, used to rotate from the equator to the ecliptic of J2000 in zones 1 and 5
// (as the standalone program does). It is computed once; the value is close to 84381.448".
double owenEps0()
{
	static const double eps0 = []()
	{
		double e, c, o, ps;
		getPrecessionAnglesOwenClassicExact(J2000, &e, &c, &o, &ps);
		return e;
	}();
	return eps0;
}
}

double HorizonsPrecNut::dpsiOffsetMas(HorizonsZones::Zone z, bool forMoon)
{
	switch (z)
	{
		case HorizonsZones::Zone::Zone1: return 65.80;
		case HorizonsZones::Zone::Zone2: return 65.49;
		case HorizonsZones::Zone::Zone4: return forMoon ? 0.5 : 1.79;
		case HorizonsZones::Zone::Zone5: return forMoon ? 0.5 : 2.42;
		default:                         return 0.;
	}
}

HorizonsPrecNut::Result HorizonsPrecNut::compute(const Params& p)
{
	Result res;
	res.zone = HorizonsZones::zoneFor(p.jdUT, p.jdTT, p.zone4Start);
	if (res.zone == HorizonsZones::Zone::None)
		return res;

	const bool owen = (res.zone == HorizonsZones::Zone::Zone1 || res.zone == HorizonsZones::Zone::Zone5);
	const double jd = (res.zone == HorizonsZones::Zone::Zone1 || res.zone == HorizonsZones::Zone::Zone2) ? p.jdTDB : p.jdTT;

	double rp[3][3];	// precession (zones 2 to 4) or ecliptic of J2000 + Owen precession (zones 1 and 5)
	if (owen)
	{
		// P = Rz(chi_A) Rx(-omega_A) Rz(-psi_A) Rx(eps0): ERFA rotations are applied in this order to the identity
		double epsa, chiA, omgA, psiA;
		getPrecessionAnglesOwenClassicExact(jd, &epsa, &chiA, &omgA, &psiA);
		eraIr(rp);
		eraRx(owenEps0(), rp);
		eraRz(-psiA, rp);
		eraRx(-omgA, rp);
		eraRz(chiA, rp);
		res.epsMean = epsa;
	}
	else
	{
		eraPmat76(J2000, jd - J2000, rp);
		res.epsMean = eraObl80(J2000, jd - J2000);
	}

	if (!p.useNutation)
	{
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				res.r[i][j] = rp[i][j];
		res.epsTrue = res.epsMean;
		res.ok = true;
		return res;
	}

	double dp80 = 0., de80 = 0.;
	eraNut80(J2000, jd - J2000, &dp80, &de80);
	double dpsi = dp80, deps = de80;
	const double offset = dpsiOffsetMas(res.zone, p.forMoon) / 1000.;	// arcsec
	switch (res.zone)
	{
		case HorizonsZones::Zone::Zone1:
		case HorizonsZones::Zone::Zone2:
			dpsi += offset * ARCSEC2RAD;
			break;
		case HorizonsZones::Zone::Zone3:
			dpsi += p.eopDpsi * ARCSEC2RAD;
			deps += p.eopDeps * ARCSEC2RAD;
			break;
		default:	// zones 4 and 5: last EOP plus the offset
			dpsi += (p.eopDpsi + offset) * ARCSEC2RAD;
			deps += p.eopDeps * ARCSEC2RAD;
			break;
	}
	double rn[3][3];
	eraNumat(res.epsMean, dpsi, deps, rn);
	eraRxr(rn, rp, res.r);	// P3 = N * P
	res.dpsi = dpsi;
	res.deps = deps;
	res.epsTrue = res.epsMean + deps;
	res.ok = true;
	return res;
}
