/*
 * Copyright (C) 2012 Ivan Marti-Vidal
 * Copyright (C) 2021 Thomas Heinrichs
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

#include "Observability.hpp"
#include "ObservabilityDialog.hpp"

//#include "Planet.hpp"
//#include "SolarSystem.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelObserver.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"

StelModule* ObservabilityStelPluginInterface::getStelModule() const
{
	return new Observability();
}

StelPluginInfo ObservabilityStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Observability);

	StelPluginInfo info;
	info.id = "Observability";
	info.displayedName = N_("Observability Analysis");
	info.authors = "Ivan Marti-Vidal (Onsala Space Observatory)"; // non-translatable field
	info.contact = "i.martividal@gmail.com";
	info.description = N_(
	  "Displays an analysis of a selected object's observability (rise, set, and transit times) for the "
	  "current date, as well as when it is observable through the year. An object is assumed to be "
	  "observable if it is above the horizon during a fraction of the night. Also included are the dates of "
	  "the largest separation from the Sun and acronychal and cosmical rising and setting. (Explanations are "
	  "provided in the 'About' tab of the plugin's configuration window.)");
	info.version = OBSERVABILITY_PLUGIN_VERSION;
	info.license = OBSERVABILITY_PLUGIN_LICENSE;
	return info;
}

Observability::Observability()
{
	setObjectName("Observability");
	config = StelApp::getInstance().getSettings();
	configDialog = new ObservabilityDialog();
	core = StelApp::getInstance().getCore();
}

Observability::~Observability()
{
	if (configDialog != nullptr) delete configDialog;
}

void Observability::init()
{
	loadSettings();

	StelAction* actionShow =
	  addAction("actionShow_Observability", N_("Observability"), N_("Observability"), "flagShowReport");
	addAction("actionShow_Observability_ConfigDialog", N_("Observability"),
	  N_("Observability configuration window"), configDialog, "visible", "");

	StelObjectMgr& app = StelApp::getInstance().getStelObjectMgr();
	
	connect(&app, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this,
	  SLOT(setSelectedObject(StelModule::StelModuleSelectAction)));


	connect(core, SIGNAL(locationChanged(const StelLocation&)), this,
	  SLOT(prepareCalcRiseSetTransit()));

	connect(core, SIGNAL(dateChanged(void)), this, SLOT(prepareCalcRiseSetTransit()));
	connect(core, SIGNAL(currentTimeZoneChanged(const QString&)), this, SLOT(prepareCalcRiseSetTransit()));

	Vec3d x = calculateRiseSetTransit(177.7420800, 42.333300, 71.083300, 41.7312900, 18.4409200);

	double deltaTransit = calcDeltaM(177.7420800, x[0], 42.333300, 71.083300, 40.6802100, 18.0476100,
	  41.7312900, 18.4409200, 42.7820400, 18.8274200, false, -0.5667000);

	/* Vec2i rise = calcTimeFromDayFraction(x[0]);
	Vec2i set = calcTimeFromDayFraction(x[1]);
	Vec2i transit = calcTimeFromDayFraction(x[2]); */
}

void Observability::setSelectedObject(StelModule::StelModuleSelectAction mode)
{
	if (mode == StelModuleSelectAction::RemoveFromSelection) return;

	StelObjectMgr& obj = StelApp::getInstance().getStelObjectMgr();

	if (obj.getSelectedObject().length() > 0)
	{
		prepareCalcRiseSetTransit();
	}
}

void Observability::draw(StelCore* core)
{
	StelObjectMgr& obj = StelApp::getInstance().getStelObjectMgr();
	if (obj.getSelectedObject().length() > 0)
	{
		const QString infoString =
			q_("Rise: ") + StelUtils::hoursToHmsStr(riseTransitSet[0], true) + "<br/>" +
			q_("Transit: ") + StelUtils::hoursToHmsStr(riseTransitSet[1], true) + "<br/>" + 
			q_("Set: ") + StelUtils::hoursToHmsStr(riseTransitSet[2], true) + "<br/>";
		//prepareCalcRiseSetTransit();
		obj.getSelectedObject()[0]->addToExtraInfoString(StelObject::Extra, infoString);
	}
}

void Observability::update(double x) {}

double Observability::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName) + 10.;
	return 0.;
}

