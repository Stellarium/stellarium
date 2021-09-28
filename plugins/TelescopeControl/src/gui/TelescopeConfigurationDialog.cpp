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

#include "TelescopeConfigurationDialog.hpp"
#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelStyle.hpp"
#include "StelTranslator.hpp"
#include "TelescopeControl.hpp"
#include "ui_telescopeConfigurationDialog.h"

#ifdef Q_OS_WIN
	#include "../common/ASCOMSupport.hpp"
#endif

#include <QCompleter>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QTimer>
#include <QtSerialPort/QSerialPortInfo>

TelescopeConfigurationDialog::TelescopeConfigurationDialog()
	: StelDialog("TelescopeControlConfiguration"), configuredSlot(0)
{
	ui = new Ui_telescopeConfigurationDialog();

	telescopeManager = GETSTELMODULE(TelescopeControl);

	telescopeNameValidator = new QRegExpValidator(QRegExp("[^:\"]+"), this); // Test the update for JSON
	hostNameValidator =
	  new QRegExpValidator(QRegExp("[a-zA-Z0-9\\-\\.]+"), this); // TODO: Write a proper host/IP regexp?
	circleListValidator = new QRegExpValidator(QRegExp("[0-9,\\.\\s]+"), this);
#ifdef Q_OS_WIN
	serialPortValidator = new QRegExpValidator(QRegExp("COM[0-9]+"), this);
#else
	serialPortValidator = new QRegExpValidator(QRegExp("/.*"), this);
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
	QStringList* plist = new QStringList();
	for (const auto& serialPortInfo : QSerialPortInfo::availablePorts())
	{
#ifdef Q_OS_WIN
		plist->append(serialPortInfo.portName()); // Use COM1 in the GUI instead \\.\COM1 naming
#else
		plist->append(serialPortInfo.systemLocation());
#endif
		qDebug() << "[TelescopeControl] port name:" << serialPortInfo.portName()
				 << "; vendor identifier:" << serialPortInfo.vendorIdentifier()
				 << "; product identifier:" << serialPortInfo.productIdentifier();
	}

// on linux find some virtual ports
#ifdef Q_OS_LINUX
	QStringList filters;
	filters << "ttyNET*"
			<< "ttynet*"
			<< "Telescope*";
	// look in /dev/*
	QDir dev("/dev");
	dev.setFilter(QDir::System);
	dev.setSorting(QDir::Reversed);
	dev.setNameFilters(filters);
	QFileInfoList list = dev.entryInfoList();
	for (int i = 0; i < list.size(); i++)
	{
		QFileInfo fileInfo = list.at(i);
		plist->append(fileInfo.absoluteFilePath());
	}
	// look in /tmp/* for non-root virtual ports (append ttyS8 and ttyUSB*)
	filters << "ttyS*"
			<< "ttyUSB*";
	QDir tmp("/tmp");
	tmp.setFilter(QDir::System);
	tmp.setSorting(QDir::Reversed);
	tmp.setNameFilters(filters);
	list = tmp.entryInfoList();
	for (int i = 0; i < list.size(); i++)
	{
		QFileInfo fileInfo = list.at(i);
		plist->append(fileInfo.absoluteFilePath());
	}
#endif

	return plist;
}

void TelescopeConfigurationDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		populateToolTips();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopeConfigurationDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// ASCOM Telescope client widget needs to be dynamically added in order to make use of preprocessors to exclude for non-windows
	#ifdef Q_OS_WIN
	ascomWidget = new TelescopeClientASCOMWidget(ui->scrollAreaWidgetContents);
	ui->ASCOMLayout->addWidget(ascomWidget);

	if (!ASCOMSupport::isASCOMSupported())
	{
		ui->radioButtonTelescopeASCOM->hide();
	}
	#endif

	// Inherited connect
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(dialog, SIGNAL(rejected()), this, SLOT(buttonDiscardPressed()));

	// Connect: sender, signal, receiver, member
	connect(ui->radioButtonTelescopeLocal, SIGNAL(toggled(bool)), this, SLOT(toggleTypeLocal(bool)));
	connect(ui->radioButtonTelescopeConnection, SIGNAL(toggled(bool)), this, SLOT(toggleTypeConnection(bool)));
	connect(ui->radioButtonTelescopeVirtual, SIGNAL(toggled(bool)), this, SLOT(toggleTypeVirtual(bool)));
	connect(ui->radioButtonTelescopeRTS2, SIGNAL(toggled(bool)), this, SLOT(toggleTypeRTS2(bool)));
	connect(ui->radioButtonTelescopeINDI, SIGNAL(toggled(bool)), this, SLOT(toggleTypeINDI(bool)));
	#ifdef Q_OS_WIN
	connect(ui->radioButtonTelescopeASCOM, SIGNAL(toggled(bool)), this, SLOT(toggleTypeASCOM(bool)));
	#else
	ui->radioButtonTelescopeASCOM->hide();
	#endif

	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(buttonSavePressed()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));

	connect(ui->comboBoxDeviceModel, SIGNAL(currentIndexChanged(const QString&)), this,
	  SLOT(deviceModelSelected(const QString&)));

	// Setting validators
	ui->lineEditTelescopeName->setValidator(telescopeNameValidator);
	ui->lineEditHostName->setValidator(hostNameValidator);
	ui->lineEditCircleList->setValidator(circleListValidator);
	ui->comboSerialPort->setValidator(serialPortValidator);

	populateToolTips();
}

