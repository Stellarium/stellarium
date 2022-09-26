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

#include "Nebula.hpp"
#include <QStringList>

struct observingListItem
{
	QString name;
	QString nameI18n;
	QString type;
	QString objtype;
	QString ra;
	QString dec;
	QString magnitude;
	QString constellation;
	double jd;
	QString location; // a full location encoded with StelLocation::serializeToLine()
	QString landscapeID; // landscapeID of landscape at moment of item creation.
	double fov;
	bool isVisibleMarker;
};

Q_DECLARE_METATYPE(observingListItem)

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

static constexpr char const *JSON_FILE_NAME = "observingList.json";
static constexpr char const *FILE_VERSION = "2.1";

static constexpr char const *JSON_BOOKMARKS_FILE_NAME = "bookmarks.json";
static constexpr char const *BOOKMARKS_LIST_NAME = "bookmarks list";
static constexpr char const *BOOKMARKS_LIST_DESCRIPTION = "Bookmarks of previous Stellarium version.";
static constexpr char const *SHORT_NAME_VALUE = "Observing list for Stellarium";

static constexpr char const *KEY_DEFAULT_LIST_OLUD = "defaultListOlud";
static constexpr char const *KEY_OBSERVING_LISTS = "observingLists";
static constexpr char const *KEY_CREATION_DATE = "creation date";
static constexpr char const *KEY_BOOKMARKS = "bookmarks";
static constexpr char const *KEY_NAME = "name";
static constexpr char const *KEY_NAME_I18N = "nameI18n";
static constexpr char const *KEY_JD = "jd";
static constexpr char const *KEY_RA = "ra";
static constexpr char const *KEY_DEC = "dec";
static constexpr char const *KEY_FOV = "fov";
static constexpr char const *KEY_DESCRIPTION = "description";
static constexpr char const *KEY_LANDSCAPE_ID = "landscapeID";
static constexpr char const *KEY_OBJECTS = "objects";
static constexpr char const *KEY_OBJECTS_TYPE = "objtype";
static constexpr char const *KEY_TYPE = "type";
static constexpr char const *KEY_DESIGNATION = "designation";
static constexpr char const *KEY_SORTING = "sorting";
static constexpr char const *KEY_LOCATION = "location";
static constexpr char const *KEY_MAGNITUDE = "magnitude";
static constexpr char const *KEY_CONSTELLATION = "constellation";
static constexpr char const *KEY_VERSION = "version";
static constexpr char const *KEY_SHORT_NAME = "shortName";
static constexpr char const *KEY_IS_VISIBLE_MARKER = "isVisibleMarker";

static constexpr char const *SORTING_BY_NAME = "name";
static constexpr char const *SORTING_BY_NAMEI18N = "nameI18n";
static constexpr char const *SORTING_BY_TYPE = "type";
static constexpr char const *SORTING_BY_RA = "right ascension";
static constexpr char const *SORTING_BY_DEC = "declination";
static constexpr char const *SORTING_BY_MAGNITUDE = "magnitude";
static constexpr char const *SORTING_BY_CONSTELLATION = "constellation";
static constexpr char const *SORTING_BY_DATE = "date";
static constexpr char const *SORTING_BY_LOCATION = "location";
static constexpr char const *SORTING_BY_LANDSCAPE = "landscape";

static constexpr int COLUMN_NUMBER_NAME = 1;
static constexpr int COLUMN_NUMBER_NAMEI18N = 2;
static constexpr int COLUMN_NUMBER_TYPE = 3;
static constexpr int COLUMN_NUMBER_RA = 4;
static constexpr int COLUMN_NUMBER_DEC = 5;
static constexpr int COLUMN_NUMBER_MAGNITUDE = 6;
static constexpr int COLUMN_NUMBER_CONSTELLATION = 7;
static constexpr int COLUMN_NUMBER_DATE = 8;
static constexpr int COLUMN_NUMBER_LOCATION = 9;
static constexpr int COLUMN_NUMBER_LANDSCAPE = 10;

static constexpr char const *CUSTOM_OBJECT = "CustomObject";

static const QString dash = QChar(0x2014);

class ObservingListUtil {
public:

    //! Init item
    void initItem(observingListItem &item) {

        item.jd = 0.0;
        item.type = "";
        item.objtype = "";
        item.ra = "";
        item.dec = "";
        item.name = "";
        item.nameI18n = "";
        item.location = "";
	item.landscapeID = "";
	item.fov = 0.0;
        item.constellation = "";
        item.magnitude = "";
        item.isVisibleMarker = false;
    }

    //! Get the magnitude from selected object (or a dash if unavailable)
    QString getMagnitude(const QList<StelObjectP> &selectedObject, StelCore *core) {

	if (!core)
	    return dash;

	QString objectMagnitudeStr = dash;
	const float objectMagnitude = selectedObject[0]->getVMagnitude(core);
	if (objectMagnitude > 98.f)
	{
		if (QString::compare(selectedObject[0]->getType(), "Nebula", Qt::CaseSensitive) == 0)
		{
			auto &r_nebula = dynamic_cast<Nebula &>(*selectedObject[0]);
			const float mB = r_nebula.getBMagnitude(core);
			if (mB < 98.f)
				objectMagnitudeStr = QString::number(mB);
		}
	}
	else
		objectMagnitudeStr = QString::number(objectMagnitude, 'f', 2);

	return objectMagnitudeStr;
    }
};

#endif // OBSERVINGLISTCOMMON_H
