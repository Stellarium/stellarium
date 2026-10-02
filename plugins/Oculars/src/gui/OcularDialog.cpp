/*
 * Copyright (C) 2009 Timothy Reaves
 * Copyright (C) 2011 Bogdan Marinov
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "Oculars.hpp"
#include "OcularDialog.hpp"
#include "ui_ocularDialog.h"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainView.hpp"
#include "StelTranslator.hpp"

#include <QAbstractItemModel>
#include <QDataWidgetMapper>
#include <QDebug>
#include <QFrame>
#include <QModelIndex>
#include <QSettings>
#include <QStandardItemModel>
#include <QMessageBox>
#include <limits>
#include <QRegularExpression>

OcularDialog::OcularDialog(Oculars* pluginPtr,
			   QList<CCD *>* ccds,
			   QList<Ocular *>* oculars,
			   QList<Telescope *>* telescopes,
			   QList<Lens *> *lenses)
	: StelDialog("Oculars")
	, plugin(pluginPtr)
	, ccdMapper(Q_NULLPTR)
	, ocularMapper(Q_NULLPTR)
	, telescopeMapper(Q_NULLPTR)
	, lensMapper(Q_NULLPTR)
{
	ui = new Ui_ocularDialogForm;
	this->ccds = ccds;
	ccdTableModel = new PropertyBasedTableModel(this);
	CCD* ccdModel = CCD::ccdModel();
	ccdTableModel->init(reinterpret_cast<QList<QObject *>* >(ccds), ccdModel, ccdModel->propertyMap());
	this->oculars = oculars;
	ocularTableModel = new PropertyBasedTableModel(this);
	Ocular* ocularModel = Ocular::ocularModel();
	ocularTableModel->init(reinterpret_cast<QList<QObject *>* >(oculars), ocularModel, ocularModel->propertyMap());
	this->telescopes = telescopes;
	telescopeTableModel = new PropertyBasedTableModel(this);
	Telescope* telescopeModel = Telescope::telescopeModel();
	telescopeTableModel->init(reinterpret_cast<QList<QObject *>* >(telescopes), telescopeModel, telescopeModel->propertyMap());
	
	this->lenses = lenses;
	lensTableModel = new PropertyBasedTableModel(this);
	Lens* lensModel = Lens::lensModel();
	lensTableModel->init(reinterpret_cast<QList<QObject *>* >(lenses),
			     lensModel,
			     lensModel->propertyMap());

	QRegularExpression nameExp("^\\S.*");
	validatorName = new QRegularExpressionValidator(nameExp, this);
}

OcularDialog::~OcularDialog()
{
	if (dialog)
	{
		ui->telescopeListView->clearSelection();
		ui->ocularListView->clearSelection();
		ui->ccdListView->clearSelection();
		ui->lensListView->clearSelection();
	}

	ocularTableModel->disconnect(ocularMapper);
	telescopeTableModel->disconnect(telescopeMapper);
	ccdTableModel->disconnect(ccdMapper);
	lensTableModel->disconnect(lensMapper);

	delete ui;
	ui = Q_NULLPTR;
}

void OcularDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		initAboutText();
		updateSuffixes();
	}
}

void OcularDialog::closeWindow()
{
	setVisible(false);
	StelMainView::getInstance().scene()->setActiveWindow(Q_NULLPTR);
}

void OcularDialog::deleteSelectedCCD()
{
	if (ccdTableModel->rowCount() == 1)
	{
		qDebug() << "Cannot delete the last entry.";
		QMessageBox::warning(&StelMainView::getInstance(), q_("Warning!"), q_("Cannot delete the last sensor."), QMessageBox::Ok);
		return;
	}

	if (askConfirmation())
	{
		ccdTableModel->removeRows(ui->ccdListView->currentIndex().row(), 1);
		ui->ccdListView->setCurrentIndex(ccdTableModel->index(0, 1));
		plugin->updateLists();
	}
}

void OcularDialog::deleteSelectedOcular()
{
	if (ocularTableModel->rowCount() == 1)
	{
		qDebug() << "Cannot delete the last entry.";
		QMessageBox::warning(&StelMainView::getInstance(), q_("Warning!"), q_("Cannot delete the last ocular."), QMessageBox::Ok);
		return;
	}

	if (askConfirmation())
	{
		ocularTableModel->removeRows(ui->ocularListView->currentIndex().row(), 1);
		ui->ocularListView->setCurrentIndex(ocularTableModel->index(0, 1));
		plugin->updateLists();
	}
}

void OcularDialog::deleteSelectedTelescope()
{
	if (telescopeTableModel->rowCount() == 1)
	{
		qDebug() << "Cannot delete the last entry.";
		QMessageBox::warning(&StelMainView::getInstance(), q_("Warning!"), q_("Cannot delete the last telescope."), QMessageBox::Ok);
		return;
	}

	if (askConfirmation())
	{
		telescopeTableModel->removeRows(ui->telescopeListView->currentIndex().row(), 1);
		ui->telescopeListView->setCurrentIndex(telescopeTableModel->index(0, 1));
		plugin->updateLists();
	}
}

void OcularDialog::deleteSelectedLens()
{
	if (askConfirmation())
	{
		if (lensTableModel->rowCount() > 0)
		{
			lensTableModel->removeRows(ui->lensListView->currentIndex().row(), 1);
			if (lensTableModel->rowCount() > 0)
				ui->lensListView->setCurrentIndex(lensTableModel->index(0, 1));

			plugin->updateLists();
		}
	}
}

void OcularDialog::insertNewCCD()
{
	int count = ccdTableModel->rowCount();
	ccdTableModel->insertRows(count, 1);
	ui->ccdListView->setCurrentIndex(ccdTableModel->index(count, 1));
}

void OcularDialog::insertNewOcular()
{
	int count = ocularTableModel->rowCount();
	ocularTableModel->insertRows(count, 1);
	ui->ocularListView->setCurrentIndex(ocularTableModel->index(count, 1));
}

void OcularDialog::insertNewTelescope()
{
	int count = telescopeTableModel->rowCount();
	telescopeTableModel->insertRows(count, 1);
	ui->telescopeListView->setCurrentIndex(telescopeTableModel->index(count, 1));
}

void OcularDialog::insertNewLens()
{
	int count = lensTableModel->rowCount();
	lensTableModel->insertRows(count, 1);
	ui->lensListView->setCurrentIndex(lensTableModel->index(count, 1));
}

void OcularDialog::moveUpSelectedSensor()
{
	int index = ui->ccdListView->currentIndex().row();
	if (index > 0)
	{
		ccdTableModel->moveRowUp(index);
		plugin->updateLists();
	}
}

void OcularDialog::moveUpSelectedOcular()
{
	int index = ui->ocularListView->currentIndex().row();
	if (index > 0)
	{
		ocularTableModel->moveRowUp(index);
		plugin->updateLists();
	}
}

void OcularDialog::moveUpSelectedTelescope()
{
	int index = ui->telescopeListView->currentIndex().row();
	if (index > 0)
	{
		telescopeTableModel->moveRowUp(index);
		plugin->updateLists();
	}
}

void OcularDialog::moveUpSelectedLens()
{
	int index = ui->lensListView->currentIndex().row();
	if (index > 0)
	{
		lensTableModel->moveRowUp(index);
		plugin->updateLists();
	}
}

void OcularDialog::moveDownSelectedSensor()
{
	int index = ui->ccdListView->currentIndex().row();
	if (index >= 0 && index < ccdTableModel->rowCount() - 1)
	{
		ccdTableModel->moveRowDown(index);
		plugin->updateLists();
	}
}

void OcularDialog::moveDownSelectedOcular()
{
	int index = ui->ocularListView->currentIndex().row();
	if (index >= 0 && index < ocularTableModel->rowCount() - 1)
	{
		ocularTableModel->moveRowDown(index);
		plugin->updateLists();
	}
}

void OcularDialog::moveDownSelectedTelescope()
{
	int index = ui->telescopeListView->currentIndex().row();
	if (index >= 0 && index < telescopeTableModel->rowCount() - 1)
	{
		telescopeTableModel->moveRowDown(index);
		plugin->updateLists();
	}
}

void OcularDialog::moveDownSelectedLens()
{
	int index = ui->lensListView->currentIndex().row();
	if (index >= 0 && index < lensTableModel->rowCount() - 1)
	{
		lensTableModel->moveRowDown(index);
		plugin->updateLists();
	}
}

void OcularDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &OcularDialog::retranslate);
	ui->ccdListView->setModel(ccdTableModel);
	ui->ocularListView->setModel(ocularTableModel);
	ui->telescopeListView->setModel(telescopeTableModel);
	ui->lensListView->setModel(lensTableModel);

	// Kinetic scrolling
	kineticScrollingList << ui->textBrowser << ui->telescopeListView << ui->ccdListView << ui->ocularListView << ui->lensListView;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, &StelGui::flagUseKineticScrollingChanged, this, &OcularDialog::enableKineticScrolling);
	}
	
	//Now the rest of the actions.
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo, this, &StelDialog::handleMovedTo);

	connectBoolProperty(ui->checkBoxControlPanel,		"Oculars.flagGuiPanelEnabled");
	connectIntProperty(ui->guiFontSizeSpinBox,			"Oculars.guiPanelFontSize");
	connectBoolProperty(ui->checkBoxInitialFOV,			"Oculars.flagInitFOVUsage");
	connectBoolProperty(ui->checkBoxInitialDirection,		"Oculars.flagInitDirectionUsage");
	connectBoolProperty(ui->checkBoxResolutionCriterion,		"Oculars.flagShowResolutionCriteria");
	connectBoolProperty(ui->requireSelectionCheckBox,		"Oculars.flagRequireSelection");
	connectBoolProperty(ui->limitStellarMagnitudeCheckBox,	"Oculars.flagAutoLimitMagnitude");
	connectBoolProperty(ui->hideGridsLinesCheckBox,		"Oculars.flagHideGridsLines");
	connectBoolProperty(ui->scaleImageCircleCheckBox,		"Oculars.flagScaleImageCircle");
	connectBoolProperty(ui->semiTransparencyCheckBox,		"Oculars.flagSemiTransparency");
	connectIntProperty(ui->transparencySpinBox,			"Oculars.transparencyMask");
	connectBoolProperty(ui->checkBoxDMSDegrees,		"Oculars.flagDMSDegrees");
	connectBoolProperty(ui->checkBoxHorizontalCoordinates,	"Oculars.flagHorizontalCoordinates");
	connectBoolProperty(ui->checkBoxCCDCenter,			"Oculars.flagShowCCDCenter");
	connectBoolProperty(ui->checkBoxTypeOfMount,		"Oculars.flagAutosetMountForCCD");
	connectBoolProperty(ui->checkBoxTelradFOVScaling,		"Oculars.flagScalingFOVForTelrad");
	connectBoolProperty(ui->checkBoxCCDFOVScaling,		"Oculars.flagScalingFOVForCCD");
	connectBoolProperty(ui->checkBoxCCDMaxExposureTime,	"Oculars.flagMaxExposureTimeForCCD");
	connectBoolProperty(ui->checkBoxToolbarButton,		"Oculars.flagShowOcularsButton");
	connectIntProperty(ui->arrowButtonScaleSpinBox,		"Oculars.arrowButtonScale");
	connectBoolProperty(ui->checkBoxShowCcdCropOverlay,	"Oculars.flagShowCcdCropOverlay");
	connectBoolProperty(ui->checkBoxShowCcdCropOverlayPixelGrid, "Oculars.flagShowCcdCropOverlayPixelGrid");
	connectIntProperty(ui->guiCcdCropOverlayHSizeSpinBox,	"Oculars.ccdCropOverlayHSize");
	connectIntProperty(ui->guiCcdCropOverlayVSizeSpinBox,	"Oculars.ccdCropOverlayVSize");
	connectBoolProperty(ui->contourCheckBox,			"Oculars.flagShowContour");
	connectBoolProperty(ui->cardinalsCheckBox,			"Oculars.flagShowCardinals");
	connectBoolProperty(ui->alignCrosshairCheckBox,		"Oculars.flagAlignCrosshair");
	connectBoolProperty(ui->checkBoxShowFocuserOverlay,	"Oculars.flagShowFocuserOverlay");
	connectBoolProperty(ui->checkBoxUseSmallFocuser,		"Oculars.flagUseSmallFocuserOverlay");
	connectBoolProperty(ui->checkBoxUseMediumFocuser,		"Oculars.flagUseMediumFocuserOverlay");
	connectBoolProperty(ui->checkBoxUseLargeFocuser,		"Oculars.flagUseLargeFocuserOverlay");
	connectBoolProperty(ui->checkBoxShowOAGLimits,		"Oculars.flagShowOAGLimits");
	ui->textColorToolButton->setup("Oculars.textColor", "text_color", "Oculars");
	ui->lineColorToolButton->setup("Oculars.lineColor", "line_color", "Oculars");
	ui->reticleColorToolButton->setup("Oculars.reticleColor", "reticle_color", "Oculars");
	ui->focuserColorToolButton->setup("Oculars.focuserColor", "focuser_color", "Oculars");

	setupTelradFOVspins(plugin->getTelradFOV());
	connect(plugin, &Oculars::telradFOVChanged, this, &OcularDialog::setupTelradFOVspins);
	connect(ui->doubleSpinBoxTelradFOV1, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &OcularDialog::updateTelradCustomFOV);
	connect(ui->doubleSpinBoxTelradFOV2, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &OcularDialog::updateTelradCustomFOV);
	connect(ui->doubleSpinBoxTelradFOV3, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &OcularDialog::updateTelradCustomFOV);
	connect(ui->doubleSpinBoxTelradFOV4, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &OcularDialog::updateTelradCustomFOV);
	connect(ui->pushButtonRestoreTelradFOV, &QPushButton::clicked, this, [=] () { plugin->setTelradFOV(Vec4f(0.5f, 2.0f, 4.0f, 0.0f));} );

	// The add & delete buttons
	connect(ui->addCCD,          &QPushButton::clicked, this, &OcularDialog::insertNewCCD);
	connect(ui->deleteCCD,       &QPushButton::clicked, this, &OcularDialog::deleteSelectedCCD);
	connect(ui->addOcular,       &QPushButton::clicked, this, &OcularDialog::insertNewOcular);
	connect(ui->deleteOcular,    &QPushButton::clicked, this, &OcularDialog::deleteSelectedOcular);
	connect(ui->addLens,         &QPushButton::clicked, this, &OcularDialog::insertNewLens);
	connect(ui->deleteLens,      &QPushButton::clicked, this, &OcularDialog::deleteSelectedLens);
	connect(ui->addTelescope,    &QPushButton::clicked, this, &OcularDialog::insertNewTelescope);
	connect(ui->deleteTelescope, &QPushButton::clicked, this, &OcularDialog::deleteSelectedTelescope);

	// Validators
	ui->ccdName->setValidator(validatorName);
	ui->ocularName->setValidator(validatorName);
	ui->telescopeName->setValidator(validatorName);
	ui->lensName->setValidator(validatorName);

	initAboutText();
	updateSuffixes();

	connect(ui->pushButtonMoveOcularUp,      &QPushButton::pressed, this, &OcularDialog::moveUpSelectedOcular);
	connect(ui->pushButtonMoveOcularDown,    &QPushButton::pressed, this, &OcularDialog::moveDownSelectedOcular);
	connect(ui->pushButtonMoveSensorUp,      &QPushButton::pressed, this, &OcularDialog::moveUpSelectedSensor);
	connect(ui->pushButtonMoveSensorDown,    &QPushButton::pressed, this, &OcularDialog::moveDownSelectedSensor);
	connect(ui->pushButtonMoveTelescopeUp,   &QPushButton::pressed, this, &OcularDialog::moveUpSelectedTelescope);
	connect(ui->pushButtonMoveTelescopeDown, &QPushButton::pressed, this, &OcularDialog::moveDownSelectedTelescope);
	connect(ui->pushButtonMoveLensUp,        &QPushButton::pressed, this, &OcularDialog::moveUpSelectedLens);
	connect(ui->pushButtonMoveLensDown,      &QPushButton::pressed, this, &OcularDialog::moveDownSelectedLens);

	// The CCD mapper
	ccdMapper = new QDataWidgetMapper();
	ccdMapper->setModel(ccdTableModel);
	ccdMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	ccdMapper->addMapping(ui->ccdName,       0);
	ccdMapper->addMapping(ui->ccdChipY,      1);
	ccdMapper->addMapping(ui->ccdChipX,      2);
	ccdMapper->addMapping(ui->ccdResX,       3);
	ccdMapper->addMapping(ui->ccdResY,       4);
	ccdMapper->addMapping(ui->ccdRotAngle,   5);
	ccdMapper->addMapping(ui->ccdBinningX,   6);
	ccdMapper->addMapping(ui->ccdBinningY,   7);
	ccdMapper->addMapping(ui->OAG_checkBox, 8);
	ccdMapper->addMapping(ui->OAGPrismH,    9);
	ccdMapper->addMapping(ui->OAGPrismW,    10);
	ccdMapper->addMapping(ui->OAGDist,      11);
	ccdMapper->addMapping(ui->OAGPrismPA,   12);
	ccdMapper->toFirst();
	connect(ui->ccdListView->selectionModel(), &QItemSelectionModel::currentRowChanged,
	        ccdMapper, &QDataWidgetMapper::setCurrentModelIndex);
	connect(ui->ccdListView, &QListView::doubleClicked, this, &OcularDialog::selectCCD);
	ui->ccdListView->setSelectionBehavior(QAbstractItemView::SelectRows);	
	int index = plugin->getSelectedCCDIndex();
	ui->ccdListView->setCurrentIndex(ccdTableModel->index(index>0 ? index : 0, 1));

	connect(ui->ccdChipY,    &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(ui->ccdChipX,    &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(ui->ccdResX,     &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(ui->ccdResY,     &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(ui->ccdRotAngle, &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(ui->ccdBinningX, &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(ui->ccdBinningY, &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(ui->OAG_checkBox,&QCheckBox::stateChanged, this, &OcularDialog::updateCCD);
	connect(ui->OAGPrismH,   &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(ui->OAGPrismW,   &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(ui->OAGDist,     &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(ui->OAGPrismPA,  &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateCCD);
	connect(plugin, &Oculars::selectedCCDRotationAngleChanged, this, &OcularDialog::updateCCDRotationAngles);
	connect(plugin, &Oculars::selectedCCDPrismPositionAngleChanged, this, &OcularDialog::updateCCDRotationAngles);

	// The ocular mapper
	ocularMapper = new QDataWidgetMapper();
	ocularMapper->setModel(ocularTableModel);
	ocularMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	ocularMapper->addMapping(ui->ocularName,                 0);
	ocularMapper->addMapping(ui->ocularAFov,                 1);
	ocularMapper->addMapping(ui->ocularFL,                   2);
	ocularMapper->addMapping(ui->ocularFieldStop,            3);
	ocularMapper->addMapping(ui->binocularsCheckBox,         4, "checked");
	ocularMapper->addMapping(ui->permanentCrosshairCheckBox, 5, "checked");	
	ocularMapper->toFirst();
	connect(ui->ocularListView->selectionModel(), &QItemSelectionModel::currentRowChanged,
	        ocularMapper, &QDataWidgetMapper::setCurrentModelIndex);
	connect(ui->ocularListView, &QListView::doubleClicked, this, &OcularDialog::selectOcular);
	ui->ocularListView->setSelectionBehavior(QAbstractItemView::SelectRows);
	index = plugin->getSelectedOcularIndex();
	ui->ocularListView->setCurrentIndex(ocularTableModel->index(index>0 ? index : 0, 1));

	// We need particular refresh methods to see immediate feedback.
	connect(ui->ocularAFov,                 &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateOcular);
	connect(ui->ocularFL,                   &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateOcular);
	connect(ui->ocularFieldStop,            &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateOcular);
	connect(ui->binocularsCheckBox,         &QCheckBox::stateChanged, this, &OcularDialog::updateOcular);
	connect(ui->permanentCrosshairCheckBox, &QCheckBox::stateChanged, this, &OcularDialog::updateOcular);

	// The lens mapper
	lensMapper = new QDataWidgetMapper();
	lensMapper->setModel(lensTableModel);
	lensMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	lensMapper->addMapping(ui->lensName,       0);
	lensMapper->addMapping(ui->lensMultiplier, 1);
	lensMapper->toFirst();
	connect(ui->lensListView->selectionModel(), &QItemSelectionModel::currentRowChanged,
	        lensMapper, &QDataWidgetMapper::setCurrentModelIndex);
	connect(ui->lensListView, &QListView::doubleClicked, this, &OcularDialog::selectLens);
	ui->lensListView->setSelectionBehavior(QAbstractItemView::SelectRows);
	index = plugin->getSelectedLensIndex();
	ui->lensListView->setCurrentIndex(lensTableModel->index(index>0 ? index : 0, 1));

	connect(ui->lensMultiplier, &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateLens);

	// The telescope mapper
	telescopeMapper = new QDataWidgetMapper();
	telescopeMapper->setModel(telescopeTableModel);
	telescopeMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	telescopeMapper->addMapping(ui->telescopeName,     0);
	telescopeMapper->addMapping(ui->telescopeDiameter, 1);
	telescopeMapper->addMapping(ui->telescopeFL,       2);
	telescopeMapper->addMapping(ui->telescopeHFlip,    3, "checked");
	telescopeMapper->addMapping(ui->telescopeVFlip,    4, "checked");
	telescopeMapper->addMapping(ui->telescopeEQ,       5, "checked");
	telescopeMapper->toFirst();
	connect(ui->telescopeListView->selectionModel(), &QItemSelectionModel::currentRowChanged,
	        telescopeMapper, &QDataWidgetMapper::setCurrentModelIndex);
	connect(ui->telescopeListView, &QListView::doubleClicked, this, &OcularDialog::selectTelescope);
	ui->telescopeListView->setSelectionBehavior(QAbstractItemView::SelectRows);
	index = plugin->getSelectedTelescopeIndex();
	ui->telescopeListView->setCurrentIndex(telescopeTableModel->index(index>0 ? index : 0, 1));

	connect(ui->telescopeDiameter, &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateTelescope);
	connect(ui->telescopeFL,       &QDoubleSpinBox::editingFinished, this, &OcularDialog::updateTelescope);
	connect(ui->telescopeHFlip,    &QCheckBox::stateChanged, this, &OcularDialog::updateTelescope);
	connect(ui->telescopeVFlip,    &QCheckBox::stateChanged, this, &OcularDialog::updateTelescope);
	connect(ui->telescopeEQ,       &QCheckBox::stateChanged, this, &OcularDialog::updateTelescope);

	connect(ui->binocularsCheckBox, &QCheckBox::toggled, this, &OcularDialog::setLabelsDescriptionText);
	connect(ui->checkBoxControlPanel, &QCheckBox::toggled, this, &OcularDialog::updateGuiOptions);
	connect(ui->semiTransparencyCheckBox, &QCheckBox::toggled, this, &OcularDialog::updateGuiOptions);
	connect(ui->checkBoxShowFocuserOverlay, &QCheckBox::toggled, this, &OcularDialog::updateGuiOptions);
	connect(ui->checkBoxShowCcdCropOverlay, &QCheckBox::toggled, this, &OcularDialog::updateGuiOptions);
	connect(ui->limitStellarMagnitudeCheckBox, &QCheckBox::toggled, this, &OcularDialog::updateTelescope);
	setLabelsDescriptionText(ui->binocularsCheckBox->isChecked());
	updateGuiOptions();
}

void OcularDialog::setupTelradFOVspins(Vec4f fov)
{
	ui->doubleSpinBoxTelradFOV1->setValue(static_cast<double>(fov[0]));
	ui->doubleSpinBoxTelradFOV2->setValue(static_cast<double>(fov[1]));
	ui->doubleSpinBoxTelradFOV3->setValue(static_cast<double>(fov[2]));
	ui->doubleSpinBoxTelradFOV4->setValue(static_cast<double>(fov[3]));
}

void OcularDialog::updateCCDRotationAngles()
{
	if (dialog->isVisible())
	{
		if (plugin->getSelectedCCDIndex()==ccdMapper->currentIndex())
		{
			ui->ccdRotAngle->setValue(plugin->getSelectedCCDRotationAngle());
			ui->OAGPrismPA->setValue(plugin->getSelectedCCDPrismPositionAngle());
		}
	}
}

void OcularDialog::updateTelradCustomFOV()
{
	Vec4f fov(static_cast<float>(ui->doubleSpinBoxTelradFOV1->value()),
		  static_cast<float>(ui->doubleSpinBoxTelradFOV2->value()),
		  static_cast<float>(ui->doubleSpinBoxTelradFOV3->value()),
		  static_cast<float>(ui->doubleSpinBoxTelradFOV4->value()));
	plugin->setTelradFOV(fov);
}

// We need particular refresh methods to see immediate feedback.
void OcularDialog::updateOcular()
{
	ocularMapper->submit();
	plugin->selectOcularAtIndex(plugin->getSelectedOcularIndex());
}

void OcularDialog::selectOcular(const QModelIndex)
{
	plugin->selectOcularAtIndex(ocularMapper->currentIndex());
	plugin->updateLists();
}

void OcularDialog::updateLens()
{
	lensMapper->submit();
	plugin->selectLensAtIndex(plugin->getSelectedLensIndex());
}

void OcularDialog::selectLens(const QModelIndex)
{
	plugin->selectLensAtIndex(lensMapper->currentIndex());
	plugin->updateLists();
}

void OcularDialog::updateCCD()
{
	ccdMapper->submit();
	plugin->selectCCDAtIndex(plugin->getSelectedCCDIndex());
}

void OcularDialog::selectCCD(const QModelIndex)
{
	plugin->selectCCDAtIndex(ccdMapper->currentIndex());
	plugin->updateLists();
}

void OcularDialog::updateTelescope()
{
	telescopeMapper->submit();
	plugin->selectTelescopeAtIndex(plugin->getSelectedTelescopeIndex());
}

void OcularDialog::selectTelescope(const QModelIndex)
{
	plugin->selectTelescopeAtIndex(telescopeMapper->currentIndex());
	plugin->updateLists();
}

void OcularDialog::setLabelsDescriptionText(bool state)
{
	if (state)
	{
		// TRANSLATORS: tFOV for binoculars (tFOV = True Field of View)
		ui->labelFOV->setText(q_("tFOV:"));
		// TRANSLATORS: Magnification factor for binoculars
		ui->labelFL->setText(q_("Magnification factor:"));
		ui->labelFS->setText(q_("Diameter:"));
		ui->ocularFL->setSuffix("x");
	}
	else
	{
		ui->labelFOV->setText(q_("aFOV:"));
		ui->labelFL->setText(q_("Focal length:"));
		ui->labelFS->setText(q_("Field stop:"));
		ui->ocularFL->setSuffix(QString(" %1").arg(qc_("mm","millimeters")));
	}
}

void OcularDialog::updateSuffixes()
{
	const QString qMM = QString(" %1").arg(qc_("mm","millimeters"));
	const QString qPX = QString(" %1").arg(qc_("px", "pixels"));
	const QString qDG = QChar(0x00B0);
	ui->telescopeFL->setSuffix(qMM);
	ui->telescopeDiameter->setSuffix(qMM);
	ui->ccdChipX->setSuffix(qMM);
	ui->ccdChipY->setSuffix(qMM);
	ui->OAGDist->setSuffix(qMM);
	ui->OAGPrismH->setSuffix(qMM);
	ui->OAGPrismW->setSuffix(qMM);
	ui->ocularFieldStop->setSuffix(qMM);

	ui->ccdResX->setSuffix(qPX);
	ui->ccdResY->setSuffix(qPX);

	ui->ccdRotAngle->setSuffix(qDG);
	ui->OAGPrismPA->setSuffix(qDG);
	ui->ocularAFov->setSuffix(qDG);

	ui->lensMultiplier->setSuffix("x");
}

void OcularDialog::initAboutText()
{
	//BM: Most of the text for now is the original contents of the About widget.
	QString html = "<html><head><title></title></head><body>";

	html += "<h2>" + q_("Oculars Plug-in") + "</h2><table class='layout' width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + OCULARS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + OCULARS_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Timothy Reaves &lt;treaves@silverfieldstech.com&gt;</td></tr>";
	html += "<tr><td rowspan=\"7\"><strong>" + q_("Contributors") + ":</strong></td><td>Bogdan Marinov</td></tr>";
	html += "<tr><td>Alexander Wolf</td></tr>";
	html += "<tr><td>Georg Zotti</td></tr>";
	html += "<tr><td>Rumen G. Bogdanovski &lt;rumen@skyarchive.org&gt;</td></tr>";
	html += "<tr><td>Pawel Stolowski (" + q_("Barlow lens feature") + ")</td></tr>";		
	html += "<tr><td>Matt Hughes (" + q_("Sensor crop overlay feature") + ")</td></tr>";
	html += "<tr><td>Dhia Moakhar (" + q_("Pixel grid feature") + ")</td></tr>";
	html += "</table>";

	// Overview
	html += "<h3>" + q_("Overview") + "</h3>";

	html += "<p>" + q_("This plugin is intended to simulate what you would see through an eyepiece.  This configuration dialog can be used to add, modify, or delete eyepieces and telescopes, as well as CCD Sensors.  Your first time running the app will populate some samples to get you started.") + "</p>";
	html += "<p>" + q_("You can choose to scale the image you see on the screen.") + " ";
	html +=         q_("This is intended to show you a better comparison of what one eyepiece/telescope combination will be like when compared to another.") + " ";
	html +=         q_("The same eyepiece in two different telescopes of differing focal length will produce two different exit pupils, changing the view somewhat.") + " ";
	html +=         q_("The trade-off of this is that, with the image scaled, a large part of the screen can be wasted.") + " ";
	html +=         q_("Therefore we recommend that you leave it off, unless you feel you have a need for it.") + "</p>";
	html += "<p>" + q_("You can toggle a crosshair in the view.") + "</p>";
	html += "<p>" + QString(q_("You can toggle a Telrad finder. This feature draws three concentric circles of 0.5%1, 2.0%1, and 4.0%1, helping you see what you would expect to see with the naked eye through the Telrad (or similar) finder.")).arg(QChar(0x00B0));
	html +=         q_("You can adjust the diameters or even add a fourth circle if you have a different finder, or revert to the Telrad standard sizes.") + "</p>";
	html += "<p>" + q_("If you find any issues, please let me know. Enjoy!") + "</p>";

	// Keys
	html += "<h3>" + q_("Hot Keys") + "</h3>";
	html += "<p>" + q_("The plug-in's key bindings can be edited in the Keyboard shortcuts editor (F7).") + "</p>";

	// Notes
	html += "<h3>" + q_("Notes") + "</h3>";
	html += "<p>" +  q_("The sensor view has a feature to show a sensor crop overlay with information about the crop size. The size of this rectangle may be adjusted when binning is active (e.g. crop size of 100px will be adjusted to 99px by binning 3).") + " ";
	html +=          q_("In this case, information about crop size overlay will be marked by %1.").arg("[*]") + " ";
	html +=          q_("This mark is also displayed if the crop size is larger than the sensor size.") + "</p>";
	html += "<p>" + q_("The sensor view has also a feature to show a max time of exposure for selected solar system objects (the mount has sidereal speed enabled) to avoid a movement of more than 1 pixel width.") + "</p>";

	html += StelApp::getInstance().getModuleMgr().getStandardSupportLinksInfo("Oculars plugin");
	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
		ui->textBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	ui->textBrowser->setHtml(html);
}

void OcularDialog::updateGuiOptions()
{
	bool flag = ui->checkBoxControlPanel->isChecked();
	ui->guiFontSizeLabel->setEnabled(flag);
	ui->guiFontSizeSpinBox->setEnabled(flag);

	ui->transparencySpinBox->setEnabled(ui->semiTransparencyCheckBox->isChecked());

	flag = ui->checkBoxShowFocuserOverlay->isChecked();
	ui->labelFocuserOverlay->setEnabled(flag);
	ui->checkBoxUseSmallFocuser->setEnabled(flag);
	ui->checkBoxUseMediumFocuser->setEnabled(flag);
	ui->checkBoxUseLargeFocuser->setEnabled(flag);

	flag = ui->checkBoxShowCcdCropOverlay->isChecked();
	ui->guiCcdCropOverlaySizeLabel->setEnabled(flag);
	ui->guiCcdCropOverlayHSizeSpinBox->setEnabled(flag);
	ui->guiCcdCropOverlayVSizeSpinBox->setEnabled(flag);
	ui->checkBoxShowCcdCropOverlayPixelGrid->setEnabled(flag);
}

