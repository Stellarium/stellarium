/*
 * Stellarium Satellites Plug-in: satellites custom filter feature
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "SatellitesFilterDialog.hpp"
#include "ui_satellitesFilterDialog.h"

#include "StelApp.hpp"
#include "StelTranslator.hpp"

SatellitesFilterDialog::SatellitesFilterDialog()
	: StelDialog("SatellitesFilter")
{
	ui = new Ui_satellitesFilterDialog;
}

SatellitesFilterDialog::~SatellitesFilterDialog()
{
	delete ui;
}

void SatellitesFilterDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		populateTexts();
	}
}

void SatellitesFilterDialog::setVisible(bool visible)
{
	StelDialog::setVisible(visible);
}

void SatellitesFilterDialog::createDialogContent()
{
	ui->setupUi(dialog);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectBoolProperty(ui->inclinationCheckBox,  "Satellites.flagCFInclination");
	connectDoubleProperty(ui->minInclination,     "Satellites.minCFInclination");
	connectDoubleProperty(ui->maxInclination,     "Satellites.maxCFInclination");
	connectBoolProperty(ui->periodCheckBox,       "Satellites.flagCFPeriod");
	connectDoubleProperty(ui->minPeriod,          "Satellites.minCFPeriod");
	connectDoubleProperty(ui->maxPeriod,          "Satellites.maxCFPeriod");
	connectBoolProperty(ui->eccentricityCheckBox, "Satellites.flagCFEccentricity");
	connectDoubleProperty(ui->minEccentricity,    "Satellites.minCFEccentricity");
	connectDoubleProperty(ui->maxEccentricity,    "Satellites.maxCFEccentricity");
	connectBoolProperty(ui->apogeeCheckBox,       "Satellites.flagCFApogee");
	connectDoubleProperty(ui->minApogee,          "Satellites.minCFApogee");
	connectDoubleProperty(ui->maxApogee,          "Satellites.maxCFApogee");
	connectBoolProperty(ui->perigeeCheckBox,      "Satellites.flagCFPerigee");
	connectDoubleProperty(ui->minPerigee,         "Satellites.minCFPerigee");
	connectDoubleProperty(ui->maxPerigee,         "Satellites.maxCFPerigee");
	connectBoolProperty(ui->stdMagnitudeCheckBox, "Satellites.flagCFKnownStdMagnitude");

	populateTexts();
}

void SatellitesFilterDialog::populateTexts()
{
	QString km = qc_("km", "distance");
	QString m = qc_("m", "time");
	ui->minApogee->setSuffix(QString(" %1").arg(km));
	ui->maxApogee->setSuffix(QString(" %1").arg(km));
	ui->minPerigee->setSuffix(QString(" %1").arg(km));
	ui->maxPerigee->setSuffix(QString(" %1").arg(km));
	ui->minPeriod->setSuffix(QString(" %1").arg(m));
	ui->maxPeriod->setSuffix(QString(" %1").arg(m));
	ui->minInclination->setSuffix("°");
	ui->maxInclination->setSuffix("°");
}
