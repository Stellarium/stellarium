/* 
Copyright 2004 Robert Spearman

Refactoring of customorbit.cpp from Celestia 1.3.2
Copyright (C) 2001, Chris Laurel <claurel@shatters.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <math.h>
#include "misc_stellplanet.h"


#define TWOPI 6.28318530717958647692
#define LPEJ 0.23509484  /* Longitude of perihelion of Jupiter */
#define AU 149597870.691

  /* These are required because the orbits of the Jovian 
     satellites are computed in units of their parent planets' radii. */
static const double JupiterRadius = 71398.0;
static const double JupAscendingNode = 0.3875155;

double pfmod( double x, double y );

double pfmod( double x, double y ) {

  int quotient = (int) fabs(x / y);
  if (x < 0.0)
    return x + (quotient + 1) * y;
  else
    return x - quotient * y;
}

double degToRan(double d);

double degToRad(double d) {
  return d/180 * M_PI;
}

void ComputeGalileanElements(double t,
                             double* l1, double* l2, double* l3, double* l4,
                             double* p1, double* p2, double* p3, double* p4,
                             double* w1, double* w2, double* w3, double* w4,
                             double* gamma, double* phi, double* psi,
                             double* G, double* Gp)
{
    // Parameter t is Julian days, epoch 1950.0.
    *l1 = 1.8513962 + 3.551552269981*t;
    *l2 = 3.0670952 + 1.769322724929*t;
    *l3 = 2.1041485 + 0.87820795239*t;
    *l4 = 1.473836 + 0.37648621522*t;

    *p1 = 1.69451 + 2.8167146e-3*t;
    *p2 = 2.702927 + 8.248962e-4*t;
    *p3 = 3.28443 + 1.24396e-4*t;
    *p4 = 5.851859 + 3.21e-5*t;

    *w1 = 5.451267 - 2.3176901e-3*t;
    *w2 = 1.753028 - 5.695121e-4*t;
    *w3 = 2.080331 - 1.25263e-4*t;
    *w4 = 5.630757 - 3.07063e-5*t;

    *gamma = 5.7653e-3*sin(2.85674 + 1.8347e-5*t) + 6.002e-4*sin(0.60189 - 2.82274e-4*t);
    *phi = 3.485014 + 3.033241e-3*t;
    *psi = 5.524285 - 3.63e-8*t;
    *G = 0.527745 + 1.45023893e-3*t + *gamma;
    *Gp = 0.5581306 + 5.83982523e-4*t;
}


/* local to jupiter coords */

