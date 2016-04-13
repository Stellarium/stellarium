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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelProjector.hpp"
#include "LandscapeMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StarMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "SolarSystem.hpp"
#include "NebulaMgr.hpp"
#include "MilkyWay.hpp"
#include "ZodiacalLight.hpp"
#include "ConstellationMgr.hpp"
#include "SporadicMeteorMgr.hpp"
#include "StelStyle.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelGuiBase.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelActionMgr.hpp"
#include "StelMovementMgr.hpp"

#include <QDebug>
#include <QFrame>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QDialog>
#include <QStringList>

ViewDialog::ViewDialog(QObject* parent) : StelDialog(parent)
{
	dialogName = "View";
	ui = new Ui_viewDialogForm;
	addRemoveLandscapesDialog = NULL;
	atmosphereDialog=NULL;
	greatRedSpotDialog=NULL;
}

ViewDialog::~ViewDialog()
{
	delete ui;
	ui=NULL;
	delete addRemoveLandscapesDialog;
	addRemoveLandscapesDialog = NULL;
	delete atmosphereDialog;
	atmosphereDialog = NULL;
	delete greatRedSpotDialog;
	greatRedSpotDialog = NULL;
}

void ViewDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateZhrDescription(GETSTELMODULE(SporadicMeteorMgr)->getZHR());
		populateLists();
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

	// Note: GZ had a day of "fun" on 20160411-12 sorting out the merge with the restructured method from trunk.
	// I had to check whether all entries from re-arranged trunk are present in the property-enabled versions here.
	// I hope I have everything working again.
	// TODO: New method: populateLightPollution may be useful. Make sure it is.
	// Jupiter's GRS must become property, and recheck the other "from trunk" entries.

	connectDoubleProperty(ui->viewportOffsetSpinBox, "prop_StelMovementMgr_viewportVerticalOffsetTarget");

	connect(ui->culturesListWidget, SIGNAL(currentTextChanged(const QString&)),&StelApp::getInstance().getSkyCultureMgr(),SLOT(setCurrentSkyCultureNameI18(QString)));
	connect(&StelApp::getInstance(), SIGNAL(skyCultureChanged(QString)), this, SLOT(skyCultureChanged()));
	connect(ui->projectionListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(changeProjection(const QString&)));
	connect(StelApp::getInstance().getCore(), SIGNAL(currentProjectionTypeChanged(StelCore::ProjectionType)),this,SLOT(projectionChanged()));

	// Connect and initialize checkboxes and other widgets

	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);

	// DSO
	updateSelectedCatalogsCheckBoxes();
	connect(nmgr, SIGNAL(catalogFiltersChanged(Nebula::CatalogGroup)), this, SLOT(updateSelectedCatalogsCheckBoxes()));
	connect(ui->buttonGroupDisplayedDSOCatalogs, SIGNAL(buttonClicked(int)), this, SLOT(setSelectedCatalogsFromCheckBoxes()));
	updateSelectedTypesCheckBoxes();
	connect(nmgr, SIGNAL(typeFiltersChanged(Nebula::TypeGroup)), this, SLOT(updateSelectedTypesCheckBoxes()));
	connect(ui->buttonGroupDisplayedDSOTypes, SIGNAL(buttonClicked(int)), this, SLOT(setSelectedTypesFromCheckBoxes()));
	connectGroupBox(ui->groupBoxDSOTypeFilters,"actionSet_Nebula_TypeFilterUsage");
	connectBoolProperty(ui->checkBoxProportionalHints, "prop_NebulaMgr_hintsProportional");
	connectBoolProperty(ui->checkBoxSurfaceBrightnessUsage, "prop_NebulaMgr_flagSurfaceBrightnessUsage");

	// From Trunk, but seems OK here. It was close to end before.
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));

	// Stars section
	connectGroupBox(ui->starGroupBox, "actionShow_Stars"); // NEW FROM TRUNK
	connectBoolProperty(ui->starTwinkleCheckBox, "prop_SkyDrawer_flagTwinkle");
	connectDoubleProperty(ui->starScaleRadiusDoubleSpinBox,"prop_SkyDrawer_absoluteStarScale");
	connectDoubleProperty(ui->starRelativeScaleDoubleSpinBox, "prop_SkyDrawer_relativeStarScale");
	connectDoubleProperty(ui->milkyWayBrightnessDoubleSpinBox, "prop_MilkyWay_intensity");
	connectDoubleProperty(ui->zodiacalLightBrightnessDoubleSpinBox, "prop_ZodiacalLight_intensity");
	connectDoubleProperty(ui->starTwinkleAmountDoubleSpinBox,"prop_SkyDrawer_twinkleAmount");
	connectBoolProperty(ui->adaptationCheckbox,"prop_SkyDrawer_flagLuminanceAdaptation");

	// Limit Magnitudes
	// Stars
	connectBoolProperty(ui->starLimitMagnitudeCheckBox,"prop_SkyDrawer_flagStarMagnitudeLimit");
	connectDoubleProperty(ui->starLimitMagnitudeDoubleSpinBox, "prop_SkyDrawer_customStarMagLimit");
	// Planets
	connectBoolProperty(ui->planetLimitMagnitudeCheckBox,"prop_SkyDrawer_flagPlanetMagnitudeLimit");
	connectDoubleProperty(ui->planetLimitMagnitudeDoubleSpinBox,"prop_SkyDrawer_customPlanetMagLimit");
	// DSO
	connectBoolProperty(ui->nebulaLimitMagnitudeCheckBox,"prop_SkyDrawer_flagNebulaMagnitudeLimit");
	connectDoubleProperty(ui->nebulaLimitMagnitudeDoubleSpinBox,"prop_SkyDrawer_customNebulaMagLimit");

	// Planets section
	//connectCheckBox(ui->showPlanetCheckBox, "actionShow_Planets"); // ui element MAY HAVE BEEN RENAMED IN TRUNK?
	connectGroupBox(ui->planetsGroupBox, "actionShow_Planets"); // THIS ONE FROM TRUNK
	connectCheckBox(ui->planetMarkerCheckBox, "actionShow_Planets_Hints");

	connectBoolProperty(ui->planetScaleMoonCheckBox, "prop_SolarSystem_flagMoonScale");
	connectDoubleProperty(ui->moonScaleFactor,"prop_SolarSystem_moonScale");

	connectCheckBox(ui->planetOrbitCheckBox, "actionShow_Planets_Orbits");
	connectBoolProperty(ui->planetIsolatedOrbitCheckBox, "prop_SolarSystem_flagIsolatedOrbits");
	connectBoolProperty(ui->planetIsolatedTrailsCheckBox, "prop_SolarSystem_flagIsolatedTrails");
	connectBoolProperty(ui->planetLightSpeedCheckBox, "prop_SolarSystem_flagLightTravelTime");

	// NEW SECTION FROM TRUNK: GreatRedSpot (Jupiter)
	// TODO: put under Properties system!
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	Q_ASSERT(ssmgr);
	bool grsFlag = ssmgr->getFlagCustomGrsSettings();
	ui->customGrsSettingsCheckBox->setChecked(grsFlag);
	connect(ui->customGrsSettingsCheckBox, SIGNAL(toggled(bool)), this, SLOT(setFlagCustomGrsSettings(bool)));
	ui->pushButtonGrsDetails->setEnabled(grsFlag);
	connect(ui->pushButtonGrsDetails, SIGNAL(clicked()), this, SLOT(showGreatRedSpotDialog()));

	// Shooting stars section
	SporadicMeteorMgr* mmgr = GETSTELMODULE(SporadicMeteorMgr);
	Q_ASSERT(mmgr);

	int zhr = mmgr->getZHR();
	ui->zhrSlider->setValue(zhr);
	ui->zhrSpinBox->setValue(zhr);
	updateZhrDescription(zhr);
	//connect to the data, instead of the GUI
	connect(mmgr, SIGNAL(zhrChanged(int)), this, SLOT(updateZhrDescription(int)));
	connect(mmgr, SIGNAL(zhrChanged(int)), ui->zhrSlider, SLOT(setValue(int)));
	connect(mmgr, SIGNAL(zhrChanged(int)), ui->zhrSpinBox, SLOT(setValue(int)));
	connect(ui->zhrSlider, SIGNAL(valueChanged(int)), mmgr, SLOT(setZHR(int)));
	connect(ui->zhrSpinBox, SIGNAL(valueChanged(int)), mmgr, SLOT(setZHR(int)));

	// Labels section
	connectCheckBox(ui->starLabelCheckBox, "actionShow_Stars_Labels");
	connectGroupBox(ui->groupBoxLabelsAndMarkers, "actionShow_Nebulas");
	connectCheckBox(ui->planetLabelCheckBox, "actionShow_Planets_Labels");

	connectDoubleProperty(ui->starsLabelsHorizontalSlider,"prop_StarMgr_labelsAmount",0.0,10.0);
	connectDoubleProperty(ui->planetsLabelsHorizontalSlider, "prop_SolarSystem_labelsAmount",0.0,10.0);
	connectDoubleProperty(ui->nebulasLabelsHorizontalSlider, "prop_NebulaMgr_labelsAmount",0.0,10.0);
	connectDoubleProperty(ui->nebulasHintsHorizontalSlider, "prop_NebulaMgr_hintsAmount",0.0,10.0);

	// Landscape section
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	Q_ASSERT(lmgr);
	connectCheckBox(ui->showGroundCheckBox, "actionShow_Ground");
	connectCheckBox(ui->showFogCheckBox, "actionShow_Fog");
	connectGroupBox(ui->atmosphereGroupBox, "actionShow_Atmosphere");
	connectCheckBox(ui->landscapeIlluminationCheckBox, "actionShow_LandscapeIllumination");
	connectCheckBox(ui->landscapeLabelsCheckBox, "actionShow_LandscapeLabels");

	connectBoolProperty(ui->landscapePositionCheckBox, "prop_LandscapeMgr_flagLandscapeSetsLocation");

	connectBoolProperty(ui->landscapeBrightnessCheckBox,"prop_LandscapeMgr_flagLandscapeUseMinimalBrightness");
	connect(lmgr,SIGNAL(flagLandscapeUseMinimalBrightnessChanged(bool)),ui->localLandscapeBrightnessCheckBox,SLOT(setEnabled(bool)));
	connect(lmgr,SIGNAL(flagLandscapeUseMinimalBrightnessChanged(bool)),ui->landscapeBrightnessSpinBox,SLOT(setEnabled(bool)));
	ui->localLandscapeBrightnessCheckBox->setEnabled(lmgr->getFlagLandscapeUseMinimalBrightness());
	ui->landscapeBrightnessSpinBox->setEnabled(lmgr->getFlagLandscapeUseMinimalBrightness());

	connectDoubleProperty(ui->landscapeBrightnessSpinBox,"prop_LandscapeMgr_defaultMinimalBrightness");
	connectBoolProperty(ui->localLandscapeBrightnessCheckBox,"prop_LandscapeMgr_flagLandscapeSetsMinimalBrightness");

	connect(ui->landscapesListWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(changeLandscape(QListWidgetItem*)));
	connect(lmgr, SIGNAL(currentLandscapeChanged(QString,QString)), this, SLOT(landscapeChanged(QString,QString)));

	connect(ui->useAsDefaultLandscapeCheckBox, SIGNAL(clicked()), this, SLOT(setCurrentLandscapeAsDefault()));
	connect(lmgr,SIGNAL(defaultLandscapeChanged(QString)),this,SLOT(updateDefaultLandscape()));
	updateDefaultLandscape();

	connect(lmgr, SIGNAL(landscapesChanged()), this, SLOT(populateLists()));
	connect(ui->pushButtonAddRemoveLandscapes, SIGNAL(clicked()), this, SLOT(showAddRemoveLandscapesDialog()));

	// Light pollution
	StelSkyDrawer* drawer = StelApp::getInstance().getCore()->getSkyDrawer();
	// TODO: In trunk, populateLightPollution has been added, while socis has setLightPollutionSpinBoxStatus.
	// The trunk version also sets value of the spinbox, this seems more complete.
	//setLightPollutionSpinBoxStatus();
	populateLightPollution();
	ui->useLocationDataCheckBox->setChecked(lmgr->getFlagUseLightPollutionFromDatabase());
	connect(ui->useLocationDataCheckBox, SIGNAL(toggled(bool)), lmgr, SLOT(setFlagUseLightPollutionFromDatabase(bool)));
	connect(lmgr, SIGNAL(lightPollutionUsageChanged(bool)), ui->useLocationDataCheckBox, SLOT(setChecked(bool)));
	// TODO: Decide which is better?
	//connect(lmgr, SIGNAL(lightPollutionUsageChanged(bool)), this, SLOT(setLightPollutionSpinBoxStatus()));
	connect(lmgr, SIGNAL(lightPollutionUsageChanged(bool)), this, SLOT(populateLightPollution()));


	ui->lightPollutionSpinBox->setValue(drawer->getBortleScaleIndex());
	connect(ui->lightPollutionSpinBox, SIGNAL(valueChanged(int)), drawer, SLOT(setBortleScaleIndex(int)));
	connect(drawer, SIGNAL(bortleScaleIndexChanged(int)), ui->lightPollutionSpinBox, SLOT(setValue(int)));
	connect(drawer, SIGNAL(bortleScaleIndexChanged(int)), this, SLOT(setBortleScaleToolTip(int)));

	// Note: This has vanished in the merge on 20160411. -- Ah, moved to ConfigurationDialog.
	//connectBoolProperty(ui->autoChangeLandscapesCheckBox,"prop_LandscapeMgr_flagLandscapeAutoSelection");
	
	// atmosphere details
	connect(ui->pushButtonAtmosphereDetails, SIGNAL(clicked()), this, SLOT(showAtmosphereDialog()));

	// Grid and lines
	connectCheckBox(ui->showEquatorLineCheckBox, "actionShow_Equator_Line");
	connectCheckBox(ui->showEquatorJ2000LineCheckBox, "actionShow_Equator_J2000_Line");
	connectCheckBox(ui->showEclipticLineJ2000CheckBox, "actionShow_Ecliptic_J2000_Line");
	connectCheckBox(ui->showEclipticLineOfDateCheckBox, "actionShow_Ecliptic_Line");
	connectCheckBox(ui->showMeridianLineCheckBox, "actionShow_Meridian_Line");
	connectCheckBox(ui->showLongitudeLineCheckBox, "actionShow_Longitude_Line");
	connectCheckBox(ui->showHorizonLineCheckBox, "actionShow_Horizon_Line");
	connectCheckBox(ui->showEquatorialGridCheckBox, "actionShow_Equatorial_Grid");
	connectCheckBox(ui->showGalacticGridCheckBox, "actionShow_Galactic_Grid");
	connectCheckBox(ui->showGalacticEquatorLineCheckBox, "actionShow_Galactic_Equator_Line");
	connectCheckBox(ui->showAzimuthalGridCheckBox, "actionShow_Azimuthal_Grid");
	connectCheckBox(ui->showEquatorialJ2000GridCheckBox, "actionShow_Equatorial_J2000_Grid");
	connectCheckBox(ui->showEclipticGridJ2000CheckBox, "actionShow_Ecliptic_J2000_Grid");
	connectCheckBox(ui->showEclipticGridOfDateCheckBox, "actionShow_Ecliptic_Grid");
	connectCheckBox(ui->showCardinalPointsCheckBox, "actionShow_Cardinal_Points");
	connectCheckBox(ui->showPrecessionCirclesCheckBox, "actionShow_Precession_Circles");
	connectCheckBox(ui->showPrimeVerticalLineCheckBox, "actionShow_Prime_Vertical_Line");
	connectCheckBox(ui->showColuresLineCheckBox, "actionShow_Colure_Lines");

	// Constellations
	ConstellationMgr* cmgr = GETSTELMODULE(ConstellationMgr);
	Q_ASSERT(cmgr);
	connectCheckBox(ui->showConstellationLinesCheckBox, "actionShow_Constellation_Lines");
	connectCheckBox(ui->showConstellationLabelsCheckBox, "actionShow_Constellation_Labels");
	connectCheckBox(ui->showConstellationBoundariesCheckBox, "actionShow_Constellation_Boundaries");
	connectCheckBox(ui->showConstellationArtCheckBox, "actionShow_Constellation_Art");

	connectDoubleProperty(ui->constellationArtBrightnessSpinBox,"prop_ConstellationMgr_artIntensity");
	connectDoubleProperty(ui->constellationLineThicknessSpinBox,"prop_ConstellationMgr_constellationLineThickness");

	// Starlore
	connect(ui->useAsDefaultSkyCultureCheckBox, SIGNAL(clicked()), this, SLOT(setCurrentCultureAsDefault()));
	connect(&StelApp::getInstance().getSkyCultureMgr(), SIGNAL(defaultSkyCultureChanged(QString)),this,SLOT(updateDefaultSkyCulture()));
	updateDefaultSkyCulture();

	connectCheckBox(ui->nativeNameCheckBox,"actionShow_Skyculture_Nativenames");

	// allow to display short names and inhibit translation.
	connectIntProperty(ui->skyCultureNamesStyleComboBox,"prop_ConstellationMgr_constellationDisplayStyle");

	// Sky layers. This not yet finished and not visible in releases.
	// TODO: These 4 lines are commented away in trunk.
	populateSkyLayersList();
	connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(populateSkyLayersList()));
	connect(ui->skyLayerListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(skyLayersSelectionChanged(const QString&)));
	connect(ui->skyLayerEnableCheckBox, SIGNAL(stateChanged(int)), this, SLOT(skyLayersEnabledChanged(int)));

	updateTabBarListWidgetWidth();
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
	ui->checkBoxOtherType->setChecked(flags & Nebula::TypeOther);
}