void TelescopeConfigurationDialog::populateToolTips()
{
	ui->doubleSpinBoxTelescopeDelay->setToolTip(
	  QString("<p>%1</p>")
		.arg(q_("The approximate time it takes for the signals from the telescope to reach Stellarium. "
				"Increase this value if the reticle is skipping.")));
	ui->doubleSpinBoxRTS2Refresh->setToolTip(
	  QString("<p>%1</p>")
		.arg(q_("Refresh rate of the RTS2 telescope. Delay before sending next telescope status request. The "
				"default value of 0.5 second works fine with most setups.")));
}

// Set the configuration panel in a predictable state
void TelescopeConfigurationDialog::initConfigurationDialog()
{
	ui->groupBoxConnectionSettings->hide();
	ui->groupBoxDeviceSettings->hide();
	ui->groupBoxRTS2Settings->hide();
	ui->INDIProperties->hide();
	#ifdef Q_OS_WIN
	ascomWidget->hide();
	#endif

	// Reusing code used in both methods that call this one
	deviceModelNames = telescopeManager->getDeviceModels().keys();

	// Name
	ui->lineEditTelescopeName->clear();

	// Equinox
	ui->radioButtonJ2000->setChecked(true);

	// Connect at startup
	ui->checkBoxConnectAtStartup->setChecked(false);

	// Serial port
	QStringList* plist = listSerialPorts();
	ui->comboSerialPort->clear();
	ui->comboSerialPort->addItems(*plist);
	ui->comboSerialPort->activated(plist->value(0));
	ui->comboSerialPort->setEditText(plist->value(0));
	delete (plist);

	// Populating the list of available devices
	ui->comboBoxDeviceModel->clear();
	if (!deviceModelNames.isEmpty())
	{
		deviceModelNames.sort();
		ui->comboBoxDeviceModel->addItems(deviceModelNames);
	}
	ui->comboBoxDeviceModel->setCurrentIndex(0);

	// FOV circles
	ui->checkBoxCircles->setChecked(false);
	ui->lineEditCircleList->clear();

	// It is very unlikely that this situation will happen any more due to the
	// embedded telescope servers.
	if (deviceModelNames.isEmpty())
	{
		ui->radioButtonTelescopeLocal->setEnabled(false);
		ui->radioButtonTelescopeConnection->setChecked(true);
		toggleTypeConnection(true); // Not called if the button is already checked
	}
	else
	{
		ui->radioButtonTelescopeLocal->setEnabled(true);
		ui->radioButtonTelescopeLocal->setChecked(true);
		toggleTypeLocal(true); // Not called if the button is already checked
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

	// Read the telescope properties
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
	QString rts2Url;
	QString rts2Username;
	QString rts2Password;
	int rts2Refresh;
	QString ascomDeviceId;
	bool ascomUseDeviceEqCoordType;

	if (!telescopeManager->getTelescopeAtSlot(slot, connectionType, name, equinox, host, portTCP, delay,
		  connectAtStartup, circles, deviceModelName, serialPortName, rts2Url, rts2Username, rts2Password,
		  rts2Refresh, ascomDeviceId, ascomUseDeviceEqCoordType))
	{
		// TODO: Add debug
		return;
	}
	ui->lineEditTelescopeName->setText(name);

	if (connectionType == ConnectionInternal && !deviceModelName.isEmpty())
	{
		ui->radioButtonTelescopeLocal->setChecked(true);
		ui->lineEditHostName->setText("localhost"); // TODO: Remove magic word!

		// Make the current device model selected in the list
		int index = ui->comboBoxDeviceModel->findText(deviceModelName);
		if (index < 0)
		{
			qDebug() << "TelescopeConfigurationDialog: Current device model is not in the list?";
			emit changesDiscarded();
			return;
		}
		else
			ui->comboBoxDeviceModel->setCurrentIndex(index);

		// Initialize the serial port value
		ui->comboSerialPort->activated(serialPortName);
		ui->comboSerialPort->setEditText(serialPortName);
	}
	else if (connectionType == ConnectionRemote)
	{
		ui->radioButtonTelescopeConnection->setChecked(true); // Calls toggleTypeConnection(true)
		ui->lineEditHostName->setText(host);
	}
	else if (connectionType == ConnectionLocal)
	{
		ui->radioButtonTelescopeConnection->setChecked(true);
		ui->lineEditHostName->setText("localhost");
	}
	else if (connectionType == ConnectionVirtual)
	{
		ui->radioButtonTelescopeVirtual->setChecked(true);
	}
	else if (connectionType == ConnectionRTS2)
	{
		ui->radioButtonTelescopeRTS2->setChecked(true);
		ui->lineEditRTS2Url->setText(rts2Url);
		ui->lineEditRTS2Username->setText(rts2Username);
		ui->lineEditRTS2Password->setText(rts2Password);
		ui->doubleSpinBoxRTS2Refresh->setValue(SECONDS_FROM_MICROSECONDS(rts2Refresh));
	}
	else if (connectionType == ConnectionINDI)
	{
		ui->radioButtonTelescopeINDI->setChecked(true);
		ui->INDIProperties->setHost(host);
		ui->INDIProperties->setPort(portTCP);
		ui->INDIProperties->setSelectedDevice(deviceModelName);
	}
	#ifdef Q_OS_WIN
	else if (connectionType == ConnectionASCOM)
	{
		ui->radioButtonTelescopeASCOM->setChecked(true);
		ascomWidget->setSelectedDevice(ascomDeviceId);
		ascomWidget->setUseDeviceEqCoordType(ascomUseDeviceEqCoordType);
	}
	#endif

	// Equinox
	if (equinox == "JNow")
		ui->radioButtonJNow->setChecked(true);
	else
		ui->radioButtonJ2000->setChecked(true);

	// Circles
	if (!circles.isEmpty())
	{
		ui->checkBoxCircles->setChecked(true);

		QStringList circleList;
		for (int i = 0; i < circles.size(); i++)
			circleList.append(QString::number(circles[i]));
		ui->lineEditCircleList->setText(circleList.join(", "));
	}

	// TCP port
	ui->spinBoxTCPPort->setValue(portTCP);

	// Delay
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(delay)); // Microseconds to seconds

	// Connect at startup
	ui->checkBoxConnectAtStartup->setChecked(connectAtStartup);
}

