/*
 * Stellarium
 * 
 * Copyright (C) 2016 Alexander Wolf
 * 
 * Last modification: Jocelyn GIROD October 2019
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
class BookmarksListNameDialog;

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
    
protected:
    //! Initialize the dialog widgets and connect the signals/slots.
    virtual void createDialogContent();
	Ui_bookmarksDialogForm *ui;
    
private:
    static constexpr const char* FILE_PREFIX = "bookmarks_";
	
    enum BookmarksColumns {
		ColumnUUID,	//! 0 - UUID of bookmark
		ColumnName,	//! 1 - name or designation of object
		ColumnNameI18n,	//! 2 - Localized name of object
		ColumnDate,	//! 3 - date and time (optional)
		ColumnLocation,	//! 4 - location (optional)
		ColumnCount	//! 5 - total number of columns
	};
    
	//! Data model
    QStandardItemModel * bookmarksListModel;

	class StelCore* core;
	class StelObjectMgr* objectMgr;
	class LabelMgr* labelMgr;

	QString bookmarksJsonPath;
    QString jsonFilePath;
    QString bookmarksDirectoryPath;
	QHash<QString, bookmark> bookmarksCollection;
	QList<int> highlightLabelIDs;

	//! Update header names for bookmarks table
	void setBookmarksHeaderNames();

	void addModelRow(int number, QString uuid, QString name, QString nameI18n = "", QString date = "", QString Location = "");

	void loadBookmarks();
	void saveBookmarks() const;
	void goToBookmark(QString uuid);
    
    //! Load all the bookmarks list files
    void loadBookmarksListFiles();
    
    BookmarksListNameDialog *bookmarksListNameDialog;

public slots:
        void retranslate();


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
    void showBookmarksListNameDialog();
    
    void createNewBookmarksFile();
    
    //! Method called when a list name is selected in the combobox
    void loadSelectedBookmarksList(int selectedIndex );
    
    void deleteCurrentBookmarksList();


};


#endif // BOOKMARKSDIALOG_HPP
