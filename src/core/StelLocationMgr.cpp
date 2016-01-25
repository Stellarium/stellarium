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

#include <QStringListModel>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QSettings>

StelLocationMgr::StelLocationMgr()
{
	QSettings* conf = StelApp::getInstance().getSettings();

	// The line below allows to re-generate the location file, you still need to gunzip it manually afterward.
	// generateBinaryLocationFile("data/base_locations.txt", false, "data/base_locations.bin");

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
	const QMap<QString, StelLocation>& cities = loadCities(fileName, isUserLocation);
	QFile binfile(binFilePath);
	if(binfile.open(QIODevice::WriteOnly))
	{
		QDataStream out(&binfile);
		out.setVersion(QDataStream::Qt_4_6);
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
		in.setVersion(QDataStream::Qt_4_6);
		in >> res;
		return res;
	}
	else
	{
		QDataStream in(&sourcefile);
		in.setVersion(QDataStream::Qt_4_6);
		in >> res;
		return res;
	}
}

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
	QNetworkRequest req( QUrl( QString("http://freegeoip.net/csv/") ) );	
	req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
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
		// Tested with and without working network connection.
		QByteArray answer=networkReply->readAll();
		qDebug() << "IP answer:" << answer;
		// answer/splitline example:     "222.222.222.222","AT","Austria","","","","","47.3333","13.3333","",""
		// The parts from freegeoip are: ip,country_code,country_name,region_code,region_name,city,zipcode,latitude,longitude,metro_code,area_code
		// Changed before 2014-11-21 to: 222.222.222.222,AT,Austria,"","","","",Europe/Vienna,47.33,13.33,0<CR><LF> (i.e., only empty strings have "")
		//                          Now: ip,country_code,country_name,region_code,region_name,city,zipcode,Timezone_name,latitude,longitude,metro_code
		// longitude and latitude should always be filled.
		// A few tests:
		if (answer.count(',') != 10 )
		{
			qDebug() << "StelLocationMgr: Malformatted answer in IP-based location lookup: \n\t" << answer;
			qDebug() << "StelLocationMgr: Will not change location.";
			networkReply->deleteLater();
			return;
		}
		const QStringList& splitline = QString(answer).split(",");
		if (splitline.count() != 11 )
		{
			qDebug() << "StelLocationMgr: Unexpected answer in IP-based location lookup: \n\t" << answer;
			qDebug() << "StelLocationMgr: Will not change location.";
			networkReply->deleteLater();
			return;
		}
		// KEEP FOR DEBUGGING:
		//for (int i=0; i<splitline.count(); ++i)
		//	qDebug() << "Component" << i << "length:" << splitline.at(i).length() << ":" << splitline.at(i);
		if ((splitline.at(8)=="\"\"") || (splitline.at(9)=="\"\"")) // empty coordinates?
		{
			qDebug() << "StelLocationMgr: Invalid coordinates from IP-based lookup. Ignoring: \n\t" << answer;
			networkReply->deleteLater();
			return;
		}
		float latitude=splitline.at(8).toFloat();
		float longitude=splitline.at(9).toFloat();
		QString locLine= // we re-pack into a new line that will be parsed back by StelLocation...
				QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\t0")
				.arg(splitline.at(5) == "\"\"" ? QString("%1, %2").arg(latitude).arg(longitude) : splitline.at(5))
				.arg(splitline.at(4) == "\"\"" ? "IPregion"  : splitline.at(4))
				.arg(splitline.at(2) == "\"\"" ? "IPcountry" : splitline.at(2)) // countryCode
				.arg("X") // role: X=user-defined
				.arg(0)   // population: unknown
				.arg(latitude<0 ? QString("%1S").arg(-latitude, 0, 'f', 6) : QString("%1N").arg(latitude, 0, 'f', 6))
				.arg(longitude<0 ? QString("%1W").arg(-longitude, 0, 'f', 6) : QString("%1E").arg(longitude, 0, 'f', 6));
		location=StelLocation::createFromLine(locLine); // in lack of a regular constructor ;-)
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
