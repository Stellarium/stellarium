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

#ifndef PLANETARYEVENTSMGR_HPP
#define PLANETARYEVENTSMGR_HPP

#include <QObject>
#include <QMap>
#include <QPair>
#include <QString>
#include <initializer_list>

#include "SolarSystem.hpp"   // defines PlanetP = QSharedPointer<Planet>

class StelCore;
class SolarSystem;

// Event-type string constants.  Plain constexpr char* — visible everywhere,
// zero linkage issues, no per-TU copies.
struct PE {
    static constexpr const char* MercuryInfConj    = "Mercury inferior conjunction";
    static constexpr const char* MercurySupConj    = "Mercury superior conjunction";
    static constexpr const char* MercuryElongE     = "Mercury greatest elongation eastern";
    static constexpr const char* MercuryElongW     = "Mercury greatest elongation western";

    static constexpr const char* VenusInfConj      = "Venus inferior conjunction";
    static constexpr const char* VenusSupConj      = "Venus superior conjunction";
    static constexpr const char* VenusElongE       = "Venus greatest elongation eastern";
    static constexpr const char* VenusElongW       = "Venus greatest elongation western";

    static constexpr const char* MarsOpposition    = "Mars opposition";
    static constexpr const char* MarsSupConj       = "Mars superior conjunction";

    static constexpr const char* JupiterOpposition = "Jupiter opposition";
    static constexpr const char* JupiterSupConj    = "Jupiter superior conjunction";

    static constexpr const char* SaturnOpposition  = "Saturn opposition";
    static constexpr const char* SaturnSupConj     = "Saturn superior conjunction";

    static constexpr const char* UranusOpposition  = "Uranus opposition";
    static constexpr const char* UranusSupConj     = "Uranus superior conjunction";

    static constexpr const char* NeptuneOpposition = "Neptune opposition";
    static constexpr const char* NeptuneSupConj    = "Neptune superior conjunction";

    static constexpr const char* MoonNew           = "Moon new";
    static constexpr const char* MoonFirstQuarter  = "Moon first quarter";
    static constexpr const char* MoonFull          = "Moon full";
    static constexpr const char* MoonLastQuarter   = "Moon last quarter";
    static constexpr const char* MoonPerigee       = "Moon perigee";
    static constexpr const char* MoonApogee        = "Moon apogee";

    static constexpr const char* EarthPerihelion   = "Earth perihelion";
    static constexpr const char* EarthAphelion     = "Earth aphelion";
};

class PlanetaryEventsMgr : public QObject
{
    Q_OBJECT

public:
    explicit PlanetaryEventsMgr(QObject* parent = nullptr);
    ~PlanetaryEventsMgr() override = default;

    //! Nearest event of @p eventType strictly before @p fromJD.
    //! Returns 0.0 if none found within the search window.
    double findPrev(double fromJD, const QString& eventType) const;

    //! Nearest event of @p eventType strictly after @p fromJD.
    //! Returns 0.0 if none found within the search window.
    double findNext(double fromJD, const QString& eventType) const;

private:
    static constexpr double WIN_MERCURY = 200.0;
    static constexpr double WIN_VENUS   = 650.0;
    static constexpr double WIN_MARS    = 1600.0;  // synodic 780d, need 2×
    static constexpr double WIN_JUPITER = 800.0;   // synodic 399d, need 2×
    static constexpr double WIN_SATURN  = 800.0;   // synodic 378d, need 2×
    static constexpr double WIN_URANUS  = 800.0;   // synodic 370d, need 2×
    static constexpr double WIN_NEPTUNE = 800.0;   // synodic 368d, need 2×
    static constexpr double WIN_MOON    =  60.0;
    static constexpr double WIN_EARTH   = 366.0;

    StelCore*    core        = nullptr;
    SolarSystem* solarSystem = nullptr;
    PlanetP sun, earth, moon;

    double windowFor(const QString& eventType) const;

    double toJDE(double jd) const;
    Vec3d  geocentricJ2000(double jde, const PlanetP& planet) const;
    double angularSeparation(double jd, const PlanetP& p1, const PlanetP& p2, int mode) const;
    double heliocentricDistance(double jde, const PlanetP& planet) const;
    double geocentricMoonDistance(double jde) const;
    double moonSunEclLonDiff(double jde) const;
    bool   secondObjectIsEast(double jd, const PlanetP& p1, const PlanetP& p2) const;
    double initialStep(const QString& planetName) const;

    QMap<double,double> scanForType(const QString& eventType,
                                    double startJD, double stopJD) const;

    // Analytical perihelion (perihelion=true) / aphelion (perihelion=false) for Earth.
    // Uses the Meeus "Astronomical Algorithms" formula.
    QMap<double,double> earthOrbitalPoints(double startJD, double stopJD,
                                           bool perihelion) const;

    QMap<double,double> scanConjOpp(const PlanetP& planet, const PlanetP& ref,
                                    double startJD, double stopJD, int mode) const;
    QMap<double,double> scanElongation(const PlanetP& planet,
                                       double startJD, double stopJD) const;
    QMap<double,double> scanOrbitalPoint(const PlanetP& planet,
                                         double startJD, double stopJD, bool minimal) const;
    QMap<double,double> scanLunarPhase(double startJD, double stopJD, double phaseTarget) const;
    QMap<double,double> scanMoonApogeePerigee(double startJD, double stopJD, bool perigee) const;

    bool refineConjOpp(QPair<double,double>& out, const PlanetP& p1, const PlanetP& p2,
                       double jd, double step, int prevSign, int mode) const;
    bool refineElongation(QPair<double,double>& out, const PlanetP& planet,
                          double jd, double stopJD, double step) const;
    bool refineOrbitalPoint(QPair<double,double>& out, const PlanetP& planet,
                            double jd, double stopJD, double step, bool minimal) const;
    bool refineLunarPhase(QPair<double,double>& out,
                          double jd, double stopJD, double step, double phaseTarget) const;
    bool refineMoonApogeePerigee(QPair<double,double>& out,
                                 double jd, double lo, double hi, bool perigee) const;
};

#endif /* PLANETARYEVENTSMGR_HPP */
