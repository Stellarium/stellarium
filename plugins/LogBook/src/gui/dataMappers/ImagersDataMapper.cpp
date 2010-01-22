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

#include "ImagersDataMapper.hpp"
#include "LogBookCommon.hpp"
#include "ui_ImagersWidget.h"

#include <QDebug>
#include <QModelIndex>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QVariant>


ImagersDataMapper::ImagersDataMapper(Ui_ImagersWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent) : QObject(parent)
{
	widget = aWidget;
	
	QRegExp nonEmptyStringRegEx("\\S+.*");
	QRegExpValidator *nonEmptyStringValidator = new QRegExpValidator(nonEmptyStringRegEx, this);
	widget->modelLineEdit->setValidator(nonEmptyStringValidator);
	
	tableModel = tableModels[IMAGERS];
	
	setupConnections();
	widget->imagersListView->setCurrentIndex(tableModel->index(0, 1));	
}

ImagersDataMapper::~ImagersDataMapper()
{
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected slots
#endif
/* ********************************************************************* */
void ImagersDataMapper::deleteSelectedImager()
{
	QModelIndex selection = widget->imagersListView->currentIndex();
	if (selection.row() != -1 && tableModel->rowCount() > 1) {
		tableModel->removeRows(selection.row(), 1);
		widget->imagersListView->setCurrentIndex(tableModel->index(0, 1));
	}
}

void ImagersDataMapper::imagerSelected(const QModelIndex &index)
{
	
	if (index.row() != -1) {
		lastRowNumberSelected = index.row();
		populateFormWithIndex(index);
	}
}

void ImagersDataMapper::insertNewImager()
{
    QSqlField field1("model", QVariant::String);
    QSqlField field2("vendor", QVariant::String);
	field1.setValue(QVariant("Model"));
	field2.setValue(QVariant("Vendor"));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	
	if (tableModel->insertRecord(-1, newRecord)) {
		widget->imagersListView->setCurrentIndex(tableModel->index(tableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new Imager.  The error is: " << tableModel->lastError();
	}
}

void ImagersDataMapper::modelChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("model") != widget->modelLineEdit->text()) {
		record.setValue("model", widget->modelLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Imager.  Error is: " << tableModel->lastError();
		}
	}
	widget->imagersListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void ImagersDataMapper::vendorChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("vendor") != widget->vendorLineEdit->text()) {
		record.setValue("vendor", widget->vendorLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Imager.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->imagersListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected methods
#endif
/* ********************************************************************* */
QSqlRecord ImagersDataMapper::currentRecord()
{ 
	if (lastRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModel->record(lastRowNumberSelected);
	}
}

void ImagersDataMapper::populateFormWithIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		QSqlRecord record = tableModel->record(index.row());
		widget->idLabel->setText(record.value("imager_id").toString());
		widget->modelLineEdit->setText(record.value("model").toString());
		widget->vendorLineEdit->setText(record.value("vendor").toString());
		setupConnections();
	}
}

void ImagersDataMapper::setupConnections()
{
	connect(widget->addImagerButton, SIGNAL(clicked()), this, SLOT(insertNewImager()));
	connect(widget->deleteImagerButton, SIGNAL(clicked()), this, SLOT(deleteSelectedImager()));
	connect(widget->imagersListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(imagerSelected(const QModelIndex &)));
	connect(widget->imagersListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(imagerSelected(QModelIndex)));
	connect(widget->modelLineEdit, SIGNAL(editingFinished()), this, SLOT(modelChanged()));
	connect(widget->vendorLineEdit, SIGNAL(editingFinished()), this, SLOT(vendorChanged()));
}

void ImagersDataMapper::teardownConnections()
{
	disconnect(widget->addImagerButton, SIGNAL(clicked()), this, SLOT(insertNewImager()));
	disconnect(widget->deleteImagerButton, SIGNAL(clicked()), this, SLOT(deleteSelectedImager()));
	disconnect(widget->imagersListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(imagerSelected(const QModelIndex &)));
	disconnect(widget->imagersListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			   this, SLOT(imagerSelected(QModelIndex)));
	disconnect(widget->modelLineEdit, SIGNAL(editingFinished()), this, SLOT(modelChanged()));
	disconnect(widget->vendorLineEdit, SIGNAL(editingFinished()), this, SLOT(vendorChanged()));
}

