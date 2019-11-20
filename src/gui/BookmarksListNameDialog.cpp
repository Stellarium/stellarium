/*
 * Stellarium
 * 
 * Copyright (C) 2019 Jocelyn GIROD
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include <QDebug>
#include <QRegExpValidator>

#include "BookmarksListNameDialog.hpp"
#include "ui_bookmarksListNameDialog.h"
#include "StelApp.hpp"

//! Constructor
BookmarksListNameDialog::BookmarksListNameDialog()
{
    ui = new Ui_bookmarksListNameDialogForm;
    bookmarksListName = "";
}

//! Destructor
BookmarksListNameDialog::~BookmarksListNameDialog()
{
    delete ui;
}

//! Initialize the dialog widgets and connect the signals/slots.
void BookmarksListNameDialog::createDialogContent()
{
    ui->setupUi(dialog);
    
    //Signals and slots
    connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
    connect(ui->buttonOk, SIGNAL(clicked()), this, SLOT(buttonOkPressed()));
    connect(ui->buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
    
    // only alphanumerics caracters, _ and -
    ui->lineEditListName->setValidator(new QRegExpValidator( QRegExp("[A-Za-z0-9_-]+"), this ));
    // only 40 characters
    ui->lineEditListName->setMaxLength(40);
    
}


//! Retranslate the dialog
void BookmarksListNameDialog::retranslate()
{
    if (dialog)
	{
		ui->retranslateUi(dialog);
	}

}

// Called when the button ok is pressed
void BookmarksListNameDialog::buttonOkPressed()
{
    bookmarksListName = ui->lineEditListName->text();
    
    this->close();
    emit listNameRetrieved();
}


//! Get the bookmarks list file name
QString BookmarksListNameDialog::getBookmarksListName() const
{
    return bookmarksListName;
}

//! Get ui
Ui_bookmarksListNameDialogForm * BookmarksListNameDialog::getUi() const
{
    return ui;
}

//! Display the dialog and iniit the text of the lineEditListName
void BookmarksListNameDialog::displayBookmarksListNameDialog()
{
    ui->lineEditListName->setText("");
    setVisible(true);
}



