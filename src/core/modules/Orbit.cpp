// orbit.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// CometOrbit, InitHyp,InitPar,InitEll,Init3D: Copyright (C) 1995,2007,2008 Johannes Gajdosik
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <functional>
#include <algorithm>
#include <cmath>
#include <cstring>

#include "Solve.hpp"
#include "Orbit.hpp"

using namespace std;

#define EPSILON 1e-10

#if defined(_MSC_VER)
// cuberoot is missing in VC++ !?
#define cbrt(x) pow((x),1./3.)
#endif

static
void InitHyp(double q,double n,double e,double dt,double &a1,double &a2) {
  const double a = q/(e-1.0);
  const double M = n * dt;
  double H = M;
  for (;;) { // Newton
    const double Hp = H;
    H = H-(e*sinh(H)-H-M)/(e*cosh(H)-1);
    if (fabs(H - Hp) < EPSILON) break;
  }
  const double h1 = q*sqrt((e+1.0)/(e-1.0));
  a1 = a*(e-cosh(H));
  a2 = h1*sinh(H);
}

static
void InitPar(double q,double n,double dt,double &a1,double &a2) {
  const double A = n*dt;
  const double h = sqrt(A*A+1.0);
  double c = cbrt(fabs(A)+h);
  c = c*c;
  const double tan_nu_h = 2*A/(1+c+1/c);
  a1 = q*(1-tan_nu_h*tan_nu_h);
  a2 = 2.0*q*tan_nu_h;
}

static
void InitEll(double q,double n,double e,double dt,double &a1,double &a2) {
  const double a = q/(1.0-e);
  double M = fmod(n*dt,2*M_PI);
  if (M < 0.0) M += 2.0*M_PI;
  double H = M;
  for (;;) { // Newton
    const double Hp = H;
    H = H-(M-H+e*sin(H))/(e*cos(H)-1);
    if (fabs(H-Hp) < EPSILON) break;
  }
  const double h1 = q*sqrt((1.0+e)/(1.0-e));
  a1 = a*(cos(H)-e);
  a2 = h1*sin(H);
}

void Init3D(double i,double Omega,double o,double a1,double a2,
            double &x1,double &x2,double &x3) {
  const double co = cos(o);
  const double so = sin(o);
  const double cOm = cos(Omega);
  const double sOm = sin(Omega);
  const double ci = cos(i);
  const double si = sin(i);
  const double d11=-so*sOm*ci+co*cOm;
  const double d12=-co*sOm*ci-so*cOm;
  const double d21= so*cOm*ci+co*sOm;
  const double d22= co*cOm*ci-so*sOm;
  const double d31= so*si;
  const double d32= co*si;
  x1 = d11*a1+d12*a2;
  x2 = d21*a1+d22*a2;
  x3 = d31*a1+d32*a2;
}


CometOrbit::CometOrbit(double pericenterDistance,
                       double eccentricity,
                       double inclination,
                       double ascendingNode,
                       double argOfPerhelion,
                       double timeAtPerihelion,
                       double meanMotion,
                       double parentRotObliquity,
                       double parentRotAscendingnode,
                       double parentRotJ2000Longitude)
           :q(pericenterDistance),e(eccentricity),i(inclination),
            Om(ascendingNode),o(argOfPerhelion),t0(timeAtPerihelion),
            n(meanMotion) {
  const double c_obl = cos(parentRotObliquity);
  const double s_obl = sin(parentRotObliquity);
  const double c_nod = cos(parentRotAscendingnode);
  const double s_nod = sin(parentRotAscendingnode);
  const double cj = cos(parentRotJ2000Longitude);
  const double sj = sin(parentRotJ2000Longitude);
//  rotateToVsop87[0] =  c_nod;
//  rotateToVsop87[1] = -s_nod * c_obl;
//  rotateToVsop87[2] =  s_nod * s_obl;
//  rotateToVsop87[3] =  s_nod;
//  rotateToVsop87[4] =  c_nod * c_obl;
//  rotateToVsop87[5] = -c_nod * s_obl;
//  rotateToVsop87[6] =  0.0;
//  rotateToVsop87[7] =          s_obl;
//  rotateToVsop87[8] =          c_obl;
  rotateToVsop87[0] =  c_nod*cj-s_nod*c_obl*sj;
  rotateToVsop87[1] = -c_nod*sj-s_nod*c_obl*cj;
  rotateToVsop87[2] =           s_nod*s_obl;
  rotateToVsop87[3] =  s_nod*cj+c_nod*c_obl*sj;
  rotateToVsop87[4] = -s_nod*sj+c_nod*c_obl*cj;
  rotateToVsop87[5] =          -c_nod*s_obl;
  rotateToVsop87[6] =                 s_obl*sj;
  rotateToVsop87[7] =                 s_obl*cj;
  rotateToVsop87[8] =                 c_obl;
}

