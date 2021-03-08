/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
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


#include "ViewDialog.hpp"
#include "ui_viewDialog.h"
#include "AddRemoveLandscapesDialog.hpp"
#include "AtmosphereDialog.hpp"
#include "GreatRedSpotDialog.hpp"
#include "ConfigureDSOColorsDialog.hpp"
#include "ConfigureOrbitColorsDialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelProjector.hpp"
#include "StelModuleMgr.hpp"
#include "StarMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "NebulaMgr.hpp"
#include "AsterismMgr.hpp"
#include "StelStyle.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelGuiBase.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelActionMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelUtils.hpp"
#include "StelHips.hpp"

#include <QDebug>
#include <QFrame>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QDialog>
#include <QStringList>
#include <QJsonArray>

ViewDialog::ViewDialog(QObject* parent) : StelDialog("View", parent)
	, addRemoveLandscapesDialog(Q_NULLPTR)
	, atmosphereDialog(Q_NULLPTR)
	, greatRedSpotDialog(Q_NULLPTR)
	, configureDSOColorsDialog(Q_NULLPTR)
	, configureOrbitColorsDialog(Q_NULLPTR)
{
	ui = new Ui_viewDialogForm;	
}

ViewDialog::~ViewDialog()
{
	delete ui;
	ui=Q_NULLPTR;
	delete addRemoveLandscapesDialog;
	addRemoveLandscapesDialog = Q_NULLPTR;
	delete atmosphereDialog;
	atmosphereDialog = Q_NULLPTR;
	delete greatRedSpotDialog;
	greatRedSpotDialog = Q_NULLPTR;
	delete configureDSOColorsDialog;
	configureDSOColorsDialog = Q_NULLPTR;
	delete configureOrbitColorsDialog;
	configureOrbitColorsDialog = Q_NULLPTR;
}

void ViewDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateZhrDescription(StelApp::getInstance().getModule("SporadicMeteorMgr")->property("zhr").toInt());
		populateLists();
		populateToolTips();
		populatePlanetMagnitudeAlgorithmsList();
		populatePlanetMagnitudeAlgorithmDescription();
		setBortleScaleToolTip(StelApp::getInstance().getCore()->getSkyDrawer()->getBortleScaleIndex());
		populateHipsGroups();
		updateHips();
		//Hack to shrink the tabs to optimal size after language change
		//by causing the list items to be laid out again.
		updateTabBarListWidgetWidth();
	}
}

void ViewDialog::styleChanged()
{
	if (dialog)
	{
		populateLists();
		populateToolTips();
		populatePlanetMagnitudeAlgorithmsList();
		populatePlanetMagnitudeAlgorithmDescription();
		populateHipsGroups();
	}
}

void ViewDialog::connectGroupBox(QGroupBox* groupBox, const QString& actionId)
{
	StelAction* action = StelApp::getInstance().getStelActionManager()->findAction(actionId);
	Q_ASSERT(action);
	groupBox->setChecked(action->isChecked());
	connect(action, SIGNAL(toggled(bool)), groupBox, SLOT(setChecked(bool)));
	connect(groupBox, SIGNAL(toggled(bool)), action, SLOT(setChecked(bool)));
}

void ViewDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	// Set the Sky tab activated by default
	ui->stackedWidget->setCurrentIndex(0);
	ui->stackListWidget->setCurrentRow(0);
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
	// Kinetic scrolling
	kineticScrollingList << ui->projectionListWidget << ui->culturesListWidget << ui->skyCultureTextBrowser << ui->landscapesListWidget
			     << ui->landscapeTextBrowser << ui->surveysListWidget << ui->surveysTextBrowser;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	populateLists();
	populateToolTips();

	// TODOs after properties merge:
	// New method: populateLightPollution may be useful. Make sure it is.
	// Jupiter's GRS should become property, and recheck the other "from trunk" entries.
	connect(ui->culturesListWidget, SIGNAL(currentTextChanged(const QString&)),&StelApp::getInstance().getSkyCultureMgr(),SLOT(setCurrentSkyCultureNameI18(QString)));
	connect(&StelApp::getInstance().getSkyCultureMgr(), SIGNAL(currentSkyCultureChanged(QString)), this, SLOT(skyCultureChanged()));

	// Connect and initialize checkboxes and other widgets
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	Q_ASSERT(ssmgr);
	// Stars section
	connectGroupBox(ui->starGroupBox, "actionShow_Stars");
	connectDoubleProperty(ui->starScaleRadiusDoubleSpinBox,"StelSkyDrawer.absoluteStarScale");
	connectDoubleProperty(ui->starRelativeScaleDoubleSpinBox, "StelSkyDrawer.relativeStarScale");
	connectBoolProperty(ui->starTwinkleCheckBox, "StelSkyDrawer.flagStarTwinkle");
	connectDoubleProperty(ui->starTwinkleAmountDoubleSpinBox, "StelSkyDrawer.twinkleAmount");
	connectBoolProperty(ui->starLimitMagnitudeCheckBox,"StelSkyDrawer.flagStarMagnitudeLimit");
	connectDoubleProperty(ui->starLimitMagnitudeDoubleSpinBox, "StelSkyDrawer.customStarMagLimit");
	connectBoolProperty(ui->spikyStarsCheckBox, "StelSkyDrawer.flagStarSpiky");
	connectCheckBox(ui->starLabelCheckBox, "actionShow_Stars_Labels");
	connectDoubleProperty(ui->starsLabelsHorizontalSlider,"StarMgr.labelsAmount",0.0,10.0);
	connectBoolProperty(ui->checkBoxAdditionalNamesStars, "StarMgr.flagAdditionalNamesDisplayed");
	connectBoolProperty(ui->checkBoxStarDesignationsOnlyUsage, "StarMgr.flagDesignationLabels");

	// Sky section
	connectBoolProperty(ui->milkyWayCheckBox, "MilkyWay.flagMilkyWayDisplayed");
	connectDoubleProperty(ui->milkyWayBrightnessDoubleSpinBox, "MilkyWay.intensity");
	connectDoubleProperty(ui->milkyWaySaturationDoubleSpinBox, "MilkyWay.saturation");
	connectBoolProperty(ui->zodiacalLightCheckBox, "ZodiacalLight.flagZodiacalLightDisplayed");
	connectDoubleProperty(ui->zodiacalLightBrightnessDoubleSpinBox, "ZodiacalLight.intensity");
	connectBoolProperty(ui->adaptationCheckbox, "StelSkyDrawer.flagLuminanceAdaptation");

	// Light pollution
	StelModule* lmgr = StelApp::getInstance().getModule("LandscapeMgr");
	Q_ASSERT(lmgr);
	StelSkyDrawer* drawer = StelApp::getInstance().getCore()->getSkyDrawer();
	Q_ASSERT(drawer);
	populateLightPollution();
	connectBoolProperty(ui->useLightPollutionFromLocationDataCheckBox, "LandscapeMgr.flagUseLightPollutionFromDatabase");
	connect(lmgr, SIGNAL(flagUseLightPollutionFromDatabaseChanged(bool)), this, SLOT(populateLightPollution()));
	connectIntProperty(ui->lightPollutionSpinBox, "StelSkyDrawer.bortleScaleIndex");
	connect(drawer, SIGNAL(bortleScaleIndexChanged(int)), this, SLOT(setBortleScaleToolTip(int)));

	// atmosphere details
	connect(ui->pushButtonAtmosphereDetails, SIGNAL(clicked()), this, SLOT(showAtmosphereDialog()));

	// Planets section
	connectGroupBox(ui->planetsGroupBox, "actionShow_Planets");
	connectCheckBox(ui->planetMarkerCheckBox, "actionShow_Planets_Hints");
	connectCheckBox(ui->planetOrbitCheckBox, "actionShow_Planets_Orbits");
	connectBoolProperty(ui->planetIsolatedOrbitCheckBox, "SolarSystem.flagIsolatedOrbits");
	connectBoolProperty(ui->planetOrbitOnlyCheckBox, "SolarSystem.flagPlanetsOrbitsOnly");
	connectBoolProperty(ui->planetOrbitPermanentCheckBox, "SolarSystem.flagPermanentOrbits");
	connectIntProperty(ui->planetOrbitsThicknessSpinBox, "SolarSystem.orbitsThickness");
	connect(ui->pushButtonOrbitColors, SIGNAL(clicked(bool)), this, SLOT(showConfigureOrbitColorsDialog()));
	populateOrbitsControls(ssmgr->getFlagOrbits());
	connect(ssmgr,SIGNAL(flagOrbitsChanged(bool)), this, SLOT(populateOrbitsControls(bool)));
	connectBoolProperty(ui->planetLightSpeedCheckBox, "SolarSystem.flagLightTravelTime");
	connectBoolProperty(ui->planetUseObjModelsCheckBox, "SolarSystem.flagUseObjModels");
	connectBoolProperty(ui->planetShowObjSelfShadowsCheckBox, "SolarSystem.flagShowObjSelfShadows");
	ui->planetShowObjSelfShadowsCheckBox->setEnabled(ssmgr->getFlagUseObjModels());
	connect(ssmgr,SIGNAL(flagUseObjModelsChanged(bool)),ui->planetShowObjSelfShadowsCheckBox, SLOT(setEnabled(bool)));
	connectBoolProperty(ui->planetLimitMagnitudeCheckBox,"StelSkyDrawer.flagPlanetMagnitudeLimit");
	connectDoubleProperty(ui->planetLimitMagnitudeDoubleSpinBox,"StelSkyDrawer.customPlanetMagLimit");
	connectBoolProperty(ui->planetScaleMoonCheckBox, "SolarSystem.flagMoonScale");
	connectDoubleProperty(ui->moonScaleFactor,"SolarSystem.moonScale");
	connectBoolProperty(ui->planetScaleMinorBodyCheckBox, "SolarSystem.flagMinorBodyScale");
	connectDoubleProperty(ui->minorBodyScaleFactor,"SolarSystem.minorBodyScale");
	connectBoolProperty(ui->planetScalePlanetsCheckBox, "SolarSystem.flagPlanetScale");
	connectDoubleProperty(ui->planetScaleFactor,"SolarSystem.planetScale");
	connectBoolProperty(ui->planetScaleSunCheckBox, "SolarSystem.flagSunScale");
	connectDoubleProperty(ui->sunScaleFactor,"SolarSystem.sunScale");
	connectCheckBox(ui->planetLabelCheckBox, "actionShow_Planets_Labels");
	connectCheckBox(ui->planetNomenclatureCheckBox, "actionShow_Planets_Nomenclature");
	connectDoubleProperty(ui->planetsLabelsHorizontalSlider, "SolarSystem.labelsAmount",0.0,10.0);	
	connectCheckBox(ui->planetNomenclatureCheckBox, "actionShow_Planets_Nomenclature");
	connectColorButton(ui->planetNomenclatureColor, "NomenclatureMgr.nomenclatureColor", "color/planet_nomenclature_color");
	connectColorButton(ui->planetLabelColor, "SolarSystem.labelsColor", "color/planet_names_color");
	connectColorButton(ui->planetTrailsColor, "SolarSystem.trailsColor", "color/object_trails_color");
	connectBoolProperty(ui->planetTrailsCheckBox, "SolarSystem.trailsDisplayed");
	connectIntProperty(ui->planetTrailsThicknessSpinBox, "SolarSystem.trailsThickness");	
	connectBoolProperty(ui->planetIsolatedTrailsCheckBox, "SolarSystem.flagIsolatedTrails");
	connectIntProperty(ui->planetIsolatedTrailsSpinBox, "SolarSystem.numberIsolatedTrails");
	connectBoolProperty(ui->drawMoonHaloCheckBox, "SolarSystem.flagDrawMoonHalo");
	connectBoolProperty(ui->drawSunGlareCheckBox, "SolarSystem.flagDrawSunHalo");
	connectBoolProperty(ui->shadowEnlargementDanjonCheckBox, "SolarSystem.earthShadowEnlargementDanjon");
	populateTrailsControls(ssmgr->getFlagTrails());
	connect(ssmgr,SIGNAL(trailsDisplayedChanged(bool)), this, SLOT(populateTrailsControls(bool)));
	connectBoolProperty(ui->hidePlanetNomenclatureCheckBox, "NomenclatureMgr.localNomenclatureHided");
	StelModule* mnmgr = StelApp::getInstance().getModule("NomenclatureMgr");
	ui->hidePlanetNomenclatureCheckBox->setEnabled(mnmgr->property("nomenclatureDisplayed").toBool());
	connect(mnmgr,SIGNAL(nomenclatureDisplayedChanged(bool)),ui->hidePlanetNomenclatureCheckBox, SLOT(setEnabled(bool)));

	populatePlanetMagnitudeAlgorithmsList();
	int idx = ui->planetMagnitudeAlgorithmComboBox->findData(Planet::getApparentMagnitudeAlgorithm(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use Mallama&Hilton_2018 as default
		idx = ui->planetMagnitudeAlgorithmComboBox->findData(Planet::MallamaHilton_2018, Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->planetMagnitudeAlgorithmComboBox->setCurrentIndex(idx);
	connect(ui->planetMagnitudeAlgorithmComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setPlanetMagnitudeAlgorithm(int)));
	populatePlanetMagnitudeAlgorithmDescription();

	// GreatRedSpot (Jupiter)
	connectBoolProperty(ui->customGrsSettingsCheckBox, "SolarSystem.flagCustomGrsSettings");
	ui->pushButtonGrsDetails->setEnabled(ssmgr->getFlagCustomGrsSettings());
	connect(ssmgr, SIGNAL(flagCustomGrsSettingsChanged(bool)), ui->pushButtonGrsDetails, SLOT(setEnabled(bool)));
	connect(ui->pushButtonGrsDetails, SIGNAL(clicked()), this, SLOT(showGreatRedSpotDialog()));

	// Shooting stars section
	StelModule* mmgr = StelApp::getInstance().getModule("SporadicMeteorMgr");
	Q_ASSERT(mmgr);
	connectIntProperty(ui->zhrSpinBox, "SporadicMeteorMgr.zhr");
	connectIntProperty(ui->zhrSlider, "SporadicMeteorMgr.zhr", ui->zhrSlider->minimum(), ui->zhrSlider->maximum());
	updateZhrDescription(mmgr->property("zhr").toInt());
	connect(mmgr, SIGNAL(zhrChanged(int)), this, SLOT(updateZhrDescription(int)));

	// DSO tab contents
	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	updateSelectedCatalogsCheckBoxes();
	connect(nmgr, SIGNAL(catalogFiltersChanged(Nebula::CatalogGroup)), this, SLOT(updateSelectedCatalogsCheckBoxes()));
	connect(ui->buttonGroupDisplayedDSOCatalogs, SIGNAL(buttonClicked(int)), this, SLOT(setSelectedCatalogsFromCheckBoxes()));
	updateSelectedTypesCheckBoxes();
	connect(nmgr, SIGNAL(typeFiltersChanged(Nebula::TypeGroup)), this, SLOT(updateSelectedTypesCheckBoxes()));
	connect(ui->buttonGroupDisplayedDSOTypes, SIGNAL(buttonClicked(int)), this, SLOT(setSelectedTypesFromCheckBoxes()));
	connectGroupBox(ui->groupBoxDSOTypeFilters,"actionSet_Nebula_TypeFilterUsage");
	// DSO Labels section
	connectGroupBox(ui->groupBoxDSOLabelsAndMarkers, "actionShow_Nebulas");
	connectDoubleProperty(ui->nebulasLabelsHorizontalSlider, "NebulaMgr.labelsAmount",0.0,10.0);
	connectDoubleProperty(ui->nebulasHintsHorizontalSlider, "NebulaMgr.hintsAmount",0.0,10.0);
	connectBoolProperty(ui->checkBoxDesignationsOnlyUsage, "NebulaMgr.flagDesignationLabels");
	connectBoolProperty(ui->checkBoxProportionalHints, "NebulaMgr.hintsProportional");
	connectBoolProperty(ui->checkBoxOutlines, "NebulaMgr.flagOutlinesDisplayed");
	connectBoolProperty(ui->checkBoxSurfaceBrightnessUsage, "NebulaMgr.flagSurfaceBrightnessUsage");
	connectBoolProperty(ui->nebulaLimitMagnitudeCheckBox,"StelSkyDrawer.flagNebulaMagnitudeLimit");
	connectBoolProperty(ui->checkBoxAdditionalNamesDSO, "NebulaMgr.flagAdditionalNamesDisplayed");
	connectDoubleProperty(ui->nebulaLimitMagnitudeDoubleSpinBox,"StelSkyDrawer.customNebulaMagLimit");
	connectBoolProperty(ui->nebulaLimitSizeCheckBox, "NebulaMgr.flagUseSizeLimits");
	connectDoubleProperty(ui->nebulaLimitSizeMinDoubleSpinBox, "NebulaMgr.minSizeLimit");
	connectDoubleProperty(ui->nebulaLimitSizeMaxDoubleSpinBox, "NebulaMgr.maxSizeLimit");
	connect(ui->pushButtonConfigureDSOColors, SIGNAL(clicked()), this, SLOT(showConfigureDSOColorsDialog()));

	// Landscape section
	connectCheckBox(ui->showGroundCheckBox, "actionShow_Ground");
	connectCheckBox(ui->showFogCheckBox, "actionShow_Fog");
	connectCheckBox(ui->atmosphereCheckBox, "actionShow_Atmosphere");
	connectCheckBox(ui->landscapeIlluminationCheckBox, "actionShow_LandscapeIllumination");
	connectCheckBox(ui->landscapeLabelsCheckBox, "actionShow_LandscapeLabels");

	connectBoolProperty(ui->landscapePositionCheckBox, "LandscapeMgr.flagLandscapeSetsLocation");

	connectBoolProperty(ui->landscapeBrightnessCheckBox,"LandscapeMgr.flagLandscapeUseMinimalBrightness");
	connect(lmgr,SIGNAL(flagLandscapeUseMinimalBrightnessChanged(bool)),ui->localLandscapeBrightnessCheckBox,SLOT(setEnabled(bool)));
	connect(lmgr,SIGNAL(flagLandscapeUseMinimalBrightnessChanged(bool)),ui->landscapeBrightnessSpinBox,SLOT(setEnabled(bool)));
	ui->localLandscapeBrightnessCheckBox->setEnabled(lmgr->property("flagLandscapeUseMinimalBrightness").toBool());
	ui->landscapeBrightnessSpinBox->setEnabled(lmgr->property("flagLandscapeUseMinimalBrightness").toBool());
	connectDoubleProperty(ui->landscapeBrightnessSpinBox,"LandscapeMgr.defaultMinimalBrightness");
	connectBoolProperty(ui->localLandscapeBrightnessCheckBox,"LandscapeMgr.flagLandscapeSetsMinimalBrightness");
	connectBoolProperty(ui->landscapePolylineCheckBox, "LandscapeMgr.flagPolyLineDisplayedOnly");
	connectIntProperty(ui->landscapePolylineThicknessSpinBox, "LandscapeMgr.polyLineThickness");
	connect(ui->landscapesListWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(changeLandscape(QListWidgetItem*)));
	connect(lmgr, SIGNAL(currentLandscapeChanged(QString,QString)), this, SLOT(landscapeChanged(QString,QString)));
	connect(ui->useAsDefaultLandscapeCheckBox, SIGNAL(clicked()), this, SLOT(setCurrentLandscapeAsDefault()));
	connect(lmgr,SIGNAL(defaultLandscapeChanged(QString)),this,SLOT(updateDefaultLandscape()));
	updateDefaultLandscape();
	connect(lmgr, SIGNAL(landscapesChanged()), this, SLOT(populateLists()));
	connect(ui->pushButtonAddRemoveLandscapes, SIGNAL(clicked()), this, SLOT(showAddRemoveLandscapesDialog()));

	// Grid and lines
	connectGroupBox(ui->celestialSphereGroupBox,			"actionShow_Gridlines");
	connectCheckBox(ui->showEquatorLineCheckBox,			"actionShow_Equator_Line");
	connectCheckBox(ui->showEquatorJ2000LineCheckBox,		"actionShow_Equator_J2000_Line");
	connectCheckBox(ui->showEclipticLineJ2000CheckBox,		"actionShow_Ecliptic_J2000_Line");
	connectCheckBox(ui->showEclipticLineOfDateCheckBox,		"actionShow_Ecliptic_Line");
	connectCheckBox(ui->showInvariablePlaneCheckBox,		"actionShow_Invariable_Plane_Line");
	connectCheckBox(ui->showSolarEquatorCheckBox,			"actionShow_Solar_Equator_Line");
	connectCheckBox(ui->showMeridianLineCheckBox,			"actionShow_Meridian_Line");
	connectCheckBox(ui->showLongitudeLineCheckBox,			"actionShow_Longitude_Line");
	connectCheckBox(ui->showHorizonLineCheckBox,			"actionShow_Horizon_Line");
	connectCheckBox(ui->showEquatorialGridCheckBox,			"actionShow_Equatorial_Grid");
	connectCheckBox(ui->showGalacticGridCheckBox,			"actionShow_Galactic_Grid");
	connectCheckBox(ui->showGalacticEquatorLineCheckBox,		"actionShow_Galactic_Equator_Line");
	connectCheckBox(ui->showSupergalacticGridCheckBox,		"actionShow_Supergalactic_Grid");
	connectCheckBox(ui->showSupergalacticEquatorLineCheckBox,	"actionShow_Supergalactic_Equator_Line");
	connectCheckBox(ui->showAzimuthalGridCheckBox,			"actionShow_Azimuthal_Grid");
	connectCheckBox(ui->showEquatorialJ2000GridCheckBox,		"actionShow_Equatorial_J2000_Grid");
	connectCheckBox(ui->showEclipticGridJ2000CheckBox,		"actionShow_Ecliptic_J2000_Grid");
	connectCheckBox(ui->showEclipticGridOfDateCheckBox,		"actionShow_Ecliptic_Grid");
	connectCheckBox(ui->showCardinalPointsCheckBox,			"actionShow_Cardinal_Points");
	connectCheckBox(ui->showCompassMarksCheckBox,			"actionShow_Compass_Marks");
	connectCheckBox(ui->showPrecessionCirclesCheckBox,		"actionShow_Precession_Circles");
	connectCheckBox(ui->showPrimeVerticalLineCheckBox,		"actionShow_Prime_Vertical_Line");
	connectCheckBox(ui->showCurrentVerticalLineCheckBox,		"actionShow_Current_Vertical_Line");
	connectCheckBox(ui->showColuresLineCheckBox,			"actionShow_Colure_Lines");
	connectCheckBox(ui->showCircumpolarCirclesCheckBox,		"actionShow_Circumpolar_Circles");
	connectCheckBox(ui->showUmbraCheckBox,		                "actionShow_Umbra_Circle");
	connectCheckBox(ui->showPenumbraCheckBox,		        "actionShow_Penumbra_Circle");
	connectCheckBox(ui->showCelestialJ2000PolesCheckBox,		"actionShow_Celestial_J2000_Poles");
	connectCheckBox(ui->showCelestialPolesCheckBox,			"actionShow_Celestial_Poles");
	connectCheckBox(ui->showZenithNadirCheckBox,			"actionShow_Zenith_Nadir");
	connectCheckBox(ui->showEclipticJ2000PolesCheckBox,		"actionShow_Ecliptic_J2000_Poles");
	connectCheckBox(ui->showEclipticPolesCheckBox,			"actionShow_Ecliptic_Poles");
	connectCheckBox(ui->showGalacticPolesCheckBox,			"actionShow_Galactic_Poles");
	connectCheckBox(ui->showGalacticCenterCheckBox,			"actionShow_Galactic_Center");
	connectCheckBox(ui->showSupergalacticPolesCheckBox,		"actionShow_Supergalactic_Poles");
	connectCheckBox(ui->showEquinoxJ2000PointsCheckBox,		"actionShow_Equinox_J2000_Points");
	connectCheckBox(ui->showEquinoxPointsCheckBox,			"actionShow_Equinox_Points");
	connectCheckBox(ui->showSolsticeJ2000PointsCheckBox,		"actionShow_Solstice_J2000_Points");
	connectCheckBox(ui->showSolsticePointsCheckBox,			"actionShow_Solstice_Points");
	connectCheckBox(ui->showAntisolarPointCheckBox,			"actionShow_Antisolar_Point");
	connectCheckBox(ui->showApexPointsCheckBox,			"actionShow_Apex_Points");
	connectCheckBox(ui->showFOVCenterMarkerCheckBox,		"actionShow_FOV_Center_Marker");
	connectCheckBox(ui->showFOVCircularMarkerCheckBox,		"actionShow_FOV_Circular_Marker");
	connectCheckBox(ui->showFOVRectangularMarkerCheckBox,		"actionShow_FOV_Rectangular_Marker");
	connectDoubleProperty(ui->fovCircularMarkerSizeDoubleSpinBox,	          "SpecialMarkersMgr.fovCircularMarkerSize");
	connectDoubleProperty(ui->fovRectangularMarkerWidthDoubleSpinBox,         "SpecialMarkersMgr.fovRectangularMarkerWidth");
	connectDoubleProperty(ui->fovRectangularMarkerHeightDoubleSpinBox,        "SpecialMarkersMgr.fovRectangularMarkerHeight");
	connectDoubleProperty(ui->fovRectangularMarkerRotationAngleDoubleSpinBox, "SpecialMarkersMgr.fovRectangularMarkerRotationAngle");
	// The thickness of lines
	connectIntProperty(ui->lineThicknessSpinBox,			"GridLinesMgr.lineThickness");
	connectIntProperty(ui->partThicknessSpinBox,			"GridLinesMgr.partThickness");
	connectBoolProperty(ui->equatorPartsCheckBox,			"GridLinesMgr.equatorPartsDisplayed");
	connectBoolProperty(ui->equatorJ2000PartsCheckBox,		"GridLinesMgr.equatorJ2000PartsDisplayed");
	connectBoolProperty(ui->eclipticPartsCheckBox,			"GridLinesMgr.eclipticPartsDisplayed");
	connectBoolProperty(ui->eclipticJ2000PartsCheckBox,		"GridLinesMgr.eclipticJ2000PartsDisplayed");
	connectBoolProperty(ui->solarEquatorPartsCheckBox,		"GridLinesMgr.solarEquatorPartsDisplayed");
	connectBoolProperty(ui->longitudePartsCheckBox,			"GridLinesMgr.longitudePartsDisplayed");
	connectBoolProperty(ui->horizonPartsCheckBox,			"GridLinesMgr.horizonPartsDisplayed");
	connectBoolProperty(ui->meridianPartsCheckBox,			"GridLinesMgr.meridianPartsDisplayed");
	connectBoolProperty(ui->primeVerticalPartsCheckBox,		"GridLinesMgr.primeVerticalPartsDisplayed");
	connectBoolProperty(ui->currentVerticalPartsCheckBox,		"GridLinesMgr.currentVerticalPartsDisplayed");
	connectBoolProperty(ui->colurePartsCheckBox,			"GridLinesMgr.colurePartsDisplayed");
	connectBoolProperty(ui->precessionPartsCheckBox,		"GridLinesMgr.precessionPartsDisplayed");
	connectBoolProperty(ui->galacticEquatorPartsCheckBox,		"GridLinesMgr.galacticEquatorPartsDisplayed");
	connectBoolProperty(ui->supergalacticEquatorPartsCheckBox,	"GridLinesMgr.supergalacticEquatorPartsDisplayed");
	connectBoolProperty(ui->equatorLabelsCheckBox,			"GridLinesMgr.equatorPartsLabeled");
	connectBoolProperty(ui->equatorJ2000LabelsCheckBox,		"GridLinesMgr.equatorJ2000PartsLabeled");
	connectBoolProperty(ui->eclipticLabelsCheckBox,			"GridLinesMgr.eclipticPartsLabeled");
	connectBoolProperty(ui->eclipticJ2000LabelsCheckBox,		"GridLinesMgr.eclipticJ2000PartsLabeled");
	connectBoolProperty(ui->solarEquatorLabelsCheckBox,		"GridLinesMgr.solarEquatorPartsLabeled");
	connectBoolProperty(ui->longitudeLabelsCheckBox,		"GridLinesMgr.longitudePartsLabeled");
	connectBoolProperty(ui->horizonLabelsCheckBox,			"GridLinesMgr.horizonPartsLabeled");
	connectBoolProperty(ui->meridianLabelsCheckBox,			"GridLinesMgr.meridianPartsLabeled");
	connectBoolProperty(ui->primeVerticalLabelsCheckBox,		"GridLinesMgr.primeVerticalPartsLabeled");
	connectBoolProperty(ui->currentVerticalLabelsCheckBox,		"GridLinesMgr.currentVerticalPartsLabeled");
	connectBoolProperty(ui->colureLabelsCheckBox,			"GridLinesMgr.colurePartsLabeled");
	connectBoolProperty(ui->precessionLabelsCheckBox,		"GridLinesMgr.precessionPartsLabeled");
	connectBoolProperty(ui->galacticEquatorLabelsCheckBox,		"GridLinesMgr.galacticEquatorPartsLabeled");
	connectBoolProperty(ui->supergalacticEquatorLabelsCheckBox,	"GridLinesMgr.supergalacticEquatorPartsLabeled");

	connectColorButton(ui->colorEclipticGridJ2000,		"GridLinesMgr.eclipticJ2000GridColor",		"color/ecliptical_J2000_color");
	connectColorButton(ui->colorEclipticGridOfDate,		"GridLinesMgr.eclipticGridColor",		"color/ecliptical_color");
	connectColorButton(ui->colorEquatorialJ2000Grid,	"GridLinesMgr.equatorJ2000GridColor",		"color/equatorial_J2000_color");
	connectColorButton(ui->colorEquatorialGrid,		"GridLinesMgr.equatorGridColor",		"color/equatorial_color");
	connectColorButton(ui->colorGalacticGrid,		"GridLinesMgr.galacticGridColor",		"color/galactic_color");
	connectColorButton(ui->colorSupergalacticGrid,		"GridLinesMgr.supergalacticGridColor",		"color/supergalactic_color");
	connectColorButton(ui->colorAzimuthalGrid,		"GridLinesMgr.azimuthalGridColor",		"color/azimuthal_color");
	connectColorButton(ui->colorEclipticLineJ2000,		"GridLinesMgr.eclipticJ2000LineColor",		"color/ecliptic_J2000_color");
	connectColorButton(ui->colorEclipticLineOfDate,		"GridLinesMgr.eclipticLineColor",		"color/ecliptic_color");
	connectColorButton(ui->colorInvariablePlane,		"GridLinesMgr.invariablePlaneLineColor",	"color/invariable_plane_color");
	connectColorButton(ui->colorSolarEquatorLine,		"GridLinesMgr.solarEquatorLineColor",		"color/solar_equator_color");
	connectColorButton(ui->colorEquatorJ2000Line,		"GridLinesMgr.equatorJ2000LineColor",		"color/equator_J2000_color");
	connectColorButton(ui->colorEquatorLine,		"GridLinesMgr.equatorLineColor",		"color/equator_color");
	connectColorButton(ui->colorGalacticEquatorLine,	"GridLinesMgr.galacticEquatorLineColor",	"color/galactic_equator_color");
	connectColorButton(ui->colorSupergalacticEquatorLine,	"GridLinesMgr.supergalacticEquatorLineColor",	"color/supergalactic_equator_color");
	connectColorButton(ui->colorHorizonLine,		"GridLinesMgr.horizonLineColor",		"color/horizon_color");
	connectColorButton(ui->colorLongitudeLine,		"GridLinesMgr.longitudeLineColor",		"color/oc_longitude_color");
	connectColorButton(ui->colorColuresLine,		"GridLinesMgr.colureLinesColor",		"color/colures_color");
	connectColorButton(ui->colorCircumpolarCircles,		"GridLinesMgr.circumpolarCirclesColor",		"color/circumpolar_circles_color");
	connectColorButton(ui->colorUmbraCircle,		"GridLinesMgr.umbraCircleColor",		"color/umbra_circle_color");
	connectColorButton(ui->colorPenumbraCircle,		"GridLinesMgr.penumbraCircleColor",		"color/penumbra_circle_color");
	connectColorButton(ui->colorPrecessionCircles,		"GridLinesMgr.precessionCirclesColor",		"color/precession_circles_color");
	connectColorButton(ui->colorPrimeVerticalLine,		"GridLinesMgr.primeVerticalLineColor",		"color/prime_vertical_color");
	connectColorButton(ui->colorCurrentVerticalLine,	"GridLinesMgr.currentVerticalLineColor",	"color/current_vertical_color");
	connectColorButton(ui->colorMeridianLine,		"GridLinesMgr.meridianLineColor",		"color/meridian_color");
	connectColorButton(ui->colorCelestialJ2000Poles,	"GridLinesMgr.celestialJ2000PolesColor",	"color/celestial_J2000_poles_color");
	connectColorButton(ui->colorCelestialPoles,		"GridLinesMgr.celestialPolesColor",		"color/celestial_poles_color");
	connectColorButton(ui->colorZenithNadir,		"GridLinesMgr.zenithNadirColor",		"color/zenith_nadir_color");
	connectColorButton(ui->colorEclipticJ2000Poles,		"GridLinesMgr.eclipticJ2000PolesColor",		"color/ecliptic_J2000_poles_color");
	connectColorButton(ui->colorEclipticPoles,		"GridLinesMgr.eclipticPolesColor",		"color/ecliptic_poles_color");
	connectColorButton(ui->colorGalacticPoles,		"GridLinesMgr.galacticPolesColor",		"color/galactic_poles_color");
	connectColorButton(ui->colorGalacticCenter,		"GridLinesMgr.galacticCenterColor",		"color/galactic_center_color");
	connectColorButton(ui->colorSupergalacticPoles,		"GridLinesMgr.supergalacticPolesColor",		"color/supergalactic_poles_color");
	connectColorButton(ui->colorEquinoxJ2000Points,		"GridLinesMgr.equinoxJ2000PointsColor",		"color/equinox_J2000_points_color");
	connectColorButton(ui->colorEquinoxPoints,		"GridLinesMgr.equinoxPointsColor",		"color/equinox_points_color");
	connectColorButton(ui->colorSolsticeJ2000Points,	"GridLinesMgr.solsticeJ2000PointsColor",	"color/solstice_J2000_points_color");
	connectColorButton(ui->colorSolsticePoints,		"GridLinesMgr.solsticePointsColor",		"color/solstice_points_color");
	connectColorButton(ui->colorAntisolarPoint,		"GridLinesMgr.antisolarPointColor",		"color/antisolar_point_color");
	connectColorButton(ui->colorApexPoints,			"GridLinesMgr.apexPointsColor",			"color/apex_points_color");
	connectColorButton(ui->colorFOVCenterMarker,		"SpecialMarkersMgr.fovCenterMarkerColor",	"color/fov_center_marker_color");
	connectColorButton(ui->colorFOVCircularMarker,		"SpecialMarkersMgr.fovCircularMarkerColor",	"color/fov_circular_marker_color");
	connectColorButton(ui->colorFOVRectangularMarker,	"SpecialMarkersMgr.fovRectangularMarkerColor",	"color/fov_rectangular_marker_color");
	connectColorButton(ui->colorCardinalPoints,		"LandscapeMgr.cardinalsPointsColor",		"color/cardinal_color");
	connectColorButton(ui->colorCompassMarks,		"SpecialMarkersMgr.compassMarksColor",		"color/compass_marks_color");

	// Projection
	connect(ui->projectionListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(changeProjection(const QString&)));
	connect(StelApp::getInstance().getCore(), SIGNAL(currentProjectionTypeChanged(StelCore::ProjectionType)),this,SLOT(projectionChanged()));
	connectDoubleProperty(ui->viewportOffsetSpinBox, "StelMovementMgr.viewportVerticalOffsetTarget");

	// Starlore
	connect(ui->useAsDefaultSkyCultureCheckBox, SIGNAL(clicked()), this, SLOT(setCurrentCultureAsDefault()));
	connect(&StelApp::getInstance().getSkyCultureMgr(), SIGNAL(defaultSkyCultureChanged(QString)),this,SLOT(updateDefaultSkyCulture()));
	updateDefaultSkyCulture();

	// allow to display short names and inhibit translation.
	connectIntProperty(ui->skyCultureNamesStyleComboBox,		"ConstellationMgr.constellationDisplayStyle");
	connectCheckBox(ui->nativePlanetNamesCheckBox,			"actionShow_Skyculture_NativePlanetNames");
	connectCheckBox(ui->showConstellationLinesCheckBox,		"actionShow_Constellation_Lines");
	connectIntProperty(ui->constellationLineThicknessSpinBox,	"ConstellationMgr.constellationLineThickness");
	connectCheckBox(ui->showConstellationLabelsCheckBox,		"actionShow_Constellation_Labels");
	connectCheckBox(ui->showConstellationBoundariesCheckBox,	"actionShow_Constellation_Boundaries");
	connectIntProperty(ui->constellationBoundariesThicknessSpinBox,	"ConstellationMgr.constellationBoundariesThickness");
	connectCheckBox(ui->showConstellationArtCheckBox,		"actionShow_Constellation_Art");	
	connectDoubleProperty(ui->constellationArtBrightnessSpinBox,	"ConstellationMgr.artIntensity");

	connectColorButton(ui->colorConstellationBoundaries,		"ConstellationMgr.boundariesColor",	"color/const_boundary_color");
	connectColorButton(ui->colorConstellationLabels,		"ConstellationMgr.namesColor",		"color/const_names_color");
	connectColorButton(ui->colorConstellationLines,			"ConstellationMgr.linesColor",		"color/const_lines_color");

	connectCheckBox(ui->showAsterismLinesCheckBox,		"actionShow_Asterism_Lines");
	connectIntProperty(ui->asterismLineThicknessSpinBox,	"AsterismMgr.asterismLineThickness");
	connectCheckBox(ui->showAsterismLabelsCheckBox,		"actionShow_Asterism_Labels");
	connectCheckBox(ui->showRayHelpersCheckBox,		"actionShow_Ray_Helpers");
	connectIntProperty(ui->rayHelperThicknessSpinBox,	"AsterismMgr.rayHelperThickness");

	connectColorButton(ui->colorAsterismLabels,	"AsterismMgr.namesColor",	"color/asterism_names_color");
	connectColorButton(ui->colorAsterismLines,	"AsterismMgr.linesColor",	"color/asterism_lines_color");
	connectColorButton(ui->colorRayHelpers,	        "AsterismMgr.rayHelpersColor",	"color/rayhelper_lines_color");

	// Font selection
	connectIntProperty(ui->constellationsFontSizeSpinBox,	"ConstellationMgr.fontSize");
	connectIntProperty(ui->asterismsFontSizeSpinBox,		"AsterismMgr.fontSize");

	// Hips mgr.
	populateHipsGroups();
	StelModule *hipsmgr = StelApp::getInstance().getModule("HipsMgr");	
	connect(hipsmgr, SIGNAL(surveysChanged()), this, SLOT(updateHips()));
	connect(ui->surveyTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateHips()));
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(updateHips()));
	connect(ui->surveysListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(updateHips()), Qt::QueuedConnection);
	connect(ui->surveysListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(hipsListItemChanged(QListWidgetItem*)));
	updateHips();

	updateTabBarListWidgetWidth();
}

