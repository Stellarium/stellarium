/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009-2010 Bogdan Marinov (this file)
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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"
#include "TelescopeControl.hpp"
#include "TelescopeConfigurationDialog.hpp"
#include "TelescopeDialog.hpp"
#include "ui_telescopeDialog.h"
#include "StelGui.hpp"
#include <QDebug>
#include <QFrame>
#include <QTimer>
#include <QFile>
#include <QFileDialog>
#include <QHash>
#include <QHeaderView>
#include <QSettings>
#include <QStandardItem>

using namespace TelescopeControlGlobals;


TelescopeDialog::TelescopeDialog()
{
	ui = new Ui_telescopeDialogForm;
	
	//TODO: Fix this - it's in the same plugin
	telescopeManager = GETSTELMODULE(TelescopeControl);
	telescopeListModel = new QStandardItemModel(0, ColumnCount);
	
	//TODO: This shouldn't be a hash...
	statusString[StatusNA] = QString("N/A");
	statusString[StatusStarting] = QString("Starting");
	statusString[StatusConnecting] = QString("Connecting");
	statusString[StatusConnected] = QString("Connected");
	statusString[StatusDisconnected] = QString("Disconnected");
	statusString[StatusStopped] = QString("Stopped");
	
	telescopeCount = 0;
}

TelescopeDialog::~TelescopeDialog()
{	
	delete ui;
	
	delete telescopeListModel;
}

