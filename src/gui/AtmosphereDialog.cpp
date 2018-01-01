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
#include "StelModuleMgr.hpp"
#include "AtmosphereDialog.hpp"
#include "Skylight.hpp"
#include "ui_atmosphereDialog.h"

AtmosphereDialog::AtmosphereDialog()
	: StelDialog("Atmosphere")
	, skylight(NULL)
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
	// TODO: after rebase, change the UI elements below to property changing.
	connectDoubleProperty(ui->doubleSpinBox_globalBrightness, "LandscapeMgr.atmLumFactor");

	connectBoolProperty(ui->checkBox_TfromK, "StelSkyDrawer.flagTfromK");
	connect(ui->checkBox_TfromK, SIGNAL(toggled(bool)), ui->doubleSpinBox_T, SLOT(setDisabled(bool)));
	connect(ui->extinctionDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setTfromK(double)));
	connectDoubleProperty(ui->doubleSpinBox_T, "StelSkyDrawer.turbidity");

	connectBoolProperty(ui->checkBox_sRGB, "Skylight.flagSRGB");

	ui->comboBox_skylightDecimals->addItem("1", 1);
	ui->comboBox_skylightDecimals->addItem("0.1", .1);
	ui->comboBox_skylightDecimals->addItem("0.01", .01);
	ui->comboBox_skylightDecimals->addItem("0.001", .001);
	ui->comboBox_skylightDecimals->addItem("0.0001", .0001);
	ui->comboBox_skylightDecimals->addItem("0.00001", .00001);
	connect(ui->comboBox_skylightDecimals, SIGNAL(currentIndexChanged(int)), this, SLOT(setIncrements(int)));

	connectBoolProperty(ui->checkBox_blendSun, "StelSkyDrawer.flagUseSunBlending");
	connectBoolProperty(ui->checkBox_drawSunAfterAtmosphere, "StelSkyDrawer.flagDrawSunAfterAtmosphere");

	connectDoubleProperty(ui->doubleSpinBox_zX11, "Skylight.zX11");
	connectDoubleProperty(ui->doubleSpinBox_zX12, "Skylight.zX12");
	connectDoubleProperty(ui->doubleSpinBox_zX13, "Skylight.zX13");
	connectDoubleProperty(ui->doubleSpinBox_zX21, "Skylight.zX21");
	connectDoubleProperty(ui->doubleSpinBox_zX22, "Skylight.zX22");
	connectDoubleProperty(ui->doubleSpinBox_zX23, "Skylight.zX23");
	connectDoubleProperty(ui->doubleSpinBox_zX24, "Skylight.zX24");
	connectDoubleProperty(ui->doubleSpinBox_zX31, "Skylight.zX31");
	connectDoubleProperty(ui->doubleSpinBox_zX32, "Skylight.zX32");
	connectDoubleProperty(ui->doubleSpinBox_zX33, "Skylight.zX33");
	connectDoubleProperty(ui->doubleSpinBox_zX34, "Skylight.zX34");
	connectDoubleProperty(ui->doubleSpinBox_zY11, "Skylight.zY11");
	connectDoubleProperty(ui->doubleSpinBox_zY12, "Skylight.zY12");
	connectDoubleProperty(ui->doubleSpinBox_zY13, "Skylight.zY13");
	connectDoubleProperty(ui->doubleSpinBox_zY21, "Skylight.zY21");
	connectDoubleProperty(ui->doubleSpinBox_zY22, "Skylight.zY22");
	connectDoubleProperty(ui->doubleSpinBox_zY23, "Skylight.zY23");
	connectDoubleProperty(ui->doubleSpinBox_zY24, "Skylight.zY24");
	connectDoubleProperty(ui->doubleSpinBox_zY31, "Skylight.zY31");
	connectDoubleProperty(ui->doubleSpinBox_zY32, "Skylight.zY32");
	connectDoubleProperty(ui->doubleSpinBox_zY33, "Skylight.zY33");
	connectDoubleProperty(ui->doubleSpinBox_zY34, "Skylight.zY34");

	connectDoubleProperty(ui->doubleSpinBox_AYT, "Skylight.AYt");
	connectDoubleProperty(ui->doubleSpinBox_BYT, "Skylight.BYt");
	connectDoubleProperty(ui->doubleSpinBox_CYT, "Skylight.CYt");
	connectDoubleProperty(ui->doubleSpinBox_DYT, "Skylight.DYt");
	connectDoubleProperty(ui->doubleSpinBox_EYT, "Skylight.EYt");
	connectDoubleProperty(ui->doubleSpinBox_AY , "Skylight.AYc");
	connectDoubleProperty(ui->doubleSpinBox_BY , "Skylight.BYc");
	connectDoubleProperty(ui->doubleSpinBox_CY , "Skylight.CYc");
	connectDoubleProperty(ui->doubleSpinBox_DY , "Skylight.DYc");
	connectDoubleProperty(ui->doubleSpinBox_EY , "Skylight.EYc");

	connectDoubleProperty(ui->doubleSpinBox_AxT, "Skylight.Axt");
	connectDoubleProperty(ui->doubleSpinBox_BxT, "Skylight.Bxt");
	connectDoubleProperty(ui->doubleSpinBox_CxT, "Skylight.Cxt");
	connectDoubleProperty(ui->doubleSpinBox_DxT, "Skylight.Dxt");
	connectDoubleProperty(ui->doubleSpinBox_ExT, "Skylight.Ext");
	connectDoubleProperty(ui->doubleSpinBox_Ax , "Skylight.Axc");
	connectDoubleProperty(ui->doubleSpinBox_Bx , "Skylight.Bxc");
	connectDoubleProperty(ui->doubleSpinBox_Cx , "Skylight.Cxc");
	connectDoubleProperty(ui->doubleSpinBox_Dx , "Skylight.Dxc");
	connectDoubleProperty(ui->doubleSpinBox_Ex , "Skylight.Exc");

	connectDoubleProperty(ui->doubleSpinBox_AyT, "Skylight.Ayt");
	connectDoubleProperty(ui->doubleSpinBox_ByT, "Skylight.Byt");
	connectDoubleProperty(ui->doubleSpinBox_CyT, "Skylight.Cyt");
	connectDoubleProperty(ui->doubleSpinBox_DyT, "Skylight.Dyt");
	connectDoubleProperty(ui->doubleSpinBox_EyT, "Skylight.Eyt");
	connectDoubleProperty(ui->doubleSpinBox_Ay , "Skylight.Ayc");
	connectDoubleProperty(ui->doubleSpinBox_By , "Skylight.Byc");
	connectDoubleProperty(ui->doubleSpinBox_Cy , "Skylight.Cyc");
	connectDoubleProperty(ui->doubleSpinBox_Dy , "Skylight.Dyc");
	connectDoubleProperty(ui->doubleSpinBox_Ey , "Skylight.Eyc");

	connect(ui->pushButton_ResetDistYPreetham, SIGNAL(released()), this, SLOT(resetYPreet()));
	connect(ui->pushButton_ResetDistxPreetham, SIGNAL(released()), this, SLOT(resetxPreet()));
	connect(ui->pushButton_ResetDistyPreetham, SIGNAL(released()), this, SLOT(resetyPreet()));
	connect(ui->pushButton_ResetDistYStel    , SIGNAL(released()), this, SLOT(resetYStel()));
	connect(ui->pushButton_ResetDistxStel    , SIGNAL(released()), this, SLOT(resetxStel()));
	connect(ui->pushButton_ResetDistyStel    , SIGNAL(released()), this, SLOT(resetyStel()));
	connect(ui->pushButton_ResetZXpreetham, SIGNAL(released()), this, SLOT(resetZxPreet()));
	connect(ui->pushButton_ResetZYpreetham, SIGNAL(released()), this, SLOT(resetZyPreet()));
	connect(ui->pushButton_ResetZXstel,     SIGNAL(released()), this, SLOT(resetZxStel()));
	connect(ui->pushButton_ResetZYstel,     SIGNAL(released()), this, SLOT(resetZyStel()));
}

