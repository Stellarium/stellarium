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
#include "OcularsGuiPanel.hpp"

#include "GridLinesMgr.hpp"
#include "LabelMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelPainter.hpp"
#include "StelProjector.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelMainWindow.hpp"
#include "StelTranslator.hpp"
#include "SkyGui.hpp"

#include <QAction>
#include <QGraphicsWidget>
#include <QKeyEvent>
#include <QDebug>
#include <QMenu>
#include <QMouseEvent>
#include <QtNetwork>
#include <QPixmap>
#include <QSignalMapper>

#include <cmath>

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>	/* Header File For The GLU Library */
#else
#include <GL/glu.h>	/* Header File For The GLU Library */
#endif

#ifdef Q_WS_MAC
extern void qt_set_sequence_auto_mnemonic(bool b);
#endif

static QSettings *settings; //!< The settings as read in from the ini file.

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark StelModuleMgr Methods
#endif
/* ********************************************************************* */
//! This method is the one called automatically by the StelModuleMgr just
//! after loading the dynamic library
StelModule* OcularsStelPluginInterface::getStelModule() const
{
	return new Oculars();
}

StelPluginInfo OcularsStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(Oculars);

	StelPluginInfo info;
	info.id = "Oculars";
	info.displayedName = N_("Oculars");
	info.authors = "Timothy Reaves, Bogdan Marinov";
	info.contact = "treaves@silverfieldstech.com";
	info.description = N_("Shows the sky as if looking through a telescope eyepiece. (Only magnification and field of view are simulated.) It can also show a sensor frame and a Telrad sight.");
	return info;
}

Q_EXPORT_PLUGIN2(Oculars, OcularsStelPluginInterface)


/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Instance Methods
#endif
/* ********************************************************************* */
Oculars::Oculars():
	pxmapGlow(NULL),
	pxmapOnIcon(NULL),
	pxmapOffIcon(NULL),
	toolbarButton(NULL),
	actionShowOcular(0),
	actionShowCrosshairs(0),
	actionShowSensor(0),
	actionShowTelrad(0),
	guiPanel(0)
{
	flagShowCCD = false;
	flagShowOculars = false;
	flagShowCrosshairs = false;
	flagShowTelrad = false;
	ready = false;
	requireSelection = true;
	useMaxEyepieceAngle = true;

	font.setPixelSize(14);
	maxEyepieceAngle = 0.0;

	ccds = QList<CCD *>();
	oculars = QList<Ocular *>();
	telescopes = QList<Telescope *>();
	ccdRotationSignalMapper = new QSignalMapper(this);
	ccdsSignalMapper = new QSignalMapper(this);
	ocularsSignalMapper = new QSignalMapper(this);
	telescopesSignalMapper = new QSignalMapper(this);
	
	selectedCCDIndex = -1;
	selectedOcularIndex = -1;
	selectedTelescopeIndex = -1;
	
	usageMessageLabelID = -1;

	setObjectName("Oculars");

#ifdef Q_WS_MAC
	qt_set_sequence_auto_mnemonic(true);
#endif
}

Oculars::~Oculars()
{
	delete ocularDialog;
	ocularDialog = NULL;
	if (guiPanel)
		delete guiPanel;
	
	if (pxmapGlow)
		delete pxmapGlow;
	if (pxmapOnIcon)
		delete pxmapOnIcon;
	if (pxmapOffIcon)
		delete pxmapOffIcon;
	
	qDeleteAll(ccds);
	ccds.clear();
	qDeleteAll(telescopes);
	telescopes.clear();
	qDeleteAll(oculars);
	oculars.clear();
}

QSettings* Oculars::appSettings()
{
	return settings;
}


/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark StelModule Methods
#endif
/* ********************************************************************* */
bool Oculars::configureGui(bool show)
{
	if (show) {
		ocularDialog->setVisible(true);
	}

	return ready;
}

void Oculars::deinit()
{
	// update the ini file.
	settings->remove("ccd");
	settings->remove("ocular");
	settings->remove("telescope");
	int index = 0;
	foreach(CCD* ccd, ccds) {
		QString prefix = "ccd/" + QVariant(index).toString() + "/";
		settings->setValue(prefix + "name", ccd->name());
		settings->setValue(prefix + "resolutionX", ccd->resolutionX());
		settings->setValue(prefix + "resolutionY", ccd->resolutionY());
		settings->setValue(prefix + "chip_width", ccd->chipWidth());
		settings->setValue(prefix + "chip_height", ccd->chipHeight());
		settings->setValue(prefix + "pixel_width", ccd->pixelWidth());
		settings->setValue(prefix + "pixel_height", ccd->pixelWidth());
		index++;
	}
	index = 0;
	foreach(Ocular* ocular, oculars) {
		QString prefix = "ocular/" + QVariant(index).toString() + "/";
		settings->setValue(prefix + "name", ocular->name());
		settings->setValue(prefix + "afov", ocular->appearentFOV());
		settings->setValue(prefix + "efl", ocular->effectiveFocalLength());
		settings->setValue(prefix + "fieldStop", ocular->fieldStop());
		settings->setValue(prefix + "binoculars", ocular->isBinoculars());
		index++;
	}
	index = 0;
	foreach(Telescope* telescope, telescopes){
		QString prefix = "telescope/" + QVariant(index).toString() + "/";
		settings->setValue(prefix + "name", telescope->name());
		settings->setValue(prefix + "focalLength", telescope->focalLength());
		settings->setValue(prefix + "diameter", telescope->diameter());
		settings->setValue(prefix + "hFlip", telescope->isHFlipped());
		settings->setValue(prefix + "vFlip", telescope->isVFlipped());
		index++;
	}
	settings->setValue("ocular_count", oculars.count());
	settings->setValue("telescope_count", telescopes.count());
	settings->setValue("ccd_count", ccds.count());
	settings->sync();
}

//! Draw any parts on the screen which are for our module
void Oculars::draw(StelCore* core)
{
	if (flagShowTelrad) {
		paintTelrad();
	} else if (flagShowOculars){
		// Ensure there is a selected ocular & telescope
		if (selectedCCDIndex > ccds.count()) {
			qWarning() << "Oculars: the selected sensor index of "
								 << selectedCCDIndex << " is greater than the sensor count of "
								 << ccds.count() << ". Module disabled!";
			ready = false;
		}
		if (selectedOcularIndex > oculars.count()) {
			qWarning() << "Oculars: the selected ocular index of "
								 << selectedOcularIndex << " is greater than the ocular count of "
								 << oculars.count() << ". Module disabled!";
			ready = false;
		}
		else if (selectedTelescopeIndex > telescopes.count()) {
			qWarning() << "Oculars: the selected telescope index of "
								 << selectedTelescopeIndex << " is greater than the telescope count of "
								 << telescopes.count() << ". Module disabled!";
			ready = false;
		}
		
		if (ready) {
			if (selectedOcularIndex > -1) {
				paintOcularMask();
				if (flagShowCrosshairs)  {
					paintCrosshairs();
				}
			}
			if (guiPanelEnabled)
			{
				// Reset the state to allow the panel to be painted normally
				glDisable(GL_TEXTURE_2D);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}
			else
			{
				// Paint the information in the upper-right hand corner
				paintText(core);
			}
		}
	} else if (flagShowCCD) {
		paintCCDBounds();
		if (guiPanelEnabled)
		{
			// Reset the state to allow the panel to be painted normally
			glDisable(GL_TEXTURE_2D);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
		}
		else
		{
			// Paint the information in the upper-right hand corner
			paintText(core);
		}
	}
}

