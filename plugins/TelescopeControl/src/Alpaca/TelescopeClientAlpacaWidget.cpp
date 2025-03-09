/*
 * Copyright (C) 2019 Gion Kunz <gion.kunz@gmail.com> (ASCOM)
 * Copyright (C) 2025 Georg Zotti (Alpaca port)
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

#include "StelApp.hpp"
#include "StelTranslator.hpp"
#include "TelescopeClientAlpacaWidget.hpp"
#include "ui_TelescopeClientAlpacaWidget.h"

TelescopeClientAlpacaWidget::TelescopeClientAlpacaWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::TelescopeClientAlpacaWidget), Alpaca(new AlpacaDevice(parent))
{
	ui->setupUi(this);
	ui->eqCoordTypeSourceAlpacaRadio->setChecked(true);
	connect(ui->chooseButton, &QPushButton::clicked, this, &TelescopeClientAlpacaWidget::onChooseButtonClicked);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
}

TelescopeClientAlpacaWidget::~TelescopeClientAlpacaWidget()
{
	delete ui;
}

void TelescopeClientAlpacaWidget::retranslate()
{
	ui->groupBox->setTitle(q_("Alpaca Settings"));
	ui->chooseButton->setText(q_("Choose Alpaca Telescope"));
	ui->selectedDeviceLabel->setText(q_("Selected Device:"));
	ui->selectedDevice->setText(q_("No device selected"));
	ui->eqCoordTypeSourceLabel->setText(q_("Source for coordinate system:"));
	ui->eqCoordTypeSourceAlpacaRadio->setToolTip(q_("Let the Alpaca device tell Stellarium what coordinate system to use. Most mounts will handle this correctly, but some might not."));
	ui->eqCoordTypeSourceAlpacaRadio->setText(q_("Let Alpaca device decide"));
	ui->eqCoordTypeSourceStellariumRadio->setToolTip(q_("Use the coordinate system that is configured in the general telescope properties for this telescope."));
	ui->eqCoordTypeSourceStellariumRadio->setText(q_("Use Stellarium settings"));
}

void TelescopeClientAlpacaWidget::onChooseButtonClicked()
{
	mSelectedDevice = AlpacaDevice::showDeviceChooser(mSelectedDevice != nullptr ? mSelectedDevice : "");
	setSelectedDevice(mSelectedDevice);
}

QString TelescopeClientAlpacaWidget::selectedDevice() const
{
	return ui->selectedDevice->text();
}

void TelescopeClientAlpacaWidget::setSelectedDevice(const QString& device)
{
	mSelectedDevice = device;
	ui->selectedDevice->setText(device);
}

bool TelescopeClientAlpacaWidget::useDeviceEqCoordType() const 
{
	return ui->eqCoordTypeSourceAlpacaRadio->isChecked();
}

void TelescopeClientAlpacaWidget::setUseDeviceEqCoordType(const bool& useDevice)
{
	ui->eqCoordTypeSourceAlpacaRadio->setChecked(useDevice);
	ui->eqCoordTypeSourceStellariumRadio->setChecked(!useDevice);
}
