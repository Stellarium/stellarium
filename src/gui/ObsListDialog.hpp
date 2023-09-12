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
#include "StelObjectType.hpp"

//! @class ObsListDialog
//! Since V0.21.2, this class manages the ObservingLists, successor of the Bookmarks feature available since 0.15.
//! Updated in Version 1.0: ObservingLists Version 2.0
//! Updated in Version 23.1: ObservingLists Version 2.1
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
//!     "{84744f7b-c353-45b0-8394-69af2a1e0917}": { // List OLUD. This is a unique ID
//! 	"creation date": "2022-09-29 20:05:07",
//!	"last edit": "2022-11-29 22:15:38",
//! 	"description": "Bookmarks of previous Stellarium version.",
//! 	"name": "bookmarks list",
//! 	"objects": [                                // List is stored alphabetized, but given here in contextualized order for clarity.
//! 	    {
//! 		"designation": "HIP 57632",         // Catalog name. Object should be found by one of StelObjectModuleSubclasses->getStelObjectType(designation, type)
//! 		"type": "Star"                      // Object type code (actually its class name in Stellarium) given by StelObjectModuleSubclasses->getStelObjectType() or StelObject->getType()
//!             "objtype" : "binary, pulsating variable" // More extensive description, localized. This will be re-read on program start to ensure current language.
//! 		"name": "Denebola",                 // Primary English name of the object. This will be re-read on program start to ensure current language.
//! 		"nameI18n": "Denebola",             // Translated name at time of addition to the list. Added for reference when using the exported list elsewhere. The name is retranslated in Stellarium after loading because user language may have changed.
//! 		"ra": "11h49m05.0s",                // Optional. Right Ascension in J2000.0 frame. Good if moving object is stored. If empty, current coordinates will be added on loading.
//! 		"dec": "+14°34'15\"",               // Optional. Declination     in J2000.0 frame. Good if moving object is stored. If empty, current coordinates will be added on loading.
//! 		"constellation": "Leo",             // Optional. Good if moving object is stored.
//! 		"magnitude": "2.10",                // Optional. Good if moving object is stored.
//! 		"fov": 0,                           // Optional. Good if event is stored.
//! 		"jd": 0,                            // Optional. Good if event is stored.
//! 		"landscapeID": "",                  // Optional. Good if event is stored.
//! 		"location": "",                     // Optional. Good if event is stored.
//! 		"isVisibleMarker": false,           // UNKNOWN PURPOSE or NO PROPER IMPLEMENTATION OF WHATEVER THIS SHOULD DO
//! 	    },
//!         ... <other objects>
//! 	],
//! 	"sorting": ""
//!     },                                            // end of list 84744f7b-...
//!     "{bd40274c-a321-40c1-a6f3-bc8f11026326}": {   // List OLUD of next list.
//! 	"creation date": "2022-12-21 11:12:39",
//!	"last edit": "2023-07-29 22:23:38",
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
//! 		"name": "Deneb",                     // Used to be nameI18n, but this is a bad idea for list exchange!
//! 		"ra": "20h41m24.4s",
//! 		"type": "Star"
//! 		"objType": "double star, pulsating variable star"
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
//! Fix a confusion introduced in the 1.* series:
//! The ObsList has entries
//! - "designation": The catalog number (DSO), HIP number (star), or canonical name (planet).
//! - "nameI18n": translated name for display. Actually this is bad design in case of list exchange.
//! - "type": As given by ObjectP->getType() or getObjectType()? This was inconsistent.
//! FIXES:
//! - "designation" used in combination with type as real unique object ID. For DSO, getDSODesignationWIC() must be used.
//! - "type": Stellarium class name. Used to unambiguously identify an object even if equal-named objects exist in different classes.
//! - "name" (new entry) English name of object. Will be written for external use, but never read.
//! - "nameI18n" translated name. Will be written for external use, but never read.


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
		QString name;          //!< Relates to name in the JSON file. This is the object's englishName
		QString nameI18n;      //!< Relates to nameI18n in the JSON file. This is recreated from the actual object and current language during loading of the list. In the JSON file, it has purely add-on value.
		QString objClass;      //!< Object kind ID oriented on Stellarium's object class: Star, Planet (also for sun, moons, Comets, MinorPlanets!), Nebula, ...
		QString objTypeI18n;   //!< More physical type description: star (also Sun!), planet, moon, open cluster, galaxy, region in the sky, ...
		QString ra;            //!< Optional, eq. J2000.0 position at date jd; useful for moving objects
		QString dec;           //!< Optional, eq. J2000.0 position at date jd; useful for moving objects
		QString magnitude;     //!< NOT a redundant bit of information! In case of moving objects, it spares computing details on loading.
		QString constellation; //!< NOT a redundant bit of information! In case of moving objects, it spares computing positions on loading.
		double jd;             //!< optional: stores date of observation
		QString location;      //!< optional: should be a full location encoded with StelLocation::serializeToLine()
		QString landscapeID;   //!< optional: landscapeID of landscape at moment of item creation.
		double fov;            //!< optional: Field of view
		bool isVisibleMarker;  //!< Something around user-markers or SIMBAD-retrieved objects. Test and document!

		//! constructor
		observingListItem():
		designation(""),
		name(""),
		nameI18n(""),
		objClass(""),
		objTypeI18n(""),
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
				{QString(KEY_NAME)             , name},
				{QString(KEY_NAME_I18N)        , nameI18n},
				{QString(KEY_TYPE)             , objClass},
				{QString(KEY_OBJECTS_TYPE)     , objTypeI18n},
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
		ColumnDesignation,   //! English name or catalog designation of object
		ColumnNameI18n,      //! Localized name/nickname of object
		ColumnType,          //! Localized detailed Type of the object
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

