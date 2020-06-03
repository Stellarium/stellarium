/*
 * Stellarium
 * Copyright (C) 2013 Alexander Wolf
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
#include "ui_customDeltaTEquationDialog.h"

#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelTranslator.hpp"
#include "StelObjectMgr.hpp"

CustomDeltaTEquationDialog::CustomDeltaTEquationDialog() : StelDialog("CustomDeltaTEquation")
{
	ui = new Ui_customDeltaTEquationDialogForm;
	conf = StelApp::getInstance().getSettings();
	core = StelApp::getInstance().getCore();

	ndot = core->getDeltaTCustomNDot();
	year = core->getDeltaTCustomYear();
	coeff = core->getDeltaTCustomEquationCoefficients();
}

CustomDeltaTEquationDialog::~CustomDeltaTEquationDialog()
{
	delete ui;
	ui=Q_NULLPTR;
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

	ui->labelNDot->setText(QString("n%1:").arg(QChar(0x2032)));

	ui->lineEditCoefficientA->setText(QString("%1").arg(coeff[0]));
	ui->lineEditCoefficientB->setText(QString("%1").arg(coeff[1]));
	ui->lineEditCoefficientC->setText(QString("%1").arg(coeff[2]));
	ui->lineEditYear->setText(QString("%1").arg(year));
	ui->lineEditNDot->setText(QString("%1").arg(ndot));

	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->lineEditNDot, SIGNAL(textEdited(const QString&)), this, SLOT(setNDot(const QString&)));
	connect(ui->lineEditYear, SIGNAL(textEdited(const QString&)), this, SLOT(setYear(const QString&)));
	connect(ui->lineEditCoefficientA, SIGNAL(textEdited(const QString&)), this, SLOT(setCoeffA(const QString&)));
	connect(ui->lineEditCoefficientB, SIGNAL(textEdited(const QString&)), this, SLOT(setCoeffB(const QString&)));
	connect(ui->lineEditCoefficientC, SIGNAL(textEdited(const QString&)), this, SLOT(setCoeffC(const QString&)));
}

void CustomDeltaTEquationDialog::saveSettings(void) const
{
	conf->beginGroup("custom_time_correction");

	conf->setValue("year", year);
	conf->setValue("ndot", ndot);
	conf->setValue("coefficients", QString("%1,%2,%3").arg(coeff[0]).arg(coeff[1]).arg(coeff[2]));

	conf->endGroup();
}

void CustomDeltaTEquationDialog::setNDot(const QString& v)
{
	ndot = v.toDouble();
	core->setDeltaTCustomNDot(ndot);
	saveSettings();
}

void CustomDeltaTEquationDialog::setYear(const QString& v)
{
	year = v.toDouble();
	core->setDeltaTCustomYear(year);
	saveSettings();
}

void CustomDeltaTEquationDialog::setCoeffA(const QString& v)
{
	coeff[0] = v.toDouble();
	core->setDeltaTCustomEquationCoefficients(coeff);
	saveSettings();
}

void CustomDeltaTEquationDialog::setCoeffB(const QString& v)
{
	coeff[1] = v.toDouble();
	core->setDeltaTCustomEquationCoefficients(coeff);
	saveSettings();
}

void CustomDeltaTEquationDialog::setCoeffC(const QString& v)
{
	coeff[2] = v.toDouble();
	core->setDeltaTCustomEquationCoefficients(coeff);
	saveSettings();
}

void CustomDeltaTEquationDialog::setDescription() const
{
	ui->stelWindowTitle->setText(q_("Custom equation for %1T").arg(QChar(0x0394)));
	ui->labelDescription->setText(q_("A typical equation for calculation of %1T looks like:").arg(QChar(0x0394)));
	ui->labelEquation->setText(QString("<strong>%1T = a + b%2u + c%3u%4,</strong>").arg(QChar(0x0394)).arg(QChar(0x00B7)).arg(QChar(0x00B7)).arg(QChar(0x00B2)));
	ui->labelSubEquation->setText(QString("%1 <em>u = (%2 - y)/100</em>").arg(q_("where")).arg(q_("year")));
	QString tooltip = q_("Secular acceleration of the Moon");
	ui->labelNDot->setToolTip(tooltip);
	ui->lineEditNDot->setToolTip(tooltip);
}
