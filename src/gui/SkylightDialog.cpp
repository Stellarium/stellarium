/*
 * Stellarium
 * Copyright (C) 2022 Georg Zotti
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

#include "SkylightDialog.hpp"
#include "ui_skylightDialog.h"

SkylightDialog::SkylightDialog()
	: StelDialog("Skylight")
{
	ui = new Ui_skylightDialogForm;
}

SkylightDialog::~SkylightDialog()
{
	delete ui;
}

void SkylightDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void SkylightDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectBoolProperty(ui->checkBox_Schaefer, "Skylight.flagSchaefer");

	ui->comboBox_skylightDecimals->addItem("1", 1);
	ui->comboBox_skylightDecimals->addItem("0.1", .1);
	ui->comboBox_skylightDecimals->addItem("0.01", .01);
	ui->comboBox_skylightDecimals->addItem("0.001", .001);
	ui->comboBox_skylightDecimals->addItem("0.0001", .0001);
	ui->comboBox_skylightDecimals->addItem("0.00001", .00001);
	connect(ui->comboBox_skylightDecimals, SIGNAL(currentIndexChanged(int)), this, SLOT(setIncrements(int)));
	ui->comboBox_skylightDecimals->setCurrentIndex(2); // start with 0.01 increment

	connectBoolProperty(ui->checkBox_earlySunHalo, "StelSkyDrawer.flagEarlySunHalo");
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
	connect(ui->resetPreetPushButton,     SIGNAL(released()), this, SLOT(resetPreet()));
	connect(ui->resetStellPushButton,     SIGNAL(released()), this, SLOT(resetStel()));
}

void SkylightDialog::setIncrements(int idx)
{
	Q_UNUSED(idx)
	const double val=ui->comboBox_skylightDecimals->currentData().toDouble();
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

void SkylightDialog::resetYPreet()
{
	ui->doubleSpinBox_AYT->setValue( 0.1787);
	ui->doubleSpinBox_AY ->setValue(-1.4630);
	ui->doubleSpinBox_BYT->setValue(-0.3554);
	ui->doubleSpinBox_BY ->setValue(+0.4275);
	ui->doubleSpinBox_CYT->setValue(-0.0227);
	ui->doubleSpinBox_CY ->setValue(+5.3251);
	ui->doubleSpinBox_DYT->setValue( 0.1206);
	ui->doubleSpinBox_DY ->setValue(-2.5771);
	ui->doubleSpinBox_EYT->setValue(-0.0670);
	ui->doubleSpinBox_EY ->setValue(+0.3703);
}

void SkylightDialog::resetxPreet()
{
	ui->doubleSpinBox_AxT->setValue(-0.0193);
	ui->doubleSpinBox_Ax ->setValue(-0.2592);
	ui->doubleSpinBox_BxT->setValue(-0.0665);
	ui->doubleSpinBox_Bx ->setValue(+0.0008);
	ui->doubleSpinBox_CxT->setValue(-0.0004);
	ui->doubleSpinBox_Cx ->setValue(+0.2125);
	ui->doubleSpinBox_DxT->setValue(-0.0641);
	ui->doubleSpinBox_Dx ->setValue(-0.8989);
	ui->doubleSpinBox_ExT->setValue(-0.0033);
	ui->doubleSpinBox_Ex ->setValue(+0.0452);
}

void SkylightDialog::resetyPreet()
{
	ui->doubleSpinBox_AyT->setValue(-0.0167);
	ui->doubleSpinBox_Ay ->setValue(-0.2608);
	ui->doubleSpinBox_ByT->setValue(-0.0950);
	ui->doubleSpinBox_By ->setValue(+0.0092);
	ui->doubleSpinBox_CyT->setValue(-0.0079);
	ui->doubleSpinBox_Cy ->setValue(+0.2102);
	ui->doubleSpinBox_DyT->setValue(-0.0441);
	ui->doubleSpinBox_Dy ->setValue(-1.6537);
	ui->doubleSpinBox_EyT->setValue(-0.0109);
	ui->doubleSpinBox_Ey ->setValue(+0.0529);
}

void SkylightDialog::resetYStel()
{
	resetYPreet();
	ui->doubleSpinBox_AYT->setValue( 0.2787);
	ui->doubleSpinBox_AY ->setValue(-1.0630);
	ui->doubleSpinBox_CY ->setValue(+6.3251);
}

void SkylightDialog::resetxStel()
{
	ui->doubleSpinBox_AxT->setValue(-0.0148);
	ui->doubleSpinBox_Ax ->setValue(-0.1703);
	ui->doubleSpinBox_BxT->setValue(-0.0664);
	ui->doubleSpinBox_Bx ->setValue(+0.0011);
	ui->doubleSpinBox_CxT->setValue(-0.0005);
	ui->doubleSpinBox_Cx ->setValue(+0.2127);
	ui->doubleSpinBox_DxT->setValue(-0.0641); // ident
	ui->doubleSpinBox_Dx ->setValue(-0.8992);
	ui->doubleSpinBox_ExT->setValue(-0.0035);
	ui->doubleSpinBox_Ex ->setValue(+0.0453);
}

void SkylightDialog::resetyStel()
{
	ui->doubleSpinBox_AyT->setValue(-0.0131);
	ui->doubleSpinBox_Ay ->setValue(-0.2498);
	ui->doubleSpinBox_ByT->setValue(-0.0951);
	ui->doubleSpinBox_By ->setValue(+0.0092); // ident
	ui->doubleSpinBox_CyT->setValue(-0.0082);
	ui->doubleSpinBox_Cy ->setValue(+0.2404);
	ui->doubleSpinBox_DyT->setValue(-0.0438);
	ui->doubleSpinBox_Dy ->setValue(-1.0539);
	ui->doubleSpinBox_EyT->setValue(-0.0109); // ident
	ui->doubleSpinBox_Ey ->setValue(+0.0531);
}

//void resetZYPreet();
void SkylightDialog::resetZxPreet()
{
	// Note: values from the preprint document have one decimal digit more than the final paper.
	ui->doubleSpinBox_zX11->setValue( 0.00166);
	ui->doubleSpinBox_zX12->setValue(-0.00375);
	ui->doubleSpinBox_zX13->setValue(+0.00209);
	ui->doubleSpinBox_zX21->setValue(-0.02903);
	ui->doubleSpinBox_zX22->setValue(+0.06377);
	ui->doubleSpinBox_zX23->setValue(-0.03202);
	ui->doubleSpinBox_zX24->setValue(+0.00394);
	ui->doubleSpinBox_zX31->setValue( 0.11693);
	ui->doubleSpinBox_zX32->setValue(-0.21196);
	ui->doubleSpinBox_zX33->setValue(+0.06052);
	ui->doubleSpinBox_zX34->setValue(+0.25886);
}

void SkylightDialog::resetZyPreet()
{
    // Note: values from the preprint document have one decimal digit more than the final paper.
	ui->doubleSpinBox_zY11->setValue(  0.00275);
	ui->doubleSpinBox_zY12->setValue( -0.00610);
	ui->doubleSpinBox_zY13->setValue( +0.00317);
	ui->doubleSpinBox_zY21->setValue( -0.04214);
	ui->doubleSpinBox_zY22->setValue( +0.08970);
	ui->doubleSpinBox_zY23->setValue( -0.04153);
	ui->doubleSpinBox_zY24->setValue( +0.00516);
	ui->doubleSpinBox_zY31->setValue(  0.15346);
	ui->doubleSpinBox_zY32->setValue( -0.26756);
	ui->doubleSpinBox_zY33->setValue( +0.06670);
	ui->doubleSpinBox_zY34->setValue( +0.26688);
}
//void resetZYStel();
void SkylightDialog::resetZxStel()
{
	resetZxPreet();
	ui->doubleSpinBox_zX11->setValue( 0.00216); // FC
	ui->doubleSpinBox_zX31->setValue( 0.10169); // FC
}

void SkylightDialog::resetZyStel()
{
	resetZyPreet();
	ui->doubleSpinBox_zY31->setValue(  0.14535); // FC
}

void SkylightDialog::resetStel()
{
    resetYStel();
    resetxStel();
    resetyStel();
    resetZxStel();
    resetZyStel();
}

void SkylightDialog::resetPreet()
{
    resetYPreet();
    resetxPreet();
    resetyPreet();
    resetZxPreet();
    resetZyPreet();
}
