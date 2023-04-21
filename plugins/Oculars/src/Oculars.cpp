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

#include "LabelMgr.hpp"
#include "SkyGui.hpp"
#include "StelActionMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelPainter.hpp"
#include "StelProjector.hpp"
#include "StelTextureMgr.hpp"
#include "StelTranslator.hpp"
#include "SolarSystem.hpp"
#include "NebulaMgr.hpp"
#include "StelUtils.hpp"
#include "StelPropertyMgr.hpp"

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
	info.authors = "Timothy Reaves, Bogdan Marinov, Alexander Wolf, Georg Zotti";
	info.contact = STELLARIUM_DEV_URL;
	info.description = N_("Shows the sky as if looking through a telescope eyepiece. (Only magnification and field of view are simulated.) It can also show a sensor frame and a Telrad sight.");
	info.version = OCULARS_PLUGIN_VERSION;
	info.license = OCULARS_PLUGIN_LICENSE;
	return info;
}

Oculars::Oculars()
	: selectedCCDIndex(-1)
	, selectedOcularIndex(-1)
	, selectedTelescopeIndex(-1)
	, selectedLensIndex(-1)
	, selectedFinderIndex(-1)
	, selectedCCDRotationAngle(0.0)
	, selectedCCDPrismPositionAngle(0.0)
	, arrowButtonScale(150)
	, pluginMode(OcNone)
	, flagModeOcular(false)
	, flagModeSensor(false)
	, flagModeTelrad(false)
	, flagModeFinder(false)
	, flagShowCrosshairs(false)
	, usageMessageLabelID(-1)
	, flagCardinalPointsMain(false)
	, flagAdaptationMain(false)

	, flagLimitStarsMain(false)
	, magLimitStarsMain(0.0)
	, flagLimitPlanetsMain(false)
	, magLimitPlanetsMain(0.0)
	, flagLimitDSOsMain(false)
	, magLimitDSOsMain(0.0)
	, flagLimitStarsOculars(false)
	, magLimitStarsOculars(0.0)
	, flagLimitPlanetsOculars(false)
	, magLimitPlanetsOculars(0.0)
	, flagLimitDSOsOculars(false)
	, magLimitDSOsOculars(0.0)
	, flagLimitStarsFinder(false)
	, magLimitStarsFinder(0.0)
	, flagLimitPlanetsFinder(false)
	, magLimitPlanetsFinder(0.0)
	, flagLimitDSOsFinder(false)
	, magLimitDSOsFinder(0.0)

	, flagAutoLimitMagnitude(false)
	, relativeStarScaleMain(1.0)
	, absoluteStarScaleMain(1.0)
	, relativeStarScaleOculars(1.0)
	, absoluteStarScaleOculars(1.0)
	, relativeStarScaleCCD(1.0)
	, absoluteStarScaleCCD(1.0)
	, flagMoonScaleMain(false)
	, flagMinorBodiesScaleMain(false)
	, flagPlanetScaleMain(false)
	, flagSunScaleMain(false)
	, flagDSOPropHintMain(false)
	, milkyWaySaturationMain(1.0)
	, maxEyepieceAngle(0.0)
	, flagRequireSelection(true)
	, flagScaleImageCircle(true)
	, flagGuiPanelEnabled(false)
	, flagDMSDegrees(false)
	, flagHorizontalCoordinates(false)
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
	, actionShowFinder(Q_NULLPTR)
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
	, selectedSSO(Q_NULLPTR)
	, actualFOV(0.)
	, initialFOV(0.)
	, flagInitFOVUsage(false)
	, flagInitDirectionUsage(false)
	, flagAutosetMountForCCD(false)
	, flagScalingFOVForTelrad(false)
	, flagScalingFOVForCCD(true)
	, flagMaxExposureTimeForCCD(false)
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
	finders = QList<Finder *> ();

#ifdef Q_OS_MACOS
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
	qDeleteAll(finders);
	finders.clear();
}

QSettings* Oculars::getSettings()
{
	return settings;
}

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
	// force update the ini file on exit.
	settings->remove("ccd");
	settings->remove("ocular");
	settings->remove("telescope");
	settings->remove("lens");
	settings->remove("finder");
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
	index = 0;
	for (auto* finder : qAsConst(finders))
	{
		finder->writeToSettings(settings, index);
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
	settings->setValue("finder_index", selectedFinderIndex);

	StelCore *core = StelApp::getInstance().getCore();
	StelSkyDrawer *skyDrawer = core->getSkyDrawer();
	disconnect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double)));
	disconnect(skyDrawer, SIGNAL(flagStarMagnitudeLimitChanged(bool)), this, SLOT(handleStarMagLimitToggle(bool)));
	setPluginMode(OcNone); // Restore Main App's settings

	settings->setValue("stars_scale_relative", QString::number(relativeStarScaleOculars, 'f', 2));
	settings->setValue("stars_scale_absolute", QString::number(absoluteStarScaleOculars, 'f', 2));
	settings->setValue("stars_scale_relative_ccd", QString::number(relativeStarScaleCCD, 'f', 2));
	settings->setValue("stars_scale_absolute_ccd", QString::number(absoluteStarScaleCCD, 'f', 2));
	settings->setValue("stars_scale_relative_finder", QString::number(relativeStarScaleFinder, 'f', 2));
	settings->setValue("stars_scale_absolute_finder", QString::number(absoluteStarScaleFinder, 'f', 2));

	settings->setValue("limit_stellar_magnitude_oculars_val", QString::number(magLimitStarsOculars, 'f', 2));
	settings->setValue("limit_stellar_magnitude_oculars", flagLimitStarsOculars);
	settings->setValue("limit_stellar_magnitude_finder_val", QString::number(magLimitStarsFinder, 'f', 2));
	settings->setValue("limit_stellar_magnitude_finder", flagLimitStarsFinder);

	settings->setValue("limit_planet_magnitude_oculars_val", QString::number(magLimitPlanetsOculars, 'f', 2));
	settings->setValue("limit_planet_magnitude_oculars", flagLimitPlanetsOculars);
	settings->setValue("limit_planet_magnitude_finder_val", QString::number(magLimitPlanetsFinder, 'f', 2));
	settings->setValue("limit_planet_magnitude_finder", flagLimitPlanetsFinder);

	settings->setValue("limit_nabula_magnitude_oculars_val", QString::number(magLimitDSOsOculars, 'f', 2));
	settings->setValue("limit_nabula_magnitude_oculars", flagLimitDSOsOculars);
	settings->setValue("limit_nabula_magnitude_finder_val", QString::number(magLimitDSOsFinder, 'f', 2));
	settings->setValue("limit_nabula_magnitude_finder", flagLimitDSOsFinder);

	settings->setValue("text_color", textColor.toStr());
	settings->setValue("line_color", lineColor.toStr());
	settings->setValue("focuser_color", focuserColor.toStr());
	settings->setValue("oculars_version", QString::number(static_cast<double>(MIN_OCULARS_INI_VERSION), 'f', 2));
	settings->sync();

	disconnect(this, SIGNAL(selectedOcularChanged(int)), this, SLOT(updateOcularReticle()));
	//disconnect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	disconnect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslateGui()));

	protractorTexture.clear();
	protractorFlipVTexture.clear();
	protractorFlipHTexture.clear();
	protractorFlipHVTexture.clear();
}

