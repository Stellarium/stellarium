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

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelLocationMgr.hpp"
#include "StelUtils.hpp"
#include "StelJsonParser.hpp"
#include "StelLocaleMgr.hpp"

#include <QStringListModel>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QNetworkInterface>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QSettings>
#include <QTimeZone>


TimezoneNameMap StelLocationMgr::locationDBToIANAtranslations;

StelLocationMgr::StelLocationMgr()
{

	// initialize the static QMap first if necessary.
	if (locationDBToIANAtranslations.count()==0)
	{
		// Missing on Qt5.7/Win10 as of 2017-03-18.
		locationDBToIANAtranslations.insert("Europe/Astrakhan", "UTC+04:00");
		locationDBToIANAtranslations.insert("Europe/Ulyanovsk", "UTC+04:00");
		locationDBToIANAtranslations.insert("Europe/Kirov",     "UTC+03:00");
		locationDBToIANAtranslations.insert("Asia/Hebron",      "Asia/Jerusalem");
		locationDBToIANAtranslations.insert("Asia/Gaza",        "Asia/Jerusalem"); // or use UTC+2:00? (political issue...)
		locationDBToIANAtranslations.insert("Asia/Kolkata",     "Asia/Calcutta");
		locationDBToIANAtranslations.insert("Asia/Kathmandu",   "Asia/Katmandu");
		locationDBToIANAtranslations.insert("Asia/Tomsk",       "Asia/Novosibirsk");
		locationDBToIANAtranslations.insert("Asia/Barnaul",     "UTC+07:00");
		locationDBToIANAtranslations.insert("Asia/Ho_Chi_Minh", "Asia/Saigon");
		locationDBToIANAtranslations.insert("Asia/Hovd",        "UTC+07:00");
		locationDBToIANAtranslations.insert("America/Argentina/Buenos_Aires", "America/Buenos_Aires");
		locationDBToIANAtranslations.insert("America/Argentina/Jujuy",        "America/Jujuy");
		locationDBToIANAtranslations.insert("America/Argentina/Mendoza",      "America/Mendoza");
		locationDBToIANAtranslations.insert("America/Argentina/Catamarca",    "America/Catamarca");
		locationDBToIANAtranslations.insert("America/Argentina/Cordoba",      "America/Cordoba");
		locationDBToIANAtranslations.insert("America/Indiana/Indianapolis",   "America/Indianapolis");
		locationDBToIANAtranslations.insert("America/Kentucky/Louisville",    "America/Louisville");
		locationDBToIANAtranslations.insert("America/Miquelon",               "UTC-03:00");  // Small Canadian island.
		locationDBToIANAtranslations.insert("Africa/Asmara",     "Africa/Asmera");
		locationDBToIANAtranslations.insert("Atlantic/Faroe",    "Atlantic/Faeroe");
		locationDBToIANAtranslations.insert("Pacific/Pohnpei",   "Pacific/Ponape");
		locationDBToIANAtranslations.insert("Pacific/Pitcairn",  "UTC-08:00");
		// Missing on Qt5.5.1/Ubuntu 16.04.1 LTE as of 2017-03-18
		locationDBToIANAtranslations.insert("Asia/Rangoon",      "Asia/Yangon"); // UTC+6:30 Missing on Ubuntu/Qt5.5.1.
		locationDBToIANAtranslations.insert( "", "UTC");
		// N.B. Further missing TZ names will be printed out in the log.txt. Resolve these by adding into this list.
		// TODO later: create a text file in user data directory.
	}

	QSettings* conf = StelApp::getInstance().getSettings();

	// The line below allows to re-generate the location file, you still need to gunzip it manually afterward.
	if (conf->value("devel/convert_locations_list", false).toBool())
		generateBinaryLocationFile("data/base_locations.txt", false, "data/base_locations.bin");

	locations = loadCitiesBin("data/base_locations.bin.gz");
	locations.unite(loadCities("data/user_locations.txt", true));
	
	// Init to Paris France because it's the center of the world.
	lastResortLocation = locationForString(conf->value("init_location/last_location", "Paris, France").toString());
}

StelLocationMgr::StelLocationMgr(const LocationList &locations)
{
	setLocations(locations);

	QSettings* conf = StelApp::getInstance().getSettings();
	// Init to Paris France because it's the center of the world.
	lastResortLocation = locationForString(conf->value("init_location/last_location", "Paris, France").toString());
}

