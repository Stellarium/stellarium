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
#include "ConstellationMgr.hpp"
#include "AsterismMgr.hpp"
#include "MilkyWay.hpp"
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
#include "StelPropertyMgr.hpp"
#include "LandscapeMgr.hpp"

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QGraphicsWidget>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QMessageBox>

#include <cmath>
#include <stdexcept>

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
	info.contact = "https://github.com/Stellarium/stellarium";
	info.description = N_("Shows the sky as if looking through a telescope eyepiece. (Only magnification and field of view are simulated.) It can also show a sensor frame and a Telrad sight.");
	info.version = OCULARS_PLUGIN_VERSION;
	info.license = OCULARS_PLUGIN_LICENSE;
	return info;
}


/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Instance Methods
#endif
/* ****************************************************************************************************************** */
Oculars::Oculars()
	: selectedCCDIndex(-1)
	, selectedOcularIndex(-1)
	, selectedTelescopeIndex(-1)
	, selectedLensIndex(-1)
	, selectedCCDRotationAngle(0.0)
	, selectedCCDPrismPositionAngle(0.0)
	, arrowButtonScale(150)
	, flagShowCCD(false)
	, flagShowOculars(false)
	, flagShowCrosshairs(false)
	, flagShowTelrad(false)
	, usageMessageLabelID(-1)
	, flagCardinalPointsMain(false)
	, flagAdaptationMain(false)
	, flagLimitStarsMain(false)
	, magLimitStarsMain(0.0)
	, flagLimitStarsOculars(false)
	, magLimitStarsOculars(0.0)
	, flagAutoLimitMagnitude(false)
	, flagLimitDSOsMain(false)
	, magLimitDSOsMain(0.0)
	, flagLimitPlanetsMain(false)
	, magLimitPlanetsMain(0.0)
	, relativeStarScaleMain(1.0)
	, absoluteStarScaleMain(1.0)
	, relativeStarScaleOculars(1.0)
	, absoluteStarScaleOculars(1.0)
	, relativeStarScaleCCD(1.0)
	, absoluteStarScaleCCD(1.0)
	, flagMoonScaleMain(false)
	, flagMinorBodiesScaleMain(false)
	, milkyWaySaturation(1.0)
	, maxEyepieceAngle(0.0)
	, flagRequireSelection(true)
	, flagScaleImageCircle(true)
	, flagGuiPanelEnabled(false)
	, flagDMSDegrees(false)
	, flagSemiTransparency(false)
	, transparencyMask(85)
	, flagHideGridsLines(false)
	, flagGridLinesDisplayedMain(true)
	, flagConstellationLinesMain(true)
	, flagConstellationBoundariesMain(true)
	, flagAsterismLinesMain(true)
	, flagRayHelpersLinesMain(true)
	, flipVertMain(false)
	, flipHorzMain(false)
	, pxmapGlow(Q_NULLPTR)
	, pxmapOnIcon(Q_NULLPTR)
	, pxmapOffIcon(Q_NULLPTR)
	, toolbarButton(Q_NULLPTR)
	, flagShowOcularsButton(false)
	, ocularDialog(Q_NULLPTR)
	, ready(false)
	, actionShowOcular(Q_NULLPTR)
	, actionShowCrosshairs(Q_NULLPTR)
	, actionShowSensor(Q_NULLPTR)
	, actionShowTelrad(Q_NULLPTR)
	, actionConfiguration(Q_NULLPTR)
	, actionMenu(Q_NULLPTR)
	, actionTelescopeIncrement(Q_NULLPTR)
	, actionTelescopeDecrement(Q_NULLPTR)
	, actionOcularIncrement(Q_NULLPTR)
	, actionOcularDecrement(Q_NULLPTR)
	, guiPanel(Q_NULLPTR)
	, guiPanelFontSize(12)
	, textColor(0.)
	, lineColor(0.)
	, focuserColor(0.)
	, actualFOV(0.)
	, initialFOV(0.)
	, flagInitFOVUsage(false)
	, flagInitDirectionUsage(false)
	, flagAutosetMountForCCD(false)
	, flagScalingFOVForTelrad(false)
	, flagScalingFOVForCCD(true)
	, flagShowResolutionCriteria(false)
	, equatorialMountEnabledMain(false)
	, reticleRotation(0.)
	, flagShowCcdCropOverlay(false)
	, flagShowCcdCropOverlayPixelGrid(false)
	, ccdCropOverlayHSize(DEFAULT_CCD_CROP_OVERLAY_SIZE)
	, ccdCropOverlayVSize(DEFAULT_CCD_CROP_OVERLAY_SIZE)
	, flagShowContour(false)
	, flagShowCardinals(false)
	, flagAlignCrosshair(false)
	, telradFOV(0.5f,2.f,4.f)
	, flagShowFocuserOverlay(false)
	, flagUseSmallFocuserOverlay(false)
	, flagUseMediumFocuserOverlay(true)
	, flagUseLargeFocuserOverlay(true)
{
	setObjectName("Oculars");
	// Design font size is 14, based on default app fontsize 13.
	setFontSizeFromApp(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSizeFromApp(int)));

	ccds = QList<CCD *>();
	oculars = QList<Ocular *>();
	telescopes = QList<Telescope *>();
	lenses = QList<Lens *> ();

#ifdef Q_OS_MAC
	qt_set_sequence_auto_mnemonic(true);
#endif
}

Oculars::~Oculars()
{
	delete ocularDialog;
	ocularDialog = Q_NULLPTR;
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
	qDeleteAll(lenses);
	lenses.clear();
}

QSettings* Oculars::getSettings()
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
	for (auto* ccd : qAsConst(ccds))
	{
		ccd->writeToSettings(settings, index);
		index++;
	}
	index = 0;
	for (auto* ocular : qAsConst(oculars))
	{
		ocular->writeToSettings(settings, index);
		index++;
	}
	index = 0;
	for (auto* telescope : qAsConst(telescopes))
	{
		telescope->writeToSettings(settings, index);
		index++;
	}
	index = 0;
	for (auto* lens : qAsConst(lenses))
	{
		lens->writeToSettings(settings, index);
		index++;
	}

	settings->setValue("ocular_count", oculars.count());
	settings->setValue("telescope_count", telescopes.count());
	settings->setValue("ccd_count", ccds.count());
	settings->setValue("lens_count", lenses.count());
	settings->setValue("ocular_index", selectedOcularIndex);
	settings->setValue("telescope_index", selectedTelescopeIndex);
	settings->setValue("ccd_index", selectedCCDIndex);
	settings->setValue("lens_index", selectedLensIndex);

	StelCore *core = StelApp::getInstance().getCore();
	StelSkyDrawer *skyDrawer = core->getSkyDrawer();
	disconnect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double)));
	disconnect(skyDrawer, SIGNAL(flagStarMagnitudeLimitChanged(bool)), this, SLOT(handleStarMagLimitToggle(bool)));
	if (flagShowCCD)
	{
		// Retrieve and restore star scales
		relativeStarScaleCCD=skyDrawer->getRelativeStarScale();
		absoluteStarScaleCCD=skyDrawer->getAbsoluteStarScale();
		skyDrawer->setRelativeStarScale(relativeStarScaleMain);
		skyDrawer->setAbsoluteStarScale(absoluteStarScaleMain);
	}
	else if (flagShowOculars)
	{
		qDebug() << "Oculars::deinit() .. restoring skyDrawer values while ocular view is active";

		if (!getFlagAutoLimitMagnitude())
		{
			flagLimitStarsOculars=skyDrawer->getFlagStarMagnitudeLimit();
			magLimitStarsOculars=skyDrawer->getCustomStarMagnitudeLimit();
		}
		skyDrawer->setCustomStarMagnitudeLimit(magLimitStarsMain);
		skyDrawer->setFlagStarMagnitudeLimit(flagLimitStarsMain);
		// Retrieve and restore star scales
		relativeStarScaleOculars=skyDrawer->getRelativeStarScale();
		absoluteStarScaleOculars=skyDrawer->getAbsoluteStarScale();
		skyDrawer->setRelativeStarScale(relativeStarScaleMain);
		skyDrawer->setAbsoluteStarScale(absoluteStarScaleMain);
	}

	settings->setValue("stars_scale_relative", QString::number(relativeStarScaleOculars, 'f', 2));
	settings->setValue("stars_scale_absolute", QString::number(absoluteStarScaleOculars, 'f', 2));
	settings->setValue("stars_scale_relative_ccd", QString::number(relativeStarScaleCCD, 'f', 2));
	settings->setValue("stars_scale_absolute_ccd", QString::number(absoluteStarScaleCCD, 'f', 2));
	settings->setValue("limit_stellar_magnitude_oculars_val", QString::number(magLimitStarsOculars, 'f', 2));
	settings->setValue("limit_stellar_magnitude_oculars", flagLimitStarsOculars);
	settings->setValue("text_color", textColor.toStr());
	settings->setValue("line_color", lineColor.toStr());
	settings->setValue("focuser_color", focuserColor.toStr());
	settings->sync();

	disconnect(this, SIGNAL(selectedOcularChanged(int)), this, SLOT(updateOcularReticle()));
	//disconnect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	disconnect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslateGui()));

	cardinalsNormalTexture.clear();
	cardinalsMirroredTexture.clear();
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
		if (selectedOcularIndex >= 0)
		{
			paintOcularMask(core);
			if (flagShowCrosshairs)
				paintCrosshairs();

			if (!flagGuiPanelEnabled)
			{
				// Paint the information in the upper-right hand corner
				paintText(core);
			}
		}
		else
		{
			qWarning() << "Oculars: the selected ocular index of "
				   << selectedOcularIndex << " is greater than the ocular count of "
				   << oculars.count() << ". Module disabled!";
		}
	}
	else if (flagShowCCD)
	{
		paintCCDBounds();
		if (!flagGuiPanelEnabled)
		{
			// Paint the information in the upper-right hand corner
			paintText(core);
		}
	}
}

//! Determine which "layer" the plugin's drawing will happen on.
double Oculars::getCallOrder(StelModuleActionName actionName) const
{
	double order = 1000.0; // Very low priority, unless we interact.

	if (actionName==StelModule::ActionHandleMouseMoves ||
	    actionName==StelModule::ActionHandleMouseClicks)
	{
		// Make sure we are called before MovementMgr (we need to even call it once!)
		order = StelApp::getInstance().getModuleMgr().getModule("StelMovementMgr")->getCallOrder(actionName) - 1.0;
	}
	else if (actionName==StelModule::ActionDraw)
	{
		order = GETSTELMODULE(LabelMgr)->getCallOrder(actionName) + 100.0;
	}
	return order;
}

void Oculars::handleMouseClicks(class QMouseEvent* event)
{
	StelCore *core = StelApp::getInstance().getCore();
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000, StelCore::RefractionAuto);
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
	qreal ppx = params.devicePixelsPerPixel;
	
	if (guiPanel)
	{
		// Remove all events on the sky within Ocular GUI Panel.
		if (event->x()>guiPanel->pos().x() && event->y()>(prj->getViewportHeight()-guiPanel->size().height()))
		{
			event->setAccepted(true);
			return;
		}
	}

	// In case we show oculars with black circle, ignore mouse presses outside image circle:
	// https://sourceforge.net/p/stellarium/discussion/278769/thread/57893bb3/?limit=25#75c0
	if ((flagShowOculars) ) //&& !getFlagUseSemiTransparency()) // Not sure: ignore or allow selection of semi-hidden stars?
	{
		float wh = prj->getViewportWidth()*0.5f; // get half of width of the screen
		float hh = prj->getViewportHeight()*0.5f; // get half of height of the screen
		float mx = event->x()-wh; // point 0 in center of the screen, axis X directed to right
		float my = event->y()-hh; // point 0 in center of the screen, axis Y directed to bottom

		double inner = 0.5 * params.viewportFovDiameter * ppx;
		// See if we need to scale the mask
		if (flagScaleImageCircle && oculars[selectedOcularIndex]->apparentFOV() > 0.0 && !oculars[selectedOcularIndex]->isBinoculars())
		{
			inner = oculars[selectedOcularIndex]->apparentFOV() * inner / maxEyepieceAngle;
		}

		if (mx*mx+my*my>static_cast<float>(inner*inner)) // click outside ocular circle? Gobble event.
		{
			event->setAccepted(true);
			return;
		}
	}

	StelMovementMgr *movementManager = core->getMovementMgr();

	if (flagShowOculars)
		movementManager->handleMouseClicks(event); // force it here for selection!

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
		// The ocular is displayed, but no object is selected.  So don't track the stars.  We may have locked
		// the position of the screen if the movement keys were used.  so call this to be on the safe side.
		movementManager->setFlagLockEquPos(false);		
	}
	event->setAccepted(false);
}

