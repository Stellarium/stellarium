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

#include "ObservationsDialog.hpp"

#include "FieldConcatModel.hpp"
#include "LogBook.hpp"
#include "LogBookCommon.hpp"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelModuleMgr.hpp"
#include "StelStyle.hpp"

#include "ui_ObservationsDialog.h"

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
ObservationsDialog::ObservationsDialog(QMap<QString, QSqlTableModel *> theTableModels, int sessionID)
{
	ui = new Ui_ObservationsDialog;
	tableModels = theTableModels;
	sessionKey = sessionID;
}

ObservationsDialog::~ObservationsDialog()
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
void ObservationsDialog::languageChanged()
{
	if (dialog) {
		ui->retranslateUi(dialog);
	}
}


void ObservationsDialog::styleChanged()
{
	// Nothing for now
}

void ObservationsDialog::updateStyle()
{
	if(dialog) {
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		const StelStyle pluginStyle = GETSTELMODULE(LogBook)->getModuleStyleSheet(gui->getStelStyle());
		dialog->setStyleSheet(pluginStyle.qtStyleSheet);
		ui->accessoriesTextEdit->document()->setDefaultStyleSheet(QString(pluginStyle.htmlStyleSheet));
		ui->notesTextEdit->document()->setDefaultStyleSheet(QString(pluginStyle.htmlStyleSheet));
	}
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Slot Methods
#endif
/* ********************************************************************* */
void ObservationsDialog::accessoriesTextChanged()
{
	QSqlRecord record = currentObservationRecord();
	
	if (!record.isEmpty() && record.value("accessories").toString() != ui->accessoriesTextEdit->toPlainText()) {
		record.setValue("accessories", ui->accessoriesTextEdit->toPlainText());
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void ObservationsDialog::barlowChanged(const QString &newValue)
{
	QSqlRecord record = currentObservationRecord();
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

void ObservationsDialog::beginDateTimeChanged(const QDateTime &datetime)
{
	QSqlRecord record = currentObservationRecord();
	if (!record.isEmpty() && record.value("begin") != datetime.toString("yyyy/MM/dd HH:mm")) {
		record.setValue("begin", datetime.toString("yyyy/MM/dd HH:mm"));
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void ObservationsDialog::deleteSelectedObservation()
{
	QModelIndex selection = ui->observationsListView->currentIndex();
	if (selection.row() != -1 && observationsModel->rowCount() > 1) {
		observationsModel->removeRows(selection.row(), 1);
		ui->observationsListView->setCurrentIndex(observationsModel->index(0, 1));
	}
}

void ObservationsDialog::endDateTimeChanged(const QDateTime &datetime)
{
	QSqlRecord record = currentObservationRecord();
	if (!record.isEmpty() && record.value("end") != datetime.toString("yyyy/MM/dd HH:mm")) {
		record.setValue("end", datetime.toString("yyyy/MM/dd HH:mm"));
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void ObservationsDialog::filterChanged(const QString &newValue)
{
	QSqlRecord record = currentObservationRecord();
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

void ObservationsDialog::imagerChanged(const QString &newValue)
{
	QSqlRecord record = currentObservationRecord();
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

void ObservationsDialog::insertNewObservation()
{
    QSqlField field1("observer_id", QVariant::Int);
    QSqlField field2("session_id", QVariant::Int);
    QSqlField field3("target_id", QVariant::Int);
    QSqlField field4("begin", QVariant::String);
    QSqlField field5("end", QVariant::String);
    QSqlField field6("optics_id", QVariant::Int);
	
	field1.setValue(QVariant(1));
	field2.setValue(QVariant(sessionKey));
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

void ObservationsDialog::limitingMagnitudeChanged(const QString &newValue)
{
	QSqlRecord record = currentObservationRecord();
	
	if (!record.isEmpty() && record.value("limiting_magnitude").toString() != newValue) {
		record.setValue("limiting_magnitude", newValue);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void ObservationsDialog::notesTextChanged()
{
	QSqlRecord record = currentObservationRecord();
	
	if (!record.isEmpty() && record.value("notes").toString() != ui->accessoriesTextEdit->toPlainText()) {
		record.setValue("notes", ui->accessoriesTextEdit->toPlainText());
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void ObservationsDialog::ocularChanged(const QString &newValue)
{
	QSqlRecord record = currentObservationRecord();
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

void ObservationsDialog::observerChanged(const QString &newValue)
{
	QSqlRecord record = currentObservationRecord();
	int newIndex = fieldModels[QString(OBSERVATIONS)]->idForDisplayString(newValue);
	
	if (!record.isEmpty() && record.value("observer_id").toInt() != newIndex) {
		record.setValue("observer_id", newIndex);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void ObservationsDialog::observationSelected(const QModelIndex &index)
{
	if (index.row() != -1) {
		lastObservationRowNumberSelected = index.row();
		populateFormWithObservationIndex(index);
	}
}

void ObservationsDialog::opticChanged(const QString &newValue)
{
	QSqlRecord record = currentObservationRecord();
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

void ObservationsDialog::seeingChanged(const QString &newValue)
{
	QSqlRecord record = currentObservationRecord();
	
	if (!record.isEmpty() && record.value("seeing").toString() != newValue) {
		record.setValue("seeing", newValue);
		tableModels[OBSERVATIONS]->setRecord(lastObservationRowNumberSelected, record);
		if (!tableModels[OBSERVATIONS]->submit()) {
			qWarning() << "LogBook: could not update observation.  Error is: " << tableModels[OBSERVATIONS]->lastError();
		}
	}
	ui->observationsListView->setCurrentIndex(tableModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));
}

void ObservationsDialog::targetChanged(const QString &newValue)
{
	QSqlRecord record = currentObservationRecord();
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

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Protected Methods
#endif
/* ********************************************************************* */
void ObservationsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	// All of the table models
	setupModels();
	setupConnections();

	// Now select the first one, which causes it to populate
	ui->observationsListView->setCurrentIndex(fieldModels[OBSERVATIONS]->index(lastObservationRowNumberSelected, 1));

	//Initialize the style
	updateStyle();
}

QSqlRecord ObservationsDialog::currentObservationRecord()
{
	if (lastObservationRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModels[OBSERVATIONS]->record(lastObservationRowNumberSelected);
	}
}

void ObservationsDialog::populateFormWithObservationIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		//We first need to handle the new session
		QSqlRecord record = tableModels[OBSERVATIONS]->record(index.row());
		ui->beginDateTimeEdit->setDateTime(QDateTime::fromString(record.value("begin").toString(), "yyyy/MM/dd HH:mm"));
		ui->endDateTimeEdit->setDateTime(QDateTime::fromString(record.value("end").toString(), "yyyy/MM/dd HH:mm"));
		ui->accessoriesTextEdit->setPlainText(record.value("accessories").toString());
		ui->notesTextEdit->setPlainText(record.value("notes").toString());
		
		QModelIndexList list1 
			= tableModels[OBSERVERS]->match(tableModels[OBSERVERS]->index(0, 0), Qt::DisplayRole, record.value("observer_id"));
		QSqlRecord typeRecord1 = tableModels[OBSERVERS]->record(list1[0].row());
		ui->observerComboBox->setCurrentIndex(ui->observerComboBox->
											  findText(fieldModels[QString(OBSERVERS)]->
													   displayStringForRecord(typeRecord1)));
		
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

void ObservationsDialog::setupConnections()
{
	connect(ui->addObservationButton, SIGNAL(clicked()), this, SLOT(insertNewObservation()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->observationsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(observationSelected(QModelIndex)));
		
	connect(ui->observerComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(observerChanged(const QString&)));
	connect(ui->opticComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(opticChanged(const QString&)));
	connect(ui->ocularComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(ocularChanged(const QString&)));
	connect(ui->barlowComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(barlowChanged(const QString&)));
	connect(ui->filterComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(filterChanged(const QString&)));
	connect(ui->targetComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(targetChanged(const QString&)));
	connect(ui->limitingMagComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(limitingMagnitudeChanged(const QString&)));
	connect(ui->seeingComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(seeingChanged(const QString&)));
	connect(ui->imagerComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(imagerChanged(const QString&)));
	connect(ui->beginDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(beginDateTimeChanged(const QDateTime&)));
	connect(ui->endDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(endDateTimeChanged(const QDateTime&)));
	connect(ui->accessoriesTextEdit, SIGNAL(editingFinished()), this, SLOT(accessoriesTextChanged()));
	connect(ui->notesTextEdit, SIGNAL(editingFinished()), this, SLOT(notesTextChanged()));
}

void ObservationsDialog::setupModels()
{
	QSqlDatabase db = QSqlDatabase::database("LogBook");
	// We want the relational table model so we can join the target name with the date in the display string
	observationsModel = new QSqlRelationalTableModel(this, db);
	observationsModel->setTable(OBSERVATIONS);
	observationsModel->setObjectName("Observations Table Model");
	observationsModel->setRelation(3, QSqlRelation(TARGETS, "target_id", "name"));
	observationsModel->setEditStrategy(QSqlTableModel::OnFieldChange);
	observationsModel->setFilter(QString("session_id = %1").arg(sessionKey));
	observationsModel->select();
	
	fieldModels[QString(OBSERVERS)] = new FieldConcatModel(tableModels[OBSERVERS], QStringList() << "surname" << "name", ", " , this);
	fieldModels[OPTICS] = new FieldConcatModel(tableModels[OPTICS], QStringList() << "model" << "vendor", " | " , this);
	fieldModels[OCULARS] = new FieldConcatModel(tableModels[OCULARS], QStringList() << "model" << "vendor", " | " , this);
	fieldModels[BARLOWS] = new FieldConcatModel(tableModels[BARLOWS], QStringList() << "model" << "vendor", " | " , this);
	fieldModels[FILTERS] = new FieldConcatModel(tableModels[FILTERS], QStringList() << "type" << "vendor", " | " , this);
	fieldModels[TARGETS] = new FieldConcatModel(tableModels[TARGETS], QStringList() << "name" << "alias", " | " , this);
	fieldModels[IMAGERS] = new FieldConcatModel(tableModels[IMAGERS], QStringList() << "model" << "vendor", " | " , this);
	fieldModels[OBSERVATIONS] = new FieldConcatModel(observationsModel, QStringList() << "name" << "begin", " at " , this);
	
	ui->observerComboBox->setModel(fieldModels[QString(OBSERVERS)]);
	ui->observerComboBox->setModelColumn(1);
	ui->observerComboBox->setCurrentIndex(-1);

	ui->opticComboBox->setModel(fieldModels[OPTICS]);
	ui->opticComboBox->setModelColumn(1);
	ui->opticComboBox->setCurrentIndex(-1);
	
	ui->ocularComboBox->setModel(fieldModels[OCULARS]);
	ui->ocularComboBox->setModelColumn(1);
	ui->ocularComboBox->setCurrentIndex(-1);
	
	ui->barlowComboBox->setModel(fieldModels[BARLOWS]);
	ui->barlowComboBox->setModelColumn(1);
	ui->barlowComboBox->setCurrentIndex(-1);
	
	ui->filterComboBox->setModel(fieldModels[FILTERS]);
	ui->filterComboBox->setModelColumn(1);
	ui->filterComboBox->setCurrentIndex(-1);
	
	ui->targetComboBox->setModel(fieldModels[TARGETS]);
	ui->targetComboBox->setModelColumn(1);
	ui->targetComboBox->setCurrentIndex(-1);
	
	ui->imagerComboBox->setModel(fieldModels[IMAGERS]);
	ui->imagerComboBox->setModelColumn(1);
	ui->imagerComboBox->setCurrentIndex(-1);

	ui->observationsListView->setModel(fieldModels[OBSERVATIONS]);
	ui->observationsListView->setModelColumn(1);
}

void ObservationsDialog::teardownConnections()
{
	disconnect(ui->addObservationButton, SIGNAL(clicked()), this, SLOT(insertNewObservation()));
	disconnect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	disconnect(ui->observationsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			   this, SLOT(observationSelected(QModelIndex)));
		
	disconnect(ui->observerComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(observerChanged(const QString&)));
	disconnect(ui->opticComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(opticChanged(const QString&)));
	disconnect(ui->ocularComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(ocularChanged(const QString&)));
	disconnect(ui->barlowComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(barlowChanged(const QString&)));
	disconnect(ui->filterComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(filterChanged(const QString&)));
	disconnect(ui->targetComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(targetChanged(const QString&)));
	disconnect(ui->limitingMagComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(limitingMagnitudeChanged(const QString&)));
	disconnect(ui->seeingComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(seeingChanged(const QString&)));
	disconnect(ui->imagerComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(imagerChanged(const QString&)));
	disconnect(ui->beginDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(beginDateTimeChanged(const QDateTime&)));
	disconnect(ui->endDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(endDateTimeChanged(const QDateTime&)));
	disconnect(ui->accessoriesTextEdit, SIGNAL(editingFinished()), this, SLOT(accessoriesTextChanged()));
	disconnect(ui->notesTextEdit, SIGNAL(editingFinished()), this, SLOT(notesTextChanged()));
}
