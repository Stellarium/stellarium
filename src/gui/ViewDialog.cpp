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
#include <QColorDialog>
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

	//ui->viewTabWidget->removeTab(4);

#ifdef Q_OS_WIN
	//Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->projectionListWidget << ui->culturesListWidget << ui->skyCultureTextBrowser << ui->landscapesListWidget;
	StelDialog::installKineticScrolling(addscroll);
#endif

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
	connectCheckBox(ui->starLabelCheckBox, "actionShow_Stars_Labels");
	connectDoubleProperty(ui->starsLabelsHorizontalSlider,"StarMgr.labelsAmount",0.0,10.0);
	connectBoolProperty(ui->checkBoxAdditionalNamesStars, "StarMgr.flagAdditionalNamesDisplayed");

	// Sky section
	connectBoolProperty(ui->milkyWayCheckBox, "MilkyWay.flagMilkyWayDisplayed");
	connectDoubleProperty(ui->milkyWayBrightnessDoubleSpinBox, "MilkyWay.intensity");
	connectBoolProperty(ui->zodiacalLightCheckBox, "ZodiacalLight.flagZodiacalLightDisplayed");
	connectDoubleProperty(ui->zodiacalLightBrightnessDoubleSpinBox, "ZodiacalLight.intensity");
	connectBoolProperty(ui->adaptationCheckbox, "StelSkyDrawer.flagLuminanceAdaptation");

	// Light pollution
	StelModule* lmgr = StelApp::getInstance().getModule("LandscapeMgr");
	Q_ASSERT(lmgr);
	StelSkyDrawer* drawer = StelApp::getInstance().getCore()->getSkyDrawer();
	Q_ASSERT(drawer);
	// TODO: In trunk, populateLightPollution has been added, while socis15 has setLightPollutionSpinBoxStatus.
	// TODO: Decide which is better?
	// The trunk version also sets value of the spinbox, this seems more complete.
	//setLightPollutionSpinBoxStatus();
	populateLightPollution();
	connectBoolProperty(ui->useLightPollutionFromLocationDataCheckBox, "LandscapeMgr.flagUseLightPollutionFromDatabase");

	//connect(lmgr, SIGNAL(lightPollutionUsageChanged(bool)), this, SLOT(setLightPollutionSpinBoxStatus()));
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
	ui->planetIsolatedOrbitCheckBox->setEnabled(ssmgr->getFlagOrbits());
	connect(ssmgr,SIGNAL(flagOrbitsChanged(bool)),ui->planetIsolatedOrbitCheckBox, SLOT(setEnabled(bool)));
	connectBoolProperty(ui->planetOrbitOnlyCheckBox, "SolarSystem.flagPlanetsOrbitsOnly");
	ui->planetOrbitOnlyCheckBox->setEnabled(ssmgr->getFlagPlanetsOrbitsOnly());
	connect(ssmgr,SIGNAL(flagOrbitsChanged(bool)),ui->planetOrbitOnlyCheckBox, SLOT(setEnabled(bool)));
	connectBoolProperty(ui->planetIsolatedTrailsCheckBox, "SolarSystem.flagIsolatedTrails");
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
	connectCheckBox(ui->planetLabelCheckBox, "actionShow_Planets_Labels");
	connectCheckBox(ui->planetNomenclatureCheckBox, "actionShow_Planets_Nomenclature");
	connectDoubleProperty(ui->planetsLabelsHorizontalSlider, "SolarSystem.labelsAmount",0.0,10.0);
	connect(ui->pushButtonOrbitColors, SIGNAL(clicked(bool)), this, SLOT(showConfigureOrbitColorsDialog()));
	connectCheckBox(ui->planetNomenclatureCheckBox, "actionShow_Planets_Nomenclature");
	colorButton(ui->planetNomenclatureColor, "NomenclatureMgr.nomenclatureColor");
	connect(ui->planetNomenclatureColor, SIGNAL(released()), this, SLOT(askPlanetNomenclatureColor()));
	colorButton(ui->planetLabelColor, "SolarSystem.labelsColor");
	connect(ui->planetLabelColor, SIGNAL(released()), this, SLOT(askPlanetLabelsColor()));
	colorButton(ui->planetTrailsColor, "SolarSystem.trailsColor");
	connect(ui->planetTrailsColor, SIGNAL(released()), this, SLOT(askPlanetTrailsColor()));
	connectBoolProperty(ui->planetTrailsCheckBox, "SolarSystem.trailsDisplayed");
	ui->planetIsolatedTrailsCheckBox->setEnabled(ssmgr->getFlagIsolatedTrails());
	connect(ssmgr,SIGNAL(flagIsolatedTrailsChanged(bool)),ui->planetIsolatedTrailsCheckBox, SLOT(setEnabled(bool)));
	connectBoolProperty(ui->hidePlanetNomenclatureCheckBox, "NomenclatureMgr.localNomenclatureHided");

	StelModule* mnmgr = StelApp::getInstance().getModule("NomenclatureMgr");
	ui->hidePlanetNomenclatureCheckBox->setEnabled(mnmgr->property("nomenclatureDisplayed").toBool());
	connect(mnmgr,SIGNAL(nomenclatureDisplayedChanged(bool)),ui->hidePlanetNomenclatureCheckBox, SLOT(setEnabled(bool)));

	populatePlanetMagnitudeAlgorithmsList();
	int idx = ui->planetMagnitudeAlgorithmComboBox->findData(Planet::getApparentMagnitudeAlgorithm(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use ExplanSupl2013 as default
		idx = ui->planetMagnitudeAlgorithmComboBox->findData(Planet::ExplanatorySupplement_2013, Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->planetMagnitudeAlgorithmComboBox->setCurrentIndex(idx);
	connect(ui->planetMagnitudeAlgorithmComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setPlanetMagnitudeAlgorithm(int)));
	populatePlanetMagnitudeAlgorithmDescription();

	// GreatRedSpot (Jupiter)
	// TODO: put under Properties system!
	bool grsFlag = ssmgr->getFlagCustomGrsSettings();
	ui->customGrsSettingsCheckBox->setChecked(grsFlag);
	connect(ui->customGrsSettingsCheckBox, SIGNAL(toggled(bool)), this, SLOT(setFlagCustomGrsSettings(bool)));
	ui->pushButtonGrsDetails->setEnabled(grsFlag);
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

	connect(ui->landscapesListWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(changeLandscape(QListWidgetItem*)));
	connect(lmgr, SIGNAL(currentLandscapeChanged(QString,QString)), this, SLOT(landscapeChanged(QString,QString)));

	connect(ui->useAsDefaultLandscapeCheckBox, SIGNAL(clicked()), this, SLOT(setCurrentLandscapeAsDefault()));
	connect(lmgr,SIGNAL(defaultLandscapeChanged(QString)),this,SLOT(updateDefaultLandscape()));
	updateDefaultLandscape();

	connect(lmgr, SIGNAL(landscapesChanged()), this, SLOT(populateLists()));
	connect(ui->pushButtonAddRemoveLandscapes, SIGNAL(clicked()), this, SLOT(showAddRemoveLandscapesDialog()));

	// Grid and lines
	connectGroupBox(ui->celestialSphereGroupBox,				"actionShow_Gridlines");
	connectCheckBox(ui->showEquatorLineCheckBox,			"actionShow_Equator_Line");
	connectCheckBox(ui->showEquatorJ2000LineCheckBox,		"actionShow_Equator_J2000_Line");
	connectCheckBox(ui->showEclipticLineJ2000CheckBox,		"actionShow_Ecliptic_J2000_Line");
	connectCheckBox(ui->showEclipticLineOfDateCheckBox,		"actionShow_Ecliptic_Line");
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
	connectCheckBox(ui->showPrecessionCirclesCheckBox,		"actionShow_Precession_Circles");
	connectCheckBox(ui->showPrimeVerticalLineCheckBox,		"actionShow_Prime_Vertical_Line");
	connectCheckBox(ui->showColuresLineCheckBox,			"actionShow_Colure_Lines");
	connectCheckBox(ui->showCircumpolarCirclesCheckBox,		"actionShow_Circumpolar_Circles");
	connectCheckBox(ui->showCelestialJ2000PolesCheckBox,		"actionShow_Celestial_J2000_Poles");
	connectCheckBox(ui->showCelestialPolesCheckBox,			"actionShow_Celestial_Poles");
	connectCheckBox(ui->showZenithNadirCheckBox,			"actionShow_Zenith_Nadir");
	connectCheckBox(ui->showEclipticJ2000PolesCheckBox,		"actionShow_Ecliptic_J2000_Poles");
	connectCheckBox(ui->showEclipticPolesCheckBox,			"actionShow_Ecliptic_Poles");
	connectCheckBox(ui->showGalacticPolesCheckBox,			"actionShow_Galactic_Poles");
	connectCheckBox(ui->showSupergalacticPolesCheckBox,		"actionShow_Supergalactic_Poles");
	connectCheckBox(ui->showEquinoxJ2000PointsCheckBox,		"actionShow_Equinox_J2000_Points");
	connectCheckBox(ui->showEquinoxPointsCheckBox,			"actionShow_Equinox_Points");
	connectCheckBox(ui->showSolsticeJ2000PointsCheckBox,		"actionShow_Solstice_J2000_Points");
	connectCheckBox(ui->showSolsticePointsCheckBox,			"actionShow_Solstice_Points");

	colorButton(ui->colorEclipticGridJ2000,		"GridLinesMgr.eclipticJ2000GridColor");
	colorButton(ui->colorEclipticGridOfDate,		"GridLinesMgr.eclipticGridColor");
	colorButton(ui->colorEquatorialJ2000Grid,		"GridLinesMgr.equatorJ2000GridColor");
	colorButton(ui->colorEquatorialGrid,			"GridLinesMgr.equatorGridColor");
	colorButton(ui->colorGalacticGrid,			"GridLinesMgr.galacticGridColor");
	colorButton(ui->colorSupergalacticGrid,		"GridLinesMgr.supergalacticGridColor");
	colorButton(ui->colorAzimuthalGrid,			"GridLinesMgr.azimuthalGridColor");
	colorButton(ui->colorEclipticLineJ2000,		"GridLinesMgr.eclipticJ2000LineColor");
	colorButton(ui->colorEclipticLineOfDate,		"GridLinesMgr.eclipticLineColor");
	colorButton(ui->colorEquatorJ2000Line,		"GridLinesMgr.equatorJ2000LineColor");
	colorButton(ui->colorEquatorLine,			"GridLinesMgr.equatorLineColor");
	colorButton(ui->colorGalacticEquatorLine,		"GridLinesMgr.galacticEquatorLineColor");
	colorButton(ui->colorSupergalacticEquatorLine,	"GridLinesMgr.supergalacticEquatorLineColor");
	colorButton(ui->colorHorizonLine,			"GridLinesMgr.horizonLineColor");
	colorButton(ui->colorLongitudeLine,			"GridLinesMgr.longitudeLineColor");
	colorButton(ui->colorColuresLine,			"GridLinesMgr.colureLinesColor");
	colorButton(ui->colorCircumpolarCircles,		"GridLinesMgr.circumpolarCirclesColor");
	colorButton(ui->colorPrecessionCircles,		"GridLinesMgr.precessionCirclesColor");
	colorButton(ui->colorPrimeVerticalLine,		"GridLinesMgr.primeVerticalLineColor");
	colorButton(ui->colorMeridianLine,			"GridLinesMgr.meridianLineColor");
	colorButton(ui->colorCelestialJ2000Poles,		"GridLinesMgr.celestialJ2000PolesColor");
	colorButton(ui->colorCelestialPoles,			"GridLinesMgr.celestialPolesColor");
	colorButton(ui->colorZenithNadir,			"GridLinesMgr.zenithNadirColor");
	colorButton(ui->colorEclipticJ2000Poles,		"GridLinesMgr.eclipticJ2000PolesColor");
	colorButton(ui->colorEclipticPoles,			"GridLinesMgr.eclipticPolesColor");
	colorButton(ui->colorGalacticPoles,			"GridLinesMgr.galacticPolesColor");
	colorButton(ui->colorSupergalacticPoles,		"GridLinesMgr.supergalacticPolesColor");
	colorButton(ui->colorEquinoxJ2000Points,		"GridLinesMgr.equinoxJ2000PointsColor");
	colorButton(ui->colorEquinoxPoints,			"GridLinesMgr.equinoxPointsColor");
	colorButton(ui->colorSolsticeJ2000Points,		"GridLinesMgr.solsticeJ2000PointsColor");
	colorButton(ui->colorSolsticePoints,			"GridLinesMgr.solsticePointsColor");
	colorButton(ui->colorCardinalPoints,			"LandscapeMgr.cardinalsPointsColor");

	connect(ui->colorEclipticGridJ2000,			SIGNAL(released()), this, SLOT(askEclipticJ2000GridColor()));
	connect(ui->colorEclipticGridOfDate,			SIGNAL(released()), this, SLOT(askEclipticGridColor()));
	connect(ui->colorEquatorialJ2000Grid,			SIGNAL(released()), this, SLOT(askEquatorJ2000GridColor()));
	connect(ui->colorEquatorialGrid,				SIGNAL(released()), this, SLOT(askEquatorGridColor()));
	connect(ui->colorGalacticGrid,				SIGNAL(released()), this, SLOT(askGalacticGridColor()));
	connect(ui->colorSupergalacticGrid,			SIGNAL(released()), this, SLOT(askSupergalacticGridColor()));
	connect(ui->colorAzimuthalGrid,				SIGNAL(released()), this, SLOT(askAzimuthalGridColor()));
	connect(ui->colorEclipticLineJ2000,			SIGNAL(released()), this, SLOT(askEclipticLineJ2000Color()));
	connect(ui->colorEclipticLineOfDate,			SIGNAL(released()), this, SLOT(askEclipticLineColor()));
	connect(ui->colorEquatorJ2000Line,			SIGNAL(released()), this, SLOT(askEquatorLineJ2000Color()));
	connect(ui->colorEquatorLine,				SIGNAL(released()), this, SLOT(askEquatorLineColor()));
	connect(ui->colorGalacticEquatorLine,			SIGNAL(released()), this, SLOT(askGalacticEquatorLineColor()));
	connect(ui->colorSupergalacticEquatorLine,		SIGNAL(released()), this, SLOT(askSupergalacticEquatorLineColor()));
	connect(ui->colorHorizonLine,				SIGNAL(released()), this, SLOT(askHorizonLineColor()));
	connect(ui->colorLongitudeLine,				SIGNAL(released()), this, SLOT(askLongitudeLineColor()));
	connect(ui->colorColuresLine,				SIGNAL(released()), this, SLOT(askColureLinesColor()));
	connect(ui->colorCircumpolarCircles,			SIGNAL(released()), this, SLOT(askCircumpolarCirclesColor()));
	connect(ui->colorPrecessionCircles,			SIGNAL(released()), this, SLOT(askPrecessionCirclesColor()));
	connect(ui->colorPrimeVerticalLine,			SIGNAL(released()), this, SLOT(askPrimeVerticalLineColor()));
	connect(ui->colorMeridianLine,				SIGNAL(released()), this, SLOT(askMeridianLineColor()));
	connect(ui->colorCelestialJ2000Poles,			SIGNAL(released()), this, SLOT(askCelestialJ2000PolesColor()));
	connect(ui->colorCelestialPoles,				SIGNAL(released()), this, SLOT(askCelestialPolesColor()));
	connect(ui->colorZenithNadir,				SIGNAL(released()), this, SLOT(askZenithNadirColor()));
	connect(ui->colorEclipticJ2000Poles,			SIGNAL(released()), this, SLOT(askEclipticJ2000PolesColor()));
	connect(ui->colorEclipticPoles,				SIGNAL(released()), this, SLOT(askEclipticPolesColor()));
	connect(ui->colorGalacticPoles,				SIGNAL(released()), this, SLOT(askGalacticPolesColor()));
	connect(ui->colorSupergalacticPoles,			SIGNAL(released()), this, SLOT(askSupergalacticPolesColor()));
	connect(ui->colorEquinoxJ2000Points,			SIGNAL(released()), this, SLOT(askEquinoxJ2000PointsColor()));
	connect(ui->colorEquinoxPoints,				SIGNAL(released()), this, SLOT(askEquinoxPointsColor()));
	connect(ui->colorSolsticeJ2000Points,			SIGNAL(released()), this, SLOT(askSolsticeJ2000PointsColor()));
	connect(ui->colorSolsticePoints,				SIGNAL(released()), this, SLOT(askSolsticePointsColor()));
	connect(ui->colorCardinalPoints,				SIGNAL(released()), this, SLOT(askCardinalPointsColor()));

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
	connectIntProperty(ui->constellationLineThicknessSpinBox,		"ConstellationMgr.constellationLineThickness");
	connectCheckBox(ui->showConstellationLabelsCheckBox,		"actionShow_Constellation_Labels");
	connectCheckBox(ui->showConstellationBoundariesCheckBox,	"actionShow_Constellation_Boundaries");
	connectCheckBox(ui->showConstellationArtCheckBox,			"actionShow_Constellation_Art");
	connectDoubleProperty(ui->constellationArtBrightnessSpinBox,	"ConstellationMgr.artIntensity");

	colorButton(ui->colorConstellationBoundaries,	"ConstellationMgr.boundariesColor");
	colorButton(ui->colorConstellationLabels,		"ConstellationMgr.namesColor");
	colorButton(ui->colorConstellationLines,		"ConstellationMgr.linesColor");
	connect(ui->colorConstellationBoundaries,		SIGNAL(released()), this, SLOT(askConstellationBoundariesColor()));
	connect(ui->colorConstellationLabels,			SIGNAL(released()), this, SLOT(askConstellationLabelsColor()));
	connect(ui->colorConstellationLines,			SIGNAL(released()), this, SLOT(askConstellationLinesColor()));

	connectCheckBox(ui->showAsterismLinesCheckBox,		"actionShow_Asterism_Lines");
	connectIntProperty(ui->asterismLineThicknessSpinBox,	"AsterismMgr.asterismLineThickness");
	connectCheckBox(ui->showAsterismLabelsCheckBox,		"actionShow_Asterism_Labels");

	connectCheckBox(ui->showRayHelpersCheckBox,		"actionShow_Ray_Helpers");
	connectIntProperty(ui->rayHelperThicknessSpinBox,		"AsterismMgr.rayHelperThickness");

	colorButton(ui->colorAsterismLabels,	"AsterismMgr.namesColor");
	colorButton(ui->colorAsterismLines,	"AsterismMgr.linesColor");
	colorButton(ui->colorRayHelpers,	"AsterismMgr.rayHelpersColor");
	connect(ui->colorAsterismLabels,	SIGNAL(released()), this, SLOT(askAsterismLabelsColor()));
	connect(ui->colorAsterismLines,		SIGNAL(released()), this, SLOT(askAsterismLinesColor()));
	connect(ui->colorRayHelpers,		SIGNAL(released()), this, SLOT(askRayHelpersColor()));

	// Sky layers. This not yet finished and not visible in releases.
	// TODO: These 4 lines are commented away in trunk.
	populateSkyLayersList();
	connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(populateSkyLayersList()));
	connect(ui->skyLayerListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(skyLayersSelectionChanged(const QString&)));
	connect(ui->skyLayerEnableCheckBox, SIGNAL(stateChanged(int)), this, SLOT(skyLayersEnabledChanged(int)));

	// Hips mgr.
	StelModule *hipsmgr = StelApp::getInstance().getModule("HipsMgr");
	connect(hipsmgr, SIGNAL(surveysChanged()), this, SLOT(updateHips()));
	connect(ui->surveysListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(updateHips()), Qt::QueuedConnection);
	connect(ui->surveysListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(hipsListItemChanged(QListWidgetItem*)));
	updateHips();

	updateTabBarListWidgetWidth();
}

