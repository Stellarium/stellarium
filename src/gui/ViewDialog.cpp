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
	ui = new Ui_viewDialogForm;
	addRemoveLandscapesDialog = NULL;
	atmosphereDialog=NULL;
}

ViewDialog::~ViewDialog()
{
	delete ui;
	ui=NULL;
	delete addRemoveLandscapesDialog;
	addRemoveLandscapesDialog = NULL;
	delete atmosphereDialog;
	atmosphereDialog = NULL;
}

void ViewDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateZhrDescription();
		populateLists();
		populateLightPollution();

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

void ViewDialog::connectCheckBox(QCheckBox* checkBox, const QString& actionId)
{
	StelAction* action = StelApp::getInstance().getStelActionManager()->findAction(actionId);
	Q_ASSERT(action);
	checkBox->setChecked(action->isChecked());
	connect(action, SIGNAL(toggled(bool)), checkBox, SLOT(setChecked(bool)));
	connect(checkBox, SIGNAL(toggled(bool)), action, SLOT(setChecked(bool)));
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

	populateLists();
	connect(ui->culturesListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(skyCultureChanged(const QString&)));
	connect(ui->projectionListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(projectionChanged(const QString&)));
	connect(ui->landscapesListWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(landscapeChanged(QListWidgetItem*)));

	// Connect and initialize checkboxes and other widgets

	// Stars section
	ui->starTwinkleCheckBox->setChecked(StelApp::getInstance().getCore()->getSkyDrawer()->getFlagTwinkle());
	connect(ui->starTwinkleCheckBox, SIGNAL(toggled(bool)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setFlagTwinkle(bool)));

	ui->starScaleRadiusDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getAbsoluteStarScale());
	connect(ui->starScaleRadiusDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setAbsoluteStarScale(double)));

	ui->starRelativeScaleDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getRelativeStarScale());
	connect(ui->starRelativeScaleDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setRelativeStarScale(double)));

	MilkyWay* mw = GETSTELMODULE(MilkyWay);
	ui->milkyWayBrightnessDoubleSpinBox->setValue(mw->getIntensity());
	connect(ui->milkyWayBrightnessDoubleSpinBox, SIGNAL(valueChanged(double)), mw, SLOT(setIntensity(double)));

	ZodiacalLight* zl = GETSTELMODULE(ZodiacalLight);
	ui->zodiacalLightBrightnessDoubleSpinBox->setValue(zl->getIntensity());
	connect(ui->zodiacalLightBrightnessDoubleSpinBox, SIGNAL(valueChanged(double)), zl, SLOT(setIntensity(double)));

	ui->starTwinkleAmountDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getTwinkleAmount());
	connect(ui->starTwinkleAmountDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setTwinkleAmount(double)));

	ui->adaptationCheckbox->setChecked(StelApp::getInstance().getCore()->getSkyDrawer()->getFlagLuminanceAdaptation());
	connect(ui->adaptationCheckbox, SIGNAL(toggled(bool)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setFlagLuminanceAdaptation(bool)));

	// Limit Magnitudes
	const StelSkyDrawer* drawer = StelApp::getInstance().getCore()->getSkyDrawer();
	ui->starLimitMagnitudeCheckBox->setChecked(drawer->getFlagStarMagnitudeLimit());
	ui->nebulaLimitMagnitudeCheckBox->setChecked(drawer->getFlagNebulaMagnitudeLimit());
	ui->starLimitMagnitudeDoubleSpinBox->setValue(drawer->getCustomStarMagnitudeLimit());
	ui->nebulaLimitMagnitudeDoubleSpinBox->setValue(drawer->getCustomNebulaMagnitudeLimit());
	
	connect(ui->starLimitMagnitudeCheckBox, SIGNAL(toggled(bool)),
	        drawer, SLOT(setFlagStarMagnitudeLimit(bool)));
	connect(ui->nebulaLimitMagnitudeCheckBox, SIGNAL(toggled(bool)),
	        drawer, SLOT(setFlagNebulaMagnitudeLimit(bool)));
	connect(ui->starLimitMagnitudeDoubleSpinBox, SIGNAL(valueChanged(double)),
	        drawer, SLOT(setCustomStarMagnitudeLimit(double)));
	connect(ui->nebulaLimitMagnitudeDoubleSpinBox,
	        SIGNAL(valueChanged(double)),
	        drawer,
	        SLOT(setCustomNebulaMagnitudeLimit(double)));

	// Planets section
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	ui->showPlanetCheckBox->setChecked(ssmgr->getFlagPlanets());
	connect(ui->showPlanetCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagPlanets(bool)));

	ui->planetMarkerCheckBox->setChecked(ssmgr->getFlagHints());
	connect(ui->planetMarkerCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagHints(bool)));

	ui->planetScaleMoonCheckBox->setChecked(ssmgr->getFlagMoonScale());
	connect(ui->planetScaleMoonCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagMoonScale(bool)));
	ui->moonScaleFactor->setValue(ssmgr->getMoonScale());
	connect(ui->moonScaleFactor, SIGNAL(valueChanged(double)), ssmgr, SLOT(setMoonScale(double)));

	ui->planetOrbitCheckBox->setChecked(ssmgr->getFlagOrbits());
	connect(ui->planetOrbitCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagOrbits(bool)));

	ui->planetLightSpeedCheckBox->setChecked(ssmgr->getFlagLightTravelTime());
	connect(ui->planetLightSpeedCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagLightTravelTime(bool)));

	// Shooting stars section
	SporadicMeteorMgr* mmgr = GETSTELMODULE(SporadicMeteorMgr);
	Q_ASSERT(mmgr);

	ui->zhrSpinBox->setValue(mmgr->getZHR());
	connect(mmgr, SIGNAL(zhrChanged(int)), this, SLOT(updateZhrControls(int)));
	connect(ui->zhrSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setZhrFromControls(int)));
	updateZhrDescription();

	// Labels section
	StarMgr* smgr = GETSTELMODULE(StarMgr);
	connectCheckBox(ui->starLabelCheckBox, "actionShow_Stars_Labels");
	connectCheckBox(ui->nebulaLabelCheckBox, "actionShow_Nebulas");
	connectCheckBox(ui->planetLabelCheckBox, "actionShow_Planets_Labels");

	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);

	ui->starsLabelsHorizontalSlider->setValue((int)(smgr->getLabelsAmount()*10.f));
	connect(ui->starsLabelsHorizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(starsLabelsValueChanged(int)));
	ui->planetsLabelsHorizontalSlider->setValue((int)(ssmgr->getLabelsAmount()*10.f));
	connect(ui->planetsLabelsHorizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(planetsLabelsValueChanged(int)));
	ui->nebulasLabelsHorizontalSlider->setValue((int)(nmgr->getHintsAmount()*10.f));
	connect(ui->nebulasLabelsHorizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(nebulasLabelsValueChanged(int)));

	// Landscape section
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	connectCheckBox(ui->showGroundCheckBox, "actionShow_Ground");
	connectCheckBox(ui->showFogCheckBox, "actionShow_Fog");
	connectGroupBox(ui->atmosphereGroupBox, "actionShow_Atmosphere");
	connectCheckBox(ui->landscapeIlluminationCheckBox, "actionShow_LandscapeIllumination");
	connectCheckBox(ui->landscapeLabelsCheckBox, "actionShow_LandscapeLabels");

	ui->landscapePositionCheckBox->setChecked(lmgr->getFlagLandscapeSetsLocation());
	connect(ui->landscapePositionCheckBox, SIGNAL(toggled(bool)), lmgr, SLOT(setFlagLandscapeSetsLocation(bool)));

	ui->landscapeBrightnessCheckBox->setChecked(lmgr->getFlagLandscapeUseMinimalBrightness());
	connect(ui->landscapeBrightnessCheckBox, SIGNAL(toggled(bool)), this, SLOT(setFlagLandscapeUseMinimalBrightness(bool)));
	ui->landscapeBrightnessSpinBox->setValue(lmgr->getDefaultMinimalBrightness());
	connect(ui->landscapeBrightnessSpinBox, SIGNAL(valueChanged(double)), lmgr, SLOT(setDefaultMinimalBrightness(double)));
	ui->localLandscapeBrightnessCheckBox->setChecked(lmgr->getFlagLandscapeSetsMinimalBrightness());
	connect(ui->localLandscapeBrightnessCheckBox, SIGNAL(toggled(bool)), lmgr, SLOT(setFlagLandscapeSetsMinimalBrightness(bool)));
	populateLandscapeMinimalBrightness();

	// Light pollution
	populateLightPollution();
	ui->useLocationDataCheckBox->setChecked(lmgr->getFlagUseLightPollutionFromDatabase());
	connect(ui->useLocationDataCheckBox, SIGNAL(toggled(bool)), lmgr, SLOT(setFlagUseLightPollutionFromDatabase(bool)));
	connect(lmgr, SIGNAL(lightPollutionUsageChanged(bool)), this, SLOT(populateLightPollution()));
	connect(ui->lightPollutionSpinBox, SIGNAL(valueChanged(int)), lmgr, SLOT(setAtmosphereBortleLightPollution(int)));
	connect(ui->lightPollutionSpinBox, SIGNAL(valueChanged(int)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setBortleScaleIndex(int)));
	connect(ui->lightPollutionSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setBortleScaleToolTip(int)));

	ui->autoChangeLandscapesCheckBox->setChecked(lmgr->getFlagLandscapeAutoSelection());
	connect(ui->autoChangeLandscapesCheckBox, SIGNAL(toggled(bool)), lmgr, SLOT(setFlagLandscapeAutoSelection(bool)));
	
	// atmosphere details
	connect(ui->pushButtonAtmosphereDetails, SIGNAL(clicked()), this, SLOT(showAtmosphereDialog()));

	ui->useAsDefaultLandscapeCheckBox->setChecked(lmgr->getCurrentLandscapeID()==lmgr->getDefaultLandscapeID());
	ui->useAsDefaultLandscapeCheckBox->setEnabled(lmgr->getCurrentLandscapeID()!=lmgr->getDefaultLandscapeID());
	connect(ui->useAsDefaultLandscapeCheckBox, SIGNAL(clicked()), this, SLOT(setCurrentLandscapeAsDefault()));

	connect(GETSTELMODULE(LandscapeMgr), SIGNAL(landscapesChanged()), this, SLOT(populateLists()));
	connect(ui->pushButtonAddRemoveLandscapes, SIGNAL(clicked()), this, SLOT(showAddRemoveLandscapesDialog()));

	// Grid and lines
	connectCheckBox(ui->showEquatorLineCheckBox, "actionShow_Equator_Line");
	connectCheckBox(ui->showEclipticLineCheckBox, "actionShow_Ecliptic_Line");
	connectCheckBox(ui->showMeridianLineCheckBox, "actionShow_Meridian_Line");
	connectCheckBox(ui->showLongitudeLineCheckBox, "actionShow_Longitude_Line");
	connectCheckBox(ui->showHorizonLineCheckBox, "actionShow_Horizon_Line");
	connectCheckBox(ui->showEquatorialGridCheckBox, "actionShow_Equatorial_Grid");
	connectCheckBox(ui->showGalacticGridCheckBox, "actionShow_Galactic_Grid");
	connectCheckBox(ui->showGalacticEquatorLineCheckBox, "actionShow_Galactic_Equator_Line");
	connectCheckBox(ui->showAzimuthalGridCheckBox, "actionShow_Azimuthal_Grid");
	connectCheckBox(ui->showEquatorialJ2000GridCheckBox, "actionShow_Equatorial_J2000_Grid");
	connectCheckBox(ui->showEclipticGridJ2000CheckBox, "actionShow_Ecliptic_J2000_Grid");
	connectCheckBox(ui->showCardinalPointsCheckBox, "actionShow_Cardinal_Points");

	// Constellations
	ConstellationMgr* cmgr = GETSTELMODULE(ConstellationMgr);
	connectCheckBox(ui->showConstellationLinesCheckBox, "actionShow_Constellation_Lines");
	connectCheckBox(ui->showConstellationLabelsCheckBox, "actionShow_Constellation_Labels");
	connectCheckBox(ui->showConstellationBoundariesCheckBox, "actionShow_Constellation_Boundaries");
	connectCheckBox(ui->showConstellationArtCheckBox, "actionShow_Constellation_Art");
	ui->constellationArtBrightnessSpinBox->setValue(cmgr->getArtIntensity());
	connect(ui->constellationArtBrightnessSpinBox, SIGNAL(valueChanged(double)), cmgr, SLOT(setArtIntensity(double)));
	ui->constellationLineThicknessSpinBox->setValue(cmgr->getConstellationLineThickness());
	connect(ui->constellationLineThicknessSpinBox, SIGNAL(valueChanged(double)), cmgr, SLOT(setConstellationLineThickness(double)));

	// Starlore
	connect(ui->useAsDefaultSkyCultureCheckBox, SIGNAL(clicked()), this, SLOT(setCurrentCultureAsDefault()));
	const bool b = StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID()==StelApp::getInstance().getSkyCultureMgr().getDefaultSkyCultureID();
	ui->useAsDefaultSkyCultureCheckBox->setChecked(b);
	ui->useAsDefaultSkyCultureCheckBox->setEnabled(!b);
	connect(ui->nativeNameCheckBox, SIGNAL(clicked(bool)), ssmgr, SLOT(setFlagNativeNames(bool)));
	ui->nativeNameCheckBox->setChecked(ssmgr->getFlagNativeNames());
	// GZ NEW allow to display short names and inhibit translation.
	connect(ui->skyCultureNamesStyleComboBox, SIGNAL(currentIndexChanged(int)), cmgr, SLOT(setConstellationDisplayStyle(int)));

	// Sky layers. This not yet finished and not visible in releases.
	populateSkyLayersList();
	connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(populateSkyLayersList()));
	connect(ui->skyLayerListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(skyLayersSelectionChanged(const QString&)));
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
	connect(ui->skyLayerEnableCheckBox, SIGNAL(stateChanged(int)), this, SLOT(skyLayersEnabledChanged(int)));

	QTimer* refreshTimer = new QTimer(this);
	connect(refreshTimer, SIGNAL(timeout()), this, SLOT(updateFromProgram()));
	refreshTimer->start(200);

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

