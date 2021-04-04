/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2021 Georg Zotti
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

#include "StelCore.hpp"
#include "Planet.hpp"
#include "StelUtils.hpp"

#include <limits>
#include <QString>
#include <QDebug>

// Also include the GRS corrections here
bool RotationElements::flagCustomGrsSettings = false;
double RotationElements::customGrsJD = 2456901.5;
double RotationElements::customGrsLongitude = 216.;
double RotationElements::customGrsDrift = 15.;

RotationElements::PlanetCorrections RotationElements::planetCorrections;


void RotationElements::updatePlanetCorrections(const double JDE, const PlanetCorrection planet)
{
	// The angles are always given in degrees. We let the compiler do the conversion. Leave it for readability!
	const double d=(JDE-J2000);
	const double T=d/36525.0;

	switch (planet){
		case EarthMoon:
			if (fabs(JDE-planetCorrections.JDE_E)>StelCore::JD_MINUTE)
			{ // Moon/Earth correction terms. This is from WGCCRE2009. We must fmod them, else we have sin(LARGE)>>1
				planetCorrections.JDE_E=JDE; // keep record of when these values are valid.
				planetCorrections.E1= M_PI_180* (125.045 - remainder( 0.0529921*d, 360.0));
				planetCorrections.E2= M_PI_180* (250.089 - remainder( 0.1059842*d, 360.0));
				planetCorrections.E3= M_PI_180* (260.008 + remainder(13.0120009*d, 360.0));
				planetCorrections.E4= M_PI_180* (176.625 + remainder(13.3407154*d, 360.0));
				planetCorrections.E5= M_PI_180* (357.529 + remainder( 0.9856003*d, 360.0));
				planetCorrections.E6= M_PI_180* (311.589 + remainder(26.4057084*d, 360.0));
				planetCorrections.E7= M_PI_180* (134.963 + remainder(13.0649930*d, 360.0));
				planetCorrections.E8= M_PI_180* (276.617 + remainder( 0.3287146*d, 360.0));
				planetCorrections.E9= M_PI_180* ( 34.226 + remainder( 1.7484877*d, 360.0));
				planetCorrections.E10=M_PI_180* ( 15.134 - remainder( 0.1589763*d, 360.0));
				planetCorrections.E11=M_PI_180* (119.743 + remainder( 0.0036096*d, 360.0));
				planetCorrections.E12=M_PI_180* (239.961 + remainder( 0.1643573*d, 360.0));
				planetCorrections.E13=M_PI_180* ( 25.053 + remainder(12.9590088*d, 360.0));
			}
			break;
		case Mars:
			if (fabs(JDE-planetCorrections.JDE_E)>5*StelCore::JD_MINUTE)
			{ // Mars correction terms. This is from WGCCRE2015. We must fmod them, else we have sin(LARGE)>>1
				planetCorrections.JDE_M=JDE; // keep record of when these values are valid.
				planetCorrections.M1= M_PI_180* (190.72646643 + remainder(   15917.10818695*T, 360.0));
				planetCorrections.M2= M_PI_180* ( 21.46892470 + remainder(   31834.27934054*T, 360.0));
				planetCorrections.M3= M_PI_180* (332.86082793 + remainder(   19139.89694742*T, 360.0));
				planetCorrections.M4= M_PI_180* (394.93256437 + remainder(   38280.79631835*T, 360.0));
				planetCorrections.M5= M_PI_180* (189.63271560 + remainder(41215158.18420050*T, 360.0) + remainder(12.71192322*T*T, 360.0));
				planetCorrections.M6= M_PI_180* (121.46893664 + remainder(     660.22803474*T, 360.0));
				planetCorrections.M7= M_PI_180* (231.05028581 + remainder(     660.99123540*T, 360.0));
				planetCorrections.M8= M_PI_180* (251.37314025 + remainder(    1320.50145245*T, 360.0));
				planetCorrections.M9= M_PI_180* (217.98635955 + remainder(   38279.96125550*T, 360.0));
				planetCorrections.M10=M_PI_180* (196.19729402 + remainder(   19139.83628608*T, 360.0));
			}
			break;
		case Jupiter:
			if (fabs(JDE-planetCorrections.JDE_J)>0.025) // large changes in the values below :-(
			{
				planetCorrections.JDE_J=JDE; // keep record of when these values are valid.
				planetCorrections.Ja =M_PI_180* ( 99.360714+remainder( 4850.4046*T, 360.0)); // Jupiter axis terms, Table 10.1
				planetCorrections.Jb =M_PI_180* (175.895369+remainder( 1191.9605*T, 360.0));
				planetCorrections.Jc =M_PI_180* (300.323162+remainder(  262.5475*T, 360.0));
				planetCorrections.Jd =M_PI_180* (114.012305+remainder( 6070.2476*T, 360.0));
				planetCorrections.Je =M_PI_180* ( 49.511251+remainder(   64.3000*T, 360.0));
				planetCorrections.J1 =M_PI_180* ( 73.32    +remainder(91472.9   *T, 360.0)); // corrective terms for Jupiter' moons, Table 10.10
				planetCorrections.J2 =M_PI_180* ( 24.62    +remainder(45137.2   *T, 360.0));
				planetCorrections.J3 =M_PI_180* (283.90    +remainder( 4850.7   *T, 360.0));
				planetCorrections.J4 =M_PI_180* (355.80    +remainder( 1191.3   *T, 360.0));
				planetCorrections.J5 =M_PI_180* (119.90    +remainder(  262.1   *T, 360.0));
				planetCorrections.J6 =M_PI_180* (229.80    +remainder(   64.3   *T, 360.0));
				planetCorrections.J7 =M_PI_180* (352.25    +remainder( 2382.6   *T, 360.0));
				planetCorrections.J8 =M_PI_180* (113.35    +remainder( 6070.0   *T, 360.0));
			}
			break;
		case Saturn:
			if (fabs(JDE-planetCorrections.JDE_S)>0.025) // large changes in the values below :-(
			{
				planetCorrections.JDE_S=JDE; // keep record of when these values are valid.
				planetCorrections.S1=M_PI_180* (353.32+remainder( 75706.7*T, 360.0)); // corrective terms for Saturn's moons, Table 10.12
				planetCorrections.S2=M_PI_180* ( 28.72+remainder( 75706.7*T, 360.0));
				planetCorrections.S3=M_PI_180* (177.40+remainder(-36505.5*T, 360.0));
				planetCorrections.S4=M_PI_180* (300.00+remainder( -7225.9*T, 360.0));
				planetCorrections.S5=M_PI_180* (316.45+remainder(   506.2*T, 360.0));
				planetCorrections.S6=M_PI_180* (345.20+remainder( -1016.3*T, 360.0));
			}
			break;
		case Uranus:
			if (fabs(JDE-planetCorrections.JDE_U)>0.025) // large changes in the values below :-(
			{
				planetCorrections.JDE_U=JDE; // keep record of when these values are valid.
				planetCorrections.U1 =M_PI_180* (115.75+remainder(54991.87*T, 360.0)); // corrective terms for Uranus's moons, Table 10.14.
				planetCorrections.U2 =M_PI_180* (141.69+remainder(41887.66*T, 360.0));
				//planetCorrections.U3 =M_PI_180* (135.03+remainder(29927.35*T, 360.0)); // not in 0.20
				planetCorrections.U4 =M_PI_180* ( 61.77+remainder(25733.59*T, 360.0));
				planetCorrections.U5 =M_PI_180* (249.32+remainder(24471.46*T, 360.0));
				planetCorrections.U6 =M_PI_180* ( 43.86+remainder(22278.41*T, 360.0));
				//planetCorrections.U7 =M_PI_180* ( 77.66+remainder(20289.42*T, 360.0)); // not in 0.20
				//planetCorrections.U8 =M_PI_180* (157.36+remainder(16652.76*T, 360.0)); // not in 0.20
				//planetCorrections.U9 =M_PI_180* (101.81+remainder(12872.63*T, 360.0)); // not in 0.20
				planetCorrections.U10=M_PI_180* (138.64+remainder( 8061.81*T, 360.0));
				planetCorrections.U11=M_PI_180* (102.23+remainder(-2024.22*T, 360.0));
				planetCorrections.U12=M_PI_180* (316.41+remainder( 2863.96*T, 360.0));
				planetCorrections.U13=M_PI_180* (304.01+remainder(  -51.94*T, 360.0));
				planetCorrections.U14=M_PI_180* (308.71+remainder(  -93.17*T, 360.0));
				planetCorrections.U15=M_PI_180* (340.82+remainder(  -75.32*T, 360.0));
				planetCorrections.U16=M_PI_180* (259.14+remainder( -504.81*T, 360.0));
			}
			break;
		case Neptune:
			if (fabs(JDE-planetCorrections.JDE_N)>0.025) // large changes in the values below :-(
			{
				planetCorrections.JDE_N=JDE; // keep record of when these values are valid.
				planetCorrections.Na=M_PI_180* (357.85+remainder(   52.316*T, 360.0)); // Neptune axis term
				planetCorrections.N1=M_PI_180* (323.92+remainder(62606.6*T, 360.0)); // corrective terms for Neptune's moons, Table 10.15 (N=Na!)
				planetCorrections.N2=M_PI_180* (220.51+remainder(55064.2*T, 360.0));
				planetCorrections.N3=M_PI_180* (354.27+remainder(46564.5*T, 360.0));
				planetCorrections.N4=M_PI_180* ( 75.31+remainder(26109.4*T, 360.0));
				planetCorrections.N5=M_PI_180* ( 35.36+remainder(14325.4*T, 360.0));
				planetCorrections.N6=M_PI_180* (142.61+remainder( 2824.6*T, 360.0));
				planetCorrections.N7=M_PI_180* (177.85+remainder(   52.316*T, 360.0));
			}
			break;
	}
}

