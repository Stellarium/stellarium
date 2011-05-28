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
	delete ui;
	ui = NULL;
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
void OcularDialog::keyBindingTogglePluginChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/toggle_oculars", newString);
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	QAction* action = gui->getGuiActions("toggle_oculars");
	if (action != NULL) {
		action->setShortcut(QKeySequence(newString.trimmed()));
	}
}

void OcularDialog::keyBindingPopupNavigatorConfigChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/popup_navigator", newString);
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	QAction* action = gui->getGuiActions("actionShow_Ocular_Menu");
	if (action != NULL) {
		action->setShortcut(QKeySequence(newString.trimmed()));
	}
}
					  
void OcularDialog::requireSelectionStateChanged(int state)
{
	bool requireSelection = (state == Qt::Checked);
	bool requireSelectionToZoom = Oculars::appSettings()->value("require_selection_to_zoom", 1.0).toBool();
	if (requireSelection != requireSelectionToZoom) {
		Oculars::appSettings()->setValue("require_selection_to_zoom", requireSelection);
		Oculars::appSettings()->sync();\
		emit(requireSelectionChanged(requireSelection));
	}
}

void OcularDialog::scaleImageCircleStateChanged(int state)
{
	bool shouldScale = (state == Qt::Checked);
	bool useMaxImageCircle = Oculars::appSettings()->value("use_max_exit_circle",01.0).toBool();
	if (shouldScale != useMaxImageCircle) {
		Oculars::appSettings()->setValue("use_max_exit_circle", shouldScale);
		Oculars::appSettings()->sync();\
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
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(languageChanged()));
	ui->ccdListView->setModel(ccdTableModel);
	ui->ocularListView->setModel(ocularTableModel);
	ui->telescopeListView->setModel(telescopeTableModel);
	
	//Now the rest of the actions.
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->scaleImageCircleCheckBox, SIGNAL(stateChanged(int)), this, SLOT(scaleImageCircleStateChanged(int)));
	connect(ui->requireSelectionCheckBox, SIGNAL(stateChanged(int)), this, SLOT(requireSelectionStateChanged(int)));
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
	QString bindingString = Oculars::appSettings()->value("bindings/toggle_oculars", "Ctrl+O").toString();
	ui->togglePluginLineEdit->setText(bindingString);
	bindingString = Oculars::appSettings()->value("bindings/popup_navigator", "Alt+O").toString();
	ui->togglePopupNavigatorWindowLineEdit->setText(bindingString);
	connect(ui->togglePluginLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingTogglePluginChanged(const QString&)));
	connect(ui->togglePopupNavigatorWindowLineEdit, SIGNAL(textEdited(const QString&)), 
			this, SLOT(keyBindingPopupNavigatorConfigChanged(const QString&)));
						  
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
	if (Oculars::appSettings()->value("require_selection_to_zoom", 1.0).toBool()) {
		ui->requireSelectionCheckBox->setCheckState(Qt::Checked);
	}
	if (Oculars::appSettings()->value("use_max_exit_circle", 0.0).toBool()) {
		ui->scaleImageCircleCheckBox->setCheckState(Qt::Checked);
	}

	//Initialize the style
	updateStyle();
}
