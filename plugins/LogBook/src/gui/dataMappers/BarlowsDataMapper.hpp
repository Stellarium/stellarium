/*
 * Copyright (C) 2009 Timothy Reaves
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _BARLOWSDATAMAPPER_HPP_
#define _BARLOWSDATAMAPPER_HPP_

#include <QObject>
#include <QMap>

class Ui_BarlowsWidget;
class QModelIndex;
class QSqlTableModel;
class QSqlRecord;

class BarlowsDataMapper : public QObject {
	Q_OBJECT
	
public:
	BarlowsDataMapper(Ui_BarlowsWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent = 0);
	virtual ~BarlowsDataMapper();
	
protected slots:
	void barlowSelected(const QModelIndex &index);
	void deleteSelectedBarlow();
	void factorChanged();
	void insertNewBarlow();
	void modelChanged();
	void vendorChanged();
	
protected:
	QSqlRecord currentRecord();
	void populateFormWithIndex(const QModelIndex &index);
	void setupConnections();
	void teardownConnections();
	
private:
	int lastRowNumberSelected;
	QSqlTableModel *tableModel;
	Ui_BarlowsWidget *widget;
	
};

#endif // _BARLOWSDATAMAPPER_HPP_