const QMap<QString, RotationElements::axisRotFuncType>RotationElements::axisRotCorrFuncMap={
	{"Moon",       &RotationElements::corrWMoon},
	{"Mercury",    &RotationElements::corrWMercury},
	{"Mars",       &RotationElements::corrWMars},
	{"Jupiter",    &RotationElements::corrWJupiter},
	{"Neptune",    &RotationElements::corrWNeptune},
	{"Phobos",     &RotationElements::corrWPhobos},
	{"Deimos",     &RotationElements::corrWDeimos},
	{"Io",         &RotationElements::corrWIo},
	{"Europa",     &RotationElements::corrWEuropa},
	{"Ganymede",   &RotationElements::corrWGanymede},
	{"Callisto",   &RotationElements::corrWCallisto},
	{"Amalthea",   &RotationElements::corrWAmalthea},
	{"Thebe",      &RotationElements::corrWThebe},
	{"Mimas",      &RotationElements::corrWMimas},
	{"Tethys",     &RotationElements::corrWTethys},
	{"Rhea",       &RotationElements::corrWRhea},
	{"Janus",      &RotationElements::corrWJanus},
	{"Epimetheus", &RotationElements::corrWEpimetheus},
	{"Cordelia",   &RotationElements::corrWCordelia},
	{"Ophelia",    &RotationElements::corrWOphelia},
	{"Bianca",     &RotationElements::corrWBianca},
	{"Cressida",   &RotationElements::corrWCressida},
	{"Desdemona",  &RotationElements::corrWDesdemona},
	{"Juliet",     &RotationElements::corrWJuliet},
	{"Portia",     &RotationElements::corrWPortia},
	{"Rosalind",   &RotationElements::corrWRosalind},
	{"Belinda",    &RotationElements::corrWBelinda},
	{"Puck",       &RotationElements::corrWPuck},
	{"Ariel",      &RotationElements::corrWAriel},
	{"Umbriel",    &RotationElements::corrWUmbriel},
	{"Titania",    &RotationElements::corrWTitania},
	{"Oberon",     &RotationElements::corrWOberon},
	{"Miranda",    &RotationElements::corrWMiranda},
	{"Triton",     &RotationElements::corrWTriton},
	{"Naiad",      &RotationElements::corrWNaiad},
	{"Thalassa",   &RotationElements::corrWThalassa},
	{"Despina",    &RotationElements::corrWDespina},
	{"Galatea",    &RotationElements::corrWGalatea},
	{"Larissa",    &RotationElements::corrWLarissa},
	{"Proteus",    &RotationElements::corrWProteus}
};