protected:
	Ui_obsListDialogForm *ui;

	//! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() override;

private:
	class StelCore *core;
	class StelObjectMgr *objectMgr;
	class LandscapeMgr *landscapeMgr;
	class LabelMgr *labelMgr;

	QStandardItemModel *itemModel;       //!< Data for the table display.
	const QString observingListJsonPath; //!< Path to observingList.json file, set once in constructor.
	const QString bookmarksJsonPath;     //!< Path to bookmarks.json, set once in constructor.

	//! This QVariantMap represents the contents of the observingList.json file. Read ONCE at start. Contains 4 key/value pairs:
	//! - defaultListOlud: The OLUD of the currently configured default list. If empty or invalid, the first list is used as default.
	//! - observingLists:  QVariantMap of OLUD/ObsList pairs
	//! - shortName: "Observing list for Stellarium"
	//! - version: "2.1"
	//! The other methods can load, add, delete, or edit lists from the observingLists list.
	QVariantMap jsonMap;
	QVariantMap observingLists; //!< Contains the observingLists map from the JSON file

	QString defaultOlud;  //!< OLUD (UUID) of default list
	QString selectedOlud; //!< OLUD has to be set before calling loadSelectedObservingList. Could be called currentOlud

	//! filled when loading a selected observing list. This is needed for the interaction with the table/itemModel in GUI
	QHash<QString, observingListItem> currentItemCollection;

	QList<int> highlightLabelIDs; //!< int label IDs for addressing labels by the HighlightMgr

	QList<QString> listNames; //!< List names, used for the ComboBox
	QString currentListName;  //!< Name of list with selectedOlud
	QString sorting;	  //!< Sorting of the list ex: right ascension

	bool flagUseJD;         //!< Property. Store/retrieve date
	bool flagUseLandscape;  //!< Property. Store/retrieve landscape
	bool flagUseLocation;   //!< Property. Store/retrieve location
	bool flagUseFov;        //!< Property. Store/retrieve field of view

	bool tainted;           //!< List has changed. Needs write on exit.
	bool isEditMode;        //!< true if in Edit/Create New mode.
	bool isCreationMode;    //!< if true we are in creation mode otherwise in edit mode. We ONLY need this to decide what to do in case of a CANCEL during EDIT.

	//! Set header names for observing list table
	void setObservingListHeaderNames();

	//! Append row in the obsListListModel (the table to display/select list entries)
	//! @param olud id of the record
	//! @param designation name or the designation of the object
	//! @param nameI18n localized name of the object
	//! @param typeI18n Localized detailed type (e.g. moon, cubewano, galaxy, binary star) of the object
	//! @param ra right assencion of the object
	//! @param dec declination of the object
	//! @param magnitude magnitude of the object
	//! @param constellation constellation in which the object is located
	//! @param date human-redable event date (optional)
	//! @param location name of location (optional)
	//! @param landscapeID of observation (optional)
	void addModelRow(const QString &olud, const QString &designation, const QString &nameI18n, const QString &typeI18n,
			 const QString &ra, const QString &dec, const QString &magnitude, const QString &constellation,
			 const QString &date, const QString &location, const QString &landscapeID);

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
	//! @return OLUD of the imported bookmarks list.
	QString saveBookmarksHashInObservingLists(const QHash<QString, observingListItem> &bookmarksHash);

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
	//! Label all bookmarked objects of the current list with a label
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
	static const QString KEY_LAST_EDIT;
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