void ViewDialog::populateOrbitsControls(bool flag)
{
	ui->planetIsolatedOrbitCheckBox->setEnabled(flag);
	ui->planetOrbitOnlyCheckBox->setEnabled(flag);
	ui->planetOrbitPermanentCheckBox->setEnabled(flag);
	ui->planetOrbitsThicknessSpinBox->setEnabled(flag);
	ui->pushButtonOrbitColors->setEnabled(flag);
}

void ViewDialog::populateTrailsControls(bool flag)
{
	ui->planetIsolatedTrailsCheckBox->setEnabled(flag);
	ui->planetIsolatedTrailsSpinBox->setEnabled(flag);
	ui->planetTrailsThicknessSpinBox->setEnabled(flag);
}

// Heuristic function to decide in which group to put a survey.
static QString getHipsType(const HipsSurveyP hips)
{
	QJsonObject properties = hips->property("properties").toJsonObject();
	if (!hips->isPlanetarySurvey())
		return "dss";
	if (properties["type"].toString() == "planet") // TODO: switch to use hips->isPlanetarySurvey() and multiple surveys for Solar system bodies
		return "sol";
	return "other";
}

void ViewDialog::updateHips()
{
	if (!ui->page_surveys->isVisible()) return;
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	StelModule *hipsmgr = StelApp::getInstance().getModule("HipsMgr");
	QMetaObject::invokeMethod(hipsmgr, "loadSources");

	QComboBox* typeComboBox = ui->surveyTypeComboBox;
	QVariant selectedType = typeComboBox->itemData(typeComboBox->currentIndex());
	if (selectedType.isNull())
		selectedType = "dss";

	// Update survey list.
	QListWidget* l = ui->surveysListWidget;

	if (!hipsmgr->property("loaded").toBool())
	{
		l->clear();
		new QListWidgetItem(q_("Loading ..."), l);
		return;
	}

	QJsonObject currentInfo;
	QString currentSurvey = l->currentItem() ? l->currentItem()->data(Qt::UserRole).toString() : "";
	QListWidgetItem* currentItem = Q_NULLPTR;
	HipsSurveyP currentHips;

	l->blockSignals(true);
	l->clear();
	QList<HipsSurveyP> hipslist = hipsmgr->property("surveys").value<QList<HipsSurveyP>>();

	for (auto hips: hipslist)
	{
		if (getHipsType(hips) != selectedType)
			continue;
		QString url = hips->property("url").toString();
		QJsonObject properties = hips->property("properties").toJsonObject();
		QString title = properties["obs_title"].toString();
		if (title.isEmpty())
			continue;
		QListWidgetItem* item = new QListWidgetItem(title, l);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(hips->property("visible").toBool() ? Qt::Checked : Qt::Unchecked);
		item->setData(Qt::UserRole, url);
		if (url == currentSurvey)
		{
			currentItem = item;
			currentHips = hips;
		}
		disconnect(hips.data(), Q_NULLPTR, this, Q_NULLPTR);
		connect(hips.data(), SIGNAL(statusChanged()), this, SLOT(updateHips()));
	}
	l->sortItems(Qt::AscendingOrder);
	l->setCurrentItem(currentItem);
	l->scrollToItem(currentItem);
	l->blockSignals(false);

	if (!currentHips)
	{
		ui->surveysTextBrowser->setText("");
	}
	else
	{
		QJsonObject props = currentHips->property("properties").toJsonObject();
		QString html = QString("<h1>%1</h1>\n").arg(props["obs_title"].toString());
		if (props.contains("obs_copyright") && props.contains("obs_copyright_url"))
		{
			html += QString("<p>Copyright <a href='%2'>%1</a></p>\n")
					.arg(props["obs_copyright"].toString()).arg(props["obs_copyright_url"].toString());
		}
		html += QString("<p>%1</p>\n").arg(props["obs_description"].toString());
		html += "<h2>" + q_("properties") + "</h2>\n<ul>\n";
		for (auto iter = props.constBegin(); iter != props.constEnd(); iter++)
		{
			html += QString("<li><b>%1</b> %2</li>\n").arg(iter.key()).arg(iter.value().toString());
		}
		html += "</ul>\n";
		if (gui)
			ui->surveysTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
		ui->surveysTextBrowser->setHtml(html);
	}
}

