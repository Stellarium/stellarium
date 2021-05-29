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

#include "ConstellationMgr.hpp"
#include "LabelMgr.hpp"
#include "LandscapeMgr.hpp"
#include "MilkyWay.hpp"
#include "SkyGui.hpp"
#include "SolarSystem.hpp"
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
#include "StelPropertyMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

#include <QAction>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>

#include <array>
#include <cmath>
#include <qglobal.h>
#include <stdexcept>

extern void        qt_set_sequence_auto_mnemonic(bool b);

static QSettings * settings; //!< The settings as read in from the ini file.

/* ****************************************************************************************************************** */
// MARK: - StelModuleMgr Methods
/* ****************************************************************************************************************** */
//! This method is the one called automatically by the StelModuleMgr just
//! after loading the dynamic library
auto               OcularsStelPluginInterface::getStelModule() const -> StelModule *
{
	return new Oculars();
}

auto OcularsStelPluginInterface::getPluginInfo() const -> StelPluginInfo
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(Oculars);

	StelPluginInfo info;
	info.id            = "Oculars";
	info.displayedName = N_("Oculars");
	info.authors       = "Timothy Reaves";
	info.contact       = "https://github.com/Stellarium/stellarium";
	info.description   = N_("Shows the sky as if looking through a telescope eyepiece. (Only magnification and field of "
                         "view are simulated.) It can also show a sensor frame and a Telrad sight.");
	info.version       = OCULARS_PLUGIN_VERSION;
	info.license       = OCULARS_PLUGIN_LICENSE;
	return info;
}

/* ****************************************************************************************************************** */
// MARK: - CTORS & DTORS
/* ****************************************************************************************************************** */
Oculars::Oculars()
	: m_alignCrosshair(false)
	, m_arrowButtonScale(150)
	, m_autoLimitMagnitude(false)
	, m_ccdCropOverlayHSize(DEFAULT_CCD_CROP_OVERLAY_SIZE)
	, m_ccdCropOverlayVSize(DEFAULT_CCD_CROP_OVERLAY_SIZE)
	, m_ccdDisplayed(false)
	, m_crosshairsDisplayed(false)
	, m_displayFocuserOverlayLarge(true)
	, m_displayFocuserOverlayMedium(true)
	, m_displayFocuserOverlaySmall(false)
	, m_hideGridsAndLines(false)
	, m_ocularDisplayed(false)
	, m_requireSelectionToZoom(true)
	, m_scaleFOVForTelrad(false)
	, m_scaleFOVForCCD(true)
	, m_scaleImageCircle(true)
	, m_showCCDCropOverlay(false)
	, m_showCCDCropOverlayPixelGrid(false)
	, m_showCardinalPoints(false)
	, m_showFocuserOverlay(false)
	, m_showOcularContour(false)
	, m_showOcularsButton(false)
	, m_showResolutionCriteria(false)
	, m_telradDisplayed(false)
	, m_transparencyLevel(85)
	, m_useDecimalDegrees(false)
	, m_useInitialDirection(false)
	, m_useInitialFOV(false)
	, m_useSemiTransparency(false)
	, m_indexSelectedCCD(-1)
	, m_indexSelectedLens(-1)
	, m_indexSelectedOcular(-1)
	, m_indexSelectedTelescope(-1)
	, selectedCCDRotationAngle(0.0)
	, selectedCCDPrismPositionAngle(0.0)
	, usageMessageLabelID(-1)
	, flagCardinalPointsMain(false)
	, flagAdaptationMain(false)
	, flagLimitStarsMain(false)
	, magLimitStarsMain(0.0)
	, flagLimitStarsOculars(false)
	, magLimitStarsOculars(0.0)
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
	, flagGuiPanelEnabled(false)
	, flagGridLinesDisplayedMain(true)
	, flagConstellationLinesMain(true)
	, flagConstellationBoundariesMain(true)
	, flagAsterismLinesMain(true)
	, flagRayHelpersLinesMain(true)
	, flipVertMain(false)
	, flipHorzMain(false)
	, pxmapGlow(nullptr)
	, pxmapOnIcon(nullptr)
	, pxmapOffIcon(nullptr)
	, toolbarButton(nullptr)
	, ocularDialog(nullptr)
	, ready(false)
	, actionShowOcular(nullptr)
	, actionShowCrosshairs(nullptr)
	, actionShowSensor(nullptr)
	, actionShowTelrad(nullptr)
	, actionConfiguration(nullptr)
	, actionMenu(nullptr)
	, actionTelescopeIncrement(nullptr)
	, actionTelescopeDecrement(nullptr)
	, actionOcularIncrement(nullptr)
	, actionOcularDecrement(nullptr)
	, guiPanel(nullptr)
	, guiPanelFontSize(12)
	, textColor(0.0)
	, lineColor(0.0)
	, focuserColor(0.0)
	, actualFOV(0.0)
	, initialFOV(0.0)
	, flagAutosetMountForCCD(false)
	, equatorialMountEnabledMain(false)
	, reticleRotation(0.0)
	, telradFOV(0.5F, 2.f, 4.f)
{
	setObjectName("Oculars");
	// Design font size is 14, based on default app fontsize 13.
	setFontSizeFromApp(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), &StelApp::screenFontSizeChanged, this, &Oculars::setFontSizeFromApp);

	ccds       = QList<CCD *>();
	oculars    = QList<Ocular *>();
	telescopes = QList<Telescope *>();
	lenses     = QList<Lens *>();

	qt_set_sequence_auto_mnemonic(true);
}

/* ****************************************************************************************************************** */
// MARK: - Instance methods
/* ****************************************************************************************************************** */
auto Oculars::getCcdCropOverlayHSize() const -> int
{
	return m_ccdCropOverlayHSize;
}
auto Oculars::getCcdCropOverlayVSize() const -> int
{
	return m_ccdCropOverlayVSize;
}
auto Oculars::isOcularDisplayed() const -> bool
{
	return m_ocularDisplayed;
}

auto Oculars::isCCDDisplayed() const -> bool
{
	return m_ccdDisplayed;
}
auto Oculars::isCrosshairsDisplayed() const -> bool
{
	return m_crosshairsDisplayed;
}
auto Oculars::isTelradSelected() const -> bool
{
	return m_telradDisplayed;
}
auto Oculars::getFlagGuiPanelEnabled() const -> bool
{
	return flagGuiPanelEnabled;
}
auto Oculars::getFocuserColor() const -> Vec3f
{
	return focuserColor;
}
auto Oculars::isScaleImageCircle() const -> bool
{
	return m_scaleImageCircle;
}
auto Oculars::getFlagShowOcularsButton() const -> bool
{
	return m_showOcularsButton;
}
auto Oculars::getGuiPanelFontSize() const -> int
{
	return guiPanelFontSize;
}
auto Oculars::getLineColor() const -> Vec3f
{
	return lineColor;
}

auto Oculars::getSettings() -> QSettings *
{
	return settings;
}

auto Oculars::getSelectedCCDRotationAngle() const -> double
{
	if (m_indexSelectedCCD < 0) {
		return 0.0;
	}
	auto * ccd = ccds.at(m_indexSelectedCCD);
	if (ccd != nullptr) {
		return ccd->rotationAngle();
	}
	return 0.0;
}

auto Oculars::getSelectedCCDPrismPositionAngle() const -> double
{
	if (m_indexSelectedCCD < 0) {
		return 0.0;
	}
	auto * ccd = ccds.at(m_indexSelectedCCD);
	if (ccd != nullptr) {
		return ccd->prismPosAngle();
	}
	return 0.0;
}
auto Oculars::indexSelectedCCD() const -> int
{
	return m_indexSelectedCCD;
}
auto Oculars::indexSelectedOcular() const -> int
{
	return m_indexSelectedOcular;
}
auto Oculars::indexSelectedTelescope() const -> int
{
	return m_indexSelectedTelescope;
}
auto Oculars::indexSelectedLens() const -> int
{
	return m_indexSelectedLens;
}
auto Oculars::getTextColor() const -> Vec3f
{
	return textColor;
}

/* ****************************************************************************************************************** */
// MARK: - StelModule Methods
/* ****************************************************************************************************************** */
auto Oculars::configureGui(bool show) -> bool
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
	settings->remove("lens");
	int index = 0;
	for (auto * ccd : qAsConst(ccds)) {
		ccd->writeToSettings(settings, index);
		index++;
	}
	index = 0;
	for (auto * ocular : qAsConst(oculars)) {
		ocular->writeToSettings(settings, index);
		index++;
	}
	index = 0;
	for (auto * telescope : qAsConst(telescopes)) {
		telescope->writeToSettings(settings, index);
		index++;
	}
	index = 0;
	for (auto * lens : qAsConst(lenses)) {
		lens->writeToSettings(settings, index);
		index++;
	}

	settings->setValue("ocular_count", oculars.count());
	settings->setValue("telescope_count", telescopes.count());
	settings->setValue("ccd_count", ccds.count());
	settings->setValue("lens_count", lenses.count());
	settings->setValue("ocular_index", m_indexSelectedOcular);
	settings->setValue("telescope_index", m_indexSelectedTelescope);
	settings->setValue("ccd_index", m_indexSelectedCCD);
	settings->setValue("lens_index", m_indexSelectedLens);

	auto * core      = StelApp::getInstance().getCore();
	auto * skyDrawer = core->getSkyDrawer();
	disconnect(skyDrawer, &StelSkyDrawer::customStarMagLimitChanged, this, &Oculars::setMagLimitStarsOcularsManual);
	disconnect(skyDrawer, &StelSkyDrawer::flagStarMagnitudeLimitChanged, this, &Oculars::handleStarMagLimitToggle);
	if (m_ccdDisplayed) {
		// Retrieve and restore star scales
		relativeStarScaleCCD = skyDrawer->getRelativeStarScale();
		absoluteStarScaleCCD = skyDrawer->getAbsoluteStarScale();
		skyDrawer->setRelativeStarScale(relativeStarScaleMain);
		skyDrawer->setAbsoluteStarScale(absoluteStarScaleMain);
	} else if (m_ocularDisplayed) {
		qDebug() << "Oculars::deinit() .. restoring skyDrawer values while ocular view is active";

		if (!isAutoLimitMagnitude()) {
			flagLimitStarsOculars = skyDrawer->getFlagStarMagnitudeLimit();
			magLimitStarsOculars  = skyDrawer->getCustomStarMagnitudeLimit();
		}
		skyDrawer->setCustomStarMagnitudeLimit(magLimitStarsMain);
		skyDrawer->setFlagStarMagnitudeLimit(flagLimitStarsMain);
		// Retrieve and restore star scales
		relativeStarScaleOculars = skyDrawer->getRelativeStarScale();
		absoluteStarScaleOculars = skyDrawer->getAbsoluteStarScale();
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

	disconnect(this, &Oculars::indexSelectedOcularChanged, this, &Oculars::updateOcularReticle);
	disconnect(&StelApp::getInstance(), &StelApp::languageChanged, this, &Oculars::retranslateGui);

	textureProtractor.clear();
	textureProtractorFlipH.clear();
	textureProtractorFlipHV.clear();
	textureProtractorFlipV.clear();
}

//! Draw any parts on the screen which are for our module
void Oculars::draw(StelCore * core)
{
	if (m_telradDisplayed) {
		paintTelrad();
	} else if (m_ocularDisplayed) {
		if (m_indexSelectedOcular >= 0) {
			paintOcularMask(core);
			if (m_crosshairsDisplayed) {
				paintCrosshairs();
			}

			if (!flagGuiPanelEnabled) {
				// Paint the information in the upper-right hand corner
				paintText(core);
			}
		} else {
			qWarning() << "Oculars: the selected ocular index of " << m_indexSelectedOcular
				   << " is greater than the ocular count of " << oculars.count() << ". Module disabled!";
		}
	} else if (m_ccdDisplayed) {
		paintCCDBounds();
		if (!flagGuiPanelEnabled) {
			// Paint the information in the upper-right hand corner
			paintText(core);
		}
	}
}

//! Determine which "layer" the plugin's drawing will happen on.
auto Oculars::getCallOrder(StelModuleActionName actionName) const -> double
{
	double order = 1000.0; // Very low priority, unless we interact.

	if (actionName == StelModule::ActionHandleMouseMoves || actionName == StelModule::ActionHandleMouseClicks) {
		// Make sure we are called before MovementMgr (we need to even call it once!)
		order = StelApp::getInstance().getModuleMgr().getModule("StelMovementMgr")->getCallOrder(actionName) - 1.0;
	} else if (actionName == StelModule::ActionDraw) {
		order = GETSTELMODULE(LabelMgr)->getCallOrder(actionName) + 100.0;
	}
	return order;
}