//! Determine which "layer" the plugin's drawing will happen on.
double Oculars::getCallOrder(StelModuleActionName actionName) const
{
	// TODO; this really doesn't seem to have any effect.  I've tried everything from -100 to +100,
	//		and a calculated value.  It all seems to work the same regardless.
	double order = 1000.0;
	if (actionName==StelModule::ActionHandleKeys || actionName==StelModule::ActionHandleMouseMoves)
	{
		order = StelApp::getInstance().getModuleMgr().getModule("StelMovementMgr")->getCallOrder(actionName) - 1.0;
	}
	else if (actionName==StelModule::ActionDraw)
	{
		order = GETSTELMODULE(LabelMgr)->getCallOrder(actionName) + 100.0;
	}

	return order;
}

const StelStyle Oculars::getModuleStyleSheet(const StelStyle& style)
{
	StelStyle pluginStyle(style);
	if (style.confSectionName == "color") {
		pluginStyle.qtStyleSheet.append(normalStyleSheet);
	} else{
		pluginStyle.qtStyleSheet.append(nightStyleSheet);
	}
	return pluginStyle;
}

void Oculars::handleMouseClicks(class QMouseEvent* event)
{
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected()){
		if (flagShowOculars) {
			// center the selected object in the ocular, and track.
			movementManager->setFlagTracking(true);
		} else {
			// remove the usage label if it is being displayed.
			hideUsageMessageIfDisplayed();
		}
	} else if(flagShowOculars) {
		//TODO: this is broke in Stellarium.
		// The ocular is displayed, but no object is selected.  So don't track the stars.  We may have locked
		// the position of the screen if the movement keys were used.  so call this to be on the safe side.
		movementManager->setFlagLockEquPos(false);
		// Do we need to set this?
		// movementManager->setFlagTracking(false);
	}
	event->setAccepted(false);
}

void Oculars::handleKeys(QKeyEvent* event)
{
	if (!flagShowOculars && !flagShowCCD) {
		return;
	}
	// We onle care about the arrow keys.  This flag tracks that.
	bool consumeEvent = false;
	
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	
	if (event->type() == QEvent::KeyPress) {
		// Direction and zoom replacements
		switch (event->key()) {
			case Qt::Key_Left:
				movementManager->turnLeft(true);
				consumeEvent = true;
				break;
			case Qt::Key_Right:
				movementManager->turnRight(true);
				consumeEvent = true;
				break;
			case Qt::Key_Up:
				if (!event->modifiers().testFlag(Qt::ControlModifier)) {
					movementManager->turnUp(true);
				}
				consumeEvent = true;
				break;
			case Qt::Key_Down:
				if (!event->modifiers().testFlag(Qt::ControlModifier)) {
					movementManager->turnDown(true);
				}
				consumeEvent = true;
				break;
			case Qt::Key_PageUp:
				movementManager->zoomIn(true);
				consumeEvent = true;
				break;
			case Qt::Key_PageDown:
				movementManager->zoomOut(true);
				consumeEvent = true;
				break;
			case Qt::Key_Shift:
				movementManager->moveSlow(true);
				consumeEvent = true;
				break;
		}
	} else {
		// When a deplacement key is released stop mooving
		switch (event->key()) {
			case Qt::Key_Left:
				movementManager->turnLeft(false);
				consumeEvent = true;
				break;
			case Qt::Key_Right:
				movementManager->turnRight(false);
				consumeEvent = true;
				break;
			case Qt::Key_Up:
				movementManager->turnUp(false);
				consumeEvent = true;
				break;
			case Qt::Key_Down:
				movementManager->turnDown(false);
				consumeEvent = true;
				break;
			case Qt::Key_PageUp:
				movementManager->zoomIn(false);
				consumeEvent = true;
				break;
			case Qt::Key_PageDown:
				movementManager->zoomOut(false);
				consumeEvent = true;
				break;
			case Qt::Key_Shift:
				movementManager->moveSlow(false);
				consumeEvent = true;
				break;
		}
		if (consumeEvent) {
			// We don't want to re-center the object; just hold the current position.
			movementManager->setFlagLockEquPos(true);
		}
	}
	if (consumeEvent) {
		event->accept();
	} else {
		event->setAccepted(false);
	}
}

void Oculars::init()
{
	qDebug() << "Ocular plugin - press Command-O to toggle eyepiece view mode. Press ALT-o for configuration.";

	// Load settings from ocular.ini
	try {
		validateAndLoadIniFile();
		// assume all is well
		ready = true;

		requireSelection = settings->value("require_selection_to_zoom", 1.0).toBool();
		useMaxEyepieceAngle = settings->value("use_max_exit_circle", 0.0).toBool();
		int ocularCount = settings->value("ocular_count", 0).toInt();
		int actualOcularCount = ocularCount;
		for (int index = 0; index < ocularCount; index++) {
			Ocular *newOcular = Ocular::ocularFromSettings(settings, index);
			if (newOcular != NULL) {
				oculars.append(newOcular);
			} else {
				actualOcularCount--;
			}
		}
		if (actualOcularCount < 1) {
			if (actualOcularCount < ocularCount) {
				qWarning() << "The Oculars ini file appears to be corrupt; delete it.";
			} else {
				qWarning() << "There are no oculars defined for the Oculars plugin; plugin will be disabled.";
			}
			ready = false;
		} else {
			selectedOcularIndex = 0;
		}

		int ccdCount = settings->value("ccd_count", 0).toInt();
		int actualCcdCount = ccdCount;
		for (int index = 0; index < ccdCount; index++) {
			CCD *newCCD = CCD::ccdFromSettings(settings, index);
			if (newCCD != NULL) {
				ccds.append(newCCD);
			} else {
				actualCcdCount--;
			}
		}
		if (actualCcdCount < ccdCount) {
			qWarning() << "The Oculars ini file appears to be corrupt; delete it.";
			ready = false;
		}

		int telescopeCount = settings->value("telescope_count", 0).toInt();
		int actualTelescopeCount = telescopeCount;
		for (int index = 0; index < telescopeCount; index++) {
			Telescope *newTelescope = Telescope::telescopeFromSettings(settings, index);
			if (newTelescope != NULL) {
				telescopes.append(newTelescope);
			}else {
				actualTelescopeCount--;
			}
		}
		if (actualTelescopeCount < 1) {
			if (actualTelescopeCount < telescopeCount) {
				qWarning() << "The Oculars ini file appears to be corrupt; delete it.";
			} else {
				qWarning() << "There are no telescopes defined for the Oculars plugin; plugin will be disabled.";
			}
			ready = false;
		} else {
			selectedTelescopeIndex = 0;
		}

		ocularDialog = new OcularDialog(this, &ccds, &oculars, &telescopes);
		initializeActivationActions();
		determineMaxEyepieceAngle();
		
		guiPanelEnabled = settings->value("enable_control_panel", false).toBool();
		enableGuiPanel(guiPanelEnabled);

		setFlagDecimalDegrees(settings->value("use_decimal_degrees", false).toBool());
	} catch (std::runtime_error& e) {
		qWarning() << "WARNING: unable to locate ocular.ini file or create a default one for Ocular plugin: " << e.what();
		ready = false;
	}

	//Load the module's custom style sheets
	QFile styleSheetFile;
	styleSheetFile.setFileName(":/ocular/normalStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text)) {
		normalStyleSheet = styleSheetFile.readAll();
	}
	styleSheetFile.close();
	styleSheetFile.setFileName(":/ocular/nightStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text)) {
		nightStyleSheet = styleSheetFile.readAll();
	}
	styleSheetFile.close();
	connect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(const QString&)),
					this, SLOT(setStelStyle(const QString&)));
	
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
					this, SLOT(retranslateGui()));
}