void ViewDialog::setFlagLandscapeUseMinimalBrightness(bool b)
{
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	lmgr->setFlagLandscapeUseMinimalBrightness(b);
	populateLandscapeMinimalBrightness();
}

void ViewDialog::populateLandscapeMinimalBrightness()
{
	if (ui->landscapeBrightnessCheckBox->isChecked())
	{
		ui->localLandscapeBrightnessCheckBox->setEnabled(true);
		ui->landscapeBrightnessSpinBox->setEnabled(true);
	}
	else
	{
		ui->localLandscapeBrightnessCheckBox->setEnabled(false);
		ui->landscapeBrightnessSpinBox->setEnabled(false);
	}
}

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
	ui->useAsDefaultLandscapeCheckBox->setChecked(lmgr->getDefaultLandscapeID()==lmgr->getCurrentLandscapeID());
	ui->useAsDefaultLandscapeCheckBox->setEnabled(lmgr->getDefaultLandscapeID()!=lmgr->getCurrentLandscapeID());
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

void ViewDialog::skyCultureChanged(const QString& cultureName)
{
	StelApp::getInstance().getSkyCultureMgr().setCurrentSkyCultureNameI18(cultureName);
	updateSkyCultureText();
	const bool b = StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID()==StelApp::getInstance().getSkyCultureMgr().getDefaultSkyCultureID();
	ui->useAsDefaultSkyCultureCheckBox->setChecked(b);
	ui->useAsDefaultSkyCultureCheckBox->setEnabled(!b);
}

