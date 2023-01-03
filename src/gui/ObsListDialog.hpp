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

//! @class ObsListDialog
//! Since V0.21.2, this class manages the ObservingLists, successor of the Bookmarks feature available since 0.15.
//! Updated in Version 1.0: ObservingLists Version 2.0
//!
//! This tool manages Observing Lists based on a unique list identifier, the OLUD = Observing List Unique Designation
//! Old bookmarks.json files are auto-imported at first run. The old file is then not touched any further and could still be used in older versions of Stellarium.
//! This first bookmarks list is then the first stored ObservingList.
//! You can create new lists, edit them, delete them, import/export them and exchange with other users etc.
//! You can also highlight all bookmarked objects of the active list
//!
//! When creating an entry for a selected object, you can optionally store current time, location, landscapeID and field of view.
//! This helps to retrieve the possible same view if location and/or time matter, e.g. for a list of Solar eclipses with their locations and landscapes.
//! Obviously, a list of favourite DSOs would not need those data (but maybe FoV is still meaningful).
//! On retrieval, the same optional elements can again be selected, so you can retrieve an object without stored time or location, if that is meaningful.
//! If location or landscape IDs cannot be found, they are not changed.
//!
//! An observingList.json looks like this:
//! {
//! "defaultListOlud": "{6d297068-a644-4d1b-9d2d-9c2dd64eef53}",
//! "observingLists": {
//!     "{84744f7b-c353-45b0-8394-69af2a1e0917}": {
//! 	"creation date": "2022-09-29 20:05:07",
//! 	"description": "Bookmarks of previous Stellarium version.",
//! 	"name": "bookmarks list",
//! 	"objects": [
//! 	    {
//! 		"constellation": "Leo",
//! 		"dec": "+14°34'15\"",
//! 		"designation": "HIP 57632",
//! 		"fov": 0,
//! 		"isVisibleMarker": false,
//! 		"jd": 0,
//! 		"landscapeID": "",
//! 		"location": "",
//! 		"magnitude": "2.10",
//! 		"nameI18n": "Denebola",
//! 		"ra": "11h49m05.0s",
//! 		"type": "Star"
//! 	    },
//!         ... <other objects>
//! 	],
//! 	"sorting": ""
//!     },
//!     "{bd40274c-a321-40c1-a6f3-bc8f11026326}": {
//! 	"creation date": "2022-12-21 11:12:39",
//! 	"description": "test of unification",
//! 	"name": "mine_edited",
//! 	"objects": [
//! 	    {
//! 		"constellation": "Cyg",
//! 		"dec": "+45°16'59\"",
//! 		"designation": "HIP 102098",
//! 		"fov": 0,
//! 		"isVisibleMarker": true,
//! 		"jd": 0,
//! 		"landscapeID": "",
//! 		"location": "",
//! 		"magnitude": "1.25",
//! 		"nameI18n": "Deneb",
//! 		"ra": "20h41m24.4s",
//! 		"type": "double star, pulsating variable star"
//! 	    },
//!         ...  <other objects>
//! 	],
//! 	"sorting": ""
//!     },
//!     ... <other observingLists>
//! },
//! "shortName": "Observing list for Stellarium",
//! "version": "2.0"
//! }
//!
//!
//! Updated for 23.1: Integrated functions of extra edit dialog, deep refactoring.
//! You cannot delete the default list. Choose another list as default before deleting the displayed one.
//! You cannot delete the last list.
//! Importing a JSON file with observingLists will import all lists and unconditionally overwrite existing lists with the same OLUD.
//! Exporting writes an observingList file with only the currently displayed list.
//!
//! Attempt to fix a confusion introduced in the 1.* series:
//! The ObsList has entries
//! - "designation": The catalog number (DSO), HIP number (star), or canonical name (planet)
//! - "nameI18n": translated name for display. Actually this is bad in case of exchange.
//! - "type": As given by ObjectP->


class Ui_obsListDialogForm;

