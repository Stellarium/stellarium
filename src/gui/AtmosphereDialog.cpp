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
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"
#include "AtmosphereDialog.hpp"
#include "StelPropertyMgr.hpp"
#include "ui_atmosphereDialog.h"

AtmosphereDialog::AtmosphereDialog()
	: StelDialog("Atmosphere")
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

	// Experimental settings, protected by Skylight.flagGuiPublic
	StelPropertyMgr* propMgr = StelApp::getInstance().getStelPropertyManager();
	if (propMgr->getProperty("Skylight.flagGuiPublic")->getValue().toBool())
	{
		connectBoolProperty(ui->checkBox_TfromK, "StelSkyDrawer.flagTfromK");

		if (ui->checkBox_TfromK->isChecked())
		{
			// prepare T display
			double T=25.*(propMgr->getProperty("StelSkyDrawer.extinctionCoefficient")->getValue().toDouble()-0.16)+1.;
			ui->doubleSpinBox_T->setValue(T);
		}
		connect(ui->checkBox_TfromK, SIGNAL(toggled(bool)), ui->doubleSpinBox_T, SLOT(setDisabled(bool)));
		connect(ui->extinctionDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setTfromK(double)));
		connectDoubleProperty(ui->doubleSpinBox_T, "StelSkyDrawer.turbidity");

		connectBoolProperty(ui->checkBox_noScatter, "LandscapeMgr.atmosphereNoScatter");
	}
	else
		ui->groupBox_experimental->hide();
}

void AtmosphereDialog::setStandardAtmosphere()
{
	// See https://en.wikipedia.org/wiki/International_Standard_Atmosphere#ICAO_Standard_Atmosphere
	ui->pressureDoubleSpinBox->setValue(1013.25);
	ui->temperatureDoubleSpinBox->setValue(15.0);
	// See http://www.icq.eps.harvard.edu/ICQExtinct.html
	ui->extinctionDoubleSpinBox->setValue(0.2);
}

void AtmosphereDialog::setTfromK(double k)
{
	if (ui->checkBox_TfromK->isChecked())
	{
		double T=25.*(k-0.16)+1.;
		ui->doubleSpinBox_T->setValue(T);
		StelSkyDrawer* skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();

		skyDrawer->setT(T);
	}
}