void StelLocationMgr::setLocations(const LocationList &locations)
{
	for(LocationList::const_iterator it = locations.constBegin();it!=locations.constEnd();++it)
	{
		this->locations.insert(it->getID(),*it);
	}

	emit locationListChanged();
}

void StelLocationMgr::generateBinaryLocationFile(const QString& fileName, bool isUserLocation, const QString& binFilePath) const
{
	qWarning() << "Generating a locations list...";
	const QMap<QString, StelLocation>& cities = loadCities(fileName, isUserLocation);
	QFile binfile(StelFileMgr::findFile(binFilePath));
	if(binfile.open(QIODevice::WriteOnly))
	{
		QDataStream out(&binfile);
		out.setVersion(QDataStream::Qt_5_2);
		out << cities;
		binfile.close();
	}
}

LocationMap StelLocationMgr::loadCitiesBin(const QString& fileName)
{
	QMap<QString, StelLocation> res;
	QString cityDataPath = StelFileMgr::findFile(fileName);
	if (cityDataPath.isEmpty())
		return res;

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::ReadOnly))
	{
		qWarning() << "ERROR: Could not open location data file: " << QDir::toNativeSeparators(cityDataPath);
		return res;
	}

	if (fileName.endsWith(".gz"))
	{
		QDataStream in(StelUtils::uncompress(sourcefile.readAll()));
		in.setVersion(QDataStream::Qt_5_2);
		in >> res;
	}
	else
	{
		QDataStream in(&sourcefile);
		in.setVersion(QDataStream::Qt_5_2);
		in >> res;
	}
	// Now res has all location data. However, some timezone names are not available in various versions of Qt.
	// Sanity checks: It seems we must translate timezone names. Quite a number on Windows, but also still some on Linux.
	QList<QByteArray> availableTimeZoneList=QTimeZone::availableTimeZoneIds();
	QStringList unknownTZlist;
	QMap<QString, StelLocation>::iterator i=res.begin();
	while (i!=res.end())
	{
		StelLocation loc=i.value();
		if ((loc.ianaTimeZone!="LMST") &&  (loc.ianaTimeZone!="LTST") && ( ! availableTimeZoneList.contains(loc.ianaTimeZone.toUtf8())) )
		{
			// TZ name which is currently unknown to Qt detected. See if we can translate it, if not: complain to qDebug().
			QString fixTZname=sanitizeTimezoneStringFromLocationDB(loc.ianaTimeZone);
			if (availableTimeZoneList.contains(fixTZname.toUtf8()))
			{
				loc.ianaTimeZone=fixTZname;
				i.value() = loc;
			}
			else
			{
				qDebug() << "StelLocationMgr::loadCitiesBin(): TimeZone for " << loc.name <<  " not found: " << loc.ianaTimeZone;
				unknownTZlist.append(loc.ianaTimeZone);
			}
		}
		++i;
	}
	if (unknownTZlist.length()>0)
	{
		unknownTZlist.removeDuplicates();
		qDebug() << "StelLocationMgr::loadCitiesBin(): Summary of unknown TimeZones:";
		QStringList::const_iterator t=unknownTZlist.begin();
		while (t!=unknownTZlist.end())
		{
			qDebug() << *t;
			++t;
		}
		qDebug() << "Please report these timezone names (this logfile) to the Stellarium developers.";
		// Note to developers: Fill those names and replacements to the map above.
	}

	return res;
}

