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
#include "SkyGui.hpp"
#include "StelActionMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelPainter.hpp"
#include "StelProjector.hpp"
#include "StelTextureMgr.hpp"
#include "StelTranslator.hpp"
#include "SolarSystem.hpp"
#include "StelUtils.hpp"

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QGraphicsWidget>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QtNetwork>
#include <QPixmap>
#include <QSignalMapper>

#include <cmath>

extern void qt_set_sequence_auto_mnemonic(bool b);

static QSettings *settings; //!< The settings as read in from the ini file.

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark StelModuleMgr Methods
#endif
/* ****************************************************************************************************************** */
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
	info.authors = "Timothy Reaves";
	info.contact = "treaves@silverfieldstech.com";
	info.description = N_("Shows the sky as if looking through a telescope eyepiece. (Only magnification and field of view are simulated.) It can also show a sensor frame and a Telrad sight.");
	info.version = OCULARS_PLUGIN_VERSION;
	return info;
}


/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Instance Methods
#endif
/* ****************************************************************************************************************** */
Oculars::Oculars():
	selectedCCDIndex(-1),
	selectedOcularIndex(-1),
	selectedTelescopeIndex(-1),
	selectedLensIndex(-1),
	flagShowCCD(false),
	flagShowOculars(false),
	flagShowCrosshairs(false),
	flagShowTelrad(false),
	usageMessageLabelID(-1),
	flagAzimuthalGrid(false),
	flagGalacticGrid(false),
	flagEquatorGrid(false),
	flagEquatorJ2000Grid(false),
	flagEquatorLine(false),
	flagEclipticLine(false),
	flagEclipticJ2000Grid(false),
	flagMeridianLine(false),
	flagLongitudeLine(false),
	flagHorizonLine(false),
	flagGalacticEquatorLine(false),
	flagAdaptation(false),
	flagLimitStars(false),
	magLimitStars(0.0),
	flagLimitDSOs(false),
	magLimitDSOs(0.0),
	flagLimitPlanets(false),
	magLimitPlanets(0.0),
	flagMoonScale(false),
	maxEyepieceAngle(0.0),
	requireSelection(true),
	flagLimitMagnitude(false),	
	useMaxEyepieceAngle(true),
	guiPanelEnabled(false),
	flagDecimalDegrees(false),
	flipVert(false),
	flipHorz(false),
	ccdRotationSignalMapper(0),
	ccdsSignalMapper(0),
	ocularsSignalMapper(0),
	telescopesSignalMapper(0),
	lenseSignalMapper(0),
	pxmapGlow(NULL),
	pxmapOnIcon(NULL),
	pxmapOffIcon(NULL),
	toolbarButton(NULL),
	ocularDialog(NULL),
	ready(false),
	actionShowOcular(0),
	actionShowCrosshairs(0),
	actionShowSensor(0),
	actionShowTelrad(0),
	actionConfiguration(0),
	actionMenu(0),
	actionTelescopeIncrement(0),
	actionTelescopeDecrement(0),
	actionOcularIncrement(0),
	actionOcularDecrement(0),
	guiPanel(0),
	actualFOV(0),
	initialFOV(0),
	flagInitFOVUsage(false),
	flagUseFlipForCCD(false),
	reticleRotation(0)
{
	// Font size is 14
	font.setPixelSize(StelApp::getInstance().getBaseFontSize()+1);

	ccds = QList<CCD *>();
	oculars = QList<Ocular *>();
	telescopes = QList<Telescope *>();
	lense = QList<Lens *> ();

	ccdRotationSignalMapper = new QSignalMapper(this);
	ccdsSignalMapper = new QSignalMapper(this);
	ocularsSignalMapper = new QSignalMapper(this);
	telescopesSignalMapper = new QSignalMapper(this);
	lenseSignalMapper = new QSignalMapper(this);
	
	setObjectName("Oculars");

#ifdef Q_OS_MAC
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
	qDeleteAll(lense);
	lense.clear();
}

QSettings* Oculars::appSettings()
{
	return settings;
}


/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark StelModule Methods
#endif
/* ****************************************************************************************************************** */
bool Oculars::configureGui(bool show)
{
	if (show)
	{
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
	settings->remove("lens");
	int index = 0;
	foreach(CCD* ccd, ccds)
	{
		ccd->writeToSettings(settings, index);
		index++;
	}
	index = 0;
	foreach(Ocular * ocular, oculars)
	{
		ocular->writeToSettings(settings, index);
		index++;
	}
	index = 0;
	foreach(Telescope* telescope, telescopes)
	{
		telescope->writeToSettings(settings, index);
		index++;
	}
	index = 0;
	foreach(Lens* lens, lense)
	{
		lens->writeToSettings(settings, index);
		index++;
	}
        
	settings->setValue("ocular_count", oculars.count());
	settings->setValue("telescope_count", telescopes.count());
	settings->setValue("ccd_count", ccds.count());
	settings->setValue("lens_count", lense.count());
	settings->sync();

	disconnect(this, SIGNAL(selectedOcularChanged()), this, SLOT(updateOcularReticle()));
	disconnect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	disconnect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslateGui()));
}

