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

#ifndef _ADDONTABLEPROXYMODEL_HPP_
#define _ADDONTABLEPROXYMODEL_HPP_

#include <QSortFilterProxyModel>

class AddOnTableProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	AddOnTableProxyModel(QString tableName, QObject* parent=0);

	QModelIndex mapFromSource(const QModelIndex & sourceIndex) const;
	QModelIndex mapToSource(const QModelIndex & proxyIndex) const;

	QModelIndex index(int row, int column, const QModelIndex& i=QModelIndex()) const;
	QModelIndex parent(const QModelIndex&) const;

	int rowCount(const QModelIndex&) const;
	int columnCount(const QModelIndex&) const;

	QVariant data(const QModelIndex &ind, int role) const;
};

#endif // _ADDONTABLEPROXYMODEL_HPP_