void TelescopeDialog::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopeDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	
	//Connect: sender, signal, receiver, method
	//Page: Telescopes
	connect(ui->pushButtonChangeStatus, SIGNAL(clicked()), this, SLOT(buttonChangeStatusPressed()));
	connect(ui->pushButtonConfigure, SIGNAL(clicked()), this, SLOT(buttonConfigurePressed()));
	connect(ui->pushButtonAdd, SIGNAL(clicked()), this, SLOT(buttonAddPressed()));
	connect(ui->pushButtonRemove, SIGNAL(clicked()), this, SLOT(buttonRemovePressed()));
	
	connect(ui->telescopeTreeView, SIGNAL(clicked (const QModelIndex &)), this, SLOT(selectTelecope(const QModelIndex &)));
	//connect(ui->telescopeTreeView, SIGNAL(activated (const QModelIndex &)), this, SLOT(configureTelescope(const QModelIndex &)));
	
	//Page: Options:
	connect(ui->checkBoxReticles, SIGNAL(stateChanged(int)), this, SLOT(toggleReticles(int)));
	connect(ui->checkBoxLabels, SIGNAL(stateChanged(int)), this, SLOT(toggleLabels(int)));
	connect(ui->checkBoxCircles, SIGNAL(stateChanged(int)), this, SLOT(toggleCircles(int)));
	
	connect(ui->checkBoxEnableLogs, SIGNAL(toggled(bool)), telescopeManager, SLOT(setFlagUseTelescopeServerLogs(bool)));
	
	connect(ui->checkBoxUseExecutables, SIGNAL(toggled(bool)), ui->labelExecutablesDirectory, SLOT(setEnabled(bool)));
	connect(ui->checkBoxUseExecutables, SIGNAL(toggled(bool)), ui->lineEditExecutablesDirectory, SLOT(setEnabled(bool)));
	connect(ui->checkBoxUseExecutables, SIGNAL(toggled(bool)), ui->pushButtonPickExecutablesDirectory, SLOT(setEnabled(bool)));
	connect(ui->checkBoxUseExecutables, SIGNAL(toggled(bool)), this, SLOT(checkBoxUseExecutablesToggled(bool)));
	
	connect(ui->pushButtonPickExecutablesDirectory, SIGNAL(clicked()), this, SLOT(buttonBrowseServerDirectoryPressed()));
	
	//In other dialogs:
	connect(&configurationDialog, SIGNAL(changesDiscarded()), this, SLOT(discardChanges()));
	connect(&configurationDialog, SIGNAL(changesSaved(QString, ConnectionType)), this, SLOT(saveChanges(QString, ConnectionType)));
	
	//Initialize the style
	updateStyle();
	
	//Initializing the list of telescopes
	telescopeListModel->setColumnCount(ColumnCount);
	QStringList headerStrings;
	headerStrings << "#";
	//headerStrings << "Start";
	headerStrings <<  "Status";
	headerStrings << "Type";
	headerStrings << "Name";
	telescopeListModel->setHorizontalHeaderLabels(headerStrings);
	
	ui->telescopeTreeView->setModel(telescopeListModel);
	ui->telescopeTreeView->header()->setMovable(false);
	ui->telescopeTreeView->header()->setResizeMode(ColumnSlot, QHeaderView::ResizeToContents);
	ui->telescopeTreeView->header()->setStretchLastSection(true);
	
	//Populating the list
	QStandardItem * tempItem;
	QModelIndex modelIndex;

	//Cycle the slots
	for (int slotNumber = MIN_SLOT_NUMBER; slotNumber < SLOT_NUMBER_LIMIT; slotNumber++)
	{
		//Slot #
		//int slotNumber = (i+1)%SLOT_COUNT;//Making sure slot 0 is last
		
		//Make sure that this is initialized for all slots
		telescopeStatus[slotNumber] = StatusNA;
		
		//Read the telescope properties
		QString name;
		ConnectionType connectionType;
		QString equinox;
		QString host;
		int portTCP;
		int delay;
		bool connectAtStartup;
		QList<double> circles;
		QString serverName;
		QString portSerial;
		if(!telescopeManager->getTelescopeAtSlot(slotNumber, connectionType, name, equinox, host, portTCP, delay, connectAtStartup, circles, serverName, portSerial))
			continue;
		
		//Determine the server type
		QString connectionTypeLabel;
		switch (connectionType)
		{
			case ConnectionInternal:
				connectionTypeLabel = "local, Stellarium";
				break;
			case ConnectionLocal:
				connectionTypeLabel = "local, external";
				break;
			case ConnectionRemote:
				connectionTypeLabel = "remote, unknown";
				break;
			case ConnectionVirtual:
			default:
				connectionTypeLabel = "virtual";
				break;
		}
		telescopeType[slotNumber] = connectionType;
		
		//Determine the telescope's status
		if (telescopeManager->isConnectedClientAtSlot(slotNumber))
		{
			telescopeStatus[slotNumber] = StatusConnected;
		}
		else
		{
			//TODO: Fix this!
			//At startup everything exists and attempts to connect
			telescopeStatus[slotNumber] = StatusConnecting;
		}
		
		//New column on a new row in the list: Slot number
		int lastRow = telescopeListModel->rowCount();
		tempItem = new QStandardItem(QString::number(slotNumber));
		tempItem->setEditable(false);
		telescopeListModel->setItem(lastRow, ColumnSlot, tempItem);
		
		//TODO: This is not updated, because it was commented out
		//tempItem = new QStandardItem;
		//tempItem->setEditable(false);
		//tempItem->setCheckable(true);
		//tempItem->setCheckState(Qt::Checked);
		//tempItem->setData("If checked, this telescope will start when Stellarium is started", Qt::ToolTipRole);
		//telescopeListModel->setItem(lastRow, ColumnStartup, tempItem);//Start-up checkbox
		
		//New column on a new row in the list: Telescope status
		tempItem = new QStandardItem(statusString[telescopeStatus[slotNumber]]);
		tempItem->setEditable(false);
		telescopeListModel->setItem(lastRow, ColumnStatus, tempItem);
		
		//New column on a new row in the list: Telescope type
		tempItem = new QStandardItem(connectionTypeLabel);
		tempItem->setEditable(false);
		telescopeListModel->setItem(lastRow, ColumnType, tempItem);
		
		//New column on a new row in the list: Telescope name
		tempItem = new QStandardItem(name);
		tempItem->setEditable(false);
		telescopeListModel->setItem(lastRow, ColumnName, tempItem);
		
		//After everything is done, count this as loaded
		telescopeCount++;
	}
	
	//Finished populating the table, let's sort it by slot number
	//ui->telescopeTreeView->setSortingEnabled(true);//Set in the .ui file
	ui->telescopeTreeView->sortByColumn(ColumnSlot, Qt::AscendingOrder);
	//(Works even when the table is empty)
	//(Makes redundant the delay of 0 above)
	
	//TODO: Reuse code.
	if(telescopeCount > 0)
	{
		ui->telescopeTreeView->setFocus();
		ui->telescopeTreeView->header()->setResizeMode(ColumnType, QHeaderView::ResizeToContents);
		ui->labelWarning->setText(LABEL_TEXT_CONTROL_TIP);
	}
	else
	{
		ui->pushButtonChangeStatus->setEnabled(false);
		ui->pushButtonConfigure->setEnabled(false);
		ui->pushButtonRemove->setEnabled(false);
		ui->pushButtonAdd->setFocus();
		if(telescopeManager->getDeviceModels().isEmpty())
			ui->labelWarning->setText(LABEL_TEXT_NO_DEVICE_MODELS);
		else
			ui->labelWarning->setText(LABEL_TEXT_ADD_TIP);
	}
	
	if(telescopeCount >= SLOT_COUNT)
		ui->pushButtonAdd->setEnabled(false);
	
	//Checkboxes
	ui->checkBoxReticles->setChecked(telescopeManager->getFlagTelescopeReticles());
	ui->checkBoxLabels->setChecked(telescopeManager->getFlagTelescopeLabels());
	ui->checkBoxCircles->setChecked(telescopeManager->getFlagTelescopeCircles());
	ui->checkBoxEnableLogs->setChecked(telescopeManager->getFlagUseTelescopeServerLogs());
	
	//Telescope server directory
	ui->checkBoxUseExecutables->setChecked(telescopeManager->getFlagUseServerExecutables());
	ui->lineEditExecutablesDirectory->setText(telescopeManager->getServerExecutablesDirectoryPath());
	
	//About page
	//TODO: Expand
	QString htmlPage = "<html><head></head><body>";
	htmlPage += "<h2>Stellarium Telescope Control Plug-in</h2>";
	htmlPage += "<h3>Version " + QString(PLUGIN_VERSION) + "</h3>";
	QFile helpFile(":/telescopeControl/help.utf8");
	helpFile.open(QFile::ReadOnly | QFile::Text);
	htmlPage += helpFile.readAll();
	helpFile.close();
	htmlPage += "</body></html>";
	ui->textBrowserAbout->setHtml(htmlPage);
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->textBrowserAbout->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	
	//Everything must be initialized by now, start the updateTimer
	//TODO: Find if it's possible to run it only when the dialog is visible
	QTimer* updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateTelescopeStates()));
	updateTimer->start(200);
}

