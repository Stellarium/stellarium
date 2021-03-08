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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/
#include "AddRemoveLandscapesDialog.hpp"
#include "ui_addRemoveLandscapesDialog.h"

#include "Dialog.hpp"
#include "LandscapeMgr.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelMainView.hpp"

#include <QDebug>
#include <QFileDialog>
#include <QString>
#include <QMessageBox>

AddRemoveLandscapesDialog::AddRemoveLandscapesDialog() : StelDialog("AddRemoveLandscapes")
{
	ui = new Ui_addRemoveLandscapesDialogForm;
	landscapeManager = GETSTELMODULE(LandscapeMgr);
	lastUsedDirectoryPath = QDir::homePath();
}

AddRemoveLandscapesDialog::~AddRemoveLandscapesDialog()
{
	delete ui;
}

void AddRemoveLandscapesDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void AddRemoveLandscapesDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->pushButtonBrowseForArchive, SIGNAL(clicked()), this, SLOT(browseForArchiveClicked()));
	connect(ui->listWidgetUserLandscapes, SIGNAL(currentRowChanged(int)), this, SLOT(updateSidePane(int)));
	connect(ui->pushButtonRemove, SIGNAL(clicked()), this, SLOT(removeClicked()));
	connect(ui->pushButtonMessageOK, SIGNAL(clicked()), this, SLOT(messageAcknowledged()));

	connect(landscapeManager, SIGNAL(landscapesChanged()), this, SLOT(populateLists()));
	connect(landscapeManager, SIGNAL(errorUnableToOpen(QString)), this, SLOT(messageUnableToOpen(QString)));
	connect(landscapeManager, SIGNAL(errorNotArchive()), this, SLOT(messageNotArchive()));
	connect(landscapeManager, SIGNAL(errorNotUnique(QString)), this, SLOT(messageNotUnique(QString)));
	connect(landscapeManager, SIGNAL(errorRemoveManually(QString)), this, SLOT(messageRemoveManually(QString)));

	ui->groupBoxMessage->setVisible(false);

	populateLists();
}

void AddRemoveLandscapesDialog::setVisible(bool v)
{
	StelDialog::setVisible(v);
	//Make sure that every time when the dialog is displayed, the current
	//landscape is selected in the list of user landscapes if it is in the list.
	populateLists();
}

void AddRemoveLandscapesDialog::populateLists()
{
	ui->listWidgetUserLandscapes->clear();
	QStringList landscapes = landscapeManager->getUserLandscapeIDs();
	if (!landscapes.isEmpty())
	{
		landscapes.sort();
		ui->listWidgetUserLandscapes->addItems(landscapes);
		//If the current landscape is in the list of user landscapes, select its entry
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
		updateSidePane(-1);
	}
}

void AddRemoveLandscapesDialog::browseForArchiveClicked()
{
	QString caption = q_("Select a ZIP archive that contains a Stellarium landscape");
	// TRANSLATORS: This string is displayed in the "Files of type:" drop-down list in the standard file selection dialog.
	QString filter = q_("ZIP archives");
	filter += " (*.zip)";
	QString sourceArchivePath = QFileDialog::getOpenFileName(Q_NULLPTR, caption, lastUsedDirectoryPath, filter);
	bool useLandscape = ui->checkBoxUseLandscape->isChecked();
	if (!sourceArchivePath.isEmpty() && QFile::exists(sourceArchivePath))
	{
		//Remember the last successfully used directory
		lastUsedDirectoryPath = QFileInfo(sourceArchivePath).path();

		QString newLandscapeID = landscapeManager->installLandscapeFromArchive(sourceArchivePath, useLandscape);
		if(!newLandscapeID.isEmpty())
		{
			//Show a message
			QString successMessage  = QString(q_("Landscape \"%1\" has been installed successfully.")).arg(newLandscapeID);
			displayMessage(q_("Success"), successMessage);

			//Make the new landscape selected in the list
			//populateLists(); //No longer needed after the migration to signals/slots
			ui->listWidgetUserLandscapes->setCurrentItem((ui->listWidgetUserLandscapes->findItems(newLandscapeID, Qt::MatchExactly)).first());
		}
		else
		{
			//If no error message has been displayed by the signal/slot mechanism,
			//display a generic message.
			if (!ui->groupBoxMessage->isVisible())
			{
				//Show an error message
				QString failureMessage = q_("No landscape was installed.");
				displayMessage(q_("Error!"), failureMessage);
			}
		}
	}
}

