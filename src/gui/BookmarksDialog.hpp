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

#ifndef BOOKMARKSDIALOG_HPP
#define BOOKMARKSDIALOG_HPP

#include <QObject>
#include <QStandardItemModel>
#include <QMap>
#include <QDir>
#include <QUuid>

#include "StelDialog.hpp"
#include "StelCore.hpp"

class Ui_bookmarksDialogForm;

struct bookmark
{
	QString name;
	QString nameI18n;
	QString ra;
	QString dec;
	bool isVisibleMarker;
	QString jd;
	QString location;
	double fov;
};
Q_DECLARE_METATYPE(bookmark)

class BookmarksDialog : public StelDialog
{
	Q_OBJECT

public:
	BookmarksDialog(QObject* parent);
	virtual ~BookmarksDialog();

	//! Notify that the application style changed
	void styleChanged();

public slots:
        void retranslate();

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
        virtual void createDialogContent();
	Ui_bookmarksDialogForm *ui;

private slots:
	void addBookmarkButtonPressed();
	void removeBookmarkButtonPressed();
	void goToBookmarkButtonPressed();
	void clearBookmarksButtonPressed();

	void highlightBookrmarksButtonPressed();
	void clearHighlightsButtonPressed();

	void exportBookmarks();
	void importBookmarks();

	void selectCurrentBookmark(const QModelIndex &modelIdx);

private:
	enum BookmarksColumns {
		ColumnUUID,	//! UUID of bookmark
		ColumnName,	//! name or designation of object
		ColumnNameI18n,	//! Localized name of object
		ColumnDate,	//! date and time (optional)
		ColumnLocation,	//! location (optional)
		ColumnCount	//! total number of columns
	};
	QStandardItemModel * bookmarksListModel;

	class StelCore* core;
	class StelObjectMgr* objectMgr;
	class LabelMgr* labelMgr;

	QString bookmarksJsonPath;
	QHash<QString, bookmark> bookmarksCollection;
	QList<int> highlightLabelIDs;

	//! Update header names for bookmarks table
	void setBookmarksHeaderNames();

	void addModelRow(int number, QString uuid, QString name, QString nameI18n = "", QString date = "", QString Location = "");

	void loadBookmarks();
	void saveBookmarks() const;
	void goToBookmark(QString uuid);
};


#endif // BOOKMARKSDIALOG_HPP