void Oculars::setStelStyle(const QString&)
{
	if(ocularDialog) {
		ocularDialog->updateStyle();
	}
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Private slots Methods
#endif
/* ********************************************************************* */
void Oculars::determineMaxEyepieceAngle()
{
	if (ready) {
		foreach (Ocular* ocular, oculars) {
			if (ocular->appearentFOV() > maxEyepieceAngle) {
				maxEyepieceAngle = ocular->appearentFOV();
			}
		}
	}
	// insure it is not zero
	if (maxEyepieceAngle == 0.0) {
		maxEyepieceAngle = 1.0;
	}
}

void Oculars::instrumentChanged()
{
	// We only zoom if in ocular mode.
	if (flagShowOculars) {
		zoom(true);
	}
}

void Oculars::setRequireSelection(bool state)
{
	requireSelection = state;
}

void Oculars::setScaleImageCircle(bool state)
{
	if (state) {
		determineMaxEyepieceAngle();
	}
	useMaxEyepieceAngle = state;
}

void Oculars::setScreenFOVForCCD()
{
	if (selectedCCDIndex > -1 && selectedTelescopeIndex > -1) {
		StelCore *core = StelApp::getInstance().getCore();
		StelMovementMgr *movementManager = core->getMovementMgr();
		double actualFOVx = ccds[selectedCCDIndex]->getActualFOVx(telescopes[selectedTelescopeIndex]);
		double actualFOVy = ccds[selectedCCDIndex]->getActualFOVy(telescopes[selectedTelescopeIndex]);
		if (actualFOVx < actualFOVy) {
			actualFOVx = actualFOVy;
		}
		movementManager->setFlagTracking(true);
		movementManager->zoomTo(actualFOVx * 3.0, 0.0);
	}
}

void Oculars::enableGuiPanel(bool enable)
{
	if (enable)
	{
		if (!guiPanel)
		{
			StelApp& app = StelApp::getInstance();
			StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
			Q_ASSERT(gui);
			guiPanel = new OcularsGuiPanel(this, gui->getSkyGui());
			
			if (flagShowOculars)
				guiPanel->showOcularGui();
			else if (flagShowCCD)
				guiPanel->showCcdGui();
		}
	}
	else
	{
		if (guiPanel)
		{
			guiPanel->hide();
			delete guiPanel;
			guiPanel = 0;
		}
	}
	guiPanelEnabled = enable;
	settings->setValue("enable_control_panel", enable);	
	settings->sync();
}

void Oculars::retranslateGui()
{
	if (guiPanel)
	{
		// TODO: Fix this hack!
		
		// Delete and re-create the panel to retranslate its trings
		guiPanel->hide();
		delete guiPanel;
		guiPanel = 0;
		
		StelApp& app = StelApp::getInstance();
		StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
		Q_ASSERT(gui);
		guiPanel = new OcularsGuiPanel(this, gui->getSkyGui());
		
		if (flagShowOculars)
			guiPanel->showOcularGui();
		else if (flagShowCCD)
			guiPanel->showCcdGui();
	}
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Slots Methods
#endif
/* ********************************************************************* */
void Oculars::updateLists()
{
	if (oculars.isEmpty())
	{
		selectedOcularIndex = -1;
		actionShowOcular->setChecked(false);
	}
	else
	{
		if (selectedOcularIndex >= oculars.count())
			selectedOcularIndex = oculars.count() - 1;

		if (flagShowOculars)
			emit selectedOcularChanged();
	}

	if (telescopes.isEmpty())
	{
		selectedTelescopeIndex = -1;
		actionShowOcular->setChecked(false);
		actionShowSensor->setChecked(false);
	}
	else
	{
		if (selectedTelescopeIndex >= telescopes.count())
			selectedTelescopeIndex = telescopes.count() - 1;

		if (flagShowOculars || flagShowCCD)
			emit selectedTelescopeChanged();
	}

	if (ccds.isEmpty())
	{
		selectedCCDIndex = -1;
		actionShowSensor->setChecked(false);
	}
	else
	{
		if (selectedCCDIndex >= ccds.count())
			selectedCCDIndex = ccds.count() - 1;

		if (flagShowCCD)
			emit selectedCCDChanged();
	}
}

void Oculars::ccdRotationReset()
{
	ccdRotationAngle = 0.0;
}

void Oculars::enableOcular(bool enableOcularMode)
{
	if (enableOcularMode)
	{
		// Close the sensor view if it's displayed
		if (flagShowCCD)
		{
			if (actionShowSensor->isChecked())
				actionShowSensor->setChecked(false);
			flagShowCCD = false;
			selectedCCDIndex = -1;
		}

		// Close the Telrad sight if it's displayed
		if (flagShowTelrad)
		{
			if (actionShowTelrad->isChecked())
				actionShowTelrad->setChecked(false);
		}

		// Check to ensure that we have enough oculars & telescopes, as they may have been edited in the config dialog
		if (oculars.count() == 0) {
			selectedOcularIndex = -1;
			qWarning() << "No oculars found";
		} else if (oculars.count() > 0 && selectedOcularIndex == -1) {
			selectedOcularIndex = 0;
		}
		if (telescopes.count() == 0) {
			selectedTelescopeIndex = -1;
			qWarning() << "No telescopes found";
		} else if (telescopes.count() > 0 && selectedTelescopeIndex == -1) {
			selectedTelescopeIndex = 0;
		}
	}

	if (!ready  || selectedOcularIndex == -1 ||  (selectedTelescopeIndex == -1 && !isBinocularDefined())) {
		qWarning() << "The Oculars module has been disabled.";
		return;
	}

	StelCore *core = StelApp::getInstance().getCore();
	LabelMgr* labelManager = GETSTELMODULE(LabelMgr);

	// Toggle the ocular view on & off. To toggle on, we want to ensure there is a selected object.
	if (!flagShowOculars && requireSelection && !StelApp::getInstance().getStelObjectMgr().getWasSelected() ) {
		if (usageMessageLabelID == -1) {
			QFontMetrics metrics(font);
			QString labelText = q_("Please select an object before switching to ocular view.");
			StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
			int xPosition = projectorParams.viewportCenter[0];
			xPosition = xPosition - 0.5 * (metrics.width(labelText));
			int yPosition = projectorParams.viewportCenter[1];
			yPosition = yPosition - 0.5 * (metrics.height());
			usageMessageLabelID = labelManager->labelScreen(labelText, xPosition, yPosition,
																											true, font.pixelSize(), "#99FF99");
		}
		// we didn't accept the new status - make sure the toolbar button reflects this
		disconnect(actionShowOcular, SIGNAL(toggled(bool)),
							 this, SLOT(enableOcular(bool)));
		actionShowOcular->setChecked(false);
		connect(actionShowOcular, SIGNAL(toggled(bool)),
						this, SLOT(enableOcular(bool)));
	} else {
		if (selectedOcularIndex != -1) {
			// remove the usage label if it is being displayed.
			hideUsageMessageIfDisplayed();
			flagShowOculars = enableOcularMode;
			zoom(false);
			//BM: I hope this is the right place...
			if (guiPanel)
				guiPanel->showOcularGui();
		}
	}
}

void Oculars::decrementCCDIndex()
{
	selectedCCDIndex--;
	if (selectedCCDIndex == -1) {
		selectedCCDIndex = ccds.count() - 1;
	}
	emit(selectedCCDChanged());
}

void Oculars::decrementOcularIndex()
{
	selectedOcularIndex--;
	if (selectedOcularIndex == -1) {
		selectedOcularIndex = oculars.count() - 1;
	}
	// validate the new selection
	if (selectedOcularIndex > -1 && !oculars[selectedOcularIndex]->isBinoculars()) {
		if ( selectedTelescopeIndex == -1 && telescopes.count() == 0) {
			// reject the change
			selectedOcularIndex++;
		} else if (selectedTelescopeIndex == -1) {
			selectedTelescopeIndex = 0;
			emit(selectedOcularChanged());
		} else {
			emit(selectedOcularChanged());
		}
	} else {
		emit(selectedOcularChanged());
	}
}

void Oculars::decrementTelescopeIndex()
{
	selectedTelescopeIndex--;
	if (selectedTelescopeIndex == -1) {
		selectedTelescopeIndex = telescopes.count() - 1;
	}
	emit(selectedTelescopeChanged());
}

void Oculars::displayPopupMenu()
{
	QMenu* popup = new QMenu(&StelMainWindow::getInstance());

	if (flagShowOculars)
	{
		// We are in Oculars mode
		// We want to show all of the Oculars, and if the current ocular is not a binocular,
		// we will also show the telescopes.
		if (!oculars.isEmpty())
		{
			popup->addAction(q_("&Previous ocular"), this, SLOT(decrementOcularIndex()));
			popup->addAction(q_("&Next ocular"), this, SLOT(incrementOcularIndex()));
			QMenu* submenu = new QMenu(q_("Select &ocular"), popup);
			int availableOcularCount = 0;
			for (int index = 0; index < oculars.count(); ++index)
			{
				QString label;
				if (availableOcularCount < 10)
				{
					label = QString("&%1: %2").arg(availableOcularCount).arg(oculars[index]->name());
				}
				else
				{
					label = oculars[index]->name();
				}
				//BM: Does this happen at all any more?
				QAction* action = 0;
				if (selectedTelescopeIndex == -1) {
					if (oculars[index]->isBinoculars()) {
						action = submenu->addAction(label, ocularsSignalMapper, SLOT(map()));
						availableOcularCount++;
						ocularsSignalMapper->setMapping(action, QString("%1").arg(index));
					}
				} else {
					action = submenu->addAction(label, ocularsSignalMapper, SLOT(map()));
					availableOcularCount++;
					ocularsSignalMapper->setMapping(action, QString("%1").arg(index));
				}
				if (action && index == selectedOcularIndex)
				{
					action->setCheckable(true);
					action->setChecked(true);
				}
			}
			popup->addMenu(submenu);
			popup->addSeparator();
		}

		// If there is more than one telescope, show the prev/next/list complex.
		// If the selected ocular is a binoculars, show nothing.
		if (telescopes.count() > 1 && (selectedOcularIndex > -1 && !oculars[selectedOcularIndex]->isBinoculars()))
		{
			QMenu* submenu = addTelescopeSubmenu(popup);
			popup->addMenu(submenu);
			popup->addSeparator();
		}

		QAction* action = popup->addAction(q_("Toggle &crosshair"));
		action->setCheckable(true);
		action->setChecked(flagShowCrosshairs);
		connect(action, SIGNAL(toggled(bool)),
						actionShowCrosshairs, SLOT(setChecked(bool)));
	} else {
		// We are not in ocular mode
		// We want to show the CCD's, and if a CCD is selected, the telescopes
		//(as a CCD requires a telescope) and the general menu items.
		QAction* action = new QAction(q_("Configure &Oculars"), popup);
		action->setCheckable(true);
		action->setChecked(ocularDialog->visible());
		connect(action, SIGNAL(triggered(bool)),
						ocularDialog, SLOT(setVisible(bool)));
		popup->addAction(action);
		popup->addSeparator();

		if (!flagShowTelrad) {
			QAction* action = popup->addAction(q_("Toggle &CCD"));
			action->setCheckable(true);
			action->setChecked(flagShowCCD);
			connect(action, SIGNAL(toggled(bool)),
							actionShowSensor, SLOT(setChecked(bool)));
		}
		
		if (!flagShowCCD) {
			QAction* action = popup->addAction(q_("Toggle &Telrad"));
			action->setCheckable(true);
			action->setChecked(flagShowTelrad);
			connect(action, SIGNAL(toggled(bool)),
							actionShowTelrad, SLOT(setChecked(bool)));
		}
		
		popup->addSeparator();
		if (flagShowCCD && selectedCCDIndex > -1 && selectedTelescopeIndex > -1)
		{
			popup->addAction(q_("&Previous CCD"), this, SLOT(decrementCCDIndex()));
			popup->addAction(q_("&Next CCD"), this, SLOT(incrementCCDIndex()));
			QMenu* submenu = new QMenu(q_("&Select CCD"), popup);
			for (int index = 0; index < ccds.count(); ++index)
			{
				QString label;
				if (index < 10)
				{
					label = QString("&%1: %2").arg(index).arg(ccds[index]->name());
				}
				else
				{
					label = ccds[index]->name();
				}
				QAction* action = submenu->addAction(label, ccdsSignalMapper, SLOT(map()));
				if (index == selectedCCDIndex)
				{
					action->setCheckable(true);
					action->setChecked(true);
				}
				ccdsSignalMapper->setMapping(action, QString("%1").arg(index));
			}
			popup->addMenu(submenu);
			
			submenu = new QMenu(q_("&Rotate CCD"), popup);
			QAction* rotateAction = NULL;
			rotateAction = submenu->addAction(QString("&1: -90") + QChar(0x00B0),
																				ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-90"));
			rotateAction = submenu->addAction(QString("&2: -45") + QChar(0x00B0),
																				ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-45"));
			rotateAction = submenu->addAction(QString("&3: -15") + QChar(0x00B0),
																				ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-15"));
			rotateAction = submenu->addAction(QString("&4: -5") + QChar(0x00B0),
																				ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-5"));
			rotateAction = submenu->addAction(QString("&5: -1") + QChar(0x00B0),
																				ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-1"));
			rotateAction = submenu->addAction(QString("&6: +1") + QChar(0x00B0),
																				ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("1"));
			rotateAction = submenu->addAction(QString("&7: +5") + QChar(0x00B0),
																				ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("5"));
			rotateAction = submenu->addAction(QString("&8: +15") + QChar(0x00B0),
																				ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("15"));
			rotateAction = submenu->addAction(QString("&9: +45") + QChar(0x00B0),
																				ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("45"));
			rotateAction = submenu->addAction(QString("&0: +90") + QChar(0x00B0),
																				ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("90"));
			rotateAction = submenu->addAction(q_("&Reset rotation"), this, SLOT(ccdRotationReset()));
			popup->addMenu(submenu);
			
			popup->addSeparator();
		}
		if (flagShowCCD && selectedCCDIndex > -1 && telescopes.count() > 1)
		{
			QMenu* submenu = addTelescopeSubmenu(popup);
			popup->addMenu(submenu);
			popup->addSeparator();
		}
	}

	popup->exec(QCursor::pos());
	delete popup;
}

void Oculars::incrementCCDIndex()
{
	selectedCCDIndex++;
	if (selectedCCDIndex == ccds.count()) {
		selectedCCDIndex = 0;
	}
	emit(selectedCCDChanged());
}

void Oculars::incrementOcularIndex()
{
	selectedOcularIndex++;
	if (selectedOcularIndex == oculars.count()) {
		selectedOcularIndex = 0;
	}
	// validate the new selection
	if (selectedOcularIndex > -1 && !oculars[selectedOcularIndex]->isBinoculars()) {
		if ( selectedTelescopeIndex == -1 && telescopes.count() == 0) {
			// reject the change
			selectedOcularIndex++;
		} else if (selectedTelescopeIndex == -1) {
			selectedTelescopeIndex = 0;
			emit(selectedOcularChanged());
		} else {
			emit(selectedOcularChanged());
		}
	} else {
		emit(selectedOcularChanged());
	}
}

void Oculars::incrementTelescopeIndex()
{
	selectedTelescopeIndex++;
	if (selectedTelescopeIndex == telescopes.count()) {
		selectedTelescopeIndex = 0;
	}
	emit(selectedTelescopeChanged());
}

void Oculars::rotateCCD(QString amount)
{
	ccdRotationAngle += amount.toInt();
	if (ccdRotationAngle >= 360)
		ccdRotationAngle -= 360;
	if (ccdRotationAngle <= -360)
		ccdRotationAngle += 360;
}

void Oculars::selectCCDAtIndex(QString indexString)
{
	int index = indexString.toInt();
	if (index > -2 && index < ccds.count()) {
		selectedCCDIndex = index;
		emit(selectedCCDChanged());
	}
}

void Oculars::selectOcularAtIndex(QString indexString)
{
	int index = indexString.toInt();

	// validate the new selection
	if (oculars[index]->isBinoculars()) {
		selectedOcularIndex = index;
		emit(selectedOcularChanged());
	} else {
		if ( selectedTelescopeIndex == -1 && telescopes.count() == 0) {
			// reject the change
		} else {
			if (selectedTelescopeIndex == -1) {
				selectedTelescopeIndex = 0;
			}
			selectedOcularIndex = index;
			emit(selectedOcularChanged());
		}
	}
}

void Oculars::selectTelescopeAtIndex(QString indexString)
{
	int index = indexString.toInt();
	if (index > -2 && index < telescopes.count()) {
		selectedTelescopeIndex = index;
		emit(selectedTelescopeChanged());
	}
}

void Oculars::toggleCCD(bool show)
{
	//If there are no sensors...
	if (ccds.isEmpty() || telescopes.isEmpty())
	{
		//TODO: BM: Make this an on-screen message and/or disable the button
		//if there are no sensors.
		if (show)
			qWarning() << "Oculars plugin: Unable to display a sensor boundary:"
								 << "No sensors or telescopes are defined.";
		flagShowCCD = false;
		selectedCCDIndex = -1;
		show = false;
		if (actionShowSensor->isChecked())
			actionShowSensor->setChecked(false);
	}

	if (show)
	{
		//Mutually exclusive with the ocular mode
		hideUsageMessageIfDisplayed();
		if (flagShowOculars)
		{
			if (actionShowOcular->isChecked())
				actionShowOcular->setChecked(false);
		}

		if (flagShowTelrad)
		{
			if (actionShowTelrad->isChecked())
				actionShowTelrad->setChecked(false);
		}

		if (selectedTelescopeIndex < 0)
			selectedTelescopeIndex = 0;
		if (selectedCCDIndex < 0)
			selectedCCDIndex = 0;
		flagShowCCD = true;
		setScreenFOVForCCD();

		if (guiPanel)
			guiPanel->showCcdGui();
	}
	else
	{
		flagShowCCD = false;

		//Zoom out
		StelCore *core = StelApp::getInstance().getCore();
		StelMovementMgr *movementManager = core->getMovementMgr();
		movementManager->zoomTo(movementManager->getInitFov());
		movementManager->setFlagTracking(false);

		if (guiPanel)
			guiPanel->foldGui();
	}
}

void Oculars::toggleCCD()
{
	if (flagShowCCD)
		toggleCCD(false);
	else
		toggleCCD(true);
}

void Oculars::toggleCrosshairs(bool show)
{
	if (show && flagShowOculars)
	{
		flagShowCrosshairs = true;
	}
	else
	{
		flagShowCrosshairs = false;
	}
}

void Oculars::toggleTelrad(bool show)
{
	if (show)
	{
		hideUsageMessageIfDisplayed();
		if (actionShowOcular->isChecked())
			actionShowOcular->setChecked(false);
		if (actionShowSensor->isChecked())
			actionShowSensor->setChecked(false);
	}
	flagShowTelrad = show;
}

void Oculars::toggleTelrad()
{
	if (flagShowTelrad)
		toggleTelrad(false);
	else
		toggleTelrad(true);
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Private Methods
#endif
/* ********************************************************************* */
void Oculars::initializeActivationActions()
{
	// TRANSLATORS: Title of a group of key bindings in the Help window
	QString group = N_("Plugin Key Bindings");
	//Bogdan: Temporary, for consistency and to avoid confusing the translators
	//TODO: Fix this when the key bindings feature is implemented
	//QString group = N_("Oculars Plugin");
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);

	//This action needs to be connected to the enableOcular() slot after
	//the necessary button is created to prevent the button from being checked
	//the first time this action is checked. See:
	//http://doc.qt.nokia.com/4.7/signalsandslots.html#signals
	QString shortcutStr = settings->value("bindings/toggle_oculars", "Ctrl+O").toString();
	actionShowOcular = gui->addGuiAction("actionShow_Ocular",
																			 N_("Ocular view"),
																			 shortcutStr, "",
																			 group,
																			 true);
	actionShowOcular->setChecked(flagShowOculars);
	// Make a toolbar button
	try {
		pxmapGlow = new QPixmap(":/graphicGui/glow32x32.png");
		pxmapOnIcon = new QPixmap(":/ocular/bt_ocular_on.png");
		pxmapOffIcon = new QPixmap(":/ocular/bt_ocular_off.png");
		toolbarButton = new StelButton(NULL,
																	 *pxmapOnIcon,
																	 *pxmapOffIcon,
																	 *pxmapGlow,
																	 actionShowOcular);
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	} catch (std::runtime_error& e) {
		qWarning() << "WARNING: unable create toolbar button for Oculars plugin: " << e.what();
	}
	connect(actionShowOcular, SIGNAL(toggled(bool)),
					this, SLOT(enableOcular(bool)));

	shortcutStr = settings->value("bindings/popup_navigator", "Alt+O").toString();
	actionMenu = gui->addGuiAction("actionShow_Ocular_Menu",
																 N_("Oculars popup menu"),
																 shortcutStr, "",
																 group,
																 true);
	connect(actionMenu, SIGNAL(toggled(bool)),
					this, SLOT(displayPopupMenu()));

	actionShowCrosshairs = gui->addGuiAction("actionShow_Ocular_Crosshairs",
																					 N_("Crosshairs"),
																					 QString(), "",
																					 group,
																					 true);
	connect(actionShowCrosshairs, SIGNAL(toggled(bool)),
					this, SLOT(toggleCrosshairs(bool)));

	actionShowSensor = gui->addGuiAction("actionShow_Sensor",
																			 N_("Image sensor frame"),
																			 QString(), "",
																			 group,
																			 true);
	connect(actionShowSensor, SIGNAL(toggled(bool)),
					this, SLOT(toggleCCD(bool)));

	actionShowTelrad = gui->addGuiAction("actionShow_Telrad",
																			 N_("Telrad sight"),
																			 QString(), "",
																			 group,
																			 true);
	connect(actionShowTelrad, SIGNAL(toggled(bool)),
					this, SLOT(toggleTelrad(bool)));

	actionConfiguration = gui->addGuiAction("actionOpen_Oculars_Configuration",
																					N_("Oculars plugin configuration"),
																					QString(), "",
																					group,
																					true);
	connect(actionConfiguration, SIGNAL(toggled(bool)),
					ocularDialog, SLOT(setVisible(bool)));
	connect(ocularDialog, SIGNAL(visibleChanged(bool)),
					actionConfiguration, SLOT(setChecked(bool)));

	connect(this, SIGNAL(selectedCCDChanged()), this, SLOT(instrumentChanged()));
	connect(this, SIGNAL(selectedCCDChanged()), this, SLOT(setScreenFOVForCCD()));
	connect(this, SIGNAL(selectedOcularChanged()), this, SLOT(instrumentChanged()));
	connect(this, SIGNAL(selectedTelescopeChanged()), this, SLOT(instrumentChanged()));
	connect(this, SIGNAL(selectedTelescopeChanged()), this, SLOT(setScreenFOVForCCD()));
	connect(ocularDialog, SIGNAL(requireSelectionChanged(bool)), this, SLOT(setRequireSelection(bool)));
	connect(ocularDialog, SIGNAL(scaleImageCircleChanged(bool)), this, SLOT(setScaleImageCircle(bool)));

	connect(ccdRotationSignalMapper, SIGNAL(mapped(QString)), this, SLOT(rotateCCD(QString)));
	connect(ccdsSignalMapper, SIGNAL(mapped(QString)), this, SLOT(selectCCDAtIndex(QString)));
	connect(ccdsSignalMapper, SIGNAL(mapped(QString)), this, SLOT(setScreenFOVForCCD()));
	connect(ocularsSignalMapper, SIGNAL(mapped(QString)), this, SLOT(selectOcularAtIndex(QString)));
	connect(telescopesSignalMapper, SIGNAL(mapped(QString)), this, SLOT(selectTelescopeAtIndex(QString)));
	connect(telescopesSignalMapper, SIGNAL(mapped(QString)), this, SLOT(setScreenFOVForCCD()));
}

bool Oculars::isBinocularDefined()
{
	bool binocularFound = false;
	foreach (Ocular* ocular, oculars) {
		if (ocular->isBinoculars()) {
			binocularFound = true;
			break;
		}
	}
	return binocularFound;
}

void Oculars::paintCCDBounds()
{
	StelCore *core = StelApp::getInstance().getCore();
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();

	glDisable(GL_BLEND);
	glColor3f(0.f,0.f,0.f);
	glPushMatrix();
	glTranslated(params.viewportCenter[0], params.viewportCenter[1], 0.0);
	glRotated(ccdRotationAngle, 0.0, 0.0, 1.0);
	GLdouble screenFOV = params.fov;

	// draw sensor rectangle
	if(selectedCCDIndex != -1) {
		CCD *ccd = ccds[selectedCCDIndex];
		if (ccd) {
			glColor4f(0.77, 0.14, 0.16, 0.5);
			Telescope *telescope = telescopes[selectedTelescopeIndex];
			double ccdXRatio = ccd->getActualFOVx(telescope) / screenFOV;
			double ccdYRatio = ccd->getActualFOVy(telescope) / screenFOV;
			// As the FOV is based on the narrow aspect of the screen, we need to calculate
			// height & width based soley off of that dimension.
			int aspectIndex = 2;
			if (params.viewportXywh[2] > params.viewportXywh[3]) {
				aspectIndex = 3;
			}
			float width = params.viewportXywh[aspectIndex] * ccdYRatio;
			float height = params.viewportXywh[aspectIndex] * ccdXRatio;

			if (width > 0.0 && height > 0.0) {
				glBegin(GL_LINE_LOOP);
				glVertex2f(-width / 2.0, height / 2.0);
				glVertex2f(width / 2.0, height / 2.0);
				glVertex2f(width / 2.0, -height / 2.0);
				glVertex2f(-width / 2.0, -height / 2.0);
				glEnd();
			}
		}
	}

	glPopMatrix();
}

void Oculars::paintCrosshairs()
{
	const StelProjectorP projector = StelApp::getInstance().getCore()->getProjection(StelCore::FrameEquinoxEqu);
	StelCore *core = StelApp::getInstance().getCore();
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
	// Center of screen
	Vec2i centerScreen(projector->getViewportPosX()+projector->getViewportWidth()/2,
										 projector->getViewportPosY()+projector->getViewportHeight()/2);
	GLdouble length = 0.5 * params.viewportFovDiameter;
	// See if we need to scale the length
	if (useMaxEyepieceAngle && oculars[selectedOcularIndex]->appearentFOV() > 0.0) {
		length = oculars[selectedOcularIndex]->appearentFOV() * length / maxEyepieceAngle;
	}

	// Draw the lines
	StelPainter painter(projector);
	painter.setColor(0.77, 0.14, 0.16, 1);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0], centerScreen[1] + length);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0], centerScreen[1] - length);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0] + length, centerScreen[1]);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0] - length, centerScreen[1]);
}

