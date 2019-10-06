/*
 * Copyright (C) 2019 Gion Kunz <gion.kunz@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "TelescopeClientASCOMWidget.hpp"
#include "ui_TelescopeClientASCOMWidget.h"

TelescopeClientASCOMWidget::TelescopeClientASCOMWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::TelescopeClientASCOMWidget), ascom(new ASCOMDevice(parent))
{
	ui->setupUi(this);
	ui->eqCoordTypeSourceASCOMRadio->setChecked(true);
	QObject::connect(
	  ui->chooseButton, &QPushButton::clicked, this, &TelescopeClientASCOMWidget::onChooseButtonClicked);
}

TelescopeClientASCOMWidget::~TelescopeClientASCOMWidget()
{
	delete ui;
}

void TelescopeClientASCOMWidget::onChooseButtonClicked()
{
	mSelectedDevice = ASCOMDevice::showDeviceChooser(mSelectedDevice != Q_NULLPTR ? mSelectedDevice : "");
	setSelectedDevice(mSelectedDevice);
}

QString TelescopeClientASCOMWidget::selectedDevice() const
{
	return ui->selectedDevice->text();
}

void TelescopeClientASCOMWidget::setSelectedDevice(const QString& device)
{
	mSelectedDevice = device;
	ui->selectedDevice->setText(device);
}

bool TelescopeClientASCOMWidget::useDeviceEqCoordType() const 
{
	return ui->eqCoordTypeSourceASCOMRadio->isChecked();
}

void TelescopeClientASCOMWidget::setUseDeviceEqCoordType(const bool& useDevice)
{
	if (useDevice)
	{
		ui->eqCoordTypeSourceASCOMRadio->setChecked(true);
		ui->eqCoordTypeSourceStellariumRadio->setChecked(false);
	}
	else
	{
		ui->eqCoordTypeSourceASCOMRadio->setChecked(false);
		ui->eqCoordTypeSourceStellariumRadio->setChecked(true);
	}
}