// Done in the following: TZ name sanitizing also for text file!
LocationMap StelLocationMgr::loadCities(const QString& fileName, bool isUserLocation)
{
	// Load the cities from data file
	QMap<QString, StelLocation> locations;
	QString cityDataPath = StelFileMgr::findFile(fileName);
	if (cityDataPath.isEmpty())
	{
		// Note it is quite normal not to have a user locations file (e.g. first run)
		if (!isUserLocation)
			qWarning() << "WARNING: Failed to locate location data file: " << QDir::toNativeSeparators(fileName);
		return locations;
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "ERROR: Could not open location data file: " << QDir::toNativeSeparators(cityDataPath);
		return locations;
	}

	// Read the data serialized from the file.
	// Code below borrowed from Marble (http://edu.kde.org/marble/)
	QTextStream sourcestream(&sourcefile);
	sourcestream.setCodec("UTF-8");
	StelLocation loc;
	while (!sourcestream.atEnd())
	{
		const QString& rawline=sourcestream.readLine();
		if (rawline.isEmpty() || rawline.startsWith('#') || (rawline.split("\t").count() < 8))
			continue;
		loc = StelLocation::createFromLine(rawline);
		loc.isUserLocation = isUserLocation;
		const QString& locId = loc.getID();

		if (locations.contains(locId))
		{
			// Add the state in the name of the existing one and the new one to differentiate
			StelLocation loc2 = locations[locId];
			if (!loc2.state.isEmpty())
				loc2.name += " ("+loc2.state+")";
			// remove and re-add the fixed version
			locations.remove(locId);
			locations.insert(loc2.getID(), loc2);

			if (!loc.state.isEmpty())
				loc.name += " ("+loc.state+")";
			locations.insert(loc.getID(), loc);
		}
		else
		{
			locations.insert(locId, loc);
		}
	}
	sourcefile.close();
	return locations;
}

static float parseAngle(const QString& s, bool* ok)
{
	float ret;
	// First try normal decimal value.
	ret = s.toFloat(ok);
	if (*ok) return ret;
	// Try GPS coordinate like +121°33'38.28"
	QRegExp reg("([+-]?[\\d.]+)°(?:([\\d.]+)')?(?:([\\d.]+)\")?");
	if (reg.exactMatch(s))
	{
		float deg = reg.capturedTexts()[1].toFloat(ok);
		if (!*ok) return 0;
		float min = reg.capturedTexts()[2].isEmpty()? 0 : reg.capturedTexts()[2].toFloat(ok);
		if (!*ok) return 0;
		float sec = reg.capturedTexts()[3].isEmpty()? 0 : reg.capturedTexts()[3].toFloat(ok);
		if (!*ok) return 0;
		return deg + min / 60 + sec / 3600;
	}
	return 0;
}

const StelLocation StelLocationMgr::locationForString(const QString& s) const
{
	QMap<QString, StelLocation>::const_iterator iter = locations.find(s);
	if (iter!=locations.end())
	{
		return iter.value();
	}
	StelLocation ret;
	// Maybe it is a coordinate set ? (e.g. GPS 25.107363,121.558807 )
	QRegExp reg("(?:(.+)\\s+)?(.+),(.+)");
	if (reg.exactMatch(s))
	{
		bool ok;
		// We have a set of coordinates
		ret.latitude = parseAngle(reg.capturedTexts()[2].trimmed(), &ok);
		if (!ok) ret.role = '!';
		ret.longitude = parseAngle(reg.capturedTexts()[3].trimmed(), &ok);
		if (!ok) ret.role = '!';
		ret.name = reg.capturedTexts()[1].trimmed();
		ret.planetName = "Earth";
		return ret;
	}
	ret.role = '!';
	return ret;
}

const StelLocation StelLocationMgr::locationFromCLI() const
{
	StelLocation ret;
	QSettings* conf = StelApp::getInstance().getSettings();
	bool ok;
	conf->beginGroup("location_run_once");
	ret.latitude = parseAngle(StelUtils::radToDmsStr(conf->value("latitude").toFloat(), true), &ok);
	if (!ok) ret.role = '!';
	ret.longitude = parseAngle(StelUtils::radToDmsStr(conf->value("longitude").toFloat(), true), &ok);
	if (!ok) ret.role = '!';
	ret.altitude = conf->value("altitude", 0).toInt(&ok);
	ret.planetName = conf->value("home_planet", "Earth").toString();
	ret.landscapeKey = conf->value("landscape_name", "guereins").toString();
	conf->endGroup();
	conf->remove("location_run_once");
	return ret;
}

// Get whether a location can be permanently added to the list of user locations
bool StelLocationMgr::canSaveUserLocation(const StelLocation& loc) const
{
	return loc.isValid() && locations.find(loc.getID())==locations.end();
}