void ViewDialog::populateHipsGroups()
{
	// Update the groups combobox.
	QComboBox* typeComboBox = ui->surveyTypeComboBox;
	int index = typeComboBox->currentIndex();
	QVariant selectedType = typeComboBox->itemData(index);
	if (selectedType.isNull())
		selectedType = "dss";
	typeComboBox->clear();
	typeComboBox->addItem(q_("Deep Sky"), "dss");
	typeComboBox->addItem(q_("Solar System"), "sol");
	index = typeComboBox->findData(selectedType, Qt::UserRole, Qt::MatchCaseSensitive);
	typeComboBox->setCurrentIndex(index);
}

void ViewDialog::hipsListItemChanged(QListWidgetItem* item)
{
	QListWidget* l = item->listWidget();
	l->blockSignals(true);
	StelModule *hipsmgr = StelApp::getInstance().getModule("HipsMgr");
	QString url = item->data(Qt::UserRole).toString();
	HipsSurveyP hips;
	QMetaObject::invokeMethod(hipsmgr, "getSurveyByUrl", Qt::DirectConnection,
			Q_RETURN_ARG(HipsSurveyP, hips), Q_ARG(QString, url));
	Q_ASSERT(hips);
	if (item->checkState() == Qt::Checked)
	{
		l->setCurrentItem(item);
		hips->setProperty("visible", true);
	}
	else
	{
		hips->setProperty("visible", false);
	}
	l->blockSignals(false);
}

