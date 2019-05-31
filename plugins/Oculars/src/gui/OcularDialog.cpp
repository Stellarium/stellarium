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
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainView.hpp"
#include "StelTranslator.hpp"
#include "StelActionMgr.hpp"

#include <QAbstractItemModel>
#include <QDataWidgetMapper>
#include <QDebug>
#include <QFrame>
#include <QModelIndex>
#include <QSettings>
#include <QStandardItemModel>
#include <limits>

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
	ccdTableModel->init(reinterpret_cast<QList<QObject *>* >(ccds),
			    ccdModel,
			    ccdModel->propertyMap());
	this->oculars = oculars;
	ocularTableModel = new PropertyBasedTableModel(this);
	Ocular* ocularModel = Ocular::ocularModel();
	ocularTableModel->init(reinterpret_cast<QList<QObject *>* >(oculars),
			       ocularModel,
			       ocularModel->propertyMap());
	this->telescopes = telescopes;
	telescopeTableModel = new PropertyBasedTableModel(this);
	Telescope* telescopeModel = Telescope::telescopeModel();
	telescopeTableModel->init(reinterpret_cast<QList<QObject *>* >(telescopes),
				  telescopeModel,
				  telescopeModel->propertyMap());
	
	this->lenses = lenses;
	lensTableModel = new PropertyBasedTableModel(this);
	Lens* lensModel = Lens::lensModel();
	lensTableModel->init(reinterpret_cast<QList<QObject *>* >(lenses),
			     lensModel,
			     lensModel->propertyMap());

	QRegExp nameExp("^\\S.*");
	validatorName = new QRegExpValidator(nameExp, this);
}

