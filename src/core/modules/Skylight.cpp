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
	thetas(0.f),
	T(0.f),
	zenithLuminance(0.f),
	zenithColorX(0.f),
	zenithColorY(0.f),
	eyeLumConversion(0.f)

{
	AY = BY = CY = DY = EY = 0.f;
	Ax = Bx = Cx = Dx = Ex = 0.f;
	Ay = By = Cy = Dy = Ey = 0.f;
	term_x = term_y = term_Y = 0.f;
	// conf
	QSettings* conf = StelApp::getInstance().getSettings();

	AYt=conf->value("Skylight/AYt",  0.1787f).toFloat();
	AYc=conf->value("Skylight/AYc", -1.4630f).toFloat();
	BYt=conf->value("Skylight/BYt", -0.3554f).toFloat();
	BYc=conf->value("Skylight/BYc", +0.4275f).toFloat();
	CYt=conf->value("Skylight/CYt", -0.0227f).toFloat();
	CYc=conf->value("Skylight/CYc", +5.3251f).toFloat();
	DYt=conf->value("Skylight/DYt",  0.1206f).toFloat();
	DYc=conf->value("Skylight/DYc", -2.5771f).toFloat();
	EYt=conf->value("Skylight/EYt", -0.0670f).toFloat();
	EYc=conf->value("Skylight/EYc", +0.3703f).toFloat();

	Axt=conf->value("Skylight/Axt", -0.0193f).toFloat();
	Axc=conf->value("Skylight/Axc", -0.2592f).toFloat();
	Bxt=conf->value("Skylight/Bxt", -0.0665f).toFloat();
	Bxc=conf->value("Skylight/Bxc", +0.0008f).toFloat();
	Cxt=conf->value("Skylight/Cxt", -0.0004f).toFloat();
	Cxc=conf->value("Skylight/Cxc", +0.2125f).toFloat();
	Dxt=conf->value("Skylight/Dxt", -0.0641f).toFloat();
	Dxc=conf->value("Skylight/Dxc", -0.8989f).toFloat();
	Ext=conf->value("Skylight/Ext", -0.0033f).toFloat();
	Exc=conf->value("Skylight/Exc", +0.0452f).toFloat();

	Ayt=conf->value("Skylight/Ayt", -0.0167f).toFloat();
	Ayc=conf->value("Skylight/Ayc", -0.2608f).toFloat();
	Byt=conf->value("Skylight/Byt", -0.0950f).toFloat();
	Byc=conf->value("Skylight/Byc", +0.0092f).toFloat();
	Cyt=conf->value("Skylight/Cyt", -0.0079f).toFloat();
	Cyc=conf->value("Skylight/Cyc", +0.2102f).toFloat();
	Dyt=conf->value("Skylight/Dyt", -0.0441f).toFloat();
	Dyc=conf->value("Skylight/Dyc", -1.6537f).toFloat();
	Eyt=conf->value("Skylight/Eyt", -0.0109f).toFloat();
	Eyc=conf->value("Skylight/Eyc", +0.0529f).toFloat();

	zX11=conf->value("Skylight/zX11",  0.00166f).toFloat();
	zX12=conf->value("Skylight/zX12", -0.00375f).toFloat();
	zX13=conf->value("Skylight/zX13", +0.00209f).toFloat();
	zX21=conf->value("Skylight/zX21", -0.02903f).toFloat();
	zX22=conf->value("Skylight/zX22", +0.06377f).toFloat();
	zX23=conf->value("Skylight/zX23", -0.03202f).toFloat();
	zX24=conf->value("Skylight/zX24", +0.00394f).toFloat();
	zX31=conf->value("Skylight/zX31",  0.11693f).toFloat();
	zX32=conf->value("Skylight/zX32", -0.21196f).toFloat();
	zX33=conf->value("Skylight/zX33", +0.06052f).toFloat();
	zX34=conf->value("Skylight/zX34", +0.25886f).toFloat();

	zY11=conf->value("Skylight/zY11",  0.00275f).toFloat();
	zY12=conf->value("Skylight/zY12", -0.00610f).toFloat();
	zY13=conf->value("Skylight/zY13", +0.00317f).toFloat();
	zY21=conf->value("Skylight/zY21", -0.04214f).toFloat();
	zY22=conf->value("Skylight/zY22", +0.08970f).toFloat();
	zY23=conf->value("Skylight/zY23", -0.04153f).toFloat();
	zY24=conf->value("Skylight/zY24", +0.00516f).toFloat();
	zY31=conf->value("Skylight/zY31",  0.15346f).toFloat();
	zY32=conf->value("Skylight/zY32", -0.26756f).toFloat();
	zY33=conf->value("Skylight/zY33", +0.06670f).toFloat();
	zY34=conf->value("Skylight/zY34", +0.26688f).toFloat();


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
	thetas = std::acos((float)sunPos[2]);
	T = _turbidity;

	// Precomputation of the distribution coefficients and zenith luminances/color
	computeZenithLuminance();
	computeZenithColor();
	computeLuminanceDistributionCoefs();
	computeColorDistributionCoefs();

	// Precompute everything possible to increase the get_CIE_value() function speed
	float cos_thetas = sunPos[2];
	term_x = zenithColorX   / ((1.f + Ax * std::exp(Bx)) * (1.f + Cx * std::exp(Dx*thetas) + Ex * cos_thetas * cos_thetas));
	term_y = zenithColorY   / ((1.f + Ay * std::exp(By)) * (1.f + Cy * std::exp(Dy*thetas) + Ey * cos_thetas * cos_thetas));
	term_Y = 0.0; // zenithLuminance /((1.f + AY * std::exp(BY)) * (1.f + CY * std::exp(DY*thetas) + EY * cos_thetas * cos_thetas));
}

