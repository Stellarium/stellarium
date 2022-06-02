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

#ifndef OBSERVINGLISTCOMMON_H
#define OBSERVINGLISTCOMMON_H

struct observingListItem {
    QString name;
    QString nameI18n;
    QString type;
    QString ra;
    QString dec;
    QString magnitude; //TODO voir si c'est utilisé
    QString constellation;
    double jd;
    QString location;
    double fov;
    bool isVisibleMarker; //TODO voir si c'est vraiment utilisé
};
Q_DECLARE_METATYPE ( observingListItem )

//TODO certain champ peuvent ne pas servir - penser à les supprimer
// à vérifier après la V2
struct bookmark
{
	QString name;
	QString nameI18n;
	QString ra;
	QString dec;
	bool isVisibleMarker;
	double jd;
	QString location;
	double fov;

};
Q_DECLARE_METATYPE(bookmark)

enum ObsListColumns {
    ColumnUUID,	    //! UUID of object
    ColumnName,	    //! Name or designation of object
    ColumnNameI18n,	    //! Localized name of object
    ColumnType,     //! Type of the object
    ColumnRa,	  //! Right ascension of the object
    ColumnDec,	 //! Declination of the object
    ColumnMagnitude,	//! Magnitude of the object
    ColumnConstellation,    //! Constellation in which the object is located
    ColumnJD,   //! Date in julian day
    ColumnLocation,     //! Location where the object is observed
    ColumnCount	    //! Total number of columns
};

//TODO certaine clé peuvent ne pas servir - penser à les supprimer
// à vérifier après la V2

static constexpr char const * JSON_FILE_NAME = "observingList.json";
static constexpr char const * FILE_VERSION = "2.0";

static constexpr char const * JSON_BOOKMARKS_FILE_NAME = "bookmarks.json";
static constexpr char const * BOOKMARKS_LIST_NAME = "bookmarks list";
static constexpr char const * BOOKMARKS_LIST_DESCRIPTION = "Bookmarks of previous Stellarium version.";
static constexpr char const * SHORT_NAME_VALUE = "Observing list for Stellarium";

static constexpr char const * KEY_DEFAULT_LIST_OLUD = "defaultListOlud";
static constexpr char const * KEY_OBSERVING_LISTS = "observingLists";
static constexpr char const * KEY_CREATION_DATE = "creation date";
static constexpr char const * KEY_BOOKMARKS = "bookmarks";
static constexpr char const * KEY_NAME = "name";
static constexpr char const * KEY_NAME_I18N = "nameI18n";
static constexpr char const * KEY_JD = "jd";
static constexpr char const * KEY_RA = "ra";
static constexpr char const * KEY_DEC = "dec";
static constexpr char const * KEY_FOV = "fov";
static constexpr char const * KEY_DESCRIPTION = "description";
static constexpr char const * KEY_LANDSCAPE_ID = "landscape id";
static constexpr char const * KEY_OBJECTS = "objects";
static constexpr char const * KEY_OBJECTS_TYPE = "objtype";
static constexpr char const * KEY_DESIGNATION = "designation";
static constexpr char const * KEY_SORTING = "sorting";
static constexpr char const * KEY_LOCATION = "location";
static constexpr char const * KEY_MAGNITUDE = "magnitude";
static constexpr char const * KEY_CONSTELLATION = "constellation";
static constexpr char const * KEY_VERSION = "version";
static constexpr char const * KEY_SHORT_NAME = "shortName";
static constexpr char const * KEY_IS_VISIBLE_MARKER = "isVisibleMarker";

static constexpr char const * SORTING_BY_NAME = "name";
static constexpr char const * SORTING_BY_TYPE = "type";
static constexpr char const * SORTING_BY_RA = "right ascension";
static constexpr char const * SORTING_BY_DEC = "declination";
static constexpr char const * SORTING_BY_MAGNITUDE = "magnitude";
static constexpr char const * SORTING_BY_CONSTELLATION = "constellation";

static constexpr int COLUMN_NUMBER_NAME = 1;
static constexpr int COLUMN_NUMBER_TYPE = 3;
static constexpr int COLUMN_NUMBER_RA = 4;
static constexpr int COLUMN_NUMBER_DEC = 5;
static constexpr int COLUMN_NUMBER_MAGNITUDE = 6;
static constexpr int COLUMN_NUMBER_CONSTELLATION = 7;

static constexpr char const * CUSTOM_OBJECT = "CustomObject";


#endif // OBSLISTDIALOG_H