OcularDialog::~OcularDialog()
{
	ocularTableModel->disconnect();
	telescopeTableModel->disconnect();
	ccdTableModel->disconnect();
	lensTableModel->disconnect();

	delete ui;
	ui = Q_NULLPTR;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark StelModule Methods
#endif
/* ********************************************************************* */
void OcularDialog::retranslate()
{
	if (dialog) {
		ui->retranslateUi(dialog);
		initAboutText();
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
	StelMainView::getInstance().scene()->setActiveWindow(0);
}

void OcularDialog::deleteSelectedCCD()
{
	ccdTableModel->removeRows(ui->ccdListView->currentIndex().row(), 1);
	ui->ccdListView->setCurrentIndex(ccdTableModel->index(0, 1));
	plugin->updateLists();
}

void OcularDialog::deleteSelectedOcular()
{
	if (ocularTableModel->rowCount() == 1) {
		qDebug() << "Cannot delete the last entry.";
	} else {
		ocularTableModel->removeRows(ui->ocularListView->currentIndex().row(), 1);
		ui->ocularListView->setCurrentIndex(ocularTableModel->index(0, 1));
		plugin->updateLists();
	}
}

void OcularDialog::deleteSelectedTelescope()
{
	if (telescopeTableModel->rowCount() == 1) {
		qDebug() << "Cannot delete the last entry.";
	} else {
		telescopeTableModel->removeRows(ui->telescopeListView->currentIndex().row(), 1);
		ui->telescopeListView->setCurrentIndex(telescopeTableModel->index(0, 1));
		plugin->updateLists();
	}
}

void OcularDialog::deleteSelectedLens()
{
    if (lensTableModel->rowCount() > 0) {
	lensTableModel->removeRows(ui->lensListView->currentIndex().row(), 1);
	if (lensTableModel->rowCount() > 0) {
	    ui->lensListView->setCurrentIndex(lensTableModel->index(0, 1));
        }
        plugin->updateLists();
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
	telescopeTableModel->insertRows(telescopeTableModel->rowCount(), 1);
	ui->telescopeListView->setCurrentIndex(telescopeTableModel->index(telescopeTableModel->rowCount() - 1, 1));
}

void OcularDialog::insertNewLens()
{
	lensTableModel->insertRows(lensTableModel->rowCount(), 1);
	ui->lensListView->setCurrentIndex(lensTableModel->index(lensTableModel->rowCount() - 1, 1));
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

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Protected Methods
#endif
/* ********************************************************************* */
void OcularDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
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
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	
	//Now the rest of the actions.
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectBoolProperty(ui->checkBoxControlPanel,          "Oculars.flagGuiPanelEnabled");
	connectIntProperty(ui->guiFontSizeSpinBox,             "Oculars.guiPanelFontSize");
	connectBoolProperty(ui->checkBoxInitialFOV,            "Oculars.flagInitFOVUsage");
	connectBoolProperty(ui->checkBoxInitialDirection,      "Oculars.flagInitDirectionUsage");
	connectBoolProperty(ui->checkBoxResolutionCriterion,   "Oculars.flagShowResolutionCriterions");
	connectBoolProperty(ui->requireSelectionCheckBox,      "Oculars.flagRequireSelection");
	connectBoolProperty(ui->limitStellarMagnitudeCheckBox, "Oculars.flagLimitMagnitude");
	connectBoolProperty(ui->hideGridsLinesCheckBox,        "Oculars.flagHideGridsLines");
	connectBoolProperty(ui->scaleImageCircleCheckBox,      "Oculars.flagScaleImageCircle");
	connectBoolProperty(ui->semiTransparencyCheckBox,      "Oculars.flagSemiTransparency");
	connectBoolProperty(ui->checkBoxDMSDegrees,            "Oculars.flagDMSDegrees");
	connectBoolProperty(ui->checkBoxTypeOfMount,           "Oculars.flagAutosetMountForCCD");
	connectBoolProperty(ui->checkBoxTelradFOVScaling,      "Oculars.flagScalingFOVForTelrad");
	connectBoolProperty(ui->checkBoxToolbarButton,         "Oculars.flagShowOcularsButton");
	connectDoubleProperty(ui->arrowButtonScaleDoubleSpinBox, "Oculars.arrowButtonScale");
	connectBoolProperty(ui->checkBoxShowCcdCropOverlay,    "Oculars.flagShowCcdCropOverlay");
	connectIntProperty(ui->guiCcdCropOverlaySizeSpinBox,   "Oculars.ccdCropOverlaySize");

	// The add & delete buttons
	connect(ui->addCCD,          SIGNAL(clicked()), this, SLOT(insertNewCCD()));
	connect(ui->deleteCCD,       SIGNAL(clicked()), this, SLOT(deleteSelectedCCD()));
	connect(ui->addOcular,       SIGNAL(clicked()), this, SLOT(insertNewOcular()));
	connect(ui->deleteOcular,    SIGNAL(clicked()), this, SLOT(deleteSelectedOcular()));
	connect(ui->addLens,         SIGNAL(clicked()), this, SLOT(insertNewLens()));
	connect(ui->deleteLens,      SIGNAL(clicked()), this, SLOT(deleteSelectedLens()));
	connect(ui->addTelescope,    SIGNAL(clicked()), this, SLOT(insertNewTelescope()));
	connect(ui->deleteTelescope, SIGNAL(clicked()), this, SLOT(deleteSelectedTelescope()));

	// Validators
	ui->ccdName->setValidator(validatorName);
	ui->ocularName->setValidator(validatorName);
	ui->telescopeName->setValidator(validatorName);
	ui->lensName->setValidator(validatorName);

	initAboutText();

	connect(ui->pushButtonMoveOcularUp,      SIGNAL(pressed()), this, SLOT(moveUpSelectedOcular()));
	connect(ui->pushButtonMoveOcularDown,    SIGNAL(pressed()), this, SLOT(moveDownSelectedOcular()));
	connect(ui->pushButtonMoveSensorUp,      SIGNAL(pressed()), this, SLOT(moveUpSelectedSensor()));
	connect(ui->pushButtonMoveSensorDown,    SIGNAL(pressed()), this, SLOT(moveDownSelectedSensor()));
	connect(ui->pushButtonMoveTelescopeUp,   SIGNAL(pressed()), this, SLOT(moveUpSelectedTelescope()));
	connect(ui->pushButtonMoveTelescopeDown, SIGNAL(pressed()), this, SLOT(moveDownSelectedTelescope()));
	connect(ui->pushButtonMoveLensUp,        SIGNAL(pressed()), this, SLOT(moveUpSelectedLens()));
	connect(ui->pushButtonMoveLensDown,      SIGNAL(pressed()), this, SLOT(moveDownSelectedLens()));

	// The CCD mapper
	ccdMapper = new QDataWidgetMapper();
	ccdMapper->setModel(ccdTableModel);
	ccdMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	ccdMapper->addMapping(ui->ccdName,       0);
	ccdMapper->addMapping(ui->ccdChipY,      1);
	ccdMapper->addMapping(ui->ccdChipX,      2);
	ccdMapper->addMapping(ui->ccdPixelY,     3);
	ccdMapper->addMapping(ui->ccdPixelX,     4);
	ccdMapper->addMapping(ui->ccdResX,       5);
	ccdMapper->addMapping(ui->ccdResY,       6);
	ccdMapper->addMapping(ui->ccdRotAngle,   7);
	ccdMapper->addMapping(ui->ccdBinningX,   8);
	ccdMapper->addMapping(ui->ccdBinningY,   9);
	ccdMapper->addMapping(ui->OAG_checkBox, 10);
	ccdMapper->addMapping(ui->OAGPrismH,    11);
	ccdMapper->addMapping(ui->OAGPrismW,    12);
	ccdMapper->addMapping(ui->OAGDist,      13);
	ccdMapper->addMapping(ui->OAGPrismPA,   14);
	ccdMapper->toFirst();
	connect(ui->ccdListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
		ccdMapper, SLOT(setCurrentModelIndex(QModelIndex)));
	connectDoubleProperty(ui->ccdRotAngle, "Oculars.selectedCCDRotationAngle");
	ui->ccdListView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->ccdListView->setCurrentIndex(ccdTableModel->index(0, 1));

	connect(ui->ccdChipY,    SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->ccdChipX,    SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->ccdPixelY,   SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->ccdPixelX,   SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->ccdResX,     SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->ccdResY,     SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->ccdRotAngle, SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->ccdBinningX, SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->ccdBinningY, SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->OAG_checkBox,SIGNAL(stateChanged(int)), ccdMapper, SLOT(submit()));
	connect(ui->OAGPrismH,   SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->OAGPrismW,   SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->OAGDist,     SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));
	connect(ui->OAGPrismPA,  SIGNAL(editingFinished()), ccdMapper, SLOT(submit()));

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
	connect(ui->ocularListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
		ocularMapper, SLOT(setCurrentModelIndex(QModelIndex)));
	ui->ocularListView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->ocularListView->setCurrentIndex(ocularTableModel->index(0, 1));

	// We need particular refresh methods to see immediate feedback.
	connect(ui->ocularAFov,                 SIGNAL(editingFinished()), this, SLOT(updateOcular()));
	connect(ui->ocularFL,                   SIGNAL(editingFinished()), this, SLOT(updateOcular()));
	connect(ui->ocularFieldStop,            SIGNAL(editingFinished()), this, SLOT(updateOcular()));
	connect(ui->binocularsCheckBox,         SIGNAL(stateChanged(int)), this, SLOT(updateOcular()));
	connect(ui->permanentCrosshairCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateOcular()));

	// The lens mapper
	lensMapper = new QDataWidgetMapper();
	lensMapper->setModel(lensTableModel);
	lensMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	lensMapper->addMapping(ui->lensName,       0);
	lensMapper->addMapping(ui->lensMultiplier, 1);
	lensMapper->toFirst();
	connect(ui->lensListView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
		lensMapper, SLOT(setCurrentModelIndex(QModelIndex)));
	ui->lensListView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->lensListView->setCurrentIndex(lensTableModel->index(0, 1));

	connect(ui->lensMultiplier, SIGNAL(editingFinished()), this, SLOT(updateLens()));

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
	connect(ui->telescopeListView->selectionModel() , SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
		telescopeMapper, SLOT(setCurrentModelIndex(QModelIndex)));
	ui->telescopeListView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->telescopeListView->setCurrentIndex(telescopeTableModel->index(0, 1));

	connect(ui->telescopeDiameter, SIGNAL(editingFinished()), this, SLOT(updateTelescope()));
	connect(ui->telescopeFL,       SIGNAL(editingFinished()), this, SLOT(updateTelescope()));
	connect(ui->telescopeHFlip,    SIGNAL(stateChanged(int)), this, SLOT(updateTelescope()));
	connect(ui->telescopeVFlip,    SIGNAL(stateChanged(int)), this, SLOT(updateTelescope()));
	connect(ui->telescopeEQ,       SIGNAL(stateChanged(int)), this, SLOT(updateTelescope()));

	connect(ui->binocularsCheckBox, SIGNAL(toggled(bool)), this, SLOT(setLabelsDescriptionText(bool)));
}

// We need particular refresh methods to see immediate feedback.
void OcularDialog::updateOcular()
{
	ocularMapper->submit();
	plugin->selectOcularAtIndex(plugin->getSelectedOcularIndex());
}

void OcularDialog::updateLens()
{
	lensMapper->submit();
	plugin->selectLensAtIndex(plugin->getSelectedLensIndex());
}

void OcularDialog::updateTelescope()
{
	telescopeMapper->submit();
	plugin->selectTelescopeAtIndex(plugin->getSelectedTelescopeIndex());
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
	}
	else
	{
		ui->labelFOV->setText(q_("aFOV:"));
		ui->labelFL->setText(q_("Focal length:"));
		ui->labelFS->setText(q_("Field stop:"));
	}
}

