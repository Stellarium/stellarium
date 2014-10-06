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

#include <QtDebug>
#include <QStringBuilder>

#include "JsonTableModel.hpp"

JsonTableModel::JsonTableModel(QMap<qint64, AddOn*> addons, QObject* parent)
	: QAbstractTableModel(parent)
	, m_addons(addons)
{
}

int JsonTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return m_addons.size();
}

int JsonTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 2;
}

QVariant JsonTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= m_addons.size() || index.row() < 0)
	{
		return QVariant();
	}

	if (role == Qt::DisplayRole)
	{
		AddOn* addon = m_addons.values().at(index.row());
		if (index.column() == 0)
			return addon->getAddOnId();
		else if (index.column() == 1)
			return addon->getTitle();
	}

	return QVariant();
}

QVariant JsonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
	{
		return QVariant();
	}

	switch (section)
	{
		case 0:
			return tr("ID");

		case 1:
			return tr("Title");

		default:
			return QVariant();
	}

	return QVariant();
}
