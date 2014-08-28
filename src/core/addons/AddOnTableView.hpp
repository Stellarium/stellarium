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

#ifndef _ADDONTABLEVIEW_HPP_
#define _ADDONTABLEVIEW_HPP_

#include <QAbstractItemModel>
#include <QButtonGroup>
#include <QCheckBox>
#include <QTableView>

#include "AddOnWidget.hpp"
#include "widget/CheckedHeader.hpp"

class AddOnTableView : public QTableView
{
	Q_OBJECT
public:
	AddOnTableView(QWidget* parent=0);
	virtual ~AddOnTableView();

	void mousePressEvent(QMouseEvent* e);
	void mouseDoubleClickEvent(QMouseEvent* e);
	void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void setModel(QAbstractItemModel* model);

	QHash<int, QStringList> getSelectedAddonsToInstall() { return m_iSelectedAddOnsToInstall; }
	QHash<int, QStringList> getSelectedAddonsToRemove() { return m_iSelectedAddOnsToRemove; }
	QCheckBox* getCheckBox(int pRow) { return (QCheckBox*) m_pCheckboxGroup->button(pRow); }

signals:
	// these signals are useful to handle the status of the install/remove buttons
	void somethingToInstall(bool yes);
	void somethingToRemove(bool yes);
	void rowChecked(int row, bool checked);

public slots:
	void setAllChecked(bool checked);

private slots:
	void slotCheckRow(int pRow, int checked);
	void slotRowChecked(int pRow, bool checked);

private:
	CheckedHeader* m_pCheckedHeader;
	QButtonGroup* m_pCheckboxGroup;
	QHash<int, AddOnWidget*> m_widgets;
	QHash<int, QStringList> m_iSelectedAddOnsToInstall;
	QHash<int, QStringList> m_iSelectedAddOnsToRemove;
	AddOnWidget* insertAddOnWidget(int wRow);
	bool isCompatible(QString first, QString last);
};

#endif // _ADDONTABLEVIEW_HPP_
