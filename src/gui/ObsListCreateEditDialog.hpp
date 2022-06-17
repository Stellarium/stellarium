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
    static auto Instance(std::string listUuid) -> ObsListCreateEditDialog *;

    static void kill();

    //! Notify that the application style changed
    void styleChanged() override;

    //! called when click on button close in top right corner
    void close() override;

    void setListName(QList<QString> listName);

protected:
    //! Initialize the dialog widgets and connect the signals/slots.
    void createDialogContent() override;

private:
    static ObsListCreateEditDialog *m_instance;
    Ui_obsListCreateEditDialogForm *ui;
    //! To know if the dialog is open in creation mode or editionn mode
    // if true we are in creation mode otherwise in edition mode
    bool isCreationMode;
    bool isSaveAs;
    QStandardItemModel *obsListListModel;

    class StelCore *core;

    class StelObjectMgr *objectMgr;

    class LandscapeMgr *landscapeMgr;

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

    //! Add row in the obsListListModel
    //! @param number row number
    //! @param uuid id of the record
    //! @param name name or the designation of the object
    //! @param type type of the object
    //! @param ra right ascension of the object
    //! @param dec declination of the object
    //! @param magnitude magnitude of the object
    //! @param constellation constellation in which the object is located
    void addModelRow(int number, const QString &uuid, const QString &name, const QString &nameI18n, const QString &type,
                     const QString &ra, const QString &dec, const QString &magnitude, const QString &constellation);

    //! Save the object informations into json file
    void saveObservedObjectsInJsonFile();

    //! Load the observing liste in case of edit mode
    void loadObservingList();

    //Private constructor and destructor
    explicit ObsListCreateEditDialog(std::string listUuid);

    ~ObsListCreateEditDialog() override;

public slots:

    void retranslate() override;

private slots:

    void obsListAddObjectButtonPressed();

    void obsListRemoveObjectButtonPressed();

    void obsListExportListButtonPressed();

    void obsListImportListButtonPresssed();

    void obsListSaveButtonPressed();

    void obsListExitButtonPressed();

    void headerClicked(int index);

    void nameOfListTextChange();

signals:

    //To notified that the exit button is clicked
    void exitButtonClicked();
};

#endif // OBSLISTCREATEEDITDIALOG_H
