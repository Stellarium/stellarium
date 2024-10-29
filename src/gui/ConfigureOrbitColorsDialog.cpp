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
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	QString activeColorStyle = StelApp::getInstance().getModule("SolarSystem")->property("orbitColorStyle").toString();

	if (activeColorStyle=="groups")
		ui->groupsRadioButton->setChecked(true);
	else if (activeColorStyle=="major_planets")
		ui->majorPlanetsRadioButton->setChecked(true);
	else if (activeColorStyle=="major_planets_minor_types")
		ui->majorPlanetsAndMinorTypeRadioButton->setChecked(true);
	else
		ui->oneColorRadioButton->setChecked(true);
	connect(ui->oneColorRadioButton, SIGNAL(clicked(bool)), this, SLOT(setColorStyle()));
	connect(ui->groupsRadioButton, SIGNAL(clicked(bool)), this, SLOT(setColorStyle()));
	connect(ui->majorPlanetsRadioButton, SIGNAL(clicked(bool)), this, SLOT(setColorStyle()));
	connect(ui->majorPlanetsAndMinorTypeRadioButton, SIGNAL(clicked(bool)), this, SLOT(setColorStyle()));

	ui->colorGenericOrbits           ->setup("SolarSystem.orbitsColor",                     "color/sso_orbits_color");
	ui->colorGroupsMajorPlanetsOrbits->setup("SolarSystem.majorPlanetsOrbitsColor",         "color/major_planet_orbits_color");
	ui->colorGroupsMinorPlanetsOrbits->setup("SolarSystem.minorPlanetsOrbitsColor",         "color/minor_planet_orbits_color");
	ui->colorGroupsDwarfPlanetsOrbits->setup("SolarSystem.dwarfPlanetsOrbitsColor",         "color/dwarf_planet_orbits_color");
	ui->colorGroupsMoonsOrbits       ->setup("SolarSystem.moonsOrbitsColor",                "color/moon_orbits_color");
	ui->colorGroupsCubewanosOrbits   ->setup("SolarSystem.cubewanosOrbitsColor",            "color/cubewano_orbits_color");
	ui->colorGroupsPlutinosOrbits    ->setup("SolarSystem.plutinosOrbitsColor",             "color/plutino_orbits_color");
	ui->colorGroupsSDOOrbits         ->setup("SolarSystem.scatteredDiskObjectsOrbitsColor", "color/sdo_orbits_color");
	ui->colorGroupsOCOOrbits         ->setup("SolarSystem.oortCloudObjectsOrbitsColor",     "color/oco_orbits_color");
	ui->colorGroupsCometsOrbits      ->setup("SolarSystem.cometsOrbitsColor",               "color/comet_orbits_color");
	ui->colorGroupsSednoidsOrbits    ->setup("SolarSystem.sednoidsOrbitsColor",             "color/sednoid_orbits_color");
	ui->colorGroupsInterstellarOrbits->setup("SolarSystem.interstellarOrbitsColor",         "color/interstellar_orbits_color");
	ui->colorMPMercuryOrbit          ->setup("SolarSystem.mercuryOrbitColor",               "color/mercury_orbit_color");
	ui->colorMPVenusOrbit            ->setup("SolarSystem.venusOrbitColor",                 "color/venus_orbit_color");
	ui->colorMPEarthOrbit            ->setup("SolarSystem.earthOrbitColor",                 "color/earth_orbit_color");
	ui->colorMPMarsOrbit             ->setup("SolarSystem.marsOrbitColor",                  "color/mars_orbit_color");
	ui->colorMPJupiterOrbit          ->setup("SolarSystem.jupiterOrbitColor",               "color/jupiter_orbit_color");
	ui->colorMPSaturnOrbit           ->setup("SolarSystem.saturnOrbitColor",                "color/saturn_orbit_color");
	ui->colorMPUranusOrbit           ->setup("SolarSystem.uranusOrbitColor",                "color/uranus_orbit_color");
	ui->colorMPNeptuneOrbit          ->setup("SolarSystem.neptuneOrbitColor",               "color/neptune_orbit_color");
}

void ConfigureOrbitColorsDialog::setColorStyle()
{
	QString colorStyle;
	if (ui->majorPlanetsRadioButton->isChecked())
		colorStyle = "major_planets";
	else if (ui->majorPlanetsAndMinorTypeRadioButton->isChecked())
		colorStyle = "major_planets_minor_types";
	else if (ui->groupsRadioButton->isChecked())
		colorStyle = "groups";
	else
		colorStyle = "one_color";

	StelApp::getInstance().getModule("SolarSystem")->setProperty("orbitColorStyle", colorStyle);
	StelApp::getInstance().getSettings()->setValue("astro/planets_orbits_color_style", colorStyle);
}
