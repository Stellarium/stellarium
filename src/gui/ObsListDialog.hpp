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
#include <QSortFilterProxyModel>

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
//! You can create new lists, edit them, delete them, export them and exchange with other users etc.
//!
//! When creating an entry for a selected object, you can optionally store current time, location, landscapeID and field of view.
//! This helps to retrieve the possible same view if location and/or time matter. Obviously, a list of favourite DSOs would not need those data (but maybe FoV is still meaningful).
//! On retrieval, the same optional elements can again be selected, so you can retrieve an object without stored time or location, if that is meaningful.
//! If location or landscape IDs cannot be found, they are not changed.



class Ui_obsListDialogForm;

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

	//! Notify that the application style changed
	//void styleChanged() Q_DECL_OVERRIDE;

	void setVisible(bool v) Q_DECL_OVERRIDE;

protected:
	Ui_obsListDialogForm *ui;

	//! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() override;

private:
	QStandardItemModel *obsListListModel;
	class StelCore *core;
	class StelObjectMgr *objectMgr;
	class LandscapeMgr *landscapeMgr;
	class LabelMgr *labelMgr;

	// FIXME: Explain why we need a std::string here
	std::string selectedObservingListUuid;
	QString observingListJsonPath;
	QString bookmarksJsonPath;
	QHash<QString, observingListItem> observingListItemCollection;
	QList<int> highlightLabelIDs;
	QString defaultListOlud_;
	QList<QString> listName_;
	QStringList listNamesModel;
	//double currentJd;
	ObservingListUtil util;

	//properties:
	bool flagUseJD;
	bool flagUseLandscape;
	bool flagUseLocation;
	bool flagUseFov;

	//! Set header names for observing list table
	void setObservingListHeaderNames();

	void invokeObsListCreateEditDialog(std::string listOlud);

	ObsListCreateEditDialog *createEditDialog_instance;

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

	//! Load the bookmarks of bookmarks.json file into observing lists file
	void loadBookmarksInObservingList();

	void saveBookmarksInObsListJsonFile(const QHash<QString, observingListItem> &bookmarksCollection);

	//! Load list from JSON file
	QVariantList loadListFromJson(const QVariantMap &map, const QString& listOlud);

	//! Populate list names into combo box
	void populateListNameInComboBox(QVariantMap map);

	//! Populate data into combo box
	void populateDataInComboBox(QVariantMap map, const QString &defaultListOlud);

	//! Sort the obsListTreeView by the column name given in parameter
	void sortObsListTreeViewByColumnName(const QString &columnName);

	//! Clear highlights
	void clearHighlight();

	//! get the defaultListOlud from Json file
	QString extractDefaultListOludFromJsonFile();

	static bool checkIfBookmarksListExists(const QVariantMap &allListsMap);
signals:
	void flagUseJDChanged(bool b);
	void flagUseLandscapeChanged(bool b);
	void flagUseLocationChanged(bool b);
	void flagUseFovChanged(bool b);

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

	void obsListHighLightAllButtonPressed();

	void obsListClearHighLightButtonPressed();

	void obsListNewListButtonPressed();

	void obsListEditButtonPressed();

	void obsListCreateEditDialogClosed();

	// void obsListExitButtonPressed();

	void obsListDeleteButtonPressed();


	//! Method called when a list name is selected in the combobox
	//! @param selectedIndex the index of the list name in the combo box
	void loadSelectedObservingList(int selectedIndex);

	//! Select and go to object
	//! @param index the QModelIndex of the list
	void selectAndGoToObject(QModelIndex index);
};

#endif // OBSLISTDIALOG_H
