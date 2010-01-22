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

#include "FiltersDataMapper.hpp"
#include "LogBookCommon.hpp"
#include "ui_FiltersWidget.h"

#include <QDebug>
#include <QModelIndex>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QVariant>


FiltersDataMapper::FiltersDataMapper(Ui_FiltersWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent) : QObject(parent)
{
	widget = aWidget;
	
	QRegExp nonEmptyStringRegEx("\\S+.*");
	QRegExpValidator *nonEmptyStringValidator = new QRegExpValidator(nonEmptyStringRegEx, this);
	widget->typeLineEdit->setValidator(nonEmptyStringValidator);
	widget->vendorLineEdit->setValidator(nonEmptyStringValidator);
	
	tableModel = tableModels[FILTERS];
	
	setupConnections();
	widget->filtersListView->setCurrentIndex(tableModel->index(0, 1));	
}

FiltersDataMapper::~FiltersDataMapper()
{
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected slots
#endif
/* ********************************************************************* */
void FiltersDataMapper::colorChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("color") != widget->colorLineEdit->text()) {
		record.setValue("color", widget->colorLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update filter.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->filtersListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void FiltersDataMapper::deleteSelectedFilter()
{
	QModelIndex selection = widget->filtersListView->currentIndex();
	if (selection.row() != -1 && tableModel->rowCount() > 1) {
		tableModel->removeRows(selection.row(), 1);
		widget->filtersListView->setCurrentIndex(tableModel->index(0, 1));
	}
}

void FiltersDataMapper::filterSelected(const QModelIndex &index)
{
	if (index.row() != -1) {
		lastRowNumberSelected = index.row();
		populateFormWithIndex(index);
	}
}

void FiltersDataMapper::insertNewFilter()
{
    QSqlField field1("type", QVariant::String);
    QSqlField field2("vendor", QVariant::String);
    QSqlField field3("model", QVariant::String);
    QSqlField field4("color", QVariant::String);
	field1.setValue(QVariant("Type"));
	field2.setValue(QVariant("Vendor"));
	field3.setValue(QVariant("Model"));
	field4.setValue(QVariant("Color"));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	newRecord.append(field3);
	newRecord.append(field4);
	
	if (tableModel->insertRecord(-1, newRecord)) {
		widget->filtersListView->setCurrentIndex(tableModel->index(tableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new filter.  The error is: " << tableModel->lastError();
	}
}

void FiltersDataMapper::modelChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("model") != widget->modelLineEdit->text()) {
		record.setValue("model", widget->modelLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update filter.  Error is: " << tableModel->lastError();
		}
	}
	widget->filtersListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void FiltersDataMapper::typeChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("type") != widget->typeLineEdit->text()) {
		record.setValue("type", widget->typeLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update filter.  Error is: " << tableModel->lastError();
		}
	}
	widget->filtersListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void FiltersDataMapper::vendorChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("vendor") != widget->vendorLineEdit->text()) {
		record.setValue("vendor", widget->vendorLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update filter.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->filtersListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected methods
#endif
/* ********************************************************************* */
QSqlRecord FiltersDataMapper::currentRecord()
{ 
	if (lastRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModel->record(lastRowNumberSelected);
	}
}

void FiltersDataMapper::populateFormWithIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		QSqlRecord record = tableModel->record(index.row());
		widget->idLabel->setText(record.value("filter_id").toString());
		widget->typeLineEdit->setText(record.value("type").toString());
		widget->modelLineEdit->setText(record.value("model").toString());
		widget->vendorLineEdit->setText(record.value("vendor").toString());
		widget->colorLineEdit->setText(record.value("color").toString());
		setupConnections();
	}
}

void FiltersDataMapper::setupConnections()
{
	connect(widget->addFilterButton, SIGNAL(clicked()), this, SLOT(insertNewFilter()));
	connect(widget->deleteFilterButton, SIGNAL(clicked()), this, SLOT(deleteSelectedFilter()));
	connect(widget->filtersListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(filterSelected(const QModelIndex &)));
	connect(widget->filtersListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(filterSelected(QModelIndex)));
	connect(widget->modelLineEdit, SIGNAL(editingFinished()), this, SLOT(modelChanged()));
	connect(widget->vendorLineEdit, SIGNAL(editingFinished()), this, SLOT(vendorChanged()));
	connect(widget->typeLineEdit, SIGNAL(editingFinished()), this, SLOT(typeChanged()));
	connect(widget->colorLineEdit, SIGNAL(editingFinished()), this, SLOT(colorChanged()));
}

void FiltersDataMapper::teardownConnections()
{
	disconnect(widget->addFilterButton, SIGNAL(clicked()), this, SLOT(insertNewFilter()));
	disconnect(widget->deleteFilterButton, SIGNAL(clicked()), this, SLOT(deleteSelectedFilter()));
	disconnect(widget->filtersListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(filterSelected(const QModelIndex &)));
	disconnect(widget->filtersListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			   this, SLOT(filterSelected(QModelIndex)));
	disconnect(widget->modelLineEdit, SIGNAL(editingFinished()), this, SLOT(modelChanged()));
	disconnect(widget->vendorLineEdit, SIGNAL(editingFinished()), this, SLOT(vendorChanged()));
	disconnect(widget->typeLineEdit, SIGNAL(editingFinished()), this, SLOT(typeChanged()));
	disconnect(widget->colorLineEdit, SIGNAL(editingFinished()), this, SLOT(colorChanged()));
}

