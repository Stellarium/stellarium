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

//! A model encapsulating a satellite list.
//! Used for GUI presentation purposes.
//! 
//! It can show satellite names in their hint colors. This behavior can be
//! toggled with enableColoredNames(), e.g. to honor night mode.
//! 
//! @warning The model keeps a pointer to the satellite list, not a copy,
//! so every time that list is modified outside the model (i.e. satellites are
//! added or removed), you need to call beginSatellitesChange() and
//! endSatellitesChange().
//!
//! @todo Decide whether to use a table model with multiple columns (current),
//! or a list model dumping everything through data(). Both require enums - for
//! column numbers or data roles.
//! @ingroup satellites
class SatellitesListModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	SatellitesListModel(QList<SatelliteP>* satellites, QObject *parent = Q_NULLPTR);
	
	void setSatelliteList(QList<SatelliteP>* satellites);
	
	//! @name Reimplemented model handling methods.
	//@{
	Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
	bool setData(const QModelIndex& index, const QVariant& value, int role) Q_DECL_OVERRIDE;
	int rowCount(const QModelIndex& parent) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex& parent) const Q_DECL_OVERRIDE;
	//@}
	
signals:
	
public slots:
	//! Tell the model that its internal data structure is about to be modified.
	//! This should be called every time when the underlying data structure is 
	//! about to be modified outside the model! (That is, when satellites are
	//! about to be added or removed.) Otherwise views that use this
	//! model won't work correctly.
	void beginSatellitesChange();
	//! Tell the model that its internal data has been modified.
	//! Don't call this without calling beginSatellitesChange() first!
	//! This should be called every time when the underlying data structure has 
	//! been modified outside the model! (That is, when satellites have been
	//! added or removed.) Otherwise views that use this model won't work
	//! correctly.
	void endSatellitesChange();
	void enableColoredNames(bool enable = true);
	
private:
	//! Pointer to the external data object represented by the model.
	//! Call updateSatellitesData() every time that data is modified outside
	//! the model!
	QList<SatelliteP>* satelliteList;
	
	//! Enabled by default.
	bool coloredNames;
};

#endif // SATELLITESLISTMODEL_HPP
