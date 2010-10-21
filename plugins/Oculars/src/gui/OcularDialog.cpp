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
#include <QDataWidgetMapper>
#include <QDebug>
#include <QFrame>
#include <QModelIndex>
#include <QSettings>
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
}


void OcularDialog::deleteSelectedOcular()
{
}

void OcularDialog::deleteSelectedTelescope()
{
}

void OcularDialog::insertNewCCD()
{
}

void OcularDialog::insertNewOcular()
{
}

void OcularDialog::insertNewTelescope()
{
}

void OcularDialog::updateCCD()
{
}

void OcularDialog::updateOcular()
{
}

void OcularDialog::updateTelescope()
{
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
void OcularDialog::testCurrentIndexChanged(const QModelIndex& newIndex)
{
//	qDebug() << "Index is now: " << newIndex;
//	QModelIndex theIndex = ocularTableModel->index(newIndex.row(), 0);
//	qDebug() << "  - " << ocularTableModel->data(theIndex, Qt::DisplayRole);
//	theIndex = ocularTableModel->index(newIndex.row(), 1);
//	qDebug() << "  - " << ocularTableModel->data(theIndex, Qt::DisplayRole);
//	theIndex = ocularTableModel->index(newIndex.row(), 2);
//	qDebug() << "  - " << ocularTableModel->data(theIndex, Qt::DisplayRole);
//	theIndex = ocularTableModel->index(newIndex.row(), 3);
//	qDebug() << "  - " << ocularTableModel->data(theIndex, Qt::DisplayRole);
}

void OcularDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->ccdListView->setModel(ccdTableModel);
	ui->ocularListView->setModel(ocularTableModel);
	ui->telescopeListView->setModel(telescopeTableModel);
	
	//Now the rest of the actions.
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->updateCCDButton, SIGNAL(clicked()), this, SLOT(updateCCD()));
	connect(ui->updateOcularButton, SIGNAL(clicked()), this, SLOT(updateOcular()));
	connect(ui->updateTelescopeButton, SIGNAL(clicked()), this, SLOT(updateTelescope()));
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

	// The CCD mapper
	ccdMapper = new QDataWidgetMapper();
	ccdMapper->setModel(ccdTableModel);
//	CCDMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
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
//	ocularMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	ocularMapper->addMapping(ui->ocularName, 0);
	ocularMapper->addMapping(ui->ocularAFov, 1);
	ocularMapper->addMapping(ui->ocularFL, 2);
	ocularMapper->addMapping(ui->ocularFieldStop, 3);
	ocularMapper->toFirst();
	connect(ui->ocularListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
			  ocularMapper, SLOT(setCurrentModelIndex(QModelIndex)));
	connect(ui->ocularListView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
			  this, SLOT(testCurrentIndexChanged(QModelIndex)));
	ui->ocularListView->setCurrentIndex(ocularTableModel->index(0, 1));

	// The telescope mapper
	telescopeMapper = new QDataWidgetMapper();
	telescopeMapper->setModel(telescopeTableModel);
//	telescopeMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
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
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	QString ocularIniPath = StelFileMgr::findFile("modules/Oculars/", flags) + "ocular.ini";
	QSettings settings(ocularIniPath, QSettings::IniFormat);
	bool useMaxImageCircle = settings.value("use_max_exit_circle", false).toBool();
	if (useMaxImageCircle) {
		ui->scaleImageCircleCheckBox->setCheckState(Qt::Checked);
	}

	//Initialize the style
	updateStyle();
}

