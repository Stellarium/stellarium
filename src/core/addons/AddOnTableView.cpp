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
#include "StelAddOnMgr.hpp"
#include "StelUtils.hpp"

AddOnTableView::AddOnTableView(QWidget* parent)
	: QTableView(parent)
	, m_pCheckedHeader(NULL)
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
		this, SIGNAL(rowChecked(int, bool)));
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
	m_pCheckedHeader = new CheckedHeader(lastColumn, Qt::Horizontal, this);
	setHorizontalHeader(m_pCheckedHeader);
	horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	horizontalHeader()->setSectionResizeMode(lastColumn, QHeaderView::ResizeToContents);
	horizontalHeader()->setVisible(true);
	connect(m_pCheckedHeader, SIGNAL(toggled(bool)),
		this, SLOT(setAllChecked(bool)), Qt::UniqueConnection);

	// Hide imcompatible add-ons
	// Insert checkboxes to the checkboxgroup (rows)
	for (int row=0; row < model->rowCount(); row=row+2)
	{
		QString first = model->index(row, 2).data().toString();
		QString last = model->index(row, 3).data().toString();
		if (StelApp::getInstance().getStelAddOnMgr().isCompatible(first, last))
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

void AddOnTableView::mouseDoubleClickEvent(QMouseEvent *e)
{
	QModelIndex index = indexAt(e->pos());
	const int row = index.row();
	if (index.isValid() && !(row%2))
	{
		// toogle state
		bool state = getCheckBox(row)->checkState();
		slotCheckRow(index.row(), state?0:2);
	}
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
	int wRow; // addonwidget row
	if (!deselected.isEmpty())
	{
		wRow = deselected.first().top() + 1;
		if (wRow % 2)
		{
			hideRow(wRow);
		}
	}
	if (!selected.isEmpty())
	{
		wRow = selected.first().top() + 1;
		if (wRow % 2)
		{
			insertAddOnWidget(wRow);
			showRow(wRow);
		}
	}
	update();
}

AddOnWidget* AddOnTableView::insertAddOnWidget(int wRow)
{
	if (m_widgets.contains(wRow))
	{
		return m_widgets.value(wRow);
	}
	AddOnTableProxyModel* model = (AddOnTableProxyModel*) this->model();
	int addOnId = model->findIndex(wRow, COLUMN_ADDONID).data().toInt();
	AddOnWidget* widget = new AddOnWidget(this, wRow);
	widget->init(addOnId);
	setRowHeight(wRow, widget->height());
	setIndexWidget(model->index(wRow, 0), widget);
	m_widgets.insert(wRow, widget);
	if (objectName() == "texturesTableView")
	{
		connect(widget, SIGNAL(checkRow(int, int)),
			this, SLOT(slotCheckRow(int, int)));
	}
	return m_widgets.value(wRow);
}

void AddOnTableView::slotCheckRow(int pRow, int checked)
{
	m_pCheckboxGroup->blockSignals(true);
	QCheckBox* cbox = getCheckBox(pRow);
	cbox->setCheckState((Qt::CheckState) checked);
	slotRowChecked(pRow, checked);
	m_pCheckboxGroup->blockSignals(false);
}

void AddOnTableView::setAllChecked(bool checked)
{
	for (int row=0; row < model()->rowCount(); row=row+2)
	{
		slotCheckRow(row, checked?2:0);
		emit(rowChecked(row, checked));
	}
}

void AddOnTableView::slotRowChecked(int pRow, bool checked)
{
	AddOnWidget* widget = insertAddOnWidget(pRow+1);
	AddOnTableProxyModel* model = (AddOnTableProxyModel*) this->model();
	int addOnId = model->findIndex(pRow, COLUMN_ADDONID).data().toInt();
	int installed = model->findIndex(pRow, COLUMN_INSTALLED).data(Qt::EditRole).toInt();
	if (checked)
	{
		QStringList selectedFilesToInstall = widget->getSelectedFilesToInstall();
		QStringList selectedFilesToRemove = widget->getSelectedFilesToRemove();

		if (installed == 2)
		{
			m_iSelectedAddOnsToRemove.insert(addOnId, selectedFilesToRemove);
		}
		else if (installed == 1) // partially
		{
			if (selectedFilesToInstall.isEmpty())
				m_iSelectedAddOnsToInstall.remove(addOnId);
			else
				m_iSelectedAddOnsToInstall.insert(addOnId, selectedFilesToInstall);

			if (selectedFilesToRemove.isEmpty())
				m_iSelectedAddOnsToRemove.remove(addOnId);
			else
				m_iSelectedAddOnsToRemove.insert(addOnId, selectedFilesToRemove);
		}
		else
		{
			m_iSelectedAddOnsToInstall.insert(addOnId, selectedFilesToInstall);
		}
	}
	else
	{
		if (installed == 2)
		{
			m_iSelectedAddOnsToRemove.remove(addOnId);
		}
		else if (installed == 1) // partially
		{
			m_iSelectedAddOnsToInstall.remove(addOnId);
			m_iSelectedAddOnsToRemove.remove(addOnId);
		}
		else
		{
			m_iSelectedAddOnsToInstall.remove(addOnId);
		}
	}

	emit(somethingToInstall(m_iSelectedAddOnsToInstall.size() > 0));
	emit(somethingToRemove(m_iSelectedAddOnsToRemove.size() > 0));

	// update checkbox header
	if (m_pCheckedHeader)
	{
		int countChecked = 0;
		foreach (QAbstractButton* cbox, m_pCheckboxGroup->buttons())
		{
			if (cbox->isChecked())
				countChecked++;
		}

		if (countChecked ==	model->sourceModel()->rowCount()) // all rows checked ?
		{
			m_pCheckedHeader->setChecked(true);
		} else if (countChecked == 0){
			m_pCheckedHeader->setChecked(false);
		}
	}
}
