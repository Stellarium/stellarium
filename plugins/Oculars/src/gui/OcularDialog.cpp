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

OcularDialog::OcularDialog(Oculars* pluginPtr, QList<CCD *>* ccds, QList<Ocular *>* oculars, QList<Telescope *>* telescopes, QList<Barlow *>* barlows) :
	plugin(pluginPtr)
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
												 ocularModel, ocularModel->propertyMap());
	this->telescopes = telescopes;
	telescopeTableModel = new PropertyBasedTableModel(this);
	Telescope* telescopeModel = Telescope::telescopeModel();
	telescopeTableModel->init(reinterpret_cast<QList<QObject *>* >(telescopes),
														telescopeModel,
														telescopeModel->propertyMap());
	
	this->barlows = barlows;
	barlowTableModel = new PropertyBasedTableModel(this);
	Barlow* barlowModel = Barlow::barlowModel();
	barlowTableModel->init(reinterpret_cast<QList<QObject *>* >(barlows), barlowModel, barlowModel->propertyMap());

	validatorPositiveInt = new QIntValidator(0, std::numeric_limits<int>::max(), this);
	validatorPositiveDouble = new QDoubleValidator(.0, std::numeric_limits<double>::max(), 24, this);
	validatorOcularAFOV = new QDoubleValidator(1.0, 120.0, 1, this);
	validatorOcularEFL = new QDoubleValidator(1.0, 60.0, 1, this);
	validatorTelescopeDiameter = new QDoubleValidator(1.0, 1000.0, 1, this);
	validatorTelescopeFL = new QDoubleValidator(1.0, 10000.0, 1, this);
	validatorBarlowMultipler = new QDoubleValidator(1.0, 6.0, 1, this);
	QRegExp nameExp("^\\S.*");
	validatorName = new QRegExpValidator(nameExp, this);
}

OcularDialog::~OcularDialog()
{
	ocularTableModel->disconnect();
	telescopeTableModel->disconnect();
	ccdTableModel->disconnect();
	barlowTableModel->disconnect();

	delete ui;
	ui = NULL;
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
		const StelStyle pluginStyle = plugin->getModuleStyleSheet(gui->getStelStyle());
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
	plugin->updateLists();
}

void OcularDialog::deleteSelectedOcular()
{
	if (ocularTableModel->rowCount() == 1) {
		qDebug() << "Can not delete the last entry.";
	} else {
		ocularTableModel->removeRows(ui->ocularListView->currentIndex().row(), 1);
		ui->ocularListView->setCurrentIndex(ocularTableModel->index(0, 1));
		plugin->updateLists();
	}
}

void OcularDialog::deleteSelectedTelescope()
{
	if (telescopeTableModel->rowCount() == 1) {
		qDebug() << "Can not delete the last entry.";
	} else {
		telescopeTableModel->removeRows(ui->telescopeListView->currentIndex().row(), 1);
		ui->telescopeListView->setCurrentIndex(telescopeTableModel->index(0, 1));
		plugin->updateLists();
	}
}

