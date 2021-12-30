/*
 * Copyright (C) 2003 Fabien Chereau
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

// Class which compute the daylight sky color
// Fast implementation of the algorithm from the article
// "A Practical Analytic Model for Daylight" by A. J. Preetham, Peter Shirley and Brian Smits.

#include "Skylight.hpp"

#include <cmath>
#include <cstdlib>
#include <QSettings>

Skylight::Skylight() :
	thetas(0.),
	T(0.),
	zenithLuminance(0.),
	zenithColorX(0.),
	zenithColorY(0.),
	eyeLumConversion(0.),
	flagSRGB(false),
	flagSchaefer(true)
{
	setObjectName("Skylight");
	AY = BY = CY = DY = EY = 0.;
	Ax = Bx = Cx = Dx = Ex = 0.;
	Ay = By = Cy = Dy = Ey = 0.;
	term_x = term_y = term_Y = 0.;
	// conf
	QSettings* conf = StelApp::getInstance().getSettings();

	AYt=conf->value("Skylight/AYt",  0.1787).toDouble();
	AYc=conf->value("Skylight/AYc", -1.4630).toDouble();
	BYt=conf->value("Skylight/BYt", -0.3554).toDouble();
	BYc=conf->value("Skylight/BYc", +0.4275).toDouble();
	CYt=conf->value("Skylight/CYt", -0.0227).toDouble();
	CYc=conf->value("Skylight/CYc", +5.3251).toDouble();
	DYt=conf->value("Skylight/DYt",  0.1206).toDouble();
	DYc=conf->value("Skylight/DYc", -2.5771).toDouble();
	EYt=conf->value("Skylight/EYt", -0.0670).toDouble();
	EYc=conf->value("Skylight/EYc", +0.3703).toDouble();

	Axt=conf->value("Skylight/Axt", -0.0193).toDouble();
	Axc=conf->value("Skylight/Axc", -0.2592).toDouble();
	Bxt=conf->value("Skylight/Bxt", -0.0665).toDouble();
	Bxc=conf->value("Skylight/Bxc", +0.0008).toDouble();
	Cxt=conf->value("Skylight/Cxt", -0.0004).toDouble();
	Cxc=conf->value("Skylight/Cxc", +0.2125).toDouble();
	Dxt=conf->value("Skylight/Dxt", -0.0641).toDouble();
	Dxc=conf->value("Skylight/Dxc", -0.8989).toDouble();
	Ext=conf->value("Skylight/Ext", -0.0033).toDouble();
	Exc=conf->value("Skylight/Exc", +0.0452).toDouble();

	Ayt=conf->value("Skylight/Ayt", -0.0167).toDouble();
	Ayc=conf->value("Skylight/Ayc", -0.2608).toDouble();
	Byt=conf->value("Skylight/Byt", -0.0950).toDouble();
	Byc=conf->value("Skylight/Byc", +0.0092).toDouble();
	Cyt=conf->value("Skylight/Cyt", -0.0079).toDouble();
	Cyc=conf->value("Skylight/Cyc", +0.2102).toDouble();
	Dyt=conf->value("Skylight/Dyt", -0.0441).toDouble();
	Dyc=conf->value("Skylight/Dyc", -1.6537).toDouble();
	Eyt=conf->value("Skylight/Eyt", -0.0109).toDouble();
	Eyc=conf->value("Skylight/Eyc", +0.0529).toDouble();

	zX11=conf->value("Skylight/zX11",  0.00166).toDouble();
	zX12=conf->value("Skylight/zX12", -0.00375).toDouble();
	zX13=conf->value("Skylight/zX13", +0.00209).toDouble();
	zX21=conf->value("Skylight/zX21", -0.02903).toDouble();
	zX22=conf->value("Skylight/zX22", +0.06377).toDouble();
	zX23=conf->value("Skylight/zX23", -0.03202).toDouble();
	zX24=conf->value("Skylight/zX24", +0.00394).toDouble();
	zX31=conf->value("Skylight/zX31",  0.11693).toDouble();
	zX32=conf->value("Skylight/zX32", -0.21196).toDouble();
	zX33=conf->value("Skylight/zX33", +0.06052).toDouble();
	zX34=conf->value("Skylight/zX34", +0.25886).toDouble();

	zY11=conf->value("Skylight/zY11",  0.00275).toDouble();
	zY12=conf->value("Skylight/zY12", -0.00610).toDouble();
	zY13=conf->value("Skylight/zY13", +0.00317).toDouble();
	zY21=conf->value("Skylight/zY21", -0.04214).toDouble();
	zY22=conf->value("Skylight/zY22", +0.08970).toDouble();
	zY23=conf->value("Skylight/zY23", -0.04153).toDouble();
	zY24=conf->value("Skylight/zY24", +0.00516).toDouble();
	zY31=conf->value("Skylight/zY31",  0.15346).toDouble();
	zY32=conf->value("Skylight/zY32", -0.26756).toDouble();
	zY33=conf->value("Skylight/zY33", +0.06670).toDouble();
	zY34=conf->value("Skylight/zY34", +0.26688).toDouble();

	flagSRGB=conf->value("Skylight/use_sRGB", false).toBool();
	flagSchaefer=conf->value("Skylight/use_Schaefer", true).toBool();
}

Skylight::~Skylight()
{
}

// NOT USED IN THE PROGRAM
/*
void Skylight::setParams(float _sunZenithAngle, float _turbidity)
{
	// Set the two main variables
	thetas = _sunZenithAngle;
	T = _turbidity;

	// Precomputation of the distribution coefficients and zenith luminances/color
	computeZenithLuminance();
	computeZenithColor();
	computeLuminanceDistributionCoefs();
	computeColorDistributionCoefs();

	// Precompute everything possible to increase the get_CIE_value() function speed
	float cos_thetas = std::cos(thetas);
	term_x = zenithColorX   / ((1.f + Ax * std::exp(Bx)) * (1.f + Cx * std::exp(Dx*thetas) + Ex * cos_thetas * cos_thetas));
	term_y = zenithColorY   / ((1.f + Ay * std::exp(By)) * (1.f + Cy * std::exp(Dy*thetas) + Ey * cos_thetas * cos_thetas));
	term_Y = zenithLuminance / ((1.f + AY * std::exp(BY)) * (1.f + CY * std::exp(DY*thetas) + EY * cos_thetas * cos_thetas));
}
*/

