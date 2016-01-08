/*
 * Stellarium TelescopeControl Plug-in
 * 
 * Copyright (C) 2009-2011 Bogdan Marinov (this file)
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
#include "StelTranslator.hpp"

#include <QDebug>
#include <QCompleter>
#include <QFrame>
#include <QTimer>
#include <QtSerialPort/QSerialPortInfo>
#include <QFile>
#include <QDir>

TelescopeConfigurationDialog::TelescopeConfigurationDialog()
	: configuredSlot(0)
{
	ui = new Ui_telescopeConfigurationDialog;
	
	telescopeManager = GETSTELMODULE(TelescopeControl);

	telescopeNameValidator = new QRegExpValidator (QRegExp("[^:\"]+"), this);//Test the update for JSON
	hostNameValidator = new QRegExpValidator (QRegExp("[a-zA-Z0-9\\-\\.]+"), this);//TODO: Write a proper host/IP regexp?
	circleListValidator = new QRegExpValidator (QRegExp("[0-9,\\.\\s]+"), this);
	#ifdef Q_OS_WIN
	serialPortValidator = new QRegExpValidator (QRegExp("COM[0-9]+"), this);
	#else
	serialPortValidator = new QRegExpValidator (QRegExp("/.*"), this);
	#endif
}

TelescopeConfigurationDialog::~TelescopeConfigurationDialog()
{	
	delete ui;
	
	delete telescopeNameValidator;
	delete hostNameValidator;
	delete circleListValidator;
	delete serialPortValidator;
}

QStringList* TelescopeConfigurationDialog::listSerialPorts()
{
	// list real serial ports
	QStringList *plist = new QStringList();
	foreach (const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()) {
		plist->append(serialPortInfo.systemLocation());
	}

// on linux find some virtual ports
#ifdef Q_OS_LINUX
	QStringList filters;
	filters << "ttyNET*" << "ttynet*" << "Telescope*";
	// look in /dev/*
	QDir dev("/dev");
	dev.setFilter(QDir::System);
	dev.setSorting(QDir::Reversed);
	dev.setNameFilters(filters);
	QFileInfoList list = dev.entryInfoList();
	for (int i = 0; i < list.size(); i ++) {
		QFileInfo fileInfo = list.at(i);
		plist->append(fileInfo.absoluteFilePath());
	}
	// look in /tmp/* for non-root virtual ports (append ttyS8 and ttyUSB*)
	filters << "ttyS*" << "ttyUSB*";
	QDir tmp("/tmp");
	tmp.setFilter(QDir::System);
	tmp.setSorting(QDir::Reversed);
	tmp.setNameFilters(filters);
	list = tmp.entryInfoList();
	for (int i = 0; i < list.size(); i ++) {
		QFileInfo fileInfo = list.at(i);
		plist->append(fileInfo.absoluteFilePath());
	}
#endif

	return plist;
}

void TelescopeConfigurationDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopeConfigurationDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	connect(dialog, SIGNAL(rejected()), this, SLOT(buttonDiscardPressed()));
	
	//Connect: sender, signal, receiver, member
	connect(ui->radioButtonTelescopeLocal, SIGNAL(toggled(bool)), this, SLOT(toggleTypeLocal(bool)));
	connect(ui->radioButtonTelescopeConnection, SIGNAL(toggled(bool)), this, SLOT(toggleTypeConnection(bool)));
	connect(ui->radioButtonTelescopeVirtual, SIGNAL(toggled(bool)), this, SLOT(toggleTypeVirtual(bool)));
	
	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(buttonSavePressed()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	
	connect(ui->comboBoxDeviceModel, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(deviceModelSelected(const QString&)));
	
	//Setting validators
	ui->lineEditTelescopeName->setValidator(telescopeNameValidator);
	ui->lineEditHostName->setValidator(hostNameValidator);
	ui->lineEditCircleList->setValidator(circleListValidator);
	ui->comboSerialPort->setValidator(serialPortValidator);
}

//Set the configuration panel in a predictable state
void TelescopeConfigurationDialog::initConfigurationDialog()
{
	//Reusing code used in both methods that call this one
	deviceModelNames = telescopeManager->getDeviceModels().keys();
	
	//Name
	ui->lineEditTelescopeName->clear();

	//Equinox
	ui->radioButtonJ2000->setChecked(true);

	//Connect at startup
	ui->checkBoxConnectAtStartup->setChecked(false);

	//Serial port
	QStringList *plist = listSerialPorts();
	ui->comboSerialPort->clear();
	ui->comboSerialPort->addItems(*plist);
	ui->comboSerialPort->activated(plist->value(0));
	ui->comboSerialPort->setEditText(plist->value(0));
	delete(plist);
	
	//Populating the list of available devices
	ui->comboBoxDeviceModel->clear();
	if (!deviceModelNames.isEmpty())
	{
		deviceModelNames.sort();
		ui->comboBoxDeviceModel->addItems(deviceModelNames);
	}
	ui->comboBoxDeviceModel->setCurrentIndex(0);
	
	//FOV circles
	ui->checkBoxCircles->setChecked(false);
	ui->lineEditCircleList->clear();
	
	//It is very unlikely that this situation will happen any more due to the
	//embedded telescope servers.
	if(deviceModelNames.isEmpty())
	{
		ui->radioButtonTelescopeLocal->setEnabled(false);
		ui->radioButtonTelescopeConnection->setChecked(true);
		toggleTypeConnection(true);//Not called if the button is already checked
	}
	else
	{
		ui->radioButtonTelescopeLocal->setEnabled(true);
		ui->radioButtonTelescopeLocal->setChecked(true);
		toggleTypeLocal(true);//Not called if the button is already checked
	}
}

void TelescopeConfigurationDialog::initNewTelescopeConfiguration(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText(q_("Add New Telescope"));
	ui->lineEditTelescopeName->setText(QString("New Telescope %1").arg(QString::number(configuredSlot)));
	
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(DEFAULT_DELAY));
}

void TelescopeConfigurationDialog::initExistingTelescopeConfiguration(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText(q_("Configure Telescope"));
	
	//Read the telescope properties
	QString name;
	ConnectionType connectionType;
	QString equinox;
	QString host;
	int portTCP;
	int delay;
	bool connectAtStartup;
	QList<double> circles;
	QString deviceModelName;
	QString serialPortName;
	if(!telescopeManager->getTelescopeAtSlot(slot, connectionType, name, equinox, host, portTCP, delay, connectAtStartup, circles, deviceModelName, serialPortName))
	{
		//TODO: Add debug
		return;
	}
	
	ui->lineEditTelescopeName->setText(name);
	
	if(!deviceModelName.isEmpty())
	{
		ui->radioButtonTelescopeLocal->setChecked(true);
		
		ui->lineEditHostName->setText("localhost");//TODO: Remove magic word!
		
		//Make the current device model selected in the list
		int index = ui->comboBoxDeviceModel->findText(deviceModelName);
		if(index < 0)
		{
			qDebug() << "TelescopeConfigurationDialog: Current device model is not in the list?";
			emit changesDiscarded();
			return;
		}
		else
		{
			ui->comboBoxDeviceModel->setCurrentIndex(index);
		}
		//Initialize the serial port value
		ui->comboSerialPort->activated(serialPortName);
		ui->comboSerialPort->setEditText(serialPortName);
	}
	else if (connectionType == ConnectionRemote)
	{
		ui->radioButtonTelescopeConnection->setChecked(true);//Calls toggleTypeConnection(true)
		ui->lineEditHostName->setText(host);
	}
	else if (connectionType == ConnectionLocal)
	{
		ui->radioButtonTelescopeConnection->setChecked(true);
		ui->lineEditHostName->setText("localhost");
	}
	else
	{
		ui->radioButtonTelescopeVirtual->setChecked(true);
	}

	//Equinox
	if (equinox == "JNow")
		ui->radioButtonJNow->setChecked(true);
	else
		ui->radioButtonJ2000->setChecked(true);
	
	//Circles
	if(!circles.isEmpty())
	{
		ui->checkBoxCircles->setChecked(true);
		
		QStringList circleList;
		for(int i = 0; i < circles.size(); i++)
			circleList.append(QString::number(circles[i]));
		ui->lineEditCircleList->setText(circleList.join(", "));
	}
	
	//TCP port
	ui->spinBoxTCPPort->setValue(portTCP);
	
	//Delay
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(delay));//Microseconds to seconds
	
	//Connect at startup
	ui->checkBoxConnectAtStartup->setChecked(connectAtStartup);
}

void TelescopeConfigurationDialog::toggleTypeLocal(bool isChecked)
{
	if(isChecked)
	{
		//Re-initialize values that may have been changed
		ui->comboBoxDeviceModel->setCurrentIndex(0);
		QStringList *plist = listSerialPorts();
		ui->comboSerialPort->activated(plist->value(0));
		ui->comboSerialPort->setEditText(plist->value(0));
		delete(plist);
		ui->lineEditHostName->setText("localhost");
		ui->spinBoxTCPPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

		//Enable/disable controls
		ui->labelHost->setEnabled(false);
		ui->lineEditHostName->setEnabled(false);

		ui->scrollArea->ensureWidgetVisible(ui->groupBoxTelescopeProperties);
	}
	else
	{
		ui->labelHost->setEnabled(true);
		ui->lineEditHostName->setEnabled(true);
	}
}

void TelescopeConfigurationDialog::toggleTypeConnection(bool isChecked)
{
	if(isChecked)
	{
		//Re-initialize values that may have been changed
		ui->lineEditHostName->setText("localhost");
		ui->spinBoxTCPPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

		ui->groupBoxDeviceSettings->setEnabled(false);

		ui->scrollArea->ensureWidgetVisible(ui->groupBoxTelescopeProperties);
	}
	else
	{
		ui->groupBoxDeviceSettings->setEnabled(true);
	}
}

void TelescopeConfigurationDialog::toggleTypeVirtual(bool isChecked)
{
	//TODO: This really should be done in the GUI
	ui->groupBoxDeviceSettings->setEnabled(!isChecked);
	ui->groupBoxConnectionSettings->setEnabled(!isChecked);

	ui->scrollArea->ensureWidgetVisible(ui->groupBoxTelescopeProperties);
}

void TelescopeConfigurationDialog::buttonSavePressed()
{
	//Main telescope properties
	QString name = ui->lineEditTelescopeName->text().trimmed();

	if(name.isEmpty())
		return;

	QString host = ui->lineEditHostName->text();

	if(host.isEmpty() || !validateHost(host))
		return;
	
	int delay = MICROSECONDS_FROM_SECONDS(ui->doubleSpinBoxTelescopeDelay->value());
	int portTCP = ui->spinBoxTCPPort->value();
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

	QString equinox("J2000");
	if (ui->radioButtonJNow->isChecked())
		equinox = "JNow";
	
	//Type and server properties
	//TODO: When adding, check for success!
	ConnectionType type = ConnectionNA;
	if(ui->radioButtonTelescopeLocal->isChecked())
	{
		//Read the serial port
		QString serialPortName = ui->comboSerialPort->currentText();
		#ifdef Q_OS_WIN
		if(!serialPortName.startsWith(SERIAL_PORT_PREFIX) && !serialPortName.startsWith(SERIAL_PORT_PREFIX2))
			return;//TODO: Add more validation!
		#else
		if(!serialPortName.startsWith(SERIAL_PORT_PREFIX))
			return;//TODO: Add more validation!
		#endif
		
		type = ConnectionInternal;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, equinox, host, portTCP, delay, connectAtStartup, circles, ui->comboBoxDeviceModel->currentText(), serialPortName);
	}
	else if (ui->radioButtonTelescopeConnection->isChecked())
	{
		if(host == "localhost")
			type = ConnectionLocal;
		else
			type = ConnectionRemote;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, equinox, host, portTCP, delay, connectAtStartup, circles);
	}
	else if (ui->radioButtonTelescopeVirtual->isChecked())
	{
		type = ConnectionVirtual;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, equinox, QString(), portTCP, delay, connectAtStartup, circles);
	}
	
	emit changesSaved(name, type);
}

void TelescopeConfigurationDialog::buttonDiscardPressed()
{
	emit changesDiscarded();
}

void TelescopeConfigurationDialog::deviceModelSelected(const QString& deviceModelName)
{
	ui->labelDeviceModelDescription->setText(telescopeManager->getDeviceModels().value(deviceModelName).description);
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(telescopeManager->getDeviceModels().value(deviceModelName).defaultDelay));
}

bool TelescopeConfigurationDialog::validateHost(QString hostName)
{
    // Simple validation by ping
    int exitCode;
#ifdef Q_OS_WIN
    // WTF? It's not working anymore!
    //exitCode = QProcess::execute("ping", QStringList() << "-n 1" << hostName);
    exitCode = 0;
#else
    exitCode = QProcess::execute("ping", QStringList() << "-c1" << hostName);
#endif
    return (0 == exitCode);
    //TODO: Add debug if host not alive?
}