const QMap<QString, RotationElements::axisOriFuncType>RotationElements::axisOriCorrFuncMap={
	{"Moon",       &RotationElements::corrOriMoon},
	{"Mars",       &RotationElements::corrOriMars},
	{"Jupiter",    &RotationElements::corrOriJupiter},
	{"Neptune",    &RotationElements::corrOriNeptune},
	{"Phobos",     &RotationElements::corrOriPhobos},
	{"Deimos",     &RotationElements::corrOriDeimos},
	{"Io",         &RotationElements::corrOriIo},
	{"Europa",     &RotationElements::corrOriEuropa},
	{"Ganymede",   &RotationElements::corrOriGanymede},
	{"Callisto",   &RotationElements::corrOriCallisto},
	{"Amalthea",   &RotationElements::corrOriAmalthea},
	{"Thebe",      &RotationElements::corrOriThebe},
	{"Mimas",      &RotationElements::corrOriMimas},
	{"Tethys",     &RotationElements::corrOriTethys},
	{"Rhea",       &RotationElements::corrOriRhea},
	{"Janus",      &RotationElements::corrOriJanus},
	{"Epimetheus", &RotationElements::corrOriEpimetheus},
	{"Ariel",      &RotationElements::corrOriAriel},
	{"Umbriel",    &RotationElements::corrOriUmbriel},
	{"Titania",    &RotationElements::corrOriTitania},
	{"Oberon",     &RotationElements::corrOriOberon},
	{"Miranda",    &RotationElements::corrOriMiranda},
	{"Cordelia",   &RotationElements::corrOriCordelia},
	{"Ophelia",    &RotationElements::corrOriOphelia},
	{"Bianca",     &RotationElements::corrOriBianca},
	{"Cressida",   &RotationElements::corrOriCressida},
	{"Desdemona",  &RotationElements::corrOriDesdemona},
	{"Juliet",     &RotationElements::corrOriJuliet},
	{"Portia",     &RotationElements::corrOriPortia},
	{"Rosalind",   &RotationElements::corrOriRosalind},
	{"Belinda",    &RotationElements::corrOriBelinda},
	{"Puck",       &RotationElements::corrOriPuck},
	{"Triton",     &RotationElements::corrOriTriton},
	{"Naiad",      &RotationElements::corrOriNaiad},
	{"Thalassa",   &RotationElements::corrOriThalassa},
	{"Despina",    &RotationElements::corrOriDespina},
	{"Galatea",    &RotationElements::corrOriGalatea},
	{"Larissa",    &RotationElements::corrOriLarissa},
	{"Proteus",    &RotationElements::corrOriProteus}
};



double RotationElements::corrWMoon(const double d, const double T)
{
	Q_UNUSED(T)
	return -1.4e-12*d*d                        + 3.5610*sin(planetCorrections.E1)
		+0.1208*sin(planetCorrections.E2)  - 0.0642*sin(planetCorrections.E3)  +  0.0158*sin(planetCorrections.E4)
		+0.0252*sin(planetCorrections.E5)  - 0.0066*sin(planetCorrections.E6)  -  0.0047*sin(planetCorrections.E7)
		-0.0046*sin(planetCorrections.E8)  + 0.0028*sin(planetCorrections.E9)  +  0.0052*sin(planetCorrections.E10)
		+0.0040*sin(planetCorrections.E11) + 0.0019*sin(planetCorrections.E12) -  0.0044*sin(planetCorrections.E13);
}

double RotationElements::corrWMercury(const double d, const double T)
{
	Q_UNUSED(T)
	const double M1=(174.7910857*M_PI_180) + remainder(( 4.092335*M_PI_180)*d, 2.0*M_PI);
	const double M2=(349.5821714*M_PI_180) + remainder(( 8.184670*M_PI_180)*d, 2.0*M_PI);
	const double M3=(164.3732571*M_PI_180) + remainder((12.277005*M_PI_180)*d, 2.0*M_PI);
	const double M4=(339.1643429*M_PI_180) + remainder((16.369340*M_PI_180)*d, 2.0*M_PI);
	const double M5=(153.9554286*M_PI_180) + remainder((20.461675*M_PI_180)*d, 2.0*M_PI);

	return -0.00000571*sin(M5) - 0.00002539*sin(M4) - 0.00011040*sin(M3) - 0.00112309*sin(M2) + 0.01067257*sin(M1);
}