void TelescopeDialog::toggleReticles(int state)
{
	if(state == Qt::Checked)
	{
		telescopeManager->setFlagTelescopeReticles(true);
	}
	else
	{
		telescopeManager->setFlagTelescopeReticles(false);
	}
}

void TelescopeDialog::toggleLabels(int state)
{
	if(state == Qt::Checked)
	{
		telescopeManager->setFlagTelescopeLabels(true);
	}
	else
	{
		telescopeManager->setFlagTelescopeLabels(false);
	}
}

void TelescopeDialog::toggleCircles(int state)
{
	if(state == Qt::Checked)
	{
		telescopeManager->setFlagTelescopeCircles(true);
	}
	else
	{
		telescopeManager->setFlagTelescopeCircles(false);
	}
}

void TelescopeDialog::selectTelecope(const QModelIndex & index)
{
	//Extract selected item index
	int selectedSlot = telescopeListModel->data( telescopeListModel->index(index.row(),0) ).toInt();
	updateStatusButtonForSlot(selectedSlot);

	//In all cases
	ui->pushButtonRemove->setEnabled(true);
}

void TelescopeDialog::configureTelescope(const QModelIndex & currentIndex)
{
	configuredTelescopeIsNew = false;
	configuredSlot = telescopeListModel->data( telescopeListModel->index(currentIndex.row(), ColumnSlot) ).toInt();
	
	//Stop the telescope first if necessary
	if(telescopeType[configuredSlot] != ConnectionInternal && telescopeStatus[configuredSlot] != StatusDisconnected)
	{
		if(telescopeManager->stopTelescopeAtSlot(configuredSlot)) //Act as "Disconnect"
				telescopeStatus[configuredSlot] = StatusDisconnected;
		else
			return;
	}
	else if(telescopeStatus[configuredSlot] != StatusStopped)
	{
		if(telescopeManager->stopTelescopeAtSlot(configuredSlot)) //Act as "Stop"
					telescopeStatus[configuredSlot] = StatusStopped;
	}
	//Update the status in the list
	telescopeListModel->setData(telescopeListModel->index(ui->telescopeTreeView->currentIndex().row(), ColumnStatus), statusString[telescopeStatus[configuredSlot]], Qt::DisplayRole);
	
	setVisible(false);
	configurationDialog.setVisible(true); //This should be called first to actually create the dialog content
	
	configurationDialog.initExistingTelescopeConfiguration(configuredSlot);
}