void ViewDialog::updateTabBarListWidgetWidth()
{
	ui->stackListWidget->setWrapping(false);
	// Update list item sizes after translation
	ui->stackListWidget->adjustSize();
	QAbstractItemModel* model = ui->stackListWidget->model();
	if (!model)
	{
		return;
	}
	// stackListWidget->font() does not work properly!
	// It has a incorrect fontSize in the first loading, which produces the bug#995107.
	QFont font;
	font.setPixelSize(14);
	font.setWeight(75);
	QFontMetrics fontMetrics(font);
	int iconSize = ui->stackListWidget->iconSize().width();
	int width = 0;
	for (int row = 0; row < model->rowCount(); row++)
	{
		int textWidth = fontMetrics.boundingRect(ui->stackListWidget->item(row)->text()).width();
		width += iconSize > textWidth ? iconSize : textWidth; // use the wider one
		width += 24; // margin - 12px left and 12px right
	}
	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
}

void ViewDialog::setSelectedCatalogsFromCheckBoxes()
{
	Nebula::CatalogGroup flags(Q_NULLPTR);
	if (ui->checkBoxNGC->isChecked())
		flags |= Nebula::CatNGC;
	if (ui->checkBoxIC->isChecked())
		flags |= Nebula::CatIC;
	if (ui->checkBoxM->isChecked())
		flags |= Nebula::CatM;
	if (ui->checkBoxC->isChecked())
		flags |= Nebula::CatC;
	if (ui->checkBoxB->isChecked())
		flags |= Nebula::CatB;
	if (ui->checkBoxSh2->isChecked())
		flags |= Nebula::CatSh2;
	if (ui->checkBoxVdB->isChecked())
		flags |= Nebula::CatVdB;
	if (ui->checkBoxRCW->isChecked())
		flags |= Nebula::CatRCW;
	if (ui->checkBoxLBN->isChecked())
		flags |= Nebula::CatLBN;
	if (ui->checkBoxLDN->isChecked())
		flags |= Nebula::CatLDN;
	if (ui->checkBoxCr->isChecked())
		flags |= Nebula::CatCr;
	if (ui->checkBoxMel->isChecked())
		flags |= Nebula::CatMel;
	if (ui->checkBoxCed->isChecked())
		flags |= Nebula::CatCed;
	if (ui->checkBoxPGC->isChecked())
		flags |= Nebula::CatPGC;
	if (ui->checkBoxUGC->isChecked())
		flags |= Nebula::CatUGC;
	if (ui->checkBoxArp->isChecked())
		flags |= Nebula::CatArp;
	if (ui->checkBoxVV->isChecked())
		flags |= Nebula::CatVV;
	if (ui->checkBoxPK->isChecked())
		flags |= Nebula::CatPK;
	if (ui->checkBoxPNG->isChecked())
		flags |= Nebula::CatPNG;
	if (ui->checkBoxSNRG->isChecked())
		flags |= Nebula::CatSNRG;
	if (ui->checkBoxACO->isChecked())
		flags |= Nebula::CatACO;
	if (ui->checkBoxHCG->isChecked())
		flags |= Nebula::CatHCG;	
	if (ui->checkBoxESO->isChecked())
		flags |= Nebula::CatESO;
	if (ui->checkBoxVdBH->isChecked())
		flags |= Nebula::CatVdBH;
	if (ui->checkBoxDWB->isChecked())
		flags |= Nebula::CatDWB;
	if (ui->checkBoxTr->isChecked())
		flags |= Nebula::CatTr;
	if (ui->checkBoxSt->isChecked())
		flags |= Nebula::CatSt;
	if (ui->checkBoxRu->isChecked())
		flags |= Nebula::CatRu;
	if (ui->checkBoxVdBHa->isChecked())
		flags |= Nebula::CatVdBHa;
	if (ui->checkBoxOther->isChecked())
		flags |= Nebula::CatOther;

	GETSTELMODULE(NebulaMgr)->setCatalogFilters(flags);
}

