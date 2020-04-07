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
    std::string selectedObservingListUuid;

    //! Set header names for observing list table
    void setObservingListHeaderNames();
    
    void invokeObsListCreateEditDialog(std::string listUuid);

    ObsListCreateEditDialog * createEditDialog_instance;


public slots:
    void retranslate();

private slots:
    void obsListHighLightAllButtonPressed();
    void obsListClearHighLightButtonPressed();
    void obsListNewListButtonPressed();
    void obsListEditButtonPressed();
    void obsListCreateEditDialogClosed();

};

#endif // OBSLISTDIALOG_H
