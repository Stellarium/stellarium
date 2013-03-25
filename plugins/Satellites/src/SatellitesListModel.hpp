/*
 * Copyright (C) 2013 Bogdan Marinov
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SATELLITESLISTMODEL_HPP
#define SATELLITESLISTMODEL_HPP

#include <QAbstractTableModel>
#include <QList>
#include <QSharedPointer>

#include "Satellite.hpp"

typedef QSharedPointer<Satellite> SatelliteP;

//! A model encapsulating a satellite list.
//! Used for GUI presentation purposes.
//! 
//! It can show satellite names in their hint colors. This behavior can be
//! toggled with enableColoredNames(), e.g. to honor night mode.
//! @todo Decide whether to use a table model with multiple columns (current),
//! or a list model dumping everything through data(). Both require enums - for
//! column numbers or data roles.
class SatellitesListModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	SatellitesListModel(QList<SatelliteP>* satellites, QObject *parent = 0);
	
	void setSatelliteList(QList<SatelliteP>* satellites);
	
	//! Description of the model columns.
	enum Column {
		ColumnNameId = 0,
		ColumnDisplayed,
		ColumnOrbit
	};
	
	//! @name Reimplemented model handling methods.
	//@{
	Qt::ItemFlags flags(const QModelIndex& index) const;
	QVariant data(const QModelIndex& index, int role) const;
	int rowCount(const QModelIndex& parent) const;
	int columnCount(const QModelIndex& parent) const;
	//@}
	
signals:
	
public slots:
	void enableColoredNames(bool enable = true);
	
private:
	QList<SatelliteP>* satelliteList;
	
	//! Enabled by default.
	bool coloredNames;
};

#endif // SATELLITESLISTMODEL_HPP
