/*
 * Time Navigator plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PlanetaryEventsMgr.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"
#include "SolarSystem.hpp"

#include <QtGlobal>
#include <QDebug>
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────────────────────────────────────

PlanetaryEventsMgr::PlanetaryEventsMgr(QObject* parent)
    : QObject(parent)
{
    core        = StelApp::getInstance().getCore();
    solarSystem = GETSTELMODULE(SolarSystem);

    sun   = solarSystem->getSun();
    earth = solarSystem->getEarth();
    moon  = solarSystem->getMoon();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public interface
// ─────────────────────────────────────────────────────────────────────────────


// ─────────────────────────────────────────────────────────────────────────────
//  windowFor / scanForType / findPrev / findNext
// ─────────────────────────────────────────────────────────────────────────────

double PlanetaryEventsMgr::windowFor(const QString& t) const
{
    if (t.startsWith("Mercury")) return WIN_MERCURY;
    if (t.startsWith("Venus"))   return WIN_VENUS;
    if (t.startsWith("Mars"))    return WIN_MARS;
    if (t.startsWith("Jupiter")) return WIN_JUPITER;
    if (t.startsWith("Saturn"))  return WIN_SATURN;
    if (t.startsWith("Uranus"))  return WIN_URANUS;
    if (t.startsWith("Neptune")) return WIN_NEPTUNE;
    if (t.startsWith("Moon"))    return WIN_MOON;
    return WIN_EARTH;
}

// ─────────────────────────────────────────────────────────────────────────────
//  scanForType: run the appropriate scanner for the given event-type string
//  and return a JD→value map of all hits in [startJD, stopJD].
// ─────────────────────────────────────────────────────────────────────────────

QMap<double,double> PlanetaryEventsMgr::scanForType(const QString& type,
                                                     double startJD,
                                                     double stopJD) const
{
    // ── Inner planet conjunctions ─────────────────────────────────────────────
    // scanConjOpp returns ALL conjunctions (both inf and sup) for inner planets.
    // We filter here to whichever kind was requested.
    auto innerConj = [&](const QString& name, bool wantInferior)
        -> QMap<double,double>
    {
        PlanetP p = solarSystem->searchByEnglishName(name);
        if (!p) return {};
        const auto raw = scanConjOpp(p, sun, startJD, stopJD, 0);
        QMap<double,double> result;
        for (auto it = raw.constBegin(); it != raw.constEnd(); ++it)
        {
            const double jde     = toJDE(it.key());
            const double dPlanet = (p->getHeliocentricEclipticPos(jde)
                                    - earth->getHeliocentricEclipticPos(jde)).norm();
            const bool inferior  = dPlanet < earth->getHeliocentricEclipticPos(jde).norm();
            if (inferior == wantInferior)
                result.insert(it.key(), it.value());
        }
        return result;
    };

    // ── Elongation ────────────────────────────────────────────────────────────
    auto innerElong = [&](const QString& name, bool wantEastern)
        -> QMap<double,double>
    {
        PlanetP p = solarSystem->searchByEnglishName(name);
        if (!p) return {};
        const auto raw = scanElongation(p, startJD, stopJD);
        QMap<double,double> result;
        for (auto it = raw.constBegin(); it != raw.constEnd(); ++it)
        {
            // negative value → eastern (see refineElongation)
            if (wantEastern == (it.value() < 0.0))
                result.insert(it.key(), it.value());
        }
        return result;
    };

    // ── Dispatch ──────────────────────────────────────────────────────────────
    if (type == PE::MercuryInfConj)    return innerConj("Mercury", true);
    if (type == PE::MercurySupConj)    return innerConj("Mercury", false);
    if (type == PE::MercuryElongE)     return innerElong("Mercury", true);
    if (type == PE::MercuryElongW)     return innerElong("Mercury", false);
    if (type == PE::VenusInfConj)      return innerConj("Venus", true);
    if (type == PE::VenusSupConj)      return innerConj("Venus", false);
    if (type == PE::VenusElongE)       return innerElong("Venus", true);
    if (type == PE::VenusElongW)       return innerElong("Venus", false);

    auto outerPlanet = [&](const QString& name) -> PlanetP {
        return solarSystem->searchByEnglishName(name);
    };


    if (type == PE::MarsOpposition)  { PlanetP p=outerPlanet("Mars");    return p?scanConjOpp(p,sun,startJD,stopJD,1):QMap<double,double>{}; }
    if (type == PE::MarsSupConj)     { PlanetP p=outerPlanet("Mars");    return p?scanConjOpp(p,sun,startJD,stopJD,0):QMap<double,double>{}; }

    if (type == PE::JupiterOpposition){ PlanetP p=outerPlanet("Jupiter"); return p?scanConjOpp(p,sun,startJD,stopJD,1):QMap<double,double>{}; }
    if (type == PE::JupiterSupConj)   { PlanetP p=outerPlanet("Jupiter"); return p?scanConjOpp(p,sun,startJD,stopJD,0):QMap<double,double>{}; }

    if (type == PE::SaturnOpposition) { PlanetP p=outerPlanet("Saturn");  return p?scanConjOpp(p,sun,startJD,stopJD,1):QMap<double,double>{}; }
    if (type == PE::SaturnSupConj)    { PlanetP p=outerPlanet("Saturn");  return p?scanConjOpp(p,sun,startJD,stopJD,0):QMap<double,double>{}; }

    if (type == PE::UranusOpposition) { PlanetP p=outerPlanet("Uranus");  return p?scanConjOpp(p,sun,startJD,stopJD,1):QMap<double,double>{}; }
    if (type == PE::UranusSupConj)    { PlanetP p=outerPlanet("Uranus");  return p?scanConjOpp(p,sun,startJD,stopJD,0):QMap<double,double>{}; }

    if (type == PE::NeptuneOpposition){ PlanetP p=outerPlanet("Neptune"); return p?scanConjOpp(p,sun,startJD,stopJD,1):QMap<double,double>{}; }
    if (type == PE::NeptuneSupConj)   { PlanetP p=outerPlanet("Neptune"); return p?scanConjOpp(p,sun,startJD,stopJD,0):QMap<double,double>{}; }

    if (type == PE::MoonNew)          return scanLunarPhase(startJD, stopJD,   0.0);
    if (type == PE::MoonFirstQuarter) return scanLunarPhase(startJD, stopJD,  90.0);
    if (type == PE::MoonFull)         return scanLunarPhase(startJD, stopJD, 180.0);
    if (type == PE::MoonLastQuarter)  return scanLunarPhase(startJD, stopJD, 270.0);
    if (type == PE::MoonPerigee)      return scanMoonApogeePerigee(startJD, stopJD, true);
    if (type == PE::MoonApogee)       return scanMoonApogeePerigee(startJD, stopJD, false);

    if (type == PE::EarthPerihelion)  return earthOrbitalPoints(startJD, stopJD, true);
    if (type == PE::EarthAphelion)    return earthOrbitalPoints(startJD, stopJD, false);

    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public: findPrev / findNext
// ─────────────────────────────────────────────────────────────────────────────

double PlanetaryEventsMgr::findPrev(double fromJD, const QString& eventType) const
{
    const double halfWin = windowFor(eventType);
    const QMap<double,double> hits = scanForType(eventType,
                                                  fromJD - halfWin,
                                                  fromJD - 1.0 / 1440.0);
    if (hits.isEmpty()) return 0.0;
    // QMap is sorted ascending — last key is the nearest before fromJD.
    return hits.lastKey();
}

double PlanetaryEventsMgr::findNext(double fromJD, const QString& eventType) const
{
    const double halfWin = windowFor(eventType);
    const QMap<double,double> hits = scanForType(eventType,
                                                  fromJD + 1.0 / 1440.0,
                                                  fromJD + halfWin);
    if (hits.isEmpty()) return 0.0;
    // First key is the nearest after fromJD.
    return hits.firstKey();
}


// ─────────────────────────────────────────────────────────────────────────────
//  Earth perihelion / aphelion — analytical (Meeus, Astronomical Algorithms ch.27)
// ─────────────────────────────────────────────────────────────────────────────

QMap<double,double> PlanetaryEventsMgr::earthOrbitalPoints(double startJD,
                                                             double stopJD,
                                                             bool perihelion) const
{
    QMap<double,double> result;

    // Convert a JD to a fractional year.
    auto jdToYear = [](double jd) -> double {
        return 2000.0 + (jd - 2451545.0) / 365.25;
    };

    // Meeus eq. 27.1 / 27.2: approximate JDE of perihelion (k integer or .0)
    // and aphelion (k + 0.5).
    // JDE = 2451547.507 + 365.2596358 * k   (perihelion, k = 0 at Jan 1999.971)
    // JDE = 2451534.007 + 365.2596358 * k   (aphelion,   k = 0 at Jul 1999.471)
    // Correction terms follow (Meeus table 27.a):
    auto computeJDE = [](double k, bool isPerihelion) -> double {
        double JDE0;
        if (isPerihelion)
            JDE0 = 2451547.507 + 365.2596358 * k;
        else
            // Meeus eq. 27.2: aphelion uses k + 0.5
            JDE0 = 2451547.507 + 365.2596358 * (k + 0.5);

        // Correction terms (Meeus table 27.a, largest terms only — sub-hour accuracy)
        const double T  = (JDE0 - 2451545.0) / 36525.0;
        const double T2 = T * T;
        const double T3 = T2 * T;

        // Anomaly of Sun (mean) in radians
        const double M  = (357.5291 + 35999.0503 * T - 0.0001559 * T2 - 0.00000048 * T3)
                          * M_PI / 180.0;
        const double M2 = 2.0 * M;
        const double M3 = 3.0 * M;
        const double M4 = 4.0 * M;

        double corr;
        if (isPerihelion)
        {
            corr =   1.278   * std::sin( -M )
                   - 0.055   * std::sin( -M2)
                   + 0.011   * std::sin( -M3)
                   - 0.00003 * std::sin( -M4);
        }
        else
        {
            corr =  -1.352   * std::sin(  M )
                   + 0.061   * std::sin(  M2)
                   + 0.011   * std::sin(  M3);
        }
        return JDE0 + corr;
    };

    // Find the range of k values whose JDE falls inside [startJD, stopJD].
    // k = 0 corresponds to perihelion ~Jan 3 1999 / aphelion ~Jul 3 1999.
    const double yearStart = jdToYear(startJD);
    const double yearStop  = jdToYear(stopJD);

    // k step: each k covers one year.  Search a slightly wider k range to be safe.
    int kMin = static_cast<int>(std::floor(yearStart - 2000.0)) - 1;
    int kMax = static_cast<int>(std::ceil (yearStop  - 2000.0)) + 1;

    for (int k = kMin; k <= kMax; ++k)
    {
        // For aphelion use k + 0.5 in the formula, but here we've already
        // absorbed that into the different JDE0 base constant above.
        // k is always an integer for both perihelion and aphelion.
        const double jde = computeJDE(static_cast<double>(k), perihelion);
        // Convert JDE back to JD (approximate — DeltaT is small here).
        const double jd  = jde - core->computeDeltaT(jde) / 86400.0;
        if (jd >= startJD && jd <= stopJD)
            result.insert(jd, jde);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  JDE / geometry helpers
// ─────────────────────────────────────────────────────────────────────────────

double PlanetaryEventsMgr::toJDE(double jd) const
{
    return jd + core->computeDeltaT(jd) / 86400.0;
}

Vec3d PlanetaryEventsMgr::geocentricJ2000(double jde,
                                           const PlanetP& planet) const
{
    // Equivalent to planet->getJ2000EquatorialPos(core) but without
    // requiring core->setJD() / core->update().  Aberration (~20 arcsec)
    // is intentionally omitted — irrelevant for zero-crossing detection.
    return StelCore::matVsop87ToJ2000.multiplyWithoutTranslation(
        planet->getHeliocentricEclipticPos(jde)
        - earth->getHeliocentricEclipticPos(jde)
    );
}

double PlanetaryEventsMgr::angularSeparation(double jd,
                                              const PlanetP& p1,
                                              const PlanetP& p2,
                                              int mode) const
{
    const double jde = toJDE(jd);
    const Vec3d v1   = geocentricJ2000(jde, p1);
    const Vec3d v2   = geocentricJ2000(jde, p2);
    double angle     = v1.angle(v2);
    if (mode == 1)          // opposition: minimise π − angle
        angle = M_PI - angle;
    return angle;
}

double PlanetaryEventsMgr::heliocentricDistance(double jde,
                                                 const PlanetP& planet) const
{
    return planet->getHeliocentricEclipticPos(jde).norm();
}

double PlanetaryEventsMgr::geocentricMoonDistance(double jde) const
{
    // Moon's parent is Earth; getEclipticPos(jde) gives the Moon-Earth vector.
    return moon->getEclipticPos(jde).norm();
}

double PlanetaryEventsMgr::moonSunEclLonDiff(double jde) const
{
    // Ecliptic longitude of Moon and Sun (via heliocentric positions of Earth
    // and Moon).  We only need the longitudinal difference.
    const Vec3d earthHelio = earth->getHeliocentricEclipticPos(jde);
    const Vec3d moonHelio  = moon ->getHeliocentricEclipticPos(jde);

    // Geocentric ecliptic position of Sun (= −Earth helio direction).
    const Vec3d sunGeo  = -earthHelio;
    const Vec3d moonGeo = moonHelio - earthHelio;

    const double lonSun  = std::atan2(sunGeo.v[1],  sunGeo.v[0])  * M_180_PI;
    const double lonMoon = std::atan2(moonGeo.v[1], moonGeo.v[0]) * M_180_PI;

    return StelUtils::fmodpos(lonMoon - lonSun, 360.0);
}

bool PlanetaryEventsMgr::secondObjectIsEast(double jd,
                                             const PlanetP& p1,
                                             const PlanetP& p2) const
{
    const double jde = toJDE(jd);
    const Vec3d v1   = geocentricJ2000(jde, p1);
    const Vec3d v2   = geocentricJ2000(jde, p2);
    const double lon1 = std::atan2(v1.v[1], v1.v[0]) * M_180_PI;
    const double lon2 = std::atan2(v2.v[1], v2.v[0]) * M_180_PI;
    return StelUtils::fmodpos(lon2 - lon1, 360.0) < 180.0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scan step helper
// ─────────────────────────────────────────────────────────────────────────────

double PlanetaryEventsMgr::initialStep(const QString& name) const
{
    // 1-day step for all outer planets — safe (opposition minima span days to
    // weeks) and fast enough (≤1600 evaluations per button click, <2ms).
    // Faster-moving bodies keep their finer steps.
    static const QMap<QString,double> steps = {
        { "Moon",     0.25 },
        { "Venus",    1.0  },
        { "Mercury",  1.0  },
        { "Mars",     1.0  },
        { "Jupiter",  1.0  },
        { "Saturn",   1.0  },
        { "Neptune",  1.0  },
        { "Uranus",   1.0  },
        { "Earth",    1.0  },
    };
    return steps.value(name, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Approach scanners
// ─────────────────────────────────────────────────────────────────────────────

// ── Conjunction (mode=0) / Opposition (mode=1) ────────────────────────────
QMap<double,double>
PlanetaryEventsMgr::scanConjOpp(const PlanetP& planet, const PlanetP& ref,
                                  double startJD, double stopJD, int mode) const
{
    QMap<double,double> result;
    QPair<double,double> extremum;

    // Use only the planet's own step — the Sun is a fixed reference, and
    // "Sun" step=1.0 in the table is intended for the Sun as a moving target,
    // not as a reference body.  This matches the effective behaviour of
    // AstroCalcDialog::findInitialStep for non-minor-planet objects (which
    // always returns min(10, (stopJD-startJD)/16) = 10 for our window).
    const double step0 = initialStep(planet->getEnglishName());
    double step    = step0;

    // Start the scan half a step before startJD so that the scan always
    // observes the full descending slope before any nearby minimum.
    // Without this offset, events within one step of startJD are missed
    // because prevSgn never reaches -1 before the sign reverses.
    double jd      = startJD - step0 * 0.5;
    double prevVal = angularSeparation(jd, planet, ref, mode);
    int    prevSgn = 0;
    jd += step;

    while (jd <= stopJD)
    {
        double val = angularSeparation(jd, planet, ref, mode);
        int    sgn = StelUtils::sign(val - prevVal);

        // Note: do NOT scale step by (ΔV/V) here — near opposition/conjunction
        // V→0 so the ratio explodes, causing a giant step that skips the event.
        // With step0=1 day the scan is already fine-grained enough.

        if (sgn != prevSgn && prevSgn == -1)
        {
            if (refineConjOpp(extremum, planet, ref, jd, step, sgn, mode))
            {
                double sepDeg = extremum.second * M_180_PI;
                if (sepDeg < 180.0)
                    result.insert(extremum.first, extremum.second);
            }
        }
        prevVal = val;  prevSgn = sgn;  jd += step;
    }
    return result;
}

// ── Greatest elongation ────────────────────────────────────────────────────
QMap<double,double>
PlanetaryEventsMgr::scanElongation(const PlanetP& planet,
                                    double startJD, double stopJD) const
{
    QMap<double,double> result;
    QPair<double,double> extremum;

    const double step0 = initialStep(planet->getEnglishName());
    double step     = step0;
    double jd       = startJD;
    double prevDist = angularSeparation(jd, planet, sun, 0);
    jd += step;

    while (jd <= stopJD)
    {
        double dist   = angularSeparation(jd, planet, sun, 0);
        double factor = (dist != 0.0) ? qAbs((dist - prevDist) / dist) : 0.0;
        step = (factor > 10.0) ? step0 * factor / 10.0 : step0;

        if (dist > prevDist)
        {
            if (step > step0)
            {
                jd -= step;  step = step0;
                while (jd <= stopJD)
                {
                    dist = angularSeparation(jd, planet, sun, 0);
                    if (dist < prevDist) break;
                    prevDist = dist;  jd += step;
                }
            }
            double savedStep = step;
            if (refineElongation(extremum, planet, jd, stopJD, step))
            {
                result.insert(extremum.first, extremum.second);
                jd += 2.0 * savedStep;
            }
        }
        prevDist = dist;  jd += step;
    }
    return result;
}

// ── Perihelion / Aphelion ─────────────────────────────────────────────────
QMap<double,double>
PlanetaryEventsMgr::scanOrbitalPoint(const PlanetP& planet,
                                      double startJD, double stopJD,
                                      bool minimal) const
{
    QMap<double,double> result;
    QPair<double,double> extremum;

    const double step0    = initialStep(planet->getEnglishName());
    const double stopJDfx = stopJD + step0;
    double step      = step0;
    double jd        = startJD - step0;
    double prevDist  = heliocentricDistance(toJDE(jd), planet);
    jd += step0;

    while (jd <= stopJDfx)
    {
        const double jde  = toJDE(jd);
        double dist        = heliocentricDistance(jde, planet);
        double factor      = (dist != 0.0) ? qAbs((dist - prevDist) / dist) : 0.0;
        step = (factor > 10.0) ? step0 * factor / 10.0 : step0;

        // minimal==true  → perihelion: dist must have been decreasing then starts increasing.
        // minimal==false → aphelion:   dist must have been increasing then starts decreasing.
        bool trigger = minimal ? (dist > prevDist) : (dist < prevDist);
        if (trigger)
        {
            if (step > step0)
            {
                jd -= step;  step = step0;
                while (jd <= stopJDfx)
                {
                    dist = heliocentricDistance(toJDE(jd), planet);
                    bool cont = minimal ? (dist <= prevDist) : (dist >= prevDist);
                    if (!cont) break;
                    prevDist = dist;  jd += step;
                }
            }
            if (refineOrbitalPoint(extremum, planet, jd, stopJDfx, step, minimal))
            {
                if (extremum.first > startJD && extremum.first < stopJD)
                    result.insert(extremum.first, extremum.second);
            }
        }
        prevDist = dist;  jd += step;
    }
    return result;
}

// ── Lunar phases ──────────────────────────────────────────────────────────
QMap<double,double>
PlanetaryEventsMgr::scanLunarPhase(double startJD, double stopJD,
                                    double phaseTarget) const
{
    // Step 0.25 day for the Moon (its synodic period is ~29.5 days).
    constexpr double step0 = 0.25;
    QMap<double,double> result;
    QPair<double,double> extremum;

    double jd   = startJD;
    double prev = moonSunEclLonDiff(toJDE(jd));
    jd += step0;

    while (jd <= stopJD)
    {
        double curr = moonSunEclLonDiff(toJDE(jd));

        // Detect crossing of phaseTarget (accounting for 360°/0° wrap).
        double dPrev = StelUtils::fmodpos(prev - phaseTarget, 360.0);
        double dCurr = StelUtils::fmodpos(curr - phaseTarget, 360.0);

        // A crossing occurs when we go from just below 360 to near 0
        // (i.e. dPrev > 180 and dCurr < 180, meaning the phase target
        // was crossed in the ascending direction).
        if (dPrev > 180.0 && dCurr < 180.0)
        {
            if (refineLunarPhase(extremum, jd, stopJD, step0, phaseTarget))
                result.insert(extremum.first, extremum.second);
        }

        prev = curr;  jd += step0;
    }
    return result;
}

// ── Moon apogee / perigee ─────────────────────────────────────────────────
QMap<double,double>
PlanetaryEventsMgr::scanMoonApogeePerigee(double startJD, double stopJD,
                                           bool perigee) const
{
    constexpr double step0 = 0.5;   // half day — Moon moves fast
    QMap<double,double> result;
    QPair<double,double> extremum;

    double jd       = startJD;
    double prevDist = geocentricMoonDistance(toJDE(jd));
    jd += step0;
    double prevPrevDist = prevDist;
    prevDist = geocentricMoonDistance(toJDE(jd));
    jd += step0;

    while (jd <= stopJD)
    {
        double dist = geocentricMoonDistance(toJDE(jd));

        // Detect a genuine extremum: distance was moving in one direction and
        // now reverses.  This only fires ONCE per extremum, not for every step
        // on the far side.
        // perigee: prevPrevDist > prevDist (decreasing) AND dist > prevDist (now increasing)
        // apogee:  prevPrevDist < prevDist (increasing) AND dist < prevDist (now decreasing)
        bool trigger = perigee ? (prevPrevDist > prevDist && dist > prevDist)
                                : (prevPrevDist < prevDist && dist < prevDist);
        if (trigger)
        {
            // The extremum is bracketed in [jd - 2*step0, jd].
            if (refineMoonApogeePerigee(extremum, jd - step0, jd - 2.0*step0, jd, perigee))
                result.insert(extremum.first, extremum.second);
        }
        prevPrevDist = prevDist;
        prevDist = dist;
        jd += step0;
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Precise bisectors
// ─────────────────────────────────────────────────────────────────────────────

// ── Conjunction / Opposition ──────────────────────────────────────────────
bool PlanetaryEventsMgr::refineConjOpp(QPair<double,double>& out,
                                        const PlanetP& p1, const PlanetP& p2,
                                        double jd, double step, int prevSign,
                                        int mode) const
{
    double prevVal = angularSeparation(jd, p1, p2, mode);
    step = -step / 2.0;
    prevSign = -prevSign;

    while (true)
    {
        jd += step;
        double val = angularSeparation(jd, p1, p2, mode);

        if (qAbs(step) < 1.0 / 1440.0)   // < 1 minute
        {
            out.first  = jd - step / 2.0;
            out.second = angularSeparation(out.first, p1, p2, mode);
            // Confirm this is a minimum (not a maximum) — same logic as original.
            return out.second < angularSeparation(out.first - 5.0, p1, p2, mode);
        }
        int sgn = StelUtils::sign(val - prevVal);
        if (sgn != prevSign)
        {
            step    = -step / 2.0;
            sgn     = -sgn;
        }
        prevVal  = val;
        prevSign = sgn;
    }
}

// ── Greatest elongation ────────────────────────────────────────────────────
bool PlanetaryEventsMgr::refineElongation(QPair<double,double>& out,
                                           const PlanetP& planet,
                                           double jd, double stopJD,
                                           double step) const
{
    double prevDist = angularSeparation(jd, planet, sun, 0);
    step = -step / 2.0;

    while (true)
    {
        jd += step;
        double dist = angularSeparation(jd, planet, sun, 0);

        if (qAbs(step) < 1.0 / 1440.0)
        {
            out.first  = jd - step / 2.0;
            out.second = angularSeparation(out.first, planet, sun, 0);
            if (out.second > angularSeparation(out.first - 5.0, planet, sun, 0))
            {
                // Eastern elongation → negative value (matches AstroCalcDialog convention).
                if (!secondObjectIsEast(out.first, planet, sun))
                    out.second *= -1.0;
                return true;
            }
            return false;
        }
        if (dist < prevDist)
            step = -step / 2.0;
        prevDist = dist;

        if (jd > stopJD)
            return false;
    }
}

// ── Perihelion / Aphelion ─────────────────────────────────────────────────
bool PlanetaryEventsMgr::refineOrbitalPoint(QPair<double,double>& out,
                                             const PlanetP& planet,
                                             double jd, double stopJD,
                                             double step, bool minimal) const
{
    double prevDist = heliocentricDistance(toJDE(jd), planet);
    step /= -2.0;

    while (true)
    {
        jd += step;
        double dist = heliocentricDistance(toJDE(jd), planet);

        if (qAbs(step) < 1.0 / 1440.0)
        {
            out.first  = jd - step / 2.0;
            out.second = heliocentricDistance(toJDE(out.first), planet);
            if (minimal)
            {
                // Verify it really is a minimum.
                if (out.second > heliocentricDistance(toJDE(out.first - 5.0), planet))
                {
                    out.second *= -1.0;   // negative → perihelion (matches original)
                    return true;
                }
                return false;
            }
            else
            {
                if (out.second < heliocentricDistance(toJDE(out.first - 5.0), planet))
                    return true;
                return false;
            }
        }
        bool shouldFlip = minimal ? (dist > prevDist) : (dist < prevDist);
        if (shouldFlip) step /= -2.0;
        prevDist = dist;

        if (jd > stopJD)
            return false;
    }
}

// ── Lunar phase ────────────────────────────────────────────────────────────
bool PlanetaryEventsMgr::refineLunarPhase(QPair<double,double>& out,
                                           double jd, double /*stopJD*/,
                                           double step,
                                           double phaseTarget) const
{
    // The scan detected a crossing of phaseTarget between (jd - step) and jd.
    // Bisect that interval to sub-minute precision.
    double lo = jd - step;   // before crossing: diff > 180
    double hi = jd;          // after  crossing: diff < 180

    while ((hi - lo) > 1.0 / 1440.0)  // until < 1 minute
    {
        double mid  = (lo + hi) * 0.5;
        double diff = StelUtils::fmodpos(
            moonSunEclLonDiff(toJDE(mid)) - phaseTarget, 360.0);
        if (diff > 180.0)
            lo = mid;   // crossing is still ahead
        else
            hi = mid;   // crossing is behind us
    }

    out.first  = (lo + hi) * 0.5;
    out.second = phaseTarget;
    return true;
}