void ViewDialog::setSelectedTypesFromCheckBoxes()
{
	Nebula::TypeGroup flags(Q_NULLPTR);
	if (ui->checkBoxGalaxiesType->isChecked())
		flags |= Nebula::TypeGalaxies;
	if (ui->checkBoxActiveGalaxiesType->isChecked())
		flags |= Nebula::TypeActiveGalaxies;
	if (ui->checkBoxInteractingGalaxiesType->isChecked())
		flags |= Nebula::TypeInteractingGalaxies;
	if (ui->checkBoxOpenStarClustersType->isChecked())
		flags |= Nebula::TypeOpenStarClusters;
	if (ui->checkBoxGlobularStarClustersType->isChecked())
		flags |= Nebula::TypeGlobularStarClusters;
	if (ui->checkBoxBrightNebulaeType->isChecked())
		flags |= Nebula::TypeBrightNebulae;
	if (ui->checkBoxDarkNebulaeType->isChecked())
		flags |= Nebula::TypeDarkNebulae;
	if (ui->checkBoxPlanetaryNebulaeType->isChecked())
		flags |= Nebula::TypePlanetaryNebulae;
	if (ui->checkBoxHydrogenRegionsType->isChecked())
		flags |= Nebula::TypeHydrogenRegions;
	if (ui->checkBoxSupernovaRemnantsType->isChecked())
		flags |= Nebula::TypeSupernovaRemnants;
	if (ui->checkBoxGalaxyClustersType->isChecked())
		flags |= Nebula::TypeGalaxyClusters;
	if (ui->checkBoxOtherType->isChecked())
		flags |= Nebula::TypeOther;

	GETSTELMODULE(NebulaMgr)->setTypeFilters(flags);
}


void ViewDialog::updateSelectedCatalogsCheckBoxes()
{
	const Nebula::CatalogGroup& flags = GETSTELMODULE(NebulaMgr)->getCatalogFilters();
	ui->checkBoxNGC->setChecked(flags & Nebula::CatNGC);
	ui->checkBoxIC->setChecked(flags & Nebula::CatIC);
	ui->checkBoxM->setChecked(flags & Nebula::CatM);
	ui->checkBoxC->setChecked(flags & Nebula::CatC);
	ui->checkBoxB->setChecked(flags & Nebula::CatB);
	ui->checkBoxSh2->setChecked(flags & Nebula::CatSh2);
	ui->checkBoxVdB->setChecked(flags & Nebula::CatVdB);
	ui->checkBoxRCW->setChecked(flags & Nebula::CatRCW);
	ui->checkBoxLDN->setChecked(flags & Nebula::CatLDN);
	ui->checkBoxLBN->setChecked(flags & Nebula::CatLBN);
	ui->checkBoxCr->setChecked(flags & Nebula::CatCr);
	ui->checkBoxMel->setChecked(flags & Nebula::CatMel);
	ui->checkBoxCed->setChecked(flags & Nebula::CatCed);
	ui->checkBoxPGC->setChecked(flags & Nebula::CatPGC);
	ui->checkBoxUGC->setChecked(flags & Nebula::CatUGC);
	ui->checkBoxArp->setChecked(flags & Nebula::CatArp);
	ui->checkBoxVV->setChecked(flags & Nebula::CatVV);
	ui->checkBoxPK->setChecked(flags & Nebula::CatPK);
	ui->checkBoxPNG->setChecked(flags & Nebula::CatPNG);
	ui->checkBoxSNRG->setChecked(flags & Nebula::CatSNRG);
	ui->checkBoxACO->setChecked(flags & Nebula::CatACO);
	ui->checkBoxHCG->setChecked(flags & Nebula::CatHCG);	
	ui->checkBoxESO->setChecked(flags & Nebula::CatESO);
	ui->checkBoxVdBH->setChecked(flags & Nebula::CatVdBH);
	ui->checkBoxDWB->setChecked(flags & Nebula::CatDWB);
	ui->checkBoxTr->setChecked(flags & Nebula::CatTr);
	ui->checkBoxSt->setChecked(flags & Nebula::CatSt);
	ui->checkBoxRu->setChecked(flags & Nebula::CatRu);
	ui->checkBoxVdBHa->setChecked(flags & Nebula::CatVdBHa);
	ui->checkBoxOther->setChecked(flags & Nebula::CatOther);	
}

void ViewDialog::updateSelectedTypesCheckBoxes()
{
	const Nebula::TypeGroup& flags = GETSTELMODULE(NebulaMgr)->getTypeFilters();
	ui->checkBoxGalaxiesType->setChecked(flags & Nebula::TypeGalaxies);
	ui->checkBoxActiveGalaxiesType->setChecked(flags & Nebula::TypeActiveGalaxies);
	ui->checkBoxInteractingGalaxiesType->setChecked(flags & Nebula::TypeInteractingGalaxies);
	ui->checkBoxOpenStarClustersType->setChecked(flags & Nebula::TypeOpenStarClusters);
	ui->checkBoxGlobularStarClustersType->setChecked(flags & Nebula::TypeGlobularStarClusters);
	ui->checkBoxBrightNebulaeType->setChecked(flags & Nebula::TypeBrightNebulae);
	ui->checkBoxDarkNebulaeType->setChecked(flags & Nebula::TypeDarkNebulae);
	ui->checkBoxPlanetaryNebulaeType->setChecked(flags & Nebula::TypePlanetaryNebulae);
	ui->checkBoxHydrogenRegionsType->setChecked(flags & Nebula::TypeHydrogenRegions);
	ui->checkBoxSupernovaRemnantsType->setChecked(flags & Nebula::TypeSupernovaRemnants);
	ui->checkBoxGalaxyClustersType->setChecked(flags & Nebula::TypeGalaxyClusters);
	ui->checkBoxOtherType->setChecked(flags & Nebula::TypeOther);
}

// 20160411. New function introduced with trunk merge. Not sure yet if useful or bad with property connections?.
void ViewDialog::populateLightPollution()
{
	StelCore *core = StelApp::getInstance().getCore();
	StelModule *lmgr = StelApp::getInstance().getModule("LandscapeMgr");
	int bIdx = core->getSkyDrawer()->getBortleScaleIndex();
	if (lmgr->property("flagUseLightPollutionFromDatabase").toBool())
	{
		StelLocation loc = core->getCurrentLocation();
		bIdx = loc.bortleScaleIndex;
		if (!loc.planetName.contains("Earth")) // location not on Earth...
			bIdx = 1;
		if (bIdx<1) // ...or it observatory, or it unknown location
			bIdx = loc.DEFAULT_BORTLE_SCALE_INDEX;
		ui->lightPollutionSpinBox->setEnabled(false);
	}
	else
		ui->lightPollutionSpinBox->setEnabled(true);

	ui->lightPollutionSpinBox->setValue(bIdx);
	setBortleScaleToolTip(bIdx);
}