void Oculars::init()
{
	// Load settings from ocular.ini
	try {
		validateAndLoadIniFile();
		// assume all is well
		ready = true;

		setFlagRequireSelection(settings->value("require_selection_to_zoom", true).toBool());
		flagScaleImageCircle = settings->value("use_max_exit_circle", false).toBool();
		int ocularCount = settings->value("ocular_count", 0).toInt();
		int actualOcularCount = ocularCount;
		for (int index = 0; index < ocularCount; index++)
		{
			Ocular *newOcular = Ocular::ocularFromSettings(settings, index);
			if (newOcular != Q_NULLPTR)
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
			selectedOcularIndex = settings->value("ocular_index", 0).toInt();
		}

		int ccdCount = settings->value("ccd_count", 0).toInt();
		int actualCcdCount = ccdCount;
		for (int index = 0; index < ccdCount; index++)
		{
			CCD *newCCD = CCD::ccdFromSettings(settings, index);
			if (newCCD != Q_NULLPTR)
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
		selectedCCDIndex = settings->value("ccd_index", 0).toInt();

		int telescopeCount = settings->value("telescope_count", 0).toInt();
		int actualTelescopeCount = telescopeCount;
		for (int index = 0; index < telescopeCount; index++)
		{
			Telescope *newTelescope = Telescope::telescopeFromSettings(settings, index);
			if (newTelescope != Q_NULLPTR)
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
			selectedTelescopeIndex = settings->value("telescope_index", 0).toInt();
		}

		int lensCount = settings->value("lens_count", 0).toInt();
		int actualLensCount = lensCount;
		for (int index = 0; index<lensCount; index++)
		{
			Lens *newLens = Lens::lensFromSettings(settings, index);
			if (newLens != Q_NULLPTR)
			{
				lenses.append(newLens);
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
		selectedLensIndex=settings->value("lens_index", -1).toInt(); // Lens is not selected by default!

		pxmapGlow = new QPixmap(":/graphicGui/miscGlow32x32.png");
		pxmapOnIcon = new QPixmap(":/ocular/bt_ocular_on.png");
		pxmapOffIcon = new QPixmap(":/ocular/bt_ocular_off.png");

		ocularDialog = new OcularDialog(this, &ccds, &oculars, &telescopes, &lenses);
		initializeActivationActions();
		determineMaxEyepieceAngle();

		guiPanelFontSize=settings->value("gui_panel_fontsize", 12).toInt();
		enableGuiPanel(settings->value("enable_control_panel", true).toBool());
		textColor=Vec3f(settings->value("text_color", "0.8,0.48,0.0").toString());
		lineColor=Vec3f(settings->value("line_color", "0.77,0.14,0.16").toString());
		telradFOV=Vec4f(settings->value("telrad_fov", "0.5,2.0,4.0,0.0").toString());
		focuserColor=Vec3f(settings->value("focuser_color", "0.0,0.67,1.0").toString());

		// This must come ahead of setFlagAutosetMountForCCD (GH #505)
		StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();
		equatorialMountEnabledMain = propMgr->getStelPropertyValue("StelMovementMgr.equatorialMount").toBool();

		// For historical reasons, name of .ini entry and description of checkbox (and therefore flag name) are reversed.
		setFlagDMSDegrees( ! settings->value("use_decimal_degrees", false).toBool());
		setFlagAutoLimitMagnitude(settings->value("autolimit_stellar_magnitude", true).toBool());
		flagLimitStarsOculars=settings->value("limit_stellar_magnitude_oculars", false).toBool();
		magLimitStarsOculars=settings->value("limit_stellar_magnitude_oculars_val", 12.).toDouble();
		connect(this, SIGNAL(flagAutoLimitMagnitudeChanged(bool)), this, SLOT(handleAutoLimitToggle(bool))); // only after first initialisation!
		setFlagInitFovUsage(settings->value("use_initial_fov", false).toBool());
		setFlagInitDirectionUsage(settings->value("use_initial_direction", false).toBool());
		setFlagUseSemiTransparency(settings->value("use_semi_transparency", false).toBool());
		setTransparencyMask(settings->value("transparency_mask", 85).toInt());
		setFlagHideGridsLines(settings->value("hide_grids_and_lines", true).toBool());
		setFlagAutosetMountForCCD(settings->value("use_mount_autoset", false).toBool());
		setFlagScalingFOVForTelrad(settings->value("use_telrad_fov_scaling", true).toBool());
		setFlagScalingFOVForCCD(settings->value("use_ccd_fov_scaling", true).toBool());
		setFlagShowResolutionCriteria(settings->value("show_resolution_criteria", false).toBool());
		// TODO: Remove this conversion tool in version 0.21 or 0.22
		if (settings->value("arrow_scale").toDouble()<100.) // convert old value and type
			setArrowButtonScale(static_cast<int>(settings->value("arrow_scale", 1.5).toDouble()*100.));
		else
			setArrowButtonScale(settings->value("arrow_scale", 150).toInt());
		setFlagShowOcularsButton(settings->value("show_toolbar_button", false).toBool());
		relativeStarScaleOculars=settings->value("stars_scale_relative", 1.0).toDouble();
		absoluteStarScaleOculars=settings->value("stars_scale_absolute", 1.0).toDouble();
		relativeStarScaleCCD=settings->value("stars_scale_relative_ccd", 1.0).toDouble();
		absoluteStarScaleCCD=settings->value("stars_scale_absolute_ccd", 1.0).toDouble();
		setFlagShowCcdCropOverlay(settings->value("show_ccd_crop_overlay", false).toBool());
		setFlagShowCcdCropOverlayPixelGrid(settings-> value("ccd_crop_overlay_pixel_grid",false).toBool());
		setCcdCropOverlayHSize(settings->value("ccd_crop_overlay_hsize", DEFAULT_CCD_CROP_OVERLAY_SIZE).toInt());
		setCcdCropOverlayVSize(settings->value("ccd_crop_overlay_vsize", DEFAULT_CCD_CROP_OVERLAY_SIZE).toInt());
		setFlagShowContour(settings->value("show_ocular_contour", false).toBool());
		setFlagShowCardinals(settings->value("show_ocular_cardinals", false).toBool());
		setFlagAlignCrosshair(settings->value("align_crosshair", false).toBool());
		setFlagShowFocuserOverlay(settings->value("show_focuser_overlay", false).toBool());
		setFlagUseSmallFocuserOverlay(settings->value("use_small_focuser_overlay", false).toBool());
		setFlagUseMediumFocuserOverlay(settings->value("use_medium_focuser_overlay", true).toBool());
		setFlagUseLargeFocuserOverlay(settings->value("use_large_focuser_overlay", false).toBool());
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate ocular.ini file or create a default one for Ocular plugin: " << e.what();
		ready = false;
	}

	cardinalsNormalTexture = StelApp::getInstance().getTextureManager().createTexture(":/ocular/cardinals.png");
	cardinalsMirroredTexture = StelApp::getInstance().getTextureManager().createTexture(":/ocular/cardinals-mirrored.png");	
	// enforce check existence of reticle for the current eyepiece
	updateOcularReticle();

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslateGui()));
	connect(this, SIGNAL(selectedOcularChanged(int)), this, SLOT(updateOcularReticle()));
	StelCore *core = StelApp::getInstance().getCore();
	StelSkyDrawer *skyDrawer = core->getSkyDrawer();
	connect(skyDrawer, SIGNAL(flagStarMagnitudeLimitChanged(bool)), this, SLOT(handleStarMagLimitToggle(bool)));
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
		for (const auto* ocular : oculars)
		{
			if (ocular->apparentFOV() > maxEyepieceAngle)
			{
				maxEyepieceAngle = ocular->apparentFOV();
			}
		}
	}
	// ensure it is not zero
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
		// If we are already in Ocular mode, we must reset scalings because zoom() also resets.
		StelSkyDrawer *skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();
		skyDrawer->setRelativeStarScale(relativeStarScaleMain);
		skyDrawer->setAbsoluteStarScale(absoluteStarScaleMain);
		zoom(true);
	}
	else if (flagShowCCD)
		setScreenFOVForCCD();
}

void Oculars::setFlagScaleImageCircle(bool state)
{
	if (state)
	{
		determineMaxEyepieceAngle();
	}
	flagScaleImageCircle = state;
	settings->setValue("use_max_exit_circle", state);
	settings->sync();
	emit flagScaleImageCircleChanged(state);
}

void Oculars::setScreenFOVForCCD()
{
	// CCD is not shown and FOV scaling is disabled, but telescope is changed - do not change FOV!
	if (!(getFlagScalingFOVForCCD() && flagShowCCD))
		return;

	Lens * lens = selectedLensIndex >=0  ? lenses[selectedLensIndex] : Q_NULLPTR;
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
		double factor = 1.75;
		if (ccds[selectedCCDIndex]->hasOAG()) factor *= 2;
		movementManager->setFlagTracking(true);
		movementManager->zoomTo(actualFOVx * factor, 0.f);
	}
}

void Oculars::enableGuiPanel(bool enable)
{
	if (enable)
	{
		if (!guiPanel)
		{
			StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
			if (gui)
			{
				guiPanel = new OcularsGuiPanel(this, gui->getSkyGui());
				if (flagShowOculars)
					guiPanel->showOcularGui();
				else if (flagShowCCD)
					guiPanel->showCcdGui();
			}
		}
	}
	else
	{
		if (guiPanel)
		{
			guiPanel->hide();
			delete guiPanel;
			guiPanel = Q_NULLPTR;
		}
	}
	flagGuiPanelEnabled = enable;
	settings->setValue("enable_control_panel", enable);
	settings->sync();
	emit flagGuiPanelEnabledChanged(enable);
}

void Oculars::retranslateGui()
{
	if (guiPanel)
	{
		// TODO: Fix this hack!
		
		// Delete and re-create the panel to retranslate its trings
		guiPanel->hide();
		delete guiPanel;
		guiPanel = Q_NULLPTR;
		
		StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		if (gui)
		{
			guiPanel = new OcularsGuiPanel(this, gui->getSkyGui());
			if (flagShowOculars)
				guiPanel->showOcularGui();
			else if (flagShowCCD)
				guiPanel->showCcdGui();
		}
	}
}

