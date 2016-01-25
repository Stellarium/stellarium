/*
 * FOV plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "FOV.hpp"
#include "FOVWindow.hpp"
#include "ui_fovWindow.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

FOVWindow::FOVWindow()
	: fov(NULL)
{
	ui = new Ui_fovWindowForm();
	dialogName = "FOV";
}

FOVWindow::~FOVWindow()
{
	delete ui;
}

void FOVWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateAboutText();
	}
}

void FOVWindow::createDialogContent()
{
	fov = GETSTELMODULE(FOV);
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	populateFOV();

	connect(ui->doubleSpinBoxFOV0, SIGNAL(valueChanged(double)), this, SLOT(updateFOV0(double)));
	connect(ui->doubleSpinBoxFOV1, SIGNAL(valueChanged(double)), this, SLOT(updateFOV1(double)));
	connect(ui->doubleSpinBoxFOV2, SIGNAL(valueChanged(double)), this, SLOT(updateFOV2(double)));
	connect(ui->doubleSpinBoxFOV3, SIGNAL(valueChanged(double)), this, SLOT(updateFOV3(double)));
	connect(ui->doubleSpinBoxFOV4, SIGNAL(valueChanged(double)), this, SLOT(updateFOV4(double)));
	connect(ui->doubleSpinBoxFOV5, SIGNAL(valueChanged(double)), this, SLOT(updateFOV5(double)));
	connect(ui->doubleSpinBoxFOV6, SIGNAL(valueChanged(double)), this, SLOT(updateFOV6(double)));
	connect(ui->doubleSpinBoxFOV7, SIGNAL(valueChanged(double)), this, SLOT(updateFOV7(double)));
	connect(ui->doubleSpinBoxFOV8, SIGNAL(valueChanged(double)), this, SLOT(updateFOV8(double)));
	connect(ui->doubleSpinBoxFOV9, SIGNAL(valueChanged(double)), this, SLOT(updateFOV9(double)));

	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveFOVSettings()));
	connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(resetFOVSettings()));

	updateAboutText();	
}

void FOVWindow::updateAboutText()
{
	ui->labelTitle->setText(q_("Field of View plug-in"));
	QString version = QString(q_("Version %1")).arg(FOV_VERSION);
	ui->labelVersion->setText(version);
}

void FOVWindow::saveFOVSettings()
{
	fov->saveSettingsToConfig();
}

void FOVWindow::resetFOVSettings()
{
	fov->restoreDefaults();
	populateFOV();
}

void FOVWindow::updateFOV0(double value)
{
	fov->setQuickFOV(value, 0);
}

void FOVWindow::updateFOV1(double value)
{
	fov->setQuickFOV(value, 1);
}

void FOVWindow::updateFOV2(double value)
{
	fov->setQuickFOV(value, 2);
}

void FOVWindow::updateFOV3(double value)
{
	fov->setQuickFOV(value, 3);
}

void FOVWindow::updateFOV4(double value)
{
	fov->setQuickFOV(value, 4);
}

void FOVWindow::updateFOV5(double value)
{
	fov->setQuickFOV(value, 5);
}

void FOVWindow::updateFOV6(double value)
{
	fov->setQuickFOV(value, 6);
}

void FOVWindow::updateFOV7(double value)
{
	fov->setQuickFOV(value, 7);
}

void FOVWindow::updateFOV8(double value)
{
	fov->setQuickFOV(value, 8);
}

void FOVWindow::updateFOV9(double value)
{
	fov->setQuickFOV(value, 9);
}

void FOVWindow::populateFOV()
{
	ui->doubleSpinBoxFOV0->setValue(fov->getQuickFOV(0));
	ui->doubleSpinBoxFOV1->setValue(fov->getQuickFOV(1));
	ui->doubleSpinBoxFOV2->setValue(fov->getQuickFOV(2));
	ui->doubleSpinBoxFOV3->setValue(fov->getQuickFOV(3));
	ui->doubleSpinBoxFOV4->setValue(fov->getQuickFOV(4));
	ui->doubleSpinBoxFOV5->setValue(fov->getQuickFOV(5));
	ui->doubleSpinBoxFOV6->setValue(fov->getQuickFOV(6));
	ui->doubleSpinBoxFOV7->setValue(fov->getQuickFOV(7));
	ui->doubleSpinBoxFOV8->setValue(fov->getQuickFOV(8));
	ui->doubleSpinBoxFOV9->setValue(fov->getQuickFOV(9));
}
