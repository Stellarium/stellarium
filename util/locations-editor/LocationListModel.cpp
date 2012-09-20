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
#include <QMimeData>
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
			loc = result->addLocationId(loc);
			if (loc)
			{
				result->locations.append(loc);
				
				if (loc->hasDuplicate)
				{
					log << lineNum << ": duplicate ID (\"" << loc->stelId
					    << "\") with line " << result->lastDupLine << ": "
					    << line << endl;
				}
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
		stream << i.key();
		i.value()->toBinary(stream);
	}
	return true;
}


int LocationListModel::findNextDuplicateRow(int startRow, bool wrapAround) const
{
	int count = locations.count();
	if (startRow < 0 || startRow >= count)
		return -1;
	
	Location* loc = locations[startRow];
	if (!loc->hasDuplicate)
		return -1;
	
	for (int row = startRow + 1; row < count; row++)
	{
		if (locations[row]->stelId == loc->stelId)
			return row;
		
		if (wrapAround && (row == (count - 1)))
		{
			row = -1;
			count = startRow;
			wrapAround = false;
		}
	}
	
	return -1;
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
			if (role == (Qt::UserRole + 1))
				return loc.latitude;
			break;
			
		case 6: // Longitude
			if (role == Qt::DisplayRole || role == Qt::EditRole)
				return loc.longitudeStr;
			if (role == Qt::ToolTipRole)
				return loc.longitude;
			if (role == (Qt::UserRole + 1))
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

bool LocationListModel::dropMimeData(const QMimeData* data,
                                     Qt::DropAction action,
                                     int row, int column,
                                     const QModelIndex &parent)
{
	Q_UNUSED(column);
	
	if (action == Qt::IgnoreAction)
		return true;
	
	// Rows can be dropped only between other rows, not on to of items.
	if (parent.isValid())
		return false;
	
	// For now, it can accept only the internal format.
	if (!data->hasFormat("application/x-stellarium-location-list"))
		return false;
	
	QByteArray binData = data->data("application/x-stellarium-location-list");
	QDataStream dataStream(&binData, QIODevice::ReadOnly);
	QList<Location> insertedLocations;
	dataStream >> insertedLocations;
	
	int beginRow = 0;
	if (row > 0)
		beginRow = row;
	int count = insertedLocations.count();
	beginInsertRows(QModelIndex(), beginRow, beginRow + count - 1);
	foreach (const Location& loc, insertedLocations)
	{
		Location* newLocation = new Location(loc);
		locations.insert(row++, newLocation);
		addLocationId(newLocation);
	}
	endInsertRows();
	return true;
}

Qt::ItemFlags LocationListModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index) |
	                      Qt::ItemIsEnabled;
	
	if (!index.isValid())
		return flags | Qt::ItemIsDropEnabled;

	return flags | Qt::ItemIsEditable | Qt::ItemIsDragEnabled;
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

bool LocationListModel::insertRows(int row,
                                   int count,
                                   const QModelIndex& parent)
{
	Q_UNUSED(parent);
	if (row < 0 || row >= locations.count())
		return false;
	int endRow = row + count - 1;
	
	beginInsertRows(QModelIndex(), row, endRow);
	for (int i = row; i <= endRow; i++)
		locations.insert(i, new Location());
	endInsertRows();
	
	return true;
}

QMimeData* LocationListModel::mimeData(const QModelIndexList& indexes) const
{
	QSet<int> rows;
	foreach (const QModelIndex& index, indexes)
	{
		if (index.row() >= 0)
			rows.insert(index.row());
	}
	
	QByteArray textData;
	QTextStream textStream(&textData);
	textStream.setCodec("UTF-8");
	QList<Location> exportedLocations;
	
	QList<int> sortedRows = rows.toList();
	foreach (int row, sortedRows)
	{
		Location* loc = locations[row];
		textStream << loc->toLine() << endl;
		exportedLocations.append(*loc);
	}
	
	QByteArray binaryData;
	QDataStream dataStream(&binaryData, QIODevice::WriteOnly);
	dataStream << exportedLocations;
	
	QMimeData* mimeData = new QMimeData();
	mimeData->setData("application/x-stellarium-location-list", binaryData);
	mimeData->setData("text/tab-separated-values;charset=UTF-8", textData);
	mimeData->setData("text/plain;charset=UTF-8", textData);
	
	return mimeData;
}

