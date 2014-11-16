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
 
#ifndef _ADDREMOVELANDSCAPESDIALOG_HPP_
#define _ADDREMOVELANDSCAPESDIALOG_HPP_

#include <QObject>
#include <QStringList>

#include "StelDialog.hpp"

class Ui_addRemoveLandscapesDialogForm;
class LandscapeMgr;

//! @class AddRemoveLandscapesDialog
class AddRemoveLandscapesDialog : public StelDialog
{
	Q_OBJECT
public:
	AddRemoveLandscapesDialog();
	virtual ~AddRemoveLandscapesDialog();

public slots:
	void retranslate();
	//! This function overrides the non-virtual StelDialog::setVisible()
	//! to allow the current landscape to be selected in the list of user
	//! landscapes (if it is in the list) every time the dialog is displayed.
	void setVisible(bool);
	void populateLists();
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent();
	Ui_addRemoveLandscapesDialogForm* ui;

private slots:
	void browseForArchiveClicked();
	void removeClicked();
	void updateSidePane(int newRow);

	//! Hides the message group box and returns to the "Add" group box.
	//! Usually called by clicking the "OK" button in the message box.
	void messageAcknowledged();

	void messageUnableToOpen(QString path);
	void messageNotArchive();
	void messageNotUnique(QString nameOrID);
	void messageRemoveManually(QString path);

private:
	LandscapeMgr* landscapeManager;

	//! Path to the directory last used by QFileDialog in buttonAddClicked().
	//! Initialized with QDir::homePath() in Landscape(). (DOESN'T WORK!)
	QString lastUsedDirectoryPath;

	//! Displays a message in place of the "Add" box.
	//! Pressing the "OK" button in the message box calls messageAcknowledged().
	//! @param title the title of the QGroupBox that contains the message
	//! @param message the text of the message itself
	void displayMessage(QString title, QString message);
};

#endif // _ADDREMOVELANDSCAPESDIALOG_