void ViewDialog::setBortleScaleToolTip(int Bindex)
{
	int i = Bindex-1;
	QStringList list, nelm;
	//TRANSLATORS: Short description for Class 1 of the Bortle scale
	list.append(q_("Excellent dark-sky site"));
	//TRANSLATORS: Short description for Class 2 of the Bortle scale
	list.append(q_("Typical truly dark site"));
	//TRANSLATORS: Short description for Class 3 of the Bortle scale
	list.append(q_("Rural sky"));
	//TRANSLATORS: Short description for Class 4 of the Bortle scale
	list.append(q_("Rural/suburban transition"));
	//TRANSLATORS: Short description for Class 5 of the Bortle scale
	list.append(q_("Suburban sky"));
	//TRANSLATORS: Short description for Class 6 of the Bortle scale
	list.append(q_("Bright suburban sky"));
	//TRANSLATORS: Short description for Class 7 of the Bortle scale
	list.append(q_("Suburban/urban transition"));
	//TRANSLATORS: Short description for Class 8 of the Bortle scale
	list.append(q_("City sky"));
	//TRANSLATORS: Short description for Class 9 of the Bortle scale
	list.append(q_("Inner-city sky"));

	nelm.append("7.6-8.0");
	nelm.append("7.1-7.5");
	nelm.append("6.6-7.0");
	nelm.append("6.1-6.5");
	nelm.append("5.6-6.0");
	nelm.append("5.1-5.5");
	nelm.append("4.6-5.0");
	nelm.append("4.1-4.5");
	nelm.append("4.0");

	QString tooltip = QString("%1 (%2 %3)")
			.arg(list.at(i))
			.arg(q_("The naked-eye limiting magnitude is"))
			.arg(nelm.at(i));

	ui->lightPollutionSpinBox->setToolTip(tooltip);
}

void ViewDialog::populateToolTips()
{
	ui->planetUseObjModelsCheckBox->setToolTip(QString("<p>%1</p>").arg(q_("Uses a polygonal 3D model for some selected subplanetary objects (small moons, asteroids, comets) instead of a spherical approximation")));
	ui->planetShowObjSelfShadowsCheckBox->setToolTip(QString("<p>%1</p>").arg(q_("Use a &quot;shadow map&quot; to simulate self-shadows of non-convex solar system objects. May reduce shadow penumbra quality on some objects.")));

	QString degree = QChar(0x00B0);
	ui->fovCircularMarkerSizeDoubleSpinBox->setSuffix(degree);
	ui->fovRectangularMarkerHeightDoubleSpinBox->setSuffix(degree);
	ui->fovRectangularMarkerWidthDoubleSpinBox->setSuffix(degree);
	ui->fovRectangularMarkerRotationAngleDoubleSpinBox->setSuffix(degree);
}

void ViewDialog::populateLists()
{
	// Fill the culture list widget from the available list
	StelApp& app = StelApp::getInstance();
	QListWidget* l = ui->culturesListWidget;
	l->blockSignals(true);
	l->clear();
	l->addItems(app.getSkyCultureMgr().getSkyCultureListI18());
	l->setCurrentItem(l->findItems(app.getSkyCultureMgr().getCurrentSkyCultureNameI18(), Qt::MatchExactly).at(0));
	l->blockSignals(false);
	updateSkyCultureText();

	// populate language printing combo. (taken from DeltaT combo)
	StelModule* cmgr = app.getModule("ConstellationMgr");
	Q_ASSERT(cmgr);
	Q_ASSERT(ui->skyCultureNamesStyleComboBox);
	QComboBox* cultureNamesStyleComboBox = ui->skyCultureNamesStyleComboBox;

	cultureNamesStyleComboBox->blockSignals(true);
	cultureNamesStyleComboBox->clear();
	QMetaEnum enumerator = cmgr->metaObject()->property(cmgr->metaObject()->indexOfProperty("constellationDisplayStyle")).enumerator();
	cultureNamesStyleComboBox->addItem(q_("Abbreviated"), enumerator.keyToValue("constellationsAbbreviated"));
	cultureNamesStyleComboBox->addItem(q_("Native"), enumerator.keyToValue("constellationsNative"));  // Please make this always a transcript into European letters!
	cultureNamesStyleComboBox->addItem(q_("Translated"), enumerator.keyToValue("constellationsTranslated"));
	//cultureNamesStyleComboBox->addItem(q_("English"),    ConstellationMgr::constellationsEnglish); // This is not useful.
	//Restore the selection
	int index = cultureNamesStyleComboBox->findData(cmgr->property("constellationDisplayStyle").toInt(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (index==-1) index=2; // Default: Translated
	cultureNamesStyleComboBox->setCurrentIndex(index);
	cultureNamesStyleComboBox->blockSignals(false);

	const StelCore* core = app.getCore();
	StelGui* gui = dynamic_cast<StelGui*>(app.getGui());

	// Fill the projection list
	l = ui->projectionListWidget;
	l->blockSignals(true);
	l->clear();	
	const QStringList mappings = core->getAllProjectionTypeKeys();
	for (const auto& s : mappings)
	{
		l->addItem(core->projectionTypeKeyToNameI18n(s));
	}
	l->setCurrentItem(l->findItems(core->getCurrentProjectionNameI18n(), Qt::MatchExactly).at(0));
	l->blockSignals(false);
	if (gui)
		ui->projectionTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->projectionTextBrowser->setHtml(core->getProjection(StelCore::FrameJ2000)->getHtmlSummary());	

	// Fill the landscape list
	l = ui->landscapesListWidget;
	l->blockSignals(true);
	l->clear();
	StelModule* lmgr = app.getModule("LandscapeMgr");
	QStringList landscapeList = lmgr->property("allLandscapeNames").toStringList();
	for (const auto& landscapeName : qAsConst(landscapeList))
	{
		QString label = q_(landscapeName);
		QListWidgetItem* item = new QListWidgetItem(label);
		item->setData(Qt::UserRole, landscapeName);
		l->addItem(item);
	}
	l->sortItems(); // they may have been translated!
	QString selectedLandscapeName = lmgr->property("currentLandscapeName").toString();
	for (int i = 0; i < l->count(); i++)
	{
		if (l->item(i)->data(Qt::UserRole).toString() == selectedLandscapeName)
		{
			l->setCurrentRow(i);
			break;
		}
	}
	l->blockSignals(false);	
	QStringList searchPaths;
	searchPaths << StelFileMgr::findFile("landscapes/" + lmgr->property("currentLandscapeID").toString());

	ui->landscapeTextBrowser->setSearchPaths(searchPaths);
	if (gui)
		ui->landscapeTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));	
	ui->landscapeTextBrowser->setHtml(lmgr->property("currentLandscapeHtmlDescription").toString());	
	updateDefaultLandscape();
}

void ViewDialog::skyCultureChanged()
{
	QListWidget* l = ui->culturesListWidget;
	l->setCurrentItem(l->findItems(StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureNameI18(), Qt::MatchExactly).at(0));
	updateSkyCultureText();
	updateDefaultSkyCulture();
}

// fill the description text window, not the names in the sky.
void ViewDialog::updateSkyCultureText()
{
	StelApp& app = StelApp::getInstance();
	QString skyCultureId = app.getSkyCultureMgr().getCurrentSkyCultureID();
	QString html = app.getSkyCultureMgr().getCurrentSkyCultureHtmlDescription();

	QStringList searchPaths;
	searchPaths << StelFileMgr::findFile("skycultures/" + skyCultureId);

	ui->skyCultureTextBrowser->setSearchPaths(searchPaths);
	StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
	if (gui)
		ui->skyCultureTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->skyCultureTextBrowser->setHtml(html);	
}

void ViewDialog::changeProjection(const QString& projectionNameI18n)
{
	StelCore* core = StelApp::getInstance().getCore();
	core->setCurrentProjectionTypeKey(core->projectionNameI18nToTypeKey(projectionNameI18n));
}

void ViewDialog::projectionChanged()
{
	StelCore* core = StelApp::getInstance().getCore();
	QListWidget* l = ui->projectionListWidget;
	l->setCurrentItem(l->findItems(core->getCurrentProjectionNameI18n(), Qt::MatchExactly).at(0),QItemSelectionModel::SelectCurrent);
	ui->projectionTextBrowser->setHtml(core->getProjection(StelCore::FrameJ2000)->getHtmlSummary());
}


void ViewDialog::changeLandscape(QListWidgetItem* item)
{
	StelApp::getInstance().getModule("LandscapeMgr")->setProperty("currentLandscapeName", item->data(Qt::UserRole).toString());
}

void ViewDialog::landscapeChanged(QString id, QString name)
{
	StelModule* lmgr = StelApp::getInstance().getModule("LandscapeMgr");
	for (int i = 0; i < ui->landscapesListWidget->count(); i++)
	{
		if (ui->landscapesListWidget->item(i)->data(Qt::UserRole).toString() == name)
		{
			ui->landscapesListWidget->setCurrentRow(i, QItemSelectionModel::SelectCurrent);
			break;
		}
	}

	QStringList searchPaths;
	searchPaths << StelFileMgr::findFile("landscapes/" + id);

	ui->landscapeTextBrowser->setSearchPaths(searchPaths);
	ui->landscapeTextBrowser->setHtml(lmgr->property("currentLandscapeHtmlDescription").toString());
	updateDefaultLandscape();
	// Reset values that might have changed.
	ui->showFogCheckBox->setChecked(lmgr->property("fogDisplayed").toBool());
}

