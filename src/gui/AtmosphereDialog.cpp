/*
 * Stellarium
 * Copyright (C) 2011 Georg Zotti
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
#include "AtmosphereDialog.hpp"
#include "ui_atmosphereDialog.h"

AtmosphereDialog::AtmosphereDialog()
	: StelDialog("Atmosphere")
	, refraction(Q_NULLPTR)
	, extinction(Q_NULLPTR)

{
	ui = new Ui_atmosphereDialogForm;
}

AtmosphereDialog::~AtmosphereDialog()
{
	delete ui;
}

void AtmosphereDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}


void AtmosphereDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectDoubleProperty(ui->pressureDoubleSpinBox,"StelSkyDrawer.atmospherePressure");
	connectDoubleProperty(ui->temperatureDoubleSpinBox,"StelSkyDrawer.atmosphereTemperature");
	connectDoubleProperty(ui->extinctionDoubleSpinBox,"StelSkyDrawer.extinctionCoefficient");

	connect(ui->standardAtmosphereButton, SIGNAL(clicked()), this, SLOT(setStandardAtmosphere()));
}

void AtmosphereDialog::setStandardAtmosphere()
{
	// See https://en.wikipedia.org/wiki/International_Standard_Atmosphere#ICAO_Standard_Atmosphere
	ui->pressureDoubleSpinBox->setValue(1013.25);
	ui->temperatureDoubleSpinBox->setValue(15.0);
	// See http://www.icq.eps.harvard.edu/ICQExtinct.html
	ui->extinctionDoubleSpinBox->setValue(0.2);
}
