/*
 * Stellarium TelescopeControl Plug-in
 * 
 * Copyright (C) 2009 Bogdan Marinov (this file)
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
#include "ui_telescopeConfigurationDialog.h"
#include "TelescopeConfigurationDialog.hpp"
#include "TelescopeControl.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"

#include <QDebug>
#include <QCompleter>
#include <QFrame>
#include <QTimer>

TelescopeConfigurationDialog::TelescopeConfigurationDialog()
{
	ui = new Ui_telescopeConfigurationDialogForm;
	
	telescopeManager = GETSTELMODULE(TelescopeControl);

	tcpPortValidator = new QIntValidator(0, 65535, this);//AFAIK, this is the port number range
	telescopeNameValidator = new QRegExpValidator (QRegExp("[^:\"]+"), this);//Test the update for JSON
	hostNameValidator = new QRegExpValidator (QRegExp("[a-zA-Z0-9\\-\\.]+"), this);//TODO: Write a proper host/IP regexp?
	circleListValidator = new QRegExpValidator (QRegExp("[0-9,\\.\\s]+"), this);
	#ifdef Q_OS_WIN32
	serialPortValidator = new QRegExpValidator (QRegExp("COM[0-9]+"), this);
	#else
	serialPortValidator = new QRegExpValidator (QRegExp("/dev/*"), this);
	#endif
}

TelescopeConfigurationDialog::~TelescopeConfigurationDialog()
{	
	delete ui;
	
	delete tcpPortValidator;
	delete telescopeNameValidator;
	delete hostNameValidator;
	delete circleListValidator;
}

void TelescopeConfigurationDialog::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopeConfigurationDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	connect(dialog, SIGNAL(rejected()), this, SLOT(buttonDiscardPressed()));
	
	//Connect: sender, signal, receiver, member
	connect(ui->checkBoxCircles, SIGNAL(stateChanged(int)), this, SLOT(toggleCircles(int)));
	
	connect(ui->radioButtonTelescopeLocal, SIGNAL(toggled(bool)), this, SLOT(toggleTypeServer(bool)));
	connect(ui->radioButtonTelescopeConnection, SIGNAL(toggled(bool)), this, SLOT(toggleTypeConnection(bool)));
	
	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(buttonSavePressed()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	
	connect(ui->comboBoxDeviceModel, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(deviceModelSelected(const QString&)));
	
	//Setting validators
	ui->lineEditTCPPort->setValidator(tcpPortValidator);
	ui->lineEditTelescopeName->setValidator(telescopeNameValidator);
	ui->lineEditHostName->setValidator(hostNameValidator);
	ui->lineEditCircleList->setValidator(circleListValidator);
	
	ui->comboBoxSerialPort->completer()->setModelSorting(QCompleter::CaseSensitivelySortedModel);
	
	//Initialize the style
	setStelStyle(*StelApp::getInstance().getCurrentStelStyle());
}

//Set the configuration panel in a predictable state
//TODO: Rewrite this and/or put it somewhere else
// - All the "Type" radio buttons are enabled (none is selected)
// - The "Name" field is empty
// - The "Serial port" should be visible
// - The "Device Type" combo box should list the available servers
// - The "Host" field should be disabled but empty
// - The "FOV circles" checkbox should be unchecked
// - and the list of circles should be empty and disabled
void TelescopeConfigurationDialog::initConfigurationDialog()
{
	//Reusing code used in both methods that call this one
	deviceModelNames = telescopeManager->getDeviceModels().keys();
	
	//Type
	ui->radioButtonTelescopeLocal->setEnabled(true);
	ui->radioButtonTelescopeConnection->setEnabled(true);
	
	//Name
	ui->lineEditTelescopeName->clear();
	
	//Serial port
	ui->comboBoxSerialPort->clear();
	ui->comboBoxSerialPort->addItems(SERIAL_PORT_NAMES);
	
	//Populating the list of available servers
	ui->labelDeviceModel->setVisible(true);
	ui->labelDeviceModelDescription->setVisible(true);
	ui->comboBoxDeviceModel->setVisible(true);
	ui->comboBoxDeviceModel->clear();
	if (!deviceModelNames.isEmpty())
	{
		deviceModelNames.sort();
		ui->comboBoxDeviceModel->addItems(deviceModelNames);
	}
	ui->comboBoxDeviceModel->setCurrentIndex(0);
	
	//Host
	ui->lineEditHostName->setEnabled(false);
	ui->lineEditHostName->clear();
	
	//TCP port (is this necessary?)
	ui->lineEditTCPPort->clear();
	
	//FOV circles
	ui->checkBoxCircles->setChecked(false);
	ui->labelCircles->setEnabled(false);
	ui->lineEditCircleList->setEnabled(false);
	ui->lineEditCircleList->clear();
}

void TelescopeConfigurationDialog::initNewTelescopeConfiguration(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("Add New Telescope");
	ui->lineEditTelescopeName->setText(QString("New Telescope %1").arg(QString::number(configuredSlot)));
	
	if(deviceModelNames.isEmpty())
	{
		ui->radioButtonTelescopeLocal->setEnabled(false);
		ui->radioButtonTelescopeConnection->setChecked(true);
		toggleTypeConnection(true);//Reusing code, may cause problems
	}
	else
	{
		ui->radioButtonTelescopeLocal->setEnabled(true);
		ui->radioButtonTelescopeLocal->setChecked(true);
		toggleTypeServer(true);//Reusing code, may cause problems
	}
	
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(DEFAULT_DELAY));
}

void TelescopeConfigurationDialog::initExistingTelescopeConfiguration(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("Configure Telescope");
	
	//Read the telescope properties
	QString name;
	QString host;
	int portTCP;
	int delay;
	bool connectAtStartup;
	QList<double> circles;
	QString deviceModelName;
	QString serialPortName;
	if(!telescopeManager->getTelescopeAtSlot(slot, name, host, portTCP, delay, connectAtStartup, circles, deviceModelName, serialPortName))
	{
		//TODO: Add debug
		return;
	}
	
	ui->lineEditTelescopeName->setText(name);
	
	if(!deviceModelName.isEmpty())
	{
		ui->radioButtonTelescopeLocal->setChecked(true);
		//ui->radioButtonTelescopeConnection->setEnabled(false);
		
		ui->lineEditHostName->setText("localhost");//TODO: Remove magic word!
		
		//Make the current device model selected in the list
		int index = ui->comboBoxDeviceModel->findText(deviceModelName);
		if(index < 0)
		{
			qDebug() << "TelescopeConfigurationDialog: Current device model is not in the list?";
			emit discardChanges();
			return;
		}
		else
		{
			ui->comboBoxDeviceModel->setCurrentIndex(index);
		}
		//Initialize the serial port value
		index = ui->comboBoxSerialPort->findText(serialPortName);
		if(index < 0)
		{
			//If the serial port's name is not in the list, add it and make it selected
			ui->comboBoxSerialPort->insertItem(0, serialPortName);
			ui->comboBoxSerialPort->setCurrentIndex(0);
			qDebug() << "TelescopeConfigurationDialog: Adding to serial port list:" << serialPortName;
		}
		else
			ui->comboBoxSerialPort->setCurrentIndex(index);
	}
	else //Local or remote connection
	{
		//ui->radioButtonTelescopeLocal->setEnabled(false);
		//ui->radioButtonTelescopeConnection->setEnabled(false);
		ui->radioButtonTelescopeConnection->setChecked(true);
		
		ui->toolBoxSettings->setItemEnabled(ui->toolBoxSettings->indexOf(ui->pageDeviceSettings), false);
		ui->labelHost->setEnabled(true);
		ui->lineEditHostName->setText(host);
		ui->lineEditHostName->setEnabled(true);
	}
	
	//Circles
	if(!circles.isEmpty())
	{
		ui->checkBoxCircles->setChecked(true);
		ui->labelCircles->setEnabled(true);
		ui->lineEditCircleList->setEnabled(true);
		
		QStringList circleList;
		for(int i = 0; i < circles.size(); i++)
			circleList.append(QString::number(circles[i]));
		ui->lineEditCircleList->setText(circleList.join(", "));
	}
	
	//TCP port
	ui->lineEditTCPPort->setText(QString::number(portTCP));
	
	//Delay
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(delay));//Microseconds to seconds
	
	//Connect at startup
	ui->checkBoxConnectAtStartup->setChecked(connectAtStartup);
}

void TelescopeConfigurationDialog::toggleTypeServer(bool isChecked)
{
	if(isChecked)
	{
		ui->labelHost->setEnabled(false);
		ui->lineEditHostName->setEnabled(false);
		ui->lineEditHostName->setText("localhost");
		ui->toolBoxSettings->setItemEnabled(ui->toolBoxSettings->indexOf(ui->pageDeviceSettings), true);
		ui->toolBoxSettings->setCurrentIndex(ui->toolBoxSettings->indexOf(ui->pageDeviceSettings));
		ui->comboBoxDeviceModel->setCurrentIndex(0);//Topmost device
		ui->lineEditTCPPort->setText(QString::number(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot)));
	}
}

void TelescopeConfigurationDialog::toggleTypeConnection(bool isChecked)
{
	if(isChecked)
	{
		ui->labelHost->setEnabled(true);
		ui->lineEditHostName->setEnabled(true);
		ui->lineEditHostName->setText("localhost");
		ui->toolBoxSettings->setItemEnabled(ui->toolBoxSettings->indexOf(ui->pageDeviceSettings), false);
		ui->lineEditTCPPort->setText(QString::number(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot)));
	}
}

void TelescopeConfigurationDialog::toggleCircles(int state)
{
	if(state == Qt::Checked)
	{
		ui->labelCircles->setEnabled(true);
		ui->lineEditCircleList->setEnabled(true);
		ui->lineEditCircleList->setFocus();
	}
	else
	{
		ui->labelCircles->setEnabled(false);
		ui->lineEditCircleList->clear();
		ui->lineEditCircleList->setEnabled(false);
	}
}

void TelescopeConfigurationDialog::buttonSavePressed()
{
	//Main telescope properties
	QString name = ui->lineEditTelescopeName->text().trimmed();
	if(name.isEmpty())
		return;
	QString host = ui->lineEditHostName->text();
	if(host.isEmpty())//TODO: Validate host
		return;
	
	int delay = MICROSECONDS_FROM_SECONDS(ui->doubleSpinBoxTelescopeDelay->value());
	int portTCP = ui->lineEditTCPPort->text().toInt();
	bool connectAtStartup = ui->checkBoxConnectAtStartup->isChecked();
	
	//Circles
	//TODO: This will change if there is a validator for that field
	QList<double> circles;
	QString rawCircles = ui->lineEditCircleList->text().trimmed();
	QStringList circleStrings;
	if(ui->checkBoxCircles->isChecked() && !(rawCircles.isEmpty()))
	{
		circleStrings = rawCircles.simplified().remove(' ').split(',', QString::SkipEmptyParts);
		circleStrings.removeDuplicates();
		circleStrings.sort();
		
		for(int i = 0; i < circleStrings.size(); i++)
		{
			if(i >= MAX_CIRCLE_COUNT)
				break;
			double circle = circleStrings.at(i).toDouble();
			if(circle > 0.0)
				circles.append(circle);
		}
	}
	
	//Type and server properties
	//TODO: When adding, check for success!
	TelescopeConnection type = ConnectionNA;
	if(ui->radioButtonTelescopeLocal->isChecked())
	{
		//Read the serial port
		QString serialPortName = ui->comboBoxSerialPort->currentText();
		if(!serialPortName.startsWith(SERIAL_PORT_PREFIX))
			return;//TODO: Add more validation!
		
		type = ConnectionLocalInternal;
		telescopeManager->addTelescopeAtSlot(configuredSlot, name, host, portTCP, delay, connectAtStartup, circles, ui->comboBoxDeviceModel->currentText(), serialPortName);
	}
	else if (ui->radioButtonTelescopeConnection->isChecked())
	{
		if(host == "localhost")
			type = ConnectionLocalExternal;
		else
			type = ConnectionRemote;
		telescopeManager->addTelescopeAtSlot(configuredSlot, name, host, portTCP, delay, connectAtStartup, circles);
	}
	
	emit saveChanges(name, type);
}

void TelescopeConfigurationDialog::buttonDiscardPressed()
{
	emit discardChanges();
}

void TelescopeConfigurationDialog::deviceModelSelected(const QString& deviceModelName)
{
	ui->labelDeviceModelDescription->setText(telescopeManager->getDeviceModels().value(deviceModelName).description);
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(telescopeManager->getDeviceModels().value(deviceModelName).defaultDelay));
}

void TelescopeConfigurationDialog::setStelStyle(const StelStyle& style)
{
	if(dialog)
		dialog->setStyleSheet(telescopeManager->getModuleStyleSheet(style).qtStyleSheet);
}