void OcularDialog::initAboutText()
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	//BM: Most of the text for now is the original contents of the About widget.
	QString html = "<html><head><title></title></head><body>";

	html += "<h2>" + q_("Oculars Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + OCULARS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + OCULARS_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Timothy Reaves &lt;treaves@silverfieldstech.com&gt;</td></tr>";
	html += "<tr><td rowspan=5><strong>" + q_("Contributors") + ":</strong></td><td>Bogdan Marinov</td></tr>";
	html += "<tr><td>Pawel Stolowski (" + q_("Barlow lens feature") + ")</td></tr>";
	html += "<tr><td>Alexander Wolf</td></tr>";
	html += "<tr><td>Rumen G. Bogdanovski &lt;rumen@skyarchive.org&gt;</td></tr>";
	html += "<tr><td>Georg Zotti</td></tr>";
	html += "<tr><td>Matt Hughes (" + q_("Sensor crop overlay feature") + ")</td></tr>";
	html += "</table>";

	//Overview
	html += "<h3>" + q_("Overview") + "</h3>";

	html += "<p>" + q_("This plugin is intended to simulate what you would see through an eyepiece.  This configuration dialog can be used to add, modify, or delete eyepieces and telescopes, as well as CCD Sensors.  Your first time running the app will populate some samples to get your started.") + "</p>";
	html += "<p>" + q_("You can choose to scale the image you see on the screen.") + " ";
	html +=         q_("This is intended to show you a better comparison of what one eyepiece/telescope combination will be like when compared to another.") + " ";
	html +=         q_("The same eyepiece in two different telescopes of differing focal length will produce two different exit pupils, changing the view somewhat.") + " ";
	html +=         q_("The trade-off of this is that, with the image scaled, a large part of the screen can be wasted.") + " ";
	html +=         q_("Therefore I recommend that you leave it off, unless you feel you have a need of it.") + "</p>";
	html += "<p>" + q_("You can toggle a crosshair in the view.  Ideally, I wanted this to be aligned to North.  I've been unable to do so.  So currently it aligns to the top of the screen.") + "</p>";
	html += "<p>" + QString(q_("You can toggle a Telrad finder; this can only be done when you have not turned on the Ocular view.  This feature draws three concentric circles of 0.5%1, 2.0%1, and 4.0%1, helping you see what you would expect to see with the naked eye through the Telrad (or similar) finder.")).arg(QChar(0x00B0)) + "</p>";
	html += "<p>" + q_("If you find any issues, please let me know.  Enjoy!") + "</p>";

	//Keys
	html += "<h3>" + q_("Hot Keys") + "</h3>";
	html += "<p>" + q_("The plug-in's key bindings can be edited in the Keyboard shortcuts editor (F7).") + "</p>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	Q_ASSERT(actionMgr);
	StelAction* actionOcular = actionMgr->findAction("actionShow_Ocular");
	Q_ASSERT(actionOcular);
	StelAction* actionMenu = actionMgr->findAction("actionShow_Ocular_Menu");
	Q_ASSERT(actionMenu);
	QKeySequence ocularShortcut = actionOcular->getShortcut();
	QString ocularString = ocularShortcut.toString(QKeySequence::NativeText);
	ocularString = ocularString.toHtmlEscaped();
	if (ocularString.isEmpty())
		ocularString = q_("[no key defined]");
	QKeySequence menuShortcut = actionMenu->getShortcut();
	QString menuString = menuShortcut.toString(QKeySequence::NativeText);
	menuString = menuString.toHtmlEscaped();
	if (menuString.isEmpty())
		menuString = q_("[no key defined]");

	html += "<ul>";
	html += "<li>";
	html += QString("<strong>%1:</strong> %2").arg(ocularString).arg(q_("Switches on/off the ocular overlay."));
	html += "</li>";
	
	html += "<li>";
	html += QString("<strong>%1:</strong> %2").arg(menuString).arg(q_("Opens the pop-up navigation menu."));
	html += "</li>";

	html += "<li>";
	html += QString("<strong>%1:</strong> %2").arg("Alt+M").arg(q_("Rotate reticle pattern of the eyepiece clockwise."));
	html += "</li>";

	html += "<li>";
	html += QString("<strong>%1:</strong> %2").arg("Shift+Alt+M").arg(q_("Rotate reticle pattern of the eyepiece сounterclockwise."));
	html += "</li>";

	html += "</ul>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Oculars plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Oculars_plugin\">\\1</a>") + "</li>";
	html += "</ul></p></body></html>";

	QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
	ui->textBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);

	ui->textBrowser->setHtml(html);
}
