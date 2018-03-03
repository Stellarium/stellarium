/*
 * Stellarium
 * Copyright (C) 2015 Alexander Wolf
 * Copyright (C) 2016 Nick Fedoseev (visualization of ephemeris)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelLocaleMgr.hpp"
#include "StelFileMgr.hpp"

#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "NebulaMgr.hpp"
#include "Nebula.hpp"

#ifdef USE_STATIC_PLUGIN_SATELLITES
#include "../plugins/Satellites/src/Satellites.hpp"
#endif

#include "AstroCalcDialog.hpp"
#include "ui_astroCalcDialog.h"
#include "external/qcustomplot/qcustomplot.h"

#include <QFileDialog>
#include <QDir>

QVector<Vec3d> AstroCalcDialog::EphemerisListCoords;
QVector<QString> AstroCalcDialog::EphemerisListDates;
QVector<float> AstroCalcDialog::EphemerisListMagnitudes;
int AstroCalcDialog::DisplayedPositionIndex = -1;
float AstroCalcDialog::brightLimit = 10.f;
float AstroCalcDialog::minY = -90.f;
float AstroCalcDialog::maxY = 90.f;
float AstroCalcDialog::minYme = -90.f;
float AstroCalcDialog::maxYme = 90.f;
float AstroCalcDialog::minYsun = -90.f;
float AstroCalcDialog::maxYsun = 90.f;
float AstroCalcDialog::minYmoon = -90.f;
float AstroCalcDialog::maxYmoon = 90.f;
float AstroCalcDialog::minY1 = -1001.f;
float AstroCalcDialog::maxY1 = 1001.f;
float AstroCalcDialog::minY2 = -1001.f;
float AstroCalcDialog::maxY2 = 1001.f;
float AstroCalcDialog::transitX = -1.f;
QString AstroCalcDialog::yAxis1Legend = "";
QString AstroCalcDialog::yAxis2Legend = "";

AstroCalcDialog::AstroCalcDialog(QObject* parent)
	: StelDialog("AstroCalc", parent)
	, currentTimeLine(Q_NULLPTR)
	, plotAltVsTime(false)	
	, plotAltVsTimeSun(false)
	, plotAltVsTimeMoon(false)
	, plotAltVsTimePositive(false)
	, plotMonthlyElevation(false)
	, plotMonthlyElevationPositive(false)
	, delimiter(", ")
	, acEndl("\n")
{
	ui = new Ui_astroCalcDialogForm;
	core = StelApp::getInstance().getCore();
	solarSystem = GETSTELMODULE(SolarSystem);
	dsoMgr = GETSTELMODULE(NebulaMgr);
	objectMgr = GETSTELMODULE(StelObjectMgr);
	starMgr = GETSTELMODULE(StarMgr);
	mvMgr = GETSTELMODULE(StelMovementMgr);
	localeMgr = &StelApp::getInstance().getLocaleMgr();
	conf = StelApp::getInstance().getSettings();
	ephemerisHeader.clear();
	phenomenaHeader.clear();
	positionsHeader.clear();
}

AstroCalcDialog::~AstroCalcDialog()
{
	if (currentTimeLine)
	{
		currentTimeLine->stop();
		delete currentTimeLine;
		currentTimeLine = Q_NULLPTR;
	}
	delete ui;
}

void AstroCalcDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setCelestialPositionsHeaderNames();
		setEphemerisHeaderNames();
		setPhenomenaHeaderNames();
		populateCelestialBodyList();
		populateCelestialCategoryList();
		populateEphemerisTimeStepsList();
		populateMajorPlanetList();
		populateGroupCelestialBodyList();
		currentCelestialPositions();
		prepareAxesAndGraph();
		populateFunctionsList();
		prepareXVsTimeAxesAndGraph();
		prepareMonthlyEleveationAxesAndGraph();
		drawAltVsTimeDiagram();
		populateTimeIntervalsList();
		populateWutGroups();
		// Hack to shrink the tabs to optimal size after language change
		// by causing the list items to be laid out again.
		updateTabBarListWidgetWidth();
	}
}

void AstroCalcDialog::styleChanged()
{
	// Nothing for now
}

void AstroCalcDialog::createDialogContent()
{
	ui->setupUi(dialog);

#ifdef Q_OS_WIN
	// Kinetic scrolling for tablet pc and pc
	QList<QWidget*> addscroll;
	addscroll << ui->celestialPositionsTreeWidget << ui->ephemerisTreeWidget << ui->phenomenaTreeWidget
			  << ui->wutCategoryListWidget << ui->wutMatchingObjectsListWidget;
	installKineticScrolling(addscroll);
	acEndl = "\r\n";
#else
	acEndl = "\n";
#endif

	// Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	ui->stackedWidget->setCurrentIndex(0);
	ui->stackListWidget->setCurrentRow(0);
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	initListCelestialPositions();
	initListPhenomena();
	populateCelestialBodyList();
	populateCelestialCategoryList();
	populateEphemerisTimeStepsList();
	populateMajorPlanetList();
	populateGroupCelestialBodyList();
	// Altitude vs. Time feature
	prepareAxesAndGraph();
	drawCurrentTimeDiagram();
	// Graphs feature
	populateFunctionsList();
	prepareXVsTimeAxesAndGraph();
	// Monthly Elevation
	prepareMonthlyEleveationAxesAndGraph();
	// WUT
	populateTimeIntervalsList();
	populateWutGroups();

	double JD = core->getJD() + core->getUTCOffset(core->getJD()) / 24;
	QDateTime currentDT = StelUtils::jdToQDateTime(JD);
	ui->dateFromDateTimeEdit->setDateTime(currentDT);
	ui->dateToDateTimeEdit->setDateTime(currentDT.addMonths(1));
	ui->phenomenFromDateEdit->setDateTime(currentDT);
	ui->phenomenToDateEdit->setDateTime(currentDT.addMonths(1));
	ui->monthlyElevationTimeInfo->setStyleSheet("font-size: 18pt");

	// TODO: Switch a QDateTimeEdit to StelDateTimeEdit widget to apply wide range of dates
	QDate min = QDate(100, 1, 1);
	ui->dateFromDateTimeEdit->setMinimumDate(min);
	ui->dateToDateTimeEdit->setMinimumDate(min);
	ui->phenomenFromDateEdit->setMinimumDate(min);
	ui->phenomenToDateEdit->setMinimumDate(min);

	// bug #1350669 (https://bugs.launchpad.net/stellarium/+bug/1350669)
	connect(ui->celestialPositionsTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), ui->celestialPositionsTreeWidget, SLOT(repaint()));

	ui->celestialMagnitudeDoubleSpinBox->setValue(conf->value("astrocalc/celestial_magnitude_limit", 6.0).toDouble());
	connect(ui->celestialMagnitudeDoubleSpinBox, SIGNAL(valueChanged(double)), this,  SLOT(saveCelestialPositionsMagnitudeLimit(double)));

	ui->horizontalCoordinatesCheckBox->setChecked(conf->value("astrocalc/flag_horizontal_coordinates", false).toBool());
	connect(ui->horizontalCoordinatesCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveCelestialPositionsHorizontalCoordinatesFlag(bool)));

	connect(ui->celestialPositionsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentCelestialPosition(QModelIndex)));
	connect(ui->celestialPositionsUpdateButton, SIGNAL(clicked()), this, SLOT(currentCelestialPositions()));
	connect(ui->celestialPositionsSaveButton, SIGNAL(clicked()), this, SLOT(saveCelestialPositions()));
	connect(ui->celestialCategoryComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveCelestialPositionsCategory(int)));
	connect(dsoMgr, SIGNAL(catalogFiltersChanged(Nebula::CatalogGroup)), this, SLOT(populateCelestialCategoryList()));
	connect(dsoMgr, SIGNAL(catalogFiltersChanged(Nebula::CatalogGroup)), this, SLOT(currentCelestialPositions()));
	connect(dsoMgr, SIGNAL(flagSizeLimitsUsageChanged(bool)), this, SLOT(currentCelestialPositions()));
	connect(dsoMgr, SIGNAL(minSizeLimitChanged(double)), this, SLOT(currentCelestialPositions()));
	connect(dsoMgr, SIGNAL(maxSizeLimitChanged(double)), this, SLOT(currentCelestialPositions()));

	connectBoolProperty(ui->ephemerisShowMarkersCheckBox, "SolarSystem.ephemerisMarkersDisplayed");
	connectBoolProperty(ui->ephemerisShowDatesCheckBox, "SolarSystem.ephemerisDatesDisplayed");
	connectBoolProperty(ui->ephemerisShowMagnitudesCheckBox, "SolarSystem.ephemerisMagnitudesDisplayed");
	connectBoolProperty(ui->ephemerisHorizontalCoordinatesCheckBox, "SolarSystem.ephemerisHorizontalCoordinates");
	initListEphemeris();
	connect(ui->ephemerisHorizontalCoordinatesCheckBox, SIGNAL(toggled(bool)), this, SLOT(reGenerateEphemeris()));
	connect(ui->ephemerisPushButton, SIGNAL(clicked()), this, SLOT(generateEphemeris()));
	connect(ui->ephemerisCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupEphemeris()));
	connect(ui->ephemerisSaveButton, SIGNAL(clicked()), this, SLOT(saveEphemeris()));
	connect(ui->ephemerisTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentEphemeride(QModelIndex)));
	connect(ui->ephemerisTreeWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(onChangedEphemerisPosition(QModelIndex)));
	connect(ui->ephemerisStepComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveEphemerisTimeStep(int)));
	connect(ui->celestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveEphemerisCelestialBody(int)));

	ui->phenomenaOppositionCheckBox->setChecked(conf->value("astrocalc/flag_phenomena_opposition", false).toBool());
	connect(ui->phenomenaOppositionCheckBox, SIGNAL(toggled(bool)), this, SLOT(savePhenomenaOppositionFlag(bool)));
	ui->allowedSeparationDoubleSpinBox->setValue(conf->value("astrocalc/phenomena_angular_separation", 1.0).toDouble());
	connect(ui->allowedSeparationDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(savePhenomenaAngularSeparation(double)));

	connect(ui->phenomenaPushButton, SIGNAL(clicked()), this, SLOT(calculatePhenomena()));
	connect(ui->phenomenaCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupPhenomena()));
	connect(ui->phenomenaTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentPhenomen(QModelIndex)));
	connect(ui->phenomenaSaveButton, SIGNAL(clicked()), this, SLOT(savePhenomena()));
	connect(ui->object1ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(savePhenomenaCelestialBody(int)));
	connect(ui->object2ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(savePhenomenaCelestialGroup(int)));

	plotAltVsTimeSun = conf->value("astrocalc/altvstime_sun", false).toBool();
	plotAltVsTimeMoon = conf->value("astrocalc/altvstime_moon", false).toBool();
	plotAltVsTimePositive = conf->value("astrocalc/altvstime_positive_only", false).toBool();
	ui->sunAltitudeCheckBox->setChecked(plotAltVsTimeSun);
	ui->moonAltitudeCheckBox->setChecked(plotAltVsTimeMoon);
	ui->positiveAltitudeOnlyCheckBox->setChecked(plotAltVsTimePositive);
	connect(ui->sunAltitudeCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveAltVsTimeSunFlag(bool)));
	connect(ui->moonAltitudeCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveAltVsTimeMoonFlag(bool)));
	connect(ui->positiveAltitudeOnlyCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveAltVsTimePositiveFlag(bool)));
	connect(ui->altVsTimePlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseOverLine(QMouseEvent*)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawAltVsTimeDiagram()));
	connect(core, SIGNAL(dateChanged()), this, SLOT(drawAltVsTimeDiagram()));
	drawAltVsTimeDiagram();

	// Monthly Elevation
	plotMonthlyElevationPositive = conf->value("astrocalc/me_positive_only", false).toBool();
	ui->monthlyElevationPositiveCheckBox->setChecked(plotMonthlyElevationPositive);
	ui->monthlyElevationTime->setValue(conf->value("astrocalc/me_time", 0).toInt());	
	syncMonthlyElevationTime();
	connect(ui->monthlyElevationTime, SIGNAL(sliderReleased()), this, SLOT(updateMonthlyElevationTime()));
	connect(ui->monthlyElevationTime, SIGNAL(sliderMoved(int)), this, SLOT(syncMonthlyElevationTime()));
	connect(ui->monthlyElevationPositiveCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveMonthlyElevationPositiveFlag(bool)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawMonthlyElevationGraph()));
	connect(core, SIGNAL(dateChangedByYear()), this, SLOT(drawMonthlyElevationGraph()));
	drawMonthlyElevationGraph();

	connect(ui->graphsCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveGraphsCelestialBody(int)));
	connect(ui->graphsFirstComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveGraphsFirstId(int)));
	connect(ui->graphsSecondComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveGraphsSecondId(int)));
	connect(ui->drawGraphsPushButton, SIGNAL(clicked()), this, SLOT(drawXVsTimeGraphs()));

	ui->wutMagnitudeDoubleSpinBox->setValue(conf->value("astrocalc/wut_magnitude_limit", 10.0).toDouble());
	connect(ui->wutMagnitudeDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(saveWutMagnitudeLimit(double)));
	connect(ui->wutComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveWutTimeInterval(int)));
	connect(ui->wutCategoryListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(calculateWutObjects()));
	connect(ui->wutMatchingObjectsListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(selectWutObject()));
	connect(ui->saveObjectsButton, SIGNAL(clicked()), this, SLOT(saveWutObjects()));
	connect(dsoMgr, SIGNAL(catalogFiltersChanged(Nebula::CatalogGroup)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(typeFiltersChanged(Nebula::TypeGroup)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(flagSizeLimitsUsageChanged(bool)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(minSizeLimitChanged(double)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(maxSizeLimitChanged(double)), this, SLOT(calculateWutObjects()));

	currentCelestialPositions();

	currentTimeLine = new QTimer(this);
	connect(currentTimeLine, SIGNAL(timeout()), this, SLOT(drawCurrentTimeDiagram()));
	connect(currentTimeLine, SIGNAL(timeout()), this, SLOT(computePlanetaryData()));	
	currentTimeLine->start(500); // Update 'now' line position every 0.5 seconds

	connect(ui->firstCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveFirstCelestialBody(int)));
	connect(ui->secondCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveSecondCelestialBody(int)));	

	connect(solarSystem, SIGNAL(solarSystemDataReloaded()), this, SLOT(updateSolarSystemData()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(updateAstroCalcData()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawAltVsTimeDiagram()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawMonthlyElevationGraph()));
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(changePage(QListWidgetItem*, QListWidgetItem*)));

	updateTabBarListWidgetWidth();
}

void AstroCalcDialog::updateAstroCalcData()
{
	drawAltVsTimeDiagram();
	populateCelestialBodyList();
	populateMajorPlanetList();
}

void AstroCalcDialog::saveAltVsTimeSunFlag(bool state)
{
	if (plotAltVsTimeSun!=state)
	{
		plotAltVsTimeSun = state;
		conf->setValue("astrocalc/altvstime_sun", plotAltVsTimeSun);

		drawAltVsTimeDiagram();
	}
}

void AstroCalcDialog::saveAltVsTimeMoonFlag(bool state)
{
	if (plotAltVsTimeMoon!=state)
	{
		plotAltVsTimeMoon = state;
		conf->setValue("astrocalc/altvstime_moon", plotAltVsTimeMoon);

		drawAltVsTimeDiagram();
	}
}

void AstroCalcDialog::saveAltVsTimePositiveFlag(bool state)
{
	if (plotAltVsTimePositive!=state)
	{
		plotAltVsTimePositive = state;
		conf->setValue("astrocalc/altvstime_positive_only", plotAltVsTimePositive);

		drawAltVsTimeDiagram();
	}
}

void AstroCalcDialog::initListCelestialPositions()
{
	ui->celestialPositionsTreeWidget->clear();
	ui->celestialPositionsTreeWidget->setColumnCount(CColumnCount);

	setCelestialPositionsHeaderNames();

	ui->celestialPositionsTreeWidget->header()->setSectionsMovable(false);
}

void AstroCalcDialog::setCelestialPositionsHeaderNames()
{
	Q_ASSERT(ui->celestialCategoryComboBox);
	QComboBox* category = ui->celestialCategoryComboBox;
	int celType = category->itemData(category->currentIndex()).toInt();

	bool horizon = ui->horizontalCoordinatesCheckBox->isChecked();

	positionsHeader.clear();
	// TRANSLATORS: name of object
	positionsHeader << q_("Name");
	if (horizon)
	{
		// TRANSLATORS: azimuth
		positionsHeader << q_("Azimuth");
		// TRANSLATORS: altitude
		positionsHeader << q_("Altitude");
	}
	else
	{
		// TRANSLATORS: right ascension
		positionsHeader << q_("RA (J2000)");
		// TRANSLATORS: declination
		positionsHeader << q_("Dec (J2000)");
	}
	if (celType == 12 || celType == 102 || celType == 111) // check for dark nebulae
	{
		// TRANSLATORS: opacity
		positionsHeader << q_("opacity");
	}
	else
	{
		// TRANSLATORS: magnitude
		positionsHeader << q_("mag");
	}
	// TRANSLATORS: angular size, arcminutes
	positionsHeader << QString("%1, '").arg(q_("A.S."));
	if (celType == 170)
	{
		// TRANSLATORS: separation, arcseconds
		positionsHeader << QString("%1, \"").arg(q_("sep."));
	}
	else if (celType == 171)
	{
		// TRANSLATORS: period, days
		positionsHeader << QString("%1, %2").arg(q_("per."), qc_("d", "days"));
	}
	else if (celType >= 200)
	{
		// TRANSLATORS: distance, AU
		positionsHeader << QString("%1, %2").arg(q_("dist."), qc_("AU", "distance, astronomical unit"));
	}
	else if (celType == 172)
	{
		// TRANSLATORS: proper motion, arcsecond per year
		positionsHeader << QString("%1, %2").arg(q_("P.M."), qc_("\"/yr", "arcsecond per year"));
	}
	else
	{
		// TRANSLATORS: surface brightness
		positionsHeader << q_("S.B.");
	}
	// TRANSLATORS: type of object
	positionsHeader << q_("Type");

	ui->celestialPositionsTreeWidget->setHeaderLabels(positionsHeader);
	// adjust the column width
	for (int i = 0; i < CColumnCount; ++i)
	{
		ui->celestialPositionsTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::onChangedEphemerisPosition(const QModelIndex& modelIndex)
{
	DisplayedPositionIndex = modelIndex.row();
}

void AstroCalcDialog::populateCelestialCategoryList()
{
	Q_ASSERT(ui->celestialCategoryComboBox);

	QComboBox* category = ui->celestialCategoryComboBox;

	category->blockSignals(true);
	int index = category->currentIndex();
	QVariant selectedCategoryId = category->itemData(index);

	const Nebula::CatalogGroup& catalogFilters = dsoMgr->getCatalogFilters();

	category->clear();
	// TODO: Automatic sync list with QMap<QString, QString> StelObjectMgr::objectModulesMap() data
	category->addItem(q_("Galaxies"), "0");
	category->addItem(q_("Active galaxies"), "1");
	category->addItem(q_("Radio galaxies"), "2");
	category->addItem(q_("Interacting galaxies"), "3");
	category->addItem(q_("Bright quasars"), "4");
	category->addItem(q_("Star clusters"), "5");
	category->addItem(q_("Open star clusters"), "6");
	category->addItem(q_("Globular star clusters"), "7");
	category->addItem(q_("Stellar associations"), "8");
	category->addItem(q_("Star clouds"), "9");
	category->addItem(q_("Nebulae"), "10");
	category->addItem(q_("Planetary nebulae"), "11");
	category->addItem(q_("Dark nebulae"), "12");
	category->addItem(q_("Reflection nebulae"), "13");
	category->addItem(q_("Bipolar nebulae"), "14");
	category->addItem(q_("Emission nebulae"), "15");
	category->addItem(q_("Clusters associated with nebulosity"), "16");
	category->addItem(q_("HII regions"), "17");
	category->addItem(q_("Supernova remnants"), "18");
	category->addItem(q_("Interstellar matter"), "19");
	category->addItem(q_("Emission objects"), "20");
	category->addItem(q_("BL Lac objects"), "21");
	category->addItem(q_("Blazars"), "22");
	category->addItem(q_("Molecular Clouds"), "23");
	category->addItem(q_("Young Stellar Objects"), "24");
	category->addItem(q_("Possible Quasars"), "25");
	category->addItem(q_("Possible Planetary Nebulae"), "26");
	category->addItem(q_("Protoplanetary Nebulae"), "27");
	category->addItem(q_("Symbiotic stars"), "29");
	category->addItem(q_("Emission-line stars"), "30");
	category->addItem(q_("Supernova candidates"), "31");
	category->addItem(q_("Supernova remnant candidates"), "32");
	category->addItem(q_("Clusters of galaxies"), "33");
	if (catalogFilters & Nebula::CatM)
		category->addItem(q_("Messier Catalogue"), "100");
	if (catalogFilters & Nebula::CatC)
		category->addItem(q_("Caldwell Catalogue"), "101");
	if (catalogFilters & Nebula::CatB)
		category->addItem(q_("Barnard Catalogue"), "102");
	if (catalogFilters & Nebula::CatSh2)
		category->addItem(q_("Sharpless Catalogue"), "103");
	if (catalogFilters & Nebula::CatVdB)
		category->addItem(q_("Van den Bergh Catalogue"), "104");
	if (catalogFilters & Nebula::CatRCW)
		category->addItem(q_("The Catalogue of Rodgers, Campbell, and Whiteoak"), "105");
	if (catalogFilters & Nebula::CatCr)
		category->addItem(q_("Collinder Catalogue"), "106");
	if (catalogFilters & Nebula::CatMel)
		category->addItem(q_("Melotte Catalogue"), "107");
	if (catalogFilters & Nebula::CatNGC)
		category->addItem(q_("New General Catalogue"), "108");
	if (catalogFilters & Nebula::CatIC)
		category->addItem(q_("Index Catalogue"), "109");
	if (catalogFilters & Nebula::CatLBN)
		category->addItem(q_("Lynds' Catalogue of Bright Nebulae"), "110");
	if (catalogFilters & Nebula::CatLDN)
		category->addItem(q_("Lynds' Catalogue of Dark Nebulae"), "111");
	if (catalogFilters & Nebula::CatPGC)
		category->addItem(q_("Principal Galaxy Catalog"), "112");
	if (catalogFilters & Nebula::CatUGC)
		category->addItem(q_("The Uppsala General Catalogue of Galaxies"), "113");
	if (catalogFilters & Nebula::CatCed)
		category->addItem(q_("Cederblad Catalog"), "114");
	if (catalogFilters & Nebula::CatArp)
		category->addItem(q_("The Catalogue of Peculiar Galaxies"), "115");
	if (catalogFilters & Nebula::CatVV)
		category->addItem(q_("The Catalogue of Interacting Galaxies"), "116");
	if (catalogFilters & Nebula::CatPK)
		category->addItem(q_("The Catalogue of Galactic Planetary Nebulae"), "117");
	if (catalogFilters & Nebula::CatPNG)
		category->addItem(q_("The Strasbourg-ESO Catalogue of Galactic Planetary Nebulae"), "118");
	if (catalogFilters & Nebula::CatSNRG)
		category->addItem(q_("A catalogue of Galactic supernova remnants"), "119");
	if (catalogFilters & Nebula::CatACO)
		category->addItem(q_("A Catalog of Rich Clusters of Galaxies"), "120");
	if (catalogFilters & Nebula::CatHCG)
		category->addItem(q_("Hickson Compact Group"), "121");
	category->addItem(q_("Dwarf galaxies"), "150");
	category->addItem(q_("Herschel 400 Catalogue"), "151");
	category->addItem(q_("Bright double stars"), "170");
	category->addItem(q_("Bright variable stars"), "171");
	category->addItem(q_("Bright stars with high proper motion"), "172");
	category->addItem(q_("Solar system objects"), "200");
	category->addItem(q_("Solar system objects: comets"), "201");
	category->addItem(q_("Solar system objects: minor bodies"), "202");
	category->addItem(q_("Solar system objects: planets"), "203");

	index = category->findData(selectedCategoryId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index < 0) // read config data
		index = category->findData(conf->value("astrocalc/celestial_category", "200").toString(), Qt::UserRole, Qt::MatchCaseSensitive);

	if (index < 0) // Unknown yet? Default step: Solar system objects
		index = category->findData("200", Qt::UserRole, Qt::MatchCaseSensitive);

	category->setCurrentIndex(index);
	category->model()->sort(0);
	category->blockSignals(false);
}

void AstroCalcDialog::saveCelestialPositionsMagnitudeLimit(double mag)
{
	conf->setValue("astrocalc/celestial_magnitude_limit", QString::number(mag, 'f', 2));
	// Refresh the celestial bodies positions table
	currentCelestialPositions();
}

void AstroCalcDialog::saveCelestialPositionsHorizontalCoordinatesFlag(bool b)
{
	conf->setValue("astrocalc/flag_horizontal_coordinates", b);
	// Refresh the celestial bodies positions table
	currentCelestialPositions();
}

void AstroCalcDialog::saveCelestialPositionsCategory(int index)
{
	Q_ASSERT(ui->celestialCategoryComboBox);
	QComboBox* category = ui->celestialCategoryComboBox;
	conf->setValue("astrocalc/celestial_category", category->itemData(index).toInt());
	// Refresh the celestial bodies positions table
	currentCelestialPositions();
}

void AstroCalcDialog::currentCelestialPositions()
{
	float ra, dec;
	QString raStr, decStr, extra, angularSize, celObjName = "", celObjId = "";

	initListCelestialPositions();

	double mag = ui->celestialMagnitudeDoubleSpinBox->value();
	bool horizon = ui->horizontalCoordinatesCheckBox->isChecked();
	bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();

	StelCore* core = StelApp::getInstance().getCore();
	double JD = core->getJD();
	ui->celestialPositionsTimeLabel->setText(q_("Positions on %1").arg(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))));

	Q_ASSERT(ui->celestialCategoryComboBox);
	QComboBox* category = ui->celestialCategoryComboBox;
	QString celType = category->itemData(category->currentIndex()).toString();
	int celTypeId = celType.toInt();

	if (celTypeId < 170)
	{
		QString mu;
		if (dsoMgr->getFlagSurfaceBrightnessShortNotationUsage())
		{
			mu = QString("<sup>m</sup>/%1'").arg(QChar(0x2B1C));
			if (dsoMgr->getFlagSurfaceBrightnessArcsecUsage())
				mu = QString("<sup>m</sup>/%1\"").arg(QChar(0x2B1C));
		}
		else
		{
			mu = QString("%1/%2<sup>2</sup>").arg(qc_("mag", "magnitude"), q_("arcmin"));
			if (dsoMgr->getFlagSurfaceBrightnessArcsecUsage())
				mu = QString("%1/%2<sup>2</sup>").arg(qc_("mag", "magnitude"), q_("arcsec"));
		}
		float magOp;
		QString dsoName;
		QString asToolTip = QString("%1, %2").arg(q_("Average angular size"), q_("arcmin"));
		// Deep-sky objects
		QList<NebulaP> celestialObjects = dsoMgr->getDeepSkyObjectsByType(celType);
		foreach (const NebulaP& obj, celestialObjects)
		{
			if (celTypeId == 12 || celTypeId == 102 || celTypeId == 111) // opacity cannot be extincted
				magOp = obj->getVMagnitude(core);
			else
				magOp = obj->getVMagnitudeWithExtinction(core);

			if (obj->objectInDisplayedCatalog() && obj->objectInAllowedSizeRangeLimits() && magOp <= mag && obj->isAboveRealHorizon(core))
			{
				if (horizon)
				{
					StelUtils::rectToSphe(&ra, &dec, obj->getAltAzPosAuto(core));
					float direction = 3.; // N is zero, E is 90 degrees
					if (useSouthAzimuth)
						direction = 2.;
					ra = direction * M_PI - ra;
					if (ra > M_PI * 2)
						ra -= M_PI * 2;
					raStr = StelUtils::radToDmsStr(ra, true);
					decStr = StelUtils::radToDmsStr(dec, true);
				}
				else
				{
					StelUtils::rectToSphe(&ra, &dec, obj->getJ2000EquatorialPos(core));
					raStr = StelUtils::radToHmsStr(ra);
					decStr = StelUtils::radToDmsStr(dec, true);
				}

				ACCelPosTreeWidgetItem* treeItem =
				  new ACCelPosTreeWidgetItem(ui->celestialPositionsTreeWidget);

				celObjName = obj->getNameI18n();
				celObjId = obj->getDSODesignation();
				if (celObjId.isEmpty())
					dsoName = celObjName;
				else if (celObjName.isEmpty())
					dsoName = celObjId;
				else
					dsoName = QString("%1 (%2)").arg(celObjId, celObjName);

				extra = QString::number(obj->getSurfaceBrightnessWithExtinction(core), 'f', 2);
				if (extra.toFloat() > 90.f)
					extra = QChar(0x2014);

				// Convert to arcminutes the average angular size of deep-sky object
				angularSize = QString::number(obj->getAngularSize(core) * 120.f, 'f', 3);
				if (angularSize.toFloat() < 0.01f)
					angularSize = QChar(0x2014);

				treeItem->setText(CColumnName, dsoName);
				treeItem->setText(CColumnRA, raStr);
				treeItem->setTextAlignment(CColumnRA, Qt::AlignRight);
				treeItem->setText(CColumnDec, decStr);
				treeItem->setTextAlignment(CColumnDec, Qt::AlignRight);
				treeItem->setText(CColumnMagnitude, QString::number(magOp, 'f', 2));
				treeItem->setTextAlignment(CColumnMagnitude, Qt::AlignRight);
				treeItem->setText(CColumnAngularSize, angularSize);
				treeItem->setTextAlignment(CColumnAngularSize, Qt::AlignRight);
				treeItem->setToolTip(CColumnAngularSize, asToolTip);
				treeItem->setText(CColumnExtra, extra);
				treeItem->setTextAlignment(CColumnExtra, Qt::AlignRight);
				treeItem->setToolTip(CColumnExtra, mu);
				treeItem->setText(CColumnType, q_(obj->getTypeString()));
			}
		}
	}
	else if (celTypeId == 200)
	{
		QList<PlanetP> allPlanets = solarSystem->getAllPlanets();
		QString distanceInfo = q_("Planetocentric distance");
		if (core->getUseTopocentricCoordinates()) distanceInfo = q_("Topocentric distance");
		QString distanceUM = qc_("AU", "distance, astronomical unit");
		QString sToolTip = QString("%1, %2").arg(distanceInfo, distanceUM);
		QString asToolTip = QString("%1, %2").arg(q_("Angular size (with rings, if any)"), q_("arcmin"));
		Vec3d pos;
		foreach (const PlanetP& planet, allPlanets)
		{
			if ((planet->getPlanetType() != Planet::isUNDEFINED && planet != core->getCurrentPlanet()) && planet->getVMagnitudeWithExtinction(core) <= mag && planet->isAboveRealHorizon(core))
			{
				pos = planet->getJ2000EquatorialPos(core);
				if (horizon)
				{
					StelUtils::rectToSphe(&ra, &dec, planet->getAltAzPosAuto(core));
					float direction = 3.; // N is zero, E is 90 degrees
					if (useSouthAzimuth)
						direction = 2.;
					ra = direction * M_PI - ra;
					if (ra > M_PI * 2)
						ra -= M_PI * 2;
					raStr = StelUtils::radToDmsStr(ra, true);
					decStr = StelUtils::radToDmsStr(dec, true);
				}
				else
				{
					StelUtils::rectToSphe(&ra, &dec, pos);
					raStr = StelUtils::radToHmsStr(ra);
					decStr = StelUtils::radToDmsStr(dec, true);
				}

				extra = QString::number(pos.length(), 'f', 5); // A.U.

				// Convert to arcseconds the angular size of Solar system object (with rings, if any)
				angularSize = QString::number(planet->getAngularSize(core) * 120.f, 'f', 4);
				if (angularSize.toFloat() < 1e-4 || planet->getPlanetType() == Planet::isComet)
					angularSize = QChar(0x2014);

				ACCelPosTreeWidgetItem* treeItem = new ACCelPosTreeWidgetItem(ui->celestialPositionsTreeWidget);
				treeItem->setText(CColumnName, planet->getNameI18n());
				treeItem->setText(CColumnRA, raStr);
				treeItem->setTextAlignment(CColumnRA, Qt::AlignRight);
				treeItem->setText(CColumnDec, decStr);
				treeItem->setTextAlignment(CColumnDec, Qt::AlignRight);
				treeItem->setText(CColumnMagnitude, QString::number(planet->getVMagnitudeWithExtinction(core), 'f', 2));
				treeItem->setTextAlignment(CColumnMagnitude, Qt::AlignRight);
				treeItem->setText(CColumnAngularSize, angularSize);
				treeItem->setTextAlignment(CColumnAngularSize, Qt::AlignRight);
				treeItem->setToolTip(CColumnAngularSize, asToolTip);
				treeItem->setText(CColumnExtra, extra);
				treeItem->setTextAlignment(CColumnExtra, Qt::AlignRight);
				treeItem->setToolTip(CColumnExtra, sToolTip);
				treeItem->setText(CColumnType, q_(planet->getPlanetTypeString()));
			}
		}
	}
	else if (celTypeId == 201)
	{
		QList<PlanetP> allMinorBodies = solarSystem->getAllMinorBodies();
		QString distanceInfo = q_("Planetocentric distance");
		if (core->getUseTopocentricCoordinates()) distanceInfo = q_("Topocentric distance");
		QString distanceUM = qc_("AU", "distance, astronomical unit");
		QString sToolTip = QString("%1, %2").arg(distanceInfo, distanceUM);
		QString asToolTip = QString("%1, %2").arg(q_("Angular size (with rings, if any)"), q_("arcmin"));
		Vec3d pos;
		foreach (const PlanetP& planet, allMinorBodies)
		{
			if ((planet->getPlanetType() == Planet::isComet && planet != core->getCurrentPlanet()) && planet->getVMagnitudeWithExtinction(core) <= mag && planet->isAboveRealHorizon(core))
			{
				pos = planet->getJ2000EquatorialPos(core);
				if (horizon)
				{
					StelUtils::rectToSphe(&ra, &dec, planet->getAltAzPosAuto(core));
					float direction = 3.; // N is zero, E is 90 degrees
					if (useSouthAzimuth)
						direction = 2.;
					ra = direction * M_PI - ra;
					if (ra > M_PI * 2)
						ra -= M_PI * 2;
					raStr = StelUtils::radToDmsStr(ra, true);
					decStr = StelUtils::radToDmsStr(dec, true);
				}
				else
				{
					StelUtils::rectToSphe(&ra, &dec, pos);
					raStr = StelUtils::radToHmsStr(ra);
					decStr = StelUtils::radToDmsStr(dec, true);
				}

				extra = QString::number(pos.length(), 'f', 5); // A.U.

				ACCelPosTreeWidgetItem* treeItem = new ACCelPosTreeWidgetItem(ui->celestialPositionsTreeWidget);
				treeItem->setText(CColumnName, planet->getNameI18n());
				treeItem->setText(CColumnRA, raStr);
				treeItem->setTextAlignment(CColumnRA, Qt::AlignRight);
				treeItem->setText(CColumnDec, decStr);
				treeItem->setTextAlignment(CColumnDec, Qt::AlignRight);
				treeItem->setText(CColumnMagnitude, QString::number(planet->getVMagnitudeWithExtinction(core), 'f', 2));
				treeItem->setTextAlignment(CColumnMagnitude, Qt::AlignRight);
				treeItem->setText(CColumnAngularSize, QChar(0x2014));
				treeItem->setTextAlignment(CColumnAngularSize, Qt::AlignRight);
				treeItem->setToolTip(CColumnAngularSize, asToolTip);
				treeItem->setText(CColumnExtra, extra);
				treeItem->setTextAlignment(CColumnExtra, Qt::AlignRight);
				treeItem->setToolTip(CColumnExtra, sToolTip);
				treeItem->setText(CColumnType, q_(planet->getPlanetTypeString()));
			}
		}
	}
	else if (celTypeId == 202)
	{
		QList<PlanetP> allMinorBodies = solarSystem->getAllMinorBodies();
		QString distanceInfo = q_("Planetocentric distance");
		if (core->getUseTopocentricCoordinates()) distanceInfo = q_("Topocentric distance");
		QString distanceUM = qc_("AU", "distance, astronomical unit");
		QString sToolTip = QString("%1, %2").arg(distanceInfo, distanceUM);
		QString asToolTip = QString("%1, %2").arg(q_("Angular size (with rings, if any)"), q_("arcmin"));
		Vec3d pos;
		foreach (const PlanetP& planet, allMinorBodies)
		{
			Planet::PlanetType ptype = planet->getPlanetType();
			if (((ptype == Planet::isAsteroid || ptype == Planet::isCubewano || ptype == Planet::isDwarfPlanet || ptype == Planet::isOCO || ptype == Planet::isPlutino || ptype == Planet::isSDO || ptype == Planet::isSednoid) && planet != core->getCurrentPlanet())
			      && planet->getVMagnitudeWithExtinction(core) <= mag && planet->isAboveRealHorizon(core))
			{
				pos = planet->getJ2000EquatorialPos(core);
				if (horizon)
				{
					StelUtils::rectToSphe(&ra, &dec, planet->getAltAzPosAuto(core));
					float direction = 3.; // N is zero, E is 90 degrees
					if (useSouthAzimuth)
						direction = 2.;
					ra = direction * M_PI - ra;
					if (ra > M_PI * 2)
						ra -= M_PI * 2;
					raStr = StelUtils::radToDmsStr(ra, true);
					decStr = StelUtils::radToDmsStr(dec, true);
				}
				else
				{
					StelUtils::rectToSphe(&ra, &dec, pos);
					raStr = StelUtils::radToHmsStr(ra);
					decStr = StelUtils::radToDmsStr(dec, true);
				}

				extra = QString::number(pos.length(), 'f', 5); // A.U.

				// Convert to arcseconds the angular size of Solar system object (with rings, if any)
				angularSize = QString::number(planet->getAngularSize(core) * 120.f, 'f', 4);
				if (angularSize.toFloat() < 1e-4)
					angularSize = QChar(0x2014);

				ACCelPosTreeWidgetItem* treeItem = new ACCelPosTreeWidgetItem(ui->celestialPositionsTreeWidget);
				treeItem->setText(CColumnName, planet->getNameI18n());
				treeItem->setText(CColumnRA, raStr);
				treeItem->setTextAlignment(CColumnRA, Qt::AlignRight);
				treeItem->setText(CColumnDec, decStr);
				treeItem->setTextAlignment(CColumnDec, Qt::AlignRight);
				treeItem->setText(CColumnMagnitude, QString::number(planet->getVMagnitudeWithExtinction(core), 'f', 2));
				treeItem->setTextAlignment(CColumnMagnitude, Qt::AlignRight);
				treeItem->setText(CColumnAngularSize, angularSize);
				treeItem->setTextAlignment(CColumnAngularSize, Qt::AlignRight);
				treeItem->setToolTip(CColumnAngularSize, asToolTip);
				treeItem->setText(CColumnExtra, extra);
				treeItem->setTextAlignment(CColumnExtra, Qt::AlignRight);
				treeItem->setToolTip(CColumnExtra, sToolTip);
				treeItem->setText(CColumnType, q_(planet->getPlanetTypeString()));
			}
		}
	}
	else if (celTypeId == 203)
	{
		QList<PlanetP> allPlanets = solarSystem->getAllPlanets();
		QString distanceInfo = q_("Planetocentric distance");
		if (core->getUseTopocentricCoordinates())
			distanceInfo = q_("Topocentric distance");
		QString distanceUM = qc_("AU", "distance, astronomical unit");
		QString sToolTip = QString("%1, %2").arg(distanceInfo, distanceUM);
		QString asToolTip = QString("%1, %2").arg(q_("Angular size (with rings, if any)"), q_("arcmin"));
		Vec3d pos;
		foreach (const PlanetP& planet, allPlanets)
		{
			if ((planet->getPlanetType() == Planet::isPlanet && planet != core->getCurrentPlanet()) && planet->getVMagnitudeWithExtinction(core) <= mag && planet->isAboveRealHorizon(core))
			{
				pos = planet->getJ2000EquatorialPos(core);
				if (horizon)
				{
					StelUtils::rectToSphe(&ra, &dec, planet->getAltAzPosAuto(core));
					float direction = 3.; // N is zero, E is 90 degrees
					if (useSouthAzimuth)
						direction = 2.;
					ra = direction * M_PI - ra;
					if (ra > M_PI * 2)
						ra -= M_PI * 2;
					raStr = StelUtils::radToDmsStr(ra, true);
					decStr = StelUtils::radToDmsStr(dec, true);
				}
				else
				{
					StelUtils::rectToSphe(&ra, &dec, pos);
					raStr = StelUtils::radToHmsStr(ra);
					decStr = StelUtils::radToDmsStr(dec, true);
				}

				extra = QString::number(pos.length(), 'f', 5); // A.U.

				// Convert to arcseconds the angular size of Solar system object (with rings, if any)
				angularSize = QString::number(planet->getAngularSize(core) * 120.f, 'f', 4);
				if (angularSize.toFloat() < 1e-4)
					angularSize = QChar(0x2014);

				ACCelPosTreeWidgetItem* treeItem = new ACCelPosTreeWidgetItem(ui->celestialPositionsTreeWidget);
				treeItem->setText(CColumnName, planet->getNameI18n());
				treeItem->setText(CColumnRA, raStr);
				treeItem->setTextAlignment(CColumnRA, Qt::AlignRight);
				treeItem->setText(CColumnDec, decStr);
				treeItem->setTextAlignment(CColumnDec, Qt::AlignRight);
				treeItem->setText(CColumnMagnitude, QString::number(planet->getVMagnitudeWithExtinction(core), 'f', 2));
				treeItem->setTextAlignment(CColumnMagnitude, Qt::AlignRight);
				treeItem->setText(CColumnAngularSize, angularSize);
				treeItem->setTextAlignment(CColumnAngularSize, Qt::AlignRight);
				treeItem->setToolTip(CColumnAngularSize, asToolTip);
				treeItem->setText(CColumnExtra, extra);
				treeItem->setTextAlignment(CColumnExtra, Qt::AlignRight);
				treeItem->setToolTip(CColumnExtra, sToolTip);
				treeItem->setText(CColumnType, q_(planet->getPlanetTypeString()));
			}
		}
	}
	else
	{
		// stars
		QString sType = q_("star");
		QString sToolTip = "";
		float wdsSep;
		QList<StelACStarData> celestialObjects;
		if (celTypeId == 170)
		{
			// double stars
			celestialObjects = starMgr->getHipparcosDoubleStars();
			sType = q_("double star");
		}
		else if (celTypeId == 171)
		{
			// variable stars
			celestialObjects = starMgr->getHipparcosVariableStars();
			sType = q_("variable star");
		}
		else
		{
			// stars with high proper motion
			celestialObjects = starMgr->getHipparcosHighPMStars();
			sType = q_("star with high proper motion");
		}

		foreach (const StelACStarData& star, celestialObjects)
		{
			StelObjectP obj = star.firstKey();
			if (obj->getVMagnitudeWithExtinction(core) <= mag && obj->isAboveRealHorizon(core))
			{
				if (horizon)
				{
					StelUtils::rectToSphe(&ra, &dec, obj->getAltAzPosAuto(core));
					float direction = 3.; // N is zero, E is 90 degrees
					if (useSouthAzimuth)
						direction = 2.;
					ra = direction * M_PI - ra;
					if (ra > M_PI * 2)
						ra -= M_PI * 2;
					raStr = StelUtils::radToDmsStr(ra, true);
					decStr = StelUtils::radToDmsStr(dec, true);
				}
				else
				{
					StelUtils::rectToSphe(&ra, &dec, obj->getJ2000EquatorialPos(core));
					raStr = StelUtils::radToHmsStr(ra);
					decStr = StelUtils::radToDmsStr(dec, true);
				}

				if (celTypeId == 170) // double stars
				{
					wdsSep = star.value(obj);
					extra = QString::number(wdsSep, 'f', 3); // arcseconds
					sToolTip = StelUtils::decDegToDmsStr(wdsSep / 3600.f);
				}
				else if (celTypeId == 171) // variable stars
				{
					if (star.value(obj) > 0.f)
						extra = QString::number(star.value(obj), 'f', 5); // days
					else
						extra = QChar(0x2014); // dash
				}
				else												  // stars with high proper motion
					extra = QString::number(star.value(obj), 'f', 5); // "/yr

				ACCelPosTreeWidgetItem* treeItem = new ACCelPosTreeWidgetItem(ui->celestialPositionsTreeWidget);
				treeItem->setText(CColumnName, obj->getNameI18n());
				treeItem->setText(CColumnRA, raStr);
				treeItem->setTextAlignment(CColumnRA, Qt::AlignRight);
				treeItem->setText(CColumnDec, decStr);
				treeItem->setTextAlignment(CColumnDec, Qt::AlignRight);
				treeItem->setText(CColumnMagnitude, QString::number(obj->getVMagnitudeWithExtinction(core), 'f', 2));
				treeItem->setTextAlignment(CColumnMagnitude, Qt::AlignRight);
				treeItem->setText(CColumnAngularSize, QChar(0x2014)); // No angular size of stars!
				treeItem->setToolTip(CColumnAngularSize, "");
				treeItem->setTextAlignment(CColumnAngularSize, Qt::AlignRight);
				treeItem->setText(CColumnExtra, extra);
				treeItem->setTextAlignment(CColumnExtra, Qt::AlignRight);
				treeItem->setToolTip(CColumnExtra, sToolTip);
				treeItem->setText(CColumnType, sType);
			}
		}
	}

	// adjust the column width
	for (int i = 0; i < CColumnCount; ++i)
	{
		ui->celestialPositionsTreeWidget->resizeColumnToContents(i);
	}

	// sort-by-name
	ui->celestialPositionsTreeWidget->sortItems(CColumnName, Qt::AscendingOrder);
}

void AstroCalcDialog::saveCelestialPositions()
{
	QString filter = q_("CSV (Comma delimited)");
	filter.append(" (*.csv)");
	QString filePath = QFileDialog::getSaveFileName(0, q_("Save celestial positions of objects as..."), QDir::homePath() + "/positions.csv", filter);
	QFile celPos(filePath);
	if (!celPos.open(QFile::WriteOnly | QFile::Truncate))
	{
		qWarning() << "AstroCalc: Unable to open file" << QDir::toNativeSeparators(filePath);
		return;
	}

	QTextStream celPosList(&celPos);
	celPosList.setCodec("UTF-8");

	int count = ui->celestialPositionsTreeWidget->topLevelItemCount();
	int columns = positionsHeader.size();

	for (int i = 0; i < columns; i++)
	{
		QString h = positionsHeader.at(i).trimmed();
		if (h.contains(","))
			celPosList << QString("\"%1\"").arg(h);
		else
			celPosList << h;

		if (i < columns - 1)
			celPosList << delimiter;
		else
			celPosList << acEndl;
	}

	for (int i = 0; i < count; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			celPosList << ui->celestialPositionsTreeWidget->topLevelItem(i)->text(j);
			if (j < columns - 1)
				celPosList << delimiter;
			else
				celPosList << acEndl;
		}
	}

	celPos.close();
}

void AstroCalcDialog::selectCurrentCelestialPosition(const QModelIndex& modelIndex)
{
	// Find the object
	QString nameI18n = modelIndex.sibling(modelIndex.row(), CColumnName).data().toString();

	QStringList list = nameI18n.split("(");
	if (list.count() > 0 && nameI18n.lastIndexOf("(") != 0 && nameI18n.lastIndexOf("/") < 0)
		nameI18n = list.at(0).trimmed();

	if (objectMgr->findAndSelectI18n(nameI18n) || objectMgr->findAndSelect(nameI18n))
	{
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			mvMgr->moveToObject(newSelected[0], mvMgr->getAutoMoveDuration());
			mvMgr->setFlagTracking(true);
		}
	}
}

void AstroCalcDialog::selectCurrentEphemeride(const QModelIndex& modelIndex)
{
	// Find the object
	QString name = ui->celestialBodyComboBox->currentData().toString();
	double JD = modelIndex.sibling(modelIndex.row(), EphemerisJD).data().toDouble();

	if (objectMgr->findAndSelectI18n(name) || objectMgr->findAndSelect(name))
	{
		core->setJD(JD);
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			// Can't point to home planet
			if (newSelected[0]->getEnglishName() != core->getCurrentLocation().planetName)
			{
				mvMgr->moveToObject(newSelected[0], mvMgr->getAutoMoveDuration());
				mvMgr->setFlagTracking(true);
			}
			else
			{
				GETSTELMODULE(StelObjectMgr)->unSelect();
			}
		}
	}
}

void AstroCalcDialog::setEphemerisHeaderNames()
{
	bool horizon = ui->ephemerisHorizontalCoordinatesCheckBox->isChecked();

	ephemerisHeader.clear();
	ephemerisHeader << q_("Date and Time");
	ephemerisHeader << q_("Julian Day");
	if (horizon)
	{
		// TRANSLATORS: azimuth
		ephemerisHeader << q_("Azimuth");
		// TRANSLATORS: altitude
		ephemerisHeader << q_("Altitude");
	}
	else
	{
		// TRANSLATORS: right ascension
		ephemerisHeader << q_("RA (J2000)");
		// TRANSLATORS: declination
		ephemerisHeader << q_("Dec (J2000)");
	}
	// TRANSLATORS: magnitude
	ephemerisHeader << q_("mag");
	// TRANSLATORS: phase
	ephemerisHeader << q_("phase");
	// TRANSLATORS: distance, AU
	ephemerisHeader << QString("%1, %2").arg(q_("dist."), qc_("AU", "distance, astronomical unit"));
	// TRANSLATORS: elongation
	ephemerisHeader << q_("elong.");
	ui->ephemerisTreeWidget->setHeaderLabels(ephemerisHeader);

	// adjust the column width
	for (int i = 0; i < EphemerisCount; ++i)
	{
		ui->ephemerisTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListEphemeris()
{
	ui->ephemerisTreeWidget->clear();
	ui->ephemerisTreeWidget->setColumnCount(EphemerisCount);
	setEphemerisHeaderNames();
	ui->ephemerisTreeWidget->header()->setSectionsMovable(false);
}

void AstroCalcDialog::reGenerateEphemeris()
{
	if (EphemerisListCoords.size() > 0)
		generateEphemeris(); // Update list of ephemeris
	else
		initListEphemeris(); // Just update headers
}

void AstroCalcDialog::generateEphemeris()
{
	float currentStep, ra, dec;
	Vec3d observerHelioPos;
	QString currentPlanet = ui->celestialBodyComboBox->currentData().toString();
	QString distanceInfo = q_("Planetocentric distance");
	if (core->getUseTopocentricCoordinates())
		distanceInfo = q_("Topocentric distance");
	QString distanceUM = qc_("AU", "distance, astronomical unit");

	QString elongStr = "", phaseStr = "";
	bool horizon = ui->ephemerisHorizontalCoordinatesCheckBox->isChecked();
	bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();

	initListEphemeris();

	double solarDay = 1.0;
	double siderealDay = 1.0;
	const PlanetP& cplanet = core->getCurrentPlanet();
	if (!cplanet->getEnglishName().contains("observer", Qt::CaseInsensitive))
	{
		solarDay = cplanet->getMeanSolarDay();
		siderealDay = cplanet->getSiderealDay();
	}

	switch (ui->ephemerisStepComboBox->currentData().toInt())
	{
		case 1:
			currentStep = 10 * StelCore::JD_MINUTE;
			break;
		case 2:
			currentStep = 30 * StelCore::JD_MINUTE;
			break;
		case 3:
			currentStep = StelCore::JD_HOUR;
			break;
		case 4:
			currentStep = 6 * StelCore::JD_HOUR;
			break;
		case 5:
			currentStep = 12 * StelCore::JD_HOUR;
			break;
		case 6:
			currentStep = solarDay;
			break;
		case 7:
			currentStep = 5 * solarDay;
			break;
		case 8:
			currentStep = 10 * solarDay;
			break;
		case 9:
			currentStep = 15 * solarDay;
			break;
		case 10:
			currentStep = 30 * solarDay;
			break;
		case 11:
			currentStep = 60 * solarDay;
			break;
		case 12:
			currentStep = StelCore::JD_DAY;
			break;
		case 13:
			currentStep = 5 * StelCore::JD_DAY;
			break;
		case 14:
			currentStep = 10 * StelCore::JD_DAY;
			break;
		case 15:
			currentStep = 15 * StelCore::JD_DAY;
			break;
		case 16:
			currentStep = 30 * StelCore::JD_DAY;
			break;
		case 17:
			currentStep = 60 * StelCore::JD_DAY;
			break;
		case 18:
			currentStep = siderealDay;
			break;
		case 19:
			currentStep = 5 * siderealDay;
			break;
		case 20:
			currentStep = 10 * siderealDay;
			break;
		case 21:
			currentStep = 15 * siderealDay;
			break;
		case 22:
			currentStep = 30 * siderealDay;
			break;
		case 23:
			currentStep = 60 * siderealDay;
			break;
		case 24:
			currentStep = 100 * solarDay;
			break;
		case 25:
			currentStep = 100 * siderealDay;
			break;
		case 26:
			currentStep = 100 * StelCore::JD_DAY;
			break;
		default:
			currentStep = solarDay;
			break;
	}

	PlanetP obj = solarSystem->searchByEnglishName(currentPlanet);
	if (obj)
	{
		double currentJD = core->getJD(); // save current JD
		double firstJD = StelUtils::qDateTimeToJd(ui->dateFromDateTimeEdit->dateTime());
		firstJD = firstJD - core->getUTCOffset(firstJD) / 24;
		int elements = (int)((StelUtils::qDateTimeToJd(ui->dateToDateTimeEdit->dateTime()) - firstJD) / currentStep);
		EphemerisListCoords.clear();
		EphemerisListCoords.reserve(elements);
		EphemerisListDates.clear();
		EphemerisListDates.reserve(elements);
		EphemerisListMagnitudes.clear();
		EphemerisListMagnitudes.reserve(elements);
		bool withTime = false;
		QString dash = QChar(0x2014); // dash
		if (currentStep < StelCore::JD_DAY)
			withTime = true;

		if (obj == solarSystem->getSun())
		{
			phaseStr = dash;
			elongStr = dash;
		}

		Vec3d pos;
		QString raStr = "", decStr = "";

		for (int i = 0; i < elements; i++)
		{
			double JD = firstJD + i * currentStep;
			core->setJD(JD);
			core->update(0); // force update to get new coordinates

			if (horizon)
			{
				pos = obj->getAltAzPosAuto(core);
				StelUtils::rectToSphe(&ra, &dec, pos);
				float direction = 3.; // N is zero, E is 90 degrees
				if (useSouthAzimuth)
					direction = 2.;
				ra = direction * M_PI - ra;
				if (ra > M_PI * 2)
					ra -= M_PI * 2;
				raStr = StelUtils::radToDmsStr(ra, true);
				decStr = StelUtils::radToDmsStr(dec, true);
			}
			else
			{
				pos = obj->getJ2000EquatorialPos(core);
				StelUtils::rectToSphe(&ra, &dec, pos);
				raStr = StelUtils::radToHmsStr(ra);
				decStr = StelUtils::radToDmsStr(dec, true);
			}

			EphemerisListCoords.append(pos);
			if (withTime)
				EphemerisListDates.append(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD)));
			else
				EphemerisListDates.append(localeMgr->getPrintableDateLocal(JD));
			EphemerisListMagnitudes.append(obj->getVMagnitudeWithExtinction(core));
			StelUtils::rectToSphe(&ra, &dec, pos);

			observerHelioPos = core->getObserverHeliocentricEclipticPos();

			if (phaseStr != dash)
				phaseStr = QString("%1%").arg(QString::number(obj->getPhase(observerHelioPos) * 100, 'f', 2));

			if (elongStr != dash)
				elongStr = StelUtils::radToDmsStr(obj->getElongation(observerHelioPos), true);

			ACEphemTreeWidgetItem* treeItem = new ACEphemTreeWidgetItem(ui->ephemerisTreeWidget);
			// local date and time
			treeItem->setText(EphemerisDate,
			  QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD)));
			treeItem->setText(EphemerisJD, QString::number(JD, 'f', 5));
			treeItem->setText(EphemerisRA, raStr);
			treeItem->setTextAlignment(EphemerisRA, Qt::AlignRight);
			treeItem->setText(EphemerisDec, decStr);
			treeItem->setTextAlignment(EphemerisDec, Qt::AlignRight);
			treeItem->setText(EphemerisMagnitude, QString::number(obj->getVMagnitudeWithExtinction(core), 'f', 2));
			treeItem->setTextAlignment(EphemerisMagnitude, Qt::AlignRight);
			treeItem->setText(EphemerisPhase, phaseStr);
			treeItem->setTextAlignment(EphemerisPhase, Qt::AlignRight);
			treeItem->setText(EphemerisDistance, QString::number(obj->getJ2000EquatorialPos(core).length(), 'f', 6));
			treeItem->setTextAlignment(EphemerisDistance, Qt::AlignRight);
			treeItem->setToolTip(EphemerisDistance, QString("%1, %2").arg(distanceInfo, distanceUM));
			treeItem->setText(EphemerisElongation, elongStr);
			treeItem->setTextAlignment(EphemerisElongation, Qt::AlignRight);
		}
		core->setJD(currentJD); // restore time
	}

	// adjust the column width
	for (int i = 0; i < EphemerisCount; ++i)
	{
		ui->ephemerisTreeWidget->resizeColumnToContents(i);
	}

	// sort-by-date
	ui->ephemerisTreeWidget->sortItems(EphemerisDate, Qt::AscendingOrder);
}

void AstroCalcDialog::saveEphemeris()
{
	QString filter = q_("CSV (Comma delimited)");
	filter.append(" (*.csv)");
	QString filePath = QFileDialog::getSaveFileName(0, q_("Save calculated ephemerides as..."), QDir::homePath() + "/ephemeris.csv", filter);
	QFile ephem(filePath);
	if (!ephem.open(QFile::WriteOnly | QFile::Truncate))
	{
		qWarning() << "AstroCalc: Unable to open file" << QDir::toNativeSeparators(filePath);
		return;
	}

	QTextStream ephemList(&ephem);
	ephemList.setCodec("UTF-8");

	int count = ui->ephemerisTreeWidget->topLevelItemCount();
	int columns = ephemerisHeader.size();

	for (int i = 0; i < columns; i++)
	{
		QString h = ephemerisHeader.at(i).trimmed();
		if (h.contains(","))
			ephemList << QString("\"%1\"").arg(h);
		else
			ephemList << h;

		if (i < columns - 1)
			ephemList << delimiter;
		else
			ephemList << acEndl;
	}

	for (int i = 0; i < count; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			ephemList << ui->ephemerisTreeWidget->topLevelItem(i)->text(j);
			if (j < columns - 1)
				ephemList << delimiter;
			else
				ephemList << acEndl;
		}
	}

	ephem.close();
}

void AstroCalcDialog::cleanupEphemeris()
{
	EphemerisListCoords.clear();
	ui->ephemerisTreeWidget->clear();
}

void AstroCalcDialog::populateCelestialBodyList()
{
	Q_ASSERT(ui->celestialBodyComboBox);
	Q_ASSERT(ui->graphsCelestialBodyComboBox);
	Q_ASSERT(ui->firstCelestialBodyComboBox);
	Q_ASSERT(ui->secondCelestialBodyComboBox);

	QComboBox* planets = ui->celestialBodyComboBox;
	QComboBox* graphsp = ui->graphsCelestialBodyComboBox;
	QComboBox* firstCB = ui->firstCelestialBodyComboBox;
	QComboBox* secondCB = ui->secondCelestialBodyComboBox;

	QList<PlanetP> ss = solarSystem->getAllPlanets();

	// Save the current selection to be restored later
	planets->blockSignals(true);
	int indexP = planets->currentIndex();
	QVariant selectedPlanetId = planets->itemData(indexP);
	planets->clear();

	graphsp->blockSignals(true);
	int indexG = graphsp->currentIndex();
	QVariant selectedGraphsPId = graphsp->itemData(indexG);
	graphsp->clear();

	firstCB->blockSignals(true);
	int indexFCB = firstCB->currentIndex();
	QVariant selectedFirstCelestialBodyId = firstCB->itemData(indexFCB);
	firstCB->clear();

	secondCB->blockSignals(true);
	int indexSCB = secondCB->currentIndex();
	QVariant selectedSecondCelestialBodyId = secondCB->itemData(indexSCB);
	secondCB->clear();

	// For each planet, display the localized name and store the original as user
	// data. Unfortunately, there's no other way to do this than with a cycle.
	foreach (const PlanetP& p, ss)
	{
		if (!p->getEnglishName().contains("Observer", Qt::CaseInsensitive))
		{
			if (p->getEnglishName() != core->getCurrentPlanet()->getEnglishName())
			{
				planets->addItem(p->getNameI18n(), p->getEnglishName());
				graphsp->addItem(p->getNameI18n(), p->getEnglishName());
			}
			firstCB->addItem(p->getNameI18n(), p->getEnglishName());
			secondCB->addItem(p->getNameI18n(), p->getEnglishName());
		}
	}
	// Restore the selection
	indexP = planets->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexP < 0)
		indexP = planets->findData(conf->value("astrocalc/ephemeris_celestial_body", "Moon").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	planets->setCurrentIndex(indexP);
	planets->model()->sort(0);

	indexG = graphsp->findData(selectedGraphsPId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexG < 0)
		indexG = graphsp->findData(conf->value("astrocalc/graphs_celestial_body", "Moon").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	graphsp->setCurrentIndex(indexG);
	graphsp->model()->sort(0);

	indexFCB = firstCB->findData(selectedFirstCelestialBodyId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexFCB < 0)
		indexFCB = firstCB->findData(conf->value("astrocalc/first_celestial_body", "Sun").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	firstCB->setCurrentIndex(indexFCB);
	firstCB->model()->sort(0);

	indexSCB = secondCB->findData(selectedSecondCelestialBodyId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexSCB < 0)
		indexSCB = secondCB->findData(conf->value("astrocalc/second_celestial_body", "Earth").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	secondCB->setCurrentIndex(indexSCB);
	secondCB->model()->sort(0);

	planets->blockSignals(false);
	graphsp->blockSignals(false);
	firstCB->blockSignals(false);
	secondCB->blockSignals(false);
}

void AstroCalcDialog::saveEphemerisCelestialBody(int index)
{
	Q_ASSERT(ui->celestialBodyComboBox);
	QComboBox* planets = ui->celestialBodyComboBox;
	conf->setValue("astrocalc/ephemeris_celestial_body", planets->itemData(index).toString());
}

void AstroCalcDialog::saveGraphsCelestialBody(int index)
{
	Q_ASSERT(ui->graphsCelestialBodyComboBox);
	QComboBox* planets = ui->graphsCelestialBodyComboBox;
	conf->setValue("astrocalc/graphs_celestial_body", planets->itemData(index).toString());
}

void AstroCalcDialog::saveGraphsFirstId(int index)
{
	Q_ASSERT(ui->graphsFirstComboBox);
	conf->setValue("astrocalc/graphs_first_id", ui->graphsFirstComboBox->itemData(index).toInt());
}

void AstroCalcDialog::saveGraphsSecondId(int index)
{
	Q_ASSERT(ui->graphsSecondComboBox);
	conf->setValue("astrocalc/graphs_second_id", ui->graphsSecondComboBox->itemData(index).toInt());
}

void AstroCalcDialog::populateEphemerisTimeStepsList()
{
	Q_ASSERT(ui->ephemerisStepComboBox);

	QComboBox* steps = ui->ephemerisStepComboBox;
	steps->blockSignals(true);
	int index = steps->currentIndex();
	QVariant selectedStepId = steps->itemData(index);

	steps->clear();
	steps->addItem(q_("10 minutes"), "1");
	steps->addItem(q_("30 minutes"), "2");
	steps->addItem(q_("1 hour"), "3");
	steps->addItem(q_("6 hours"), "4");
	steps->addItem(q_("12 hours"), "5");
	steps->addItem(q_("1 solar day"), "6");
	steps->addItem(q_("5 solar days"), "7");
	steps->addItem(q_("10 solar days"), "8");
	steps->addItem(q_("15 solar days"), "9");
	steps->addItem(q_("30 solar days"), "10");
	steps->addItem(q_("60 solar days"), "11");
	steps->addItem(q_("100 solar days"), "24");
	steps->addItem(q_("1 sidereal day"), "18");
	steps->addItem(q_("5 sidereal days"), "19");
	steps->addItem(q_("10 sidereal days"), "20");
	steps->addItem(q_("15 sidereal days"), "21");
	steps->addItem(q_("30 sidereal days"), "22");
	steps->addItem(q_("60 sidereal days"), "23");
	steps->addItem(q_("100 sidereal days"), "25");
	steps->addItem(q_("1 Julian day"), "12");
	steps->addItem(q_("5 Julian days"), "13");
	steps->addItem(q_("10 Julian days"), "14");
	steps->addItem(q_("15 Julian days"), "15");
	steps->addItem(q_("30 Julian days"), "16");
	steps->addItem(q_("60 Julian days"), "17");
	steps->addItem(q_("100 Julian days"), "26");

	index = steps->findData(selectedStepId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index < 0)
	{
		// default step: one day
		index = steps->findData(conf->value("astrocalc/ephemeris_time_step", "6").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	steps->setCurrentIndex(index);
	steps->blockSignals(false);
}

void AstroCalcDialog::saveEphemerisTimeStep(int index)
{
	Q_ASSERT(ui->ephemerisStepComboBox);
	QComboBox* steps = ui->ephemerisStepComboBox;
	conf->setValue("astrocalc/ephemeris_time_step", steps->itemData(index).toInt());
}

void AstroCalcDialog::populateMajorPlanetList()
{
	Q_ASSERT(ui->object1ComboBox); // object 1 is always major planet

	QComboBox* majorPlanet = ui->object1ComboBox;
	QList<PlanetP> planets = solarSystem->getAllPlanets();
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	// Save the current selection to be restored later
	majorPlanet->blockSignals(true);
	int index = majorPlanet->currentIndex();
	QVariant selectedPlanetId = majorPlanet->itemData(index);
	majorPlanet->clear();
	// For each planet, display the localized name and store the original as user
	// data. Unfortunately, there's no other way to do this than with a cycle.
	foreach (const PlanetP& planet, planets)
	{
		// major planets and the Sun
		if ((planet->getPlanetType() == Planet::isPlanet || planet->getPlanetType() == Planet::isStar) && planet->getEnglishName() != core->getCurrentPlanet()->getEnglishName())
			majorPlanet->addItem(trans.qtranslate(planet->getNameI18n()), planet->getEnglishName());

		// moons of the current planet
		if (planet->getPlanetType() == Planet::isMoon && planet->getEnglishName() != core->getCurrentPlanet()->getEnglishName() && planet->getParent() == core->getCurrentPlanet())
			majorPlanet->addItem(trans.qtranslate(planet->getNameI18n()), planet->getEnglishName());
	}
	// Restore the selection
	index = majorPlanet->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index < 0)
		index = majorPlanet->findData(conf->value("astrocalc/phenomena_celestial_body", "Venus").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	majorPlanet->setCurrentIndex(index);
	majorPlanet->model()->sort(0);
	majorPlanet->blockSignals(false);
}

void AstroCalcDialog::savePhenomenaCelestialBody(int index)
{
	Q_ASSERT(ui->object1ComboBox);
	QComboBox* planets = ui->object1ComboBox;
	conf->setValue("astrocalc/phenomena_celestial_body", planets->itemData(index).toString());
}

void AstroCalcDialog::populateGroupCelestialBodyList()
{
	Q_ASSERT(ui->object2ComboBox);

	QComboBox* groups = ui->object2ComboBox;
	groups->blockSignals(true);
	int index = groups->currentIndex();
	QVariant selectedGroupId = groups->itemData(index);

	QString brLimit = QString::number(brightLimit, 'f', 1);
	groups->clear();
	groups->addItem(q_("Solar system"), "0");
	groups->addItem(q_("Planets"), "1");
	groups->addItem(q_("Asteroids"), "2");
	groups->addItem(q_("Plutinos"), "3");
	groups->addItem(q_("Comets"), "4");
	groups->addItem(q_("Dwarf planets"), "5");
	groups->addItem(q_("Cubewanos"), "6");
	groups->addItem(q_("Scattered disc objects"), "7");
	groups->addItem(q_("Oort cloud objects"), "8");
	groups->addItem(q_("Sednoids"), "9");
	groups->addItem(q_("Bright stars (<%1 mag)").arg(QString::number(brightLimit - 5.0f, 'f', 1)), "10");
	groups->addItem(q_("Bright double stars (<%1 mag)").arg(QString::number(brightLimit - 5.0f, 'f', 1)), "11");
	groups->addItem(q_("Bright variable stars (<%1 mag)").arg(QString::number(brightLimit - 5.0f, 'f', 1)), "12");
	groups->addItem(q_("Bright star clusters (<%1 mag)").arg(brLimit), "13");
	groups->addItem(q_("Planetary nebulae"), "14");
	groups->addItem(q_("Bright nebulae (<%1 mag)").arg(brLimit), "15");
	groups->addItem(q_("Dark nebulae"), "16");
	groups->addItem(q_("Bright galaxies (<%1 mag)").arg(brLimit), "17");
	groups->addItem(q_("Symbiotic stars"), "18");
	groups->addItem(q_("Emission-line stars"), "19");

	index = groups->findData(selectedGroupId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index < 0)
		index = groups->findData(conf->value("astrocalc/phenomena_celestial_group", "1").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	groups->setCurrentIndex(index);
	groups->model()->sort(0);
	groups->blockSignals(false);
}

void AstroCalcDialog::savePhenomenaCelestialGroup(int index)
{
	Q_ASSERT(ui->object2ComboBox);
	QComboBox* group = ui->object2ComboBox;
	conf->setValue("astrocalc/phenomena_celestial_group", group->itemData(index).toInt());
}

void AstroCalcDialog::cleanupPhenomena()
{
	ui->phenomenaTreeWidget->clear();
}

void AstroCalcDialog::savePhenomenaOppositionFlag(bool b)
{
	conf->setValue("astrocalc/flag_phenomena_opposition", b);
}

void AstroCalcDialog::savePhenomenaAngularSeparation(double v)
{
	conf->setValue("astrocalc/phenomena_angular_separation", QString::number(v, 'f', 5));
}

void AstroCalcDialog::drawAltVsTimeDiagram()
{
	// Avoid crash!
	if (core->getCurrentPlanet()->getEnglishName().contains("->")) // We are on the spaceship!
		return;

	// special case - plot the graph when tab is visible
	if (!plotAltVsTime)
		return;

	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();

	if (!selectedObjects.isEmpty())
	{
		// X axis - time; Y axis - altitude
		QList<double> aX, aY, sX, sY, sYn, sYa, sYc, mX, mY;
		QVector<double> xs, ys, ysn, ysa, ysc, xm, ym;

		StelObjectP selectedObject = selectedObjects[0];
		bool onEarth = core->getCurrentPlanet()==solarSystem->getEarth();

		double currentJD = core->getJD();
		double noon = (int)currentJD;
		double az, alt, deg, ltime, JD;
		bool sign;

		double shift = core->getUTCOffset(currentJD) / 24.0;
		double xMaxY = -100.f;
		int step = 180;
		int limit = 485;
		bool isSatellite = false;
		if (selectedObject->getType() == "Satellite") // Reduce accuracy for satellites
		{
			limit = 121;
			step = 720;
			isSatellite = true;
		}
		for (int i = -5; i <= limit; i++) // 24 hours + 15 minutes in both directions
		{
			// A new point on the graph every 3 minutes with shift to right 12 hours
			// to get midnight at the center of diagram (i.e. accuracy is 3 minutes)
			ltime = i * step + 43200;
			aX.append(ltime);
			JD = noon + ltime / 86400 - shift - 0.5;
			core->setJD(JD);
			if (isSatellite)
			{
#ifdef USE_STATIC_PLUGIN_SATELLITES
				GETSTELMODULE(Satellites)->update(0.0); // force update to avoid caching! WTF???
#endif
			}
			else
				core->update(0.0);

			StelUtils::rectToSphe(&az, &alt, selectedObject->getAltAzPosAuto(core));
			StelUtils::radToDecDeg(alt, sign, deg);
			if (!sign) deg *= -1;
			aY.append(deg);
			if (deg > xMaxY)
			{
				xMaxY = deg;
				transitX = ltime;
			}
		}

		if (plotAltVsTimeSun)
		{
			PlanetP sun = solarSystem->getSun();			
			for (int i = -1; i <= 25; i++)
			{
				ltime = i * 3600 + 43200;
				sX.append(ltime);
				JD = noon + ltime / 86400 - shift - 0.5;
				core->setJD(JD);				 
				core->update(0.0);
				StelUtils::rectToSphe(&az, &alt, sun->getAltAzPosAuto(core));
				StelUtils::radToDecDeg(alt, sign, deg);
				if (!sign) deg *= -1;
				sY.append(deg);
				sYc.append(deg + 6);
				sYn.append(deg + 12);
				sYa.append(deg + 18);
			}
		}

		if (plotAltVsTimeMoon && onEarth)
		{
			PlanetP moon = solarSystem->getMoon();			
			for (int i = -1; i <= 25; i++)
			{
				ltime = i * 3600 + 43200;
				mX.append(ltime);
				JD = noon + ltime / 86400 - shift - 0.5;
				core->setJD(JD);
				core->update(0.0);
				StelUtils::rectToSphe(&az, &alt, moon->getAltAzPosAuto(core));
				StelUtils::radToDecDeg(alt, sign, deg);
				if (!sign) deg *= -1;
				mY.append(deg);
			}
		}

		core->setJD(currentJD);

		QVector<double> x = aX.toVector(), y = aY.toVector();
		double minYa = aY.first();
		double maxYa = aY.first();

		foreach (double temp, aY)
		{
			if (maxYa < temp) maxYa = temp;
			if (minYa > temp) minYa = temp;
		}

		minY = minYa - 2.0;
		maxY = maxYa + 2.0;

		// additional data: Sun + Twilight
		if (plotAltVsTimeSun)
		{
			xs = sX.toVector();
			ys = sY.toVector();
			ysc = sYc.toVector();
			ysn = sYn.toVector();
			ysa = sYa.toVector();
			double minYs = sY.first();
			double maxYs = sY.first();

			foreach (double temp, sY)
			{
				if (maxYs < temp) maxYs = temp;
				if (minYs > temp) minYs = temp;
			}

			minY =  (minY < minYs - 2.0) ? minY : minYs - 2.f;
			maxY = (maxY > maxYs + 2.0) ? maxY : maxYs + 2.0;
		}

		// additional data: Moon
		if (plotAltVsTimeMoon && onEarth)
		{
			xm = mX.toVector();
			ym = mY.toVector();
			double minYm = mY.first();
			double maxYm = mY.first();

			foreach (double temp, mY)
			{
				if (maxYm < temp) maxYm = temp;
				if (minYm > temp) minYm = temp;
			}

			minY =  (minY < minYm - 2.0) ? minY : minYm - 2.f;
			maxY = (maxY > maxYm + 2.0) ? maxY : maxYm + 2.0;
		}

		if (plotAltVsTimePositive && minY<0.f)
			minY = 0;

		prepareAxesAndGraph();
		drawCurrentTimeDiagram();

		QString name = selectedObject->getNameI18n();
		if (name.isEmpty() && selectedObject->getType() == "Nebula")
			name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();

		drawTransitTimeDiagram();

		ui->altVsTimePlot->graph(0)->setData(x, y);
		ui->altVsTimePlot->graph(0)->setName(name);
		if (plotAltVsTimeSun)
		{
			ui->altVsTimePlot->graph(3)->setData(xs, ys);
			ui->altVsTimePlot->graph(4)->setData(xs, ysc);
			ui->altVsTimePlot->graph(5)->setData(xs, ysn);
			ui->altVsTimePlot->graph(6)->setData(xs, ysa);
		}
		if (plotAltVsTimeMoon && onEarth)
			ui->altVsTimePlot->graph(7)->setData(xm, ym);

		ui->altVsTimePlot->replot();
	}

	// clean up the data when selection is removed
	if (!objectMgr->getWasSelected())
	{
		ui->altVsTimePlot->graph(0)->data()->clear(); // main data: Altitude vs. Time graph
		ui->altVsTimePlot->graph(2)->data()->clear(); // additional data: Transit Time Diagram
		ui->altVsTimePlot->graph(3)->data()->clear(); // additional data: Sun
		ui->altVsTimePlot->graph(4)->data()->clear(); // additional data: Civil Twilight
		ui->altVsTimePlot->graph(5)->data()->clear(); // additional data: Nautical Twilight
		ui->altVsTimePlot->graph(6)->data()->clear(); // additional data: Astronomical Twilight
		ui->altVsTimePlot->graph(7)->data()->clear(); // additional data: Moon
		ui->altVsTimePlot->replot();
	}
}

// Added vertical line indicating "now"
void AstroCalcDialog::drawCurrentTimeDiagram()
{
	// special case - plot the graph when tab is visible
	if (!plotAltVsTime)
		return;

	double currentJD = core->getJD();
	double now = ((currentJD + 0.5 - (int)currentJD) * 86400.0) + core->getUTCOffset(currentJD) * 3600.0;
	if (now > 129600) now -= 86400;
	if (now < 43200) now += 86400;
	QList<double> ax, ay;
	ax.append(now);
	ax.append(now);
	ay.append(minY);
	ay.append(maxY);
	QVector<double> x = ax.toVector(), y = ay.toVector();
	ui->altVsTimePlot->graph(1)->setData(x, y);
	ui->altVsTimePlot->replot();
}

// Added vertical line indicating time of transit
void AstroCalcDialog::drawTransitTimeDiagram()
{
	// special case - plot the graph when tab is visible
	if (!plotAltVsTime)
		return;

	QList<double> ax, ay;
	ax.append(transitX);
	ax.append(transitX);
	ay.append(minY);
	ay.append(maxY);
	QVector<double> x = ax.toVector(), y = ay.toVector();	
	ui->altVsTimePlot->graph(2)->setData(x, y);
	ui->altVsTimePlot->replot();
}

void AstroCalcDialog::prepareAxesAndGraph()
{
	QString xAxisStr = q_("Local Time");
	QString yAxisStr = QString("%1, %2").arg(q_("Altitude"), QChar(0x00B0));

	QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);

	ui->altVsTimePlot->clearGraphs();

	// main data: Altitude vs. Time graph
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->altVsTimePlot->graph(0)->setPen(QPen(Qt::red, 1));
	ui->altVsTimePlot->graph(0)->setLineStyle(QCPGraph::lsLine);
	ui->altVsTimePlot->graph(0)->rescaleAxes(true);

	// additional data: Current Time Diagram
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->graph(1)->setPen(QPen(Qt::yellow, 1));
	ui->altVsTimePlot->graph(1)->setLineStyle(QCPGraph::lsLine);
	ui->altVsTimePlot->graph(1)->setName("[Now]");

	// additional data: Transit Time Diagram
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->graph(2)->setPen(QPen(Qt::cyan, 1));
	ui->altVsTimePlot->graph(2)->setLineStyle(QCPGraph::lsLine);
	ui->altVsTimePlot->graph(2)->setName("[Transit]");

	// additional data: Sun Elevation
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->graph(3)->setPen(QPen(Qt::darkBlue, 1));
	ui->altVsTimePlot->graph(3)->setLineStyle(QCPGraph::lsLine);
	ui->altVsTimePlot->graph(3)->setName("[Sun]");
	// additional data: Civil Twilight
	QPen pen;
	pen.setStyle(Qt::DotLine);
	pen.setWidth(1);
	pen.setColor(Qt::blue);
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->graph(4)->setPen(pen);
	ui->altVsTimePlot->graph(4)->setName("[Civil Twilight]");
	// additional data: Nautical Twilight
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->graph(5)->setPen(pen);
	ui->altVsTimePlot->graph(5)->setName("[Nautical Twilight]");
	// additional data: Astronomical Twilight	
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->graph(6)->setPen(pen);
	ui->altVsTimePlot->graph(6)->setName("[Astronomical Twilight]");

	// additional data: Moon Elevation
	pen.setStyle(Qt::DashLine);
	pen.setColor(Qt::darkBlue);
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->graph(7)->setPen(pen);
	ui->altVsTimePlot->graph(7)->setName("[Moon]");

	ui->altVsTimePlot->xAxis->setLabel(xAxisStr);
	ui->altVsTimePlot->yAxis->setLabel(yAxisStr);

	ui->altVsTimePlot->xAxis->setRange(43200, 129600); // 24 hours since 12h00m (range in seconds)
	ui->altVsTimePlot->xAxis->setScaleType(QCPAxis::stLinear);
	ui->altVsTimePlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
	ui->altVsTimePlot->xAxis->setLabelColor(axisColor);
	ui->altVsTimePlot->xAxis->setTickLabelColor(axisColor);
	ui->altVsTimePlot->xAxis->setBasePen(axisPen);
	ui->altVsTimePlot->xAxis->setTickPen(axisPen);
	ui->altVsTimePlot->xAxis->setSubTickPen(axisPen);
	ui->altVsTimePlot->xAxis->setDateTimeFormat("H:mm");
	ui->altVsTimePlot->xAxis->setDateTimeSpec(Qt::UTC); // Qt::UTC + core->getUTCOffset() give local time
	ui->altVsTimePlot->xAxis->setAutoTickStep(false);
	ui->altVsTimePlot->xAxis->setTickStep(7200); // step is 2 hours (in seconds)
	ui->altVsTimePlot->xAxis->setAutoSubTicks(false);
	ui->altVsTimePlot->xAxis->setSubTickCount(7);

	ui->altVsTimePlot->yAxis->setRange(minY, maxY);
	ui->altVsTimePlot->yAxis->setScaleType(QCPAxis::stLinear);
	ui->altVsTimePlot->yAxis->setLabelColor(axisColor);
	ui->altVsTimePlot->yAxis->setTickLabelColor(axisColor);
	ui->altVsTimePlot->yAxis->setBasePen(axisPen);
	ui->altVsTimePlot->yAxis->setTickPen(axisPen);
	ui->altVsTimePlot->yAxis->setSubTickPen(axisPen);
}

void AstroCalcDialog::drawXVsTimeGraphs()
{
	PlanetP ssObj = solarSystem->searchByEnglishName(ui->graphsCelestialBodyComboBox->currentData().toString());
	if (!ssObj.isNull())
	{
		// X axis - time; Y axis - altitude
		QList<double> aX, aY, bY;

		double currentJD = core->getJD();
		int year, month, day;
		double startJD, JD, ltime, distance, angularSize;
		StelUtils::getDateFromJulianDay(currentJD, &year, &month, &day);
		StelUtils::getJDFromDate(&startJD, year, 1, 1, 0, 0, 0);

		float width = 1.0f;
		int dYear = (int)core->getCurrentPlanet()->getSiderealPeriod() + 3;

		for (int i = -2; i <= dYear; i++)
		{
			JD = startJD + i;
			ltime = (JD - startJD) * StelCore::ONE_OVER_JD_SECOND;
			aX.append(ltime);

			core->setJD(JD);

			switch (ui->graphsFirstComboBox->currentData().toInt())
			{
				case GraphMagnitudeVsTime:
					aY.append(ssObj->getVMagnitude(core));
					break;
				case GraphPhaseVsTime:
					aY.append(ssObj->getPhase(core->getObserverHeliocentricEclipticPos()) * 100.f);
					break;
				case GraphDistanceVsTime:
					distance = ssObj->getJ2000EquatorialPos(core).length();
					if (distance < 0.1)
						distance *= AU / 1000.f;
					aY.append(distance);
					break;
				case GraphElongationVsTime:
					aY.append(ssObj->getElongation(core->getObserverHeliocentricEclipticPos()) * 180. / M_PI);
					break;
				case GraphAngularSizeVsTime:
					angularSize = ssObj->getAngularSize(core) * 360. / M_PI;
					if (angularSize < 1.)
						angularSize *= 60.;
					aY.append(angularSize);
					break;
				case GraphPhaseAngleVsTime:
					aY.append(ssObj->getPhaseAngle(core->getObserverHeliocentricEclipticPos()) * 180. / M_PI);
					break;
			}

			switch (ui->graphsSecondComboBox->currentData().toInt())
			{
				case GraphMagnitudeVsTime:
					bY.append(ssObj->getVMagnitude(core));
					break;
				case GraphPhaseVsTime:
					bY.append(ssObj->getPhase(core->getObserverHeliocentricEclipticPos()) * 100.f);
					break;
				case GraphDistanceVsTime:
					distance = ssObj->getJ2000EquatorialPos(core).length();
					if (distance < 0.1)
						distance *= AU / 1000.f;
					bY.append(distance);
					break;
				case GraphElongationVsTime:
					bY.append(ssObj->getElongation(core->getObserverHeliocentricEclipticPos()) * 180. / M_PI);
					break;
				case GraphAngularSizeVsTime:
					angularSize = ssObj->getAngularSize(core) * 360. / M_PI;
					if (angularSize < 1.)
						angularSize *= 60.;
					bY.append(angularSize);
					break;
				case GraphPhaseAngleVsTime:
					bY.append(ssObj->getPhaseAngle(core->getObserverHeliocentricEclipticPos()) * 180. / M_PI);
					break;
			}

			core->update(0.0);
		}
		core->setJD(currentJD);

		QVector<double> x = aX.toVector(), ya = aY.toVector(), yb = bY.toVector();

		double minYa = aY.first();
		double maxYa = aY.first();

		foreach (double temp, aY)
		{
			if (maxYa < temp) maxYa = temp;
			if (minYa > temp) minYa = temp;
		}

		width = (maxYa - minYa) / 50.f;
		minY1 = minYa - width;
		maxY1 = maxYa + width;

		minYa = bY.first();
		maxYa = bY.first();

		foreach (double temp, bY)
		{
			if (maxYa < temp) maxYa = temp;
			if (minYa > temp) minYa = temp;
		}

		width = (maxYa - minYa) / 50.f;
		minY2 = minYa - width;
		maxY2 = maxYa + width;

		prepareXVsTimeAxesAndGraph();

		ui->graphsPlot->clearGraphs();

		ui->graphsPlot->addGraph(ui->graphsPlot->xAxis, ui->graphsPlot->yAxis);
		ui->graphsPlot->setBackground(QBrush(QColor(86, 87, 90)));
		ui->graphsPlot->graph(0)->setPen(QPen(Qt::red, 1));
		ui->graphsPlot->graph(0)->setLineStyle(QCPGraph::lsLine);
		ui->graphsPlot->graph(0)->rescaleAxes(true);
		ui->graphsPlot->graph(0)->setData(x, ya);
		ui->graphsPlot->graph(0)->setName("[0]");

		ui->graphsPlot->addGraph(ui->graphsPlot->xAxis, ui->graphsPlot->yAxis2);
		ui->graphsPlot->setBackground(QBrush(QColor(86, 87, 90)));
		ui->graphsPlot->graph(1)->setPen(QPen(Qt::yellow, 1));
		ui->graphsPlot->graph(1)->setLineStyle(QCPGraph::lsLine);
		ui->graphsPlot->graph(1)->rescaleAxes(true);
		ui->graphsPlot->graph(1)->setData(x, yb);
		ui->graphsPlot->graph(1)->setName("[1]");

		ui->graphsPlot->replot();
	}
}

void AstroCalcDialog::populateFunctionsList()
{
	Q_ASSERT(ui->graphsFirstComboBox);
	Q_ASSERT(ui->graphsSecondComboBox);

	typedef QPair<QString, GraphsTypes> graph;
	graph cf;
	QList<graph> functions;
	functions.clear();
	cf.first = q_("Magnitude vs. Time");
	cf.second = GraphMagnitudeVsTime;
	functions.append(cf);
	cf.first = q_("Phase vs. Time");
	cf.second = GraphPhaseVsTime;
	functions.append(cf);
	cf.first = q_("Distance vs. Time");
	cf.second = GraphDistanceVsTime;
	functions.append(cf);
	cf.first = q_("Elongation vs. Time");
	cf.second = GraphElongationVsTime;
	functions.append(cf);
	cf.first = q_("Angular size vs. Time");
	cf.second = GraphAngularSizeVsTime;
	functions.append(cf);
	cf.first = q_("Phase angle vs. Time");
	cf.second = GraphPhaseAngleVsTime;
	functions.append(cf);

	QComboBox* first = ui->graphsFirstComboBox;
	QComboBox* second = ui->graphsSecondComboBox;
	first->blockSignals(true);
	second->blockSignals(true);

	int indexF = first->currentIndex();
	QVariant selectedFirstId = first->itemData(indexF);
	int indexS = second->currentIndex();
	QVariant selectedSecondId = second->itemData(indexS);

	first->clear();
	second->clear();

	foreach (const graph& f, functions)
	{
		first->addItem(f.first, f.second);
		second->addItem(f.first, f.second);
	}

	indexF = first->findData(selectedFirstId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexF < 0)
		indexF = first->findData(conf->value("astrocalc/graphs_first_id", GraphMagnitudeVsTime).toInt(), Qt::UserRole, Qt::MatchCaseSensitive);
	first->setCurrentIndex(indexF);
	first->model()->sort(0);

	indexS = second->findData(selectedSecondId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexS < 0)
		indexS = second->findData(conf->value("astrocalc/graphs_second_id", GraphPhaseVsTime).toInt(), Qt::UserRole, Qt::MatchCaseSensitive);
	second->setCurrentIndex(indexS);
	second->model()->sort(0);

	first->blockSignals(false);
	second->blockSignals(false);
}

void AstroCalcDialog::prepareXVsTimeAxesAndGraph()
{
	QString xAxisStr = q_("Date");
	QString distMU = qc_("AU", "distance, astronomical unit");
	QString asMU = QString("'");

	PlanetP ssObj = solarSystem->searchByEnglishName(ui->graphsCelestialBodyComboBox->currentData().toString());
	if (!ssObj.isNull())
	{
		if (ssObj->getJ2000EquatorialPos(core).length() < 0.1)
		{
			// TRANSLATORS: Megameter (SI symbol: Mm; Megameter is a unit of length in the metric system,
			// equal to one million metres)
			distMU = q_("Mm");
		}
		if ((ssObj->getAngularSize(core) * 360. / M_PI) < 1.) asMU = QString("\"");
	}

	bool direction1 = false;
	bool direction2 = false;

	switch (ui->graphsFirstComboBox->currentData().toInt())
	{
		case GraphMagnitudeVsTime:
			yAxis1Legend = q_("Magnitude");
			if (minY1 < -1000.f) minY1 = 0.f;
			if (maxY1 > 1000.f) maxY1 = 6.f;
			direction1 = true;
			break;
		case GraphPhaseVsTime:
			yAxis1Legend = QString("%1, %").arg(q_("Phase"));
			if (minY1 < -1000.f) minY1 = 0.f;
			if (maxY1 > 1000.f) maxY1 = 100.f;
			break;
		case GraphDistanceVsTime:
			yAxis1Legend = QString("%1, %2").arg(q_("Distance"), distMU);
			if (minY1 < -1000.f) minY1 = 0.f;
			if (maxY1 > 1000.f) maxY1 = 50.f;
			break;
		case GraphElongationVsTime:
			yAxis1Legend = QString("%1, %2").arg(q_("Elongation"), QChar(0x00B0));
			if (minY1 < -1000.f) minY1 = 0.f;
			if (maxY1 > 1000.f) maxY1 = 180.f;
			break;
		case GraphAngularSizeVsTime:
			yAxis1Legend = QString("%1, %2").arg(q_("Angular size"), asMU);
			if (minY1 < -1000.f) minY1 = 0.f;
			if (maxY1 > 1000.f) maxY1 = 30.f;
			break;
		case GraphPhaseAngleVsTime:
			yAxis1Legend = QString("%1, %2").arg(q_("Phase angle"), QChar(0x00B0));
			if (minY1 < -1000.f) minY1 = 0.f;
			if (maxY1 > 1000.f) maxY1 = 180.f;
			break;
	}

	switch (ui->graphsSecondComboBox->currentData().toInt())
	{
		case GraphMagnitudeVsTime:
			yAxis2Legend = q_("Magnitude");
			if (minY2 < -1000.f) minY2 = 0.f;
			if (maxY2 > 1000.f) maxY2 = 6.f;
			direction2 = true;
			break;
		case GraphPhaseVsTime:
			yAxis2Legend = QString("%1, %").arg(q_("Phase"));
			if (minY2 < -1000.f) minY2 = 0.f;
			if (maxY2 > 1000.f) maxY2 = 100.f;
			break;
		case GraphDistanceVsTime:
			yAxis2Legend = QString("%1, %2").arg(q_("Distance"), distMU);
			if (minY2 < -1000.f) minY2 = 0.f;
			if (maxY2 > 1000.f) maxY2 = 50.f;
			break;
		case GraphElongationVsTime:
			yAxis2Legend = QString("%1, %2").arg(q_("Elongation"), QChar(0x00B0));
			if (minY2 < -1000.f) minY2 = 0.f;
			if (maxY2 > 1000.f) maxY2 = 180.f;
			break;
		case GraphAngularSizeVsTime:
			yAxis2Legend = QString("%1, %2").arg(q_("Angular size"), asMU);
			if (minY2 < -1000.f) minY2 = 0.f;
			if (maxY2 > 1000.f) maxY2 = 30.f;
			break;
		case GraphPhaseAngleVsTime:
			yAxis2Legend = QString("%1, %2").arg(q_("Phase angle"), QChar(0x00B0));
			if (minY2 < -1000.f) minY2 = 0.f;
			if (maxY2 > 1000.f) maxY2 = 180.f;
			break;
	}

	QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);

	ui->graphsPlot->xAxis->setLabel(xAxisStr);
	ui->graphsPlot->yAxis->setLabel(yAxis1Legend);
	ui->graphsPlot->yAxis2->setLabel(yAxis2Legend);

	int dYear = ((int)core->getCurrentPlanet()->getSiderealPeriod() + 1) * 86400;
	ui->graphsPlot->xAxis->setRange(0, dYear);
	ui->graphsPlot->xAxis->setScaleType(QCPAxis::stLinear);
	ui->graphsPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
	ui->graphsPlot->xAxis->setLabelColor(axisColor);
	ui->graphsPlot->xAxis->setTickLabelColor(axisColor);
	ui->graphsPlot->xAxis->setBasePen(axisPen);
	ui->graphsPlot->xAxis->setTickPen(axisPen);
	ui->graphsPlot->xAxis->setSubTickPen(axisPen);
	ui->graphsPlot->xAxis->setDateTimeFormat("d\nMMM");
	ui->graphsPlot->xAxis->setDateTimeSpec(Qt::UTC);
	ui->graphsPlot->xAxis->setAutoTicks(true);
	ui->graphsPlot->xAxis->setAutoTickCount(15);

	ui->graphsPlot->yAxis->setRange(minY1, maxY1);
	ui->graphsPlot->yAxis->setScaleType(QCPAxis::stLinear);
	ui->graphsPlot->yAxis->setLabelColor(axisColor);
	ui->graphsPlot->yAxis->setTickLabelColor(axisColor);
	ui->graphsPlot->yAxis->setBasePen(axisPen);
	ui->graphsPlot->yAxis->setTickPen(axisPen);
	ui->graphsPlot->yAxis->setSubTickPen(axisPen);
	ui->graphsPlot->yAxis->setRangeReversed(direction1);

	ui->graphsPlot->yAxis2->setRange(minY2, maxY2);
	ui->graphsPlot->yAxis2->setScaleType(QCPAxis::stLinear);
	ui->graphsPlot->yAxis2->setLabelColor(axisColor);
	ui->graphsPlot->yAxis2->setTickLabelColor(axisColor);
	ui->graphsPlot->yAxis2->setBasePen(axisPen);
	ui->graphsPlot->yAxis2->setTickPen(axisPen);
	ui->graphsPlot->yAxis2->setSubTickPen(axisPen);
	ui->graphsPlot->yAxis2->setRangeReversed(direction2);
	ui->graphsPlot->yAxis2->setVisible(true);

	ui->graphsPlot->clearGraphs();
	ui->graphsPlot->addGraph(ui->graphsPlot->xAxis, ui->graphsPlot->yAxis);
	ui->graphsPlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->graphsPlot->graph(0)->setPen(QPen(Qt::red, 1));
	ui->graphsPlot->graph(0)->setLineStyle(QCPGraph::lsLine);
	ui->graphsPlot->graph(0)->rescaleAxes(true);

	ui->graphsPlot->addGraph(ui->graphsPlot->xAxis, ui->graphsPlot->yAxis2);
	ui->graphsPlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->graphsPlot->graph(1)->setPen(QPen(Qt::yellow, 1));
	ui->graphsPlot->graph(1)->setLineStyle(QCPGraph::lsLine);
	ui->graphsPlot->graph(1)->rescaleAxes(true);
}

void AstroCalcDialog::prepareMonthlyEleveationAxesAndGraph()
{
	QString xAxisStr = q_("Date");
	QString yAxisStr = QString("%1, %2").arg(q_("Altitude"), QChar(0x00B0));

	QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);

	ui->monthlyElevationGraph->xAxis->setLabel(xAxisStr);
	ui->monthlyElevationGraph->yAxis->setLabel(yAxisStr);

	int dYear = ((int)core->getCurrentPlanet()->getSiderealPeriod() + 1) * 86400;
	ui->monthlyElevationGraph->xAxis->setRange(0, dYear);
	ui->monthlyElevationGraph->xAxis->setScaleType(QCPAxis::stLinear);
	ui->monthlyElevationGraph->xAxis->setTickLabelType(QCPAxis::ltDateTime);
	ui->monthlyElevationGraph->xAxis->setLabelColor(axisColor);
	ui->monthlyElevationGraph->xAxis->setTickLabelColor(axisColor);
	ui->monthlyElevationGraph->xAxis->setBasePen(axisPen);
	ui->monthlyElevationGraph->xAxis->setTickPen(axisPen);
	ui->monthlyElevationGraph->xAxis->setSubTickPen(axisPen);
	ui->monthlyElevationGraph->xAxis->setDateTimeFormat("d\nMMM");
	ui->monthlyElevationGraph->xAxis->setDateTimeSpec(Qt::UTC);
	ui->monthlyElevationGraph->xAxis->setAutoTicks(true);
	ui->monthlyElevationGraph->xAxis->setAutoTickCount(15);

	ui->monthlyElevationGraph->yAxis->setRange(minYme, maxYme);
	ui->monthlyElevationGraph->yAxis->setScaleType(QCPAxis::stLinear);
	ui->monthlyElevationGraph->yAxis->setLabelColor(axisColor);
	ui->monthlyElevationGraph->yAxis->setTickLabelColor(axisColor);
	ui->monthlyElevationGraph->yAxis->setBasePen(axisPen);
	ui->monthlyElevationGraph->yAxis->setTickPen(axisPen);
	ui->monthlyElevationGraph->yAxis->setSubTickPen(axisPen);

	ui->monthlyElevationGraph->clearGraphs();
	ui->monthlyElevationGraph->addGraph();
	ui->monthlyElevationGraph->setBackground(QBrush(QColor(86, 87, 90)));
	ui->monthlyElevationGraph->graph(0)->setPen(QPen(Qt::red, 1));
	ui->monthlyElevationGraph->graph(0)->setLineStyle(QCPGraph::lsLine);
	ui->monthlyElevationGraph->graph(0)->rescaleAxes(true);
}

void AstroCalcDialog::syncMonthlyElevationTime()
{
	ui->monthlyElevationTimeInfo->setText(QString("%1 %2").arg(QString::number(ui->monthlyElevationTime->value()), qc_("h", "time")));
}

void AstroCalcDialog::updateMonthlyElevationTime()
{
	syncMonthlyElevationTime();

	conf->setValue("astrocalc/me_time", ui->monthlyElevationTime->value());

	drawMonthlyElevationGraph();
}

void AstroCalcDialog::saveMonthlyElevationPositiveFlag(bool state)
{
	if (plotMonthlyElevationPositive!=state)
	{
		plotMonthlyElevationPositive = state;
		conf->setValue("astrocalc/me_positive_only", plotMonthlyElevationPositive);

		drawMonthlyElevationGraph();
	}
}

void AstroCalcDialog::drawMonthlyElevationGraph()
{
	ui->monthlyElevationCelestialObjectLabel->setText("");

	// Avoid crash!
	if (core->getCurrentPlanet()->getEnglishName().contains("->")) // We are on the spaceship!
		return;

	// special case - plot the graph when tab is visible
	if (!plotMonthlyElevation)
		return;

	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();

	if (!selectedObjects.isEmpty())
	{
		// X axis - time; Y axis - altitude
		QList<double> aX, aY;

		StelObjectP selectedObject = selectedObjects[0];

		if (selectedObject->getType() == "Satellite")
		{
			ui->monthlyElevationGraph->graph(0)->data()->clear();
			ui->monthlyElevationGraph->replot();
			return;
		}

		double currentJD = core->getJD();
		int hour = ui->monthlyElevationTime->value();

		double az, alt, deg;
		bool sign;
		int year, month, day;
		double startJD, JD, ltime;
		StelUtils::getDateFromJulianDay(currentJD, &year, &month, &day);
		StelUtils::getJDFromDate(&startJD, year, 1, 1, hour, 0, 0);
		startJD -= core->getUTCOffset(startJD)/24; // Time zone correction

		int dYear = (int)core->getCurrentPlanet()->getSiderealPeriod() + 3;

		for (int i = -2; i <= dYear; i++)
		{
			JD = startJD + i;
			ltime = (JD - startJD) * StelCore::ONE_OVER_JD_SECOND;
			aX.append(ltime);

			core->setJD(JD);
			StelUtils::rectToSphe(&az, &alt, selectedObject->getAltAzPosAuto(core));
			StelUtils::radToDecDeg(alt, sign, deg);
			if (!sign) deg *= -1;
			aY.append(deg);

			core->update(0.0);
		}
		core->setJD(currentJD);

		QVector<double> x = aX.toVector(), y = aY.toVector();

		double minYa = aY.first();
		double maxYa = aY.first();

		foreach (double temp, aY)
		{
			if (maxYa < temp) maxYa = temp;
			if (minYa > temp) minYa = temp;
		}

		minYme = minYa - 2.0;
		maxYme = maxYa + 2.0;

		if (plotMonthlyElevationPositive && minYme<0.f)
			minYme = 0.f;

		prepareMonthlyEleveationAxesAndGraph();

		QString name = selectedObject->getNameI18n();
		if (name.isEmpty() && selectedObject->getType() == "Nebula")
			name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();

		ui->monthlyElevationGraph->graph(0)->setData(x, y);
		ui->monthlyElevationGraph->graph(0)->setName(name);
		ui->monthlyElevationGraph->replot();

		ui->monthlyElevationCelestialObjectLabel->setText(name);

	}

	// clean up the data when selection is removed
	if (!objectMgr->getWasSelected())
	{
		ui->monthlyElevationGraph->graph(0)->data()->clear();
		ui->monthlyElevationGraph->replot();
	}

}

void AstroCalcDialog::mouseOverLine(QMouseEvent* event)
{
	double x = ui->altVsTimePlot->xAxis->pixelToCoord(event->pos().x());
	double y = ui->altVsTimePlot->yAxis->pixelToCoord(event->pos().y());

	QCPAbstractPlottable* abstractGraph = ui->altVsTimePlot->plottableAt(event->pos(), false);
	QCPGraph* graph = qobject_cast<QCPGraph*>(abstractGraph);

	if (x > ui->altVsTimePlot->xAxis->range().lower && x < ui->altVsTimePlot->xAxis->range().upper
	    && y > ui->altVsTimePlot->yAxis->range().lower && y < ui->altVsTimePlot->yAxis->range().upper)
	{
		if (graph)
		{
			QString info;
			double JD;
			if (graph->name() == "[Now]")
			{
				JD = core->getJD();
				info = q_("Now about %1").arg(StelUtils::jdToQDateTime(JD + core->getUTCOffset(JD)/24).toString("H:mm"));
			}
			else if (graph->name() == "[Transit]")
			{
				JD = transitX / 86400.0 + (int)core->getJD() - 0.5;
				info = q_("Passage of meridian at approximately %1").arg(StelUtils::jdToQDateTime(JD - core->getUTCOffset(JD)).toString("H:mm"));
			}
			else if (graph->name() == "[Sun]")
				info = solarSystem->getSun()->getNameI18n();
			else if (graph->name() == "[Moon]")
				info = solarSystem->getMoon()->getNameI18n();
			else if (graph->name() == "[Civil Twilight]")
				info = q_("Line of civil twilight");
			else if (graph->name() == "[Nautical Twilight]")
				info = q_("Line of nautical twilight");
			else if (graph->name() == "[Astronomical Twilight]")
				info = q_("Line of astronomical twilight");
			else
			{
				JD = x / 86400.0 + (int)core->getJD() - 0.5;
				QString LT = StelUtils::jdToQDateTime(JD - core->getUTCOffset(JD)).toString("H:mm");

				if (StelApp::getInstance().getFlagShowDecimalDegrees())
					info = QString("%1<br />%2: %3<br />%4: %5%6").arg(ui->altVsTimePlot->graph(0)->name(), q_("Local Time"), LT, q_("Altitude"), QString::number(y, 'f', 2), QChar(0x00B0));
				else
					info = QString("%1<br />%2: %3<br />%4: %5").arg(ui->altVsTimePlot->graph(0)->name(), q_("Local Time"), LT, q_("Altitude"), StelUtils::decDegToDmsStr(y));
			}

			QToolTip::hideText();
			QToolTip::showText(event->globalPos(), info, ui->altVsTimePlot, ui->altVsTimePlot->rect());
		}
		else
			QToolTip::hideText();
	}

	ui->altVsTimePlot->update();
	ui->altVsTimePlot->replot();
}

void AstroCalcDialog::setPhenomenaHeaderNames()
{
	phenomenaHeader.clear();
	phenomenaHeader << q_("Phenomenon");
	phenomenaHeader << q_("Date and Time");
	phenomenaHeader << q_("Object 1");
	phenomenaHeader << q_("Object 2");
	phenomenaHeader << q_("Separation");
	ui->phenomenaTreeWidget->setHeaderLabels(phenomenaHeader);

	// adjust the column width
	for (int i = 0; i < PhenomenaCount; ++i)
	{
		ui->phenomenaTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListPhenomena()
{
	ui->phenomenaTreeWidget->clear();
	ui->phenomenaTreeWidget->setColumnCount(PhenomenaCount);
	setPhenomenaHeaderNames();
	ui->phenomenaTreeWidget->header()->setSectionsMovable(false);
}

void AstroCalcDialog::selectCurrentPhenomen(const QModelIndex& modelIndex)
{
	// Find the object
	QString name = ui->object1ComboBox->currentData().toString();
	double JD = modelIndex.sibling(modelIndex.row(), PhenomenaDate).data(Qt::UserRole).toDouble();

	if (objectMgr->findAndSelectI18n(name) || objectMgr->findAndSelect(name))
	{
		core->setJD(JD);
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			mvMgr->moveToObject(newSelected[0], mvMgr->getAutoMoveDuration());
			mvMgr->setFlagTracking(true);
		}
	}
}

void AstroCalcDialog::calculatePhenomena()
{
	QString currentPlanet = ui->object1ComboBox->currentData().toString();
	double separation = ui->allowedSeparationDoubleSpinBox->value();
	bool opposition = ui->phenomenaOppositionCheckBox->isChecked();

	initListPhenomena();

	QList<PlanetP> objects;
	objects.clear();
	QList<PlanetP> allObjects = solarSystem->getAllPlanets();

	QList<NebulaP> dso;
	dso.clear();
	QVector<NebulaP> allDSO = dsoMgr->getAllDeepSkyObjects();

	QList<StelObjectP> star, doubleStar, variableStar;
	star.clear();
	doubleStar.clear();
	variableStar.clear();
	QList<StelObjectP> hipStars = starMgr->getHipparcosStars();
	QList<StelACStarData> doubleHipStars = starMgr->getHipparcosDoubleStars();
	QList<StelACStarData> variableHipStars = starMgr->getHipparcosVariableStars();

	int obj2Type = ui->object2ComboBox->currentData().toInt();
	switch (obj2Type)
	{
		case 0: // Solar system
			foreach (const PlanetP& object, allObjects)
			{
				if (object->getPlanetType() != Planet::isUNDEFINED)
					objects.append(object);
			}
			break;
		case 1: // Planets
			foreach (const PlanetP& object, allObjects)
			{
				if (object->getPlanetType() == Planet::isPlanet && object->getEnglishName() != core->getCurrentPlanet()->getEnglishName() && object->getEnglishName() != currentPlanet)
					objects.append(object);
			}
			break;
		case 2: // Asteroids
			foreach (const PlanetP& object, allObjects)
			{
				if (object->getPlanetType() == Planet::isAsteroid)
					objects.append(object);
			}
			break;
		case 3: // Plutinos
			foreach (const PlanetP& object, allObjects)
			{
				if (object->getPlanetType() == Planet::isPlutino)
					objects.append(object);
			}
			break;
		case 4: // Comets
			foreach (const PlanetP& object, allObjects)
			{
				if (object->getPlanetType() == Planet::isComet)
					objects.append(object);
			}
			break;
		case 5: // Dwarf planets
			foreach (const PlanetP& object, allObjects)
			{
				if (object->getPlanetType() == Planet::isDwarfPlanet)
					objects.append(object);
			}
			break;
		case 6: // Cubewanos
			foreach (const PlanetP& object, allObjects)
			{
				if (object->getPlanetType() == Planet::isCubewano)
					objects.append(object);
			}
			break;
		case 7: // Scattered disc objects
			foreach (const PlanetP& object, allObjects)
			{
				if (object->getPlanetType() == Planet::isSDO)
					objects.append(object);
			}
			break;
		case 8: // Oort cloud objects
			foreach (const PlanetP& object, allObjects)
			{
				if (object->getPlanetType() == Planet::isOCO)
					objects.append(object);
			}
			break;
		case 9: // Sednoids
			foreach (const PlanetP& object, allObjects)
			{
				if (object->getPlanetType() == Planet::isSednoid)
					objects.append(object);
			}
			break;
		case 10: // Stars
			foreach (const StelObjectP& object, hipStars)
			{
				if (object->getVMagnitude(core) < (brightLimit - 5.0f))
					star.append(object);
			}
			break;
		case 11: // Double stars
			foreach (const StelACStarData& object, doubleHipStars)
			{
				if (object.firstKey()->getVMagnitude(core) < (brightLimit - 5.0f))
					star.append(object.firstKey());
			}
			break;
		case 12: // Variable stars
			foreach (const StelACStarData& object, variableHipStars)
			{
				if (object.firstKey()->getVMagnitude(core) < (brightLimit - 5.0f))
					star.append(object.firstKey());
			}
			break;
		case 13: // Star clusters
			foreach (const NebulaP& object, allDSO)
			{
				if (object->getVMagnitude(core) < brightLimit && (object->getDSOType() == Nebula::NebCl || object->getDSOType() == Nebula::NebOc || object->getDSOType() == Nebula::NebGc || object->getDSOType() == Nebula::NebSA || object->getDSOType() == Nebula::NebSC || object->getDSOType() == Nebula::NebCn))
					dso.append(object);
			}
			break;
		case 14: // Planetary nebulae
			foreach (const NebulaP& object, allDSO)
			{
				if (object->getDSOType() == Nebula::NebPn || object->getDSOType() == Nebula::NebPossPN || object->getDSOType() == Nebula::NebPPN)
					dso.append(object);
			}
			break;
		case 15: // Bright nebulae
			foreach (const NebulaP& object, allDSO)
			{
				if (object->getVMagnitude(core) < brightLimit && (object->getDSOType() == Nebula::NebN || object->getDSOType() == Nebula::NebBn || object->getDSOType() == Nebula::NebEn || object->getDSOType() == Nebula::NebRn || object->getDSOType() == Nebula::NebHII || object->getDSOType() == Nebula::NebISM || object->getDSOType() == Nebula::NebCn || object->getDSOType() == Nebula::NebSNR))
					dso.append(object);
			}
			break;
		case 16: // Dark nebulae
			foreach (const NebulaP& object, allDSO)
			{
				if (object->getDSOType() == Nebula::NebDn || object->getDSOType() == Nebula::NebMolCld || object->getDSOType() == Nebula::NebYSO)
					dso.append(object);
			}
			break;
		case 17: // Galaxies
			foreach (const NebulaP& object, allDSO)
			{
				if (object->getVMagnitude(core) < brightLimit && (object->getDSOType() == Nebula::NebGx || object->getDSOType() == Nebula::NebAGx || object->getDSOType() == Nebula::NebRGx || object->getDSOType() == Nebula::NebQSO || object->getDSOType() == Nebula::NebPossQSO || object->getDSOType() == Nebula::NebBLL || object->getDSOType() == Nebula::NebBLA || object->getDSOType() == Nebula::NebIGx))
					dso.append(object);
			}
			break;
		case 18: // Symbiotic stars
			foreach (const NebulaP& object, allDSO)
			{
				if (object->getDSOType() == Nebula::NebSymbioticStar)
					dso.append(object);
			}
			break;
		case 19: // Emission-line stars
			foreach (const NebulaP& object, allDSO)
			{
				if (object->getDSOType() == Nebula::NebEmissionLineStar)
					dso.append(object);
			}
			break;
	}

	PlanetP planet = solarSystem->searchByEnglishName(currentPlanet);
	if (planet)
	{
		double currentJD = core->getJD();   // save current JD
		double currentJDE = core->getJDE(); // save current JDE
		double startJD = StelUtils::qDateTimeToJd(QDateTime(ui->phenomenFromDateEdit->date()));
		double stopJD = StelUtils::qDateTimeToJd(QDateTime(ui->phenomenToDateEdit->date().addDays(1)));
		startJD = startJD - core->getUTCOffset(startJD) / 24;
		stopJD = stopJD - core->getUTCOffset(stopJD) / 24;

		// Calculate the limits on coordinates for speed-up of calculations
		double coordsLimit = std::abs(core->getCurrentPlanet()->getRotObliquity(currentJDE)) + std::abs(planet->getRotObliquity(currentJDE)) + 0.026;
		coordsLimit += separation * M_PI / 180;
		double ra, dec;

		if (obj2Type < 10)
		{
			// Solar system objects
			foreach (PlanetP obj, objects)
			{
				// conjunction
				fillPhenomenaTable(findClosestApproach(planet, obj, startJD, stopJD, separation, false), planet, obj, false);
				// opposition
				if (opposition)
					fillPhenomenaTable(findClosestApproach(planet, obj, startJD, stopJD, separation, true), planet, obj, true);
			}
		}
		else if (obj2Type == 10 || obj2Type == 11 || obj2Type == 12)
		{
			// Stars
			foreach (StelObjectP obj, star)
			{
				StelUtils::rectToSphe(&ra, &dec, obj->getEquinoxEquatorialPos(core));
				// Add limits on coordinates for speed-up calculations
				if (dec <= coordsLimit && dec >= -coordsLimit)
				{
					// conjunction
					fillPhenomenaTable(findClosestApproach(planet, obj, startJD, stopJD, separation), planet, obj);
				}
			}
		}
		else
		{
			// Deep-sky objects
			foreach (NebulaP obj, dso)
			{
				StelUtils::rectToSphe(&ra, &dec, obj->getEquinoxEquatorialPos(core));
				// Add limits on coordinates for speed-up calculations
				if (dec <= coordsLimit && dec >= -coordsLimit)
				{
					// conjunction
					fillPhenomenaTable(findClosestApproach(planet, obj, startJD, stopJD, separation), planet, obj);
				}
			}
		}

		core->setJD(currentJD); // restore time
		core->update(0);
	}

	// adjust the column width
	for (int i = 0; i < PhenomenaCount; ++i)
	{
		ui->phenomenaTreeWidget->resizeColumnToContents(i);
	}

	// sort-by-date
	ui->phenomenaTreeWidget->sortItems(PhenomenaDate, Qt::AscendingOrder);
}

void AstroCalcDialog::savePhenomena()
{
	QString filter = q_("CSV (Comma delimited)");
	filter.append(" (*.csv)");
	QString filePath = QFileDialog::getSaveFileName(0, q_("Save calculated phenomena as..."), QDir::homePath() + "/phenomena.csv", filter);
	QFile phenomena(filePath);
	if (!phenomena.open(QFile::WriteOnly | QFile::Truncate))
	{
		qWarning() << "AstroCalc: Unable to open file" << QDir::toNativeSeparators(filePath);
		return;
	}

	QTextStream phenomenaList(&phenomena);
	phenomenaList.setCodec("UTF-8");

	int count = ui->phenomenaTreeWidget->topLevelItemCount();
	int columns = phenomenaHeader.size();

	for (int i = 0; i < columns; i++)
	{
		QString h = phenomenaHeader.at(i).trimmed();
		if (h.contains(","))
			phenomenaList << QString("\"%1\"").arg(h);
		else
			phenomenaList << h;

		if (i < columns - 1)
			phenomenaList << delimiter;
		else
			phenomenaList << acEndl;
	}

	for (int i = 0; i < count; i++)
	{

		for (int j = 0; j < columns; j++)
		{
			phenomenaList << ui->phenomenaTreeWidget->topLevelItem(i)->text(j);
			if (j < columns - 1)
				phenomenaList << delimiter;
			else
				phenomenaList << acEndl;
		}
	}

	phenomena.close();
}

void AstroCalcDialog::fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const PlanetP object2, bool opposition)
{
	QMap<double, double>::ConstIterator it;
	for (it = list.constBegin(); it != list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		bool occultation = false;
		double s1 = object1->getSpheroidAngularSize(core);
		double s2 = object2->getSpheroidAngularSize(core);
		if (opposition)
		{
			phenomenType = q_("Opposition");
			// Added a special case - lunar eclipse
			if (qAbs(separation) <= 0.02 && ((object1->getEnglishName() == "Moon"  && object2->getEnglishName() == "Sun") || (object1->getEnglishName() == "Sun"  && object2->getEnglishName() == "Moon")))
				phenomenType = q_("Eclipse");

			separation += M_PI;
		}
		else if (separation < (s2 * M_PI / 180.) || separation < (s1 * M_PI / 180.))
		{
			double d1 = object1->getJ2000EquatorialPos(core).length();
			double d2 = object2->getJ2000EquatorialPos(core).length();
			if ((d1 < d2 && s1 <= s2) || (d1 > d2 && s1 > s2))
				phenomenType = q_("Transit");
			else
				phenomenType = q_("Occultation");

			// Added a special case - solar eclipse
			if (qAbs(s1 - s2) <= 0.05 && (object1->getEnglishName() == "Sun"  || object2->getEnglishName() == "Sun")) // 5% error of difference of sizes
				phenomenType = q_("Eclipse");

			occultation = true;
		}

		ACPhenTreeWidgetItem* treeItem = new ACPhenTreeWidgetItem(ui->phenomenaTreeWidget);
		treeItem->setText(PhenomenaType, phenomenType);
		// local date and time
		treeItem->setText(PhenomenaDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(it.key()), localeMgr->getPrintableTimeLocal(it.key())));
		treeItem->setData(PhenomenaDate, Qt::UserRole, it.key());
		treeItem->setText(PhenomenaObject1, object1->getNameI18n());
		treeItem->setText(PhenomenaObject2, object2->getNameI18n());
		if (occultation)
			treeItem->setText(PhenomenaSeparation, QChar(0x2014));
		else
			treeItem->setText(PhenomenaSeparation, StelUtils::radToDmsStr(separation));
	}
}

QMap<double, double> AstroCalcDialog::findClosestApproach(PlanetP& object1, PlanetP& object2, double startJD, double stopJD, double maxSeparation, bool opposition)
{
	double dist, prevDist, step, step0;
	int sgn, prevSgn = 0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	step0 = (stopJD - startJD) / 12.0;
	if (step0 > 24.8 * 365.25) step0 = 24.8 * 365.25;

	if (object1->getEnglishName() == "Neptune" || object2->getEnglishName() == "Neptune" || object1->getEnglishName() == "Uranus" || object2->getEnglishName() == "Uranus")
	{
		if (step0 > 3652.5)
			step0 = 3652.5;
	}
	if (object1->getEnglishName() == "Jupiter" || object2->getEnglishName() == "Jupiter" || object1->getEnglishName() == "Saturn" || object2->getEnglishName() == "Saturn")
	{
		if (step0 > 365.25)
			step0 = 365.f;
	}
	if (object1->getEnglishName() == "Mars" || object2->getEnglishName() == "Mars")
	{
		if (step0 > 10.f)
			step0 = 10.f;
	}
	if (object1->getEnglishName() == "Venus" || object2->getEnglishName() == "Venus" || object1->getEnglishName() == "Mercury" || object2->getEnglishName() == "Mercury")
	{
		if (step0 > 5.f)
			step0 = 5.f;
	}
	if (object1->getEnglishName() == "Moon" || object2->getEnglishName() == "Moon")
	{
		if (step0 > 0.25)
			step0 = 0.25;
	}

	step = step0;
	double jd = startJD;
	prevDist = findDistance(jd, object1, object2, opposition);
	jd += step;
	while (jd <= stopJD)
	{
		dist = findDistance(jd, object1, object2, opposition);
		sgn = StelUtils::sign(dist - prevDist);

		double factor = qAbs((dist - prevDist) / dist);
		if (factor > 10.f)
			step = step0 * factor / 10.f;
		else
			step = step0;

		if (sgn != prevSgn && prevSgn == -1)
		{
			if (step > step0)
			{
				jd -= step;
				step = step0;
				sgn = prevSgn;
				while (jd <= stopJD)
				{
					dist = findDistance(jd, object1, object2, opposition);
					sgn = StelUtils::sign(dist - prevDist);
					if (sgn != prevSgn)
						break;

					prevDist = dist;
					prevSgn = sgn;
					jd += step;
				}
			}

			if (findPrecise(&extremum, object1, object2, jd, step, sgn, opposition))
			{
				double sep = extremum.second * 180. / M_PI;
				if (sep < maxSeparation)
					separations.insert(extremum.first, extremum.second);
			}
		}

		prevDist = dist;
		prevSgn = sgn;
		jd += step;
	}

	return separations;
}

bool AstroCalcDialog::findPrecise(QPair<double, double>* out, PlanetP object1, PlanetP object2, double JD,
  double step, int prevSign, bool opposition)
{
	int sgn;
	double dist, prevDist;

	if (out == Q_NULLPTR)
		return false;

	prevDist = findDistance(JD, object1, object2, opposition);
	step = -step / 2.f;
	prevSign = -prevSign;

	while (true)
	{
		JD += step;
		dist = findDistance(JD, object1, object2, opposition);

		if (qAbs(step) < 1.f / 1440.f)
		{
			out->first = JD - step / 2.0;
			out->second = findDistance(JD - step / 2.0, object1, object2, opposition);
			if (out->second < findDistance(JD - 5.0, object1, object2, opposition))
				return true;
			else
				return false;
		}
		sgn = StelUtils::sign(dist - prevDist);
		if (sgn != prevSign)
		{
			step = -step / 2.0;
			sgn = -sgn;
		}
		prevDist = dist;
		prevSign = sgn;
	}
}

double AstroCalcDialog::findDistance(double JD, PlanetP object1, PlanetP object2, bool opposition)
{
	core->setJD(JD);
	core->update(0);
	Vec3d obj1 = object1->getJ2000EquatorialPos(core);
	Vec3d obj2 = object2->getJ2000EquatorialPos(core);
	double angle = obj1.angle(obj2);
	if (opposition)
		angle = M_PI - angle;
	return angle;
}

void AstroCalcDialog::fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const NebulaP object2)
{
	QMap<double, double>::ConstIterator it;
	for (it = list.constBegin(); it != list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		bool occultation = false;
		if (separation < (object2->getAngularSize(core) * M_PI / 180.) || separation < (object1->getSpheroidAngularSize(core) * M_PI / 180.))
		{
			phenomenType = q_("Occultation");
			occultation = true;
		}

		ACPhenTreeWidgetItem* treeItem = new ACPhenTreeWidgetItem(ui->phenomenaTreeWidget);
		treeItem->setText(PhenomenaType, phenomenType);
		// local date and time
		treeItem->setText(PhenomenaDate,
		  QString("%1 %2").arg(localeMgr->getPrintableDateLocal(it.key()), localeMgr->getPrintableTimeLocal(it.key())));
		treeItem->setData(PhenomenaDate, Qt::UserRole, it.key());
		treeItem->setText(PhenomenaObject1, object1->getNameI18n());
		if (!object2->getNameI18n().isEmpty())
			treeItem->setText(PhenomenaObject2, object2->getNameI18n());
		else
			treeItem->setText(PhenomenaObject2, object2->getDSODesignation());
		if (occultation)
			treeItem->setText(PhenomenaSeparation, QChar(0x2014));
		else
			treeItem->setText(PhenomenaSeparation, StelUtils::radToDmsStr(separation));
	}
}

QMap<double, double> AstroCalcDialog::findClosestApproach(PlanetP& object1, NebulaP& object2, double startJD, double stopJD, double maxSeparation)
{
	double dist, prevDist, step, step0;
	int sgn, prevSgn = 0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	step0 = (stopJD - startJD) / 8.0;
	if (step0 > 24.8 * 365.25)
		step0 = 24.8 * 365.25;

	if (object1->getEnglishName() == "Neptune" || object1->getEnglishName() == "Uranus")
	{
		if (step0 > 3652.5)
			step0 = 3652.5;
	}
	if (object1->getEnglishName() == "Jupiter" || object1->getEnglishName() == "Saturn")
	{
		if (step0 > 365.25)
			step0 = 365.f;
	}
	if (object1->getEnglishName() == "Mars")
	{
		if (step0 > 10.f)
			step0 = 10.f;
	}
	if (object1->getEnglishName() == "Venus" || object1->getEnglishName() == "Mercury")
	{
		if (step0 > 5.f)
			step0 = 5.f;
	}
	if (object1->getEnglishName() == "Moon")
	{
		if (step0 > 0.25)
			step0 = 0.25;
	}

	step = step0;
	double jd = startJD;
	prevDist = findDistance(jd, object1, object2);
	jd += step;
	while (jd <= stopJD)
	{
		dist = findDistance(jd, object1, object2);
		sgn = StelUtils::sign(dist - prevDist);

		double factor = qAbs((dist - prevDist) / dist);
		if (factor > 10.f)
			step = step0 * factor / 10.f;
		else
			step = step0;

		if (sgn != prevSgn && prevSgn == -1)
		{
			if (step > step0)
			{
				jd -= step;
				step = step0;
				sgn = prevSgn;
				while (jd <= stopJD)
				{
					dist = findDistance(jd, object1, object2);
					sgn = StelUtils::sign(dist - prevDist);
					if (sgn != prevSgn)
						break;

					prevDist = dist;
					prevSgn = sgn;
					jd += step;
				}
			}

			if (findPrecise(&extremum, object1, object2, jd, step, sgn))
			{
				double sep = extremum.second * 180. / M_PI;
				if (sep < maxSeparation) separations.insert(extremum.first, extremum.second);
			}
		}

		prevDist = dist;
		prevSgn = sgn;
		jd += step;
	}

	return separations;
}

bool AstroCalcDialog::findPrecise(QPair<double, double>* out, PlanetP object1, NebulaP object2, double JD, double step, int prevSign)
{
	int sgn;
	double dist, prevDist;

	if (out == Q_NULLPTR)
		return false;

	prevDist = findDistance(JD, object1, object2);
	step = -step / 2.f;
	prevSign = -prevSign;

	while (true)
	{
		JD += step;
		dist = findDistance(JD, object1, object2);

		if (qAbs(step) < 1.f / 1440.f)
		{
			out->first = JD - step / 2.0;
			out->second = findDistance(JD - step / 2.0, object1, object2);
			if (out->second < findDistance(JD - 5.0, object1, object2))
				return true;
			else
				return false;
		}
		sgn = StelUtils::sign(dist - prevDist);
		if (sgn != prevSign)
		{
			step = -step / 2.0;
			sgn = -sgn;
		}
		prevDist = dist;
		prevSign = sgn;
	}
}

double AstroCalcDialog::findDistance(double JD, PlanetP object1, NebulaP object2)
{
	core->setJD(JD);
	core->update(0);
	Vec3d obj1 = object1->getJ2000EquatorialPos(core);
	Vec3d obj2 = object2->getJ2000EquatorialPos(core);
	return obj1.angle(obj2);
}

void AstroCalcDialog::fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const StelObjectP object2)
{
	QMap<double, double>::ConstIterator it;
	for (it = list.constBegin(); it != list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		bool occultation = false;
		if (separation < (object2->getAngularSize(core) * M_PI / 180.) || separation < (object1->getSpheroidAngularSize(core) * M_PI / 180.))
		{
			phenomenType = q_("Occultation");
			occultation = true;
		}

		ACPhenTreeWidgetItem* treeItem = new ACPhenTreeWidgetItem(ui->phenomenaTreeWidget);
		treeItem->setText(PhenomenaType, phenomenType);
		// local date and time
		treeItem->setText(PhenomenaDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(it.key()), localeMgr->getPrintableTimeLocal(it.key())));
		treeItem->setData(PhenomenaDate, Qt::UserRole, it.key());
		treeItem->setText(PhenomenaObject1, object1->getNameI18n());
		treeItem->setText(PhenomenaObject2, object2->getNameI18n());
		if (occultation)
			treeItem->setText(PhenomenaSeparation, QChar(0x2014));
		else
			treeItem->setText(PhenomenaSeparation, StelUtils::radToDmsStr(separation));
	}
}

QMap<double, double> AstroCalcDialog::findClosestApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD, double maxSeparation)
{
	double dist, prevDist, step, step0;
	int sgn, prevSgn = 0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	step0 = (stopJD - startJD) / 8.0;
	if (step0 > 24.8 * 365.25)
		step0 = 24.8 * 365.25;

	if (object1->getEnglishName() == "Neptune" || object1->getEnglishName() == "Uranus")
	{
		if (step0 > 1811.25)
			step0 = 1811.25;
	}
	if (object1->getEnglishName() == "Jupiter" || object1->getEnglishName() == "Saturn")
	{
		if (step0 > 181.125)
			step0 = 181.125;
	}
	if (object1->getEnglishName() == "Mars")
	{
		if (step0 > 5.f)
			step0 = 5.0;
	}
	if (object1->getEnglishName() == "Venus" || object1->getEnglishName() == "Mercury")
	{
		if (step0 > 2.5f)
			step0 = 2.5;
	}
	if (object1->getEnglishName() == "Moon")
	{
		if (step0 > 0.25)
			step0 = 0.25;
	}

	step = step0;
	double jd = startJD;
	prevDist = findDistance(jd, object1, object2);
	jd += step;
	while (jd <= stopJD)
	{
		dist = findDistance(jd, object1, object2);
		sgn = StelUtils::sign(dist - prevDist);

		double factor = qAbs((dist - prevDist) / dist);
		if (factor > 10.f)
			step = step0 * factor / 10.f;
		else
			step = step0;

		if (sgn != prevSgn && prevSgn == -1)
		{
			if (step > step0)
			{
				jd -= step;
				step = step0;
				sgn = prevSgn;
				while (jd <= stopJD)
				{
					dist = findDistance(jd, object1, object2);
					sgn = StelUtils::sign(dist - prevDist);
					if (sgn != prevSgn)
						break;

					prevDist = dist;
					prevSgn = sgn;
					jd += step;
				}
			}

			if (findPrecise(&extremum, object1, object2, jd, step, sgn))
			{
				double sep = extremum.second * 180. / M_PI;
				if (sep < maxSeparation)
					separations.insert(extremum.first, extremum.second);
			}
		}

		prevDist = dist;
		prevSgn = sgn;
		jd += step;
	}

	return separations;
}

bool AstroCalcDialog::findPrecise(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double step, int prevSign)
{
	int sgn;
	double dist, prevDist;

	if (out == Q_NULLPTR)
		return false;

	prevDist = findDistance(JD, object1, object2);
	step = -step / 2.f;
	prevSign = -prevSign;

	while (true)
	{
		JD += step;
		dist = findDistance(JD, object1, object2);

		if (qAbs(step) < 1.f / 1440.f)
		{
			out->first = JD - step / 2.0;
			out->second = findDistance(JD - step / 2.0, object1, object2);
			if (out->second < findDistance(JD - 5.0, object1, object2))
				return true;
			else
				return false;
		}
		sgn = StelUtils::sign(dist - prevDist);
		if (sgn != prevSign)
		{
			step = -step / 2.0;
			sgn = -sgn;
		}
		prevDist = dist;
		prevSign = sgn;
	}
}

double AstroCalcDialog::findDistance(double JD, PlanetP object1, StelObjectP object2)
{
	core->setJD(JD);
	core->update(0);
	Vec3d obj1 = object1->getJ2000EquatorialPos(core);
	Vec3d obj2 = object2->getJ2000EquatorialPos(core);
	return obj1.angle(obj2);
}

void AstroCalcDialog::changePage(QListWidgetItem* current, QListWidgetItem* previous)
{
	if (!current)
		current = previous;

	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));

	// special case
	if (ui->stackListWidget->row(current) == 0)
		currentCelestialPositions();

	// special case - plot the graph when tab 'Alt. vs Time' is visible
	if (ui->stackListWidget->row(current) == 3)
	{
		plotAltVsTime = true;
		drawAltVsTimeDiagram(); // Is object already selected?
	}
	else
		plotAltVsTime = false;

	// special case - plot the graph when tab 'Monthly Elevation' is visible
	if (ui->stackListWidget->row(current) == 4)
	{
		plotMonthlyElevation = true;
		drawMonthlyElevationGraph(); // Is object already selected?
	}
	else
		plotMonthlyElevation = false;

	// special case (PCalc)
	if (ui->stackListWidget->row(current) == 7)
		computePlanetaryData();
}

void AstroCalcDialog::updateTabBarListWidgetWidth()
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
		width += 24;										  // margin - 12px left and 12px right
	}

	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
}

void AstroCalcDialog::updateSolarSystemData()
{
	if (dialog)
	{
		populateCelestialBodyList();
		populateGroupCelestialBodyList();
		currentCelestialPositions();
		calculateWutObjects();
	}
}

void AstroCalcDialog::populateTimeIntervalsList()
{
	Q_ASSERT(ui->wutComboBox);

	QComboBox* wut = ui->wutComboBox;
	wut->blockSignals(true);
	int index = wut->currentIndex();
	QVariant selectedIntervalId = wut->itemData(index);

	wut->clear();
	wut->addItem(qc_("In the Evening", "Celestial object is observed..."), "0");
	wut->addItem(qc_("In the Morning", "Celestial object is observed..."), "1");
	wut->addItem(qc_("Around Midnight", "Celestial object is observed..."), "2");
	wut->addItem(qc_("In Any Time of the Night", "Celestial object is observed..."), "3");

	index = wut->findData(selectedIntervalId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index < 0)
		index = wut->findData(conf->value("astrocalc/wut_time_interval", "0").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	wut->setCurrentIndex(index);
	wut->model()->sort(0);
	wut->blockSignals(false);
}

void AstroCalcDialog::populateWutGroups()
{
	Q_ASSERT(ui->wutCategoryListWidget);

	QListWidget* category = ui->wutCategoryListWidget;
	category->blockSignals(true);

	wutCategories.clear();
	wutCategories.insert(q_("Planets"), 0);
	wutCategories.insert(q_("Bright stars"), 1);
	wutCategories.insert(q_("Bright nebulae"), 2);
	wutCategories.insert(q_("Dark nebulae"), 3);
	wutCategories.insert(q_("Galaxies"), 4);
	wutCategories.insert(q_("Star clusters"), 5);
	wutCategories.insert(q_("Asteroids"), 6);
	wutCategories.insert(q_("Comets"), 7);
	wutCategories.insert(q_("Plutinos"), 8);
	wutCategories.insert(q_("Dwarf planets"), 9);
	wutCategories.insert(q_("Cubewanos"), 10);
	wutCategories.insert(q_("Scattered disc objects"), 11);
	wutCategories.insert(q_("Oort cloud objects"), 12);
	wutCategories.insert(q_("Sednoids"), 13);
	wutCategories.insert(q_("Planetary nebulae"), 14);
	wutCategories.insert(q_("Bright double stars"), 15);
	wutCategories.insert(q_("Bright variable stars"), 16);
	wutCategories.insert(q_("Bright stars with high proper motion"), 17);
	wutCategories.insert(q_("Symbiotic stars"), 18);
	wutCategories.insert(q_("Emission-line stars"), 19);
	wutCategories.insert(q_("Supernova candidates"), 20);
	wutCategories.insert(q_("Supernova remnant candidates"), 21);
	wutCategories.insert(q_("Supernova remnants"), 22);
	wutCategories.insert(q_("Clusters of galaxies"), 23);

	category->clear();
	category->addItems(wutCategories.keys());
	category->sortItems(Qt::AscendingOrder);

	category->blockSignals(false);
}

void AstroCalcDialog::saveWutMagnitudeLimit(double mag)
{
	conf->setValue("astrocalc/wut_magnitude_limit", QString::number(mag, 'f', 2));
	calculateWutObjects();
}

void AstroCalcDialog::saveWutTimeInterval(int index)
{
	Q_ASSERT(ui->wutComboBox);
	QComboBox* wutTimeInterval = ui->wutComboBox;
	conf->setValue("astrocalc/wut_time_interval", wutTimeInterval->itemData(index).toInt());

	// Calculate WUT objects!
	calculateWutObjects();
}

void AstroCalcDialog::calculateWutObjects()
{
	ui->wutMatchingObjectsListWidget->clear();
	if (ui->wutCategoryListWidget->currentItem())
	{
		QString categoryName = ui->wutCategoryListWidget->currentItem()->text();
		int categoryId = wutCategories.value(categoryName);

		wutObjects.clear();

		QList<PlanetP> allObjects = solarSystem->getAllPlanets();
		QVector<NebulaP> allDSO = dsoMgr->getAllDeepSkyObjects();
		QList<StelObjectP> hipStars = starMgr->getHipparcosStars();
		QList<StelACStarData> dblHipStars = starMgr->getHipparcosDoubleStars();
		QList<StelACStarData> varHipStars = starMgr->getHipparcosVariableStars();
		QList<StelACStarData> hpmHipStars = starMgr->getHipparcosHighPMStars();

		const Nebula::TypeGroup& tflags = dsoMgr->getTypeFilters();

		double magLimit = ui->wutMagnitudeDoubleSpinBox->value();
		double JD = core->getJD();
		double wutJD = (int)JD;
		double az, alt;

		// Dirty hack to calculate sunrise/sunset
		// FIXME: This block of code should be replaced in future!
		PlanetP sun = GETSTELMODULE(SolarSystem)->getSun();
		double sunset = -1, sunrise = -1, midnight = -1, lc = 100.0;
		bool flag = false;
		for (int i = 0; i < 288; i++) // Check position every 5 minutes...
		{
			wutJD = (int)JD + i * 0.0034722;
			core->setJD(wutJD);
			core->update(0);
			StelUtils::rectToSphe(&az, &alt, sun->getAltAzPosAuto(core));
			alt = std::fmod(alt, 2.0 * M_PI) * 180. / M_PI;
			if (alt >= -7 && alt <= -5 && !flag)
			{
				sunset = wutJD;
				flag = true;
			}
			if (alt >= -7 && alt <= -5 && flag)
				sunrise = wutJD;

			if (alt < lc)
			{
				midnight = wutJD;
				lc = alt;
			}
		}
		core->setJD(JD);

		QList<double> wutJDList;
		wutJDList.clear();

		QComboBox* wut = ui->wutComboBox;
		switch (wut->itemData(wut->currentIndex()).toInt())
		{
			case 1: // Morning
				wutJDList << sunrise;
				break;
			case 2: // Night
				wutJDList << midnight;
				break;
			case 3:
				wutJDList << sunrise << midnight << sunset;
				break;
			default: // Evening
				wutJDList << sunset;
				break;
		}

		for (int i = 0; i < wutJDList.count(); i++)
		{
			core->setJD(wutJDList.at(i));
			core->update(0);

			switch (categoryId)
			{
				case 1: // Bright stars
					foreach (const StelObjectP& object, hipStars)
					{
						if (object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 2: // Bright nebulae
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						if ((bool)(tflags & Nebula::TypeBrightNebulae)
						    && (ntype == Nebula::NebN || ntype == Nebula::NebBn || ntype == Nebula::NebEn || ntype == Nebula::NebRn || ntype == Nebula::NebHII || ntype == Nebula::NebISM || ntype == Nebula::NebCn || ntype == Nebula::NebSNR)
						    && object->getVMagnitudeWithExtinction(core) <= magLimit	&& object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				case 3: // Dark nebulae
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						if ((bool)(tflags & Nebula::TypeDarkNebulae)
						    && (ntype == Nebula::NebDn || ntype == Nebula::NebMolCld	 || ntype == Nebula::NebYSO)
						    && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				case 4: // Galaxies
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						if ((bool)(tflags & Nebula::TypeGalaxies)
						    && (ntype == Nebula::NebGx || ntype == Nebula::NebAGx || ntype == Nebula::NebRGx || ntype == Nebula::NebQSO || ntype == Nebula::NebPossQSO || ntype == Nebula::NebBLL || ntype == Nebula::NebBLA || ntype == Nebula::NebIGx)
						    && object->getVMagnitudeWithExtinction(core) <= magLimit
						    && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				case 5: // Star clusters
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						if ((bool)(tflags & Nebula::TypeStarClusters)
						    && (ntype == Nebula::NebCl || ntype == Nebula::NebOc || ntype == Nebula::NebGc || ntype == Nebula::NebSA || ntype == Nebula::NebSC || ntype == Nebula::NebCn)
						    && object->getVMagnitudeWithExtinction(core) <= magLimit
						    && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				case 6: // Asteroids
					foreach (const PlanetP& object, allObjects)
					{
						if (object->getPlanetType() == Planet::isAsteroid && object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 7: // Comets
					foreach (const PlanetP& object, allObjects)
					{
						if (object->getPlanetType() == Planet::isComet && object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 8: // Plutinos
					foreach (const PlanetP& object, allObjects)
					{
						if (object->getPlanetType() == Planet::isPlutino && object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 9: // Dwarf planets
					foreach (const PlanetP& object, allObjects)
					{
						if (object->getPlanetType() == Planet::isDwarfPlanet && object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 10: // Cubewanos
					foreach (const PlanetP& object, allObjects)
					{
						if (object->getPlanetType() == Planet::isCubewano && object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 11: // Scattered disc objects
					foreach (const PlanetP& object, allObjects)
					{
						if (object->getPlanetType() == Planet::isSDO && object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 12: // Oort cloud objects
					foreach (const PlanetP& object, allObjects)
					{
						if (object->getPlanetType() == Planet::isOCO && object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 13: // Sednoids
					foreach (const PlanetP& object, allObjects)
					{
						if (object->getPlanetType() == Planet::isSednoid && object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 14: // Planetary nebulae
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						if ((bool)(tflags & Nebula::TypePlanetaryNebulae)
						    && (ntype == Nebula::NebPn || ntype == Nebula::NebPossPN || ntype == Nebula::NebPPN)
						    && object->getVMagnitudeWithExtinction(core) <= magLimit
						    && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				case 15: // Bright double stars
					foreach (const StelACStarData& dblStar, dblHipStars)
					{
						StelObjectP object = dblStar.firstKey();
						if (object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 16: // Bright variale stars
					foreach (const StelACStarData& varStar, varHipStars)
					{
						StelObjectP object = varStar.firstKey();
						if (object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 17: // Bright stars with high proper motion
					foreach (const StelACStarData& hpmStar, hpmHipStars)
					{
						StelObjectP object = hpmStar.firstKey();
						if (object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
				case 18: // Symbiotic stars
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						if ((bool)(tflags & Nebula::TypeOther) && (ntype == Nebula::NebSymbioticStar)
						    && object->getVMagnitudeWithExtinction(core) <= magLimit
						    && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				case 19: // Emission-line stars
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						if ((bool)(tflags & Nebula::TypeOther) && (ntype == Nebula::NebEmissionLineStar)
						    && (object->getVMagnitudeWithExtinction(core) <= magLimit)
						    && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				case 20: // Supernova candidates
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						bool visible = ((object->getVMagnitudeWithExtinction(core) <= magLimit) || (object->getVMagnitude(core) > 90.f && magLimit >= 19.f));
						if ((bool)(tflags & Nebula::TypeSupernovaRemnants) && (ntype == Nebula::NebSNC) && visible && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				case 21: // Supernova remnant candidates
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						bool visible = ((object->getVMagnitudeWithExtinction(core) <= magLimit) || (object->getVMagnitude(core) > 90.f && magLimit >= 19.f));
						if ((bool)(tflags & Nebula::TypeSupernovaRemnants) && (ntype == Nebula::NebSNRC) && visible && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				case 22: // Supernova remnants
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						bool visible = ((object->getVMagnitudeWithExtinction(core) <= magLimit) || (object->getVMagnitude(core) > 90.f && magLimit >= 19.f));
						if ((bool)(tflags & Nebula::TypeSupernovaRemnants) && (ntype == Nebula::NebSNR) && visible && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				case 23: // Clusters of galaxies
					foreach (const NebulaP& object, allDSO)
					{
						Nebula::NebulaType ntype = object->getDSOType();
						if ((bool)(tflags & Nebula::TypeGalaxyClusters) && (ntype == Nebula::NebGxCl)
						    && object->getVMagnitudeWithExtinction(core) <= magLimit
						    && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if (d.isEmpty() && n.isEmpty())
								continue;

							if (d.isEmpty())
								wutObjects.insert(n, n);
							else if (n.isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(QString("%1 (%2)").arg(d, n), d);
						}
					}
					break;
				default: // Planets
					foreach (const PlanetP& object, allObjects)
					{
						if (object->getPlanetType() == Planet::isPlanet && object->getVMagnitudeWithExtinction(core) <= magLimit && object->isAboveRealHorizon(core))
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
					break;
			}
		}

		core->setJD(JD);
		ui->wutMatchingObjectsListWidget->blockSignals(true);
		ui->wutMatchingObjectsListWidget->clear();
		ui->wutMatchingObjectsListWidget->addItems(wutObjects.keys());
		ui->wutMatchingObjectsListWidget->sortItems(Qt::AscendingOrder);
		ui->wutMatchingObjectsListWidget->blockSignals(false);
	}
}

void AstroCalcDialog::selectWutObject()
{
	if (ui->wutMatchingObjectsListWidget->currentItem())
	{
		QString wutObjectEnglisName = wutObjects.value(ui->wutMatchingObjectsListWidget->currentItem()->text());
		if (objectMgr->findAndSelectI18n(wutObjectEnglisName) || objectMgr->findAndSelect(wutObjectEnglisName))
		{
			const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
			if (!newSelected.empty())
			{
				// Can't point to home planet
				if (newSelected[0]->getEnglishName() != core->getCurrentLocation().planetName)
				{
					mvMgr->moveToObject(newSelected[0], mvMgr->getAutoMoveDuration());
					mvMgr->setFlagTracking(true);
				}
				else
				{
					GETSTELMODULE(StelObjectMgr)->unSelect();
				}
			}
		}
	}
}

void AstroCalcDialog::saveWutObjects()
{
	QString filter = q_("Text file");
	filter.append(" (*.txt)");
	QString filePath = QFileDialog::getSaveFileName(0, q_("Save list of objects as..."), QDir::homePath() + "/wut-objects.txt", filter);
	QFile objlist(filePath);
	if (!objlist.open(QFile::WriteOnly | QFile::Truncate))
	{
		qWarning() << "AstroCalc: Unable to open file" << QDir::toNativeSeparators(filePath);
		return;
	}

	QTextStream wutObjList(&objlist);
	wutObjList.setCodec("UTF-8");

	for (int row = 0; row < ui->wutMatchingObjectsListWidget->count(); ++row)
	{
		wutObjList << ui->wutMatchingObjectsListWidget->item(row)->text() << acEndl;
	}

	objlist.close();
}

void AstroCalcDialog::saveFirstCelestialBody(int index)
{
	Q_ASSERT(ui->firstCelestialBodyComboBox);
	QComboBox* celestialBody = ui->firstCelestialBodyComboBox;
	conf->setValue("astrocalc/first_celestial_body", celestialBody->itemData(index).toString());

	computePlanetaryData();
}

void AstroCalcDialog::saveSecondCelestialBody(int index)
{
	Q_ASSERT(ui->secondCelestialBodyComboBox);
	QComboBox* celestialBody = ui->secondCelestialBodyComboBox;
	conf->setValue("astrocalc/second_celestial_body", celestialBody->itemData(index).toString());

	computePlanetaryData();
}

void AstroCalcDialog::computePlanetaryData()
{
	Q_ASSERT(ui->firstCelestialBodyComboBox);
	Q_ASSERT(ui->secondCelestialBodyComboBox);

	QComboBox* fbody = ui->firstCelestialBodyComboBox;
	QComboBox* sbody = ui->secondCelestialBodyComboBox;

	QString firstCelestialBody = fbody->currentData(Qt::UserRole).toString();
	QString secondCelestialBody = sbody->currentData(Qt::UserRole).toString();
	QString currentPlanet = core->getCurrentPlanet()->getEnglishName();

	PlanetP firstCBId = solarSystem->searchByEnglishName(firstCelestialBody);
	Vec3d posFCB = firstCBId->getJ2000EquatorialPos(core);
	PlanetP secondCBId = solarSystem->searchByEnglishName(secondCelestialBody);
	Vec3d posSCB = secondCBId->getJ2000EquatorialPos(core);

	double distanceAu = (posFCB - posSCB).length();
	double distanceKm = AU * distanceAu;
	// TRANSLATORS: Unit of measure for distance - kilometers
	QString km = qc_("km", "distance");
	// TRANSLATORS: Unit of measure for distance - milliones kilometers
	QString Mkm = qc_("M km", "distance");
	QString distAU, distKM;
	bool useKM = true;
	if (distanceAu < 0.1)
	{
		distAU = QString::number(distanceAu, 'f', 5);
		distKM = QString::number(distanceKm, 'f', 3);
		useKM = true;
	}
	else
	{
		distAU = QString::number(distanceAu, 'f', 5);
		distKM = QString::number(distanceKm / 1.0e6, 'f', 3);
		useKM = false;
	}

	double r = std::acos(sin(posFCB.latitude()) * sin(posSCB.latitude()) + cos(posFCB.latitude()) * cos(posSCB.latitude()) * cos(posFCB.longitude() - posSCB.longitude()));

	unsigned int d, m;
	double s, dd;
	bool sign;

	StelUtils::radToDms(r, sign, d, m, s);
	StelUtils::radToDecDeg(r, sign, dd);

	double spcb1 = firstCBId->getSiderealPeriod();
	double spcb2 = secondCBId->getSiderealPeriod();
	int cb1 = std::round(spcb1);
	int cb2 = std::round(spcb2);
	QString orbitalResonance = QChar(0x2014);
	bool spin = false;
	QString parentFCBName = "";
	if (firstCelestialBody != "Sun")
		parentFCBName = firstCBId->getParent()->getEnglishName();
	QString parentSCBName = "";
	if (secondCelestialBody != "Sun")
		parentSCBName = secondCBId->getParent()->getEnglishName();

	if (firstCelestialBody == parentSCBName)
	{
		cb1 = std::round(secondCBId->getSiderealPeriod());
		cb2 = std::round(secondCBId->getSiderealDay());
		spin = true;
	}
	else if (secondCelestialBody == parentFCBName)
	{
		cb1 = std::round(firstCBId->getSiderealPeriod());
		cb2 = std::round(firstCBId->getSiderealDay());
		spin = true;
	}
	int gcd = StelUtils::gcd(cb1, cb2);

	QString distanceUM = qc_("AU", "distance, astronomical unit");
	ui->labelLinearDistanceValue->setText(QString("%1 %2 (%3 %4)").arg(distAU).arg(distanceUM).arg(distKM).arg(useKM ? km : Mkm));

	QString angularDistance = QChar(0x2014);
	if (firstCelestialBody != currentPlanet && secondCelestialBody != currentPlanet)
		angularDistance = QString("%1%2 %3' %4\" (%5%2)").arg(d).arg(QChar(0x00B0)).arg(m).arg(s, 0, 'f', 2).arg(dd, 0, 'f', 5);
	ui->labelAngularDistanceValue->setText(angularDistance);

	if (cb1 > 0 && cb2 > 0)
	{
		orbitalResonance = QString("%1:%2").arg(qAbs(std::round(cb1 / gcd))).arg(qAbs(std::round(cb2 / gcd))); // Very accurate resonances!
		if (spin)
			orbitalResonance.append(QString(" (%1)").arg(q_("spin-orbit resonance")));
	}

	ui->labelOrbitalResonanceValue->setText(orbitalResonance);

	// TRANSLATORS: Unit of measure for speed - kilometers per second
	QString kms = qc_("km/s", "speed");

	double orbVelFCB = firstCBId->getEclipticVelocity().length();
	QString orbitalVelocityFCB = QChar(0x2014);
	if (orbVelFCB > 0.)
		orbitalVelocityFCB = QString("%1 %2").arg(QString::number(orbVelFCB * AU/86400., 'f', 3)).arg(kms);

	ui->labelOrbitalVelocityFCBValue->setText(orbitalVelocityFCB);

	double orbVelSCB = secondCBId->getEclipticVelocity().length();
	QString orbitalVelocitySCB = QChar(0x2014);
	if (orbVelSCB>0.)
		orbitalVelocitySCB = QString("%1 %2").arg(QString::number(orbVelSCB * AU/86400., 'f', 3)).arg(kms);

	ui->labelOrbitalVelocitySCBValue->setText(orbitalVelocitySCB);

	// TRANSLATORS: Unit of measure for period - days
	QString days = qc_("days", "duration");
	QString synodicPeriod = QChar(0x2014);
	bool showSP = true;
	if (firstCelestialBody == secondCelestialBody || firstCelestialBody == "Sun" || secondCelestialBody == "Sun")
		showSP = false;
	if ((firstCBId->getPlanetTypeString()=="moon" && parentFCBName!=secondCelestialBody) || (secondCBId->getPlanetTypeString()=="moon" && parentSCBName!=firstCelestialBody))
		showSP = false;
	if (spcb1 > 0.0 && spcb2 > 0.0 && showSP)
	{
		double sp = qAbs(1/(1/spcb1 - 1/spcb2));
		synodicPeriod = QString("%1 %2 (%3 a)").arg(QString::number(sp, 'f', 3)).arg(days).arg(QString::number(sp/365.25, 'f', 5));
	}

	ui->labelSynodicPeriodValue->setText(synodicPeriod);
}
