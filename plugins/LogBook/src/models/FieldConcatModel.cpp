/*
 * Copyright (C) 2010 Timothy Reaves
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

#include "FieldConcatModel.hpp"

#include <QDebug>
#include <QSqlRecord>
#include <QModelIndex>
#include <QSqlTableModel>
#include <QVariant>

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Creation & Destruction
#endif
/* ********************************************************************* */
FieldConcatModel::FieldConcatModel(QSqlTableModel *backingModel, QStringList fieldNames, QString seperationString, QObject *parent) 
	: QAbstractTableModel(parent)
{
	// These asserts do basic sanity checking of the model
    Q_ASSERT_X(backingModel != 0, "FieldConcatModel::FieldConcatModel", "The backing model can not be null.");
    Q_ASSERT_X(fieldNames.size() != 0, "FieldConcatModel::FieldConcatModel", "The list of field names can not be empty.");
    Q_ASSERT_X(seperationString != 0, "FieldConcatModel::FieldConcatModel", "The seperation string can not be null.");
	
	dataModel = backingModel;
	fields = QStringList(fieldNames);
	seperator = QString(seperationString);
	connect(dataModel, SIGNAL(destroyed()), this, SIGNAL(destroyed()));
	connect(dataModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(dataChanged(QModelIndex,QModelIndex)));
	connect(dataModel, SIGNAL(headerDataChanged(Qt::Orientation,int,int)), this, SIGNAL(headerDataChanged(Qt::Orientation,int,int)));
	connect(dataModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SIGNAL(rowsInserted(QModelIndex,int,int)));
	connect(dataModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
	connect(dataModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SIGNAL(rowsRemoved(QModelIndex,int,int)));
	connect(dataModel, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)), this, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)));
	connect(dataModel, SIGNAL(columnsRemoved(QModelIndex,int,int)), this, SIGNAL(columnsRemoved(QModelIndex,int,int)));
	connect(dataModel, SIGNAL(columnsInserted(QModelIndex,int,int)), this, SIGNAL(columnsInserted(QModelIndex,int,int)));
	
	connect(dataModel, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
	connect(dataModel, SIGNAL(layoutChanged()), this, SIGNAL(layoutChanged()));
}

FieldConcatModel::~FieldConcatModel()
{
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark QAbstractTableModel subclass methods
#endif
/* ********************************************************************* */
int FieldConcatModel::columnCount(const QModelIndex&) const
{
	return 2;
}

QString FieldConcatModel::displayStringForRecord(QSqlRecord& record) const
{
	QString data;
	if (!record.isEmpty()) {
		data = QString("");
		foreach (QString field, fields) {
			QVariant value = record.value(field);
			if (value.isValid() && value.toString() != "") {
				if (data == "") {
					data.append(value.toString());
				} else {
					data.append(seperator).append(value.toString());
				}
				
			}
		}
	}
	return data;
}

QVariant FieldConcatModel::data(const QModelIndex & index, int role) const
{
	QVariant data;
	if (role == Qt::DisplayRole) {
		QSqlRecord record = dataModel->record(index.row());
		if (!record.isEmpty()) {
			if (index.column() == 0) {
				data = record.value(0);
			} else {
				data = QVariant(displayStringForRecord(record));
			}
		}
	}
	return data;
}

int FieldConcatModel::idForDisplayString(QString)
{
	return 0;
}

int FieldConcatModel::rowCount(const QModelIndex & parent) const
{
	return dataModel->rowCount(parent);
}

void FieldConcatModel::setFilter(const QString& filter)
{
	dataModel->setFilter(filter);
}