void AddRemoveLandscapesDialog::removeClicked()
{
	int reply = QMessageBox(QMessageBox::Question,
				q_("Remove an installed landscape"),
				q_("Do you really want to remove this landscape?"),
                QMessageBox::Yes|QMessageBox::No,
                &StelMainView::getInstance()).exec();

	if (reply == QMessageBox::Yes)
	{
		QString landscapeID = ui->listWidgetUserLandscapes->currentItem()->data(0).toString();
		if(landscapeManager->removeLandscape(landscapeID))
		{
			//populateLists();//No longer needed after the migration to signals/slots
			QString successMessage  = QString(q_("Landscape \"%1\" has been removed successfully.")).arg(landscapeID);
			displayMessage(q_("Success"), successMessage);
		}
		else
		{
			//If no error message has been displayed by the signal/slot mechanism,
			//display a generic message.
			if (!ui->groupBoxMessage->isVisible())
			{
				//Show an error message
				//NB! This string is used elsewhere. Modify both to avoid two nearly identical translations.
				QString failureMessage = q_("The selected landscape could not be (completely) removed.");
				displayMessage(q_("Error!"), failureMessage);
			}
		}
	}
}

void AddRemoveLandscapesDialog::updateSidePane(int newRow)
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
	// TRANSLATORS: MiB = mebibytes (IEC 60027-2 standard for 2^20 bytes)
	ui->labelLandscapeSize->setText(QString(q_("Size on disk: %1 MiB")).arg(landscapeSize, 0, 'f', 2));
}

void AddRemoveLandscapesDialog::messageAcknowledged()
{
	ui->groupBoxMessage->setVisible(false);
	ui->groupBoxAdd->setVisible(true);
	ui->groupBoxRemove->setVisible(true);
	ui->labelMessage->clear();
	ui->groupBoxMessage->setTitle(QString());
}

void AddRemoveLandscapesDialog::displayMessage(QString title, QString message)
{
	ui->labelMessage->setText(message);
	ui->groupBoxMessage->setTitle(title);
	ui->groupBoxMessage->setVisible(true);
	ui->groupBoxAdd->setVisible(false);
	ui->groupBoxRemove->setVisible(false);
}

void AddRemoveLandscapesDialog::messageUnableToOpen(QString path)
{
	QString failureMessage = q_("No landscape was installed.");
	failureMessage.append(" ");
	// TRANSLATORS: The parameter is a file/directory path that may be quite long.
	failureMessage.append(q_("Stellarium cannot open for reading or writing %1").arg(path));
	displayMessage(q_("Error!"), failureMessage);
}

void AddRemoveLandscapesDialog::messageNotArchive()
{
	QString failureMessage = q_("No landscape was installed.") + " " + q_("The selected file is not a ZIP archive or does not contain a Stellarium landscape.");
	displayMessage(q_("Error!"), failureMessage);
}

void AddRemoveLandscapesDialog::messageNotUnique(QString nameOrID)
{
	// TRANSLATORS: The parameter is the duplicate name or identifier.
	QString nameMessage = q_("A landscape with the same name or identifier (%1) already exists.").arg(nameOrID);
	QString failureMessage = q_("No landscape was installed.") + " " + nameMessage;
	displayMessage(q_("Error!"), failureMessage);
}

void AddRemoveLandscapesDialog::messageRemoveManually(QString path)
{
	//NB! This string is used elsewhere. Modify both to avoid two nearly identical translations.
	QString failureMessage = q_("The selected landscape could not be (completely) removed.");
	failureMessage.append(" ");
	// TRANSLATORS: The parameter is a file/directory path that may be quite long. "It" refers to a landscape that can't be removed.
	failureMessage.append(q_("You can remove it manually by deleting the following directory: %1").arg(path));
	displayMessage(q_("Error!"), failureMessage);
}