void Oculars::paintTelrad()
{
	if (!flagShowOculars) {
		const StelProjectorP projector = StelApp::getInstance().getCore()->getProjection(StelCore::FrameEquinoxEqu);
		// prevent unused warnings -MNG
		// StelCore *core = StelApp::getInstance().getCore();
		// StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();

		// StelPainter drawing
		StelPainter painter(projector);
		painter.setColor(0.77, 0.14, 0.16, 1.0);
		Vec2i centerScreen(projector->getViewportPosX()+projector->getViewportWidth()/2,
											 projector->getViewportPosY()+projector->getViewportHeight()/2);
		painter.drawCircle(centerScreen[0], centerScreen[1], 0.5 * projector->getPixelPerRadAtCenter() * (M_PI/180) * (0.5));
		painter.drawCircle(centerScreen[0], centerScreen[1], 0.5 * projector->getPixelPerRadAtCenter() * (M_PI/180) * (2.0));
		painter.drawCircle(centerScreen[0], centerScreen[1], 0.5 * projector->getPixelPerRadAtCenter() * (M_PI/180) * (4.0));

	}
}

void Oculars::paintOcularMask()
{
	StelCore *core = StelApp::getInstance().getCore();
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();

	glDisable(GL_BLEND);
	glColor3f(0.f,0.f,0.f);
	glPushMatrix();
	glTranslated(params.viewportCenter[0], params.viewportCenter[1], 0.0);
	GLUquadricObj *quadric = gluNewQuadric();

	GLdouble inner = 0.5 * params.viewportFovDiameter;

	// See if we need to scale the mask
	if (useMaxEyepieceAngle && oculars[selectedOcularIndex]->appearentFOV() > 0.0 && !oculars[selectedOcularIndex]->isBinoculars()) {
		inner = oculars[selectedOcularIndex]->appearentFOV() * inner / maxEyepieceAngle;
	}

	GLdouble outer = params.viewportXywh[2] + params.viewportXywh[3];
	// Draw the mask
	gluDisk(quadric, inner, outer, 256, 1);
	// the gray circle
	glColor3f(0.15f,0.15f,0.15f);
	gluDisk(quadric, inner - 1.0, inner, 256, 1);
	gluDeleteQuadric(quadric);
	glPopMatrix();
}