double RotationElements::corrWMars(const double d, const double T)
{
	Q_UNUSED(d)
	return
	+0.000145*sin(M_PI_180*(129.071773 + remainder(19140.0328244*T, 360.)))
	+0.000157*sin(M_PI_180*( 36.352167 + remainder(38281.0473591*T, 360.)))
	+0.000040*sin(M_PI_180*( 56.668646 + remainder(57420.9295360*T, 360.)))
	+0.000001*sin(M_PI_180*( 67.364003 + remainder(76560.2552215*T, 360.)))
	+0.000001*sin(M_PI_180*(104.792680 + remainder(95700.4387578*T, 360.)))
	+0.584542*sin(M_PI_180*( 95.391654 + remainder(    0.5042615*T, 360.)));
}

// The default W delivers SystemII longitude.
// We have to shift by GRS position and texture position.
// The final value will no longer be W but the rotation value required to show the GRS.
double RotationElements::corrWJupiter(const double d, const double T)
{
	Q_UNUSED(T)
	const double JDE=d+J2000;
	// Note that earth-bound computations of "Central Meridian, System II" do not apply here.
	// For comparison, see http://www.projectpluto.com/grs_form.htm
	// Instead we patch W_II.
	// https://skyandtelescope.org/observing/interactive-sky-watching-tools/transit-times-of-jupiters-great-red-spot/ writes:
	// These predictions assume the Red Spot was at Jovian System II longitude 349° in January 2021 and continues to drift 1.75° per month,
	// based on historical trends noted by JUPOS. If the GRS moves elsewhere, it will transit 1 2/3 minutes late for every 1° of longitude greater
	// than that used in this tool or 1 2/3 minutes early for every 1° less than the longitude in this tool.
	// GRS longitude was at 2014-09-08 216d with a drift of 1.25d every month
	// Updated 01/2021 noting LII=349 and drift 1.75 degrees/month.
	double longitudeGRS = (flagCustomGrsSettings ?
		customGrsLongitude + customGrsDrift*(JDE - customGrsJD)/365.25 :
		349+1.75*(JDE - 2459216.)/30.);
	return - longitudeGRS  +  ((187./512.)*360.-90.); // Last term is pixel position of GRS in texture, and a spherical coordinate offset for the texture.
	// To verify:
	// GRS at 2015-02-26 23:07 UT on picture at https://maximusphotography.files.wordpress.com/2015/03/jupiter-febr-26-2015.jpg
	//        2014-02-25 19:03 UT    http://www.damianpeach.com/jup1314/2014_02_25rgb0305.jpg
	//	  2013-05-01 10:29 UT    http://astro.christone.net/jupiter/jupiter2012/jupiter20130501.jpg
	//        2012-10-26 00:12 UT at http://www.lunar-captures.com//jupiter2012_files/121026_JupiterGRS_Tar.jpg
	//	  2011-08-28 02:29 UT at http://www.damianpeach.com/jup1112/2011_08_28rgb.jpg
}

double RotationElements::corrWNeptune(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return  -0.48 * sin(planetCorrections.Na);
}

double RotationElements::corrWPhobos(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return 12.72192797*T*T
	 +1.42421769*sin(planetCorrections.M1)
	 -0.02273783*sin(planetCorrections.M2)
	 +0.00410711*sin(planetCorrections.M3)
	 +0.00631964*sin(planetCorrections.M4)
	 -1.143     *sin(planetCorrections.M5);
}

double RotationElements::corrWDeimos(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return
	 -2.73954829*sin(planetCorrections.M6)
	 -0.39968606*sin(planetCorrections.M7)
	 -0.06563259*sin(planetCorrections.M8)
	 -0.02912940*sin(planetCorrections.M9)
	 +0.01699160*sin(planetCorrections.M10);
}

double RotationElements::corrWIo(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.085)*sin(planetCorrections.J3) - (0.022)*sin(planetCorrections.J4);
}

double RotationElements::corrWEuropa(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.980)*sin(planetCorrections.J4) - (0.054)*sin(planetCorrections.J5) - (0.014)*sin(planetCorrections.J6) - (0.008)*sin(planetCorrections.J7);
}

double RotationElements::corrWGanymede(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (0.033)*sin(planetCorrections.J4) - (0.389)*sin(planetCorrections.J5) - (0.082)*sin(planetCorrections.J6);
}

double RotationElements::corrWCallisto(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (0.061)*sin(planetCorrections.J5) - (0.533)*sin(planetCorrections.J6) - (0.009)*sin(planetCorrections.J8);
}

double RotationElements::corrWAmalthea(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (0.76)*sin(planetCorrections.J1) - (0.01)*sin(2.*planetCorrections.J1);
}

double RotationElements::corrWThebe(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (1.91)*sin(planetCorrections.J2) - (0.04)*sin(2.*planetCorrections.J2);
}

double RotationElements::corrWMimas(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-13.48)*sin(planetCorrections.S3) - (44.85)*sin(planetCorrections.S5);
}

double RotationElements::corrWTethys(const double d, const double T){
	Q_UNUSED(d) Q_UNUSED(T)
	return (-9.60)*sin(planetCorrections.S4) + (2.23)*sin(planetCorrections.S5);
}

double RotationElements::corrWRhea(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-3.08)*sin(planetCorrections.S6);
}

double RotationElements::corrWJanus(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (1.613)*sin(planetCorrections.S2) - (0.023)*sin(2.*planetCorrections.S2);
}

double RotationElements::corrWEpimetheus(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return 3.133 * sin(planetCorrections.S1) - (0.086)*sin(2.*planetCorrections.S1);
}

double RotationElements::corrWCordelia(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.04)*sin(planetCorrections.U1);
}

double RotationElements::corrWOphelia(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.03)*sin(planetCorrections.U2);
}

double RotationElements::corrWBianca(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.04)*sin(planetCorrections.U3);
}

double RotationElements::corrWCressida(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.01)*sin(planetCorrections.U4);
}