void Oculars::handleMouseClicks(class QMouseEvent * event)
{
	StelCore *                         core   = StelApp::getInstance().getCore();
	const StelProjectorP               prj    = core->getProjection(StelCore::FrameJ2000, StelCore::RefractionAuto);
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
	qreal                              ppx    = params.devicePixelsPerPixel;

	if (guiPanel != nullptr) {
		// Remove all events on the sky within Ocular GUI Panel.
		if (event->x() > guiPanel->pos().x() && event->y() > (prj->getViewportHeight() - guiPanel->size().height())) {
			event->setAccepted(true);
			return;
		}
	}

	// In case we show oculars with black circle, ignore mouse presses outside image circle:
	if (m_ocularDisplayed) {
		int    wh    = prj->getViewportWidth() / 2;  // get half of width of the screen
		int    hh    = prj->getViewportHeight() / 2; // get half of height of the screen
		int    mx    = event->x() - wh;              // point 0 in center of the screen, axis X directed to right
		int    my    = event->y() - hh;              // point 0 in center of the screen, axis Y directed to bottom

		double inner = 0.5 * params.viewportFovDiameter * ppx;
		// See if we need to scale the mask
		if (m_scaleImageCircle && oculars.at(m_indexSelectedOcular)->apparentFOV() > 0.0 &&
				!oculars.at(m_indexSelectedOcular)->isBinoculars()) {
			inner = oculars.at(m_indexSelectedOcular)->apparentFOV() * inner / maxEyepieceAngle;
		}

		if (mx * mx + my * my > inner * inner) { // click outside ocular circle? Gobble event.
			event->setAccepted(true);
			return;
		}
	}

	StelMovementMgr * movementManager = core->getMovementMgr();

	if (m_ocularDisplayed) {
		movementManager->handleMouseClicks(event); // force it here for selection!
	}

	if (StelApp::getInstance().getStelObjectMgr().getWasSelected()) {
		if (m_ocularDisplayed) {
			// center the selected object in the ocular, and track.
			movementManager->setFlagTracking(true);
		} else {
			// remove the usage label if it is being displayed.
			hideUsageMessageIfDisplayed();
		}
	} else if (m_ocularDisplayed) {
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
		ready                    = true;

		m_requireSelectionToZoom = settings->value("require_selection_to_zoom", true).toBool();
		m_scaleImageCircle       = settings->value("use_max_exit_circle", false).toBool();
		int ocularCount          = settings->value("ocular_count", 0).toInt();
		int actualOcularCount    = ocularCount;
		for (int index = 0; index < ocularCount; index++) {
			auto * newOcular = new Ocular(this);
			newOcular->initFromSettings(settings, index);
			if (newOcular != nullptr) {
				oculars.append(newOcular);
			} else {
				actualOcularCount--;
			}
		}
		if (actualOcularCount < 1) {
			ready = false;
		} else {
			m_indexSelectedOcular = settings->value("ocular_index", 0).toInt();
		}

		int ccdCount       = settings->value("ccd_count", 0).toInt();
		int actualCcdCount = ccdCount;
		for (int index = 0; index < ccdCount; index++) {
			auto * newCCD = new CCD(this);
			newCCD->initFromSettings(settings, index);
			if (newCCD != nullptr) {
				ccds.append(newCCD);
			} else {
				actualCcdCount--;
			}
		}
		if (actualCcdCount < ccdCount) {
			qWarning() << "The Oculars ini file appears to be corrupt; delete it.";
			ready = false;
		}
		m_indexSelectedCCD       = settings->value("ccd_index", 0).toInt();

		int telescopeCount       = settings->value("telescope_count", 0).toInt();
		int actualTelescopeCount = telescopeCount;
		for (int index = 0; index < telescopeCount; index++) {
			auto * newTelescope = new Telescope(this);
			newTelescope->initFromSettings(settings, index);
			if (newTelescope != nullptr) {
				telescopes.append(newTelescope);
			} else {
				actualTelescopeCount--;
			}
		}
		if (actualTelescopeCount < 1) {
			ready = false;
		} else {
			m_indexSelectedTelescope = settings->value("telescope_index", 0).toInt();
		}

		int lensCount       = settings->value("lens_count", 0).toInt();
		int actualLensCount = lensCount;
		for (int index = 0; index < lensCount; index++) {
			auto * newLens = new Lens(this);
			newLens->initFromSettings(settings, index);
			if (newLens != nullptr) {
				lenses.append(newLens);
			} else {
				actualLensCount--;
			}
		}
		if (lensCount > 0 && actualLensCount < lensCount) {
			qWarning() << "The Oculars ini file appears to be corrupt; delete it.";
		}
		m_indexSelectedLens = settings->value("lens_index", -1).toInt(); // Lens is not selected by default!

		pxmapGlow           = new QPixmap(":/graphicGui/miscGlow32x32.png");
		pxmapOnIcon         = new QPixmap(":/ocular/bt_ocular_on.png");
		pxmapOffIcon        = new QPixmap(":/ocular/bt_ocular_off.png");

		ocularDialog        = new OcularDialog(this, &ccds, &oculars, &telescopes, &lenses);
		initializeActivationActions();
		determineMaxEyepieceAngle();

		enableGuiPanel(settings->value("enable_control_panel", true).toBool());
		setHideGridsAndLines(settings->value("hide_grids_and_lines", true).toBool());
		setFlagAutosetMountForCCD(settings->value("use_mount_autoset", false).toBool());
		setShowOcularsButton(settings->value("show_toolbar_button", false).toBool());

		guiPanelFontSize              = settings->value("gui_panel_fontsize", 12).toInt();
		textColor                     = Vec3f(settings->value("text_color", "0.8,0.48,0.0").toString());
		lineColor                     = Vec3f(settings->value("line_color", "0.77,0.14,0.16").toString());
		telradFOV                     = Vec4d(settings->value("telrad_fov", "0.5,2.0,4.0,0.0").toString());
		focuserColor                  = Vec3f(settings->value("focuser_color", "0.0,0.67,1.0").toString());

		// This must come ahead of setFlagAutosetMountForCCD (GH #505)
		StelPropertyMgr * propMgr     = StelApp::getInstance().getStelPropertyManager();
		equatorialMountEnabledMain    = propMgr->getStelPropertyValue("StelMovementMgr.equatorialMount").toBool();

		// For historical reasons, name of .ini entry and description of checkbox (and therefore flag name) are reversed.
		m_useDecimalDegrees           = !settings->value("use_decimal_degrees", false).toBool();
		m_autoLimitMagnitude          = settings->value("autolimit_stellar_magnitude", true).toBool();
		flagLimitStarsOculars         = settings->value("limit_stellar_magnitude_oculars", false).toBool();
		magLimitStarsOculars          = settings->value("limit_stellar_magnitude_oculars_val", 12.0).toDouble();
		m_useInitialFOV               = settings->value("use_initial_fov", false).toBool();
		m_useInitialDirection         = settings->value("use_initial_direction", false).toBool();
		m_useSemiTransparency         = settings->value("use_semi_transparency", false).toBool();
		m_transparencyLevel           = settings->value("transparency_mask", 85).toInt();
		m_scaleFOVForTelrad           = settings->value("use_telrad_fov_scaling", true).toBool();
		m_scaleFOVForCCD              = settings->value("use_ccd_fov_scaling", true).toBool();
		m_showResolutionCriteria      = settings->value("show_resolution_criteria", false).toBool();
		m_arrowButtonScale            = settings->value("arrow_scale", 150).toInt();
		relativeStarScaleOculars      = settings->value("stars_scale_relative", 1.0).toDouble();
		absoluteStarScaleOculars      = settings->value("stars_scale_absolute", 1.0).toDouble();
		relativeStarScaleCCD          = settings->value("stars_scale_relative_ccd", 1.0).toDouble();
		absoluteStarScaleCCD          = settings->value("stars_scale_absolute_ccd", 1.0).toDouble();
		m_showCCDCropOverlay          = settings->value("show_ccd_crop_overlay", false).toBool();
		m_showCCDCropOverlayPixelGrid = settings->value("ccd_crop_overlay_pixel_grid", false).toBool();
		m_ccdCropOverlayHSize         = settings->value("ccd_crop_overlay_hsize", DEFAULT_CCD_CROP_OVERLAY_SIZE).toInt();
		m_ccdCropOverlayVSize         = settings->value("ccd_crop_overlay_vsize", DEFAULT_CCD_CROP_OVERLAY_SIZE).toInt();
		m_showOcularContour           = settings->value("show_ocular_contour", false).toBool();
		m_showCardinalPoints          = settings->value("show_ocular_cardinals", false).toBool();
		m_alignCrosshair              = settings->value("align_crosshair", false).toBool();
		m_showFocuserOverlay          = settings->value("show_focuser_overlay", false).toBool();
		m_displayFocuserOverlaySmall  = settings->value("use_small_focuser_overlay", false).toBool();
		m_displayFocuserOverlayMedium = settings->value("use_medium_focuser_overlay", true).toBool();
		m_displayFocuserOverlayLarge  = settings->value("use_large_focuser_overlay", false).toBool();
	} catch (std::runtime_error & e) {
		qWarning() << "WARNING: unable to locate ocular.ini file or create a default one for Ocular plugin: " << e.what();
		ready = false;
	}

	textureProtractor       = StelApp::getInstance().getTextureManager().createTexture(":/ocular/Protractor.png");
	textureProtractorFlipH  = StelApp::getInstance().getTextureManager().createTexture(":/ocular/ProtractorFlipH.png");
	textureProtractorFlipHV = StelApp::getInstance().getTextureManager().createTexture(":/ocular/ProtractorFlipHV.png");
	textureProtractorFlipV  = StelApp::getInstance().getTextureManager().createTexture(":/ocular/ProtractorFlipV.png");
	// enforce check existence of reticle for the current eyepiece
	updateOcularReticle();

	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &Oculars::retranslateGui);
	connect(this, &Oculars::indexSelectedOcularChanged, this, &Oculars::updateOcularReticle);
	auto * core      = StelApp::getInstance().getCore();
	auto * skyDrawer = core->getSkyDrawer();
	connect(skyDrawer, &StelSkyDrawer::flagStarMagnitudeLimitChanged, this, &Oculars::handleStarMagLimitToggle);
}

void Oculars::update(double deltaTime)
{
	Q_UNUSED(deltaTime)
}

/* ****************************************************************************************************************** */
// MARK: - Private slots Methods
/* ****************************************************************************************************************** */
void Oculars::determineMaxEyepieceAngle()
{
	if (ready) {
		for (const auto * ocular : oculars) {
			if (ocular->apparentFOV() > maxEyepieceAngle) {
				maxEyepieceAngle = ocular->apparentFOV();
			}
		}
	}
	// ensure it is not zero
	if (maxEyepieceAngle == 0.0) {
		maxEyepieceAngle = 1.0;
	}
}

void Oculars::instrumentChanged()
{
	// We only zoom if in ocular mode.
	if (m_ocularDisplayed) {
		// If we are already in Ocular mode, we must reset scalings because zoom() also resets.
		StelSkyDrawer * skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();
		skyDrawer->setRelativeStarScale(relativeStarScaleMain);
		skyDrawer->setAbsoluteStarScale(absoluteStarScaleMain);
		zoom(true);
	} else if (m_ccdDisplayed) {
		setScreenFOVForCCD();
	}
}

void Oculars::setScaleImageCircle(bool state)
{
	if (state) {
		determineMaxEyepieceAngle();
	}
	m_scaleImageCircle = state;
	settings->setValue("use_max_exit_circle", state);
	settings->sync();
	emit scaleImageCircleChanged(state);
}

void Oculars::setScreenFOVForCCD()
{
	// CCD is not shown and FOV scaling is disabled, but telescope is changed - do not change FOV!
	if (!(isScaleFOVForCCD() && m_ccdDisplayed)) {
		return;
	}

	auto * lens = m_indexSelectedLens >= 0 ? lenses.at(m_indexSelectedLens) : nullptr;
	if (m_indexSelectedCCD > -1 && m_indexSelectedTelescope > -1) {
		auto * core            = StelApp::getInstance().getCore();
		auto * movementManager = core->getMovementMgr();
		double actualFOVx = ccds.at(m_indexSelectedCCD)->getActualFOVx(telescopes.at(m_indexSelectedTelescope), lens);
		double actualFOVy = ccds.at(m_indexSelectedCCD)->getActualFOVy(telescopes.at(m_indexSelectedTelescope), lens);
		if (actualFOVx < actualFOVy) {
			actualFOVx = actualFOVy;
		}
		double factor = 1.75;
		if (ccds.at(m_indexSelectedCCD)->hasOAG()) {
			factor *= 2;
		}
		movementManager->setFlagTracking(true);
		movementManager->zoomTo(actualFOVx * factor, 0.0F);
	}
}

void Oculars::enableGuiPanel(bool enable)
{
	if (enable) {
		if (guiPanel == nullptr) {
			auto * gui = dynamic_cast<StelGui *>(StelApp::getInstance().getGui());
			if (gui != nullptr) {
				guiPanel = new OcularsGuiPanel(this, gui->getSkyGui());
				if (m_ocularDisplayed) {
					guiPanel->showOcularGui();
				} else if (m_ccdDisplayed) {
					guiPanel->showCcdGui();
				}
			}
		}
	} else {
		if (guiPanel != nullptr) {
			guiPanel->hide();
			delete guiPanel;
			guiPanel = nullptr;
		}
	}
	flagGuiPanelEnabled = enable;
	settings->setValue("enable_control_panel", enable);
	settings->sync();
	emit flagGuiPanelEnabledChanged(enable);
}

void Oculars::retranslateGui()
{
	if (guiPanel != nullptr) {
		// TODO: Fix this hack!

		// Delete and re-create the panel to retranslate its trings
		guiPanel->hide();
		delete guiPanel;
		guiPanel   = nullptr;

		auto * gui = dynamic_cast<StelGui *>(StelApp::getInstance().getGui());
		if (gui != nullptr) {
			guiPanel = new OcularsGuiPanel(this, gui->getSkyGui());
			if (m_ocularDisplayed) {
				guiPanel->showOcularGui();
			} else if (m_ccdDisplayed) {
				guiPanel->showCcdGui();
			}
		}
	}
}

void Oculars::updateOcularReticle()
{
	reticleRotation            = 0.0;
	QString reticleTexturePath = oculars.at(m_indexSelectedOcular)->reticlePath();
	if (reticleTexturePath.length() == 0) {
		textureReticle = StelTextureSP();
	} else {
		StelTextureMgr &               manager = StelApp::getInstance().getTextureManager();
		// Load OpenGL textures
		StelTexture::StelTextureParams params;
		params.generateMipmaps = true;
		textureReticle         = manager.createTexture(reticleTexturePath, params);
	}
}

/* ****************************************************************************************************************** */
// MARK: - Slots Methods
/* ****************************************************************************************************************** */
void Oculars::updateLists()
{
	if (oculars.isEmpty()) {
		m_indexSelectedOcular = -1;
		setOcularDisplayed(false);
	} else {
		if (m_indexSelectedOcular >= oculars.count()) {
			m_indexSelectedOcular = oculars.count() - 1;
		}

		if (m_ocularDisplayed) {
			emit indexSelectedOcularChanged(m_indexSelectedOcular);
		}
	}

	if (telescopes.isEmpty()) {
		m_indexSelectedTelescope = -1;
		setOcularDisplayed(false);
		setCCDDisplayed(false);
	} else {
		if (m_indexSelectedTelescope >= telescopes.count()) {
			m_indexSelectedTelescope = telescopes.count() - 1;
		}

		if (m_ocularDisplayed || m_ccdDisplayed) {
			emit indexSelectedTelescopeChanged(m_indexSelectedTelescope);
		}
	}

	if (ccds.isEmpty()) {
		m_indexSelectedCCD = -1;
		setCCDDisplayed(false);
	} else {
		if (m_indexSelectedCCD >= ccds.count()) {
			m_indexSelectedCCD = ccds.count() - 1;
		}

		if (m_ccdDisplayed)
			emit indexSelectedCCDChanged(m_indexSelectedCCD);
	}

	if (lenses.isEmpty()) {
		m_indexSelectedLens = -1;
	} else {
		if (m_indexSelectedLens >= lenses.count()) {
			m_indexSelectedLens = lenses.count() - 1;
		}
	}
}

