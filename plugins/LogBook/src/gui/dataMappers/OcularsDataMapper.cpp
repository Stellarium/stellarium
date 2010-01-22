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

#include "OcularsDataMapper.hpp"
#include "LogBookCommon.hpp"
#include "ui_OcularsWidget.h"

#include <QDebug>
#include <QModelIndex>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QVariant>

OcularsDataMapper::OcularsDataMapper(Ui_OcularsWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent) : QObject(parent)
{
	widget = aWidget;
	
	QRegExp nonEmptyStringRegEx("\\S+.*");
	QRegExpValidator *nonEmptyStringValidator = new QRegExpValidator(nonEmptyStringRegEx, this);
	QDoubleValidator *ocularEFL = new QDoubleValidator(1.0, 60.0, 1, this);
	QIntValidator *ocularAFOV = new QIntValidator(35, 100, this);
	widget->modelLineEdit->setValidator(nonEmptyStringValidator);
	widget->focalLengthLineEdit->setValidator(ocularEFL);
	widget->apparentFOVLineEdit->setValidator(ocularAFOV);
	
	tableModel = tableModels[OCULARS];
	
	setupConnections();
	widget->ocularsListView->setCurrentIndex(tableModel->index(0, 1));	
}

OcularsDataMapper::~OcularsDataMapper()
{
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected slots
#endif
/* ********************************************************************* */
void OcularsDataMapper::apparentFOVChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("apparent_fov") != widget->apparentFOVLineEdit->text()) {
		record.setValue("apparent_fov", widget->apparentFOVLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Ocular.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->ocularsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void OcularsDataMapper::deleteSelectedOcular()
{
	QModelIndex selection = widget->ocularsListView->currentIndex();
	if (selection.row() != -1 && tableModel->rowCount() > 1) {
		tableModel->removeRows(selection.row(), 1);
		widget->ocularsListView->setCurrentIndex(tableModel->index(0, 1));
	}
}

void OcularsDataMapper::focalLengthChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("focal_length") != widget->focalLengthLineEdit->text()) {
		record.setValue("focal_length", widget->focalLengthLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Ocular.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->ocularsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void OcularsDataMapper::insertNewOcular()
{
    QSqlField field1("model", QVariant::String);
    QSqlField field2("vendor", QVariant::String);
    QSqlField field3("focal_length", QVariant::Double);
    QSqlField field4("apparent_fov", QVariant::Int);
	field1.setValue(QVariant("Model"));
	field2.setValue(QVariant("Vendor"));
	field3.setValue(QVariant(31.0));
	field4.setValue(QVariant(55));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	newRecord.append(field3);
	newRecord.append(field4);
	
	if (tableModel->insertRecord(-1, newRecord)) {
		widget->ocularsListView->setCurrentIndex(tableModel->index(tableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new Ocular.  The error is: " << tableModel->lastError();
	}
}

void OcularsDataMapper::modelChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("model") != widget->modelLineEdit->text()) {
		record.setValue("model", widget->modelLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Ocular.  Error is: " << tableModel->lastError();
		}
	}
	widget->ocularsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void OcularsDataMapper::ocularSelected(const QModelIndex &index)
{	
	if (index.row() != -1) {
		lastRowNumberSelected = index.row();
		populateFormWithIndex(index);
	}
}

void OcularsDataMapper::vendorChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("vendor") != widget->vendorLineEdit->text()) {
		record.setValue("vendor", widget->vendorLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Ocular.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->ocularsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected methods
#endif
/* ********************************************************************* */
QSqlRecord OcularsDataMapper::currentRecord()
{ 
	if (lastRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModel->record(lastRowNumberSelected);
	}
}

void OcularsDataMapper::populateFormWithIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		QSqlRecord record = tableModel->record(index.row());
		widget->idLabel->setText(record.value("Ocular_id").toString());
		widget->modelLineEdit->setText(record.value("model").toString());
		widget->vendorLineEdit->setText(record.value("vendor").toString());
		widget->focalLengthLineEdit->setText(record.value("focal_length").toString());
		widget->apparentFOVLineEdit->setText(record.value("apparent_fov").toString());
		setupConnections();
	}
}

void OcularsDataMapper::setupConnections()
{
	connect(widget->addOcularButton, SIGNAL(clicked()), this, SLOT(insertNewOcular()));
	connect(widget->deleteOcularButton, SIGNAL(clicked()), this, SLOT(deleteSelectedOcular()));
	connect(widget->ocularsListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(ocularSelected(const QModelIndex &)));
	connect(widget->ocularsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(ocularSelected(QModelIndex)));
	connect(widget->modelLineEdit, SIGNAL(editingFinished()), this, SLOT(modelChanged()));
	connect(widget->vendorLineEdit, SIGNAL(editingFinished()), this, SLOT(vendorChanged()));
	connect(widget->focalLengthLineEdit, SIGNAL(editingFinished()), this, SLOT(focalLengthChanged()));
	connect(widget->apparentFOVLineEdit, SIGNAL(editingFinished()), this, SLOT(apparentFOVChanged()));
}

void OcularsDataMapper::teardownConnections()
{
	disconnect(widget->addOcularButton, SIGNAL(clicked()), this, SLOT(insertNewOcular()));
	disconnect(widget->deleteOcularButton, SIGNAL(clicked()), this, SLOT(deleteSelectedOcular()));
	disconnect(widget->ocularsListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(ocularSelected(const QModelIndex &)));
	disconnect(widget->ocularsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			   this, SLOT(ocularSelected(QModelIndex)));
	disconnect(widget->modelLineEdit, SIGNAL(editingFinished()), this, SLOT(modelChanged()));
	disconnect(widget->vendorLineEdit, SIGNAL(editingFinished()), this, SLOT(vendorChanged()));
	disconnect(widget->focalLengthLineEdit, SIGNAL(editingFinished()), this, SLOT(focalLengthChanged()));
	disconnect(widget->apparentFOVLineEdit, SIGNAL(editingFinished()), this, SLOT(apparentFOVChanged()));
}

