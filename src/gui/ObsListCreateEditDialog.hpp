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
    enum ObsListColumns {
        ColumnUUID,	//! UUID of object
        ColumnName,	//! Name or designation of object
        ColumnType, //! Type of the object
        ColumnRa,	//! Right ascencion of object
        ColumnDec,	//! Declination of object
        ColumnMagnitude,	//! Magnitude of object
        ColumnConstellation, //! Constellation of object
        ColumnCount	//! Total number of columns
    };
    QStandardItemModel * obsListListModel;
    class StelCore* core;
    std::string listName_;

    //! Set header names for observing list table
    void setObservingListHeaderNames();

    //Private constructor and destructor
    ObsListCreateEditDialog (std::string listName );
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