void Oculars::updateOcularReticle(void)
{
	reticleRotation = 0.0;
	QString reticleTexturePath=oculars[selectedOcularIndex]->reticlePath();
	if (reticleTexturePath.length()==0)
		reticleTexture=StelTextureSP();
	else
	{
		StelTextureMgr& manager = StelApp::getInstance().getTextureManager();
		//Load OpenGL textures
		StelTexture::StelTextureParams params;
		params.generateMipmaps = true;
		reticleTexture = manager.createTexture(reticleTexturePath, params);
	}
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
		enableOcular(false);
	}
	else
	{
		if (selectedOcularIndex >= oculars.count())
			selectedOcularIndex = oculars.count() - 1;

		if (flagShowOculars)
			emit selectedOcularChanged(selectedOcularIndex);
	}

	if (telescopes.isEmpty())
	{
		selectedTelescopeIndex = -1;
		enableOcular(false);
		toggleCCD(false);
	}
	else
	{
		if (selectedTelescopeIndex >= telescopes.count())
			selectedTelescopeIndex = telescopes.count() - 1;

		if (flagShowOculars || flagShowCCD)
			emit selectedTelescopeChanged(selectedTelescopeIndex);
	}

	if (ccds.isEmpty())
	{
		selectedCCDIndex = -1;
		toggleCCD(false);
	}
	else
	{
		if (selectedCCDIndex >= ccds.count())
			selectedCCDIndex = ccds.count() - 1;

		if (flagShowCCD)
			emit selectedCCDChanged(selectedCCDIndex);
	}

	if (lenses.isEmpty())
		selectedLensIndex = -1;
	else
	{
		if (selectedLensIndex >= lenses.count())
			selectedLensIndex = lenses.count() - 1;
	}
}

void Oculars::ccdRotationReset()
{
	if (selectedCCDIndex<0)
		return;
	CCD *ccd = ccds[selectedCCDIndex];
	if (ccd)
	{
		ccd->setChipRotAngle(0.0);
		emit(selectedCCDChanged(selectedCCDIndex));
		emit selectedCCDRotationAngleChanged(0.0);
	}
}

void Oculars::prismPositionAngleReset()
{
	if (selectedCCDIndex<0)
		return;
	CCD *ccd = ccds[selectedCCDIndex];
	if (ccd)
	{
		ccd->setPrismPosAngle(0.0);
		emit(selectedCCDChanged(selectedCCDIndex));
		emit selectedCCDPrismPositionAngleChanged(0.0);
	}
}

void Oculars::setSelectedCCDRotationAngle(double angle)
{
	if (selectedCCDIndex<0)
		return;

	CCD *ccd = ccds[selectedCCDIndex];
	if (ccd)
	{
		ccd->setChipRotAngle(angle);
		emit selectedCCDRotationAngleChanged(angle);
	}
}

double Oculars::getSelectedCCDRotationAngle() const
{
	if (selectedCCDIndex<0)
		return 0.0;
	CCD *ccd = ccds[selectedCCDIndex];
	if (ccd) return ccd->chipRotAngle();
	else return 0.0;
}

void Oculars::setSelectedCCDPrismPositionAngle(double angle)
{
	if (selectedCCDIndex<0)
		return;

	CCD *ccd = ccds[selectedCCDIndex];
	if (ccd)
	{
		ccd->setPrismPosAngle(angle);
		emit selectedCCDPrismPositionAngleChanged(angle);
	}
}

double Oculars::getSelectedCCDPrismPositionAngle() const
{
	if (selectedCCDIndex<0)
		return 0.0;
	CCD *ccd = ccds[selectedCCDIndex];
	if (ccd) return ccd->prismPosAngle();
	else return 0.0;
}