void TelescopeConfigurationDialog::toggleTypeLocal(bool isChecked)
{
	if (isChecked)
	{
		// Re-initialize values that may have been changed
		ui->comboBoxDeviceModel->setCurrentIndex(0);
		QStringList* plist = listSerialPorts();
		ui->comboSerialPort->activated(plist->value(0));
		ui->comboSerialPort->setEditText(plist->value(0));
		delete (plist);
		ui->lineEditHostName->setText("localhost");
		ui->spinBoxTCPPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

		ui->groupBoxDeviceSettings->show();

		ui->scrollArea->ensureWidgetVisible(ui->groupBoxTelescopeProperties);
	}
	else
	{
		ui->groupBoxDeviceSettings->hide();
	}
}

void TelescopeConfigurationDialog::toggleTypeConnection(bool isChecked)
{
	if (isChecked)
	{
		// Re-initialize values that may have been changed
		ui->lineEditHostName->setText("localhost");
		ui->spinBoxTCPPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

		ui->groupBoxConnectionSettings->show();

		ui->scrollArea->ensureWidgetVisible(ui->groupBoxTelescopeProperties);
	}
	else
	{
		ui->groupBoxConnectionSettings->hide();
	}
}

void TelescopeConfigurationDialog::toggleTypeVirtual(bool isChecked)
{
	Q_UNUSED(isChecked);
	ui->scrollArea->ensureWidgetVisible(ui->groupBoxTelescopeProperties);
}

void TelescopeConfigurationDialog::toggleTypeRTS2(bool isChecked)
{
	if (isChecked)
	{
		// Re-initialize values that may have been changed
		ui->lineEditRTS2Url->setText("localhost:8889");

		ui->groupBoxRTS2Settings->show();

		ui->scrollArea->ensureWidgetVisible(ui->groupBoxRTS2Settings);
	}
	else
	{
		ui->groupBoxRTS2Settings->hide();
	}
}

void TelescopeConfigurationDialog::toggleTypeINDI(bool enabled)
{
	ui->INDIProperties->setVisible(enabled);
}