bool Observability::configureGui(bool show)
{
	if (show) configDialog->setVisible(true);
	return true;
}

void Observability::prepareCalcRiseSetTransit()
{
	StelObjectMgr& obj = StelApp::getInstance().getStelObjectMgr();

	if (obj.getSelectedObject().length() > 0)
	{
		//const double jdStore = core->getJD();
		//int jd = jdStore;
		double siderialTime = core->getLocalSiderealTime();
		double siderialTimeDeg = siderialTime * M_180_PI;

		//core->setJD(jd);
		double rightAscension, declination;
		StelUtils::rectToSphe(
		  &rightAscension, &declination, obj.getSelectedObject()[0]->getEquinoxEquatorialPos(core));

		double rightAscensionDeg = rightAscension * M_180_PI;
		double declinationDeg = declination * M_180_PI;

		//core->setJD(jdStore);
		/* const double defaultJD = core->getJD();
		core->setJD(defaultJD - 1);
		double rightAscension0, declination0;
		StelUtils::rectToSphe(
		  &rightAscension0, &declination0, obj.getSelectedObject()[0]->getEquinoxEquatorialPos(core));

		core->setJD(defaultJD + 1);
		double rightAscension2, declination2;
		StelUtils::rectToSphe(
		  &rightAscension2, &declination2, obj.getSelectedObject()[0]->getEquinoxEquatorialPos(core));

		core->setJD(defaultJD);

		rightAscension = rightAscension * M_180_PI;
		declination = declination * M_180_PI;
		rightAscension0 = rightAscension0 * M_180_PI;
		declination0 = declination0 * M_180_PI;
		rightAscension2 = rightAscension2 * M_180_PI;
		declination2 = declination2 * M_180_PI; */

		
		//siderialTimeDeg = 177.74208000;
		const StelLocation& location = core->getCurrentObserver()->getCurrentLocation();
		Vec3d x = calculateRiseSetTransit(siderialTimeDeg, location.latitude, location.longitude, rightAscension, declination);
		/* x[0] += calcDeltaM(siderialTimeDeg, x[0], location.latitude, location.longitude, rightAscension0, declination0,
	  rightAscension, declination, rightAscension2, declination2, false, -0.5667000); */

		riseTransitSet[0] = calcTimeFromDayFraction(x[0]);
		riseTransitSet[1] = calcTimeFromDayFraction(x[2]);
		riseTransitSet[2] = calcTimeFromDayFraction(x[1]);
	}
}

/// <summary>
///
/// </summary>
/// <param name="latitude">observer's geographic latitude; + for northern, - for southern hemisphere</param>
/// <param name="longitude">observer's geographic longitude; + for west, - for east</param>
/// <param name="alpha1">right ascension of body yesterday</param>
/// <param name="delta1">declination of body yesterday</param>
/// <param name="alpha2">right ascension of body today</param>
/// <param name="delta2">declination of body today</param>
/// <param name="alpha3">right ascension of body tomorrow</param>
/// <param name="delta3">declination of body tomorrow</param>
Vector3<double> Observability::calculateRiseSetTransit(double siderialTime, double latitude, double longitude,
  /*double alpha1, double delta1,*/ double alpha2, double delta2/*, double alpha3, double delta3*/)
{
	// "standard" altitude, i.e. geometric altitude of the center of the body at rising or setting (for stars
	// and planets)
	const double h0 = -0.5667000;
	// Make western longitude positive
	longitude = -longitude;

	double cosH0 = (std::sin(h0 / M_180_PI) - std::sin(latitude / M_180_PI) * std::sin(delta2 / M_180_PI))
				   / std::cos(latitude / M_180_PI) * std::cos(delta2 / M_180_PI);

	// if cosH0 is not between -1 and 1, the object stays either above or below the horizon
	if (cosH0 < -1 || cosH0 > 1) return Vector3<double>(-1, -1, -1);

	double h1 = std::acos(cosH0) / M_PI_180;
	double transit = (alpha2 + longitude - siderialTime) / 360;
	transit = valueBetween0And1(transit);
	double rise = transit - h1 / 360;
	double set = transit + h1 / 360;

	rise = valueBetween0And1(rise);
	set = valueBetween0And1(set);

	return Vector3<double>(rise, set, transit);
}