// Add permanently a location to the list of user locations
bool StelLocationMgr::saveUserLocation(const StelLocation& loc)
{
	if (!canSaveUserLocation(loc))
		return false;

	// Add in the program
	locations[loc.getID()]=loc;

	//emit before saving the list
	emit locationListChanged();

	// Append to the user location file
	QString cityDataPath = StelFileMgr::findFile("data/user_locations.txt", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::File));
	if (cityDataPath.isEmpty())
	{
		if (!StelFileMgr::exists(StelFileMgr::getUserDir()+"/data"))
		{
			if (!StelFileMgr::mkDir(StelFileMgr::getUserDir()+"/data"))
			{
				qWarning() << "ERROR - cannot create non-existent data directory" << QDir::toNativeSeparators(StelFileMgr::getUserDir()+"/data");
				qWarning() << "Location cannot be saved";
				return false;
			}
		}

		cityDataPath = StelFileMgr::getUserDir()+"/data/user_locations.txt";
		qWarning() << "Will create a new user location file: " << QDir::toNativeSeparators(cityDataPath);
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
	{
		qWarning() << "ERROR: Could not open location data file: " << QDir::toNativeSeparators(cityDataPath);
		return false;
	}

	QTextStream outstream(&sourcefile);
	outstream.setCodec("UTF-8");
	outstream << loc.serializeToLine() << '\n';
	sourcefile.close();

	return true;
}

// Get whether a location can be deleted from the list of user locations
// If the location comes from the base read only list, it cannot be deleted
bool StelLocationMgr::canDeleteUserLocation(const QString& id) const
{
	QMap<QString, StelLocation>::const_iterator iter=locations.find(id);

	// If it's not known at all there is a problem
	if (iter==locations.end())
		return false;

	return iter.value().isUserLocation;
}

// Delete permanently the given location from the list of user locations
// If the location comes from the base read only list, it cannot be deleted and false is returned
bool StelLocationMgr::deleteUserLocation(const QString& id)
{
	if (!canDeleteUserLocation(id))
		return false;

	locations.remove(id);

	//emit before saving the list
	emit locationListChanged();

	// Resave the whole remaining user locations file
	QString cityDataPath = StelFileMgr::findFile("data/user_locations.txt", StelFileMgr::Writable);
	if (cityDataPath.isEmpty())
	{
		if (!StelFileMgr::exists(StelFileMgr::getUserDir()+"/data"))
		{
			if (!StelFileMgr::mkDir(StelFileMgr::getUserDir()+"/data"))
			{
				qWarning() << "ERROR - cannot create non-existent data directory" << QDir::toNativeSeparators(StelFileMgr::getUserDir()+"/data");
				qWarning() << "Location cannot be saved";
				return false;
			}
		}

		cityDataPath = StelFileMgr::getUserDir()+"/data/user_locations.txt";
		qWarning() << "Will create a new user location file: " << QDir::toNativeSeparators(cityDataPath);
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "ERROR: Could not open location data file: " << QDir::toNativeSeparators(cityDataPath);
		return false;
	}

	QTextStream outstream(&sourcefile);
	outstream.setCodec("UTF-8");

	for (QMap<QString, StelLocation>::ConstIterator iter=locations.constBegin();iter!=locations.constEnd();++iter)
	{
		if (iter.value().isUserLocation)
		{
			outstream << iter.value().serializeToLine() << '\n';
		}
	}

	sourcefile.close();
	return true;
}

// lookup location from IP address.
void StelLocationMgr::locationFromIP()
{
	QNetworkRequest req( QUrl( QString("http://freegeoip.net/json/") ) );
	req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
	req.setRawHeader("User-Agent", StelUtils::getUserAgentString().toLatin1());
	QNetworkReply* networkReply=StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(networkReply, SIGNAL(finished()), this, SLOT(changeLocationFromNetworkLookup()));
}