// ── Moon apogee / perigee ─────────────────────────────────────────────────
bool PlanetaryEventsMgr::refineMoonApogeePerigee(QPair<double,double>& out,
                                                  double /*jd*/, double lo,
                                                  double hi,
                                                  bool perigee) const
{
    // Bisect the bracket [lo, hi] that contains the extremum.
    // For perigee: distance at mid < distance at edges → move towards the minimum.
    // We track which half contains the lower (perigee) or higher (apogee) distance.
    while ((hi - lo) > 1.0 / 1440.0)   // until < 1 minute
    {
        double mid      = (lo + hi) * 0.5;
        double distLo   = geocentricMoonDistance(toJDE(lo));
        double distMid  = geocentricMoonDistance(toJDE(mid));
        double distHi   = geocentricMoonDistance(toJDE(hi));

        if (perigee)
        {
            // Keep the half whose endpoint has the smaller distance
            // (the minimum is in that half).
            if (distLo < distHi)
                hi = mid;
            else
                lo = mid;
            Q_UNUSED(distMid);
        }
        else
        {
            // Keep the half whose endpoint has the larger distance.
            if (distLo > distHi)
                hi = mid;
            else
                lo = mid;
            Q_UNUSED(distMid);
        }
    }

    out.first  = (lo + hi) * 0.5;
    out.second = geocentricMoonDistance(toJDE(out.first));
    return true;
}
