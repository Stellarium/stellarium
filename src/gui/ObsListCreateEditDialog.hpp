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

class Ui_obsListCreateEditDialogForm;

struct observingListItem {
    QString name;
    QString nameI18n;
    QString type;
    QString ra;
    QString dec;
    QString magnitude;
    QString constellation;
    QString jd;
    QString location;
    double fov;
    bool isVisibleMarker;
};
Q_DECLARE_METATYPE(observingListItem)

class ObsListCreateEditDialog : public StelDialog
{
    Q_OBJECT

public:
    static ObsListCreateEditDialog * Instance (std::string listName );
    static void kill();

    //! Notify that the application style changed
    void styleChanged();

protected:
    static ObsListCreateEditDialog * m_instance;
    Ui_obsListCreateEditDialogForm *ui;
    //! Initialize the dialog widgets and connect the signals/slots.
    virtual void createDialogContent();

private:
    //! To know if the dialog is open in creation mode or editionn mode
    // if true we are in creation mode otherwise in edition mode
    bool isCreationMode;
    enum ObsListColumns {
        ColumnUUID,	//! UUID of object
        ColumnName,	//! Name or designation of object
        ColumnNameI18n,	//! Localized name of object
        ColumnType, //! Type of the object
        ColumnRa,	//! Right ascencion of the object
        ColumnDec,	//! Declination of the object
        ColumnMagnitude,	//! Magnitude of the object
        ColumnConstellation, //! Constellation in which the object is located
        ColumnJD, //! Date in julian day
        ColumnLocation, //! Location where the object is observed
        ColumnCount	//! Total number of columns
    };
    QStandardItemModel * obsListListModel;
    class StelCore* core;
    class StelObjectMgr* objectMgr;
    std::string listName_;
    static constexpr char const * jsonFileName = "observingList.json";
    QString observingListJsonPath;
    QHash<QString, observingListItem> observingListItemCollection;

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
    void addModelRow(int number, QString uuid, QString name, QString nameI18n, QString type, QString ra, QString dec, QString magnitude, QString constellation);
    
    //! Save the object informations into json file
    void saveObservedObject();
    
    //! Load the observing liste in case of edit mode
    void loadObservingList();

    //Private constructor and destructor
    ObsListCreateEditDialog (std::string listName);
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

signals:
    //To notified that the exit button is clicked
    void exitButtonClicked();

};

#endif // OBSLISTCREATEEDITDIALOG_H
