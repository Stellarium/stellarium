/*
 * Stellarium Location List Editor
 * Copyright (C) 2012  Bogdan Marinov
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
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef LOCATIONLISTEDITOR_HPP
#define LOCATIONLISTEDITOR_HPP

#include <QMainWindow>

#include <QList>
#include <QString>

namespace Ui {
class LocationListEditor;
}
class LocationListModel;
class QLabel;
class QItemSelectionModel;
class QSortFilterProxyModel;

//! Main class of the %Location List Editor.
class LocationListEditor : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit LocationListEditor(QWidget *parent = 0);
	~LocationListEditor();
	
protected:
	void changeEvent(QEvent* event);
	//! Reimplemented to prompt for saving.
	void closeEvent(QCloseEvent* event);
	
private:
	Ui::LocationListEditor *ui;
	//! Status bar indicator showing the number of rows and filtered rows.
	QLabel* filterIndicator;
	LocationListModel* locations;
	QSortFilterProxyModel* proxyModel;
	QItemSelectionModel* selectionModel;
	
	//! Path to the currently opened list.
	QString openFilePath;
	//! Path to the /data/base_locations.txt file of the curret project.
	//! The "current" is the project from which's /utils the app is run.
	//! Empty if not found.
	QString projectFilePath;
	//! Path to the user_locations.txt in the curren user data directory.
	//! Empty if not found.
	QString userFilePath;
	
	//! Used in system file dialogs.
	QString fileFilters;
	
	//! If there's no file loaded, hide the table and show a message.
	bool checkIfFileIsLoaded();
	//! If the file is modified, prompt to save it.
	//! @returns true if the save operation was succcessful or if the file 
	//! was not modified.
	bool checkIfFileIsSaved();
	
	//! Load a list from the specified path.
	bool loadFile(const QString& path);
	//! Save the current list to the specified path.
	bool saveFile(const QString& path);
	//! Save the current list in binary form.
	//! (The binary format used by Stellarium to speed up the base location
	//! list loading.)
	//! @param path The path to the \b text file. The function will
	//! use the same path and file name, but with ".bin.gz" suffix.
	bool saveBinary(const QString& path);
	
	//! Get a sorted list of the row numbers/list indexes of the selection.
	QList<int> getIndexesOfSelection();
	//! Get the row number/list index of the current item.
	//! @returns -1 if there's no valid current item (really unlikely).
	//! @warning There's a difference between "selected" and "current". See the Qt documentation.
	int getIndexOfCurrentRow();
	//! Set the current location and open its name for editing.
	void setCurrentlyEditedLocation(int row);
	//! Set the current row and scroll to the item.
	void goToRow(int row);
	
	//! Show the number of filtered rows and rows total in the status bar.
	void updateFilterIndicator();
	
private slots:
	//! Prompts for file location and opens that file.
	void open();
	//! Tries to open the base location list of the current project.
	//! Assumes that it is run from <project_dir>/utils/location-editor/.
	void openProjectLocations();
	//! Tries to open the location list in the current user's data directory.
	void openUserLocations();
	//! Save the currently opened file.
	//! @returns true if the file has been saved successfully.
	bool save();
	//! Save the currently opened file in another location.
	//! Propmts for a path to the new location.
	bool saveAs();
	//! Append a new location at the end of the list.
	void addNew();
	//! Insert a new location on the row before the current row.
	//! @warning There's a difference between "selected" and "current". See the Qt documentation.
	void insertBefore();
	//! Insert a new location on the row after the current row.
	//! @warning There's a difference between "selected" and "current". See the Qt documentation.
	void insertAfter();
	//! For each selected row, insert a duplicate on the next row.
	void cloneSelected();
	//! Delete the currently selected rows.
	//! @warning There's a difference between "selected" and "current". See the Qt documentation.
	void deleteSelected();
	//! Prompt the user for a row number and center that row in the view.
	void goToRow();
	//! Go to the next location with a duplicate ID in the list.
	void goToNextDuplicate();
	//! Shows a window with author and copyright information.
	void showAboutWindow();
	//! 
	void test();
	
	//! Sets the view column affected by the search/filter field.
	void setFilteredColumn(int column);
	//! 
	void setFilterCaseSensitivity(bool sensitive);
	//! Filters the view according to the current filter text and column.
	void filter();
	//! Resets the current filtering and empties the search/filter field.
	//void clearFilter();
	
	//! Show the number of selected rows in the status bar.
	void updateSelectionCount();
};

#endif // LOCATIONLISTEDITOR_HPP
