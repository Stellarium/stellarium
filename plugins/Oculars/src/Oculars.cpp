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

#include <QAction>
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
	info.displayedName = q_("Oculars");
	info.authors = "Timothy Reaves";
	info.contact = "treaves@silverfieldstech.com";
	info.description = q_("Shows the sky as if looking through a telescope eyepiece");
	return info;
}

Q_EXPORT_PLUGIN2(Oculars, OcularsStelPluginInterface)


/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Instance Methods
#endif
/* ********************************************************************* */
Oculars::Oculars() : pxmapGlow(NULL), pxmapOnIcon(NULL), pxmapOffIcon(NULL), toolbarButton(NULL)
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

}

Oculars::~Oculars()
{
	delete ocularDialog;
	ocularDialog = NULL;
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
		// Insure there is a selected ocular & telescope
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
			// Paint the information in the upper-right hand corner
			paintText(core);
		}
	} else if (flagShowCCD) {
		paintCCDBounds();
		// Paint the information in the upper-right hand corner
		paintText(core);
	}	
}

//! Determine which "layer" the plagin's drawing will happen on.
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
			if (usageMessageLabelID > -1) {
				LabelMgr *labelManager = GETSTELMODULE(LabelMgr);
				labelManager->setLabelShow(usageMessageLabelID, false);
				labelManager->deleteLabel(usageMessageLabelID);
				usageMessageLabelID = -1;
			}
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

		ocularDialog = new OcularDialog(&ccds, &oculars, &telescopes);
		initializeActivationActions();
		determineMaxEyepieceAngle();
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

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Slots Methods
#endif
/* ********************************************************************* */
void Oculars::ccdRotationReset()
{
	ccdRotationAngle = 0.0;
}

