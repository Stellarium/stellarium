/*
 * Stellarium
 * 
 * Copyright (C) 2016 Alexander Wolf
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

#ifndef _BOOKMARKSDIALOG_HPP_
#define _BOOKMARKSDIALOG_HPP_

#include <QObject>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMap>
#include <QVector>

#include "StelDialog.hpp"
#include "StelCore.hpp"

class Ui_bookmarksDialogForm;

class BookmarksDialog : public StelDialog
{
	Q_OBJECT

public:
	//! Defines the number and the order of the columns in the table that lists planetary positions
	//! @enum PlanetaryPositionsColumns
	enum BookmarksColumns {
		ColumnName,		//! name of bookmark
		//ColumnDescription,	//! description of bookmark
		ColumnRA,		//! right ascension (J2000.0)
		ColumnDec,		//! declination (J2000.0)
		ColumnDate,		//! date and time (optional)
		ColumnLocation,		//! location (optional)
		ColumnCount		//! total number of columns
	};

	BookmarksDialog(QObject* parent);
	virtual ~BookmarksDialog();

public slots:
        void retranslate();

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
        virtual void createDialogContent();
	Ui_bookmarksDialogForm *ui;

private slots:
	//! Search bookmarks and fill the list.
	void currentBookmarks();
	void selectCurrentBookmark(const QModelIndex &modelIndex);

private:
	class StelCore* core;
	class StelObjectMgr* objectMgr;

	//! Update header names for bookmarks table
	void setBookmarksHeaderNames();

	//! Init header and list of bookmarks
	void initListBookmarks();

	//! Populates the drop-down list of bookmarks.
	void populateBookmarksList();
};

// Reimplements the QTreeWidgetItem class to fix the sorting bug
class BmTreeWidgetItem : public QTreeWidgetItem
{
public:
	BmTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const
	{
		int column = treeWidget()->sortColumn();
		return text(column).toLower() < other.text(column).toLower();
	}
};

#endif // _BOOKMARKSDIALOG_HPP_
