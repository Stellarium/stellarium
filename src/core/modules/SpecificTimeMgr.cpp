/*
 * Copyright (C) 2022 Alexander Wolf
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

#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "SpecificTimeMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelTranslator.hpp"

#include <QSettings>

SpecificTimeMgr::SpecificTimeMgr()
	: twilightAltitude(0.)
{
	setObjectName("SpecificTimeMgr");
}

SpecificTimeMgr::~SpecificTimeMgr()
{
	//
}

//double SpecificTimeMgr::getCallOrder(StelModuleActionName actionName) const
//{
//	if (actionName==StelModule::ActionDraw)
//		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
//	return 0;
//}

void SpecificTimeMgr::init()
{
	core = StelApp::getInstance().getCore();
	conf = StelApp::getInstance().getSettings();
	objMgr = GETSTELMODULE(StelObjectMgr);
	PlanetP sun=GETSTELMODULE(SolarSystem)->getSun();

	QString timeGroup = N_("Specific Time");

	addAction("actionNext_Transit",		      timeGroup, N_("Next transit of the selected object"),     this, "nextTransit()");
	addAction("actionNext_Rising",		      timeGroup, N_("Next rising of the selected object"),      this, "nextRising()");
	addAction("actionNext_Setting",		      timeGroup, N_("Next setting of the selected object"),     this, "nextSetting()");
	addAction("actionToday_Transit",	      timeGroup, N_("Today's transit of the selected object"),  this, "todayTransit()");
	addAction("actionToday_Rising",		      timeGroup, N_("Today's rising of the selected object"),   this, "todayRising()");
	addAction("actionToday_Setting",	      timeGroup, N_("Today's setting of the selected object"),  this, "todaySetting()");
	addAction("actionPrevious_Transit",	      timeGroup, N_("Previous transit of the selected object"), this, "previousTransit()");
	addAction("actionPrevious_Rising",	      timeGroup, N_("Previous rising of the selected object"),  this, "previousRising()");
	addAction("actionPrevious_Setting",	      timeGroup, N_("Previous setting of the selected object"), this, "previousSetting()");

	addAction("actionNext_MorningTwilight",       timeGroup, N_("Next morning twilight"),     this, "nextMorningTwilight()");
	addAction("actionNext_EveningTwilight",       timeGroup, N_("Next evening twilight"),     this, "nextEveningTwilight()");
	addAction("actionToday_MorningTwilight",      timeGroup, N_("Today's morning twilight"),  this, "todayMorningTwilight()");
	addAction("actionToday_EveningTwilight",      timeGroup, N_("Today's evening twilight"),  this, "todayEveningTwilight()");
	addAction("actionPrevious_MorningTwilight",   timeGroup, N_("Previous morning twilight"), this, "previousMorningTwilight()");
	addAction("actionPrevious_EveningTwilight",   timeGroup, N_("Previous evening twilight"), this, "previousEveningTwilight()");

	addAction("actionNext_MorningAtAltitude",     timeGroup, N_("Selected object at altitude at next morning"),      this, "nextMorningAtAltitude()");
	addAction("actionToday_MorningAtAltitude",    timeGroup, N_("Selected object at altitude this morning"),         this, "todayMorningAtAltitude()");
	addAction("actionPrevious_MorningAtAltitude", timeGroup, N_("Selected object at altitude at previous morning"),  this, "previousMorningAtAltitude()");
	addAction("actionNext_EveningAtAltitude",     timeGroup, N_("Selected object at altitude at next evening"),      this, "nextEveningAtAltitude()");
	addAction("actionToday_EveningAtAltitude",    timeGroup, N_("Selected object at altitude this evening"),         this, "todayEveningAtAltitude()");
	addAction("actionPrevious_EveningAtAltitude", timeGroup, N_("Selected object at altitude at previous evening"),  this, "previousEveningAtAltitude()");

	addAction("actionCurrent_March_Equinox",      timeGroup, N_("March equinox at current year"),      this, "currentMarchEquinox()");
	addAction("actionNext_March_Equinox",         timeGroup, N_("March equinox at next year"),         this, "nextMarchEquinox()");
	addAction("actionPrevious_March_Equinox",     timeGroup, N_("March equinox at previous year"),     this, "previousMarchEquinox()");

	addAction("actionCurrent_September_Equinox",  timeGroup, N_("September equinox at current year"),  this, "currentSeptemberEquinox()");
	addAction("actionNext_September_Equinox",     timeGroup, N_("September equinox at next year"),     this, "nextSeptemberEquinox()");
	addAction("actionPrevious_September_Equinox", timeGroup, N_("September equinox at previous year"), this, "previousSeptemberEquinox()");

	addAction("actionCurrent_June_Solstice",      timeGroup, N_("June solstice at current year"),      this, "currentJuneSolstice()");
	addAction("actionNext_June_Solstice",         timeGroup, N_("June solstice at next year"),         this, "nextJuneSolstice()");
	addAction("actionPrevious_June_Solstice",     timeGroup, N_("June solstice at previous year"),     this, "previousJuneSolstice()");

	addAction("actionCurrent_December_Solstice",  timeGroup, N_("December solstice at current year"),  this, "currentDecemberSolstice()");
	addAction("actionNext_December_Solstice",     timeGroup, N_("December solstice at next year"),     this, "nextDecemberSolstice()");
	addAction("actionPrevious_December_Solstice", timeGroup, N_("December solstice at previous year"), this, "previousDecemberSolstice()");

	setTwilightAltitude(conf->value("astro/twilight_altitude", -6.).toDouble());
}

void SpecificTimeMgr::deinit()
{

}

//void SpecificTimeMgr::draw(StelCore* core)
//{
//	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
//	StelPainter painter(prj);
//}

void SpecificTimeMgr::setTwilightAltitude(double alt)
{
	if (!qFuzzyCompare(alt, twilightAltitude))
	{
		twilightAltitude=alt;
		emit twilightAltitudeChanged(alt);
	}
}

void SpecificTimeMgr::nextTransit()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[1]);
	}
}

void SpecificTimeMgr::nextRising()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::nextSetting()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

void SpecificTimeMgr::previousTransit()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(-1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[1]);
	}
}

void SpecificTimeMgr::previousRising()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(-1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::previousSetting()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(-1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

void SpecificTimeMgr::todayTransit()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[1]);
	}
}

void SpecificTimeMgr::todayRising()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::todaySetting()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

void SpecificTimeMgr::todayMorningTwilight()
{
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
		core->setJD(rts[0]);
}

void SpecificTimeMgr::todayEveningTwilight()
{
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
		core->setJD(rts[2]);
}

void SpecificTimeMgr::previousMorningTwilight()
{
	core->addSolarDays(-1.0);
	core->update(0);
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
		core->setJD(rts[0]);
}

void SpecificTimeMgr::previousEveningTwilight()
{
	core->addSolarDays(-1.0);
	core->update(0);
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
		core->setJD(rts[2]);
}

void SpecificTimeMgr::nextMorningTwilight()
{
	core->addSolarDays(1.0);
	core->update(0);
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
		core->setJD(rts[0]);
}

void SpecificTimeMgr::nextEveningTwilight()
{
	core->addSolarDays(1.0);
	core->update(0);
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
		core->setJD(rts[2]);
}

void SpecificTimeMgr::todayMorningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::nextMorningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		core->addSolarDays(1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::previousMorningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		core->addSolarDays(-1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::todayEveningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

void SpecificTimeMgr::nextEveningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		core->addSolarDays(1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

void SpecificTimeMgr::previousEveningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		core->addSolarDays(-1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

static const double MeeusCoefficients[24][3] = {
	//  i (row) a{i}   b{i}       c{i}
	/*  1 */ { 485.0, 324.96,   1934.136 },
	/*  2 */ { 203.0, 337.23,  32964.467 },
	/*  3 */ { 199.0, 342.08,     20.186 },
	/*  4 */ { 182.0,  27.85, 445267.112 },
	/*  5 */ { 156.0,  73.14,  45036.886 },
	/*  6 */ { 136.0, 171.52,  22518.443 },
	/*  7 */ {  77.0, 222.54,  65928.934 },
	/*  8 */ {  74.0, 296.72,   3034.906 },
	/*  9 */ {  70.0, 243.58,   9037.513 },
	/* 10 */ {  58.0, 119.81,  33718.147 },
	/* 11 */ {  52.0, 297.17,    150.678 },
	/* 12 */ {  50.0,  21.02,   2281.226 },
	/* 13 */ {  45.0, 247.54,  29929.562 },
	/* 14 */ {  44.0, 325.15,  31555.956 },
	/* 15 */ {  29.0,  60.93,   4443.417 },
	/* 16 */ {  18.0, 155.12,  67555.328 },
	/* 17 */ {  17.0, 288.79,   4562.452 },
	/* 18 */ {  16.0, 198.04,  62894.029 },
	/* 19 */ {  14.0, 199.76,  31436.921 },
	/* 20 */ {  12.0,  95.39,  14577.848 },
	/* 21 */ {  12.0, 287.11,  31931.756 },
	/* 22 */ {  12.0, 320.81,  34777.259 },
	/* 23 */ {   9.0, 227.73,   1222.114 },
	/* 24 */ {   8.0,  15.45,  16859.074 }
};