void Oculars::ccdRotationReset()
{
	if (m_indexSelectedCCD < 0) {
		return;
	}
	auto * ccd = ccds.at(m_indexSelectedCCD);
	if (ccd != nullptr) {
		ccd->setRotationAngle(0.0);
		emit(indexSelectedCCDChanged(m_indexSelectedCCD));
		emit selectedCCDRotationAngleChanged(0.0);
	}
}

void Oculars::prismPositionAngleReset()
{
	if (m_indexSelectedCCD < 0) {
		return;
	}
	auto * ccd = ccds.at(m_indexSelectedCCD);
	if (ccd != nullptr) {
		ccd->setPrismPosAngle(0.0);
		emit(indexSelectedCCDChanged(m_indexSelectedCCD));
		emit selectedCCDPrismPositionAngleChanged(0.0);
	}
}

void Oculars::setFontSize(int s)
{
	font.setPixelSize(s);
}

void Oculars::setFontSizeFromApp(int s)
{
	font.setPixelSize(s + 1);
}

void Oculars::setSelectedCCDRotationAngle(double angle)
{
	ccds.at(m_indexSelectedCCD)->setRotationAngle(angle);
	emit selectedCCDRotationAngleChanged(angle);
}

void Oculars::setSelectedCCDPrismPositionAngle(double angle)
{
	ccds.at(m_indexSelectedCCD)->setPrismPosAngle(angle);
	emit selectedCCDPrismPositionAngleChanged(angle);
}

void Oculars::setTextColor(Vec3f color)
{
	textColor = color;
	emit textColorChanged(color);
}

void Oculars::setLineColor(Vec3f color)
{
	lineColor = color;
	emit lineColorChanged(color);
}

void Oculars::setFocuserColor(Vec3f color)
{
	focuserColor = color;
	emit focuserColorChanged(color);
}

void Oculars::setOcularDisplayed(bool enableOcularMode)
{
	if (enableOcularMode) {
		// Close the sensor view if it's displayed
		if (m_ccdDisplayed) {
			setCCDDisplayed(false);
			m_ccdDisplayed     = false;
			m_indexSelectedCCD = -1;
		}

		// Close the Telrad sight if it's displayed
		if (m_telradDisplayed) {
			setTelradDisplayed(false);
		}

		// Check to ensure that we have enough oculars & telescopes, as they may have been edited in the config dialog
		if (oculars.count() == 0) {
			m_indexSelectedOcular = -1;
			qWarning() << "No oculars found";
		} else if (oculars.count() > 0 && m_indexSelectedOcular == -1) {
			m_indexSelectedOcular = 0;
		}
		if (telescopes.count() == 0) {
			m_indexSelectedTelescope = -1;
			qWarning() << "No telescopes found";
		} else if (telescopes.count() > 0 && m_indexSelectedTelescope == -1) {
			m_indexSelectedTelescope = 0;
		}
	}

	if (!ready || m_indexSelectedOcular == -1 || (m_indexSelectedTelescope == -1 && !isBinocularDefined())) {
		qWarning() << "The Oculars module has been disabled.";
		return;
	}

	StelCore * core         = StelApp::getInstance().getCore();
	auto *     labelManager = GETSTELMODULE(LabelMgr);

	// Toggle the ocular view on & off. To toggle on, we want to ensure there is a selected object.
	if (!m_ocularDisplayed && !m_telradDisplayed && m_requireSelectionToZoom &&
			!StelApp::getInstance().getStelObjectMgr().getWasSelected()) {
		if (usageMessageLabelID == -1) {
			QFontMetrics                       metrics(font);
			QString                            labelText = q_("Please select an object before switching to ocular view.");
			StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
			int yPositionOffset = qRound(projectorParams.viewportXywh[3] * projectorParams.viewportCenterOffset[1]);
			int xPosition = qRound(projectorParams.viewportCenter[0] - 0.5 * metrics.boundingRect(labelText).width());
			int yPosition = qRound(projectorParams.viewportCenter[1] - yPositionOffset - 0.5 * metrics.height());
			const char * tcolor = "#99FF99";
			usageMessageLabelID =
					labelManager->labelScreen(labelText, xPosition, yPosition, true, font.pixelSize(), tcolor);
		}
	} else {
		if (m_indexSelectedOcular != -1) {
			// remove the usage label if it is being displayed.
			hideUsageMessageIfDisplayed();
			m_ocularDisplayed = enableOcularMode;
			zoom(false);
			// BM: I hope this is the right place...
			if (guiPanel != nullptr) {
				guiPanel->showOcularGui();
			}
		}
	}

	emit ccularDisplayChanged(m_ocularDisplayed);
}

void Oculars::decrementCCDIndex()
{
	m_indexSelectedCCD--;
	if (m_indexSelectedCCD == -1) {
		m_indexSelectedCCD = ccds.count() - 1;
	}
	emit(indexSelectedCCDChanged(m_indexSelectedCCD));
}

void Oculars::decrementOcularIndex()
{
	m_indexSelectedOcular--;
	if (m_indexSelectedOcular == -1) {
		m_indexSelectedOcular = oculars.count() - 1;
	}
	// validate the new selection
	if (m_indexSelectedOcular > -1 && !oculars.at(m_indexSelectedOcular)->isBinoculars()) {
		if (m_indexSelectedTelescope == -1 && telescopes.count() == 0) {
			// reject the change
			m_indexSelectedOcular++;
		}

		if (m_indexSelectedTelescope == -1) {
			m_indexSelectedTelescope = 0;
		}
	}
	emit(indexSelectedOcularChanged(m_indexSelectedOcular));
}

void Oculars::decrementTelescopeIndex()
{
	m_indexSelectedTelescope--;
	if (m_indexSelectedTelescope == -1) {
		m_indexSelectedTelescope = telescopes.count() - 1;
	}
	emit(indexSelectedTelescopeChanged(m_indexSelectedTelescope));
}

