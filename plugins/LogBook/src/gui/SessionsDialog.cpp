/*
 * Copyright (C) 2010 Timothy Reaves
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

#include "SessionsDialog.hpp"

#include "FieldConcatModel.hpp"
#include "LogBook.hpp"
#include "LogBookCommon.hpp"
#include "ObservationsDialog.hpp"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelModuleMgr.hpp"
#include "StelStyle.hpp"

#include "ui_SessionsDialog.h"

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
SessionsDialog::SessionsDialog(QMap<QString, QSqlTableModel *> theTableModels)
{
	ui = new Ui_SessionsDialog;
	tableModels = theTableModels;
}

SessionsDialog::~SessionsDialog()
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
void SessionsDialog::languageChanged()
{
	if (dialog) {
		ui->retranslateUi(dialog);
	}
}

void SessionsDialog::styleChanged()
{
	// Nothing for now
}

void SessionsDialog::updateStyle()
{
	if(dialog) {
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		const StelStyle pluginStyle = GETSTELMODULE(LogBook)->getModuleStyleSheet(gui->getStelStyle());
		dialog->setStyleSheet(pluginStyle.qtStyleSheet);
		ui->commentsTextEdit->document()->setDefaultStyleSheet(QString(pluginStyle.htmlStyleSheet));
		ui->weatherTextEdit->document()->setDefaultStyleSheet(QString(pluginStyle.htmlStyleSheet));
	}
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Slot Methods
#endif
/* ********************************************************************* */
void SessionsDialog::commentsTextChanged()
{
	QSqlRecord record = currentSessionRecord();
	
	if (!record.isEmpty() && record.value("comments").toString() != ui->commentsTextEdit->toPlainText()) {
		record.setValue("comments", ui->commentsTextEdit->toPlainText());
		tableModels[SESSIONS]->setRecord(lastSessionRowNumberSelected, record);
		if (!tableModels[SESSIONS]->submit()) {
			qWarning() << "LogBook: could not update session.  Error is: " << tableModels[SESSIONS]->lastError();
		}
		ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(lastSessionRowNumberSelected, 1));
	}
}

void SessionsDialog::deleteSelectedSession()
{
	QModelIndex selection = ui->sessionsListView->currentIndex();
	if (selection.row() != -1 && tableModels[SESSIONS]->rowCount() > 1) {
		tableModels[SESSIONS]->removeRows(selection.row(), 1);
		ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(0, 1));
	}
}

void SessionsDialog::insertNewSession()
{
    QSqlField field1("begin", QVariant::String);
    QSqlField field2("end", QVariant::String);
    QSqlField field3("site_id", QVariant::Int);
    QSqlField field4("observer_id", QVariant::Int);
	QDateTime dateTime = QDateTime::currentDateTime();
	field1.setValue(QVariant(dateTime.toString("yyyy/MM/dd HH:mm")));
	dateTime = dateTime.addSecs(60*60);
	field2.setValue(QVariant(dateTime.toString("yyyy/MM/dd HH:mm")));
	field3.setValue(QVariant(1));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	newRecord.append(field3);
	
	if (sessionsModel->insertRecord(-1, newRecord)) {
		ui->sessionsListView->setCurrentIndex(sessionsModel->index(sessionsModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new session.  The error is: " << sessionsModel->lastError();
	}
}

void SessionsDialog::beginDateTimeChanged(const QDateTime &datetime)
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

void SessionsDialog::endDateTimeChanged(const QDateTime &datetime)
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

void SessionsDialog::observationWindowClosed(StelDialogLogBook* theDialog)
{
	disconnect(theDialog, SIGNAL(dialogClosed(StelDialogLogBook *)), this, SLOT(observationWindowClosed(StelDialogLogBook *)));
	delete theDialog;
}

void SessionsDialog::observerChanged(const QString &newValue)
{
	QSqlRecord record = currentSessionRecord();
	int newIndex = fieldModels[QString(OBSERVERS)]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("observer_id").toInt() != newIndex) {
		record.setValue("observer_id", newIndex);
		tableModels[SESSIONS]->setRecord(lastSessionRowNumberSelected, record);
		if (!tableModels[SESSIONS]->submit()) {
			qWarning() << "LogBook: could not update session.  Error is: " << tableModels[SESSIONS]->lastError();
		}
	}
	ui->sessionsListView->setCurrentIndex(tableModels[SESSIONS]->index(lastSessionRowNumberSelected, 1));
}

void SessionsDialog::openObservations()
{
	int sessionID = currentSessionRecord().value("session_id").toInt();
	ObservationsDialog* observationsDialog = new ObservationsDialog(tableModels, sessionID);
	connect(observationsDialog, SIGNAL(dialogClosed(StelDialogLogBook *)), this, SLOT(observationWindowClosed(StelDialogLogBook *)));
	observationsDialog->updateStyle();
	observationsDialog->setVisible(true);
}

void SessionsDialog::sessionSelected(const QModelIndex &index)
{
	if (index.row() != -1) {
		lastSessionRowNumberSelected = index.row();
		populateFormWithSessionIndex(index);
	}
}

void SessionsDialog::siteChanged(const QString &newValue)
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

void SessionsDialog::weatherTextChanged()
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
void SessionsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	// All of the table models
	setupModels();
	setupConnections();
	ui->sessionsListView->setCurrentIndex(fieldModels[SESSIONS]->index(0, 1));	

	//Initialize the style
	updateStyle();
}

