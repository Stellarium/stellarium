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

#include "LogBookDialog.hpp"
#include "LogBookCommon.hpp"
#include "FieldConcatModel.hpp"

#include "StelApp.hpp"
#include "StelMainGraphicsView.hpp"
#include "ui_LogBookDialog.h"

#include <QDateTime>
#include <QDebug>
#include <QModelIndex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlRelation>
#include <QSqlRelationalTableModel>
#include <QSqlTableModel>

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Creation & Destruction
#endif
/* ********************************************************************* */
LogBookDialog::LogBookDialog(QMap<QString, QSqlTableModel *> theTableModels)
{
	ui = new Ui_LogBookDialogForm;
	tableModels = theTableModels;
}

LogBookDialog::~LogBookDialog()
{
	delete ui;
	ui = NULL;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark StelModule Methods
#endif
/* ********************************************************************* */
void LogBookDialog::languageChanged()
{
	if (dialog) {
		ui->retranslateUi(dialog);
	}
}

void LogBookDialog::setStelStyle(const StelStyle& style)
{
	if(dialog) {
		dialog->setStyleSheet(style.qtStyleSheet);
	}
}

void LogBookDialog::styleChanged()
{
	// Nothing for now
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Slot Methods
#endif
/* ********************************************************************* */
void LogBookDialog::accessoriesTextChanged()
{
	QSqlRecord record = currentSessionRecord();
	
	if (!record.isEmpty() && record.value("accessories").toString() != ui->accessoriesTextEdit->toPlainText()) {
		record.setValue("accessories", ui->accessoriesTextEdit->toPlainText());
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->sessionsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::barlowChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	int newIndex = fieldModels[BARLOWS]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("barlow_id").toInt() != newIndex) {
		record.setValue("barlow_id", newIndex);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::closeWindow()
{
	setVisible(false);
	StelMainGraphicsView::getInstance().scene()->setActiveWindow(0);
}

void LogBookDialog::commentsTextChanged()
{
	QSqlRecord record = currentSessionRecord();
	
	if (!record.isEmpty() && record.value("comments").toString() != ui->sessionNotesTextEdit->toPlainText()) {
		record.setValue("comments", ui->sessionNotesTextEdit->toPlainText());
		tableModels[SESSIONS]->setRecord(lastSessionRowNumberSelected, record);
		if (!tableModels[SESSIONS]->submit()) {
			qWarning() << "LogBook: could not update session.  Error is: " << tableModels[SESSIONS]->lastError();
		}
	}
	ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(lastSessionRowNumberSelected, 1));
}

void LogBookDialog::deleteSelectedObservation()
{
	QModelIndex selection = ui->observationsListView->currentIndex();
	if (selection.row() != -1 && observationsModel->rowCount() > 1) {
		observationsModel->removeRows(selection.row(), 1);
		ui->observationsListView->setCurrentIndex(observationsModel->index(0, 1));
	}
}

void LogBookDialog::deleteSelectedSession()
{
	QModelIndex selection = ui->sessionsListView->currentIndex();
	if (selection.row() != -1 && tableModels[SESSIONS]->rowCount() > 1) {
		tableModels[SESSIONS]->removeRows(selection.row(), 1);
		ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(0, 1));
	}
}

void LogBookDialog::filterChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	int newIndex = fieldModels[FILTERS]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("filter_id").toInt() != newIndex) {
		record.setValue("filter_id", newIndex);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::imagerChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	int newIndex = fieldModels[IMAGERS]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("imager_id").toInt() != newIndex) {
		record.setValue("imager_id", newIndex);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::insertNewObservation()
{
    QSqlField field1("observer_id", QVariant::Int);
    QSqlField field2("session_id", QVariant::Int);
    QSqlField field3("target_id", QVariant::Int);
    QSqlField field4("begin", QVariant::String);
    QSqlField field5("end", QVariant::String);
    QSqlField field6("optics_id", QVariant::Int);

	field1.setValue(QVariant(1));
	field2.setValue(QVariant(1));
	field3.setValue(QVariant(1));
	QDateTime dateTime = QDateTime::currentDateTime();
	field4.setValue(QVariant(dateTime.toString("yyyy/MM/dd HH:mm")));
	dateTime = dateTime.addSecs(600);
	field5.setValue(QVariant(dateTime.toString("yyyy/MM/dd HH:mm")));
	field6.setValue(QVariant(1));

	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	newRecord.append(field3);
	newRecord.append(field4);
	newRecord.append(field5);
	newRecord.append(field6);
	
	if (observationsModel->insertRecord(-1, newRecord)) {
		ui->observationsListView->setCurrentIndex(observationsModel->index(observationsModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new observation.  The error is: " << observationsModel->lastError();
	}
}

void LogBookDialog::insertNewSession()
{
    QSqlField field1("begin", QVariant::String);
    QSqlField field2("end", QVariant::String);
    QSqlField field3("site_id", QVariant::Int);
    QSqlField field4("observer_id", QVariant::Int);
	QDateTime dateTime = QDateTime::currentDateTime();
	field1.setValue(QVariant(dateTime.toString("yyyy/MM/dd HH:mm")));
	dateTime = dateTime.addSecs(60*60);
	field2.setValue(QVariant(dateTime.toString("yyyy/MM/dd HH:mm")));
//	field3.setValue(QVariant(1));
//	field4.setValue(QVariant(1));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
//	newRecord.append(field3);
//	newRecord.append(field4);
	
	if (tableModels[SESSIONS]->insertRecord(-1, newRecord)) {
		ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(tableModels[SESSIONS]->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new session.  The error is: " << tableModels[SESSIONS]->lastError();
	}
}

void LogBookDialog::limitingMagnitudeChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	
	if (!record.isEmpty() && record.value("limiting_magnitude").toString() != newValue) {
		record.setValue("limiting_magnitude", newValue);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::notesTextChanged()
{
	QSqlRecord record = currentSessionRecord();
	
	if (!record.isEmpty() && record.value("notes").toString() != ui->accessoriesTextEdit->toPlainText()) {
		record.setValue("notes", ui->accessoriesTextEdit->toPlainText());
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->sessionsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::ocularChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	int newIndex = fieldModels[OCULARS]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("ocular_id").toInt() != newIndex) {
		record.setValue("ocular_id", newIndex);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::observationBeginDateTimeChanged(const QDateTime &datetime)
{
	QSqlRecord record = currentSessionRecord();
	if (!record.isEmpty() && record.value("begin") != datetime.toString("yyyy/MM/dd HH:mm")) {
		record.setValue("begin", datetime.toString("yyyy/MM/dd HH:mm"));
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::observationEndDateTimeChanged(const QDateTime &datetime)
{
	QSqlRecord record = currentSessionRecord();
	if (!record.isEmpty() && record.value("end") != datetime.toString("yyyy/MM/dd HH:mm")) {
		record.setValue("end", datetime.toString("yyyy/MM/dd HH:mm"));
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::observationObserverChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	int newIndex = fieldModels[QString(OBSERVATIONS) + QString(OBSERVERS)]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("observer_id").toInt() != newIndex) {
		record.setValue("observer_id", newIndex);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::observationSelected(const QModelIndex &index)
{
	if (index.row() != -1) {
		lastObservationRowNumberSelected = index.row();
		//Switch the to observation tab
		ui->tabWidget->setCurrentIndex(1);
		ui->observationTabWidget->setCurrentIndex(0);
		populateFormWithObservationIndex(index);
	}
}

void LogBookDialog::opticChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	int newIndex = fieldModels[OPTICS]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("optics_id").toInt() != newIndex) {
		record.setValue("optics_id", newIndex);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::seeingChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	
	if (!record.isEmpty() && record.value("seeing").toString() != newValue) {
		record.setValue("seeing", newValue);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::sessionBeginDateTimeChanged(const QDateTime &datetime)
{
	QSqlRecord record = currentSessionRecord();
	if (!record.isEmpty() && record.value("begin") != datetime.toString("yyyy/MM/dd HH:mm")) {
		record.setValue("begin", datetime.toString("yyyy/MM/dd HH:mm"));
		tableModels[SESSIONS]->setRecord(lastSessionRowNumberSelected, record);
		if (!tableModels[SESSIONS]->submit()) {
			qWarning() << "LogBook: could not update session.  Error is: " << tableModels[SESSIONS]->lastError();
		}
	}
	ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(lastSessionRowNumberSelected, 1));
}

void LogBookDialog::sessionEndDateTimeChanged(const QDateTime &datetime)
{
	QSqlRecord record = currentSessionRecord();
	if (!record.isEmpty() && record.value("end") != datetime.toString("yyyy/MM/dd HH:mm")) {
		record.setValue("end", datetime.toString("yyyy/MM/dd HH:mm"));
		tableModels[SESSIONS]->setRecord(lastSessionRowNumberSelected, record);
		if (!tableModels[SESSIONS]->submit()) {
			qWarning() << "LogBook: could not update session.  Error is: " << tableModels[SESSIONS]->lastError();
		}
	}
	ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(lastSessionRowNumberSelected, 1));
}

void LogBookDialog::sessionObserverChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	int newIndex = fieldModels[QString(SESSIONS) + QString(OBSERVERS)]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("observer_id").toInt() != newIndex) {
		record.setValue("observer_id", newIndex);
		tableModels[SESSIONS]->setRecord(lastSessionRowNumberSelected, record);
		if (!tableModels[SESSIONS]->submit()) {
			qWarning() << "LogBook: could not update session.  Error is: " << tableModels[SESSIONS]->lastError();
		}
	}
	ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(lastSessionRowNumberSelected, 1));
}

void LogBookDialog::sessionSelected(const QModelIndex &index)
{
	if (index.row() != -1) {
		lastSessionRowNumberSelected = index.row();
		//Switch the to session tab
		ui->tabWidget->setCurrentIndex(0);
		populateFormWithSessionIndex(index);
	}
}

void LogBookDialog::siteChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	int newIndex = fieldModels[SITES]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("site_id").toInt() != newIndex) {
		record.setValue("site_id", newIndex);
		tableModels[SESSIONS]->setRecord(lastSessionRowNumberSelected, record);
		if (!tableModels[SESSIONS]->submit()) {
			qWarning() << "LogBook: could not update session.  Error is: " << tableModels[SESSIONS]->lastError();
		}
	}
	ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(lastSessionRowNumberSelected, 1));
}

void LogBookDialog::targetChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	int newIndex = fieldModels[TARGETS]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("target_id").toInt() != newIndex) {
		record.setValue("target_id", newIndex);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void LogBookDialog::weatherTextChanged()
{
	QSqlRecord record = currentSessionRecord();
	
	if (!record.isEmpty() && record.value("weather").toString() != ui->weatherTextEdit->toPlainText()) {
		record.setValue("weather", ui->weatherTextEdit->toPlainText());
		tableModels[SESSIONS]->setRecord(lastSessionRowNumberSelected, record);
		if (!tableModels[SESSIONS]->submit()) {
			qWarning() << "LogBook: could not update session.  Error is: " << tableModels[SESSIONS]->lastError();
		}
	}
	ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(lastSessionRowNumberSelected, 1));
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Protected Methods
#endif
/* ********************************************************************* */
void LogBookDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// All of the table models
	setupModels();

	//Initialize the style
	setStelStyle(*StelApp::getInstance().getCurrentStelStyle());
	
	setupConnections();
	ui->sessionsListView->setCurrentIndex(fieldModels[SESSIONS]->index(0, 1));	
}

QSqlRecord LogBookDialog::currentObservationRecord()
{
	if (lastObservationRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModels[OBSERVATIONS]->record(lastObservationRowNumberSelected);
	}
}

QSqlRecord LogBookDialog::currentSessionRecord()
{
	if (lastSessionRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModels[SESSIONS]->record(lastSessionRowNumberSelected);
	}
}

void LogBookDialog::populateFormWithObservationIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		//We first need to handle the new session
		QSqlRecord record = tableModels[OBSERVATIONS]->record(index.row());
		ui->observationBeginDateTimeEdit->setDateTime(QDateTime::fromString(record.value("begin").toString(), "yyyy/MM/dd HH:mm"));
		ui->observationEndDateTimeEdit->setDateTime(QDateTime::fromString(record.value("end").toString(), "yyyy/MM/dd HH:mm"));
		ui->accessoriesTextEdit->setPlainText(record.value("accessories").toString());
		ui->observationNotesTextEdit->setPlainText(record.value("notes").toString());
		
		QModelIndexList list1 
		= tableModels[OBSERVERS]->match(tableModels[OBSERVERS]->index(0, 0), Qt::DisplayRole, record.value("observer_id"));
		QSqlRecord typeRecord1 = tableModels[OBSERVERS]->record(list1[0].row());
		ui->observationObserverComboBox->setCurrentIndex(ui->sessionObserverComboBox
														 ->findText(fieldModels[QString(SESSIONS) + QString(OBSERVERS)]->displayStringForRecord(typeRecord1)));
		
		list1 = tableModels[OPTICS]->match(tableModels[OPTICS]->index(0, 0), Qt::DisplayRole, record.value("optics_id"));
		typeRecord1 = tableModels[OPTICS]->record(list1[0].row());
		ui->opticComboBox->setCurrentIndex(ui->opticComboBox->findText(fieldModels[OPTICS]->displayStringForRecord(typeRecord1)));
		
		list1 = tableModels[OCULARS]->match(tableModels[OCULARS]->index(0, 0), Qt::DisplayRole, record.value("ocular_id"));
		typeRecord1 = tableModels[OCULARS]->record(list1[0].row());
		ui->ocularComboBox->setCurrentIndex(ui->ocularComboBox->findText(fieldModels[OCULARS]->displayStringForRecord(typeRecord1)));
		
		list1 = tableModels[BARLOWS]->match(tableModels[BARLOWS]->index(0, 0), Qt::DisplayRole, record.value("barlow_id"));
		typeRecord1 = tableModels[BARLOWS]->record(list1[0].row());
		ui->barlowComboBox->setCurrentIndex(ui->barlowComboBox->findText(fieldModels[BARLOWS]->displayStringForRecord(typeRecord1)));
		
		list1 = tableModels[FILTERS]->match(tableModels[FILTERS]->index(0, 0), Qt::DisplayRole, record.value("filter_id"));
		typeRecord1 = tableModels[FILTERS]->record(list1[0].row());
		ui->filterComboBox->setCurrentIndex(ui->filterComboBox->findText(fieldModels[FILTERS]->displayStringForRecord(typeRecord1)));
		
		list1 = tableModels[TARGETS]->match(tableModels[TARGETS]->index(0, 0), Qt::DisplayRole, record.value("target_id"));
		typeRecord1 = tableModels[TARGETS]->record(list1[0].row());
		ui->targetComboBox->setCurrentIndex(ui->targetComboBox->findText(fieldModels[TARGETS]->displayStringForRecord(typeRecord1)));
		
		list1 = tableModels[IMAGERS]->match(tableModels[IMAGERS]->index(0, 0), Qt::DisplayRole, record.value("imager_id"));
		typeRecord1 = tableModels[IMAGERS]->record(list1[0].row());
		ui->imagerComboBox->setCurrentIndex(ui->imagerComboBox->findText(fieldModels[IMAGERS]->displayStringForRecord(typeRecord1)));
		
		ui->limitingMagComboBox->setCurrentIndex(ui->limitingMagComboBox->findText(record.value("limiting_magnitude").toString()));
		ui->seeingComboBox->setCurrentIndex(ui->seeingComboBox->findText(record.value("seeing").toString()));
		
		setupConnections();
	}
}

void LogBookDialog::populateFormWithSessionIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		//We first need to handle the new session
		QSqlRecord record = tableModels[SESSIONS]->record(index.row());
		ui->beginSessionDateTimeEdit->setDateTime(QDateTime::fromString(record.value("begin").toString(), "yyyy/MM/dd HH:mm"));
		ui->endSessionDateTimeEdit->setDateTime(QDateTime::fromString(record.value("end").toString(), "yyyy/MM/dd HH:mm"));
		ui->weatherTextEdit->setPlainText(record.value("weather").toString());
		ui->sessionNotesTextEdit->setPlainText(record.value("comments").toString());
		
		QModelIndexList list1 
		= tableModels[OBSERVERS]->match(tableModels[OBSERVERS]->index(0, 0), Qt::DisplayRole, record.value("observer_id"));
		QSqlRecord typeRecord1 = tableModels[OBSERVERS]->record(list1[0].row());
		ui->sessionObserverComboBox
		->setCurrentIndex(ui->sessionObserverComboBox
						  ->findText(fieldModels[QString(SESSIONS) + QString(OBSERVERS)]->displayStringForRecord(typeRecord1)));
		
		list1 = tableModels[SITES]->match(tableModels[SITES]->index(0, 0), Qt::DisplayRole, record.value("site_id"));
		typeRecord1 = tableModels[SITES]->record(list1[0].row());
		ui->siteComboBox->setCurrentIndex(ui->siteComboBox->findText(typeRecord1.value("name").toString()));
		setupConnections();
		
		// We want the observations change to trigger
		// First, we re-filter the model
		observationsListModel->setFilter(QString("session_id = %1").arg(record.value("session_id").toInt()));
		lastObservationRowNumberSelected = 1;
		// Now select the first one, which causes it to populate
		ui->observationsListView->setCurrentIndex(observationsListModel->index(lastObservationRowNumberSelected, 1));
	}
}