void TelescopeDialog::buttonChangeStatusPressed()
{
	if(!ui->telescopeTreeView->currentIndex().isValid())
		return;
	
	//Extract selected slot
	int selectedSlot = telescopeListModel->data( telescopeListModel->index(ui->telescopeTreeView->currentIndex().row(), ColumnSlot) ).toInt();
	
	//TODO: As most of these are asynchronous actions, it looks like that there should be a queue...
	
	if(telescopeType[selectedSlot] != ConnectionInternal) 
	{
		//Can't be launched by Stellarium -> can't be stopped by Stellarium
		//Can be only connected/disconnected
		if(telescopeStatus[selectedSlot] == StatusDisconnected)
		{
			if(telescopeManager->startTelescopeAtSlot(selectedSlot)) //Act as "Connect"
				telescopeStatus[selectedSlot] = StatusConnecting;
		}
		else
		{
			if(telescopeManager->stopTelescopeAtSlot(selectedSlot)) //Act as "Disconnect"
				telescopeStatus[selectedSlot] = StatusDisconnected;
		}
	}
	else
	{
		switch(telescopeStatus[selectedSlot]) //Why the switch?
		{
			case StatusNA:
			case StatusStopped:
			{
				if(telescopeManager->startTelescopeAtSlot(selectedSlot)) //Act as "Start"
					telescopeStatus[selectedSlot] = StatusConnecting;
			}
			break;
			case StatusConnecting:
			case StatusConnected:
			{
				if(telescopeManager->stopTelescopeAtSlot(selectedSlot)) //Act as "Stop"
					telescopeStatus[selectedSlot] = StatusStopped;
			}
			break;
			default:
				break;
		}
	}
	
	//Update the status in the list
	telescopeListModel->setData(telescopeListModel->index(ui->telescopeTreeView->currentIndex().row(), ColumnStatus), statusString[telescopeStatus[selectedSlot]], Qt::DisplayRole);
}

void TelescopeDialog::buttonConfigurePressed()
{
	if(ui->telescopeTreeView->currentIndex().isValid())
		configureTelescope(ui->telescopeTreeView->currentIndex());
}

void TelescopeDialog::buttonAddPressed()
{
	if(telescopeCount >= SLOT_COUNT)
		return;
	
	configuredTelescopeIsNew = true;
	
	//Find the first unoccupied slot (there is at least one)
	for (configuredSlot = MIN_SLOT_NUMBER; configuredSlot < SLOT_NUMBER_LIMIT; configuredSlot++)
	{
		//configuredSlot = (i+1)%SLOT_COUNT;
		if(telescopeStatus[configuredSlot] == StatusNA)
			break;
	}
	
	setVisible(false);
	configurationDialog.setVisible(true); //This should be called first to actually create the dialog content
	configurationDialog.initNewTelescopeConfiguration(configuredSlot);
}

void TelescopeDialog::buttonRemovePressed()
{
	if(!ui->telescopeTreeView->currentIndex().isValid())
		return;
	
	//Extract selected slot
	int selectedSlot = telescopeListModel->data( telescopeListModel->index(ui->telescopeTreeView->currentIndex().row(),0) ).toInt();
	
	//Stop the telescope if necessary and remove it
	if(telescopeManager->stopTelescopeAtSlot(selectedSlot))
	{
		//TODO: Update status?
		if(!telescopeManager->removeTelescopeAtSlot(selectedSlot))
		{
			//TODO: Add debug
			return;
		}
	}
	else
	{
		//TODO: Add debug
		return;
	}
	
	//Save the changes to file
	telescopeManager->saveTelescopes();
	
	telescopeStatus[selectedSlot] = StatusNA;
	telescopeCount -= 1;
	
//Update the interface to reflect the changes:
	
	//Make sure that the header section keeps it size
	if(telescopeCount == 0)
		ui->telescopeTreeView->header()->setResizeMode(ColumnType, QHeaderView::Interactive);
	
	//Remove the telescope from the table/tree
	telescopeListModel->removeRow(ui->telescopeTreeView->currentIndex().row());
	
	//If there are less than the maximal number of telescopes now, new ones can be added
	if(telescopeCount < SLOT_COUNT)
		ui->pushButtonAdd->setEnabled(true);
	
	//If there are no telescopes left, disable some buttons
	if(telescopeCount == 0)
	{
		//TODO: Fix the phantom text of the Status button (reuse code?)
		//IDEA: Vsible/invisible instead of enabled/disabled?
		//The other buttons expand to take the place (delete spacers)
		ui->pushButtonChangeStatus->setEnabled(false);
		ui->pushButtonConfigure->setEnabled(false);
		ui->pushButtonRemove->setEnabled(false);
		if(telescopeManager->getDeviceModels().isEmpty())
			ui->labelWarning->setText(LABEL_TEXT_NO_DEVICE_MODELS);
		else
			ui->labelWarning->setText(LABEL_TEXT_ADD_TIP);
	}
	else
	{
		ui->labelWarning->setText(LABEL_TEXT_CONTROL_TIP);
		ui->telescopeTreeView->setCurrentIndex(telescopeListModel->index(0,0));
	}
}