void Oculars::enableOcular(bool enableOcularMode)
{
	if (enableOcularMode)
	{
		// Close the sensor view if it's displayed
		if (flagShowCCD)
		{
			toggleCCD(false);
			flagShowCCD = false;
			selectedCCDIndex = -1;
		}

		// Close the Telrad sight if it's displayed
		if (flagShowTelrad)
		{
			toggleTelrad(false);
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
	if (!flagShowOculars && !flagShowTelrad && flagRequireSelection && !StelApp::getInstance().getStelObjectMgr().getWasSelected() )
	{
		if (usageMessageLabelID == -1)
		{
			QFontMetrics metrics(font);
			QString labelText = q_("Please select an object before switching to ocular view.");
			StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
			int yPositionOffset = qRound(projectorParams.viewportXywh[3]*projectorParams.viewportCenterOffset[1]);
			int xPosition = qRound(projectorParams.viewportCenter[0] - 0.5 * metrics.boundingRect(labelText).width());
			int yPosition = qRound(projectorParams.viewportCenter[1] - yPositionOffset - 0.5 * metrics.height());
			const char *tcolor = "#99FF99";
			usageMessageLabelID = labelManager->labelScreen(labelText, xPosition, yPosition,
									true, font.pixelSize(), tcolor);
		}
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

	emit enableOcularChanged(flagShowOculars);
}

void Oculars::decrementCCDIndex()
{
	selectedCCDIndex--;
	if (selectedCCDIndex == -1)
	{
		selectedCCDIndex = ccds.count() - 1;
	}
	emit(selectedCCDChanged(selectedCCDIndex));
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

		if (selectedTelescopeIndex == -1)
			selectedTelescopeIndex = 0;
	}
	emit(selectedOcularChanged(selectedOcularIndex));
}

void Oculars::decrementTelescopeIndex()
{
	selectedTelescopeIndex--;
	if (selectedTelescopeIndex == -1)
	{
		selectedTelescopeIndex = telescopes.count() - 1;
	}
	emit(selectedTelescopeChanged(selectedTelescopeIndex));
}

void Oculars::decrementLensIndex()
{
	selectedLensIndex--;
	if (selectedLensIndex == lenses.count())
	{
		selectedLensIndex = -1;
	}
	if (selectedLensIndex == -2)
	{
		selectedLensIndex = lenses.count() - 1;
	}
	emit(selectedLensChanged(selectedLensIndex));
}

void Oculars::rotateReticleClockwise()
{
	// Step: 5 degrees
	reticleRotation -= 5.0;
}

void Oculars::rotateReticleCounterclockwise()
{
	// Step: 5 degrees
	reticleRotation += 5.0;
}

void Oculars::displayPopupMenu()
{
	QMenu * popup = new QMenu(&StelMainView::getInstance());

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
				QAction* action = Q_NULLPTR;
				if (selectedTelescopeIndex != -1 || oculars[index]->isBinoculars())
				{
					action = submenu->addAction(label, [=](){selectOcularAtIndex(index);});
					availableOcularCount++;
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
				QAction* action = submenu->addAction(label, [=](){selectCCDAtIndex(index);});
				if (index == selectedCCDIndex)
				{
					action->setCheckable(true);
					action->setChecked(true);
				}
			}
			popup->addMenu(submenu);
			
			submenu = new QMenu(q_("&Rotate CCD"), popup);
			submenu->addAction(QString("&1: -90") + QChar(0x00B0), [=](){rotateCCD(-90);});
			submenu->addAction(QString("&2: -45") + QChar(0x00B0), [=](){rotateCCD(-45);});
			submenu->addAction(QString("&3: -15") + QChar(0x00B0), [=](){rotateCCD(-15);});
			submenu->addAction(QString("&4: -5") + QChar(0x00B0),  [=](){rotateCCD(-5);});
			submenu->addAction(QString("&5: -1") + QChar(0x00B0),  [=](){rotateCCD(-1);});
			submenu->addAction(QString("&6: +1") + QChar(0x00B0),  [=](){rotateCCD(1);});
			submenu->addAction(QString("&7: +5") + QChar(0x00B0),  [=](){rotateCCD(5);});
			submenu->addAction(QString("&8: +15") + QChar(0x00B0), [=](){rotateCCD(15);});
			submenu->addAction(QString("&9: +45") + QChar(0x00B0), [=](){rotateCCD(45);});
			submenu->addAction(QString("&0: +90") + QChar(0x00B0), [=](){rotateCCD(90);});

			submenu->addAction(q_("&Reset rotation"), this, SLOT(ccdRotationReset()));
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

#if QT_VERSION >= 0x050700 && defined(Q_OS_WIN)
	popup->showTearOffMenu(QCursor::pos());
#endif
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
	emit(selectedCCDChanged(selectedCCDIndex));
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

		if (selectedTelescopeIndex == -1)
			selectTelescopeAtIndex(0);
	}
	emit(selectedOcularChanged(selectedOcularIndex));
}

void Oculars::incrementTelescopeIndex()
{
	selectedTelescopeIndex++;
	if (selectedTelescopeIndex == telescopes.count())
	{
		selectedTelescopeIndex = 0;
	}
	emit(selectedTelescopeChanged(selectedTelescopeIndex));
}

void Oculars::incrementLensIndex()
{
	selectedLensIndex++;
	if (selectedLensIndex == lenses.count())
	{
		selectedLensIndex = -1;
	}
	emit(selectedLensChanged(selectedLensIndex));
}

void Oculars::disableLens()
{
	selectedLensIndex = -1;
	emit(selectedLensChanged(selectedLensIndex));
}

void Oculars::rotateCCD(int amount)
{
	CCD *ccd = ccds[selectedCCDIndex];
	if (!ccd) return;
	double angle = ccd->chipRotAngle();
	angle += amount;
	if (angle >= 360)
	{
		angle -= 360;
	}
	else if (angle <= -360)
	{
		angle += 360;
	}
	ccd->setChipRotAngle(angle);	
}

void Oculars::rotatePrism(int amount)
{
	CCD *ccd = ccds[selectedCCDIndex];
	if (!ccd) return;
	double angle = ccd->prismPosAngle();
	angle += amount;
	if (angle >= 360)
	{
		angle -= 360;
	}
	else if (angle <= -360)
	{
		angle += 360;
	}
	ccd->setPrismPosAngle(angle);
}

void Oculars::selectCCDAtIndex(int index)
{
	if (index > -1 && index < ccds.count())
	{
		selectedCCDIndex = index;
		emit(selectedCCDChanged(index));
	}
}

void Oculars::selectOcularAtIndex(int index)
{
	if (selectedTelescopeIndex == -1)
		selectTelescopeAtIndex(0);

	if (index > -1 && index < oculars.count() && (telescopes.count() >= 0 || oculars[index]->isBinoculars()))
	{
		selectedOcularIndex = index;
		emit(selectedOcularChanged(index));
	}
}

void Oculars::selectTelescopeAtIndex(int index)
{
	if (index > -1 && index < telescopes.count())
	{
		selectedTelescopeIndex = index;
		emit(selectedTelescopeChanged(index));
	}
}

void Oculars::selectLensAtIndex(int index)
{
	if (index > -2 && index < lenses.count())
	{
		selectedLensIndex = index;
		emit(selectedLensChanged(index));
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
		{
			qWarning() << "Oculars plugin: Unable to display a sensor boundary: No sensors or telescopes are defined.";
			QMessageBox::warning(&StelMainView::getInstance(), q_("Warning!"), q_("Unable to display a sensor boundary: No sensors or telescopes are defined."), QMessageBox::Ok);
		}
		flagShowCCD = false;
		selectedCCDIndex = -1;
		show = false;
	}

	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	StelSkyDrawer *skyDrawer = core->getSkyDrawer();
	if (show)
	{
		initialFOV = movementManager->getCurrentFov();
		//Mutually exclusive with the ocular mode
		hideUsageMessageIfDisplayed();
		if (flagShowOculars)
			enableOcular(false);

		if (flagShowTelrad) {
			toggleTelrad(false);
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

		// Change scales for stars. (Even restoring from ocular view has restored main program's values at this point.)
		relativeStarScaleMain=skyDrawer->getRelativeStarScale();
		absoluteStarScaleMain=skyDrawer->getAbsoluteStarScale();
		skyDrawer->setRelativeStarScale(relativeStarScaleCCD);
		skyDrawer->setAbsoluteStarScale(absoluteStarScaleCCD);

		if (guiPanel)
		{
			guiPanel->showCcdGui();
		}
	}
	else
	{
		flagShowCCD = false;

		// Restore star scales
		relativeStarScaleCCD=skyDrawer->getRelativeStarScale();
		absoluteStarScaleCCD=skyDrawer->getAbsoluteStarScale();
		skyDrawer->setRelativeStarScale(relativeStarScaleMain);
		skyDrawer->setAbsoluteStarScale(absoluteStarScaleMain);
		movementManager->setFlagTracking(false);
		//Zoom out
		if (getFlagInitFovUsage())
			movementManager->zoomTo(movementManager->getInitFov());
		else if (!flagShowTelrad)
			movementManager->zoomTo(initialFOV);

		if (getFlagInitDirectionUsage())
			movementManager->setViewDirectionJ2000(core->altAzToJ2000(movementManager->getInitViewingDirection(), StelCore::RefractionOff));

		if (getFlagAutosetMountForCCD())
		{
			StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();
			propMgr->setStelPropertyValue("StelMovementMgr.equatorialMount", equatorialMountEnabledMain);
		}

		if (guiPanel)
		{
			guiPanel->foldGui();
		}
	}

	emit enableCCDChanged(flagShowCCD);
}

void Oculars::toggleCCD()
{
	toggleCCD(!flagShowCCD);
}

void Oculars::toggleCrosshairs(bool show)
{
	if(show != flagShowCrosshairs)
	{
		flagShowCrosshairs = show;
		emit enableCrosshairsChanged(show);
	}
}

void Oculars::toggleTelrad(bool show)
{
	if(show!=flagShowTelrad)
	{
		flagShowTelrad = show;

		StelMovementMgr* movementMgr = StelApp::getInstance().getCore()->getMovementMgr();
		if (show)
		{
			hideUsageMessageIfDisplayed();
			enableOcular(false);
			toggleCCD(false);
			// NOTE: Added special zoom level for Telrad
			if (flagScalingFOVForTelrad)
			{
				float fov = qMax(qMax(telradFOV[0], telradFOV[1]), qMax(telradFOV[2], telradFOV[3]));
				movementMgr->zoomTo(static_cast<double>(fov)*2.);
			}
		}
		else if (getFlagInitFovUsage()) // Restoration of FOV is needed?
			movementMgr->zoomTo(movementMgr->getInitFov());

		if (getFlagInitDirectionUsage())
			movementMgr->setViewDirectionJ2000(StelApp::getInstance().getCore()->altAzToJ2000(movementMgr->getInitViewingDirection(), StelCore::RefractionOff));

		emit enableTelradChanged(flagShowTelrad);
	}
}

void Oculars::toggleTelrad()
{
	toggleTelrad(!flagShowTelrad);
}

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Private Methods
#endif
/* ****************************************************************************************************************** */
void Oculars::initializeActivationActions()
{
	QString ocularsGroup = N_("Oculars");
	actionShowOcular = addAction("actionShow_Ocular", ocularsGroup, N_("Ocular view"), "enableOcular", "Ctrl+O");
	actionMenu = addAction("actionShow_Ocular_Menu", ocularsGroup, N_("Oculars popup menu"), "displayPopupMenu()", "Alt+O");
	actionShowCrosshairs = addAction("actionShow_Ocular_Crosshairs", ocularsGroup, N_("Show crosshairs"), "enableCrosshairs", "Alt+C");
	actionShowSensor = addAction("actionShow_Sensor", ocularsGroup, N_("Image sensor frame"), "enableCCD");
	actionShowTelrad = addAction("actionShow_Telrad", ocularsGroup, N_("Telrad sight"), "enableTelrad", "Ctrl+B");
	actionConfiguration = addAction("actionOpen_Oculars_Configuration", ocularsGroup, N_("Toggle Oculars configuration window"), ocularDialog, "visible", ""); // Allow assign shortkey
	addAction("actionShow_Oculars_GUI", ocularsGroup, N_("Toggle Oculars button bar"), "flagGuiPanelEnabled"); // Allow assign shortkey
	// Select next telescope via keyboard
	addAction("actionShow_Telescope_Increment", ocularsGroup, N_("Select next telescope"), "incrementTelescopeIndex()");
	// Select previous telescope via keyboard
	addAction("actionShow_Telescope_Decrement", ocularsGroup, N_("Select previous telescope"), "decrementTelescopeIndex()");
	// Select next eyepiece via keyboard
	addAction("actionShow_Ocular_Increment", ocularsGroup, N_("Select next eyepiece"), "incrementOcularIndex()");
	// Select previous eyepiece via keyboard
	addAction("actionShow_Ocular_Decrement", ocularsGroup, N_("Select previous eyepiece"), "decrementOcularIndex()");
	addAction("actionShow_Ocular_Rotate_Reticle_Clockwise", ocularsGroup, N_("Rotate reticle pattern of the eyepiece clockwise"), "rotateReticleClockwise()", "Alt+M");
	addAction("actionShow_Ocular_Rotate_Reticle_Counterclockwise", ocularsGroup, N_("Rotate reticle pattern of the eyepiece counterclockwise"), "rotateReticleCounterclockwise()", "Shift+Alt+M");
	addAction("actionShow_Sensor_Crop_Overlay", ocularsGroup, N_("Toggle sensor crop overlay"), "toggleCropOverlay()");
	addAction("actionShow_Sensor_Pixel_Grid", ocularsGroup, N_("Toggle sensor pixel grid"), "togglePixelGrid()");
	addAction("actionShow_Sensor_Focuser_Overlay", ocularsGroup, N_("Toggle focuser overlay"), "toggleFocuserOverlay()");

	connect(this, SIGNAL(selectedCCDChanged(int)),       this, SLOT(instrumentChanged()));	
	connect(this, SIGNAL(selectedOcularChanged(int)),    this, SLOT(instrumentChanged()));
	connect(this, SIGNAL(selectedTelescopeChanged(int)), this, SLOT(instrumentChanged()));	
	connect(this, SIGNAL(selectedLensChanged(int)),      this, SLOT(instrumentChanged()));
}

bool Oculars::isBinocularDefined()
{
	bool binocularFound = false;
	for (auto* ocular : oculars)
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
	int fontSize = StelApp::getInstance().getScreenFontSize();
	StelCore *core = StelApp::getInstance().getCore();
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
	Lens *lens = selectedLensIndex >=0  ? lenses[selectedLensIndex] : Q_NULLPTR;

	const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);
	double screenFOV = static_cast<double>(params.fov);
	Vec2i centerScreen(projector->getViewportPosX() + projector->getViewportWidth() / 2,
			   projector->getViewportPosY() + projector->getViewportHeight() / 2);

	// draw sensor rectangle
	if (selectedCCDIndex > -1 && selectedTelescopeIndex > -1)
	{
		CCD *ccd = ccds[selectedCCDIndex];
		if (ccd)
		{
			StelPainter painter(projector);
			painter.setColor(lineColor);
			painter.setFont(font);
			Telescope *telescope = telescopes[selectedTelescopeIndex];

			const double ccdXRatio = ccd->getActualFOVx(telescope, lens) / screenFOV;
			const double ccdYRatio = ccd->getActualFOVy(telescope, lens) / screenFOV;

			const double fovX = ccd->getActualFOVx(telescope, lens);
			const double fovY = ccd->getActualFOVy(telescope, lens);

			// As the FOV is based on the narrow aspect of the screen, we need to calculate
			// height & width based soley off of that dimension.
			int aspectIndex = 2;
			if (params.viewportXywh[2] > params.viewportXywh[3])
			{
				aspectIndex = 3;
			}
			const float width = params.viewportXywh[aspectIndex] * static_cast<float>(ccdXRatio * params.devicePixelsPerPixel);
			const float height = params.viewportXywh[aspectIndex] * static_cast<float>(ccdYRatio * params.devicePixelsPerPixel);

			// Get Crop size taking into account the binning rounded to the lower limit and limiting it to sensor size
			const float actualCropOverlayX = (std::min(ccd->resolutionX(), ccdCropOverlayHSize) / ccd->binningX()) * ccd->binningX();
			const float actualCropOverlayY = (std::min(ccd->resolutionY(), ccdCropOverlayVSize)  / ccd->binningY()) * ccd->binningY();
			// Calculate the size of the CCD crop overlay
			const float overlayWidth = width * actualCropOverlayX / ccd->resolutionX();
			const float overlayHeight = height * actualCropOverlayY / ccd->resolutionY();

			//calculate the size of a pixel in the image
			float pixelProjectedWidth = width /ccd->resolutionX() * ccd->binningX();
			float pixelProjectedHeight = height /ccd->resolutionY()* ccd->binningY();

			double polarAngle = 0;
			// if the telescope is Equatorial derotate the field
			if (telescope->isEquatorial())
			{
				Vec3d CPos;
				Vector2<qreal> cpos = projector->getViewportCenter();
				projector->unProject(cpos[0], cpos[1], CPos);
				Vec3d CPrel(CPos);
				CPrel[2]*=0.2;
				Vec3d crel;
				projector->project(CPrel, crel);
				polarAngle = atan2(cpos[1] - crel[1], cpos[0] - crel[0]) * (-180.0)/M_PI; // convert to degrees
				if (CPos[2] > 0) polarAngle += 90.0;
				else polarAngle -= 90.0;
			}

			if (getFlagAutosetMountForCCD())
			{
				StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();
				propMgr->setStelPropertyValue("actionSwitch_Equatorial_Mount", telescope->isEquatorial());
				polarAngle = 0;
			}

			if (width > 0.0f && height > 0.0f)
			{
				QPoint a, b;
				QTransform transform = QTransform().translate(centerScreen[0], centerScreen[1]).rotate(-(ccd->chipRotAngle() + polarAngle));
				// bottom line
				a = transform.map(QPoint(static_cast<int>(-width*0.5f), static_cast<int>(-height*0.5f)));
				b = transform.map(QPoint(static_cast<int>(width*0.5f), static_cast<int>(-height*0.5f)));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
				// top line
				a = transform.map(QPoint(static_cast<int>(-width*0.5f), static_cast<int>(height*0.5f)));
				b = transform.map(QPoint(static_cast<int>(width*0.5f), static_cast<int>(height*0.5f)));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
				// left line
				a = transform.map(QPoint(static_cast<int>(-width*0.5f), static_cast<int>(-height*0.5f)));
				b = transform.map(QPoint(static_cast<int>(-width*0.5f), static_cast<int>(height*0.5f)));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
				// right line
				a = transform.map(QPoint(static_cast<int>(width*0.5f), static_cast<int>(height*0.50f)));
				b = transform.map(QPoint(static_cast<int>(width*0.5f), static_cast<int>(-height*0.5f)));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());

				// Tool for showing a resolution box overlay
				if (flagShowCcdCropOverlay)
				{
					// bottom line
					a = transform.map(QPoint(static_cast<int>(-overlayWidth*0.5f), static_cast<int>(-overlayHeight*0.5f)));
					b = transform.map(QPoint(static_cast<int>(overlayWidth*0.5f), static_cast<int>(-overlayHeight*0.5f)));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// top line
					a = transform.map(QPoint(static_cast<int>(-overlayWidth*0.5f), static_cast<int>(overlayHeight*0.5f)));
					b = transform.map(QPoint(static_cast<int>(overlayWidth*0.5f), static_cast<int>(overlayHeight*0.5f)));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// left line
					a = transform.map(QPoint(static_cast<int>(-overlayWidth*0.5f), static_cast<int>(-overlayHeight*0.5f)));
					b = transform.map(QPoint(static_cast<int>(-overlayWidth*0.5f), static_cast<int>(overlayHeight*0.5f)));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// right line
					a = transform.map(QPoint(static_cast<int>(overlayWidth*0.5f), static_cast<int>(overlayHeight*0.5f)));
					b = transform.map(QPoint(static_cast<int>(overlayWidth*0.5f), static_cast<int>(-overlayHeight*0.5f)));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					
					// Tool to show full CCD grid overlay
					if (flagShowCcdCropOverlayPixelGrid)
					{
						// vertical lines
						for (int l =1 ; l< actualCropOverlayX/ccd->binningX(); l++ )
						{
							a = transform.map(QPoint(static_cast<int>(overlayWidth*0.5f- l*pixelProjectedWidth), static_cast<int>(-overlayHeight*0.5f)));
							b = transform.map(QPoint(static_cast<int>(overlayWidth*0.5f- l*pixelProjectedWidth), static_cast<int>(overlayHeight*0.5f)));
							painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
						}
						// horizontal lines
						for (int l =1 ; l< actualCropOverlayY/ccd->binningY(); l++ )
						{
							a = transform.map(QPoint(static_cast<int>(-overlayWidth*0.5f), static_cast<int>(overlayHeight*0.5f - l*pixelProjectedHeight)));
							b = transform.map(QPoint(static_cast<int>(overlayWidth*0.5f), static_cast<int>(overlayHeight*0.5f - l*pixelProjectedHeight)));
							painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
						}
					}
				}
				if(ccd->hasOAG())
				{
					const double InnerOAGRatio = ccd->getInnerOAGRadius(telescope, lens) / screenFOV;
					const double OuterOAGRatio = ccd->getOuterOAGRadius(telescope, lens) / screenFOV;
					const double prismXRatio = ccd->getOAGActualFOVx(telescope, lens) / screenFOV;
					const int in_oag_r = qRound(params.viewportXywh[aspectIndex] * InnerOAGRatio * params.devicePixelsPerPixel);
					const int out_oag_r = qRound(params.viewportXywh[aspectIndex] * OuterOAGRatio * params.devicePixelsPerPixel);
					const int h_width = qRound(params.viewportXywh[aspectIndex] * prismXRatio * params.devicePixelsPerPixel * 0.5);

					painter.drawCircle(centerScreen[0], centerScreen[1], in_oag_r);
					painter.drawCircle(centerScreen[0], centerScreen[1], out_oag_r);

					QTransform oag_transform = QTransform().translate(centerScreen[0], centerScreen[1]).rotate(-(ccd->chipRotAngle() + polarAngle + ccd->prismPosAngle()));

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

				const double ratioLimit = 0.25;
				const double ratioLimitCrop = 0.75;
				if (ccdXRatio>=ratioLimit || ccdYRatio>=ratioLimit)
				{
					// draw cross at center					
					const int cross = qRound(10 * params.devicePixelsPerPixel); // use permanent size of cross (10px)
					a = transform.map(QPoint(-cross, -cross));
					b = transform.map(QPoint(cross, cross));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					a = transform.map(QPoint(-cross, cross));
					b = transform.map(QPoint(cross, -cross));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// calculate coordinates of the center and show it
					Vec3d centerPosition;
					projector->unProject(centerScreen[0], centerScreen[1], centerPosition);
					double cx, cy;
					QString cxt, cyt;
					StelUtils::rectToSphe(&cx,&cy,core->equinoxEquToJ2000(centerPosition, StelCore::RefractionOff)); // Calculate RA/DE (J2000.0) and show it...
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
					float scaleFactor = static_cast<float>(1.2 * params.devicePixelsPerPixel);
					// Coordinates of center of visible field of view for CCD (red rectangle)
					QString coords = QString("%1:").arg(qc_("RA/Dec (J2000.0) of cross", "abbreviated in the plugin"));
					a = transform.map(QPoint(qRound(-width*0.5f), qRound(height*0.5f + 5.f + fontSize*scaleFactor)));
					painter.drawText(a.x(), a.y(), coords, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
					coords = QString("%1/%2").arg(cxt.simplified()).arg(cyt);
					a = transform.map(QPoint(qRound(-width*0.5f), qRound(height*0.5f + 5.f)));
					painter.drawText(a.x(), a.y(), coords, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
					// Dimensions of visible field of view for CCD (red rectangle)
					a = transform.map(QPoint(qRound(-width*0.5f), qRound(-height*0.5f - fontSize*scaleFactor)));
					painter.drawText(a.x(), a.y(), getDimensionsString(fovX, fovY), static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
					// Horizontal and vertical scales of visible field of view for CCD (red rectangle)
					//TRANSLATORS: Unit of measure for scale - arc-seconds per pixel
					QString unit = q_("\"/px");
					QString scales = QString("%1%3 %4 %2%3")
							.arg(QString::number(fovX*3600*ccd->binningX()/ccd->resolutionX(), 'f', 4))
							.arg(QString::number(fovY*3600*ccd->binningY()/ccd->resolutionY(), 'f', 4))
							.arg(unit)
							.arg(QChar(0x00D7));
					a = transform.map(QPoint(qRound(width*0.5f - painter.getFontMetrics().boundingRect(scales).width()*params.devicePixelsPerPixel), qRound(-height*0.5f - fontSize*scaleFactor)));
					painter.drawText(a.x(), a.y(), scales, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
					// Rotation angle of visible field of view for CCD (red rectangle)
					QString angle = QString("%1%2").arg(QString::number(ccd->chipRotAngle(), 'f', 1)).arg(QChar(0x00B0));
					a = transform.map(QPoint(qRound(width*0.5f - painter.getFontMetrics().boundingRect(angle).width()*params.devicePixelsPerPixel), qRound(height*0.5f + 5.f)));
					painter.drawText(a.x(), a.y(), angle, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));

					if(flagShowCcdCropOverlay && (ccdXRatio>=ratioLimitCrop || ccdYRatio>=ratioLimitCrop))
					{
						// show the CCD crop overlay text
						QString resolutionOverlayText = QString("%1%3 %4 %2%3")
								.arg(QString::number(actualCropOverlayX, 'd', 0))
								.arg(QString::number(actualCropOverlayY, 'd', 0))
								.arg(qc_("px", "pixel"))
								.arg(QChar(0x00D7));
						if(actualCropOverlayX!=ccdCropOverlayHSize || actualCropOverlayY!=ccdCropOverlayVSize)
							resolutionOverlayText.append(" [*]");
						a = transform.map(QPoint(qRound(overlayWidth*0.5f - painter.getFontMetrics().boundingRect(resolutionOverlayText).width()*params.devicePixelsPerPixel), qRound(-overlayHeight*0.5f - fontSize*scaleFactor)));
						painter.drawText(a.x(), a.y(), resolutionOverlayText, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
					}
				}

				if (getFlagShowFocuserOverlay())
				{
					painter.setColor(focuserColor);
					if (getFlagUseSmallFocuserOverlay())
						painter.drawCircle(centerScreen[0], centerScreen[1], qRound(params.viewportXywh[aspectIndex] * (0.5*ccd->getFocuserFOV(telescope, lens, 1.25)/ screenFOV) * params.devicePixelsPerPixel));
					if (getFlagUseMediumFocuserOverlay())
						painter.drawCircle(centerScreen[0], centerScreen[1], qRound(params.viewportXywh[aspectIndex] * (0.5*ccd->getFocuserFOV(telescope, lens, 2.)/ screenFOV) * params.devicePixelsPerPixel));
					if (getFlagUseLargeFocuserOverlay())
						painter.drawCircle(centerScreen[0], centerScreen[1], qRound(params.viewportXywh[aspectIndex] * (0.5*ccd->getFocuserFOV(telescope, lens, 3.3)/ screenFOV) * params.devicePixelsPerPixel));
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
	float length = 0.5f * static_cast<float>(params.viewportFovDiameter);
	// See if we need to scale the length
	if (flagScaleImageCircle && oculars[selectedOcularIndex]->apparentFOV() > 0.0 && !oculars[selectedOcularIndex]->isBinoculars())
	{
		length *= static_cast<float>(oculars[selectedOcularIndex]->apparentFOV() / maxEyepieceAngle);
	}
	length *= static_cast<float>(params.devicePixelsPerPixel);
	double polarAngle = 0.;
	if (getFlagAlignCrosshair())
	{
		Vec3d CPos;
		Vector2<qreal> cpos = projector->getViewportCenter();
		projector->unProject(cpos[0], cpos[1], CPos);
		Vec3d CPrel(CPos);
		CPrel[2]*=0.2;
		Vec3d crel;
		projector->project(CPrel, crel);
		polarAngle = atan2(cpos[1] - crel[1], cpos[0] - crel[0]) * (-180.0)/M_PI; // convert to degrees
		if (CPos[2] > 0) polarAngle += 90.0;
		else polarAngle -= 90.0;
	}
	// Draw the lines
	StelPainter painter(projector);
	painter.setColor(lineColor);
	QPoint a, b;
	int hw = qRound(length);
	QTransform ch_transform = QTransform().translate(centerScreen[0], centerScreen[1]).rotate(-polarAngle);
	a = ch_transform.map(QPoint(0, -hw));
	b = ch_transform.map(QPoint(0, hw));
	painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
	a = ch_transform.map(QPoint(-hw, 0));
	b = ch_transform.map(QPoint(hw, 0));
	painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
}

void Oculars::paintTelrad()
{
	if (!flagShowOculars)
	{
		StelCore *core = StelApp::getInstance().getCore();
		const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);		
		// StelPainter drawing
		StelPainter painter(projector);		
		painter.setColor(lineColor);
		Vec2i centerScreen(projector->getViewportPosX()+projector->getViewportWidth()/2,
				   projector->getViewportPosY()+projector->getViewportHeight()/2);
		const float pixelsPerRad = projector->getPixelPerRadAtCenter(); // * params.devicePixelsPerPixel;
		if (telradFOV[0]>0.f) painter.drawCircle(centerScreen[0], centerScreen[1], 0.5f * pixelsPerRad * static_cast<float>(M_PI/180) * (telradFOV[0]));
		if (telradFOV[1]>0.f) painter.drawCircle(centerScreen[0], centerScreen[1], 0.5f * pixelsPerRad * static_cast<float>(M_PI/180) * (telradFOV[1]));
		if (telradFOV[2]>0.f) painter.drawCircle(centerScreen[0], centerScreen[1], 0.5f * pixelsPerRad * static_cast<float>(M_PI/180) * (telradFOV[2]));
		if (telradFOV[3]>0.f) painter.drawCircle(centerScreen[0], centerScreen[1], 0.5f * pixelsPerRad * static_cast<float>(M_PI/180) * (telradFOV[3]));
	}
}

void Oculars::paintOcularMask(const StelCore *core)
{
	if (oculars[selectedOcularIndex]->hasPermanentCrosshair())
		paintCrosshairs();

	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	StelPainter painter(prj);
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();

	double inner = 0.5 * params.viewportFovDiameter * params.devicePixelsPerPixel;
	// See if we need to scale the mask
	if (flagScaleImageCircle && oculars[selectedOcularIndex]->apparentFOV() > 0.0 && !oculars[selectedOcularIndex]->isBinoculars())
	{
		inner = oculars[selectedOcularIndex]->apparentFOV() * inner / maxEyepieceAngle;
	}
	Vec2i centerScreen(prj->getViewportPosX()+prj->getViewportWidth()/2, prj->getViewportPosY()+prj->getViewportHeight()/2);

	painter.setBlending(true);
	// Paint the reticale, if needed
	if (!reticleTexture.isNull())
	{
		painter.setColor(lineColor);
		reticleTexture->bind();
		/* Why it need?
		int textureHeight;
		int textureWidth;
		reticleTexture->getDimensions(textureWidth, textureHeight);
		*/
		painter.drawSprite2dMode(centerScreen[0], centerScreen[1], static_cast<float>(inner / params.devicePixelsPerPixel), static_cast<float>(reticleRotation));
	}

	const float alpha = getFlagUseSemiTransparency() ? getTransparencyMask()*0.01f : 1.f;
	painter.setColor(0.f,0.f,0.f,alpha);

	GLfloat outerRadius = static_cast<GLfloat>(params.viewportXywh[2] * params.devicePixelsPerPixel + params.viewportXywh[3] * params.devicePixelsPerPixel);
	GLint slices = 239;

	GLfloat sinCache[240];
	GLfloat cosCache[240];
	GLfloat vertices[(240+1)*2][3];
	GLfloat deltaRadius;
	GLfloat radiusHigh;

	/* Compute length (needed for normal calculations) */
	deltaRadius=outerRadius-static_cast<GLfloat>(inner);

	/* Cache is the vertex locations cache */
	for (int i=0; i<=slices; i++)
	{
		GLfloat angle=static_cast<GLfloat>(M_PI*2.0)*i/slices;
		sinCache[i]=static_cast<GLfloat>(sinf(angle));
		cosCache[i]=static_cast<GLfloat>(cosf(angle));
	}

	sinCache[slices]=sinCache[0];
	cosCache[slices]=cosCache[0];

	/* Enable arrays */
	painter.enableClientStates(true);
	painter.setVertexPointer(3, GL_FLOAT, vertices);

	radiusHigh=outerRadius-deltaRadius;
	for (int i=0; i<=slices; i++)
	{
		vertices[i*2][0]= centerScreen[0] + outerRadius*sinCache[i];
		vertices[i*2][1]= centerScreen[1] + outerRadius*cosCache[i];
		vertices[i*2][2] = 0.0;
		vertices[i*2+1][0]= centerScreen[0] + radiusHigh*sinCache[i];
		vertices[i*2+1][1]= centerScreen[1] + radiusHigh*cosCache[i];
		vertices[i*2+1][2] = 0.0;
	}
	painter.drawFromArray(StelPainter::TriangleStrip, (slices+1)*2, 0, false);
	painter.enableClientStates(false);

	if (getFlagShowContour())
	{
		painter.setColor(lineColor);
		painter.drawCircle(centerScreen[0], centerScreen[1], static_cast<float>(inner));
	}

	if (getFlagShowCardinals())
	{
		// Compute polar angle for cardinals and show it
		const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);
		Vec3d CPos;
		Vector2<qreal> cpos = projector->getViewportCenter();
		projector->unProject(cpos[0], cpos[1], CPos);
		Vec3d CPrel(CPos);
		CPrel[2]*=0.2;
		Vec3d crel;
		projector->project(CPrel, crel);
		double polarAngle = atan2(cpos[1] - crel[1], cpos[0] - crel[0]) * (-180.0)/M_PI; // convert to degrees
		if (CPos[2] > 0)
			polarAngle += 90.0;
		else
			polarAngle -= 90.0;

		painter.setColor(lineColor);
		if (core->getFlipHorz() && !core->getFlipVert())
			cardinalsMirroredTexture->bind();
		else
			cardinalsNormalTexture->bind();
		painter.drawSprite2dMode(centerScreen[0], centerScreen[1], static_cast<float>(inner / params.devicePixelsPerPixel), static_cast<float>(-polarAngle));		
	}
}

void Oculars::paintText(const StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	StelPainter painter(prj);

	// Get the current instruments
	CCD *ccd = Q_NULLPTR;
	if(selectedCCDIndex != -1)
	{
		ccd = ccds[selectedCCDIndex];
	}
	Ocular *ocular = Q_NULLPTR;
	if(selectedOcularIndex !=-1)
	{
		ocular = oculars[selectedOcularIndex];
	}
	Telescope *telescope = Q_NULLPTR;
	if(selectedTelescopeIndex != -1)
	{
		telescope = telescopes[selectedTelescopeIndex];
	}
	Lens *lens = selectedLens();

	// set up the color and the GL state
	painter.setColor(textColor);
	painter.setBlending(true);

	// Get the X & Y positions, and the line height
	painter.setFont(font);
	QString widthString = "MMMMMMMMMMMMMMMMMMMMM";
	const double insetFromRHS = painter.getFontMetrics().boundingRect(widthString).width();
	StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
	int yPositionOffset = qRound(projectorParams.viewportXywh[3]*projectorParams.viewportCenterOffset[1]);
	int xPosition = qRound(projectorParams.devicePixelsPerPixel*projectorParams.viewportXywh[2] - insetFromRHS);
	int yPosition = qRound(projectorParams.devicePixelsPerPixel*projectorParams.viewportXywh[3] - yPositionOffset - 20);
	const int lineHeight = painter.getFontMetrics().height();

	// The Ocular
	if (flagShowOculars && ocular!=Q_NULLPTR)
	{
		QString ocularNumberLabel;
		QString name = ocular->name();
		QString ocularI18n = q_("Ocular");
		if (ocular->isBinoculars())
			ocularI18n = q_("Binocular");
		if (name.isEmpty())
		{
			ocularNumberLabel = QString("%1 #%2").arg(ocularI18n).arg(selectedOcularIndex);
		}
		else
		{
			ocularNumberLabel = QString("%1 #%2: %3").arg(ocularI18n).arg(selectedOcularIndex).arg(name);
		}
		// The name of the ocular could be really long.
		if (name.length() > widthString.length())
		{
			xPosition -= qRound(insetFromRHS*0.5);
		}
		painter.drawText(xPosition, yPosition, ocularNumberLabel);
		yPosition-=lineHeight;
		
		if (!ocular->isBinoculars())
		{
			// TRANSLATORS: FL = Focal length
			QString eFocalLengthLabel = QString(q_("Ocular FL: %1 mm")).arg(QString::number(ocular->effectiveFocalLength(), 'f', 1));
			painter.drawText(xPosition, yPosition, eFocalLengthLabel);
			yPosition-=lineHeight;
			
			QString ocularFov = QString::number(ocular->apparentFOV(), 'f', 2);
			ocularFov.append(QChar(0x00B0));//Degree sign
			// TRANSLATORS: aFOV = apparent field of view
			QString ocularFOVLabel = QString(q_("Ocular aFOV: %1")).arg(ocularFov);
			painter.drawText(xPosition, yPosition, ocularFOVLabel);
			yPosition-=lineHeight;

			QString lensNumberLabel;
			// Barlow and Shapley lens
			if (lens != Q_NULLPTR) // it's null if lens is not selected (lens index = -1)
			{
				QString lensName = lens->getName();
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

			if (telescope!=Q_NULLPTR)
			{
				QString telescopeName = telescope->name();
				QString telescopeString = "";

				if (telescopeName.isEmpty())
					telescopeString = QString("%1").arg(selectedTelescopeIndex);
				else
					telescopeString = QString("%1: %2").arg(selectedTelescopeIndex).arg(telescopeName);

				painter.drawText(xPosition, yPosition, QString(q_("Telescope #%1")).arg(telescopeString));
				yPosition-=lineHeight;

				// General info
				double mag = ocular->magnification(telescope, lens);
				QString magString = QString::number(mag, 'f', 1);
				magString.append(QChar(0x02E3)); // Was 0x00D7
				magString.append(QString(" (%1D)").arg(QString::number(mag/telescope->diameter(), 'f', 2)));

				painter.drawText(xPosition, yPosition, QString(q_("Magnification: %1")).arg(magString));
				yPosition-=lineHeight;

				if (mag>0)
				{
					QString exitPupil = QString::number(telescope->diameter()/mag, 'f', 2);

					painter.drawText(xPosition, yPosition, QString(q_("Exit pupil: %1 mm")).arg(exitPupil));
					yPosition-=lineHeight;
				}

				QString fovString = QString::number(ocular->actualFOV(telescope, lens), 'f', 5);
				fovString.append(QChar(0x00B0));//Degree sign

				painter.drawText(xPosition, yPosition, QString(q_("FOV: %1")).arg(fovString));
			}
		}
	}

	// The CCD
	if (flagShowCCD && ccd!=Q_NULLPTR)
	{
		QString ccdSensorLabel, ccdInfoLabel, ccdBinningInfo;
		QString name = "";
		QString telescopeName = "";
		double fovX = 0.0;
		double fovY = 0.0;
		if (telescope!=Q_NULLPTR)
		{
			fovX = ccd->getActualFOVx(telescope, lens);
			fovY = ccd->getActualFOVy(telescope, lens);
			name = ccd->name();
			telescopeName = telescope->name();
		}

		ccdInfoLabel = QString(q_("Dimensions: %1")).arg(getDimensionsString(fovX, fovY));
		ccdBinningInfo = QString("%1: %2 %4 %3").arg(q_("Binning")).arg(ccd->binningX()).arg(ccd->binningY()).arg(QChar(0x00D7));
		
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
		painter.drawText(xPosition, yPosition, ccdBinningInfo);
		yPosition-=lineHeight;
		painter.drawText(xPosition, yPosition, telescopeNumberLabel);
	}
}

void Oculars::validateAndLoadIniFile()
{
	// Ensure the module directory exists
	StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Oculars");
	StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
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
			qDebug() << "Oculars::validateAndLoadIniFile() copied default_ocular.ini to " << QDir::toNativeSeparators(ocularIniPath);
			// The resource is read only, and the new file inherits this, so set write-able.
			QFile dest(ocularIniPath);
			dest.setPermissions(dest.permissions() | QFile::WriteOwner);
		}
	}
	else
	{
		qDebug() << "Oculars::validateAndLoadIniFile() ocular.ini exists at: " << QDir::toNativeSeparators(ocularIniPath) << ". Checking version...";
		QSettings mySettings(ocularIniPath, QSettings::IniFormat);
		const float ocularsVersion = mySettings.value("oculars_version", 0.0).toFloat();
		qWarning() << "Oculars::validateAndLoadIniFile() found existing ini file version " << ocularsVersion;

		if (ocularsVersion < MIN_OCULARS_INI_VERSION)
		{
			qWarning() << "Oculars::validateAndLoadIniFile() existing ini file version " << ocularsVersion
				   << " too old to use; required version is " << MIN_OCULARS_INI_VERSION << ". Copying over new one.";
			// delete last "old" file, if it exists
			QFile deleteFile(ocularIniPath + ".old");
			deleteFile.remove();

			// Rename the old one, and copy over a new one
			QFile oldFile(ocularIniPath);
			if (!oldFile.rename(ocularIniPath + ".old"))
			{
				qWarning() << "Oculars::validateAndLoadIniFile() cannot move ocular.ini resource to ocular.ini.old at path  " + QDir::toNativeSeparators(ocularIniPath);
			}
			else
			{
				qWarning() << "Oculars::validateAndLoadIniFile() ocular.ini resource renamed to ocular.ini.old at path  " + QDir::toNativeSeparators(ocularIniPath);
				QFile src(":/ocular/default_ocular.ini");
				if (!src.copy(ocularIniPath))
				{
					qWarning() << "Oculars::validateIniFile cannot copy default_ocular.ini resource to [non-existing] " + QDir::toNativeSeparators(ocularIniPath);
				}
				else
				{
					qDebug() << "Oculars::validateAndLoadIniFile() copied default_ocular.ini to " << QDir::toNativeSeparators(ocularIniPath);
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
	Q_ASSERT(flagShowOculars == false);
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();	
	StelSkyDrawer *skyDrawer = core->getSkyDrawer();

	if (flagHideGridsLines)
		toggleLines(true);

	StelApp::getInstance().getStelPropertyManager()->setStelPropertyValue("MilkyWay.saturation", milkyWaySaturation);
	disconnect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double)));
	// restore values, but keep current to enable toggling.
	if (!getFlagAutoLimitMagnitude())
	{
		flagLimitStarsOculars=skyDrawer->getFlagStarMagnitudeLimit();
		magLimitStarsOculars=skyDrawer->getCustomStarMagnitudeLimit();
	}
	skyDrawer->setCustomStarMagnitudeLimit(magLimitStarsMain);
	skyDrawer->setFlagStarMagnitudeLimit(flagLimitStarsMain);
	relativeStarScaleOculars=skyDrawer->getRelativeStarScale();
	absoluteStarScaleOculars=skyDrawer->getAbsoluteStarScale();
	skyDrawer->setRelativeStarScale(relativeStarScaleMain);
	skyDrawer->setAbsoluteStarScale(absoluteStarScaleMain);
	skyDrawer->setFlagLuminanceAdaptation(flagAdaptationMain);
	skyDrawer->setFlagPlanetMagnitudeLimit(flagLimitPlanetsMain);
	skyDrawer->setFlagNebulaMagnitudeLimit(flagLimitDSOsMain);
	skyDrawer->setCustomPlanetMagnitudeLimit(magLimitPlanetsMain);
	skyDrawer->setCustomNebulaMagnitudeLimit(magLimitDSOsMain);
	movementManager->setFlagTracking(false);
	movementManager->setFlagEnableZoomKeys(true);
	movementManager->setFlagEnableMouseNavigation(true);

	GETSTELMODULE(SolarSystem)->setFlagMoonScale(flagMoonScaleMain);
	GETSTELMODULE(SolarSystem)->setFlagMinorBodyScale(flagMinorBodiesScaleMain);

	// Set the screen display
	core->setFlipHorz(flipHorzMain);
	core->setFlipVert(flipVertMain);

	if (getFlagInitFovUsage())
		movementManager->zoomTo(movementManager->getInitFov());
	else if (!flagShowTelrad)
		movementManager->zoomTo(initialFOV);

	if (getFlagInitDirectionUsage())
		movementManager->setViewDirectionJ2000(core->altAzToJ2000(movementManager->getInitViewingDirection(), StelCore::RefractionOff));
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
			StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();

			if (flagHideGridsLines)
			{
				// Store current state for later resetting
				flagGridLinesDisplayedMain	= propMgr->getStelPropertyValue("GridLinesMgr.gridlinesDisplayed").toBool();
				flagCardinalPointsMain		= propMgr->getStelPropertyValue("LandscapeMgr.cardinalsPointsDisplayed").toBool();
				flagConstellationLinesMain	= propMgr->getStelPropertyValue("ConstellationMgr.linesDisplayed").toBool();
				flagConstellationBoundariesMain	= propMgr->getStelPropertyValue("ConstellationMgr.boundariesDisplayed").toBool();
				flagAsterismLinesMain		= propMgr->getStelPropertyValue("AsterismMgr.linesDisplayed").toBool();
				flagRayHelpersLinesMain		= propMgr->getStelPropertyValue("AsterismMgr.rayHelpersDisplayed").toBool();
			}

			StelSkyDrawer *skyDrawer = core->getSkyDrawer();
			// Current state
			flagAdaptationMain	= skyDrawer->getFlagLuminanceAdaptation();
			flagLimitStarsMain	= skyDrawer->getFlagStarMagnitudeLimit();
			flagLimitPlanetsMain	= skyDrawer->getFlagPlanetMagnitudeLimit();
			flagLimitDSOsMain	= skyDrawer->getFlagNebulaMagnitudeLimit();
			magLimitStarsMain	= skyDrawer->getCustomStarMagnitudeLimit();
			magLimitPlanetsMain	= skyDrawer->getCustomPlanetMagnitudeLimit();
			magLimitDSOsMain	= skyDrawer->getCustomNebulaMagnitudeLimit();
			relativeStarScaleMain	= skyDrawer->getRelativeStarScale();
			absoluteStarScaleMain	= skyDrawer->getAbsoluteStarScale();

			flagMoonScaleMain		= propMgr->getStelPropertyValue("SolarSystem.flagMoonScale").toBool();
			flagMinorBodiesScaleMain	= propMgr->getStelPropertyValue("SolarSystem.flagMinorBodyScale").toBool();

			milkyWaySaturation	= propMgr->getStelPropertyValue("MilkyWay.saturation").toDouble();

			flipHorzMain = core->getFlipHorz();
			flipVertMain = core->getFlipVert();

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

void Oculars::toggleLines(bool visible)
{
	if (flagShowTelrad)
		return;

	StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();

	if (visible)
	{
		propMgr->setStelPropertyValue("GridLinesMgr.gridlinesDisplayed", flagGridLinesDisplayedMain);
		propMgr->setStelPropertyValue("LandscapeMgr.cardinalsPointsDisplayed", flagCardinalPointsMain);
		propMgr->setStelPropertyValue("ConstellationMgr.linesDisplayed", flagConstellationLinesMain);
		propMgr->setStelPropertyValue("ConstellationMgr.boundariesDisplayed", flagConstellationBoundariesMain);
		propMgr->setStelPropertyValue("AsterismMgr.linesDisplayed", flagAsterismLinesMain);
		propMgr->setStelPropertyValue("AsterismMgr.rayHelpersDisplayed", flagRayHelpersLinesMain);
	}
	else
	{
		propMgr->setStelPropertyValue("GridLinesMgr.gridlinesDisplayed", false);
		propMgr->setStelPropertyValue("LandscapeMgr.cardinalsPointsDisplayed", false);
		propMgr->setStelPropertyValue("ConstellationMgr.linesDisplayed", false);
		propMgr->setStelPropertyValue("ConstellationMgr.boundariesDisplayed", false);
		propMgr->setStelPropertyValue("AsterismMgr.linesDisplayed", false);
		propMgr->setStelPropertyValue("AsterismMgr.rayHelpersDisplayed", false);
	}
}

void Oculars::zoomOcular()
{
	Q_ASSERT(flagShowOculars == true);
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	StelSkyDrawer *skyDrawer = core->getSkyDrawer();

	if (flagHideGridsLines)
		toggleLines(false);

	skyDrawer->setFlagLuminanceAdaptation(false);
	StelApp::getInstance().getStelPropertyManager()->setStelPropertyValue("MilkyWay.saturation", 0.f);

	GETSTELMODULE(SolarSystem)->setFlagMoonScale(false);
	GETSTELMODULE(SolarSystem)->setFlagMinorBodyScale(false);

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
	Ocular * ocular = oculars[selectedOcularIndex];
	Telescope * telescope = Q_NULLPTR;
	Lens * lens = Q_NULLPTR;
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
			lens = lenses[selectedLensIndex];
		}
		telescope = telescopes[selectedTelescopeIndex];
		core->setFlipHorz(telescope->isHFlipped());
		core->setFlipVert(telescope->isVFlipped());
	}

	// Change relative and absolute scales for stars
	relativeStarScaleMain=skyDrawer->getRelativeStarScale();
	skyDrawer->setRelativeStarScale(relativeStarScaleOculars);
	absoluteStarScaleMain=skyDrawer->getAbsoluteStarScale();
	skyDrawer->setAbsoluteStarScale(absoluteStarScaleOculars);

	// Limit stars and DSOs	magnitude. Either compute limiting magnitude for the telescope/ocular,
	// or just use the custom oculars mode value.

	// TODO: set lim. mag without also activating flag it if it should not be activated.

	double limitMag=magLimitStarsOculars;
	if (getFlagAutoLimitMagnitude() || flagLimitStarsOculars )
	{
		if (getFlagAutoLimitMagnitude())
		{
			disconnect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double))); // we want to keep the old manual value.
			limitMag = computeLimitMagnitude(ocular, telescope);
			// TODO: Is it really good to apply the star formula to DSO?
			skyDrawer->setFlagNebulaMagnitudeLimit(true);
			skyDrawer->setCustomNebulaMagnitudeLimit(limitMag);
		}
		else
		{	// It's possible that the user changes the custom magnitude while viewing, and then changes the ocular.
			// Therefore we need a temporary connection.
			connect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double)));
		}
		skyDrawer->setFlagStarMagnitudeLimit(true);
	}
	skyDrawer->setCustomStarMagnitudeLimit(limitMag);

	actualFOV = ocular->actualFOV(telescope, lens);
	// See if the mask was scaled; if so, correct the actualFOV.
	if (flagScaleImageCircle && ocular->apparentFOV() > 0.0 && !ocular->isBinoculars())
	{
		actualFOV = maxEyepieceAngle * actualFOV / ocular->apparentFOV();
	}
	movementManager->zoomTo(actualFOV, 0.f);
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
	if (selectedLensIndex >= 0 && selectedLensIndex < lenses.count())
	{
		return lenses[selectedLensIndex];
	}
	return Q_NULLPTR;
}

