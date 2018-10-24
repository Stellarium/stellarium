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

	colorButton(ui->colorGenericOrbits,		"SolarSystem.orbitsColor");
	colorButton(ui->colorGroupsMajorPlanetsOrbits,	"SolarSystem.majorPlanetsOrbitsColor");
	colorButton(ui->colorGroupsMinorPlanetsOrbits,	"SolarSystem.minorPlanetsOrbitsColor");
	colorButton(ui->colorGroupsDwarfPlanetsOrbits,	"SolarSystem.dwarfPlanetsOrbitsColor");
	colorButton(ui->colorGroupsMoonsOrbits,		"SolarSystem.moonsOrbitsColor");
	colorButton(ui->colorGroupsCubewanosOrbits,	"SolarSystem.cubewanosOrbitsColor");
	colorButton(ui->colorGroupsPlutinosOrbits,	"SolarSystem.plutinosOrbitsColor");
	colorButton(ui->colorGroupsSDOOrbits,		"SolarSystem.scatteredDiskObjectsOrbitsColor");
	colorButton(ui->colorGroupsOCOOrbits,		"SolarSystem.oortCloudObjectsOrbitsColor");
	colorButton(ui->colorGroupsCometsOrbits,	"SolarSystem.cometsOrbitsColor");
	colorButton(ui->colorGroupsSednoidsOrbits,	"SolarSystem.sednoidsOrbitsColor");
	colorButton(ui->colorGroupsInterstellarOrbits,	"SolarSystem.interstellarOrbitsColor");
	colorButton(ui->colorMPMercuryOrbit,		"SolarSystem.mercuryOrbitColor");
	colorButton(ui->colorMPVenusOrbit,		"SolarSystem.venusOrbitColor");
	colorButton(ui->colorMPEarthOrbit,		"SolarSystem.earthOrbitColor");
	colorButton(ui->colorMPMarsOrbit,		"SolarSystem.marsOrbitColor");
	colorButton(ui->colorMPJupiterOrbit,		"SolarSystem.jupiterOrbitColor");
	colorButton(ui->colorMPSaturnOrbit,		"SolarSystem.saturnOrbitColor");
	colorButton(ui->colorMPUranusOrbit,		"SolarSystem.uranusOrbitColor");
	colorButton(ui->colorMPNeptuneOrbit,		"SolarSystem.neptuneOrbitColor");

	connect(ui->colorGenericOrbits,			SIGNAL(released()), this, SLOT(askGenericOrbitColor()));
	connect(ui->colorGroupsMajorPlanetsOrbits,	SIGNAL(released()), this, SLOT(askMajorPlanetsGroupOrbitColor()));
	connect(ui->colorGroupsMinorPlanetsOrbits,	SIGNAL(released()), this, SLOT(askMinorPlanetsGroupOrbitColor()));
	connect(ui->colorGroupsDwarfPlanetsOrbits,	SIGNAL(released()), this, SLOT(askDwarfPlanetsGroupOrbitColor()));
	connect(ui->colorGroupsMoonsOrbits,		SIGNAL(released()), this, SLOT(askMoonsGroupOrbitColor()));
	connect(ui->colorGroupsCubewanosOrbits,		SIGNAL(released()), this, SLOT(askCubewanosGroupOrbitColor()));
	connect(ui->colorGroupsPlutinosOrbits,		SIGNAL(released()), this, SLOT(askPlutinosGroupOrbitColor()));
	connect(ui->colorGroupsSDOOrbits,		SIGNAL(released()), this, SLOT(askSDOGroupOrbitColor()));
	connect(ui->colorGroupsOCOOrbits,		SIGNAL(released()), this, SLOT(askOCOGroupOrbitColor()));
	connect(ui->colorGroupsCometsOrbits,		SIGNAL(released()), this, SLOT(askCometsGroupOrbitColor()));
	connect(ui->colorGroupsSednoidsOrbits,		SIGNAL(released()), this, SLOT(askSednoidsGroupOrbitColor()));
	connect(ui->colorGroupsInterstellarOrbits,	SIGNAL(released()), this, SLOT(askInterstellarGroupOrbitColor()));
	connect(ui->colorMPMercuryOrbit,		SIGNAL(released()), this, SLOT(askMercuryOrbitColor()));
	connect(ui->colorMPVenusOrbit,			SIGNAL(released()), this, SLOT(askVenusOrbitColor()));
	connect(ui->colorMPEarthOrbit,			SIGNAL(released()), this, SLOT(askEarthOrbitColor()));
	connect(ui->colorMPMarsOrbit,			SIGNAL(released()), this, SLOT(askMarsOrbitColor()));
	connect(ui->colorMPJupiterOrbit,		SIGNAL(released()), this, SLOT(askJupiterOrbitColor()));
	connect(ui->colorMPSaturnOrbit,			SIGNAL(released()), this, SLOT(askSaturnOrbitColor()));
	connect(ui->colorMPUranusOrbit,			SIGNAL(released()), this, SLOT(askUranusOrbitColor()));
	connect(ui->colorMPNeptuneOrbit,		SIGNAL(released()), this, SLOT(askNeptuneOrbitColor()));
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