void LogBookDialog::setupConnections()
{
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->addSessionPushButton, SIGNAL(clicked()), this, SLOT(insertNewSession()));
	connect(ui->sessionsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(sessionSelected(QModelIndex)));
	connect(ui->observationsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(observationSelected(QModelIndex)));

	connect(ui->beginSessionDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), 
			this, SLOT(sessionBeginDateTimeChanged(const QDateTime&)));
	connect(ui->endSessionDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(sessionEndDateTimeChanged(const QDateTime&)));
	connect(ui->sessionObserverComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(sessionObserverChanged(const QString&)));
	connect(ui->siteComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(siteChanged(const QString&)));
	connect(ui->weatherTextEdit, SIGNAL(editingFinished()), this, SLOT(weatherTextChanged()));
	connect(ui->sessionNotesTextEdit, SIGNAL(editingFinished()), this, SLOT(commentsTextChanged()));

	connect(ui->observationObserverComboBox, SIGNAL(currentIndexChanged(const QString&)), 
			this, SLOT(observationObserverChanged(const QString&)));
	connect(ui->opticComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(opticChanged(const QString&)));
	connect(ui->ocularComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(ocularChanged(const QString&)));
	connect(ui->barlowComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(barlowChanged(const QString&)));
	connect(ui->filterComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(filterChanged(const QString&)));
	connect(ui->targetComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(targetChanged(const QString&)));
	connect(ui->limitingMagComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(limitingMagnitudeChanged(const QString&)));
	connect(ui->seeingComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(seeingChanged(const QString&)));
	connect(ui->imagerComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(imagerChanged(const QString&)));
	connect(ui->observationBeginDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), 
			this, SLOT(observationBeginDateTimeChanged(const QDateTime&)));
	connect(ui->observationEndDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), 
			this, SLOT(observationEndDateTimeChanged(const QDateTime&)));
	connect(ui->accessoriesTextEdit, SIGNAL(editingFinished()), this, SLOT(accessoriesTextChanged()));
	connect(ui->observationNotesTextEdit, SIGNAL(editingFinished()), this, SLOT(notesTextChanged()));
}