void get_europa_parent_coords(double JD, double * X, double * Y, double * Z)
{

  /* Computation will yield latitude(L), longitude(B) and distance(R) relative to Jupiter
     which we convert to rectangular coords for jupiter */

    double t;
    double l1, l2, l3, l4;
    double p1, p2, p3, p4;
    double w1, w2, w3, w4;
    double gamma, phi, psi, G, Gp;
    double sigma, L, B, R;
    double T, P;

    /* Epoch for Galilean satellites is 1976 Aug 10 */
    t = JD - 2443000.5;

    ComputeGalileanElements(t,
                            &l1, &l2, &l3, &l4,
                            &p1, &p2, &p3, &p4,
                            &w1, &w2, &w3, &w4,
                            &gamma, &phi, &psi, &G, &Gp);

    /* Calculate periodic terms for longitude */
    sigma = 1.06476*sin(2*(l2 - l3)) + 0.04256*sin(l1 - 2*l2 + p3)
          + 0.03581*sin(l2 - p3) + 0.02395*sin(l1 - 2*l2 + p4)
          + 0.01984*sin(l2 - p4) - 0.01778*sin(phi)
          + 0.01654*sin(l2 - p2) + 0.01334*sin(l2 - 2*l3 + p2)
          + 0.01294*sin(p3 - p4) - 0.01142*sin(l2 - l3)
          - 0.01057*sin(G) - 7.75e-3*sin(2*(psi - LPEJ))
          + 5.24e-3*sin(2*(l1 - l2)) - 4.6e-3*sin(l1 - l3)
          + 3.16e-3*sin(psi - 2*G + w3 - 2*LPEJ) - 2.03e-3*sin(p1 + p3 - 2*LPEJ - 2*G)
          + 1.46e-3*sin(psi - w3) - 1.45e-3*sin(2*G)
          + 1.25e-3*sin(psi - w4) - 1.15e-3*sin(l1 - 2*l3 + p3)
          - 9.4e-4*sin(2*(l2 - w2)) + 8.6e-4*sin(2*(l1 - 2*l2 + w2))
          - 8.6e-4*sin(5*Gp - 2*G + 0.9115) - 7.8e-4*sin(l2 - l4)
          - 6.4e-4*sin(3*l3 - 7*l4 + 4*p4) + 6.4e-4*sin(p1 - p4)
          - 6.3e-4*sin(l1 - 2*l3 + p4) + 5.8e-4*sin(w3 - w4)
          + 5.6e-4*sin(2*(psi - LPEJ - G)) + 5.6e-4*sin(2*(l2 - l4))
          + 5.5e-4*sin(2*(l1 - l3)) + 5.2e-4*sin(3*l3 - 7*l4 + p3 +3*p4)
          - 4.3e-4*sin(l1 - p3) + 4.1e-4*sin(5*(l2 - l3))
          + 4.1e-4*sin(p4 - LPEJ) + 3.2e-4*sin(w2 - w3)
          + 3.2e-4*sin(2*(l3 - G - LPEJ));
    sigma = pfmod(sigma, 360.0);
    sigma = degToRad(sigma);
    L = l2 + sigma;

    /* Calculate periodic terms for the tangent of the latitude */
    B = 8.1004e-3*sin(L - w2) + 4.512e-4*sin(L - w3)
      - 3.284e-4*sin(L - psi) + 1.160e-4*sin(L - w4)
      + 2.72e-5*sin(l1 - 2*l3 + 1.0146*sigma + w2) - 1.44e-5*sin(L - w1)
      + 1.43e-5*sin(L + psi - 2*LPEJ - 2*G) + 3.5e-6*sin(L - psi + G)
      - 2.8e-6*sin(l1 - 2*l3 + 1.0146*sigma + w3);
    B = atan(B);

    /* Calculate the periodic terms for distance */
    R = 9.3848e-3*cos(l1 - l2) - 3.116e-4*cos(l2 - p3)
      - 1.744e-4*cos(l2 - p4) - 1.442e-4*cos(l2 - p2)
      + 5.53e-5*cos(l2 - l3) + 5.23e-5*cos(l1 - l3)
      - 2.9e-5*cos(2*(l1 - l2)) + 1.64e-5*cos(2*(l2 - w2))
      + 1.07e-5*cos(l1 - 2*l3 + p3) - 1.02e-5*cos(l2 - p1)
      - 9.1e-6*cos(2*(l1 - l3));
    R = 9.39657 * JupiterRadius * (1 + R);

    T = (JD - 2433282.423) / 36525.0;
    P = 1.3966626*T + 3.088e-4*T*T;
    L += degToRad(P);

    L += JupAscendingNode;

    /* Corrections for internal coordinate system ??? */
    //B -= (M_PI/2);
    //L += M_PI;

    /*
    return Point3d(cos(L) * sin(B) * R,
                   cos(B) * R,
                   -sin(L) * sin(B) * R);

    */

    /* convert to rectangular coord */
    //    sphe_to_rect(L, B, R, X, Y, Z);
    // convert R to AU
    sphe_to_rect(L, B, R/AU, X, Y, Z);
}


/* local to jupiter coords */