QMenu* Oculars::addLensSubmenu(QMenu* parent)
{
	Q_ASSERT(parent);

	QMenu *submenu = new QMenu(q_("&Lens"), parent);
	submenu->addAction(q_("&Previous lens"), this, SLOT(decrementLensIndex()));
	submenu->addAction(q_("&Next lens"), this, SLOT(incrementLensIndex()));
	submenu->addSeparator();
	submenu->addAction(q_("None"), this, SLOT(disableLens()));

	for (int index = 0; index < lenses.count(); ++index)
	{
		QString label;
		if (index < 10)
		{
			label = QString("&%1: %2").arg(index).arg(lenses[index]->getName());
		}
		else
		{
			label = lenses[index]->getName();
		}
		QAction* action = submenu->addAction(label, [=](){selectLensAtIndex(index);});
		if (index == selectedLensIndex)
		{
			action->setCheckable(true);
			action->setChecked(true);
		}
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
		QAction* action = submenu->addAction(label, [=](){selectTelescopeAtIndex(index);});
		if (index == selectedTelescopeIndex)
		{
			action->setCheckable(true);
			action->setChecked(true);
		}
	}

	return submenu;
}

void Oculars::setFlagDMSDegrees(const bool b)
{
	flagDMSDegrees = b;
	settings->setValue("use_decimal_degrees", !b);
	settings->sync();
	emit flagDMSDegreesChanged(b);
}

