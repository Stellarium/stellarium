/*
 * Copyright (C) 2017 Alessandro Siniscalchi <asiniscalchi@gmail.com>
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

#include "TelescopeClientINDIWidget.hpp"
#include "ui_TelescopeClientINDIWidget.h"

TelescopeClientINDIWidget::TelescopeClientINDIWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TelescopeClientINDIWidget)
{
	ui->setupUi(this);
	QObject::connect(ui->connectButton, &QPushButton::clicked, this, &TelescopeClientINDIWidget::onConnectionButtonClicked);
	QObject::connect(&mConnection, &INDIConnection::newDeviceReceived, this, &TelescopeClientINDIWidget::onDevicesChanged);
	QObject::connect(&mConnection, &INDIConnection::removeDeviceReceived, this, &TelescopeClientINDIWidget::onDevicesChanged);
	QObject::connect(&mConnection, &INDIConnection::serverDisconnectedReceived, this, &TelescopeClientINDIWidget::onServerDisconnected);
}

TelescopeClientINDIWidget::~TelescopeClientINDIWidget()
{
	delete ui;
}

QString TelescopeClientINDIWidget::host() const
{
	return ui->lineEditHostName->text();
}

void TelescopeClientINDIWidget::setHost(const QString &host)
{
	ui->lineEditHostName->setText(host);
}

int TelescopeClientINDIWidget::port() const
{
	return ui->spinBoxTCPPort->value();
}

void TelescopeClientINDIWidget::setPort(int port)
{
	ui->spinBoxTCPPort->setValue(port);
}

QString TelescopeClientINDIWidget::selectedDevice() const
{
	return ui->devicesComboBox->currentText();
}

void TelescopeClientINDIWidget::setSelectedDevice(const QString &device)
{
	ui->devicesComboBox->setCurrentText(device);
}

void TelescopeClientINDIWidget::onConnectionButtonClicked()
{
	if (mConnection.isServerConnected())
		mConnection.disconnectServer();

	QString host = ui->lineEditHostName->text();
	QString port = ui->spinBoxTCPPort->text();
	mConnection.setServer(host.toStdString().c_str(), port.toInt());
	mConnection.connectServer();
}

void TelescopeClientINDIWidget::onDevicesChanged()
{
	ui->devicesComboBox->clear();
	auto devices = mConnection.devices();
	ui->devicesComboBox->addItems(devices);
}

void TelescopeClientINDIWidget::onServerDisconnected(int code)
{
	Q_UNUSED(code)
	ui->devicesComboBox->clear();
}