void AtmosphereDialog::setStandardAtmosphere()
{
	// See https://en.wikipedia.org/wiki/International_Standard_Atmosphere#ICAO_Standard_Atmosphere
	ui->pressureDoubleSpinBox->setValue(1013.25);
	ui->temperatureDoubleSpinBox->setValue(15.0);
	// See http://www.icq.eps.harvard.edu/ICQExtinct.html
	ui->extinctionDoubleSpinBox->setValue(0.2);
}

void AtmosphereDialog::setIncrements(int idx)
{
	double val=ui->comboBox_skylightDecimals->currentData().toDouble();
	ui->doubleSpinBox_zX11->setSingleStep(val);
	ui->doubleSpinBox_zX12->setSingleStep(val);
	ui->doubleSpinBox_zX13->setSingleStep(val);
	ui->doubleSpinBox_zX21->setSingleStep(val);
	ui->doubleSpinBox_zX22->setSingleStep(val);
	ui->doubleSpinBox_zX23->setSingleStep(val);
	ui->doubleSpinBox_zX24->setSingleStep(val);
	ui->doubleSpinBox_zX31->setSingleStep(val);
	ui->doubleSpinBox_zX32->setSingleStep(val);
	ui->doubleSpinBox_zX33->setSingleStep(val);
	ui->doubleSpinBox_zX34->setSingleStep(val);
	ui->doubleSpinBox_zY11->setSingleStep(val);
	ui->doubleSpinBox_zY12->setSingleStep(val);
	ui->doubleSpinBox_zY13->setSingleStep(val);
	ui->doubleSpinBox_zY21->setSingleStep(val);
	ui->doubleSpinBox_zY22->setSingleStep(val);
	ui->doubleSpinBox_zY23->setSingleStep(val);
	ui->doubleSpinBox_zY24->setSingleStep(val);
	ui->doubleSpinBox_zY31->setSingleStep(val);
	ui->doubleSpinBox_zY32->setSingleStep(val);
	ui->doubleSpinBox_zY33->setSingleStep(val);
	ui->doubleSpinBox_zY34->setSingleStep(val);

	ui->doubleSpinBox_AYT->setSingleStep(val);
	ui->doubleSpinBox_BYT->setSingleStep(val);
	ui->doubleSpinBox_CYT->setSingleStep(val);
	ui->doubleSpinBox_DYT->setSingleStep(val);
	ui->doubleSpinBox_EYT->setSingleStep(val);
	ui->doubleSpinBox_AY ->setSingleStep(val);
	ui->doubleSpinBox_BY ->setSingleStep(val);
	ui->doubleSpinBox_CY ->setSingleStep(val);
	ui->doubleSpinBox_DY ->setSingleStep(val);
	ui->doubleSpinBox_EY ->setSingleStep(val);

	ui->doubleSpinBox_AxT->setSingleStep(val);
	ui->doubleSpinBox_BxT->setSingleStep(val);
	ui->doubleSpinBox_CxT->setSingleStep(val);
	ui->doubleSpinBox_DxT->setSingleStep(val);
	ui->doubleSpinBox_ExT->setSingleStep(val);
	ui->doubleSpinBox_Ax ->setSingleStep(val);
	ui->doubleSpinBox_Bx ->setSingleStep(val);
	ui->doubleSpinBox_Cx ->setSingleStep(val);
	ui->doubleSpinBox_Dx ->setSingleStep(val);
	ui->doubleSpinBox_Ex ->setSingleStep(val);

	ui->doubleSpinBox_AyT->setSingleStep(val);
	ui->doubleSpinBox_ByT->setSingleStep(val);
	ui->doubleSpinBox_CyT->setSingleStep(val);
	ui->doubleSpinBox_DyT->setSingleStep(val);
	ui->doubleSpinBox_EyT->setSingleStep(val);
	ui->doubleSpinBox_Ay ->setSingleStep(val);
	ui->doubleSpinBox_By ->setSingleStep(val);
	ui->doubleSpinBox_Cy ->setSingleStep(val);
	ui->doubleSpinBox_Dy ->setSingleStep(val);
	ui->doubleSpinBox_Ey ->setSingleStep(val);
}