double Observability::valueBetween0And1(double x)
{
	if (x > 1)
	{
		x -= 1;
	}
	else if (x < 0)
	{
		x += 1;
	}
	return x;
}

double Observability::calcTimeFromDayFraction(double fraction)
{
	// time zone adjustments
	fraction += core->getUTCOffset(core->getJD()) / 24;
	return fraction * 24;
}

double Observability::interpolation(double m, double y1, double y2, double y3)
{
	// m + deltaT / 86400
	double n = m + core->computeDeltaT(core->getJD()) / 86400;
	// y2 - y1
	double a = y2 - y1;
	// y3 - y2
	double b = y3 - y2;
	// b - a
	double c = b - a;
	return y2 + n / 2 * (a + b + n * c);
}

double Observability::calcDeltaM(double siderialTime, double m, double latitude, double longitude,
	double alpha1, double delta1, double alpha2, double delta2, double alpha3, double delta3,
	bool isTransit, double h0)
{
	double greenwichSiderialTime = ((siderialTime + 360.985647) * m) / M_180_PI;
	//greenwichSiderialTime = 4.79401;
	double rightAscension = interpolation(m, alpha1, alpha2, alpha3);
	double localeHourAngle = greenwichSiderialTime - longitude - rightAscension;
	
	if (isTransit) return -(localeHourAngle / 360);

	double declination = interpolation(m, delta1, delta2, delta3);
	double altitude = std::asin(std::sin(latitude / M_180_PI) * std::sin(declination)
				+ std::cos(latitude / M_180_PI) * std::cos(declination) * std::cos(localeHourAngle));

	return (altitude - h0)
		   / (360 * std::cos(declination) * std::cos(latitude / M_180_PI) * std::sin(localeHourAngle));
}

void Observability::showGoodNights(bool b)
{
	flagShowGoodNights = b;
	// save setting
	config->setValue("Observability/show_good_nights", flagShowGoodNights);
	emit flagShowGoodNightsChanged(b);
}

void Observability::showBestNight(bool b)
{
	flagShowBestNight = b;
	// save setting
	config->setValue("Observability/show_best_night", flagShowBestNight);
	emit flagShowBestNightChanged(b);
}

void Observability::showToday(bool b)
{
	flagShowToday = b;
	// save setting
	config->setValue("Observability/show_today", flagShowToday);
	emit flagShowTodayChanged(b);
}

void Observability::showFullMoon(bool b)
{
	flagShowFullMoon = b;
	// save setting
	config->setValue("Observability/show_full_moon", flagShowFullMoon);
	emit flagShowFullMoonChanged(b);
}

void Observability::showAcroCos(bool b)
{
	flagShowAcroCos = b;
	// save setting
	config->setValue("Observability/show_acro_cos", flagShowAcroCos);
	emit flagShowAcroCosChanged(b);
}

void Observability::setTwilightAltitudeDeg(int alt)
{
	twilightAltitudeDeg = alt;
	// save setting
	config->setValue("Observability/twilight_altitude", twilightAltitudeDeg);
	emit twilightAltitudeDegChanged(alt);
}

void Observability::setHorizonAltitudeDeg(int alt)
{
	horizonAltitudeDeg = alt;
	// save setting
	config->setValue("Observability/horizon_altitude", horizonAltitudeDeg);
	emit horizonAltitudeDegChanged(alt);
}

void Observability::enableReport(bool b)
{
	if (b != flagShowReport)
	{
		flagShowReport = b;
		emit flagReportVisibilityChanged(b);
	}
}

void Observability::restoreDefaultSettings()
{
	Q_ASSERT(config);
	// Remove the old values
	config->remove("Observability");
	// load default settings
	loadSettings();

	config->beginGroup("Observability");

	config->endGroup();
}

void Observability::loadSettings()
{
	showGoodNights(config->value("Observability/show_good_nights", true).toBool());
	showBestNight(config->value("Observability/show_best_night", true).toBool());
	showToday(config->value("Observability/show_today", true).toBool());
	showFullMoon(config->value("Observability/show_full_moon", true).toBool());
	showAcroCos(config->value("Observability/show_acro_cos", true).toBool());
	setTwilightAltitudeDeg(config->value("Observability/twilight_altitude", 0).toInt());
	setHorizonAltitudeDeg(config->value("Observability/horizon_altitude", 0).toInt());
}