//! Draw any parts on the screen which are for our module
void Oculars::draw(StelCore* core)
{
	switch (pluginMode){
		case OcNone:
			break;
		case OcTelrad:
			paintTelrad();
			break;
		case OcOcular:
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
			{	// TODO 2021: "Module disabled" is printed but not visible here. Change message or set ocularMode=OcNone?
				qWarning() << "Oculars: the selected ocular index of "
					   << selectedOcularIndex << " is greater than the ocular count of "
					   << oculars.count() << ". Module disabled!";
			}
			break;
		case OcFinder:
			if (selectedFinderIndex >= 0)
			{
				paintOcularMask(core); // TODO: paintFinderMask?
				if (flagShowCrosshairs)
					paintCrosshairs();

				if (!flagGuiPanelEnabled)
				{
					// Paint the information in the upper-right hand corner
					paintText(core);
				}
			}
			else
			{	// TODO 2021: "Module disabled" is printed but not visible here. Change message or set ocularMode=OcNone?
				qWarning() << "Oculars: the selected finder index of "
					   << selectedFinderIndex << " is greater than the finder count of "
					   << finders.count() << ". Module disabled!";
			}
			break;
		case OcSensor:
			paintCCDBounds();
			if (!flagGuiPanelEnabled)
			{
				// Paint the information in the upper-right hand corner
				paintText(core);
			}
			break;
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
	StelMovementMgr *movementManager = core->getMovementMgr();
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000, StelCore::RefractionAuto);
	const StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
	const qreal ppx = params.devicePixelsPerPixel;
	
	if (guiPanel)
	{
		// Remove all events on the sky within Ocular GUI Panel.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		if (event->position().x()>guiPanel->pos().x() && event->position().y()>(prj->getViewportHeight()-guiPanel->size().height()))
#else
		if (event->x()>guiPanel->pos().x() && event->y()>(prj->getViewportHeight()-guiPanel->size().height()))
#endif
		{
			event->setAccepted(true);
			return;
		}
	}

	// In case we show oculars with black circle, ignore mouse presses outside image circle:
	// https://sourceforge.net/p/stellarium/discussion/278769/thread/57893bb3/?limit=25#75c0
	if ((pluginMode==OcOcular) || (pluginMode==OcFinder)) //&& !getFlagUseSemiTransparency()) // Not sure: ignore or allow selection of semi-hidden stars?
	{
		float wh = prj->getViewportWidth()*0.5f; // get half of width of the screen
		float hh = prj->getViewportHeight()*0.5f; // get half of height of the screen
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		float mx = event->position().x()-wh; // point 0 in center of the screen, axis X directed to right
		float my = event->position().y()-hh; // point 0 in center of the screen, axis Y directed to bottom
#else
		float mx = event->x()-wh; // point 0 in center of the screen, axis X directed to right
		float my = event->y()-hh; // point 0 in center of the screen, axis Y directed to bottom
#endif

		double inner = 0.5 * params.viewportFovDiameter * ppx;
		// See if we need to scale the mask (only for real ocular views...)
		if ((pluginMode==OcOcular) && flagScaleImageCircle && oculars[selectedOcularIndex]->apparentFOV() > 0.0)
		{
			inner = oculars[selectedOcularIndex]->apparentFOV() * inner / maxEyepieceAngle;
		}

		if (mx*mx+my*my>static_cast<float>(inner*inner)) // click outside ocular circle? Gobble event.
		{
			event->setAccepted(true);
			return;
		}
	}

	if ((pluginMode==OcOcular) || (pluginMode==OcFinder))
		movementManager->handleMouseClicks(event); // force it here for selection!

	if (StelApp::getInstance().getStelObjectMgr().getWasSelected())
	{
		if ((pluginMode==OcOcular) || (pluginMode==OcFinder))
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
	else if((pluginMode==OcOcular) || (pluginMode==OcFinder))
	{
		// The ocular is displayed, but no object is selected.  So don't track the stars.  We may have locked
		// the position of the screen if the movement keys were used.  so call this to be on the safe side.
		movementManager->setFlagLockEquPos(false);		
	}
	event->setAccepted(false);
}

void Oculars::updateLatestSelectedSSO()
{
	QList<StelObjectP> selectedObjects = GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if (!selectedObjects.isEmpty())
	{
		selectedSSO = (selectedObjects[0]->getType()=="Planet") ? dynamic_cast<Planet*>(selectedObjects[0].data()) : Q_NULLPTR;
	}
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

		// Read Oculars. Convert "isBinocular" type from version before 0.21.0 to Finder type.
		const int ocularCount = settings->value("ocular_count", 0).toInt();
		int actualOcularCount = ocularCount;
		int ocularIsActuallyFinder=0;
		for (int index = 0; index < ocularCount; index++)
		{
			Ocular *newOcular = Ocular::ocularFromSettings(settings, index);
			if (newOcular != Q_NULLPTR)
			{
				// V0.21.0: process old config files to convert "binocular" oculars into finders.
				// TBD: Remove this in about 0.23.0
				if (newOcular->isBinoculars())
				{
					qDebug() << "Converting Ocular" << newOcular->name() << "(marked as 'binocular') to Finder";
					Finder *f=new Finder();
					f->setName(newOcular->name());
					f->setTrueFOV(newOcular->apparentFOV());
					f->setMagnification(newOcular->effectiveFocalLength());
					f->setAperture(newOcular->fieldStop());
					f->setPermanentCrosshair(newOcular->hasPermanentCrosshair());
					f->setReticlePath(newOcular->reticlePath());
					finders.append(f);
					delete newOcular;
					ocularIsActuallyFinder++;
					actualOcularCount--;

				}
				else
				{
					oculars.append(newOcular);
				}
			}
			else
			{
				actualOcularCount--;
			}
		}
		if (actualOcularCount < 1)
		{
			if (actualOcularCount < ocularCount-ocularIsActuallyFinder)
			{
				qWarning() << "The Oculars ini file appears to be corrupt (inconsistent number of oculars); edit or delete it.";
			}
			else
			{
				qWarning() << "There are no oculars defined for the Oculars plugin.";
			}
			qWarning() << "Oculars plugin will be disabled.";
			ready = false;
		}
		else
		{
			selectedOcularIndex = settings->value("ocular_index", 0).toInt();
		}

		const int ccdCount = settings->value("ccd_count", 0).toInt();
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

		const int telescopeCount = settings->value("telescope_count", 0).toInt();
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
				qWarning() << "The Oculars ini file appears to be corrupt (inconsistent number of telescopes); edit or delete it.";
			}
			else
			{
				qWarning() << "There are no telescopes defined for the Oculars plugin.";
			}
			qWarning() << "Oculars plugin will be disabled.";
			ready = false;
		}
		else
		{
			selectedTelescopeIndex = settings->value("telescope_index", 0).toInt();
		}

		const int lensCount = settings->value("lens_count", 0).toInt();
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
			qWarning() << "The Oculars ini file appears to be corrupt  (inconsistent number of lenses); edit or delete it.";
		}
		selectedLensIndex=settings->value("lens_index", -1).toInt(); // Lens is not selected by default!

		const int finderCount = settings->value("finder_count", 0).toInt();
		int actualFinderCount = finderCount;
		for (int index = 0; index<finderCount; index++)
		{
			Finder *newFinder = Finder::finderFromSettings(settings, index);
			if (newFinder != Q_NULLPTR)
			{
				finders.append(newFinder);
			}
			else
			{
				actualFinderCount--;
			}
		}
		if (finderCount > 0 && actualFinderCount < finderCount)
		{
			qWarning() << "The Oculars ini file appears to be corrupt (inconsistent number of finders); edit or delete it.";
		}
		selectedFinderIndex=settings->value("finder_index", 0).toInt();

		pxmapGlow = new QPixmap(":/graphicGui/miscGlow32x32.png");
		pxmapOnIcon = new QPixmap(":/ocular/bt_ocular_on.png");
		pxmapOffIcon = new QPixmap(":/ocular/bt_ocular_off.png");

		qDebug() << "Oculars::init(): OcularDialog";
		ocularDialog = new OcularDialog(this, &ccds, &oculars, &telescopes, &lenses, &finders);
		qDebug() << "Oculars::init(): initializeActivationActions";
		initializeActivationActions();
		qDebug() << "Oculars::init(): determineMaxEyepieceAngle";
		determineMaxEyepieceAngle();
		qDebug() << "Oculars::init(): enableGuiPanel";


		guiPanelFontSize=settings->value("gui_panel_fontsize", 12).toInt();
		enableGuiPanel(settings->value("enable_control_panel", true).toBool());
		textColor=Vec3f(settings->value("text_color", "0.8,0.48,0.0").toString());
		lineColor=Vec3f(settings->value("line_color", "0.77,0.14,0.16").toString());
		telradFOV=Vec4f(settings->value("telrad_fov", "0.5,2.0,4.0,0.0").toString());
		focuserColor=Vec3f(settings->value("focuser_color", "0.0,0.67,1.0").toString());

		qDebug() << "Oculars::init(): setting up properties";

		// This must come ahead of setFlagAutosetMountForCCD (GH #505)
		StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();
		equatorialMountEnabledMain = propMgr->getStelPropertyValue("StelMovementMgr.equatorialMount").toBool();

		// For historical reasons, name of .ini entry and description of checkbox (and therefore flag name) are reversed.
		setFlagDMSDegrees( ! settings->value("use_decimal_degrees", false).toBool());
		setFlagHorizontalCoordinates(settings->value("use_horizontal_coordinates", false).toBool());
		setFlagAutoLimitMagnitude(settings->value("autolimit_stellar_magnitude", true).toBool());
		connect(this, SIGNAL(flagAutoLimitMagnitudeChanged(bool)), this, SLOT(handleAutoLimitToggle(bool))); // only after first initialisation!

		flagLimitStarsOculars=settings->value("limit_stellar_magnitude_oculars", false).toBool();
		magLimitStarsOculars=settings->value("limit_stellar_magnitude_oculars_val", 12.).toDouble();
		flagLimitPlanetsOculars=settings->value("limit_planet_magnitude_oculars", false).toBool();
		magLimitPlanetsOculars=settings->value("limit_planet_magnitude_oculars_val", 12.).toDouble();
		flagLimitDSOsOculars=settings->value("limit_nebula_magnitude_oculars", false).toBool();
		magLimitDSOsOculars=settings->value("limit_nebula_magnitude_oculars_val", 12.).toDouble();

		flagLimitStarsFinder=settings->value("limit_stellar_magnitude_finder", false).toBool();
		magLimitStarsFinder=settings->value("limit_stellar_magnitude_finder_val", 10.).toDouble();
		flagLimitPlanetsFinder=settings->value("limit_planet_magnitude_finder", false).toBool();
		magLimitPlanetsFinder=settings->value("limit_planet_magnitude_finder_val", 10.).toDouble();
		flagLimitDSOsFinder=settings->value("limit_nebula_magnitude_finder", false).toBool();
		magLimitDSOsFinder=settings->value("limit_nebula_magnitude_finder_val", 10.).toDouble();

		setFlagInitFovUsage(settings->value("use_initial_fov", false).toBool());
		setFlagInitDirectionUsage(settings->value("use_initial_direction", false).toBool());
		setFlagUseSemiTransparency(settings->value("use_semi_transparency", false).toBool());
		setTransparencyMask(settings->value("transparency_mask", 85).toInt());
		setFlagHideGridsLines(settings->value("hide_grids_and_lines", true).toBool());
		setFlagAutosetMountForCCD(settings->value("use_mount_autoset", false).toBool());
		setFlagScalingFOVForTelrad(settings->value("use_telrad_fov_scaling", true).toBool());
		setFlagScalingFOVForCCD(settings->value("use_ccd_fov_scaling", true).toBool());
		setFlagMaxExposureTimeForCCD(settings->value("show_ccd_exposure_time", false).toBool());
		setFlagShowResolutionCriteria(settings->value("show_resolution_criteria", false).toBool());
		setArrowButtonScale(settings->value("arrow_scale", 150).toInt());
		setFlagShowOcularsButton(settings->value("show_toolbar_button", false).toBool());
		relativeStarScaleOculars=settings->value("stars_scale_relative", 1.0).toDouble();
		absoluteStarScaleOculars=settings->value("stars_scale_absolute", 1.0).toDouble();
		relativeStarScaleCCD=settings->value("stars_scale_relative_ccd", 1.0).toDouble();
		absoluteStarScaleCCD=settings->value("stars_scale_absolute_ccd", 1.0).toDouble();
		relativeStarScaleFinder=settings->value("stars_scale_relative_finder", 1.0).toDouble();
		absoluteStarScaleFinder=settings->value("stars_scale_absolute_finder", 1.0).toDouble();
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

	protractorTexture = StelApp::getInstance().getTextureManager().createTexture(":/ocular/Protractor.png");
	protractorFlipHTexture = StelApp::getInstance().getTextureManager().createTexture(":/ocular/ProtractorFlipH.png");
	protractorFlipVTexture = StelApp::getInstance().getTextureManager().createTexture(":/ocular/ProtractorFlipV.png");
	protractorFlipHVTexture = StelApp::getInstance().getTextureManager().createTexture(":/ocular/ProtractorFlipHV.png");
	// enforce check existence of reticle for the current eyepiece
	updateOcularReticle();

	qDebug() << "Oculars::init(): connections...";

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslateGui()));
	connect(this, SIGNAL(selectedOcularChanged(int)), this, SLOT(updateOcularReticle()));
	StelSkyDrawer *skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();
	connect(skyDrawer, SIGNAL(flagStarMagnitudeLimitChanged(bool)), this, SLOT(handleStarMagLimitToggle(bool)));
	StelObjectMgr* objectMgr = GETSTELMODULE(StelObjectMgr);
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(updateLatestSelectedSSO()));
	qDebug() << "Oculars::init(): done";
}