// fill the description text window, not the names in the sky.
void ViewDialog::updateSkyCultureText()
{
	StelApp& app = StelApp::getInstance();
	QString skyCultureId = app.getSkyCultureMgr().getCurrentSkyCultureID();
	QString lang = app.getLocaleMgr().getAppLanguage();
	if (!QString("pt_BR zh_CN zh_HK zh_TW").contains(lang))
	{
		lang = lang.split("_").at(0);
	}
	QString descPath = StelFileMgr::findFile("skycultures/" + skyCultureId + "/description."+lang+".utf8");
	if (descPath.isEmpty())
	{
		descPath = StelFileMgr::findFile("skycultures/" + skyCultureId + "/description.en.utf8");
		if (descPath.isEmpty())
			qWarning() << "WARNING: can't find description for skyculture" << skyCultureId;
	}

	QStringList searchPaths;
	searchPaths << StelFileMgr::findFile("skycultures/" + skyCultureId);

	ui->skyCultureTextBrowser->setSearchPaths(searchPaths);
	StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
	Q_ASSERT(gui);
	ui->skyCultureTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	if (descPath.isEmpty())
	{
		ui->skyCultureTextBrowser->setHtml(q_("No description"));
	}
	else
	{
		QFile f(descPath);
		QString htmlFile;
		if(f.open(QIODevice::ReadOnly))
		{
			htmlFile = QString::fromUtf8(f.readAll());
			f.close();
		}
		ui->skyCultureTextBrowser->setHtml(htmlFile);
	}
}

