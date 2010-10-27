/*
 * Comet and asteroids importer plug-in for Stellarium
 *
 * Copyright (C) 2010 Bogdan Marinov
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "CAImporter.hpp"

#include "ManualImportWindow.hpp"
#include "ui_manualImportWindow.h"

#include <QColor>
#include <QColorDialog>

#include "StelApp.hpp"
#include "StelModuleMgr.hpp"


ManualImportWindow::ManualImportWindow()
{
	ui = new Ui_manualImportWindow();
	ssoManager = GETSTELMODULE(CAImporter);
}

ManualImportWindow::~ManualImportWindow()
{
	delete ui;
}

void ManualImportWindow::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->lineEditColor, SIGNAL(textChanged(QString)), this, SLOT(parseColorString(QString)));
	connect(ui->pushButtonSelectColor, SIGNAL(clicked()), this, SLOT(selectColor()));

	ui->labelLongitudeOfTheAscendingNode->setText(QString("Longitude of the ascending node %1:").arg(QChar(0x03A9)));//Capital omega
	ui->radioButtonArgumentOfPeriapsis->setText(QString("Argument of periapsis %1:").arg(QChar(0x3C9)));//Lowercase omega
	ui->radioButtonLongitudeOfPeriapsis->setText(QString("Longitude of periapsis %1:").arg(QChar(0x3D6)));
}

void ManualImportWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void ManualImportWindow::selectColor()
{
	QColor color = QColorDialog::getColor(objectColor);
	objectColor =  color;
	ui->lineEditColor->setText(QString("%1, %2, %3").arg(color.redF()).arg(color.greenF()).arg(color.blueF()));
	setColorButtonColor(color);
}

void ManualImportWindow::parseColorString(QString colorCode)
{
	QStringList colorComponents = colorCode.split(QChar(','));
	int count = colorComponents.count();
	if (count < 3 || count > 4)
		return;

	bool ok;
	double red = colorComponents.at(0).toDouble(&ok);
	if (!ok || red < 0.0 || red > 1.0)
		return;
	double green = colorComponents.at(1).toDouble(&ok);
	if (!ok || green < 0.0 || green > 1.0)
		return;
	double blue = colorComponents.at(2).toDouble(&ok);
	if (!ok || blue < 0.0 || blue > 1.0)
		return;

	QColor color;
	color.setRedF(red);
	color.setGreenF(green);
	color.setBlueF(blue);

	if (count == 4)
	{
		double alpha = colorComponents.at(3).toDouble(&ok);
		if (!ok || alpha < 0.0 || alpha > 1.0)
			return;
		color.setAlphaF(alpha);
	}

	objectColor = color;
	setColorButtonColor(color);
}

void ManualImportWindow::setColorButtonColor(QColor newColor)
{
	qDebug() << "setColorButtonColor()";
	QPixmap pixmap(16, 16);
	pixmap.fill(newColor);
	ui->pushButtonSelectColor->setIcon(QIcon(pixmap));
}

void ManualImportWindow::toggleCometOrbit(bool)
{
	//
}

void ManualImportWindow::toggleEllipticOrbit(bool)
{
	//
}

void ManualImportWindow::toggleObjectSpecificOrbit(bool)
{
	//
}

void ManualImportWindow::toggleMeanMotionOrPeriod(bool)
{
	//
}

