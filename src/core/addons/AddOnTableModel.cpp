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

#include <QDateTime>
#include <QtDebug>
#include <QStringBuilder>

#include "AddOnTableModel.hpp"
#include "StelAddOnDAO.hpp"
#include "StelAddOnMgr.hpp"
#include "StelTranslator.hpp"

AddOnTableModel::AddOnTableModel(QString tableName)
{
	QStringList extraColumns;
	if (tableName == TABLE_CATALOG)
	{
		extraColumns << COLUMN_TYPE << COLUMN_VERSION;
	}
	else if (tableName == TABLE_LANGUAGE_PACK)
	{
		extraColumns << COLUMN_TYPE << COLUMN_LAST_UPDATE;
	}
	else if (tableName == TABLE_SKY_CULTURE)
	{
		extraColumns << COLUMN_LAST_UPDATE;
	}
	else
	{
		extraColumns << COLUMN_VERSION;
	}

	setQuery(getQuery(tableName, extraColumns));
	insertColumn(columnCount()); // checkbox
	initHeaderData();
}

QString AddOnTableModel::getQuery(QString table, QStringList extraColumns)
{
	QString query = "SELECT " % table % ".id, addon.id AS addon, first_stel, "
			"last_stel, title," % extraColumns.join(",") %", installed "
			"FROM addon INNER JOIN " % table %
			" ON addon.id = " % table % ".addon";
	return query;
}

void AddOnTableModel::initHeaderData()
{
	m_columnNameTranslated.insert(q_("Title"), COLUMN_TITLE);
	m_columnNameTranslated.insert(q_("Type"), COLUMN_TYPE);
	m_columnNameTranslated.insert(q_("Last Version"), COLUMN_VERSION);
	m_columnNameTranslated.insert(q_("Last Update"), COLUMN_LAST_UPDATE);
	m_columnNameTranslated.insert(q_("Installed"), COLUMN_INSTALLED);

	QHashIterator<QString, QString> i(m_columnNameTranslated);
	while (i.hasNext())
	{
		i.next();
		setHeaderData(findColumn(i.value()), Qt::Horizontal, i.key());
	}

	setHeaderData(columnCount()-1, Qt::Horizontal, ""); //checkbox
}

QVariant AddOnTableModel::data(const QModelIndex& index, int role) const
{
	if (role == Qt::TextAlignmentRole)
	{
	  return Qt::AlignCenter;
	}

	if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole))
	{
		return QVariant();
	}

	QVariant value = QSqlQueryModel::data(index, role);
	QString column = m_columnNameTranslated.value(headerData(
				index.column(), Qt::Horizontal).toString());

	if (column == COLUMN_LAST_UPDATE)
	{
		value = qVariantFromValue(QDateTime::fromMSecsSinceEpoch(value.Int*1000));
	}
	else if (column == COLUMN_INSTALLED && role == Qt::DisplayRole)
	{
		switch (value.toInt())
		{
			case StelAddOnMgr::PartiallyInstalled:
				value = qVariantFromValue(q_("Partially"));
				break;
			case StelAddOnMgr::FullyInstalled:
				value = qVariantFromValue(q_("Yes"));
				break;
			case StelAddOnMgr::Installing:
				value = qVariantFromValue(q_("Installing"));
				break;
			case StelAddOnMgr::Corrupted:
				value = qVariantFromValue(q_("Corrupted"));
				break;
			case StelAddOnMgr::InvalidFormat:
				value = qVariantFromValue(q_("Invalid format"));
				break;
			case StelAddOnMgr::UnableToWrite:
				value = qVariantFromValue(q_("Unable to write"));
				break;
			case StelAddOnMgr::UnableToRead:
				value = qVariantFromValue(q_("Unable to read"));
				break;
			case StelAddOnMgr::UnableToRemove:
				value = qVariantFromValue(q_("Unable to remove"));
				break;
			case StelAddOnMgr::PartiallyRemoved:
				value = qVariantFromValue(q_("Partially removed"));
				break;
			case StelAddOnMgr::DownloadFailed:
				value = qVariantFromValue(q_("Download failed"));
				break;
			default:
				value = qVariantFromValue(q_("No"));
		}
	}

	return value;
}

QModelIndex AddOnTableModel::findIndex(int row, const QString& columnName)
{
	return index(row, findColumn(columnName));
}

int AddOnTableModel::findColumn(const QString& columnName)
{
	QString headerName, rawHeaderName;
	for (int column = 0; column < columnCount(); column++)
	{
		headerName = headerData(column, Qt::Horizontal).toString();
		rawHeaderName = m_columnNameTranslated.value(headerName, "");
		headerName = rawHeaderName.isEmpty() ? headerName : rawHeaderName;
		if (columnName == headerName)
		{
			return column;
		}
	}
	return -1;
}