void get_callisto_parent_coords(double JD, double * X, double * Y, double * Z)
{

    //Computation will yield latitude(L), longitude(B) and distance(R) relative to Jupiter
    double t;
    double l1, l2, l3, l4;
    double p1, p2, p3, p4;
    double w1, w2, w3, w4;
    double gamma, phi, psi, G, Gp;
    double sigma, L, B, R;
    double T, P;

    //Epoch for Galilean satellites is 1976 Aug 10
    t = JD - 2443000.5;

    ComputeGalileanElements(t,
                            &l1, &l2, &l3, &l4,
                            &p1, &p2, &p3, &p4,
                            &w1, &w2, &w3, &w4,
                            &gamma, &phi, &psi, &G, &Gp);

    //Calculate periodic terms for longitude
    sigma =
        0.84287*sin(l4 - p4)
        + 0.03431*sin(p4 - p3)
        - 0.03305*sin(2*(psi - LPEJ))
        - 0.03211*sin(G)
        - 0.01862*sin(l4 - p3)
        + 0.01186*sin(psi - w4)
        + 6.23e-3*sin(l4 + p4 - 2*G - 2*LPEJ)
        + 3.87e-3*sin(2*(l4 - p4))
        - 2.84e-3*sin(5*Gp - 2*G + 0.9115)
        - 2.34e-3*sin(2*(psi - p4))
        - 2.23e-3*sin(l3 - l4)
        - 2.08e-3*sin(l4 - LPEJ)
        + 1.78e-3*sin(psi + w4 - 2*p4)
        + 1.34e-3*sin(p4 - LPEJ)
        + 1.25e-3*sin(2*(l4 - G - LPEJ))
        - 1.17e-3*sin(2*G)
        - 1.12e-3*sin(2*(l3 - l4))
        + 1.07e-3*sin(3*l3 - 7*l4 + 4*p4)
        + 1.02e-3*sin(l4 - G - LPEJ)
        + 9.6e-4*sin(2*l4 - psi - w4)
        + 8.7e-4*sin(2*(psi - w4))
        - 8.5e-4*sin(3*l3 - 7*l4 + p3 + 3*p4)
        + 8.5e-4*sin(l3 - 2*l4 + p4)
        - 8.1e-4*sin(2*(l4 - psi))
        + 7.1e-4*sin(l4 + p4 - 2*LPEJ - 3*G)
        + 6.1e-4*sin(l1 - l4)
        - 5.6e-4*sin(psi - w3)
        - 5.4e-4*sin(l3 - 2*l4 + p3)
        + 5.1e-4*sin(l2 - l4)
        + 4.2e-4*sin(2*(psi - G - LPEJ))
        + 3.9e-4*sin(2*(p4 - w4))
        + 3.6e-4*sin(psi + LPEJ - p4 - w4)
        + 3.5e-4*sin(2*Gp - G + 3.2877)
        - 3.5e-4*sin(l4 - p4 + 2*LPEJ - 2*psi)
        - 3.2e-4*sin(l4 + p4 - 2*LPEJ - G)
        + 3.0e-4*sin(2*Gp - 2*G + 2.6032)
        + 2.9e-4*sin(3*l3 - 7*l4 + 2*p3 + 2*p4)
        + 2.8e-4*sin(l4 - p4 + 2*psi - 2*LPEJ)
        - 2.8e-4*sin(2*(l4 - w4))
        - 2.7e-4*sin(p3 - p4 + w3 - w4)
        - 2.6e-4*sin(5*Gp - 3*G + 3.2877)
        + 2.5e-4*sin(w4 - w3)
        - 2.5e-4*sin(l2 - 3*l3 + 2*l4)
        - 2.3e-4*sin(3*(l3 - l4))
        + 2.1e-4*sin(2*l4 - 2*LPEJ - 3*G)
        - 2.1e-4*sin(2*l3 - 3*l4 + p4)
        + 1.9e-4*sin(l4 - p4 - G)
        - 1.9e-4*sin(2*l4 - p3 - p4)
        - 1.8e-4*sin(l4 - p4 + G)
        - 1.6e-4*sin(l4 + p3 - 2*LPEJ - 2*G);
    sigma = pfmod(sigma, 360.0);
    sigma = degToRad(sigma);
    L = l4 + sigma;

    //Calculate periodic terms for the tangent of the latitude
    B =
        - 7.6579e-3 * sin(L - psi)
        + 4.4134e-3 * sin(L - w4)
        - 5.112e-4  * sin(L - w3)
        + 7.73e-5   * sin(L + psi - 2*LPEJ - 2*G)
        + 1.04e-5   * sin(L - psi + G)
        - 1.02e-5   * sin(L - psi - G)
        + 8.8e-6    * sin(L + psi - 2*LPEJ - 3*G)
        - 3.8e-6    * sin(L + psi - 2*LPEJ - G);
    B = atan(B);

    //Calculate the periodic terms for distance
    R =
        - 7.3546e-3 * cos(l4 - p4)
        + 1.621e-4  * cos(l4 - p3)
        + 9.74e-5   * cos(l3 - l4)
        - 5.43e-5   * cos(l4 + p4 - 2*LPEJ - 2*G)
        - 2.71e-5   * cos(2*(l4 - p4))
        + 1.82e-5   * cos(l4 - LPEJ)
        + 1.77e-5   * cos(2*(l3 - l4))
        - 1.67e-5   * cos(2*l4 - psi - w4)
        + 1.67e-5   * cos(psi - w4)
        - 1.55e-5   * cos(2*(l4 - LPEJ - G))
        + 1.42e-5   * cos(2*(l4 - psi))
        + 1.05e-5   * cos(l1 - l4)
        + 9.2e-6    * cos(l2 - l4)
        - 8.9e-6    * cos(l4 - LPEJ -G)
        - 6.2e-6    * cos(l4 + p4 - 2*LPEJ - 3*G)
        + 4.8e-6    * cos(2*(l4 - w4));
    
    R = 26.36273 * JupiterRadius * (1 + R);

    T = (JD - 2433282.423) / 36525.0;
    P = 1.3966626*T + 3.088e-4*T*T;
    L += degToRad(P);

    L += JupAscendingNode;

    sphe_to_rect(L, B, R/AU, X, Y, Z);

}



