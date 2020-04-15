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
    QString magnitude;
    QString constellation;
    QString jd;
    QString location;
    double fov;
    bool isVisibleMarker;
};
Q_DECLARE_METATYPE ( observingListItem )

enum ObsListColumns {
    ColumnUUID,	//! UUID of object
    ColumnName,	//! Name or designation of object
    ColumnNameI18n,	//! Localized name of object
    ColumnType, //! Type of the object
    ColumnRa,	//! Right ascencion of the object
    ColumnDec,	//! Declination of the object
    ColumnMagnitude,	//! Magnitude of the object
    ColumnConstellation, //! Constellation in which the object is located
    ColumnJD, //! Date in julian day
    ColumnLocation, //! Location where the object is observed
    ColumnCount	//! Total number of columns
};

static constexpr char const * JSON_FILE_NAME = "observingList.json";
static constexpr char const * FILE_VERSION = "1.0";

static constexpr char const * KEY_DEFAULT_LIST_UUID = "defaultListUuid";
static constexpr char const * KEY_OBSERVING_LISTS = "observingLists";
static constexpr char const * KEY_NAME = "name";
static constexpr char const * KEY_JD = "jd";
static constexpr char const * KEY_DESCRIPTION = "description";
static constexpr char const * KEY_OBJECTS = "objects";
static constexpr char const * KEY_DESIGNATION = "designation";
static constexpr char const * KEY_SORTING = "sorting";
static constexpr char const * KEY_LOCATION = "location";
static constexpr char const * KEY_VERSION = "version";
static constexpr char const * KEY_SHORT_NAME = "shortName";

static constexpr char const * SORTING_BY_NAME = "name";
static constexpr char const * SORTING_BY_TYPE = "type";
static constexpr char const * SORTING_BY_RA = "right ascension";
static constexpr char const * SORTING_BY_DEC = "declination";
static constexpr char const * SORTING_BY_MAGNITUDE = "magnitude";
static constexpr char const * SORTING_BY_CONSTTELLATION = "constellation";


#endif // OBSLISTDIALOG_H
