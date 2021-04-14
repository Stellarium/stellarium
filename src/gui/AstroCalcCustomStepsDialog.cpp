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
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	ui->timeStepDoubleSpinBox->setValue(conf->value("astrocalc/custom_time_step", 1.0).toDouble());
	connect(ui->timeStepDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(saveTimeStep(double)));

	populateUnitMeasurementsList();
	connect(ui->unitMeasurementComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveUnitMeasurement(int)));
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
