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

#ifndef _FIELDCONCATMODEL_HPP_
#define _FIELDCONCATMODEL_HPP_

#include <QObject>
#include <QAbstractTableModel>
#include <QModelIndexList>
#include <QStringList>

class QModelIndex;
class QSqlRecord;
class QSqlTableModel;
class QVariant;

//! Allows for concatinating fields from a model for disply.
//! This class allows a table model to concatinate fields for the purpose of display.  Qt widgets that accept default Qt models allow
//! the user to specify which column's value to use for display purposes, but only one column can be used.  Here, the user can provide
//! a list of column - field - names to use, as well as a string to use to seperate the columns.
//! As this model is intended for list and combo box selection, it is read-only.
//! If any of the fields values are invalid or empty, they are skipped in the concatination operation, and the seperator as well.
//! @author Timothy Reaves
class FieldConcatModel : public QAbstractTableModel {
	Q_OBJECT
	
public:
	//! the default constructor
	//! @param backingModel the model this modul uses as a source of data.
	//! @param fieldNames a list of the names of the fields to be concatinated.  The fields are used in the order they are in the list.
	//! @param seperationString the string used to seperate the fields values.  Defaults to " | ". 
	//! @param parent the parent of this object.
	FieldConcatModel(QSqlTableModel *backingModel, QStringList fieldNames, QString seperationString = " | ", QObject *parent = 0);
	virtual ~FieldConcatModel();
	virtual int columnCount(const QModelIndex& parent = QModelIndex() ) const;
	QString displayStringForRecord(QSqlRecord& record) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole ) const;
	int idForDisplayString(QString displayString);
	virtual int rowCount(const QModelIndex& parent = QModelIndex() ) const;
	void setFilter(const QString& filter); 
/*
signals:
	void destroyed();
	void dataChanged(QModelIndex,QModelIndex);
	void headerDataChanged(Qt::Orientation,int,int);
	void rowsInserted(QModelIndex,int,int);
	void rowsAboutToBeRemoved(QModelIndex,int,int);
	void rowsRemoved(QModelIndex,int,int);
	void columnsAboutToBeRemoved(QModelIndex,int,int);
	void columnsRemoved(QModelIndex,int,int);
	void columnsInserted(QModelIndex,int,int);
	
	void modelReset();
	void layoutChanged();
*/
private:
	QStringList fields;
	QString seperator;
	QSqlTableModel *dataModel;
};

#endif // _FIELDCONCATMODEL_HPP_
