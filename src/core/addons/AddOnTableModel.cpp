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

#include "AddOnTableModel.hpp"
#include "StelAddOnDAO.hpp"
#include "StelTranslator.hpp"

AddOnTableModel::AddOnTableModel(QString tableName)
{
	QString query;
	if (tableName == TABLE_CATALOG)
	{
		query = "SELECT " % tableName % ".id, addon.id, "
			"title, type, version, installed, NULL "
			"FROM addon INNER JOIN " % tableName %
			" ON addon.id = " % tableName % ".addon";
		setQuery(query);
		setHeaderData(0, Qt::Horizontal, q_("Id"));
		setHeaderData(1, Qt::Horizontal, q_("AddOnId"));
		setHeaderData(2, Qt::Horizontal, q_("Title"));
		setHeaderData(3, Qt::Horizontal, q_("Type"));
		setHeaderData(4, Qt::Horizontal, q_("Last Version"));
		setHeaderData(5, Qt::Horizontal, q_("Installed"));
		setHeaderData(6, Qt::Horizontal, "");
	}
	else if (tableName == TABLE_LANGUAGE_PACK)
	{
		query = "SELECT " % tableName % ".id, addon.id, "
			"title, type, last_update, installed, NULL "
			"FROM addon INNER JOIN " % tableName %
			" ON addon.id = " % tableName % ".addon";
		setQuery(query);
		setHeaderData(0, Qt::Horizontal, q_("Id"));
		setHeaderData(1, Qt::Horizontal, q_("AddOnId"));
		setHeaderData(2, Qt::Horizontal, q_("Title"));
		setHeaderData(3, Qt::Horizontal, q_("Type"));
		setHeaderData(4, Qt::Horizontal, q_("Last Update"));
		setHeaderData(5, Qt::Horizontal, q_("Installed"));
		setHeaderData(6, Qt::Horizontal, "");
	}
	else if (tableName == TABLE_SKY_CULTURE)
	{
		query = "SELECT " % tableName % ".id, addon.id, "
			"title, last_update, installed, NULL "
			"FROM addon INNER JOIN " % tableName %
			" ON addon.id = " % tableName % ".addon";
		setQuery(query);
		setHeaderData(0, Qt::Horizontal, q_("Id"));
		setHeaderData(1, Qt::Horizontal, q_("AddOnId"));
		setHeaderData(2, Qt::Horizontal, q_("Title"));
		setHeaderData(3, Qt::Horizontal, q_("Last Update"));
		setHeaderData(4, Qt::Horizontal, q_("Installed"));
		setHeaderData(5, Qt::Horizontal, "");
	}
	else
	{
		query = "SELECT " % tableName % ".id, addon.id, "
			"title, version, installed, NULL "
			"FROM addon INNER JOIN " % tableName %
			" ON addon.id = " % tableName % ".addon";
		setQuery(query);
		setHeaderData(0, Qt::Horizontal, q_("Id"));
		setHeaderData(1, Qt::Horizontal, q_("AddOnId"));
		setHeaderData(2, Qt::Horizontal, q_("Title"));
		setHeaderData(3, Qt::Horizontal, q_("Last Version"));
		setHeaderData(4, Qt::Horizontal, q_("Installed"));
		setHeaderData(5, Qt::Horizontal, "");
	}
}

QVariant AddOnTableModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || role != Qt::DisplayRole)
	{
		return QVariant();
	}

	QVariant value = QSqlQueryModel::data(index, role);

	for (int i=0; i<columnCount(); i++)
	{
		qDebug() << headerData(i, Qt::Horizontal);
	}

	return value;
}