double RotationElements::corrWDesdemona(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return -0.04 * sin(planetCorrections.U5);
}

double RotationElements::corrWJuliet(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return -0.02 * sin(planetCorrections.U6);
}

double RotationElements::corrWPortia(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return -0.02 * sin(planetCorrections.U7);
}

double RotationElements::corrWRosalind(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return -0.08 * sin(planetCorrections.U8);
}

double RotationElements::corrWBelinda(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return -0.01 * sin(planetCorrections.U9);
}

double RotationElements::corrWPuck(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return -0.09 * sin(planetCorrections.U10);
}

double RotationElements::corrWAriel(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return 0.05 * sin(planetCorrections.U12) + 0.08 * sin(planetCorrections.U13);
}

double RotationElements::corrWUmbriel(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return -0.09 * sin(planetCorrections.U12) + 0.06 * sin(planetCorrections.U14);
}

double RotationElements::corrWTitania(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return 0.08 * sin(planetCorrections.U15);
}

double RotationElements::corrWOberon(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return 0.04 * sin(planetCorrections.U16);
}

double RotationElements::corrWMiranda(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return   -1.27 * sin(planetCorrections.U12) + 0.15 * sin(2.*planetCorrections.U12)
		+ 1.15 * sin(planetCorrections.U11) - 0.09 * sin(2.*planetCorrections.U11);
}

double RotationElements::corrWTriton(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return   22.25 * sin(   planetCorrections.N7) + 6.73 * sin(2.*planetCorrections.N7)
		+ 2.05 * sin(3.*planetCorrections.N7) + 0.74 * sin(4.*planetCorrections.N7)
		+ 0.28 * sin(5.*planetCorrections.N7) + 0.11 * sin(6.*planetCorrections.N7)
		+ 0.05 * sin(7.*planetCorrections.N7) + 0.02 * sin(8.*planetCorrections.N7)
		+ 0.01 * sin(9.*planetCorrections.N7);
}

double RotationElements::corrWNaiad(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.48)*sin(planetCorrections.Na) + (4.40)*sin(planetCorrections.N1) - (0.27)*sin(2.*planetCorrections.N1);
}

double RotationElements::corrWThalassa(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.48)*sin(planetCorrections.Na) + (0.19)*sin(planetCorrections.N2);
}

double RotationElements::corrWDespina(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.49)*sin(planetCorrections.Na) + (0.06)*sin(planetCorrections.N3);
}

double RotationElements::corrWGalatea(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.48)*sin(planetCorrections.Na) + (0.05)*sin(planetCorrections.N4);
}

double RotationElements::corrWLarissa(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.48)*sin(planetCorrections.Na) + (0.19)*sin(planetCorrections.N5);
}

double RotationElements::corrWProteus(const double d, const double T)
{
	Q_UNUSED(d) Q_UNUSED(T)
	return (-0.48)*sin(planetCorrections.Na) + (0.04)*sin(planetCorrections.N6);
}

void RotationElements::corrOriMoon(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	// This is from WGCCRE2009.
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA += - (3.8787*M_PI_180)*sin(planetCorrections.E1)  - (0.1204*M_PI_180)*sin(planetCorrections.E2) + (0.0700*M_PI_180)*sin(planetCorrections.E3)
			- (0.0172*M_PI_180)*sin(planetCorrections.E4)  + (0.0072*M_PI_180)*sin(planetCorrections.E6) - (0.0052*M_PI_180)*sin(planetCorrections.E10)
			+ (0.0043*M_PI_180)*sin(planetCorrections.E13);
	*J2000NPoleDE += + (1.5419*M_PI_180)*cos(planetCorrections.E1)  + (0.0239*M_PI_180)*cos(planetCorrections.E2) - (0.0278*M_PI_180)*cos(planetCorrections.E3)
			+ (0.0068*M_PI_180)*cos(planetCorrections.E4)  - (0.0029*M_PI_180)*cos(planetCorrections.E6) + (0.0009*M_PI_180)*cos(planetCorrections.E7)
			+ (0.0008*M_PI_180)*cos(planetCorrections.E10) - (0.0009*M_PI_180)*cos(planetCorrections.E13);
}

void RotationElements::corrOriMars(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d)
	*J2000NPoleRA+=	 (0.000068*M_PI_180)*sin(M_PI_180*(198.991226+remainder(19139.4819985*T, 360.)))
			+(0.000238*M_PI_180)*sin(M_PI_180*(226.292679+remainder(38280.8511281*T, 360.)))
			+(0.000052*M_PI_180)*sin(M_PI_180*(249.663391+remainder(57420.7251593*T, 360.)))
			+(0.000009*M_PI_180)*sin(M_PI_180*(266.183510+remainder(76560.6367950*T, 360.)))
			+(0.419057*M_PI_180)*sin(M_PI_180*( 79.398797+remainder(    0.5042615*T, 360.)));
	*J2000NPoleDE+=	 (0.000051*M_PI_180)*cos(M_PI_180*(122.433576+remainder(19139.9407476*T, 360.)))
			+(0.000141*M_PI_180)*cos(M_PI_180*( 43.058401+remainder(38280.8753272*T, 360.)))
			+(0.000031*M_PI_180)*cos(M_PI_180*( 57.663379+remainder(57420.7517205*T, 360.)))
			-(0.000005*M_PI_180)*cos(M_PI_180*( 79.476401+remainder(76560.6495004*T, 360.)))
			+(1.591274*M_PI_180)*cos(M_PI_180*(166.325722+remainder(    0.5042615*T, 360.)));
}