class ObsListDialog : public StelDialog
{
	Q_OBJECT
	// Configure optionally stored/retrieved items
	Q_PROPERTY(bool flagUseJD        READ getFlagUseJD        WRITE setFlagUseJD        NOTIFY flagUseJDChanged)
	Q_PROPERTY(bool flagUseLandscape READ getFlagUseLandscape WRITE setFlagUseLandscape NOTIFY flagUseLandscapeChanged)
	Q_PROPERTY(bool flagUseLocation  READ getFlagUseLocation  WRITE setFlagUseLocation  NOTIFY flagUseLocationChanged)
	Q_PROPERTY(bool flagUseFov       READ getFlagUseFov       WRITE setFlagUseFov       NOTIFY flagUseFovChanged)

public:
	explicit ObsListDialog(QObject *parent);

	~ObsListDialog() override;

	struct observingListItem
	{
	public:
		QString designation;   //!< Relates to designation in the JSON file
		QString nameI18n;      //!< Relates to name in the JSON file.
		QString type;          //!< type oriented on Stellarium's object class: Star, Planet (also for sun and moons!), Nebula, ...
		QString objtype;       //!< More physical type description: star (also Sun!), planet, moon, open cluster, galaxy, region in the sky, ...
		QString ra;
		QString dec;
		QString magnitude;
		QString constellation; //!< NOT a redundant bit of information! In case of moving objects, it spares computing positions on loading.
		double jd;             //!< optional: stores date of observation
		QString location;      //!< optional: should be a full location encoded with StelLocation::serializeToLine()
		QString landscapeID;   //!< optional: landscapeID of landscape at moment of item creation.
		double fov;            //!< optional: Field of view
		bool isVisibleMarker;  //!<

		//! constructor
		observingListItem():
		designation(""),
		nameI18n(""),
		type(""),
		objtype(""),
		ra(""),
		dec(""),
		magnitude(""),
		constellation(""),
		jd(0.0),
		location(""),
		landscapeID(""),
		fov(0.0),
		isVisibleMarker(false)
		{}
		//! Convert to QVariantMap
		QVariantMap toVariantMap(){
			return {
				{QString(KEY_DESIGNATION)      , designation},
				{QString(KEY_NAME_I18N)        , nameI18n},
				{QString(KEY_TYPE)             , type},
				{QString(KEY_OBJECTS_TYPE)     , objtype},
				{QString(KEY_RA)               , ra},
				{QString(KEY_DEC)              , dec},
				{QString(KEY_MAGNITUDE)        , magnitude},
				{QString(KEY_CONSTELLATION)    , constellation},
				{QString(KEY_JD)               , jd},
				{QString(KEY_LOCATION)         , location},
				{QString(KEY_LANDSCAPE_ID)     , landscapeID},
				{QString(KEY_FOV)              , fov>1.e-6 ? fov : 0},
				{QString(KEY_IS_VISIBLE_MARKER), isVisibleMarker}};
		}
	};

	enum ObsListColumns {
		ColumnUUID,          //! UUID of object
		ColumnName,          //! Name or designation of object
		ColumnNameI18n,      //! Localized name of object
		ColumnType,          //! Type of the object
		ColumnRa,            //! Right ascension of the object
		ColumnDec,           //! Declination of the object
		ColumnMagnitude,     //! Magnitude of the object
		ColumnConstellation, //! Constellation in which the object is located
		ColumnDate,          //! Date
		ColumnLocation,      //! Location where the object is observed
		ColumnLandscapeID,   //! LandscapeID linked to object observation
		ColumnCount          //! Total number of columns
	};
	Q_ENUM(ObsListColumns)
	// Notify that the application style changed
	//void styleChanged() override;

	//void setVisible(bool v) override;

protected:
	Ui_obsListDialogForm *ui;

	//! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() override;

private:
	class StelCore *core;
	class StelObjectMgr *objectMgr;
	class LandscapeMgr *landscapeMgr;
	class LabelMgr *labelMgr;