void Oculars::paintText(const StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	StelPainter painter(prj);

	// Get the current instruments
	CCD *ccd = NULL;
	if(selectedCCDIndex != -1) {
		ccd = ccds[selectedCCDIndex];
	}
	Ocular *ocular = oculars[selectedOcularIndex];
	Telescope *telescope = telescopes[selectedTelescopeIndex];

	// set up the color and the GL state
	painter.setColor(0.8, 0.48, 0.0, 1);
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	// Get the X & Y positions, and the line height
	painter.setFont(font);
	QString widthString = "MMMMMMMMMMMMMMMMMMM";
	float insetFromRHS = painter.getFontMetrics().width(widthString);
	StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
	int xPosition = projectorParams.viewportXywh[2];
	xPosition -= insetFromRHS;
	int yPosition = projectorParams.viewportXywh[3];
	yPosition -= 40;
	const int lineHeight = painter.getFontMetrics().height();
	
	
	// The Ocular
	if (flagShowOculars) {
		QString ocularNumberLabel;
		QString name = ocular->name();
		if (name.isEmpty())
		{
			ocularNumberLabel = QString(q_("Ocular #%1"))
					.arg(selectedOcularIndex);
		}
		else
		{
			ocularNumberLabel = QString(q_("Ocular #%1: %2"))
					.arg(selectedOcularIndex)
					.arg(name);
		}
		// The name of the ocular could be really long.
		if (name.length() > widthString.length()) {
			xPosition -= (insetFromRHS / 2.0);
		}
		painter.drawText(xPosition, yPosition, ocularNumberLabel);
		yPosition-=lineHeight;
		
		if (!ocular->isBinoculars()) {
			QString eFocalLength = QVariant(ocular->effectiveFocalLength()).toString();
			// TRANSLATORS: FL = Focal length
			QString eFocalLengthLabel = QString(q_("Ocular FL: %1 mm")).arg(eFocalLength);
			painter.drawText(xPosition, yPosition, eFocalLengthLabel);
			yPosition-=lineHeight;
			
			QString ocularFov = QString::number(ocular->appearentFOV());
			ocularFov.append(QChar(0x00B0));//Degree sign
			// TRANSLATORS: aFOV = apparent field of view
			QString ocularFOVLabel = QString(q_("Ocular aFOV: %1")).arg(ocularFov);
			painter.drawText(xPosition, yPosition, ocularFOVLabel);
			yPosition-=lineHeight;
			
			// The telescope
			QString telescopeNumberLabel;
			QString telescopeName = telescope->name();
			if (telescopeName.isEmpty())
			{
				telescopeNumberLabel = QString(q_("Telescope #%1"))
						.arg(selectedTelescopeIndex);
			}
			else
			{
				telescopeNumberLabel = QString(q_("Telescope #%1: %2"))
						.arg(selectedTelescopeIndex)
						.arg(telescopeName);
			}
			painter.drawText(xPosition, yPosition, telescopeNumberLabel);
			yPosition-=lineHeight;
			
			// General info
			double magnification = ((int)(ocular->magnification(telescope) * 10.0)) / 10.0;
			QString magString = QString::number(magnification);
			magString.append(QChar(0x00D7));//Multiplication sign
			QString magnificationLabel = QString(q_("Magnification: %1"))
					.arg(magString);
			painter.drawText(xPosition, yPosition, magnificationLabel);
			yPosition-=lineHeight;
			
			double fov = ((int)(ocular->actualFOV(telescope) * 10000.00)) / 10000.0;
			QString fovString = QString::number(fov);
			fovString.append(QChar(0x00B0));//Degree sign
			QString fovLabel = QString(q_("FOV: %1")).arg(fovString);
			painter.drawText(xPosition, yPosition, fovLabel);
		}
	}

	// The CCD
	if (flagShowCCD) {
		QString ccdSensorLabel, ccdInfoLabel;
		double fovX = ((int)(ccd->getActualFOVx(telescope) * 1000.0)) / 1000.0;
		double fovY = ((int)(ccd->getActualFOVy(telescope) * 1000.0)) / 1000.0;		
		ccdInfoLabel = QString(q_("Dimensions: %1")).arg(getDimensionsString(fovX, fovY));
		
		QString name = ccd->name();
		if (name.isEmpty())
		{
			ccdSensorLabel = QString(q_("Sensor #%1")).arg(selectedCCDIndex);
		}
		else
		{
			ccdSensorLabel = QString(q_("Sensor #%1: %2"))
					.arg(selectedCCDIndex)
					.arg(name);
		}
		painter.drawText(xPosition, yPosition, ccdSensorLabel);
		yPosition-=lineHeight;
		painter.drawText(xPosition, yPosition, ccdInfoLabel);
		yPosition-=lineHeight;

		// The telescope
		QString telescopeNumberLabel;
		QString telescopeName = telescope->name();
		if (telescopeName.isEmpty())
		{
			telescopeNumberLabel = QString(q_("Telescope #%1"))
					.arg(selectedTelescopeIndex);
		}
		else
		{
			telescopeNumberLabel = QString(q_("Telescope #%1: %2"))
					.arg(selectedTelescopeIndex)
					.arg(telescopeName);
		}
		painter.drawText(xPosition, yPosition, telescopeNumberLabel);
	}
	
}

