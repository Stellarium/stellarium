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

#include "HorizonsSite.hpp"
#include "HorizonsPrecNut.hpp"

#include <erfa.h>
#include <cmath>

namespace
{
const double J2000 = 2451545.0;
// (SS) 2026-11-09 own constants: the macros of erfam.h (DAS2R, ...) are not available through erfa.h
const double DAS2R = 4.848136811095359935899141e-6;	// arcsec -> rad
const double D2PI = 6.283185307179586476925287;
const double DAYSEC = 86400.0;
const double OMEGA_EARTH = 7.2921151467064e-5;	// rad/s, as the standalone program
}

double HorizonsSite::equationOfEquinoxes(double tCy, double dpsi, double epsMean)
{
	const double om = eraAnpm((450160.280 - (482002.628 + (7.455 + 0.008 * tCy) * tCy) * tCy) * DAS2R + std::fmod(5.0 * tCy, 1.0) * D2PI);
	return dpsi * std::cos(epsMean) + DAS2R * (0.00264 * std::sin(om) + 0.000063 * std::sin(om + om));
}

HorizonsSite::Result HorizonsSite::compute(const Params& p)
{
	Result res;

	HorizonsPrecNut::Params pp;
	pp.jdUT = p.jdUT;
	pp.jdTDB = p.jdTDB;
	pp.jdTT = p.jdTT;
	pp.zone4Start = p.zone4Start;
	pp.useNutation = p.useNutation;
	pp.eopDpsi = p.eopDpsi;
	pp.eopDeps = p.eopDeps;
	pp.forMoon = p.forMoon;
	const HorizonsPrecNut::Result pn = HorizonsPrecNut::compute(pp);
	res.zone = pn.zone;
	if (!pn.ok)
		return res;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			res.rSite[i][j] = pn.rSite[i][j];

	// no EOP before 1962-01-20: JD(UT) is UT1 and the pole is the mean pole
	const bool early = (pn.zone == HorizonsZones::Zone::Zone1 || pn.zone == HorizonsZones::Zone::Zone2);
	res.ut1utc = early ? 0. : p.ut1utc;
	res.xp = early ? 0. : p.xp;
	res.yp = early ? 0. : p.yp;
	res.lod = early ? 0. : p.lod;

	// 1. site in ITRS (WGS84, ellipsoid 1 of ERFA)
	if (eraGd2gc(1, p.lonRad, p.latRad, p.heightM, res.rItrs) != 0)
		return res;

	// 2. sidereal times: GMST 1982 + equation of the equinoxes with dPsi of the sidereal time (no empirical offset)
	res.gmst = eraAnp(eraGmst82(p.jdUT, res.ut1utc / DAYSEC));
	const double eqe = p.useNutation ? equationOfEquinoxes(pn.tCy, pn.dpsiST, pn.epsMean) : 0.;
	res.gast = eraAnp(res.gmst + eqe);
	res.last = eraAnp(res.gast + p.lonRad);

	// 3. polar motion matrix (identity before 1962)
	const double xpS = res.xp * DAS2R;
	const double ypS = res.yp * DAS2R;
	const double spS = early ? 0. : eraSp00(J2000, p.jdTT - J2000);
	eraIr(res.rpom);
	eraRz(spS, res.rpom);
	eraRy(-xpS, res.rpom);
	eraRx(-ypS, res.rpom);

	// 4. ITRS -> TIRS -> true equator of date -> J2000
	double rTirs[3], rz[3][3];
	eraTrxp(res.rpom, res.rItrs, rTirs);
	eraIr(rz);
	eraRz(-res.gast, rz);
	eraRxp(rz, rTirs, res.rTod);
	res.lastPM = eraAnp(std::atan2(res.rTod[1], res.rTod[0]));
	eraTrxp(res.rSite, res.rTod, res.rJ2000);

	// 5. velocity (diurnal aberration): omega x r in the true equator of date
	const double omega = OMEGA_EARTH * (1.0 - res.lod / DAYSEC);
	res.vTod[0] = -omega * res.rTod[1];
	res.vTod[1] =  omega * res.rTod[0];
	res.vTod[2] = 0.;
	eraTrxp(res.rSite, res.vTod, res.vJ2000);

	res.ok = true;
	return res;
}

void HorizonsSite::hourAngleAzEl(double lastPM, const double p3[3], double latRad, double& ha, double& az, double& el)
{
	double v[3] = { p3[0], p3[1], p3[2] };
	double ra, dec;
	eraC2s(v, &ra, &dec);
	ra = eraAnp(ra);
	ha = eraAnpm(lastPM - ra);	// west positive, as Horizons
	eraHd2ae(ha, dec, latRad, &az, &el);
}
