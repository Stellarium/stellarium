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

class ObsListCreateEditDialog : public StelDialog
{
    Q_OBJECT

public:
    static ObsListCreateEditDialog * Instance ( std::string listUuid );
    static void kill();

    //! Notify that the application style changed
    void styleChanged();
    
    //! called when click on button close in top right corner
    void close();

protected:
    static ObsListCreateEditDialog * m_instance;
    Ui_obsListCreateEditDialogForm *ui;
    //! Initialize the dialog widgets and connect the signals/slots.
    virtual void createDialogContent();

private:
    //! To know if the dialog is open in creation mode or editionn mode
    // if true we are in creation mode otherwise in edition mode
    bool isCreationMode;
    QStandardItemModel * obsListListModel;
    class StelCore* core;
    class StelObjectMgr* objectMgr;
    std::string listUuid_;
    QString observingListJsonPath;
    QHash<QString, observingListItem> observingListItemCollection;

    //! Sorting of the list ex: right ascencion
    QString sorting;

    //! Set header names for observing list table
    void setObservingListHeaderNames();

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

    //! Save the object informations into json file
    void saveObservedObject();

    //! Load the observing liste in case of edit mode
    void loadObservingList();

    //Private constructor and destructor
    ObsListCreateEditDialog ( std::string listUuid );
    virtual ~ObsListCreateEditDialog();


public slots:
    void retranslate();
private slots:
    void obsListAddObjectButtonPressed();
    void obsListRemoveObjectButtonPressed();
    void obsListExportListButtonPressed();
    void obsListImportListButtonPresssed();
    void obsListSaveButtonPressed();
    void obsListExitButtonPressed();
    void headerClicked ( int index );

signals:
    //To notified that the exit button is clicked
    void exitButtonClicked();
};

#endif // OBSLISTCREATEEDITDIALOG_H
