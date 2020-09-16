/*
 * Copyright (C) 2013 Bogdan Marinov
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "SatellitesListModel.hpp"

#include <QBrush>
#include <QColor>

SatellitesListModel::SatellitesListModel(QList<SatelliteP>* satellites,
                                         QObject* parent) :
    QAbstractTableModel(parent),
    satelliteList(Q_NULLPTR),
    coloredNames(true)
{
	Q_ASSERT(satellites);
	
	satelliteList = satellites;
}

void SatellitesListModel::setSatelliteList(QList<SatelliteP>* satellites)
{
	if (!satellites)
		return;
	
	beginResetModel();
	satelliteList = satellites;
	endResetModel();
}

Qt::ItemFlags SatellitesListModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
	         return Qt::ItemIsEnabled;
	
	return QAbstractItemModel::flags(index);
	//TODO: If you decide to make it a table, boolean columns must be checkable.
}

QVariant SatellitesListModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || index.row() < 0 ||
	        index.row() >= satelliteList->size())
		return QVariant();
	
	SatelliteP sat = satelliteList->at(index.row());
	switch (role)
	{
		case Qt::DisplayRole:
			return (sat->name);			
		case Qt::ForegroundRole:
			if (coloredNames)
			{
				return QBrush(sat->hintColor.toQColor());
			}
			else
				break;			
		case Qt::UserRole:
			return (sat->id);			
		case SatDescriptionRole:
			return (sat->description);
		case SatStdMagnitudeRole:
			return (sat->stdMag);
		case SatRCSRole:
			return (sat->RCS);
		case SatFlagsRole:
			return (QVariant::fromValue<SatFlags>(sat->getFlags()));			
		case SatGroupsRole:
			return (QVariant::fromValue<GroupSet>(sat->groups));			
		case FirstLineRole:
			return (sat->tleElements.first);			
		case SecondLineRole:
			return (sat->tleElements.second);			
		default:
			break;
	}
	return QVariant();
}

bool SatellitesListModel::setData(const QModelIndex& index,
                                  const QVariant& value,
                                  int role)
{
	if (!index.isValid() || index.row() < 0 ||
	        index.row() >= satelliteList->size())
		return false;
	
	SatelliteP sat = satelliteList->at(index.row());
	switch (role)
	{
		case SatGroupsRole:
			sat->groups = value.value<GroupSet>();
			return true;
			
		case SatFlagsRole:
			sat->setFlags(value.value<SatFlags>());
			return true;
			
		default:
			return false;
	}
}

int SatellitesListModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return satelliteList->count();
}

int SatellitesListModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return 1;
	//TODO: For now...
}

void SatellitesListModel::beginSatellitesChange()
{
	beginResetModel();
}

void SatellitesListModel::endSatellitesChange()
{
	endResetModel();
}

void SatellitesListModel::enableColoredNames(bool enable)
{
	coloredNames = enable;
}
