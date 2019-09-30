/*
 * Stellarium
 * Copyright (C) 2016 Alexander Wolf
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPropertyMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "ConfigureOrbitColorsDialog.hpp"
#include "ui_orbitColorsDialog.h"

#include <QSettings>
#include <QColorDialog>

ConfigureOrbitColorsDialog::ConfigureOrbitColorsDialog() : StelDialog("ConfigureOrbitColorsDialog")
{
	ui = new Ui_ConfigureOrbitColorsDialogForm;
}

ConfigureOrbitColorsDialog::~ConfigureOrbitColorsDialog()
{
	delete ui;
}

void ConfigureOrbitColorsDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}


void ConfigureOrbitColorsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	QString activeColorStyle = StelApp::getInstance().getModule("SolarSystem")->property("orbitColorStyle").toString();

	if (activeColorStyle=="groups")
		ui->groupsRadioButton->setChecked(true);
	else if (activeColorStyle=="major_planets")
		ui->majorPlanetsRadioButton->setChecked(true);
	else
		ui->oneColorRadioButton->setChecked(true);
	connect(ui->oneColorRadioButton, SIGNAL(clicked(bool)), this, SLOT(setColorStyle()));
	connect(ui->groupsRadioButton, SIGNAL(clicked(bool)), this, SLOT(setColorStyle()));
	connect(ui->majorPlanetsRadioButton, SIGNAL(clicked(bool)), this, SLOT(setColorStyle()));

	connectColorButton(ui->colorGenericOrbits,             "SolarSystem.orbitsColor",                     "color/sso_orbits_color");
	connectColorButton(ui->colorGroupsMajorPlanetsOrbits,  "SolarSystem.majorPlanetsOrbitsColor",         "color/major_planet_orbits_color");
	connectColorButton(ui->colorGroupsMinorPlanetsOrbits,  "SolarSystem.minorPlanetsOrbitsColor",         "color/minor_planet_orbits_color");
	connectColorButton(ui->colorGroupsDwarfPlanetsOrbits,  "SolarSystem.dwarfPlanetsOrbitsColor",         "color/dwarf_planet_orbits_color");
	connectColorButton(ui->colorGroupsMoonsOrbits,         "SolarSystem.moonsOrbitsColor",                "color/moon_orbits_color");
	connectColorButton(ui->colorGroupsCubewanosOrbits,     "SolarSystem.cubewanosOrbitsColor",            "color/cubewano_orbits_color");
	connectColorButton(ui->colorGroupsPlutinosOrbits,      "SolarSystem.plutinosOrbitsColor",             "color/plutino_orbits_color");
	connectColorButton(ui->colorGroupsSDOOrbits,           "SolarSystem.scatteredDiskObjectsOrbitsColor", "color/sdo_orbits_color");
	connectColorButton(ui->colorGroupsOCOOrbits,           "SolarSystem.oortCloudObjectsOrbitsColor",     "color/oco_orbits_color");
	connectColorButton(ui->colorGroupsCometsOrbits,        "SolarSystem.cometsOrbitsColor",               "color/comet_orbits_color");
	connectColorButton(ui->colorGroupsSednoidsOrbits,      "SolarSystem.sednoidsOrbitsColor",             "color/sednoid_orbits_color");
	connectColorButton(ui->colorGroupsInterstellarOrbits,  "SolarSystem.interstellarOrbitsColor",         "color/interstellar_orbits_color");
	connectColorButton(ui->colorMPMercuryOrbit,            "SolarSystem.mercuryOrbitColor",               "color/mercury_orbit_color");
	connectColorButton(ui->colorMPVenusOrbit,              "SolarSystem.venusOrbitColor",                 "color/venus_orbit_color");
	connectColorButton(ui->colorMPEarthOrbit,              "SolarSystem.earthOrbitColor",                 "color/earth_orbit_color");
	connectColorButton(ui->colorMPMarsOrbit,               "SolarSystem.marsOrbitColor",                  "color/mars_orbit_color");
	connectColorButton(ui->colorMPJupiterOrbit,            "SolarSystem.jupiterOrbitColor",               "color/jupiter_orbit_color");
	connectColorButton(ui->colorMPSaturnOrbit,             "SolarSystem.saturnOrbitColor",                "color/saturn_orbit_color");
	connectColorButton(ui->colorMPUranusOrbit,             "SolarSystem.uranusOrbitColor",                "color/uranus_orbit_color");
	connectColorButton(ui->colorMPNeptuneOrbit,            "SolarSystem.neptuneOrbitColor",               "color/neptune_orbit_color");
}

void ConfigureOrbitColorsDialog::setColorStyle()
{
	QString colorStyle;
	if (ui->majorPlanetsRadioButton->isChecked())
		colorStyle = "major_planets";
	else if (ui->groupsRadioButton->isChecked())
		colorStyle = "groups";
	else
		colorStyle = "one_color";

	StelApp::getInstance().getModule("SolarSystem")->setProperty("orbitColorStyle", colorStyle);
	StelApp::getInstance().getSettings()->setValue("astro/planets_orbits_color_style", colorStyle);
}