QStringList LocationListModel::mimeTypes() const
{
	QStringList types;
	types << "text/plain;charset=UTF-8";
	types << "text/tab-separated-values;charset=UTF-8";
	types << "application/x-stellarium-location-list";
	return types;
}

bool LocationListModel::removeRows(int row,
                                   int count,
                                   const QModelIndex& parent)
{
	Q_UNUSED(parent);
	
	if (row >= locations.count() || row + count <= 0)
		return false;
	//int endRow = qMax(row + count - 1, locations.count() - 1);;
	
	// As this is used only by drag-moving, there's no need for other actions.
	//beginRemoveRows(QModelIndex(), row, endRow);
	for (int i = 0; i < count; i++)
		removeLocation(row);
	//endRemoveRows();
	
	return true;
}

bool LocationListModel::setData(const QModelIndex& index,
                                const QVariant& value,
                                int role)
{
	if (!isValidIndex(index) || role != Qt::EditRole)
		return false;
	
	Location* loc = locations[index.row()];
	switch (index.column())
	{
		case 0:
		{
			QString name = value.toString();
			if (name.isEmpty())
				return false;
			if (name != loc->name)
			{
				loc->name = loc->stelName = name;
				updateDuplicates(loc);
				loc->hasDuplicate = false;
				addLocationId(loc);
			}
		}
			break;
			
		case 1:
		{
			QString region = value.toString();
			if (region != loc->region)
			{
				loc->region = region;
				loc->stelName = loc->name;
				updateDuplicates(loc);
				loc->hasDuplicate = false;
				addLocationId(loc);
			}
		}
			break;
			
		case 2:
		{
			QString country = value.toString();
			if (country != loc->country)
			{
				loc->country = country;
				loc->countryName = Location::stringToCountry(country);
				loc->stelName = loc->name;
				updateDuplicates(loc);
				loc->hasDuplicate = false;
				addLocationId(loc);
			}
		}
			break;
			
		case 3:
		{
			QChar role = value.toChar(); // TODO: Validate
			loc->role = (role != '\0') ? role : QChar('N');// TODO: Decide default role character
		}
			break;
			
		case 4:
		{
			int population = value.toInt();
			if (population < 0)
				return false;
			loc->population = population;
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
			loc->latitude = latitude;
			loc->latitudeStr = latStr;
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
			loc->longitude = longitude;
			loc->longitudeStr = longStr;
		}
			break;
			
		case 7:
			loc->altitude = value.toInt();
			break;
			
		case 8:
		{
			float lightPollution = value.toFloat();
			if (lightPollution < -1. || lightPollution > 9.0)
				return false;
			// TODO: This may be a problem. I hate floating point operations...
			if (lightPollution < 0. && lightPollution != -1.)
				return false;
			loc->bortleScaleIndex = lightPollution;
		}
			break;
			
		case 9:
			loc->timeZone = value.toString();
			break;
			
		case 10:
			loc->planetName = value.toString();
			break;
			
		case 11:
			loc->landscapeKey = value.toString();
			break;
			
		default:
			;
	}
	emit dataChanged(index, index);
	setModified(true);
	return true;
}

/*
bool LocationListModel::setItemData(const QModelIndex &index,
                                    const QMap<int, QVariant> &roles)
{
	if (!isValidIndex(index))
		return false;
	
	Location* loc = locations[index.row()];
	QVariant value = roles[Qt::EditRole];
	qDebug() << index << index.parent() << value;
	int column = index.column();
	switch (column)
	{
		case 0:
			loc->name = loc->stelName = value.toString();
			break;
		case 1:
			loc->region = value.toString();
			loc->stelName = loc->name;
			break;
		case 2:
		{
			QString country = value.toString();
			loc->country = country;
			loc->countryName = Location::stringToCountry(country);
			loc->stelName = loc->name;
		}
			break;
		case 3:
			loc->role = value.toChar();
			break;
		case 4:
			loc->population = value.toString();
			break;
		case 5:
			loc->latitudeStr = value.toString();
			loc->latitude = roles[Qt::UserRole + 1].toFloat();
			break;
		case 6:
			loc->longitudeStr = value.toString();
			loc->longitude = roles[Qt::UserRole + 1].toFloat();
			break;
		case 7:
			loc->altitude = value.toInt();
			break;
		case 8:
			loc->bortleScaleIndex = value.toFloat();
			break;
		case 9:
			loc->timeZone = value.toString();
			break;
		case 10:
			loc->planetName = value.toString();
			break;
		case 11:
			loc->landscapeKey = value.toString();
			break;
		default:
			;
	}
	
	if (column >= 0 && column <= 2)
	{
		updateDuplicates(loc);
		loc->hasDuplicate = false;
		addLocationId(loc);
	}
	
	emit dataChanged(index, index);
	setModified(true);
	return true;
}
*/

