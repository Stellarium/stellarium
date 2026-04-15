/*
 * Stellarium
 * Copyright (C) 2020 Alexander Wolf
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
#include "Dialog.hpp"
#include "StelTranslator.hpp"
#include "StelObjectMgr.hpp"

#include "AstroCalcCustomStepsDialog.hpp"
#include "ui_astroCalcCustomStepsDialog.h"

AstroCalcCustomStepsDialog::AstroCalcCustomStepsDialog() : StelDialog("AstroCalcCustomSteps")
{
	ui = new Ui_astroCalcCustomStepsDialogForm;
	conf = StelApp::getInstance().getSettings();
}

AstroCalcCustomStepsDialog::~AstroCalcCustomStepsDialog()
{
	delete ui;
	ui=Q_NULLPTR;
}

void AstroCalcCustomStepsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		populateUnitMeasurementsList();
	}
}


void AstroCalcCustomStepsDialog::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	ui->timeStepDoubleSpinBox->setValue(conf->value("astrocalc/custom_time_step", 1.0).toDouble());
	connect(ui->timeStepDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(saveTimeStep(double)));

	populateUnitMeasurementsList();
	connect(ui->unitMeasurementComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveUnitMeasurement(int)));

	// Sun at altitude settings
	const double savedAlt = conf->value("astrocalc/ephemeris_sun_altitude", -10.0).toDouble();
	ui->sunAltitudeSpinBox->setValue(savedAlt);
	conf->setValue("astrocalc/ephemeris_sun_altitude", savedAlt);

	ui->sunAltCrossingComboBox->addItem(q_("Evening"), 0);
	ui->sunAltCrossingComboBox->addItem(q_("Morning"), 1);
	const int savedCrossing = conf->value("astrocalc/ephemeris_sun_altitude_evening", 0).toInt();
	ui->sunAltCrossingComboBox->setCurrentIndex(savedCrossing);
	conf->setValue("astrocalc/ephemeris_sun_altitude_evening", savedCrossing);
	connect(ui->sunAltitudeSpinBox,    SIGNAL(valueChanged(double)), this, SLOT(saveSunAltitude(double)));
	connect(ui->sunAltCrossingComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveSunAltitudeCrossing(int)));

	// Opposition planet settings
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

	// Both conditional group boxes start hidden; caller must invoke setActiveTimeStep()
	ui->sunAtAltitudeGroupBox->setVisible(false);
	ui->oppositionGroupBox->setVisible(false);
}

void AstroCalcCustomStepsDialog::populateUnitMeasurementsList()
{
	Q_ASSERT(ui->unitMeasurementComboBox);

	QComboBox* steps = ui->unitMeasurementComboBox;

	steps->blockSignals(true);
	int index = steps->currentIndex();
	QVariant selectedCategoryId = steps->itemData(index);
	steps->clear();
	// NOTE: Update AstroCalc::getCustomTimeStep() after changes!
	steps->addItem(qc_("minute","unit of measurement"), "1");
	steps->addItem(qc_("hour","unit of measurement"), "2");
	steps->addItem(qc_("solar day","unit of measurement"), "3");
	steps->addItem(qc_("sidereal day","unit of measurement"), "4");
	steps->addItem(qc_("Julian day","unit of measurement"), "5");
	steps->addItem(qc_("synodic month","unit of measurement"), "6");
	steps->addItem(qc_("draconic month","unit of measurement"), "7");
	steps->addItem(qc_("mean tropical month","unit of measurement"), "8");
	steps->addItem(qc_("anomalistic month","unit of measurement"), "9");
	steps->addItem(qc_("sidereal year","unit of measurement"), "10");
	steps->addItem(qc_("Julian year","unit of measurement"), "11");
	steps->addItem(qc_("Gaussian year","unit of measurement"), "12");
	steps->addItem(qc_("anomalistic year","unit of measurement"), "13");
	steps->addItem(qc_("saros","unit of measurement"), "14");

	index = steps->findData(selectedCategoryId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index < 0) // read config data
		index = steps->findData(conf->value("astrocalc/custom_time_step_unit", "3").toString(), Qt::UserRole, Qt::MatchCaseSensitive);

	if (index < 0) // Unknown yet? Default step: solar day
		index = steps->findData("3", Qt::UserRole, Qt::MatchCaseSensitive);

	steps->setCurrentIndex(index);
	steps->model()->sort(0);
	steps->blockSignals(false);
}

void AstroCalcCustomStepsDialog::saveUnitMeasurement(int index)
{
	Q_ASSERT(ui->unitMeasurementComboBox);
	QComboBox* category = ui->unitMeasurementComboBox;
	conf->setValue("astrocalc/custom_time_step_unit", category->itemData(index).toInt());
}

void AstroCalcCustomStepsDialog::saveTimeStep(double value)
{
	conf->setValue("astrocalc/custom_time_step", value);
}

void AstroCalcCustomStepsDialog::saveSunAltitude(double alt)
{
	conf->setValue("astrocalc/ephemeris_sun_altitude", alt);
}

void AstroCalcCustomStepsDialog::saveSunAltitudeCrossing(int index)
{
	conf->setValue("astrocalc/ephemeris_sun_altitude_evening", index);
}

void AstroCalcCustomStepsDialog::saveOppositionPlanet(int index)
{
	conf->setValue("astrocalc/ephemeris_opposition_planet",
	               ui->oppositionPlanetComboBox->itemData(index).toString());
}

void AstroCalcCustomStepsDialog::setActiveTimeStep(int stepId)
{
	// May be called before createDialogContent() if the dialog hasn't been shown yet
	if (!ui || !dialog)
		return;

	// Show only the group box that is relevant for the current time step.
	// Both are hidden for every other step so the dialog stays compact.
	const bool isSunAlt     = (stepId == 41); // EphemerisTimeStepSunAtAltitude
	const bool isOpposition = (stepId == 42); // EphemerisTimeStepOpposition
	const bool isCustom     = (stepId == 0);  // custom interval

	ui->customStepsGroupBox->setVisible(isCustom);
	ui->sunAtAltitudeGroupBox->setVisible(isSunAlt);
	ui->oppositionGroupBox->setVisible(isOpposition);

	// Resize the dialog to fit only the visible content
	if (dialog)
		dialog->adjustSize();
}