void CometOrbit::positionAtTimevInVSOP87Coordinates(double JD,double *v) const {
  JD -= t0;
  double a1,a2;
  // temporary solve freezes for near-parabolic comets - using (e < 0.9999) for elliptical orbits
  // TODO: improve calculations orbits for near-parabolic comets --AW
  if (e < 0.9999) InitEll(q,n,e,JD,a1,a2);
  else if (e > 1.0) InitHyp(q,n,e,JD,a1,a2);
  else InitPar(q,n,JD,a1,a2);
  double p0,p1,p2;
  Init3D(i,Om,o,a1,a2,p0,p1,p2);
  v[0] = rotateToVsop87[0]*p0 + rotateToVsop87[1]*p1 + rotateToVsop87[2]*p2;
  v[1] = rotateToVsop87[3]*p0 + rotateToVsop87[4]*p1 + rotateToVsop87[5]*p2;
  v[2] = rotateToVsop87[6]*p0 + rotateToVsop87[7]*p1 + rotateToVsop87[8]*p2;
}



EllipticalOrbit::EllipticalOrbit(double pericenterDistance,
                                 double eccentricity,
                                 double inclination,
                                 double ascendingNode,
                                 double argOfPeriapsis,
                                 double meanAnomalyAtEpoch,
                                 double period,
                                 double epoch,
                                 double parentRotObliquity,
                                 double parentRotAscendingnode,
								 double parentRotJ2000Longitude) :

    pericenterDistance(pericenterDistance),
    eccentricity(eccentricity),
    inclination(inclination),
    ascendingNode(ascendingNode),
    argOfPeriapsis(argOfPeriapsis),
    meanAnomalyAtEpoch(meanAnomalyAtEpoch),
    period(period),
    epoch(epoch)
{

  const double c_obl = cos(parentRotObliquity);
  const double s_obl = sin(parentRotObliquity);
  const double c_nod = cos(parentRotAscendingnode);
  const double s_nod = sin(parentRotAscendingnode);
  const double cj = cos(parentRotJ2000Longitude);
  const double sj = sin(parentRotJ2000Longitude);

  rotateToVsop87[0] =  c_nod*cj-s_nod*c_obl*sj;
  rotateToVsop87[1] = -c_nod*sj-s_nod*c_obl*cj;
  rotateToVsop87[2] =           s_nod*s_obl;
  rotateToVsop87[3] =  s_nod*cj+c_nod*c_obl*sj;
  rotateToVsop87[4] = -s_nod*sj+c_nod*c_obl*cj;
  rotateToVsop87[5] =          -c_nod*s_obl;
  rotateToVsop87[6] =                 s_obl*sj;
  rotateToVsop87[7] =                 s_obl*cj;
  rotateToVsop87[8] =                 c_obl;
}

// Standard iteration for solving Kepler's Equation
struct SolveKeplerFunc1 : public unary_function<double, double>
{
    double ecc;
    double M;

    SolveKeplerFunc1(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        return M + ecc * sin(x);
    }
};


// Faster converging iteration for Kepler's Equation; more efficient
// than above for orbits with eccentricities greater than 0.3.  This
// is from Jean Meeus's _Astronomical Algorithms_ (2nd ed), p. 199
struct SolveKeplerFunc2 : public unary_function<double, double>
{
    double ecc;
    double M;

    SolveKeplerFunc2(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        return x + (M + ecc * sin(x) - x) / (1 - ecc * cos(x));
    }
};

double sign(double x)
{
    if (x < 0.)
        return -1.;
    else if (x > 0.)
        return 1.;
    else
        return 0.;
}

struct SolveKeplerLaguerreConway : public unary_function<double, double>
{
    double ecc;
    double M;

    SolveKeplerLaguerreConway(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        double s = ecc * sin(x);
        double c = ecc * cos(x);
        double f = x - s - M;
        double f1 = 1 - c;
        double f2 = s;
        x += -5 * f / (f1 + sign(f1) * sqrt(abs(16 * f1 * f1 - 20 * f * f2)));

        return x;
    }
};

struct SolveKeplerLaguerreConwayHyp : public unary_function<double, double>
{
    double ecc;
    double M;

    SolveKeplerLaguerreConwayHyp(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        double s = ecc * sinh(x);
        double c = ecc * cosh(x);
        double f = s - x - M;
        double f1 = c - 1;
        double f2 = s;
        x += -5 * f / (f1 + sign(f1) * sqrt(abs(16 * f1 * f1 - 20 * f * f2)));

        return x;
    }
};

typedef pair<double, double> Solution;


