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
#include <QTableView>

#include "AddOnWidget.hpp"

class AddOnTableView : public QTableView
{
	Q_OBJECT
public:
	AddOnTableView(QWidget* parent=0);
	virtual ~AddOnTableView();

	void mousePressEvent(QMouseEvent* e);
	void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void setModel(QAbstractItemModel* model);

	void clearSelection();
	void setAllChecked(bool checked);
	QList<QPair<int, int> > getSelectedAddons() { return m_iSelectedAddOns; }

signals:
	// these signals are useful to handle the status of the install/remove buttons
	void somethingToInstall(bool yes);
	void somethingToRemove(bool yes);

private slots:
	void slotRowChecked(int row, bool checked);

private:
	QButtonGroup* m_pCheckboxGroup;
	QHash<int, AddOnWidget*> m_widgets;
	QList<QPair<int, int> > m_iSelectedAddOns;
	void insertAddOnWidget(int row);
	bool isCompatible(QString first, QString last);
};

#endif // _ADDONTABLEVIEW_HPP_