// Heuristic function to decide in which group to put a survey.
static QString getHipsType(const HipsSurveyP hips)
{
	QJsonObject properties = hips->property("properties").toJsonObject();
	if (!properties.contains("type") && !properties["client_category"].toString().contains("solar system", Qt::CaseInsensitive))
		return "dss";
	if (properties["type"].toString() == "planet") // || properties["client_category"].toString().contains("solar system", Qt::CaseInsensitive))
		return "sol";
	return "other";
}

void ViewDialog::updateHips()
{

	// Update the groups combobox.
	QComboBox* typeComboBox = ui->surveyTypeComboBox;
	disconnect(typeComboBox, 0, 0, 0);
	int index = typeComboBox->currentIndex();
	QVariant selectedType = typeComboBox->itemData(index);
	if (selectedType.isNull()) selectedType = "dss";
	typeComboBox->clear();
	typeComboBox->addItem(q_("Deep Sky"), "dss");
	typeComboBox->addItem(q_("Solar System"), "sol");
	index = typeComboBox->findData(selectedType, Qt::UserRole, Qt::MatchCaseSensitive);
	typeComboBox->setCurrentIndex(index);
	connect(typeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateHips()));

	// Update survey list.
	StelModule *hipsmgr = StelApp::getInstance().getModule("HipsMgr");
	QListWidget* l = ui->surveysListWidget;
	QJsonObject currentInfo;
	QString currentSurvey = l->currentItem() ? l->currentItem()->data(Qt::UserRole).toString() : "";
	QListWidgetItem* currentItem = NULL;
	HipsSurveyP currentHips;

	l->blockSignals(true);
	l->clear();
	QList<HipsSurveyP> hipslist = hipsmgr->property("surveys").value<QList<HipsSurveyP>>();

	for (auto hips: hipslist)
	{
		if (getHipsType(hips) != selectedType) continue;
		QString url = hips->property("url").toString();
		QJsonObject properties = hips->property("properties").toJsonObject();
		QString title = properties["obs_title"].toString();
		if (title.isEmpty()) continue;
		QListWidgetItem* item = new QListWidgetItem(title, l);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(hips->property("visible").toBool() ? Qt::Checked : Qt::Unchecked);
		item->setData(Qt::UserRole, url);
		if (url == currentSurvey)
		{
			currentItem = item;
			currentHips = hips;
		}
		disconnect(hips.data(), 0, this, 0);
		connect(hips.data(), SIGNAL(statusChanged()), this, SLOT(updateHips()));
	}
	l->setCurrentItem(currentItem);
	l->blockSignals(false);

	if (!currentHips)
	{
		ui->surveysTextBrowser->setText("");
	}
	else
	{
		QJsonObject props = currentHips->property("properties").toJsonObject();
		QString html = QString("<h1>%1</h1>\n").arg(props["obs_title"].toString());

		html += QString("<p>%1</p>\n").arg(props["obs_description"].toString());

		html += "<h2>properties</h2>\n<ul>\n";
		for (auto iter = props.constBegin(); iter != props.constEnd(); iter++)
		{
			html += QString("<li><b>%1</b> %2</li>\n").arg(iter.key()).arg(iter.value().toString());
		}
		html += "</ul>\n";
		ui->surveysTextBrowser->setHtml(html);
	}

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

