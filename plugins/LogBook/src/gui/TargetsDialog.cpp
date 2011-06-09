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

#include "TargetsDialog.hpp"
#include "LogBookCommon.hpp"
#include "LogBook.hpp"
#include "ui_TargetsDialog.h"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelObjectType.hpp"
#include "StelMainScriptAPI.hpp"
#include "StelStyle.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QList>
#include <QListIterator>
#include <QModelIndex>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QVariant>


TargetsDialog::TargetsDialog(QMap<QString, QSqlTableModel *> theTableModels)
{
	tableModel = theTableModels[TARGETS];
	ui = new Ui_TargetsDialogForm();
	typeTableModel = theTableModels[TARGET_TYPES];
}

TargetsDialog::~TargetsDialog()
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
void TargetsDialog::languageChanged()
{
	if (dialog) {
		ui->retranslateUi(dialog);
	}
}

void TargetsDialog::styleChanged()
{
	// Nothing for now
}

void TargetsDialog::updateStyle()
{
	if(dialog) {
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		const StelStyle pluginStyle = GETSTELMODULE(LogBook)->getModuleStyleSheet(gui->getStelStyle());
		dialog->setStyleSheet(pluginStyle.qtStyleSheet);
		ui->notesTextEdit->document()->setDefaultStyleSheet(QString(pluginStyle.htmlStyleSheet));
	}
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Slot Methods
#endif
/* ********************************************************************* */
void TargetsDialog::aliasChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("alias") != ui->aliasLineEdit->text()) {
		record.setValue("alias", ui->aliasLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update target.  Error is: " << tableModel->lastError();
		}
	}
	ui->targetsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void TargetsDialog::catalogNumberChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("catalog_number") != ui->catalogNumberLineEdit->text()) {
		record.setValue("catalog_number", ui->catalogNumberLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update target.  Error is: " << tableModel->lastError();
		}
	}
	ui->targetsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void TargetsDialog::closeWindow()
{
	setVisible(false);
	StelMainGraphicsView::getInstance().scene()->setActiveWindow(0);
}

void TargetsDialog::createTargetFromSelection()
{
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected()) {
		QList<StelObjectP> selectedObjects = StelApp::getInstance().getStelObjectMgr().getSelectedObject();
		QListIterator<StelObjectP> objectIterator(selectedObjects);
		StelCore* core = StelApp::getInstance().getCore();
		while (objectIterator.hasNext()) {
			StelObjectP stelObject = objectIterator.next();
			QString type = stelObject->getType();
			QString englishName = stelObject->getEnglishName();
			if (englishName == "") {
				englishName = "No Name";
			}
			float magnatude = stelObject->getVMagnitude(core);
			double angularSize = stelObject->getAngularSize(core);
			
			Vec3d pos;
			double ra, dec;
			// ra/dec in J2000
			pos = stelObject->getJ2000EquatorialPos(core);
			StelUtils::rectToSphe(&ra, &dec, pos);
			
		    QSqlField field1("name", QVariant::String);
			QSqlField field2("right_ascension", QVariant::Double);
			QSqlField field3("declination", QVariant::Double);
			QSqlField field4("target_type_id", QVariant::Int);
			QSqlField field5("magnitude", QVariant::Double);
			QSqlField field6("size", QVariant::Double);
			field1.setValue(QVariant(englishName));
			field2.setValue(QVariant(ra));
			field3.setValue(QVariant(dec));
			field4.setValue(QVariant(1));
			field5.setValue(QVariant(magnatude));
			field6.setValue(QVariant(angularSize));
			QSqlRecord newRecord = QSqlRecord();
			newRecord.append(field1);
			newRecord.append(field2);
			newRecord.append(field3);
			newRecord.append(field4);
			newRecord.append(field5);
			newRecord.append(field6);
			
			if (tableModel->insertRecord(-1, newRecord)) {
				ui->targetsListView->setCurrentIndex(tableModel->index(tableModel->rowCount() - 1, 1));
			} else {
				qWarning() << "LogBook: could not insert new target.  The error is: " << tableModel->lastError();
			}
		}
	} else {
		qDebug() << "====> Nothing selected.";
	}
}	

void TargetsDialog::declinationChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("declination") != ui->j2000DecLineEdit->text().toDouble()) {
		record.setValue("declination", ui->j2000DecLineEdit->text().toDouble());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update target.  Error is: " << tableModel->lastError();
		}
	}
	ui->targetsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void TargetsDialog::deleteSelectedTarget()
{
	QModelIndex selection = ui->targetsListView->currentIndex();
	if (selection.row() != -1 && tableModel->rowCount() > 1) {
		tableModel->removeRows(selection.row(), 1);
		ui->targetsListView->setCurrentIndex(tableModel->index(0, 1));
	}
}

