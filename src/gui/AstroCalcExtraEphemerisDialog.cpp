/*
 * Stellarium
 * Copyright (C) 2019 Alexander Wolf
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

#include "AstroCalcExtraEphemerisDialog.hpp"
#include "ui_astroCalcExtraEphemerisDialog.h"

#include "Dialog.hpp"
#include "StelApp.hpp"

AstroCalcExtraEphemerisDialog::AstroCalcExtraEphemerisDialog() : StelDialog("AstroCalcExtraEphemeris")
{
	ui = new Ui_astroCalcExtraEphemerisDialogForm;
	conf = StelApp::getInstance().getSettings();
}

AstroCalcExtraEphemerisDialog::~AstroCalcExtraEphemerisDialog()
{
	delete ui;
	ui=Q_NULLPTR;
}

void AstroCalcExtraEphemerisDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);		
	}
}

void AstroCalcExtraEphemerisDialog::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->skipDataCheckBox, SIGNAL(clicked()), this, SLOT(setOptionStatus()));
	connect(ui->smartDatesRadio,  SIGNAL(toggled(bool)), this, SLOT(setOptionStatus()));
	connect(ui->customDateRadio,  SIGNAL(toggled(bool)), this, SLOT(setOptionStatus()));
	connect(ui->firstOfMonthOnlyCheckBox, SIGNAL(toggled(bool)), this, SLOT(setOptionStatus()));
	connect(ui->antiClutterCheckBox, SIGNAL(toggled(bool)), this, SLOT(setOptionStatus()));

	connectBoolProperty(ui->skipDataCheckBox,	"SolarSystem.ephemerisSkippedData");
	connectBoolProperty(ui->skipMarkersCheckBox,	"SolarSystem.ephemerisSkippedMarkers");
	connectIntProperty(ui->dataStepSpinBox,		"SolarSystem.ephemerisDataStep");
	// Date format radio buttons — QRadioButton inherits QAbstractButton, so
	// connectBoolProperty works directly. smartDatesRadio maps to the smart-dates
	// property (true = smart on); customDateRadio is the logical inverse and is
	// driven automatically by Qt's radio-button exclusivity within the group box.
	connectBoolProperty(ui->smartDatesRadio, "SolarSystem.ephemerisSmartDates");
	connectBoolProperty(ui->scaleMarkersCheckBox,	"SolarSystem.ephemerisScaleMarkersDisplayed");
	connectBoolProperty(ui->alwaysOnCheckBox,	"SolarSystem.ephemerisAlwaysOn");
	connectBoolProperty(ui->currentLocationCheckBox,"SolarSystem.ephemerisNow" );
	connectIntProperty(ui->lineThicknessSpinBox,	"SolarSystem.ephemerisLineThickness");

	// Label date component checkboxes
	connectBoolProperty(ui->labelYearCheckBox,	"SolarSystem.ephemerisLabelYear");
	connectBoolProperty(ui->labelMonthCheckBox,	"SolarSystem.ephemerisLabelMonth");
	connectBoolProperty(ui->labelDayCheckBox,	"SolarSystem.ephemerisLabelDay");
	connectBoolProperty(ui->labelHourCheckBox,	"SolarSystem.ephemerisLabelHour");
	connectBoolProperty(ui->labelMinuteCheckBox,	"SolarSystem.ephemerisLabelMinute");
	connectBoolProperty(ui->labelSecondCheckBox,	"SolarSystem.ephemerisLabelSecond");

	// Anti-clutter
	connectBoolProperty(ui->antiClutterCheckBox,	"SolarSystem.ephemerisLabelAntiClutter");
	connectIntProperty(ui->antiClutterSpinBox,	"SolarSystem.ephemerisLabelAntiClutterPx");

	// First-of-month-only
	connectBoolProperty(ui->firstOfMonthOnlyCheckBox, "SolarSystem.ephemerisFirstOfMonthOnly");

	// Sun at altitude settings (stored directly in conf, read by AstroCalcDialog::generateEphemeris)
	// Save defaults to conf immediately so generateEphemeris() can read them even if user doesn't touch the spinbox
	const double savedAlt = conf->value("astrocalc/ephemeris_sun_altitude", -10.0).toDouble();
	ui->sunAltitudeSpinBox->setValue(savedAlt);
	conf->setValue("astrocalc/ephemeris_sun_altitude", savedAlt);

	// Populate the crossing combobox: 0 = Evening, 1 = Morning
	ui->sunAltCrossingComboBox->addItem(q_("Evening"), 0);
	ui->sunAltCrossingComboBox->addItem(q_("Morning"), 1);
	const int savedCrossing = conf->value("astrocalc/ephemeris_sun_altitude_evening", 0).toInt();
	ui->sunAltCrossingComboBox->setCurrentIndex(savedCrossing);
	conf->setValue("astrocalc/ephemeris_sun_altitude_evening", savedCrossing);
	connect(ui->sunAltitudeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(saveSunAltitude(double)));
	connect(ui->sunAltCrossingComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveSunAltitudeCrossing(int)));

	// Opposition planet combobox
	ui->oppositionPlanetComboBox->addItem(q_("Mars"),    "Mars");
	ui->oppositionPlanetComboBox->addItem(q_("Jupiter"), "Jupiter");
	ui->oppositionPlanetComboBox->addItem(q_("Saturn"),  "Saturn");
	ui->oppositionPlanetComboBox->addItem(q_("Uranus"),  "Uranus");
	ui->oppositionPlanetComboBox->addItem(q_("Neptune"), "Neptune");
	const QString savedOppPlanet = conf->value("astrocalc/ephemeris_opposition_planet", "Mars").toString();
	int oppIdx = ui->oppositionPlanetComboBox->findData(savedOppPlanet);
	if (oppIdx < 0) oppIdx = 0;
	ui->oppositionPlanetComboBox->setCurrentIndex(oppIdx);
	connect(ui->oppositionPlanetComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveOppositionPlanet(int)));

	setOptionStatus();
}

void AstroCalcExtraEphemerisDialog::setOptionStatus()
{
	ui->skipMarkersCheckBox->setEnabled(ui->skipDataCheckBox->isChecked());

	// When "smart dates" radio is on, the per-component checkboxes are not used
	const bool smartDates = ui->smartDatesRadio->isChecked();
	const bool firstOfMonth = ui->firstOfMonthOnlyCheckBox->isChecked();
	const bool enableComponents = !smartDates && !firstOfMonth;
	ui->labelYearCheckBox->setEnabled(enableComponents);
	ui->labelMonthCheckBox->setEnabled(enableComponents);
	ui->labelDayCheckBox->setEnabled(enableComponents);
	ui->labelHourCheckBox->setEnabled(enableComponents);
	ui->labelMinuteCheckBox->setEnabled(enableComponents);
	ui->labelSecondCheckBox->setEnabled(enableComponents);

	// Anti-clutter spinbox enabled only when anti-clutter checkbox is checked
	ui->antiClutterSpinBox->setEnabled(ui->antiClutterCheckBox->isChecked());
}

void AstroCalcExtraEphemerisDialog::saveSunAltitude(double alt)
{
	conf->setValue("astrocalc/ephemeris_sun_altitude", alt);
}

void AstroCalcExtraEphemerisDialog::saveSunAltitudeCrossing(int index)
{
	conf->setValue("astrocalc/ephemeris_sun_altitude_evening", index);
}

void AstroCalcExtraEphemerisDialog::saveOppositionPlanet(int index)
{
	conf->setValue("astrocalc/ephemeris_opposition_planet",
	               ui->oppositionPlanetComboBox->itemData(index).toString());
}