Qt::DropActions LocationListModel::supportedDropActions() const
{
	return Qt::MoveAction;
}


void LocationListModel::insertLocation(int row, Location *loc)
{
	if (!loc)
		return;
	
	if (row < 0 || row > locations.count())
		return;
	// (Works even if locations is empty, i.e. count() == 0.)
	
	beginInsertRows(QModelIndex(), row, row);
	
	locations.insert(row, loc);
	addLocationId(loc);
	
	endInsertRows();
	setModified(true);
}

void LocationListModel::cloneLocation(int row, bool atEnd)
{
	if (row < 0 || row >= locations.count())
		return;
	
	int newRow = atEnd ? locations.count() : (row + 1);
	insertLocation(newRow, new Location(*locations.value(row)));
}

void LocationListModel::removeLocation(int row)
{
	if (row < 0 || row >= locations.count())
		return;
	
	beginRemoveRows(QModelIndex(), row, row);
	
	Location* loc = locations.takeAt(row);
	updateDuplicates(loc);
	delete loc;
	
	endRemoveRows();
	setModified(true);
}

void LocationListModel::setModified(bool changed)
{
	if (wasModified == changed)
		return;
	wasModified = changed;
	emit modified(changed);
}

Location* LocationListModel::addLocationId(Location* loc, bool skipDuplicates)
{
	QString id = loc->generateId();
	Location* dupLoc = stelIds.take(id); // In multi-map, returns the most recent
	if (dupLoc)
	{
		//qDebug() << "Duplicate found for ID" << id;
		loc->extendId();
		dupLoc->extendId();
		//qDebug() << "Extending ID to" << loc->stelId << dupLoc->stelId;
		if (loc->stelId == dupLoc->stelId)
		{
			//qDebug() << "Extended IDs match.";
			lastDupLine = dupLoc->lineNum;
			
			//Instead of deleting, mark both as duplicates.
			loc->hasDuplicate = true;
			dupLoc->hasDuplicate = true;
		}
		else
		{
			// There may be already another one...
			Location* extDupLoc = stelIds.value(loc->stelId, 0);
			if (extDupLoc)
			{
				lastDupLine = extDupLoc->lineNum;
				loc->hasDuplicate = true;
				extDupLoc->hasDuplicate = true;
			}
		}
		if (skipDuplicates && loc->hasDuplicate)
		{
			delete loc;
			loc = 0;
			// Restore the original ID of the original location,
			// because the map key wasn't changed.
			// Note: this is not (yet?) done in Stellarium.
			dupLoc->stelName = dupLoc->name;
			dupLoc->generateId();
		}
		stelIds.insertMulti(dupLoc->stelId, dupLoc);
	}
	
	if (loc)
		stelIds.insertMulti(loc->stelId, loc);
	
	return loc;
}

void LocationListModel::updateDuplicates(Location* loc)
{
	QString id = loc->stelId;
	QMap<QString, Location*>::iterator i = stelIds.lowerBound(id);
	QMap<QString, Location*>::iterator upperBound = stelIds.upperBound(id);
	QList<Location*> duplicates;
	while (i != upperBound)
	{
		if (i.value() != loc)
			duplicates.append(i.value());
			
		i = stelIds.erase(i);
	}
	
	foreach (Location* duplicate, duplicates)
	{
		duplicate->hasDuplicate = false;
		duplicate->stelName = duplicate->name;
		addLocationId(duplicate);
	}
	return;
}

bool LocationListModel::isValidIndex(const QModelIndex& index) const
{
	return (index.isValid() &&
	    (index.column() < 12) &&
	    (index.row() < locations.count()));
}