void RotationElements::corrOriJupiter(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=	 (0.000117*M_PI_180)*sin(planetCorrections.Ja)
			+(0.000938*M_PI_180)*sin(planetCorrections.Jb)
			+(0.001432*M_PI_180)*sin(planetCorrections.Jc)
			+(0.000030*M_PI_180)*sin(planetCorrections.Jd)
			+(0.002150*M_PI_180)*sin(planetCorrections.Je);
	*J2000NPoleDE+=	 (0.000050*M_PI_180)*cos(planetCorrections.Ja)
			+(0.000404*M_PI_180)*cos(planetCorrections.Jb)
			+(0.000617*M_PI_180)*cos(planetCorrections.Jc)
			-(0.000013*M_PI_180)*cos(planetCorrections.Jd)
			+(0.000926*M_PI_180)*cos(planetCorrections.Je);
}

void RotationElements::corrOriNeptune(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+= (0.7 *M_PI_180)*sin(planetCorrections.Na);
	*J2000NPoleDE-= (0.51*M_PI_180)*cos(planetCorrections.Na);
}

void RotationElements::corrOriPhobos(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=
			+(-1.78428399*M_PI_180)*sin(planetCorrections.M1)
			+(+0.02212824*M_PI_180)*sin(planetCorrections.M2)
			+(-0.01028251*M_PI_180)*sin(planetCorrections.M3)
			+(-0.00475595*M_PI_180)*sin(planetCorrections.M4);
	*J2000NPoleDE+=
			+(-1.07516537*M_PI_180)*cos(planetCorrections.M1)
			+(+0.00668626*M_PI_180)*cos(planetCorrections.M2)
			+(-0.00648740*M_PI_180)*cos(planetCorrections.M3)
			+(+0.00281576*M_PI_180)*cos(planetCorrections.M4);
}

void RotationElements::corrOriDeimos(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=   (3.09217726*M_PI_180)*sin(planetCorrections.M6)
			+(0.22980637*M_PI_180)*sin(planetCorrections.M7)
			+(0.06418655*M_PI_180)*sin(planetCorrections.M8)
			+(0.02533537*M_PI_180)*sin(planetCorrections.M9)
			+(0.00778695*M_PI_180)*sin(planetCorrections.M10);

	*J2000NPoleDE-=   (1.83936004*M_PI_180)*cos(planetCorrections.M6)
			+(0.14325320*M_PI_180)*cos(planetCorrections.M7)
			+(0.01911409*M_PI_180)*cos(planetCorrections.M8)
			-(0.01482590*M_PI_180)*cos(planetCorrections.M9)
			+(0.00192430*M_PI_180)*cos(planetCorrections.M10);
}

void RotationElements::corrOriIo(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*0.094)*sin(planetCorrections.J3) + (M_PI_180*0.024)*sin(planetCorrections.J4);
	*J2000NPoleDE+=(M_PI_180*0.040)*cos(planetCorrections.J3) + (M_PI_180*0.011)*cos(planetCorrections.J4);
}

void RotationElements::corrOriEuropa(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*1.086)*sin(planetCorrections.J4) + (M_PI_180*0.060)*sin(planetCorrections.J5) + (M_PI_180*0.015)*sin(planetCorrections.J6) + (M_PI_180*0.009)*sin(planetCorrections.J7);
	*J2000NPoleDE+=(M_PI_180*0.468)*cos(planetCorrections.J4) + (M_PI_180*0.026)*cos(planetCorrections.J5) + (M_PI_180*0.007)*cos(planetCorrections.J6) + (M_PI_180*0.002)*cos(planetCorrections.J7);
}

void RotationElements::corrOriGanymede(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-.037)*sin(planetCorrections.J4) + (M_PI_180*0.431)*sin(planetCorrections.J5) + (M_PI_180*0.091)*sin(planetCorrections.J6);
	*J2000NPoleDE+=(M_PI_180*-.016)*cos(planetCorrections.J4) + (M_PI_180*0.186)*cos(planetCorrections.J5) + (M_PI_180*0.039)*cos(planetCorrections.J6);
}

void RotationElements::corrOriCallisto(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-.068)*sin(planetCorrections.J5) + (M_PI_180*0.590)*sin(planetCorrections.J6) + (M_PI_180*0.010)*sin(planetCorrections.J8);
	*J2000NPoleDE+=(M_PI_180*-.029)*cos(planetCorrections.J5) + (M_PI_180*0.254)*cos(planetCorrections.J6) - (M_PI_180*0.004)*cos(planetCorrections.J8);
}

void RotationElements::corrOriAmalthea(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*0.01)*sin(2.*planetCorrections.J1) - (M_PI_180*0.84)*sin(planetCorrections.J1);
	*J2000NPoleDE-=(M_PI_180*0.36)*cos(planetCorrections.J1);
}

void RotationElements::corrOriThebe(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-2.11)*sin(planetCorrections.J2) + (M_PI_180*0.04)*sin(2.*planetCorrections.J2);
	*J2000NPoleDE+=(M_PI_180*-0.91)*cos(planetCorrections.J2) + (M_PI_180*0.01)*cos(2.*planetCorrections.J2);
}

void RotationElements::corrOriMimas(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*13.56)*sin(planetCorrections.S3);
	*J2000NPoleDE+=(M_PI_180*-1.53)*cos(planetCorrections.S3);
}

void RotationElements::corrOriTethys(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 9.66)*sin(planetCorrections.S4);
	*J2000NPoleDE+=(M_PI_180*-1.09)*cos(planetCorrections.S4);
}

void RotationElements::corrOriRhea(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI/180.* 3.10)*sin(planetCorrections.S6);
	*J2000NPoleDE+=(M_PI/180.*-0.35)*cos(planetCorrections.S6);
}

