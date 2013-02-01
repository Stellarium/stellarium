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
#include "MeteorMgr.hpp"
#include "MilkyWay.hpp"
#include "GridLinesMgr.hpp"
#include "ConstellationMgr.hpp"
#include "StelStyle.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelGuiBase.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"

#include <QDebug>
#include <QFrame>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QDialog>

ViewDialog::ViewDialog()
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
		setZhrFromControls();
		populateLists();

		//Hack to shrink the tabs to optimal size after language change
		//by causing the list items to be laid out again.
		ui->stackListWidget->setWrapping(false);
	}
}

void ViewDialog::styleChanged()
{
	if (dialog)
	{
		populateLists();
	}
}

void ViewDialog::updateIconsColor()
{
	QPixmap pixmap(50, 50);
	QStringList icons;
	icons << "sky" << "markings" << "landscape" << "starlore";
	bool redIcon = false;
	if (StelApp::getInstance().getVisionModeNight())
		redIcon = true;

	foreach(const QString &iconName, icons)
	{
		pixmap.load(":/graphicGui/tabicon-" + iconName +".png");
		if (redIcon)
			pixmap = StelButton::makeRed(pixmap);

		ui->stackListWidget->item(icons.indexOf(iconName))->setIcon(QIcon(pixmap));
	}
}

void ViewDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(QString)), this, SLOT(updateIconsColor()));

	// Set the Sky tab activated by default
	ui->stackedWidget->setCurrentIndex(0);
	updateIconsColor();
	ui->stackListWidget->setCurrentRow(0);

	//ui->viewTabWidget->removeTab(4);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	populateLists();
	connect(ui->culturesListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(skyCultureChanged(const QString&)));
	connect(ui->projectionListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(projectionChanged(const QString&)));
	connect(ui->landscapesListWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(landscapeChanged(QListWidgetItem*)));

	// Connect and initialize checkboxes and other widgets

	// Stars section
	QAction* a;

	ui->starTwinkleCheckBox->setChecked(StelApp::getInstance().getCore()->getSkyDrawer()->getFlagTwinkle());
	connect(ui->starTwinkleCheckBox, SIGNAL(toggled(bool)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setFlagTwinkle(bool)));

	ui->starScaleRadiusDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getAbsoluteStarScale());
	connect(ui->starScaleRadiusDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setAbsoluteStarScale(double)));

	ui->starRelativeScaleDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getRelativeStarScale());
	connect(ui->starRelativeScaleDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setRelativeStarScale(double)));

	MilkyWay* mw = GETSTELMODULE(MilkyWay);
	ui->milkyWayBrightnessDoubleSpinBox->setValue(mw->getIntensity());
	connect(ui->milkyWayBrightnessDoubleSpinBox, SIGNAL(valueChanged(double)), mw, SLOT(setIntensity(double)));

	ui->starTwinkleAmountDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getTwinkleAmount());
	connect(ui->starTwinkleAmountDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setTwinkleAmount(double)));

	ui->adaptationCheckbox->setChecked(StelApp::getInstance().getCore()->getSkyDrawer()->getFlagLuminanceAdaptation());
	connect(ui->adaptationCheckbox, SIGNAL(toggled(bool)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setFlagLuminanceAdaptation(bool)));

	// Planets section
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	ui->showPlanetCheckBox->setChecked(ssmgr->getFlagPlanets());
	connect(ui->showPlanetCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagPlanets(bool)));

	ui->planetMarkerCheckBox->setChecked(ssmgr->getFlagHints());
	connect(ui->planetMarkerCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagHints(bool)));

	ui->planetScaleMoonCheckBox->setChecked(ssmgr->getFlagMoonScale());
	connect(ui->planetScaleMoonCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagMoonScale(bool)));

	ui->planetOrbitCheckBox->setChecked(ssmgr->getFlagOrbits());
	connect(ui->planetOrbitCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagOrbits(bool)));

	ui->planetLightSpeedCheckBox->setChecked(ssmgr->getFlagLightTravelTime());
	connect(ui->planetLightSpeedCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagLightTravelTime(bool)));

	// Shooting stars section
	MeteorMgr* mmgr = GETSTELMODULE(MeteorMgr);
	Q_ASSERT(mmgr);
	updateZhrControls(mmgr->getZHR());
	connect(mmgr, SIGNAL(zhrChanged(int)),
					this, SLOT(updateZhrControls(int)));
	connect(ui->zhrNone, SIGNAL(clicked()), this, SLOT(setZhrFromControls()));
	connect(ui->zhr10, SIGNAL(clicked()), this, SLOT(setZhrFromControls()));
	connect(ui->zhr80, SIGNAL(clicked()), this, SLOT(setZhrFromControls()));
	connect(ui->zhr10000, SIGNAL(clicked()), this, SLOT(setZhrFromControls()));
	connect(ui->zhr144000, SIGNAL(clicked()), this, SLOT(setZhrFromControls()));

	// Labels section
	StarMgr* smgr = GETSTELMODULE(StarMgr);
	ui->starLabelCheckBox->setChecked(smgr->getFlagLabels());
	connect(ui->starLabelCheckBox, SIGNAL(toggled(bool)), smgr, SLOT(setFlagLabels(bool)));

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	ui->nebulaLabelCheckBox->setChecked(nmgr->getFlagHints());
	a = gui->getGuiAction("actionShow_Nebulas");
	connect(a, SIGNAL(toggled(bool)), ui->nebulaLabelCheckBox, SLOT(setChecked(bool)));
	connect(ui->nebulaLabelCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->planetLabelCheckBox->setChecked(ssmgr->getFlagLabels());
	connect(ui->planetLabelCheckBox, SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagLabels(bool)));

	ui->starsLabelsHorizontalSlider->setValue((int)(smgr->getLabelsAmount()*10.f));
	connect(ui->starsLabelsHorizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(starsLabelsValueChanged(int)));
	ui->planetsLabelsHorizontalSlider->setValue((int)(ssmgr->getLabelsAmount()*10.f));
	connect(ui->planetsLabelsHorizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(planetsLabelsValueChanged(int)));
	ui->nebulasLabelsHorizontalSlider->setValue((int)(nmgr->getHintsAmount()*10.f));
	connect(ui->nebulasLabelsHorizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(nebulasLabelsValueChanged(int)));

	// Landscape section
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	ui->showGroundCheckBox->setChecked(lmgr->getFlagLandscape());
	a = gui->getGuiAction("actionShow_Ground");
	connect(a, SIGNAL(toggled(bool)), ui->showGroundCheckBox, SLOT(setChecked(bool)));
	connect(ui->showGroundCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showFogCheckBox->setChecked(lmgr->getFlagFog());
	connect(ui->showFogCheckBox, SIGNAL(toggled(bool)), lmgr, SLOT(setFlagFog(bool)));

	ui->showAtmosphereCheckBox->setChecked(lmgr->getFlagAtmosphere());
	a = gui->getGuiAction("actionShow_Atmosphere");
	connect(a, SIGNAL(toggled(bool)), ui->showAtmosphereCheckBox, SLOT(setChecked(bool)));
	connect(ui->showAtmosphereCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->landscapePositionCheckBox->setChecked(lmgr->getFlagLandscapeSetsLocation());
	connect(ui->landscapePositionCheckBox, SIGNAL(toggled(bool)), lmgr, SLOT(setFlagLandscapeSetsLocation(bool)));

	ui->lightPollutionSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getBortleScale());
	connect(ui->lightPollutionSpinBox, SIGNAL(valueChanged(int)), lmgr, SLOT(setAtmosphereBortleLightPollution(int)));
	connect(ui->lightPollutionSpinBox, SIGNAL(valueChanged(int)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setBortleScale(int)));
	
	// GZ: changes for refraction
	//ui->pressureDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getAtmospherePressure());
	//connect(ui->pressureDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setAtmospherePressure(double)));
	//ui->temperatureDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getAtmosphereTemperature());
	//connect(ui->temperatureDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setAtmosphereTemperature(double)));
	//ui->extinctionDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getExtinctionCoefficient());
	//connect(ui->extinctionDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setExtinctionCoefficient(double)));
	//// instead
	connect(ui->pushButtonAtmosphereDetails, SIGNAL(clicked()), this, SLOT(showAtmosphereDialog()));
	// GZ: Done


	ui->useAsDefaultLandscapeCheckBox->setChecked(lmgr->getCurrentLandscapeID()==lmgr->getDefaultLandscapeID());
	ui->useAsDefaultLandscapeCheckBox->setEnabled(lmgr->getCurrentLandscapeID()!=lmgr->getDefaultLandscapeID());
	connect(ui->useAsDefaultLandscapeCheckBox, SIGNAL(clicked()), this, SLOT(setCurrentLandscapeAsDefault()));

	connect(GETSTELMODULE(LandscapeMgr), SIGNAL(landscapesChanged()), this, SLOT(populateLists()));
	connect(ui->pushButtonAddRemoveLandscapes, SIGNAL(clicked()), this, SLOT(showAddRemoveLandscapesDialog()));

	// Grid and lines
	GridLinesMgr* glmgr = GETSTELMODULE(GridLinesMgr);
	ui->showEquatorLineCheckBox->setChecked(glmgr->getFlagEquatorLine());
	a = gui->getGuiAction("actionShow_Equator_Line");
	connect(a, SIGNAL(toggled(bool)), ui->showEquatorLineCheckBox, SLOT(setChecked(bool)));
	connect(ui->showEquatorLineCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showEclipticLineCheckBox->setChecked(glmgr->getFlagEclipticLine());
	a = gui->getGuiAction("actionShow_Ecliptic_Line");
	connect(a, SIGNAL(toggled(bool)), ui->showEclipticLineCheckBox, SLOT(setChecked(bool)));
	connect(ui->showEclipticLineCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showMeridianLineCheckBox->setChecked(glmgr->getFlagMeridianLine());
	a = gui->getGuiAction("actionShow_Meridian_Line");
	connect(a, SIGNAL(toggled(bool)), ui->showMeridianLineCheckBox, SLOT(setChecked(bool)));
	connect(ui->showMeridianLineCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showHorizonLineCheckBox->setChecked(glmgr->getFlagHorizonLine());
	a = gui->getGuiAction("actionShow_Horizon_Line");
	connect(a, SIGNAL(toggled(bool)), ui->showHorizonLineCheckBox, SLOT(setChecked(bool)));
	connect(ui->showHorizonLineCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showEquatorialGridCheckBox->setChecked(glmgr->getFlagEquatorGrid());
	a = gui->getGuiAction("actionShow_Equatorial_Grid");
	connect(a, SIGNAL(toggled(bool)), ui->showEquatorialGridCheckBox, SLOT(setChecked(bool)));
	connect(ui->showEquatorialGridCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showGalacticGridCheckBox->setChecked(glmgr->getFlagGalacticGrid());
	a = gui->getGuiAction("actionShow_Galactic_Grid");
	connect(a, SIGNAL(toggled(bool)), ui->showGalacticGridCheckBox, SLOT(setChecked(bool)));
	connect(ui->showGalacticGridCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showGalacticPlaneLineCheckBox->setChecked(glmgr->getFlagGalacticPlaneLine());
	a = gui->getGuiAction("actionShow_Galactic_Plane_Line");
	connect(a, SIGNAL(toggled(bool)), ui->showGalacticPlaneLineCheckBox, SLOT(setChecked(bool)));
	connect(ui->showGalacticPlaneLineCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showAzimuthalGridCheckBox->setChecked(glmgr->getFlagAzimuthalGrid());
	a = gui->getGuiAction("actionShow_Azimuthal_Grid");
	connect(a, SIGNAL(toggled(bool)), ui->showAzimuthalGridCheckBox, SLOT(setChecked(bool)));
	connect(ui->showAzimuthalGridCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showEquatorialJ2000GridCheckBox->setChecked(glmgr->getFlagEquatorJ2000Grid());
	a = gui->getGuiAction("actionShow_Equatorial_J2000_Grid");
	connect(a, SIGNAL(toggled(bool)), ui->showEquatorialJ2000GridCheckBox, SLOT(setChecked(bool)));
	connect(ui->showEquatorialJ2000GridCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showEclipticGridJ2000CheckBox->setChecked(glmgr->getFlagEclipticJ2000Grid());
	a = gui->getGuiAction("actionShow_Ecliptic_J2000_Grid");
	connect(a, SIGNAL(toggled(bool)), ui->showEclipticGridJ2000CheckBox, SLOT(setChecked(bool)));
	connect(ui->showEclipticGridJ2000CheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showCardinalPointsCheckBox->setChecked(lmgr->getFlagCardinalsPoints());
	a = gui->getGuiAction("actionShow_Cardinal_Points");
	connect(a, SIGNAL(toggled(bool)), ui->showCardinalPointsCheckBox, SLOT(setChecked(bool)));
	connect(ui->showCardinalPointsCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	// Constellations
	ConstellationMgr* cmgr = GETSTELMODULE(ConstellationMgr);

	ui->showConstellationLinesCheckBox->setChecked(cmgr->getFlagLines());
	a = gui->getGuiAction("actionShow_Constellation_Lines");
	connect(a, SIGNAL(toggled(bool)), ui->showConstellationLinesCheckBox, SLOT(setChecked(bool)));
	connect(ui->showConstellationLinesCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showConstellationLabelsCheckBox->setChecked(cmgr->getFlagLabels());
	a = gui->getGuiAction("actionShow_Constellation_Labels");
	connect(a, SIGNAL(toggled(bool)), ui->showConstellationLabelsCheckBox, SLOT(setChecked(bool)));
	connect(ui->showConstellationLabelsCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showConstellationBoundariesCheckBox->setChecked(cmgr->getFlagBoundaries());
	a = gui->getGuiAction("actionShow_Constellation_Boundaries");
	connect(a, SIGNAL(toggled(bool)), ui->showConstellationBoundariesCheckBox, SLOT(setChecked(bool)));
	connect(ui->showConstellationBoundariesCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->showConstellationArtCheckBox->setChecked(cmgr->getFlagArt());
	a = gui->getGuiAction("actionShow_Constellation_Art");
	connect(a, SIGNAL(toggled(bool)), ui->showConstellationArtCheckBox, SLOT(setChecked(bool)));
	connect(ui->showConstellationArtCheckBox, SIGNAL(toggled(bool)), a, SLOT(setChecked(bool)));

	ui->constellationArtBrightnessSpinBox->setValue(cmgr->getArtIntensity());
	connect(ui->constellationArtBrightnessSpinBox, SIGNAL(valueChanged(double)), cmgr, SLOT(setArtIntensity(double)));

	// Starlore
	connect(ui->useAsDefaultSkyCultureCheckBox, SIGNAL(clicked()), this, SLOT(setCurrentCultureAsDefault()));
	const bool b = StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID()==StelApp::getInstance().getSkyCultureMgr().getDefaultSkyCultureID();
	ui->useAsDefaultSkyCultureCheckBox->setChecked(b);
	ui->useAsDefaultSkyCultureCheckBox->setEnabled(!b);

	// Sky layers
	populateSkyLayersList();
	connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(populateSkyLayersList()));
	connect(ui->skyLayerListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(skyLayersSelectionChanged(const QString&)));
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
	connect(ui->skyLayerEnableCheckBox, SIGNAL(stateChanged(int)), this, SLOT(skyLayersEnabledChanged(int)));


	QTimer* refreshTimer = new QTimer(this);
	connect(refreshTimer, SIGNAL(timeout()), this, SLOT(updateFromProgram()));
	refreshTimer->start(200);
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

	const StelCore* core = StelApp::getInstance().getCore();

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
	ui->projectionTextBrowser->setHtml(core->getProjection(StelCore::FrameJ2000)->getHtmlSummary());

	// Fill the landscape list
	l = ui->landscapesListWidget;
	l->blockSignals(true);
	l->clear();
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	QStringList landscapeList = lmgr->getAllLandscapeNames();
	foreach (const QString landscapeId, landscapeList)
	{
		QString label = q_(landscapeId);
		QListWidgetItem* item = new QListWidgetItem(label);
		item->setData(Qt::UserRole, landscapeId);
		l->addItem(item);
	}
	QString selectedLandscapeId = lmgr->getCurrentLandscapeName();
	for (int i = 0; i < l->count(); i++)
	{
		if (l->item(i)->data(Qt::UserRole).toString() == selectedLandscapeId)
		{
			l->setCurrentRow(i);
			break;
		}
	}
	l->blockSignals(false);
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

void ViewDialog::updateSkyCultureText()
{
	StelApp& app = StelApp::getInstance();
	QString skyCultureId = app.getSkyCultureMgr().getCurrentSkyCultureID();
	QString descPath;
	try
	{
		QString lang = app.getLocaleMgr().getAppLanguage();
		if (!QString("pt_BR zh_CN zh_HK zh_TW").contains(lang))
		{
			lang = lang.split("_").at(0);
		}
		descPath = StelFileMgr::findFile("skycultures/" + skyCultureId + "/description."+lang+".utf8");
	}
	catch (std::runtime_error& e)
	{
		try
		{
			descPath = StelFileMgr::findFile("skycultures/" + skyCultureId + "/description.en.utf8");
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "WARNING: can't find description for skyculture" << skyCultureId;
		}
	}

	QStringList searchPaths;
	try
	{
		searchPaths << StelFileMgr::findFile("skycultures/" + skyCultureId);
	}
	catch (std::runtime_error& e) {}

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
		f.open(QIODevice::ReadOnly);
		QString htmlFile = QString::fromUtf8(f.readAll());
#if QT_VERSION == 0x040800
		// Workaround for https://bugreports.qt-project.org/browse/QTBUG-24077
		QString path = QFileInfo(f).path();
		QString newtag = "<img src=\"" + path + "/\\1";
		htmlFile.replace(QRegExp("<img src=\"(\\w)"), newtag);
#endif
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
	ui->lightPollutionSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getBortleScale());
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


void ViewDialog::setZhrFromControls()
{
	MeteorMgr* mmgr = GETSTELMODULE(MeteorMgr);
	int zhr=-1;
	if (ui->zhrNone->isChecked())
	{
		mmgr->setFlagShow(false);
		zhr = 0;
	}
	else
	{
		mmgr->setFlagShow(true);
	}
	if (ui->zhr10->isChecked())
		zhr = 10;
	if (ui->zhr80->isChecked())
		zhr = 80;
	if (ui->zhr10000->isChecked())
		zhr = 10000;
	if (ui->zhr144000->isChecked())
		zhr = 144000;
	if (zhr!=mmgr->getZHR())
	{
		mmgr->setZHR(zhr);
	}
	
	updateZhrDescription(zhr);
}

void ViewDialog::updateZhrControls(int zhr)
{
	// As the radio buttons are tied to the clicked() signal,
	// it won't be triggered by setting the value programmatically.
	switch(zhr)
	{
		case 0: ui->zhrNone->setChecked(true); break;
		case 80: ui->zhr80->setChecked(true); break;
		case 10000: ui->zhr10000->setChecked(true); break;
		case 144000: ui->zhr144000->setChecked(true); break;
		default: ui->zhr10->setChecked(true); break;
	}
	
	updateZhrDescription(zhr);
}

void ViewDialog::updateZhrDescription(int zhr)
{
	switch (zhr)
	{
		case 0:
			ui->zhrLabel->setText("<small><i>"+q_("No shooting stars")+"</i></small>");
			break;
		case 10:
			ui->zhrLabel->setText("<small><i>"+q_("Normal rate")+"</i></small>");
			break;
		case 80:
			ui->zhrLabel->setText("<small><i>"+q_("Standard Perseids rate")+"</i></small>");
			break;
		case 10000:
			ui->zhrLabel->setText("<small><i>"+q_("Exceptional Leonid rate")+"</i></small>");
			break;
		case 144000:
			ui->zhrLabel->setText("<small><i>"+q_("Highest rate ever (1966 Leonids)")+"</i></small>");
			break;
		default:
			ui->zhrLabel->setText(QString("<small><i>")+"Error"+"</i></small>");
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
