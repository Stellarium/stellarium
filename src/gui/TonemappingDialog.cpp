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

#include <QUrl>
#include <QSettings>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"
#include "StelLocaleMgr.hpp"
#include "TonemappingDialog.hpp"
#include "StelToneReproducer.hpp"

#include "ui_tonemappingDialog.h"

TonemappingDialog::TonemappingDialog() : StelDialog("Tonemapping")
{
	ui = new Ui_TonemappingDialogForm;
}

TonemappingDialog::~TonemappingDialog()
{
	delete ui;
	ui=Q_NULLPTR;
}

void TonemappingDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void TonemappingDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectDoubleProperty(ui->dalSpinBox,   "StelToneReproducer.displayAdaptationLuminance");
	connectDoubleProperty(ui->dmSpinBox,    "StelToneReproducer.maxDisplayLuminance");
	connectDoubleProperty(ui->gammaSpinBox, "StelToneReproducer.displayGamma");
	connectBoolProperty(ui->checkBox_extraGamma, "StelToneReproducer.flagUseTmGamma");
	connectBoolProperty(ui->checkBox_sRGB,  "StelToneReproducer.flagSRGB");

	connect(ui->resetPushButton, SIGNAL(clicked(bool)), this, SLOT(resetTonemapping()));
}

void TonemappingDialog::resetTonemapping()
{
	StelToneReproducer *t = StelApp::getInstance().getCore()->getToneReproducer();
	// Defaults as of Stellarium 0.21.0 and earlier. (since 0.10?)
	t->setDisplayAdaptationLuminance(50.);
	t->setDisplayGamma(2.2222f);
	t->setMaxDisplayLuminance(100.);
	t->setFlagUseTmGamma(true);
	t->setFlagSRGB(true);
}