	QStandardItemModel *itemModel; //!< Data for the table display.
	const QString observingListJsonPath; //!< Path to observingList.json file, set once in constructor.
	const QString bookmarksJsonPath;     //!< Path to bookmarks.json, set once in constructor.

	//! This QVariantMap represents the contents of the observingList.json file. Read ONCE at start. Contains 4 key/value pairs:
	//! - defaultListOlud: The OLUD of the currently configured default list. If empty or invalid, the first list is used as default.
	//! - observingLists:  QVariantMap of OLUD/ObsList pairs
	//! - shortName: "Observing list for Stellarium"
	//! - version: "2.0"
	//! The other methods can load, add, delete, or edit lists from the observingLists list.
	QVariantMap jsonMap;
	QVariantMap observingLists; //!< Contains the observingLists map from the JSON file

	QString defaultOlud;  //!< OLUD (UUID) of default list
	QString selectedOlud; //!< OLUD has to be set before calling loadSelectedObservingList. Could be called currentListOlud

	//! filled when loading a selected observing list. This is needed for the interaction with the table/itemModel in GUI
	QHash<QString, observingListItem> currentItemCollection;

	QList<int> highlightLabelIDs; //!< int label IDs for addressing labels by the HighlightMgr

	//!List names, used for the ComboBox
	QList<QString> listNames;
	QString currentListName;
	QString sorting;	//!< Sorting of the list ex: right ascension

	//properties:
	bool flagUseJD;
	bool flagUseLandscape;
	bool flagUseLocation;
	bool flagUseFov;

	bool tainted;           //!< Needs write on exit
	bool isEditMode;        //!< true if in Edit/Create New mode.
	bool isCreationMode;    //!< if true we are in creation mode otherwise in edit mode. We ONLY need this to decide what to do in case of a CANCEL during EDIT.

	//! Set header names for observing list table
	void setObservingListHeaderNames();

	//! Append row in the obsListListModel (the table to display/select list entries)
	//! @param olud id of the record
	//! @param name name or the designation of the object
	//! @param type type of the object
	//! @param ra right assencion of the object
	//! @param dec declination of the object
	//! @param magnitude magnitude of the object
	//! @param constellation constellation in which the object is located
	//! @param date human-redable event date (optional)
	//! @param location name of location (optional)
	//! @param landscapeID of observation (optional)
	void addModelRow(const QString &olud, const QString &name, const QString &nameI18n, const QString &type,
			 const QString &ra, const QString &dec, const QString &magnitude, const QString &constellation,
			 const QString &date, const QString &location, const QString &landscapeID);

	//! Returns the defaultListOlud from the jsonMap, or an empty QString.
	QString extractDefaultOlud();

	//! Load the lists names from jsonMap,
	//! Populate the list names into combo box and extract defaultOlud
	void loadListNames();

	//! Load the selected observing list (selectedOlud) from the jsonMap.
	void loadSelectedList();

	//! Load the default list. If no default has been defined, current or first list (from comboBox) is loaded and are made default.
	void loadDefaultList();

	//! Load the bookmarks of a file (usually bookmarks.json) into a temporary structure
	QHash<QString, observingListItem> loadBookmarksFile(QFile &file);

	//! Prepare the currently displayed/edited list for storage
	//! Returns QVariantList with keys={creation date, description, name, objects, sorting}
	QVariantMap prepareCurrentList(QHash<QString, observingListItem> &itemHash);

	//! Put the bookmarks in bookmarksHash into observingLists under the listname "bookmarks list". Does not write JSON!
	void saveBookmarksHashInObservingLists(const QHash<QString, observingListItem> &bookmarksHash);

	//! Sort the obsListTreeView by the column name given in parameter
	void sortObsListTreeViewByColumnName(const QString &columnName);


	//! Check if bookmarks list already exists in observing list file,
	//! i.e., if the old bookmarks file has already be loaded.
	bool checkIfBookmarksListExists();

	//! NEW: Switch between regular and edit modes.
	//! @param newList activate creation mode. This is only relevant to aborts
	void switchEditMode(bool enableEditMode, bool newList);

