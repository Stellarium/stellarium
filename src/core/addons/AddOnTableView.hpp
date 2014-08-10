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
#include <QTableView>

class AddOnTableView : public QTableView
{
	Q_OBJECT
public:
	AddOnTableView(QWidget* parent=0);
	virtual ~AddOnTableView();

	void setModel(QAbstractItemModel* model);

private:
	bool isCompatible(QString first, QString last);
};

#endif // _ADDONTABLEVIEW_HPP_