void TargetsDialog::distanceChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("distance") != ui->distanceLineEdit->text().toDouble()) {
		record.setValue("distance", ui->distanceLineEdit->text().toDouble());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update target.  Error is: " << tableModel->lastError();
		}
	}
	ui->targetsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void TargetsDialog::insertNewTarget()
{
    QSqlField field1("name", QVariant::String);
    QSqlField field3("right_ascension", QVariant::Double);
    QSqlField field4("declination", QVariant::Double);
    QSqlField field6("target_type_id", QVariant::Int);
	field1.setValue(QVariant("name"));
	field3.setValue(QVariant(0.0));
	field4.setValue(QVariant(0.0));
	field6.setValue(QVariant(1));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field3);
	newRecord.append(field4);
	newRecord.append(field6);
	
	if (tableModel->insertRecord(-1, newRecord)) {
		ui->targetsListView->setCurrentIndex(tableModel->index(tableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "LogBook: could not insert new target.  The error is: " << tableModel->lastError();
	}
}

void TargetsDialog::magnitudeChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("magnitude") != ui->magnitudeLineEdit->text().toDouble()) {
		record.setValue("magnitude", ui->magnitudeLineEdit->text().toDouble());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update target.  Error is: " << tableModel->lastError();
		}
	}
	ui->targetsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void TargetsDialog::nameChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("name") != ui->nameLineEdit->text()) {
		record.setValue("name", ui->nameLineEdit->text());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update target.  Error is: " << tableModel->lastError();
		}
	}
	ui->targetsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void TargetsDialog::notesChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("notes") != ui->notesTextEdit->toPlainText()) {
		record.setValue("notes", ui->notesTextEdit->toPlainText());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update target.  Error is: " << tableModel->lastError();
		}
	}
	ui->targetsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void TargetsDialog::rightAscentionChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("right_ascention") != ui->j2000RALineEdit->text().toDouble()) {
		record.setValue("right_ascention", ui->j2000RALineEdit->text().toDouble());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update target.  Error is: " << tableModel->lastError();
		}
	}
	ui->targetsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void TargetsDialog::sizeChanged()
{
	QSqlRecord record = currentRecord();
	if (!record.isEmpty() && record.value("size") != ui->sizeLineEdit->text().toDouble()) {
		record.setValue("size", ui->sizeLineEdit->text().toDouble());
		tableModel->setRecord(lastRowNumberSelected, record);
		if (!tableModel->submit()) {
			qWarning() << "LogBook: could not update target.  Error is: " << tableModel->lastError();
		}
	}
	ui->targetsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void TargetsDialog::targetSelected(const QModelIndex &index)
{
	if (index.row() != -1) {
		lastRowNumberSelected = index.row();
		populateFormWithIndex(index);
	}
}

void TargetsDialog::targetTypeChanged(const QString &newValue)
{
	QSqlRecord record = currentRecord();
	QModelIndexList list1 = typeTableModel->match(typeTableModel->index(0, 0), Qt::DisplayRole, record.value("target_type_id"));
	Qt::MatchFlags flags = Qt::MatchExactly | Qt::MatchWrap;
	QModelIndexList list2 = typeTableModel->match(typeTableModel->index(0, 1), Qt::DisplayRole, newValue, 1, flags);

	if (!record.isEmpty() && !list1.isEmpty() && !list2.isEmpty()) {
		QSqlRecord typeRecord1 = typeTableModel->record(list1[0].row());
		QSqlRecord typeRecord2 = typeTableModel->record(list2[0].row());
		if (typeRecord1.value("target_type_id").toInt() != typeRecord2.value("target_type_id").toInt()) {
			record.setValue("target_type_id", typeRecord2.value("target_type_id").toInt());
			tableModel->setRecord(lastRowNumberSelected, record);
			if (!tableModel->submit()) {
				qWarning() << "LogBook: could not update target.  Error is: " << tableModel->lastError();
			}
		}
	}
	ui->targetsListView->setCurrentIndex(tableModel->index(lastRowNumberSelected, 1));
}

void TargetsDialog::updateRTValues()
{
/*	
	// ra/dec
	pos = stelObject->getEquinoxEquatorialPos(nav);
	StelUtils::rectToSphe(&ra, &dec, pos);
	qDebug() << tr("====> RA/DE: %1/%2").arg(StelUtils::radToHmsStr(ra,true), StelUtils::radToDmsStr(dec,true));
	
	// altitude/azimuth
	pos = stelObject->getAltAzPos(nav);
	StelUtils::rectToSphe(&azi, &alt, pos);
	qDebug() << "====> AltAzPos:: " << alt << ", " << azi;
 */
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Protected Methods
#endif
/* ********************************************************************* */
void TargetsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->targetsListView->setModel(tableModel);
	ui->targetsListView->setModelColumn(1);
	ui->typeComboBox->setModel(typeTableModel);
	ui->typeComboBox->setModelColumn(typeTableModel->fieldIndex("type"));
	
	setupConnections();
	ui->targetsListView->setCurrentIndex(tableModel->index(0, 1));	

	//Initialize the style
	updateStyle();
}

QSqlRecord TargetsDialog::currentRecord()
{ 
	if (lastRowNumberSelected == -1) {
		return QSqlRecord();
	} else {
		return tableModel->record(lastRowNumberSelected);
	}
}

void TargetsDialog::populateFormWithIndex(const QModelIndex &index)
{
	if (index.row() != -1) {
		teardownConnections();
		QSqlRecord record = tableModel->record(index.row());
		ui->nameLineEdit->setText(record.value("name").toString());
		ui->aliasLineEdit->setText(record.value("alias").toString());
		ui->j2000RALineEdit->setText(record.value("right_ascension").toString());
		ui->j2000DecLineEdit->setText(record.value("declination").toString());
		ui->magnitudeLineEdit->setText(record.value("magnitude").toString());
		ui->notesTextEdit->setPlainText(record.value("notes").toString());
		ui->sizeLineEdit->setText(record.value("size").toString());
		ui->distanceLineEdit->setText(record.value("distance").toString());
		ui->catalogNumberLineEdit->setText(record.value("catalog_number").toString());
		
		QModelIndexList list1 = typeTableModel->match(typeTableModel->index(0, 0), Qt::DisplayRole, record.value("target_type_id"));
		QSqlRecord typeRecord1 = typeTableModel->record(list1[0].row());
		ui->typeComboBox->setCurrentIndex(ui->typeComboBox->findText(typeRecord1.value("type").toString()));
		
		setupConnections();
	}
}

void TargetsDialog::setupConnections()
{	
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->targetsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			this, SLOT(targetSelected(QModelIndex)));
	connect(ui->typeComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(targetTypeChanged(const QString&)));
	connect(ui->addFromSelectionButton, SIGNAL(clicked()), this, SLOT(createTargetFromSelection()));
	connect(ui->addTargetButton, SIGNAL(clicked()), this, SLOT(insertNewTarget()));
	connect(ui->deleteTargetButton, SIGNAL(clicked()), this, SLOT(deleteSelectedTarget()));
	connect(ui->aliasLineEdit, SIGNAL(editingFinished()), this, SLOT(aliasChanged()));
	connect(ui->catalogNumberLineEdit, SIGNAL(editingFinished()), this, SLOT(catalogNumberChanged()));
	connect(ui->j2000DecLineEdit, SIGNAL(editingFinished()), this, SLOT(declinationChanged()));
	connect(ui->j2000RALineEdit, SIGNAL(editingFinished()), this, SLOT(rightAscentionChanged()));
	connect(ui->distanceLineEdit, SIGNAL(editingFinished()), this, SLOT(distanceChanged()));
	connect(ui->magnitudeLineEdit, SIGNAL(editingFinished()), this, SLOT(magnitudeChanged()));
	connect(ui->sizeLineEdit, SIGNAL(editingFinished()), this, SLOT(sizeChanged()));
	connect(ui->notesTextEdit, SIGNAL(textChanged()), this, SLOT(notesChanged()));
}

