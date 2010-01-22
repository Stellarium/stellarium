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

#ifndef _SITESDATAMAPPER_HPP_
#define _SITESDATAMAPPER_HPP_

#include <QObject>
#include <QMap>

class Ui_SitesWidget;
class QModelIndex;
class QSqlTableModel;
class QSqlRecord;

class SitesDataMapper : public QObject {
	Q_OBJECT
	
public:
	SitesDataMapper(Ui_SitesWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent = 0);
	virtual ~SitesDataMapper();
	
protected slots:
	void addSiteForCurrentLocation();
	void deleteSelectedSite();
	void elevationChanged();
	void insertNewSite();
	void latitudeChanged();
	void longitudeChanged();
	void nameChanged();
	void siteSelected(const QModelIndex &index);
	void timezoneOffsetChanged();
	
protected:
	QSqlRecord currentRecord();
	void populateFormWithIndex(const QModelIndex &index);
	void setupConnections();
	void teardownConnections();
	
private:
	int lastRowNumberSelected;
	QSqlTableModel *tableModel;
	Ui_SitesWidget *widget;
	
};

#endif // _SITESDATAMAPPER_HPP_