#ifdef Q_OS_WIN
void TelescopeConfigurationDialog::toggleTypeASCOM(bool enabled)
{
	ascomWidget->setVisible(enabled);	
}
#endif

void TelescopeConfigurationDialog::buttonSavePressed()
{
	// Main telescope properties
	QString name = ui->lineEditTelescopeName->text().trimmed();

	if (name.isEmpty()) return;

	QString host = ui->lineEditHostName->text();

	if (host.isEmpty()) // Remove validation of hostname
		return;

	int delay = qRound(MICROSECONDS_FROM_SECONDS(ui->doubleSpinBoxTelescopeDelay->value()));
	int portTCP = ui->spinBoxTCPPort->value();
	bool connectAtStartup = ui->checkBoxConnectAtStartup->isChecked();

	// Circles
	// TODO: This will change if there is a validator for that field
	QList<double> circles;
	QString rawCircles = ui->lineEditCircleList->text().trimmed();
	QStringList circleStrings;
	if (ui->checkBoxCircles->isChecked() && !(rawCircles.isEmpty()))
	{
		#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
		circleStrings = rawCircles.simplified().remove(' ').split(',', Qt::SkipEmptyParts);
		#else
		circleStrings = rawCircles.simplified().remove(' ').split(',', QString::SkipEmptyParts);
		#endif
		circleStrings.removeDuplicates();
		circleStrings.sort();

		for (int i = 0; i < circleStrings.size(); i++)
		{
			if (i >= MAX_CIRCLE_COUNT) break;
			double circle = circleStrings.at(i).toDouble();
			if (circle > 0.0) circles.append(circle);
		}
	}

	QString equinox("J2000");
	if (ui->radioButtonJNow->isChecked()) equinox = "JNow";

	// Type and server properties
	// TODO: When adding, check for success!
	ConnectionType type = ConnectionNA;
	if (ui->radioButtonTelescopeLocal->isChecked())
	{
		// Read the serial port
		QString serialPortName = ui->comboSerialPort->currentText();
		type = ConnectionInternal;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, equinox, host, portTCP, delay,
		  connectAtStartup, circles, ui->comboBoxDeviceModel->currentText(), serialPortName);
	}
	else if (ui->radioButtonTelescopeConnection->isChecked())
	{
		if (host == "localhost")
			type = ConnectionLocal;
		else
			type = ConnectionRemote;
		telescopeManager->addTelescopeAtSlot(
		  configuredSlot, type, name, equinox, host, portTCP, delay, connectAtStartup, circles);
	}
	else if (ui->radioButtonTelescopeVirtual->isChecked())
	{
		type = ConnectionVirtual;
		telescopeManager->addTelescopeAtSlot(
		  configuredSlot, type, name, equinox, QString(), portTCP, delay, connectAtStartup, circles);
	}
	else if (ui->radioButtonTelescopeRTS2->isChecked())
	{
		type = ConnectionRTS2;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, equinox, host, portTCP, delay,
		  connectAtStartup, circles, QString(), QString(), ui->lineEditRTS2Url->text(),
		  ui->lineEditRTS2Username->text(), ui->lineEditRTS2Password->text(),
		  qRound(MICROSECONDS_FROM_SECONDS(ui->doubleSpinBoxRTS2Refresh->value())));
	}
	else if (ui->radioButtonTelescopeINDI->isChecked())
	{
		type = ConnectionINDI;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, equinox, ui->INDIProperties->host(),
		  ui->INDIProperties->port(), delay, connectAtStartup, circles, ui->INDIProperties->selectedDevice());
	}
	#ifdef Q_OS_WIN
	else if (ui->radioButtonTelescopeASCOM->isChecked())
	{
		type = ConnectionASCOM;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, equinox, host, portTCP, delay,
		  connectAtStartup, circles, QString(), QString(), QString(), QString(), QString(), -1,
		  ascomWidget->selectedDevice(), ascomWidget->useDeviceEqCoordType());
	}
	#endif

	emit changesSaved(name, type);
}

void TelescopeConfigurationDialog::buttonDiscardPressed()
{
	emit changesDiscarded();
}

void TelescopeConfigurationDialog::deviceModelSelected(const QString& deviceModelName)
{
	ui->labelDeviceModelDescription->setText(
	  q_(telescopeManager->getDeviceModels().value(deviceModelName).description));
	ui->doubleSpinBoxTelescopeDelay->setValue(
	  SECONDS_FROM_MICROSECONDS(telescopeManager->getDeviceModels().value(deviceModelName).defaultDelay));
}