//! Draw any parts on the screen which are for our module
void Oculars::draw(StelCore* core)
{
	if (flagShowTelrad)
	{
		paintTelrad();
	}
	else if (flagShowOculars)
	{
		// Ensure there is a selected ocular & telescope
		if (selectedCCDIndex > ccds.count())
		{
			qWarning() << "Oculars: the selected sensor index of "
								 << selectedCCDIndex << " is greater than the sensor count of "
								 << ccds.count() << ". Module disabled!";
			ready = false;
		}
		if (selectedOcularIndex > oculars.count())
		{
			qWarning() << "Oculars: the selected ocular index of "
								 << selectedOcularIndex << " is greater than the ocular count of "
								 << oculars.count() << ". Module disabled!";
			ready = false;
		}
		else if (selectedTelescopeIndex > telescopes.count())
		{
			qWarning() << "Oculars: the selected telescope index of "
								 << selectedTelescopeIndex << " is greater than the telescope count of "
								 << telescopes.count() << ". Module disabled!";
			ready = false;
		}
		
		if (ready)
		{
			if (selectedOcularIndex > -1)
			{
				paintOcularMask(core);
				if (flagShowCrosshairs)
				{
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
	}
	else if (flagShowCCD)
	{
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
	if (StelApp::getInstance().getVisionModeNight())
	{
		pluginStyle.qtStyleSheet.append(nightStyleSheet);
	}
	else
	{
		pluginStyle.qtStyleSheet.append(normalStyleSheet);
	}
	return pluginStyle;
}

void Oculars::handleMouseClicks(class QMouseEvent* event)
{
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected())
	{
		if (flagShowOculars)
		{
			// center the selected object in the ocular, and track.
			movementManager->setFlagTracking(true);
		}
		else
		{
			// remove the usage label if it is being displayed.
			hideUsageMessageIfDisplayed();
		}
	}
	else if(flagShowOculars)
	{
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
	if (!flagShowOculars && !flagShowCCD)
	{
		return;
	}
	// We onle care about the arrow keys.  This flag tracks that.
	bool consumeEvent = false;
	
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	
	if (event->type() == QEvent::KeyPress)
	{
		// Direction and zoom replacements
		switch (event->key())
		{
			case Qt::Key_Left:
				movementManager->turnLeft(true);
				consumeEvent = true;
				break;
			case Qt::Key_Right:
				movementManager->turnRight(true);
				consumeEvent = true;
				break;
			case Qt::Key_Up:
				if (!event->modifiers().testFlag(Qt::ControlModifier))
				{
					movementManager->turnUp(true);
				}
				consumeEvent = true;
				break;
			case Qt::Key_Down:
				if (!event->modifiers().testFlag(Qt::ControlModifier))
				{
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
			case Qt::Key_M:
				double multiplier = 1.0;
				if (event->modifiers().testFlag(Qt::ControlModifier))
				{
					multiplier = 0.1;
				}
				if (event->modifiers().testFlag(Qt::AltModifier))
				{
					multiplier = 5.0;
				}
				if (event->modifiers().testFlag(Qt::ShiftModifier))
				{
					reticleRotation += (1.0 * multiplier);
				}
				else
				{
					reticleRotation -= (1.0 * multiplier);
				}
				qDebug() << reticleRotation;
				consumeEvent = true;
				break;
		}
	}
	else
	{
		// When a deplacement key is released stop mooving
		switch (event->key())
		{
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
		if (consumeEvent)
		{
			// We don't want to re-center the object; just hold the current position.
			movementManager->setFlagLockEquPos(true);
		}
	}
	if (consumeEvent)
	{
		event->accept();
	}
	else
	{
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
		for (int index = 0; index < ocularCount; index++)
		{
			Ocular *newOcular = Ocular::ocularFromSettings(settings, index);
			if (newOcular != NULL)
			{
				oculars.append(newOcular);
			}
			else
			{
				actualOcularCount--;
			}
		}
		if (actualOcularCount < 1)
		{
			if (actualOcularCount < ocularCount)
			{
				qWarning() << "The Oculars ini file appears to be corrupt; delete it.";
			}
			else
			{
				qWarning() << "There are no oculars defined for the Oculars plugin; plugin will be disabled.";
			}
			ready = false;
		}
		else
		{
			selectedOcularIndex = 0;
		}

		int ccdCount = settings->value("ccd_count", 0).toInt();
		int actualCcdCount = ccdCount;
		for (int index = 0; index < ccdCount; index++)
		{
			CCD *newCCD = CCD::ccdFromSettings(settings, index);
			if (newCCD != NULL)
			{
				ccds.append(newCCD);
			}
			else
			{
				actualCcdCount--;
			}
		}
		if (actualCcdCount < ccdCount)
		{
			qWarning() << "The Oculars ini file appears to be corrupt; delete it.";
			ready = false;
		}

		int telescopeCount = settings->value("telescope_count", 0).toInt();
		int actualTelescopeCount = telescopeCount;
		for (int index = 0; index < telescopeCount; index++)
		{
			Telescope *newTelescope = Telescope::telescopeFromSettings(settings, index);
			if (newTelescope != NULL)
			{
				telescopes.append(newTelescope);
			}
			else
			{
				actualTelescopeCount--;
			}
		}
		if (actualTelescopeCount < 1)
		{
			if (actualTelescopeCount < telescopeCount)
			{
				qWarning() << "The Oculars ini file appears to be corrupt; delete it.";
			}
			else
			{
				qWarning() << "There are no telescopes defined for the Oculars plugin; plugin will be disabled.";
			}
			ready = false;
		}
		else
		{
			selectedTelescopeIndex = 0;
		}

		int lensCount = settings->value("lens_count", 0).toInt();
		int actualLensCount = lensCount;
		for (int index = 0; index<lensCount; index++)
		{
			Lens *newLens = Lens::lensFromSettings(settings, index);
			if (newLens != NULL)
			{
				lense.append(newLens);
			}
			else
			{
				actualLensCount--;
			}
		}
		if (lensCount > 0 && actualLensCount < lensCount)
		{
			qWarning() << "The Oculars ini file appears to be corrupt; delete it.";
		}

		ocularDialog = new OcularDialog(this, &ccds, &oculars, &telescopes, &lense);
		initializeActivationActions();
		determineMaxEyepieceAngle();
		
		guiPanelEnabled = settings->value("enable_control_panel", true).toBool();
		enableGuiPanel(guiPanelEnabled);

		setFlagDecimalDegrees(settings->value("use_decimal_degrees", false).toBool());
		setFlagLimitMagnitude(settings->value("limit_stellar_magnitude", true).toBool());
		setFlagInitFovUsage(settings->value("use_initial_fov", false).toBool());
		setFlagUseFlipForCCD(settings->value("use_ccd_flip", false).toBool());
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate ocular.ini file or create a default one for Ocular plugin: " << e.what();
		ready = false;
	}

	//Load the module's custom style sheets
	QFile styleSheetFile;
	styleSheetFile.setFileName(":/ocular/normalStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text))
	{
		normalStyleSheet = styleSheetFile.readAll();
	}
	styleSheetFile.close();
	styleSheetFile.setFileName(":/ocular/nightStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text))
	{
		nightStyleSheet = styleSheetFile.readAll();
	}
	styleSheetFile.close();
	connect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslateGui()));
	connect(this, SIGNAL(selectedOcularChanged()), this, SLOT(updateOcularReticle()));
}

void Oculars::setStelStyle(const QString&)
{
	if(ocularDialog)
	{
		ocularDialog->updateStyle();
	}
}

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Private slots Methods
#endif
/* ****************************************************************************************************************** */
void Oculars::determineMaxEyepieceAngle()
{
	if (ready)
	{
		foreach (Ocular* ocular, oculars)
		{
			if (ocular->appearentFOV() > maxEyepieceAngle)
			{
				maxEyepieceAngle = ocular->appearentFOV();
			}
		}
	}
	// insure it is not zero
	if (maxEyepieceAngle == 0.0)
	{
		maxEyepieceAngle = 1.0;
	}
}

void Oculars::instrumentChanged()
{
	// We only zoom if in ocular mode.
	if (flagShowOculars)
	{
		zoom(true);
	}
}

void Oculars::setRequireSelection(bool state)
{
	requireSelection = state;
}

void Oculars::setScaleImageCircle(bool state)
{
	if (state)
	{
		determineMaxEyepieceAngle();
	}
	useMaxEyepieceAngle = state;
}

void Oculars::setScreenFOVForCCD()
{
	Lens * lens = selectedLensIndex >=0  ? lense[selectedLensIndex] : NULL;
	if (selectedCCDIndex > -1 && selectedTelescopeIndex > -1)
	{
		StelCore *core = StelApp::getInstance().getCore();
		StelMovementMgr *movementManager = core->getMovementMgr();
		double actualFOVx = ccds[selectedCCDIndex]->getActualFOVx(telescopes[selectedTelescopeIndex], lens);
		double actualFOVy = ccds[selectedCCDIndex]->getActualFOVy(telescopes[selectedTelescopeIndex], lens);
		if (actualFOVx < actualFOVy)
		{
			actualFOVx = actualFOVy;
		}
		movementManager->setFlagTracking(true);
		movementManager->zoomTo(actualFOVx * 1.75, 0.0);
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

void Oculars::updateOcularReticle(void)
{
	reticleRotation = 0.0;
	StelTextureMgr& manager = StelApp::getInstance().getTextureManager();
	//Load OpenGL textures
	StelTexture::StelTextureParams params;
	params.generateMipmaps = true;
	reticleTexture = manager.createTexture(oculars[selectedOcularIndex]->reticlePath(), params);
}

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Slots Methods
#endif
/* ****************************************************************************************************************** */
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
	CCD *ccd = ccds[selectedCCDIndex];
	if (ccd) ccd->setChipRotAngle(0.0);
	emit(selectedCCDChanged());
}

double Oculars::ccdRotationAngle() const
{
	CCD *ccd = ccds[selectedCCDIndex];
	if (ccd) return ccd->chipRotAngle();
	else return 0.0;
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
		if (oculars.count() == 0)
		{
			selectedOcularIndex = -1;
			qWarning() << "No oculars found";
		}
		else if (oculars.count() > 0 && selectedOcularIndex == -1)
		{
			selectedOcularIndex = 0;
		}
		if (telescopes.count() == 0)
		{
			selectedTelescopeIndex = -1;
			qWarning() << "No telescopes found";
		}
		else if (telescopes.count() > 0 && selectedTelescopeIndex == -1)
		{
			selectedTelescopeIndex = 0;
		}
	}

	if (!ready  || selectedOcularIndex == -1 ||  (selectedTelescopeIndex == -1 && !isBinocularDefined()))
	{
		qWarning() << "The Oculars module has been disabled.";
		return;
	}

	StelCore *core = StelApp::getInstance().getCore();
	LabelMgr* labelManager = GETSTELMODULE(LabelMgr);

	// Toggle the ocular view on & off. To toggle on, we want to ensure there is a selected object.
	if (!flagShowOculars && requireSelection && !StelApp::getInstance().getStelObjectMgr().getWasSelected() )
	{
		if (usageMessageLabelID == -1)
		{
			QFontMetrics metrics(font);
			QString labelText = q_("Please select an object before switching to ocular view.");
			StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
			int xPosition = projectorParams.viewportCenter[0];
			xPosition = xPosition - 0.5 * (metrics.width(labelText));
			int yPosition = projectorParams.viewportCenter[1];
			yPosition = yPosition - 0.5 * (metrics.height());
			const char *tcolor = "#99FF99";
			usageMessageLabelID = labelManager->labelScreen(labelText, xPosition, yPosition,
									true, font.pixelSize(), tcolor);
		}
		// we didn't accept the new status - make sure the toolbar button reflects this
		disconnect(actionShowOcular, SIGNAL(toggled(bool)), this, SLOT(enableOcular(bool)));
		actionShowOcular->setChecked(false);
		connect(actionShowOcular, SIGNAL(toggled(bool)), this, SLOT(enableOcular(bool)));
	}
	else
	{
		if (selectedOcularIndex != -1)
		{
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
	if (selectedCCDIndex == -1)
	{
		selectedCCDIndex = ccds.count() - 1;
	}
	emit(selectedCCDChanged());
}

void Oculars::decrementOcularIndex()
{
	selectedOcularIndex--;
	if (selectedOcularIndex == -1)
	{
		selectedOcularIndex = oculars.count() - 1;
	}
	// validate the new selection
	if (selectedOcularIndex > -1 && !oculars[selectedOcularIndex]->isBinoculars())
	{
		if ( selectedTelescopeIndex == -1 && telescopes.count() == 0)
		{
			// reject the change
			selectedOcularIndex++;
		}
		else if (selectedTelescopeIndex == -1)
		{
			selectedTelescopeIndex = 0;
			emit(selectedOcularChanged());
		}
		else
		{
			emit(selectedOcularChanged());
		}
	}
	else
	{
		emit(selectedOcularChanged());
	}
}

void Oculars::decrementTelescopeIndex()
{
	selectedTelescopeIndex--;
	if (selectedTelescopeIndex == -1)
	{
		selectedTelescopeIndex = telescopes.count() - 1;
	}
	emit(selectedTelescopeChanged());
}

void Oculars::decrementLensIndex()
{
	selectedLensIndex--;
	if (selectedLensIndex == lense.count())
	{
		selectedLensIndex = -1;
	}
	if (selectedLensIndex == -2)
	{
		selectedLensIndex = lense.count() - 1;
	}
	emit(selectedLensChanged());
}

void Oculars::displayPopupMenu()
{
	QMenu * popup = new QMenu(&StelMainView::getInstance());
	StelGui * gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	qDebug() <<this->getModuleStyleSheet(gui->getStelStyle()).qtStyleSheet;
	popup->setStyleSheet(this->getModuleStyleSheet(gui->getStelStyle()).qtStyleSheet);

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
				if (selectedTelescopeIndex == -1)
				{
					if (oculars[index]->isBinoculars())
					{
						action = submenu->addAction(label, ocularsSignalMapper, SLOT(map()));
						availableOcularCount++;
						ocularsSignalMapper->setMapping(action, QString("%1").arg(index));
					}
				}
				else
				{
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
			submenu = addLensSubmenu(popup);
			popup->addMenu(submenu);
			popup->addSeparator();
		}

		QAction* action = popup->addAction(q_("Toggle &crosshair"));
		action->setCheckable(true);
		action->setChecked(flagShowCrosshairs);
		connect(action, SIGNAL(toggled(bool)), actionShowCrosshairs, SLOT(setChecked(bool)));
	}
	else
	{
		// We are not in ocular mode
		// We want to show the CCD's, and if a CCD is selected, the telescopes
		//(as a CCD requires a telescope) and the general menu items.
		QAction* action = new QAction(q_("Configure &Oculars"), popup);
		action->setCheckable(true);
		action->setChecked(ocularDialog->visible());
		connect(action, SIGNAL(triggered(bool)), ocularDialog, SLOT(setVisible(bool)));
		popup->addAction(action);
		popup->addSeparator();

		if (!flagShowTelrad)
		{
			QAction* action = popup->addAction(q_("Toggle &CCD"));
			action->setCheckable(true);
			action->setChecked(flagShowCCD);
			connect(action, SIGNAL(toggled(bool)), actionShowSensor, SLOT(setChecked(bool)));
		}
		
		if (!flagShowCCD)
		{
			QAction* action = popup->addAction(q_("Toggle &Telrad"));
			action->setCheckable(true);
			action->setChecked(flagShowTelrad);
			connect(action, SIGNAL(toggled(bool)), actionShowTelrad, SLOT(setChecked(bool)));
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
			rotateAction = submenu->addAction(QString("&1: -90") + QChar(0x00B0), ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-90"));
			rotateAction = submenu->addAction(QString("&2: -45") + QChar(0x00B0), ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-45"));
			rotateAction = submenu->addAction(QString("&3: -15") + QChar(0x00B0), ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-15"));
			rotateAction = submenu->addAction(QString("&4: -5") + QChar(0x00B0), ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-5"));
			rotateAction = submenu->addAction(QString("&5: -1") + QChar(0x00B0), ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("-1"));
			rotateAction = submenu->addAction(QString("&6: +1") + QChar(0x00B0), ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("1"));
			rotateAction = submenu->addAction(QString("&7: +5") + QChar(0x00B0), ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("5"));
			rotateAction = submenu->addAction(QString("&8: +15") + QChar(0x00B0), ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("15"));
			rotateAction = submenu->addAction(QString("&9: +45") + QChar(0x00B0), ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("45"));
			rotateAction = submenu->addAction(QString("&0: +90") + QChar(0x00B0), ccdRotationSignalMapper, SLOT(map()));
			ccdRotationSignalMapper->setMapping(rotateAction, QString("90"));
			rotateAction = submenu->addAction(q_("&Reset rotation"), this, SLOT(ccdRotationReset()));
			popup->addMenu(submenu);
			
			popup->addSeparator();
		}
		if (flagShowCCD && selectedCCDIndex > -1 && telescopes.count() > 1)
		{
			QMenu* submenu = addTelescopeSubmenu(popup);
			popup->addMenu(submenu);
			submenu = addLensSubmenu(popup);
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
	if (selectedCCDIndex == ccds.count())
	{
		selectedCCDIndex = 0;
	}
	emit(selectedCCDChanged());
}

void Oculars::incrementOcularIndex()
{
	selectedOcularIndex++;
	if (selectedOcularIndex == oculars.count())
	{
		selectedOcularIndex = 0;
	}
	// validate the new selection
	if (selectedOcularIndex > -1 && !oculars[selectedOcularIndex]->isBinoculars())
	{
		if ( selectedTelescopeIndex == -1 && telescopes.count() == 0)
		{
			// reject the change
			selectedOcularIndex++;
		}
		else if (selectedTelescopeIndex == -1)
		{
			selectedTelescopeIndex = 0;
			emit(selectedOcularChanged());
		}
		else
		{
			emit(selectedOcularChanged());
		}
	}
	else
	{
		emit(selectedOcularChanged());
	}
}

void Oculars::incrementTelescopeIndex()
{
	selectedTelescopeIndex++;
	if (selectedTelescopeIndex == telescopes.count())
	{
		selectedTelescopeIndex = 0;
	}
	emit(selectedTelescopeChanged());
}

void Oculars::incrementLensIndex()
{
	selectedLensIndex++;
	if (selectedLensIndex == lense.count())
	{
		selectedLensIndex = -1;
	}
	emit(selectedLensChanged());
}

void Oculars::disableLens()
{
	selectedLensIndex = -1;
	emit(selectedLensChanged());
}

void Oculars::rotateCCD(QString amount)
{
	CCD *ccd = ccds[selectedCCDIndex];
	if (!ccd) return;
	double angle = ccd->chipRotAngle();
	angle += amount.toInt();
	if (angle >= 360)
	{
		angle -= 360;
	}
	else if (angle <= -360)
	{
		angle += 360;
	}
	ccd->setChipRotAngle(angle);
	emit(selectedCCDChanged());
}

void Oculars::selectCCDAtIndex(QString indexString)
{
	int index = indexString.toInt();
	if (index > -2 && index < ccds.count())
	{
		selectedCCDIndex = index;
		emit(selectedCCDChanged());
	}
}

void Oculars::selectOcularAtIndex(QString indexString)
{
	int index = indexString.toInt();

	// validate the new selection
	if (oculars[index]->isBinoculars())
	{
		selectedOcularIndex = index;
		emit(selectedOcularChanged());
	}
	else
	{
		if ( selectedTelescopeIndex == -1 && telescopes.count() == 0)
		{
			// reject the change
		}
		else
		{
			if (selectedTelescopeIndex == -1)
			{
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
	if (index > -2 && index < telescopes.count())
	{
		selectedTelescopeIndex = index;
		emit(selectedTelescopeChanged());
	}
}

void Oculars::selectLensAtIndex(QString indexString)
{
	int index = indexString.toInt();
	if (index > -2 && index < lense.count())
	{
		selectedLensIndex = index;
		emit(selectedLensChanged());
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
			qWarning() << "Oculars plugin: Unable to display a sensor boundary: No sensors or telescopes are defined.";
		flagShowCCD = false;
		selectedCCDIndex = -1;
		show = false;
		if (actionShowSensor->isChecked())
		{
			actionShowSensor->setChecked(false);
		}
	}

	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();	
	if (show)
	{
		initialFOV = movementManager->getCurrentFov();
		//Mutually exclusive with the ocular mode
		hideUsageMessageIfDisplayed();
		if (flagShowOculars)
		{
			if (actionShowOcular->isChecked())
			{
				actionShowOcular->setChecked(false);
			}
		}

		if (flagShowTelrad) {
			if (actionShowTelrad->isChecked()) {
				actionShowTelrad->setChecked(false);
			}
		}

		if (selectedTelescopeIndex < 0)
		{
			selectedTelescopeIndex = 0;
		}
		if (selectedCCDIndex < 0)
		{
			selectedCCDIndex = 0;
		}
		flagShowCCD = true;
		setScreenFOVForCCD();		

		if (guiPanel)
		{
			guiPanel->showCcdGui();
		}
	}
	else
	{
		flagShowCCD = false;

		movementManager->setFlagTracking(false);
		//Zoom out		
		if (getFlagInitFovUsage())
			movementManager->zoomTo(movementManager->getInitFov());
		else
			movementManager->zoomTo(initialFOV);

		if (getFlagUseFlipForCCD())
		{
			core->setFlipHorz(false);
			core->setFlipVert(false);
		}

		if (guiPanel)
		{
			guiPanel->foldGui();
		}
	}
}

void Oculars::toggleCCD()
{
	if (flagShowCCD)
	{
		toggleCCD(false);
	}
	else
	{
		toggleCCD(true);
	}
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

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Private Methods
#endif
/* ****************************************************************************************************************** */
void Oculars::initializeActivationActions()
{
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);

	QString ocularsGroup = N_("Oculars");
	actionShowOcular = addAction("actionShow_Ocular", ocularsGroup, N_("Ocular view"), "enableOcular(bool)", "Ctrl+O");
	actionShowOcular->setChecked(flagShowOculars);
	// Make a toolbar button
	try
	{
		pxmapGlow = new QPixmap(":/graphicGui/glow32x32.png");
		pxmapOnIcon = new QPixmap(":/ocular/bt_ocular_on.png");
		pxmapOffIcon = new QPixmap(":/ocular/bt_ocular_off.png");
		toolbarButton = new StelButton(NULL, *pxmapOnIcon, *pxmapOffIcon, *pxmapGlow, "actionShow_Ocular");
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable create toolbar button for Oculars plugin: " << e.what();
	}

	actionMenu = addAction("actionShow_Ocular_Menu", ocularsGroup, N_("Oculars popup menu"), "displayPopupMenu()", "Alt+O");
	actionShowCrosshairs = addAction("actionShow_Ocular_Crosshairs", ocularsGroup, N_("Show crosshairs"), "toggleCrosshairs(bool)", "Alt+C");
	actionShowSensor = addAction("actionShow_Sensor", ocularsGroup, N_("Image sensor frame"), "toggleCCD(bool)");
	actionShowTelrad = addAction("actionShow_Telrad", ocularsGroup, N_("Telrad sight"), "toggleTelrad(bool)", "Ctrl+B");
	actionConfiguration = addAction("actionOpen_Oculars_Configuration", ocularsGroup, N_("Oculars plugin configuration"), ocularDialog, "visible");
	// Select next telescope via keyboard
	addAction("actionShow_Telescope_Increment", ocularsGroup, N_("Select next telescope"), "incrementTelescopeIndex()", "");
	// Select previous telescope via keyboard
	addAction("actionShow_Telescope_Decrement", ocularsGroup, N_("Select previous telescope"), "decrementTelescopeIndex()", "");
	// Select next eyepiece via keyboard
	addAction("actionShow_Ocular_Increment", ocularsGroup, N_("Select next eyepiece"), "incrementOcularIndex()", "");
	// Select previous eyepiece via keyboard
	addAction("actionShow_Ocular_Decrement", ocularsGroup, N_("Select previous eyepiece"), "decrementOcularIndex()", "");
	connect(this, SIGNAL(selectedCCDChanged()), this, SLOT(instrumentChanged()));
	connect(this, SIGNAL(selectedCCDChanged()), this, SLOT(setScreenFOVForCCD()));
	connect(this, SIGNAL(selectedOcularChanged()), this, SLOT(instrumentChanged()));
	connect(this, SIGNAL(selectedTelescopeChanged()), this, SLOT(instrumentChanged()));
	connect(this, SIGNAL(selectedTelescopeChanged()), this, SLOT(setScreenFOVForCCD()));
	connect(this, SIGNAL(selectedLensChanged()), this, SLOT(instrumentChanged()));
	connect(ocularDialog, SIGNAL(requireSelectionChanged(bool)), this, SLOT(setRequireSelection(bool)));
	connect(ocularDialog, SIGNAL(scaleImageCircleChanged(bool)), this, SLOT(setScaleImageCircle(bool)));

	connect(ccdRotationSignalMapper, SIGNAL(mapped(QString)), this, SLOT(rotateCCD(QString)));
	connect(ccdsSignalMapper, SIGNAL(mapped(QString)), this, SLOT(selectCCDAtIndex(QString)));
	connect(ccdsSignalMapper, SIGNAL(mapped(QString)), this, SLOT(setScreenFOVForCCD()));
	connect(ocularsSignalMapper, SIGNAL(mapped(QString)), this, SLOT(selectOcularAtIndex(QString)));
	connect(telescopesSignalMapper, SIGNAL(mapped(QString)), this, SLOT(selectTelescopeAtIndex(QString)));
	connect(telescopesSignalMapper, SIGNAL(mapped(QString)), this, SLOT(setScreenFOVForCCD()));
	connect(lenseSignalMapper, SIGNAL(mapped(QString)), this, SLOT(selectLensAtIndex(QString)));
}

bool Oculars::isBinocularDefined()
{
	bool binocularFound = false;
	foreach (Ocular* ocular, oculars)
	{
		if (ocular->isBinoculars())
		{
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
	Lens *lens = selectedLensIndex >=0  ? lense[selectedLensIndex] : NULL;

	const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);	
	double screenFOV = params.fov;

	// draw sensor rectangle
	if(selectedCCDIndex != -1)
	{
		CCD *ccd = ccds[selectedCCDIndex];
		if (ccd)
		{
			StelPainter painter(projector);
			painter.setColor(0.77f, 0.14f, 0.16f, 1.0f);
			Telescope *telescope = telescopes[selectedTelescopeIndex];

			const double ccdXRatio = ccd->getActualFOVx(telescope, lens) / screenFOV;
			const double ccdYRatio = ccd->getActualFOVy(telescope, lens) / screenFOV;

			// flip are needed and allowed?
			float ratioLimit = 0.125f;
			if (getFlagUseFlipForCCD() && (ccdXRatio>=ratioLimit || ccdYRatio>=ratioLimit))
			{
				core->setFlipHorz(telescope->isHFlipped());
				core->setFlipVert(telescope->isVFlipped());
			}
			else
			{
				core->setFlipHorz(false);
				core->setFlipVert(false);
			}

			// As the FOV is based on the narrow aspect of the screen, we need to calculate
			// height & width based soley off of that dimension.
			int aspectIndex = 2;
			if (params.viewportXywh[2] > params.viewportXywh[3])
			{
				aspectIndex = 3;
			}
			float width = params.viewportXywh[aspectIndex] * ccdYRatio * params.devicePixelsPerPixel;
			float height = params.viewportXywh[aspectIndex] * ccdXRatio * params.devicePixelsPerPixel;

			double polarAngle = 0;
			// if the telescope is Equatorial derotate the field
			if (telescope->isEquatorial())
			{
				Vec3d CPos;
				Vec2f cpos = projector->getViewportCenter();
				projector->unProject(cpos[0], cpos[1], CPos);
				Vec3d CPrel(CPos);
				CPrel[2]*=0.2;
				Vec3d crel;
				projector->project(CPrel, crel);
				polarAngle = atan2(cpos[1] - crel[1], cpos[0] - crel[0]) * (-180.0)/M_PI; // convert to degrees
				if (CPos[2] > 0) polarAngle += 90.0;
				else polarAngle -= 90.0;
			}

			if (width > 0.0 && height > 0.0)
			{
				QPoint a, b;
				QTransform transform = QTransform().translate(params.viewportCenter[0] * params.devicePixelsPerPixel,
						params.viewportCenter[1] * params.devicePixelsPerPixel).rotate(-(ccd->chipRotAngle() + polarAngle));
				// bottom line
				a = transform.map(QPoint(-width/2.0, -height/2.0));
				b = transform.map(QPoint(width/2.0, -height/2.0));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
				// top line
				a = transform.map(QPoint(-width/2.0, height/2.0));
				b = transform.map(QPoint(width/2.0, height/2.0));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
				// left line
				a = transform.map(QPoint(-width/2.0, -height/2.0));
				b = transform.map(QPoint(-width/2.0, height/2.0));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
				// right line
				a = transform.map(QPoint(width/2.0, height/2.0));
				b = transform.map(QPoint(width/2.0, -height/2.0));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());

				if(ccd->hasOAG())
				{
					const double InnerOAGRatio = ccd->getInnerOAGRadius(telescope, lens) / screenFOV;
					const double OuterOAGRatio = ccd->getOuterOAGRadius(telescope, lens) / screenFOV;
					const double prismXRatio = ccd->getOAGActualFOVx(telescope, lens) / screenFOV;
					float in_oag_r = params.viewportXywh[aspectIndex] * InnerOAGRatio * params.devicePixelsPerPixel;
					float out_oag_r = params.viewportXywh[aspectIndex] * OuterOAGRatio * params.devicePixelsPerPixel;
					float h_width = params.viewportXywh[aspectIndex] * prismXRatio * params.devicePixelsPerPixel / 2.0;

					//painter.setColor(0.60f, 0.20f, 0.20f, .5f);
					painter.drawCircle(params.viewportCenter[0] * params.devicePixelsPerPixel,
							   params.viewportCenter[1] * params.devicePixelsPerPixel, in_oag_r);
					painter.drawCircle(params.viewportCenter[0] * params.devicePixelsPerPixel,
							   params.viewportCenter[1] * params.devicePixelsPerPixel, out_oag_r);

					QTransform oag_transform = QTransform().translate(params.viewportCenter[0] * params.devicePixelsPerPixel,
							params.viewportCenter[1] * params.devicePixelsPerPixel).rotate(-(ccd->chipRotAngle() + polarAngle + ccd->prismPosAngle()));

					// bottom line
					a = oag_transform.map(QPoint(-h_width, in_oag_r));
					b = oag_transform.map(QPoint(h_width, in_oag_r));
					painter.drawLine2d(a.x(),a.y(), b.x(), b.y());
					// top line
					a = oag_transform.map(QPoint(-h_width, out_oag_r));
					b = oag_transform.map(QPoint(h_width, out_oag_r));
					painter.drawLine2d(a.x(),a.y(), b.x(), b.y());
					// left line
					a = oag_transform.map(QPoint(-h_width, out_oag_r));
					b = oag_transform.map(QPoint(-h_width, in_oag_r));
					painter.drawLine2d(a.x(),a.y(), b.x(), b.y());
					// right line
					a = oag_transform.map(QPoint(h_width, out_oag_r));
					b = oag_transform.map(QPoint(h_width, in_oag_r));
					painter.drawLine2d(a.x(),a.y(), b.x(), b.y());
				}

				// Tool for planning a mosaic astrophotography: shows a small cross at center of CCD's
				// frame and equatorial coordinates for epoch J2000.0 of that center.
				// Details: https://bugs.launchpad.net/stellarium/+bug/1404695

				ratioLimit = 0.25f;
				if (ccdXRatio>=ratioLimit || ccdYRatio>=ratioLimit)
				{
					// draw cross at center
					float cross = width>height ? height/50.f : width/50.f;
					a = transform.map(QPoint(0.f, -cross));
					b = transform.map(QPoint(0.f, cross));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					a = transform.map(QPoint(-cross, 0.f));
					b = transform.map(QPoint(cross, 0.f));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// calculate coordinates of the center and show it
					Vec3d centerPosition;
					Vec2f center = projector->getViewportCenter();
					projector->unProject(center[0], center[1], centerPosition);
					double cx, cy;
					QString cxt, cyt;
					StelUtils::rectToSphe(&cx,&cy,core->equinoxEquToJ2000(centerPosition)); // Calculate RA/DE (J2000.0) and show it...
					bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
					if (withDecimalDegree)
					{
						cxt = StelUtils::radToDecDegStr(cx, 5, false, true);
						cyt = StelUtils::radToDecDegStr(cy);
					}
					else
					{
						cxt = StelUtils::radToHmsStr(cx, true);
						cyt = StelUtils::radToDmsStr(cy, true);
					}
					QString coords = QString("%1: %2/%3").arg(qc_("RA/Dec (J2000.0) of cross", "abbreviated in the plugin")).arg(cxt).arg(cyt);
					a = transform.map(QPoint(-width/2.0, height/2.0 + 5.f));
					painter.drawText(a.x(), a.y(), coords, -(ccd->chipRotAngle() + polarAngle));
				}
			}
		}
	}

}

void Oculars::paintCrosshairs()
{
	StelCore *core = StelApp::getInstance().getCore();
	const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
	// Center of screen
	Vec2i centerScreen(projector->getViewportPosX()+projector->getViewportWidth()/2,
			   projector->getViewportPosY()+projector->getViewportHeight()/2);
	float length = 0.5 * params.viewportFovDiameter;
	// See if we need to scale the length
	if (useMaxEyepieceAngle && oculars[selectedOcularIndex]->appearentFOV() > 0.0)
	{
		length = oculars[selectedOcularIndex]->appearentFOV() * length / maxEyepieceAngle;
	}
	length *= params.devicePixelsPerPixel;
	// Draw the lines
	StelPainter painter(projector);
	painter.setColor(0.77f, 0.14f, 0.16f, 1.f);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0], centerScreen[1] + length);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0], centerScreen[1] - length);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0] + length, centerScreen[1]);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0] - length, centerScreen[1]);
}

void Oculars::paintTelrad()
{
	if (!flagShowOculars) {
		const StelProjectorP projector = StelApp::getInstance().getCore()->getProjection(StelCore::FrameEquinoxEqu);
		// StelPainter drawing
		StelPainter painter(projector);
		painter.setColor(0.77f, 0.14f, 0.16f, 1.f);
		Vec2i centerScreen(projector->getViewportPosX() + projector->getViewportWidth() / 2,
				   projector->getViewportPosY() + projector->getViewportHeight() / 2);
		float pixelsPerRad = projector->getPixelPerRadAtCenter();
		painter.drawCircle(centerScreen[0], centerScreen[1], 0.5 * pixelsPerRad * (M_PI/180) * (0.5));
		painter.drawCircle(centerScreen[0], centerScreen[1], 0.5 * pixelsPerRad * (M_PI/180) * (2.0));
		painter.drawCircle(centerScreen[0], centerScreen[1], 0.5 * pixelsPerRad * (M_PI/180) * (4.0));

	}
}

void Oculars::paintOcularMask(const StelCore *core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	StelPainter painter(prj);
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();

	double inner = 0.5 * params.viewportFovDiameter * params.devicePixelsPerPixel;
	// See if we need to scale the mask
	if (useMaxEyepieceAngle && oculars[selectedOcularIndex]->appearentFOV() > 0.0 && !oculars[selectedOcularIndex]->isBinoculars())
	{
		inner = oculars[selectedOcularIndex]->appearentFOV() * inner / maxEyepieceAngle;
	}

	// Paint the reticale, if needed
	if (!reticleTexture.isNull())
	{
		glEnable(GL_BLEND);
		painter.enableTexture2d(true);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		painter.setColor(0.77f, 0.14f, 0.16f, 1.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
		reticleTexture->bind();

		int textureHeight;
		int textureWidth;
		reticleTexture->getDimensions(textureWidth, textureHeight);

		painter.drawSprite2dMode(params.viewportXywh[2] / 2 * params.devicePixelsPerPixel,
					 params.viewportXywh[3] / 2 * params.devicePixelsPerPixel,
					 inner, reticleRotation);
	}

	if (oculars[selectedOcularIndex]->hasPermanentCrosshair())
	{
		paintCrosshairs();
	}	
	painter.drawViewportShape(inner);
}

void Oculars::paintText(const StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	StelPainter painter(prj);	

	// Get the current instruments
	CCD *ccd = NULL;
	if(selectedCCDIndex != -1)
	{
		ccd = ccds[selectedCCDIndex];
	}
	Ocular *ocular = NULL;
	if(selectedOcularIndex !=-1)
	{
		ocular = oculars[selectedOcularIndex];
	}
	Telescope *telescope = NULL;
	if(selectedTelescopeIndex != -1)
	{
		telescope = telescopes[selectedTelescopeIndex];
	}
	Lens *lens = NULL;
	if(selectedLensIndex != -1)
	{
		lens = lense[selectedLensIndex];
	}

	// set up the color and the GL state
	painter.setColor(0.8f, 0.48f, 0.f, 1.f);
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
	if (flagShowOculars && ocular!=NULL)
	{
		QString ocularNumberLabel;
		QString name = ocular->name();
		if (name.isEmpty())
		{
			ocularNumberLabel = QString(q_("Ocular #%1")).arg(selectedOcularIndex);
		}
		else
		{
			ocularNumberLabel = QString(q_("Ocular #%1: %2")).arg(selectedOcularIndex).arg(name);
		}
		// The name of the ocular could be really long.
		if (name.length() > widthString.length())
		{
			xPosition -= (insetFromRHS / 2.0);
		}
		painter.drawText(xPosition, yPosition, ocularNumberLabel);
		yPosition-=lineHeight;
		
		if (!ocular->isBinoculars())
		{
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
	
			QString lensNumberLabel;
			// Barlow and Shapley lens
			if (lens != NULL) // it's null if lens is not selected (lens index = -1)
			{
				QString lensName = lens->name();
				if (lensName.isEmpty())
				{
					lensNumberLabel = QString(q_("Lens #%1")).arg(selectedLensIndex);
				}
				else
				{
					lensNumberLabel = QString (q_("Lens #%1: %2")).arg(selectedLensIndex).arg(lensName);
				}
			}
			else
			{
				lensNumberLabel = QString (q_("Lens: none"));
			}
			painter.drawText(xPosition, yPosition, lensNumberLabel);
			yPosition-=lineHeight;
		
			// The telescope
			QString telescopeString = "";
			QString magString = "";
			QString fovString = "";
			QString exitPupil = "";

			if (telescope!=NULL)
			{
				QString telescopeName = telescope->name();

				if (telescopeName.isEmpty())
					telescopeString = QString("%1").arg(selectedTelescopeIndex);
				else
					telescopeString = QString("%1: %2").arg(selectedTelescopeIndex).arg(telescopeName);

				// General info
				if (lens!=NULL)
				{
					magString = QString::number(((int)(ocular->magnification(telescope, lens) * 10.0)) / 10.0);
					magString.append(QChar(0x00D7));//Multiplication sign

					fovString = QString::number(((int)(ocular->actualFOV(telescope, lens) * 10000.00)) / 10000.0);
					fovString.append(QChar(0x00B0));//Degree sign

					exitPupil = QString::number(telescope->diameter()/ocular->magnification(telescope, lens), 'f', 2);
				}
			}

			painter.drawText(xPosition, yPosition, QString(q_("Telescope #%1")).arg(telescopeString));
			yPosition-=lineHeight;

			painter.drawText(xPosition, yPosition, QString(q_("Magnification: %1")).arg(magString));
			yPosition-=lineHeight;

			painter.drawText(xPosition, yPosition, QString(q_("Exit pupil: %1 mm")).arg(exitPupil));
			yPosition-=lineHeight;

			painter.drawText(xPosition, yPosition, QString(q_("FOV: %1")).arg(fovString));
		}
	}

	// The CCD
	if (flagShowCCD && ccd!=NULL)
	{
		QString ccdSensorLabel, ccdInfoLabel;
		QString name = "";
		QString telescopeName = "";
		double fovX = 0.0;
		double fovY = 0.0;
		if (telescope!=NULL && lens!=NULL)
		{
			fovX = ((int)(ccd->getActualFOVx(telescope, lens) * 1000.0)) / 1000.0;
			fovY = ((int)(ccd->getActualFOVy(telescope, lens) * 1000.0)) / 1000.0;
			name = ccd->name();
			telescopeName = telescope->name();
		}

		ccdInfoLabel = QString(q_("Dimensions: %1")).arg(getDimensionsString(fovX, fovY));
		
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
		// The telescope
		QString telescopeNumberLabel;		
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
		painter.drawText(xPosition, yPosition, ccdSensorLabel);
		yPosition-=lineHeight;
		painter.drawText(xPosition, yPosition, ccdInfoLabel);
		yPosition-=lineHeight;
		painter.drawText(xPosition, yPosition, telescopeNumberLabel);
	}
	
}

void Oculars::validateAndLoadIniFile()
{
	// Insure the module directory exists
	StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Oculars");
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	QString ocularIniPath = StelFileMgr::findFile("modules/Oculars/", flags) + "ocular.ini";
	if (ocularIniPath.isEmpty())
		return;

	// If the ini file does not already exist, create it from the resource in the QT resource
	if(!QFileInfo(ocularIniPath).exists())
	{
		QFile src(":/ocular/default_ocular.ini");
		if (!src.copy(ocularIniPath))
		{
			qWarning() << "Oculars::validateIniFile cannot copy default_ocular.ini resource to [non-existing] "
										+ ocularIniPath;
		}
		else
		{
			qDebug() << "Oculars::validateIniFile copied default_ocular.ini to " << QDir::toNativeSeparators(ocularIniPath);
			// The resource is read only, and the new file inherits this, so set write-able.
			QFile dest(ocularIniPath);
			dest.setPermissions(dest.permissions() | QFile::WriteOwner);
		}
	}
	else
	{
		qDebug() << "Oculars::validateIniFile ocular.ini exists at: " << QDir::toNativeSeparators(ocularIniPath) << ". Checking version...";
		QSettings settings(ocularIniPath, QSettings::IniFormat);
		double ocularsVersion = settings.value("oculars_version", "0.0").toDouble();
		qWarning() << "Oculars::validateIniFile found existing ini file version " << ocularsVersion;

		if (ocularsVersion < MIN_OCULARS_INI_VERSION)
		{
			qWarning() << "Oculars::validateIniFile existing ini file version " << ocularsVersion
								 << " too old to use; required version is " << MIN_OCULARS_INI_VERSION << ". Copying over new one.";
			// delete last "old" file, if it exists
			QFile deleteFile(ocularIniPath + ".old");
			deleteFile.remove();

			// Rename the old one, and copy over a new one
			QFile oldFile(ocularIniPath);
			if (!oldFile.rename(ocularIniPath + ".old"))
			{
				qWarning() << "Oculars::validateIniFile cannot move ocular.ini resource to ocular.ini.old at path  " + QDir::toNativeSeparators(ocularIniPath);
			}
			else
			{
				qWarning() << "Oculars::validateIniFile ocular.ini resource renamed to ocular.ini.old at path  " + QDir::toNativeSeparators(ocularIniPath);
				QFile src(":/ocular/default_ocular.ini");
				if (!src.copy(ocularIniPath))
				{
					qWarning() << "Oculars::validateIniFile cannot copy default_ocular.ini resource to [non-existing] " + QDir::toNativeSeparators(ocularIniPath);
				}
				else
				{
					qDebug() << "Oculars::validateIniFile copied default_ocular.ini to " << QDir::toNativeSeparators(ocularIniPath);
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
	StelSkyDrawer *skyManager = core->getSkyDrawer();

	gridManager->setFlagAzimuthalGrid(flagAzimuthalGrid);
	gridManager->setFlagGalacticGrid(flagGalacticGrid);
	gridManager->setFlagEquatorGrid(flagEquatorGrid);
	gridManager->setFlagEquatorJ2000Grid(flagEquatorJ2000Grid);
	gridManager->setFlagEquatorLine(flagEquatorLine);
	gridManager->setFlagEclipticLine(flagEclipticLine);
	gridManager->setFlagEclipticJ2000Grid(flagEclipticJ2000Grid);
	gridManager->setFlagMeridianLine(flagMeridianLine);
	gridManager->setFlagLongitudeLine(flagLongitudeLine);
	gridManager->setFlagHorizonLine(flagHorizonLine);
	gridManager->setFlagGalacticEquatorLine(flagGalacticEquatorLine);
	skyManager->setFlagLuminanceAdaptation(flagAdaptation);
	skyManager->setFlagStarMagnitudeLimit(flagLimitStars);
	skyManager->setFlagPlanetMagnitudeLimit(flagLimitPlanets);
	skyManager->setFlagNebulaMagnitudeLimit(flagLimitDSOs);
	skyManager->setCustomStarMagnitudeLimit(magLimitStars);
	skyManager->setCustomPlanetMagnitudeLimit(magLimitPlanets);
	skyManager->setCustomNebulaMagnitudeLimit(magLimitDSOs);
	movementManager->setFlagTracking(false);
	movementManager->setFlagEnableZoomKeys(true);
	movementManager->setFlagEnableMouseNavigation(true);

	GETSTELMODULE(SolarSystem)->setFlagMoonScale(flagMoonScale);

	// Set the screen display
	core->setMaskType(StelProjector::MaskNone);
	core->setFlipHorz(flipHorz);
	core->setFlipVert(flipVert);

	if (getFlagInitFovUsage())
		movementManager->zoomTo(movementManager->getInitFov());
	else
		movementManager->zoomTo(initialFOV);
}

void Oculars::zoom(bool zoomedIn)
{
	if (flagShowOculars && selectedOcularIndex == -1)
	{
		// The user cycled out the selected ocular
		flagShowOculars = false;
	}

	if (flagShowOculars)
	{
		if (!zoomedIn)
		{
			StelCore *core = StelApp::getInstance().getCore();

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
			flagLongitudeLine = gridManager->getFlagLongitudeLine();
			flagHorizonLine = gridManager->getFlagHorizonLine();
			flagGalacticEquatorLine = gridManager->getFlagGalacticEquatorLine();

			StelSkyDrawer *skyManager = core->getSkyDrawer();
			// Current state
			flagAdaptation = skyManager->getFlagLuminanceAdaptation();
			flagLimitStars = skyManager->getFlagStarMagnitudeLimit();
			flagLimitPlanets = skyManager->getFlagPlanetMagnitudeLimit();
			flagLimitDSOs = skyManager->getFlagNebulaMagnitudeLimit();
			magLimitStars = skyManager->getCustomStarMagnitudeLimit();
			magLimitPlanets = skyManager->getCustomPlanetMagnitudeLimit();
			magLimitDSOs = skyManager->getCustomNebulaMagnitudeLimit();

			flagMoonScale = GETSTELMODULE(SolarSystem)->getFlagMoonScale();

			flipHorz = core->getFlipHorz();
			flipVert = core->getFlipVert();

			StelMovementMgr *movementManager = core->getMovementMgr();
			initialFOV = movementManager->getCurrentFov();
		}

		// set new state
		zoomOcular();
	}
	else
	{
		//reset to original state
		unzoomOcular();
	}
}

void Oculars::zoomOcular()
{
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	GridLinesMgr *gridManager = (GridLinesMgr *)StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr");

	StelSkyDrawer *skyManager = core->getSkyDrawer();

	gridManager->setFlagAzimuthalGrid(false);
	gridManager->setFlagGalacticGrid(false);
	gridManager->setFlagEquatorGrid(false);
	gridManager->setFlagEquatorJ2000Grid(false);
	gridManager->setFlagEquatorLine(false);
	gridManager->setFlagEclipticLine(false);
	gridManager->setFlagEclipticJ2000Grid(false);
	gridManager->setFlagMeridianLine(false);
	gridManager->setFlagLongitudeLine(false);
	gridManager->setFlagHorizonLine(false);
	gridManager->setFlagGalacticEquatorLine(false);
	skyManager->setFlagLuminanceAdaptation(false);

	GETSTELMODULE(SolarSystem)->setFlagMoonScale(false);
	
	movementManager->setFlagTracking(true);
	movementManager->setFlagEnableZoomKeys(false);
	movementManager->setFlagEnableMouseNavigation(false);
	
	// We won't always have a selected object
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected())
	{
		StelObjectP selectedObject = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0];
		movementManager->moveToJ2000(selectedObject->getEquinoxEquatorialPos(core), movementManager->mountFrameToJ2000(Vec3d(0., 0., 1.)), 0.0, StelMovementMgr::ZoomIn);
	}

	// Set the screen display
	core->setMaskType(StelProjector::MaskDisk);
	Ocular * ocular = oculars[selectedOcularIndex];
	Telescope * telescope = NULL;
	Lens * lens = NULL;
	// Only consider flip is we're not binoculars
	if (ocular->isBinoculars())
	{
		core->setFlipHorz(false);
		core->setFlipVert(false);
	}
	else
	{
		if (selectedLensIndex >= 0)
		{
			lens = lense[selectedLensIndex];
		}
		telescope = telescopes[selectedTelescopeIndex];
		core->setFlipHorz(telescope->isHFlipped());
		core->setFlipVert(telescope->isVFlipped());		
	}

	// Limit stars and DSOs	if it enable and it's telescope + eyepiece combination
	if (getFlagLimitMagnitude() && !ocular->isBinoculars())
	{
		// Simplified calculation of the penetrating power of the telescope
		double limitMag = 2.1 + 5*std::log10(telescope->diameter());

		skyManager->setFlagStarMagnitudeLimit(true);
		skyManager->setFlagNebulaMagnitudeLimit(true);
		skyManager->setCustomStarMagnitudeLimit(limitMag);
		skyManager->setCustomNebulaMagnitudeLimit(limitMag);
	}

	actualFOV = ocular->actualFOV(telescope, lens);
	// See if the mask was scaled; if so, correct the actualFOV.
	if (useMaxEyepieceAngle && ocular->appearentFOV() > 0.0 && !ocular->isBinoculars())
	{
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

Lens* Oculars::selectedLens()
{
	if (selectedLensIndex >= 0 && selectedLensIndex < lense.count())
	{
		return lense[selectedLensIndex];
	}
	return NULL;
}

QMenu* Oculars::addLensSubmenu(QMenu* parent)
{
	Q_ASSERT(parent);
    
	QMenu *submenu = new QMenu(q_("&Lens"), parent);
	submenu->addAction(q_("&Previous lens"), this, SLOT(decrementLensIndex()));
	submenu->addAction(q_("&Next lens"), this, SLOT(incrementLensIndex()));
	submenu->addSeparator();
	submenu->addAction(q_("None"), this, SLOT(disableLens()));

	for (int index = 0; index < lense.count(); ++index)
	{
		QString label;
		if (index < 10)
		{
			label = QString("&%1: %2").arg(index).arg(lense[index]->name());
		}
		else
		{
			label = lense[index]->name();
		}
		QAction* action = submenu->addAction(label, lenseSignalMapper, SLOT(map()));
		if (index == selectedLensIndex)
		{
			action->setCheckable(true);
			action->setChecked(true);
		}
		lenseSignalMapper->setMapping(action, QString("%1").arg(index));
	}
	return submenu;
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

void Oculars::setFlagLimitMagnitude(const bool b)
{
	flagLimitMagnitude = b;
	settings->setValue("limit_stellar_magnitude", b);
	settings->sync();
}

bool Oculars::getFlagLimitMagnitude() const
{
	return flagLimitMagnitude;
}

void Oculars::setFlagInitFovUsage(const bool b)
{
	flagInitFOVUsage = b;
	settings->setValue("use_initial_fov", b);
	settings->sync();
}

bool Oculars::getFlagInitFovUsage() const
{
	return flagInitFOVUsage;
}

void Oculars::setFlagUseFlipForCCD(const bool b)
{
	flagUseFlipForCCD = b;
	settings->setValue("use_ccd_flip", b);
	settings->sync();
}

bool Oculars::getFlagUseFlipForCCD() const
{
	return flagUseFlipForCCD;
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
