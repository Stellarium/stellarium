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

#include "Oculars.hpp"
#include "OcularDialog.hpp"
#include "ui_ocularDialog.h"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelTranslator.hpp"

#include <QAbstractItemModel>
#include <QAction>
#include <QDataWidgetMapper>
#include <QDebug>
#include <QFrame>
#include <QModelIndex>
#include <QSettings>
#include <QStandardItemModel>
#include <limits>

OcularDialog::OcularDialog(QList<CCD *>* ccds, QList<Ocular *>* oculars, QList<Telescope *>* telescopes)
{
	ui = new Ui_ocularDialogForm;
	this->ccds = ccds;
	ccdTableModel = new PropertyBasedTableModel(this);
	ccdTableModel->init(reinterpret_cast<QList<QObject *>* >(ccds),
							  CCD::ccdModel(),
							  CCD::ccdModel()->propertyMap());
	this->oculars = oculars;
	ocularTableModel = new PropertyBasedTableModel(this);
	ocularTableModel->init(reinterpret_cast<QList<QObject *>* >(oculars),
								  Ocular::ocularModel(), Ocular::ocularModel()->propertyMap());
	this->telescopes = telescopes;
	telescopeTableModel = new PropertyBasedTableModel(this);
	telescopeTableModel->init(reinterpret_cast<QList<QObject *>* >(telescopes),
									  Telescope::telescopeModel(),
									  Telescope::telescopeModel()->propertyMap());
	
	validatorPositiveInt = new QIntValidator(0, std::numeric_limits<int>::max(), this);
	validatorPositiveDouble = new QDoubleValidator(.0, std::numeric_limits<double>::max(), 24, this);
	validatorOcularAFOV = new QIntValidator(35, 120, this);
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
		ui->ocularName->setValidator(0);
		ui->ccdName->setValidator(0);
		ui->ccdResX->setValidator(0);
		ui->ccdResY->setValidator(0);
		ui->ccdChipX->setValidator(0);
		ui->ccdChipY->setValidator(0);
		ui->ccdPixelX->setValidator(0);
		ui->ccdPixelX->setValidator(0);
		delete ccdMapper;
		ccdMapper = NULL;
		delete ocularMapper;
		ocularMapper = NULL;
		delete telescopeMapper;
		telescopeMapper = NULL;
	}
	delete ui;
	ui = NULL;
	delete validatorPositiveInt;
	validatorPositiveInt = NULL;
	delete validatorPositiveDouble;
	validatorPositiveDouble = NULL;
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

void OcularDialog::styleChanged()
{
	// Nothing for now
}

void OcularDialog::updateStyle()
{
	if(dialog) {
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		const StelStyle pluginStyle = GETSTELMODULE(Oculars)->getModuleStyleSheet(gui->getStelStyle());
		dialog->setStyleSheet(pluginStyle.qtStyleSheet);
		ui->textBrowser->document()->setDefaultStyleSheet(QString(pluginStyle.htmlStyleSheet));
	}
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

void OcularDialog::deleteSelectedCCD()
{
	ccdTableModel->removeRows(ui->ccdListView->currentIndex().row(), 1);
	ui->ccdListView->setCurrentIndex(ccdTableModel->index(0, 1));
}


void OcularDialog::deleteSelectedOcular()
{
	if (ocularTableModel->rowCount() == 1) {
		qDebug() << "Can not delete the last entry.";
	} else {
		ocularTableModel->removeRows(ui->ocularListView->currentIndex().row(), 1);
		ui->ocularListView->setCurrentIndex(ocularTableModel->index(0, 1));
	}
}

void OcularDialog::deleteSelectedTelescope()
{
	if (telescopeTableModel->rowCount() == 1) {
		qDebug() << "Can not delete the last entry.";
	} else {
		telescopeTableModel->removeRows(ui->telescopeListView->currentIndex().row(), 1);
		ui->telescopeListView->setCurrentIndex(telescopeTableModel->index(0, 1));
	}
}

void OcularDialog::insertNewCCD()
{
	ccdTableModel->insertRows(ccdTableModel->rowCount(), 1);
	ui->ccdListView->setCurrentIndex(ccdTableModel->index(ccdTableModel->rowCount() - 1, 1));
}

void OcularDialog::insertNewOcular()
{
	ocularTableModel->insertRows(ocularTableModel->rowCount(), 1);
	ui->ocularListView->setCurrentIndex(ocularTableModel->index(ocularTableModel->rowCount() - 1, 1));
}

void OcularDialog::insertNewTelescope()
{
	telescopeTableModel->insertRows(ccdTableModel->rowCount(), 1);
	ui->telescopeListView->setCurrentIndex(telescopeTableModel->index(telescopeTableModel->rowCount() - 1, 1));
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Private Slot Methods
#endif
/* ********************************************************************* */
void OcularDialog::keyBindingTextTogglePluginChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/toggle_oculars", newString);
	this->updateActionMapping("toggle_oculars", newString);
}

void OcularDialog::keyBindingTextTogglePluginConfigChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/toggle_config_dialog", newString);
	this->updateActionMapping("actionShow_Ocular_Window", newString);
}