/* local to jupiter coords */

void get_io_parent_coords(double JD, double * X, double * Y, double * Z)
{

    //Computation will yield latitude(L), longitude(B) and distance(R) relative to Jupiter
    double t;
    double l1, l2, l3, l4;
    double p1, p2, p3, p4;
    double w1, w2, w3, w4;
    double gamma, phi, psi, G, Gp;
    double sigma, L, B, R;
    double T, P;

    // Epoch for Galilean satellites is 1976.0 Aug 10
    t = JD - 2443000.5;

    ComputeGalileanElements(t,
                            &l1, &l2, &l3, &l4,
                            &p1, &p2, &p3, &p4,
                            &w1, &w2, &w3, &w4,
                            &gamma, &phi, &psi, &G, &Gp);


    // Calculate periodic terms for longitude
    sigma = 0.47259*sin(2*(l1 - l2)) - 0.03478*sin(p3 - p4)
          + 0.01081*sin(l2 - 2*l3 + p3) + 7.38e-3*sin(phi)
          + 7.13e-3*sin(l2 - 2*l3 + p2) - 6.74e-3*sin(p1 + p3 - 2*LPEJ - 2*G)
          + 6.66e-3*sin(l2 - 2*l3 + p4) + 4.45e-3*sin(l1 - p3)
          - 3.54e-3*sin(l1 - l2) - 3.17e-3*sin(2*(psi - LPEJ))
          + 2.65e-3*sin(l1 - p4) - 1.86e-3*sin(G)
          + 1.62e-3*sin(p2 - p3) + 1.58e-3*sin(4*(l1 - l2))
          - 1.55e-3*sin(l1 - l3) - 1.38e-3*sin(psi + w3 - 2*LPEJ - 2*G)
          - 1.15e-3*sin(2*(l1 - 2*l2 + w2)) + 8.9e-4*sin(p2 - p4)
          + 8.5e-4*sin(l1 + p3 - 2*LPEJ - 2*G) + 8.3e-4*sin(w2 - w3)
          + 5.3e-4*sin(psi - w2);
    sigma = pfmod(sigma, 360.0);
    sigma = degToRad(sigma);
    L = l1 + sigma;

    // Calculate periodic terms for the tangent of the latitude
    B = 6.393e-4*sin(L - w1) + 1.825e-4*sin(L - w2)
      + 3.29e-5*sin(L - w3) - 3.11e-5*sin(L - psi)
      + 9.3e-6*sin(L - w4) + 7.5e-6*sin(3*L - 4*l2 - 1.9927*sigma + w2)
      + 4.6e-6*sin(L + psi - 2*LPEJ - 2*G);
    B = atan(B);

    // Calculate the periodic terms for distance
    R = -4.1339e-3*cos(2*(l1 - l2)) - 3.87e-5*cos(l1 - p3)
      - 2.14e-5*cos(l1 - p4) + 1.7e-5*cos(l1 - l2)
      - 1.31e-5*cos(4*(l1 - l2)) + 1.06e-5*cos(l1 - l3)
      - 6.6e-6*cos(l1 + p3 - 2*LPEJ - 2*G);
    R = 5.90569 * JupiterRadius * (1 + R);

    T = (JD - 2433282.423) / 36525.0;
    P = 1.3966626*T + 3.088e-4*T*T;
    L += degToRad(P);

    L += JupAscendingNode;



   sphe_to_rect(L, B, R/AU, X, Y, Z);
}


/* local to jupiter coords */