void ViewDialog::setFlagCustomGrsSettings(bool b)
{
	GETSTELMODULE(SolarSystem)->setFlagCustomGrsSettings(b);
	ui->pushButtonGrsDetails->setEnabled(b);
	if (!b)
		greatRedSpotDialog->setVisible(false);
}


// 20160411. New function introduced with trunk merge. Not sure yet if useful or bad with property connections?.
void ViewDialog::populateLightPollution()
{
	StelCore *core = StelApp::getInstance().getCore();
	LandscapeMgr *lmgr = GETSTELMODULE(LandscapeMgr);
	int bIdx = core->getSkyDrawer()->getBortleScaleIndex();
	if (lmgr->getFlagUseLightPollutionFromDatabase())
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
	LandscapeMgr *lmgr = GETSTELMODULE(LandscapeMgr);
	ui->lightPollutionSpinBox->setEnabled(!lmgr->getFlagUseLightPollutionFromDatabase());
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
	ConstellationMgr* cmgr=GETSTELMODULE(ConstellationMgr);
	Q_ASSERT(cmgr);
	Q_ASSERT(ui->skyCultureNamesStyleComboBox);
	QComboBox* cultureNamesStyleComboBox = ui->skyCultureNamesStyleComboBox;
	cultureNamesStyleComboBox->blockSignals(true);
	cultureNamesStyleComboBox->clear();
	cultureNamesStyleComboBox->addItem(q_("Abbreviated"),  ConstellationMgr::constellationsAbbreviated);
	cultureNamesStyleComboBox->addItem(q_("Native"),       ConstellationMgr::constellationsNative);  // Please make this always a transcript into European letters!
	cultureNamesStyleComboBox->addItem(q_("Translated"),   ConstellationMgr::constellationsTranslated);
	//cultureNamesStyleComboBox->addItem(q_("English"),    ConstellationMgr::constellationsEnglish); // This is not useful.
	//Restore the selection
	int index = cultureNamesStyleComboBox->findData(cmgr->getConstellationDisplayStyle(), Qt::UserRole, Qt::MatchCaseSensitive);
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
	l->setCurrentItem(l->findItems(core->projectionTypeKeyToNameI18n(core->getCurrentProjectionTypeKey()), Qt::MatchExactly).at(0));
	l->blockSignals(false);
	ui->projectionTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->projectionTextBrowser->setHtml(core->getProjection(StelCore::FrameJ2000)->getHtmlSummary());

	// Fill the landscape list
	l = ui->landscapesListWidget;
	l->blockSignals(true);
	l->clear();
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	QStringList landscapeList = lmgr->getAllLandscapeNames();
	foreach (const QString landscapeName, landscapeList)
	{
		QString label = q_(landscapeName);
		QListWidgetItem* item = new QListWidgetItem(label);
		item->setData(Qt::UserRole, landscapeName);
		l->addItem(item);
	}
	l->sortItems(); // they may have been translated!
	QString selectedLandscapeName = lmgr->getCurrentLandscapeName();
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
	ui->landscapeTextBrowser->setHtml(lmgr->getCurrentLandscapeHtmlDescription());
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
	l->setCurrentItem(l->findItems(core->projectionTypeKeyToNameI18n(core->getCurrentProjectionTypeKey()), Qt::MatchExactly).at(0),QItemSelectionModel::SelectCurrent);
	ui->projectionTextBrowser->setHtml(core->getProjection(StelCore::FrameJ2000)->getHtmlSummary());
}


