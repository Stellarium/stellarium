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

class Ui_obsListDialogForm;

class ObsListDialog : public StelDialog
{
    Q_OBJECT

public:
    explicit ObsListDialog ( QObject* parent );
    ~ObsListDialog() override;

    //! Notify that the application style changed
    void styleChanged() override;
    
    void setVisible ( bool v ) override;

protected:
    Ui_obsListDialogForm *ui;
    //! Initialize the dialog widgets and connect the signals/slots.
    void createDialogContent() override;

private:
    QStandardItemModel * obsListListModel;
    class StelCore* core;
    class StelObjectMgr* objectMgr;
    class LandscapeMgr* landscapeMgr;
    class LabelMgr* labelMgr;
    std::string selectedObservingListUuid;
    QString observingListJsonPath;
    QString bookmarksJsonPath;
    QHash<QString, observingListItem> observingListItemCollection;
    QList<int> highlightLabelIDs;
    QString defaultListOlud_;
    QList<QString> listName_;
    QStringList listNamesModel;

    //! Set header names for observing list table
    void setObservingListHeaderNames();

    void invokeObsListCreateEditDialog ( std::string listOlud );

    ObsListCreateEditDialog * createEditDialog_instance;

    //! Add row in the obsListListModel
    //! @param number row number
    //! @param olud id of the record
    //! @param name name or the designation of the object
    //! @param type type of the object
    //! @param ra right assencion of the object
    //! @param dec declination of the object
    //! @param magnitude magnitude of the object
    //! @param constellation constellation in which the object is located
    void addModelRow (int number, const QString& olud, const QString& name, const QString& nameI18n, const QString& type, const QString& ra, const QString& dec, const QString& magnitude, const QString& constellation );

    //! Load the selected observing list from Json file into dialog.
    //! @param listOlud the olud (id) of the list
    void loadSelectedObservingListFromJsonFile (const QString& listOlud );

    //! Load the lists names from Json file to populate the combo box and get the default list olud
    void loadListsNameFromJsonFile();
    
    //! Load the default list
    void loadDefaultList();
    
    //! Load the bookmarks of bookmarks.json file into observing lists file
    void loadBookmarksInObservingList();
    void saveBookmarksInObsListJsonFile(const QHash<QString, bookmark>& bookmarksCollection);
    auto checkIfBookmarksListExists(QVariantMap allListsMap) -> bool;
    
    //! Load list from JSON file
    auto loadListFromJson(const QVariantMap& map, QString listOlud) -> QVariantList;
    
    //! Populate list names into combo box
    void populateListNameInComboBox(QVariantMap map);
    
    //! Populate data into combo box
    void populateDataInComboBox(QVariantMap map, const QString& defaultListOlud);
    
    //! Sort the obsListTreeView by the column name given in parameter
    void sortObsListTreeViewByColumnName(QString columnName);
    
    //! Clear highlights
    void clearHighlight();

    //! get the defaultListOlud from Json file
    QString extractDefaultListOludFromJsonFile();


public slots:
    void retranslate();

private slots:
    void obsListHighLightAllButtonPressed();
    void obsListClearHighLightButtonPressed();
    void obsListNewListButtonPressed();
    void obsListEditButtonPressed();
    void obsListCreateEditDialogClosed();
    void obsListExitButtonPressed();
    void obsListDeleteButtonPressed();
    
    

    //! Method called when a list name is selected in the combobox
    //! @param selectedIndex the index of the list name in the combo box
    void loadSelectedObservingList ( int selectedIndex );

    //! Select and go to object
    //! @param index the QModelIndex of the list
    void selectAndGoToObject ( QModelIndex index );
};

#endif // OBSLISTDIALOG_H