void RotationElements::corrOriJanus(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*0.023)*sin(2.*planetCorrections.S2)-(M_PI_180*1.623)*sin(planetCorrections.S2);
	*J2000NPoleDE+=(M_PI_180*0.001)*cos(2.*planetCorrections.S2)-(M_PI_180*0.183)*cos(planetCorrections.S2);
}

void RotationElements::corrOriEpimetheus(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*0.086)*sin(2.*planetCorrections.S1)-(M_PI_180*3.153)*sin(planetCorrections.S1);
	*J2000NPoleDE+=(M_PI_180*0.005)*cos(2.*planetCorrections.S1)-(M_PI_180*0.356)*cos(planetCorrections.S1);
}

void RotationElements::corrOriAriel(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 0.29)*sin(planetCorrections.U13);
	*J2000NPoleDE+=(M_PI_180* 0.28)*cos(planetCorrections.U13);
}

void RotationElements::corrOriUmbriel(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 0.21)*sin(planetCorrections.U14);
	*J2000NPoleDE+=(M_PI_180* 0.2 )*cos(planetCorrections.U14);
}

void RotationElements::corrOriTitania(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 0.29)*sin(planetCorrections.U15);
	*J2000NPoleDE+=(M_PI_180* 0.28)*cos(planetCorrections.U15);
}

void RotationElements::corrOriOberon(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 0.16)*sin(planetCorrections.U16);
	*J2000NPoleDE+=(M_PI_180* 0.16)*cos(planetCorrections.U16);
}

void RotationElements::corrOriMiranda(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 4.41)*sin(planetCorrections.U11) - (M_PI_180* 0.04)*sin(2.*planetCorrections.U11);
	*J2000NPoleDE+=(M_PI_180* 4.25)*cos(planetCorrections.U11) - (M_PI_180* 0.02)*cos(2.*planetCorrections.U11);
}

void RotationElements::corrOriCordelia(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-0.15)*sin(planetCorrections.U1);
	*J2000NPoleDE+=(M_PI_180* 0.14)*cos(planetCorrections.U1);
}

void RotationElements::corrOriOphelia(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-0.09)*sin(planetCorrections.U2);
	*J2000NPoleDE+=(M_PI_180* 0.09)*cos(planetCorrections.U2);
}

void RotationElements::corrOriBianca(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-0.16)*sin(planetCorrections.U3);
	*J2000NPoleDE+=(M_PI_180* 0.16)*cos(planetCorrections.U3);
}

void RotationElements::corrOriCressida(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-0.04)*sin(planetCorrections.U4);
	*J2000NPoleDE+=(M_PI_180* 0.04)*cos(planetCorrections.U4);
}

void RotationElements::corrOriDesdemona(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-0.17)*sin(planetCorrections.U5);
	*J2000NPoleDE+=(M_PI_180* 0.16)*cos(planetCorrections.U5);
}

void RotationElements::corrOriJuliet(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-0.06)*sin(planetCorrections.U6);
	*J2000NPoleDE+=(M_PI_180* 0.06)*cos(planetCorrections.U6);
}

void RotationElements::corrOriPortia(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-0.09)*sin(planetCorrections.U7);
	*J2000NPoleDE+=(M_PI_180* 0.09)*cos(planetCorrections.U7);
}

void RotationElements::corrOriRosalind(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-0.29)*sin(planetCorrections.U8);
	*J2000NPoleDE+=(M_PI_180* 0.28)*cos(planetCorrections.U8);
}

void RotationElements::corrOriBelinda(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-0.03)*sin(planetCorrections.U9);
	*J2000NPoleDE+=(M_PI_180* 0.03)*cos(planetCorrections.U9);
}

void RotationElements::corrOriPuck(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180*-0.33)*sin(planetCorrections.U10);
	*J2000NPoleDE+=(M_PI_180* 0.31)*cos(planetCorrections.U10);
}

void RotationElements::corrOriTriton(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=    (M_PI_180*-32.35)*sin(planetCorrections.N7)  - (M_PI_180*6.28)*sin(2.*planetCorrections.N7)
			- (M_PI_180*2.08)*sin(3.*planetCorrections.N7) - (M_PI_180*0.74)*sin(4.*planetCorrections.N7)
			- (M_PI_180*0.28)*sin(5.*planetCorrections.N7) - (M_PI_180*0.11)*sin(6.*planetCorrections.N7)
			- (M_PI_180*0.07)*sin(7.*planetCorrections.N7) - (M_PI_180*0.02)*sin(8.*planetCorrections.N7)
			- (M_PI_180*0.01)*sin(9.*planetCorrections.N7);
	*J2000NPoleDE+=    (M_PI_180* 22.55)*cos(planetCorrections.N7)  + (M_PI_180*2.10)*cos(2.*planetCorrections.N7)
			+ (M_PI_180*0.55)*cos(3.*planetCorrections.N7) + (M_PI_180*0.16)*cos(4.*planetCorrections.N7)
			+ (M_PI_180*0.05)*cos(5.*planetCorrections.N7) + (M_PI_180*0.02)*cos(6.*planetCorrections.N7)
			+ (M_PI_180*0.01)*cos(7.*planetCorrections.N7);
}

void RotationElements::corrOriNaiad(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 0.70)*sin(planetCorrections.Na) - (M_PI_180* 6.49)*sin(planetCorrections.N1) + (M_PI_180* 0.25)*sin(2.*planetCorrections.N1);
	*J2000NPoleDE+=(M_PI_180*-0.51)*cos(planetCorrections.Na) - (M_PI_180* 4.75)*cos(planetCorrections.N1) + (M_PI_180* 0.09)*cos(2.*planetCorrections.N1);
}

