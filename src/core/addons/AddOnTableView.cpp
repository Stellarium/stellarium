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

#include <QHeaderView>
#include <QStringBuilder>

#include "AddOnTableProxyModel.hpp"
#include "AddOnTableView.hpp"
#include "StelAddOnDAO.hpp"
#include "StelUtils.hpp"
#include "widget/CheckedHeader.hpp"

AddOnTableView::AddOnTableView(QWidget* parent)
	: QTableView(parent)
{
	setAutoFillBackground(true);
	verticalHeader()->setVisible(false);
	setAlternatingRowColors(false);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setEditTriggers(false);
}

AddOnTableView::~AddOnTableView()
{
	for (int row=1; row < model()->rowCount(); row=row+2)
	{
		delete m_pWidgets.take(row);
	}
	m_pWidgets.clear();
}

void AddOnTableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	if (!deselected.isEmpty())
	{
		int row = deselected.first().top();
		if (!(row % 2))
		{
			hideRow(row + 1);
		}
	}
	if (!selected.isEmpty())
	{
		int row = selected.first().top();
		if (!(row % 2))
		{
			insertAddOnWidget(row + 1);
			showRow(row + 1);
		}
	}
	update();
}

void AddOnTableView::insertAddOnWidget(int row)
{
	if (m_pWidgets.contains(row))
	{
		return;
	}
	AddOnWidget* widget = new AddOnWidget(this);
	setRowHeight(row, widget->height());
	setIndexWidget(model()->index(row, 0), widget);
	m_pWidgets.insert(row, widget);
}

void AddOnTableView::setModel(QAbstractItemModel* model)
{
	QTableView::setModel(model);

	// Add checkbox in the last column
	int lastColumn = model->columnCount() - 1; // checkbox column
	setHorizontalHeader(new CheckedHeader(lastColumn, Qt::Horizontal, this));
	horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	horizontalHeader()->setSectionResizeMode(lastColumn, QHeaderView::ResizeToContents);

	// Hide internal columns
	AddOnTableProxyModel* proxy = (AddOnTableProxyModel*) model;
	hideColumn(proxy->findColumn(COLUMN_ID));
	hideColumn(proxy->findColumn(COLUMN_ADDONID));
	hideColumn(proxy->findColumn(COLUMN_FIRST_STEL));
	hideColumn(proxy->findColumn(COLUMN_LAST_STEL));

	// Hide imcompatible add-ons
	for (int row=0; row < model->rowCount(); row=row+2)
	{
		QString first = model->index(row, 2).data().toString();
		QString last = model->index(row, 3).data().toString();
		setRowHidden(row, !isCompatible(first, last));
	}

	// Hide and span empty rows
	for (int row=1; row < model->rowCount(); row=row+2)
	{
		setSpan(row, 0, 1, model->columnCount());
		hideRow(row);
	}
}

bool AddOnTableView::isCompatible(QString first, QString last)
{
	QStringList c = StelUtils::getApplicationVersion().split(".");
	QStringList f = first.split(".");
	QStringList l = last.split(".");

	if (c.size() < 3 || f.size() < 3 || l.size() < 3) {
		return false; // invalid version
	}

	int currentVersion = QString(c.at(0) % "00" % c.at(1) % "0" % c.at(2)).toInt();
	int firstVersion = QString(f.at(0) % "00" % f.at(1) % "0" % f.at(2)).toInt();
	int lastVersion = QString(l.at(0) % "00" % l.at(1) % "0" % l.at(2)).toInt();

	if (currentVersion < firstVersion || currentVersion > lastVersion)
	{
		return false; // out of bounds
	}

	return true;
}