void AtmosphereDialog::resetYPreet()
{
	ui->doubleSpinBox_AYT->setValue( 0.1787f);
	ui->doubleSpinBox_AY ->setValue(-1.4630f);
	ui->doubleSpinBox_BYT->setValue(-0.3554f);
	ui->doubleSpinBox_BY ->setValue(+0.4275f);
	ui->doubleSpinBox_CYT->setValue(-0.0227f);
	ui->doubleSpinBox_CY ->setValue(+5.3251f);
	ui->doubleSpinBox_DYT->setValue( 0.1206f);
	ui->doubleSpinBox_DY ->setValue(-2.5771f);
	ui->doubleSpinBox_EYT->setValue(-0.0670f);
	ui->doubleSpinBox_EY ->setValue(+0.3703f);
}

void AtmosphereDialog::resetxPreet()
{
	ui->doubleSpinBox_AxT->setValue(-0.0193f);
	ui->doubleSpinBox_Ax ->setValue(-0.2592f);
	ui->doubleSpinBox_BxT->setValue(-0.0665f);
	ui->doubleSpinBox_Bx ->setValue(+0.0008f);
	ui->doubleSpinBox_CxT->setValue(-0.0004f);
	ui->doubleSpinBox_Cx ->setValue(+0.2125f);
	ui->doubleSpinBox_DxT->setValue(-0.0641f);
	ui->doubleSpinBox_Dx ->setValue(-0.8989f);
	ui->doubleSpinBox_ExT->setValue(-0.0033f);
	ui->doubleSpinBox_Ex ->setValue(+0.0452f);
}

void AtmosphereDialog::resetyPreet()
{
	ui->doubleSpinBox_AyT->setValue(-0.0167f);
	ui->doubleSpinBox_Ay ->setValue(-0.2608f);
	ui->doubleSpinBox_ByT->setValue(-0.0950f);
	ui->doubleSpinBox_By ->setValue(+0.0092f);
	ui->doubleSpinBox_CyT->setValue(-0.0079f);
	ui->doubleSpinBox_Cy ->setValue(+0.2102f);
	ui->doubleSpinBox_DyT->setValue(-0.0441f);
	ui->doubleSpinBox_Dy ->setValue(-1.6537f);
	ui->doubleSpinBox_EyT->setValue(-0.0109f);
	ui->doubleSpinBox_Ey ->setValue(+0.0529f);
}