void Oculars::validateAndLoadIniFile()
{
	// Insure the module directory exists
	StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Oculars");
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	QString ocularIniPath = StelFileMgr::findFile("modules/Oculars/", flags) + "ocular.ini";

	// If the ini file does not already exist, create it from the resource in the QT resource
	if(!QFileInfo(ocularIniPath).exists()) {
		QFile src(":/ocular/default_ocular.ini");
		if (!src.copy(ocularIniPath)) {
			qWarning() << "Oculars::validateIniFile cannot copy default_ocular.ini resource to [non-existing] "
										+ ocularIniPath;
		} else {
			qDebug() << "Oculars::validateIniFile copied default_ocular.ini to " << ocularIniPath;
			// The resource is read only, and the new file inherits this, so set write-able.
			QFile dest(ocularIniPath);
			dest.setPermissions(dest.permissions() | QFile::WriteOwner);
		}
	} else {
		qDebug() << "Oculars::validateIniFile ocular.ini exists at: " << ocularIniPath << ". Checking version...";
		QSettings settings(ocularIniPath, QSettings::IniFormat);
		double ocularsVersion = settings.value("oculars_version", "0.0").toDouble();
		qWarning() << "Oculars::validateIniFile found existing ini file version " << ocularsVersion;

		if (ocularsVersion < MIN_OCULARS_INI_VERSION) {
			qWarning() << "Oculars::validateIniFile existing ini file version " << ocularsVersion
								 << " too old to use; required version is " << MIN_OCULARS_INI_VERSION << ". Copying over new one.";
			// delete last "old" file, if it exists
			QFile deleteFile(ocularIniPath + ".old");
			deleteFile.remove();

			// Rename the old one, and copy over a new one
			QFile oldFile(ocularIniPath);
			if (!oldFile.rename(ocularIniPath + ".old")) {
				qWarning() << "Oculars::validateIniFile cannot move ocular.ini resource to ocular.ini.old at path  " + ocularIniPath;
			} else {
				qWarning() << "Oculars::validateIniFile ocular.ini resource renamed to ocular.ini.old at path  " + ocularIniPath;
				QFile src(":/ocular/default_ocular.ini");
				if (!src.copy(ocularIniPath)) {
					qWarning() << "Oculars::validateIniFile cannot copy default_ocular.ini resource to [non-existing] " + ocularIniPath;
				} else {
					qDebug() << "Oculars::validateIniFile copied default_ocular.ini to " << ocularIniPath;
					// The resource is read only, and the new file inherits this...  make sure the new file
					// is writable by the Stellarium process so that updates can be done.
					QFile dest(ocularIniPath);
					dest.setPermissions(dest.permissions() | QFile::WriteOwner);
				}
			}
		}
	}
	settings = new QSettings(ocularIniPath, QSettings::IniFormat, this);
}