void Oculars::decrementLensIndex()
{
	m_indexSelectedLens--;
	if (m_indexSelectedLens == lenses.count()) {
		m_indexSelectedLens = -1;
	}
	if (m_indexSelectedLens == -2) {
		m_indexSelectedLens = lenses.count() - 1;
	}
	emit(indexSelectedLensChanged(m_indexSelectedLens));
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
	auto * popup = new QMenu(&StelMainView::getInstance());

	if (m_ocularDisplayed) {
		// We are in Oculars mode
		// We want to show all of the Oculars, and if the current ocular is not a binocular,
		// we will also show the telescopes.
		if (!oculars.isEmpty()) {
			popup->addAction(q_("&Previous ocular"), this, &Oculars::decrementOcularIndex);
			popup->addAction(q_("&Next ocular"), this, &Oculars::incrementOcularIndex);
			auto * submenu              = new QMenu(q_("Select &ocular"), popup);
			int    availableOcularCount = 0;
			for (int index = 0; index < oculars.count(); ++index) {
				QString label;
				if (availableOcularCount < 10) {
					label = QString("&%1: %2").arg(availableOcularCount).arg(oculars.at(index)->name());
				} else {
					label = oculars.at(index)->name();
				}
				// BM: Does this happen at all any more?
				QAction * action = nullptr;
				if (m_indexSelectedTelescope != -1 || oculars.at(index)->isBinoculars()) {
					action = submenu->addAction(label, [=]() { setIndexSelectedOcular(index); });
					availableOcularCount++;
				}

				if ((action != nullptr) && index == m_indexSelectedOcular) {
					action->setCheckable(true);
					action->setChecked(true);
				}
			}
			popup->addMenu(submenu);
			popup->addSeparator();
		}

		// If there is more than one telescope, show the prev/next/list complex.
		// If the selected ocular is a binoculars, show nothing.
		if (telescopes.count() > 1 &&
				(m_indexSelectedOcular > -1 && !oculars.at(m_indexSelectedOcular)->isBinoculars())) {
			QMenu * submenu = addTelescopeSubmenu(popup);
			popup->addMenu(submenu);
			submenu = addLensSubmenu(popup);
			popup->addMenu(submenu);
			popup->addSeparator();
		}

		QAction * action = popup->addAction(q_("Toggle &crosshair"));
		action->setCheckable(true);
		action->setChecked(m_crosshairsDisplayed);
		connect(action, &QAction::toggled, actionShowCrosshairs, &StelAction::setChecked);
	} else {
		// We are not in ocular mode
		// We want to show the CCD's, and if a CCD is selected, the telescopes
		//(as a CCD requires a telescope) and the general menu items.
		auto * action = new QAction(q_("Configure &Oculars"), popup);
		action->setCheckable(true);
		action->setChecked(ocularDialog->visible());
		connect(action, &QAction::triggered, ocularDialog, &OcularDialog::setVisible);
		popup->addAction(action);
		popup->addSeparator();

		if (!m_telradDisplayed) {
			QAction * actionToggleCCD = popup->addAction(q_("Toggle &CCD"));
			actionToggleCCD->setCheckable(true);
			actionToggleCCD->setChecked(m_ccdDisplayed);
			connect(actionToggleCCD, &QAction::toggled, actionShowSensor, &StelAction::setChecked);
		}

		if (!m_ccdDisplayed) {
			QAction * actionToggleTelrad = popup->addAction(q_("Toggle &Telrad"));
			actionToggleTelrad->setCheckable(true);
			actionToggleTelrad->setChecked(m_telradDisplayed);
			connect(actionToggleTelrad, &QAction::toggled, actionShowTelrad, &StelAction::setChecked);
		}

		popup->addSeparator();
		if (m_ccdDisplayed && m_indexSelectedCCD > -1 && m_indexSelectedTelescope > -1) {
			popup->addAction(q_("&Previous CCD"), this, &Oculars::decrementCCDIndex);
			popup->addAction(q_("&Next CCD"), this, &Oculars::incrementCCDIndex);
			auto * submenu = new QMenu(q_("&Select CCD"), popup);
			for (int index = 0; index < ccds.count(); ++index) {
				QString label;
				if (index < 10) {
					label = QString("&%1: %2").arg(index).arg(ccds.at(index)->name());
				} else {
					label = ccds.at(index)->name();
				}
				QAction * actionelectCCD = submenu->addAction(label, [=]() { setIndexSelectedCCD(index); });
				if (index == m_indexSelectedCCD) {
					actionelectCCD->setCheckable(true);
					actionelectCCD->setChecked(true);
				}
			}
			popup->addMenu(submenu);

			submenu = new QMenu(q_("&Rotate CCD"), popup);
			submenu->addAction(QString("&1: -90º"), [=]() { rotateCCD(-90); });
			submenu->addAction(QString("&2: -45º"), [=]() { rotateCCD(-45); });
			submenu->addAction(QString("&3: -15º"), [=]() { rotateCCD(-15); });
			submenu->addAction(QString("&4: -5º"), [=]() { rotateCCD(-5); });
			submenu->addAction(QString("&5: -1º"), [=]() { rotateCCD(-1); });
			submenu->addAction(QString("&6: +1º"), [=]() { rotateCCD(1); });
			submenu->addAction(QString("&7: +5º"), [=]() { rotateCCD(5); });
			submenu->addAction(QString("&8: +15º"), [=]() { rotateCCD(15); });
			submenu->addAction(QString("&9: +45º"), [=]() { rotateCCD(45); });
			submenu->addAction(QString("&0: +90º"), [=]() { rotateCCD(90); });

			submenu->addAction(q_("&Reset rotation"), this, &Oculars::ccdRotationReset);
			popup->addMenu(submenu);
			popup->addSeparator();
		}
		if (m_ccdDisplayed && m_indexSelectedCCD > -1 && telescopes.count() > 1) {
			QMenu * submenu = addTelescopeSubmenu(popup);
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
	m_indexSelectedCCD++;
	if (m_indexSelectedCCD == ccds.count()) {
		m_indexSelectedCCD = 0;
	}
	emit(indexSelectedCCDChanged(m_indexSelectedCCD));
}

void Oculars::incrementOcularIndex()
{
	m_indexSelectedOcular++;
	if (m_indexSelectedOcular == oculars.count()) {
		m_indexSelectedOcular = 0;
	}
	// validate the new selection
	if (m_indexSelectedOcular > -1 && !oculars.at(m_indexSelectedOcular)->isBinoculars()) {
		if (m_indexSelectedTelescope == -1 && telescopes.count() == 0) {
			// reject the change
			m_indexSelectedOcular++;
		}

		if (m_indexSelectedTelescope == -1) {
			setIndexSelectedTelescope(0);
		}
	}
	emit(indexSelectedOcularChanged(m_indexSelectedOcular));
}

void Oculars::incrementTelescopeIndex()
{
	m_indexSelectedTelescope++;
	if (m_indexSelectedTelescope == telescopes.count()) {
		m_indexSelectedTelescope = 0;
	}
	emit(indexSelectedTelescopeChanged(m_indexSelectedTelescope));
}

void Oculars::incrementLensIndex()
{
	m_indexSelectedLens++;
	if (m_indexSelectedLens == lenses.count()) {
		m_indexSelectedLens = -1;
	}
	emit(indexSelectedLensChanged(m_indexSelectedLens));
}

void Oculars::disableLens()
{
	m_indexSelectedLens = -1;
	emit(indexSelectedLensChanged(m_indexSelectedLens));
}

void Oculars::rotateCCD(int amount)
{
	auto * ccd = ccds.at(m_indexSelectedCCD);
	if (ccd != nullptr) {
		double angle = ccd->rotationAngle();
		angle += amount;
		if (angle >= 360) {
			angle -= 360;
		} else if (angle <= -360) {
			angle += 360;
		}
		ccd->setRotationAngle(angle);
	}
}

void Oculars::rotatePrism(int amount)
{
	CCD * ccd = ccds.at(m_indexSelectedCCD);
	if (ccd != nullptr) {
		double angle = ccd->prismPosAngle();
		angle += amount;
		if (angle >= 360) {
			angle -= 360;
		} else if (angle <= -360) {
			angle += 360;
		}
		ccd->setPrismPosAngle(angle);
	}
}

void Oculars::setIndexSelectedCCD(int index)
{
	if (index > -1 && index < ccds.count()) {
		m_indexSelectedCCD = index;
		emit(indexSelectedCCDChanged(index));
	}
}

void Oculars::setIndexSelectedOcular(int index)
{
	if (m_indexSelectedTelescope == -1) {
		setIndexSelectedTelescope(0);
	}

	if (index > -1 && index < oculars.count() && (telescopes.count() >= 0 || oculars.at(index)->isBinoculars())) {
		m_indexSelectedOcular = index;
		emit(indexSelectedOcularChanged(index));
	}
}

void Oculars::setIndexSelectedTelescope(int index)
{
	if (index > -1 && index < telescopes.count()) {
		m_indexSelectedTelescope = index;
		emit(indexSelectedTelescopeChanged(index));
	}
}

void Oculars::setIndexSelectedLens(int index)
{
	if (index > -2 && index < lenses.count()) {
		m_indexSelectedLens = index;
		emit(indexSelectedLensChanged(index));
	}
}

void Oculars::toggleCCD()
{
	setCCDDisplayed(!m_ccdDisplayed);
}

void Oculars::setCCDDisplayed(bool show)
{
	// If there are no sensors...
	if (ccds.isEmpty() || telescopes.isEmpty()) {
		// TODO: BM: Make this an on-screen message and/or disable the button
		// if there are no sensors.
		if (show) {
			qWarning() << "Oculars plugin: Unable to display a sensor boundary: No sensors or telescopes are defined.";
			QMessageBox::warning(&StelMainView::getInstance(),
					     q_("Warning!"),
					     q_("Unable to display a sensor boundary: No sensors or telescopes are defined."),
					     QMessageBox::Ok);
		}
		m_ccdDisplayed     = false;
		m_indexSelectedCCD = -1;
		show               = false;
	}

	StelCore *        core            = StelApp::getInstance().getCore();
	StelMovementMgr * movementManager = core->getMovementMgr();
	StelSkyDrawer *   skyDrawer       = core->getSkyDrawer();
	if (show) {
		initialFOV = movementManager->getCurrentFov();
		// Mutually exclusive with the ocular mode
		hideUsageMessageIfDisplayed();
		if (m_ocularDisplayed) {
			setOcularDisplayed(false);
		}

		if (m_telradDisplayed) {
			setTelradDisplayed(false);
		}

		if (m_indexSelectedTelescope < 0) {
			m_indexSelectedTelescope = 0;
		}
		if (m_indexSelectedCCD < 0) {
			m_indexSelectedCCD = 0;
		}
		m_ccdDisplayed = true;
		setScreenFOVForCCD();

		// Change scales for stars. (Even restoring from ocular view has restored main program's values at this point.)
		relativeStarScaleMain = skyDrawer->getRelativeStarScale();
		absoluteStarScaleMain = skyDrawer->getAbsoluteStarScale();
		skyDrawer->setRelativeStarScale(relativeStarScaleCCD);
		skyDrawer->setAbsoluteStarScale(absoluteStarScaleCCD);

		if (guiPanel != nullptr) {
			guiPanel->showCcdGui();
		}
	} else {
		m_ccdDisplayed       = false;

		// Restore star scales
		relativeStarScaleCCD = skyDrawer->getRelativeStarScale();
		absoluteStarScaleCCD = skyDrawer->getAbsoluteStarScale();
		skyDrawer->setRelativeStarScale(relativeStarScaleMain);
		skyDrawer->setAbsoluteStarScale(absoluteStarScaleMain);
		movementManager->setFlagTracking(false);
		// Zoom out
		if (isUseInitialFOV()) {
			movementManager->zoomTo(movementManager->getInitFov());
		} else if (!m_telradDisplayed) {
			movementManager->zoomTo(initialFOV);
		}

		if (isUseInitialDirectiom()) {
			movementManager->setViewDirectionJ2000(
						core->altAzToJ2000(movementManager->getInitViewingDirection(), StelCore::RefractionOff));
		}

		if (getFlagAutosetMountForCCD()) {
			StelPropertyMgr * propMgr = StelApp::getInstance().getStelPropertyManager();
			propMgr->setStelPropertyValue("StelMovementMgr.equatorialMount", equatorialMountEnabledMain);
		}

		if (guiPanel != nullptr) {
			guiPanel->foldGui();
		}
	}

	emit ccdDisplayedChanged(m_ccdDisplayed);
}

void Oculars::setCrosshairsDisplayed(bool show)
{
	if (show != m_crosshairsDisplayed) {
		m_crosshairsDisplayed = show;
		emit crosshairsDisplayedChanged(show);
	}
}

void Oculars::toggleTelrad()
{
	setTelradDisplayed(!m_telradDisplayed);
}

void Oculars::setTelradDisplayed(bool show)
{
	if (show != m_telradDisplayed) {
		m_telradDisplayed             = show;

		StelMovementMgr * movementMgr = StelApp::getInstance().getCore()->getMovementMgr();
		if (show) {
			hideUsageMessageIfDisplayed();
			setOcularDisplayed(false);
			setCCDDisplayed(false);
			// NOTE: Added special zoom level for Telrad
			if (m_scaleFOVForTelrad) {
				double fov = qMax(qMax(telradFOV[0], telradFOV[1]), qMax(telradFOV[2], telradFOV[3]));
				movementMgr->zoomTo(fov * 2.0);
			}
		} else if (isUseInitialFOV()) { // Restoration of FOV is needed?
			movementMgr->zoomTo(movementMgr->getInitFov());
		}

		if (isUseInitialDirectiom()) {
			movementMgr->setViewDirectionJ2000(StelApp::getInstance().getCore()->altAzToJ2000(
								   movementMgr->getInitViewingDirection(), StelCore::RefractionOff));
		}

		emit telradDisplayChanged(m_telradDisplayed);
	}
}

/* ****************************************************************************************************************** */
// MARK: - Private Methods
/* ****************************************************************************************************************** */
void Oculars::initializeActivationActions()
{
	QString ocularsGroup = N_("Oculars");
	actionShowOcular     = addAction("actionShow_Ocular", ocularsGroup, N_("Ocular view"), "ocularDisplayed", "Ctrl+O");
	actionMenu =
			addAction("actionShow_Ocular_Menu", ocularsGroup, N_("Oculars popup menu"), "displayPopupMenu()", "Alt+O");
	actionShowCrosshairs =
			addAction("actionShow_Ocular_Crosshairs", ocularsGroup, N_("Show crosshairs"), "crosshairsDisplayed", "Alt+C");
	actionShowSensor    = addAction("actionShow_Sensor", ocularsGroup, N_("Image sensor frame"), "ccdDisplayed");
	actionShowTelrad    = addAction("actionShow_Telrad", ocularsGroup, N_("Telrad sight"), "telradDisplayed", "Ctrl+B");
	actionConfiguration = addAction("actionOpen_Oculars_Configuration",
					ocularsGroup,
					N_("Toggle Oculars configuration window"),
					ocularDialog,
					"visible",
					""); // Allow assign shortkey
	addAction("actionShow_Oculars_GUI",
		  ocularsGroup,
		  N_("Toggle Oculars button bar"),
		  "flagGuiPanelEnabled"); // Allow assign shortkey
	// Select next telescope via keyboard
	addAction("actionShow_Telescope_Increment", ocularsGroup, N_("Select next telescope"), "incrementTelescopeIndex()");
	// Select previous telescope via keyboard
	addAction(
				"actionShow_Telescope_Decrement", ocularsGroup, N_("Select previous telescope"), "decrementTelescopeIndex()");
	// Select next eyepiece via keyboard
	addAction("actionShow_Ocular_Increment", ocularsGroup, N_("Select next eyepiece"), "incrementOcularIndex()");
	// Select previous eyepiece via keyboard
	addAction("actionShow_Ocular_Decrement", ocularsGroup, N_("Select previous eyepiece"), "decrementOcularIndex()");
	addAction("actionShow_Ocular_Rotate_Reticle_Clockwise",
		  ocularsGroup,
		  N_("Rotate reticle pattern of the eyepiece clockwise"),
		  "rotateReticleClockwise()",
		  "Alt+M");
	addAction("actionShow_Ocular_Rotate_Reticle_Counterclockwise",
		  ocularsGroup,
		  N_("Rotate reticle pattern of the eyepiece counterclockwise"),
		  "rotateReticleCounterclockwise()",
		  "Shift+Alt+M");
	addAction("actionShow_Sensor_Crop_Overlay", ocularsGroup, N_("Toggle sensor crop overlay"), "toggleCropOverlay()");
	addAction("actionShow_Sensor_Pixel_Grid", ocularsGroup, N_("Toggle sensor pixel grid"), "togglePixelGrid()");
	addAction("actionShow_Sensor_Focuser_Overlay", ocularsGroup, N_("Toggle focuser overlay"), "toggleFocuserOverlay()");

	connect(this, &Oculars::indexSelectedCCDChanged, this, &Oculars::instrumentChanged);
	connect(this, &Oculars::indexSelectedOcularChanged, this, &Oculars::instrumentChanged);
	connect(this, &Oculars::indexSelectedTelescopeChanged, this, &Oculars::instrumentChanged);
	connect(this, &Oculars::indexSelectedLensChanged, this, &Oculars::instrumentChanged);
	connect(this, &Oculars::autoLimitMagnitudeChanged, this, &Oculars::handleAutoLimitToggle);
}

auto Oculars::isBinocularDefined() -> bool
{
	bool binocularFound = false;
	for (auto * ocular : oculars) {
		if (ocular->isBinoculars()) {
			binocularFound = true;
			break;
		}
	}
	return binocularFound;
}

void Oculars::paintCCDBounds() const
{
	int        fontSize  = StelApp::getInstance().getScreenFontSize();
	auto *     core      = StelApp::getInstance().getCore();
	auto       params    = core->getCurrentStelProjectorParams();
	auto *     lens      = m_indexSelectedLens >= 0 ? lenses.at(m_indexSelectedLens) : nullptr;

	const auto projector = core->getProjection(StelCore::FrameEquinoxEqu);
	auto       screenFOV = static_cast<double>(params.fov);
	Vec2i      centerScreen(projector->getViewportPosX() + projector->getViewportWidth() / 2,
				projector->getViewportPosY() + projector->getViewportHeight() / 2);

	// draw sensor rectangle
	if (m_indexSelectedCCD > -1 && m_indexSelectedTelescope > -1) {
		auto * ccd = ccds.at(m_indexSelectedCCD);
		if (ccd != nullptr) {
			StelPainter painter(projector);
			painter.setColor(lineColor);
			painter.setFont(font);
			Telescope *  telescope   = telescopes.at(m_indexSelectedTelescope);

			const double ccdXRatio   = ccd->getActualFOVx(telescope, lens) / screenFOV;
			const double ccdYRatio   = ccd->getActualFOVy(telescope, lens) / screenFOV;

			const double fovX        = ccd->getActualFOVx(telescope, lens);
			const double fovY        = ccd->getActualFOVy(telescope, lens);

			// As the FOV is based on the narrow aspect of the screen, we need to calculate
			// height & width based soley off of that dimension.
			int          aspectIndex = 2;
			if (params.viewportXywh[2] > params.viewportXywh[3]) {
				aspectIndex = 3;
			}
			const float width =
					params.viewportXywh[aspectIndex] * static_cast<float>(ccdXRatio * params.devicePixelsPerPixel);
			const float height =
					params.viewportXywh[aspectIndex] * static_cast<float>(ccdYRatio * params.devicePixelsPerPixel);

			// Get Crop size taking into account the binning rounded to the lower limit and limiting it to sensor size
			const float actualCropOverlayX =
					(std::min(ccd->resolutionX(), m_ccdCropOverlayHSize) / ccd->binningX()) * ccd->binningX();
			const float actualCropOverlayY =
					(std::min(ccd->resolutionY(), m_ccdCropOverlayVSize) / ccd->binningY()) * ccd->binningY();
			// Calculate the size of the CCD crop overlay
			const float overlayWidth         = width * actualCropOverlayX / ccd->resolutionX();
			const float overlayHeight        = height * actualCropOverlayY / ccd->resolutionY();

			// calculate the size of a pixel in the image
			float       pixelProjectedWidth  = width / ccd->resolutionX() * ccd->binningX();
			float       pixelProjectedHeight = height / ccd->resolutionY() * ccd->binningY();

			double      polarAngle           = 0;
			// if the telescope is Equatorial derotate the field
			if (telescope->isEquatorial()) {
				Vec3d          CPos;
				Vector2<qreal> cpos = projector->getViewportCenter();
				projector->unProject(cpos[0], cpos[1], CPos);
				Vec3d CPrel(CPos);
				CPrel[2] *= 0.2;
				Vec3d crel;
				projector->project(CPrel, crel);
				polarAngle = atan2(cpos[1] - crel[1], cpos[0] - crel[0]) * (-Degrees180) / M_PI; // convert to degrees
				if (CPos[2] > 0) {
					polarAngle += Degrees90;
				} else {
					polarAngle -= Degrees90;
				}
			}

			if (getFlagAutosetMountForCCD()) {
				StelPropertyMgr * propMgr = StelApp::getInstance().getStelPropertyManager();
				propMgr->setStelPropertyValue("actionSwitch_Equatorial_Mount", telescope->isEquatorial());
				polarAngle = 0;
			}

			if (width > 0.0F && height > 0.0F) {
				QPoint     a;
				QPoint     b;
				QTransform transform =
						QTransform().translate(centerScreen[0], centerScreen[1]).rotate(-(ccd->rotationAngle() + polarAngle));
				// bottom line
				a = transform.map(QPoint(static_cast<int>(-width * 0.5F), static_cast<int>(-height * 0.5F)));
				b = transform.map(QPoint(static_cast<int>(width * 0.5F), static_cast<int>(-height * 0.5F)));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
				// top line
				a = transform.map(QPoint(static_cast<int>(-width * 0.5F), static_cast<int>(height * 0.5F)));
				b = transform.map(QPoint(static_cast<int>(width * 0.5F), static_cast<int>(height * 0.5F)));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
				// left line
				a = transform.map(QPoint(static_cast<int>(-width * 0.5F), static_cast<int>(-height * 0.5F)));
				b = transform.map(QPoint(static_cast<int>(-width * 0.5F), static_cast<int>(height * 0.5F)));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
				// right line
				a = transform.map(QPoint(static_cast<int>(width * 0.5F), static_cast<int>(height * 0.50f)));
				b = transform.map(QPoint(static_cast<int>(width * 0.5F), static_cast<int>(-height * 0.5F)));
				painter.drawLine2d(a.x(), a.y(), b.x(), b.y());

				// Tool for showing a resolution box overlay
				if (m_showCCDCropOverlay) {
					// bottom line
					a =
							transform.map(QPoint(static_cast<int>(-overlayWidth * 0.5F), static_cast<int>(-overlayHeight * 0.5F)));
					b =
							transform.map(QPoint(static_cast<int>(overlayWidth * 0.5F), static_cast<int>(-overlayHeight * 0.5F)));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// top line
					a =
							transform.map(QPoint(static_cast<int>(-overlayWidth * 0.5F), static_cast<int>(overlayHeight * 0.5F)));
					b = transform.map(QPoint(static_cast<int>(overlayWidth * 0.5F), static_cast<int>(overlayHeight * 0.5F)));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// left line
					a =
							transform.map(QPoint(static_cast<int>(-overlayWidth * 0.5F), static_cast<int>(-overlayHeight * 0.5F)));
					b =
							transform.map(QPoint(static_cast<int>(-overlayWidth * 0.5F), static_cast<int>(overlayHeight * 0.5F)));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// right line
					a = transform.map(QPoint(static_cast<int>(overlayWidth * 0.5F), static_cast<int>(overlayHeight * 0.5F)));
					b =
							transform.map(QPoint(static_cast<int>(overlayWidth * 0.5F), static_cast<int>(-overlayHeight * 0.5F)));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());

					// Tool to show full CCD grid overlay
					if (m_showCCDCropOverlayPixelGrid) {
						// vertical lines
						for (int l = 1; l < actualCropOverlayX / ccd->binningX(); l++) {
							a = transform.map(QPoint(static_cast<int>(overlayWidth * 0.5F - l * pixelProjectedWidth),
										 static_cast<int>(-overlayHeight * 0.5F)));
							b = transform.map(QPoint(static_cast<int>(overlayWidth * 0.5F - l * pixelProjectedWidth),
										 static_cast<int>(overlayHeight * 0.5F)));
							painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
						}
						// horizontal lines
						for (int l = 1; l < actualCropOverlayY / ccd->binningY(); l++) {
							a = transform.map(QPoint(static_cast<int>(-overlayWidth * 0.5F),
										 static_cast<int>(overlayHeight * 0.5F - l * pixelProjectedHeight)));
							b = transform.map(QPoint(static_cast<int>(overlayWidth * 0.5F),
										 static_cast<int>(overlayHeight * 0.5F - l * pixelProjectedHeight)));
							painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
						}
					}
				}
				if (ccd->hasOAG()) {
					const double InnerOAGRatio = ccd->getInnerOAGRadius(telescope, lens) / screenFOV;
					const double OuterOAGRatio = ccd->getOuterOAGRadius(telescope, lens) / screenFOV;
					const double prismXRatio   = ccd->getOAGActualFOVx(telescope, lens) / screenFOV;
					const int    in_oag_r =
							qRound(params.viewportXywh[aspectIndex] * InnerOAGRatio * params.devicePixelsPerPixel);
					const int out_oag_r =
							qRound(params.viewportXywh[aspectIndex] * OuterOAGRatio * params.devicePixelsPerPixel);
					const int h_width =
							qRound(params.viewportXywh[aspectIndex] * prismXRatio * params.devicePixelsPerPixel * 0.5);

					painter.drawCircle(centerScreen[0], centerScreen[1], in_oag_r);
					painter.drawCircle(centerScreen[0], centerScreen[1], out_oag_r);

					QTransform oag_transform = QTransform()
							.translate(centerScreen[0], centerScreen[1])
							.rotate(-(ccd->rotationAngle() + polarAngle + ccd->prismPosAngle()));

					// bottom line
					a = oag_transform.map(QPoint(-h_width, in_oag_r));
					b = oag_transform.map(QPoint(h_width, in_oag_r));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// top line
					a = oag_transform.map(QPoint(-h_width, out_oag_r));
					b = oag_transform.map(QPoint(h_width, out_oag_r));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// left line
					a = oag_transform.map(QPoint(-h_width, out_oag_r));
					b = oag_transform.map(QPoint(-h_width, in_oag_r));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// right line
					a = oag_transform.map(QPoint(h_width, out_oag_r));
					b = oag_transform.map(QPoint(h_width, in_oag_r));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
				}

				// Tool for planning a mosaic astrophotography: shows a small cross at center of CCD's
				// frame and equatorial coordinates for epoch J2000.0 of that center.
				// Details: https://bugs.launchpad.net/stellarium/+bug/1404695

				const double ratioLimit     = 0.25;
				const double ratioLimitCrop = 0.75;
				if (ccdXRatio >= ratioLimit || ccdYRatio >= ratioLimit) {
					// draw cross at center
					const int cross = qRound(10 * params.devicePixelsPerPixel); // use permanent size of cross (10px)
					a               = transform.map(QPoint(-cross, -cross));
					b               = transform.map(QPoint(cross, cross));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					a = transform.map(QPoint(-cross, cross));
					b = transform.map(QPoint(cross, -cross));
					painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
					// calculate coordinates of the center and show it
					Vec3d centerPosition;
					projector->unProject(centerScreen[0], centerScreen[1], centerPosition);
					double  cx;
					double  cy;
					QString cxt;
					QString cyt;
					StelUtils::rectToSphe(
								&cx,
								&cy,
								core->equinoxEquToJ2000(centerPosition,
											StelCore::RefractionOff)); // Calculate RA/DE (J2000.0) and show it...
					bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
					if (withDecimalDegree) {
						cxt = StelUtils::radToDecDegStr(cx, 5, false, true);
						cyt = StelUtils::radToDecDegStr(cy);
					} else {
						cxt = StelUtils::radToHmsStr(cx, true);
						cyt = StelUtils::radToDmsStr(cy, true);
					}
					auto scaleFactor = static_cast<float>(1.2 * params.devicePixelsPerPixel);
					// Coordinates of center of visible field of view for CCD (red rectangle)
					auto coords      = QString("%1:").arg(qc_("RA/Dec (J2000.0) of cross", "abbreviated in the plugin"));
					a = transform.map(QPoint(qRound(-width * 0.5F), qRound(height * 0.5F + 5.f + fontSize * scaleFactor)));
					painter.drawText(a.x(), a.y(), coords, static_cast<float>(-(ccd->rotationAngle() + polarAngle)));
					coords = QString("%1/%2").arg(cxt.simplified()).arg(cyt);
					a      = transform.map(QPoint(qRound(-width * 0.5F), qRound(height * 0.5F + 5.f)));
					painter.drawText(a.x(), a.y(), coords, static_cast<float>(-(ccd->rotationAngle() + polarAngle)));
					// Dimensions of visible field of view for CCD (red rectangle)
					a = transform.map(QPoint(qRound(-width * 0.5F), qRound(-height * 0.5F - fontSize * scaleFactor)));
					painter.drawText(a.x(),
							 a.y(),
							 getDimensionsString(fovX, fovY),
							 static_cast<float>(-(ccd->rotationAngle() + polarAngle)));
					// Horizontal and vertical scales of visible field of view for CCD (red rectangle)
					// TRANSLATORS: Unit of measure for scale - arc-seconds per pixel
					QString unit   = q_("\"/px");
					QString scales = QString("%1%3 %4 %2%3")
							.arg(QString::number(fovX * 3600 * ccd->binningX() / ccd->resolutionX(), 'f', 4))
							.arg(QString::number(fovY * 3600 * ccd->binningY() / ccd->resolutionY(), 'f', 4))
							.arg(unit)
							.arg(QChar(0x00D7));
					a = transform.map(QPoint(qRound(width * 0.5F - painter.getFontMetrics().boundingRect(scales).width() *
									params.devicePixelsPerPixel),
								 qRound(-height * 0.5F - fontSize * scaleFactor)));
					painter.drawText(a.x(), a.y(), scales, static_cast<float>(-(ccd->rotationAngle() + polarAngle)));
					// Rotation angle of visible field of view for CCD (red rectangle)
					QString angle = QString("%1%2").arg(QString::number(ccd->rotationAngle(), 'f', 1)).arg(QChar(0x00B0));
					a = transform.map(QPoint(qRound(width * 0.5F - painter.getFontMetrics().boundingRect(angle).width() *
									params.devicePixelsPerPixel),
								 qRound(height * 0.5F + 5.f)));
					painter.drawText(a.x(), a.y(), angle, static_cast<float>(-(ccd->rotationAngle() + polarAngle)));

					if (m_showCCDCropOverlay && (ccdXRatio >= ratioLimitCrop || ccdYRatio >= ratioLimitCrop)) {
						// show the CCD crop overlay text
						QString resolutionOverlayText = QString("%1%3 %4 %2%3")
								.arg(QString::number(actualCropOverlayX, 'd', 0))
								.arg(QString::number(actualCropOverlayY, 'd', 0))
								.arg(qc_("px", "pixel"))
								.arg(QChar(0x00D7));
						if (actualCropOverlayX != m_ccdCropOverlayHSize || actualCropOverlayY != m_ccdCropOverlayVSize)
							resolutionOverlayText.append(" [*]");
						a = transform.map(QPoint(
									  qRound(overlayWidth * 0.5F - painter.getFontMetrics().boundingRect(resolutionOverlayText).width() *
										 params.devicePixelsPerPixel),
									  qRound(-overlayHeight * 0.5F - fontSize * scaleFactor)));
						painter.drawText(
									a.x(), a.y(), resolutionOverlayText, static_cast<float>(-(ccd->rotationAngle() + polarAngle)));
					}
				}

				if (isShowFocuserOverlay()) {
					painter.setColor(focuserColor);
					if (displayFocuserOverlaySmall()) {
						painter.drawCircle(centerScreen[0],
								centerScreen[1],
								qRound(params.viewportXywh[aspectIndex] *
								       (0.5 * CCD::getFocuserFOV(telescope, lens, 1.25) / screenFOV) *
								       params.devicePixelsPerPixel));
					}
					if (isDisplayFocuserOverlayMedium()) {
						painter.drawCircle(centerScreen[0],
								centerScreen[1],
								qRound(params.viewportXywh[aspectIndex] *
								       (0.5 * CCD::getFocuserFOV(telescope, lens, 2.0) / screenFOV) *
								       params.devicePixelsPerPixel));
					}
					if (isDisplayFocuserOverlayLarge()) {
						painter.drawCircle(centerScreen[0],
								centerScreen[1],
								qRound(params.viewportXywh[aspectIndex] *
								       (0.5 * CCD::getFocuserFOV(telescope, lens, 3.3) / screenFOV) *
								       params.devicePixelsPerPixel));
					}
				}
			}
		}
	}
}

void Oculars::paintCrosshairs() const
{
	StelCore *                         core      = StelApp::getInstance().getCore();
	const StelProjectorP               projector = core->getProjection(StelCore::FrameEquinoxEqu);
	StelProjector::StelProjectorParams params    = core->getCurrentStelProjectorParams();
	// Center of screen
	Vec2i                              centerScreen(projector->getViewportPosX() + projector->getViewportWidth() / 2,
							projector->getViewportPosY() + projector->getViewportHeight() / 2);
	float                              length = 0.5F * static_cast<float>(params.viewportFovDiameter);
	// See if we need to scale the length
	if (m_scaleImageCircle && oculars.at(m_indexSelectedOcular)->apparentFOV() > 0.0 &&
			!oculars.at(m_indexSelectedOcular)->isBinoculars()) {
		length *= static_cast<float>(oculars.at(m_indexSelectedOcular)->apparentFOV() / maxEyepieceAngle);
	}
	length *= static_cast<float>(params.devicePixelsPerPixel);
	double polarAngle = 0.;
	if (isAlignCrosshair()) {
		Vec3d          CPos;
		Vector2<qreal> cpos = projector->getViewportCenter();
		projector->unProject(cpos[0], cpos[1], CPos);
		Vec3d CPrel(CPos);
		CPrel[2] *= 0.2;
		Vec3d crel;
		projector->project(CPrel, crel);
		polarAngle = atan2(cpos[1] - crel[1], cpos[0] - crel[0]) * (-Degrees180) / M_PI; // convert to degrees
		if (CPos[2] > 0) {
			polarAngle += Degrees90;
		} else {
			polarAngle -= Degrees90;
		}
	}
	// Draw the lines
	StelPainter painter(projector);
	painter.setColor(lineColor);
	QPoint     a;
	QPoint     b;
	int        hw           = qRound(length);
	QTransform ch_transform = QTransform().translate(centerScreen[0], centerScreen[1]).rotate(-polarAngle);
	a                       = ch_transform.map(QPoint(0, -hw));
	b                       = ch_transform.map(QPoint(0, hw));
	painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
	a = ch_transform.map(QPoint(-hw, 0));
	b = ch_transform.map(QPoint(hw, 0));
	painter.drawLine2d(a.x(), a.y(), b.x(), b.y());
}

void Oculars::paintTelrad() const
{
	if (!m_ocularDisplayed) {
		StelCore *           core      = StelApp::getInstance().getCore();
		const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);
		// StelPainter drawing
		StelPainter          painter(projector);
		painter.setColor(lineColor);
		Vec2i       centerScreen(projector->getViewportPosX() + projector->getViewportWidth() / 2,
					 projector->getViewportPosY() + projector->getViewportHeight() / 2);
		const float pixelsPerRad = projector->getPixelPerRadAtCenter(); // * params.devicePixelsPerPixel;
		if (telradFOV[0] > 0.f) {
			painter.drawCircle(
						centerScreen[0], centerScreen[1], 0.5F * pixelsPerRad * static_cast<float>(M_PI / 180) * (telradFOV[0]));
		}
		if (telradFOV[1] > 0.f) {
			painter.drawCircle(
						centerScreen[0], centerScreen[1], 0.5F * pixelsPerRad * static_cast<float>(M_PI / 180) * (telradFOV[1]));
		}
		if (telradFOV[2] > 0.f) {
			painter.drawCircle(
						centerScreen[0], centerScreen[1], 0.5F * pixelsPerRad * static_cast<float>(M_PI / 180) * (telradFOV[2]));
		}
		if (telradFOV[3] > 0.f) {
			painter.drawCircle(
						centerScreen[0], centerScreen[1], 0.5F * pixelsPerRad * static_cast<float>(M_PI / 180) * (telradFOV[3]));
		}
	}
}

