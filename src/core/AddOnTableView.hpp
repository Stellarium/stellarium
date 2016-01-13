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

#ifndef _ADDONTABLEVIEW_HPP_
#define _ADDONTABLEVIEW_HPP_

#include <QAbstractItemModel>
#include <QButtonGroup>
#include <QCheckBox>
#include <QTableView>

#include "AddOnWidget.hpp"
#include "AddOnHeader.hpp"

class AddOnTableView : public QTableView
{
	Q_OBJECT
public:
	AddOnTableView(QWidget* parent=0);
	virtual ~AddOnTableView();

	void mousePressEvent(QMouseEvent* e);
	void mouseDoubleClickEvent(QMouseEvent* e);
	void setModel(QAbstractItemModel* model);

	QCheckBox* getCheckBox(int pRow) { return (QCheckBox*) m_pCheckboxGroup->button(pRow); }

signals:
	void addonSelected(AddOn* addon);

public slots:
	void clearSelection();
	void setAllChecked(bool checked);

private slots:
	void slotRowChecked(int);

private:
	AddOnHeader* m_pAddOnHeader;
	QButtonGroup* m_pCheckboxGroup;
};

#endif // _ADDONTABLEVIEW_HPP_
