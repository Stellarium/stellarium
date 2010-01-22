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

#include "OpticsDataMapper.hpp"
#include "LogBookCommon.hpp"
#include "ui_OpticsWidget.h"

#include <QDebug>
#include <QModelIndex>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QVariant>

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Creation & Destruction
#endif
/* ********************************************************************* */
OpticsDataMapper::OpticsDataMapper(Ui_OpticsWidget *aWidget, QMap<QString, QSqlTableModel *> tableModels, QObject *parent) : QObject(parent)
{
	widget = aWidget;
	
	QRegExp nonEmptyStringRegEx("\\S+.*");
	QRegExpValidator *nonEmptyStringValidator = new QRegExpValidator(nonEmptyStringRegEx, this);
	QDoubleValidator *apertureValidator = new QDoubleValidator(1.0, 10000.0, 1, this);
	widget->modelLineEdit->setValidator(nonEmptyStringValidator);
	widget->apertureLineEdit->setValidator(apertureValidator);

	tableModel = tableModels[OPTICS];
	typeTableModel = tableModels[OPTICS_TYPE];
	
	widget->typeComboBox->setModel(typeTableModel);
	widget->typeComboBox->setModelColumn(typeTableModel->fieldIndex("name"));
	
	setupConnections();
	widget->opticsListView->setCurrentIndex(tableModel->index(0, 1));	
}

OpticsDataMapper::~OpticsDataMapper()
{
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark pretiected slots
#endif
/* ********************************************************************* */
void OpticsDataMapper::apertureChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("aperture").toDouble() != widget->apertureLineEdit->text().toDouble()) {
		record.setValue("aperture", widget->apertureLineEdit->text().toDouble());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update optic.  Error is: " << tableModel->lastError();
		}
	}
	widget->opticsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void OpticsDataMapper::deleteSelectedOptic()
{
	QModelIndex selection = widget->opticsListView->currentIndex();
	if (selection.row() != -1 && tableModel->rowCount() > 1) {
		tableModel->removeRows(selection.row(), 1);
		widget->opticsListView->setCurrentIndex(tableModel->index(0, 1));
	}
}

void OpticsDataMapper::focalLengthChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("focal_length").toDouble() != widget->focalLengthLineEdit->text().toDouble()) {
		record.setValue("focal_length", widget->focalLengthLineEdit->text().toDouble());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update optic.  Error is: " << tableModel->lastError();
		}
	}
	widget->opticsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void OpticsDataMapper::hFlipChanged(int state)
{
	QSqlRecord record = currentRecord();
	bool isSelected = (state == Qt::Checked);
	if (!record.isEmpty() && record.value("hFlip").toBool() != isSelected) {
		record.setValue("hFlip", isSelected);
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update optic hFlip.  Error is: " << tableModel->lastError();
		}
	}
	widget->opticsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void OpticsDataMapper::insertNewOptic()
{
    QSqlField field1("model", QVariant::String);
    QSqlField field2("vendor", QVariant::String);
    QSqlField field3("optics_type_id", QVariant::Int);
    QSqlField field4("focal_length", QVariant::Double);
    QSqlField field5("aperture", QVariant::Double);
    QSqlField field6("hFlip", QVariant::Bool);
    QSqlField field7("vFlip", QVariant::Bool);
    QSqlField field8("light_transmission", QVariant::Double);
	field1.setValue(QVariant("Model"));
	field2.setValue(QVariant("Vendor"));
	field3.setValue(QVariant(10));
	field4.setValue(QVariant(3190));
	field5.setValue(QVariant(355.6));
	field6.setValue(QVariant(false));
	field7.setValue(QVariant(false));
	field8.setValue(QVariant(99.1));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	newRecord.append(field3);
	newRecord.append(field4);
	newRecord.append(field5);
	newRecord.append(field6);
	newRecord.append(field7);
	newRecord.append(field8);
	
	if (tableModel->insertRecord(-1, newRecord)) {
		widget->opticsListView->setCurrentIndex(tableModel->index(tableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new optic.  The error is: " << tableModel->lastError();
	}
}

void OpticsDataMapper::lightTransmissionChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("light_transmission").toDouble() != widget->lightTransmissionLineEdit->text().toDouble()) {
		record.setValue("light_transmission", widget->lightTransmissionLineEdit->text().toDouble());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update optic.  Error is: " << tableModel->lastError();
		}
	}
	widget->opticsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void OpticsDataMapper::modelChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("model") != widget->modelLineEdit->text()) {
		record.setValue("model", widget->modelLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update optic.  Error is: " << tableModel->lastError();
		}
	}
	widget->opticsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void OpticsDataMapper::opticSelected(const QModelIndex &index)
{
	
	if (index.row() != -1) {
		lastRowNumberSelected = index.row();
		populateFormWithIndex(index);
	}
}