bool Oculars::getFlagDMSDegrees() const
{
	return flagDMSDegrees;
}

void Oculars::setFlagRequireSelection(const bool b)
{
	flagRequireSelection = b;
	settings->setValue("require_selection_to_zoom", b);
	settings->sync();
	emit flagRequireSelectionChanged(b);
}

bool Oculars::getFlagRequireSelection() const
{
	return flagRequireSelection;
}

void Oculars::setFlagAutoLimitMagnitude(const bool b)
{
	flagAutoLimitMagnitude = b;
	settings->setValue("autolimit_stellar_magnitude", b);
	settings->sync();
	emit flagAutoLimitMagnitudeChanged(b);
}

bool Oculars::getFlagAutoLimitMagnitude() const
{
	return flagAutoLimitMagnitude;
}

void Oculars::setMagLimitStarsOcularsManual(double mag)
{
	magLimitStarsOculars = mag;
	settings->setValue("limit_stellar_magnitude_oculars_val", mag);
	settings->sync();
	// This is no property, no need to emit a signal.
}

double Oculars::getMagLimitStarsOcularsManual() const
{
	return magLimitStarsOculars;
}

void Oculars::setFlagInitFovUsage(const bool b)
{
	flagInitFOVUsage = b;
	settings->setValue("use_initial_fov", b);
	settings->sync();
	emit flagInitFOVUsageChanged(b);
}