void Oculars::paintOcularMask(StelCore * core) const
{
	if (oculars.at(m_indexSelectedOcular)->hasPermanentCrosshair()) {
		paintCrosshairs();
	}

	const StelProjectorP               prj = core->getProjection(StelCore::FrameAltAz);
	StelPainter                        painter(prj);
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();

	double                             inner  = 0.5 * params.viewportFovDiameter * params.devicePixelsPerPixel;
	// See if we need to scale the mask
	if (m_scaleImageCircle && oculars.at(m_indexSelectedOcular)->apparentFOV() > 0.0 &&
			!oculars.at(m_indexSelectedOcular)->isBinoculars()) {
		inner = oculars.at(m_indexSelectedOcular)->apparentFOV() * inner / maxEyepieceAngle;
	}
	Vec2i centerScreen(prj->getViewportPosX() + prj->getViewportWidth() / 2,
			   prj->getViewportPosY() + prj->getViewportHeight() / 2);

	painter.setBlending(true);
	// Paint the reticale, if needed
	if (!textureReticle.isNull()) {
		painter.setColor(lineColor);
		textureReticle->bind();
		painter.drawSprite2dMode(centerScreen[0],
				centerScreen[1],
				static_cast<float>(inner / params.devicePixelsPerPixel),
				static_cast<float>(reticleRotation));
	}

	const float alpha = useSemiTransparency() ? transparencyLevel() * 0.01F : 1.0F;
	painter.setColor(0.0F, 0.0F, 0.0F, alpha);

	auto    outerRadius = static_cast<GLfloat>(params.viewportXywh[2] * params.devicePixelsPerPixel +
			params.viewportXywh[3] * params.devicePixelsPerPixel);
	GLint   slices      = 239;

	//   GLfloat sinCache2[240];
	std::array<GLfloat, 240> sinCache{};
	//   GLfloat cosCache[240];
	std::array<GLfloat, 240> cosCache{};
	GLfloat vertices[(240 + 1) * 2][3];
	GLfloat deltaRadius = 0.0;
	GLfloat radiusHigh = 0.0;

	/* Compute length (needed for normal calculations) */
	deltaRadius = outerRadius - static_cast<GLfloat>(inner);

	/* Cache is the vertex locations cache */
	for (int i = 0; i <= slices; i++) {
		GLfloat angle = static_cast<GLfloat>(M_PI * 2.0) * i / slices;
		sinCache[i]   = static_cast<GLfloat>(sinf(angle));
		cosCache[i]   = static_cast<GLfloat>(cosf(angle));
	}

	sinCache[slices] = sinCache[0];
	cosCache[slices] = cosCache[0];

	/* Enable arrays */
	painter.enableClientStates(true);
	painter.setVertexPointer(3, GL_FLOAT, vertices);

	radiusHigh = outerRadius - deltaRadius;
	for (int i = 0; i <= slices; i++) {
		vertices[i * 2][0]     = centerScreen[0] + outerRadius * sinCache[i];
		vertices[i * 2][1]     = centerScreen[1] + outerRadius * cosCache[i];
		vertices[i * 2][2]     = 0.0;
		vertices[i * 2 + 1][0] = centerScreen[0] + radiusHigh * sinCache[i];
		vertices[i * 2 + 1][1] = centerScreen[1] + radiusHigh * cosCache[i];
		vertices[i * 2 + 1][2] = 0.0;
	}
	painter.drawFromArray(StelPainter::TriangleStrip, (slices + 1) * 2, 0, false);
	painter.enableClientStates(false);

	if (isShowOcularContour()) {
		painter.setColor(lineColor);
		painter.drawCircle(centerScreen[0], centerScreen[1], static_cast<float>(inner));
	}

	if (isShowCardinalPoints()) {
		// Compute polar angle for cardinals and show it
		const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);
		Vec3d                CPos;
		Vector2<qreal>       cpos = projector->getViewportCenter();
		projector->unProject(cpos[0], cpos[1], CPos);
		Vec3d CPrel(CPos);
		CPrel[2] *= 0.2;
		Vec3d crel;
		projector->project(CPrel, crel);
		double polarAngle = atan2(cpos[1] - crel[1], cpos[0] - crel[0]) * (-Degrees180) / M_PI; // convert to degrees
		if (CPos[2] > 0) {
			polarAngle += Degrees90;
		} else {
			polarAngle -= Degrees90;
		}

		painter.setColor(lineColor);
		bool flipHorizontal = core->getFlipHorz();
		bool flipVertical   = core->getFlipVert();
		if (m_indexSelectedTelescope > 0) {
			// If a telescope is in use, that over-rides the system
			flipHorizontal = telescopes.at(m_indexSelectedTelescope)->isHFlipped();
			flipVertical   = telescopes.at(m_indexSelectedTelescope)->isVFlipped();
		}
		if (flipHorizontal && !flipVertical) {
			textureProtractorFlipH->bind();
		} else if (!flipHorizontal && flipVertical) {
			textureProtractorFlipV->bind();
		} else if (flipHorizontal && flipVertical) {
			textureProtractorFlipHV->bind();
		} else {
			textureProtractor->bind();
		}

		core->setFlipHorz(flipHorizontal);
		core->setFlipVert(flipVertical);
		painter.drawSprite2dMode(centerScreen[0],
				centerScreen[1],
				static_cast<float>(inner / params.devicePixelsPerPixel),
				static_cast<float>(-polarAngle));
	}
}

