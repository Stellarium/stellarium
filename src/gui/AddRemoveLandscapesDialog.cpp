/*
 * Stellarium
 * 
 * Copyright (C) 2010 Bogdan Marinov (add/remove landscapes feature)
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
#include "AddRemoveLandscapesDialog.hpp"
#include "ui_addRemoveLandscapesDialog.h"

#include "Dialog.hpp"
#include "LandscapeMgr.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"

#include <QDebug>
#include <QFileDialog>
#include <QString>

AddRemoveLandscapesDialog::AddRemoveLandscapesDialog()
{
	ui = new Ui_addRemoveLandscapesDialogForm;
	
	landscapeManager = GETSTELMODULE(LandscapeMgr);

	lastUsedDirectoryPath = QDir::homePath();

	//TODO: Find a way to have this initialized from CMake
	defaultLandscapeIDs = (QStringList() << "guereins" << "trees" << "moon" << "hurricane" << "ocean" << "garching" << "mars" << "saturn");
}

AddRemoveLandscapesDialog::~AddRemoveLandscapesDialog()
{
	delete ui;
}

void AddRemoveLandscapesDialog::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void AddRemoveLandscapesDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Connect all signals and slots here: sender, signal, receiver, method
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->pushButtonAdd, SIGNAL(clicked()), this, SLOT(buttonAddClicked()));
	connect(ui->listWidgetUserLandscapes, SIGNAL(currentRowChanged(int)), this, SLOT(landscapeListCurrentRowChanged(int)));
	connect(ui->pushButtonRemove, SIGNAL(clicked()), this, SLOT(buttonRemoveClicked()));
	connect(ui->pushButtonMessageOK, SIGNAL(clicked()), this, SLOT(messageAcknowledged()));

	connect(landscapeManager, SIGNAL(landscapesChanged()), this, SLOT(populateLists()));

	ui->groupBoxMessage->setVisible(false);

	populateLists();
}

void AddRemoveLandscapesDialog::populateLists()
{
	ui->listWidgetUserLandscapes->clear();
	QStringList landscapes = landscapeManager->getAllLandscapeIDs();
	foreach (QString landscapeID, defaultLandscapeIDs)
	{
		landscapes.removeAll(landscapeID);
	}
	if (!landscapes.isEmpty())
	{
		landscapes.sort();
		ui->listWidgetUserLandscapes->addItems(landscapes);
		if((ui->listWidgetUserLandscapes->findItems(landscapeManager->getCurrentLandscapeID(), Qt::MatchExactly).isEmpty()))
		{
			//If the current landscape is not in the list, simply select the first row
			ui->listWidgetUserLandscapes->setCurrentRow(0);
		}
		else
		{
			ui->listWidgetUserLandscapes->setCurrentItem(ui->listWidgetUserLandscapes->findItems(landscapeManager->getCurrentLandscapeID(), Qt::MatchExactly).first());
		}
	}
	else
	{
		//Force disabling the side pane
		landscapeListCurrentRowChanged(-1);
	}
}

void AddRemoveLandscapesDialog::buttonAddClicked()
{
	QString sourceArchivePath = QFileDialog::getOpenFileName(NULL, "Select a ZIP archive containing a Stellarium landscape...", lastUsedDirectoryPath, "ZIP archives (*.zip)");
	bool useLandscape = ui->checkBoxUseLandscape->isChecked();
	if (!sourceArchivePath.isEmpty() && QFile::exists(sourceArchivePath))
	{
		QString newLandscapeID = landscapeManager->installLandscapeFromArchive(sourceArchivePath, useLandscape);
		if(!newLandscapeID.isEmpty())
		{
			//Show a message
			displayMessage("Add", "Landscape installed successfully.");
			ui->groupBoxAdd->setVisible(false);
			//Make the new landscape selected in the list
			//populateLists(); //No longer needed after the migration to signals/slots
			ui->listWidgetUserLandscapes->setCurrentItem((ui->listWidgetUserLandscapes->findItems(newLandscapeID, Qt::MatchExactly)).first());
		}
		else
		{
			//Show an error message
			displayMessage("Error!", "Landscape was not installed.");
			ui->groupBoxAdd->setVisible(false);
		}
	}
}

void AddRemoveLandscapesDialog::buttonRemoveClicked()
{
	QString landscapeID = ui->listWidgetUserLandscapes->currentItem()->data(0).toString();
	if(landscapeManager->removeLandscape(landscapeID))
	{
		//populateLists();//No longer needed after the migration to signals/slots
		//TODO: Display messages instead
	}

}

void AddRemoveLandscapesDialog::landscapeListCurrentRowChanged(int newRow)
{
	bool displaySidePane = (newRow >= 0);
	ui->labelLandscapeName->setVisible(displaySidePane);
	ui->labelLandscapeSize->setVisible(displaySidePane);
	ui->pushButtonRemove->setEnabled(displaySidePane);
	ui->labelWarning->setEnabled(displaySidePane);

	if (!displaySidePane)
		return;

	QString landscapeID = ui->listWidgetUserLandscapes->item(newRow)->data(0).toString();
	//Name
	ui->labelLandscapeName->setText("<h3>"+landscapeManager->loadLandscapeName(landscapeID)+"</h3>");
	//Size in MiB
	double landscapeSize = landscapeManager->loadLandscapeSize(landscapeID) / (double)(1024*1024);
	ui->labelLandscapeSize->setText(QString("Size on disk: %1 MiB").arg(landscapeSize, 0, 'f', 2));
}

void AddRemoveLandscapesDialog::messageAcknowledged()
{
	ui->groupBoxMessage->setVisible(false);
	ui->groupBoxAdd->setVisible(true);
	ui->labelMessage->clear();
}

void AddRemoveLandscapesDialog::displayMessage(QString title, QString message)
{
	ui->labelMessage->setText(message);
	ui->groupBoxMessage->setTitle(title);
	ui->groupBoxMessage->setVisible(true);
}
