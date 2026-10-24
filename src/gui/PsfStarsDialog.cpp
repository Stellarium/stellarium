/*
 * Stellarium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "PsfStarsDialog.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"
#include "ui_psfStarsDialog.h"

PsfStarsDialog::PsfStarsDialog()
	: StelDialog("PsfStars")
{
	ui = new Ui_PsfStarsDialogForm;
}

PsfStarsDialog::~PsfStarsDialog()
{
	delete ui;
	ui = Q_NULLPTR;
}

void PsfStarsDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void PsfStarsDialog::createDialogContent()
{
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &PsfStarsDialog::retranslate);
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo,      this, &PsfStarsDialog::handleMovedTo);

	connectDoubleProperty(ui->pointRadiusDoubleSpinBox, "StelSkyDrawer.psfStarPointRadius");
	connectDoubleProperty(ui->flareDecayDoubleSpinBox, "StelSkyDrawer.psfStarFlareDecay");
	connectDoubleProperty(ui->flareStrengthDoubleSpinBox, "StelSkyDrawer.psfStarFlareStrength");
	connectDoubleProperty(ui->brightSourceMagLimitDoubleSpinBox, "StelSkyDrawer.psfStarBrightSourceMagLimit");
	connectDoubleProperty(ui->moonGlareReductionDoubleSpinBox, "StelSkyDrawer.psfMoonGlareReduction");
	connectBoolProperty(ui->moonHaloTextureCheckBox, "StelSkyDrawer.flagPsfMoonHaloTexture");
	connectBoolProperty(ui->projectionCorrectionCheckBox, "StelSkyDrawer.flagPsfStarProjectionCorrection");
	connect(ui->resetDefaultsButton, &QPushButton::clicked, this, []()
	{
		StelApp::getInstance().getCore()->getSkyDrawer()->resetPsfStarSettingsToDefaults();
	});
}
