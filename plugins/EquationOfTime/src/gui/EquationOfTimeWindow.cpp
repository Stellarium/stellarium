/*
 * Equation of Time plug-in for Stellarium
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

#include "EquationOfTime.hpp"
#include "EquationOfTimeWindow.hpp"
#include "ui_equationOfTimeWindow.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

EquationOfTimeWindow::EquationOfTimeWindow()
{
	ui = new Ui_equationOfTimeWindowForm();
}

EquationOfTimeWindow::~EquationOfTimeWindow()
{
	delete ui;
}

void EquationOfTimeWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateAboutText();
	}
}

void EquationOfTimeWindow::createDialogContent()
{
	equationOfTime = GETSTELMODULE(EquationOfTime);
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	ui->checkBoxEnableAtStartup->setChecked(equationOfTime->getFlagEnableAtStartup());
	connect(ui->checkBoxEnableAtStartup, SIGNAL(clicked(bool)), equationOfTime, SLOT(setFlagEnableAtStartup(bool)));

	ui->checkBoxInvertedValue->setChecked(equationOfTime->getFlagInvertedValue());
	connect(ui->checkBoxInvertedValue, SIGNAL(clicked(bool)), equationOfTime, SLOT(setFlagInvertedValue(bool)));

	ui->checkBoxMsFormat->setChecked(equationOfTime->getFlagMsFormat());
	connect(ui->checkBoxMsFormat, SIGNAL(clicked(bool)), equationOfTime, SLOT(setFlagMsFormat(bool)));

	ui->spinBoxFontSize->setValue(equationOfTime->getFontSize());
	connect(ui->spinBoxFontSize, SIGNAL(valueChanged(int)), equationOfTime, SLOT(setFontSize(int)));

	ui->checkBoxShowButton->setChecked(equationOfTime->getFlagShowEOTButton());
	connect(ui->checkBoxShowButton, SIGNAL(clicked(bool)), equationOfTime, SLOT(setFlagShowEOTButton(bool)));

	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveEquationOfTimeSettings()));
	connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(resetEquationOfTimeSettings()));

	updateAboutText();	
}

void EquationOfTimeWindow::updateAboutText()
{
	ui->labelTitle->setText(q_("Equation of Time plug-in"));
	QString version = QString(q_("Version %1")).arg(EQUATIONOFTIME_PLUGIN_VERSION);
	ui->labelVersion->setText(version);
}

void EquationOfTimeWindow::saveEquationOfTimeSettings()
{
	equationOfTime->saveSettingsToConfig();
}

void EquationOfTimeWindow::resetEquationOfTimeSettings()
{
	equationOfTime->restoreDefaults();
}
