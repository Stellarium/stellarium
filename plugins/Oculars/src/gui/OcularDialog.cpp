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

#include "Ocular.hpp"
#include "Oculars.hpp"
#include "OcularDialog.hpp"
#include "ui_ocularDialog.h"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelTranslator.hpp"

#include <QDataWidgetMapper>
#include <QDebug>
#include <QFrame>
#include <QModelIndex>
#include <QSettings>
#include <QSqlError>
#include <QSqlField>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlTableModel>

OcularDialog::OcularDialog(QSqlTableModel *ocularsTableModel, QSqlTableModel *telescopesTableModel)
{
	ui = new Ui_ocularDialogForm;
	this->ocularsTableModel = ocularsTableModel;
	this->telescopesTableModel = telescopesTableModel;

	validatorOcularAFOV = new QIntValidator(35, 100, this);
	validatorOcularEFL = new QDoubleValidator(1.0, 60.0, 1, this);
	validatorTelescopeDiameter = new QDoubleValidator(1.0, 1000.0, 1, this);
	validatorTelescopeFL = new QDoubleValidator(1.0, 10000.0, 1, this);
	QRegExp nameExp("^\\S.*");
	validatorName = new QRegExpValidator(nameExp, this);
}

OcularDialog::~OcularDialog()
{
	//These exist only if the window has been shown once:
	if (dialog)
	{
		ui->ocularAFov->setValidator(0);
		ui->ocularFL->setValidator(0);
		ui->telescopeFL->setValidator(0);
		ui->telescopeDiameter->setValidator(0);
		ui->ocularName->setValidator(0);
		ui->telescopeName->setValidator(0);
		delete ocularMapper;
		ocularMapper = NULL;
		delete telescopeMapper;
		telescopeMapper = NULL;
	}
	
	delete ui;
	ui = NULL;
	
	delete validatorOcularAFOV;
	validatorOcularAFOV = NULL;
	delete validatorOcularEFL;
	validatorOcularEFL = NULL;
	delete validatorTelescopeDiameter;
	validatorTelescopeDiameter = NULL;
	delete validatorTelescopeFL;
	validatorTelescopeFL = NULL;
	delete validatorName;
	validatorName = NULL;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark StelModule Methods
#endif
/* ********************************************************************* */
void OcularDialog::languageChanged()
{
	if (dialog) {
		ui->retranslateUi(dialog);
	}
}

void OcularDialog::setStelStyle(const StelStyle& style)
{
	if(dialog) {
		const StelStyle pluginStyle = GETSTELMODULE(Oculars)->getModuleStyleSheet(style);
		dialog->setStyleSheet(pluginStyle.qtStyleSheet);
		ui->textBrowser->document()->setDefaultStyleSheet(QString(pluginStyle.htmlStyleSheet));
	}
}

void OcularDialog::styleChanged()
{
	// Nothing for now
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Slot Methods
#endif
/* ********************************************************************* */
void OcularDialog::closeWindow()
{
	setVisible(false);
	StelMainGraphicsView::getInstance().scene()->setActiveWindow(0);
}

void OcularDialog::deleteSelectedOcular()
{
	QModelIndex selection = ui->ocularListView->currentIndex();
	if (selection.row() != -1 && ocularsTableModel->rowCount() > 1) {
		ocularsTableModel->removeRows(selection.row(), 1);
		ui->ocularListView->setCurrentIndex(ocularsTableModel->index(0, 1));
	}
}

void OcularDialog::deleteSelectedTelescope()
{
	QModelIndex selection = ui->telescopeListView->currentIndex();
	if (selection.row() != -1 && telescopesTableModel->rowCount() > 1) {
		telescopesTableModel->removeRows(selection.row(), 1);
		ui->telescopeListView->setCurrentIndex(telescopesTableModel->index(0, 1));
	}
}

void OcularDialog::insertNewOcular()
{
    QSqlField field1("name", QVariant::String);
    QSqlField field2("afov", QVariant::Double);
    QSqlField field3("efl", QVariant::Double);
    QSqlField field4("fieldStop", QVariant::Double);
	field1.setValue(QVariant("New Ocular"));
	field2.setValue(QVariant(82));
	field3.setValue(QVariant(32));
	field4.setValue(QVariant(0));
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	newRecord.append(field3);
	newRecord.append(field4);

	if (ocularsTableModel->insertRecord(-1, newRecord)) {
		ui->ocularListView->setCurrentIndex(ocularsTableModel->index(ocularsTableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "Oculars: could not insert new ocular.  The error is: " << ocularsTableModel->lastError();
	}
}

void OcularDialog::insertNewTelescope()
{
    QSqlField field1("name", QVariant::String);
    QSqlField field2("focalLength", QVariant::Double);
    QSqlField field3("diameter", QVariant::Double);
    QSqlField field4("vFlip", QVariant::String);
    QSqlField field5("hFlip", QVariant::String);
	field1.setValue(QVariant("New Telescope"));
	field2.setValue(QVariant(500));
	field3.setValue(QVariant(80));
	field4.setValue(QVariant("false"));
	field5.setValue(QVariant("false"));
	
	QSqlRecord newRecord = QSqlRecord();
	newRecord.append(field1);
	newRecord.append(field2);
	newRecord.append(field3);
	newRecord.append(field4);
	newRecord.append(field5);

	if (telescopesTableModel->insertRecord(-1, newRecord)) {
		ui->telescopeListView->setCurrentIndex(telescopesTableModel->index(telescopesTableModel->rowCount() - 1, 1));
	} else {
		qWarning() << "Oculars: could not insert new telescope.  The error is: " << telescopesTableModel->lastError();
	}
}

void OcularDialog::ocularSelected(const QModelIndex &index)
{
}

void OcularDialog::telescopeSelected(const QModelIndex &index)
{
}

void OcularDialog::updateOcular()
{
	int selectionIndex = ui->ocularListView->currentIndex().row();
	if (ocularMapper->submit()) {
		ui->ocularListView->setCurrentIndex(ocularsTableModel->index(selectionIndex, 1));
	} else {
		qWarning() << "Oculars: error saving modified ocular.  Error is: " << ocularsTableModel->lastError();
	}
}

void OcularDialog::updateTelescope()
{
	int selectionIndex = ui->telescopeListView->currentIndex().row();
	if (telescopeMapper->submit()) {
		ui->telescopeListView->setCurrentIndex(telescopesTableModel->index(selectionIndex, 1));
	} else {
		qWarning() << "Oculars: error saving modified telescope.  Error is: " << telescopesTableModel->lastError();
	}

}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Private Slot Methods
#endif
/* ********************************************************************* */
void OcularDialog::scaleImageCircleStateChanged(int state)
{
	bool shouldScale = (state == Qt::Checked);
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	QString ocularIniPath = StelFileMgr::findFile("modules/Oculars/", flags) + "ocular.ini";
	QSettings settings(ocularIniPath, QSettings::IniFormat);
	bool useMaxImageCircle = settings.value("use_max_exit_circle", 0.0).toBool();
	if (state != useMaxImageCircle) {
		settings.setValue("use_max_exit_circle", shouldScale);
		emit(scaleImageCircleChanged(shouldScale));
	}
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Protected Methods
#endif
/* ********************************************************************* */
void OcularDialog::createDialogContent()
{
	ui->setupUi(dialog);

	//Now the rest of the actions.
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->updateOcularButton, SIGNAL(clicked()), this, SLOT(updateOcular()));
	connect(ui->updateTelescopeButton, SIGNAL(clicked()), this, SLOT(updateTelescope()));
	connect(ui->scaleImageCircleCheckBox, SIGNAL(stateChanged(int)), this, SLOT(scaleImageCircleStateChanged(int)));
	// The add & delete buttons
	connect(ui->addOcular, SIGNAL(clicked()), this, SLOT(insertNewOcular()));
	connect(ui->deleteOcular, SIGNAL(clicked()), this, SLOT(deleteSelectedOcular()));
	connect(ui->addTelescope, SIGNAL(clicked()), this, SLOT(insertNewTelescope()));
	connect(ui->deleteTelescope, SIGNAL(clicked()), this, SLOT(deleteSelectedTelescope()));
	
	
	// Oculars model
	ui->ocularListView->setModel(ocularsTableModel);
	ui->ocularListView->setModelColumn(1);
	connect(ui->ocularListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(ocularSelected(const QModelIndex &)));
	
	ui->telescopeListView->setModel(telescopesTableModel);
	ui->telescopeListView->setModelColumn(1);
	connect(ui->telescopeListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(telescopeSelected(const QModelIndex &)));
	
	// Validators
	ui->ocularAFov->setValidator(validatorOcularAFOV);
	ui->ocularFL->setValidator(validatorOcularEFL);
	ui->ocularFieldStop->setValidator(validatorOcularEFL);
	ui->telescopeFL->setValidator(validatorTelescopeFL);
	ui->telescopeDiameter->setValidator(validatorTelescopeDiameter);
	ui->ocularName->setValidator(validatorName);
	ui->telescopeName->setValidator(validatorName);

	// The ocular mapper
	ocularMapper = new QDataWidgetMapper();
	ocularMapper->setModel(ocularsTableModel);
//	ocularMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	ocularMapper->addMapping(ui->ocularID, ocularsTableModel->fieldIndex("id"), "text");
	ocularMapper->addMapping(ui->ocularName, ocularsTableModel->fieldIndex("name"));
	ocularMapper->addMapping(ui->ocularAFov, ocularsTableModel->fieldIndex("afov"));
	ocularMapper->addMapping(ui->ocularFL, ocularsTableModel->fieldIndex("efl"));
	ocularMapper->addMapping(ui->ocularFieldStop, ocularsTableModel->fieldIndex("fieldStop"));
	ocularMapper->toFirst();
	ui->ocularListView->setCurrentIndex(ocularsTableModel->index(0, 1));
	connect(ui->ocularListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			ocularMapper, SLOT(setCurrentModelIndex(QModelIndex)));

	
	// The telescope mapper
	telescopeMapper = new QDataWidgetMapper();
	telescopeMapper->setModel(telescopesTableModel);
//	telescopeMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	telescopeMapper->addMapping(ui->telescopeID, telescopesTableModel->fieldIndex("id"), "text");
	telescopeMapper->addMapping(ui->telescopeName, telescopesTableModel->fieldIndex("name"));
	telescopeMapper->addMapping(ui->telescopeDiameter, telescopesTableModel->fieldIndex("diameter"));
	telescopeMapper->addMapping(ui->telescopeFL, telescopesTableModel->fieldIndex("focalLength"));
	telescopeMapper->addMapping(ui->telescopeVFlip, telescopesTableModel->fieldIndex("vFlip"), "checked");
	telescopeMapper->addMapping(ui->telescopeHFlip, telescopesTableModel->fieldIndex("hFlip"), "checked");
	ocularMapper->toFirst();
	ui->telescopeListView->setCurrentIndex(telescopesTableModel->index(0, 1));
	connect(ui->telescopeListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), 
			telescopeMapper, SLOT(setCurrentModelIndex(QModelIndex)));

	// set the initial state
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	QString ocularIniPath = StelFileMgr::findFile("modules/Oculars/", flags) + "ocular.ini";
	QSettings settings(ocularIniPath, QSettings::IniFormat);
	bool useMaxImageCircle = settings.value("use_max_exit_circle", false).toBool();
	if (useMaxImageCircle) {
		ui->scaleImageCircleCheckBox->setCheckState(Qt::Checked);
	}
	
	//Initialize the style
	setStelStyle(*StelApp::getInstance().getCurrentStelStyle());
}

