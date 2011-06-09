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

#include "SitesDataMapper.hpp"
#include "LogBookCommon.hpp"
#include "ui_SitesWidget.h"
#include "LimitingDoubleValidator.hpp"

#include <QDebug>
#include <QModelIndex>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QVariant>

#include "StelApp.hpp"
#include "StelCore.hpp"

SitesDataMapper::SitesDataMapper(Ui_SitesWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent) : QObject(parent)
{
	widget = aWidget;
	
	QRegExp nonEmptyStringRegEx("\\S+.*");
	QRegExpValidator *nonEmptyStringValidator = new QRegExpValidator(nonEmptyStringRegEx, this);
	LimitingDoubleValidator *latitudeValidator = new LimitingDoubleValidator(-90.0, 90.0, 6, this);
	LimitingDoubleValidator *longitudeValidator = new LimitingDoubleValidator(-180.0, 180.0, 6, this);
	widget->nameLineEdit->setValidator(nonEmptyStringValidator);
	widget->latitudeLineEdit->setValidator(latitudeValidator);
	widget->longitudeLineEdit->setValidator(longitudeValidator);
	
	tableModel = tableModels[SITES];
	
	setupConnections();
	widget->sitesListView->setCurrentIndex(tableModel->index(0, 1));
}

SitesDataMapper::~SitesDataMapper()
{
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected slots
#endif
/* ********************************************************************* */
void SitesDataMapper::addSiteForCurrentLocation()
{
	const StelCore* core = StelApp::getInstance().getCore();
    QSqlField field1("name", QVariant::String);
    QSqlField field2("longitude", QVariant::Double);
    QSqlField field3("latitude", QVariant::Double);
    QSqlField field4("elevation", QVariant::Int);
    QSqlField field5("timezone_offset", QVariant::Double);
	field1.setValue(QVariant(core->getCurrentLocation().name));
	field2.setValue(QVariant(core->getCurrentLocation().longitude));
	field3.setValue(QVariant(core->getCurrentLocation().latitude));
	field4.setValue(QVariant(core->getCurrentLocation().altitude));
	field5.setValue(QVariant(0));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	newRecord.append(field3);
	newRecord.append(field4);
	newRecord.append(field5);
	
	if (tableModel->insertRecord(-1, newRecord)) {
		widget->sitesListView->setCurrentIndex(tableModel->index(tableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new Site.  The error is: " << tableModel->lastError();
	}
}

void SitesDataMapper::deleteSelectedSite()
{
	QModelIndex selection = widget->sitesListView->currentIndex();
	if (selection.row() != -1 && tableModel->rowCount() > 1) {
		tableModel->removeRows(selection.row(), 1);
		widget->sitesListView->setCurrentIndex(tableModel->index(0, 1));
	}
}

void SitesDataMapper::elevationChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("elevation") != widget->elevationLineEdit->text()) {
		record.setValue("elevation", widget->elevationLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Site.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->sitesListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void SitesDataMapper::insertNewSite()
{
    QSqlField field1("name", QVariant::String);
    QSqlField field2("longitude", QVariant::Double);
    QSqlField field3("latitude", QVariant::Double);
    QSqlField field4("elevation", QVariant::Int);
    QSqlField field5("timezone_offset", QVariant::Double);
	field1.setValue(QVariant("Homerville"));
	field2.setValue(QVariant(-82.126162));
	field3.setValue(QVariant(41.029239));
	field4.setValue(QVariant(300));
	field5.setValue(QVariant(4));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	newRecord.append(field3);
	newRecord.append(field4);
	newRecord.append(field5);
	
	if (tableModel->insertRecord(-1, newRecord)) {
		widget->sitesListView->setCurrentIndex(tableModel->index(tableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new Site.  The error is: " << tableModel->lastError();
	}
}

void SitesDataMapper::latitudeChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("latatude") != widget->latitudeLineEdit->text()) {
		record.setValue("latatude", widget->latitudeLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Site.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->sitesListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void SitesDataMapper::longitudeChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("longatude") != widget->longitudeLineEdit->text()) {
		record.setValue("longatude", widget->longitudeLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Site.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->sitesListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void SitesDataMapper::nameChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("name") != widget->nameLineEdit->text()) {
		record.setValue("name", widget->nameLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Site.  Error is: " << tableModel->lastError();
		}
	}
	widget->sitesListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void SitesDataMapper::siteSelected(const QModelIndex &index)
{
	if (index.row() != -1) {
		lastRowNumberSelected = index.row();
		populateFormWithIndex(index);
	}
}

void SitesDataMapper::timezoneOffsetChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("timezone_offset") != widget->tzOffsetLineEdit->text()) {
		record.setValue("timezone_offset", widget->tzOffsetLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Site.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->sitesListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected methods
#endif
/* ********************************************************************* */
QSqlRecord SitesDataMapper::currentRecord()
{ 
	if (lastRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModel->record(lastRowNumberSelected);
	}
}

void SitesDataMapper::populateFormWithIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		QSqlRecord record = tableModel->record(index.row());
		widget->idLabel->setText(record.value("site_id").toString());
		widget->nameLineEdit->setText(record.value("name").toString());
		widget->longitudeLineEdit->setText(record.value("longatude").toString());
		widget->latitudeLineEdit->setText(record.value("latatude").toString());
		widget->elevationLineEdit->setText(record.value("elevation").toString());
		widget->tzOffsetLineEdit->setText(record.value("timezone_offset").toString());
		setupConnections();
	}
}

void SitesDataMapper::setupConnections()
{
	connect(widget->addSiteButton, SIGNAL(clicked()), this, SLOT(insertNewSite()));
	connect(widget->deleteSiteButton, SIGNAL(clicked()), this, SLOT(deleteSelectedSite()));
	connect(widget->addCurrentLocationPushButton, SIGNAL(clicked()), this, SLOT(addSiteForCurrentLocation()));
	connect(widget->sitesListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(siteSelected(const QModelIndex &)));
	connect(widget->sitesListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(siteSelected(QModelIndex)));
	connect(widget->nameLineEdit, SIGNAL(editingFinished()), this, SLOT(nameChanged()));
	connect(widget->longitudeLineEdit, SIGNAL(editingFinished()), this, SLOT(longitudeChanged()));
	connect(widget->latitudeLineEdit, SIGNAL(editingFinished()), this, SLOT(latitudeChanged()));
	connect(widget->elevationLineEdit, SIGNAL(editingFinished()), this, SLOT(elevationChanged()));
	connect(widget->tzOffsetLineEdit, SIGNAL(editingFinished()), this, SLOT(timezoneOffsetChanged()));
}

void SitesDataMapper::teardownConnections()
{
	disconnect(widget->addSiteButton, SIGNAL(clicked()), this, SLOT(insertNewSite()));
	disconnect(widget->deleteSiteButton, SIGNAL(clicked()), this, SLOT(deleteSelectedSite()));
	disconnect(widget->addCurrentLocationPushButton, SIGNAL(clicked()), this, SLOT(addSiteForCurrentLocation()));
	disconnect(widget->sitesListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(siteSelected(const QModelIndex &)));
	disconnect(widget->sitesListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			   this, SLOT(siteSelected(QModelIndex)));
	disconnect(widget->nameLineEdit, SIGNAL(editingFinished()), this, SLOT(nameChanged()));
	disconnect(widget->longitudeLineEdit, SIGNAL(editingFinished()), this, SLOT(longitudeChanged()));
	disconnect(widget->latitudeLineEdit, SIGNAL(editingFinished()), this, SLOT(latitudeChanged()));
	disconnect(widget->elevationLineEdit, SIGNAL(editingFinished()), this, SLOT(elevationChanged()));
	disconnect(widget->tzOffsetLineEdit, SIGNAL(editingFinished()), this, SLOT(timezoneOffsetChanged()));
}