void ViewDialog::showAddRemoveLandscapesDialog()
{
	if(addRemoveLandscapesDialog == Q_NULLPTR)
		addRemoveLandscapesDialog = new AddRemoveLandscapesDialog();

	addRemoveLandscapesDialog->setVisible(true);
}

void ViewDialog::showAtmosphereDialog()
{
	if(atmosphereDialog == Q_NULLPTR)
		atmosphereDialog = new AtmosphereDialog();

	atmosphereDialog->setVisible(true);
}

void ViewDialog::showGreatRedSpotDialog()
{
	if(greatRedSpotDialog == Q_NULLPTR)
		greatRedSpotDialog = new GreatRedSpotDialog();

	greatRedSpotDialog->setVisible(true);
}

void ViewDialog::showConfigureDSOColorsDialog()
{
	if(configureDSOColorsDialog == Q_NULLPTR)
		configureDSOColorsDialog = new ConfigureDSOColorsDialog();

	configureDSOColorsDialog->setVisible(true);
}

void ViewDialog::showConfigureOrbitColorsDialog()
{
	if(configureOrbitColorsDialog == Q_NULLPTR)
		configureOrbitColorsDialog = new ConfigureOrbitColorsDialog();

	configureOrbitColorsDialog->setVisible(true);
}

void ViewDialog::updateZhrDescription(int zhr)
{
	// GZ changed to small regions instead of hard "case" to better see a manual setting.
	if (zhr==0)
		ui->zhrLabel->setText("<small><i>"+q_("No shooting stars")+"</i></small>");
	else if (zhr<=10)
		ui->zhrLabel->setText("<small><i>"+q_("Normal rate")+"</i></small>");
	else if ((zhr>=20) && (zhr<=30)) // was 25
		ui->zhrLabel->setText("<small><i>"+q_("Standard Orionids rate")+"</i></small>");
	else if ((zhr>=90) && (zhr<=110)) // was 100
		ui->zhrLabel->setText("<small><i>"+q_("Standard Perseids rate")+"</i></small>");
	else if ((zhr>=111) && (zhr<=132)) // was 120
		ui->zhrLabel->setText("<small><i>"+q_("Standard Geminids rate")+"</i></small>");
	else if ((zhr>=180) && (zhr<=220)) // was 200
		ui->zhrLabel->setText("<small><i>"+q_("Exceptional Perseid rate")+"</i></small>");
	else if ((zhr>=900) && (zhr<=1100)) // was 1000
		ui->zhrLabel->setText("<small><i>"+q_("Meteor storm rate")+"</i></small>");
	else if ((zhr>=5400) && (zhr<=6600)) // was 6000
		ui->zhrLabel->setText("<small><i>"+q_("Exceptional Draconid rate")+"</i></small>");
	else if ((zhr>=9000) && (zhr<=11000)) // was 10000
		ui->zhrLabel->setText("<small><i>"+q_("Exceptional Leonid rate")+"</i></small>");
	else if ((zhr>=130000) && (zhr<=160000)) // was 144000
		ui->zhrLabel->setText("<small><i>"+q_("Very high rate (1966 Leonids)")+"</i></small>");
	else if (zhr>=230000) // was 240000
		ui->zhrLabel->setText("<small><i>"+q_("Highest rate ever (1833 Leonids)")+"</i></small>");
	else
		ui->zhrLabel->setText("");
}

void ViewDialog::setCurrentLandscapeAsDefault(void)
{
	StelModule* lmgr = StelApp::getInstance().getModule("LandscapeMgr");
	Q_ASSERT(lmgr);
	lmgr->setProperty("defaultLandscapeID", lmgr->property("currentLandscapeID"));
}

void ViewDialog::setCurrentCultureAsDefault(void)
{
	StelApp::getInstance().getSkyCultureMgr().setDefaultSkyCultureID(StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID());
}

void ViewDialog::updateDefaultSkyCulture()
{
	// Check that the useAsDefaultSkyCultureCheckBox needs to be updated
	bool b = StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID()==StelApp::getInstance().getSkyCultureMgr().getDefaultSkyCultureID();
	ui->useAsDefaultSkyCultureCheckBox->setChecked(b);
	ui->useAsDefaultSkyCultureCheckBox->setEnabled(!b);
	// Check that ray helpers and asterism lines are defined
	b = GETSTELMODULE(AsterismMgr)->isLinesDefined();
	ui->showAsterismLinesCheckBox->setEnabled(b);
	ui->showAsterismLabelsCheckBox->setEnabled(b);
	ui->asterismLineThicknessSpinBox->setEnabled(b);
	ui->colorAsterismLines->setEnabled(b);
	ui->colorAsterismLabels->setEnabled(b);
	ui->showRayHelpersCheckBox->setEnabled(b);
	ui->rayHelperThicknessSpinBox->setEnabled(b);
	ui->colorRayHelpers->setEnabled(b);
	ui->asterismsFontSizeSpinBox->setEnabled(b);
	ui->labelAsterismsFontSize->setEnabled(b);
}

void ViewDialog::updateDefaultLandscape()
{
	StelModule* lmgr = StelApp::getInstance().getModule("LandscapeMgr");
	Q_ASSERT(lmgr);
	bool isDefault = lmgr->property("currentLandscapeID") == lmgr->property("defaultLandscapeID");
	ui->useAsDefaultLandscapeCheckBox->setChecked(isDefault);
	ui->useAsDefaultLandscapeCheckBox->setEnabled(!isDefault);
}

void ViewDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
		current = previous;
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}

void ViewDialog::populatePlanetMagnitudeAlgorithmsList()
{
	Q_ASSERT(ui->planetMagnitudeAlgorithmComboBox);
	QComboBox* algorithms = ui->planetMagnitudeAlgorithmComboBox;
	//Save the current selection to be restored later
	algorithms->blockSignals(true);
	int index = algorithms->currentIndex();
	QVariant selectedAlgorithmId = algorithms->itemData(index);
	algorithms->clear();
	//For each algorithm, display the localized name and store the key as user data.
	algorithms->addItem(qc_("G. Müller (1893)",              "magnitude algorithm"), Planet::Mueller_1893);
	algorithms->addItem(qc_("Astronomical Almanac (1984)",   "magnitude algorithm"), Planet::AstronomicalAlmanac_1984);
	algorithms->addItem(qc_("Explanatory Supplement (1992)", "magnitude algorithm"), Planet::ExplanatorySupplement_1992);
	algorithms->addItem(qc_("Explanatory Supplement (2013)", "magnitude algorithm"), Planet::ExplanatorySupplement_2013);
	algorithms->addItem(qc_("Mallama & Hilton (2018)",       "magnitude algorithm"), Planet::MallamaHilton_2018);
	algorithms->addItem(qc_("Generic",                       "magnitude algorithm"), Planet::Generic);
	//Restore the selection
	index = algorithms->findData(selectedAlgorithmId, Qt::UserRole, Qt::MatchCaseSensitive);
	algorithms->setCurrentIndex(index);
	//algorithms->model()->sort(0);
	algorithms->blockSignals(false);
}

void ViewDialog::setPlanetMagnitudeAlgorithm(int algorithmID)
{
	Planet::ApparentMagnitudeAlgorithm currentAlgorithm = static_cast<Planet::ApparentMagnitudeAlgorithm>(ui->planetMagnitudeAlgorithmComboBox->itemData(algorithmID).toInt());
	Planet::setApparentMagnitudeAlgorithm(currentAlgorithm);
	populatePlanetMagnitudeAlgorithmDescription();
}

void ViewDialog::populatePlanetMagnitudeAlgorithmDescription()
{
	int currentAlgorithm = ui->planetMagnitudeAlgorithmComboBox->findData(Planet::getApparentMagnitudeAlgorithm(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (currentAlgorithm==-1)
	{
		// Use Mallama&Hilton 2018 as default
		currentAlgorithm = ui->planetMagnitudeAlgorithmComboBox->findData(Planet::MallamaHilton_2018, Qt::UserRole, Qt::MatchCaseSensitive);
	}
	QString info = "";
	switch (currentAlgorithm) {
		case Planet::AstronomicalAlmanac_1984:
			info = q_("The algorithm was used in the <em>Astronomical Almanac</em> (1984 and later) and gives V (instrumental) magnitudes (allegedly from D.L. Harris).");
			break;
		case Planet::Mueller_1893:
			info = q_("The algorithm is based on visual observations 1877-1891 by G. Müller and was still republished in the <em>Explanatory Supplement to the Astronomical Ephemeris</em> (1961).");
			break;
		case Planet::ExplanatorySupplement_1992:
			info = q_("The algorithm was published in the 2nd edition of the <em>Explanatory Supplement to the Astronomical Almanac</em> (1992).");
			break;
		case Planet::ExplanatorySupplement_2013:
			info = q_("The algorithm was published in the 3rd edition of the <em>Explanatory Supplement to the Astronomical Almanac</em> (2013).");
			break;
		case Planet::MallamaHilton_2018:
			info = q_("The algorithm was published by A. Mallama & J. L. Hilton: <em>Computing apparent planetary magnitudes for the Astronomical Almanac.</em> Astronomy&Computing 25 (2018) 10-24.");
			break;
		default:
			info = q_("Visual magnitude based on phase angle and albedo.");
			break;
	}
	ui->planetMagnitudeAlgorithmDescription->setText(QString("<small>%1</small>").arg(info));
}