double EllipticalOrbit::eccentricAnomaly(double M) const
{
    if (eccentricity == 0.0)
    {
        // Circular orbit
        return M;
    }
    else if (eccentricity < 0.2)
    {
        // Low eccentricity, so use the standard iteration technique
        Solution sol = solveIteration_fixed(SolveKeplerFunc1(eccentricity, M), M, 5);
        return sol.first;
    }
    else if (eccentricity < 0.9)
    {
        // Higher eccentricity elliptical orbit; use a more complex but
        // much faster converging iteration.
        Solution sol = solveIteration_fixed(SolveKeplerFunc2(eccentricity, M), M, 6);
        // Debugging
        // qDebug("ecc: %f, error: %f mas\n",
        //        eccentricity, radToDeg(sol.second) * 3600000);
        return sol.first;
    }
    else if (eccentricity < 1.0)
    {
        // Extremely stable Laguerre-Conway method for solving Kepler's
        // equation.  Only use this for high-eccentricity orbits, as it
        // requires more calcuation.
        double E = M + 0.85 * eccentricity * sign(sin(M));
        Solution sol = solveIteration_fixed(SolveKeplerLaguerreConway(eccentricity, M), E, 8);
        return sol.first;
    }
    else if (eccentricity == 1.0)
    {
        // Nearly parabolic orbit; very common for comets
        // TODO: handle this
        return M;
    }
    else
    {
        // Laguerre-Conway method for hyperbolic (ecc > 1) orbits.
        double E = log(2 * M / eccentricity + 1.85);
        Solution sol = solveIteration_fixed(SolveKeplerLaguerreConwayHyp(eccentricity, M), E, 30);
        return sol.first;
    }
}


Vec3d EllipticalOrbit::positionAtE(double E) const
{
    double x, z;

    if (eccentricity < 1.0)
    {
        double a = pericenterDistance / (1.0 - eccentricity);
        x = a * (cos(E) - eccentricity);
        z = a * sqrt(1 - eccentricity * eccentricity) * -sin(E);
    }
    else if (eccentricity > 1.0)
    {
        double a = pericenterDistance / (1.0 - eccentricity);
        x = -a * (eccentricity - cosh(E));
        z = -a * sqrt(eccentricity * eccentricity - 1) * -sinh(E);
    }
    else
    {
        // TODO: Handle parabolic orbits
        x = 0.0;
        z = 0.0;
    }

    Mat4d R = (Mat4d::zrotation(ascendingNode) *
               Mat4d::xrotation(inclination) *
               Mat4d::zrotation(argOfPeriapsis));

    return R * Vec3d(x, -z, 0);
}


// Return the offset from the center
Vec3d EllipticalOrbit::positionAtTime(double t) const
{
    t = t - epoch;
    double meanMotion = 2.0 * M_PI / period;
    double meanAnomaly = meanAnomalyAtEpoch + t * meanMotion;
    double E = eccentricAnomaly(meanAnomaly);
    
    return positionAtE(E);
}

//void EllipticalOrbit::positionAtTime(double JD, double * X, double * Y, double * Z) const
//{
//	Vec3d pos = positionAtTime(JD);
//	*X=pos[2];
//	*Y=pos[0];
//	*Z=pos[1];
//}

//void EllipticalOrbit::positionAtTimev(double JD, double* v)
//{
//	Vec3d pos = positionAtTime(JD);
//	v[0]=pos[2];
//	v[1]=pos[0];
//	v[2]=pos[1];
//}

void EllipticalOrbit::positionAtTimevInVSOP87Coordinates(double JD, double* v) const
{
  Vec3d pos = positionAtTime(JD);
  v[0] = rotateToVsop87[0]*pos[0] + rotateToVsop87[1]*pos[1] + rotateToVsop87[2]*pos[2];
  v[1] = rotateToVsop87[3]*pos[0] + rotateToVsop87[4]*pos[1] + rotateToVsop87[5]*pos[2];
  v[2] = rotateToVsop87[6]*pos[0] + rotateToVsop87[7]*pos[1] + rotateToVsop87[8]*pos[2];
}

double EllipticalOrbit::getPeriod() const
{
    return period;
}


double EllipticalOrbit::getBoundingRadius() const
{
    // TODO: watch out for unbounded parabolic and hyperbolic orbits
    return pericenterDistance * ((1.0 + eccentricity) / (1.0 - eccentricity));
}


void EllipticalOrbit::sample(double, double, int nSamples, OrbitSampleProc& proc) const
{
	double dE = 2. * M_PI / (double) nSamples;
    for (int i = 0; i < nSamples; i++)
        proc.sample(positionAtE(dE * i));
}


Vec3d CachingOrbit::positionAtTime(double jd) const
{
    if (jd != lastTime)
    {
        lastTime = jd;
        lastPosition = computePosition(jd);
    }
    return lastPosition;
}


void CachingOrbit::sample(double start, double t, int nSamples,
                          OrbitSampleProc& proc) const
{
    double dt = t / (double) nSamples;
    for (int i = 0; i < nSamples; i++)
        proc.sample(positionAtTime(start + dt * i));
}