void Oculars::paintText(const StelCore * core) const
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	StelPainter          painter(prj);

	// Get the current instruments
	CCD *                ccd = nullptr;
	if (m_indexSelectedCCD != -1) {
		ccd = ccds.at(m_indexSelectedCCD);
	}
	Ocular * ocular = nullptr;
	if (m_indexSelectedOcular != -1) {
		ocular = oculars.at(m_indexSelectedOcular);
	}
	Telescope * telescope = nullptr;
	if (m_indexSelectedTelescope != -1) {
		telescope = telescopes.at(m_indexSelectedTelescope);
	}
	Lens * lens = selectedLens();

	// set up the color and the GL state
	painter.setColor(textColor);
	painter.setBlending(true);

	// Get the X & Y positions, and the line height
	painter.setFont(font);
	QString                            widthString     = "MMMMMMMMMMMMMMMMMMMMM";
	const double                       insetFromRHS    = painter.getFontMetrics().boundingRect(widthString).width();
	StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
	int yPositionOffset = qRound(projectorParams.viewportXywh[3] * projectorParams.viewportCenterOffset[1]);
	int xPosition       = qRound(projectorParams.devicePixelsPerPixel * projectorParams.viewportXywh[2] - insetFromRHS);
	int yPosition =
			qRound(projectorParams.devicePixelsPerPixel * projectorParams.viewportXywh[3] - yPositionOffset - 20);
	const int lineHeight = painter.getFontMetrics().height();

	// The Ocular
	if (m_ocularDisplayed && ocular != nullptr) {
		QString ocularNumberLabel;
		QString name       = ocular->name();
		QString ocularI18n = q_("Ocular");
		if (ocular->isBinoculars()) {
			ocularI18n = q_("Binocular");
		}
		if (name.isEmpty()) {
			ocularNumberLabel = QString("%1 #%2").arg(ocularI18n).arg(m_indexSelectedOcular);
		} else {
			ocularNumberLabel = QString("%1 #%2: %3").arg(ocularI18n).arg(m_indexSelectedOcular).arg(name);
		}
		// The name of the ocular could be really long.
		if (name.length() > widthString.length()) {
			xPosition -= qRound(insetFromRHS * 0.5);
		}
		painter.drawText(xPosition, yPosition, ocularNumberLabel);
		yPosition -= lineHeight;

		if (!ocular->isBinoculars()) {
			// TRANSLATORS: FL = Focal length
			QString eFocalLengthLabel =
					QString(q_("Ocular FL: %1 mm")).arg(QString::number(ocular->effectiveFocalLength(), 'f', 1));
			painter.drawText(xPosition, yPosition, eFocalLengthLabel);
			yPosition -= lineHeight;

			QString ocularFov = QString::number(ocular->apparentFOV(), 'f', 2);
			ocularFov.append(QChar(0x00B0)); // Degree sign
			// TRANSLATORS: aFOV = apparent field of view
			QString ocularFOVLabel = QString(q_("Ocular aFOV: %1")).arg(ocularFov);
			painter.drawText(xPosition, yPosition, ocularFOVLabel);
			yPosition -= lineHeight;

			QString lensNumberLabel;
			// Barlow and Shapley lens
			if (lens != nullptr) // it's null if lens is not selected (lens index = -1)
			{
				QString lensName = lens->name();
				if (lensName.isEmpty()) {
					lensNumberLabel = QString(q_("Lens #%1")).arg(m_indexSelectedLens);
				} else {
					lensNumberLabel = QString(q_("Lens #%1: %2")).arg(m_indexSelectedLens).arg(lensName);
				}
			} else {
				lensNumberLabel = QString(q_("Lens: none"));
			}
			painter.drawText(xPosition, yPosition, lensNumberLabel);
			yPosition -= lineHeight;

			if (telescope != nullptr) {
				QString telescopeName   = telescope->name();
				QString telescopeString = "";

				if (telescopeName.isEmpty())
					telescopeString = QString("%1").arg(m_indexSelectedTelescope);
				else
					telescopeString = QString("%1: %2").arg(m_indexSelectedTelescope).arg(telescopeName);

				painter.drawText(xPosition, yPosition, QString(q_("Telescope #%1")).arg(telescopeString));
				yPosition -= lineHeight;

				// General info
				double  mag       = ocular->magnification(telescope, lens);
				QString magString = QString::number(mag, 'f', 1);
				magString.append(QChar(0x02E3)); // Was 0x00D7
				magString.append(QString(" (%1D)").arg(QString::number(mag / telescope->diameter(), 'f', 2)));

				painter.drawText(xPosition, yPosition, QString(q_("Magnification: %1")).arg(magString));
				yPosition -= lineHeight;

				if (mag > 0) {
					QString exitPupil = QString::number(telescope->diameter() / mag, 'f', 2);

					painter.drawText(xPosition, yPosition, QString(q_("Exit pupil: %1 mm")).arg(exitPupil));
					yPosition -= lineHeight;
				}

				QString fovString = QString::number(ocular->actualFOV(telescope, lens), 'f', 5);
				fovString.append(QChar(0x00B0)); // Degree sign

				painter.drawText(xPosition, yPosition, QString(q_("FOV: %1")).arg(fovString));
			}
		}
	}

	// The CCD
	if (m_ccdDisplayed && ccd != nullptr) {
		QString ccdSensorLabel;
		QString ccdInfoLabel;
		QString ccdBinningInfo;
		QString name          = "";
		QString telescopeName = "";
		double  fovX          = 0.0;
		double  fovY          = 0.0;
		if (telescope != nullptr) {
			fovX          = ccd->getActualFOVx(telescope, lens);
			fovY          = ccd->getActualFOVy(telescope, lens);
			name          = ccd->name();
			telescopeName = telescope->name();
		}

		ccdInfoLabel = QString(q_("Dimensions: %1")).arg(getDimensionsString(fovX, fovY));
		ccdBinningInfo =
				QString("%1: %2 %4 %3").arg(q_("Binning")).arg(ccd->binningX()).arg(ccd->binningY()).arg(QChar(0x00D7));

		if (name.isEmpty()) {
			ccdSensorLabel = QString(q_("Sensor #%1")).arg(m_indexSelectedCCD);
		} else {
			ccdSensorLabel = QString(q_("Sensor #%1: %2")).arg(m_indexSelectedCCD).arg(name);
		}
		// The telescope
		QString telescopeNumberLabel;
		if (telescopeName.isEmpty()) {
			telescopeNumberLabel = QString(q_("Telescope #%1")).arg(m_indexSelectedTelescope);
		} else {
			telescopeNumberLabel = QString(q_("Telescope #%1: %2")).arg(m_indexSelectedTelescope).arg(telescopeName);
		}
		painter.drawText(xPosition, yPosition, ccdSensorLabel);
		yPosition -= lineHeight;
		painter.drawText(xPosition, yPosition, ccdInfoLabel);
		yPosition -= lineHeight;
		painter.drawText(xPosition, yPosition, ccdBinningInfo);
		yPosition -= lineHeight;
		painter.drawText(xPosition, yPosition, telescopeNumberLabel);
	}
}

