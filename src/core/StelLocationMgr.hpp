/*
 * Copyright (C) 2008 Fabien Chereau
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef STELLOCATIONMGR_HPP
#define STELLOCATIONMGR_HPP

#include "StelLocation.hpp"
#include <QString>
#include <QObject>
#include <QMetaType>
#include <QMap>

typedef QList<StelLocation> LocationList;
typedef QMap<QString,StelLocation> LocationMap;
typedef QMap<QByteArray,QByteArray> TimezoneNameMap;

class GPSLookupHelper;

//! @class StelLocationMgr
//! Manage the list of available location.
class StelLocationMgr : public QObject
{
	Q_OBJECT

public:
	//! Default constructor which loads the list of locations from the base and user location files.
	StelLocationMgr();
	~StelLocationMgr();

	//! Construct a StelLocationMgr which uses the locations given instead of loading them from the files.
	StelLocationMgr(const LocationList& locations);

	//! Replaces the loaded location list
	void setLocations(const LocationList& locations);

	//! Return the list of all loaded locations
	LocationList getAll() const {return locations.values();}

	//! Returns a map of all loaded locations. The key is the location ID, suitable for a list view.
	LocationMap getAllMap() const { return locations; }

	//! Return the StelLocation from a CLI
	const StelLocation locationFromCLI() const;

	//! Return a valid location when no valid one was found.
	const StelLocation& getLastResortLocation() const {return lastResortLocation;}
	
	//! Get whether a location can be permanently added to the list of user locations
	//! The main constraint is that the small string must be unique
	bool canSaveUserLocation(const StelLocation& loc) const;

	//! Add permanently a location to the list of user locations
	//! It is later identified by its small string
	bool saveUserLocation(const StelLocation& loc);

	//! Get whether a location can be deleted from the list of user locations
	//! If the location comes from the base read only list, it cannot be deleted
	//! @param id the location ID
	bool canDeleteUserLocation(const QString& id) const;

	//! Delete permanently the given location from the list of user locations
	//! If the location comes from the base read only list, it cannot be deleted and false is returned
	//! @param id the location ID
	bool deleteUserLocation(const QString& id);

	//! Find list of locations within @param radiusDegrees of selected (usually screen-clicked) coordinates.
	LocationMap pickLocationsNearby(const QString planetName, const float longitude, const float latitude, const float radiusDegrees);
	//! Find list of locations in a particular country only.
	LocationMap pickLocationsInCountry(const QString country);

public slots:
	//! Return the StelLocation for a given string
	//! Can match location name, or coordinates
	const StelLocation locationForString(const QString& s) const;

	//! Find location via online lookup of IP address
	void locationFromIP();

	//! return a QStringList of valid timezone names in Stellarium's location database.
	QStringList getAllTimezoneNames() const;

#ifdef ENABLE_GPS
	//! Try to get a location from GPS lookup.
	//! This prefers GPSD on non-Windows platforms, and uses Qt positioning with a NMEA serial device otherwise
	//! Use the gpsResult() signal to determine if the location was set successfully.
	//! With argument 0, this always signals true.
	//! @note When using GPSD not on localhost, don't forget the -G switch when starting gpsd there!
	//! @param interval set negative to just fetch one position, mseconds to start periodic query, or 0 to stop those.
	//! It may be better to leave it running to observe incoming data, then switch off when fix seems good.
	//! When disabled, the NMEA device and its serial connection is released.
	void locationFromGPS(int interval=-1);
#endif

	//! Check timezone string and return either the same or one that we use in the Stellarium location database.
	//! If timezone name starts with "UTC", always return unchanged.
	//! This is required to store timezone names exactly as we know them, and not mix ours and current-IANA spelling flavour.
	static QString sanitizeTimezoneStringForLocationDB(QString tzString);
	//! Attempt to translate a timezone name from those used in Stellarium's location database to a name which is known
	//! to Qt at runtime as result of QTimeZone::availableTimeZoneIds(). That list may be updated by OS anytime and is known to differ
	//! between OSes. Some spellings may be different, or in some cases some names get simply translated to "UTC+HH:MM" style.
	//! The empty string gets translated to "UTC".
	static QString sanitizeTimezoneStringFromLocationDB(QString dbString);

signals:
	//! Can be used to detect changes to the full location list
	//! i.e. when the user added or removed locations
	void locationListChanged();

#ifdef ENABLE_GPS
	//! emitted when GPS location query and setting location either succeed or fail.
	//! @param success true if successful, false in case of any error (no device, timeout, bad fix, ...).
	void gpsQueryFinished(bool success);
#endif
private slots:
	//! Process answer from online lookup of IP address
	void changeLocationFromNetworkLookup();
#ifdef ENABLE_GPS
	void changeLocationFromGPSQuery(const StelLocation& loc);
	void gpsQueryError(const QString& err);
#endif
private:
	void generateBinaryLocationFile(const QString& txtFile, bool isUserLocation, const QString& binFile) const;

	//! Load cities from a file
	static LocationMap loadCities(const QString& fileName, bool isUserLocation);
	static LocationMap loadCitiesBin(const QString& fileName);

	//! The list of all loaded locations
	LocationMap locations;
	//! A Map which has to be used to replace, system- and Qt-version dependent,
	//! timezone names from our location database to the code names currently used by Qt.
	//! Required to avoid https://bugs.launchpad.net/stellarium/+bug/1662132,
	//! details on IANA names with Qt at http://doc.qt.io/qt-5/qtimezone.html.
	//! This has nothing to do with the Windows timezone names!
	//! Key: TZ name as used in our database.
	//! Value: TZ name as may be available instead in the currently running version of Qt.
	//! The list has to be maintained based on empirical observations.
	//! @todo Make it load from a configurable external file.
	static TimezoneNameMap locationDBToIANAtranslations;
	
	StelLocation lastResortLocation;

	GPSLookupHelper *nmeaHelper,*libGpsHelper;
};

#endif // STELLOCATIONMGR_HPP