void Oculars::determineMaxEyepieceAngle()
{
	if (ready)
	{
		for (const auto* ocular : qAsConst(oculars))
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
	switch (pluginMode) {
		case OcOcular: // We only zoom if in ocular mode.
			// If we are already in Ocular mode, we must reset scalings because zoom() also resets.
			// TODO 2021: AVOID THIS LIKE HELL! All state switching only in the master state switcher
			//StelSkyDrawer *skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();
			//skyDrawer->setRelativeStarScale(relativeStarScaleMain);
			//skyDrawer->setAbsoluteStarScale(absoluteStarScaleMain);
			//zoom(true);
			zoomOcular(); // zoom to possibly other settings
			break;
		case OcSensor:
			setScreenFOVForCCD();
			break;
		default:
			break;
	}
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
	if (!(getFlagScalingFOVForCCD() && pluginMode==OcSensor))
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
				guiPanel = new OcularsGuiPanel(this, gui->getSkyGui()); // TODO 2021: consider extending the constructor with an OcularMode argument?
				if (pluginMode==OcOcular)
					guiPanel->showOcularGui();
				else if (pluginMode==OcSensor)
					guiPanel->showCcdGui();
				else if (pluginMode==OcFinder)
					guiPanel->showFinderGui();
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
	if (flagGuiPanelEnabled!=enable)
	{	flagGuiPanelEnabled = enable;
		settings->setValue("enable_control_panel", enable);
		settings->sync();
		emit flagGuiPanelEnabledChanged(enable);
	}
}

void Oculars::retranslateGui()
{
	if (guiPanel)
	{
		// TODO: Fix this hack! TODO 2021: This was 2011... why/what to fix?
		
		// Delete and re-create the panel to retranslate its strings
		guiPanel->hide();
		delete guiPanel;
		guiPanel = Q_NULLPTR;
		
		// avoid code repetition!
		enableGuiPanel(true);
//		StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
//		if (gui)
//		{
//			guiPanel = new OcularsGuiPanel(this, gui->getSkyGui()); // TODO 2021: consider extending the constructor with an OcularMode argument?
//			if (ocularMode==OcOcular)
//				guiPanel->showOcularGui();
//			else if (ocularMode==OcSensor)
//				guiPanel->showCcdGui();
//		}
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

void Oculars::updateLists()
{
	if (oculars.isEmpty())
	{
		selectedOcularIndex = -1;
		//enableOcular(false);
		if (pluginMode==OcOcular)
			setPluginMode(OcNone);
	}
	else
	{
		if (selectedOcularIndex >= oculars.count())
			selectedOcularIndex = oculars.count() - 1;

		if (pluginMode==OcOcular)
			emit selectedOcularChanged(selectedOcularIndex);
	}

	if (telescopes.isEmpty())
	{
		selectedTelescopeIndex = -1;
		//enableOcular(false);
		//toggleCCD(false);
		if ((pluginMode==OcOcular) || (pluginMode==OcSensor))
			setPluginMode(OcNone);
		// TODO 2021: Switching both possible modes off was the original logic. But what if we have an Ocular that is isBinocular()?
	}
	else
	{
		if (selectedTelescopeIndex >= telescopes.count())
			selectedTelescopeIndex = telescopes.count() - 1;

		if (pluginMode==OcOcular || pluginMode==OcSensor)
			emit selectedTelescopeChanged(selectedTelescopeIndex);
	}

	if (ccds.isEmpty())
	{
		selectedCCDIndex = -1;
		//toggleCCD(false);
		if (pluginMode==OcSensor)
			setPluginMode(OcNone);

	}
	else
	{
		if (selectedCCDIndex >= ccds.count())
			selectedCCDIndex = ccds.count() - 1;

		if (pluginMode==OcSensor)
			emit selectedCCDChanged(selectedCCDIndex);
	}

	if (lenses.isEmpty())
		selectedLensIndex = -1;
	else
	{
		if (selectedLensIndex >= lenses.count())
			selectedLensIndex = lenses.count() - 1;
		// TODO 2021: Write here as comment why no instrument update (emit ***Changed()) is necessary? It seems this should change things like auto-determined visibility limits?
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
		emit selectedCCDChanged(selectedCCDIndex); // TODO: Why do we emit this? We did not change the index! Probably the function can be replaced by setSelectedCCDRotationAngle(0)
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
		emit selectedCCDChanged(selectedCCDIndex);
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

void Oculars::decrementCCDIndex()
{
	selectedCCDIndex--;
	if (selectedCCDIndex == -1)
	{
		selectedCCDIndex = ccds.count() - 1;
	}
	emit selectedCCDChanged(selectedCCDIndex);
}

void Oculars::decrementFinderIndex()
{
	selectedFinderIndex--;
	if (selectedFinderIndex == -1)
	{
		selectedFinderIndex = finders.count() - 1;
	}
	emit(selectedFinderChanged(selectedFinderIndex));
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
	emit selectedOcularChanged(selectedOcularIndex);
}

void Oculars::decrementTelescopeIndex()
{
	selectedTelescopeIndex--;
	if (selectedTelescopeIndex == -1)
	{
		selectedTelescopeIndex = telescopes.count() - 1;
	}
	emit selectedTelescopeChanged(selectedTelescopeIndex);
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
	emit selectedLensChanged(selectedLensIndex);
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

	if (pluginMode==OcOcular)
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
					action = submenu->addAction(label, submenu, [=](){selectOcularAtIndex(index);});
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
		// We want to show the Sensors, and if a Sensor is selected, the telescopes
		//(as a Sensor requires a telescope) and the general menu items.
		QAction* action = new QAction(q_("Configure &Oculars"), popup);
		action->setCheckable(true);
		action->setChecked(ocularDialog->visible());
		connect(action, SIGNAL(triggered(bool)), ocularDialog, SLOT(setVisible(bool)));
		popup->addAction(action);
		popup->addSeparator();

		if (pluginMode!=OcTelrad)
		{
			QAction* action = popup->addAction(q_("Toggle &CCD"));
			action->setCheckable(true);
			action->setChecked(pluginMode==OcSensor);
			connect(action, SIGNAL(toggled(bool)), actionShowSensor, SLOT(setChecked(bool)));
		}
		
		if (pluginMode!=OcSensor)
		{
			QAction* action = popup->addAction(q_("Toggle &Telrad"));
			action->setCheckable(true);
			action->setChecked(pluginMode==OcTelrad);
			connect(action, SIGNAL(toggled(bool)), actionShowTelrad, SLOT(setChecked(bool)));
		}
		
		popup->addSeparator();
		if ((pluginMode==OcSensor) && selectedCCDIndex > -1 && selectedTelescopeIndex > -1)
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
				QAction* action = submenu->addAction(label, submenu, [=](){selectCCDAtIndex(index);});
				if (index == selectedCCDIndex)
				{
					action->setCheckable(true);
					action->setChecked(true);
				}
			}
			popup->addMenu(submenu);
			
			submenu = new QMenu(q_("&Rotate CCD"), popup);
			submenu->addAction(QString("&1: -90") + QChar(0x00B0), submenu, [=](){rotateCCD(-90);});
			submenu->addAction(QString("&2: -45") + QChar(0x00B0), submenu, [=](){rotateCCD(-45);});
			submenu->addAction(QString("&3: -15") + QChar(0x00B0), submenu, [=](){rotateCCD(-15);});
			submenu->addAction(QString("&4: -5") + QChar(0x00B0),  submenu, [=](){rotateCCD(-5);});
			submenu->addAction(QString("&5: -1") + QChar(0x00B0),  submenu, [=](){rotateCCD(-1);});
			submenu->addAction(QString("&6: +1") + QChar(0x00B0),  submenu, [=](){rotateCCD(1);});
			submenu->addAction(QString("&7: +5") + QChar(0x00B0),  submenu, [=](){rotateCCD(5);});
			submenu->addAction(QString("&8: +15") + QChar(0x00B0), submenu, [=](){rotateCCD(15);});
			submenu->addAction(QString("&9: +45") + QChar(0x00B0), submenu, [=](){rotateCCD(45);});
			submenu->addAction(QString("&0: +90") + QChar(0x00B0), submenu, [=](){rotateCCD(90);});

			submenu->addAction(q_("&Reset rotation"), this, SLOT(ccdRotationReset()));
			popup->addMenu(submenu);			
			popup->addSeparator();
		}
		if ((pluginMode==OcSensor) && selectedCCDIndex > -1 && telescopes.count() > 1)
		{
			QMenu* submenu = addTelescopeSubmenu(popup);
			popup->addMenu(submenu);
			submenu = addLensSubmenu(popup);
			popup->addMenu(submenu);
			popup->addSeparator();
		}
	}

#ifdef Q_OS_WIN
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
	emit selectedCCDChanged(selectedCCDIndex);
}

void Oculars::incrementFinderIndex()
{
	selectedFinderIndex++;
	if (selectedFinderIndex == finders.count())
	{
		selectedFinderIndex = 0;
	}
	emit(selectedFinderChanged(selectedFinderIndex));
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
	emit selectedOcularChanged(selectedOcularIndex);
}

void Oculars::incrementTelescopeIndex()
{
	selectedTelescopeIndex++;
	if (selectedTelescopeIndex == telescopes.count())
	{
		selectedTelescopeIndex = 0;
	}
	emit selectedTelescopeChanged(selectedTelescopeIndex);
}

void Oculars::incrementLensIndex()
{
	selectedLensIndex++;
	if (selectedLensIndex == lenses.count())
	{
		selectedLensIndex = -1;
	}
	emit selectedLensChanged(selectedLensIndex);
}

void Oculars::disableLens()
{
	selectedLensIndex = -1;
	emit selectedLensChanged(selectedLensIndex);
}

void Oculars::rotateCCD(int amount)
{
	CCD *ccd = ccds[selectedCCDIndex];
	if (!ccd) return;
	double angle = ccd->chipRotAngle();
	angle = StelUtils::fmodpos(angle+amount, 360.); // TODO: NEW 2021: only [0...360] OK? then delete the commented section below and this comment.
//	if (angle >= 360)
//	{
//		angle -= 360;
//	}
//	else if (angle <= -360)
//	{
//		angle += 360;
//	}
	ccd->setChipRotAngle(angle);	
	emit selectedCCDRotationAngleChanged(angle);
}

void Oculars::rotatePrism(int amount)
{
	CCD *ccd = ccds[selectedCCDIndex];
	if (!ccd) return;
	double angle = ccd->prismPosAngle();
	angle = StelUtils::fmodpos(angle+amount, 360.); // TODO: NEW 2021: only [0...360] OK? then delete the commented section below and this comment.
//	if (angle >= 360)
//	{
//		angle -= 360;
//	}
//	else if (angle <= -360)
//	{
//		angle += 360;
//	}
	ccd->setPrismPosAngle(angle);
	emit selectedCCDPrismPositionAngleChanged(angle);
}

void Oculars::selectCCDAtIndex(int index)
{
	if (index > -1 && index < ccds.count())
	{
		selectedCCDIndex = index;
		emit selectedCCDChanged(index);
	}
}

void Oculars::selectOcularAtIndex(int index)
{
	if (selectedTelescopeIndex == -1)
		selectTelescopeAtIndex(0);

	if (index > -1 && index < oculars.count() && (telescopes.count() >= 0 || oculars[index]->isBinoculars()))
	{
		selectedOcularIndex = index;
		emit selectedOcularChanged(index);
	}
}

void Oculars::selectTelescopeAtIndex(int index)
{
	if (index > -1 && index < telescopes.count())
	{
		selectedTelescopeIndex = index;
		emit selectedTelescopeChanged(index);
	}
}

void Oculars::selectLensAtIndex(int index)
{
	if (index > -2 && index < lenses.count())
	{
		selectedLensIndex = index;
		emit selectedLensChanged(index);
	}
}

void Oculars::selectFinderAtIndex(int index)
{
	if (index > -1 && index < finders.count())
	{
		selectedFinderIndex = index;
		emit(selectedFinderChanged(index));
	}
}

void Oculars::toggleCCD()
{
	setPluginMode(pluginMode==OcSensor ? OcNone : OcSensor);
}

void Oculars::toggleCrosshairs(bool show)
{
	if(show != flagShowCrosshairs)
	{
		flagShowCrosshairs = show;
		emit enableCrosshairsChanged(show);
	}
}

void Oculars::toggleTelrad()
{
	setPluginMode(pluginMode==OcTelrad ? OcNone : OcTelrad);
}

void Oculars::toggleFinder()
{
	setPluginMode(pluginMode==OcFinder ? OcNone : OcFinder);
}

void Oculars::initializeActivationActions()
{
	QString ocularsGroup = N_("Oculars");
	actionMenu = addAction("actionShow_Ocular_Menu", ocularsGroup, N_("Oculars popup menu"), "displayPopupMenu()", "Alt+O");
	actionShowOcular = addAction("actionShow_Ocular", ocularsGroup, N_("Ocular view"), "flagModeOcular", "Ctrl+O");
	actionShowSensor = addAction("actionShow_Sensor", ocularsGroup, N_("Image sensor frame"), "flagModeSensor");
	actionShowTelrad = addAction("actionShow_Telrad", ocularsGroup, N_("Telrad sight"), "flagModeTelrad", "Ctrl+B");
	actionShowFinder = addAction("actionShow_Finder", ocularsGroup, N_("Finder view"), "flagModeFinder"); // TODO: Define default hotkey? Ctrl+Alt+B?
	actionShowCrosshairs = addAction("actionShow_Ocular_Crosshairs", ocularsGroup, N_("Show crosshairs"), "enableCrosshairs", "Alt+C");
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
			// height & width based solely off of that dimension.
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

				const double ratioLimit = 0.375;
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
					QString cxt, cyt, coords;
					bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
					if (getFlagHorizontalCoordinates())
					{
						bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
						coords = QString("%1:").arg(qc_("Az/Alt of cross", "abbreviated in the plugin"));
						StelUtils::rectToSphe(&cy,&cx,core->equinoxEquToAltAz(centerPosition, StelCore::RefractionAuto));
						const double direction = (useSouthAzimuth ? 2. : 3.); // N is zero, E is 90 degrees
						cy = direction*M_PI - cy;
						if (cy > M_PI*2)
							cy -= M_PI*2;

						if (withDecimalDegree)
						{
							cxt = StelUtils::radToDecDegStr(cy);
							cyt = StelUtils::radToDecDegStr(cx);
						}
						else
						{
							cxt = StelUtils::radToDmsStr(cy);
							cyt = StelUtils::radToDmsStr(cx);
						}
					}
					else
					{
						coords = QString("%1:").arg(qc_("RA/Dec (J2000.0) of cross", "abbreviated in the plugin"));
						StelUtils::rectToSphe(&cx,&cy,core->equinoxEquToJ2000(centerPosition, StelCore::RefractionOff)); // Calculate RA/DE (J2000.0) and show it...
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
					}

					float scaleFactor = static_cast<float>(1.2 * params.devicePixelsPerPixel);
					// Coordinates of center of visible field of view for CCD (red rectangle)
					a = transform.map(QPoint(qRound(-width*0.5f), qRound(height*0.5f + 5.f + fontSize*scaleFactor)));
					painter.drawText(a.x(), a.y(), coords, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
					coords = QString("%1/%2").arg(cxt.simplified(), cyt);
					a = transform.map(QPoint(qRound(-width*0.5f), qRound(height*0.5f + 5.f)));
					painter.drawText(a.x(), a.y(), coords, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
					// Dimensions of visible field of view for CCD (red rectangle)
					a = transform.map(QPoint(qRound(-width*0.5f), qRound(-height*0.5f - fontSize*scaleFactor)));
					painter.drawText(a.x(), a.y(), getDimensionsString(fovX, fovY), static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
					// Horizontal and vertical scales of visible field of view for CCD (red rectangle)
					//TRANSLATORS: Unit of measure for scale - arc-seconds per pixel
					QString unit = q_("\"/px");
					QString scales = QString("%1%3 %4 %2%3").arg(QString::number(fovX*3600*ccd->binningX()/ccd->resolutionX(), 'f', 4), QString::number(fovY*3600*ccd->binningY()/ccd->resolutionY(), 'f', 4), unit, QChar(0x00D7));
					a = transform.map(QPoint(qRound(width*0.5f - painter.getFontMetrics().boundingRect(scales).width()*params.devicePixelsPerPixel), qRound(-height*0.5f - fontSize*scaleFactor)));
					painter.drawText(a.x(), a.y(), scales, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
					// Rotation angle of visible field of view for CCD (red rectangle)
					QString angle = QString("%1%2").arg(QString::number(ccd->chipRotAngle(), 'f', 1)).arg(QChar(0x00B0));
					a = transform.map(QPoint(qRound(width*0.5f - painter.getFontMetrics().boundingRect(angle).width()*params.devicePixelsPerPixel), qRound(height*0.5f + 5.f)));
					painter.drawText(a.x(), a.y(), angle, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));

					if(flagShowCcdCropOverlay && (ccdXRatio>=ratioLimitCrop || ccdYRatio>=ratioLimitCrop))
					{
						// show the CCD crop overlay text
						QString resolutionOverlayText = QString("%1%3 %4 %2%3").arg(QString::number(actualCropOverlayX, 'd', 0), QString::number(actualCropOverlayY, 'd', 0), qc_("px", "pixels"), QChar(0x00D7));
						if(actualCropOverlayX!=ccdCropOverlayHSize || actualCropOverlayY!=ccdCropOverlayVSize)
							resolutionOverlayText.append(" [*]");
						a = transform.map(QPoint(qRound(overlayWidth*0.5f - painter.getFontMetrics().boundingRect(resolutionOverlayText).width()*params.devicePixelsPerPixel), qRound(-overlayHeight*0.5f - fontSize*scaleFactor)));
						painter.drawText(a.x(), a.y(), resolutionOverlayText, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
					}

					if (getFlagMaxExposureTimeForCCD() && selectedSSO!=Q_NULLPTR)
					{
						double properMotion = StelUtils::fmodpos(selectedSSO->getHourlyProperMotion(core)[0], 2.0*M_PI) * M_180_PI;
						if (properMotion>0.)
						{
							double sqf = qMin(fovX*3600*ccd->binningX()/ccd->resolutionX(), fovY*3600*ccd->binningY()/ccd->resolutionY());
							double exposure = 3600.*sqf/qRound(3600.*properMotion);
							if (exposure>0.)
							{
								// TRANSLATORS: "Max exposure" is short version of phrase "Max time of exposure"
								QString exposureTime = QString("%1: %2 %3").arg(q_("Max exposure"), QString::number(qRound(exposure*10.)/10., 'd', 1), qc_("s", "time"));
								a = transform.map(QPoint(qRound(width*0.5f - painter.getFontMetrics().boundingRect(exposureTime).width()*params.devicePixelsPerPixel), qRound(-height*0.5f - 2.0f*fontSize*scaleFactor)));
								painter.drawText(a.x(), a.y(), exposureTime, static_cast<float>(-(ccd->chipRotAngle() + polarAngle)));
							}
						}
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
	Q_ASSERT(pluginMode==OcOcular);
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
	Q_ASSERT(pluginMode==OcTelrad);
	StelCore *core = StelApp::getInstance().getCore();
	const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);
	// StelPainter drawing
	StelPainter painter(projector);
	painter.setColor(lineColor);
	Vec2i centerScreen(projector->getViewportPosX()+projector->getViewportWidth()/2,
			   projector->getViewportPosY()+projector->getViewportHeight()/2);
	const float pixelsPerRad = projector->getPixelPerRadAtCenter(); // * params.devicePixelsPerPixel;
	for (int i=0; i<4; i++)
		if (telradFOV[i]>0.f) painter.drawCircle(centerScreen[0], centerScreen[1], 0.5f * pixelsPerRad * static_cast<float>(M_PI/180) * (telradFOV[i]));
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
	// Paint the reticle, if needed
	if (!reticleTexture.isNull())
	{
		painter.setColor(lineColor);
		reticleTexture->bind();
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
		bool flipH = core->getFlipHorz();
		bool flipV = core->getFlipVert();
		if (flipH && flipV)
			protractorFlipHVTexture->bind();
		else if (flipH && !flipV)
			protractorFlipHTexture->bind();
		else if (!flipH && flipV)
			protractorFlipVTexture->bind();
		else
			protractorTexture->bind();
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
	Finder *finder = Q_NULLPTR;
	if(selectedFinderIndex != -1)
	{
		finder = finders[selectedFinderIndex];
	}
	Lens *lens = selectedLens();

	// set up the color and the GL state
	painter.setColor(textColor);
	painter.setBlending(true);

	// Get the X & Y positions, and the line height
	painter.setFont(font);
	QString widthString = "MMMMMMMMMMMMMMMMMMMMMM";
	const double insetFromRHS = painter.getFontMetrics().boundingRect(widthString).width();
	StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
	int yPositionOffset = qRound(projectorParams.viewportXywh[3]*projectorParams.viewportCenterOffset[1]);
	const double ppx = projectorParams.devicePixelsPerPixel;
	int xPosition = qRound(ppx*(projectorParams.viewportXywh[2] - insetFromRHS));
	int yPosition = qRound(ppx*(projectorParams.viewportXywh[3] - 20) - yPositionOffset);
	const int lineHeight = qRound(ppx*painter.getFontMetrics().height());

	// The Ocular
	if (pluginMode==OcOcular && ocular!=Q_NULLPTR)
	{
		QString ocularNumberLabel;
		QString name = ocular->name();
		QString ocularI18n = ocular->isBinoculars() ? q_("Binocular") : q_("Ocular");
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

	// The Finder
	else if (pluginMode==OcFinder && finder!=Q_NULLPTR)
	{
		QString finderNumberLabel;
		QString name = finder->name();
		QString finderI18n = q_("Finder");
		if (name.isEmpty())
		{
			finderNumberLabel = QString("%1 #%2").arg(finderI18n).arg(selectedFinderIndex);
		}
		else
		{
			finderNumberLabel = QString("%1 #%2: %3").arg(finderI18n).arg(selectedFinderIndex).arg(name);
		}
		// The name of the finder could be really long.
		if (name.length() > widthString.length())
		{
			xPosition -= qRound(insetFromRHS*0.5);
		}
		painter.drawText(xPosition, yPosition, finderNumberLabel);
		yPosition-=lineHeight;

		// General info
		QString magString = QString::number(finder->magnification(), 'f', 1);
		magString.append(QChar(0x02E3)); // Was 0x00D7
		painter.drawText(xPosition, yPosition, QString(q_("Magnification: %1")).arg(magString));
		yPosition-=lineHeight;

		QString fApertureLabel = QString(q_("Finder aperture: %1")).arg(QString::number(finder->aperture(), 'f', 1));
		painter.drawText(xPosition, yPosition, fApertureLabel);
		yPosition-=lineHeight;

		QString fovString = QString::number(finder->trueFOV(), 'f', 5);
		fovString.append(QChar(0x00B0));//Degree sign
		painter.drawText(xPosition, yPosition, QString(q_("True FOV: %1")).arg(fovString));
		yPosition-=lineHeight;

		QString finderFov = QString::number(finder->apparentFOV(), 'f', 2);
		finderFov.append(QChar(0x00B0));//Degree sign
		// TRANSLATORS: aFOV = apparent field of view
		QString finderFOVLabel = QString(q_("Finder aFOV: %1")).arg(finderFov);
		painter.drawText(xPosition, yPosition, finderFOVLabel);
		yPosition-=lineHeight;

		QString exitPupil = QString::number(finder->aperture()/finder->magnification(), 'f', 2);
		painter.drawText(xPosition, yPosition, QString(q_("Exit pupil: %1 mm")).arg(exitPupil));
	}

	// A Sensor
	else if (pluginMode==OcSensor && ccd!=Q_NULLPTR)
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
	if(!QFileInfo::exists(ocularIniPath))
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


// TODO: Finish the disentanglement of switching modes.


// Central mode switch for this plugin. We know 4 states:
// OcNone: Plugin not active while drawing the main view. Maybe a menu in the top-right corner, and the popup menu.
// OcTelrad: Active with a simple Telrad-like screen overlay. Maybe a menu in the top-right corner. As naked-eye viewfinder, this has the main program's settings for brightness/star sizes etc, and may want to zoom in to some maximum FoV.
// OcOcular: view through Ocular+Telescope or Ocular(if flagged as "binocular"). Uses different settings for star scaling, magnitude limit (either manual or auto-determined). Needs many load/store operations on enter/leave
// OcSensor: view through Telescope+Lens+Sensor. Has its own star size settings, but uses custom star limit from main program. Needs some load/store on enter/leave.

// It seems the switching needs to work in 3 phases.
// 1. Switching out of a special mode like Ocular or sensor view.
// 2. Prepare settings for the new mode. If this is not possible, prepare OcNone mode.
// 3. Enter the new mode.
void Oculars::setPluginMode(PluginMode newMode)
{
	if (newMode==pluginMode)
		return;

	StelCore *core = StelApp::getInstance().getCore();
	LabelMgr* labelManager = GETSTELMODULE(LabelMgr);
	StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();
	StelSkyDrawer *skyDrawer = core->getSkyDrawer();
	StelMovementMgr *movementManager = core->getMovementMgr();

	// Try to avoid linked side effects like zooming. Only view quality settings should be affected.
	// 1. We always switch first to ground state (OcNone), then (2) prepare the new state, or set transition to OcNone if there are obstacles, and finally (3) activate the new state.
	switch (pluginMode) // Switching OUT OF this mode to OcNone...
	{
		case OcNone:
			break;
		case OcTelrad:
			// was toggleTelrad(false);
			if (getFlagInitFovUsage()) // Restoration of FOV is needed?
						movementManager->zoomTo(movementManager->getInitFov());
			// TODO 2021: Decide if this is really useful? We only de-activate a red zero-magnification finder. Why revert to another view direction?
			if (getFlagInitDirectionUsage())
				movementManager->setViewDirectionJ2000(StelApp::getInstance().getCore()->altAzToJ2000(movementManager->getInitViewingDirection(), StelCore::RefractionOff));
			flagModeTelrad=false;
			emit flagModeTelradChanged(false);
			break;
		case OcFinder:

			// magnitude limits and star scales.
			disconnect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double)));
			// restore values, but keep current to enable toggling.
			if (!getFlagAutoLimitMagnitude())
			{
				flagLimitStarsFinder=skyDrawer->getFlagStarMagnitudeLimit();
				magLimitStarsFinder=skyDrawer->getCustomStarMagnitudeLimit();
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


			handlePlanetScaling(true);
			// Set the screen display
			core->setFlipHorz(flipHorzMain);
			core->setFlipVert(flipVertMain);

			// from unzoomOcular()
			if (getFlagInitFovUsage())
				movementManager->zoomTo(movementManager->getInitFov());
			else
				movementManager->zoomTo(initialFOV);

			// TODO 2021: Decide if this is really useful? We only de-activate the finder. Why revert to another view direction?
			if (getFlagInitDirectionUsage())
				movementManager->setViewDirectionJ2000(core->altAzToJ2000(movementManager->getInitViewingDirection(), StelCore::RefractionOff));

			flagModeFinder=false;
			emit flagModeFinderChanged(false);
			break;
		case OcOcular:
			// TODO switch out Ocular-specific settings

			if (flagHideGridsLines)
				handleLines(true);

			propMgr->setStelPropertyValue("MilkyWay.saturation", milkyWaySaturationMain);
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
			// TODO: The next 3 could likewise actually restore previously stored (configurable) settings
			movementManager->setFlagTracking(false);
			movementManager->setFlagEnableZoomKeys(true);
			movementManager->setFlagEnableMouseNavigation(true);

			handlePlanetScaling(true);
			// Set the screen display
			core->setFlipHorz(flipHorzMain);
			core->setFlipVert(flipVertMain);

			// from enableOcular(false)
			// remove the usage label if it is being displayed.
			hideUsageMessageIfDisplayed();
			//flagShowOculars = enableOcularMode;
			// TODO: Make sure zoom() has no side effect about ocularMode / flagOcularShow.
			//zoom(false);
			if (guiPanel)
				guiPanel->foldGui();

			// from unzoomOcular()
			if (getFlagInitFovUsage())
				movementManager->zoomTo(movementManager->getInitFov());
			else
				movementManager->zoomTo(initialFOV);

			if (getFlagInitDirectionUsage())
				movementManager->setViewDirectionJ2000(core->altAzToJ2000(movementManager->getInitViewingDirection(), StelCore::RefractionOff));

			flagModeOcular=false;
			emit flagModeOcularChanged(false);

			break;
		case OcSensor:
			// TODO switch out Sensor-specific settings. Part of previous enableOcular()
			//toggleCCD(false);
			//selectedCCDIndex = -1; // TODO 2021: Required?


			// Rest from toggleCCD(false):
			//flagShowCCD = false;

			handlePlanetScaling(true);

			// Restore star scales
			relativeStarScaleCCD=skyDrawer->getRelativeStarScale();
			absoluteStarScaleCCD=skyDrawer->getAbsoluteStarScale();
			skyDrawer->setRelativeStarScale(relativeStarScaleMain);
			skyDrawer->setAbsoluteStarScale(absoluteStarScaleMain);
			movementManager->setFlagTracking(false); // TODO 2021: restore a stored setting instead?
			//Zoom out
			if (getFlagInitFovUsage())
				movementManager->zoomTo(movementManager->getInitFov());
			else if (newMode!=OcTelrad)
				movementManager->zoomTo(initialFOV);

			if (getFlagInitDirectionUsage())
				movementManager->setViewDirectionJ2000(core->altAzToJ2000(movementManager->getInitViewingDirection(), StelCore::RefractionOff));

			if (getFlagAutosetMountForCCD())
				propMgr->setStelPropertyValue("StelMovementMgr.equatorialMount", equatorialMountEnabledMain);

			if (guiPanel)
				guiPanel->foldGui();
			flagModeSensor=false;
			emit flagModeSensorChanged(false);

			break;
	}

	// 2. We are in state OcNone without having set this value yet.
	// Now try to prepare the new settings, or force OcNone if something goes wrong.
	switch (newMode) // only prepare switching INTO this mode...
	{
		case OcNone:    // fallthrough
		case OcTelrad:
			pluginMode=newMode;
			break;
		case OcFinder:
			if (finders.isEmpty())
			{
				selectedFinderIndex = -1;
				qWarning() << "No finders found";
				pluginMode=OcNone;
				break;
			}
			else if (finders.count() > 0 && selectedFinderIndex == -1)
			{
				selectedFinderIndex = 0;
			}
			pluginMode=OcFinder;
			break;
		case OcOcular:
			pluginMode=OcOcular;
			// TODO switch in Ocular-specific settings?
			// TODO: Decide if a warning panel like for sensors should be displayed?
			// Check to ensure that we have enough oculars & telescopes, as they may have been edited in the config dialog
			// was in enableOcularMode(true)
			if (oculars.isEmpty())
			{
				selectedOcularIndex = -1;
				qWarning() << "No oculars found";
				pluginMode=OcNone;
				break;
			}
			else if (oculars.count() > 0 && selectedOcularIndex == -1)
			{
				selectedOcularIndex = 0;
			}
			if (telescopes.isEmpty())
			{
				selectedTelescopeIndex = -1;
				qWarning() << "No telescopes found";
				pluginMode=OcNone;
				break;
			}
			else if (telescopes.count() > 0 && selectedTelescopeIndex == -1)
			{
				selectedTelescopeIndex = 0;
			}
			//if (!ready  || selectedOcularIndex == -1 ||  (selectedTelescopeIndex == -1 && !isBinocularDefined()))
			//{
			//	qWarning() << "The Oculars module has been disabled.";
			//	pluginMode=OcNone; // we are in state OcNone and will not enable anything now.
			//}


			// We may want to ensure there is a selected object. Disable switching if so configured.
			if (flagRequireSelection && !StelApp::getInstance().getStelObjectMgr().getWasSelected() )
			{
				if (usageMessageLabelID == -1)
				{
					QFontMetrics metrics(font);
					QString labelText = q_("Please select an object before switching to ocular view.");
					StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
					int yPositionOffset = qRound(projectorParams.viewportXywh[3]*projectorParams.viewportCenterOffset[1]);
					int xPosition = qRound(projectorParams.viewportCenter[0] - 0.5 * metrics.boundingRect(labelText).width());
					int yPosition = qRound(projectorParams.viewportCenter[1] - yPositionOffset - 0.5 * metrics.height());
					usageMessageLabelID = labelManager->labelScreen(labelText, xPosition, yPosition, true, font.pixelSize(), "#99FF99");
				}
				pluginMode=OcNone;
			}
			else
			{
				hideUsageMessageIfDisplayed();
			}
			break;
		case OcSensor:
			// TODO prepare Sensor-specific settings?
			// From toggleCCD(bool show)
			//If there are no sensors...
			if (ccds.isEmpty() || telescopes.isEmpty())
			{
				//TODO: BM: Make this an on-screen message and/or disable the button if there are no sensors.
				qWarning() << "Oculars plugin: Unable to display a sensor boundary: No sensors or telescopes are defined.";
				QMessageBox::warning(&StelMainView::getInstance(), q_("Warning!"), q_("Unable to display a sensor boundary: No sensors or telescopes are defined."), QMessageBox::Ok);
				//flagShowCCD = false;
				selectedCCDIndex = -1;
				pluginMode=OcNone;
			}
			pluginMode=OcSensor;
			break;
	}


	// 3. pluginMode has been set. Now really switch around what's needed
	switch (pluginMode){
		case OcNone:
			if (guiPanel)
				guiPanel->foldGui();
			break;
		case OcTelrad:
			// was: toggleTelrad(true);
			//hideUsageMessageIfDisplayed();
			//enableOcular(false);
			//toggleCCD(false);
			// NOTE: Added special zoom level for Telrad
			if (flagScalingFOVForTelrad)
			{
				float fov = qMax(qMax(telradFOV[0], telradFOV[1]), qMax(telradFOV[2], telradFOV[3]));
				movementManager->zoomTo(static_cast<double>(fov)*2.);
			}
			// TODO 2021: Decide if this is really useful? We only activate a red zero-magnification finder where we need it. Why revert to another view direction?
			if (getFlagInitDirectionUsage())
				movementManager->setViewDirectionJ2000(StelApp::getInstance().getCore()->altAzToJ2000(movementManager->getInitViewingDirection(), StelCore::RefractionOff));
			// TODO 2021: While we are at it, shall activating the Telrad center on a selected object?

			flagModeTelrad=true;
			emit flagModeTelradChanged(true);

			break;
		case OcFinder:
			if (guiPanel)
				guiPanel->showFinderGui();

			handlePlanetScaling(false);

			flipHorzMain = core->getFlipHorz();
			flipVertMain = core->getFlipVert();

			initialFOV = core->getMovementMgr()->getCurrentFov();
			movementManager->zoomTo(1.2*finders[selectedFinderIndex]->trueFOV());

			// TODO 2021: Decide if this is really useful? We only activate a red zero-magnification finder where we need it. Why revert to another view direction?
			if (StelApp::getInstance().getStelObjectMgr().getWasSelected())
				movementManager->setViewDirectionJ2000(StelApp::getInstance().getCore()->altAzToJ2000(movementManager->getInitViewingDirection(), StelCore::RefractionOff));
			// TODO 2021: While we are at it, shall activating the Finder center on a selected object?

			flagModeFinder=true;
			emit flagModeFinderChanged(true);

			break;
		case OcOcular:
			if (guiPanel)
				guiPanel->showOcularGui();

			if (flagHideGridsLines)
				handleLines(false);


			// this block is from previous zoom(false). These are side effects unrelated to a "zooming" operation.

			milkyWaySaturationMain	= propMgr->getStelPropertyValue("MilkyWay.saturation").toDouble();
			propMgr->setStelPropertyValue("MilkyWay.saturation", 0.f);

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
			skyDrawer->setFlagLuminanceAdaptation(false);  // Will this ever be restored to the state set by the user?

			// Change relative and absolute scales for stars
			relativeStarScaleMain=skyDrawer->getRelativeStarScale();
			skyDrawer->setRelativeStarScale(relativeStarScaleOculars);
			absoluteStarScaleMain=skyDrawer->getAbsoluteStarScale();
			skyDrawer->setAbsoluteStarScale(absoluteStarScaleOculars);

			handlePlanetScaling(false);

			flipHorzMain = core->getFlipHorz();
			flipVertMain = core->getFlipVert();

			initialFOV = core->getMovementMgr()->getCurrentFov();
			// TODO 2021: Store these settings first, and restore on leaving OcOcular mode above!
			movementManager->setFlagTracking(true);
			movementManager->setFlagEnableZoomKeys(false);
			movementManager->setFlagEnableMouseNavigation(false);

			zoomOcular();
			flagModeOcular=true;
			emit flagModeOcularChanged(true);
			break;
		case OcSensor:
			initialFOV = movementManager->getCurrentFov();
			//Mutually exclusive with the ocular mode
			hideUsageMessageIfDisplayed();

			selectedTelescopeIndex = qMax(0, selectedTelescopeIndex);
			selectedCCDIndex = qMax(0, selectedCCDIndex);
			//flagShowCCD = true;
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

			flagModeSensor=true;
			emit flagModeSensorChanged(true);
			break;
	}
	emit pluginModeChanged(pluginMode); // May trigger additional effects. TODO Investigate which, and actually avoid them if possible (easier maintenance!)
}

// 4 flag setters which are needed for the GUI buttons only :-(
void Oculars::setFlagModeOcular(bool show)
{
	setPluginMode(show ? OcOcular : OcNone );
}
void Oculars::setFlagModeSensor(bool show)
{
	setPluginMode(show ? OcSensor : OcNone );
}
void Oculars::setFlagModeTelrad(bool show)
{
	setPluginMode(show ? OcTelrad : OcNone );
}
void Oculars::setFlagModeFinder(bool show)
{
	setPluginMode(show ? OcFinder : OcNone );
}

// TODO 2021: Misnomer: This is not "toggle" (flip whatever state) but "switch"
// This should be called when a new state is entered
void Oculars::handleLines(bool visibleMain)
{
	//if (flagShowTelrad)
	//	return;
	// TODO 2021: Probably this is a better ensurement: But get rid of that later!
	//if (visible)
	//{
	//	Q_ASSERT((!flagHideGridsLines) || (pluginMode==OcNone) || (pluginMode==OcTelrad) || (pluginMode==OcFinder));
	//}
	//else {
	//	Q_ASSERT((flagHideGridsLines) && ((pluginMode==OcOcular) || (pluginMode==OcSensor)) );
	//}

	StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();

	if (visibleMain) // restore previous settings
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
		flagGridLinesDisplayedMain	= propMgr->getStelPropertyValue("GridLinesMgr.gridlinesDisplayed").toBool();
		flagCardinalPointsMain		= propMgr->getStelPropertyValue("LandscapeMgr.cardinalsPointsDisplayed").toBool();
		flagConstellationLinesMain	= propMgr->getStelPropertyValue("ConstellationMgr.linesDisplayed").toBool();
		flagConstellationBoundariesMain	= propMgr->getStelPropertyValue("ConstellationMgr.boundariesDisplayed").toBool();
		flagAsterismLinesMain		= propMgr->getStelPropertyValue("AsterismMgr.linesDisplayed").toBool();
		flagRayHelpersLinesMain		= propMgr->getStelPropertyValue("AsterismMgr.rayHelpersDisplayed").toBool();
		propMgr->setStelPropertyValue("GridLinesMgr.gridlinesDisplayed", false);
		propMgr->setStelPropertyValue("LandscapeMgr.cardinalPointsDisplayed", false);
		propMgr->setStelPropertyValue("ConstellationMgr.linesDisplayed", false);
		propMgr->setStelPropertyValue("ConstellationMgr.boundariesDisplayed", false);
		propMgr->setStelPropertyValue("AsterismMgr.linesDisplayed", false);
		propMgr->setStelPropertyValue("AsterismMgr.rayHelpersDisplayed", false);
	}
}

void Oculars::handlePlanetScaling(bool scaleMain)
{
	SolarSystem *ss=GETSTELMODULE(SolarSystem);
	if (scaleMain)
	{
		ss->setFlagMoonScale(flagMoonScaleMain);
		ss->setFlagMinorBodyScale(flagMinorBodiesScaleMain);
		ss->setFlagPlanetScale(flagPlanetScaleMain);
		ss->setFlagSunScale(flagSunScaleMain);
	}
	else
	{
		flagMoonScaleMain        = ss->getFlagMoonScale();
		flagMinorBodiesScaleMain = ss->getFlagMinorBodyScale();
		flagPlanetScaleMain      = ss->getFlagPlanetScale();
		flagSunScaleMain         = ss->getFlagSunScale();
		ss->setFlagMoonScale(false);
		ss->setFlagMinorBodyScale(false);
		ss->setFlagPlanetScale(false);
		ss->setFlagSunScale(false);
	}
}


// Call this when toggling between main (none), oculars and finder modes or when changing instruments within these modes. Other modes are seen as error.
// Either current pluginMode or newMode MUST be OcNone
// The call MUST be processed in the old pluginMode, announcing the new mode in the argument.
void Oculars::handleMagnitudeLimits(PluginMode newMode)
{
	Q_ASSERT((pluginMode==OcNone && newMode!=OcNone) || (pluginMode!=OcNone && newMode==OcNone) || (pluginMode==newMode));

	// This may be helpful while developing, to call only where useful.
	if (pluginMode==OcNone)
	{
		Q_ASSERT((newMode==OcOcular) || (newMode==OcFinder));
	}
	else if ((pluginMode==OcOcular) || (pluginMode==OcFinder))
	{
		Q_ASSERT(newMode==OcNone);
	}
	StelCore *core = StelApp::getInstance().getCore();
	StelSkyDrawer *skyDrawer = core->getSkyDrawer();

	const bool switchOff= (pluginMode!=OcNone) && (newMode==OcNone);
	const bool onlyRecalcLimits=(newMode==pluginMode); // This must then inhibit storing 'Main' values.

	double limitMagComputed=0.;
	double limitMagStars, limitMagPlanets, limitMagDSOs;
	bool flagLimitMagStars, flagLimitMagPlanets, flagLimitMagDSOs;

	// If we are going out of special modes, we *may* need to retain the manually set values.
	if (switchOff && !getFlagAutoLimitMagnitude())
	{
		switch (pluginMode)
		{
			case OcOcular:
				flagLimitStarsOculars=skyDrawer->getFlagStarMagnitudeLimit();
				flagLimitPlanetsOculars=skyDrawer->getFlagPlanetMagnitudeLimit();
				flagLimitDSOsOculars=skyDrawer->getFlagNebulaMagnitudeLimit();

				magLimitStarsOculars=skyDrawer->getCustomStarMagnitudeLimit();
				magLimitPlanetsOculars=skyDrawer->getCustomPlanetMagnitudeLimit();
				magLimitDSOsOculars=skyDrawer->getCustomNebulaMagnitudeLimit();
				break;
			case OcFinder:
				flagLimitStarsFinder=skyDrawer->getFlagStarMagnitudeLimit();
				flagLimitPlanetsFinder=skyDrawer->getFlagPlanetMagnitudeLimit();
				flagLimitDSOsFinder=skyDrawer->getFlagNebulaMagnitudeLimit();

				magLimitStarsFinder=skyDrawer->getCustomStarMagnitudeLimit();
				magLimitPlanetsFinder=skyDrawer->getCustomPlanetMagnitudeLimit();
				magLimitDSOsFinder=skyDrawer->getCustomNebulaMagnitudeLimit();
				break;
			default:
				break;
		}
	}



//	if (newMode==pluginMode)
//	{
//		limitMagStars=skyDrawer->getCustomStarMagnitudeLimit();
//		limitMagPlanets=skyDrawer->getCustomPlanetMagnitudeLimit();
//		limitMagDSOs=skyDrawer->getCustomNebulaMagnitudeLimit();
//	}
//	else
	switch (newMode)
	{
		case OcNone:
		case OcTelrad:
		case OcSensor:
			flagLimitMagStars=flagLimitStarsMain;
			flagLimitMagPlanets=flagLimitPlanetsMain;
			flagLimitMagDSOs=flagLimitDSOsMain;

			limitMagStars=magLimitStarsMain;
			limitMagPlanets=magLimitPlanetsMain;
			limitMagDSOs=magLimitDSOsMain;
			break;
		case OcOcular:
			flagLimitMagStars=flagLimitStarsOculars;
			flagLimitMagPlanets=flagLimitPlanetsOculars;
			flagLimitMagDSOs=flagLimitDSOsOculars;

			limitMagComputed=computeLimitMagnitude(telescopes[selectedTelescopeIndex]->diameter());
			limitMagStars=magLimitStarsOculars;
			limitMagPlanets=magLimitPlanetsOculars;
			limitMagDSOs=magLimitDSOsOculars;
			break;
		case OcFinder:
			flagLimitMagStars=flagLimitStarsFinder;
			flagLimitMagPlanets=flagLimitPlanetsFinder;
			flagLimitMagDSOs=flagLimitDSOsFinder;

			limitMagComputed=computeLimitMagnitude(finders[selectedFinderIndex]->aperture());
			limitMagStars=magLimitStarsFinder;
			limitMagPlanets=magLimitPlanetsFinder;
			limitMagDSOs=magLimitDSOsFinder;
			break;
	}


	// from zoomOcular()

	// Limit stars/planet/DSOs magnitude. Either compute limiting magnitude for the telescope/ocular//finder,
	// or just use the custom oculars mode value.
	// TODO: What about planets?
	if (getFlagAutoLimitMagnitude() && ((newMode==OcOcular) || (newMode==OcFinder)))
	{
		disconnect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)),    this, SLOT(setMagLimitStarsOcularsManual(double)));   // we want to keep the old manual value.
		disconnect(skyDrawer, SIGNAL(customPlanetsMagLimitChanged(double)), this, SLOT(setMagLimitPlanetsOcularsManual(double))); // we want to keep the old manual value.
		disconnect(skyDrawer, SIGNAL(customNebulaMagLimitChanged(double)),  this, SLOT(setMagLimitDSOsOcularsManual(double)));    // we want to keep the old manual value.

		// TODO: Is it really OK to apply the star formula to planets and esp. DSO?
		skyDrawer->setFlagStarMagnitudeLimit(true);
		skyDrawer->setCustomStarMagnitudeLimit(limitMagComputed);
		skyDrawer->setFlagPlanetMagnitudeLimit(true);
		skyDrawer->setCustomPlanetMagnitudeLimit(limitMagComputed);
		skyDrawer->setFlagNebulaMagnitudeLimit(true);
		skyDrawer->setCustomNebulaMagnitudeLimit(limitMagComputed);
	}
	else
	{	// It's possible that the user changes the custom magnitude while viewing, and then changes the ocular.
		// Therefore we need a temporary connection.
		connect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)),   this, SLOT(setMagLimitStarsOcularsManual(double)));
		connect(skyDrawer, SIGNAL(customPlanetMagLimitChanged(double)), this, SLOT(setMagLimitPlanetsOcularsManual(double)));
		connect(skyDrawer, SIGNAL(customNebulaMagLimitChanged(double)), this, SLOT(setMagLimitDSOsOcularsManual(double)));

		skyDrawer->setFlagStarMagnitudeLimit(flagLimitMagStars);
		skyDrawer->setCustomStarMagnitudeLimit(limitMagStars);
		skyDrawer->setFlagPlanetMagnitudeLimit(flagLimitMagPlanets);
		skyDrawer->setCustomPlanetMagnitudeLimit(limitMagPlanets);
		skyDrawer->setFlagNebulaMagnitudeLimit(flagLimitMagDSOs);
		skyDrawer->setCustomNebulaMagnitudeLimit(limitMagDSOs);
	}
}

