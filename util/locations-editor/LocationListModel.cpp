/*
 * Stellarium Location List Editor
 * Copyright (C) 2008  Fabien Chereau
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

#include "LocationListModel.hpp"

#include <QBrush>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QTextStream>

LocationListModel::LocationListModel(QObject *parent) :
    QAbstractTableModel(parent),
    wasModified(false),
    lastDupLine(0)
{
}

LocationListModel::~LocationListModel()
{
	qDeleteAll(locations);
	locations.clear();
	stelIds.clear();
}

LocationListModel* LocationListModel::load(QFile* file)
{
	LocationListModel* result = new LocationListModel();
	if (!file || !file->isReadable())
		return result;
	
	result->loadingLog.clear();
	QTextStream log(&result->loadingLog);
	log.setCodec("UTF-8");
	
	QTextStream stream(file);
	stream.setCodec("UTF-8");
	
	bool inLeadingBlock = true;
	result->leadingComments.clear();
	result->comments.clear();
	
	QString line;
	Location* loc;
	QChar commentPrefix('#');
	int lineNum = 0;
	while (!stream.atEnd())
	{
		lineNum++;
		line = stream.readLine();
		
		// Skip emtpy lines
		if (line.isEmpty())
			continue;
		
		// Aggregate comments
		if (line.startsWith(commentPrefix))
		{
			if (inLeadingBlock)
				result->leadingComments.append(line);
			else
				result->comments.append(line);
			continue;
		}
		else
			inLeadingBlock = false;
		
		loc = Location::fromLine(line);
		// Check if it's a valid location
		if (!loc)
		{
			log << lineNum << ": not a valid location: " << line << endl;
			continue;
		}
		else
		{
			loc->lineNum = lineNum;
			
			// Check for duplicate IDs and avoid them if possible.
			loc = result->addLocation(loc);
			if (loc && loc->hasDuplicate)
			{
				log << lineNum << ": duplicate ID (\"" << loc->stelId
				    << "\") with line " << result->lastDupLine << ": "
				    << line << endl;
			}
		}
	}
	return result;
}

bool LocationListModel::save(QFile* file)
{
	if (!file || !file->isWritable())
		return false;
	
	QTextStream stream(file);
	stream.setCodec("UTF-8");
	foreach(const QString& comment, leadingComments)
		stream << comment << endl;
	foreach(const Location* loc, locations)
	{
		stream << loc->toLine() << '\n';
	}
	foreach(const QString& comment, comments)
		stream << comment << endl;
	
	return true;
}

bool LocationListModel::saveBinary(QIODevice* file)
{
	if (!file || !file->isWritable())
		return false;
	
	QDataStream stream(file);
	stream.setVersion(QDataStream::Qt_4_6);
	stream << ((quint32)stelIds.uniqueKeys().count());
	QMapIterator<QString,Location*> i(stelIds);
	while (i.hasNext())
	{
		i.next();
		
		// If there are multiple values with one key, skip them.
		if (i.hasNext() && (i.key() == i.peekNext().key()))
		{
			QString key = i.key();
			// qDebug() << "Trying to skip key:" << key;
			while (i.hasNext() && (key == i.peekNext().key()))
			{
				i.next();
				// qDebug() << "Skipping" << i.key();
			}
		}
		
		// In case of a multi-map, save the last added key?
		stream << i.key() << *(i.value());
	}
	return true;
}

int LocationListModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return 12;
}

int LocationListModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return locations.count();
}

QVariant LocationListModel::data(const QModelIndex& index, int role) const
{
	if (!isValidIndex(index))
		return QVariant();
	
	const Location& loc = *locations[index.row()];
	
	if (role == Qt::BackgroundRole && loc.hasDuplicate)
	{
		return QBrush(Qt::red, Qt::SolidPattern);
	}
	
	// Columns follow the formatting of the file.
	switch (index.column())
	{
		case 0: // Name
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.name;
			if (role == Qt::ToolTipRole)
				return loc.stelId;
			break;
			
		case 1: // Region
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.region;
			break;
			
		case 2: // Country
			if (role == Qt::DisplayRole)
				return loc.countryName;
			else if (role == Qt::EditRole)
				return loc.country;
			else if (role == Qt::FontRole)
			{
				// Mark non-coded countries
				if (loc.country == loc.countryName)
				{
					QFont font;
					font.setBold(true);
					return font;
				}
				else
					return QVariant();
			}
			break;
			
		case 3: // Type
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.role;
			break;
			
		case 4: // Population
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.population;
			break;
			
		case 5: // Latitude
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.latitudeStr;
			if (role == Qt::ToolTipRole)
				return loc.latitude;
			break;
			
		case 6: // Longitude
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.longitudeStr;
			if (role == Qt::ToolTipRole)
				return loc.longitude;
			break;
			
		case 7: // Altitude
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.altitude;
			break;
			
		case 8: // Light pollution
			if (role == Qt::DisplayRole)
			{
				if (loc.bortleScaleIndex < 0.)
					return QVariant();
				else
					return loc.bortleScaleIndex;
			}
			else if (role == Qt::EditRole)
			{
				return loc.bortleScaleIndex;
			}
			
		case 9: // Time zone
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.timeZone;
			break;
			
		case 10: // Planet
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.planetName;
			break;
			
		case 11: // Landscape
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.landscapeKey;
			break;
			
		default:
			break;
	}
	return QVariant();
}

Qt::ItemFlags LocationListModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::ItemIsEnabled;
	
	return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

QVariant LocationListModel::headerData(int section,
                                       Qt::Orientation orientation,
                                       int role) const
{
	if (orientation == Qt::Horizontal)
	{
		switch (section)
		{
			case 0:
				if (role == Qt::DisplayRole)
					return "Name";
				if (role == Qt::ToolTipRole)
					return "Tool tip is StelLocationMgr ID";
			case 1:
				if (role == Qt::DisplayRole)
					return "Region";
				if (role == Qt::ToolTipRole)
					return "Region (state, municipality, etc.). Used only for disambiguation. Can be empty.";
			case 2:
				if (role == Qt::DisplayRole)
					return "Country";
				if (role == Qt::ToolTipRole)
					return "In <b>bold</b> are countries that are not entered as codes.";
			case 3:
				if (role == Qt::DisplayRole)
					return "Type";
				if (role == Qt::ToolTipRole)
					return "Location type:<ul>"
					        "<li><b>C</b> or <b>B</b> for a capital city"
					        "<li><b>R</b> for a regional captial"
					        "<li><b>N</b> for a normal city (any settlement)"
					        "<li><b>O</b> for an observatory"
					        "<li><b>L</b> for a spacecraft lander"
					        "<li><b>I</b> for a spacecraft impact"
					        "<li><b>A</b> for a spacecraft crash"
					        "<li><b>X</b> for unknown/user defined [DEFAULT]"
					        "</ul>";
			case 4:
				if (role == Qt::DisplayRole)
					return "Population";
				if (role == Qt::ToolTipRole)
					return "In thousands of people.";
			case 5:
				if (role == Qt::DisplayRole)
					return "Latitude";
				if (role == Qt::ToolTipRole)
					return "Can accept values ending in N/S or signed values";
			case 6:
				if (role == Qt::DisplayRole)
					return "Longitude";
				if (role == Qt::ToolTipRole)
					return "Can accept values ending in E/W or signed values";
			case 7:
				if (role == Qt::DisplayRole)
					return "Altitude";
			case 8:
				if (role == Qt::DisplayRole)
					return "Light Pollution";
			case 9:
				if (role == Qt::DisplayRole)
					return "Time Zone";
			case 10:
				if (role == Qt::DisplayRole)
					return "Planet";
			case 11:
				if (role == Qt::DisplayRole)
					return "Landscape";
			default:
				return QVariant();
		}
	}
	else // Vertical header is line numbers
	{
		if (role == Qt::DisplayRole)
			return (section + 1);
	}
	return QVariant();
}

bool LocationListModel::setData(const QModelIndex& index,
                                const QVariant& value,
                                int role)
{
	if (!isValidIndex(index) || role != Qt::EditRole)
		return false;
	
	Location& loc = *locations[index.row()];
	switch (index.column())
	{
		case 0:
		{
			QString name = value.toString();
			if (name != loc.name)
			{
				//
				loc.name = name;
			}
		}
			break;
			
		case 1:
			loc.region = value.toString();
			break;
			
		case 2:
			loc.country = value.toString(); // TODO: Country code?
			break;
			
		case 3:
		{
			QChar role = value.toChar(); // TODO: Validate
			loc.role = (role != '\0') ? role : QChar('N');// TODO: Decide default role character
		}
			break;
			
		case 4:
		{
			int population = value.toInt();
			if (population < 0)
				return false;
			loc.population = population;
		}
			break;
		
		case 5: // Latitude
		{
			QString latStr = value.toString();
			bool ok = false;
			float latitude = value.toFloat(&ok);
			if (ok)
			{
				latStr = Location::latitudeToString(latitude);
			}
			else
			{
				latitude = Location::latitudeFromString(latStr, &ok);
				if (!ok)
					return false;
			}
			if (latitude < -90. || latitude > 90.)
				return false;
			loc.latitude = latitude;
			loc.latitudeStr = latStr;
		}
			break;
			
		case 6:
		{
			QString longStr = value.toString();
			bool ok = false;
			float longitude = value.toFloat(&ok);
			if (ok)
			{
				longStr = Location::longitudeToString(longitude);
			}
			else
			{
				longitude = Location::longitudeFromString(longStr, &ok);
				if (!ok)
					return false;
			}
			if (longitude < -180. || longitude > 180.)
				return false;
			loc.longitude = longitude;
			loc.longitudeStr = longStr;
		}
			break;
			
		case 7:
			loc.altitude = value.toInt();
			break;
			
		case 8:
		{
			float lightPollution = value.toFloat();
			if (lightPollution < -1. || lightPollution > 9.0)
				return false;
			// TODO: This may be a problem. I hate floating point operations...
			if (lightPollution < 0. && lightPollution != -1.)
				return false;
			loc.bortleScaleIndex = lightPollution;
		}
			break;
			
		case 9:
			loc.timeZone = value.toString();
			break;
			
		case 10:
			loc.planetName = value.toString();
			break;
			
		case 11:
			loc.landscapeKey = value.toString();
			break;
			
		default:
			;
	}
	emit dataChanged(index, index);
	setModified(true);
	return true;
}

void LocationListModel::setModified(bool changed)
{
	if (wasModified == changed)
		return;
	wasModified = changed;
	emit modified(changed);
}

Location* LocationListModel::addLocation(Location* loc, bool skipDuplicates)
{
	QString id = loc->generateId();
	Location* dupLoc = stelIds.value(id, 0); // In multi-map, returns the most recent
	if (dupLoc)
	{
		loc->extendId();
		dupLoc->extendId();
		if (loc->stelId == dupLoc->stelId)
		{
			lastDupLine = dupLoc->lineNum;
			
			if (skipDuplicates)
			{
				delete loc;
				loc = 0;
				// Restore the original ID of the original location,
				// because the map key wasn't changed.
				// Note: this is not (yet?) done in Stellarium.
				dupLoc->stelName = dupLoc->name;
				dupLoc->generateId();
				// Skip adding the new location
				return 0;
			}
			
			//Instead of deleting, mark both as duplicates.
			loc->hasDuplicate = true;
			dupLoc->hasDuplicate = true;
		}
		else
		{
			stelIds.remove(id); //In mult-maps, removes the most recent
			stelIds.insertMulti(dupLoc->stelId, dupLoc);
			// The new location will be added after the block
		}
	}
	locations.append(loc);
	stelIds.insertMulti(loc->stelId, loc);
	
	return loc;
}

bool LocationListModel::isValidIndex(const QModelIndex& index) const
{
	return (index.isValid() &&
	    (index.column() < 12) &&
	    (index.row() < locations.count()));
}