void ViewDialog::colorButton(QToolButton* toolButton, QString propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Vec3f vColor = prop->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	// Use style sheet for create a nice buttons :)		
	toolButton->setStyleSheet("QToolButton { background-color:" + color.name() + "; }");
	toolButton->setFixedSize(QSize(18, 18));
}

void ViewDialog::askEclipticJ2000GridColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.eclipticJ2000GridColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEclipticGridJ2000->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("eclipticJ2000GridColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/ecliptical_J2000_color", StelUtils::vec3fToStr(vColor));
		ui->colorEclipticGridJ2000->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEclipticGridColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.eclipticGridColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEclipticGridOfDate->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("eclipticGridColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/ecliptical_color", StelUtils::vec3fToStr(vColor));
		ui->colorEclipticGridOfDate->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEquatorJ2000GridColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.equatorJ2000GridColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEquatorialJ2000Grid->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("equatorJ2000GridColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/equatorial_J2000_color", StelUtils::vec3fToStr(vColor));
		ui->colorEquatorialJ2000Grid->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEquatorGridColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.equatorGridColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEquatorialGrid->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("equatorGridColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/equatorial_color", StelUtils::vec3fToStr(vColor));
		ui->colorEquatorialGrid->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askGalacticGridColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.galacticGridColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGalacticGrid->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("galacticGridColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/galactic_color", StelUtils::vec3fToStr(vColor));
		ui->colorGalacticGrid->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askSupergalacticGridColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.supergalacticGridColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorSupergalacticGrid->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("supergalacticGridColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/supergalactic_color", StelUtils::vec3fToStr(vColor));
		ui->colorSupergalacticGrid->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askAzimuthalGridColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.azimuthalGridColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorAzimuthalGrid->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("azimuthalGridColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/azimuthal_color", StelUtils::vec3fToStr(vColor));
		ui->colorAzimuthalGrid->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEclipticLineJ2000Color()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.eclipticJ2000LineColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEclipticLineJ2000->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());		
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("eclipticJ2000LineColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/ecliptic_J2000_color", StelUtils::vec3fToStr(vColor));
		ui->colorEclipticLineJ2000->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEclipticLineColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.eclipticLineColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEclipticLineOfDate->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("eclipticLineColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/ecliptic_color", StelUtils::vec3fToStr(vColor));
		ui->colorEclipticLineOfDate->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEquatorLineJ2000Color()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.equatorJ2000LineColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEquatorJ2000Line->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("equatorJ2000LineColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/equator_J2000_color", StelUtils::vec3fToStr(vColor));
		ui->colorEquatorJ2000Line->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEquatorLineColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.equatorLineColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEquatorLine->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("equatorLineColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/equator_color", StelUtils::vec3fToStr(vColor));
		ui->colorEquatorLine->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askGalacticEquatorLineColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.galacticEquatorLineColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGalacticEquatorLine->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("galacticEquatorLineColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/galactic_equator_color", StelUtils::vec3fToStr(vColor));
		ui->colorGalacticEquatorLine->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askSupergalacticEquatorLineColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.supergalacticEquatorLineColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorSupergalacticEquatorLine->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("supergalacticEquatorLineColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/supergalactic_equator_color", StelUtils::vec3fToStr(vColor));
		ui->colorSupergalacticEquatorLine->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askHorizonLineColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.horizonLineColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorHorizonLine->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("horizonLineColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/horizon_color", StelUtils::vec3fToStr(vColor));
		ui->colorHorizonLine->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askLongitudeLineColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.longitudeLineColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorLongitudeLine->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("longitudeLineColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/oc_longitude_color", StelUtils::vec3fToStr(vColor));
		ui->colorLongitudeLine->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askColureLinesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.colureLinesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorColuresLine->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("colureLinesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/colures_color", StelUtils::vec3fToStr(vColor));
		ui->colorColuresLine->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askCircumpolarCirclesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.circumpolarCirclesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorCircumpolarCircles->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("circumpolarCirclesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/circumpolar_circles_color", StelUtils::vec3fToStr(vColor));
		ui->colorCircumpolarCircles->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askPrecessionCirclesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.precessionCirclesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorPrecessionCircles->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("precessionCirclesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/precession_circles_color", StelUtils::vec3fToStr(vColor));
		ui->colorPrecessionCircles->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askPrimeVerticalLineColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.primeVerticalLineColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorPrimeVerticalLine->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("primeVerticalLineColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/prime_vertical_color", StelUtils::vec3fToStr(vColor));
		ui->colorPrimeVerticalLine->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askMeridianLineColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.meridianLineColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorMeridianLine->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("meridianLineColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/meridian_color", StelUtils::vec3fToStr(vColor));
		ui->colorMeridianLine->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askCelestialJ2000PolesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.celestialJ2000PolesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorCelestialJ2000Poles->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("celestialJ2000PolesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/celestial_J2000_poles_color", StelUtils::vec3fToStr(vColor));
		ui->colorCelestialJ2000Poles->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askCelestialPolesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.celestialPolesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorCelestialPoles->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("celestialPolesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/celestial_poles_color", StelUtils::vec3fToStr(vColor));
		ui->colorCelestialPoles->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askZenithNadirColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.zenithNadirColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorZenithNadir->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("zenithNadirColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/zenith_nadir_color", StelUtils::vec3fToStr(vColor));
		ui->colorZenithNadir->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEclipticJ2000PolesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.eclipticJ2000PolesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEclipticJ2000Poles->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("eclipticJ2000PolesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/ecliptic_J2000_poles_color", StelUtils::vec3fToStr(vColor));
		ui->colorEclipticJ2000Poles->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEclipticPolesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.eclipticPolesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEclipticPoles->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("eclipticPolesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/ecliptic_poles_color", StelUtils::vec3fToStr(vColor));
		ui->colorEclipticPoles->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askGalacticPolesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.galacticPolesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorGalacticPoles->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("galacticPolesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/galactic_poles_color", StelUtils::vec3fToStr(vColor));
		ui->colorGalacticPoles->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askSupergalacticPolesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.supergalacticPolesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorSupergalacticPoles->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("supergalacticPolesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/supergalactic_poles_color", StelUtils::vec3fToStr(vColor));
		ui->colorSupergalacticPoles->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEquinoxJ2000PointsColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.equinoxJ2000PointsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEquinoxJ2000Points->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("equinoxJ2000PointsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/equinox_J2000_points_color", StelUtils::vec3fToStr(vColor));
		ui->colorEquinoxJ2000Points->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askEquinoxPointsColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.equinoxPointsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorEquinoxPoints->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("equinoxPointsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/equinox_points_color", StelUtils::vec3fToStr(vColor));
		ui->colorEquinoxPoints->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askSolsticeJ2000PointsColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.solsticeJ2000PointsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorSolsticeJ2000Points->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("solsticeJ2000PointsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/solstice_J2000_points_color", StelUtils::vec3fToStr(vColor));
		ui->colorSolsticeJ2000Points->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askSolsticePointsColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("GridLinesMgr.solsticePointsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorSolsticePoints->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("GridLinesMgr")->setProperty("solsticePointsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/solstice_points_color", StelUtils::vec3fToStr(vColor));
		ui->colorSolsticePoints->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askCardinalPointsColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("LandscapeMgr.cardinalsPointsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorCardinalPoints->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("LandscapeMgr")->setProperty("colorCardinalPoints", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/cardinal_color", StelUtils::vec3fToStr(vColor));
		ui->colorCardinalPoints->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askConstellationBoundariesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("ConstellationMgr.boundariesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorConstellationBoundaries->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("ConstellationMgr")->setProperty("boundariesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/const_boundary_color", StelUtils::vec3fToStr(vColor));
		ui->colorConstellationBoundaries->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askConstellationLabelsColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("ConstellationMgr.namesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorConstellationLabels->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("ConstellationMgr")->setProperty("namesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/const_names_color", StelUtils::vec3fToStr(vColor));
		ui->colorConstellationLabels->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askConstellationLinesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("ConstellationMgr.linesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorConstellationLines->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("ConstellationMgr")->setProperty("linesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/const_lines_color", StelUtils::vec3fToStr(vColor));
		ui->colorConstellationLines->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askAsterismLabelsColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("AsterismMgr.namesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorAsterismLabels->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("AsterismMgr")->setProperty("namesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/asterism_names_color", StelUtils::vec3fToStr(vColor));
		ui->colorAsterismLabels->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askAsterismLinesColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("AsterismMgr.linesColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorAsterismLines->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("AsterismMgr")->setProperty("linesColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/asterism_lines_color", StelUtils::vec3fToStr(vColor));
		ui->colorAsterismLines->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askRayHelpersColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("AsterismMgr.rayHelpersColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->colorRayHelpers->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("AsterismMgr")->setProperty("rayHelpersColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/rayhelper_lines_color", StelUtils::vec3fToStr(vColor));
		ui->colorRayHelpers->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askPlanetNomenclatureColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("NomenclatureMgr.nomenclatureColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->planetNomenclatureColor->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("NomenclatureMgr")->setProperty("nomenclatureColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/planet_nomenclature_color", StelUtils::vec3fToStr(vColor));
		ui->planetNomenclatureColor->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askPlanetLabelsColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.labelsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->planetLabelColor->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("labelsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/planet_names_color", StelUtils::vec3fToStr(vColor));
		ui->planetLabelColor->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ViewDialog::askPlanetTrailsColor()
{
	Vec3f vColor = StelApp::getInstance().getStelPropertyManager()->getProperty("SolarSystem.trailsColor")->getValue().value<Vec3f>();
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->planetTrailsColor->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getModule("SolarSystem")->setProperty("trailsColor", QVariant::fromValue(vColor));
		StelApp::getInstance().getSettings()->setValue("color/object_trails_color", StelUtils::vec3fToStr(vColor));
		ui->planetTrailsColor->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
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
		int textWidth = fontMetrics.width(ui->stackListWidget->item(row)->text());
		width += iconSize > textWidth ? iconSize : textWidth; // use the wider one
		width += 24; // margin - 12px left and 12px right
	}

	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
}

void ViewDialog::setSelectedCatalogsFromCheckBoxes()
{
	Nebula::CatalogGroup flags(0);

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

	GETSTELMODULE(NebulaMgr)->setCatalogFilters(flags);
}

void ViewDialog::setSelectedTypesFromCheckBoxes()
{
	Nebula::TypeGroup flags(0);

	if (ui->checkBoxGalaxiesType->isChecked())
		flags |= Nebula::TypeGalaxies;
	if (ui->checkBoxActiveGalaxiesType->isChecked())
		flags |= Nebula::TypeActiveGalaxies;
	if (ui->checkBoxInteractingGalaxiesType->isChecked())
		flags |= Nebula::TypeInteractingGalaxies;
	if (ui->checkBoxStarClustersType->isChecked())
		flags |= Nebula::TypeStarClusters;
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
}

void ViewDialog::updateSelectedTypesCheckBoxes()
{
	const Nebula::TypeGroup& flags = GETSTELMODULE(NebulaMgr)->getTypeFilters();

	ui->checkBoxGalaxiesType->setChecked(flags & Nebula::TypeGalaxies);
	ui->checkBoxActiveGalaxiesType->setChecked(flags & Nebula::TypeActiveGalaxies);
	ui->checkBoxInteractingGalaxiesType->setChecked(flags & Nebula::TypeInteractingGalaxies);
	ui->checkBoxStarClustersType->setChecked(flags & Nebula::TypeStarClusters);
	ui->checkBoxBrightNebulaeType->setChecked(flags & Nebula::TypeBrightNebulae);
	ui->checkBoxDarkNebulaeType->setChecked(flags & Nebula::TypeDarkNebulae);
	ui->checkBoxPlanetaryNebulaeType->setChecked(flags & Nebula::TypePlanetaryNebulae);
	ui->checkBoxHydrogenRegionsType->setChecked(flags & Nebula::TypeHydrogenRegions);
	ui->checkBoxSupernovaRemnantsType->setChecked(flags & Nebula::TypeSupernovaRemnants);
	ui->checkBoxGalaxyClustersType->setChecked(flags & Nebula::TypeGalaxyClusters);
	ui->checkBoxOtherType->setChecked(flags & Nebula::TypeOther);
}

void ViewDialog::setFlagCustomGrsSettings(bool b)
{
	GETSTELMODULE(SolarSystem)->setFlagCustomGrsSettings(b);
	ui->pushButtonGrsDetails->setEnabled(b);

	if (!b && greatRedSpotDialog!=Q_NULLPTR)
		greatRedSpotDialog->setVisible(false);
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
// The version from socis only enables the spinbox without setting its value. TODO: Decide which is better?
void ViewDialog::setLightPollutionSpinBoxStatus()
{
	StelModule *lmgr = StelApp::getInstance().getModule("LandscapeMgr");
	ui->lightPollutionSpinBox->setEnabled(!lmgr->property("flagUseLightPollutionFromDatabase").toBool());
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

	nelm.append("7.6–8.0");
	nelm.append("7.1–7.5");
	nelm.append("6.6–7.0");
	nelm.append("6.1–6.5");
	nelm.append("5.6–6.0");
	nelm.append("5.1-5.5");
	nelm.append("4.6–5.0");
	nelm.append("4.1–4.5");
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
}

void ViewDialog::populateLists()
{
	// Fill the culture list widget from the available list
	QListWidget* l = ui->culturesListWidget;
	l->blockSignals(true);
	l->clear();
	l->addItems(StelApp::getInstance().getSkyCultureMgr().getSkyCultureListI18());
	l->setCurrentItem(l->findItems(StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureNameI18(), Qt::MatchExactly).at(0));
	l->blockSignals(false);
	updateSkyCultureText();

	// populate language printing combo. (taken from DeltaT combo)
	StelModule* cmgr = StelApp::getInstance().getModule("ConstellationMgr");
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

	const StelCore* core = StelApp::getInstance().getCore();	
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);

	// Fill the projection list
	l = ui->projectionListWidget;
	l->blockSignals(true);
	l->clear();	
	const QStringList mappings = core->getAllProjectionTypeKeys();
	foreach (QString s, mappings)
	{
		l->addItem(core->projectionTypeKeyToNameI18n(s));
	}
	l->setCurrentItem(l->findItems(core->getCurrentProjectionNameI18n(), Qt::MatchExactly).at(0));
	l->blockSignals(false);
	ui->projectionTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->projectionTextBrowser->setHtml(core->getProjection(StelCore::FrameJ2000)->getHtmlSummary());

	// Fill the landscape list
	l = ui->landscapesListWidget;
	l->blockSignals(true);
	l->clear();
	StelModule* lmgr = StelApp::getInstance().getModule("LandscapeMgr");
	QStringList landscapeList = lmgr->property("allLandscapeNames").toStringList();
	foreach (const QString landscapeName, landscapeList)
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
	ui->landscapeTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->landscapeTextBrowser->setHtml(lmgr->property("currentLandscapeHtmlDescription").toString());
	updateDefaultLandscape();
}

void ViewDialog::populateSkyLayersList()
{
	ui->skyLayerListWidget->clear();
	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
	ui->skyLayerListWidget->addItems(skyLayerMgr->getAllKeys());
}

void ViewDialog::skyLayersSelectionChanged(const QString& s)
{
	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
	StelSkyLayerP l = skyLayerMgr->getSkyLayer(s);

	if (l.isNull())
		return;

	QString html = "<html><head></head><body>";
	html += "<h2>" + l->getShortName()+ "</h2>";
	html += "<p>" + l->getLayerDescriptionHtml() + "</p>";
	if (!l->getShortServerCredits().isEmpty())
		html += "<h3>" + q_("Contact") + ": " + l->getShortServerCredits() + "</h3>";
	html += "</body></html>";
	ui->skyLayerTextBrowser->setHtml(html);
	ui->skyLayerEnableCheckBox->setChecked(skyLayerMgr->getShowLayer(s));
}

void ViewDialog::skyLayersEnabledChanged(int state)
{
	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
	skyLayerMgr->showLayer(ui->skyLayerListWidget->currentItem()->text(), state);
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
	Q_ASSERT(gui);
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
	StelModule* lmgr = StelApp::getInstance().getModule("LandscapeMgr");
	lmgr->setProperty("currentLandscapeName", item->data(Qt::UserRole).toString());
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
	else if ((zhr>=108) && (zhr<=132)) // was 120
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
	algorithms->addItem(qc_("G. Mueller (1893)", "magnitude algorithm"), Planet::Mueller_1893);
	algorithms->addItem(qc_("Astronomical Almanach (1984)", "magnitude algorithm"), Planet::AstronomicalAlmanac_1984);
	algorithms->addItem(qc_("Explanatory Supplement (1992)", "magnitude algorithm"), Planet::ExplanatorySupplement_1992);
	algorithms->addItem(qc_("Explanatory Supplement (2013)", "magnitude algorithm"), Planet::ExplanatorySupplement_2013);
	algorithms->addItem(qc_("Generic", "magnitude algorithm"), Planet::Generic);
	//Restore the selection
	index = algorithms->findData(selectedAlgorithmId, Qt::UserRole, Qt::MatchCaseSensitive);
	algorithms->setCurrentIndex(index);
	//algorithms->model()->sort(0);
	algorithms->blockSignals(false);
}

void ViewDialog::setPlanetMagnitudeAlgorithm(int algorithmID)
{
	Planet::ApparentMagnitudeAlgorithm currentAlgorithm = (Planet::ApparentMagnitudeAlgorithm) ui->planetMagnitudeAlgorithmComboBox->itemData(algorithmID).toInt();
	Planet::setApparentMagnitudeAlgorithm(currentAlgorithm);
	populatePlanetMagnitudeAlgorithmDescription();
}

void ViewDialog::populatePlanetMagnitudeAlgorithmDescription()
{
	int currentAlgorithm = ui->planetMagnitudeAlgorithmComboBox->findData(Planet::getApparentMagnitudeAlgorithm(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (currentAlgorithm==-1)
	{
		// Use ExplanSupl2013 as default
		currentAlgorithm = ui->planetMagnitudeAlgorithmComboBox->findData(Planet::ExplanatorySupplement_2013, Qt::UserRole, Qt::MatchCaseSensitive);
	}
	QString info = "";
	switch (currentAlgorithm) {
		case Planet::AstronomicalAlmanac_1984:
			info = q_("The algorithm was used in the <em>Astronomical Almanac</em> (1984 and later) and gives V (instrumental) magnitudes (allegedly from D.L. Harris).");
			break;
		case Planet::Mueller_1893:
			info = q_("The algorithm is based on visual observations 1877-1891 by G. Mueller and was published in <em>Explanatory Supplement to the Astronomical Ephemeris</em> (1961).");
			break;
		case Planet::ExplanatorySupplement_1992:
			info = q_("The algorithm was published in the <em>Explanatory Supplement to the Astronomical Almanac</em> (1992).");
			break;
		case Planet::ExplanatorySupplement_2013:
			info = q_("The algorithm was published in the 3rd edition of the <em>Explanatory Supplement to the Astronomical Almanac</em> (2013).");
			break;
		default:
			info = q_("Visual magnitude based on phase angle and albedo.");
			break;
	}
	ui->planetMagnitudeAlgorithmDescription->setText(QString("<small>%1</small>").arg(info));
}
