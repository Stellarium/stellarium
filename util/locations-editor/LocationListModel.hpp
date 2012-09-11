/*
 * Stellarium Location List Editor
 * Copyright (C) 2012  Bogdan Marinov
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
 * Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef LOCATIONLISTMODEL_HPP
#define LOCATIONLISTMODEL_HPP

#include <QAbstractTableModel>

#include <QFile>
#include <QList>
#include <QString>

#include "Location.hpp"

//! Custom Model class to store the locations list.
//! Location objects are stored in QList instead of a QMap as it is in
//! Stellarium, which may lead to some problems...
//! @author Bogdan Marinov
class LocationListModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	explicit LocationListModel(QObject *parent = 0);
	LocationListModel(QList<Location> locationList, QObject *parent = 0);
	
	static LocationListModel* load(QFile* file);
	bool save(const QString& filePath);
	bool saveBinary(const QString& filePath);
	
	bool isModified() const {return modified;}
	
	// Reimplemented:
	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
	int rowCount (const QModelIndex& parent = QModelIndex() ) const;
	
	QVariant data (const QModelIndex& index, int role = Qt::DisplayRole ) const;
	QVariant headerData (int section, Qt::Orientation orientation, int role) const;
	
signals:
	
public slots:
	void setModified(bool changed = true) {modified = changed;}
	
private:
	//! Flag set to "true" if the model has been modified.
	bool modified;
	//! The location list stored by the model.
	QList<Location> locations;
};

#endif // LOCATIONLISTMODEL_HPP
