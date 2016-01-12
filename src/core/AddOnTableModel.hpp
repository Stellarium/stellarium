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

#ifndef _ADDONTABLEMODEL_HPP_
#define _ADDONTABLEMODEL_HPP_

#include <QAbstractTableModel>

#include "AddOn.hpp"
#include "AddOnDialog.hpp"
#include "StelAddOnMgr.hpp"

class AddOnTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	AddOnTableModel(QHash<QString, AddOn*> addons, QObject *parent=0);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role = Qt :: DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	AddOn* getAddOn(int row) { return m_addons.values().at(row/2); }

private:
	enum Column {
		Title,
		Type,
		Version,
		Status,
		Checkbox
	};

	QHash<QString, AddOn*> m_addons;
	QList<Column> m_iColumns;
};

#endif // _ADDONTABLEMODEL_HPP_
