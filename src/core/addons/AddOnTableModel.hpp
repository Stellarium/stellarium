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

#include <QSqlTableModel>

#include "StelTranslator.hpp"

class AddOnTableModel : public QSqlQueryModel
{
	Q_OBJECT
public:
	AddOnTableModel(QString tableName);

	QVariant data(const QModelIndex& index, int role = Qt :: DisplayRole) const;

private:
	const QString COLUMN_ID = q_("Id");
	const QString COLUMN_ADDONID = q_("AddOnId");
	const QString COLUMN_TITLE = q_("Title");
	const QString COLUMN_TYPE = q_("Type");
	const QString COLUMN_LASTVERSION = q_("Last Version");
	const QString COLUMN_LASTUPDATE = q_("Last Update");
	const QString COLUMN_INSTALLED = q_("Installed");
};

#endif // _ADDONTABLEMODEL_HPP_
