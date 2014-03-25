/*
 * Stellarium
 * Copyright (C) 2013 Alexander Wolf
 * Copyright (C) 2014 Bogdan Marinov
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

#include "CustomDeltaTEquationDialog.hpp"
#include "ui_CustomDeltaTEquationDialog.h"

#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelDeltaTMgr.hpp"
#include "StelTranslator.hpp"
#include "StelObjectMgr.hpp"

#include <QDebug>

CustomDeltaTEquationDialog::CustomDeltaTEquationDialog()
{
	ui = new Ui_CustomDeltaTEquationDialogForm;
	conf = StelApp::getInstance().getSettings();
	core = StelApp::getInstance().getCore();
	timeCorrection = core->getTimeCorrectionMgr();
	timeCorrection->getCustomAlgorithmParams(&year, &ndot,
	                                         &coeffA, &coeffB, &coeffC);
}

CustomDeltaTEquationDialog::~CustomDeltaTEquationDialog()
{
	delete ui;
	ui=NULL;
}

void CustomDeltaTEquationDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setDescription();
	}
}


void CustomDeltaTEquationDialog::createDialogContent()
{
	ui->setupUi(dialog);
	setDescription();

	ui->labelNDot->setText(QString("%1:").arg(QChar(0x1E45))); // Dotted n

	ui->lineEditCoefficientA->setText(QString("%1").arg(coeffA));
	ui->lineEditCoefficientB->setText(QString("%1").arg(coeffB));
	ui->lineEditCoefficientC->setText(QString("%1").arg(coeffC));
	ui->lineEditYear->setText(QString("%1").arg(year));
	ui->lineEditNDot->setText(QString("%1").arg(ndot));

	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// For now, I'll keep the original behavior: setting/saving. --BM
	connect(ui->lineEditNDot, SIGNAL(textEdited()),
	        this, SLOT(setValuesFromFields()));
	connect(ui->lineEditYear, SIGNAL(textEdited()),
	        this, SLOT(setValuesFromFields()));
	connect(ui->lineEditCoefficientA, SIGNAL(textEdited()),
	        this, SLOT(setValuesFromFields()));
	connect(ui->lineEditCoefficientB, SIGNAL(textEdited()),
	        this, SLOT(setValuesFromFields()));
	connect(ui->lineEditCoefficientC, SIGNAL(textEdited()),
	        this, SLOT(setValuesFromFields()));
}

void CustomDeltaTEquationDialog::setVisible(bool v)
{
	StelDialog::setVisible(v);
}

void CustomDeltaTEquationDialog::saveSettings() const
{
	conf->beginGroup("custom_time_correction");

	conf->setValue("year", year);
	conf->setValue("ndot", ndot);
	conf->setValue("coefficients", QString("%1,%2,%3").arg(coeffA).arg(coeffB).arg(coeffB));

	conf->endGroup();
}

void CustomDeltaTEquationDialog::setValuesFromFields()
{
	ndot = ui->lineEditNDot->text().toFloat();
	year = ui->lineEditYear->text().toFloat();
	coeffA = ui->lineEditCoefficientA->text().toFloat();
	coeffB = ui->lineEditCoefficientB->text().toFloat();
	coeffC = ui->lineEditCoefficientC->text().toFloat();
	timeCorrection->setCustomAlgorithmParams(year, ndot, coeffA, coeffB, coeffC);
	saveSettings();
}

void CustomDeltaTEquationDialog::setDescription() const
{
	// TODO: Fix the translations for symbol use, concatenation, etc.
	ui->stelWindowTitle->setText(q_("Custom equation for %1T").arg(QChar(0x0394)));
	ui->labelDescription->setText(q_("A typical equation for calculation of %1T looks like:").arg(QChar(0x0394)));
	ui->labelEquation->setText(QString("<strong>%1T = a + b%2u + c%3u%4,</strong>").arg(QChar(0x0394)).arg(QChar(0x00B7)).arg(QChar(0x00B7)).arg(QChar(0x00B2)));
	ui->labelSubEquation->setText(QString("%1 <em>u = (%2 - y)/100</em>").arg(q_("where")).arg(q_("year")));
}
