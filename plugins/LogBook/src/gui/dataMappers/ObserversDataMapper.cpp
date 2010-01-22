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

#include "ObserversDataMapper.hpp"
#include "LogBookCommon.hpp"
#include "ui_ObserversWidget.h"

#include <QDebug>
#include <QModelIndex>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QVariant>


ObserversDataMapper::ObserversDataMapper(Ui_ObserversWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent) : QObject(parent)
{
	widget = aWidget;
	
	QRegExp nonEmptyStringRegEx("\\S+.*");
	QRegExpValidator *nonEmptyStringValidator = new QRegExpValidator(nonEmptyStringRegEx, this);
	widget->firstNameLineEdit->setValidator(nonEmptyStringValidator);
	widget->lastNameLineEdit->setValidator(nonEmptyStringValidator);
	
	tableModel = tableModels[OBSERVERS];
	
	setupConnections();
	widget->observersListView->setCurrentIndex(tableModel->index(0, 1));	
}

ObserversDataMapper::~ObserversDataMapper()
{
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected slots
#endif
/* ********************************************************************* */
void ObserversDataMapper::deleteSelectedObserver()
{
	QModelIndex selection = widget->observersListView->currentIndex();
	if (selection.row() != -1 && tableModel->rowCount() > 1) {
		tableModel->removeRows(selection.row(), 1);
		widget->observersListView->setCurrentIndex(tableModel->index(0, 1));
	}
}

void ObserversDataMapper::firstNameChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("name") != widget->firstNameLineEdit->text()) {
		record.setValue("name", widget->firstNameLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Observer.  Error is: " << tableModel->lastError();
		}
	}
	widget->observersListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void ObserversDataMapper::insertNewObserver()
{
    QSqlField field1("name", QVariant::String);
    QSqlField field2("surname", QVariant::String);
	field1.setValue(QVariant("First"));
	field2.setValue(QVariant("Last"));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	
	if (tableModel->insertRecord(-1, newRecord)) {
		widget->observersListView->setCurrentIndex(tableModel->index(tableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new Observer.  The error is: " << tableModel->lastError();
	}
}

void ObserversDataMapper::lastNameChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("surname") != widget->lastNameLineEdit->text()) {
		record.setValue("surname", widget->lastNameLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update Observer.  Error is: " << tableModel->lastError();
		}
	}
	
	widget->observersListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void ObserversDataMapper::observerSelected(const QModelIndex &index)
{	
	if (index.row() != -1) {
		lastRowNumberSelected = index.row();
		populateFormWithIndex(index);
	}
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected methods
#endif
/* ********************************************************************* */
QSqlRecord ObserversDataMapper::currentRecord()
{ 
	if (lastRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModel->record(lastRowNumberSelected);
	}
}

void ObserversDataMapper::populateFormWithIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		QSqlRecord record = tableModel->record(index.row());
		widget->idLabel->setText(record.value("observer_id").toString());
		widget->firstNameLineEdit->setText(record.value("name").toString());
		widget->lastNameLineEdit->setText(record.value("surname").toString());
		setupConnections();
	}
}

void ObserversDataMapper::setupConnections()
{
	connect(widget->addObserverButton, SIGNAL(clicked()), this, SLOT(insertNewObserver()));
	connect(widget->deleteObserverButton, SIGNAL(clicked()), this, SLOT(deleteSelectedObserver()));
	connect(widget->observersListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(observerSelected(const QModelIndex &)));
	connect(widget->observersListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(observerSelected(QModelIndex)));
	connect(widget->firstNameLineEdit, SIGNAL(editingFinished()), this, SLOT(firstNameChanged()));
	connect(widget->lastNameLineEdit, SIGNAL(editingFinished()), this, SLOT(lastNameChanged()));
}

void ObserversDataMapper::teardownConnections()
{
	disconnect(widget->addObserverButton, SIGNAL(clicked()), this, SLOT(insertNewObserver()));
	disconnect(widget->deleteObserverButton, SIGNAL(clicked()), this, SLOT(deleteSelectedObserver()));
	disconnect(widget->observersListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(observerSelected(const QModelIndex &)));
	disconnect(widget->observersListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			   this, SLOT(observerSelected(QModelIndex)));
	disconnect(widget->firstNameLineEdit, SIGNAL(editingFinished()), this, SLOT(firstNameChanged()));
	disconnect(widget->lastNameLineEdit, SIGNAL(editingFinished()), this, SLOT(lastNameChanged()));
}