void LogBookDialog::setupModels()
{
	fieldModels[QString(SESSIONS) + QString(OBSERVERS)] 
		= new FieldConcatModel(tableModels[OBSERVERS], QStringList() << "surname" << "name", ", " , this);
	fieldModels[QString(OBSERVATIONS) + QString(OBSERVERS)] 
		= new FieldConcatModel(tableModels[OBSERVERS], QStringList() << "surname" << "name", ", " , this);
	fieldModels[OPTICS] = new FieldConcatModel(tableModels[OPTICS], QStringList() << "model" << "vendor", " | " , this);
	fieldModels[OCULARS] = new FieldConcatModel(tableModels[OCULARS], QStringList() << "model" << "vendor", " | " , this);
	fieldModels[BARLOWS] = new FieldConcatModel(tableModels[BARLOWS], QStringList() << "model" << "vendor", " | " , this);
	fieldModels[FILTERS] = new FieldConcatModel(tableModels[FILTERS], QStringList() << "type" << "vendor", " | " , this);
	fieldModels[TARGETS] = new FieldConcatModel(tableModels[TARGETS], QStringList() << "name" << "alias", " | " , this);
	fieldModels[IMAGERS] = new FieldConcatModel(tableModels[IMAGERS], QStringList() << "model" << "vendor", " | " , this);
	fieldModels[SESSIONS] = new FieldConcatModel(tableModels[SESSIONS], QStringList() << "begin" << "end", " to " , this);

	ui->siteComboBox->setModel(tableModels[SITES]);
	ui->siteComboBox->setModelColumn(1);
	
	ui->sessionObserverComboBox->setModel(fieldModels[QString(SESSIONS) + QString(OBSERVERS)]);
	ui->sessionObserverComboBox->setModelColumn(1);
	
	ui->observationObserverComboBox->setModel(fieldModels[QString(OBSERVATIONS) + QString(OBSERVERS)]);
	ui->observationObserverComboBox->setModelColumn(1);
	
	ui->opticComboBox->setModel(fieldModels[OPTICS]);
	ui->opticComboBox->setModelColumn(1);

	ui->ocularComboBox->setModel(fieldModels[OCULARS]);
	ui->ocularComboBox->setModelColumn(1);

	ui->barlowComboBox->setModel(fieldModels[BARLOWS]);
	ui->barlowComboBox->setModelColumn(1);

	ui->filterComboBox->setModel(fieldModels[FILTERS]);
	ui->filterComboBox->setModelColumn(1);

	ui->targetComboBox->setModel(fieldModels[TARGETS]);
	ui->targetComboBox->setModelColumn(1);

	ui->imagerComboBox->setModel(fieldModels[IMAGERS]);
	ui->imagerComboBox->setModelColumn(1);
	
	ui->sessionsListView->setModel(fieldModels[SESSIONS]);
	ui->sessionsListView->setModelColumn(1);
	
	QSqlDatabase db = QSqlDatabase::database("LogBook");
	observationsModel = new QSqlRelationalTableModel(this, db);
	observationsModel->setTable(OBSERVATIONS);
	observationsModel->setRelation(3, QSqlRelation(TARGETS, "target_id", "name"));
	observationsListModel = new FieldConcatModel(observationsModel, QStringList() << "name" << "begin", " at " , this);
	ui->observationsListView->setModel(observationsListModel);
	ui->observationsListView->setModelColumn(1);
}

