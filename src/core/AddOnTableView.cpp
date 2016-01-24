/*
 * Stellarium
 * Copyright (C) 2014-2016 Marcos Cardinot
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
#include <QMouseEvent>
#include <QScrollBar>
#include <QStringBuilder>

#include "AddOnTableModel.hpp"
#include "AddOnTableView.hpp"
#include "StelAddOnMgr.hpp"
#include "StelUtils.hpp"

AddOnTableView::AddOnTableView(QWidget* parent)
	: QTableView(parent)
	, m_pCheckboxGroup(new QButtonGroup(this))
{
	setAutoFillBackground(true);
	verticalHeader()->setVisible(false);
	setAlternatingRowColors(false);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setFocusPolicy(Qt::NoFocus);
	setEditTriggers(false);
	setShowGrid(false);

	m_pCheckboxGroup->setExclusive(false);
	connect(m_pCheckboxGroup, SIGNAL(buttonToggled(int, bool)),
		this, SLOT(slotButtonToggled(int, bool)));
}

AddOnTableView::~AddOnTableView()
{
	m_pCheckboxGroup->deleteLater();
}

void AddOnTableView::setModel(QAbstractItemModel* model)
{
	QTableView::setModel(model);

	int lastColumn = model->columnCount() - 1; // checkbox column
	horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	horizontalHeader()->setSectionResizeMode(lastColumn, QHeaderView::ResizeToContents);
	horizontalHeader()->setVisible(true);

	// Insert checkboxes to the checkboxgroup (rows)
	for (int row=0; row < model->rowCount(); row++)
	{
		QCheckBox* cbox = new QCheckBox();
		cbox->setStyleSheet("QCheckBox { margin-left: 8px; margin-right: 8px; margin-bottom: 2px; }");
		cbox->setAutoFillBackground(true);
		setIndexWidget(model->index(row, lastColumn), cbox);
		m_pCheckboxGroup->addButton(cbox, row);
	}
	m_checkedAddons.clear();
}

void AddOnTableView::mouseDoubleClickEvent(QMouseEvent *e)
{
	QModelIndex index = indexAt(e->pos());
	if (index.isValid())
	{
		getCheckBox(index.row())->toggle();
	}
}

void AddOnTableView::mousePressEvent(QMouseEvent *e)
{
	QModelIndex index = indexAt(e->pos());
	if (!index.isValid())
	{
		clearSelection();
		return;
	}
	selectRow(index.row());
}

void AddOnTableView::setAllChecked(bool checked)
{
	for (int row=0; row < model()->rowCount(); row++)
	{
		getCheckBox(row)->setChecked(checked);
	}
}

void AddOnTableView::clearSelection()
{
	setAllChecked(false);
	QTableView::clearSelection();
}

void AddOnTableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QAbstractItemView::selectionChanged(selected, deselected);
	AddOn* addon = NULL;
	if (!selected.isEmpty())
	{
		addon = ((AddOnTableModel*) model())->getAddOn(selected.first().top());
	}
	emit(addonSelected(addon));
}

void AddOnTableView::slotButtonToggled(int row, bool checked)
{
	AddOn* addon = ((AddOnTableModel*) model())->getAddOn(row);
	if (checked)
	{
		m_checkedAddons.insert(addon);
	}
	else
	{
		m_checkedAddons.remove(addon);
	}
	selectRow(row);
	emit(addonChecked());
}
