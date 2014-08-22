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

#include <QCheckBox>
#include <QHeaderView>
#include <QStringBuilder>

#include "AddOnTableProxyModel.hpp"
#include "AddOnTableView.hpp"
#include "StelAddOnDAO.hpp"
#include "StelUtils.hpp"
#include "widget/CheckedHeader.hpp"

AddOnTableView::AddOnTableView(QWidget* parent)
	: QTableView(parent)
	, m_pCheckboxGroup(new QButtonGroup(this))
{
	setAutoFillBackground(true);
	verticalHeader()->setVisible(false);
	setAlternatingRowColors(false);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setEditTriggers(false);
	setShowGrid(false);

	m_pCheckboxGroup->setExclusive(false);
	connect(m_pCheckboxGroup, SIGNAL(buttonToggled(int, bool)),
		this, SLOT(slotRowChecked(int, bool)));
}

AddOnTableView::~AddOnTableView()
{
	int i = 0;
	while (!m_widgets.isEmpty())
	{
		delete m_widgets.take(i);
		i++;
	}
	m_widgets.clear();

	m_pCheckboxGroup->deleteLater();
}

void AddOnTableView::mousePressEvent(QMouseEvent *e)
{
	QModelIndex index = indexAt(e->pos());
	if (!index.isValid() || !isRowHidden(index.row() + 1))
	{
		clearSelection();
		return;
	}
	selectRow(index.row());
}

void AddOnTableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	if (!deselected.isEmpty())
	{
		int drow = deselected.first().top() + 1;
		if (drow % 2)
		{
			hideRow(drow);
		}
	}
	if (!selected.isEmpty())
	{
		int srow = selected.first().top() + 1;
		if (srow % 2)
		{
			insertAddOnWidget(srow);
			showRow(srow);
		}
	}
	update();
}

void AddOnTableView::insertAddOnWidget(int row)
{
	if (m_widgets.contains(row))
	{
		return;
	}
	AddOnTableProxyModel* model = (AddOnTableProxyModel*) this->model();
	int addOnId = model->findIndex(row, COLUMN_ADDONID).data().toInt();
	AddOnWidget* widget = new AddOnWidget(this);
	widget->init(addOnId);
	setRowHeight(row, widget->height());
	setIndexWidget(model->index(row, 0), widget);
	m_widgets.insert(row, widget);
}

void AddOnTableView::setModel(QAbstractItemModel* model)
{
	QTableView::setModel(model);

	m_widgets.clear();
	m_iSelectedAddOnsToInstall.clear();
	m_iSelectedAddOnsToRemove.clear();
	emit(somethingToInstall(false));
	emit(somethingToRemove(false));

	// Add checkbox in the last column (header)
	int lastColumn = model->columnCount() - 1; // checkbox column
	CheckedHeader* checkedHeader = new CheckedHeader(lastColumn, Qt::Horizontal, this);
	setHorizontalHeader(checkedHeader);
	horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	horizontalHeader()->setSectionResizeMode(lastColumn, QHeaderView::ResizeToContents);
	horizontalHeader()->setVisible(true);
	connect(checkedHeader, SIGNAL(toggled(bool)), this, SLOT(setAllChecked(bool)), Qt::UniqueConnection);

	// Hide imcompatible add-ons
	// Insert checkboxes to the checkboxgroup (rows)
	for (int row=0; row < model->rowCount(); row=row+2)
	{
		QString first = model->index(row, 2).data().toString();
		QString last = model->index(row, 3).data().toString();
		if (isCompatible(first, last))
		{
			QCheckBox* cbox = new QCheckBox();
			cbox->setStyleSheet("QCheckBox { margin-left: 8px; margin-right: 8px; margin-bottom: 2px; }");
			cbox->setAutoFillBackground(true);
			setIndexWidget(model->index(row, lastColumn), cbox);
			m_pCheckboxGroup->addButton(cbox, row);
		}
		else
		{
			hideRow(row);
		}
	}

	// Hide internal columns
	AddOnTableProxyModel* proxy = (AddOnTableProxyModel*) model;
	hideColumn(proxy->findColumn(COLUMN_ID));
	hideColumn(proxy->findColumn(COLUMN_ADDONID));
	hideColumn(proxy->findColumn(COLUMN_FIRST_STEL));
	hideColumn(proxy->findColumn(COLUMN_LAST_STEL));

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

void AddOnTableView::clearSelection()
{
	QTableView::clearSelection();
	setAllChecked(false);
}

void AddOnTableView::setAllChecked(bool checked)
{
	foreach (QAbstractButton* cbox,m_pCheckboxGroup->buttons())
	{
		cbox->setChecked(checked);
	}
}

void AddOnTableView::slotRowChecked(int row, bool checked)
{
	AddOnTableProxyModel* model = (AddOnTableProxyModel*) this->model();
	int addOnId = model->findIndex(row, COLUMN_ADDONID).data().toInt();
	bool installed = "Yes" == model->findIndex(row, COLUMN_INSTALLED).data().toString();
	if (checked)
	{
		if (installed)
		{
			m_iSelectedAddOnsToRemove.append(addOnId);
		}
		else
		{
			m_iSelectedAddOnsToInstall.append(addOnId);
		}
	}
	else
	{
		if (installed)
		{
			m_iSelectedAddOnsToRemove.removeOne(addOnId);
		}
		else
		{
			m_iSelectedAddOnsToInstall.removeOne(addOnId);
		}
	}

	emit(somethingToInstall(m_iSelectedAddOnsToInstall.size() > 0));
	emit(somethingToRemove(m_iSelectedAddOnsToRemove.size() > 0));
}
