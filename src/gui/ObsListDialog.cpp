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

#include <StelTranslator.hpp>

#include "StelApp.hpp"
#include "ObsListDialog.hpp"
#include "ui_obsListDialog.h"

using namespace std;


ObsListDialog::ObsListDialog ( QObject* parent ) : StelDialog ( "Observing list", parent )
{
    ui = new Ui_obsListDialogForm();
    core = StelApp::getInstance().getCore();
    obsListListModel = new QStandardItemModel ( 0,ColumnCount );
    createEditDialog_instance = Q_NULLPTR;
}


ObsListDialog::~ObsListDialog()
{
    delete ui;
    delete obsListListModel;
}

/*
 * Initialize the dialog widgets and connect the signals/slots.
*/
void ObsListDialog::createDialogContent()
{
    ui->setupUi ( dialog );

    //Signals and slots
    connect ( &StelApp::getInstance(), SIGNAL ( languageChanged() ), this, SLOT ( retranslate() ) );
    connect ( ui->closeStelWindow, SIGNAL ( clicked() ), this, SLOT ( close() ) );

    connect ( ui->obsListNewListButton, SIGNAL ( clicked() ), this, SLOT ( obsListNewListButtonPressed() ) );
    connect ( ui->obsListEditListButton, SIGNAL ( clicked() ), this, SLOT ( obsListEditButtonPressed() ) );
    connect ( ui->obsListClearHighlightButton, SIGNAL ( clicked() ), this, SLOT ( obsListClearHighLightButtonPressed() ) );
    connect ( ui->obsListHighlightAllButton, SIGNAL ( clicked() ), this, SLOT ( obsListHighLightAllButtonPressed() ) );

    //Initializing the list of observing list
    obsListListModel->setColumnCount ( ColumnCount );
    setObservingListHeaderNames();

    ui->obsListTreeView->setModel ( obsListListModel );
    ui->obsListTreeView->header()->setSectionsMovable ( false );
    ui->obsListTreeView->header()->setSectionResizeMode ( ColumnName, QHeaderView::ResizeToContents );
    ui->obsListTreeView->header()->setStretchLastSection ( true );
    ui->obsListTreeView->hideColumn ( ColumnUUID );
    //Enable the sort for columns
    ui->obsListTreeView->setSortingEnabled ( true );
}

/*
 * Retranslate dialog
*/
void ObsListDialog::retranslate()
{
    if ( dialog ) {
        ui->retranslateUi ( dialog );
        setObservingListHeaderNames();
    }
}


/*
 * Style changed
*/
void ObsListDialog::styleChanged()
{
    // Nothing for now
}


/*
 * Set the header for the observing list table
 * (obsListTreeVView)
*/
void ObsListDialog::setObservingListHeaderNames()
{
    const QStringList headerStrings = {
        "UUID", // Hided the column
        q_ ( "Object name" ),
        q_ ( "Type" ),
        q_ ( "Right ascencion" ),
        q_ ( "Declination" ),
        q_ ( "Magnitude" ),
        q_ ( "Constellation" )
    };

    obsListListModel->setHorizontalHeaderLabels ( headerStrings );;
}


/*
 * Slot for button obsListHighLightAllButton
*/
void ObsListDialog::obsListHighLightAllButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListClearHighLightButton
*/
void ObsListDialog::obsListClearHighLightButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListNewListButton
*/
void ObsListDialog::obsListNewListButtonPressed()
{
    string listName = "";
    createEditDialog_instance = ObsListCreateEditDialog::Instance (listName );
    connect(createEditDialog_instance, SIGNAL(exitButtonClicked()), this, SLOT(obsListCreateEditDialogClosed()));
    createEditDialog_instance->setVisible(true);
}

/*
 * Slot for button obsListEditButton
*/
void ObsListDialog::obsListEditButtonPressed()
{
    //TODO
}


/*
 * Slot to manage the close of obsListCreateEditDialog
*/
void ObsListDialog::obsListCreateEditDialogClosed()
{
    //ObsListCreateEditDialog::kill();
    createEditDialog_instance = Q_NULLPTR;
}





