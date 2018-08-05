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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef OCULARSDATAMAPPER_HPP
#define OCULARSDATAMAPPER_HPP

#include <QObject>
#include <QMap>

class Ui_OcularsWidget;
class QModelIndex;
class QSqlTableModel;
class QSqlRecord;

class OcularsDataMapper : public QObject {
	Q_OBJECT
	
public:
	OcularsDataMapper(Ui_OcularsWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent = 0);
	virtual ~OcularsDataMapper();
	
protected slots:
	void apparentFOVChanged();
	void deleteSelectedOcular();
	void focalLengthChanged();
	void insertNewOcular();
	void modelChanged();
	void ocularSelected(const QModelIndex &index);
	void vendorChanged();
	
protected:
	QSqlRecord currentRecord();
	void populateFormWithIndex(const QModelIndex &index);
	void setupConnections();
	void teardownConnections();
	
private:
	int lastRowNumberSelected;
	QSqlTableModel *tableModel;
	Ui_OcularsWidget *widget;
	
};

#endif // OCULARSDATAMAPPER_HPP