void OcularDialog::deleteSelectedBarlow()
{
    if (barlowTableModel->rowCount() > 0) {
        barlowTableModel->removeRows(ui->barlowListView->currentIndex().row(), 1);
        if (barlowTableModel->rowCount() > 0) {
            ui->barlowListView->setCurrentIndex(barlowTableModel->index(0, 1));
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

void OcularDialog::insertNewBarlow()
{
	barlowTableModel->insertRows(barlowTableModel->rowCount(), 1);
	ui->barlowListView->setCurrentIndex(barlowTableModel->index(barlowTableModel->rowCount() - 1, 1));
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

void OcularDialog::moveUpSelectedBarlow()
{
	int index = ui->barlowListView->currentIndex().row();
	if (index > 0)
	{
		barlowTableModel->moveRowUp(index);
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

void OcularDialog::moveDownSelectedBarlow()
{
	int index = ui->barlowListView->currentIndex().row();
	if (index >= 0 && index < barlowTableModel->rowCount() - 1)
	{
		barlowTableModel->moveRowDown(index);
		plugin->updateLists();
	}
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
	QAction* action = gui->getGuiAction("actionShow_Ocular");
	if (action != NULL) {
		action->setShortcut(QKeySequence(newString.trimmed()));
	}
}

void OcularDialog::keyBindingPopupNavigatorConfigChanged(const QString& newString)
{
	Oculars::appSettings()->setValue("bindings/popup_navigator", newString);
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	QAction* action = gui->getGuiAction("actionShow_Ocular_Menu");
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
		Oculars::appSettings()->sync();
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
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
					this, SLOT(retranslate()));
	ui->ccdListView->setModel(ccdTableModel);
	ui->ocularListView->setModel(ocularTableModel);
	ui->telescopeListView->setModel(telescopeTableModel);
	ui->barlowListView->setModel(barlowTableModel);
	
	//Now the rest of the actions.
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->scaleImageCircleCheckBox, SIGNAL(stateChanged(int)), this, SLOT(scaleImageCircleStateChanged(int)));
	connect(ui->requireSelectionCheckBox, SIGNAL(stateChanged(int)), this, SLOT(requireSelectionStateChanged(int)));
	connect(ui->checkBoxControlPanel, SIGNAL(clicked(bool)), plugin, SLOT(enableGuiPanel(bool)));
	connect(ui->checkBoxDecimalDegrees, SIGNAL(clicked(bool)), plugin, SLOT(setFlagDecimalDegrees(bool)));
	
	// The add & delete buttons
	connect(ui->addCCD, SIGNAL(clicked()), this, SLOT(insertNewCCD()));
	connect(ui->deleteCCD, SIGNAL(clicked()), this, SLOT(deleteSelectedCCD()));
	connect(ui->addOcular, SIGNAL(clicked()), this, SLOT(insertNewOcular()));
	connect(ui->deleteOcular, SIGNAL(clicked()), this, SLOT(deleteSelectedOcular()));
	connect(ui->addBarlow, SIGNAL(clicked()), this, SLOT(insertNewBarlow()));
	connect(ui->deleteBarlow, SIGNAL(clicked()), this, SLOT(deleteSelectedBarlow()));
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
	ui->barlowName->setValidator(validatorName);
	ui->barlowMultipler->setValidator(validatorBarlowMultipler);
	
	// The key bindings
	QString bindingString = Oculars::appSettings()->value("bindings/toggle_oculars", "Ctrl+O").toString();
	ui->togglePluginLineEdit->setText(bindingString);
	bindingString = Oculars::appSettings()->value("bindings/popup_navigator", "Alt+O").toString();
	ui->togglePopupNavigatorWindowLineEdit->setText(bindingString);
	connect(ui->togglePluginLineEdit, SIGNAL(textEdited(const QString&)),
					this, SLOT(keyBindingTogglePluginChanged(const QString&)));
	connect(ui->togglePopupNavigatorWindowLineEdit, SIGNAL(textEdited(const QString&)),
					this, SLOT(keyBindingPopupNavigatorConfigChanged(const QString&)));
	
	initAboutText();
	connect(ui->togglePluginLineEdit, SIGNAL(textEdited(QString)),
					this, SLOT(initAboutText()));
	connect(ui->togglePopupNavigatorWindowLineEdit, SIGNAL(textEdited(QString)),
					this, SLOT(initAboutText()));

	connect(ui->pushButtonMoveOcularUp, SIGNAL(pressed()),
					this, SLOT(moveUpSelectedOcular()));
	connect(ui->pushButtonMoveOcularDown, SIGNAL(pressed()),
					this, SLOT(moveDownSelectedOcular()));
	connect(ui->pushButtonMoveSensorUp, SIGNAL(pressed()),
					this, SLOT(moveUpSelectedSensor()));
	connect(ui->pushButtonMoveSensorDown, SIGNAL(pressed()),
					this, SLOT(moveDownSelectedSensor()));
	connect(ui->pushButtonMoveTelescopeUp, SIGNAL(pressed()),
					this, SLOT(moveUpSelectedTelescope()));
	connect(ui->pushButtonMoveTelescopeDown, SIGNAL(pressed()),
					this, SLOT(moveDownSelectedTelescope()));
	connect(ui->pushButtonMoveBarlowUp, SIGNAL(pressed()),
					this, SLOT(moveUpSelectedBarlow()));
	connect(ui->pushButtonMoveBarlowDown, SIGNAL(pressed()),
					this, SLOT(moveDownSelectedBarlow()));

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

	// The barlow lens mapper
	barlowMapper = new QDataWidgetMapper();
	barlowMapper->setModel(barlowTableModel);
	barlowMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
	barlowMapper->addMapping(ui->barlowName, 0);
	barlowMapper->addMapping(ui->barlowMultipler, 1);
	barlowMapper->toFirst();
	connect(ui->barlowListView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
			barlowMapper, SLOT(setCurrentModelIndex(QModelIndex)));
	ui->barlowListView->setCurrentIndex(barlowTableModel->index(0, 1));

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
	if (Oculars::appSettings()->value("enable_control_panel", false).toBool())
	{
		ui->checkBoxControlPanel->setChecked(true);
	}
	if (Oculars::appSettings()->value("use_decimal_degrees", false).toBool())
	{
		ui->checkBoxDecimalDegrees->setChecked(true);
	}

	//Initialize the style
	updateStyle();
}

void OcularDialog::initAboutText()
{
	//BM: Most of the text for now is the original contents of the About widget.
	QString html = "<html><head><title></title></head><body>";

	html += "<h2>" + q_("Oculars Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + OCULARS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Authors") + ":</strong></td><td>Timothy Reaves &lt;treaves@silverfieldstech.com&gt;<br />Bogdan Marinov</td></tr>";
	html += "</table>";

	//Overview
	html += "<h2>" + q_("Overview") + "</h2>";

	html += "<p>" + q_("This plugin is intended to simulate what you would see through an eyepiece.  This configuration dialog can be used to add, modify, or delete eyepieces and telescopes, as well as CCD Sensors.  Your first time running the app will populate some samples to get your started.") + "</p>";
	html += "<p>" + q_("You can choose to scale the image you see on the screen.  This is intended to show you a better comparison of what one eyepiece/telescope combination will be like as compared to another.  The same eyepiece in two different telescopes of differing focal length will produce two different exit circles, changing the view someone.  The trade-off of this is that, with the image scaled, a good deal of the screen can be wasted.  Therefore I recommend that you leave it off, unless you feel you have a need of it.") + "</p>";
	html += "<p>" + q_("You can toggle a crosshair in the view.  Ideally, I wanted this to be aligned to North.  I've been unable to do so.  So currently it aligns to the top of the screen.") + "</p>";
	html += "<p>" + QString(q_("You can toggle a Telrad finder; this can only be done when you have not turned on the Ocular view.  This feature draws three concentric circles of 0.5%1, 2.0%1, and 4.0%1, helping you see what you would expect to see with the naked eye through the Telrad (or similar) finder.")).arg(QChar(0x00B0)) + "</p>";
	html += "<p>" + q_("If you find any issues, please let me know.  Enjoy!") + "</p>";

	//Keys
	html += "<h2>" + q_("Hot Keys") + "</h2>";
	html += "<p>" + q_("The plug-in's key bindings can be edited in the General Tab.") + "</p>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	QAction* actionOcular = gui->getGuiAction("actionShow_Ocular");
	Q_ASSERT(actionOcular);
	QAction* actionMenu = gui->getGuiAction("actionShow_Ocular_Menu");
	Q_ASSERT(actionMenu);
	QKeySequence ocularShortcut = actionOcular->shortcut();
	QString ocularString = ocularShortcut.toString(QKeySequence::NativeText);
	ocularString = Qt::escape(ocularString);
	if (ocularString.isEmpty())
		ocularString = q_("[no key defined]");
	QKeySequence menuShortcut = actionMenu->shortcut();
	QString menuString = menuShortcut.toString(QKeySequence::NativeText);
	menuString = Qt::escape(menuString);
	if (menuString.isEmpty())
		menuString = q_("[no key defined]");

	html += "<ul>";
	html += "<li>";
	html += QString("<strong>%1:</strong> %2").arg(ocularString).arg(q_("Switches on/off the ocular overlay."));
	html += "</li>";
	
	html += "<li>";
	html += QString("<strong>%1:</strong> %2").arg(menuString).arg(q_("Opens the pop-up navigation menu."));
	html += "</li>";
	html += "</ul>";
	html += "</body></html>";

	QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
	ui->textBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);

	ui->textBrowser->setHtml(html);
}
