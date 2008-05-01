/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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

#include "Dialog.hpp"
#include "LocationDialog.hpp"
#include "StelMainWindow.hpp"
#include "ui_locationDialogGui.h"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "Observer.hpp"

#include <QDebug>
#include <QFrame>

LocationDialog::LocationDialog() : dialog(0)
{
	ui = new Ui_locationDialogForm;
}

LocationDialog::~LocationDialog()
{
	delete ui;
}

void LocationDialog::close()
{
	emit closed();
}

void LocationDialog::setVisible(bool v)
{
	if (v) 
	{
		dialog = new DialogFrame(&StelMainWindow::getInstance());
		ui->setupUi(dialog);
		dialog->raise();
		dialog->move(200, 100);	// TODO: put in the center of the screen
		dialog->setVisible(true);

		ui->longitudeSpinBox->setDisplayFormat(AngleSpinBox::DMSLetters);
		ui->longitudeSpinBox->setPrefixType(AngleSpinBox::Longitude);
		ui->latitudeSpinBox->setDisplayFormat(AngleSpinBox::DMSLetters);
		ui->latitudeSpinBox->setPrefixType(AngleSpinBox::Latitude);

		connect(ui->closeLocation, SIGNAL(clicked()), this, SLOT(close()));
		connect(ui->graphicsView, SIGNAL(positionSelected(double, double, QString)), this, SLOT(selectPosition(double, double, QString)));
		connect(ui->graphicsView, SIGNAL(positionHighlighted(double, double, QString)), this, SLOT(highlightPosition(double, double, QString)));
		connect(ui->longitudeSpinBox, SIGNAL(valueChanged(void)), this, SLOT(spinBoxChanged(void)));
		connect(ui->latitudeSpinBox, SIGNAL(valueChanged(void)), this, SLOT(spinBoxChanged(void)));
				
		selectPosition(StelApp::getInstance().getCore()->getObservatory()->getLongitude(), StelApp::getInstance().getCore()->getObservatory()->getLatitude(), "");
		spinBoxChanged();
	}
	else
	{
		dialog->deleteLater();
		dialog = 0;
	}
}

void LocationDialog::selectPosition(double longitude, double latitude, QString city)
{
	// Set the longitude
	ui->longitudeSpinBox->setDegrees(longitude);
	// Set the latitude
	ui->latitudeSpinBox->setDegrees(latitude);
	// Set the city name
	ui->selectedLabel->setText(city);
	
	StelApp::getInstance().getCore()->getObservatory()->setLongitude(longitude);
	StelApp::getInstance().getCore()->getObservatory()->setLatitude(latitude);
}

void LocationDialog::highlightPosition(double longitude, double latitude, QString city)
{
	ui->cursorLabel->setText(city);
	ui->cursorLabel->update();
}

void LocationDialog::spinBoxChanged()
{
	double longitude = ui->longitudeSpinBox->valueDegrees();
	double latitude = ui->latitudeSpinBox->valueDegrees();
	ui->graphicsView->select(longitude, latitude);
}

