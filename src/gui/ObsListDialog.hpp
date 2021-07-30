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

class Ui_obsListDialogForm;

class ObsListDialog : public StelDialog
{
    Q_OBJECT

public:
    ObsListDialog ( QObject* parent );
    virtual ~ObsListDialog();

    //! Notify that the application style changed
    void styleChanged();

protected:
    Ui_obsListDialogForm *ui;
    //! Initialize the dialog widgets and connect the signals/slots.
    virtual void createDialogContent();

private:
    QStandardItemModel * obsListListModel;
    class StelCore* core;
    class StelObjectMgr* objectMgr;
    class LabelMgr* labelMgr;
    std::string selectedObservingListUuid;
    QString observingListJsonPath;
    QHash<QString, observingListItem> observingListItemCollection;
    QList<int> highlightLabelIDs;
    QString defaultListUuid_;

    //! Set header names for observing list table
    void setObservingListHeaderNames();

    void invokeObsListCreateEditDialog ( std::string listUuid );

    ObsListCreateEditDialog * createEditDialog_instance;

    //! Add row in the obsListListModel
    //! @param number row number
    //! @param uuid id of the record
    //! @param name name or the designation of the object
    //! @param type type of the object
    //! @param ra right ascencion of the object
    //! @param dec declination of the object
    //! @param magnitude magnitude of the object
    //! @param constellation constellation in which the object is located
    void addModelRow ( int number, QString uuid, QString name, QString nameI18n, QString type, QString ra, QString dec, QString magnitude, QString constellation );

    //! Load the selected observing list
    //! @param listUuid the uuid of the list
    void loadObservingList ( QString listUuid );

    //! Load the lists names for populate the combo box and get the default list uuid
    void loadListsName();
    
    //! Load the default list
    void loadDefaultList();
    
    //! Load list from JSON file
    QVariantList loadListFromJson(QFile &jsonFile, QString listUuid);


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