void Oculars::enableOcular(bool enableOcularMode)
{
	// If showing a CCD, cancel it.
	if (flagShowCCD) {
		flagShowCCD = false;
		selectedCCDIndex = -1;
	}
	
	if (enableOcularMode) {
		// Check to insure that we have enough oculars & telescopes, as they may have been edited in the config dialog
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

	// Toggle the plugin on & off.  To toggle on, we want to ensure there is a selected object.
	if (!flagShowOculars && requireSelection && !StelApp::getInstance().getStelObjectMgr().getWasSelected() ) {
		if (usageMessageLabelID == -1) {
			QFontMetrics metrics(font);
			QString labelText = "Please select an object before enabling Ocular.";
			StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
			int xPosition = projectorParams.viewportCenter[0];
			xPosition = xPosition - 0.5 * (metrics.width(labelText));
			int yPosition = projectorParams.viewportCenter[1];
			yPosition = yPosition - 0.5 * (metrics.height());
			usageMessageLabelID = labelManager->labelScreen(labelText, xPosition, yPosition,
																			true, font.pixelSize(), "#99FF99");
		}
		// we didn't accept the new status - make sure the toolbar button reflects this
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		disconnect(gui->getGuiActions("actionShow_Ocular"), SIGNAL(toggled(bool)), this, SLOT(enableOcular(bool)));
		gui->getGuiActions("actionShow_Ocular")->setChecked(false);
		connect(gui->getGuiActions("actionShow_Ocular"), SIGNAL(toggled(bool)), this, SLOT(enableOcular(bool)));
	} else {
		if (selectedOcularIndex != -1) {
			// remove the usage label if it is being displayed.
			if (usageMessageLabelID > -1) {
				labelManager->setLabelShow(usageMessageLabelID, false);
				labelManager->deleteLabel(usageMessageLabelID);
				usageMessageLabelID = -1;
			}
			flagShowOculars = enableOcularMode;
			zoom(false);
		}
	}
}

void Oculars::decrementCCDIndex()
{
	selectedCCDIndex--;
	if (selectedCCDIndex == -2) {
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
	QMenu* popup = new QMenu();

	if (flagShowOculars) {
		// We are in Oculars mode
		// We want to show all of the Oculars, and if the current ocular is not a binocular,
		// we will also show the telescopes.
		if (oculars.count() > 0) {
			popup->addAction("previous ocular", this, SLOT(decrementOcularIndex()), Qt::Key_1);
			popup->addAction("next ocular", this, SLOT(incrementOcularIndex()), Qt::Key_2);
			QMenu* submenu = new QMenu("select ocular", popup);
			int availableOcularCount = 0;
			for (int index = 0; index < oculars.count(); ++index) {
				if (selectedTelescopeIndex == -1) {
					if (oculars[index]->isBinoculars()) {
						QAction* action = submenu->addAction(oculars[index]->name(), ocularsSignalMapper, SLOT(map()), 
																		 QKeySequence(QString("%1").arg(availableOcularCount++)));
						ocularsSignalMapper->setMapping(action, QString("%1").arg(index));
					}
				} else {
					QAction* action = submenu->addAction(oculars[index]->name(), ocularsSignalMapper, SLOT(map()), 
																	 QKeySequence(QString("%1").arg(availableOcularCount++)));
					ocularsSignalMapper->setMapping(action, QString("%1").arg(index));
				}
			}
			popup->addMenu(submenu);
			popup->addSeparator();
		}

		if (telescopes.count() > 0 && (selectedOcularIndex > -1 && !oculars[selectedOcularIndex]->isBinoculars())) {
			popup->addAction("previous telescope", this, SLOT(decrementTelescopeIndex()), Qt::Key_3);
			popup->addAction("next telescope", this, SLOT(incrementTelescopeIndex()), Qt::Key_4);
			QMenu* submenu = new QMenu("select telescope", popup);
			for (int index = 0; index < telescopes.count(); ++index) {
				QAction* action = submenu->addAction(telescopes[index]->name(), telescopesSignalMapper, SLOT(map()), 
																 QKeySequence(QString("%1").arg(index)));
				telescopesSignalMapper->setMapping(action, QString("%1").arg(index));
			}
			popup->addMenu(submenu);
			popup->addSeparator();
		}

		popup->addAction("toggle crosshair", this, SLOT(toggleCrosshair()), Qt::Key_5);
	} else {
		int outerMenuLevel = 1;
		// We are not in Oculars mode
		// We want to show the CCD's, and if a CCD is selected, the Telescopes (as a CCD requires a telescope),
		// and the general menu items.
		QAction* action = new QAction("Configure Oculars", popup);
		action->setCheckable(TRUE);
		action->setShortcut(QString("%1").arg(outerMenuLevel++));
		connect(action, SIGNAL(toggled(bool)), ocularDialog, SLOT(setVisible(bool)));
		connect(ocularDialog, SIGNAL(visibleChanged(bool)), action, SLOT(setChecked(bool)));
		popup->addAction(action);
		popup->addSeparator();

		if (!flagShowTelrad) {
			popup->addAction("Toggle CCD", this, SLOT(toggleCCD()), QKeySequence(QString("%1").arg(outerMenuLevel++)));
		}
		
		if (!flagShowCCD) {
			popup->addAction("Toggle Telrad", this,
								  SLOT(toggleTelrad()), QKeySequence(QString("%1").arg(outerMenuLevel++)));
		}

		popup->addSeparator();
		if (selectedCCDIndex > -1 && selectedTelescopeIndex > -1) {
			popup->addAction("previous CCD", this,
								  SLOT(decrementCCDIndex()), QKeySequence(QString("%1").arg(outerMenuLevel++)));
			popup->addAction("next CCD", this,
								  SLOT(incrementCCDIndex()), QKeySequence(QString("%1").arg(outerMenuLevel++)));
			QMenu* submenu = new QMenu("select CCD", popup);
			for (int index = 0; index < ccds.count(); ++index) {
				QAction* action = submenu->addAction(ccds[index]->name(), ccdsSignalMapper, SLOT(map()), 
																 QKeySequence(QString("%1").arg(index)));
				ccdsSignalMapper->setMapping(action, QString("%1").arg(index));
			}
			popup->addMenu(submenu);
			
			submenu = new QMenu("Rotate CCD", popup);
			QAction* rotateAction = NULL;
			rotateAction = submenu->addAction(QString("-90") + QChar(0x00B0),
														 ccdRotationSignalMapper, SLOT(map()), Qt::Key_1);
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-90"));
			rotateAction = submenu->addAction(QString("-45") + QChar(0x00B0),
														 ccdRotationSignalMapper, SLOT(map()), Qt::Key_2);
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-45"));
			rotateAction = submenu->addAction(QString("-15") + QChar(0x00B0),
														 ccdRotationSignalMapper, SLOT(map()), Qt::Key_3);
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-15"));
			rotateAction = submenu->addAction(QString("-5") + QChar(0x00B0),
														 ccdRotationSignalMapper, SLOT(map()), Qt::Key_4);
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-5"));
			rotateAction = submenu->addAction(QString("-1") + QChar(0x00B0),
														 ccdRotationSignalMapper, SLOT(map()), Qt::Key_5);
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-1"));
			rotateAction = submenu->addAction(QString("+1") + QChar(0x00B0),
														 ccdRotationSignalMapper, SLOT(map()), Qt::Key_6);
			ccdRotationSignalMapper->setMapping(rotateAction, QString("1"));
			rotateAction = submenu->addAction(QString("+5") + QChar(0x00B0),
														 ccdRotationSignalMapper, SLOT(map()), Qt::Key_7);
			ccdRotationSignalMapper->setMapping(rotateAction, QString("5"));
			rotateAction = submenu->addAction(QString("+15") + QChar(0x00B0),
														 ccdRotationSignalMapper, SLOT(map()), Qt::Key_8);
			ccdRotationSignalMapper->setMapping(rotateAction, QString("15"));
			rotateAction = submenu->addAction(QString("+45") + QChar(0x00B0),
														 ccdRotationSignalMapper, SLOT(map()), Qt::Key_9);
			ccdRotationSignalMapper->setMapping(rotateAction, QString("45"));
			rotateAction = submenu->addAction(QString("+90") + QChar(0x00B0),
														 ccdRotationSignalMapper, SLOT(map()), Qt::Key_0);
			ccdRotationSignalMapper->setMapping(rotateAction, QString("90"));
			rotateAction = submenu->addAction("Reset", this, SLOT(ccdRotationReset()), Qt::Key_R);
			popup->addMenu(submenu);
			
			popup->addSeparator();
		}
		if (selectedCCDIndex > -1 && telescopes.count() > 0) {
			popup->addAction("previous telescope", this,
								  SLOT(decrementTelescopeIndex()), QKeySequence(QString("%1").arg(outerMenuLevel++)));
			popup->addAction("next telescope", this,
								  SLOT(incrementTelescopeIndex()), QKeySequence(QString("%1").arg(outerMenuLevel++)));
			QMenu* submenu = new QMenu("select telescope", popup);
			for (int index = 0; index < telescopes.count(); ++index) {
				QAction* action = submenu->addAction(telescopes[index]->name(), telescopesSignalMapper, SLOT(map()), 
																 QKeySequence(QString("%1").arg(index)));
				telescopesSignalMapper->setMapping(action, QString("%1").arg(index));
			}
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
		selectedCCDIndex = -1;
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

void Oculars::toggleCCD()
{
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	if (flagShowCCD) {
		flagShowCCD = false;
		selectedCCDIndex = -1;
		movementManager->zoomTo(movementManager->getInitFov());
		movementManager->setFlagTracking(false);
	} else {
		// Check to insure that we have enough CCDs & telescopes, as they may have been edited in the config dialog
		if (ccds.count() == 0) {
			selectedCCDIndex = -1;
			qDebug() << "No CCDs found";
		} else if (ccds.count() > 0 && selectedCCDIndex == -1) {
			selectedCCDIndex = 0;
		}
		if (telescopes.count() == 0) {
			selectedTelescopeIndex = -1;
			qDebug() << "No telescopes found";
		} else if (telescopes.count() > 0 && selectedTelescopeIndex == -1) {
			selectedTelescopeIndex = 0;
		}		
		if (!ready  || selectedCCDIndex == -1 || selectedTelescopeIndex == -1 ) {
			qDebug() << "The Oculars module has been disabled.";
			return;
		}
		flagShowCCD = true;
		setScreenFOVForCCD();
	}
}

void Oculars::toggleCrosshair()
{
	flagShowCrosshairs = !flagShowCrosshairs;
}

void Oculars::toggleTelrad()
{
	flagShowTelrad = !flagShowTelrad;
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
	gui->addGuiActions("actionShow_Ocular",
							 N_("Ocular view"),
							 settings->value("bindings/toggle_oculars", "Ctrl+O").toString(),
							 N_("Plugin Key Bindings"),
							 true);
	gui->getGuiActions("actionShow_Ocular")->setChecked(flagShowOculars);
	// Make a toolbar button
	try {
		pxmapGlow = new QPixmap(":/graphicGui/glow32x32.png");
		pxmapOnIcon = new QPixmap(":/ocular/bt_ocular_on.png");
		pxmapOffIcon = new QPixmap(":/ocular/bt_ocular_off.png");
		toolbarButton = new StelButton(NULL,
										*pxmapOnIcon,
										*pxmapOffIcon,
										*pxmapGlow,
										gui->getGuiActions("actionShow_Ocular"));
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	} catch (std::runtime_error& e) {
		qWarning() << "WARNING: unable create toolbar button for Oculars plugin: " << e.what();
	}
	connect(gui->getGuiActions("actionShow_Ocular"), SIGNAL(toggled(bool)), this, SLOT(enableOcular(bool)));

	gui->addGuiActions("actionShow_Ocular_Menu",
							 N_("Oculars popup menu"),
							 settings->value("bindings/popup_navigator", "Alt+O").toString(),
							 group,
							 true);
	connect(gui->getGuiActions("actionShow_Ocular_Menu"), SIGNAL(toggled(bool)), this, SLOT(displayPopupMenu()));

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
		StelCore *core = StelApp::getInstance().getCore();
		StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();

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
		QString ocularNumberLabel = "Ocular #" + QVariant(selectedOcularIndex).toString();
		if (ocular->name() != QString(""))  {
			ocularNumberLabel.append(" : ").append(ocular->name());
		}
		// The name of the ocular could be really long.
		if (ocular->name().length() > widthString.length()) {
			xPosition -= (insetFromRHS / 2.0);
		}
		painter.drawText(xPosition, yPosition, ocularNumberLabel);
		yPosition-=lineHeight;
		
		if (!ocular->isBinoculars()) {
			QString ocularFLLabel = "Ocular FL: " + QVariant(ocular->effectiveFocalLength()).toString() + "mm";
			painter.drawText(xPosition, yPosition, ocularFLLabel);
			yPosition-=lineHeight;
			
			QString ocularFOVLabel = "Ocular aFOV: " + QVariant(ocular->appearentFOV()).toString() + QChar(0x00B0);
			painter.drawText(xPosition, yPosition, ocularFOVLabel);
			yPosition-=lineHeight;
			
			// The telescope
			QString telescopeNumberLabel = "Telescope #" + QVariant(selectedTelescopeIndex).toString();
			if (telescope->name() != QString(""))  {
				telescopeNumberLabel.append(" : ").append(telescope->name());
			}
			painter.drawText(xPosition, yPosition, telescopeNumberLabel);
			yPosition-=lineHeight;
			
			// General info
			QString magnificationLabel = "Magnification: " + QVariant(((int)(ocular->magnification(telescope) * 10.0)) / 10.0).toString()+ "x";
			painter.drawText(xPosition, yPosition, magnificationLabel);
			yPosition-=lineHeight;
			
			QString fovLabel = "FOV: " + QVariant(((int)(ocular->actualFOV(telescope) * 10000.00)) / 10000.0).toString() + QChar(0x00B0);
			painter.drawText(xPosition, yPosition, fovLabel);
		}
	}

	// The CCD
	if (flagShowCCD) {
		QString ccdsensorLabel, ccdInfoLabel;
		double fovX = ((int)(ccd->getActualFOVx(telescope) * 1000.0)) / 1000.0;
		double fovY = ((int)(ccd->getActualFOVy(telescope) * 1000.0)) / 1000.0;
		ccdInfoLabel = "Dimension : " + QVariant(fovX).toString() + QChar(0x00B0) + "x" + QVariant(fovY).toString() + QChar(0x00B0);
		if (ccd->name() != QString("")) {
			ccdsensorLabel = "Sensor #" + QVariant(selectedCCDIndex).toString();
			ccdsensorLabel.append(" : ").append(ccd->name());
		}
		painter.drawText(xPosition, yPosition, ccdsensorLabel);
		yPosition-=lineHeight;
		painter.drawText(xPosition, yPosition, ccdInfoLabel);
		yPosition-=lineHeight;

		// The telescope
		QString telescopeNumberLabel = "Telescope #" + QVariant(selectedTelescopeIndex).toString();
		if (telescope->name() != QString(""))  {
			telescopeNumberLabel.append(" : ").append(telescope->name());
		}
		painter.drawText(xPosition, yPosition, telescopeNumberLabel);
		yPosition-=lineHeight;
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
	gridManager->setFlagEquatorGrid(flagEquatorGrid);
	gridManager->setFlagEquatorJ2000Grid(flagEquatorJ2000Grid);
	gridManager->setFlagEquatorLine(flagEquatorLine);
	gridManager->setFlagEclipticLine(flagEclipticLine);
	gridManager->setFlagMeridianLine(flagMeridianLine);
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
			flagEquatorGrid = gridManager->getFlagEquatorGrid();
			flagEquatorJ2000Grid = gridManager->getFlagEquatorJ2000Grid();
			flagEquatorLine = gridManager->getFlagEquatorLine();
			flagEclipticLine = gridManager->getFlagEclipticLine();
			flagMeridianLine = gridManager->getFlagMeridianLine();
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
	gridManager->setFlagEquatorGrid(false);
	gridManager->setFlagEquatorJ2000Grid(false);
	gridManager->setFlagEquatorLine(false);
	gridManager->setFlagEclipticLine(false);
	gridManager->setFlagMeridianLine(false);
	
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
	if (!ocular->isBinoculars()) {
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

