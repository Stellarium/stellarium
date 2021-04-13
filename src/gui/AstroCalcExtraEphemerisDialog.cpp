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
#include "StelTranslator.hpp"
#include "StelObjectMgr.hpp"

AstroCalcExtraEphemerisDialog::AstroCalcExtraEphemerisDialog() : StelDialog("AstroCalcExtraEphemeris")
{
	ui = new Ui_astroCalcExtraEphemerisDialogForm;
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
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->skipDataCheckBox, SIGNAL(clicked()), this, SLOT(setOptionStatus()));

	connectBoolProperty(ui->skipDataCheckBox,	"SolarSystem.ephemerisSkippedData");
	connectBoolProperty(ui->skipMarkersCheckBox,	"SolarSystem.ephemerisSkippedMarkers");
	connectIntProperty(ui->dataStepSpinBox,		"SolarSystem.ephemerisDataStep");
	connectBoolProperty(ui->smartDatesCheckBox,	"SolarSystem.ephemerisSmartDates");
	connectBoolProperty(ui->scaleMarkersCheckBox,	"SolarSystem.ephemerisScaleMarkersDisplayed");
	connectIntProperty(ui->lineThicknessSpinBox,	"SolarSystem.ephemerisLineThickness");

	setOptionStatus();
}

void AstroCalcExtraEphemerisDialog::setOptionStatus()
{
	ui->skipMarkersCheckBox->setEnabled(ui->skipDataCheckBox->isChecked());
}