void OpticsDataMapper::typeChanged(const QString &newValue)
{
	QSqlRecord record = currentRecord();
	QModelIndexList list1 = typeTableModel->match(typeTableModel->index(0, 0), Qt::DisplayRole, record.value("optics_type_id"));
	Qt::MatchFlags flags = Qt::MatchExactly | Qt::MatchWrap;
	QModelIndexList list2 = typeTableModel->match(typeTableModel->index(0, 1), Qt::DisplayRole, newValue, 1, flags);
	
	if (!record.isEmpty() && !list1.isEmpty() && !list2.isEmpty()) {
		QSqlRecord typeRecord1 = typeTableModel->record(list1[0].row());
		QSqlRecord typeRecord2 = typeTableModel->record(list2[0].row());
		if (typeRecord1 != typeRecord2) {
			record.setValue("optics_type_id", typeRecord2.value("optics_type_id").toInt());
			tableModel->setRecord(lastRowNumberSelected, record);
			if (!tableModel->submit()) {
				qWarning() << "LogBook: could not update optic.  Error is: " << tableModel->lastError();
			}
		}
	}
	widget->opticsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void OpticsDataMapper::vendorChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("vendor") != widget->vendorLineEdit->text()) {
		record.setValue("vendor", widget->vendorLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update optic.  Error is: " << tableModel->lastError();
		}
	}

	widget->opticsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void OpticsDataMapper::vFlipChanged(int state)
{
	QSqlRecord record = currentRecord();
	bool isSelected = (state == Qt::Checked);
	if (!record.isEmpty() && record.value("vFlip").toBool() != isSelected) {
		record.setValue("vFlip", isSelected);
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update optic vFlip.  Error is: " << tableModel->lastError();
		}
	}
	widget->opticsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}


/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark protected methods
#endif
/* ********************************************************************* */
QSqlRecord OpticsDataMapper::currentRecord()
{ 
	if (lastRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModel->record(lastRowNumberSelected);
	}
}

void OpticsDataMapper::populateFormWithIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		QSqlRecord record = tableModel->record(index.row());
		widget->idLabel->setText(record.value("optic_id").toString());
		widget->modelLineEdit->setText(record.value("model").toString());
		widget->vendorLineEdit->setText(record.value("vendor").toString());
		
		QModelIndexList list1 = typeTableModel->match(typeTableModel->index(0, 0), Qt::DisplayRole, record.value("optics_type_id"));
		QSqlRecord typeRecord1 = typeTableModel->record(list1[0].row());
		widget->typeComboBox->setCurrentIndex(widget->typeComboBox->findText(typeRecord1.value("name").toString()));
		
		widget->focalLengthLineEdit->setText(record.value("focal_length").toString());
		widget->apertureLineEdit->setText(record.value("aperture").toString());
		widget->lightTransmissionLineEdit->setText(record.value("light_transmission").toString());
		widget->hFlipCheckBox->setChecked(record.value("hFlip").toBool());
		widget->vFlipCheckBox->setChecked(record.value("vFlip").toBool());
		setupConnections();
	}
}

void OpticsDataMapper::setupConnections()
{
	connect(widget->addOpticButton, SIGNAL(clicked()), this, SLOT(insertNewOptic()));
	connect(widget->deleteOpticButton, SIGNAL(clicked()), this, SLOT(deleteSelectedOptic()));
	connect(widget->opticsListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(opticSelected(const QModelIndex &)));
	connect(widget->opticsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(opticSelected(QModelIndex)));
	connect(widget->modelLineEdit, SIGNAL(editingFinished()), this, SLOT(modelChanged()));
	connect(widget->vendorLineEdit, SIGNAL(editingFinished()), this, SLOT(vendorChanged()));
	connect(widget->typeComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(typeChanged(const QString&)));
	connect(widget->focalLengthLineEdit, SIGNAL(editingFinished()), this, SLOT(focalLengthChanged()));
	connect(widget->apertureLineEdit, SIGNAL(editingFinished()), this, SLOT(apertureChanged()));
	connect(widget->lightTransmissionLineEdit, SIGNAL(editingFinished()), this, SLOT(lightTransmissionChanged()));
	connect(widget->hFlipCheckBox, SIGNAL(stateChanged(int)), this, SLOT(hFlipChanged(int)));
	connect(widget->vFlipCheckBox, SIGNAL(stateChanged(int)), this, SLOT(vFlipChanged(int)));
}

void OpticsDataMapper::teardownConnections()
{
	disconnect(widget->addOpticButton, SIGNAL(clicked()), this, SLOT(insertNewOptic()));
	disconnect(widget->deleteOpticButton, SIGNAL(clicked()), this, SLOT(deleteSelectedOptic()));
	disconnect(widget->opticsListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(opticSelected(const QModelIndex &)));
	disconnect(widget->opticsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			   this, SLOT(opticSelected(QModelIndex)));
	disconnect(widget->modelLineEdit, SIGNAL(editingFinished()), this, SLOT(modelChanged()));
	disconnect(widget->vendorLineEdit, SIGNAL(editingFinished()), this, SLOT(vendorChanged()));
	disconnect(widget->typeComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(typeChanged(const QString&)));
	disconnect(widget->focalLengthLineEdit, SIGNAL(editingFinished()), this, SLOT(focalLengthChanged()));
	disconnect(widget->apertureLineEdit, SIGNAL(editingFinished()), this, SLOT(apertureChanged()));
	disconnect(widget->lightTransmissionLineEdit, SIGNAL(editingFinished()), this, SLOT(lightTransmissionChanged()));
	disconnect(widget->hFlipCheckBox, SIGNAL(stateChanged(int)), this, SLOT(hFlipChanged(int)));
	disconnect(widget->vFlipCheckBox, SIGNAL(stateChanged(int)), this, SLOT(vFlipChanged(int)));
}

