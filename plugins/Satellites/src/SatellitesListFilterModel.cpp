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

#include "SatellitesListFilterModel.hpp"

SatellitesListFilterModel::SatellitesListFilterModel(QObject *parent) :
	QSortFilterProxyModel(parent),
	filterFlag(SatNoFlags)
{
}

void SatellitesListFilterModel::setFilterFlag(const SatFlag& flag)
{
	filterFlag = flag;
	invalidateFilter();
}

void SatellitesListFilterModel::setFilterGroup(const QString& groupId)
{
	filterGroup = groupId;
	invalidateFilter();
}

void SatellitesListFilterModel::setSecondaryFilters(const QString& groupId,
                                                   const SatFlag& flag)
{
	filterFlag = flag;
	filterGroup = groupId;
	invalidateFilter();
}


bool SatellitesListFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
	QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
	
	// Check flag
	// TODO: find out if the NoFlags trick actually works
	QVariant data = index.data(SatFlagsRole);
	SatFlags flags = data.value<SatFlags>();
	if (!(flags & filterFlag).testFlag(filterFlag))
		return false;
	
	// Check group
	if (!filterGroup.isEmpty())
	{
		data = index.data(SatGroupsRole);
		QSet<QString> groups = data.value<GroupSet>();
		if (!groups.contains(filterGroup))
			return false;
	}
	
	// Check name
	if (index.data(Qt::DisplayRole).toString().contains(filterRegularExpression()))
		return true;
	
	// Check ID
	data = index.data(Qt::UserRole);
	QString id = data.toString();
	if (id.contains(filterRegularExpression()))
		return true;

	// search international designator
	data = index.data(SatCosparIDRole);
	id = data.toString();
	if (id.contains(filterRegularExpression()))
		return true;

	// search descriptions
	data = index.data(SatDescriptionRole);
	QString descr = data.toString();
	if (descr.contains(filterRegularExpression()))
		return true;
	
	// TODO: Possible check for "NORAD NNNN".
	return false;
}
