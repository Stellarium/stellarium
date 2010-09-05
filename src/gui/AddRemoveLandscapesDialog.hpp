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
	void languageChanged();

public slots:
	void populateLists();
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent();
	Ui_addRemoveLandscapesDialogForm* ui;

private slots:
	void buttonAddClicked();
	void buttonRemoveClicked();
	void landscapeListCurrentRowChanged(int newRow);

	//! Hides the message group box and returns to the "Add" group box.
	//! Usually called by clicking the "OK" button in the message box.
	void messageAcknowledged();

private:
	LandscapeMgr* landscapeManager;

	//! Path to the directory last used by QFileDialog in buttonAddClicked().
	//! Initialized with QDir::homePath() in Landscape(). (DOESN'T WORK!)
	QString lastUsedDirectoryPath;

	//! List of the IDs of the landscapes packaged by default with Stellarium.
	//! (So that they can't be removed.)
	//! It is populated in AddRemoveLandscapesDialog() and has to be updated
	//! manually on changes.
	//! @todo Find a way to populate it with CMake.
	QStringList defaultLandscapeIDs;

	//! Displays a message in place of the "Add" box.
	//! Pressing the "OK" button in the message box calls messageAcknowledged().
	//! @param title the title of the QGroupBox that contains the message
	//! @param message the text of the message itself
	void displayMessage(QString title, QString message);
};

#endif // _ADDREMOVELANDSCAPESDIALOG_
