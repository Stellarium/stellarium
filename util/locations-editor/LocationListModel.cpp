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

#include <QDebug>
#include <QFile>
#include <QTextStream>

LocationListModel::LocationListModel(QObject *parent) :
    QAbstractTableModel(parent),
    wasModified(false)
{
}

LocationListModel::LocationListModel(QList<Location> locationList,
                                     QObject *parent) :
    QAbstractTableModel(parent),
    wasModified(false),
    locations(locationList)
{
}

LocationListModel* LocationListModel::load(QFile* file)
{
	LocationListModel* result = new LocationListModel();
	if (!file || !file->isReadable())
		return result;
	
	QTextStream stream(file);
	stream.setCodec("UTF-8");
	QString line;
	Location loc;
	QChar commentPrefix('#');
	int numLine = 0;
	while (!stream.atEnd())
	{
		numLine++;
		line = stream.readLine();
		
		// Skip emtpy lines and comments
		if (line.isEmpty() || line.startsWith(commentPrefix))
			continue;
		
		loc = Location::fromLine(line);
		// Check if it's a valid location
		if (loc.name.isEmpty())
		{
			qWarning() << "Line" << numLine
			           << "is not a valid location:" << line;
		}
		else
		{
			result->locations.append(loc);
		}
	}
	
	return result;
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
	
	const Location& loc = locations[index.row()];
	// Columns follow the formatting of the file.
	switch (index.column())
	{
		case 0: // Name
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.name;
			break;
			
		case 1: // Region
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.region;
			break;
			
		case 2: // Country
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.country;
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
				return loc.latitude;
			break;
			
		case 6: // Longitude
			if (role == Qt::DisplayRole || role == Qt::EditRole)
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
		// TODO: Also show tooltips/status tips
		if (role != Qt::DisplayRole)
			return QVariant();
		
		switch (section)
		{
			case 0:
				return "Name";
			case 1:
				return "Region or State";
			case 2:
				return "Country";
			case 3:
				return "Type";
			case 4:
				return "Population";
			case 5:
				return "Latitude";
			case 6:
				return "Longitude";
			case 7:
				return "Altitude";
			case 8:
				return "Light Pollution";
			case 9:
				return "Time Zone";
			case 10:
				return "Planet";
			case 11:
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
	
	Location& loc = locations[index.row()];
	switch (index.column())
	{
		case 0:
			loc.name = value.toString();
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
		
		case 5:
		{
			float latitude = value.toFloat();
			if (latitude < -90. || latitude > 90.)
				return false;
			loc.latitude = latitude;
		}
			break;
			
		case 6:
		{
			float longitude = value.toFloat();
			if (longitude < -180. || longitude > 180.)
				return false;
			loc.longitude = longitude;
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

bool LocationListModel::isValidIndex(const QModelIndex& index) const
{
	return (index.isValid() &&
	    (index.column() < 12) &&
	    (index.row() < locations.count()));
}
