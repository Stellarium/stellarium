/*
 * Stellarium
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

#ifndef OBSLISTDIALOG_H
#define OBSLISTDIALOG_H

#include <QObject>
#include <QStandardItemModel>

#include "StelDialog.hpp"
#include "StelCore.hpp"
#include "ObsListCreateEditDialog.hpp"
#include "ObservingListCommon.hpp"

//! @class ObsListDialog
//! Since V0.21.2, this class manages the ObservingLists, successor of the Bookmarks feature available since 0.15.
//! Updated in Version 1.0: ObservingLists Version 2.0
//!
//! This tool manages Observing Lists based on a unique list identifier, the OLUD = Observing List Unique Designation
//! Old bookmarks.json files are auto-imported at first run. The old file is then not touched any further and could still be used in older versions of Stellarium.
//! This first bookmarks list is then the first stored ObservingList.
//! You can create new lists, edit them, delete them, import/export them and exchange with other users etc.
//! You can also highlight all bookmarked objects of the active list
//!
//! When creating an entry for a selected object, you can optionally store current time, location, landscapeID and field of view.
//! This helps to retrieve the possible same view if location and/or time matter, e.g. for a list of Solar eclipses with their locations and landscapes.
//! Obviously, a list of favourite DSOs would not need those data (but maybe FoV is still meaningful).
//! On retrieval, the same optional elements can again be selected, so you can retrieve an object without stored time or location, if that is meaningful.
//! If location or landscape IDs cannot be found, they are not changed.



class Ui_obsListUnifiedDialogForm;

class ObsListDialog : public StelDialog
{
	Q_OBJECT
	// Configure optionally stored/retrieved items
	Q_PROPERTY(bool flagUseJD        READ getFlagUseJD        WRITE setFlagUseJD        NOTIFY flagUseJDChanged)
	Q_PROPERTY(bool flagUseLandscape READ getFlagUseLandscape WRITE setFlagUseLandscape NOTIFY flagUseLandscapeChanged)
	Q_PROPERTY(bool flagUseLocation  READ getFlagUseLocation  WRITE setFlagUseLocation  NOTIFY flagUseLocationChanged)
	Q_PROPERTY(bool flagUseFov       READ getFlagUseFov       WRITE setFlagUseFov       NOTIFY flagUseFovChanged)

public:
	explicit ObsListDialog(QObject *parent);

	~ObsListDialog() Q_DECL_OVERRIDE;

	// Notify that the application style changed
	//void styleChanged() Q_DECL_OVERRIDE;

	void setVisible(bool v) Q_DECL_OVERRIDE;

protected:
	Ui_obsListUnifiedDialogForm *ui;

	//! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() Q_DECL_OVERRIDE;

private:
	QStandardItemModel *obsListListModel;
	class StelCore *core;
	class StelObjectMgr *objectMgr;
	class LandscapeMgr *landscapeMgr;
	class LabelMgr *labelMgr;

	QString observingListJsonPath; // set once in constructor. CURRENTLY reset in loading methods which is nonsense!
	QString bookmarksJsonPath;     // set once in constructor. INSTEAD: use file name as loading argument!
	QString selectedObservingListUuid;
	QHash<QString, observingListItem> observingListItemCollection;
	QList<int> highlightLabelIDs; //! int label IDs for addressing labels by the HighlightMgr
	QString defaultListOlud_;
	//List names
	QList<QString> listNames_;
	//QStringList listNamesModel; // only used in 2 methods. Allowed replacing by local vars!

	//properties:
	bool flagUseJD;
	bool flagUseLandscape;
	bool flagUseLocation;
	bool flagUseFov;

	bool isEditMode;          //! NEW: true if in Edit/Create New mode.
	// FROM OLD EDIT EXTRA DIALOG
	bool isCreationMode;    //! if true we are in creation mode otherwise in edit mode
	bool isSaveAs;
	QString listOlud_;
	//Current list name
	QString currentListName;
	//! Sorting of the list ex: right ascension
	QString sorting;


	//! Set header names for observing list table
	void setObservingListHeaderNames();

	// OLD Replace this by "switch to EDIT mode"
	//void invokeObsListCreateEditDialog(QString listOlud);

	//ObsListCreateEditDialog *createEditDialog_instance;

	//! Append row in the obsListListModel (the table to display/select list entries)
	//! @param olud id of the record
	//! @param name name or the designation of the object
	//! @param type type of the object
	//! @param ra right assencion of the object
	//! @param dec declination of the object
	//! @param magnitude magnitude of the object
	//! @param constellation constellation in which the object is located
	//! @param date human-redable event date (optional)
	//! @param location name of location (optional)
	//! @param landscapeID of observation (optional)
	void addModelRow(const QString &olud, const QString &name, const QString &nameI18n, const QString &type,
			 const QString &ra, const QString &dec, const QString &magnitude, const QString &constellation,
			 const QString &date, const QString &location, const QString &landscapeID);

	//! Load the selected observing list from Json file into dialog.
	//! @param listOlud the olud (id) of the list
	void loadSelectedObservingListFromJsonFile(const QString &listOlud);

	//! Load the lists names from Json file to populate the combo box and get the default list olud
	void loadListsNameFromJsonFile();

	//! Load the default list
	void loadDefaultList();

	//! Load the bookmarks of bookmarks.json file into a temporary structure
	QHash<QString, observingListItem> loadBookmarksInObservingList();

	void saveBookmarksInObsListJsonFile(QVariantMap &allListsMap, const QHash<QString, observingListItem> &bookmarksCollection);

	//! Load list from JSON file
	QVariantList loadListFromJson(const QVariantMap &map, const QString& listOlud);

	// Populate list names into combo box
	//! Populate combo box with list name/olud
	void populateListNameInComboBox(QVariantMap map, const QString &defaultListOlud);

	//// Populate data into combo box
	//void populateDataInComboBox(QVariantMap map, const QString &defaultListOlud);

	//! Sort the obsListTreeView by the column name given in parameter
	void sortObsListTreeViewByColumnName(const QString &columnName);

	//! get the defaultListOlud from Json file
	QString extractDefaultListOludFromJsonFile();

	static bool checkIfBookmarksListExists(const QVariantMap &allListsMap);

	//! NEW: Switch between regular and edit modes.
	void switchEditMode(bool enableEditMode);

	// EDITING MODE METHODS
	//! Save the object information into json file
	void saveObservedObjectsInJsonFile();

	//! Load the observing list in case of edit mode
	void loadObservingList();

	//! Load bookmark in observing list for import.
	void loadBookmarksInObservingList_old();

	//! Initialize the error message (obsListErrorMessage).
	void initErrorMessage();

	//! Display the error message.
	void displayErrorMessage(const QString &message);





signals:
	void flagUseJDChanged(bool b);
	void flagUseLandscapeChanged(bool b);
	void flagUseLocationChanged(bool b);
	void flagUseFovChanged(bool b);

	//EDIT METHOD: To notify that the exit button was clicked
	void exitButtonClicked();


public slots:

	void retranslate() Q_DECL_OVERRIDE;
	bool getFlagUseJD() {return flagUseJD;}
	bool getFlagUseLandscape() {return flagUseLandscape;}
	bool getFlagUseLocation() {return flagUseLocation;}
	bool getFlagUseFov() {return flagUseFov;}
	void setFlagUseJD(bool b);
	void setFlagUseLandscape(bool b);
	void setFlagUseLocation(bool b);
	void setFlagUseFov(bool b);

private slots:

	//! Label all bookmarked objects with a label
	void highlightAll();
	//! Clear highlights (remove screen labels)
	void clearHighlights();

	void obsListNewListButtonPressed();

	//! Initiate edit mode: Enable/disable GUI elements and initiate editing
	void obsListEditButtonPressed();

	//! This used to be called on closing the extra dialog.
	void finishEditMode();

	void obsListDeleteButtonPressed();


	//! Method called when a list name is selected in the combobox
	//! @param selectedIndex the index of the list name in the combo box
	void loadSelectedObservingList(int selectedIndex);

	//! Select and go to object
	//! @param index the QModelIndex of the list
	void selectAndGoToObject(QModelIndex index);


	// MOVED EDITING METHODS
	void obsListAddObjectButtonPressed();

	void obsListRemoveObjectButtonPressed();

	void obsListExportListButtonPressed();

	void obsListImportListButtonPresssed();

	void obsListSaveButtonPressed();

	void obsListCancelButtonPressed();

	void headerClicked(int index);

	void nameOfListTextChange(const QString &newText);

};

#endif // OBSLISTDIALOG_H