void ViewDialog::projectionChanged(const QString& projectionNameI18n)
{
	StelCore* core = StelApp::getInstance().getCore();
	core->setCurrentProjectionTypeKey(core->projectionNameI18nToTypeKey(projectionNameI18n));
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->projectionTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->projectionTextBrowser->setHtml(core->getProjection(StelCore::FrameJ2000)->getHtmlSummary());
}

void ViewDialog::landscapeChanged(QListWidgetItem* item)
{
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	lmgr->setCurrentLandscapeName(item->data(Qt::UserRole).toString());
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->landscapeTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->landscapeTextBrowser->setHtml(lmgr->getCurrentLandscapeHtmlDescription());
	ui->useAsDefaultLandscapeCheckBox->setChecked(lmgr->getDefaultLandscapeID()==lmgr->getCurrentLandscapeID());
	ui->useAsDefaultLandscapeCheckBox->setEnabled(lmgr->getDefaultLandscapeID()!=lmgr->getCurrentLandscapeID());
	//StelSkyDrawer *drawer=StelApp::getInstance().getSkyDrawer();
	// GZ: Reset values that might have changed.
	ui->showFogCheckBox->setChecked(lmgr->getFlagFog());
	ui->lightPollutionSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getBortleScaleIndex());
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
	//ui->temperatureDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getAtmosphereTemperature());
	//ui->extinctionDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getExtinctionCoefficient());
	//ui->pressureDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getAtmospherePressure());

	atmosphereDialog->setVisible(true);
}