double SpecificTimeMgr::getEquinox(int year, SpecificTimeMgr::Equinox equinox)
{
	double JDE0, Y;
	if (-1000<=year && year<=1000)
	{
		Y = year/1000.;
		if (equinox==Equinox::March) // March equinox
			JDE0 = 1721139.29189 + Y*(365242.13740 + Y*( 0.06134 + Y*( 0.00111 + Y*(-0.00071))));
		else // September equinox
			JDE0 = 1721325.70455 + Y*(365242.49558 + Y*(-0.11677 + Y*(-0.00297 + Y*( 0.00074))));
	}
	else if (1000<year && year<=3000)
	{
		Y = (year - 2000.)/1000.;
		if (equinox==Equinox::March) // March equinox
			JDE0 = 2451623.80984 + Y*(365242.37404 + Y*( 0.05169 + Y*(-0.00411 + Y*(-0.00057))));
		else // September equinox
			JDE0 = 2451810.21715 + Y*(365242.01767 + Y*(-0.11575 + Y*( 0.00337 + Y*( 0.00078))));
	}
	else
		return 0.0;

	const double T = (JDE0 - 2451545.0)/36525.;
	const double W = 35999.373*T - 2.47; // degrees!
	const double deltaLambda = 1 + 0.0334*cos(W*M_PI_180) + 0.0007*cos(2*W*M_PI_180);
	double S = 0.;
	for (int i=0; i<24; i++)
	{
		S += MeeusCoefficients[i][0]*cos((MeeusCoefficients[i][1] + MeeusCoefficients[i][2]*T)*M_PI_180);
	}

	return JDE0 + 0.00001*S/deltaLambda;
}

