/*
 * * Stellarium
 * Copyright (C) 2020 Jocelyn GIROD
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef OBSLISTCREATEEDITDIALOG_H
#define OBSLISTCREATEEDITDIALOG_H

#include <QObject>
#include <QStandardItemModel>

#include <string>

#include "StelDialog.hpp"
#include "ObservingListCommon.hpp"

class Ui_obsListCreateEditDialogForm;

class ObsListCreateEditDialog : public StelDialog {
	Q_OBJECT

public:
	static ObsListCreateEditDialog * Instance(std::string listUuid);

	static void kill();

	//void styleChanged() Q_DECL_OVERRIDE;

	//! called when clicking on close button in top right corner
	void close() Q_DECL_OVERRIDE;

	void setListName(QList<QString> listName);

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() override;

private:
	static ObsListCreateEditDialog *m_instance;
	Ui_obsListCreateEditDialogForm *ui;
	//! To know if the dialog is open in creation mode or edit mode
	// if true we are in creation mode otherwise in edit mode
	bool isCreationMode;
	bool isSaveAs;
	QStandardItemModel *obsListListModel;

	class StelCore *core;

	class StelObjectMgr *objectMgr;

	class LandscapeMgr *landscapeMgr;

	// FIXME: Explain why this has to be a std::string and not a QString
	std::string listOlud_;
	QString observingListJsonPath;
	ObservingListUtil util;

	// Data for observed objects
	QHash<QString, observingListItem> observingListItemCollection;

	//List names
	QList<QString> listNames_;

	//Current list name
	QString currentListName;

	//! Sorting of the list ex: right ascension
	QString sorting;

	//! Set header names for observing list table
	void setObservingListHeaderNames();

	//! Add row in the end of obsListListModel
	// // @param number row number
	//! @param olud id of the record
	//! @param name name or the designation of the object
	//! @param type type of the object
	//! @param ra right ascension of the object
	//! @param dec declination of the object
	//! @param magnitude magnitude of the object
	//! @param constellation constellation in which the object is located
	void addModelRow(const QString &olud, const QString &name, const QString &nameI18n, const QString &type,
			 const QString &ra, const QString &dec, const QString &magnitude, const QString &constellation,
			 const QString &date, const QString &location, const QString &landscapeID);

	//! Save the object informations into json file
	void saveObservedObjectsInJsonFile();

	//! Load the observing list in case of edit mode
	void loadObservingList();

	//! Load bookmark in observing list for import.
	void loadBookmarksInObservingList();

	//! Initialize the error message (obsListErrorMessage).
	void initErrorMessage();

	//! Display the error message.
	void displayErrorMessage(const QString &message);

	//Private constructor and destructor
	explicit ObsListCreateEditDialog(std::string listUuid);

	~ObsListCreateEditDialog() Q_DECL_OVERRIDE;

public slots:

	void retranslate() Q_DECL_OVERRIDE;

private slots:

	void obsListAddObjectButtonPressed();

	void obsListRemoveObjectButtonPressed();

	void obsListExportListButtonPressed();

	void obsListImportListButtonPresssed();

	void obsListSaveButtonPressed();

	void obsListExitButtonPressed();

	void headerClicked(int index);

	void nameOfListTextChange(const QString &newText);

signals:

	//To notify that the exit button was clicked
	void exitButtonClicked();
};

#endif // OBSLISTCREATEEDITDIALOG_H
