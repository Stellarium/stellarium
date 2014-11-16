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
#include "StelTranslator.hpp"

JsonTableModel::JsonTableModel(AddOn::Type type, QMap<qint64, AddOn*> addons, QObject* parent)
	: QAbstractTableModel(parent)
	, m_addons(addons)
{
	switch (type) {
		default:
			m_iColumns << Title << Type << Version << Checkbox;
	}
}

int JsonTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return m_addons.size() * 2;
}

int JsonTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return m_iColumns.size();
}

QVariant JsonTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() ||
		index.row() < 0 ||
		index.row() % 2 != 0 ||
		role != Qt::DisplayRole)
	{
		return QVariant();
	}

	QVariant value;
	AddOn* addon = m_addons.values().at(index.row() / 2);
	Column column = m_iColumns.at(index.column());
	switch (column) {
		case Title:
			value = addon->getTitle();
			break;
		case Type:
			value = addon->getType();
			break;
		case Version:
			value = addon->getVersion();
			break;
		case Checkbox:
			value = "";
			break;
	}
	return qVariantFromValue(value);
}

QVariant JsonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
	{
		return QVariant();
	}

	QString value;
	switch ((Column) section) {
		case Title:
			value = q_("Title");
			break;
		case Type:
			value = q_("Type");
			break;
		case Version:
			value = q_("Version");
			break;
		case Checkbox:
			value = "";
			break;
	}
	return qVariantFromValue(value);
}