void RotationElements::corrOriThalassa(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 0.70)*sin(planetCorrections.Na) - (M_PI_180* 0.28)*sin(planetCorrections.N2);
	*J2000NPoleDE+=(M_PI_180*-0.51)*cos(planetCorrections.Na) - (M_PI_180* 0.21)*cos(planetCorrections.N2);
}

void RotationElements::corrOriDespina(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 0.70)*sin(planetCorrections.Na) - (M_PI_180* 0.09)*sin(planetCorrections.N3);
	*J2000NPoleDE+=(M_PI_180*-0.51)*cos(planetCorrections.Na) - (M_PI_180* 0.07)*cos(planetCorrections.N3);
}

void RotationElements::corrOriGalatea(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 0.70)*sin(planetCorrections.Na) - (M_PI_180* 0.07)*sin(planetCorrections.N4);
	*J2000NPoleDE+=(M_PI_180*-0.51)*cos(planetCorrections.Na) - (M_PI_180* 0.05)*cos(planetCorrections.N4);
}

void RotationElements::corrOriLarissa(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 0.70)*sin(planetCorrections.Na) - (M_PI_180* 0.27)*sin(planetCorrections.N5);
	*J2000NPoleDE+=(M_PI_180*-0.51)*cos(planetCorrections.Na) - (M_PI_180* 0.20)*cos(planetCorrections.N5);
}

void RotationElements::corrOriProteus(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE)
{
	Q_UNUSED(d) Q_UNUSED(T)
	*J2000NPoleRA+=(M_PI_180* 0.70)*sin(planetCorrections.Na) - (M_PI_180* 0.05)*sin(planetCorrections.N6);
	*J2000NPoleDE+=(M_PI_180*-0.51)*cos(planetCorrections.Na) - (M_PI_180* 0.04)*cos(planetCorrections.N6);
}

const QList<Vec3d> RotationElements::marsMagLs =
{
	{ -20.0,  0.024, -0.030},
	{ -10.0,  0.034, -0.017},
	{   0.0,  0.036, -0.029},
	{  10.0,  0.045, -0.017},
	{  20.0,  0.038, -0.014},
	{  30.0,  0.023, -0.006},
	{  40.0,  0.015, -0.018},
	{  50.0,  0.011, -0.020},
	{  60.0,  0.000, -0.014},
	{  70.0, -0.012, -0.030},
	{  80.0, -0.018, -0.008},
	{  90.0, -0.036, -0.040},
	{ 100.0, -0.044, -0.024},
	{ 110.0, -0.059, -0.037},
	{ 120.0, -0.060, -0.036},
	{ 130.0, -0.055, -0.032},
	{ 140.0, -0.043,  0.010},
	{ 150.0, -0.041,  0.010},
	{ 160.0, -0.041, -0.001},
	{ 170.0, -0.036,  0.044},
	{ 180.0, -0.036,  0.025},
	{ 190.0, -0.018, -0.004},
	{ 200.0, -0.038, -0.016},
	{ 210.0, -0.011, -0.008},
	{ 220.0,  0.002,  0.029},
	{ 230.0,  0.004, -0.054},
	{ 240.0,  0.018,  0.0  },
	{ 250.0,  0.019,  0.055},
	{ 260.0,  0.035,  0.017},
	{ 270.0,  0.050,  0.052},
	{ 280.0,  0.035,  0.006},
	{ 290.0,  0.027,  0.087},
	{ 300.0,  0.037,  0.006},
	{ 310.0,  0.048,  0.064},
	{ 320.0,  0.025,  0.030},
	{ 330.0,  0.022,  0.019},
	{ 340.0,  0.024, -0.030},
	{ 350.0,  0.034, -0.017},
	{ 360.0,  0.036, -0.029},
	{ 370.0,  0.045, -0.017},
	{ 380.0,  0.038, -0.014}};

// Retrieve magnitude variation depending on angle Ls [radians].
// Source: A. Mallama: The magnitude and albedo of Mars. Icarus 192(2007) 404-416.
// @arg albedo true  to return longitudinal albedo correction, the average of sub-earth and sub-solar planetographic longitudes
//             false for the Orbital Longitude correction

// Orbital Longitude: In the context given by Mallama 2007, this defines the position of the Sun along the planet's "ecliptic", counted from the vernal equinox of Mars.
// Stellarium does not provide this number directly, however we have in the old rotation elements ascending node and obliquity.
// The ascending node is that of the Mars equator over the ecliptic of J2000.0 given as node=82.91 degrees. When the sun is there, it moves from the northern to the southern hemisphere of Mars.
// That means, if Mars is at heliocentric l=82.91°, the sun is crossing northwards, i.e. this is where Ls=0. Therefore Ls= (l-node) mod 2pi.
double RotationElements::getMarsMagLs(const double Ls, const bool albedo)
{
	const double l=StelUtils::fmodpos(Ls, 2.0*M_PI) * M_180_PI; // longitude in degrees [0...360]
	int pos=std::lround(std::floor(l/10.))+2; // index of central value for 5-point Spline interpolation
	Q_ASSERT(pos>=2);
	Q_ASSERT(pos<38);
	double n=(l-marsMagLs.at(pos)[0]) / 10.0;
	Q_ASSERT(abs(n)<1.);
	if (albedo)
		return StelUtils::interpolate5(n, marsMagLs.at(pos-2)[1], marsMagLs.at(pos-1)[1], marsMagLs.at(pos)[1], marsMagLs.at(pos+1)[1], marsMagLs.at(pos+2)[1]);
	else
		return StelUtils::interpolate5(n, marsMagLs.at(pos-2)[2], marsMagLs.at(pos-1)[2], marsMagLs.at(pos)[2], marsMagLs.at(pos+1)[2], marsMagLs.at(pos+2)[2]);
}