void ViewDialog::setZhrFromControls(int zhr)
{
	SporadicMeteorMgr* mmgr = GETSTELMODULE(SporadicMeteorMgr);
	if (zhr==0)
	{
		mmgr->setFlagShow(false);
	}
	else
	{
		mmgr->setFlagShow(true);
		mmgr->setZHR(zhr);
	}
	updateZhrDescription();
}

void ViewDialog::updateZhrControls(int zhr)
{
	ui->zhrSpinBox->setValue(zhr);
	updateZhrDescription();
}

void ViewDialog::updateZhrDescription()
{
	int zhr = ui->zhrSpinBox->value();
	switch (zhr)
	{
		case 0:
			ui->zhrLabel->setText("<small><i>"+q_("No shooting stars")+"</i></small>");
			break;
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ui->zhrLabel->setText("<small><i>"+q_("Normal rate")+"</i></small>");
			break;
		case 25:
			ui->zhrLabel->setText("<small><i>"+q_("Standard Orionids rate")+"</i></small>");
			break;
		case 100:
			ui->zhrLabel->setText("<small><i>"+q_("Standard Perseids rate")+"</i></small>");
			break;
		case 120:
			ui->zhrLabel->setText("<small><i>"+q_("Standard Geminids rate")+"</i></small>");
			break;
		case 200:
			ui->zhrLabel->setText("<small><i>"+q_("Exceptional Perseid rate")+"</i></small>");
			break;
		case 1000:
			ui->zhrLabel->setText("<small><i>"+q_("Meteor storm rate")+"</i></small>");
			break;
		case 6000:
			ui->zhrLabel->setText("<small><i>"+q_("Exceptional Draconid rate")+"</i></small>");
			break;
		case 10000:
			ui->zhrLabel->setText("<small><i>"+q_("Exceptional Leonid rate")+"</i></small>");
			break;
		case 144000:
			ui->zhrLabel->setText("<small><i>"+q_("Very high rate (1966 Leonids)")+"</i></small>");
			break;
		case 240000:
			ui->zhrLabel->setText("<small><i>"+q_("Highest rate ever (1833 Leonids)")+"</i></small>");
			break;
		default:
			ui->zhrLabel->setText("");
			break;
	}
}

