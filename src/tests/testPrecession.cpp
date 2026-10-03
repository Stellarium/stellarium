/*
 * Stellarium 
 * Copyright (C) 2015 Georg Zotti
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

#include <QObject>
#include <QtDebug>
#include <QVariantList>

#include "tests/testPrecession.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"
#include <erfa.h>    // (SS) 2026-06-15 Added to generate a baseline and call to eraSepp() function
#include <erfam.h>   // (SS) 2026-06-15 Added to generate a baseline and call to eraSepp() function

QTEST_GUILESS_MAIN(TestPrecession)

static const double arcSec2Rad=M_PI*2.0/(360.0*3600.0);
static const double eps0=84381.406*arcSec2Rad;

void TestPrecession::initTestCase()
{
}

// (SS) 2026-06-15 : Major changes were made to testPrecession.cpp
// 
// Stellarium's implementations of Vondrak et al (2011) long term Precession parameterisations, PQXYe and
// ChiA-OmgA-PsiA-esp0, are verified against ERFA routines (eraLtpecl, eraLtpequ and eraLtp) instead
// of against each other. This approach has revealed some erroneous entries in Stellarium's PQXYe 
// polynomial coefficients. Those have been corrected along with this revision of the test procedure. 
// 
// The ERFA routines compute PECL, PEQR and the Precession Matrix P using the "primary" precession parameteris,
// i.e. PA, QA, XA and YA. This parameterisation is identical to Stellarium's PQXYe.
// 
// The test still verifies the example provided in section A.5 of Vondrak et al (2011) paper. However, 
// this example must be corrected, since coefficient C7 of parameter QA was erroneous in 2011, as 
// indicated in the Corrigendum paper of 2012. While the Corrigendum claims that this error does not affect
// any other expression provided in the 2011 paper, this is not true for the test case of section A.5.
// Revised PECL and PEQR reference values were regenerated from a separate FORTRAN program of mine designed
// to provide quadripole precision. This allows us to improve the pass/fail criteria from 1e-6 to 1e-15 for this
// test.
// 
// The original section of the code which attempts to compute angular differences between PQXYe and 
// ChiA-OmgA-PsiA-esp0 parameterisation has been commented out and replaced by an approach more in-line 
// with what Vondrak et al (2011) do in section 6 of their paper (6. Accuracy comparisons). As detailed in 
// this section most of the comparisons are done using the equator pole vectors of each parameterisation. 
// 
// "...Consequently,most of the comparisons below (Sects 6.1 to 6.4) are based solely on the angular separation
//  between the given equator pole and that predicted by the X_A and Y_A model (Eq (9)), as implemented in 
//  the ltp_PEQU subroutine..."
// 
// Since we are using the ERFA routines as our baseline, we will be able to compute this angular separattion
// for both Stellarium's parameterisation. The first one (i.e. PQXYe) must give 0.0 arcsecond angular separation for 
// all Julian Dates since it computes the PEQR vector as per our baseline from ERFA routines. The Stellarium's 
// ChiA-OmgA-PsiA-esp0 parameterisation, however, shall show an angular separation of the PEQR vector against 
// ERFA in conformance to the blue curve of Figure 12 of the Vondrak et al (2011) paper. This will be our 
// pass/fail criteria for the two Stellarium implementations.
// 
// IMPORTANT : It shall be noted that ERFA library build row-major matrices while Stellarium build 
// column-major matrices for comppatibility with OpenGL. This difference is taken into account in this 
// test procedure with additional steps of matrix transposition and matrix product ordering, as needed.

void TestPrecession::testPrecessionAnglesVondrak()
{
	double JulianDay = 0;
	double epj = 0;
	double T   = 0;
	
	const double S   = sin(eps0);
	const double C   = cos(eps0);
	double Z, W;
	double epsilon_A, chi_A, omega_A, psi_A;
	double P_A, Q_A, X_A, Y_A;
	double VEC2, VEC3, VEQ3;
	
	double a[3]  = {0.0};
	double b[3]  = {0.0};
	double Angle0 = 0.0;
	double Angle1 = 0.0;
	double Angle5 = 0.0;
	double Angle7 = 0.0;
	double Angle9 = 0.0;
	double Angle11 = 0.0;
	double Angle13 = 0.0;

	double L0_rad;
	double I0_rad;
	double eps0_owen;
	double L_rad;
	double I_rad;
	double Delta_rad;
	
	printf("\n");
	// 1219339.078000 is the Julian Date of the example in section A.5 of Vondrak et al (2011)
	// 1375 BCE, May 03, 13:52:19.2 TT (Gregorian Calendar)
	// 1375 BCE, May 15, 13:52:19.2 TT (Julian Calendar)
	JulianDay = 1219339.078000;
	printf("Julian Day       = %13.6f\n", JulianDay);

	// Compute the Julian Epoch of the corresponding Julian Date
	epj = 2000.0 + (JulianDay - ERFA_DJ00) / ERFA_DJY;
	printf("Julian Epoch     = %18.10f\n", epj);

	// Compute time in Julian centuries from J2000
	T = (JulianDay - ERFA_DJ00) / ERFA_DJC;
	printf("Julian Centuries = %11.3f\n\n", T);

	/*************************************************************************************************************/
	/* 1st : Compute Ecliptic & Equator pole vectors & Precession matrix using ERFA library                      */
	/*************************************************************************************************************/
	
	// Compute the ecliptic pole vector for the Julian Epoch using the ERFA library
	double pecl_era[3] = {0.0};
	eraLtpecl(epj, pecl_era);
	printf("pecl_erfa vector :  %18.15f   %18.15f   %18.15f  radians\n", pecl_era[0], pecl_era[1], pecl_era[2]);

	// Compute the equator pole vector for the Julian Epoch using the ERFA library
	double pequ_era[3] = {0.0};
	eraLtpequ(epj, pequ_era);
	printf("pequ_erfa vector :  %18.15f   %18.15f   %18.15f  radians\n\n", pequ_era[0], pequ_era[1], pequ_era[2]);

	// Compute the Precession matrix P for the Julian Epoch using the ERFA library
	double rp_era[3][3] = {0.0};
	eraLtp(epj, rp_era);

	printf("Precession matrix as computed by ERFA Library\n");
	printf("rp_era  1st row  :  %18.15f   %18.15f   %18.15f  radians\n", rp_era[0][0], rp_era[0][1], rp_era[0][2]);
	printf("rp_era  2nd row  :  %18.15f   %18.15f   %18.15f  radians\n", rp_era[1][0], rp_era[1][1], rp_era[1][2]);
	printf("rp_era  3rd row  :  %18.15f   %18.15f   %18.15f  radians\n", rp_era[2][0], rp_era[2][1], rp_era[2][2]);
	printf("\n");

	/*************************************************************************************************************/
	/* 2nd : Compute Ecliptic & Equator pole vectors & Precession matrix using Stellarium's PQXYe                */
	/*************************************************************************************************************/
	
	// Get reference angles from Vondrak PQXYe parameterisation using Stellarium code
	getPrecessionAnglesVondrakPQXYe(JulianDay, &P_A, &Q_A, &X_A, &Y_A, &epsilon_A);

	// Compute the ecliptic pole vector from Stellarium's PQXYe
	Z    = sqrt(qMax(1.0 - P_A * P_A - Q_A * Q_A, 0.0));
	VEC2 = -Q_A * C - Z * S;
	VEC3 = -Q_A * S + Z * C;
	Vec3d PECL(P_A, VEC2, VEC3);

	// Compute the equator pole vector from Stellarium's PQXYe
	W = X_A * X_A + Y_A * Y_A;
	VEQ3 = (W < 1.0 ? sqrt(1.0 - W) : 0.0);
	Vec3d PEQR(X_A, Y_A, VEQ3);

	// Here we are using the PECL and PEQU of Appendix A.5 from Vondrak et al (2011) paper for reference data.
	// However, these reference values are not valid since they were obtained with the faulty coefficient C7 of Q_A
	// I therefore replaced these quad precision reference values by those computed in a dedicated FORTRAN program of mine
	// I also improved the pass/fail criteria, from 1e-6 to 1e-15 which is at double precision level.
	printf("Verification of Stellarium's PECL and PEQR vectors against improved quadrupole precision reference values\n");
	QVERIFY2(fabs(P_A  - 0.00041724785764001342) <= 1e-15, QString("JD %1: Pecl,x: %2 Difference: %3").arg(JulianDay).arg(P_A).arg(P_A   - 0.00041724785764001342).toUtf8());
	QVERIFY2(fabs(VEC2 + 0.40495491375826546509) <= 1e-15, QString("JD %1: Pecl,y: %2 Difference: %3").arg(JulianDay).arg(VEC2).arg(VEC2 + 0.40495491375826546509).toUtf8());
	QVERIFY2(fabs(VEC3 - 0.91433655932991166508) <= 1e-15, QString("JD %1: Pecl,z: %2 Difference: %3").arg(JulianDay).arg(VEC3).arg(VEC3 - 0.91433655932991166508).toUtf8());
	QVERIFY2(fabs(X_A  + 0.29437643797369031532) <= 1e-15, QString("JD %1: Pequ,x: %2 Difference: %3").arg(JulianDay).arg(X_A).arg(X_A   + 0.29437643797369031532).toUtf8());
	QVERIFY2(fabs(Y_A  + 0.11719098023370257855) <= 1e-15, QString("JD %1: Pequ,y: %2 Difference: %3").arg(JulianDay).arg(Y_A).arg(Y_A   + 0.11719098023370257855).toUtf8());
	QVERIFY2(fabs(VEQ3 - 0.94847708824082091796) <= 1e-15, QString("JD %1: Pequ,z: %2 Difference: %3").arg(JulianDay).arg(VEQ3).arg(VEQ3 - 0.94847708824082091796).toUtf8());
	
	// the same, to be seen ...
	qDebug() << QString("JD %1: Pecl,x: %2 Difference: %3").arg(JulianDay, 8, 'f', 5).arg(P_A , 23, 'f', 20).arg(P_A  - 0.00041724785764001342, 23, 'f', 20);
	qDebug() << QString("JD %1: Pecl,y: %2 Difference: %3").arg(JulianDay, 8, 'f', 5).arg(VEC2, 23, 'f', 20).arg(VEC2 + 0.40495491375826546509, 23, 'f', 20);
	qDebug() << QString("JD %1: Pecl,z: %2 Difference: %3").arg(JulianDay, 8, 'f', 5).arg(VEC3, 23, 'f', 20).arg(VEC3 - 0.91433655932991166508, 23, 'f', 20);
	qDebug() << QString("JD %1: Pequ,x: %2 Difference: %3").arg(JulianDay, 8, 'f', 5).arg(X_A , 23, 'f', 20).arg(X_A  + 0.29437643797369031532, 23, 'f', 20);
	qDebug() << QString("JD %1: Pequ,y: %2 Difference: %3").arg(JulianDay, 8, 'f', 5).arg(Y_A , 23, 'f', 20).arg(Y_A  + 0.11719098023370257855, 23, 'f', 20);
	qDebug() << QString("JD %1: Pequ,z: %2 Difference: %3").arg(JulianDay, 8, 'f', 5).arg(VEQ3, 23, 'f', 20).arg(VEQ3 - 0.94847708824082091796, 23, 'f', 20);
	
	/*************************************************************************************************************/
	/* 3rd : Build the Precession matrix P using Stellarium's PQXYe method                                       */
	/*************************************************************************************************************/
	
	// top row of the Precession matrix P is the normalized cross product of the equator pole vector by the
	// ecliptic pole vector as per equ (23) of Vondrak et al (2011) paper.
	Vec3d EQX = PEQR^PECL;
	EQX.normalize();

	// middle row of the Precession matrix P is the cross product of the equator pole vector
	// by the top vector as per equ (23) of Vondrak et al (2011) paper
	Vec3d V = PEQR^EQX;

	// bottom row of the Precession matrix P is the equator pole vector as per equ (23)
	// of Vondrak et al (2011) paper. This is already computed above as PEQR.

	// Here we instantiate an object of type Mat3d, i.e. a 3x3 matrix. However the constructor is column-major
	// for compatibilty with OpenGL. That is, columns have contiguous location in memory, which means that 
	// the first three entries in RP (i.e. vector EQX) is the first column of this matrix, not the first row of the 
	// matrix as described in equ (23) of Vondrak et al (2011). For Stellarium, the column-major matrix is then  
	// the transposed of the Precession matrix of Vondrak et al (2011) paper. To compare it with our ERFA baseline
	// I added a matrix transposition step to get the Procession matrix instead of the transposed one.
	Mat3d RP(EQX[0], EQX[1], EQX[2], V[0], V[1], V[2], PEQR[0], PEQR[1], PEQR[2]);	
	
	Mat3d RTP = RP.transpose();
	printf("\n");
	printf("3x3 Precession matrix as computed by Stellarium's PQXYe parameterisation\n");
	printf("RTP 1st row      :  %18.15f   %18.15f   %18.15f  radians\n", RTP[0], RTP[3], RTP[6]);
	printf("RTP 2nd row      :  %18.15f   %18.15f   %18.15f  radians\n", RTP[1], RTP[4], RTP[7]);
	printf("RTP 3rd row      :  %18.15f   %18.15f   %18.15f  radians\n", RTP[2], RTP[5], RTP[8]);

	/*************************************************************************************************************/
	/* 4th : Compute Ecliptic & Equator pole vectors & Precession matrix using Stellarium's ChiA-OmgA-PsiA-eps0  */
	/*************************************************************************************************************/

	// Get reference angles for Vondrak ChiA-OmgA-PsiA-esp0 (or Capitaine et al.) parameterisation
	// We have to call this twice with different dates to avoid returning the cached (zero) angles.
	// However we must pick a Julian Day that is within the allowed range of +/- 2000 centuries around J2000
	// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
	// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
	// of JulianDay later on, we use the maximum Julian Day minus 2 days.
	getPrecessionAnglesVondrak(ERFA_DJ00 + 2000 * ERFA_DJC - 2, &epsilon_A, &chi_A, &omega_A, &psi_A);
	getPrecessionAnglesVondrak(JulianDay, &epsilon_A, &chi_A, &omega_A, &psi_A);
	
	// The xrotation(angle) method (and y and z) of Stellarium is generating the transposed version of the 
	// Rx(angle) matrix of ERFA, because these Mat4d are column-major instead of row-major. This explains why
	// the original code had to compose the matrix products in reverse order compared to Vondrak et al (2011) paper. 
	// In the paper they are building matrices row-major like ERFA routines. Since the rotation matrices are all 
	// transposed (being column-major), their products (in reverse order) generate the transposed Precession matrix.
	// That is, if P = A x B x C x D, then P' = D' x C' x B' x A', where X' is the transposed of X. I therefore
	// added a trasposition step as well to make the comparison of this matrix with the ERFA's matrix.
	
	Mat4d RRot=Mat4d::xrotation(eps0)*Mat4d::zrotation(-psi_A) * Mat4d::xrotation(-omega_A) * Mat4d::zrotation(chi_A);
	
	Mat4d RTRot = RRot.transpose();
	Vec4d RTRot_row1 = RTRot.getRow(0);
	Vec4d RTRot_row2 = RTRot.getRow(1);
	Vec4d RTRot_row3 = RTRot.getRow(2);
	Vec4d RTRot_row4 = RTRot.getRow(3);
	printf("\n");
	printf("4x4 Precession matrix as computed by Stellarium's ChiA-OmgA-PsiA-eps0 parameterisation\n");
	printf("RTRot 1st row    :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTRot_row1[0], RTRot_row1[1], RTRot_row1[2], RTRot_row1[3]);
	printf("RTRot 2nd row    :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTRot_row2[0], RTRot_row2[1], RTRot_row2[2], RTRot_row2[3]);
	printf("RTRot 3rd row    :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTRot_row3[0], RTRot_row3[1], RTRot_row3[2], RTRot_row3[3]);
	printf("RTRot 4th row    :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTRot_row4[0], RTRot_row4[1], RTRot_row4[2], RTRot_row4[3]);
	printf("\n");


	/*************************************************************************************************************/
	/* 5th : Compute Angular separation between Equator pole vectors of ERFA and Stellarium's parameterisations  */
	/*************************************************************************************************************/

	// The original code was attempting to estimate some angular separation by two different methods. 
	// 
	// The first one is to identify the max element of the matDiff3x3 matrix and then verify that it is less than 2e-5 radians as a 
	// pass/fail criteria. The second method seems to compute an angle (in degree) using the trace (sum of diagonal elements) 
	// of the transposed Precession matrix for each parameterisation. Then the code looks at the difference of these angles and scale
	// it in arcsecond. For this second method the pass/fail criteria is 6 arcsecond. 
	// 
	// This is not what Vondrak & al (2011) do to compare two parameterisations. The method explained in the introduction above
	// use the equator poles that are embedded within in the Precession matrices and then compute the angular separation between them. 
	// For a row-major Precession matrix, the equator pole vector is the bottom row of a 3x3 Precession matrix P as explained in 
	// section 5.4 of the Vondrak et al (2011) paper. But since Stellarium build column-major matrices, the equator pole vector will 
	// be the 3rd column of their 3x3 matrix or the third column of the upper3x3 section of their 4x4 matrix. That is...
	// 
	// The EQR vector from rp_era is obtained from matrix elements r[2][0], r[2][1] and r[2][2]
	// The EQR vector from RP     is obtained from matrix elements r[6], r[7] and r[8]
	// The EQR vector from RRot   is obtained form matrix elements r[8], r[9] and r[10] 
	// 
	// To get the angular separation between the two EQR vectors we use the erfa routine eraSepp(a,b). 
	// 
	// I therefore commented out the original code and replaced it by this new approach which more closely matches the Vondrak & al 
	// (2011) paper. The pass/fail criteria will consist in reproducing the blue curve of Figure 12 of the Vondrak &al (2011) paper.

	// Equator pole vector from ERFA 3x3 rp_era Precession matrix based on PQXYe parameterisation
	a[0] = rp_era[2][0]; 
	a[1] = rp_era[2][1]; 
	a[2] = rp_era[2][2]; 

	// Equator pole vector from Stellarium 3x3 RP Precession matrix based on PQXYe parameterisation
	b[0] = RP[6];
	b[1] = RP[7]; 
	b[2] = RP[8];

	Angle0 = eraSepp(a, b);
	printf("Angular separation between (a) EQR of ERFA and (b) EQR of Stellarium's PQXYe parameterisations\n");
	printf("a     (radians)  :  %18.15f   %18.15f   %18.15f\n", rp_era[2][0], rp_era[2][1], rp_era[2][2]);
	printf("b     (radians)  :  %18.15f   %18.15f   %18.15f\n", RP[6], RP[7], RP[8]);
	printf("Angle0 (arcsec)  :   %0.6f\n", Angle0 * ERFA_DR2AS); // Convert from radians to Arcsecond
	printf("\n");

	// Equator pole vector from Stellarium 4x4 RRot Precession matrix based on espA-ChiA-OmgA-PsiA parameterisation	
	b[0] = RRot[8];
	b[1] = RRot[9];
	b[2] = RRot[10];

	Angle1 = eraSepp(a, b);
	printf("Angular separation between (a) EQR of ERFA and (b) EQR of Stellarium's ChiA-OmgA-PsiA-eps0 parameterisations\n");
	printf("a     (radians)  :  %18.15f   %18.15f   %18.15f\n", rp_era[2][0], rp_era[2][1], rp_era[2][2]);
	printf("b     (radians)  :  %18.15f   %18.15f   %18.15f\n", RRot[8], RRot[9], RRot[10]);
	printf("Angle1 (arcsec)  :   %0.6f\n", Angle1 * ERFA_DR2AS); // Convert from radians to Arcsecond
	printf("\n");

	/*************************************************************************************************************/
	/* 6th : Build the Precession matrix P using Owen(90) L, I, Delta, I0, L0 method                             */
	/*************************************************************************************************************/

	L0_rad           = getPrecessionAnglesOwenL0();
	printf("L0     (radians):  %18.15f\n", L0_rad);
	printf("L0     (arcsecd):  %18.11f\n\n", L0_rad * ERFA_DR2AS);

	I0_rad           = getPrecessionAnglesOwenI0();
	printf("I0     (radians):  %18.15f\n", I0_rad);
	printf("I0     (arcsecd):  %18.11f\n", I0_rad * ERFA_DR2AS);

	//getPrecessionAnglesOwenLIDelta(ERFA_DJ00 + 120 * ERFA_DJC - 2, &L_rad, &I_rad, &Delta_rad);
	getPrecessionAnglesOwenLIDelta(JulianDay, &L_rad, &I_rad, &Delta_rad);

	Mat4d RP_LID = Mat4d::zrotation(L0_rad) * Mat4d::xrotation(I0_rad) * Mat4d::zrotation(-Delta_rad) * Mat4d::xrotation(-I_rad) * Mat4d::zrotation(-L_rad);
	
	Mat4d RTP_LID = RP_LID.transpose();
	Vec4d RTP_LID_row1 = RTP_LID.getRow(0);
	Vec4d RTP_LID_row2 = RTP_LID.getRow(1);
	Vec4d RTP_LID_row3 = RTP_LID.getRow(2);
	Vec4d RTP_LID_row4 = RTP_LID.getRow(3);
	printf("\n");
	printf("4x4 Precession matrix as computed by Owen(90) L, I, Delta\n");
	printf("RTP_LID 1st row  :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTP_LID_row1[0], RTP_LID_row1[1], RTP_LID_row1[2], RTP_LID_row1[3]);
	printf("RTP_LID 2nd row  :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTP_LID_row2[0], RTP_LID_row2[1], RTP_LID_row2[2], RTP_LID_row2[3]);
	printf("RTP_LID 3rd row  :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTP_LID_row3[0], RTP_LID_row3[1], RTP_LID_row3[2], RTP_LID_row3[3]);
	printf("RTP_LID 4th row  :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTP_LID_row4[0], RTP_LID_row4[1], RTP_LID_row4[2], RTP_LID_row4[3]);
	printf("\n");

	/*************************************************************************************************************/
	/* 7th : Angular separation between Equator pole of Owen(90) L, I, Delta & the other parameterisations       */
	/*************************************************************************************************************/

	// Equator pole vector from ERFA 3x3 rp_era Precession matrix based on PQXYe parameterisation
	a[0] = rp_era[2][0];
	a[1] = rp_era[2][1];
	a[2] = rp_era[2][2]; 

	// Equator pole vector from Owen(90) Precession matrix based on L, I, Delta parameterisation	
	b[0]   = RP_LID[8];
	b[1]   = RP_LID[9];
	b[2]   = RP_LID[10];

	Angle5 = eraSepp(a, b);
	printf("Angular separation between (a) EQR of ERFA and (b) EQR of Owen(90) L, I, Delta, I0, L0 "
	       "parameterisations\n");
	printf("a     (radians)  :  %18.15f   %18.15f   %18.15f\n", rp_era[2][0], rp_era[2][1], rp_era[2][2]);
	printf("b     (radians)  :  %18.15f   %18.15f   %18.15f\n", RP_LID[8], RP_LID[9], RP_LID[10]);
	printf("Angle5 (arcsec)  :   %0.6f\n", Angle5 * ERFA_DR2AS); // Convert from radians to Arcsecond
	printf("\n");

	// Equator pole vector from Stellarium 4x4 RRot Precession matrix based on espA-ChiA-OmgA-PsiA parameterisation
	a[0] = RRot[8];
	a[1] = RRot[9];
	a[2] = RRot[10]; 

	// Equator pole vector from Owen(90) Precession matrix based on L, I, Delta parameterisation
	b[0]   = RP_LID[8];
	b[1]   = RP_LID[9];
	b[2]   = RP_LID[10];

	Angle7 = eraSepp(a, b);
	printf("Angular separation between (a) EQR of Stellarium's ChiA-OmgA-PsiA-eps0 and (b) EQR of Owen(90) L, I, Delta, I0, L0 "
	       "parameterisations\n");
	printf("a     (radians)  :  %18.15f   %18.15f   %18.15f\n", RRot[8], RRot[9], RRot[10]);
	printf("b     (radians)  :  %18.15f   %18.15f   %18.15f\n", RP_LID[8], RP_LID[9], RP_LID[10]);
	printf("Angle7 (arcsec)  :   %0.6f\n", Angle7 * ERFA_DR2AS); // Convert from radians to Arcsecond
	printf("\n");

	/*************************************************************************************************************/
	/* 8th : Build the Precession matrix P using Owen(90) Classic angles espA-ChiA-OmgA-PsiA parameterisatio     */
	/*************************************************************************************************************/

	eps0_owen = getPrecessionAngleOwenEpsilon(ERFA_DJ00);
	printf("Eps0  (radians) :  %18.15f\n", eps0_owen);
	printf("Eps0  (arcsecd) :  %18.11f\n", eps0_owen * ERFA_DR2AS);

	getPrecessionAnglesOwenClassic(ERFA_DJ00 + 120 * ERFA_DJC - 2, &epsilon_A, &chi_A, &omega_A, &psi_A);
	getPrecessionAnglesOwenClassic(JulianDay, &epsilon_A, &chi_A, &omega_A, &psi_A);

	Mat4d RP_Clas = Mat4d::xrotation(eps0_owen) * Mat4d::zrotation(-psi_A) * Mat4d::xrotation(-omega_A) * Mat4d::zrotation(chi_A);

	Mat4d RTP_Clas      = RP_Clas.transpose();
	Vec4d RTP_Clas_row1 = RTP_Clas.getRow(0);
	Vec4d RTP_Clas_row2 = RTP_Clas.getRow(1);
	Vec4d RTP_Clas_row3 = RTP_Clas.getRow(2);
	Vec4d RTP_Clas_row4 = RTP_Clas.getRow(3);
	printf("\n");
	printf("4x4 Precession matrix as computed by Owen (90) classic ChiA-OmgA-PsiA-eps0 parameterisation\n");
	printf("RTP_Clas 1st row    :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTP_Clas_row1[0], RTP_Clas_row1[1], RTP_Clas_row1[2], RTP_Clas_row1[3]);
	printf("RTP_Clas 2nd row    :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTP_Clas_row2[0], RTP_Clas_row2[1], RTP_Clas_row2[2], RTP_Clas_row2[3]);
	printf("RTP_Clas 3rd row    :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTP_Clas_row3[0], RTP_Clas_row3[1], RTP_Clas_row3[2], RTP_Clas_row3[3]);
	printf("RTP_Clas 4th row    :  %18.15f   %18.15f   %18.15f   %18.15f\n", RTP_Clas_row4[0], RTP_Clas_row4[1], RTP_Clas_row4[2], RTP_Clas_row4[3]);
	printf("\n");

	/*************************************************************************************************************/
	/* 9th : Angular separation between Equator pole of Owen(90) Classic Angles & the other parameterisations    */
	/*************************************************************************************************************/

	// Equator pole vector from ERFA 3x3 rp_era Precession matrix based on PQXYe parameterisation
	a[0] = rp_era[2][0];
	a[1] = rp_era[2][1];
	a[2] = rp_era[2][2]; 

	// Equator pole vector from Owen(90) Precession matrix based on Classic angles espA-ChiA-OmgA-PsiA parameterisation
	b[0]   = RP_Clas[8];
	b[1]   = RP_Clas[9];
	b[2]   = RP_Clas[10];

	Angle9 = eraSepp(a, b);
	printf("Angular separation between (a) EQR of ERFA and (b) EQR of Owen(90) classic ChiA-OmgA-PsiA-eps0 "
	       "parameterisations\n");
	printf("a     (radians)  :  %18.15f   %18.15f   %18.15f\n", rp_era[2][0], rp_era[2][1], rp_era[2][2]);
	printf("b     (radians)  :  %18.15f   %18.15f   %18.15f\n", RP_Clas[8], RP_Clas[9], RP_Clas[10]);
	printf("Angle9 (arcsec)  :   %0.6f\n", Angle9 * ERFA_DR2AS); // Convert from radians to Arcsecond
	printf("\n");

	// Equator pole vector from Stellarium 4x4 RRot Precession matrix based on espA-ChiA-OmgA-PsiA parameterisation
	a[0] = RRot[8];
	a[1] = RRot[9];
	a[2] = RRot[10];

	// Equator pole vector from Owen(90) Precession matrix based on Classic angles espA-ChiA-OmgA-PsiA parameterisation
	b[0] = RP_Clas[8];
	b[1] = RP_Clas[9];
	b[2] = RP_Clas[10];

	Angle11 = eraSepp(a, b);
	printf("Angular separation between (a) EQR of Stellarium's ChiA-OmgA-PsiA-eps0 and (b)  EQR of Owen(90) "
	       "classic ChiA-OmgA-PsiA-eps0 parameterisations\n");
	printf("a     (radians)  :  %18.15f   %18.15f   %18.15f\n", RRot[8], RRot[9], RRot[10]);
	printf("b     (radians)  :  %18.15f   %18.15f   %18.15f\n", RP_Clas[8], RP_Clas[9], RP_Clas[10]);
	printf("Angle11 (arcsec) :   %0.6f\n", Angle11 * ERFA_DR2AS); // Convert from radians to Arcsecond
	printf("\n");

	// Equator pole vector from Owen(90) Precession matrix based on L, I, Delta parameterisation
	a[0] = RP_LID[8];
	a[1] = RP_LID[9];
	a[2] = RP_LID[10];

	// Equator pole vector from Owen(90) Precession matrix based on Classic angles espA-ChiA-OmgA-PsiA parameterisation
	b[0] = RP_Clas[8];
	b[1] = RP_Clas[9];
	b[2] = RP_Clas[10];

	Angle13 = eraSepp(a, b);
	printf("Angular separation between (a) EQR of Owen(90) L, I, Delta, I0, L0 and (b) EQR of Owen(90) classic ChiA-OmgA-PsiA-eps0 "
	       "parameterisations\n");
	printf("a     (radians)  :  %18.15f   %18.15f   %18.15f\n", RP_LID[8], RP_LID[9], RP_LID[10]);
	printf("b     (radians)  :  %18.15f   %18.15f   %18.15f\n", RP_Clas[8], RP_Clas[9], RP_Clas[10]);
	printf("Angle13 (arcsec) :   %0.6f\n", Angle13 * ERFA_DR2AS); // Convert from radians to Arcsecond
	printf("\n");
	
	/*************************************************************************************************************/
	/* 10th : This section captures Vondrak Precession from -2000 to +2000 centuries from J2000.0                */
	/*************************************************************************************************************/

	FILE* fp1 = 0; // File pointer for output file1

	if (fopen_s(&fp1, "../../Testing/Temporary/output1.txt", "w+") != 0)
	{
		perror("Error opening file output1.txt");
		return;
	}

	fprintf(fp1, "Now Processing: Vondrak Precession only from -2000 to +2000 centuries from J2000.0 @ 1 century "
	             "per step \n");
	fprintf(fp1, "     JD_TT        "
	             "     EPJ     "
	             "     centuries  "
	             "   Sep Angle 0 "
	             "      Sep Angle 1\n");

	const double DAYSTEP1 = ERFA_DJC; // Step 1 Julian century of 36525 days

	for (int i = -2000; i <= 2000; i++)
	{
		// Compute the Julian Day, Julian Epoch and Time in Julian century
		JulianDay  = ERFA_DJ00 + i * DAYSTEP1;
		epj   = 2000.0 + (JulianDay - ERFA_DJ00) / ERFA_DJY;
		T     = (JulianDay - ERFA_DJ00) / ERFA_DJC;

		// Call ERFA routines to compute the ecliptic and equator poles and the Precession matrix P
		// ERFA routines implement the Vondrak et al. (2011) PQXYe parameterisation.
		// We will use it as our baseline parameterisation to validate Stellarium PQXYe method
		// and to compute the angular separation with Stellarium ChiA-OmgA-PsiA-esp0 parameterisation
		eraLtpecl(epj, pecl_era);
		eraLtpequ(epj, pequ_era);
		eraLtp(epj, rp_era);

		// Get the Precession parameters pA, qA, xA, yA and epsA for the given Julian Date in TT using
		// Stellarium's PQXYe parameterisation of Vondrák et al. (2011) precession model
		getPrecessionAnglesVondrakPQXYe(JulianDay, &P_A, &Q_A, &X_A, &Y_A, &epsilon_A);

		// Compute the ecliptic pole vector from Stellarium's PQXYe
		Z    = sqrt(qMax(1.0 - P_A * P_A - Q_A * Q_A, 0.0));
		VEC2 = -Q_A * C - Z * S;
		VEC3 = -Q_A * S + Z * C;
		Vec3d PECL(P_A, VEC2, VEC3);

		// Compute the equator pole vector from Stellarium's PQXYe
		W    = X_A * X_A + Y_A * Y_A;
		VEQ3 = (W < 1.0 ? sqrt(1.0 - W) : 0.0);
		Vec3d PEQR(X_A, Y_A, VEQ3);

		/***** Build the Precession matrix P using Stellarium's PQXYe method ****/
		Vec3d EQX = PEQR ^ PECL;
		EQX.normalize();
		Vec3d V = PEQR ^ EQX;
		Mat3d RP(EQX[0], EQX[1], EQX[2], V[0], V[1], V[2], PEQR[0], PEQR[1], PEQR[2]);

		// Get the Precession parameters EpsilonA, ChiA, OmegaA and PsiA for the given Julian Date in TT using
		// Stellarium's espA-ChiA-OmgA-PsiA parameterisation of Vondrák et al. (2011) precession model
		// We have to call this twice with different dates to avoid returning the cached angles.
		// However we must pick a Julian Day that is within the allowed range of +/- 2000 centuries around J2000
		// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
		// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
		// of JulianDay we use the maximum Julian Day minus 2 days.
		getPrecessionAnglesVondrak(ERFA_DJ00 + 2000 * ERFA_DJC - 2, &epsilon_A, &chi_A, &omega_A, &psi_A);
		getPrecessionAnglesVondrak(JulianDay, &epsilon_A, &chi_A, &omega_A, &psi_A);
		
		// Build the Precession matrix P using Stellarium's espA-ChiA-OmgA-PsiA method
		Mat4d RRot = Mat4d::xrotation(eps0) * Mat4d::zrotation(-psi_A) * Mat4d::xrotation(-omega_A) * Mat4d::zrotation(chi_A);

		/**** Angular separation between Equator pole vectors ****/

		// Equator pole vector from ERFA 3x3 rp_era Precession matrix based on PQXYe parameterisation
		a[0] = rp_era[2][0];
		a[1] = rp_era[2][1];
		a[2] = rp_era[2][2];

		// Equator pole vector from Stellarium 3x3 RP Precession matrix based on PQXYe parameterisation
		b[0]  = RP[6];
		b[1]  = RP[7];
		b[2]  = RP[8];

		// Angular separation between (a) EQR of ERFA and (b) EQR of Stellarium's PQXYe parameterisations
		// There should be almost no angular separation between these two EQR poles. A verification is done at nano-arcsecond level
		Angle0 = eraSepp(a, b) * ERFA_DR2AS;
		QVERIFY2(Angle0 <= 1e-9, QString("Angular separation between ERFA and Stellarium's PQXYe is greater than 1.0E-9 arcsecond --> error!").toUtf8());

		// Equator pole vector from Stellarium 4x4 RRot Precession matrix based on espA-ChiA-OmgA-PsiA parameterisation
		b[0]   = RRot[8];
		b[1]   = RRot[9];
		b[2]   = RRot[10];

		// Angular separation between (a) EQR of ERFA and (b) EQR of Stellarium's ChiA-OmgA-PsiA-eps0 parameterisations
		// This result corresponds to the blue curve of Figure 12 of Vondrak & al (2011) paper.
		Angle1 = eraSepp(a, b) * ERFA_DR2AS;

		/*************************************************************************************************/
		/***************************** Save all Results in Output1.txt file  *****************************/
		/*                       file is stored here: stellarium\Testing\Temporary                       */
		/*************************************************************************************************/

		fprintf(fp1, "%0.5f   %0.5f   %0.6f     %0.6f     %0.15f\n", JulianDay, epj, T, Angle0, Angle1);
	}


	/*************************************************************************************************************/
	/* 11th : This section captures Vondrak & Owen Precession between -200 & +200 centuries from J2000.0         */
	/*************************************************************************************************************/
		
	FILE* fp2 = 0; // File pointer for output file2

	if (fopen_s(&fp2, "../../Testing/Temporary/output2.txt", "w+") != 0)
	{
		perror("Error opening file output2.txt");
		return;
	}

	fprintf(fp2, "Now Processing: Vondrak & Owen Precession from -200 to +200 centuries from J2000.0 @ 0.2 century "
	             "per step \n");
	fprintf(fp2, "    JD_TT"
	             "            EPJ"
	             "        centuries"
	             "     Sep Angle 0"
	             "       Sep Angle 1"
	             "           Sep Angle 5"
	             "            Sep Angle 7"
	             "            Sep Angle 9"
	             "            Sep Angle 11"
	             "          Sep Angle 13\n");

	const double DAYSTEP2 = 7305.0; // Step 0.2 Julian century of 7305.0 days

	// Compute common parameter angles used by the Owen(90) theory.
	// These angles are computed at T = 0 century from J2000.0
	// Note that Owen(90) eps0 is different from Vondrak (2011) eps0
	// Vondrak eps0   = 84381.406 arcsecond
	// Owen eps0_owen = 84381.448 arcsecond
	// Owen theory still uses the 1980 value while Vondrak uses the modern value
	L0_rad = getPrecessionAnglesOwenL0();
	I0_rad = getPrecessionAnglesOwenI0();
	eps0_owen = getPrecessionAngleOwenEpsilon(ERFA_DJ00);

	for (int i = -1000; i <= 1000; i++)
	{
		// Compute the Julian Day, Julian Epoch and Time in Julian century
		JulianDay = ERFA_DJ00 + i * DAYSTEP2;
		epj       = 2000.0 + (JulianDay - ERFA_DJ00) / ERFA_DJY;
		T         = (JulianDay - ERFA_DJ00) / ERFA_DJC;

		/*********************************************************************************************************/
		/*   Build Pecession matrix P using ERFA software which is equivalent to Vondrak PQXYe parameterisaton   */
		/*********************************************************************************************************/

		// Call ERFA routines to compute the ecliptic and equator poles and the Precession matrix P
		// ERFA routines implement the Vondrak et al. (2011) PQXYe parameterisation.
		// We will use it as our baseline parameterisation to validate Stellarium PQXYe method.
		// We will use it also tp compute the angular separation with Stellarium ChiA-OmgA-PsiA-esp0 
		// parameterisation based on Vondrak et al (2011) as well as the new parameterisations using
		// Owen(90) angles L, I, Delta of the invariable plane and Owen(90) Classic angles which are 
		// the ChiA-OmgA-PsiA-espA angles first introduced by Lieske et al (1977)
		eraLtpecl(epj, pecl_era);
		eraLtpequ(epj, pequ_era);
		eraLtp(epj, rp_era);

		/*********************************************************************************************************/
		/*              Build Pecession matrix P using Stellarium's Vondrak PQXYe parameterisation                */
		/*********************************************************************************************************/

		// Get the Precession parameters pA, qA, xA, yA and epsA for the given Julian Date in TT using
		// Stellarium's PQXYe parameterisation of Vondrák et al. (2011) precession model
		getPrecessionAnglesVondrakPQXYe(JulianDay, &P_A, &Q_A, &X_A, &Y_A, &epsilon_A);

		// Compute the ecliptic pole vector from Stellarium's PQXYe
		Z    = sqrt(qMax(1.0 - P_A * P_A - Q_A * Q_A, 0.0));
		VEC2 = -Q_A * C - Z * S;
		VEC3 = -Q_A * S + Z * C;
		Vec3d PECL(P_A, VEC2, VEC3);

		// Compute the equator pole vector from Stellarium's PQXYe
		W    = X_A * X_A + Y_A * Y_A;
		VEQ3 = (W < 1.0 ? sqrt(1.0 - W) : 0.0);
		Vec3d PEQR(X_A, Y_A, VEQ3);

		/***** Build the Precession matrix P using Stellarium's PQXYe method ****/
		Vec3d EQX = PEQR ^ PECL;
		EQX.normalize();
		Vec3d V = PEQR ^ EQX;
		Mat3d RP(EQX[0], EQX[1], EQX[2], V[0], V[1], V[2], PEQR[0], PEQR[1], PEQR[2]);

		/*********************************************************************************************************/
		/*       Build Pecession matrix P using Stellarium's Vondrak espA-ChiA-OmgA-PsiA parameterisation         */
		/*********************************************************************************************************/

		// Get the Precession parameters EpsilonA, ChiA, OmegaA and PsiA for the given Julian Date in TT using
		// Stellarium's espA-ChiA-OmgA-PsiA parameterisation of Vondrák et al. (2011) precession model
		// We have to call this twice with different dates to avoid returning the cached angles.
		// However we must pick a Julian Day that is within the allowed range of +/- 2000 centuries around J2000
		// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
		// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
		// of JulianDay we use the maximum Julian Day minus 2 days.
		getPrecessionAnglesVondrak(ERFA_DJ00 + 2000 * ERFA_DJC - 2, &epsilon_A, &chi_A, &omega_A, &psi_A);
		getPrecessionAnglesVondrak(JulianDay, &epsilon_A, &chi_A, &omega_A, &psi_A);

		// Build the Precession matrix P using Stellarium's espA-ChiA-OmgA-PsiA method
		Mat4d RRot = Mat4d::xrotation(eps0) * Mat4d::zrotation(-psi_A) * Mat4d::xrotation(-omega_A) * Mat4d::zrotation(chi_A);
		
		                    /**** Angular separation between Equator pole vectors ****/

		// Equator pole vector from ERFA 3x3 rp_era Precession matrix based on PQXYe parameterisation
		a[0] = rp_era[2][0];
		a[1] = rp_era[2][1];
		a[2] = rp_era[2][2];

		// Equator pole vector from Stellarium 3x3 RP Precession matrix based on PQXYe parameterisation
		b[0] = RP[6];
		b[1] = RP[7];
		b[2] = RP[8];

		// Angular separation between (a) EQR of ERFA and (b) EQR of Stellarium's PQXYe parameterisations
		// There should be almost no angular separation between these two EQR poles. A verification is done at nano-arcsecond level
		Angle0 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond
		QVERIFY2(Angle0 <= 1e-9, QString("Angular separation between ERFA and Stellarium's PQXYe is greater "
										 "than 1.0E-9 arcsecond --> error!").toUtf8());

		// Equator pole vector from Stellarium 4x4 RRot Precession matrix based on espA-ChiA-OmgA-PsiA parameterisation
		b[0] = RRot[8];
		b[1] = RRot[9];
		b[2] = RRot[10];

		// Angular separation between (a) EQR of ERFA and (b) EQR of Stellarium's ChiA-OmgA-PsiA-eps0 parameterisations
		// This result corresponds to the blue curve of Figure 12 of Vondrak & al (2011) paper.
		Angle1 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond
	
		/*********************************************************************************************************/
		/*           Build Pecession matrix P using Owen long-term theory L, I, Delta, I0, L0 angles             */
		/*********************************************************************************************************/

		// We have to call this twice with different dates to avoid returning the cached angles.
		// However we must pick a Julian Day that is within the allowed range of +/- 200 centuries around J2000
		// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
		// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
		// of JulianDay we use the maximum Julian Day minus 2 days.
		getPrecessionAnglesOwenLIDelta(ERFA_DJ00 + 200 * ERFA_DJC - 2, &L_rad, &I_rad, &Delta_rad);
		getPrecessionAnglesOwenLIDelta(JulianDay, &L_rad, &I_rad, &Delta_rad);

		Mat4d RP_LID = Mat4d::zrotation(L0_rad) * Mat4d::xrotation(I0_rad) * Mat4d::zrotation(-Delta_rad) * Mat4d::xrotation(-I_rad) * Mat4d::zrotation(-L_rad);

		                /**** Angular separation between Equator pole vectors ****/

		// Equator pole vector from ERFA 3x3 rp_era Precession matrix based on PQXYe parameterisation
		a[0] = rp_era[2][0];
		a[1] = rp_era[2][1];
		a[2] = rp_era[2][2];

		// Equator pole vector from Owen(90) Precession matrix based on L, I, Delta parameterisation
		b[0] = RP_LID[8];
		b[1] = RP_LID[9];
		b[2] = RP_LID[10];

		// Angular separation between (a) EQR of ERFA and (b) EQR of Owen(90) L, I, Delta, I0, L0 parameterisations
		Angle5 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond
	
		// Equator pole vector from Stellarium 4x4 RRot Precession matrix based on espA-ChiA-OmgA-PsiA parameterisation
		a[0] = RRot[8];
		a[1] = RRot[9];
		a[2] = RRot[10];

		// Angular separation between (a) EQR of Stellarium's ChiA-OmgA-PsiA-eps0 and (b) EQR of Owen(90) L, I, Delta, I0, L0
		Angle7 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond
		
		/*********************************************************************************************************/
		/*              Build the Pecession matrix P using Owen long-term theory classical angles                */
		/*********************************************************************************************************/

		// We have to call this twice with different dates to avoid returning the cached angles.
		// However we must pick a Julian Day that is within the allowed range of +/- 200 centuries around J2000
		// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
		// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
		// of JulianDay we use the maximum Julian Day minus 2 days.
		getPrecessionAnglesOwenClassic(ERFA_DJ00 + 200 * ERFA_DJC - 2, &epsilon_A, &chi_A, &omega_A, &psi_A);
		getPrecessionAnglesOwenClassic(JulianDay, &epsilon_A, &chi_A, &omega_A, &psi_A);

		Mat4d RP_Clas = Mat4d::xrotation(eps0_owen) * Mat4d::zrotation(-psi_A) * Mat4d::xrotation(-omega_A) * Mat4d::zrotation(chi_A);

		                   /**** Angular separation between Equator pole vectors ****/

		// Equator pole vector from ERFA 3x3 rp_era Precession matrix based on PQXYe parameterisation
		a[0] = rp_era[2][0];
		a[1] = rp_era[2][1];
		a[2] = rp_era[2][2];

		// Equator pole vector from Owen(90) Precession matrix based on Classic angles espA-ChiA-OmgA-PsiA parameterisation
		b[0] = RP_Clas[8];
		b[1] = RP_Clas[9];
		b[2] = RP_Clas[10];

		// Angular separation between (a) EQR of ERFA and (b) EQR of Owen(90) classic ChiA-OmgA-PsiA-eps0 parameterisation
		Angle9 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond
	
		// Equator pole vector from Stellarium 4x4 RRot Precession matrix based on espA-ChiA-OmgA-PsiA parameterisation
		a[0] = RRot[8];
		a[1] = RRot[9];
		a[2] = RRot[10];

		// Angular separation between (a) EQR of Stellarium's ChiA-OmgA-PsiA-eps0 and (b)  EQR of Owen(90) classic ChiA-OmgA-PsiA-eps0 parameterisations
		Angle11 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond
		
		// Equator pole vector from Owen(90) Precession matrix based on L, I, Delta parameterisation
		a[0] = RP_LID[8];
		a[1] = RP_LID[9];
		a[2] = RP_LID[10];

		// Angular separation between (a) EQR of Owen(90) L, I, Delta, I0, L0 and (b) EQR of Owen(90) classic ChiA-OmgA-PsiA-eps0 parameterisations
		Angle13 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond
	
		/*********************************************************************************************************/
		/********************************** Save all Results in Output2.txt file  ********************************/
		/*                          file is stored here: stellarium\Testing\Temporary                            */
		/*********************************************************************************************************/
	
		fprintf(fp2,
		        "%0.5f   %0.5f   %0.6f     %0.6f     %0.15f     %0.15f     %0.15f     %0.15f     %0.15f     "
		        "%0.15f\n",
		        JulianDay, epj, T, Angle0, Angle1, Angle5, Angle7, Angle9, Angle11, Angle13);
	}
	
	/*************************************************************************************************************/
	/* 12th : This section captures Vondrak & Owen Precession between -10 & +10 centuries from J2000.0          */
	/*************************************************************************************************************/

	FILE* fp3 = 0; // File pointer for output file2

	if (fopen_s(&fp3, "../../Testing/Temporary/output3.txt", "w+") != 0)
	{
		perror("Error opening file output3.txt");
		return;
	}

	fprintf(fp3, "Now Processing: Vondrak & Owen Precession from -10 to +10 centuries from J2000.0 @ 0.01 century "
	             "per step \n");
	fprintf(fp3, "    JD_TT"
	             "            EPJ"
	             "        centuries"
	             "     Sep Angle 0"
	             "       Sep Angle 1"
	             "           Sep Angle 5"
	             "            Sep Angle 7"
	             "            Sep Angle 9"
	             "            Sep Angle 11"
	             "          Sep Angle 13\n");

	const double DAYSTEP3 = 365.25; // Step 0.01 Julian century of 365.25 days

	// Compute common parameter angles used by the Owen(90) theory.
	// These angles are computed at T = 0 century from J2000.0
	// Note that Owen(90) eps0 is different from Vondrak (2011) eps0
	// Vondrak eps0   = 84381.406 arcsecond
	// Owen eps0_owen = 84381.448 arcsecond
	// Owen theory still uses the 1980 value while Vondrak uses the modern value
	L0_rad    = getPrecessionAnglesOwenL0();
	I0_rad    = getPrecessionAnglesOwenI0();
	eps0_owen = getPrecessionAngleOwenEpsilon(ERFA_DJ00);

	for (int i = -1000; i <= 1000; i++)
	{
		// Compute the Julian Day, Julian Epoch and Time in Julian century
		JulianDay = ERFA_DJ00 + i * DAYSTEP3;
		epj       = 2000.0 + (JulianDay - ERFA_DJ00) / ERFA_DJY;
		T         = (JulianDay - ERFA_DJ00) / ERFA_DJC;

		// Call ERFA routines to compute the ecliptic and equator poles and the Precession matrix P
		// ERFA routines implement the Vondrak et al. (2011) PQXYe parameterisation.
		// We will use it as our baseline parameterisation to validate Stellarium PQXYe method.
		// We will use it also tp compute the angular separation with Stellarium ChiA-OmgA-PsiA-esp0
		// parameterisation based on Vondrak et al (2011) as well as the new parameterisations using
		// Owen(90) angles L, I, Delta of the invariable plane and Owen(90) Classic angles which are
		// the ChiA-OmgA-PsiA-espA angles first introduced by Lieske et al (1977)
		eraLtpecl(epj, pecl_era);
		eraLtpequ(epj, pequ_era);
		eraLtp(epj, rp_era);

		// Get the Precession parameters pA, qA, xA, yA and epsA for the given Julian Date in TT using
		// Stellarium's PQXYe parameterisation of Vondrák et al. (2011) precession model
		getPrecessionAnglesVondrakPQXYe(JulianDay, &P_A, &Q_A, &X_A, &Y_A, &epsilon_A);

		// Compute the ecliptic pole vector from Stellarium's PQXYe
		Z    = sqrt(qMax(1.0 - P_A * P_A - Q_A * Q_A, 0.0));
		VEC2 = -Q_A * C - Z * S;
		VEC3 = -Q_A * S + Z * C;
		Vec3d PECL(P_A, VEC2, VEC3);

		// Compute the equator pole vector from Stellarium's PQXYe
		W    = X_A * X_A + Y_A * Y_A;
		VEQ3 = (W < 1.0 ? sqrt(1.0 - W) : 0.0);
		Vec3d PEQR(X_A, Y_A, VEQ3);

		/***** Build the Precession matrix P using Stellarium's PQXYe method ****/
		Vec3d EQX = PEQR ^ PECL;
		EQX.normalize();
		Vec3d V = PEQR ^ EQX;
		Mat3d RP(EQX[0], EQX[1], EQX[2], V[0], V[1], V[2], PEQR[0], PEQR[1], PEQR[2]);

		// Get the Precession parameters EpsilonA, ChiA, OmegaA and PsiA for the given Julian Date in TT using
		// Stellarium's espA-ChiA-OmgA-PsiA parameterisation of Vondrák et al. (2011) precession model
		// We have to call this twice with different dates to avoid returning the cached angles.
		// However we must pick a Julian Day that is within the allowed range of +/- 2000 centuries around J2000
		// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
		// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
		// of JulianDay we use the maximum Julian Day minus 2 days.
		getPrecessionAnglesVondrak(ERFA_DJ00 + 2000 * ERFA_DJC - 2, &epsilon_A, &chi_A, &omega_A, &psi_A);
		getPrecessionAnglesVondrak(JulianDay, &epsilon_A, &chi_A, &omega_A, &psi_A);

		// Build the Precession matrix P using Stellarium's espA-ChiA-OmgA-PsiA method
		Mat4d RRot = Mat4d::xrotation(eps0) * Mat4d::zrotation(-psi_A) * Mat4d::xrotation(-omega_A) *
		             Mat4d::zrotation(chi_A);

		/**** Angular separation between Equator pole vectors ****/

		// Equator pole vector from ERFA 3x3 rp_era Precession matrix based on PQXYe parameterisation
		a[0] = rp_era[2][0];
		a[1] = rp_era[2][1];
		a[2] = rp_era[2][2];

		// Equator pole vector from Stellarium 3x3 RP Precession matrix based on PQXYe parameterisation	
		b[0] = RP[6];
		b[1] = RP[7];
		b[2] = RP[8];

		// Angular separation between (a) EQR of ERFA and (b) EQR of Stellarium's PQXYe parameterisations
		// There should be almost no angular separation between these two EQR poles. A verification is done at nano-arcsecond level
		Angle0 = eraSepp(a, b) * ERFA_DR2AS;
		QVERIFY2(Angle0 <= 1e-9, QString("Angular separation between ERFA and Stellarium's PQXYe is greater "
		                                 "than 1.0E-9 arcsecond --> error!")
		                                 .toUtf8());

		// Equator pole vector from Stellarium 4x4 RRot Precession matrix based on espA-ChiA-OmgA-PsiA parameterisation
		b[0] = RRot[8];
		b[1] = RRot[9];
		b[2] = RRot[10];

		// Angular separation between (a) EQR of ERFA and (b) EQR of Stellarium's ChiA-OmgA-PsiA-eps0 parameterisations
		// This result corresponds to the blue curve of Figure 12 of Vondrak & al (2011) paper.
		Angle1 = eraSepp(a, b) * ERFA_DR2AS;

		/*********************************************************************************************************/
		/*           Build Pecession matrix P using Owen long-term theory L, I, Delta, I0, L0 angles             */
		/*********************************************************************************************************/

		// We have to call this twice with different dates to avoid returning the cached angles.
		// However we must pick a Julian Day that is within the allowed range of +/- 200 centuries around J2000
		// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
		// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
		// of JulianDay we use the maximum Julian Day minus 2 days.
		getPrecessionAnglesOwenLIDelta(ERFA_DJ00 + 200 * ERFA_DJC - 2, &L_rad, &I_rad, &Delta_rad);
		getPrecessionAnglesOwenLIDelta(JulianDay, &L_rad, &I_rad, &Delta_rad);

		Mat4d RP_LID = Mat4d::zrotation(L0_rad) * Mat4d::xrotation(I0_rad) * Mat4d::zrotation(-Delta_rad) *
		               Mat4d::xrotation(-I_rad) * Mat4d::zrotation(-L_rad);

		/**** Angular separation between Equator pole vectors ****/

		// Equator pole vector from ERFA 3x3 rp_era Precession matrix based on PQXYe parameterisation
		a[0] = rp_era[2][0];
		a[1] = rp_era[2][1];
		a[2] = rp_era[2][2];

		// Equator pole vector from Owen(90) Precession matrix based on L, I, Delta parameterisation
		b[0] = RP_LID[8];
		b[1] = RP_LID[9];
		b[2] = RP_LID[10];

		// Angular separation between (a) EQR of ERFA and (b) EQR of Owen(90) L, I, Delta, I0, L0 parameterisations
		Angle5 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond

		// Equator pole vector from Stellarium 4x4 RRot Precession matrix based on espA-ChiA-OmgA-PsiA parameterisation
		a[0] = RRot[8];
		a[1] = RRot[9];
		a[2] = RRot[10];

		// Angular separation between (a) EQR of Stellarium's ChiA-OmgA-PsiA-eps0 and (b) EQR of Owen(90) L, I, Delta, I0, L0
		Angle7 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond

		/*********************************************************************************************************/
		/*              Build the Pecession matrix P using Owen long-term theory classical angles                */
		/*********************************************************************************************************/

		// We have to call this twice with different dates to avoid returning the cached angles.
		// However we must pick a Julian Day that is within the allowed range of +/- 200 centuries around J2000
		// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
		// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
		// of JulianDay we use the maximum Julian Day minus 2 days.
		getPrecessionAnglesOwenClassic(ERFA_DJ00 + 200 * ERFA_DJC - 2, &epsilon_A, &chi_A, &omega_A, &psi_A);
		getPrecessionAnglesOwenClassic(JulianDay, &epsilon_A, &chi_A, &omega_A, &psi_A);

		Mat4d RP_Clas = Mat4d::xrotation(eps0_owen) * Mat4d::zrotation(-psi_A) * Mat4d::xrotation(-omega_A) *
		                Mat4d::zrotation(chi_A);

		/**** Angular separation between Equator pole vectors ****/

		// Equator pole vector from ERFA 3x3 rp_era Precession matrix based on PQXYe parameterisation
		a[0] = rp_era[2][0];
		a[1] = rp_era[2][1];
		a[2] = rp_era[2][2];

		// Equator pole vector from Owen(90) Precession matrix based on Classic angles espA-ChiA-OmgA-PsiA parameterisation
		b[0] = RP_Clas[8];
		b[1] = RP_Clas[9];
		b[2] = RP_Clas[10];

		// Angular separation between (a) EQR of ERFA and (b) EQR of Owen(90) classic ChiA-OmgA-PsiA-eps0 parameterisation
		Angle9 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond

		// Equator pole vector from Stellarium 4x4 RRot Precession matrix based on espA-ChiA-OmgA-PsiA parameterization
		a[0] = RRot[8];
		a[1] = RRot[9];
		a[2] = RRot[10];

		// Angular separation between (a) EQR of Stellarium's ChiA-OmgA-PsiA-eps0 and (b)  EQR of Owen(90) classic ChiA-OmgA-PsiA-eps0 parameterisations
		Angle11 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond

		// Equator pole vector from Owen(90) Precession matrix based on L, I, Delta parameterisation
		a[0] = RP_LID[8];
		a[1] = RP_LID[9];
		a[2] = RP_LID[10];

		// Angular separation between (a) EQR of Owen(90) L, I, Delta, I0, L0 and (b) EQR of Owen(90) classic ChiA-OmgA-PsiA-eps0 parameterisations
		Angle13 = eraSepp(a, b) * ERFA_DR2AS; // in arcsecond

		/*********************************************************************************************************/
		/********************************** Save all Results in Output3.txt file  ********************************/
		/*                          file is stored here: stellarium\Testing\Temporary                            */
		/*********************************************************************************************************/
	
		fprintf(fp3,
		        "%0.5f   %0.5f   %0.6f     %0.6f     %0.15f     %0.15f     %0.15f     %0.15f     %0.15f     "
		        "%0.15f\n",
		        JulianDay, epj, T, Angle0, Angle1, Angle5, Angle7, Angle9, Angle11, Angle13);
	}

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
}