void Oculars::unzoomOcular()
{
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	GridLinesMgr *gridManager = (GridLinesMgr *)StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr");

	gridManager->setFlagAzimuthalGrid(flagAzimuthalGrid);
	gridManager->setFlagGalacticGrid(flagGalacticGrid);
	gridManager->setFlagEquatorGrid(flagEquatorGrid);
	gridManager->setFlagEquatorJ2000Grid(flagEquatorJ2000Grid);
	gridManager->setFlagEquatorLine(flagEquatorLine);
	gridManager->setFlagEclipticLine(flagEclipticLine);
	gridManager->setFlagEclipticJ2000Grid(flagEclipticJ2000Grid);
	gridManager->setFlagMeridianLine(flagMeridianLine);
	gridManager->setFlagHorizonLine(flagHorizonLine);
	gridManager->setFlagGalacticPlaneLine(flagGalacticPlaneLine);
	movementManager->setFlagTracking(false);
	movementManager->setFlagEnableZoomKeys(true);
	movementManager->setFlagEnableMouseNavigation(true);

	// Set the screen display
	// core->setMaskType(StelProjector::MaskNone);
	core->setFlipHorz(false);
	core->setFlipVert(false);

	movementManager->zoomTo(movementManager->getInitFov());
}

void Oculars::zoom(bool zoomedIn)
{
	if (flagShowOculars && selectedOcularIndex == -1) {
		// The user cycled out the selected ocular
		flagShowOculars = false;
	}

	if (flagShowOculars)  {
		if (!zoomedIn)  {
			GridLinesMgr *gridManager = (GridLinesMgr *)StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr");
			// Current state
			flagAzimuthalGrid = gridManager->getFlagAzimuthalGrid();
			flagGalacticGrid = gridManager->getFlagGalacticGrid();
			flagEquatorGrid = gridManager->getFlagEquatorGrid();
			flagEquatorJ2000Grid = gridManager->getFlagEquatorJ2000Grid();
			flagEquatorLine = gridManager->getFlagEquatorLine();
			flagEclipticLine = gridManager->getFlagEclipticLine();
			flagEclipticJ2000Grid = gridManager->getFlagEclipticJ2000Grid();
			flagMeridianLine = gridManager->getFlagMeridianLine();
			flagHorizonLine = gridManager->getFlagHorizonLine();
			flagGalacticPlaneLine = gridManager->getFlagGalacticPlaneLine();
		}

		// set new state
		zoomOcular();
	} else {
		//reset to original state
		unzoomOcular();
	}
}