void ViewDialog::changeLandscape(QListWidgetItem* item)
{
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	lmgr->setCurrentLandscapeName(item->data(Qt::UserRole).toString());
}

void ViewDialog::landscapeChanged(QString id, QString name)
{
	Q_UNUSED(id)

	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	for (int i = 0; i < ui->landscapesListWidget->count(); i++)
	{
		if (ui->landscapesListWidget->item(i)->data(Qt::UserRole).toString() == name)
		{
			ui->landscapesListWidget->setCurrentRow(i, QItemSelectionModel::SelectCurrent);
			break;
		}
	}
	ui->landscapeTextBrowser->setHtml(lmgr->getCurrentLandscapeHtmlDescription());
	updateDefaultLandscape();
	// Reset values that might have changed.
	ui->showFogCheckBox->setChecked(lmgr->getFlagFog());
}

void ViewDialog::showAddRemoveLandscapesDialog()
{
	if(addRemoveLandscapesDialog == NULL)
		addRemoveLandscapesDialog = new AddRemoveLandscapesDialog();

	addRemoveLandscapesDialog->setVisible(true);
}

void ViewDialog::showAtmosphereDialog()
{
	if(atmosphereDialog == NULL)
		atmosphereDialog = new AtmosphereDialog();

	atmosphereDialog->setVisible(true);
}

void ViewDialog::showGreatRedSpotDialog()
{
	if(greatRedSpotDialog == NULL)
		greatRedSpotDialog = new GreatRedSpotDialog();

	greatRedSpotDialog->setVisible(true);
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
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	Q_ASSERT(lmgr);
	lmgr->setDefaultLandscapeID(lmgr->getCurrentLandscapeID());
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
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	Q_ASSERT(lmgr);
	bool b = lmgr->getCurrentLandscapeID()==lmgr->getDefaultLandscapeID();
	ui->useAsDefaultLandscapeCheckBox->setChecked(b);
	ui->useAsDefaultLandscapeCheckBox->setEnabled(!b);
}

void ViewDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
		current = previous;
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}
