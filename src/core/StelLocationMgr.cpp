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
#include "StelFileMgr.hpp"
#include "StelLocationMgr.hpp"
#include "StelUtils.hpp"
#include "kfilterdev.h"

#include <QStringListModel>
#include <QDebug>
#include <QFile>

StelLocationMgr::StelLocationMgr()
{
	// The line below allows to re-generate the location file, you still need to gunzip it manually afterward.
	// generateBinaryLocationFile("data/base_locations.txt", false, "data/base_locations.bin");

	locations = loadCitiesBin("data/base_locations.bin.gz");
	locations.unite(loadCities("data/user_locations.txt", true));

	modelAllLocation = new QStringListModel(this);
	modelAllLocation->setStringList(locations.keys());
	
	// Init to Paris France because it's the center of the world.
	lastResortLocation = locationForSmallString("Paris, France");
}

void StelLocationMgr::generateBinaryLocationFile(const QString& fileName, bool isUserLocation, const QString& binFilePath) const
{
	const QMap<QString, StelLocation>& cities = loadCities(fileName, isUserLocation);
	QFile binfile(binFilePath);
	binfile.open(QIODevice::WriteOnly);
	QDataStream out(&binfile);
	out.setVersion(QDataStream::Qt_4_6);
	out << cities;
	binfile.close();
}

QMap<QString, StelLocation> StelLocationMgr::loadCitiesBin(const QString& fileName) const
{
	QMap<QString, StelLocation> res;
	QString cityDataPath;
	try
	{
		cityDataPath = StelFileMgr::findFile(fileName);
	}
	catch (std::runtime_error& e)
	{
		return res;
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::ReadOnly))
	{
		qWarning() << "ERROR: Could not open location data file: " << cityDataPath;
		return res;
	}

	if (fileName.endsWith(".gz"))
	{
		QIODevice* d = KFilterDev::device(&sourcefile, "application/x-gzip", false);
		d->open(QIODevice::ReadOnly);
		QDataStream in(d);
		in.setVersion(QDataStream::Qt_4_6);
		in >> res;
		d->close();
		delete d;
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

QMap<QString, StelLocation> StelLocationMgr::loadCities(const QString& fileName, bool isUserLocation) const
{
	// Load the cities from data file
	QMap<QString, StelLocation> locations;
	QString cityDataPath;
	try
	{
		cityDataPath = StelFileMgr::findFile(fileName);
	}
	catch (std::runtime_error& e)
	{
		// Note it is quite normal to nor have a user locations file (e.g. first run)
		if (!isUserLocation)
			qWarning() << "WARNING: Failed to locate location data file: " << fileName << e.what();
		return locations;
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "ERROR: Could not open location data file: " << cityDataPath;
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

StelLocationMgr::~StelLocationMgr()
{
}

const StelLocation StelLocationMgr::locationForSmallString(const QString& s, bool* ok) const
{
	QMap<QString, StelLocation>::const_iterator iter = locations.find(s);
	if (iter==locations.end())
	{
		if (ok)
			*ok=false;
		return lastResortLocation;
	}
	else
	{
		if (ok)
			*ok = true;
		return locations.value(s);
	}
}

const StelLocation StelLocationMgr::locationForString(const QString& s, bool* ok) const
{
	bool myOk;
	StelLocation ret = locationForSmallString(s, &myOk);
	if (myOk)
	{
		if (ok)
			*ok=true;
		return ret;
	}
	// Maybe it is a coordinate set ? (e.g. GPS +41d51'00" -51d00'00" )
	QRegExp reg("(.*)([\\+\\-](?:\\d+)d(?:\\d+)'(?:\\d+)\") ([\\+\\-](?:\\d+)d(?:\\d+)'(?:\\d+)\")");
	reg.setMinimal(true);
	if (!reg.exactMatch(s))
	{
		if (ok)
			*ok=false;
		return ret;
	}
	// We have a set of coordinates
	ret.name = reg.capturedTexts()[1].trimmed();
	ret.latitude = StelUtils::dmsStrToRad(reg.capturedTexts()[2]) * 180 / M_PI;
	ret.longitude = StelUtils::dmsStrToRad(reg.capturedTexts()[3]) * 180 / M_PI;
	if (ok)
		*ok=true;
	return ret;
}

// Get whether a location can be permanently added to the list of user locations
bool StelLocationMgr::canSaveUserLocation(const StelLocation& loc) const
{
	return locations.find(loc.getID())==locations.end();
}

// Add permanently a location to the list of user locations
bool StelLocationMgr::saveUserLocation(const StelLocation& loc)
{
	if (!canSaveUserLocation(loc))
		return false;

	// Add in the program
	locations[loc.getID()]=loc;

	// Append in the Qt model
	modelAllLocation->setStringList(locations.keys());

	// Append to the user location file
	QString cityDataPath;
	try
	{
		cityDataPath = StelFileMgr::findFile("data/user_locations.txt", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::File));
	}
	catch (std::runtime_error& e)
	{
		if (!StelFileMgr::exists(StelFileMgr::getUserDir()+"/data"))
		{
			if (!StelFileMgr::mkDir(StelFileMgr::getUserDir()+"/data"))
			{
				qWarning() << "ERROR - cannot create non-existent data directory" << StelFileMgr::getUserDir()+"/data";
				qWarning() << "Location cannot be saved";
				return false;
			}
		}

		cityDataPath = StelFileMgr::getUserDir()+"/data/user_locations.txt";
		qWarning() << "Will create a new user location file: " << cityDataPath;
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
	{
		qWarning() << "ERROR: Could not open location data file: " << cityDataPath;
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
	// Remove in the Qt model file
	modelAllLocation->setStringList(locations.keys());

	// Resave the whole remaining user locations file
	QString cityDataPath;
	try
	{
		cityDataPath = StelFileMgr::findFile("data/user_locations.txt", StelFileMgr::Writable);
	}
	catch (std::runtime_error& e)
	{
		if (!StelFileMgr::exists(StelFileMgr::getUserDir()+"/data"))
		{
			if (!StelFileMgr::mkDir(StelFileMgr::getUserDir()+"/data"))
			{
				qWarning() << "ERROR - cannot create non-existent data directory" << StelFileMgr::getUserDir()+"/data";
				qWarning() << "Location cannot be saved";
				return false;
			}
		}

		cityDataPath = StelFileMgr::getUserDir()+"/data/user_locations.txt";
		qWarning() << "Will create a new user location file: " << cityDataPath;
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "ERROR: Could not open location data file: " << cityDataPath;
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
