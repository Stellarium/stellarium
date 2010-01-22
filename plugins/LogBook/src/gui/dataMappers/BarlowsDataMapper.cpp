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

#include "BarlowsDataMapper.hpp"
#include "LogBookCommon.hpp"
#include "ui_BarlowsWidget.h"

#include <QDebug>
#include <QModelIndex>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QVariant>

BarlowsDataMapper::BarlowsDataMapper(Ui_BarlowsWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent) : QObject(parent)
{
	widget = aWidget;
	
	QRegExp nonEmptyStringRegEx("\\S+.*");
	QRegExpValidator *nonEmptyStringValidator = new QRegExpValidator(nonEmptyStringRegEx, this);
	QDoubleValidator *factorValidator = new QDoubleValidator(0.0, 10.0, 1, this);
	widget->modelLineEdit->setValidator(nonEmptyStringValidator);
	widget->factorLineEdit->setValidator(factorValidator);
	
	tableModel = tableModels[BARLOWS];
	
	setupConnections();
	widget->barlowsListView->setCurrentIndex(tableModel->index(0, 1));	
}

BarlowsDataMapper::~BarlowsDataMapper()
{
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected slots
#endif
/* ********************************************************************* */
void BarlowsDataMapper::barlowSelected(const QModelIndex &index)
{
	
	if (index.row() != -1) {
		lastRowNumberSelected = index.row();
		populateFormWithIndex(index);
	}
}

void BarlowsDataMapper::deleteSelectedBarlow()
{
	QModelIndex selection = widget->barlowsListView->currentIndex();
	if (selection.row() != -1 && tableModel->rowCount() > 1) {
		tableModel->removeRows(selection.row(), 1);
		widget->barlowsListView->setCurrentIndex(tableModel->index(0, 1));
	}
}

void BarlowsDataMapper::factorChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("factor") != widget->factorLineEdit->text()) {
		record.setValue("factor", widget->factorLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update barlow.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->barlowsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void BarlowsDataMapper::insertNewBarlow()
{
    QSqlField field1("model", QVariant::String);
    QSqlField field2("vendor", QVariant::String);
    QSqlField field3("factor", QVariant::Double);
	field1.setValue(QVariant("Model"));
	field2.setValue(QVariant("Vendor"));
	field3.setValue(QVariant(1.0));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	newRecord.append(field3);
	
	if (tableModel->insertRecord(-1, newRecord)) {
		widget->barlowsListView->setCurrentIndex(tableModel->index(tableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new barlow.  The error is: " << tableModel->lastError();
	}
}

void BarlowsDataMapper::modelChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("model") != widget->modelLineEdit->text()) {
		record.setValue("model", widget->modelLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update barlow.  Error is: " << tableModel->lastError();
		}
	}
	widget->barlowsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void BarlowsDataMapper::vendorChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("vendor") != widget->vendorLineEdit->text()) {
		record.setValue("vendor", widget->vendorLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update barlow.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->barlowsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected methods
#endif
/* ********************************************************************* */
QSqlRecord BarlowsDataMapper::currentRecord()
{ 
	if (lastRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModel->record(lastRowNumberSelected);
	}
}

void BarlowsDataMapper::populateFormWithIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		QSqlRecord record = tableModel->record(index.row());
		widget->idLabel->setText(record.value("barlow_id").toString());
		widget->modelLineEdit->setText(record.value("model").toString());
		widget->vendorLineEdit->setText(record.value("vendor").toString());
		widget->factorLineEdit->setText(record.value("factor").toString());
		setupConnections();
	}
}

void BarlowsDataMapper::setupConnections()
{
	connect(widget->addBarlowButton, SIGNAL(clicked()), this, SLOT(insertNewBarlow()));
	connect(widget->deleteBarlowButton, SIGNAL(clicked()), this, SLOT(deleteSelectedBarlow()));
	connect(widget->barlowsListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(barlowSelected(const QModelIndex &)));
	connect(widget->barlowsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(barlowSelected(QModelIndex)));
	connect(widget->modelLineEdit, SIGNAL(editingFinished()), this, SLOT(modelChanged()));
	connect(widget->vendorLineEdit, SIGNAL(editingFinished()), this, SLOT(vendorChanged()));
	connect(widget->factorLineEdit, SIGNAL(editingFinished()), this, SLOT(factorChanged()));
}

void BarlowsDataMapper::teardownConnections()
{
	disconnect(widget->addBarlowButton, SIGNAL(clicked()), this, SLOT(insertNewBarlow()));
	disconnect(widget->deleteBarlowButton, SIGNAL(clicked()), this, SLOT(deleteSelectedBarlow()));
	disconnect(widget->barlowsListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(barlowSelected(const QModelIndex &)));
	disconnect(widget->barlowsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(barlowSelected(QModelIndex)));
	disconnect(widget->modelLineEdit, SIGNAL(editingFinished()), this, SLOT(modelChanged()));
	disconnect(widget->vendorLineEdit, SIGNAL(editingFinished()), this, SLOT(vendorChanged()));
	disconnect(widget->factorLineEdit, SIGNAL(editingFinished()), this, SLOT(factorChanged()));
}

