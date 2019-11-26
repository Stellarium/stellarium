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

#ifndef BOOKMARKSLISTNAMEDIALOG_H
#define BOOKMARKSLISTNAMEDIALOG_H

#include <QObject>

#include "StelDialog.hpp"

class Ui_bookmarksListNameDialogForm;

/**
 * Dialog used to enter the bookmarks list name
 * This dialog is shown when we click on the button "createBookmarksListButton"
 * in BookmarksDialog
 */
class BookmarksListNameDialog :  public StelDialog
{
    Q_OBJECT
    
public:
    BookmarksListNameDialog();
    BookmarksListNameDialog(const BookmarksListNameDialog& other);

    /**
     * Destructor
     */
    ~BookmarksListNameDialog();
    
    QString getBookmarksListName() const;
    Ui_bookmarksListNameDialogForm * getUi() const;

protected:
    //! Initialize the dialog widgets and connect the signals/slots.
    virtual void createDialogContent();
    
    Ui_bookmarksListNameDialogForm *ui;
    
private:
    QString bookmarksListName;
    

signals:
    //To notified that the list name is retrieved
    void listNameRetrieved();

public slots:
        void retranslate();
        void displayBookmarksListNameDialog();

private slots:
    void buttonOkPressed();

};

#endif // BOOKMARKSLISTNAMEDIALOG_H
