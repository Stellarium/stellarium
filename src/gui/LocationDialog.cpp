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
		connect(ui->closeLocation, SIGNAL(clicked()), this, SLOT(close()));
		connect(ui->graphicsView, SIGNAL(positionSelected(float, float, QString)),
				this, SLOT(selectPosition(float, float, QString)));
		connect(ui->graphicsView, SIGNAL(positionHighlighted(float, float, QString)),
				this, SLOT(highlightPosition(float, float, QString)));
		connect(ui->longitudeSpinBox, SIGNAL(valueChanged(int)), this,
				SLOT(spinBoxChanged(void)));
		connect(ui->latitudeSpinBox, SIGNAL(valueChanged(int)), this,
				SLOT(spinBoxChanged(void)));
				
	}
	else
	{
		dialog->deleteLater();
		dialog = 0;
	}
}

void LocationDialog::selectPosition(float longitude, float latitude, QString city)
{
	// Set the longitude
	ui->longitudeSpinBox->setValue(longitude);
	// Set the latitude
	ui->latitudeSpinBox->setValue(latitude);
	// Set the city name
	ui->selectedLabel->setText(city);
}

void LocationDialog::highlightPosition(float longitude, float latitude, QString city)
{
	ui->cursorLabel->setText(city);
	ui->cursorLabel->update();
}

void LocationDialog::spinBoxChanged()
{
	float longitude = ui->longitudeSpinBox->getValue();
	float latitude = ui->latitudeSpinBox->getValue();
	ui->graphicsView->select(longitude, latitude);
}