void AtmosphereDialog::resetYStel()
{
	resetYPreet();
	ui->doubleSpinBox_AYT->setValue( 0.2787f);
	ui->doubleSpinBox_AY ->setValue(-1.0630f);
	ui->doubleSpinBox_CY ->setValue(+6.3251f);

}

void AtmosphereDialog::resetxStel()
{
	ui->doubleSpinBox_AxT->setValue(-0.0148f);
	ui->doubleSpinBox_Ax ->setValue(-0.1703f);
	ui->doubleSpinBox_BxT->setValue(-0.0664f);
	ui->doubleSpinBox_Bx ->setValue(+0.0011f);
	ui->doubleSpinBox_CxT->setValue(-0.0005f);
	ui->doubleSpinBox_Cx ->setValue(+0.2127f);
	ui->doubleSpinBox_DxT->setValue(-0.0641f);
	ui->doubleSpinBox_Dx ->setValue(-0.8992f);
	ui->doubleSpinBox_ExT->setValue(-0.0035f);
	ui->doubleSpinBox_Ex ->setValue(+0.0453f);
}

void AtmosphereDialog::resetyStel()
{
	ui->doubleSpinBox_AyT->setValue(-0.0131f);
	ui->doubleSpinBox_Ay ->setValue(-0.2498f);
	ui->doubleSpinBox_ByT->setValue(-0.0951f);
	ui->doubleSpinBox_By ->setValue(+0.0092f);  // ident
	ui->doubleSpinBox_CyT->setValue(-0.0082f);
	ui->doubleSpinBox_Cy ->setValue(+0.2404f);
	ui->doubleSpinBox_DyT->setValue(-0.0438f);
	ui->doubleSpinBox_Dy ->setValue(-1.0539f);
	ui->doubleSpinBox_EyT->setValue(-0.0109f); // ident
	ui->doubleSpinBox_Ey ->setValue(+0.0531f);
}

//void resetZYPreet();
void AtmosphereDialog::resetZxPreet()
{
	ui->doubleSpinBox_zX11->setValue( 0.00166f);
	ui->doubleSpinBox_zX12->setValue(-0.00375f);
	ui->doubleSpinBox_zX13->setValue(+0.00209f);
	ui->doubleSpinBox_zX21->setValue(-0.02903f);
	ui->doubleSpinBox_zX22->setValue(+0.06377f);
	ui->doubleSpinBox_zX23->setValue(-0.03202f);
	ui->doubleSpinBox_zX24->setValue(+0.00394f);
	ui->doubleSpinBox_zX31->setValue( 0.11693f);
	ui->doubleSpinBox_zX32->setValue(-0.21196f);
	ui->doubleSpinBox_zX33->setValue(+0.06052f);
	ui->doubleSpinBox_zX34->setValue(+0.25886f);
}

void AtmosphereDialog::resetZyPreet()
{
	ui->doubleSpinBox_zY11->setValue(  0.00275f);
	ui->doubleSpinBox_zY12->setValue( -0.00610f);
	ui->doubleSpinBox_zY13->setValue( +0.00317f);
	ui->doubleSpinBox_zY21->setValue( -0.04214f);
	ui->doubleSpinBox_zY22->setValue( +0.08970f);
	ui->doubleSpinBox_zY23->setValue( -0.04153f);
	ui->doubleSpinBox_zY24->setValue( +0.00516f);
	ui->doubleSpinBox_zY31->setValue(  0.15346f);
	ui->doubleSpinBox_zY32->setValue( -0.26756f);
	ui->doubleSpinBox_zY33->setValue( +0.06670f);
	ui->doubleSpinBox_zY34->setValue( +0.26688f);
}
//void resetZYStel();
void AtmosphereDialog::resetZxStel()
{
	resetZxPreet();
	ui->doubleSpinBox_zX11->setValue( 0.00216f); // FC
	ui->doubleSpinBox_zX31->setValue( 0.10169f); // FC
}

void AtmosphereDialog::resetZyStel()
{
	resetZyPreet();
	ui->doubleSpinBox_zY31->setValue(  0.15346f); // FC
}

void AtmosphereDialog::setTfromK(double k)
{
	if (ui->checkBox_TfromK->isChecked())
	{
		double T=25.f*(k-0.16f)+1.f;
		ui->doubleSpinBox_T->setValue(T);
		StelSkyDrawer* skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();

		skyDrawer->setT(T);
	}
}