void TelescopeDialog::saveChanges(QString name, ConnectionType type)
{
	//Save the changes to file
	telescopeManager->saveTelescopes();
	
	//Type and server properties
	telescopeType[configuredSlot] = type;
	QString typeString;
	switch (type)
	{
		case ConnectionVirtual:
			telescopeStatus[configuredSlot] = StatusStopped;
			typeString = "virtual";
			break;

		case ConnectionInternal:
			if(configuredTelescopeIsNew)
				telescopeStatus[configuredSlot] = StatusStopped;//TODO: Is there a point? Isn't it better to force the status update method?
			typeString = "local, Stellarium";
			break;

		case ConnectionLocal:
			telescopeStatus[configuredSlot] = StatusDisconnected;
			typeString = "local, external";
			break;

		case ConnectionRemote:
		default:
			telescopeStatus[configuredSlot] = StatusDisconnected;
			typeString = "remote, unknown";
	}
	
	//Update the model/list
	if(configuredTelescopeIsNew)
	{
		QList<QStandardItem *> newRow;
		newRow << new QStandardItem(QString::number(configuredSlot))
		       << new QStandardItem(statusString[telescopeStatus[configuredSlot]])
		       << new QStandardItem(typeString)
		       << new QStandardItem(name);
		telescopeListModel->appendRow(newRow);
		telescopeCount++;
	}
	else
	{
		int currentRow = ui->telescopeTreeView->currentIndex().row();
		//ColumnSlot doesn't need to be updated. :)
		telescopeListModel->setData(telescopeListModel->index(currentRow, ColumnStatus), statusString[telescopeStatus[configuredSlot]], Qt::DisplayRole);
		telescopeListModel->setData(telescopeListModel->index(currentRow, ColumnType), typeString, Qt::DisplayRole);
		telescopeListModel->setData(telescopeListModel->index(currentRow, ColumnName), name, Qt::DisplayRole);
	}
	//Sort the updated table by slot number
	ui->telescopeTreeView->sortByColumn(ColumnSlot, Qt::AscendingOrder);
	
	//Can't add more telescopes if they have reached the maximum number
	if (telescopeCount >= SLOT_COUNT)
		ui->pushButtonAdd->setEnabled(false);
	
	//
	if (telescopeCount == 0)
	{
		ui->pushButtonChangeStatus->setEnabled(false);
		ui->pushButtonConfigure->setEnabled(false);
		ui->pushButtonRemove->setEnabled(false);
		ui->telescopeTreeView->header()->setResizeMode(ColumnType, QHeaderView::Interactive);
		if(telescopeManager->getDeviceModels().isEmpty())
			ui->labelWarning->setText(LABEL_TEXT_NO_DEVICE_MODELS);
		else
			ui->labelWarning->setText(LABEL_TEXT_ADD_TIP);
	}
	else
	{
		ui->telescopeTreeView->setFocus();
		ui->telescopeTreeView->setCurrentIndex(telescopeListModel->index(0,0));
		ui->pushButtonConfigure->setEnabled(true);
		ui->pushButtonRemove->setEnabled(true);
		ui->telescopeTreeView->header()->setResizeMode(ColumnType, QHeaderView::ResizeToContents);
		ui->labelWarning->setText(LABEL_TEXT_CONTROL_TIP);
	}
	
	configuredTelescopeIsNew = false;
	configurationDialog.setVisible(false);
	setVisible(true);//Brings the current window to the foreground
}

void TelescopeDialog::discardChanges()
{
	configurationDialog.setVisible(false);
	setVisible(true);//Brings the current window to the foreground
	
	if (telescopeCount >= SLOT_COUNT)
		ui->pushButtonAdd->setEnabled(false);
	if (telescopeCount == 0)
		ui->pushButtonRemove->setEnabled(false);
	
	configuredTelescopeIsNew = false;
}