void Oculars::zoomOcular()
{
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	GridLinesMgr *gridManager =
			(GridLinesMgr *)StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr");

	gridManager->setFlagAzimuthalGrid(false);
	gridManager->setFlagGalacticGrid(false);
	gridManager->setFlagEquatorGrid(false);
	gridManager->setFlagEquatorJ2000Grid(false);
	gridManager->setFlagEquatorLine(false);
	gridManager->setFlagEclipticLine(false);
	gridManager->setFlagEclipticJ2000Grid(false);
	gridManager->setFlagMeridianLine(false);
	gridManager->setFlagHorizonLine(false);
	gridManager->setFlagGalacticPlaneLine(false);
	
	movementManager->setFlagTracking(true);
	movementManager->setFlagEnableZoomKeys(false);
	movementManager->setFlagEnableMouseNavigation(false);
	
	// We won't always have a selected object
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected()) {
		StelObjectP selectedObject = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0];
		movementManager->moveToJ2000(selectedObject->getEquinoxEquatorialPos(core), 0.0, 1);
	}

	// Set the screen display
	// core->setMaskType(StelProjector::MaskDisk);
	Ocular *ocular = oculars[selectedOcularIndex];
	Telescope *telescope = NULL;
	// Only consider flip is we're not binoculars
	if (ocular->isBinoculars())
	{
		core->setFlipHorz(false);
		core->setFlipVert(false);
	}
	else
	{
		telescope = telescopes[selectedTelescopeIndex];
		core->setFlipHorz(telescope->isHFlipped());
		core->setFlipVert(telescope->isVFlipped());
	}

	double actualFOV = ocular->actualFOV(telescope);
	// See if the mask was scaled; if so, correct the actualFOV.
	if (useMaxEyepieceAngle && ocular->appearentFOV() > 0.0 && !ocular->isBinoculars()) {
		actualFOV = maxEyepieceAngle * actualFOV / ocular->appearentFOV();
	}
	movementManager->zoomTo(actualFOV, 0.0);
}

void Oculars::hideUsageMessageIfDisplayed()
{
	if (usageMessageLabelID > -1)
	{
		LabelMgr *labelManager = GETSTELMODULE(LabelMgr);
		labelManager->setLabelShow(usageMessageLabelID, false);
		labelManager->deleteLabel(usageMessageLabelID);
		usageMessageLabelID = -1;
	}
}

QMenu* Oculars::addTelescopeSubmenu(QMenu *parent)
{
	Q_ASSERT(parent);

	QMenu* submenu = new QMenu(q_("&Telescope"), parent);
	submenu->addAction(q_("&Previous telescope"), this, SLOT(decrementTelescopeIndex()));
	submenu->addAction(q_("&Next telescope"), this, SLOT(incrementTelescopeIndex()));
	submenu->addSeparator();
	for (int index = 0; index < telescopes.count(); ++index)
	{
		QString label;
		if (index < 10)
		{
			label = QString("&%1: %2").arg(index).arg(telescopes[index]->name());
		}
		else
		{
			label = telescopes[index]->name();
		}
		QAction* action = submenu->addAction(label, telescopesSignalMapper, SLOT(map()));
		if (index == selectedTelescopeIndex)
		{
			action->setCheckable(true);
			action->setChecked(true);
		}
		telescopesSignalMapper->setMapping(action, QString("%1").arg(index));
	}

	return submenu;
}

void Oculars::setFlagDecimalDegrees(const bool b)
{
	flagDecimalDegrees = b;
	settings->setValue("use_decimal_degrees", b);	
	settings->sync();
}

bool Oculars::getFlagDecimalDegrees() const
{
	return flagDecimalDegrees;
}

QString Oculars::getDimensionsString(double fovX, double fovY) const
{
	QString stringFovX, stringFovY;
	if (getFlagDecimalDegrees())
	{
		if (fovX >= 1.0)
		{
			int degrees = (int)fovX;
			int minutes = (int)((fovX - degrees) * 60);
			stringFovX = QString::number(degrees) + QChar(0x00B0) + QString::number(minutes) + QChar(0x2032);
		}
		else
		{
			int minutes = (int)(fovX * 60);
			stringFovX = QString::number(minutes) + QChar(0x2032);
		}

		if (fovY >= 1.0)
		{
			int degrees = (int)fovY;
			int minutes = (int)((fovY - degrees) * 60);
			stringFovY = QString::number(degrees) + QChar(0x00B0) + QString::number(minutes) + QChar(0x2032);
		}
		else
		{
			int minutes = (int)(fovY * 60);
			stringFovY = QString::number(minutes) + QChar(0x2032);
		}
	}
	else
	{
		stringFovX = QString::number(fovX) + QChar(0x00B0);
		stringFovY = QString::number(fovY) + QChar(0x00B0);
	}

	return stringFovX + QChar(0x00D7) + stringFovY;
}