void OcularDialog::keyBindingTextToggleCrosshairChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/toggle_crosshair", newString);
	this->updateActionMapping("toggle_crosshair", newString);
}

void OcularDialog::keyBindingTextToggleTelradChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/toggle_telrad", newString);
	this->updateActionMapping("actionShow_Ocular_Telrad", newString);
}

void OcularDialog::keyBindingTextNextCCDChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/next_ccd", newString);
	this->updateActionMapping("next_ccd", newString);
}

void OcularDialog::keyBindingTextNextOcularChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/next_ocular", newString);
	this->updateActionMapping("next_ocular", newString);
}

void OcularDialog::keyBindingTextNextTelescopeChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/next_telescope", newString);
	this->updateActionMapping("next_telescope", newString);
}

void OcularDialog::keyBindingTextPreviousCCDChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/prev_ccd", newString);
	this->updateActionMapping("perv_ccd", newString);
}

void OcularDialog::keyBindingTextPreviousOcularChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/prev_ocular", newString);
	this->updateActionMapping("perv_ocular", newString);
}

void OcularDialog::keyBindingTextPreviousTelescopeChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/prev_telescope", newString);
	this->updateActionMapping("perv_telescope", newString);
}
					  
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

void OcularDialog::ocularIsBinocularsStateChanged(int state)
{
	bool shouldScale = (state == Qt::Checked);
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
	ui->ccdListView->setModel(ccdTableModel);
	ui->ocularListView->setModel(ocularTableModel);
	ui->telescopeListView->setModel(telescopeTableModel);
	
	//Now the rest of the actions.
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->scaleImageCircleCheckBox, SIGNAL(stateChanged(int)), this, SLOT(scaleImageCircleStateChanged(int)));
	// The add & delete buttons
	connect(ui->addCCD, SIGNAL(clicked()), this, SLOT(insertNewCCD()));
	connect(ui->deleteCCD, SIGNAL(clicked()), this, SLOT(deleteSelectedCCD()));
	connect(ui->addOcular, SIGNAL(clicked()), this, SLOT(insertNewOcular()));
	connect(ui->deleteOcular, SIGNAL(clicked()), this, SLOT(deleteSelectedOcular()));
	connect(ui->addTelescope, SIGNAL(clicked()), this, SLOT(insertNewTelescope()));
	connect(ui->deleteTelescope, SIGNAL(clicked()), this, SLOT(deleteSelectedTelescope()));

	// Validators
	ui->ccdName->setValidator(validatorName);
	ui->ccdResX->setValidator(validatorPositiveInt);
	ui->ccdResY->setValidator(validatorPositiveInt);
	ui->ccdChipX->setValidator(validatorPositiveDouble);
	ui->ccdChipY->setValidator(validatorPositiveDouble);
	ui->ccdPixelX->setValidator(validatorPositiveDouble);
	ui->ccdPixelY->setValidator(validatorPositiveDouble);
	ui->ocularAFov->setValidator(validatorOcularAFOV);
	ui->ocularFL->setValidator(validatorOcularEFL);
	ui->ocularFieldStop->setValidator(validatorOcularEFL);
	ui->telescopeFL->setValidator(validatorTelescopeFL);
	ui->telescopeDiameter->setValidator(validatorTelescopeDiameter);
	ui->ocularName->setValidator(validatorName);
	ui->telescopeName->setValidator(validatorName);
	
	// The key bindings
	ui->togglePluginLineEdit->setText(Oculars::appSettings()->value("bindings/toggle_oculars").toString());
	ui->toggleConfigWindowLineEdit->setText(Oculars::appSettings()->value("bindings/toggle_config_dialog").toString());
	ui->toggleCrosshairLineEdit->setText(Oculars::appSettings()->value("bindings/toggle_crosshair").toString());
	ui->toggleTelradLineEdit->setText(Oculars::appSettings()->value("bindings/toggle_telrad").toString());
	ui->nextCCDLineEdit->setText(Oculars::appSettings()->value("bindings/next_ccd").toString());
	ui->nextOcularLineEdit->setText(Oculars::appSettings()->value("bindings/next_ocular").toString());
	ui->nextTelescopeLineEdit->setText(Oculars::appSettings()->value("bindings/next_telescope").toString());
	ui->previousCCDLineEdit->setText(Oculars::appSettings()->value("bindings/prev_ccd").toString());
	ui->previousOcularLineEdit->setText(Oculars::appSettings()->value("bindings/prev_ocular").toString());
	ui->previousTelescopeLineEdit->setText(Oculars::appSettings()->value("bindings/prev_telescope").toString());
	connect(ui->togglePluginLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTextTogglePluginChanged(const QString&)));
	connect(ui->toggleConfigWindowLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTextTogglePluginConfigChanged(const QString&)));
	connect(ui->toggleCrosshairLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTextToggleCrosshairChanged(const QString&)));
	connect(ui->toggleTelradLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTextToggleTelradChanged(const QString&)));
	connect(ui->nextCCDLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTextNextCCDChanged(const QString&)));
	connect(ui->nextOcularLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTextNextOcularChanged(const QString&)));
	connect(ui->nextTelescopeLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTextNextTelescopeChanged(const QString&)));
	connect(ui->previousCCDLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTextPreviousCCDChanged(const QString&)));
	connect(ui->previousOcularLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTextPreviousOcularChanged(const QString&)));
	connect(ui->previousTelescopeLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTextPreviousTelescopeChanged(const QString&)));
						  
	// The CCD mapper
	ccdMapper = new QDataWidgetMapper();
	ccdMapper->setModel(ccdTableModel);
	ccdMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	ccdMapper->addMapping(ui->ccdName, 0);
	ccdMapper->addMapping(ui->ccdChipY, 1);
	ccdMapper->addMapping(ui->ccdChipX, 2);
	ccdMapper->addMapping(ui->ccdPixelY, 3);
	ccdMapper->addMapping(ui->ccdPixelX, 4);
	ccdMapper->addMapping(ui->ccdResX, 5);
	ccdMapper->addMapping(ui->ccdResY, 6);
	ccdMapper->toFirst();
	connect(ui->ccdListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
			ccdMapper, SLOT(setCurrentModelIndex(QModelIndex)));
	ui->ccdListView->setCurrentIndex(ccdTableModel->index(0, 1));

	// The ocular mapper
	ocularMapper = new QDataWidgetMapper();
	ocularMapper->setModel(ocularTableModel);
	ocularMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	ocularMapper->addMapping(ui->ocularName, 0);
	ocularMapper->addMapping(ui->ocularAFov, 1);
	ocularMapper->addMapping(ui->ocularFL, 2);
	ocularMapper->addMapping(ui->ocularFieldStop, 3);
	ocularMapper->addMapping(ui->binocularsCheckBox, 4, "checked");
	ocularMapper->toFirst();
	connect(ui->ocularListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
			  ocularMapper, SLOT(setCurrentModelIndex(QModelIndex)));
	ui->ocularListView->setCurrentIndex(ocularTableModel->index(0, 1));

	// The telescope mapper
	telescopeMapper = new QDataWidgetMapper();
	telescopeMapper->setModel(telescopeTableModel);
	telescopeMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	telescopeMapper->addMapping(ui->telescopeName, 0);
	telescopeMapper->addMapping(ui->telescopeDiameter, 1);
	telescopeMapper->addMapping(ui->telescopeFL, 2);
	telescopeMapper->addMapping(ui->telescopeHFlip, 3, "checked");
	telescopeMapper->addMapping(ui->telescopeVFlip, 4, "checked");
	ocularMapper->toFirst();
	connect(ui->telescopeListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
			telescopeMapper, SLOT(setCurrentModelIndex(QModelIndex)));
	ui->telescopeListView->setCurrentIndex(telescopeTableModel->index(0, 1));

	// set the initial state
	if (Oculars::appSettings()->value("use_max_exit_circle", false).toBool()) {
		ui->scaleImageCircleCheckBox->setCheckState(Qt::Checked);
	}

	//Initialize the style
	updateStyle();
}

void OcularDialog::updateActionMapping(const QString& actionName, const QString& newMapping)
{
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	QAction* action = gui->getGuiActions(actionName);
	if (action != NULL) {
		action->setShortcut(QKeySequence(newMapping.trimmed()));
	}
}
