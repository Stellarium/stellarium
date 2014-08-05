/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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

#include "AddOnTableModel.hpp"
#include "AddOnTableProxyModel.hpp"

AddOnTableProxyModel::AddOnTableProxyModel(QString tableName, QObject* parent)
	: QSortFilterProxyModel(parent)
{
	setSourceModel(new AddOnTableModel(tableName));
}

QModelIndex AddOnTableProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
	return index(sourceIndex.row()*2, sourceIndex.column());
}

QModelIndex AddOnTableProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
	return sourceModel()->index(proxyIndex.row()/2, proxyIndex.column());
}

QModelIndex AddOnTableProxyModel::index(int row, int column, const QModelIndex& i) const
{
	return createIndex(row, column);
}

QModelIndex AddOnTableProxyModel::parent(const QModelIndex&) const
{
	return QModelIndex();
}

int AddOnTableProxyModel::rowCount(const QModelIndex&) const
{
	return sourceModel()->rowCount() * 2;
}

int AddOnTableProxyModel::columnCount(const QModelIndex&) const
{
	return sourceModel()->columnCount();
}

QVariant AddOnTableProxyModel::data(const QModelIndex &ind, int role) const
{
	if (ind.row() % 2)
		return QVariant();

	return sourceModel()->data(mapToSource(ind), role);
}
