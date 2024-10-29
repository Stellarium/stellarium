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

#ifndef SATELLITESLISTFILTERMODEL_HPP
#define SATELLITESLISTFILTERMODEL_HPP

#include <QSortFilterProxyModel>

#include "Satellite.hpp" // for the flags

//! Custom proxy model allowing filtering by satellite group and flag.
//! @ingroup satellites
class SatellitesListFilterModel : public QSortFilterProxyModel
{
	Q_OBJECT

	//! Only satellites with this flag raised will be returned.
	//! Use Satellite::NoFlags for no filtering.
	//! Setting the flag with setFilterFlag() or setSecondaryFilters()
	//! will cause the model to be re-filtered.
	Q_PROPERTY(SatFlag filterFlag
	           READ getFilterFlag
		   WRITE setFilterFlag
		   NOTIFY filterFlagChanged)
	
	//! Only satellites belonging to this group will be returned.
	//! Use an empty group ID for no filtering.
	//! Setting the group with setFilterGroup() or setSecondaryFilters()
	//! will cause the model to be re-filtered.
	Q_PROPERTY(QString filterGroup
	           READ getFilterGroup
		   WRITE setFilterGroup
		   NOTIFY filterGroupChanged)
public:
	SatellitesListFilterModel(QObject *parent = nullptr);
	
	SatFlag getFilterFlag() const { return filterFlag; }
	void setFilterFlag(const SatFlag& flag);
	
	QString getFilterGroup() const { return filterGroup; }
	void setFilterGroup(const QString& groupId);
	
	void setSecondaryFilters(const QString& groupId,
	                         const SatFlag& flag);
	
signals:
	void filterFlagChanged(SatFlag&);
	void filterGroupChanged(QString&);
	
public slots:
	
protected:
	//! Reimplementation of the filtering method.
	bool filterAcceptsRow(int source_row,
			      const QModelIndex& source_parent) const override;
	
private:
	SatFlag filterFlag;
	QString filterGroup;
};

#endif // SATELLITESLISTFILTERMODEL_HPP