void Oculars::validateAndLoadIniFile()
{
	// Ensure the module directory exists
	StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir() + "/modules/Oculars");
	auto    flags         = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory | StelFileMgr::Writable);
	QString ocularIniPath = StelFileMgr::findFile("modules/Oculars/", flags) + "ocular.ini";
	if (ocularIniPath.isEmpty()) {
		return;
	}

	// If the ini file does not already exist, create it from the resource in the QT resource
	if (!QFileInfo::exists(ocularIniPath)) {
		QFile src(":/ocular/default_ocular.ini");
		if (!src.copy(ocularIniPath)) {
			qWarning() << "Oculars::validateIniFile cannot copy default_ocular.ini resource to [non-existing] " +
				      ocularIniPath;
		} else {
			qDebug() << "Oculars::validateAndLoadIniFile() copied default_ocular.ini to "
                  << QDir::toNativeSeparators(ocularIniPath);
			// The resource is read only, and the new file inherits this, so set write-able.
			QFile dest(ocularIniPath);
			dest.setPermissions(dest.permissions() | QFile::WriteOwner);
		}
	} else {
		qDebug() << "Oculars::validateAndLoadIniFile() ocular.ini exists at: " << QDir::toNativeSeparators(ocularIniPath)
			 << ". Checking version...";
		QSettings   mySettings(ocularIniPath, QSettings::IniFormat);
		const float ocularsVersion = mySettings.value("oculars_version", 0.0).toFloat();
		qWarning() << "Oculars::validateAndLoadIniFile() found existing ini file version " << ocularsVersion;

		if (ocularsVersion < MIN_OCULARS_INI_VERSION) {
			qWarning() << "Oculars::validateAndLoadIniFile() existing ini file version " << ocularsVersion
				   << " too old to use; required version is " << MIN_OCULARS_INI_VERSION << ". Copying over new one.";
			// delete last "old" file, if it exists
			QFile deleteFile(ocularIniPath + ".old");
			deleteFile.remove();

			// Rename the old one, and copy over a new one
			QFile oldFile(ocularIniPath);
			if (!oldFile.rename(ocularIniPath + ".old")) {
				qWarning()
						<< "Oculars::validateAndLoadIniFile() cannot move ocular.ini resource to ocular.ini.old at path  " +
						   QDir::toNativeSeparators(ocularIniPath);
			} else {
				qWarning() << "Oculars::validateAndLoadIniFile() ocular.ini resource renamed to ocular.ini.old at path  " +
					      QDir::toNativeSeparators(ocularIniPath);
				QFile src(":/ocular/default_ocular.ini");
				if (!src.copy(ocularIniPath)) {
					qWarning() << "Oculars::validateIniFile cannot copy default_ocular.ini resource to [non-existing] " +
						      QDir::toNativeSeparators(ocularIniPath);
				} else {
					qDebug() << "Oculars::validateAndLoadIniFile() copied default_ocular.ini to "
                        << QDir::toNativeSeparators(ocularIniPath);
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
	Q_ASSERT(m_ocularDisplayed == false);
	StelCore *        core            = StelApp::getInstance().getCore();
	StelMovementMgr * movementManager = core->getMovementMgr();
	StelSkyDrawer *   skyDrawer       = core->getSkyDrawer();

	if (m_hideGridsAndLines) {
		toggleLines(true);
	}

	StelApp::getInstance().getStelPropertyManager()->setStelPropertyValue("MilkyWay.saturation", milkyWaySaturation);
	disconnect(skyDrawer, &StelSkyDrawer::customStarMagLimitChanged, this, &Oculars::setMagLimitStarsOcularsManual);
	// restore values, but keep current to enable toggling.
	if (!isAutoLimitMagnitude()) {
		flagLimitStarsOculars = skyDrawer->getFlagStarMagnitudeLimit();
		magLimitStarsOculars  = skyDrawer->getCustomStarMagnitudeLimit();
	}
	skyDrawer->setCustomStarMagnitudeLimit(magLimitStarsMain);
	skyDrawer->setFlagStarMagnitudeLimit(flagLimitStarsMain);
	relativeStarScaleOculars = skyDrawer->getRelativeStarScale();
	absoluteStarScaleOculars = skyDrawer->getAbsoluteStarScale();
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

	if (isUseInitialFOV()) {
		movementManager->zoomTo(movementManager->getInitFov());
	} else if (!m_telradDisplayed) {
		movementManager->zoomTo(initialFOV);
	}

	if (isUseInitialDirectiom()) {
		movementManager->setViewDirectionJ2000(
					core->altAzToJ2000(movementManager->getInitViewingDirection(), StelCore::RefractionOff));
	}
}

void Oculars::zoom(bool zoomedIn)
{
	if (m_ocularDisplayed && m_indexSelectedOcular == -1) {
		// The user cycled out the selected ocular
		m_ocularDisplayed = false;
	}

	if (m_ocularDisplayed) {
		if (!zoomedIn) {
			StelCore *        core    = StelApp::getInstance().getCore();
			StelPropertyMgr * propMgr = StelApp::getInstance().getStelPropertyManager();

			if (m_hideGridsAndLines) {
				// Store current state for later resetting
				flagGridLinesDisplayedMain = propMgr->getStelPropertyValue("GridLinesMgr.gridlinesDisplayed").toBool();
				flagCardinalPointsMain = propMgr->getStelPropertyValue("LandscapeMgr.cardinalsPointsDisplayed").toBool();
				flagConstellationLinesMain = propMgr->getStelPropertyValue("ConstellationMgr.linesDisplayed").toBool();
				flagConstellationBoundariesMain =
						propMgr->getStelPropertyValue("ConstellationMgr.boundariesDisplayed").toBool();
				flagAsterismLinesMain   = propMgr->getStelPropertyValue("AsterismMgr.linesDisplayed").toBool();
				flagRayHelpersLinesMain = propMgr->getStelPropertyValue("AsterismMgr.rayHelpersDisplayed").toBool();
			}

			StelSkyDrawer * skyDrawer         = core->getSkyDrawer();
			// Current state
			flagAdaptationMain                = skyDrawer->getFlagLuminanceAdaptation();
			flagLimitStarsMain                = skyDrawer->getFlagStarMagnitudeLimit();
			flagLimitPlanetsMain              = skyDrawer->getFlagPlanetMagnitudeLimit();
			flagLimitDSOsMain                 = skyDrawer->getFlagNebulaMagnitudeLimit();
			magLimitStarsMain                 = skyDrawer->getCustomStarMagnitudeLimit();
			magLimitPlanetsMain               = skyDrawer->getCustomPlanetMagnitudeLimit();
			magLimitDSOsMain                  = skyDrawer->getCustomNebulaMagnitudeLimit();
			relativeStarScaleMain             = skyDrawer->getRelativeStarScale();
			absoluteStarScaleMain             = skyDrawer->getAbsoluteStarScale();

			flagMoonScaleMain                 = propMgr->getStelPropertyValue("SolarSystem.flagMoonScale").toBool();
			flagMinorBodiesScaleMain          = propMgr->getStelPropertyValue("SolarSystem.flagMinorBodyScale").toBool();

			milkyWaySaturation                = propMgr->getStelPropertyValue("MilkyWay.saturation").toDouble();

			flipHorzMain                      = core->getFlipHorz();
			flipVertMain                      = core->getFlipVert();

			StelMovementMgr * movementManager = core->getMovementMgr();
			initialFOV                        = movementManager->getCurrentFov();
		}

		// set new state
		zoomOcular();
	} else {
		// reset to original state
		unzoomOcular();
	}
}

void Oculars::toggleLines(bool visible) const
{
	if (m_telradDisplayed) {
		return;
	}

	StelPropertyMgr * propMgr = StelApp::getInstance().getStelPropertyManager();

	if (visible) {
		propMgr->setStelPropertyValue("GridLinesMgr.gridlinesDisplayed", flagGridLinesDisplayedMain);
		propMgr->setStelPropertyValue("LandscapeMgr.cardinalsPointsDisplayed", flagCardinalPointsMain);
		propMgr->setStelPropertyValue("ConstellationMgr.linesDisplayed", flagConstellationLinesMain);
		propMgr->setStelPropertyValue("ConstellationMgr.boundariesDisplayed", flagConstellationBoundariesMain);
		propMgr->setStelPropertyValue("AsterismMgr.linesDisplayed", flagAsterismLinesMain);
		propMgr->setStelPropertyValue("AsterismMgr.rayHelpersDisplayed", flagRayHelpersLinesMain);
	} else {
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
	Q_ASSERT(m_ocularDisplayed == true);
	StelCore *        core            = StelApp::getInstance().getCore();
	StelMovementMgr * movementManager = core->getMovementMgr();
	StelSkyDrawer *   skyDrawer       = core->getSkyDrawer();

	if (m_hideGridsAndLines) {
		toggleLines(false);
	}

	skyDrawer->setFlagLuminanceAdaptation(false);
	StelApp::getInstance().getStelPropertyManager()->setStelPropertyValue("MilkyWay.saturation", 0.f);

	GETSTELMODULE(SolarSystem)->setFlagMoonScale(false);
	GETSTELMODULE(SolarSystem)->setFlagMinorBodyScale(false);

	movementManager->setFlagTracking(true);
	movementManager->setFlagEnableZoomKeys(false);
	movementManager->setFlagEnableMouseNavigation(false);

	// We won't always have a selected object
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected()) {
		StelObjectP selectedObject = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0];
		movementManager->moveToJ2000(selectedObject->getEquinoxEquatorialPos(core),
					     movementManager->mountFrameToJ2000(Vec3d(0., 0., 1.)),
					     0.0,
					     StelMovementMgr::ZoomIn);
	}

	// Set the screen display
	Ocular *    ocular    = oculars.at(m_indexSelectedOcular);
	Telescope * telescope = nullptr;
	Lens *      lens      = nullptr;
	// Only consider flip is we're not binoculars
	if (ocular->isBinoculars()) {
		core->setFlipHorz(false);
		core->setFlipVert(false);
	} else {
		if (m_indexSelectedLens >= 0) {
			lens = lenses.at(m_indexSelectedLens);
		}
		telescope = telescopes.at(m_indexSelectedTelescope);
		core->setFlipHorz(telescope->isHFlipped());
		core->setFlipVert(telescope->isVFlipped());
	}

	// Change relative and absolute scales for stars
	relativeStarScaleMain = skyDrawer->getRelativeStarScale();
	skyDrawer->setRelativeStarScale(relativeStarScaleOculars);
	absoluteStarScaleMain = skyDrawer->getAbsoluteStarScale();
	skyDrawer->setAbsoluteStarScale(absoluteStarScaleOculars);

	// Limit stars and DSOs	magnitude. Either compute limiting magnitude for the telescope/ocular,
	// or just use the custom oculars mode value.

	// TODO: set lim. mag without also activating flag it if it should not be activated.

	double limitMag = magLimitStarsOculars;
	if (isAutoLimitMagnitude() || flagLimitStarsOculars) {
		if (isAutoLimitMagnitude()) {
			disconnect(skyDrawer,
				   &StelSkyDrawer::customStarMagLimitChanged,
				   this,
				   &Oculars::setMagLimitStarsOcularsManual); // we want to keep the old manual value.
			limitMag = computeLimitMagnitude(ocular, telescope);
			// TODO: Is it really good to apply the star formula to DSO?
			skyDrawer->setFlagNebulaMagnitudeLimit(true);
			skyDrawer->setCustomNebulaMagnitudeLimit(limitMag);
		} else { // It's possible that the user changes the custom magnitude while viewing, and then changes the ocular.
			// Therefore we need a temporary connection.
			connect(skyDrawer, &StelSkyDrawer::customStarMagLimitChanged, this, &Oculars::setMagLimitStarsOcularsManual);
		}
		skyDrawer->setFlagStarMagnitudeLimit(true);
	}
	skyDrawer->setCustomStarMagnitudeLimit(limitMag);

	actualFOV = ocular->actualFOV(telescope, lens);
	// See if the mask was scaled; if so, correct the actualFOV.
	if (m_scaleImageCircle && ocular->apparentFOV() > 0.0 && !ocular->isBinoculars()) {
		actualFOV = maxEyepieceAngle * actualFOV / ocular->apparentFOV();
	}
	movementManager->zoomTo(actualFOV, 0.0f);
}

void Oculars::hideUsageMessageIfDisplayed()
{
	if (usageMessageLabelID > -1) {
		auto * labelManager = GETSTELMODULE(LabelMgr);
		labelManager->setLabelShow(usageMessageLabelID, false);
		labelManager->deleteLabel(usageMessageLabelID);
		usageMessageLabelID = -1;
	}
}

auto Oculars::selectedLens() const -> Lens *
{
	if (m_indexSelectedLens >= 0 && m_indexSelectedLens < lenses.count()) {
		return lenses.at(m_indexSelectedLens);
	}
	return nullptr;
}

auto Oculars::addLensSubmenu(QMenu * parent) -> QMenu *
{
	Q_ASSERT(parent);

	QMenu * submenu = new QMenu(q_("&Lens"), parent);
	submenu->addAction(q_("&Previous lens"), this, &Oculars::decrementLensIndex);
	submenu->addAction(q_("&Next lens"), this, &Oculars::incrementLensIndex);
	submenu->addSeparator();
	submenu->addAction(q_("None"), this, &Oculars::disableLens);

	for (int index = 0; index < lenses.count(); ++index) {
		QString label;
		if (index < 10) {
			label = QString("&%1: %2").arg(index).arg(lenses.at(index)->name());
		} else {
			label = lenses.at(index)->name();
		}
		QAction * action = submenu->addAction(label, [=]() { setIndexSelectedLens(index); });
		if (index == m_indexSelectedLens) {
			action->setCheckable(true);
			action->setChecked(true);
		}
	}
	return submenu;
}

QMenu * Oculars::addTelescopeSubmenu(QMenu * parent)
{
	Q_ASSERT(parent);

	QMenu * submenu = new QMenu(q_("&Telescope"), parent);
	submenu->addAction(q_("&Previous telescope"), this, &Oculars::decrementTelescopeIndex);
	submenu->addAction(q_("&Next telescope"), this, &Oculars::incrementTelescopeIndex);
	submenu->addSeparator();
	for (int index = 0; index < telescopes.count(); ++index) {
		QString label;
		if (index < 10) {
			label = QString("&%1: %2").arg(index).arg(telescopes.at(index)->name());
		} else {
			label = telescopes.at(index)->name();
		}
		QAction * action = submenu->addAction(label, [=]() { setIndexSelectedTelescope(index); });
		if (index == m_indexSelectedTelescope) {
			action->setCheckable(true);
			action->setChecked(true);
		}
	}

	return submenu;
}

void Oculars::setUseDecimalDegrees(const bool b)
{
	m_useDecimalDegrees = b;
	settings->setValue("use_decimal_degrees", !b);
	settings->sync();
	emit useDecimalDegreesChanged(b);
}

auto Oculars::isUseDecimalDegrees() const -> bool
{
	return m_useDecimalDegrees;
}

void Oculars::setRequireSelectionToZoom(const bool b)
{
	m_requireSelectionToZoom = b;
	settings->setValue("require_selection_to_zoom", b);
	settings->sync();
	emit requireSelectionToZoomChanged(b);
}

auto Oculars::isRequireSelectionToZoom() const -> bool
{
	return m_requireSelectionToZoom;
}

void Oculars::setAutoLimitMagnitude(const bool b)
{
	m_autoLimitMagnitude = b;
	settings->setValue("autolimit_stellar_magnitude", b);
	settings->sync();
	emit autoLimitMagnitudeChanged(b);
}

auto Oculars::isAutoLimitMagnitude() const -> bool
{
	return m_autoLimitMagnitude;
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

void Oculars::setUseInitialFOV(const bool b)
{
	m_useInitialFOV = b;
	settings->setValue("use_initial_fov", b);
	settings->sync();
	emit useInitialFOVChanged(b);
}

auto Oculars::isUseInitialFOV() const -> bool
{
	return m_useInitialFOV;
}

void Oculars::setUseInitialDiretion(const bool b)
{
	m_useInitialDirection = b;
	settings->setValue("use_initial_direction", b);
	settings->sync();
	emit useInitialDirectiomChanged(b);
}

auto Oculars::isUseInitialDirectiom() const -> bool
{
	return m_useInitialDirection;
}

void Oculars::setFlagAutosetMountForCCD(const bool b)
{
	flagAutosetMountForCCD = b;
	settings->setValue("use_mount_autoset", b);
	settings->sync();

	if (!b) {
		StelPropertyMgr * propMgr = StelApp::getInstance().getStelPropertyManager();
		propMgr->setStelPropertyValue("StelMovementMgr.equatorialMount", equatorialMountEnabledMain);
	}
	emit flagAutosetMountForCCDChanged(b);
}

auto Oculars::getFlagAutosetMountForCCD() const -> bool
{
	return flagAutosetMountForCCD;
}

void Oculars::setScaleFOVForTelrad(const bool b)
{
	m_scaleFOVForTelrad = b;
	settings->setValue("use_telrad_fov_scaling", b);
	settings->sync();
	emit scaleFOVForTelradChanged(b);
}

auto Oculars::isScaleFOVForTelrad() const -> bool
{
	return m_scaleFOVForTelrad;
}

void Oculars::setTelradFOV(Vec4d fov)
{
	telradFOV = fov;
	settings->setValue("telrad_fov", fov.toStr());
	settings->sync();
	emit telradFOVChanged(fov);
}

auto Oculars::getTelradFOV() const -> Vec4d
{
	return telradFOV;
}

void Oculars::setScaleFOVForCCD(const bool b)
{
	m_scaleFOVForCCD = b;
	settings->setValue("use_ccd_fov_scaling", b);
	settings->sync();
	emit scaleFOVForCCDChanged(b);
}

auto Oculars::isScaleFOVForCCD() const -> bool
{
	return m_scaleFOVForCCD;
}

void Oculars::setUseSemiTransparency(const bool b)
{
	m_useSemiTransparency = b;
	settings->setValue("use_semi_transparency", b);
	settings->sync();
	emit flagUseSemiTransparencyChanged(b);
}

auto Oculars::useSemiTransparency() const -> bool
{
	return m_useSemiTransparency;
}

void Oculars::setTransparencyLevel(const int level)
{
	m_transparencyLevel = level;
	settings->setValue("transparency_mask", level);
	settings->sync();
	emit transparencyMaskChanged(level);
}

auto Oculars::transparencyLevel() const -> int
{
	return m_transparencyLevel;
}

void Oculars::setShowResolutionCriteria(const bool b)
{
	m_showResolutionCriteria = b;
	settings->setValue("show_resolution_criteria", b);
	settings->sync();
	emit ShowResolutionCriteriaChanged(b);
}

auto Oculars::isShowResolutionCriteria() const -> bool
{
	return m_showResolutionCriteria;
}

void Oculars::setCCDCropOverlayHSize(int size)
{
	m_ccdCropOverlayHSize = size;
	settings->setValue("ccd_crop_overlay_hsize", size);
	settings->sync();
	emit ccdCropOverlayHSizeChanged(size);
}

void Oculars::setCCDCropOverlayVSize(int size)
{
	m_ccdCropOverlayVSize = size;
	settings->setValue("ccd_crop_overlay_vsize", size);
	settings->sync();
	emit ccdCropOverlayVSizeChanged(size);
}

void Oculars::setShowCCDCropOverlay(const bool b)
{
	m_showCCDCropOverlay = b;
	settings->setValue("show_ccd_crop_overlay", b);
	settings->sync();
	emit showCCDCropOverlayChanged(b);
}

auto Oculars::isShowCCDCropOverlay() const -> bool
{
	return m_showCCDCropOverlay;
}

void Oculars::setShowCCDCropOverlayPixelGrid(const bool b)
{
	m_showCCDCropOverlayPixelGrid = b;
	settings->setValue("ccd_crop_overlay_pixel_grid", b);
	settings->sync();
	emit showCCDCropOverlayPixelGridChanged(b);
}

auto Oculars::isShowCCDCropOverlayPixelGrid() const -> bool
{
	return m_showCCDCropOverlayPixelGrid;
}

void Oculars::setShowFocuserOverlay(const bool b)
{
	m_showFocuserOverlay = b;
	settings->setValue("show_focuser_overlay", b);
	settings->sync();
	emit showFocuserOverlayChanged(b);
}

auto Oculars::isShowFocuserOverlay() const -> bool
{
	return m_showFocuserOverlay;
}

void Oculars::setDisplayFocuserOverlaySmall(const bool b)
{
	m_displayFocuserOverlaySmall = b;
	settings->setValue("use_small_focuser_overlay", b);
	settings->sync();
}

auto Oculars::displayFocuserOverlaySmall() const -> bool
{
	return m_displayFocuserOverlaySmall;
}

void Oculars::setDisplayFocuserOverlayMedium(const bool b)
{
	m_displayFocuserOverlayMedium = b;
	settings->setValue("use_medium_focuser_overlay", b);
	settings->sync();
}

auto Oculars::isDisplayFocuserOverlayMedium() const -> bool
{
	return m_displayFocuserOverlayMedium;
}

void Oculars::setDisplayFocuserOverlayLarge(const bool b)
{
	m_displayFocuserOverlayLarge = b;
	settings->setValue("use_large_focuser_overlay", b);
	settings->sync();
}

auto Oculars::isDisplayFocuserOverlayLarge() const -> bool
{
	return m_displayFocuserOverlayLarge;
}

void Oculars::setShowOcularContour(const bool b)
{
	m_showOcularContour = b;
	settings->setValue("show_ocular_contour", b);
	settings->sync();
	emit showOcularContourChanged(b);
}

auto Oculars::isShowOcularContour() const -> bool
{
	return m_showOcularContour;
}

void Oculars::setShowCardinalPoints(const bool b)
{
	m_showCardinalPoints = b;
	settings->setValue("show_ocular_cardinals", b);
	settings->sync();
	emit showCardinalPointChanged(b);
}

auto Oculars::isShowCardinalPoints() const -> bool
{
	return m_showCardinalPoints;
}

void Oculars::setAlignCrosshair(const bool b)
{
	m_alignCrosshair = b;
	settings->setValue("align_crosshair", b);
	settings->sync();
	emit alignCrosshairChanged(b);
}

auto Oculars::isAlignCrosshair() const -> bool
{
	return m_alignCrosshair;
}

void Oculars::setArrowButtonScale(const int val)
{
	m_arrowButtonScale = val;
	settings->setValue("arrow_scale", val);
	settings->sync();
	emit arrowButtonScaleChanged(val);
}

auto Oculars::arrowButtonScale() const -> int
{
	return m_arrowButtonScale;
}

void Oculars::setHideGridsAndLines(const bool b)
{
	if (b != m_hideGridsAndLines) {
		m_hideGridsAndLines = b;
		settings->setValue("hide_grids_and_lines", b);
		settings->sync();
		emit hideGridsAndLinesChanged(b);

		if (b && m_ocularDisplayed) {
			// Store current state for later resetting
			StelPropertyMgr * propMgr  = StelApp::getInstance().getStelPropertyManager();
			flagGridLinesDisplayedMain = propMgr->getStelPropertyValue("GridLinesMgr.gridlinesDisplayed").toBool();
			flagCardinalPointsMain     = propMgr->getStelPropertyValue("LandscapeMgr.cardinalsPointsDisplayed").toBool();
			flagConstellationLinesMain = propMgr->getStelPropertyValue("ConstellationMgr.linesDisplayed").toBool();
			flagConstellationBoundariesMain =
					propMgr->getStelPropertyValue("ConstellationMgr.boundariesDisplayed").toBool();
			flagAsterismLinesMain   = propMgr->getStelPropertyValue("AsterismMgr.linesDisplayed").toBool();
			flagRayHelpersLinesMain = propMgr->getStelPropertyValue("AsterismMgr.rayHelpersDisplayed").toBool();
			toggleLines(false);
		} else if (!b && m_ocularDisplayed) {
			// Restore main program state
			toggleLines(true);
		}
	}
}

auto Oculars::isHideGridsAndLines() const -> bool
{
	return m_hideGridsAndLines;
}

auto Oculars::getDimensionsString(double fovX, double fovY) const -> QString
{
	QString stringFovX;
	QString stringFovY;
	if (isUseDecimalDegrees()) {
		if (fovX >= 1.0) {
			int    degrees = static_cast<int>(fovX);
			double minutes = (fovX - degrees) * 60.;
			stringFovX     = QString::number(degrees) + QChar(0x00B0) + QString::number(minutes, 'f', 2) + QChar(0x2032);
		} else {
			double minutes = fovX * 60.;
			stringFovX     = QString::number(minutes, 'f', 2) + QChar(0x2032);
		}

		if (fovY >= 1.0) {
			int    degrees = static_cast<int>(fovY);
			double minutes = (fovY - degrees) * 60.;
			stringFovY     = QString::number(degrees) + QChar(0x00B0) + QString::number(minutes, 'f', 2) + QChar(0x2032);
		} else {
			double minutes = fovY * 60;
			stringFovY     = QString::number(minutes, 'f', 2) + QChar(0x2032);
		}
	} else {
		stringFovX = QString::number(fovX, 'f', 5) + QChar(0x00B0);
		stringFovY = QString::number(fovY, 'f', 5) + QChar(0x00B0);
	}

	return stringFovX + QChar(0x00D7) + stringFovY;
}

// Define whether the button toggling eyepieces should be visible
void Oculars::setShowOcularsButton(bool b)
{
	auto * gui = dynamic_cast<StelGui *>(StelApp::getInstance().getGui());
	if (gui) {
		if (b) {
			if (toolbarButton == nullptr) {
				// Create the pulsars button
				toolbarButton = new StelButton(nullptr, *pxmapOnIcon, *pxmapOffIcon, *pxmapGlow, "actionShow_Ocular");
			}
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		} else {
			gui->getButtonBar()->hideButton("actionShow_Ocular");
		}
	}
	m_showOcularsButton = b;
	settings->setValue("show_toolbar_button", b);
	settings->sync();

	emit flagShowOcularsButtonChanged(b);
}

void Oculars::setGuiPanelFontSize(int size)
{
	// This forces a redraw of the panel.
	if (size != guiPanelFontSize) {
		bool guiPanelVisible = guiPanel;
		if (guiPanelVisible) {
			enableGuiPanel(false);
		}
		guiPanelFontSize = size;
		if (guiPanelVisible) {
			enableGuiPanel(true);
		}

		settings->setValue("gui_panel_fontsize", size);
		settings->sync();
		emit guiPanelFontSizeChanged(size);
	}
}

void Oculars::toggleCropOverlay()
{
	setShowCCDCropOverlay(!isShowCCDCropOverlay());
}

void Oculars::togglePixelGrid()
{
	setShowCCDCropOverlayPixelGrid(!isShowCCDCropOverlayPixelGrid());
}

void Oculars::toggleFocuserOverlay()
{
	setShowFocuserOverlay(!isShowFocuserOverlay());
}

auto Oculars::computeLimitMagnitude(Ocular * ocular, Telescope * telescope) -> double
{
	// Simplified calculation of the penetrating power of the telescope
	double diameter = 0.;
	if (ocular->isBinoculars()) {
		diameter = ocular->fieldStop();
	} else {
		diameter = telescope != nullptr ? telescope->diameter()
						: 0.1; // Avoid a potential call of null pointer, and a log(0) error.
	}

	// A better formula for telescopic limiting magnitudes?
	// North, G.; Journal of the British Astronomical Association, vol.107, no.2, p.82
	// http://adsabs.harvard.edu/abs/1997JBAA..107...82N
	return 4.5 + 4.4 * std::log10(diameter);
}

void Oculars::handleAutoLimitToggle(bool on)
{
	if (!m_ocularDisplayed) {
		return;
	}

	// When we are in Oculars mode, we must toggle between the auto limit and manual limit. Logic taken from
	// zoomOcular()/unzoomOcular()
	StelCore *      core      = StelApp::getInstance().getCore();
	StelSkyDrawer * skyDrawer = core->getSkyDrawer();
	if (on) {
		Ocular *    ocular    = oculars.at(m_indexSelectedOcular);
		Telescope * telescope = nullptr;
		if (!ocular->isBinoculars()) {
			telescope = telescopes.at(m_indexSelectedTelescope);
		}
		disconnect(skyDrawer,
			   &StelSkyDrawer::customStarMagLimitChanged,
			   this,
			   &Oculars::setMagLimitStarsOcularsManual); // keep the old manual value in config.
		double limitMag = computeLimitMagnitude(ocular, telescope);
		// TODO: Is it really good to apply the star formula to DSO?
		skyDrawer->setFlagNebulaMagnitudeLimit(true);
		skyDrawer->setCustomNebulaMagnitudeLimit(limitMag);
		skyDrawer->setFlagStarMagnitudeLimit(true);
		skyDrawer->setCustomStarMagnitudeLimit(limitMag);
	} else {
		connect(skyDrawer, &StelSkyDrawer::customStarMagLimitChanged, this, &Oculars::setMagLimitStarsOcularsManual);
		skyDrawer->setCustomStarMagnitudeLimit(magLimitStarsOculars);
		skyDrawer->setFlagStarMagnitudeLimit(flagLimitStarsOculars);
	}
}

// Handle switching the main program's star limitation flag
void Oculars::handleStarMagLimitToggle(bool on)
{
	if (!m_ocularDisplayed) {
		return;
	}

	flagLimitStarsOculars = on;
	// It only makes sense to switch off the auto-limit when we switch off the limit.
	if (!on) {
		setAutoLimitMagnitude(false);
	}
}
