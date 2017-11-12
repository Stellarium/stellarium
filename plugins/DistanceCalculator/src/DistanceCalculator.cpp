/*
 * Distance Calculator plug-in for Stellarium
 *
 * Copyright (C) 2017 Alexander Wolf
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

#include "DistanceCalculator.hpp"
#include "DistanceCalculatorWindow.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelActionMgr.hpp"

#include <QSettings>

StelModule* DistanceCalculatorStelPluginInterface::getStelModule() const
{
	return new DistanceCalculator();
}

StelPluginInfo DistanceCalculatorStelPluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "DistanceCalculator";
	info.displayedName = N_("Distance Calculator");
	info.authors = "Alexander Wolf";
	info.contact = "http://stellarium.org";
	info.description = N_("This plugin allows measure linear distance between Solar system bodies.");
	info.version = DISTANCECALCULATOR_PLUGIN_VERSION;
	info.license = DISTANCECALCULATOR_PLUGIN_LICENSE;
	return info;
}

DistanceCalculator::DistanceCalculator()
	: firstCelestialBody("Sun")
	, secondCelestialBody("Earth")
{
	setObjectName("DistanceCalculator");
	distanceCalculatorDialog = new DistanceCalculatorWindow();
	conf = StelApp::getInstance().getSettings();
}

DistanceCalculator::~DistanceCalculator()
{
	delete distanceCalculatorDialog;
}

void DistanceCalculator::init()
{
	if (!conf->childGroups().contains("DistanceCalculator"))
	{
		qDebug() << "Distance Calculator: no DistanceCalculator section exists in main config file - creating with defaults";
		restoreDefaultConfigIni();
	}

	// populate settings from main config file.
	readSettingsFromConfig();

	addAction("actionShow_Distance_Calculator", N_("Distance Calculator"), N_("Open the distance calculator"), distanceCalculatorDialog, "visible", "Ctrl+Alt+C");
}

void DistanceCalculator::deinit()
{
	//
}

double DistanceCalculator::getCallOrder(StelModuleActionName) const
{
	return 0.;
}

void DistanceCalculator::update(double)
{
	//
}

bool DistanceCalculator::configureGui(bool show)
{
	if (show)
	{
		distanceCalculatorDialog->setVisible(true);
	}

	return true;
}

void DistanceCalculator::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	readSettingsFromConfig();
}

void DistanceCalculator::restoreDefaultConfigIni(void)
{
	conf->beginGroup("DistanceCalculator");

	// delete all existing Distance settings...
	conf->remove("");

	conf->setValue("first_celestial_body", "Sun");
	conf->setValue("second_celestial_body", "Earth");

	conf->endGroup();
}

void DistanceCalculator::readSettingsFromConfig(void)
{
	conf->beginGroup("DistanceCalculator");

	setFirstCelestialBody(conf->value("first_celestial_body", "Sun").toString());
	setSecondCelestialBody(conf->value("second_celestial_body", "Earth").toString());

	conf->endGroup();
}

void DistanceCalculator::saveSettingsToConfig(void)
{
	conf->beginGroup("DistanceCalculator");

	conf->setValue("first_celestial_body", getFirstCelestialBody());
	conf->setValue("second_celestial_body", getSecondCelestialBody());

	conf->endGroup();
}