void Skylight::setParamsv(const float * _sunPos, float _turbidity)
{
	// Store sun position
	sunPos[0] = _sunPos[0];
	sunPos[1] = _sunPos[1];
	sunPos[2] = _sunPos[2];

	// Set the two main variables
	//thetas = M_PI_2 - std::asin((float)sunPos[2]);
	// GZ why not directly acos?
	thetas = std::acos(static_cast<double>(sunPos[2]));
	T = static_cast<double>(_turbidity);

	// Precomputation of the distribution coefficients and zenith luminances/color
	computeZenithLuminance();
	computeZenithColor();
	computeLuminanceDistributionCoefs();
	computeColorDistributionCoefs();

	// Precompute everything possible to increase the get_CIE_value() function speed
	double cos_thetas = static_cast<double>(sunPos[2]);
	term_x = zenithColorX   / ((1. + Ax * std::exp(Bx)) * (1. + Cx * std::exp(Dx*thetas) + Ex * cos_thetas * cos_thetas));
	term_y = zenithColorY   / ((1. + Ay * std::exp(By)) * (1. + Cy * std::exp(Dy*thetas) + Ey * cos_thetas * cos_thetas));
	term_Y = flagSchaefer ? 0. : zenithLuminance/ ((1. + AY * std::exp(BY)) * (1. + CY * std::exp(DY*thetas) + EY * cos_thetas * cos_thetas));
}