void ViewDialog::starsLabelsValueChanged(int v)
{
	StarMgr* smgr = GETSTELMODULE(StarMgr);
	float a= ((float)v)/10.f;
	smgr->setLabelsAmount(a);
}

void ViewDialog::setCurrentLandscapeAsDefault(void)
{
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	Q_ASSERT(lmgr);
	lmgr->setDefaultLandscapeID(lmgr->getCurrentLandscapeID());
	ui->useAsDefaultLandscapeCheckBox->setChecked(true);
	ui->useAsDefaultLandscapeCheckBox->setEnabled(false);
}

void ViewDialog::setCurrentCultureAsDefault(void)
{
	StelApp::getInstance().getSkyCultureMgr().setDefaultSkyCultureID(StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID());
	ui->useAsDefaultSkyCultureCheckBox->setChecked(true);
	ui->useAsDefaultSkyCultureCheckBox->setEnabled(false);
}

void ViewDialog::planetsLabelsValueChanged(int v)
{
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	float a= ((float)v)/10.f;
	ssmgr->setLabelsAmount(a);
}

void ViewDialog::nebulasLabelsValueChanged(int v)
{
	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	float a= ((float)v)/10.f;
	nmgr->setHintsAmount(a);
	nmgr->setLabelsAmount(a);
}

// Update the widget to make sure it is synchrone if a value was changed programmatically
void ViewDialog::updateFromProgram()
{
	if (!dialog->isVisible())
		return;

	// Check that the useAsDefaultSkyCultureCheckBox needs to be updated
	bool b = StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID()==StelApp::getInstance().getSkyCultureMgr().getDefaultSkyCultureID();
	if (b!=ui->useAsDefaultSkyCultureCheckBox->isChecked())
	{
		ui->useAsDefaultSkyCultureCheckBox->setChecked(b);
		ui->useAsDefaultSkyCultureCheckBox->setEnabled(!b);
	}

	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	Q_ASSERT(lmgr);
	b = lmgr->getCurrentLandscapeID()==lmgr->getDefaultLandscapeID();
	if (b!=ui->useAsDefaultLandscapeCheckBox->isChecked())
	{
		ui->useAsDefaultLandscapeCheckBox->setChecked(b);
		ui->useAsDefaultLandscapeCheckBox->setEnabled(!b);
	}
}

void ViewDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
		current = previous;
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}