bool Oculars::getFlagInitFovUsage() const
{
	return flagInitFOVUsage;
}

void Oculars::setFlagInitDirectionUsage(const bool b)
{
	flagInitDirectionUsage = b;
	settings->setValue("use_initial_direction", b);
	settings->sync();
	emit flagInitDirectionUsageChanged(b);
}

bool Oculars::getFlagInitDirectionUsage() const
{
	return flagInitDirectionUsage;
}

void Oculars::setFlagAutosetMountForCCD(const bool b)
{
	flagAutosetMountForCCD = b;
	settings->setValue("use_mount_autoset", b);
	settings->sync();

	if (!b)
	{
		StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();
		propMgr->setStelPropertyValue("StelMovementMgr.equatorialMount", equatorialMountEnabledMain);
	}
	emit flagAutosetMountForCCDChanged(b);
}

bool Oculars::getFlagAutosetMountForCCD() const
{
	return  flagAutosetMountForCCD;
}

void Oculars::setFlagScalingFOVForTelrad(const bool b)
{
	flagScalingFOVForTelrad = b;
	settings->setValue("use_telrad_fov_scaling", b);
	settings->sync();
	emit flagScalingFOVForTelradChanged(b);
}

bool Oculars::getFlagScalingFOVForTelrad() const
{
	return  flagScalingFOVForTelrad;
}

void Oculars::setTelradFOV(Vec4f fov)
{
	telradFOV = fov;
	settings->setValue("telrad_fov", fov.toStr());
	settings->sync();
	emit telradFOVChanged(fov);
}

Vec4f Oculars::getTelradFOV() const
{
	return  telradFOV;
}