void ConfigureOrbitColorsDialog::colorButton(QToolButton* toolButton, QString propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Vec3f vColor = prop->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	// Use style sheet for create a nice buttons :)
	toolButton->setStyleSheet("QToolButton { background-color:" + color.name() + "; }");
	toolButton->setFixedSize(QSize(18, 18));
}

void ConfigureOrbitColorsDialog::askGenericOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.orbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGenericOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("orbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/sso_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGenericOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askMajorPlanetsGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.majorPlanetsOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsMajorPlanetsOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("majorPlanetsOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/major_planet_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsMajorPlanetsOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askMinorPlanetsGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.minorPlanetsOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsMinorPlanetsOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("minorPlanetsOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/minor_planet_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsMinorPlanetsOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askDwarfPlanetsGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.dwarfPlanetsOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsDwarfPlanetsOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("dwarfPlanetsOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/dwarf_planet_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsDwarfPlanetsOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askCometsGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.cometsOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsCometsOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("cometsOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/comet_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsCometsOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askSednoidsGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.sednoidsOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsSednoidsOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("sednoidsOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/sednoid_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsSednoidsOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askInterstellarGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.interstellarOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsInterstellarOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("interstellarOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/interstellar_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsInterstellarOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askMoonsGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.moonsOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsMoonsOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("moonsOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/moon_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsMoonsOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askCubewanosGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.cubewanosOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsCubewanosOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("cubewanosOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/cubewano_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsCubewanosOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askPlutinosGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.plutinosOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsPlutinosOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("plutinosOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/plutino_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsPlutinosOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askSDOGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.scatteredDiskObjectsOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsSDOOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("scatteredDiskObjectsOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/sdo_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsSDOOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askOCOGroupOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.oortCloudObjectsOrbitsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGroupsOCOOrbits->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("oortCloudObjectsOrbitsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/oco_orbits_color", StelUtils::vec3fToStr(vColor));
		ui->colorGroupsOCOOrbits->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askMercuryOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.mercuryOrbitColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorMPMercuryOrbit->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("mercuryOrbitColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/mercury_orbit_color", StelUtils::vec3fToStr(vColor));
		ui->colorMPMercuryOrbit->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askVenusOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.venusOrbitColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorMPVenusOrbit->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("venusOrbitColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/venus_orbit_color", StelUtils::vec3fToStr(vColor));
		ui->colorMPVenusOrbit->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askEarthOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.earthOrbitColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorMPEarthOrbit->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("earthOrbitColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/earth_orbit_color", StelUtils::vec3fToStr(vColor));
		ui->colorMPEarthOrbit->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askMarsOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.marsOrbitColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorMPMarsOrbit->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("marsOrbitColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/mars_orbit_color", StelUtils::vec3fToStr(vColor));
		ui->colorMPMarsOrbit->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askJupiterOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.jupiterOrbitColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorMPJupiterOrbit->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("jupiterOrbitColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/jupiter_orbit_color", StelUtils::vec3fToStr(vColor));
		ui->colorMPJupiterOrbit->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askSaturnOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.saturnOrbitColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorMPSaturnOrbit->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("saturnOrbitColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/saturn_orbit_color", StelUtils::vec3fToStr(vColor));
		ui->colorMPSaturnOrbit->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askUranusOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.uranusOrbitColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorMPUranusOrbit->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("uranusOrbitColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/uranus_orbit_color", StelUtils::vec3fToStr(vColor));
		ui->colorMPUranusOrbit->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ConfigureOrbitColorsDialog::askNeptuneOrbitColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.neptuneOrbitColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorMPNeptuneOrbit->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("neptuneOrbitColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/neptune_orbit_color", StelUtils::vec3fToStr(vColor));
		ui->colorMPNeptuneOrbit->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}
