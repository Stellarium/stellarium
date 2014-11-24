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
#include "StelTranslator.hpp"

AddOnTableModel::AddOnTableModel(AddOn::Category category, QHash<AddOn::Type, StelAddOnMgr::AddOnMap> addons, QObject* parent)
	: QAbstractTableModel(parent)
{
	switch (category) {
		case AddOn::CATALOG:
		{
			QMap<qint64, AddOn*> plugin = addons.value(AddOn::Plugin_Catalog);
			QMap<qint64, AddOn*> star = addons.value(AddOn::Star_Catalog);
			m_addons = plugin.unite(star);
			m_iColumns << Title << Type << Version;
			break;
		}
		case AddOn::LANDSCAPE:
		{
			m_addons = addons.value(AddOn::Landscape);
			m_iColumns << Title << Version;
			break;
		}
		case AddOn::LANGUAGEPACK:
		{
			m_addons = addons.value(AddOn::Language_Pack);
			m_iColumns << Title << Type << LastUpdate;
			break;
		}
		case AddOn::SCRIPT:
		{
			m_addons = addons.value(AddOn::Script);
			m_iColumns << Title << Version;
			break;
		}
		case AddOn::STARLORE:
		{
			m_addons = addons.value(AddOn::Sky_Culture);
			m_iColumns << Title << LastUpdate;
			break;
		}
		case AddOn::TEXTURE:
		{
			m_addons = addons.value(AddOn::Texture);
			m_iColumns << Title << Version;
			break;
		}
		default:
		{
			qWarning() << "AddOnTableModel : Invalid category!";
		}
	}
	m_iColumns << Status << Checkbox;
}

int AddOnTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return m_addons.size() * 2;
}

int AddOnTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return m_iColumns.size();
}

QVariant AddOnTableModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::TextAlignmentRole)
	{
		return Qt::AlignCenter;
	}

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
		case LastUpdate:
			value = addon->getDate();
			break;
		case Status:
			value = q_(addon->getStatusString());
			break;
		case Checkbox:
			value = "";
			break;
	}
	return qVariantFromValue(value);
}

QVariant AddOnTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
	{
		return QVariant();
	}

	QString value;
	Column column = m_iColumns.at(section);
	switch (column) {
		case Title:
			value = q_("Title");
			break;
		case Type:
			value = q_("Type");
			break;
		case Version:
			value = q_("Version");
			break;
		case LastUpdate:
			value = q_("Last Update");
			break;
		case Status:
			value = q_("Status");
			break;
		case Checkbox:
			value = "";
			break;
	}
	return qVariantFromValue(value);
}