void TelescopeDialog::updateTelescopeStates()
{
	if(telescopeCount == 0)
		return;
	
	int slotNumber = -1;
	for (int i=0; i<(telescopeListModel->rowCount()); i++)
	{
		slotNumber = telescopeListModel->data( telescopeListModel->index(i, ColumnSlot) ).toInt();
		//TODO: Check if these cover all possibilites
		if (telescopeManager->isConnectedClientAtSlot(slotNumber))
		{
			telescopeStatus[slotNumber] = StatusConnected;
		}
		else if(telescopeManager->isExistingClientAtSlot(slotNumber))
		{
			telescopeStatus[slotNumber] = StatusConnecting;
		}
		else
		{
			if(telescopeType[slotNumber] == ConnectionInternal)
				telescopeStatus[slotNumber] = StatusStopped;
			else
				telescopeStatus[slotNumber] = StatusDisconnected;
		}
		
		//Update the status in the list
		telescopeListModel->setData(telescopeListModel->index(i, ColumnStatus), statusString[telescopeStatus[slotNumber]], Qt::DisplayRole);
	}
	
	if(ui->telescopeTreeView->currentIndex().isValid())
	{
		int selectedSlot = telescopeListModel->data( telescopeListModel->index(ui->telescopeTreeView->currentIndex().row(), ColumnSlot) ).toInt();
		//Update the ChangeStatus button
		updateStatusButtonForSlot(selectedSlot);
	}
}

void TelescopeDialog::updateStatusButtonForSlot(int selectedSlot)
{
	if(telescopeType[selectedSlot] != ConnectionInternal)
	{
		//Can't be launched by Stellarium => can't be stopped by Stellarium
		//Can be only connected/disconnected
		if(telescopeStatus[selectedSlot] == StatusDisconnected)
		{
			setStatusButtonToConnect();
			ui->pushButtonChangeStatus->setEnabled(true);
		}
		else
		{
			setStatusButtonToDisconnect();
			ui->pushButtonChangeStatus->setEnabled(true);
		}
	}
	else
	{
		switch(telescopeStatus[selectedSlot])
		{
			case StatusNA:
			case StatusStopped:
				setStatusButtonToStart();
				ui->pushButtonChangeStatus->setEnabled(true);
				break;
			case StatusConnected:
			case StatusConnecting:
				setStatusButtonToStop();
				ui->pushButtonChangeStatus->setEnabled(true);
				break;
			default:
				setStatusButtonToStart();
				ui->pushButtonChangeStatus->setEnabled(false);
				ui->pushButtonConfigure->setEnabled(false);
				ui->pushButtonRemove->setEnabled(false);
				break;
		}
	}
}

void TelescopeDialog::setStatusButtonToStart()
{
	ui->pushButtonChangeStatus->setText("Start");
	ui->pushButtonChangeStatus->setToolTip("Start the selected local telescope");
}

void TelescopeDialog::setStatusButtonToStop()
{
	ui->pushButtonChangeStatus->setText("Stop");
	ui->pushButtonChangeStatus->setToolTip("Stop the selected local telescope");
}

void TelescopeDialog::setStatusButtonToConnect()
{
	ui->pushButtonChangeStatus->setText("Connect");
	ui->pushButtonChangeStatus->setToolTip("Connect to the selected telescope");
}

void TelescopeDialog::setStatusButtonToDisconnect()
{
	ui->pushButtonChangeStatus->setText("Disconnect");
	ui->pushButtonChangeStatus->setToolTip("Disconnect from the selected telescope");
}

void TelescopeDialog::updateStyle()
{
	if (dialog)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		QString style(gui->getStelStyle().htmlStyleSheet);
		ui->textBrowserAbout->document()->setDefaultStyleSheet(style);
	}
}

void TelescopeDialog::checkBoxUseExecutablesToggled(bool useExecutables)
{
	telescopeManager->setFlagUseServerExecutables(useExecutables);
}

void TelescopeDialog::buttonBrowseServerDirectoryPressed()
{
	QString newPath = QFileDialog::getExistingDirectory (0, QString("Select a directory"), telescopeManager->getServerExecutablesDirectoryPath());
	//TODO: Validation? Directory exists and contains servers?
	if(!newPath.isEmpty())
	{
		ui->lineEditExecutablesDirectory->setText(newPath);
		telescopeManager->setServerExecutablesDirectoryPath(newPath);
		telescopeManager->setFlagUseServerExecutables(true);
	}
}
