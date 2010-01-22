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

#ifndef _OPTICSDATAMAPPER_HPP_
#define _OPTICSDATAMAPPER_HPP_

#include <QObject>
#include <QMap>

class Ui_OpticsWidget;
class QModelIndex;
class QSqlTableModel;
class QSqlRecord;

class OpticsDataMapper : public QObject {
	Q_OBJECT
	
public:
	OpticsDataMapper(Ui_OpticsWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent = 0);
	virtual ~OpticsDataMapper();

protected slots:
	void apertureChanged();
	void deleteSelectedOptic();
	void focalLengthChanged();
	void hFlipChanged(int state);
	void insertNewOptic();
	void lightTransmissionChanged();
	void modelChanged();
	void opticSelected(const QModelIndex &index);
	void typeChanged(const QString &newValue);
	void vendorChanged();
	void vFlipChanged(int state);

protected:
	QSqlRecord currentRecord();
	void populateFormWithIndex(const QModelIndex &index);
	void setupConnections();
	void teardownConnections();
	
private:
	int lastRowNumberSelected;
	QSqlTableModel *tableModel;
	QSqlTableModel *typeTableModel;
	Ui_OpticsWidget *widget;
	
};

#endif // _OPTICSDATAMAPPER_HPP_