	//! Save the object information into json file
	void saveObservedObjectsInJsonFile();

	//! Get the magnitude from selected object (or a dash if unavailable)
	static QString getMagnitude(const QList<StelObjectP> &selectedObject, StelCore *core);

signals:
	void flagUseJDChanged(bool b);
	void flagUseLandscapeChanged(bool b);
	void flagUseLocationChanged(bool b);
	void flagUseFovChanged(bool b);

public slots:

	void retranslate() override;
	bool getFlagUseJD() {return flagUseJD;}
	bool getFlagUseLandscape() {return flagUseLandscape;}
	bool getFlagUseLocation() {return flagUseLocation;}
	bool getFlagUseFov() {return flagUseFov;}
	void setFlagUseJD(bool b);
	void setFlagUseLandscape(bool b);
	void setFlagUseLocation(bool b);
	void setFlagUseFov(bool b);

private slots:

	//! Label all bookmarked objects with a label
	void highlightAll();
	//! Clear highlights (remove screen labels)
	void clearHighlights();

	//! Method called when a list name is selected in the combobox
	//! @param selectedIndex the index of the list name in the combo box
	void loadSelectedObservingList(int selectedIndex);

	//! Select and go to object
	//! @param index the QModelIndex of the list
	void selectAndGoToObject(QModelIndex index);

	//! Create a new empty list (new OLUD), make it the active list and enable editing
	void newListButtonPressed();

	//! Initiate edit mode: Enable/disable GUI elements and initiate editing
	void editListButtonPressed();

	void deleteListButtonPressed();

	void addObjectButtonPressed();

	void removeObjectButtonPressed();

	void exportListButtonPressed();

	void importListButtonPressed();

	void saveButtonPressed();

	void cancelButtonPressed();

	void headerClicked(int index);

	//! Connected to the defaultList checkbox
	void defaultClicked(bool b);

private:
	static const QString JSON_FILE_NAME;
	static const QString JSON_FILE_BASENAME;
	static const QString FILE_VERSION;

	static const QString JSON_BOOKMARKS_FILE_NAME;
	static const QString BOOKMARKS_LIST_NAME;
	static const QString BOOKMARKS_LIST_DESCRIPTION;
	static const QString SHORT_NAME_VALUE;

	static const QString KEY_DEFAULT_LIST_OLUD;
	static const QString KEY_OBSERVING_LISTS;
	static const QString KEY_CREATION_DATE;
	static const QString KEY_BOOKMARKS;
	static const QString KEY_NAME;
	static const QString KEY_NAME_I18N;
	static const QString KEY_JD;
	static const QString KEY_RA;
	static const QString KEY_DEC;
	static const QString KEY_FOV;
	static const QString KEY_DESCRIPTION;
	static const QString KEY_LANDSCAPE_ID;
	static const QString KEY_OBJECTS;
	static const QString KEY_OBJECTS_TYPE;
	static const QString KEY_TYPE;
	static const QString KEY_DESIGNATION;
	static const QString KEY_SORTING;
	static const QString KEY_LOCATION;
	static const QString KEY_MAGNITUDE;
	static const QString KEY_CONSTELLATION;
	static const QString KEY_VERSION;
	static const QString KEY_SHORT_NAME;
	static const QString KEY_IS_VISIBLE_MARKER;

	static const QString SORTING_BY_NAME;
	static const QString SORTING_BY_NAMEI18N;
	static const QString SORTING_BY_TYPE;
	static const QString SORTING_BY_RA;
	static const QString SORTING_BY_DEC;
	static const QString SORTING_BY_MAGNITUDE;
	static const QString SORTING_BY_CONSTELLATION;
	static const QString SORTING_BY_DATE;
	static const QString SORTING_BY_LOCATION;
	static const QString SORTING_BY_LANDSCAPE_ID;

	static const QString CUSTOM_OBJECT;

	static const QString DASH;
};

#endif // OBSLISTDIALOG_H