// zoomOcular() is called only within state OcOcular mode, either after activation or when an instrument has been changed.
void Oculars::zoomOcular()
{
	Q_ASSERT((pluginMode==OcOcular) || (pluginMode==OcFinder));
	StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr *movementManager = core->getMovementMgr();
	//StelSkyDrawer *skyDrawer = core->getSkyDrawer();
	
	// We won't always have a selected object
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected())
	{
		StelObjectP selectedObject = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0];
		movementManager->moveToJ2000(selectedObject->getEquinoxEquatorialPos(core), movementManager->mountFrameToJ2000(Vec3d(0., 0., 1.)), 0.0, StelMovementMgr::ZoomIn);
	}

	// Set the screen display. Main program's flip settings have been stored in the Mode Switcher
	Finder *finder = finders[selectedFinderIndex];
	Ocular *ocular = oculars[selectedOcularIndex];
	Telescope *telescope = Q_NULLPTR;
	Lens *lens = Q_NULLPTR;
	// Only consider flip is we're not binoculars. VERY WRONG! There are angle finders etc.
	//if (ocular->isBinoculars())
	if (pluginMode==OcFinder)
	{
		core->setFlipHorz(finder->isHFlipped());
		core->setFlipVert(finder->isVFlipped());
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


	//handleMagnitudeLimits(OcOcular); // REPLACES THE FOLLOWING LINES
//	// Limit stars and DSOs	magnitude. Either compute limiting magnitude for the telescope/ocular,
//	// or just use the custom oculars mode value.
//	// TODO: What about planets?
//	double limitMag=magLimitStarsOculars;
//	if (getFlagAutoLimitMagnitude() || flagLimitStarsOculars )
//	{
//		if (getFlagAutoLimitMagnitude())
//		{
//			disconnect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double))); // we want to keep the old manual value.
//			limitMag = computeLimitMagnitude(telescope->diameter());
//			// TODO: Is it really good to apply the star formula to DSO and planets?
//			skyDrawer->setFlagNebulaMagnitudeLimit(true);
//			skyDrawer->setCustomNebulaMagnitudeLimit(limitMag);
//			skyDrawer->setFlagPlanetMagnitudeLimit(true);
//			skyDrawer->setCustomPlanetMagnitudeLimit(limitMag);
//		}
//		else
//		{	// It's possible that the user changes the custom magnitude while viewing, and then changes the ocular.
//			// Therefore we need a temporary connection.
//			connect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double)));
//		}
//		skyDrawer->setFlagStarMagnitudeLimit(true);
//	}
//	skyDrawer->setCustomStarMagnitudeLimit(limitMag);

	if (pluginMode==OcFinder)
	{
		actualFOV = finder->trueFOV();
	}
	else
	{
		actualFOV = ocular->actualFOV(telescope, lens);
		// See if the mask was scaled; if so, correct the actualFOV.
		if (flagScaleImageCircle && ocular->apparentFOV() > 0.0 && !ocular->isBinoculars())
		{
			actualFOV = maxEyepieceAngle * actualFOV / ocular->apparentFOV();
		}
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
		QAction* action = submenu->addAction(label, submenu, [=](){selectLensAtIndex(index);});
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
		QAction* action = submenu->addAction(label, submenu, [=](){selectTelescopeAtIndex(index);});
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

void Oculars::setFlagHorizontalCoordinates(const bool b)
{
	flagHorizontalCoordinates = b;
	settings->setValue("use_horizontal_coordinates", b);
	settings->sync();
	emit flagHorizontalCoordinatesChanged(b);
}

bool Oculars::getFlagHorizontalCoordinates() const
{
	return flagHorizontalCoordinates;
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
	Q_ASSERT(pluginMode==OcOcular);

	magLimitStarsOculars = mag;
	settings->setValue("limit_stellar_magnitude_oculars_val", mag);
	settings->sync();
	// This is no property, no need to emit a signal.
}

double Oculars::getMagLimitStarsOcularsManual() const
{
	return magLimitStarsOculars;
}

void Oculars::setMagLimitPlanetsOcularsManual(double mag)
{
	Q_ASSERT(pluginMode==OcOcular);

	magLimitPlanetsOculars = mag;
	settings->setValue("limit_planet_magnitude_oculars_val", mag);
	settings->sync();
	// This is no property, no need to emit a signal.
}

double Oculars::getMagLimitPlanetsOcularsManual() const
{
	return magLimitPlanetsOculars;
}

void Oculars::setMagLimitDSOsOcularsManual(double mag)
{
	Q_ASSERT(pluginMode==OcOcular);

	magLimitDSOsOculars = mag;
	settings->setValue("limit_nebula_magnitude_oculars_val", mag);
	settings->sync();
	// This is no property, no need to emit a signal.
}

double Oculars::getMagLimitDSOsOcularsManual() const
{
	return magLimitDSOsOculars;
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

void Oculars::setTelradFOV(const Vec4f &fov)
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

void Oculars::setFlagMaxExposureTimeForCCD(const bool b)
{
	flagMaxExposureTimeForCCD = b;
	settings->setValue("show_ccd_exposure_time", b);
	settings->sync();
	emit flagMaxExposureTimeForCCDChanged(b);
}

bool Oculars::getFlagMaxExposureTimeForCCD() const
{
	return  flagMaxExposureTimeForCCD;
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

		if (b && (pluginMode==OcOcular))
		{
			handleLines(false);
		}
		else if (!b && (pluginMode==OcOcular))
		{
			// Restore main program state
			handleLines(true);
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
				// Create the oculars button
				toolbarButton = new StelButton(Q_NULLPTR, *pxmapOnIcon, *pxmapOffIcon, *pxmapGlow, "actionShow_Oculars", false, "actionShow_Oculars_dialog");
			}
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		} else {
			gui->getButtonBar()->hideButton("actionShow_Oculars");
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

// Simplified calculation of the penetrating power of the telescope
// A better formula for telescopic limiting magnitudes?
// North, G.; Journal of the British Astronomical Association, vol.107, no.2, p.82
// http://adsabs.harvard.edu/abs/1997JBAA..107...82N
double Oculars::computeLimitMagnitude(double aperture)
{
	return 4.5 + 4.4*std::log10(qMax(aperture, 0.1));  // Avoid a potential call of null pointer, and a log(0) error.
}

void Oculars::handleAutoLimitToggle(bool on)
{
	Q_ASSERT((pluginMode != OcOcular) || (pluginMode != OcFinder)); // Trace principal calling errors...

	if ((pluginMode != OcOcular) || (pluginMode != OcFinder))
		return;

	// When we are in Oculars mode, we must toggle between the auto limit and manual limit. Logic taken from zoomOcular()/unzoomOcular()
	StelCore *core = StelApp::getInstance().getCore();
	StelSkyDrawer *skyDrawer = core->getSkyDrawer();
	if (on)
	{
		//Ocular * ocular = oculars[selectedOcularIndex];
		//Telescope * telescope = (ocular->isBinoculars() ? Q_NULLPTR : telescopes[selectedTelescopeIndex]);
		double instrumentDiameter=(pluginMode==OcOcular ? telescopes[selectedTelescopeIndex]->diameter() : finders[selectedFinderIndex]->aperture());
		disconnect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double))); // keep the old manual value in config.
		double limitMag = computeLimitMagnitude(instrumentDiameter);
		// TODO: Is it really good to apply the star formula to DSO?
		skyDrawer->setFlagNebulaMagnitudeLimit(true);
		skyDrawer->setCustomNebulaMagnitudeLimit(limitMag);
		skyDrawer->setFlagStarMagnitudeLimit(true);
		skyDrawer->setCustomStarMagnitudeLimit(limitMag);
		skyDrawer->setFlagPlanetMagnitudeLimit(true);
		skyDrawer->setCustomPlanetMagnitudeLimit(limitMag);
	}
	else
	{
		connect(skyDrawer, SIGNAL(customStarMagLimitChanged(double)), this, SLOT(setMagLimitStarsOcularsManual(double)));
		if (pluginMode==OcOcular)
		{
			skyDrawer->setCustomStarMagnitudeLimit(magLimitStarsOculars);
			skyDrawer->setFlagStarMagnitudeLimit(flagLimitStarsOculars);
			skyDrawer->setCustomPlanetMagnitudeLimit(magLimitPlanetsOculars);
			skyDrawer->setFlagPlanetMagnitudeLimit(flagLimitPlanetsOculars);
			skyDrawer->setCustomNebulaMagnitudeLimit(magLimitDSOsOculars);
			skyDrawer->setFlagNebulaMagnitudeLimit(flagLimitDSOsOculars);
		}
		else
		{
			skyDrawer->setCustomStarMagnitudeLimit(magLimitStarsFinder);
			skyDrawer->setFlagStarMagnitudeLimit(flagLimitStarsFinder);
			skyDrawer->setCustomPlanetMagnitudeLimit(magLimitPlanetsFinder);
			skyDrawer->setFlagPlanetMagnitudeLimit(flagLimitPlanetsFinder);
			skyDrawer->setCustomNebulaMagnitudeLimit(magLimitDSOsFinder);
			skyDrawer->setFlagNebulaMagnitudeLimit(flagLimitDSOsFinder);

		}
	}
}

// Handle switching the main program's star limitation flag
void Oculars::handleStarMagLimitToggle(bool on)
{
	if (pluginMode != OcOcular)
		return;

	flagLimitStarsOculars=on;
	// It only makes sense to switch off the auto-limit when we switch off the limit.
	if (!on)
	{
		setFlagAutoLimitMagnitude(false);
	}
}