void get_ganymede_parent_coords(double JD, double * X, double * Y, double * Z)
{

    //Computation will yield latitude(L), longitude(B) and distance(R) relative to Jupiter
    double t;
    double l1, l2, l3, l4;
    double p1, p2, p3, p4;
    double w1, w2, w3, w4;
    double gamma, phi, psi, G, Gp;
    double sigma, L, B, R;
    double T, P;

    //Epoch for Galilean satellites is 1976 Aug 10
    t = JD - 2443000.5;

    ComputeGalileanElements(t,
                            &l1, &l2, &l3, &l4,
                            &p1, &p2, &p3, &p4,
                            &w1, &w2, &w3, &w4,
                            &gamma, &phi, &psi, &G, &Gp);

    //Calculate periodic terms for longitude
    sigma = 0.1649*sin(l3 - p3) + 0.09081*sin(l3 - p4)
          - 0.06907*sin(l2 - l3) + 0.03784*sin(p3 - p4)
          + 0.01846*sin(2*(l3 - l4)) - 0.01340*sin(G)
          - 0.01014*sin(2*(psi - LPEJ)) + 7.04e-3*sin(l2 - 2*l3 + p3)
          - 6.2e-3*sin(l2 - 2*l3 + p2) - 5.41e-3*sin(l3 - l4)
          + 3.81e-3*sin(l2 - 2*l3 + p4) + 2.35e-3*sin(psi - w3)
          + 1.98e-3*sin(psi - w4) + 1.76e-3*sin(phi)
          + 1.3e-3*sin(3*(l3 - l4)) + 1.25e-3*sin(l1 - l3)
          - 1.19e-3*sin(5*Gp - 2*G + 0.9115) + 1.09e-3*sin(l1 - l2)
          - 1.0e-3*sin(3*l3 - 7*l4 + 4*p4) + 9.1e-4*sin(w3 - w4)
          + 8.0e-4*sin(3*l3 - 7*l4 + p3 + 3*p4) - 7.5e-4*sin(2*l2 - 3*l3 + p3)
          + 7.2e-4*sin(p1 + p3 - 2*LPEJ - 2*G) + 6.9e-4*sin(p4 - LPEJ)
          - 5.8e-4*sin(2*l3 - 3*l4 + p4) - 5.7e-4*sin(l3 - 2*l4 + p4)
          + 5.6e-4*sin(l3 + p3 - 2*LPEJ - 2*G) - 5.2e-4*sin(l2 - 2*l3 + p1)
          - 5.0e-4*sin(p2 - p3) + 4.8e-4*sin(l3 - 2*l4 + p3)
          - 4.5e-4*sin(2*l2 - 3*l3 + p4) - 4.1e-4*sin(p2 - p4)
          - 3.8e-4*sin(2*G) - 3.7e-4*sin(p3 - p4 + w3 - w4)
          - 3.2e-4*sin(3*l3 - 7*l4 + 2*p3 + 2*p4) + 3.0e-4*sin(4*(l3 - l4))
          + 2.9e-4*sin(l3 + p4 - 2*LPEJ - 2*G) - 2.8e-4*sin(w3 + psi - 2*LPEJ - 2*G)
          + 2.6e-4*sin(l3 - LPEJ - G) + 2.4e-4*sin(l2 - 3*l3 + 2*l4)
          + 2.1e-4*sin(2*(l3 - LPEJ - G)) - 2.1e-4*sin(l3 - p2)
          + 1.7e-4*sin(l3 - p3);
    sigma = pfmod(sigma, 360.0);
    sigma = degToRad(sigma);
    L = l3 + sigma;

    //Calculate periodic terms for the tangent of the latitude
    B = 3.2402e-3*sin(L - w3) - 1.6911e-3*sin(L - psi)
      + 6.847e-4*sin(L - w4) - 2.797e-4*sin(L - w2)
      + 3.21e-5*sin(L + psi - 2*LPEJ - 2*G) + 5.1e-6*sin(L - psi + G)
      - 4.5e-6*sin(L - psi - G) - 4.5e-6*sin(L + psi - 2*LPEJ)
      + 3.7e-6*sin(L + psi - 2*LPEJ - 3*G) + 3.0e-6*sin(2*l2 - 3*L + 4.03*sigma + w2)
      - 2.1e-6*sin(2*l2 - 3*L + 4.03*sigma + w3);
    B = atan(B);

    //Calculate the periodic terms for distance
    R = -1.4388e-3*cos(l3 - p3) - 7.919e-4*cos(l3 - p4)
      + 6.342e-4*cos(l2 - l3) - 1.761e-4*cos(2*(l3 - l4))
      + 2.94e-5*cos(l3 - l4) - 1.56e-5*cos(3*(l3 - l4))
      + 1.56e-5*cos(l1 - l3) - 1.53e-5*cos(l1 - l2)
      + 7.0e-6*cos(2*l2 - 3*l3 + p3) - 5.1e-6*cos(l3 + p3 - 2*LPEJ - 2*G);
    R = 14.98832 * JupiterRadius * (1 + R);

    T = (JD - 2433282.423) / 36525.0;
    P = 1.3966626*T + 3.088e-4*T*T;
    L += degToRad(P);

    L += JupAscendingNode;



    sphe_to_rect(L, B, R/AU, X, Y, Z);

}






