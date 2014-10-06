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

#ifndef _JSONTABLEMODEL_HPP_
#define _JSONTABLEMODEL_HPP_

#include <QAbstractTableModel>

#include "addons/AddOn.hpp"

class JsonTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	JsonTableModel(QString type, QMap<qint64, AddOn*> addons, QObject *parent=0);

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex& index, int role = Qt :: DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
	enum Column {
		Title,
		Type,
		Version
	};

	QMap<qint64, AddOn*> m_addons;
	QList<Column> m_iColumns;
};

#endif // _JSONTABLEMODEL_HPP_