double SpecificTimeMgr::getSolstice(int year, SpecificTimeMgr::Solstice solstice)
{
	double JDE0, Y;
	if (-1000<=year && year<=1000)
	{
		Y = year/1000.;
		if (solstice==Solstice::June) // June solstice
			JDE0 = 1721233.25401 + Y*(365241.72562 + Y*(-0.05323 + Y*( 0.00907 + Y*( 0.00025))));
		else // December solstice
			JDE0 = 1721414.39987 + Y*(365242.88257 + Y*(-0.00769 + Y*(-0.00933 + Y*(-0.00006))));
	}
	else if (1000<year && year<=3000)
	{
		Y = (year - 2000.)/1000.;
		if (solstice==Solstice::June) // June solstice
			JDE0 = 2451716.56767 + Y*(365241.62603 + Y*( 0.00325 + Y*( 0.00888 + Y*(-0.00030))));
		else // December solstice
			JDE0 = 2451900.05952 + Y*(365242.74049 + Y*(-0.06223 + Y*(-0.00823 + Y*( 0.00032))));
	}
	else
		return 0.0;

	const double T = (JDE0 - 2451545.0)/36525.;
	const double W = 35999.373*T - 2.47; // degrees!
	const double deltaLambda = 1 + 0.0334*cos(W*M_PI_180) + 0.0007*cos(2*W*M_PI_180);
	double S = 0.;
	for (int i=0; i<24; i++)
	{
		S += MeeusCoefficients[i][0]*cos((MeeusCoefficients[i][1] + MeeusCoefficients[i][2]*T)*M_PI_180);
	}

	return JDE0 + 0.00001*S/deltaLambda;
}

void SpecificTimeMgr::currentMarchEquinox()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getEquinox(year, Equinox::March);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::nextMarchEquinox()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getEquinox(year + 1, Equinox::March);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::previousMarchEquinox()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getEquinox(year - 1, Equinox::March);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::currentSeptemberEquinox()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getEquinox(year, Equinox::September);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::nextSeptemberEquinox()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getEquinox(year + 1, Equinox::September);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::previousSeptemberEquinox()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getEquinox(year - 1, Equinox::September);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::currentJuneSolstice()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getSolstice(year, Solstice::June);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::nextJuneSolstice()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getSolstice(year + 1, Solstice::June);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::previousJuneSolstice()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getSolstice(year - 1, Solstice::June);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::currentDecemberSolstice()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getSolstice(year, Solstice::December);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::nextDecemberSolstice()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getSolstice(year + 1, Solstice::December);
	if (estime>0.)
		core->setJDE(estime);
}

void SpecificTimeMgr::previousDecemberSolstice()
{
	double JD = core->getJDE();
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	double estime = getSolstice(year - 1, Solstice::December);
	if (estime>0.)
		core->setJDE(estime);
}