// slot that receives IP-based location data from the network.
void StelLocationMgr::changeLocationFromNetworkLookup()
{
	StelLocation location;
	StelCore *core=StelApp::getInstance().getCore();
	QNetworkReply* networkReply = qobject_cast<QNetworkReply*>(sender());
	if (!networkReply)
	    return;
	if (networkReply->error() == QNetworkReply::NoError) {
		//success
		QVariantMap locMap = StelJsonParser::parse(networkReply->readAll()).toMap();
		QString ipRegion = locMap.value("region_name").toString();
		QString ipCity = locMap.value("city").toString();
		QString ipCountry = locMap.value("country_name").toString(); // NOTE: Got a short name of country
		QString ipCountryCode = locMap.value("country_code").toString();
		QString ipTimeZone = locMap.value("time_zone").toString();
		float latitude=locMap.value("latitude").toFloat();
		float longitude=locMap.value("longitude").toFloat();

		qDebug() << "Got location" << QString("%1, %2, %3 (%4, %5; %6)").arg(ipCity).arg(ipRegion).arg(ipCountry).arg(latitude).arg(longitude).arg(ipTimeZone) << "for IP" << locMap.value("ip").toString();

		QString locLine= // we re-pack into a new line that will be parsed back by StelLocation...
				QString("%1\t%2\t%3\tX\t0\t%4\t%5\t0\t\t%6")
				.arg(ipCity.isEmpty() ? QString("%1, %2").arg(latitude).arg(longitude) : ipCity)
				.arg(ipRegion.isEmpty() ? "IPregion"  : ipRegion)
				.arg(ipCountryCode.isEmpty() ? "" : ipCountryCode.toLower())
				.arg(latitude<0 ? QString("%1S").arg(-latitude, 0, 'f', 6) : QString("%1N").arg(latitude, 0, 'f', 6))
				.arg(longitude<0 ? QString("%1W").arg(-longitude, 0, 'f', 6) : QString("%1E").arg(longitude, 0, 'f', 6))
				.arg(ipTimeZone.isEmpty() ? "" : ipTimeZone);
		location=StelLocation::createFromLine(locLine); // in lack of a regular constructor ;-)
		core->setCurrentTimeZone(ipTimeZone.isEmpty() ? "LMST" : ipTimeZone);
		core->moveObserverTo(location, 0.0f, 0.0f);
		QSettings* conf = StelApp::getInstance().getSettings();
		conf->setValue("init_location/last_location", QString("%1,%2").arg(latitude).arg(longitude));
	}
	else
	{
		qDebug() << "Failure getting IP-based location: \n\t" <<networkReply->errorString();
		// If there is a problem, this must not change to some other location!
		//core->moveObserverTo(lastResortLocation, 0.0f, 0.0f);
	}
	networkReply->deleteLater();
}

LocationMap StelLocationMgr::pickLocationsNearby(const QString planetName, const float longitude, const float latitude, const float radiusDegrees)
{
	QMap<QString, StelLocation> results;
	QMapIterator<QString, StelLocation> iter(locations);
	while (iter.hasNext())
	{
		iter.next();
		const StelLocation *loc=&iter.value();
		if ( (loc->planetName == planetName) &&
				(StelLocation::distanceDegrees(longitude, latitude, loc->longitude, loc->latitude) <= radiusDegrees) )
		{
			results.insert(iter.key(), iter.value());
		}
	}
	return results;
}

LocationMap StelLocationMgr::pickLocationsInCountry(const QString country)
{
	QMap<QString, StelLocation> results;
	QMapIterator<QString, StelLocation> iter(locations);
	while (iter.hasNext())
	{
		iter.next();
		const StelLocation *loc=&iter.value();
		if (loc->country == country)
		{
			results.insert(iter.key(), iter.value());
		}
	}
	return results;
}

// Check timezone string and return either the same or the corresponding string that we use in the Stellarium location database.
// If timezone name starts with "UTC", always return unchanged.
// This is required to store timezone names exactly as we know them, and not mix ours and corrent-iana spelling flavour.
// In practice, reverse lookup to locationDBToIANAtranslations
QString StelLocationMgr::sanitizeTimezoneStringForLocationDB(QString tzString)
{
	if (tzString.startsWith("UTC"))
		return tzString;
	QByteArray res=locationDBToIANAtranslations.key(tzString.toUtf8(), "---");
	if ( res != "---")
		return QString(res);
	return tzString;
}

// Attempt to translate a timezone name from those used in Stellarium's location database to a name which is valid
// as ckeckable by QTimeZone::availableTimeZoneIds(). That list may be updated anytime and is known to differ
// between OSes. Some spellings may be different, or in some cases some names get simply translated to "UTC+HH:MM" style.
// The empty string gets translated to "UTC".
QString StelLocationMgr::sanitizeTimezoneStringFromLocationDB(QString dbString)
{
	if (dbString.startsWith("UTC"))
		return dbString;
	// Maybe silences a debug later:
	if (dbString=="")
		return "UTC";
	QByteArray res=locationDBToIANAtranslations.value(dbString.toUtf8(), "---");
	if ( res != "---")
		return QString(res);
	return dbString;
}