QSqlRecord SessionsDialog::currentSessionRecord()
{
	if (lastSessionRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModels[SESSIONS]->record(lastSessionRowNumberSelected);
	}
}

void SessionsDialog::populateFormWithSessionIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		//We first need to handle the new session
		QSqlRecord record = tableModels[SESSIONS]->record(index.row());
		ui->beginDateTimeEdit->setDateTime(QDateTime::fromString(record.value("begin").toString(), "yyyy/MM/dd HH:mm"));
		ui->endDateTimeEdit->setDateTime(QDateTime::fromString(record.value("end").toString(), "yyyy/MM/dd HH:mm"));
		ui->weatherTextEdit->setPlainText(record.value("weather").toString());
		ui->commentsTextEdit->setPlainText(record.value("comments").toString());
		
		QModelIndexList list1 
			= tableModels[OBSERVERS]->match(tableModels[OBSERVERS]->index(0, 0), Qt::DisplayRole, record.value("observer_id"));
		QSqlRecord typeRecord1 = tableModels[OBSERVERS]->record(list1[0].row());
		ui->observerComboBox
			->setCurrentIndex(ui->observerComboBox
							  ->findText(fieldModels[QString(OBSERVERS)]->displayStringForRecord(typeRecord1)));
		
		list1 = tableModels[SITES]->match(tableModels[SITES]->index(0, 0), Qt::DisplayRole, record.value("site_id"));
		typeRecord1 = tableModels[SITES]->record(list1[0].row());
		ui->siteComboBox->setCurrentIndex(ui->siteComboBox->findText(typeRecord1.value("name").toString()));
		setupConnections();
		
	}
}

void SessionsDialog::setupConnections()
{
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->addSessionButton, SIGNAL(clicked()), this, SLOT(insertNewSession()));
	connect(ui->openSessionButton, SIGNAL(clicked()), this, SLOT(openObservations()));
	connect(ui->sessionsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),  this, 
			SLOT(sessionSelected(QModelIndex)));
	
	connect(ui->beginDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(beginDateTimeChanged(const QDateTime&)));
	connect(ui->endDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(endDateTimeChanged(const QDateTime&)));
	connect(ui->observerComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(observerChanged(const QString&)));
	connect(ui->siteComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(siteChanged(const QString&)));
	connect(ui->weatherTextEdit, SIGNAL(editingFinished()), this, SLOT(weatherTextChanged()));
	connect(ui->commentsTextEdit, SIGNAL(editingFinished()), this, SLOT(commentsTextChanged()));	
}

void SessionsDialog::setupModels()
{
	QSqlDatabase db = QSqlDatabase::database("LogBook");
	// We want the relational table model so we can join the target name with the date in the display string
	sessionsModel = new QSqlRelationalTableModel(this, db);
	sessionsModel->setTable(SESSIONS);
	sessionsModel->setObjectName("Sessions Table Model");
	sessionsModel->setRelation(3, QSqlRelation(SITES, "site_id", "name"));
	sessionsModel->setEditStrategy(QSqlTableModel::OnFieldChange);
	sessionsModel->select();

	fieldModels[QString(OBSERVERS)] = new FieldConcatModel(tableModels[OBSERVERS], QStringList() << "surname" << "name", ", " , this);
	fieldModels[SESSIONS] = new FieldConcatModel(sessionsModel, QStringList() << "name" << "begin", " on " , this);

	ui->siteComboBox->setModel(tableModels[SITES]);
	ui->siteComboBox->setModelColumn(1);

	ui->observerComboBox->setModel(fieldModels[QString(OBSERVERS)]);
	ui->observerComboBox->setModelColumn(1);

	ui->sessionsListView->setModel(fieldModels[SESSIONS]);
	ui->sessionsListView->setModelColumn(1);

}

void SessionsDialog::teardownConnections()
{
	disconnect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	disconnect(ui->addSessionButton, SIGNAL(clicked()), this, SLOT(insertNewSession()));
	disconnect(ui->openSessionButton, SIGNAL(clicked()), this, SLOT(openObservations()));
	disconnect(ui->sessionsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), this, 
			   SLOT(sessionSelected(QModelIndex)));
	
	disconnect(ui->beginDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(beginDateTimeChanged(const QDateTime&)));
	disconnect(ui->endDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(endDateTimeChanged(const QDateTime&)));
	disconnect(ui->observerComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(observerChanged(const QString&)));
	disconnect(ui->siteComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(siteChanged(const QString&)));
	disconnect(ui->weatherTextEdit, SIGNAL(editingFinished()), this, SLOT(weatherTextChanged()));
	disconnect(ui->commentsTextEdit, SIGNAL(editingFinished()), this, SLOT(commentsTextChanged()));	
}