void Oculars::setFlagScalingFOVForCCD(const bool b)
{
	flagScalingFOVForCCD = b;
	settings->setValue("use_ccd_fov_scaling", b);
	settings->sync();
	emit flagScalingFOVForCCDChanged(b);
}

bool Oculars::getFlagScalingFOVForCCD() const
{
	return  flagScalingFOVForCCD;
}

void Oculars::setFlagUseSemiTransparency(const bool b)
{
	flagSemiTransparency = b;
	settings->setValue("use_semi_transparency", b);
	settings->sync();
	emit flagUseSemiTransparencyChanged(b);
}

bool Oculars::getFlagUseSemiTransparency() const
{
	return flagSemiTransparency;
}

void Oculars::setTransparencyMask(const int v)
{
	transparencyMask = v;
	settings->setValue("transparency_mask", v);
	settings->sync();
	emit transparencyMaskChanged(v);
}

int Oculars::getTransparencyMask() const
{
	return transparencyMask;
}

void Oculars::setFlagShowResolutionCriteria(const bool b)
{
	flagShowResolutionCriteria = b;
	settings->setValue("show_resolution_criteria", b);
	settings->sync();
	emit flagShowResolutionCriteriaChanged(b);
}

bool Oculars::getFlagShowResolutionCriteria() const
{
	return flagShowResolutionCriteria;
}

void Oculars::setCcdCropOverlayHSize(int size) {
	ccdCropOverlayHSize = size;
	settings->setValue("ccd_crop_overlay_hsize", size);
	settings->sync();
	emit ccdCropOverlayHSizeChanged(size);
}

void Oculars::setCcdCropOverlayVSize(int size) {
	ccdCropOverlayVSize = size;
	settings->setValue("ccd_crop_overlay_vsize", size);
	settings->sync();
	emit ccdCropOverlayVSizeChanged(size);
}

void Oculars::setFlagShowCcdCropOverlay(const bool b)
{
	flagShowCcdCropOverlay = b;
	settings->setValue("show_ccd_crop_overlay", b);
	settings->sync();
	emit flagShowCcdCropOverlayChanged(b);
}

bool Oculars::getFlagShowCcdCropOverlay(void) const
{
	return flagShowCcdCropOverlay;
}

void Oculars::setFlagShowCcdCropOverlayPixelGrid(const bool b)
{
	flagShowCcdCropOverlayPixelGrid = b;
	settings->setValue("ccd_crop_overlay_pixel_grid", b);
	settings->sync();
	emit flagShowCcdCropOverlayPixelGridChanged(b);
}

bool Oculars::getFlagShowCcdCropOverlayPixelGrid(void) const
{
	return flagShowCcdCropOverlayPixelGrid;
}


void Oculars::setFlagShowFocuserOverlay(const bool b)
{
	flagShowFocuserOverlay = b;
	settings->setValue("show_focuser_overlay", b);
	settings->sync();
	emit flagShowFocuserOverlayChanged(b);
}

bool Oculars::getFlagShowFocuserOverlay(void) const
{
	return flagShowFocuserOverlay;
}

void Oculars::setFlagUseSmallFocuserOverlay(const bool b)
{
	flagUseSmallFocuserOverlay = b;
	settings->setValue("use_small_focuser_overlay", b);
	settings->sync();
	emit flagUseSmallFocuserOverlayChanged(b);
}

bool Oculars::getFlagUseSmallFocuserOverlay(void) const
{
	return flagUseSmallFocuserOverlay;
}

void Oculars::setFlagUseMediumFocuserOverlay(const bool b)
{
	flagUseMediumFocuserOverlay = b;
	settings->setValue("use_medium_focuser_overlay", b);
	settings->sync();
	emit flagUseMediumFocuserOverlayChanged(b);
}

bool Oculars::getFlagUseMediumFocuserOverlay(void) const
{
	return flagUseMediumFocuserOverlay;
}

void Oculars::setFlagUseLargeFocuserOverlay(const bool b)
{
	flagUseLargeFocuserOverlay = b;
	settings->setValue("use_large_focuser_overlay", b);
	settings->sync();
	emit flagUseLargeFocuserOverlayChanged(b);
}

bool Oculars::getFlagUseLargeFocuserOverlay(void) const
{
	return flagUseLargeFocuserOverlay;
}

void Oculars::setFlagShowContour(const bool b)
{
	flagShowContour = b;
	settings->setValue("show_ocular_contour", b);
	settings->sync();
	emit flagShowContourChanged(b);
}

bool Oculars::getFlagShowContour(void) const
{
	return flagShowContour;
}

void Oculars::setFlagShowCardinals(const bool b)
{
	flagShowCardinals = b;
	settings->setValue("show_ocular_cardinals", b);
	settings->sync();
	emit flagShowCardinalsChanged(b);
}

bool Oculars::getFlagShowCardinals(void) const
{
	return flagShowCardinals;
}

void Oculars::setFlagAlignCrosshair(const bool b)
{
	flagAlignCrosshair = b;
	settings->setValue("align_crosshair", b);
	settings->sync();
	emit flagAlignCrosshairChanged(b);
}

bool Oculars::getFlagAlignCrosshair(void) const
{
	return flagAlignCrosshair;
}

void Oculars::setArrowButtonScale(const int val)
{
	arrowButtonScale = val;
	settings->setValue("arrow_scale", val);
	settings->sync();
	emit arrowButtonScaleChanged(val);
}

int Oculars::getArrowButtonScale() const
{
	return arrowButtonScale;
}

void Oculars::setFlagHideGridsLines(const bool b)
{
	if (b != flagHideGridsLines)
	{
		flagHideGridsLines = b;
		settings->setValue("hide_grids_and_lines", b);
		settings->sync();
		emit flagHideGridsLinesChanged(b);

		if (b && flagShowOculars)
		{
			// Store current state for later resetting
			StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();
			flagGridLinesDisplayedMain	= propMgr->getStelPropertyValue("GridLinesMgr.gridlinesDisplayed").toBool();
			flagCardinalPointsMain		= propMgr->getStelPropertyValue("LandscapeMgr.cardinalsPointsDisplayed").toBool();
			flagConstellationLinesMain	= propMgr->getStelPropertyValue("ConstellationMgr.linesDisplayed").toBool();
			flagConstellationBoundariesMain	= propMgr->getStelPropertyValue("ConstellationMgr.boundariesDisplayed").toBool();
			flagAsterismLinesMain		= propMgr->getStelPropertyValue("AsterismMgr.linesDisplayed").toBool();
			flagRayHelpersLinesMain		= propMgr->getStelPropertyValue("AsterismMgr.rayHelpersDisplayed").toBool();
			toggleLines(false);
		}
		else if (!b && flagShowOculars)
		{
			// Restore main program state
			toggleLines(true);
		}
	}
}

bool Oculars::getFlagHideGridsLines() const
{
	return flagHideGridsLines;
}

QString Oculars::getDimensionsString(double fovX, double fovY) const
{
	QString stringFovX, stringFovY;
	if (getFlagDMSDegrees())
	{
		if (fovX >= 1.0)
		{
			int degrees = static_cast<int>(fovX);
			double minutes = (fovX - degrees) * 60.;
			stringFovX = QString::number(degrees) + QChar(0x00B0) + QString::number(minutes, 'f', 2) + QChar(0x2032);
		}
		else
		{
			double minutes = fovX * 60.;
			stringFovX = QString::number(minutes, 'f', 2) + QChar(0x2032);
		}

		if (fovY >= 1.0)
		{
			int degrees = static_cast<int>(fovY);
			double minutes = (fovY - degrees) * 60.;
			stringFovY = QString::number(degrees) + QChar(0x00B0) + QString::number(minutes, 'f', 2) + QChar(0x2032);
		}
		else
		{
			double minutes = fovY * 60;
			stringFovY = QString::number(minutes, 'f', 2) + QChar(0x2032);
		}
	}
	else
	{
		stringFovX = QString::number(fovX, 'f', 5) + QChar(0x00B0);
		stringFovY = QString::number(fovY, 'f', 5) + QChar(0x00B0);
	}

	return stringFovX + QChar(0x00D7) + stringFovY;
}

// Define whether the button toggling eyepieces should be visible
void Oculars::setFlagShowOcularsButton(bool b)
{
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		if (b==true) {
			if (toolbarButton==Q_NULLPTR) {
				// Create the pulsars button
				toolbarButton = new StelButton(Q_NULLPTR, *pxmapOnIcon, *pxmapOffIcon, *pxmapGlow, "actionShow_Ocular");
			}
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		} else {
			gui->getButtonBar()->hideButton("actionShow_Ocular");
		}
	}
	flagShowOcularsButton = b;
	settings->setValue("show_toolbar_button", b);
	settings->sync();

	emit flagShowOcularsButtonChanged(b);
}


void Oculars::setGuiPanelFontSize(int size)
{
	// This forces a redraw of the panel.
	if (size!=guiPanelFontSize)
	{
		bool guiPanelVisible=guiPanel;
		if (guiPanelVisible)
			enableGuiPanel(false);
		guiPanelFontSize=size;
		if (guiPanelVisible)
			enableGuiPanel(true);

		settings->setValue("gui_panel_fontsize", size);
		settings->sync();
		emit guiPanelFontSizeChanged(size);
	}
}

void Oculars::toggleCropOverlay()
{
	setFlagShowCcdCropOverlay(!getFlagShowCcdCropOverlay());
}

void Oculars::togglePixelGrid()
{
	setFlagShowCcdCropOverlayPixelGrid(!getFlagShowCcdCropOverlayPixelGrid());
}

void Oculars::toggleFocuserOverlay()
{
	setFlagShowFocuserOverlay(!getFlagShowFocuserOverlay());
}

double Oculars::computeLimitMagnitude(Ocular *ocular, Telescope *telescope)
{
	// Simplified calculation of the penetrating power of the telescope
	double diameter = 0.;
	if (ocular->isBinoculars())
		diameter = ocular->fieldStop();
	else
		diameter = telescope!=Q_NULLPTR ? telescope->diameter() : 0.1; // Avoid a potential call of null pointer, and a log(0) error.

	// A better formula for telescopic limiting magnitudes?
	// North, G.; Journal of the British Astronomical Association, vol.107, no.2, p.82
	// http://adsabs.harvard.edu/abs/1997JBAA..107...82N
	return 4.5 + 4.4*std::log10(diameter);
}

void Oculars::handleAutoLimitToggle(bool on)
{
	if (!flagShowOculars)
		return;

	// When we are in Oculars mode, we must toggle between the auto limit and manual limit. Logic taken from zoomOcular()/unzoomOcular()
	StelCore *core = StelApp::getInstance().getCore();
	StelSkyDrawer *skyDrawer = core->getSkyDrawer();
	if (on)
	{
		Ocular * ocular = oculars[selectedOcularIndex];
		Telescope * telescope = Q_NULLPTR;
		if (!ocular->isBinoculars())
		{
			telescope = telescopes[selectedTelescopeIndex];
		}
		disconnect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double))); // keep the old manual value in config.
		double limitMag = computeLimitMagnitude(ocular, telescope);
		// TODO: Is it really good to apply the star formula to DSO?
		skyDrawer->setFlagNebulaMagnitudeLimit(true);
		skyDrawer->setCustomNebulaMagnitudeLimit(limitMag);
		skyDrawer->setFlagStarMagnitudeLimit(true);
		skyDrawer->setCustomStarMagnitudeLimit(limitMag);
	}
	else
	{
		connect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double)));
		skyDrawer->setCustomStarMagnitudeLimit(magLimitStarsOculars);
		skyDrawer->setFlagStarMagnitudeLimit(flagLimitStarsOculars);
	}
}

// Handle switching the main program's star limitation flag
void Oculars::handleStarMagLimitToggle(bool on)
{
	if (!flagShowOculars)
		return;

	flagLimitStarsOculars=on;
	// It only makes sense to switch off the auto-limit when we switch off the limit.
	if (!on)
	{
		setFlagAutoLimitMagnitude(false);
	}
}
