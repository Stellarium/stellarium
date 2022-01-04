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
	T(5.),
	zenithLuminance(0.),
	zenithColorX(0.),
	zenithColorY(0.),
	eyeLumConversion(0.),
	flagSchaefer(true),
	flagGuiPublic(false)
{
	setObjectName("Skylight");
	AY = BY = CY = DY = EY = 0.;
	Ax = Bx = Cx = Dx = Ex = 0.;
	Ay = By = Cy = Dy = Ey = 0.;
	term_x = term_y = term_Y = 0.;
	// conf
	QSettings* conf = StelApp::getInstance().getSettings();

	// Defaults from Stellarium 0.15. These deviate in some places from the Preetham paper.
	// Fabien says they were hand-tuned, and look better, at least at the then-fixed T=5.
	AYt=conf->value("Skylight/AYt",  0.2787).toDouble(); // orig:  0.1787
	AYc=conf->value("Skylight/AYc", -1.0630).toDouble(); // orig: -1.4630
	BYt=conf->value("Skylight/BYt", -0.3554).toDouble();
	BYc=conf->value("Skylight/BYc", +0.4275).toDouble();
	CYt=conf->value("Skylight/CYt", -0.0227).toDouble();
	CYc=conf->value("Skylight/CYc", +6.3251).toDouble(); // orig: +5.3251
	DYt=conf->value("Skylight/DYt",  0.1206).toDouble();
	DYc=conf->value("Skylight/DYc", -2.5771).toDouble();
	EYt=conf->value("Skylight/EYt", -0.0670).toDouble();
	EYc=conf->value("Skylight/EYc", +0.3703).toDouble();

	Axt=conf->value("Skylight/Axt", -0.0148).toDouble(); // orig: -0.0193
	Axc=conf->value("Skylight/Axc", -0.1703).toDouble(); // orig: -0.2592
	Bxt=conf->value("Skylight/Bxt", -0.0664).toDouble(); // orig: -0.0665
	Bxc=conf->value("Skylight/Bxc", +0.0011).toDouble(); // orig: +0.0008
	Cxt=conf->value("Skylight/Cxt", -0.0005).toDouble(); // orig: -0.0004
	Cxc=conf->value("Skylight/Cxc", +0.2127).toDouble(); // orig: +0.2125
	Dxt=conf->value("Skylight/Dxt", -0.0641).toDouble();
	Dxc=conf->value("Skylight/Dxc", -0.8992).toDouble(); // orig: -0.8989
	Ext=conf->value("Skylight/Ext", -0.0035).toDouble(); // orig: -0.0033
	Exc=conf->value("Skylight/Exc", +0.0453).toDouble(); // orig: +0.0452

	Ayt=conf->value("Skylight/Ayt", -0.0131).toDouble(); // orig: -0.0167
	Ayc=conf->value("Skylight/Ayc", -0.2498).toDouble(); // orig: -0.2608
	Byt=conf->value("Skylight/Byt", -0.0951).toDouble(); // orig: -0.0950
	Byc=conf->value("Skylight/Byc", +0.0092).toDouble();
	Cyt=conf->value("Skylight/Cyt", -0.0082).toDouble(); // orig: -0.0079
	Cyc=conf->value("Skylight/Cyc", +0.2404).toDouble(); // orig: +0.2102
	Dyt=conf->value("Skylight/Dyt", -0.0438).toDouble(); // orig: -0.0441
	Dyc=conf->value("Skylight/Dyc", -1.0539).toDouble(); // orig: -1.6537
	Eyt=conf->value("Skylight/Eyt", -0.0109).toDouble();
	Eyc=conf->value("Skylight/Eyc", +0.0531).toDouble(); // orig: +0.0529

	zX11=conf->value("Skylight/zX11",  0.00216).toDouble(); // orig: 0.00166
	zX12=conf->value("Skylight/zX12", -0.00375).toDouble();
	zX13=conf->value("Skylight/zX13", +0.00209).toDouble();
	zX21=conf->value("Skylight/zX21", -0.02903).toDouble();
	zX22=conf->value("Skylight/zX22", +0.06377).toDouble();
	zX23=conf->value("Skylight/zX23", -0.03202).toDouble();
	zX24=conf->value("Skylight/zX24", +0.00394).toDouble();
	zX31=conf->value("Skylight/zX31",  0.10169).toDouble(); // orig: 0.11693
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
	zY31=conf->value("Skylight/zY31",  0.14535).toDouble(); // orig: 0.15346
	zY32=conf->value("Skylight/zY32", -0.26756).toDouble();
	zY33=conf->value("Skylight/zY33", +0.06670).toDouble();
	zY34=conf->value("Skylight/zY34", +0.26688).toDouble();

	flagSchaefer=conf->value("Skylight/use_Schaefer", true).toBool();
	flagGuiPublic=conf->value("Skylight/enable_gui", false).toBool();
}

Skylight::~Skylight()
{
}

void Skylight::setParamsv(const float * _sunPos, float _turbidity)
{
	// Store sun position
	sunPos[0] = _sunPos[0];
	sunPos[1] = _sunPos[1];
	sunPos[2] = _sunPos[2];

	// Set the two main variables
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