void TargetsDialog::teardownConnections()
{	
	disconnect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	disconnect(ui->targetsListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			   this, SLOT(populateFormWithIndex(QModelIndex)));
	disconnect(ui->typeComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(targetTypeChanged(const QString&)));
	disconnect(ui->addTargetButton, SIGNAL(clicked()), this, SLOT(insertNewTarget()));
	disconnect(ui->deleteTargetButton, SIGNAL(clicked()), this, SLOT(deleteSelectedTarget()));
	disconnect(ui->addFromSelectionButton, SIGNAL(clicked()), this, SLOT(createTargetFromSelection()));
	disconnect(ui->aliasLineEdit, SIGNAL(editingFinished()), this, SLOT(aliasChanged()));
	disconnect(ui->catalogNumberLineEdit, SIGNAL(editingFinished()), this, SLOT(catalogNumberChanged()));
	disconnect(ui->j2000DecLineEdit, SIGNAL(editingFinished()), this, SLOT(declinationChanged()));
	disconnect(ui->j2000RALineEdit, SIGNAL(editingFinished()), this, SLOT(rightAscentionChanged()));
	disconnect(ui->distanceLineEdit, SIGNAL(editingFinished()), this, SLOT(distanceChanged()));
	disconnect(ui->magnitudeLineEdit, SIGNAL(editingFinished()), this, SLOT(magnitudeChanged()));
	disconnect(ui->sizeLineEdit, SIGNAL(editingFinished()), this, SLOT(sizeChanged()));
	disconnect(ui->notesTextEdit, SIGNAL(textChanged()), this, SLOT(notesChanged()));
}
