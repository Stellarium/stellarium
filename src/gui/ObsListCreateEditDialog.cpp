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

#include <StelTranslator.hpp>

#include "StelFileMgr.hpp"

#include "ObsListCreateEditDialog.hpp"
#include "ui_obsListCreateEditDialog.h"

using namespace std;

ObsListCreateEditDialog * ObsListCreateEditDialog::m_instance = nullptr;

ObsListCreateEditDialog::ObsListCreateEditDialog ( string listName )
{
    listName_ = listName;

    if ( listName_.size() == 0 ) {
        isCreationMode = true;
    } else {
        isCreationMode = false;
    }

    ui = new Ui_obsListCreateEditDialogForm();
    core = StelApp::getInstance().getCore();
    obsListListModel = new QStandardItemModel ( 0,ColumnCount );
    observingListJsonPath = StelFileMgr::findFile ( "data", ( StelFileMgr::Flags ) ( StelFileMgr::Directory|StelFileMgr::Writable ) ) + "/" + QString ( jsonFileName );


}

ObsListCreateEditDialog::~ObsListCreateEditDialog()
{
    delete ui;
    delete obsListListModel;
}

/**
 * Get instance of class
*/
ObsListCreateEditDialog * ObsListCreateEditDialog::Instance ( string listName )
{
    if ( m_instance == nullptr ) {
        m_instance = new ObsListCreateEditDialog ( listName );
    }

    return m_instance;
}


/*
 * Initialize the dialog widgets and connect the signals/slots.
*/
void ObsListCreateEditDialog::createDialogContent()
{
    ui->setupUi ( dialog );

    //Signals and slots
    connect ( &StelApp::getInstance(), SIGNAL ( languageChanged() ), this, SLOT ( retranslate() ) );
    connect ( ui->closeStelWindow, SIGNAL ( clicked() ), this, SLOT ( close() ) );

    connect ( ui->obsListAddObjectButton, SIGNAL ( clicked() ), this, SLOT ( obsListAddObjectButtonPressed() ) );
    connect ( ui->obsListExitButton, SIGNAL ( clicked() ), this, SLOT ( obsListExitButtonPressed() ) );
    connect ( ui->obsListSaveButton, SIGNAL ( clicked() ), this, SLOT ( obsListSaveButtonPressed() ) );
    connect ( ui->obsListRemoveObjectButton, SIGNAL ( clicked() ), this, SLOT ( obsListRemoveObjectButtonPressed() ) );
    connect ( ui->obsListImportListButton, SIGNAL ( clicked() ), this, SLOT ( obsListImportListButtonPresssed() ) );
    connect ( ui->obsListExportListButton, SIGNAL ( clicked() ), this, SLOT ( obsListExportListButtonPressed() ) );

    //Initializing the list of observing list
    obsListListModel->setColumnCount ( ColumnCount );
    setObservingListHeaderNames();

    ui->obsListCreationEditionTreeView->setModel ( obsListListModel );
    ui->obsListCreationEditionTreeView->header()->setSectionsMovable ( false );
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode ( ColumnName, QHeaderView::ResizeToContents );
    ui->obsListCreationEditionTreeView->header()->setStretchLastSection ( true );
    ui->obsListCreationEditionTreeView->hideColumn ( ColumnUUID );
    ui->obsListCreationEditionTreeView->hideColumn ( ColumnJD );
    ui->obsListCreationEditionTreeView->hideColumn ( ColumnLocation );
    //Enable the sort for columns
    ui->obsListCreationEditionTreeView->setSortingEnabled ( true );
}

/*
 * Retranslate dialog
*/
void ObsListCreateEditDialog::retranslate()
{
    if ( dialog ) {
        ui->retranslateUi ( dialog );
    }
}

/*
 * Style changed
*/
void ObsListCreateEditDialog::styleChanged()
{
    // Nothing for now
}

/*
 * Set the header for the observing list table
 * (obsListTreeVView)
*/
void ObsListCreateEditDialog::setObservingListHeaderNames()
{
    const QStringList headerStrings = {
        "UUID", // Hided column
        q_ ( "Object name" ),
        q_ ( "Type" ),
        q_ ( "Right ascencion" ),
        q_ ( "Declination" ),
        q_ ( "Magnitude" ),
        q_ ( "Constellation" ),
        q_ ( "Date" ), // Hided column
        q_ ( "Location" ) // Hided column
    };

    obsListListModel->setHorizontalHeaderLabels ( headerStrings );
}


/*
 * Add row in the obsListListModel
*/
void ObsListCreateEditDialog::addModelRow ( int number, QString uuid, QString name, QString type, QString ra, QString dec, QString magnitude, QString constellation )
{
    QStandardItem* item = Q_NULLPTR;

    item = new QStandardItem ( uuid );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnUUID, item );

    item = new QStandardItem ( name );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnName, item );

    item = new QStandardItem ( type );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnType, item );

    item = new QStandardItem ( ra );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnRa, item );

    item = new QStandardItem ( dec );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnDec, item );

    item = new QStandardItem ( magnitude );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnMagnitude, item );

    item = new QStandardItem ( constellation );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnConstellation, item );

    for ( int i = 0; i < ColumnCount; ++i ) {
        ui->obsListCreationEditionTreeView->resizeColumnToContents ( i );
    }
}


/*
 * Slot for button obsListAddObjectButton
*/
void ObsListCreateEditDialog::obsListAddObjectButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListRemoveObjectButton
*/
void ObsListCreateEditDialog::obsListRemoveObjectButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListExportListButton
*/
void ObsListCreateEditDialog::obsListExportListButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListImportListButton
*/
void ObsListCreateEditDialog::obsListImportListButtonPresssed()
{
    //TODO
}

/*
 * Slot for button obsListSaveButton
*/
void ObsListCreateEditDialog::obsListSaveButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListExitButton
*/
void ObsListCreateEditDialog::obsListExitButtonPressed()
{
    this->close();
    emit exitButtonClicked();
}

/*
 * Destructor of singleton
*/
void ObsListCreateEditDialog::kill()
{
    if ( m_instance != nullptr ) {
        delete m_instance;
        m_instance = nullptr;
    }
}