void LogBookDialog::teardownConnections()
{
	disconnect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	disconnect(ui->addSessionPushButton, SIGNAL(clicked()), this, SLOT(insertNewSession()));
	disconnect(ui->sessionsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(sessionSelected(QModelIndex)));
	disconnect(ui->observationsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(observationSelected(QModelIndex)));
	
	disconnect(ui->beginSessionDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), 
			   this, SLOT(sessionBeginDateTimeChanged(const QDateTime&)));
	disconnect(ui->endSessionDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(sessionEndDateTimeChanged(const QDateTime&)));
	disconnect(ui->sessionObserverComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(sessionObserverChanged(const QString&)));
	disconnect(ui->siteComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(siteChanged(const QString&)));
	disconnect(ui->weatherTextEdit, SIGNAL(editingFinished()), this, SLOT(weatherTextChanged()));
	disconnect(ui->sessionNotesTextEdit, SIGNAL(editingFinished()), this, SLOT(commentsTextChanged()));
	
	disconnect(ui->observationObserverComboBox, SIGNAL(currentIndexChanged(const QString&)), 
			   this, SLOT(observationObserverChanged(const QString&)));
	disconnect(ui->opticComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(opticChanged(const QString&)));
	disconnect(ui->ocularComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(ocularChanged(const QString&)));
	disconnect(ui->barlowComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(barlowChanged(const QString&)));
	disconnect(ui->filterComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(filterChanged(const QString&)));
	disconnect(ui->targetComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(targetChanged(const QString&)));
	disconnect(ui->limitingMagComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(limitingMagnitudeChanged(const QString&)));
	disconnect(ui->seeingComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(seeingChanged(const QString&)));
	disconnect(ui->imagerComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(imagerChanged(const QString&)));
	disconnect(ui->observationBeginDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), 
			   this, SLOT(observationBeginDateTimeChanged(const QDateTime&)));
	disconnect(ui->observationEndDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), 
			   this, SLOT(observationEndDateTimeChanged(const QDateTime&)));
	disconnect(ui->accessoriesTextEdit, SIGNAL(editingFinished()), this, SLOT(accessoriesTextChanged()));
	disconnect(ui->observationNotesTextEdit, SIGNAL(editingFinished()), this, SLOT(notesTextChanged()));
}
