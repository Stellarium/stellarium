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
#include "AngleSpinBox.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "NebulaMgr.hpp"
#include "Nebula.hpp"
#include "StelActionMgr.hpp"
#include "StelSkyCultureMgr.hpp"

#ifdef USE_STATIC_PLUGIN_SATELLITES
#include "../plugins/Satellites/src/Satellites.hpp"
#endif
#ifdef USE_STATIC_PLUGIN_QUASARS
#include "../plugins/Quasars/src/Quasars.hpp"
#endif
#ifdef USE_STATIC_PLUGIN_PULSARS
#include "../plugins/Pulsars/src/Pulsars.hpp"
#endif
#ifdef USE_STATIC_PLUGIN_EXOPLANETS
#include "../plugins/Exoplanets/src/Exoplanets.hpp"
#endif
#ifdef USE_STATIC_PLUGIN_NOVAE
#include "../plugins/Novae/src/Novae.hpp"
#endif
#ifdef USE_STATIC_PLUGIN_SUPERNOVAE
#include "../plugins/Supernovae/src/Supernovae.hpp"
#endif

#include <QFileDialog>
#include <QDir>
#include <QSortFilterProxyModel>
#include <QStringListModel>

#include "AstroCalcDialog.hpp"
#include "AstroCalcExtraEphemerisDialog.hpp"
#include "AstroCalcCustomStepsDialog.hpp"
#include "ui_astroCalcDialog.h"

#include "external/qcustomplot/qcustomplot.h"
#include "external/qxlsx/xlsxdocument.h"
#include "external/qxlsx/xlsxchartsheet.h"
#include "external/qxlsx/xlsxcellrange.h"
#include "external/qxlsx/xlsxchart.h"
#include "external/qxlsx/xlsxrichstring.h"
#include "external/qxlsx/xlsxworkbook.h"
using namespace QXlsx;

QVector<Ephemeris> AstroCalcDialog::EphemerisList;
int AstroCalcDialog::DisplayedPositionIndex = -1;
float AstroCalcDialog::brightLimit = 10.f;
double AstroCalcDialog::minY = -90.;
double AstroCalcDialog::maxY = 90.;
double AstroCalcDialog::minYme = -90.;
double AstroCalcDialog::maxYme = 90.;
double AstroCalcDialog::minYsun = -90.;
double AstroCalcDialog::maxYsun = 90.;
double AstroCalcDialog::minYmoon = -90.;
double AstroCalcDialog::maxYmoon = 90.;
double AstroCalcDialog::minY1 = -1001.;
double AstroCalcDialog::maxY1 = 1001.;
double AstroCalcDialog::minY2 = -1001.;
double AstroCalcDialog::maxY2 = 1001.;
double AstroCalcDialog::transitX = -1.;
double AstroCalcDialog::minYld = 0.;
double AstroCalcDialog::maxYld = 90.;
double AstroCalcDialog::minYad = 0.;
double AstroCalcDialog::maxYad = 180.;
double AstroCalcDialog::minYadm = 0.;
double AstroCalcDialog::maxYadm = 180.;
double AstroCalcDialog::minYaz = 0.;
double AstroCalcDialog::maxYaz = 360.;
QString AstroCalcDialog::yAxis1Legend = "";
QString AstroCalcDialog::yAxis2Legend = "";
const QString AstroCalcDialog::dash = QChar(0x2014);
const QString AstroCalcDialog::delimiter(", ");

AstroCalcDialog::AstroCalcDialog(QObject* parent)
	: StelDialog("AstroCalc", parent)
	, extraEphemerisDialog(Q_NULLPTR)
	, customStepsDialog(Q_NULLPTR)
	//, wutModel(Q_NULLPTR)
	//, proxyModel(Q_NULLPTR)
	, currentTimeLine(Q_NULLPTR)
	, plotAltVsTime(false)	
	, plotAltVsTimeSun(false)
	, plotAltVsTimeMoon(false)
	, plotAltVsTimePositive(false)
	, plotMonthlyElevation(false)
	, plotMonthlyElevationPositive(false)
	, plotDistanceGraph(false)
	, plotAngularDistanceGraph(false)
	, plotAziVsTime(false)	
	, altVsTimePositiveLimit(0)
	, monthlyElevationPositiveLimit(0)
	, graphsDuration(1)
	, oldGraphJD(0)
	, graphPlotNeedsRefresh(false)
{
	ui = new Ui_astroCalcDialogForm;
	core = StelApp::getInstance().getCore();
	solarSystem = GETSTELMODULE(SolarSystem);
	dsoMgr = GETSTELMODULE(NebulaMgr);
	objectMgr = GETSTELMODULE(StelObjectMgr);
	starMgr = GETSTELMODULE(StarMgr);
	mvMgr = GETSTELMODULE(StelMovementMgr);
	propMgr = StelApp::getInstance().getStelPropertyManager();
	localeMgr = &StelApp::getInstance().getLocaleMgr();
	conf = StelApp::getInstance().getSettings();
	ephemerisHeader.clear();
	phenomenaHeader.clear();
	positionsHeader.clear();
	wutHeader.clear();
	transitHeader.clear();

	// Allow assign shortkeys
	StelActionMgr* actionsMgr = StelApp::getInstance().getStelActionManager();
	QString actionsGroup = N_("Astronomical calculations");
	//actionsMgr->addAction("actionRun_AstroCalc_SavePositions", actionsGroup, N_("Save positions"), this, "saveCelestialPositions()", "");
	actionsMgr->addAction("actionRun_AstroCalc_UpdatePositions", actionsGroup, N_("Update positions"), this, "currentCelestialPositions()", "");
	//actionsMgr->addAction("actionRun_AstroCalc_SaveEphemeris", actionsGroup, N_("Save ephemeris"), this, "saveEphemeris()", "");
	actionsMgr->addAction("actionRun_AstroCalc_CalculateEphemeris", actionsGroup, N_("Calculate ephemeris"), this, "generateEphemeris()", "");
	actionsMgr->addAction("actionRun_AstroCalc_CleanupEphemeris", actionsGroup, N_("Cleanup ephemerides"), this, "cleanupEphemeris()", "");
	//actionsMgr->addAction("actionRun_AstroCalc_SavePhenomena", actionsGroup, N_("Save phenomena"), this, "savePhenomena()", "");
	actionsMgr->addAction("actionRun_AstroCalc_CalculatePhenomena", actionsGroup, N_("Calculate phenomena"), this, "calculatePhenomena()", "");
	actionsMgr->addAction("actionRun_AstroCalc_CleanupPhenomena", actionsGroup, N_("Cleanup of phenomena"), this, "cleanupPhenomena()", "");
	//actionsMgr->addAction("actionRun_AstroCalc_SaveTransits", actionsGroup, N_("Save transits"), this, "saveTransits()", "");
	actionsMgr->addAction("actionRun_AstroCalc_CalculateTransits", actionsGroup, N_("Calculate transits"), this, "generateTransits()", "");
	actionsMgr->addAction("actionRun_AstroCalc_CleanupTransits", actionsGroup, N_("Cleanup transits"), this, "cleanupTransits()", "");
	//actionsMgr->addAction("actionRun_AstroCalc_SaveWUTObjects", actionsGroup, N_("WUT: save list of objects"), this, "saveWutObjects()", "");
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
	delete extraEphemerisDialog;
	delete customStepsDialog;
}

void AstroCalcDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setCelestialPositionsHeaderNames();
		setEphemerisHeaderNames();
		setTransitHeaderNames();
		setPhenomenaHeaderNames();
		populateCelestialBodyList();
		populateCelestialCategoryList();
		populateEphemerisTimeStepsList();
		populatePlanetList();
		populateGroupCelestialBodyList();
		currentCelestialPositions();
		prepareAxesAndGraph();
		prepareAziVsTimeAxesAndGraph();
		populateFunctionsList();
		prepareXVsTimeAxesAndGraph();
		prepareMonthlyEleveationAxesAndGraph();
		prepareDistanceAxesAndGraph();
		prepareAngularDistanceAxesAndGraph();
		drawAltVsTimeDiagram();
		drawAziVsTimeDiagram();
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

	// Kinetic scrolling
	kineticScrollingList << ui->celestialPositionsTreeWidget << ui->ephemerisTreeWidget << ui->phenomenaTreeWidget << ui->wutCategoryListWidget;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	// Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(&StelApp::getInstance().getSkyCultureMgr(), SIGNAL(currentSkyCultureChanged(QString)), this, SLOT(populateCelestialNames(QString)));
	ui->stackedWidget->setCurrentIndex(0);
	ui->stackListWidget->setCurrentRow(0);
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	initListCelestialPositions();
	initListPhenomena();	
	populateCelestialBodyList();
	populateCelestialCategoryList();
	populateEphemerisTimeStepsList();
	populatePlanetList();
	populateGroupCelestialBodyList();
	// Altitude vs. Time feature
	prepareAxesAndGraph();
	drawCurrentTimeDiagram();
	// Azimuth vs. Time feature
	prepareAziVsTimeAxesAndGraph();
	// Graphs feature
	populateFunctionsList();
	prepareXVsTimeAxesAndGraph();
	// Monthly Elevation
	prepareMonthlyEleveationAxesAndGraph();
	// WUT
	initListWUT();
	populateTimeIntervalsList();
	populateWutGroups();
	// PC
	prepareDistanceAxesAndGraph();
	prepareAngularDistanceAxesAndGraph();

	ui->mercuryMarkerColor->setText(QChar(0x263F));
	ui->venusMarkerColor->setText(QChar(0x2640));
	ui->marsMarkerColor->setText(QChar(0x2642));
	ui->jupiterMarkerColor->setText(QChar(0x2643));
	ui->saturnMarkerColor->setText(QChar(0x2644));

	double JD = core->getJD() + core->getUTCOffset(core->getJD()) / 24;
	QDateTime currentDT = StelUtils::jdToQDateTime(JD);
	ui->dateFromDateTimeEdit->setDateTime(currentDT);
	ui->dateToDateTimeEdit->setDateTime(currentDT.addMonths(1));
	ui->phenomenFromDateEdit->setDateTime(currentDT);
	ui->phenomenToDateEdit->setDateTime(currentDT.addMonths(1));
	ui->transitFromDateEdit->setDateTime(currentDT);
	ui->transitToDateEdit->setDateTime(currentDT.addMonths(1));
	ui->monthlyElevationTimeInfo->setStyleSheet("font-size: 18pt; color: rgb(238, 238, 238);");

	// TODO: Switch a QDateTimeEdit to StelDateTimeEdit widget to apply wide range of dates
	// NOTE: https://github.com/Stellarium/stellarium/issues/711
	QDate min = QDate(1600, 1, 1);
	QString validDates = QString("%1 1600/1/1 - 9999/12/31").arg(q_("Gregorian dates. Valid range:"));
	ui->dateFromDateTimeEdit->setMinimumDate(min);
	ui->dateFromDateTimeEdit->setToolTip(validDates);
	ui->dateToDateTimeEdit->setMinimumDate(min);
	ui->dateToDateTimeEdit->setToolTip(validDates);
	ui->phenomenFromDateEdit->setMinimumDate(min);
	ui->phenomenFromDateEdit->setToolTip(validDates);
	ui->phenomenToDateEdit->setMinimumDate(min);	
	ui->phenomenToDateEdit->setToolTip(validDates);
	ui->transitFromDateEdit->setMinimumDate(min);
	ui->transitFromDateEdit->setToolTip(validDates);
	ui->transitToDateEdit->setMinimumDate(min);
	ui->transitToDateEdit->setToolTip(validDates);
	ui->pushButtonExtraEphemerisDialog->setFixedSize(QSize(20, 20));
	ui->pushButtonCustomStepsDialog->setFixedSize(QSize(26, 26));

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

	connectBoolProperty(ui->ephemerisShowLineCheckBox, "SolarSystem.ephemerisLineDisplayed");
	connectBoolProperty(ui->ephemerisShowMarkersCheckBox, "SolarSystem.ephemerisMarkersDisplayed");
	connectBoolProperty(ui->ephemerisShowDatesCheckBox, "SolarSystem.ephemerisDatesDisplayed");
	connectBoolProperty(ui->ephemerisShowMagnitudesCheckBox, "SolarSystem.ephemerisMagnitudesDisplayed");
	connectBoolProperty(ui->ephemerisHorizontalCoordinatesCheckBox, "SolarSystem.ephemerisHorizontalCoordinates");
	initListEphemeris();
	initEphemerisFlagNakedEyePlanets();
	connect(ui->ephemerisHorizontalCoordinatesCheckBox, SIGNAL(toggled(bool)), this, SLOT(reGenerateEphemeris()));
	connect(ui->allNakedEyePlanetsCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveEphemerisFlagNakedEyePlanets(bool)));
	connect(ui->ephemerisPushButton, SIGNAL(clicked()), this, SLOT(generateEphemeris()));
	connect(ui->ephemerisCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupEphemeris()));
	connect(ui->ephemerisSaveButton, SIGNAL(clicked()), this, SLOT(saveEphemeris()));
	connect(ui->ephemerisTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentEphemeride(QModelIndex)));	
	connect(ui->ephemerisTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onChangedEphemerisPosition()));
	connect(ui->ephemerisStepComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveEphemerisTimeStep(int)));
	connect(ui->celestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveEphemerisCelestialBody(int)));
	connect(ui->secondaryCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveEphemerisSecondaryCelestialBody(int)));

	connectColorButton(ui->genericMarkerColor, "SolarSystem.ephemerisGenericMarkerColor", "color/ephemeris_generic_marker_color");
	connectColorButton(ui->secondaryMarkerColor, "SolarSystem.ephemerisSecondaryMarkerColor", "color/ephemeris_secondary_marker_color");
	connectColorButton(ui->selectedMarkerColor, "SolarSystem.ephemerisSelectedMarkerColor", "color/ephemeris_selected_marker_color");
	connectColorButton(ui->mercuryMarkerColor, "SolarSystem.ephemerisMercuryMarkerColor", "color/ephemeris_mercury_marker_color");
	connectColorButton(ui->venusMarkerColor, "SolarSystem.ephemerisVenusMarkerColor", "color/ephemeris_venus_marker_color");
	connectColorButton(ui->marsMarkerColor, "SolarSystem.ephemerisMarsMarkerColor", "color/ephemeris_mars_marker_color");
	connectColorButton(ui->jupiterMarkerColor, "SolarSystem.ephemerisJupiterMarkerColor", "color/ephemeris_jupiter_marker_color");
	connectColorButton(ui->saturnMarkerColor, "SolarSystem.ephemerisSaturnMarkerColor", "color/ephemeris_saturn_marker_color");

	initListTransit();
	connect(ui->transitsCalculateButton, SIGNAL(clicked()), this, SLOT(generateTransits()));
	connect(ui->transitsCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupTransits()));
	connect(ui->transitsSaveButton, SIGNAL(clicked()), this, SLOT(saveTransits()));
	connect(ui->transitTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentTransit(QModelIndex)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(setTransitCelestialBodyName()));

	// Let's use DMS and decimal degrees as acceptable values for "Maximum allowed separation" input box
	ui->allowedSeparationSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->allowedSeparationSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
	ui->allowedSeparationSpinBox->setMinimum(0.0, true);
	ui->allowedSeparationSpinBox->setMaximum(10.0, true);
	ui->allowedSeparationSpinBox->setWrapping(false);

	ui->phenomenaOppositionCheckBox->setChecked(conf->value("astrocalc/flag_phenomena_opposition", false).toBool());
	connect(ui->phenomenaOppositionCheckBox, SIGNAL(toggled(bool)), this, SLOT(savePhenomenaOppositionFlag(bool)));
	ui->phenomenaPerihelionAphelionCheckBox->setChecked(conf->value("astrocalc/flag_phenomena_perihelion", false).toBool());
	connect(ui->phenomenaPerihelionAphelionCheckBox, SIGNAL(toggled(bool)), this, SLOT(savePhenomenaPerihelionAphelionFlag(bool)));
	ui->allowedSeparationSpinBox->setDegrees(conf->value("astrocalc/phenomena_angular_separation", 1.0).toDouble());
	connect(ui->allowedSeparationSpinBox, SIGNAL(valueChanged()), this, SLOT(savePhenomenaAngularSeparation()));

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
	ui->positiveAltitudeLimitSpinBox->setValue(conf->value("astrocalc/altvstime_positive_limit", 0).toInt());
	connect(ui->sunAltitudeCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveAltVsTimeSunFlag(bool)));
	connect(ui->moonAltitudeCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveAltVsTimeMoonFlag(bool)));
	connect(ui->positiveAltitudeOnlyCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveAltVsTimePositiveFlag(bool)));
	connect(ui->positiveAltitudeLimitSpinBox, SIGNAL(valueChanged(int)), this, SLOT(saveAltVsTimePositiveLimit(int)));
	connect(ui->altVsTimePlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseOverLine(QMouseEvent*)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawAltVsTimeDiagram()));

	connect(ui->altVsTimePlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(altTimeClick(QMouseEvent*)));
	connect(ui->aziVsTimePlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(aziTimeClick(QMouseEvent*)));

	connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(handleVisibleEnabled()));

	connect(ui->aziVsTimePlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseOverAziLine(QMouseEvent*)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawAziVsTimeDiagram()));

	connect(core, SIGNAL(dateChanged()), this, SLOT(drawCurrentTimeDiagram()));
	connect(this, SIGNAL(graphDayChanged()), this, SLOT(drawAltVsTimeDiagram()));
	connect(this, SIGNAL(graphDayChanged()), this, SLOT(drawAziVsTimeDiagram()));

	// Monthly Elevation
	plotMonthlyElevationPositive = conf->value("astrocalc/me_positive_only", false).toBool();
	ui->monthlyElevationPositiveCheckBox->setChecked(plotMonthlyElevationPositive);
	ui->monthlyElevationPositiveLimitSpinBox->setValue(conf->value("astrocalc/me_positive_limit", 0).toInt());
	ui->monthlyElevationTime->setValue(conf->value("astrocalc/me_time", 0).toInt());
	syncMonthlyElevationTime();
	connect(ui->monthlyElevationTime, SIGNAL(valueChanged(int)), this, SLOT(updateMonthlyElevationTime()));
	connect(ui->monthlyElevationPositiveCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveMonthlyElevationPositiveFlag(bool)));
	connect(ui->monthlyElevationPositiveLimitSpinBox, SIGNAL(valueChanged(int)), this, SLOT(saveMonthlyElevationPositiveLimit(int)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawMonthlyElevationGraph()));
	connect(core, SIGNAL(dateChangedByYear()), this, SLOT(drawMonthlyElevationGraph()));

	connect(ui->graphsCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveGraphsCelestialBody(int)));
	connect(ui->graphsFirstComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveGraphsFirstId(int)));
	connect(ui->graphsSecondComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveGraphsSecondId(int)));
	graphsDuration = qBound(1,conf->value("astrocalc/graphs_duration",1).toInt() ,30);
	ui->graphsDurationSpinBox->setValue(graphsDuration);
	connect(ui->graphsDurationSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateGraphsDuration(int)));
	connect(ui->drawGraphsPushButton, SIGNAL(clicked()), this, SLOT(drawXVsTimeGraphs()));

	ui->angularDistanceLimitSpinBox->setValue(conf->value("astrocalc/angular_distance_limit", 40).toInt());
	connect(ui->angularDistanceLimitSpinBox, SIGNAL(valueChanged(int)), this, SLOT(saveAngularDistanceLimit(int)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawAngularDistanceGraph()));
	connect(core, SIGNAL(dateChanged()), this, SLOT(drawAngularDistanceGraph()));

	connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(handleVisibleEnabled()));

	/*
	wutModel = new QStringListModel(this);
	proxyModel = new QSortFilterProxyModel(ui->wutMatchingObjectsListView);
	proxyModel->setSourceModel(wutModel);
	proxyModel->sort(0, Qt::AscendingOrder);
	proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui->wutMatchingObjectsListView->setModel(proxyModel);
	*/

	ui->wutAngularSizeLimitCheckBox->setChecked(conf->value("astrocalc/wut_angular_limit_flag", false).toBool());
	// Let's use DMS and decimal degrees as acceptable values for angular size limits
	ui->wutAngularSizeLimitMinSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->wutAngularSizeLimitMinSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
	ui->wutAngularSizeLimitMinSpinBox->setMinimum(0.0, true);
	ui->wutAngularSizeLimitMinSpinBox->setMaximum(10.0, true);
	ui->wutAngularSizeLimitMinSpinBox->setWrapping(false);
	ui->wutAngularSizeLimitMaxSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->wutAngularSizeLimitMaxSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
	ui->wutAngularSizeLimitMaxSpinBox->setMinimum(0.0, true);
	ui->wutAngularSizeLimitMaxSpinBox->setMaximum(10.0, true);
	ui->wutAngularSizeLimitMaxSpinBox->setWrapping(false);

	// Convert from angular minutes
	ui->wutAngularSizeLimitMinSpinBox->setDegrees(conf->value("astrocalc/wut_angular_limit_min", 10.0).toDouble()/60.0);
	ui->wutAngularSizeLimitMaxSpinBox->setDegrees(conf->value("astrocalc/wut_angular_limit_max", 600.0).toDouble()/60.0);
	connect(ui->wutAngularSizeLimitCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveWutAngularSizeFlag(bool)));
	connect(ui->wutAngularSizeLimitMinSpinBox, SIGNAL(valueChanged()), this, SLOT(saveWutMinAngularSizeLimit()));
	connect(ui->wutAngularSizeLimitMaxSpinBox, SIGNAL(valueChanged()), this, SLOT(saveWutMaxAngularSizeLimit()));

	ui->wutMagnitudeDoubleSpinBox->setValue(conf->value("astrocalc/wut_magnitude_limit", 10.0).toDouble());
	connect(ui->wutMagnitudeDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(saveWutMagnitudeLimit(double)));
	connect(ui->wutComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveWutTimeInterval(int)));
	connect(ui->wutCategoryListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(calculateWutObjects()));
	//connect(ui->wutMatchingObjectsTreeWidget->selectionModel() , SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)),
	//	this, SLOT(selectWutObject(const QModelIndex&)));
	connect(ui->wutMatchingObjectsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectWutObject(QModelIndex)));
	connect(ui->saveObjectsButton, SIGNAL(clicked()), this, SLOT(saveWutObjects()));
	//connect(ui->wutMatchingObjectsLineEdit, SIGNAL(textChanged(const QString&)), proxyModel, SLOT(setFilterWildcard(const QString&)));
	ui->wutMatchingObjectsLineEdit->setVisible(false);
	connect(dsoMgr, SIGNAL(catalogFiltersChanged(Nebula::CatalogGroup)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(typeFiltersChanged(Nebula::TypeGroup)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(flagSizeLimitsUsageChanged(bool)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(minSizeLimitChanged(double)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(maxSizeLimitChanged(double)), this, SLOT(calculateWutObjects()));

	QAction *clearAction = ui->wutMatchingObjectsLineEdit->addAction(QIcon(":/graphicGui/uieBackspaceInputButton.png"), QLineEdit::ActionPosition::TrailingPosition);
	connect(clearAction, SIGNAL(triggered()), this, SLOT(searchWutClear()));
	StelModuleMgr& moduleMgr = StelApp::getInstance().getModuleMgr();
	if (moduleMgr.isPluginLoaded("Quasars"))
	{
		#ifdef USE_STATIC_PLUGIN_QUASARS
		Quasars* qsoMgr = GETSTELMODULE(Quasars);
		connect(qsoMgr, SIGNAL(flagQuasarsVisibilityChanged(bool)), this, SLOT(calculateWutObjects()));
		#endif
	}
	if (moduleMgr.isPluginLoaded("Pulsars"))
	{
		#ifdef USE_STATIC_PLUGIN_PULSARS
		Pulsars* psrMgr = GETSTELMODULE(Pulsars);
		connect(psrMgr, SIGNAL(flagPulsarsVisibilityChanged(bool)), this, SLOT(populateWutGroups()));
		//connect(psrMgr, SIGNAL(flagPulsarsVisibilityChanged(bool)), this, SLOT(calculateWutObjects()));
		#endif
	}
	if (moduleMgr.isPluginLoaded("Exoplanets"))
	{
		#ifdef USE_STATIC_PLUGIN_EXOPLANETS
		Exoplanets* epMgr = GETSTELMODULE(Exoplanets);
		connect(epMgr, SIGNAL(flagExoplanetsVisibilityChanged(bool)), this, SLOT(populateWutGroups()));
		//connect(epMgr, SIGNAL(flagExoplanetsVisibilityChanged(bool)), this, SLOT(calculateWutObjects()));
		#endif
	}

	currentCelestialPositions();

	currentTimeLine = new QTimer(this);
	connect(currentTimeLine, SIGNAL(timeout()), this, SLOT(drawCurrentTimeDiagram()));
	connect(currentTimeLine, SIGNAL(timeout()), this, SLOT(computePlanetaryData()));
	connect(currentTimeLine, SIGNAL(timeout()), this, SLOT(drawDistanceGraph()));
	currentTimeLine->start(500); // Update 'now' line position every 0.5 seconds

	connect(ui->firstCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveFirstCelestialBody(int)));
	connect(ui->secondCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveSecondCelestialBody(int)));	
	connect(ui->pcDistanceGraphPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseOverDistanceGraph(QMouseEvent*)));

	connect(solarSystem, SIGNAL(solarSystemDataReloaded()), this, SLOT(updateSolarSystemData()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(updateAstroCalcData()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawAltVsTimeDiagram()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawMonthlyElevationGraph()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawDistanceGraph()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawAngularDistanceGraph()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(initEphemerisFlagNakedEyePlanets()));

	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(changePage(QListWidgetItem*, QListWidgetItem*)));
	connect(ui->tabWidgetGraphs, SIGNAL(currentChanged(int)), this, SLOT(changeGraphsTab(int)));
	connect(ui->tabWidgetPC, SIGNAL(currentChanged(int)), this, SLOT(changePCTab(int)));

	connect(ui->pushButtonExtraEphemerisDialog, SIGNAL(clicked()), this, SLOT(showExtraEphemerisDialog()));
	connect(ui->pushButtonCustomStepsDialog, SIGNAL(clicked()), this, SLOT(showCustomStepsDialog()));

	updateTabBarListWidgetWidth();

	// Let's improve visibility of the text
	QString style = "QLabel { color: rgb(238, 238, 238); }";
	ui->celestialPositionsTimeLabel->setStyleSheet(style);
	ui->altVsTimeLabel->setStyleSheet(style);
	ui->aziVsTimeLabel->setStyleSheet(style);
	ui->monthlyElevationLabel->setStyleSheet(style);
	ui->graphsFirstLabel->setStyleSheet(style);
	ui->graphsCelestialBodyLabel->setStyleSheet(style);
	ui->graphsSecondLabel->setStyleSheet(style);	
	ui->graphsDurationLabel->setStyleSheet(style);
	ui->graphsYearsLabel->setStyleSheet(style);
	ui->angularDistanceNote->setStyleSheet(style);
	ui->angularDistanceLimitLabel->setStyleSheet(style);	
	ui->transitNotificationLabel->setStyleSheet(style);
	style = "QCheckBox { color: rgb(238, 238, 238); }";
	ui->sunAltitudeCheckBox->setStyleSheet(style);
	ui->moonAltitudeCheckBox->setStyleSheet(style);
	ui->positiveAltitudeOnlyCheckBox->setStyleSheet(style);
	ui->monthlyElevationPositiveCheckBox->setStyleSheet(style);
}

void AstroCalcDialog::populateCelestialNames(QString)
{
	populateCelestialBodyList();
	populatePlanetList();
}

void AstroCalcDialog::showExtraEphemerisDialog()
{
	if (extraEphemerisDialog == Q_NULLPTR)
		extraEphemerisDialog = new AstroCalcExtraEphemerisDialog();

	extraEphemerisDialog->setVisible(true);
}

void AstroCalcDialog::showCustomStepsDialog()
{
	if (customStepsDialog == Q_NULLPTR)
		customStepsDialog = new AstroCalcCustomStepsDialog();

	customStepsDialog->setVisible(true);
}

void AstroCalcDialog::searchWutClear()
{
	ui->wutMatchingObjectsLineEdit->clear();	
}

void AstroCalcDialog::updateAstroCalcData()
{
	drawAltVsTimeDiagram();
	populateCelestialBodyList();
	populatePlanetList();
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

void AstroCalcDialog::saveAltVsTimePositiveLimit(int limit)
{
	if (altVsTimePositiveLimit!=limit)
	{
		altVsTimePositiveLimit = limit;
		conf->setValue("astrocalc/altvstime_positive_limit", altVsTimePositiveLimit);
		drawAltVsTimeDiagram();
	}
}

void AstroCalcDialog::prepareAziVsTimeAxesAndGraph()
{
	QString xAxisStr = q_("Local Time");
	QString yAxisStr = QString("%1, %2").arg(q_("Azimuth"), QChar(0x00B0));

	QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);

	ui->aziVsTimePlot->clearGraphs();

	// main data: Azimuth vs. Time graph
	ui->aziVsTimePlot->addGraph();
	ui->aziVsTimePlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->aziVsTimePlot->graph(0)->setPen(QPen(Qt::red, 1));
	ui->aziVsTimePlot->graph(0)->setLineStyle(QCPGraph::lsLine);
	ui->aziVsTimePlot->graph(0)->rescaleAxes(true);

	// additional data: Current Time Diagram
	ui->aziVsTimePlot->addGraph();
	ui->aziVsTimePlot->graph(1)->setPen(QPen(Qt::yellow, 1));
	ui->aziVsTimePlot->graph(1)->setLineStyle(QCPGraph::lsLine);
	ui->aziVsTimePlot->graph(1)->setName("[Now]");

	ui->aziVsTimePlot->xAxis->setLabel(xAxisStr);
	ui->aziVsTimePlot->yAxis->setLabel(yAxisStr);

	ui->aziVsTimePlot->xAxis->setRange(43200, 129600); // 24 hours since 12h00m (range in seconds)
	ui->aziVsTimePlot->xAxis->setScaleType(QCPAxis::stLinear);
	ui->aziVsTimePlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
	ui->aziVsTimePlot->xAxis->setLabelColor(axisColor);
	ui->aziVsTimePlot->xAxis->setTickLabelColor(axisColor);
	ui->aziVsTimePlot->xAxis->setBasePen(axisPen);
	ui->aziVsTimePlot->xAxis->setTickPen(axisPen);
	ui->aziVsTimePlot->xAxis->setSubTickPen(axisPen);
	ui->aziVsTimePlot->xAxis->setDateTimeFormat("H:mm");
	ui->aziVsTimePlot->xAxis->setDateTimeSpec(Qt::UTC); // Qt::UTC + core->getUTCOffset() give local time
	ui->aziVsTimePlot->xAxis->setAutoTickStep(false);
	ui->aziVsTimePlot->xAxis->setTickStep(7200); // step is 2 hours (in seconds)
	ui->aziVsTimePlot->xAxis->setAutoSubTicks(false);
	ui->aziVsTimePlot->xAxis->setSubTickCount(7);

	ui->aziVsTimePlot->yAxis->setRange(minYaz, maxYaz);
	ui->aziVsTimePlot->yAxis->setScaleType(QCPAxis::stLinear);
	ui->aziVsTimePlot->yAxis->setLabelColor(axisColor);
	ui->aziVsTimePlot->yAxis->setTickLabelColor(axisColor);
	ui->aziVsTimePlot->yAxis->setBasePen(axisPen);
	ui->aziVsTimePlot->yAxis->setTickPen(axisPen);
	ui->aziVsTimePlot->yAxis->setSubTickPen(axisPen);
}

void AstroCalcDialog::drawAziVsTimeDiagram()
{
	// Avoid crash!
	if (core->getCurrentPlanet()->getEnglishName().contains("->")) // We are on the spaceship!
		return;

	// special case - plot the graph when tab is visible
	//..
	// we got notified about a reason to redraw the plot, but dialog was
	// not visible. which means we must redraw when becoming visible again!
	if (!dialog->isVisible() && plotAziVsTime)
	{
		graphPlotNeedsRefresh = true;
		return;
	}

	if (!plotAziVsTime) return;

	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();

	if (!selectedObjects.isEmpty())
	{
		bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
		// X axis - time; Y axis - azimuth
		QList<double> aX, aY;

		StelObjectP selectedObject = selectedObjects[0];
		double currentJD = core->getJD();
		double shift = core->getUTCOffset(currentJD) / 24.0;
		double noon = static_cast<int>(currentJD + shift);
		double az, alt, deg, ltime, JD;
		bool sign;

		int step = 180;
		int limit = 485;
		bool isSatellite = false;

#ifdef USE_STATIC_PLUGIN_SATELLITES
		SatelliteP sat;		
		if (selectedObject->getType() == "Satellite") 
		{
			// get reference to satellite
			isSatellite = true;
			sat = GETSTELMODULE(Satellites)->getById(selectedObject->getInfoMap(core)["catalog"].toString());
		}
#endif

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
				sat->update(0.0);
#endif
			}
			else
				core->update(0.0);

			StelUtils::rectToSphe(&az, &alt, selectedObject->getAltAzPosAuto(core));
			double direction = 3.; // N is zero, E is 90 degrees
			if (useSouthAzimuth)
				direction = 2.;
			az = direction*M_PI - az;
			if (az > M_PI*2)
				az -= M_PI*2;
			StelUtils::radToDecDeg(az, sign, deg);
			aY.append(deg);			
		}		
		core->setJD(currentJD);

		QVector<double> x = aX.toVector(), y = aY.toVector();
		minYaz = *std::min_element(aY.begin(), aY.end()) - 2.0;
		maxYaz = *std::max_element(aY.begin(), aY.end()) + 2.0;

		prepareAziVsTimeAxesAndGraph();
		drawCurrentTimeDiagram();

		QString name = selectedObject->getNameI18n();
		if (name.isEmpty())
		{
			QString otype = selectedObject->getType();
			if (otype == "Nebula")
				name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();

			if (otype == "Star" || otype=="Pulsar")
				selectedObject->getID().isEmpty() ? name = q_("Unnamed star") : name = selectedObject->getID();
		}

		drawTransitTimeDiagram();

		ui->aziVsTimePlot->graph(0)->setData(x, y);
		ui->aziVsTimePlot->graph(0)->setName(name);
		ui->aziVsTimePlot->replot();
	}

	// clean up the data when selection is removed
	if (!objectMgr->getWasSelected())
	{
		ui->aziVsTimePlot->graph(0)->data()->clear(); // main data: Azimuth vs. Time graph		
		ui->aziVsTimePlot->replot();
	}
}

void AstroCalcDialog::mouseOverAziLine(QMouseEvent* event)
{
	double x = ui->aziVsTimePlot->xAxis->pixelToCoord(event->pos().x());
	double y = ui->aziVsTimePlot->yAxis->pixelToCoord(event->pos().y());

	QCPAbstractPlottable* abstractGraph = ui->aziVsTimePlot->plottableAt(event->pos(), false);
	QCPGraph* graph = qobject_cast<QCPGraph*>(abstractGraph);

	if (x > ui->aziVsTimePlot->xAxis->range().lower && x < ui->aziVsTimePlot->xAxis->range().upper
	    && y > ui->aziVsTimePlot->yAxis->range().lower && y < ui->aziVsTimePlot->yAxis->range().upper)
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
			else
			{
				JD = x / 86400.0 + static_cast<int>(core->getJD()) - 0.5;
				QString LT = StelUtils::jdToQDateTime(JD - core->getUTCOffset(JD)).toString("H:mm");
				if (StelApp::getInstance().getFlagShowDecimalDegrees())
					info = QString("%1<br />%2: %3<br />%4: %5%6").arg(ui->aziVsTimePlot->graph(0)->name(), q_("Local Time"), LT, q_("Azimuth"), QString::number(y, 'f', 2), QChar(0x00B0));
				else
					info = QString("%1<br />%2: %3<br />%4: %5").arg(ui->aziVsTimePlot->graph(0)->name(), q_("Local Time"), LT, q_("Azimuth"), StelUtils::decDegToDmsStr(y));
			}

			QToolTip::hideText();
			QToolTip::showText(event->globalPos(), info, ui->aziVsTimePlot, ui->aziVsTimePlot->rect());
		}
		else
			QToolTip::hideText();
	}

	ui->aziVsTimePlot->update();
	ui->aziVsTimePlot->replot();
}

void AstroCalcDialog::initListCelestialPositions()
{
	ui->celestialPositionsTreeWidget->clear();
	ui->celestialPositionsTreeWidget->setColumnCount(CColumnCount);
	setCelestialPositionsHeaderNames();
	ui->celestialPositionsTreeWidget->header()->setSectionsMovable(false);
	ui->celestialPositionsTreeWidget->header()->setDefaultAlignment(Qt::AlignHCenter);
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
		positionsHeader << q_("Opac.");
	}
	else
	{
		// TRANSLATORS: magnitude
		positionsHeader << q_("Mag.");
	}
	// TRANSLATORS: angular size, arcminutes
	positionsHeader << QString("%1, '").arg(q_("A.S."));
	if (celType == 170)
	{
		// TRANSLATORS: separation, arcseconds
		positionsHeader << QString("%1, \"").arg(q_("Sep."));
	}
	else if (celType == 171)
	{
		// TRANSLATORS: period, days
		positionsHeader << QString("%1, %2").arg(q_("Per."), qc_("d", "days"));
	}
	else if (celType >= 200)
	{
		// TRANSLATORS: distance, AU
		positionsHeader << QString("%1, %2").arg(q_("Dist."), qc_("AU", "distance, astronomical unit"));
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
	// TRANSLATORS: time of transit
	positionsHeader << qc_("Transit", "celestial event; passage across a meridian");
	// TRANSLATORS: elevation
	positionsHeader << q_("Elev.");
	// TRANSLATORS: elongation
	positionsHeader << q_("Elong.");
	// TRANSLATORS: type of object
	positionsHeader << q_("Type");

	ui->celestialPositionsTreeWidget->setHeaderLabels(positionsHeader);
	adjustCelestialPositionsColumns();
}

void AstroCalcDialog::adjustCelestialPositionsColumns()
{
	// adjust the column width
	for (int i = 0; i < CColumnCount; ++i)
	{
		ui->celestialPositionsTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::onChangedEphemerisPosition()
{
	DisplayedPositionIndex =ui->ephemerisTreeWidget->currentItem()->data(EphemerisRA, Qt::UserRole).toInt();
}

double AstroCalcDialog::computeMaxElevation(StelObjectP obj)
{
	Vec3f rts = obj->getRTSTime(core);
	const double JD = core->getJD();
	const double UTCOffset = core->getUTCOffset(JD) / 24.;
	double az, alt;
	core->setJD(static_cast<int>(JD) + static_cast<double>(rts[1]/24.f) - UTCOffset + 0.5);
	core->update(0);
	StelUtils::rectToSphe(&az, &alt, obj->getAltAzPosAuto(core));
	core->setJD(JD);
	core->update(0);
	return alt;
}

void AstroCalcDialog::populateCelestialCategoryList()
{
	Q_ASSERT(ui->celestialCategoryComboBox);

	QComboBox* category = ui->celestialCategoryComboBox;

	category->blockSignals(true);
	int index = category->currentIndex();
	QVariant selectedCategoryId = category->itemData(index);
	category->clear();

	QMap<QString,QString> map = objectMgr->objectModulesMap();
	QMapIterator<QString,QString> it(map);
	QString key;
	int kn;
	while(it.hasNext())
	{
		it.next();
		key = it.key();
		if (key.startsWith("NebulaMgr") && key.contains(":"))
			category->addItem(q_(it.value()), key.remove("NebulaMgr:"));

		if (key.startsWith("StarMgr") && key.contains(":"))
		{
			kn = key.remove("StarMgr:").toInt();
			if (kn>1 && kn<=4) // Original IDs: 2, 3, 4
				category->addItem(q_(it.value()), QString::number(kn + 168)); // AstroCalc IDs: 170, 171, 172
		}
	}
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

void AstroCalcDialog::fillCelestialPositionTable(QString objectName, QString RA, QString Dec, float magnitude,
						 QString angularSize, QString angularSizeToolTip, QString extraData,
						 QString extraDataToolTip, QString transitTime, QString maxElevation,
						 QString sElongation, QString objectType)
{
	ACCelPosTreeWidgetItem* treeItem = new ACCelPosTreeWidgetItem(ui->celestialPositionsTreeWidget);
	treeItem->setText(CColumnName, objectName);
	treeItem->setText(CColumnRA, RA);
	treeItem->setTextAlignment(CColumnRA, Qt::AlignRight);
	treeItem->setText(CColumnDec, Dec);
	treeItem->setTextAlignment(CColumnDec, Qt::AlignRight);
	if (magnitude>90.f)
		treeItem->setText(CColumnMagnitude, dash);
	else
		treeItem->setText(CColumnMagnitude, QString::number(magnitude, 'f', 2));
	treeItem->setTextAlignment(CColumnMagnitude, Qt::AlignRight);
	treeItem->setText(CColumnAngularSize, angularSize);
	treeItem->setTextAlignment(CColumnAngularSize, Qt::AlignRight);
	treeItem->setToolTip(CColumnAngularSize, angularSizeToolTip);
	treeItem->setText(CColumnExtra, extraData);
	treeItem->setTextAlignment(CColumnExtra, Qt::AlignRight);
	treeItem->setToolTip(CColumnExtra, extraDataToolTip);
	treeItem->setText(CColumnTransit, transitTime);
	treeItem->setTextAlignment(CColumnTransit, Qt::AlignRight);
	treeItem->setText(CColumnMaxElevation, maxElevation);
	treeItem->setTextAlignment(CColumnMaxElevation, Qt::AlignRight);
	treeItem->setToolTip(CColumnMaxElevation, q_("Elevation of object at moment of upper culmination"));
	treeItem->setText(CColumnElongation, sElongation);
	treeItem->setTextAlignment(CColumnElongation, Qt::AlignRight);
	treeItem->setToolTip(CColumnElongation, q_("Angular distance from the Sun at the moment of computation of position"));
	treeItem->setText(CColumnType, objectType);
}

void AstroCalcDialog::currentCelestialPositions()
{
	QString extra, angularSize, sTransit, sMaxElevation, celObjName = "", celObjId = "", elongStr = "";
	QPair<QString, QString> coordinates;

	initListCelestialPositions();

	const float mag = static_cast<float>(ui->celestialMagnitudeDoubleSpinBox->value());
	const bool horizon = ui->horizontalCoordinatesCheckBox->isChecked();
	const bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

	const double JD = core->getJD();
	ui->celestialPositionsTimeLabel->setText(q_("Positions on %1").arg(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))));
	Vec3f rts;
	Vec3d observerHelioPos;
	double angularDistance;
	PlanetP sun = solarSystem->getSun();

	Q_ASSERT(ui->celestialCategoryComboBox);
	QComboBox* category = ui->celestialCategoryComboBox;
	QString celType = category->itemData(category->currentIndex()).toString();
	const int celTypeId = celType.toInt();

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
		bool passByBrightness;
		QString dsoName;
		QString asToolTip = QString("%1, %2").arg(q_("Average angular size"), q_("arcmin"));
		// Deep-sky objects
		QList<NebulaP> celestialObjects = dsoMgr->getDeepSkyObjectsByType(celType);
		for (const auto& obj : celestialObjects)
		{
			if (celTypeId == 12 || celTypeId == 102 || celTypeId == 111) // opacity cannot be extincted
				magOp = obj->getVMagnitude(core);
			else
				magOp = obj->getVMagnitudeWithExtinction(core);

			if (celTypeId==35) // Regions of the sky
			{
				passByBrightness = true;
				magOp = 99.f;
			}
			else
				passByBrightness = (magOp <= mag);

			if (obj->objectInDisplayedCatalog() && obj->objectInAllowedSizeRangeLimits() && passByBrightness && obj->isAboveRealHorizon(core))
			{
				if (horizon)
					coordinates = getStringCoordinates(obj->getAltAzPosAuto(core), horizon, useSouthAzimuth, withDecimalDegree);
				else
					coordinates = getStringCoordinates(obj->getJ2000EquatorialPos(core), horizon, useSouthAzimuth, withDecimalDegree);

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
					extra = dash;

				// Convert to arcminutes the average angular size of deep-sky object
				angularSize = QString::number(obj->getAngularSize(core) * 120., 'f', 3);
				if (angularSize.toFloat() < 0.01f)
					angularSize = dash;

				sTransit = dash;
				sMaxElevation = dash;
				rts = obj->getRTSTime(core);
				if (rts[1]>=0.f)
				{
					sTransit = StelUtils::hoursToHmsStr(rts[1], true);
					if (withDecimalDegree)
						sMaxElevation = StelUtils::radToDecDegStr(computeMaxElevation(qSharedPointerCast<StelObject>(obj)), 5, false, true);
					else
						sMaxElevation = StelUtils::radToDmsPStr(computeMaxElevation(qSharedPointerCast<StelObject>(obj)), 2);
				}

				angularDistance = obj->getJ2000EquatorialPos(core).angle(sun->getJ2000EquatorialPos(core));
				if (withDecimalDegree)
					elongStr = StelUtils::radToDecDegStr(angularDistance, 5, false, true);
				else
					elongStr = StelUtils::radToDmsStr(angularDistance, true);

				fillCelestialPositionTable(dsoName, coordinates.first, coordinates.second, magOp, angularSize, asToolTip, extra, mu, sTransit, sMaxElevation, elongStr, q_(obj->getTypeString()));
			}
		}
	}
	else if (celTypeId >= 200 && celTypeId <= 203)
	{
		QString distanceInfo = q_("Planetocentric distance");
		if (core->getUseTopocentricCoordinates())
			distanceInfo = q_("Topocentric distance");
		QString distanceUM = qc_("AU", "distance, astronomical unit");
		QString sToolTip = QString("%1, %2").arg(distanceInfo, distanceUM);
		QString asToolTip = QString("%1, %2").arg(q_("Angular size (with rings, if any)"), q_("arcmin"));
		Vec3d pos;
		bool passByType;

		QList<PlanetP> planets;
		switch (celTypeId)
		{
			case 200:
			case 203:
				planets = solarSystem->getAllPlanets();
				break;
			case 201:
			case 202:
				planets = solarSystem->getAllMinorBodies();
				break;
		}

		for (const auto& planet : planets)
		{
			passByType = false;

			switch (celTypeId)
			{
				case 200:
					if (planet->getPlanetType() != Planet::isUNDEFINED)
						passByType = true;
					break;
				case 201:
					if (planet->getPlanetType() == Planet::isComet)
						passByType = true;
					break;
				case 202:
				{
					Planet::PlanetType ptype = planet->getPlanetType();
					if (ptype == Planet::isAsteroid || ptype == Planet::isCubewano || ptype == Planet::isDwarfPlanet || ptype == Planet::isOCO || ptype == Planet::isPlutino || ptype == Planet::isSDO || ptype == Planet::isSednoid || ptype==Planet::isInterstellar)
						passByType = true;
					break;
				}
				case 203:
					if (planet->getPlanetType() == Planet::isPlanet)
						passByType = true;
					break;
			}

			if (passByType && planet != core->getCurrentPlanet() && planet->getVMagnitudeWithExtinction(core) <= mag && planet->isAboveRealHorizon(core))
			{
				pos = planet->getJ2000EquatorialPos(core);

				if (horizon)
					coordinates = getStringCoordinates(planet->getAltAzPosAuto(core), horizon, useSouthAzimuth, withDecimalDegree);
				else
					coordinates = getStringCoordinates(pos, horizon, useSouthAzimuth, withDecimalDegree);

				extra = QString::number(pos.length(), 'f', 5); // A.U.

				// Convert to arcseconds the angular size of Solar system object (with rings, if any)
				angularSize = QString::number(planet->getAngularSize(core) * 120., 'f', 4);
				if (angularSize.toFloat() < 1e-4f || planet->getPlanetType() == Planet::isComet)
					angularSize = dash;

				sTransit = dash;
				sMaxElevation = dash;
				rts = planet->getRTSTime(core);
				if (rts[1]>=0.f)
				{
					sTransit = StelUtils::hoursToHmsStr(rts[1], true);
					if (withDecimalDegree)
						sMaxElevation = StelUtils::radToDecDegStr(computeMaxElevation(qSharedPointerCast<StelObject>(planet)), 5, false, true);
					else
						sMaxElevation = StelUtils::radToDmsPStr(computeMaxElevation(qSharedPointerCast<StelObject>(planet)), 2);
				}

				if (planet!=sun)
				{
					angularDistance = planet->getElongation(core->getObserverHeliocentricEclipticPos());
					if (withDecimalDegree)
						elongStr = StelUtils::radToDecDegStr(angularDistance, 5, false, true);
					else
						elongStr = StelUtils::radToDmsStr(angularDistance, true);
				}
				else
					elongStr = dash;

				fillCelestialPositionTable(planet->getNameI18n(), coordinates.first, coordinates.second, planet->getVMagnitudeWithExtinction(core), angularSize, asToolTip, extra, sToolTip, sTransit, sMaxElevation, elongStr, q_(planet->getPlanetTypeString()));
			}
		}
	}
	else
	{
		// stars
		QString sType = q_("star");
		QString commonName, sToolTip = "";
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

		for (const auto& star : celestialObjects)
		{
			StelObjectP obj = star.firstKey();
			if (obj->getVMagnitudeWithExtinction(core) <= mag && obj->isAboveRealHorizon(core))
			{
				if (horizon)
					coordinates = getStringCoordinates(obj->getAltAzPosAuto(core), horizon, useSouthAzimuth, withDecimalDegree);
				else
					coordinates = getStringCoordinates(obj->getJ2000EquatorialPos(core), horizon, useSouthAzimuth, withDecimalDegree);

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
						extra = dash;
				}
				else	// stars with high proper motion
					extra = QString::number(star.value(obj), 'f', 5); // "/yr

				sTransit = dash;
				sMaxElevation = dash;
				rts = obj->getRTSTime(core);
				if (rts[1]>=0.f)
				{
					sTransit = StelUtils::hoursToHmsStr(rts[1], true);
					if (withDecimalDegree)
						sMaxElevation = StelUtils::radToDecDegStr(computeMaxElevation(obj), 5, false, true);
					else
						sMaxElevation = StelUtils::radToDmsPStr(computeMaxElevation(obj), 2);
				}

				angularDistance = obj->getJ2000EquatorialPos(core).angle(sun->getJ2000EquatorialPos(core));
				if (withDecimalDegree)
					elongStr = StelUtils::radToDecDegStr(angularDistance, 5, false, true);
				else
					elongStr = StelUtils::radToDmsStr(angularDistance, true);

				commonName = obj->getNameI18n();
				if (commonName.isEmpty())
					commonName = obj->getID();

				fillCelestialPositionTable(commonName, coordinates.first, coordinates.second, obj->getVMagnitudeWithExtinction(core), dash, "", extra, sToolTip, sTransit, sMaxElevation, elongStr, sType);
			}
		}
	}

	adjustCelestialPositionsColumns();
	// sort-by-name
	ui->celestialPositionsTreeWidget->sortItems(CColumnName, Qt::AscendingOrder);
}

QPair<QString, QString> AstroCalcDialog::getStringCoordinates(const Vec3d coord, const bool horizon, const bool southAzimuth, const bool decimalDegrees)
{
	double ra, dec;
	QString raStr, decStr;
	StelUtils::rectToSphe(&ra, &dec, coord);
	if (horizon)
	{
		double direction = 3.; // N is zero, E is 90 degrees
		if (southAzimuth)
			direction = 2.;
		ra = direction * M_PI - ra;
		if (ra > M_PI * 2)
			ra -= M_PI * 2;
		if (decimalDegrees)
		{
			raStr = StelUtils::radToDecDegStr(ra, 5, false, true);
			decStr = StelUtils::radToDecDegStr(dec, 5);
		}
		else
		{
			raStr = StelUtils::radToDmsStr(ra, true);
			decStr = StelUtils::radToDmsStr(dec, true);
		}
	}
	else
	{
		if (decimalDegrees)
		{
			raStr = StelUtils::radToDecDegStr(ra, 5, false, true);
			decStr = StelUtils::radToDecDegStr(dec, 5);
		}
		else
		{
			raStr = StelUtils::radToHmsStr(ra);
			decStr = StelUtils::radToDmsStr(dec, true);
		}
	}

	QPair<QString, QString> r(raStr, decStr);

	return r;
}

void AstroCalcDialog::saveCelestialPositions()
{
	QString filter = q_("Microsoft Excel Open XML Spreadsheet");
	filter.append(" (*.xlsx);;");
	filter.append(q_("CSV (Comma delimited)"));
	filter.append(" (*.csv)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save celestial positions of objects as..."),
							QDir::homePath() + "/positions.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->celestialPositionsTreeWidget, positionsHeader);
	else
	{
		int count = ui->celestialPositionsTreeWidget->topLevelItemCount();
		int columns = positionsHeader.size();

		int *width;
		width = new int[columns];
		QString sData;
		int w;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("Celestial positions of objects"));
		xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
		xlsx.addSheet(ui->celestialCategoryComboBox->currentData(Qt::DisplayRole).toString(), AbstractSheet::ST_WorkSheet);

		QXlsx::Format header;
		header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		header.setPatternBackgroundColor(Qt::yellow);
		header.setBorderStyle(QXlsx::Format::BorderThin);
		header.setBorderColor(Qt::black);
		header.setFontBold(true);
		for (int i = 0; i < columns; i++)
		{
			// Row 1: Names of columns
			sData = positionsHeader.at(i).trimmed();
			xlsx.write(1, i + 1, sData, header);
			width[i] = sData.size();
		}

		QXlsx::Format data;
		data.setHorizontalAlignment(QXlsx::Format::AlignRight);
		for (int i = 0; i < count; i++)
		{
			for (int j = 0; j < columns; j++)
			{
				// Row 2 and next: the data
				sData = ui->celestialPositionsTreeWidget->topLevelItem(i)->text(j).trimmed();
				xlsx.write(i + 2, j + 1, sData, data);
				w = sData.size();
				if (w > width[j])
				{
					width[j] = w;
				}
			}
		}

		for (int i = 0; i < columns; i++)
		{
			xlsx.setColumnWidth(i+1, width[i]+2);
		}

		delete[] width;

		// Add the date and time info for celestial positions
		xlsx.write(count + 2, 1, ui->celestialPositionsTimeLabel->text());
		QXlsx::CellRange range = CellRange(count+2, 1, count+2, columns);
		QXlsx::Format extraData;
		extraData.setBorderStyle(QXlsx::Format::BorderThin);
		extraData.setBorderColor(Qt::black);
		extraData.setPatternBackgroundColor(Qt::yellow);
		extraData.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		xlsx.mergeCells(range, extraData);

		xlsx.saveAs(filePath);
	}
}

void AstroCalcDialog::selectCurrentCelestialPosition(const QModelIndex& modelIndex)
{
	// Find the object
	QString nameI18n = modelIndex.sibling(modelIndex.row(), CColumnName).data().toString();
	bool found = (objectMgr->findAndSelectI18n(nameI18n) || objectMgr->findAndSelect(nameI18n));

	if (!found)
	{
		QStringList list = nameI18n.split("(");
		if (list.count() > 0 && nameI18n.lastIndexOf("(") != 0 && nameI18n.lastIndexOf("/") < 0)
			nameI18n = list.at(0).trimmed();

		found = (objectMgr->findAndSelectI18n(nameI18n) || objectMgr->findAndSelect(nameI18n));
	}

	if (found)
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
	QString name = modelIndex.sibling(modelIndex.row(), EphemerisCOName).data(Qt::UserRole).toString();
	double JD = modelIndex.sibling(modelIndex.row(), EphemerisDate).data(Qt::UserRole).toDouble();

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
	ephemerisHeader << q_("Name");
	ephemerisHeader << q_("Date and Time");	
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
	ephemerisHeader << q_("Mag.");
	// TRANSLATORS: phase
	ephemerisHeader << q_("Phase");
	// TRANSLATORS: distance, AU
	ephemerisHeader << QString("%1, %2").arg(q_("Dist."), qc_("AU", "distance, astronomical unit"));
	// TRANSLATORS: elongation
	ephemerisHeader << q_("Elong.");
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
	ui->ephemerisTreeWidget->header()->setDefaultAlignment(Qt::AlignHCenter);
}

void AstroCalcDialog::reGenerateEphemeris()
{
	reGenerateEphemeris(true);
}

void AstroCalcDialog::reGenerateEphemeris(bool withSelection)
{
	if (EphemerisList.size() > 0)
	{
		int row = ui->ephemerisTreeWidget->currentIndex().row();
		generateEphemeris(); // Update list of ephemeris
		if (row>0 && withSelection)
		{
			ui->ephemerisTreeWidget->setCurrentItem(ui->ephemerisTreeWidget->topLevelItem(row), 0, QItemSelectionModel::Select | QItemSelectionModel::Rows);
			ui->ephemerisTreeWidget->setFocus();
		}
	}
	else
		initListEphemeris(); // Just update headers
}

void AstroCalcDialog::generateEphemeris()
{
	double ra, dec;
	Vec3d observerHelioPos, pos;
	QString currentPlanet = ui->celestialBodyComboBox->currentData(Qt::UserRole).toString();
	QString secondaryPlanet = ui->secondaryCelestialBodyComboBox->currentData(Qt::UserRole).toString();
	QString distanceInfo = q_("Planetocentric distance");
	if (core->getUseTopocentricCoordinates())
		distanceInfo = q_("Topocentric distance");
	QString distanceUM = qc_("AU", "distance, astronomical unit");
	QString englishName, nameI18n, elongStr = "", phaseStr = "", raStr = "", decStr = "";
	const bool horizon = ui->ephemerisHorizontalCoordinatesCheckBox->isChecked();
	const bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

	DisplayedPositionIndex = -1; // deselect an ephemeris marker
	initListEphemeris();

	if (currentPlanet.isEmpty()) // avoid crash
		return;

	int idxRow = 0, colorIndex = 0;
	double currentStep;
	double solarDay = 1.0, siderealDay = 1.0, siderealYear = 365.256363004; // days
	const PlanetP& cplanet = core->getCurrentPlanet();		
	if (!cplanet->getEnglishName().contains("observer", Qt::CaseInsensitive))
	{
		if (cplanet==solarSystem->getEarth())
			solarDay = 1.0; // Special case: OK, it's Earth, let's use standard duration of the solar day
		else
			solarDay = cplanet->getMeanSolarDay();
		siderealDay = cplanet->getSiderealDay();
		siderealYear = cplanet->getSiderealPeriod();
	}
	switch (ui->ephemerisStepComboBox->currentData().toInt())
	{
		case 0: // custom time step
			currentStep = getCustomTimeStep();
			break;
		case 1:
			currentStep = 10. * StelCore::JD_MINUTE;
			break;
		case 2:
			currentStep = 30. * StelCore::JD_MINUTE;
			break;
		case 3:
			currentStep = StelCore::JD_HOUR;
			break;
		case 4:
			currentStep = 6. * StelCore::JD_HOUR;
			break;
		case 5:
			currentStep = 12. * StelCore::JD_HOUR;
			break;
		case 6:
			currentStep = solarDay;
			break;
		case 7:
			currentStep = 5. * solarDay;
			break;
		case 8:
			currentStep = 10. * solarDay;
			break;
		case 9:
			currentStep = 15. * solarDay;
			break;
		case 10:
			currentStep = 30. * solarDay;
			break;
		case 11:
			currentStep = 60. * solarDay;
			break;
		case 12:
			currentStep = StelCore::JD_DAY;
			break;
		case 13:
			currentStep = 5. * StelCore::JD_DAY;
			break;
		case 14:
			currentStep = 10. * StelCore::JD_DAY;
			break;
		case 15:
			currentStep = 15. * StelCore::JD_DAY;
			break;
		case 16:
			currentStep = 30. * StelCore::JD_DAY;
			break;
		case 17:
			currentStep = 60. * StelCore::JD_DAY;
			break;
		case 18:
			currentStep = siderealDay;
			break;
		case 19:
			currentStep = 5. * siderealDay;
			break;
		case 20:
			currentStep = 10. * siderealDay;
			break;
		case 21:
			currentStep = 15. * siderealDay;
			break;
		case 22:
			currentStep = 30. * siderealDay;
			break;
		case 23:
			currentStep = 60. * siderealDay;
			break;
		case 24:
			currentStep = 100. * solarDay;
			break;
		case 25:
			currentStep = 100. * siderealDay;
			break;
		case 26:
			currentStep = 100. * StelCore::JD_DAY;
			break;
		case 27:
			currentStep = siderealYear*solarDay;
			break;
		case 28: // 1 Julian year
			currentStep = 365.25*solarDay;
			break;
		case 29: // 1 Gaussian year
			currentStep = 365.2568983*solarDay;
			break;
		case 30: // 1 synodic month
			currentStep = 29.530588853*solarDay;
			break;
		case 31: // 1 draconic month
			currentStep = 27.212220817*solarDay;
			break;
		case 32: // 1 mean tropical month
			currentStep = 27.321582241*solarDay;
			break;
		case 33: // 1 anomalistic month
			currentStep = 27.554549878*solarDay;
			break;
		case 34: // 1 anomalistic year
			currentStep = 365.259636*solarDay;
			break;
		case 35: // 1 saros (223 synodic months)
			currentStep = 6585.321314219*solarDay;
			break;
		case 36:
			currentStep = 500. * siderealDay;
			break;
		case 37:
			currentStep = 500. * solarDay;
			break;
		case 38:
			currentStep = StelCore::JD_MINUTE;
			break;
		default:
			currentStep = solarDay;
			break;
	}

	double currentJD = core->getJD(); // save current JD
	double firstJD = StelUtils::qDateTimeToJd(ui->dateFromDateTimeEdit->dateTime());	
	firstJD = firstJD - core->getUTCOffset(firstJD) / 24.;
	double secondJD = StelUtils::qDateTimeToJd(ui->dateToDateTimeEdit->dateTime());	
	secondJD = secondJD - core->getUTCOffset(secondJD) / 24.;
	int elements = static_cast<int>((secondJD - firstJD) / currentStep);
	EphemerisList.clear();
	bool allNakedEyePlanets = (ui->allNakedEyePlanetsCheckBox->isChecked() && cplanet==solarSystem->getEarth());

	QList<PlanetP> celestialObjects;
	celestialObjects.clear();

	int n = 1;
	if (allNakedEyePlanets)
	{
		QStringList planets;
		planets << "Mercury" << "Venus" << "Mars" << "Jupiter" << "Saturn";
		for (auto planet: planets)
			celestialObjects.append(solarSystem->searchByEnglishName(planet));
		n = planets.count();
	}
	else if (secondaryPlanet!="none")
	{
		celestialObjects.append(solarSystem->searchByEnglishName(currentPlanet));
		celestialObjects.append(solarSystem->searchByEnglishName(secondaryPlanet));
		n = 2;
	}
	else
		celestialObjects.append(solarSystem->searchByEnglishName(currentPlanet));

	EphemerisList.reserve(elements*n);
	propMgr->setStelPropertyValue("SolarSystem.ephemerisDataLimit", n);

	for (auto obj: celestialObjects)
	{
		englishName = obj->getEnglishName();
		nameI18n = obj->getNameI18n();

		if (allNakedEyePlanets && cplanet==solarSystem->getEarth())
		{
			if (englishName.contains("Mercury", Qt::CaseInsensitive))
				colorIndex = 2;
			else if (englishName.contains("Venus", Qt::CaseInsensitive))
				colorIndex = 3;
			else if (englishName.contains("Mars", Qt::CaseInsensitive))
				colorIndex = 4;
			else if (englishName.contains("Jupiter", Qt::CaseInsensitive))
				colorIndex = 5;
			else if (englishName.contains("Saturn", Qt::CaseInsensitive))
				colorIndex = 6;
			else
				colorIndex = 0;
		}
		else if (secondaryPlanet!="none")
		{
			if (secondaryPlanet==englishName)
				colorIndex = 1;
			else
				colorIndex = 0;
		}

		if (obj == solarSystem->getSun())
		{
			phaseStr = dash;
			elongStr = dash;
		}

		for (int i = 0; i <= elements; i++)
		{
			double JD = firstJD + i * currentStep;
			core->setJD(JD);
			core->update(0); // force update to get new coordinates
			if (horizon)
			{
				pos = obj->getAltAzPosAuto(core);
				StelUtils::rectToSphe(&ra, &dec, pos);
				double direction = 3.; // N is zero, E is 90 degrees
				if (useSouthAzimuth)
					direction = 2.;
				ra = fmod(direction * M_PI - ra, M_PI * 2);
				if (withDecimalDegree)
				{
					raStr = StelUtils::radToDecDegStr(ra, 5, false, true);
					decStr = StelUtils::radToDecDegStr(dec, 5);
				}
				else
				{
					raStr = StelUtils::radToDmsStr(ra, true);
					decStr = StelUtils::radToDmsStr(dec, true);
				}
			}
			else
			{
				pos = obj->getJ2000EquatorialPos(core);
				StelUtils::rectToSphe(&ra, &dec, pos);
				if (withDecimalDegree)
				{
					raStr = StelUtils::radToDecDegStr(ra, 5, false, true);
					decStr = StelUtils::radToDecDegStr(dec, 5);
				}
				else
				{
					raStr = StelUtils::radToHmsStr(ra);
					decStr = StelUtils::radToDmsStr(dec, true);
				}
			}

			Ephemeris item;
			item.coord = pos;
			item.colorIndex = colorIndex;			
			item.objDate = JD;
			item.magnitude = obj->getVMagnitudeWithExtinction(core);
			EphemerisList.append(item);

			StelUtils::rectToSphe(&ra, &dec, pos);
			observerHelioPos = core->getObserverHeliocentricEclipticPos();
			if (phaseStr != dash)
				phaseStr = QString("%1%").arg(QString::number(obj->getPhase(observerHelioPos) * 100, 'f', 2));

			if (elongStr != dash)
			{
				if (withDecimalDegree)
					elongStr = StelUtils::radToDecDegStr(obj->getElongation(observerHelioPos), 5, false, true);
				else
					elongStr = StelUtils::radToDmsStr(obj->getElongation(observerHelioPos), true);
			}

			ACEphemTreeWidgetItem* treeItem = new ACEphemTreeWidgetItem(ui->ephemerisTreeWidget);
			treeItem->setText(EphemerisCOName, nameI18n);
			treeItem->setData(EphemerisCOName, Qt::UserRole, englishName);
			treeItem->setText(EphemerisDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))); // local date and time
			treeItem->setData(EphemerisDate, Qt::UserRole, JD);
			treeItem->setText(EphemerisRA, raStr);
			treeItem->setData(EphemerisRA, Qt::UserRole, idxRow);
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

			idxRow++;
		}		
	}
	core->setJD(currentJD); // restore time

	// adjust the column width
	for (int i = 0; i < EphemerisCount; ++i)
	{
		ui->ephemerisTreeWidget->resizeColumnToContents(i);
	}

	// sort-by-date
	ui->ephemerisTreeWidget->sortItems(EphemerisDate, Qt::AscendingOrder);

	emit solarSystem->requestEphemerisVisualization();
}

double AstroCalcDialog::getCustomTimeStep()
{
	double timeUnit = 1.0, solarDay = 1.0, siderealDay = 1.0, siderealYear = 365.256363004; // days
	const PlanetP& cplanet = core->getCurrentPlanet();
	if (!cplanet->getEnglishName().contains("observer", Qt::CaseInsensitive))
	{
		if (cplanet==solarSystem->getEarth())
			solarDay = 1.0; // Special case: OK, it's Earth, let's use standard duration of the solar day
		else
			solarDay = cplanet->getMeanSolarDay();
		siderealDay = cplanet->getSiderealDay();
		siderealYear = cplanet->getSiderealPeriod();
	}
	int timeStep = conf->value("astrocalc/custom_time_step", "1").toInt();
	// NOTE: Sync units with AstroCalcCustomStepsDialog::populateUnitMeasurementsList()!
	switch(conf->value("astrocalc/custom_time_step_unit", "3").toInt())
	{
		case 1: // minutes
			timeUnit = StelCore::JD_MINUTE;
			break;
		case 2: // hours
			timeUnit = StelCore::JD_HOUR;
			break;
		case 3: // solar days
			timeUnit = solarDay;
			break;
		case 4: // sidereal days
			timeUnit = siderealDay;
			break;
		case 5: // Julian days
			timeUnit = StelCore::JD_DAY;
			break;
		case 6: // synodic months
			timeUnit = 29.530588853*solarDay;
			break;
		case 7: // draconic months
			timeUnit = 27.212220817*solarDay;
			break;
		case 8: // mean tropical months
			timeUnit = 27.321582241*solarDay;
			break;
		case 9: // anomalistic months
			timeUnit = 27.554549878*solarDay;
			break;
		case 10: // sidereal years
			timeUnit = siderealYear;
			break;
		case 11: // Julian years
			timeUnit = 365.25*solarDay;
			break;
		case 12: // Gaussian years
			timeUnit = 365.2568983*solarDay;
			break;
		case 13: // Anomalistic years
			timeUnit = 365.259636*solarDay;
			break;
		case 14: // 1 saros (223 synodic months)
			timeUnit = 6585.321314219*solarDay;
			break;
	}
	return timeStep*timeUnit;
}

void AstroCalcDialog::saveEphemeris()
{
	QString filter = q_("Microsoft Excel Open XML Spreadsheet");
	filter.append(" (*.xlsx);;");
	filter.append(q_("CSV (Comma delimited)"));
	filter.append(" (*.csv)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save calculated ephemerides as..."),
							QDir::homePath() + "/ephemerides.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->ephemerisTreeWidget, ephemerisHeader);
	else
	{
		int count = ui->ephemerisTreeWidget->topLevelItemCount();
		int columns = ephemerisHeader.size();

		int *width;
		width = new int[columns];
		QString sData;
		int w;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("Ephemerides"));
		xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
		xlsx.addSheet(ui->celestialBodyComboBox->currentData(Qt::DisplayRole).toString(), AbstractSheet::ST_WorkSheet);

		QXlsx::Format header;
		header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		header.setPatternBackgroundColor(Qt::yellow);
		header.setBorderStyle(QXlsx::Format::BorderThin);
		header.setBorderColor(Qt::black);
		header.setFontBold(true);
		for (int i = 0; i < columns; i++)
		{
			// Row 1: Names of columns
			sData = ephemerisHeader.at(i).trimmed();
			xlsx.write(1, i + 1, sData, header);
			width[i] = sData.size();
		}

		QXlsx::Format data;
		data.setHorizontalAlignment(QXlsx::Format::AlignRight);
		for (int i = 0; i < count; i++)
		{
			for (int j = 0; j < columns; j++)
			{
				// Row 2 and next: the data
				sData = ui->ephemerisTreeWidget->topLevelItem(i)->text(j).trimmed();
				xlsx.write(i + 2, j + 1, sData, data);
				w = sData.size();
				if (w > width[j])
				{
					width[j] = w;
				}
			}
		}

		for (int i = 0; i < columns; i++)
		{
			xlsx.setColumnWidth(i+1, width[i]+2);
		}

		delete[] width;
		xlsx.saveAs(filePath);
	}
}

void AstroCalcDialog::cleanupEphemeris()
{
	EphemerisList.clear();
	ui->ephemerisTreeWidget->clear();
}

void AstroCalcDialog::setTransitHeaderNames()
{
	transitHeader.clear();
	transitHeader << q_("Name");
	transitHeader << q_("Date and Time");
	// TRANSLATORS: altitude
	transitHeader << q_("Altitude");
	// TRANSLATORS: magnitude
	transitHeader << q_("Mag.");
	transitHeader << q_("Solar Elongation");
	transitHeader << q_("Lunar Elongation");
	ui->transitTreeWidget->setHeaderLabels(transitHeader);

	// adjust the column width
	for (int i = 0; i < TransitCount; ++i)
	{
		ui->transitTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListTransit()
{
	ui->transitTreeWidget->clear();
	ui->transitTreeWidget->setColumnCount(TransitCount);
	setTransitHeaderNames();
	ui->transitTreeWidget->header()->setSectionsMovable(false);
	ui->transitTreeWidget->header()->setDefaultAlignment(Qt::AlignHCenter);
}

void AstroCalcDialog::generateTransits()
{
	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();
	if (!selectedObjects.isEmpty())
	{
		QString name, englishName;
		StelObjectP selectedObject = selectedObjects[0];
		name = ui->transitCelestialBodyNameLabel->text();
		selectedObject->getEnglishName().isEmpty() ? englishName = name : englishName = selectedObject->getEnglishName();
		const bool isPlanet = (selectedObject->getType() == "Planet");

		if (!name.isEmpty()) // OK, let's calculate!
		{
			const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

			initListTransit();

			double currentStep = 1.0;
			const PlanetP& planet = core->getCurrentPlanet();
			const PlanetP sun = solarSystem->getSun();
			const PlanetP moon = solarSystem->getMoon();
			const PlanetP earth = solarSystem->getEarth();
			if (!planet->getEnglishName().contains("observer", Qt::CaseInsensitive))
			{
				if (planet==earth)
					currentStep = 1.0; // Special case: OK, it's Earth, let's use standard duration of the solar day
				else
					currentStep = planet->getMeanSolarDay();
			}

			double currentJD = core->getJD();   // save current JD
			double startJD = StelUtils::qDateTimeToJd(QDateTime(ui->transitFromDateEdit->date()));
			double stopJD = StelUtils::qDateTimeToJd(QDateTime(ui->transitToDateEdit->date()));
			startJD = startJD - core->getUTCOffset(startJD) / 24.;
			stopJD = stopJD - core->getUTCOffset(stopJD) / 24.;
			int elements = static_cast<int>((stopJD - startJD) / currentStep);
			double JD, UTCshift, az, alt;
			float magnitude;
			QString altStr, magStr, elongSStr = dash, elongLStr =dash;
			for (int i = 0; i <= elements; i++)
			{
				JD = startJD + i * currentStep;
				core->setJD(JD);
				core->update(0); // force update to get new coordinates
				UTCshift = core->getUTCOffset(JD) / 24.; // Fix DST shift...
				Vec3f rts = selectedObject->getRTSTime(core);
				JD = static_cast<int>(JD) + 0.5 + rts[1]/24. - UTCshift;
				core->setJD(JD);
				core->update(0); // force update to get new coordinates
				if (isPlanet) // A tiny improvement for accuracy
				{
					Vec3f rts = selectedObject->getRTSTime(core);
					JD = static_cast<int>(JD) + 0.5 + rts[1]/24. - UTCshift;
					core->setJD(JD);
					core->update(0);
				}

				StelUtils::rectToSphe(&az, &alt, selectedObject->getAltAzPosAuto(core));
				if (withDecimalDegree)
				{
					altStr = StelUtils::radToDecDegStr(alt, 5, false, true);
					if (selectedObject!=sun)
						elongSStr = StelUtils::radToDecDegStr(selectedObject->getJ2000EquatorialPos(core).angle(sun->getJ2000EquatorialPos(core)), 5, false, true);
					else
						elongSStr = dash;
					if (selectedObject!=moon && planet==earth)
						elongLStr = StelUtils::radToDecDegStr(selectedObject->getJ2000EquatorialPos(core).angle(moon->getJ2000EquatorialPos(core)), 5, false, true);
					else
						elongLStr = dash;
				}
				else
				{
					altStr = StelUtils::radToDmsStr(alt, true);
					if (selectedObject!=sun)
						elongSStr = StelUtils::radToDmsStr(selectedObject->getJ2000EquatorialPos(core).angle(sun->getJ2000EquatorialPos(core)), true);
					else
						elongSStr = dash;
					if (selectedObject!=moon && planet==solarSystem->getEarth())
						elongLStr = StelUtils::radToDmsStr(selectedObject->getJ2000EquatorialPos(core).angle(moon->getJ2000EquatorialPos(core)), true);
					else
						elongLStr = dash;
				}
				magnitude = selectedObject->getVMagnitudeWithExtinction(core);
				if (magnitude > 50.f)
					magStr = dash;
				else
					magStr =	QString::number(magnitude, 'f', 2);

				ACTransitTreeWidgetItem* treeItem = new ACTransitTreeWidgetItem(ui->transitTreeWidget);
				treeItem->setText(TransitCOName, name);
				treeItem->setData(TransitCOName, Qt::UserRole, englishName);
				treeItem->setText(TransitDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))); // local date and time
				treeItem->setData(TransitDate, Qt::UserRole, JD);
				treeItem->setText(TransitAltitude, altStr);
				treeItem->setTextAlignment(TransitAltitude, Qt::AlignRight);
				treeItem->setText(TransitMagnitude, magStr);
				treeItem->setTextAlignment(TransitMagnitude, Qt::AlignRight);
				treeItem->setText(TransitElongation, elongSStr);
				treeItem->setTextAlignment(TransitElongation, Qt::AlignRight);
				treeItem->setText(TransitAngularDistance, elongLStr);
				treeItem->setTextAlignment(TransitAngularDistance, Qt::AlignRight);
			}
			core->setJD(currentJD);

			// adjust the column width
			for (int i = 0; i < TransitCount; ++i)
			{
				ui->transitTreeWidget->resizeColumnToContents(i);
			}

			// sort-by-date
			ui->transitTreeWidget->sortItems(TransitDate, Qt::AscendingOrder);
		}
		else
			cleanupTransits();
	}
}

void AstroCalcDialog::cleanupTransits()
{
	ui->transitTreeWidget->clear();
}

void AstroCalcDialog::selectCurrentTransit(const QModelIndex& modelIndex)
{
	// Find the object
	QString name = modelIndex.sibling(modelIndex.row(), TransitCOName).data(Qt::UserRole).toString();
	double JD = modelIndex.sibling(modelIndex.row(), TransitDate).data(Qt::UserRole).toDouble();

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

void AstroCalcDialog::setTransitCelestialBodyName()
{
	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();
	QString name;
	if (!selectedObjects.isEmpty())
	{
		StelObjectP selectedObject = selectedObjects[0];
		name = selectedObject->getNameI18n();
		if (name.isEmpty())
		{
			QString otype = selectedObject->getType();
			if (otype == "Nebula")
				name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
			if (otype == "Star" || otype=="Pulsar")
				name = selectedObject->getID();
		}
		if (selectedObject->getType()=="Satellite")
			name = QString();
	}
	ui->transitCelestialBodyNameLabel->setText(name);
}

void AstroCalcDialog::saveTransits()
{
	QString filter = q_("Microsoft Excel Open XML Spreadsheet");
	filter.append(" (*.xlsx);;");
	filter.append(q_("CSV (Comma delimited)"));
	filter.append(" (*.csv)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save calculated transits as..."),
							QDir::homePath() + "/transits.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->transitTreeWidget, ephemerisHeader);
	else
	{
		int count = ui->transitTreeWidget->topLevelItemCount();
		int columns = transitHeader.size();

		int *width;
		width = new int[columns];
		QString sData;
		int w;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("Transits"));
		xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
		xlsx.addSheet(ui->transitCelestialBodyNameLabel->text(), AbstractSheet::ST_WorkSheet);

		QXlsx::Format header;
		header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		header.setPatternBackgroundColor(Qt::yellow);
		header.setBorderStyle(QXlsx::Format::BorderThin);
		header.setBorderColor(Qt::black);
		header.setFontBold(true);
		for (int i = 0; i < columns; i++)
		{
			// Row 1: Names of columns
			sData = transitHeader.at(i).trimmed();
			xlsx.write(1, i + 1, sData, header);
			width[i] = sData.size();
		}

		QXlsx::Format data;
		data.setHorizontalAlignment(QXlsx::Format::AlignRight);
		for (int i = 0; i < count; i++)
		{
			for (int j = 0; j < columns; j++)
			{
				// Row 2 and next: the data
				sData = ui->transitTreeWidget->topLevelItem(i)->text(j).trimmed();
				xlsx.write(i + 2, j + 1, sData, data);
				w = sData.size();
				if (w > width[j])
				{
					width[j] = w;
				}
			}
		}

		for (int i = 0; i < columns; i++)
		{
			xlsx.setColumnWidth(i+1, width[i]+2);
		}

		delete[] width;
		xlsx.saveAs(filePath);
	}
}

void AstroCalcDialog::populateCelestialBodyList()
{
	Q_ASSERT(ui->celestialBodyComboBox);
	Q_ASSERT(ui->secondaryCelestialBodyComboBox);
	Q_ASSERT(ui->graphsCelestialBodyComboBox);
	Q_ASSERT(ui->firstCelestialBodyComboBox);
	Q_ASSERT(ui->secondCelestialBodyComboBox);

	QComboBox* planets = ui->celestialBodyComboBox;
	QComboBox* planets2 = ui->secondaryCelestialBodyComboBox;
	QComboBox* graphsp = ui->graphsCelestialBodyComboBox;
	QComboBox* firstCB = ui->firstCelestialBodyComboBox;
	QComboBox* secondCB = ui->secondCelestialBodyComboBox;

	QList<PlanetP> ss = solarSystem->getAllPlanets();

	// Save the current selection to be restored later
	planets->blockSignals(true);
	int indexP = planets->currentIndex();
	QVariant selectedPlanetId = planets->itemData(indexP);
	planets->clear();

	planets2->blockSignals(true);
	int indexP2 = planets2->currentIndex();
	QVariant selectedPlanet2Id = planets2->itemData(indexP2);
	planets2->clear();

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
	for (const auto& p : ss)
	{
		if (!p->getEnglishName().contains("Observer", Qt::CaseInsensitive))
		{
			if (p->getEnglishName() != core->getCurrentPlanet()->getEnglishName())
			{
				// Let's exclude moons from list of celestial body for ephemeris tool (except the moons of current planet)
				if (p->getPlanetType()==Planet::isMoon && p->getParent()==core->getCurrentPlanet())
				{
					planets->addItem(p->getNameI18n(), p->getEnglishName());
					planets2->addItem(p->getNameI18n(), p->getEnglishName());
				}
				if (p->getPlanetType()!=Planet::isMoon)
				{
					planets->addItem(p->getNameI18n(), p->getEnglishName());
					planets2->addItem(p->getNameI18n(), p->getEnglishName());
				}
				graphsp->addItem(p->getNameI18n(), p->getEnglishName());
			}
			firstCB->addItem(p->getNameI18n(), p->getEnglishName());
			secondCB->addItem(p->getNameI18n(), p->getEnglishName());
		}
	}
	planets2->addItem(dash, "none");
	// Restore the selection
	indexP = planets->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexP < 0)
	{
		indexP = planets->findData(conf->value("astrocalc/ephemeris_celestial_body", "Moon").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
		if (indexP<0)
			indexP = 0;
	}
	planets->setCurrentIndex(indexP);
	planets->model()->sort(0);

	// Restore the selection
	indexP2 = planets2->findData(selectedPlanet2Id, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexP2 < 0)
	{
		indexP2 = planets2->findData(conf->value("astrocalc/ephemeris_second_celestial_body", "none").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
		if (indexP2<0)
			indexP2 = 0;
	}
	planets2->setCurrentIndex(indexP2);
	planets2->model()->sort(0);

	indexG = graphsp->findData(selectedGraphsPId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexG < 0)
	{
		indexG = graphsp->findData(conf->value("astrocalc/graphs_celestial_body", "Moon").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
		if (indexG<0)
			indexG = 0;
	}
	graphsp->setCurrentIndex(indexG);
	graphsp->model()->sort(0);

	indexFCB = firstCB->findData(selectedFirstCelestialBodyId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexFCB < 0)
	{
		indexFCB = firstCB->findData(conf->value("astrocalc/first_celestial_body", "Sun").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
		if (indexFCB<0)
			indexFCB = 0;
	}
	firstCB->setCurrentIndex(indexFCB);
	firstCB->model()->sort(0);

	indexSCB = secondCB->findData(selectedSecondCelestialBodyId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexSCB < 0)
	{
		indexSCB = secondCB->findData(conf->value("astrocalc/second_celestial_body", "Earth").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
		if (indexSCB<0)
			indexSCB = 0;
	}
	secondCB->setCurrentIndex(indexSCB);
	secondCB->model()->sort(0);

	planets->blockSignals(false);
	planets2->blockSignals(false);
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

void AstroCalcDialog::saveEphemerisSecondaryCelestialBody(int index)
{
	Q_ASSERT(ui->secondaryCelestialBodyComboBox);
	QComboBox* planets = ui->secondaryCelestialBodyComboBox;
	conf->setValue("astrocalc/ephemeris_second_celestial_body", planets->itemData(index).toString());
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

void AstroCalcDialog::updateGraphsDuration(int duration)
{
	if (graphsDuration!=duration)
	{
		graphsDuration = duration;
		conf->setValue("astrocalc/graphs_duration", duration);
	}
}

void AstroCalcDialog::populateEphemerisTimeStepsList()
{
	Q_ASSERT(ui->ephemerisStepComboBox);

	QComboBox* steps = ui->ephemerisStepComboBox;
	steps->blockSignals(true);
	int index = steps->currentIndex();
	QVariant selectedStepId = steps->itemData(index);

	steps->clear();
	steps->addItem(q_("1 minute"), "38");
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
	steps->addItem(q_("500 solar days"), "37");
	steps->addItem(q_("1 sidereal day"), "18");
	steps->addItem(q_("5 sidereal days"), "19");
	steps->addItem(q_("10 sidereal days"), "20");
	steps->addItem(q_("15 sidereal days"), "21");
	steps->addItem(q_("30 sidereal days"), "22");
	steps->addItem(q_("60 sidereal days"), "23");
	steps->addItem(q_("100 sidereal days"), "25");
	steps->addItem(q_("500 sidereal days"), "36");
	steps->addItem(q_("1 sidereal year"), "27");
	steps->addItem(q_("1 Julian day"), "12");
	steps->addItem(q_("5 Julian days"), "13");
	steps->addItem(q_("10 Julian days"), "14");
	steps->addItem(q_("15 Julian days"), "15");
	steps->addItem(q_("30 Julian days"), "16");
	steps->addItem(q_("60 Julian days"), "17");
	steps->addItem(q_("100 Julian days"), "26");
	steps->addItem(q_("1 Julian year"), "28");
	steps->addItem(q_("1 Gaussian year"), "29");
	steps->addItem(q_("1 synodic month"), "30");
	steps->addItem(q_("1 draconic month"), "31");
	steps->addItem(q_("1 mean tropical month"), "32");
	steps->addItem(q_("1 anomalistic month"), "33");
	steps->addItem(q_("1 anomalistic year"), "34");
	steps->addItem(q_("1 saros"), "35");
	steps->addItem(q_("custom interval"), "0");

	index = steps->findData(selectedStepId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index < 0)
	{
		// default step: one day
		index = steps->findData(conf->value("astrocalc/ephemeris_time_step", "6").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	steps->setCurrentIndex(index);
	steps->blockSignals(false);
	enableCustomEphemerisTimeStepButton();
}

void AstroCalcDialog::saveEphemerisTimeStep(int index)
{
	Q_ASSERT(ui->ephemerisStepComboBox);
	QComboBox* steps = ui->ephemerisStepComboBox;
	conf->setValue("astrocalc/ephemeris_time_step", steps->itemData(index).toInt());
	enableCustomEphemerisTimeStepButton();
}

void AstroCalcDialog::initEphemerisFlagNakedEyePlanets(void)
{
	bool nep = conf->value("astrocalc/ephemeris_nakedeye_planets", "false").toBool();
	if (core->getCurrentPlanet()==solarSystem->getEarth())
	{
		ui->celestialBodyComboBox->setEnabled(!nep);
		ui->secondaryCelestialBodyComboBox->setEnabled(!nep);
		ui->allNakedEyePlanetsCheckBox->setChecked(nep);
		ui->allNakedEyePlanetsCheckBox->setEnabled(true);
	}
	else
	{
		ui->celestialBodyComboBox->setEnabled(true);
		ui->secondaryCelestialBodyComboBox->setEnabled(true);
		ui->allNakedEyePlanetsCheckBox->setChecked(false);
		ui->allNakedEyePlanetsCheckBox->setEnabled(false);
	}
}

void AstroCalcDialog::saveEphemerisFlagNakedEyePlanets(bool flag)
{
	ui->celestialBodyComboBox->setEnabled(!flag);
	ui->secondaryCelestialBodyComboBox->setEnabled(!flag);
	conf->setValue("astrocalc/ephemeris_nakedeye_planets", flag);
	reGenerateEphemeris(false);
}

void AstroCalcDialog::enableCustomEphemerisTimeStepButton()
{
	Q_ASSERT(ui->ephemerisStepComboBox);
	if (ui->ephemerisStepComboBox->currentData(Qt::UserRole).toInt()==0)
		ui->pushButtonCustomStepsDialog->setEnabled(true);
	else
		ui->pushButtonCustomStepsDialog->setEnabled(false);
}

void AstroCalcDialog::populatePlanetList()
{
	Q_ASSERT(ui->object1ComboBox); // object 1 is always major planet

	QComboBox* planetList = ui->object1ComboBox;
	QList<PlanetP> planets = solarSystem->getAllPlanets();
	const StelTranslator& trans = localeMgr->getSkyTranslator();
	QString cpName = core->getCurrentPlanet()->getEnglishName();

	// Save the current selection to be restored later
	planetList->blockSignals(true);
	int index = planetList->currentIndex();
	QVariant selectedPlanetId = planetList->itemData(index);
	planetList->clear();
	// For each planet, display the localized name and store the original as user
	// data. Unfortunately, there's no other way to do this than with a cycle.
	for (const auto& planet : planets)
	{
		// major planets and the Sun
		if ((planet->getPlanetType() == Planet::isPlanet || planet->getPlanetType() == Planet::isStar) && planet->getEnglishName() != cpName)
			planetList->addItem(trans.qtranslate(planet->getNameI18n()), planet->getEnglishName());

		// moons of the current planet
		if (planet->getPlanetType() == Planet::isMoon && planet->getEnglishName() != cpName && planet->getParent() == core->getCurrentPlanet())
			planetList->addItem(trans.qtranslate(planet->getNameI18n()), planet->getEnglishName());
	}
	// special case: selected dwarf and minot planets
	planets.clear();
	planets.append(solarSystem->searchByEnglishName("Pluto"));
	planets.append(solarSystem->searchByEnglishName("Ceres"));
	planets.append(solarSystem->searchByEnglishName("Pallas"));
	planets.append(solarSystem->searchByEnglishName("Juno"));
	planets.append(solarSystem->searchByEnglishName("Vesta"));
	for (const auto& planet : planets)
	{
		if (!planet.isNull() && planet->getEnglishName()!=cpName)
			planetList->addItem(trans.qtranslate(planet->getNameI18n()), planet->getEnglishName());
	}
	// Restore the selection
	index = planetList->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index < 0)
	{
		index = planetList->findData(conf->value("astrocalc/phenomena_celestial_body", "Venus").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
		if (index<0)
			index = 0;
	}
	planetList->setCurrentIndex(index);
	planetList->model()->sort(0);
	planetList->blockSignals(false);
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
	groups->addItem(q_("Latest selected object"), "-1");
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
	groups->addItem(q_("Planetary nebulae (<%1 mag)").arg(brLimit), "14");
	groups->addItem(q_("Bright nebulae (<%1 mag)").arg(brLimit), "15");
	groups->addItem(q_("Dark nebulae"), "16");
	groups->addItem(q_("Bright galaxies (<%1 mag)").arg(brLimit), "17");
	groups->addItem(q_("Symbiotic stars"), "18");
	groups->addItem(q_("Emission-line stars"), "19");
	groups->addItem(q_("Interstellar objects"), "20");
	groups->addItem(q_("Planets and Sun"), "21");
	groups->addItem(q_("Sun, planets and moons"), "22");
	groups->addItem(q_("Bright Solar system objects (<%1 mag)").arg(QString::number(brightLimit + 2.0f, 'f', 1)), "23");

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
	adjustPhenomenaColumns();
}

void AstroCalcDialog::savePhenomenaOppositionFlag(bool b)
{
	conf->setValue("astrocalc/flag_phenomena_opposition", b);
}

void AstroCalcDialog::savePhenomenaPerihelionAphelionFlag(bool b)
{
	conf->setValue("astrocalc/flag_phenomena_perihelion", b);
}

void AstroCalcDialog::savePhenomenaAngularSeparation()
{
	conf->setValue("astrocalc/phenomena_angular_separation", QString::number(ui->allowedSeparationSpinBox->valueDegrees(), 'f', 5));
}

void AstroCalcDialog::drawAltVsTimeDiagram()
{
	// Avoid crash!
	if (core->getCurrentPlanet()->getEnglishName().contains("->")) // We are on the spaceship!
		return;

	// special case - plot the graph when tab is visible
	//..
	// we got notified about a reason to redraw the plot, but dialog was
	// not visible. which means we must redraw when becoming visible again!
	if (!dialog->isVisible() && plotAltVsTime)
	{
		graphPlotNeedsRefresh = true;
		return;
	}

	if (!plotAltVsTime) return;

	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();

	if (!selectedObjects.isEmpty())
	{
		// X axis - time; Y axis - altitude
		QList<double> aX, aY, sX, sY, sYn, sYa, sYc, mX, mY;
		QVector<double> xs, ys, ysn, ysa, ysc, xm, ym;

		StelObjectP selectedObject = selectedObjects[0];
		bool onEarth = core->getCurrentPlanet()==solarSystem->getEarth();

		double currentJD = core->getJD();
		double shift = core->getUTCOffset(currentJD) / 24.0;
		double noon = static_cast<int>(currentJD + shift);
		double az, alt, deg, ltime, JD;
		bool sign;

		double xMaxY = -100.;
		int step = 180;
		int limit = 485;		
		bool isSatellite = false;

#ifdef USE_STATIC_PLUGIN_SATELLITES
		SatelliteP sat;		
		if (selectedObject->getType() == "Satellite") 
		{
			// get reference to satellite
			isSatellite = true;
			sat = GETSTELMODULE(Satellites)->getById(selectedObject->getInfoMap(core)["catalog"].toString());
		}
#endif

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
				// update data for that single satellite only
				sat->update(0.0);
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
		minY = *std::min_element(aY.begin(), aY.end()) - 2.0;
		maxY = *std::max_element(aY.begin(), aY.end()) + 2.0;

		// additional data: Sun + Twilight
		if (plotAltVsTimeSun)
		{
			xs = sX.toVector();
			ys = sY.toVector();
			ysc = sYc.toVector();
			ysn = sYn.toVector();
			ysa = sYa.toVector();
			double minYs = *std::min_element(sY.begin(), sY.end());
			double maxYs = *std::max_element(sY.begin(), sY.end());

			if (minY >= minYs - 2.0)  minY = minYs - 2.0;
			if (maxY <= maxYs + 20.0) maxY = maxYs + 20.0;
		}

		// additional data: Moon
		if (plotAltVsTimeMoon && onEarth)
		{
			xm = mX.toVector();
			ym = mY.toVector();
			double minYm = *std::min_element(mY.begin(), mY.end());
			double maxYm = *std::max_element(mY.begin(), mY.end());

			if (minY >= minYm - 2.0)  minY = minYm - 2.0;
			if (maxY <= maxYm + 2.0)  maxY = maxYm + 2.0;
		}

		if (plotAltVsTimePositive && minY<altVsTimePositiveLimit)
			minY = altVsTimePositiveLimit;

		prepareAxesAndGraph();
		drawCurrentTimeDiagram();

		QString name = selectedObject->getNameI18n();
		if (name.isEmpty())
		{
			QString otype = selectedObject->getType();
			if (otype == "Nebula")
				name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();

			if (otype == "Star" || otype=="Pulsar")
				selectedObject->getID().isEmpty() ? name = q_("Unnamed star") : name = selectedObject->getID();
		}

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
	// and only if dialog is visible at all
	if (!dialog->isVisible() || (!plotAltVsTime && !plotAziVsTime)) return;

	double currentJD = core->getJD();
	double UTCOffset = core->getUTCOffset(currentJD);
	double now = ((currentJD + 0.5 - static_cast<int>(currentJD)) * 86400.0) + UTCOffset * 3600.0;

	if (now > 129600) now -= 86400;
	if (now < 43200) now += 86400;
	QList<double> ax, ay;
	ax.append(now);
	ax.append(now);
	ay.append(-180.);
	ay.append(360.);
	QVector<double> x = ax.toVector(), y = ay.toVector();
	if (plotAltVsTime)
	{
		ui->altVsTimePlot->graph(1)->setData(x, y);
		ui->altVsTimePlot->replot();
	}
	if (plotAziVsTime)
	{
		ui->aziVsTimePlot->graph(1)->setData(x, y);
		ui->aziVsTimePlot->replot();
	}

	// detect roll over graph day limits.
	// if so, update the graph
	int graphJD = static_cast<int>(currentJD + UTCOffset / 24.);
	if (oldGraphJD != graphJD || graphPlotNeedsRefresh)
	{
		oldGraphJD = graphJD;
		graphPlotNeedsRefresh = false;
		emit graphDayChanged();
	}
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
		double startJD, JD, ltime, UTCshift, width = 1.0;
		StelUtils::getDateFromJulianDay(currentJD, &year, &month, &day);
		StelUtils::getJDFromDate(&startJD, year, 1, 1, 0, 0, 0);

		int dYear = static_cast<int>(core->getCurrentPlanet()->getSiderealPeriod()*graphsDuration) + 3;
		int firstGraph = ui->graphsFirstComboBox->currentData().toInt();
		int secondGraph = ui->graphsSecondComboBox->currentData().toInt();

		for (int i = -2; i <= dYear; i++)
		{
			JD = startJD + i;

			if (firstGraph==GraphTransitAltitudeVsTime || secondGraph==GraphTransitAltitudeVsTime)
			{
				core->setJD(JD);
				UTCshift = core->getUTCOffset(JD) / 24.; // Fix DST shift...
				Vec3f rts = ssObj->getRTSTime(core);
				JD += (rts[1]/24. - UTCshift);
			}

			ltime = (JD - startJD) * StelCore::ONE_OVER_JD_SECOND;
			aX.append(ltime);

			core->setJD(JD);

			aY.append(computeGraphValue(ssObj, firstGraph));
			bY.append(computeGraphValue(ssObj, secondGraph));

			core->update(0.0);
		}
		core->setJD(currentJD);

		QVector<double> x = aX.toVector(), ya = aY.toVector(), yb = bY.toVector();

		double minYa = *std::min_element(aY.begin(), aY.end());
		double maxYa = *std::max_element(aY.begin(), aY.end());

		width = (maxYa - minYa) / 50.0;
		minY1 = minYa - width;
		maxY1 = maxYa + width;

		minYa = *std::min_element(bY.begin(), bY.end());
		maxYa = *std::max_element(bY.begin(), bY.end());

		width = (maxYa - minYa) / 50.0;
		minY2 = minYa - width;
		maxY2 = maxYa + width;

		prepareXVsTimeAxesAndGraph();

		ui->graphsPlot->clearGraphs();

		ui->graphsPlot->addGraph(ui->graphsPlot->xAxis, ui->graphsPlot->yAxis);
		ui->graphsPlot->setBackground(QBrush(QColor(86, 87, 90)));
		ui->graphsPlot->graph(0)->setPen(QPen(Qt::green, 1));
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

		if (graphsDuration>1)
		{
			int JDshift = static_cast<int>(core->getCurrentPlanet()->getSiderealPeriod());
			QList<double> axj, ayj;
			for (int i = 0; i < graphsDuration; i++)
			{
				JD = startJD + i*JDshift;
				ltime = (JD - startJD) * StelCore::ONE_OVER_JD_SECOND;
				axj.append(ltime);
				axj.append(ltime);
				ayj.append(minY1);
				ayj.append(maxY1);
				QVector<double> xj = axj.toVector(), yj = ayj.toVector();
				int j = 2 + i;
				ui->graphsPlot->addGraph(ui->graphsPlot->xAxis, ui->graphsPlot->yAxis);
				ui->graphsPlot->graph(j)->setPen(QPen(Qt::red, 1, Qt::DashLine));
				ui->graphsPlot->graph(j)->setLineStyle(QCPGraph::lsLine);
				ui->graphsPlot->graph(j)->setData(xj, yj);
				ui->graphsPlot->graph(j)->setName(QString("[%1]").arg(j));
				axj.clear();
				ayj.clear();
			}
		}

		ui->graphsPlot->replot();
	}
}

double AstroCalcDialog::computeGraphValue(const PlanetP &ssObj, const int graphType)
{
	double value = 0.;
	switch (graphType)
	{
		case GraphMagnitudeVsTime:
			value = ssObj->getVMagnitude(core);
			break;
		case GraphPhaseVsTime:
			value = ssObj->getPhase(core->getObserverHeliocentricEclipticPos()) * 100.;
			break;
		case GraphDistanceVsTime:
			value =  ssObj->getJ2000EquatorialPos(core).length();
			break;
		case GraphElongationVsTime:
			value = ssObj->getElongation(core->getObserverHeliocentricEclipticPos()) * 180. / M_PI;
			break;
		case GraphAngularSizeVsTime:
		{
			value = ssObj->getAngularSize(core) * 360. / M_PI;
			if (value < 1.)
				value *= 60.;
			break;
		}
		case GraphPhaseAngleVsTime:
			value = ssObj->getPhaseAngle(core->getObserverHeliocentricEclipticPos()) * 180. / M_PI;
			break;
		case GraphHDistanceVsTime:
			value =  ssObj->getHeliocentricEclipticPos().length();
			break;
		case GraphTransitAltitudeVsTime:
		{
			double az, alt;
			bool sign;
			StelUtils::rectToSphe(&az, &alt, ssObj->getAltAzPosAuto(core));
			StelUtils::radToDecDeg(alt, sign, value); // convert to degrees
			if (!sign)
				value *= -1;
			break;
		}
		case GraphRightAscensionVsTime:
		{
			double dec_equ, ra_equ;
			StelUtils::rectToSphe(&ra_equ, &dec_equ, ssObj->getEquinoxEquatorialPos(core));
			ra_equ = 2.*M_PI-ra_equ;
			value = ra_equ*12./M_PI;
			if (value>24.)
				value -= 24.;
			break;
		}
		case GraphDeclinationVsTime:
		{
			double dec_equ, ra_equ;
			bool sign;
			StelUtils::rectToSphe(&ra_equ, &dec_equ, ssObj->getEquinoxEquatorialPos(core));
			StelUtils::radToDecDeg(dec_equ, sign, value); // convert to degrees
			if (!sign)
				value *= -1;
			break;
		}
	}
	return value;
}

void AstroCalcDialog::populateFunctionsList()
{
	Q_ASSERT(ui->graphsFirstComboBox);
	Q_ASSERT(ui->graphsSecondComboBox);

	typedef QPair<QString, GraphsTypes> graph;
	static const QList<graph> functions = {
		{ q_("Magnitude vs. Time"),    GraphMagnitudeVsTime},
		{ q_("Phase vs. Time"),        GraphPhaseVsTime},
		{ q_("Distance vs. Time"),     GraphDistanceVsTime},
		{ q_("Elongation vs. Time"),   GraphElongationVsTime},
		{ q_("Angular size vs. Time"), GraphAngularSizeVsTime},
		{ q_("Phase angle vs. Time"),  GraphPhaseAngleVsTime},
		// TRANSLATORS: The phrase "Heliocentric distance" may be long in some languages and you can short it to use in the drop-down list.
		{ q_("Heliocentric distance vs. Time"), GraphHDistanceVsTime},
		// TRANSLATORS: The phrase "Transit altitude" may be long in some languages and you can short it to use in the drop-down list.
		{ q_("Transit altitude vs. Time"), GraphTransitAltitudeVsTime},
		// TRANSLATORS: The phrase "Right ascension" may be long in some languages and you can short it to use in the drop-down list.
		{ q_("Right ascension vs. Time"), GraphRightAscensionVsTime},
		{ q_("Declination vs. Time"), GraphDeclinationVsTime}};

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

	for (const auto& f : functions)
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
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 6.0;
			direction1 = true;
			break;
		case GraphPhaseVsTime:
			yAxis1Legend = QString("%1, %").arg(q_("Phase"));
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 100.0;
			break;
		case GraphDistanceVsTime:
			yAxis1Legend = QString("%1, %2").arg(q_("Distance"), distMU);
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 50.0;
			break;
		case GraphElongationVsTime:
			yAxis1Legend = QString("%1, %2").arg(q_("Elongation"), QChar(0x00B0));
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 180.0;
			break;
		case GraphAngularSizeVsTime:
			yAxis1Legend = QString("%1, %2").arg(q_("Angular size"), asMU);
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 30.0;
			break;
		case GraphPhaseAngleVsTime:
			yAxis1Legend = QString("%1, %2").arg(q_("Phase angle"), QChar(0x00B0));
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 180.0;
			break;
		case GraphHDistanceVsTime:
			// TRANSLATORS: The phrase "Heliocentric distance" may be long in some languages and you can short it.
			yAxis1Legend = QString("%1, %2").arg(q_("Heliocentric distance"), distMU);
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 50.0;
			break;
		case GraphTransitAltitudeVsTime:
			// TRANSLATORS: The phrase "Transit altitude" may be long in some languages and you can short it.
			yAxis1Legend = QString("%1, %2").arg(q_("Transit altitude"), QChar(0x00B0));
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 90.0;
			break;
		case GraphRightAscensionVsTime:
			// TRANSLATORS: The phrase "Right ascension" may be long in some languages and you can short it.
			yAxis1Legend = QString("%1, %2").arg(qc_("Right ascension","axis name"), qc_("h","time"));
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 24.0;
			break;
		case GraphDeclinationVsTime:
			yAxis1Legend = QString("%1, %2").arg(q_("Declination"), QChar(0x00B0));
			if (minY1 < -1000.) minY1 = -90.0;
			if (maxY1 > 1000.) maxY1 = 90.0;
			break;
	}

	switch (ui->graphsSecondComboBox->currentData().toInt())
	{
		case GraphMagnitudeVsTime:
			yAxis2Legend = q_("Magnitude");
			if (minY2 < -1000.) minY2 = 0.0;
			if (maxY2 > 1000.) maxY2 = 6.0;
			direction2 = true;
			break;
		case GraphPhaseVsTime:
			yAxis2Legend = QString("%1, %").arg(q_("Phase"));
			if (minY2 < -1000.) minY2 = 0.0;
			if (maxY2 > 1000.) maxY2 = 100.0;
			break;
		case GraphDistanceVsTime:
			yAxis2Legend = QString("%1, %2").arg(q_("Distance"), distMU);
			if (minY2 < -1000.) minY2 = 0.0;
			if (maxY2 > 1000.) maxY2 = 50.0;
			break;
		case GraphElongationVsTime:
			yAxis2Legend = QString("%1, %2").arg(q_("Elongation"), QChar(0x00B0));
			if (minY2 < -1000.) minY2 = 0.0;
			if (maxY2 > 1000.) maxY2 = 180.0;
			break;
		case GraphAngularSizeVsTime:
			yAxis2Legend = QString("%1, %2").arg(q_("Angular size"), asMU);
			if (minY2 < -1000.) minY2 = 0.0;
			if (maxY2 > 1000.) maxY2 = 30.0;
			break;
		case GraphPhaseAngleVsTime:
			yAxis2Legend = QString("%1, %2").arg(q_("Phase angle"), QChar(0x00B0));
			if (minY2 < -1000.) minY2 = 0.0;
			if (maxY2 > 1000.) maxY2 = 180.0;
			break;
		case GraphHDistanceVsTime:
			// TRANSLATORS: The phrase "Heliocentric distance" may be long in some languages and you can short it.
			yAxis2Legend = QString("%1, %2").arg(q_("Heliocentric distance"), distMU);
			if (minY2 < -1000.) minY2 = 0.0;
			if (maxY2 > 1000.) maxY2 = 50.0;
			break;
		case GraphTransitAltitudeVsTime:
			// TRANSLATORS: The phrase "Transit altitude" may be long in some languages and you can short it.
			yAxis2Legend = QString("%1, %2").arg(q_("Transit altitude"), QChar(0x00B0));
			if (minY2 < -1000.) minY2 = 0.0;
			if (maxY2 > 1000.) maxY2 = 90.0;
			break;
		case GraphRightAscensionVsTime:
			// TRANSLATORS: The phrase "Right ascension" may be long in some languages and you can short it.
			yAxis2Legend = QString("%1, %2").arg(qc_("Right ascension","axis name"), qc_("h","time"));
			if (minY2 < -1000.) minY2 = 0.0;
			if (maxY2 > 1000.) maxY2 = 24.0;
			break;
		case GraphDeclinationVsTime:
			yAxis2Legend = QString("%1, %2").arg(q_("Declination"), QChar(0x00B0));
			if (minY2 < -1000.) minY2 = -90.0;
			if (maxY2 > 1000.) maxY2 = 90.0;
			break;
	}

	QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);
	QColor axisColorL(Qt::green);
	QPen axisPenL(axisColorL, 1);
	QColor axisColorR(Qt::yellow);
	QPen axisPenR(axisColorR, 1);

	ui->graphsPlot->setLocale(QLocale(localeMgr->getAppLanguage()));
	ui->graphsPlot->xAxis->setLabel(xAxisStr);
	ui->graphsPlot->yAxis->setLabel(yAxis1Legend);
	ui->graphsPlot->yAxis2->setLabel(yAxis2Legend);

	int dYear = (static_cast<int>(core->getCurrentPlanet()->getSiderealPeriod()*graphsDuration) + 1) * 86400;
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
	ui->graphsPlot->xAxis->setAutoTickCount(20);

	ui->graphsPlot->yAxis->setRange(minY1, maxY1);
	ui->graphsPlot->yAxis->setScaleType(QCPAxis::stLinear);
	ui->graphsPlot->yAxis->setLabelColor(axisColorL);
	ui->graphsPlot->yAxis->setTickLabelColor(axisColorL);
	ui->graphsPlot->yAxis->setBasePen(axisPenL);
	ui->graphsPlot->yAxis->setTickPen(axisPenL);
	ui->graphsPlot->yAxis->setSubTickPen(axisPenL);
	ui->graphsPlot->yAxis->setRangeReversed(direction1);

	ui->graphsPlot->yAxis2->setRange(minY2, maxY2);
	ui->graphsPlot->yAxis2->setScaleType(QCPAxis::stLinear);
	ui->graphsPlot->yAxis2->setLabelColor(axisColorR);
	ui->graphsPlot->yAxis2->setTickLabelColor(axisColorR);
	ui->graphsPlot->yAxis2->setBasePen(axisPenR);
	ui->graphsPlot->yAxis2->setTickPen(axisPenR);
	ui->graphsPlot->yAxis2->setSubTickPen(axisPenR);
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

	ui->monthlyElevationGraph->setLocale(QLocale(localeMgr->getAppLanguage()));
	ui->monthlyElevationGraph->xAxis->setLabel(xAxisStr);
	ui->monthlyElevationGraph->yAxis->setLabel(yAxisStr);

	int dYear = (static_cast<int>(core->getCurrentPlanet()->getSiderealPeriod()) + 1) * 86400;
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

void AstroCalcDialog::saveMonthlyElevationPositiveLimit(int limit)
{
	if (monthlyElevationPositiveLimit!=limit)
	{
		monthlyElevationPositiveLimit = limit;
		conf->setValue("astrocalc/me_positive_limit", monthlyElevationPositiveLimit);
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
	//..
	// we got notified about a reason to redraw the plot, but dialog was
	// not visible. which means we must redraw when becoming visible again!
	if (!dialog->isVisible() && plotMonthlyElevation)
	{
		graphPlotNeedsRefresh = true;
		return;
	}

	if (!plotMonthlyElevation) return;

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
		double az, alt, deg, startJD, JD, ltime;
		bool sign;
		int year, month, day;		
		StelUtils::getDateFromJulianDay(currentJD, &year, &month, &day);
		StelUtils::getJDFromDate(&startJD, year, 1, 1, hour, 0, 0);
		startJD -= core->getUTCOffset(startJD)/24; // Time zone correction
		int dYear = static_cast<int>(core->getCurrentPlanet()->getSiderealPeriod()/5.) + 3;
		for (int i = -2; i <= dYear; i++)
		{
			JD = startJD + i*5;
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
		minYme = *std::min_element(aY.begin(), aY.end()) - 2.0;
		maxYme = *std::max_element(aY.begin(), aY.end()) + 2.0;

		if (plotMonthlyElevationPositive && minYme<monthlyElevationPositiveLimit)
			minYme = monthlyElevationPositiveLimit;

		prepareMonthlyEleveationAxesAndGraph();

		QString name = selectedObject->getNameI18n();
		if (name.isEmpty())
		{
			QString otype = selectedObject->getType();
			if (otype == "Nebula")
				name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
			if (otype == "Star" || otype=="Pulsar")
				selectedObject->getID().isEmpty() ? name = q_("Unnamed star") : name = selectedObject->getID();
		}
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

// click inside AltVsTime graph area sets new current time
void AstroCalcDialog::altTimeClick(QMouseEvent* event)
{
	Qt::MouseButtons buttons = event->buttons();
	if (!(buttons & Qt::LeftButton)) return;

	double	x = ui->altVsTimePlot->xAxis->pixelToCoord(event->pos().x());
	double	y = ui->altVsTimePlot->yAxis->pixelToCoord(event->pos().y());

		if (x > ui->altVsTimePlot->xAxis->range().lower && x < ui->altVsTimePlot->xAxis->range().upper
			&& y > ui->altVsTimePlot->yAxis->range().lower && y < ui->altVsTimePlot->yAxis->range().upper)
		{
			setClickedTime(x);
		}
}

// click inside AziVsTime graph area sets new current time
void AstroCalcDialog::aziTimeClick(QMouseEvent* event)
{
	Qt::MouseButtons buttons = event->buttons();
	if (!(buttons & Qt::LeftButton)) return;

	double	x = ui->aziVsTimePlot->xAxis->pixelToCoord(event->pos().x());
	double	y = ui->aziVsTimePlot->yAxis->pixelToCoord(event->pos().y());

	if (x > ui->aziVsTimePlot->xAxis->range().lower && x < ui->aziVsTimePlot->xAxis->range().upper
		&& y > ui->aziVsTimePlot->yAxis->range().lower && y < ui->aziVsTimePlot->yAxis->range().upper)
	{
		setClickedTime(x);
	}
}


void AstroCalcDialog::setClickedTime(double posx)
{
	double JD = core->getJD();
	double shift = core->getUTCOffset(JD) / 24;
	int noonJD = static_cast<int>(JD + shift);
	JD = posx / 86400.0 + noonJD - 0.5 - shift;

	core->setRealTimeSpeed();
	core->setJD(JD);
	drawCurrentTimeDiagram();

	// if object is tracked, we make our own (smoothed) movement
	if (mvMgr->getFlagTracking())
	{
		StelObjectP obj = objectMgr->getSelectedObject()[0];
		mvMgr->moveToObject(obj, 0.4f);
	}
}

// When dialog becomes visible: check if there is a
// graph plot to refresh
void AstroCalcDialog::handleVisibleEnabled()
{
	if (dialog->isVisible())
	{
		// check which graph needs refresh (only one is set, if any)
		if (graphPlotNeedsRefresh)
		{
			if (plotAltVsTime || plotAziVsTime) 
				drawCurrentTimeDiagram();
			if (plotMonthlyElevation) 
				drawMonthlyElevationGraph();
			if (plotAngularDistanceGraph) 
				drawAngularDistanceGraph();
		}
		else
			drawCurrentTimeDiagram();
	}

	graphPlotNeedsRefresh = false;
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
				JD = transitX / 86400.0 + static_cast<int>(core->getJD()) - 0.5;
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
				JD = x / 86400.0 + static_cast<int>(core->getJD()) - 0.5;
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
	// TRANSLATORS: Magnitude of object 1
	phenomenaHeader << q_("Mag. 1");
	phenomenaHeader << q_("Object 2");
	// TRANSLATORS: Magnitude of object 2
	phenomenaHeader << q_("Mag. 2");
	phenomenaHeader << q_("Separation");
	phenomenaHeader << q_("Elevation");
	phenomenaHeader << q_("Solar Elongation");
	phenomenaHeader << q_("Lunar Elongation");
	ui->phenomenaTreeWidget->setHeaderLabels(phenomenaHeader);
	adjustPhenomenaColumns();
}

void AstroCalcDialog::adjustPhenomenaColumns()
{
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
	ui->phenomenaTreeWidget->header()->setDefaultAlignment(Qt::AlignHCenter);
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
	double separation = ui->allowedSeparationSpinBox->valueDegrees();
	bool opposition = ui->phenomenaOppositionCheckBox->isChecked();
	bool perihelion = ui->phenomenaPerihelionAphelionCheckBox->isChecked();

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
		case 0: // All Solar system objects
			for (const auto& object : allObjects)
			{
				if (object->getPlanetType() != Planet::isUNDEFINED && object->getPlanetType() != Planet::isObserver)
					objects.append(object);
			}
			break;
		case 1: // Planets
			for (const auto& object : allObjects)
			{
				if (object->getPlanetType() == Planet::isPlanet && object->getEnglishName() != core->getCurrentPlanet()->getEnglishName() && object->getEnglishName() != currentPlanet)
					objects.append(object);
			}
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 20:
		{
			static const QMap<int, Planet::PlanetType>map = {
				{2, Planet::isAsteroid},
				{3, Planet::isPlutino},
				{4, Planet::isComet},
				{5, Planet::isDwarfPlanet},
				{6, Planet::isCubewano},
				{7, Planet::isSDO},
				{8, Planet::isOCO},
				{9, Planet::isSednoid},
				{20, Planet::isInterstellar}};
			const Planet::PlanetType pType = map.value(obj2Type, Planet::isUNDEFINED);

			for (const auto& object : allObjects)
			{
				if (object->getPlanetType() == pType)
					objects.append(object);
			}
			break;
		}
		case 10: // Stars
			for (const auto& object : hipStars)
			{
				if (object->getVMagnitude(core) < (brightLimit - 5.0f))
					star.append(object);
			}
			break;
		case 11: // Double stars
			for (const auto& object : doubleHipStars)
			{
				if (object.firstKey()->getVMagnitude(core) < (brightLimit - 5.0f))
					star.append(object.firstKey());
			}
			break;
		case 12: // Variable stars
			for (const auto& object : variableHipStars)
			{
				if (object.firstKey()->getVMagnitude(core) < (brightLimit - 5.0f))
					star.append(object.firstKey());
			}
			break;
		case 13: // Star clusters
			for (const auto& object : allDSO)
			{
				if (object->getVMagnitude(core) < brightLimit && (object->getDSOType() == Nebula::NebCl || object->getDSOType() == Nebula::NebOc || object->getDSOType() == Nebula::NebGc || object->getDSOType() == Nebula::NebSA || object->getDSOType() == Nebula::NebSC || object->getDSOType() == Nebula::NebCn))
					dso.append(object);
			}
			break;
		case 14: // Planetary nebulae
			for (const auto& object : allDSO)
			{
				if (object->getVMagnitude(core) < brightLimit && (object->getDSOType() == Nebula::NebPn || object->getDSOType() == Nebula::NebPossPN || object->getDSOType() == Nebula::NebPPN))
					dso.append(object);
			}
			break;
		case 15: // Bright nebulae
			for (const auto& object : allDSO)
			{
				if (object->getVMagnitude(core) < brightLimit && (object->getDSOType() == Nebula::NebN || object->getDSOType() == Nebula::NebBn || object->getDSOType() == Nebula::NebEn || object->getDSOType() == Nebula::NebRn || object->getDSOType() == Nebula::NebHII || object->getDSOType() == Nebula::NebISM || object->getDSOType() == Nebula::NebCn || object->getDSOType() == Nebula::NebSNR))
					dso.append(object);
			}
			break;
		case 16: // Dark nebulae
			for (const auto& object : allDSO)
			{
				if (object->getDSOType() == Nebula::NebDn || object->getDSOType() == Nebula::NebMolCld || object->getDSOType() == Nebula::NebYSO)
					dso.append(object);
			}
			break;
		case 17: // Galaxies
			for (const auto& object : allDSO)
			{
				if (object->getVMagnitude(core) < brightLimit && (object->getDSOType() == Nebula::NebGx || object->getDSOType() == Nebula::NebAGx || object->getDSOType() == Nebula::NebRGx || object->getDSOType() == Nebula::NebQSO || object->getDSOType() == Nebula::NebPossQSO || object->getDSOType() == Nebula::NebBLL || object->getDSOType() == Nebula::NebBLA || object->getDSOType() == Nebula::NebIGx))
					dso.append(object);
			}
			break;
		case 18: // Symbiotic stars
			for (const auto& object : allDSO)
			{
				if (object->getDSOType() == Nebula::NebSymbioticStar)
					dso.append(object);
			}
			break;
		case 19: // Emission-line stars
			for (const auto& object : allDSO)
			{
				if (object->getDSOType() == Nebula::NebEmissionLineStar)
					dso.append(object);
			}
			break;
		case 21: // Planets and Sun
			for (const auto& object : allObjects)
			{
				if ((object->getPlanetType() == Planet::isPlanet || object->getPlanetType() == Planet::isStar) && object->getEnglishName() != core->getCurrentPlanet()->getEnglishName() && object->getEnglishName() != currentPlanet)
					objects.append(object);
			}
			break;
		case 22: // Sun, planets and moons
		{
			PlanetP cp = core->getCurrentPlanet();
			for (const auto& object : allObjects)
			{
				if ((object->getPlanetType() == Planet::isPlanet || object->getPlanetType() == Planet::isStar || (object->getParent()==cp && object->getPlanetType()==Planet::isMoon)) && object->getEnglishName() != cp->getEnglishName() && object->getEnglishName() != currentPlanet)
					objects.append(object);
			}
			break;
		}
		case 23: // Bright Solar system objects
			for (const auto& object : allObjects)
			{
				if (object->getVMagnitude(core) < (brightLimit + 2.0f) && object->getPlanetType() != Planet::isUNDEFINED)
					objects.append(object);
			}
			break;
	}

	PlanetP planet = solarSystem->searchByEnglishName(currentPlanet);
	PlanetP sun = solarSystem->getSun();
	if (planet)
	{
		double currentJD = core->getJD();   // save current JD
		double startJD = StelUtils::qDateTimeToJd(QDateTime(ui->phenomenFromDateEdit->date()));
		double stopJD = StelUtils::qDateTimeToJd(QDateTime(ui->phenomenToDateEdit->date().addDays(1)));
		startJD = startJD - core->getUTCOffset(startJD) / 24.;
		stopJD = stopJD - core->getUTCOffset(stopJD) / 24.;

		if (obj2Type == -1)
		{
			QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();
			if (!selectedObjects.isEmpty())
			{
				StelObjectP selectedObject = selectedObjects[0];
				if (selectedObject!=planet && selectedObject->getType() != "Satellite")
				{
					// conjunction
					fillPhenomenaTable(findClosestApproach(planet, selectedObject, startJD, stopJD, separation, PhenomenaTypeIndex::Conjuction), planet, selectedObject, PhenomenaTypeIndex::Conjuction);
					// opposition
					if (opposition)
						fillPhenomenaTable(findClosestApproach(planet, selectedObject, startJD, stopJD, separation, PhenomenaTypeIndex::Opposition), planet, selectedObject, PhenomenaTypeIndex::Opposition);
				}
			}
		}
		else if ((obj2Type >= 0 && obj2Type < 10) || (obj2Type >= 20 && obj2Type <= 23))
		{
			// Solar system objects
			for (auto& obj : objects)
			{
				// conjunction
				StelObjectP mObj = qSharedPointerCast<StelObject>(obj);
				fillPhenomenaTable(findClosestApproach(planet, mObj, startJD, stopJD, separation, PhenomenaTypeIndex::Conjuction), planet, obj, PhenomenaTypeIndex::Conjuction);
				// opposition
				if (opposition)
					fillPhenomenaTable(findClosestApproach(planet, mObj, startJD, stopJD, separation, PhenomenaTypeIndex::Opposition), planet, obj, PhenomenaTypeIndex::Opposition);
			}
		}
		else if (obj2Type == 10 || obj2Type == 11 || obj2Type == 12)
		{
			// Stars
			for (auto& obj : star)
			{
				// conjunction
				StelObjectP mObj = qSharedPointerCast<StelObject>(obj);
				fillPhenomenaTable(findClosestApproach(planet, mObj, startJD, stopJD, separation, PhenomenaTypeIndex::Conjuction), planet, obj, PhenomenaTypeIndex::Conjuction);
			}
		}
		else
		{
			// Deep-sky objects
			for (auto& obj : dso)
			{
				// conjunction
				StelObjectP mObj = qSharedPointerCast<StelObject>(obj);
				fillPhenomenaTable(findClosestApproach(planet, mObj, startJD, stopJD, separation, PhenomenaTypeIndex::Conjuction), planet, obj);
			}
		}

		if (planet!=sun && planet->getPlanetType()!=Planet::isMoon)
		{
			StelObjectP mObj = qSharedPointerCast<StelObject>(sun);
			if (planet->getHeliocentricEclipticPos().length()<core->getCurrentPlanet()->getHeliocentricEclipticPos().length())
			{
				// greatest elongations for inner planets
				fillPhenomenaTable(findGreatestElongationApproach(planet, mObj, startJD, stopJD), planet, sun, PhenomenaTypeIndex::GreatestElongation);
			}
			// stationary points
			fillPhenomenaTable(findStationaryPointApproach(planet, startJD, stopJD), planet, sun, PhenomenaTypeIndex::StationaryPoint);
			// perihelion and aphelion points
			if (perihelion)
				fillPhenomenaTable(findOrbitalPointApproach(planet, startJD, stopJD), planet, sun, PhenomenaTypeIndex::OrbitalPoint);
		}

		core->setJD(currentJD); // restore time
		core->update(0);
	}

	// adjust the column width
	adjustPhenomenaColumns();
	// sort-by-date
	ui->phenomenaTreeWidget->sortItems(PhenomenaDate, Qt::AscendingOrder);	
}

void AstroCalcDialog::savePhenomena()
{
	QString filter = q_("Microsoft Excel Open XML Spreadsheet");
	filter.append(" (*.xlsx);;");
	filter.append(q_("CSV (Comma delimited)"));
	filter.append(" (*.csv)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save calculated phenomena as..."),
							QDir::homePath() + "/phenomena.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->phenomenaTreeWidget, phenomenaHeader);
	else
	{
		int count = ui->phenomenaTreeWidget->topLevelItemCount();
		int columns = phenomenaHeader.size();

		int *width;
		width = new int[columns];
		QString sData;
		int w;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("Phenomena"));
		xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
		xlsx.addSheet(q_("Phenomena"), AbstractSheet::ST_WorkSheet);

		QXlsx::Format header;
		header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		header.setPatternBackgroundColor(Qt::yellow);
		header.setBorderStyle(QXlsx::Format::BorderThin);
		header.setBorderColor(Qt::black);
		header.setFontBold(true);
		for (int i = 0; i < columns; i++)
		{
			// Row 1: Names of columns
			sData = phenomenaHeader.at(i).trimmed();
			xlsx.write(1, i + 1, sData, header);
			width[i] = sData.size();
		}

		QXlsx::Format data;
		data.setHorizontalAlignment(QXlsx::Format::AlignRight);
		for (int i = 0; i < count; i++)
		{
			for (int j = 0; j < columns; j++)
			{
				// Row 2 and next: the data
				sData = ui->phenomenaTreeWidget->topLevelItem(i)->text(j).trimmed();
				xlsx.write(i + 2, j + 1, sData, data);
				w = sData.size();
				if (w > width[j])
				{
					width[j] = w;
				}
			}
		}

		for (int i = 0; i < columns; i++)
		{
			xlsx.setColumnWidth(i+1, width[i]+2);
		}

		delete[] width;
		xlsx.saveAs(filePath);
	}
}

void AstroCalcDialog::fillPhenomenaTableVis(QString phenomenType, double JD, QString firstObjectName, float firstObjectMagnitude,
					    QString secondObjectName, float secondObjectMagnitude, QString separation, QString elevation,
					    QString elongation, QString angularDistance, QString elongTooltip, QString angDistTooltip)
{
	ACPhenTreeWidgetItem* treeItem = new ACPhenTreeWidgetItem(ui->phenomenaTreeWidget);
	treeItem->setText(PhenomenaType, phenomenType);
	// local date and time
	treeItem->setText(PhenomenaDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD)));
	treeItem->setData(PhenomenaDate, Qt::UserRole, JD);
	treeItem->setText(PhenomenaObject1, firstObjectName);
	if (firstObjectMagnitude > 90.f)
		treeItem->setText(PhenomenaMagnitude1, dash);
	else
		treeItem->setText(PhenomenaMagnitude1, QString::number(firstObjectMagnitude, 'f', 2));
	treeItem->setTextAlignment(PhenomenaMagnitude1, Qt::AlignRight);
	treeItem->setToolTip(PhenomenaMagnitude1, q_("Magnitude of first object"));
	treeItem->setText(PhenomenaObject2, secondObjectName);
	if (secondObjectMagnitude > 90.f)
		treeItem->setText(PhenomenaMagnitude2, dash);
	else
		treeItem->setText(PhenomenaMagnitude2, QString::number(secondObjectMagnitude, 'f', 2));
	treeItem->setToolTip(PhenomenaMagnitude2, q_("Magnitude of second object"));
	treeItem->setTextAlignment(PhenomenaMagnitude2, Qt::AlignRight);	
	treeItem->setText(PhenomenaSeparation, separation);
	treeItem->setTextAlignment(PhenomenaSeparation, Qt::AlignRight);
	treeItem->setText(PhenomenaElevation, elevation);
	treeItem->setTextAlignment(PhenomenaElevation, Qt::AlignRight);
	treeItem->setToolTip(PhenomenaElevation, q_("Elevation of first object at moment of phenomena"));
	treeItem->setText(PhenomenaElongation, elongation);
	if (elongTooltip.isEmpty())
		treeItem->setToolTip(PhenomenaElongation, q_("Angular distance from the Sun"));
	else
		treeItem->setToolTip(PhenomenaElongation, elongTooltip);
	treeItem->setTextAlignment(PhenomenaElongation, Qt::AlignRight);
	treeItem->setText(PhenomenaAngularDistance, angularDistance);
	if (angDistTooltip.isEmpty())
		treeItem->setToolTip(PhenomenaAngularDistance, q_("Angular distance from the Moon"));
	else
		treeItem->setToolTip(PhenomenaAngularDistance, angDistTooltip);
	treeItem->setTextAlignment(PhenomenaAngularDistance, Qt::AlignRight);
}

void AstroCalcDialog::fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const PlanetP object2, int mode)
{
	QMap<double, double>::ConstIterator it;
	PlanetP sun = solarSystem->getSun();
	PlanetP moon = solarSystem->getMoon();
	PlanetP earth = solarSystem->getEarth();
	PlanetP planet = core->getCurrentPlanet();
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	double az, alt;
	for (it = list.constBegin(); it != list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		bool occultation = false;
		const double s1 = object1->getSpheroidAngularSize(core);
		const double s2 = object2->getSpheroidAngularSize(core);
		const double d1 = object1->getJ2000EquatorialPos(core).length();
		const double d2 = object2->getJ2000EquatorialPos(core).length();
		if (mode==PhenomenaTypeIndex::Opposition) // opposition
		{
			phenomenType = q_("Opposition");
			// Added a special case - lunar eclipse
			if (qAbs(separation) <= 0.02 && ((object1 == moon  && object2 == sun) || (object1 == sun  && object2 == moon)))
				phenomenType = q_("Eclipse");

			separation = M_PI - separation;
		}
		else if (mode==PhenomenaTypeIndex::GreatestElongation) // greatest elongations
		{
			if (separation < 0.0) // we use negative value for eastern elongations!
			{
				separation *= -1.0;
				phenomenType = q_("Greatest eastern elongation");
			}
			else
				phenomenType = q_("Greatest western elongation");
		}
		else if (mode==PhenomenaTypeIndex::StationaryPoint) // stationary points
		{
			if (separation < 0.0) // we use negative value for start retrograde motion!
			{
				// TRANSLATORS: The planet are stand still in the equatorial coordinates
				phenomenType = q_("Stationary (begin retrograde motion)");
			}
			else
			{
				// TRANSLATORS: The planet are stand still in the equatorial coordinates
				phenomenType = q_("Stationary (begin prograde motion)");
			}
		}
		else if (mode==PhenomenaTypeIndex::OrbitalPoint)
		{
			if (separation < 0.0) // we use negative value for perihelion!
				phenomenType = q_("Perihelion");
			else
				phenomenType = q_("Aphelion");
		}
		else if (separation < (s2 * M_PI / 180.) || separation < (s1 * M_PI / 180.))
		{
			if ((d1 < d2 && s1 <= s2) || (d1 > d2 && s1 > s2))
			{
				// The passage of the celestial body in front of another of greater apparent diameter
				phenomenType = qc_("Transit", "passage of the celestial body");
			}
			else
				phenomenType = q_("Occultation");

			// Added a special case - solar eclipse
			if (qAbs(s1 - s2) <= 0.05 && (object1 == sun || object2 == sun)) // 5% error of difference of sizes
				phenomenType = q_("Eclipse");

			occultation = true;
		}
		else if (qAbs(separation) <= 0.0087 && ((object1 == moon  && object2 == sun) || (object1 == sun  && object2 == moon))) // Added a special case - partial solar eclipse
		{
			phenomenType = q_("Eclipse");
		}
		else if (object1 == sun || object2 == sun) // this is may be superior of inferior conjuction for inner planet
		{
			double dcp = planet->getHeliocentricEclipticPos().length();
			double dp;
			if (object1 == sun)
				dp = object2->getHeliocentricEclipticPos().length();
			else
				dp = object1->getHeliocentricEclipticPos().length();
			if (dp < dcp) // OK, it's inner planet
			{
				if (object1 == sun)
				{
					if (d1<d2)
						phenomenType = q_("Superior conjunction");
					else
						phenomenType = q_("Inferior conjunction");
				}
				else
				{
					if (d2<d1)
						phenomenType = q_("Superior conjunction");
					else
						phenomenType = q_("Inferior conjunction");
				}				
			}
		}

		QString elongStr = "";
		if (((object1 == sun || object2 == sun) && mode==PhenomenaTypeIndex::Conjuction) || (object2 == sun && mode==PhenomenaTypeIndex::Opposition))
			elongStr = dash;
		else
		{
			double elongation = object1->getElongation(core->getObserverHeliocentricEclipticPos());
			if (mode==PhenomenaTypeIndex::Opposition) // calculate elongation for the second object in this case!
				elongation = object2->getElongation(core->getObserverHeliocentricEclipticPos());

			if (withDecimalDegree)
				elongStr = StelUtils::radToDecDegStr(elongation, 5, false, true);
			else
				elongStr = StelUtils::radToDmsStr(elongation, true);
		}

		QString angDistStr = "";
		if (planet != earth)
			angDistStr = dash;
		else
		{
			if (object1 == moon || object2 == moon)
				angDistStr = dash;
			else
			{
				double angularDistance = object1->getJ2000EquatorialPos(core).angle(moon->getJ2000EquatorialPos(core));
				if (mode==1) // calculate elongation for the second object in this case!
					angularDistance = object2->getJ2000EquatorialPos(core).angle(moon->getJ2000EquatorialPos(core));

				if (withDecimalDegree)
					angDistStr = StelUtils::radToDecDegStr(angularDistance, 5, false, true);
				else
					angDistStr = StelUtils::radToDmsStr(angularDistance, true);
			}
		}

		QString elongationInfo = q_("Angular distance from the Sun");
		QString angularDistanceInfo = q_("Angular distance from the Moon");
		if (mode==1)
		{
			elongationInfo = q_("Angular distance from the Sun for second object");
			angularDistanceInfo = q_("Angular distance from the Moon for second object");
		}

		QString separationStr = dash;
		float magnitude = object2->getVMagnitude(core);
		if (!occultation)
		{			
			if (withDecimalDegree)
				separationStr = StelUtils::radToDecDegStr(separation, 5, false, true);
			else
				separationStr = StelUtils::radToDmsStr(separation, true);
		}
		else
			magnitude = 99.f; // Let's hide obviously wrong data

		QString nameObj2 = object2->getNameI18n();
		if (mode==PhenomenaTypeIndex::StationaryPoint)
		{
			nameObj2 = dash;
			magnitude = 99.f;
			separationStr = dash;
		}

		QString elevationStr = dash;
		StelUtils::rectToSphe(&az, &alt, object1->getAltAzPosAuto(core));
		if (withDecimalDegree)
			elevationStr = StelUtils::radToDecDegStr(alt, 5, false, true);
		else
			elevationStr = StelUtils::radToDmsPStr(alt, 2);

		fillPhenomenaTableVis(phenomenType, it.key(), object1->getNameI18n(), object1->getVMagnitude(core), nameObj2, magnitude, separationStr, elevationStr, elongStr, angDistStr, elongationInfo, angularDistanceInfo);
	}
}

void AstroCalcDialog::fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const NebulaP object2)
{
	QMap<double, double>::ConstIterator it;
	PlanetP sun = solarSystem->getSun();
	PlanetP moon = solarSystem->getMoon();
	PlanetP earth = solarSystem->getEarth();
	PlanetP planet = core->getCurrentPlanet();
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	double az, alt;
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

		QString elongStr = "";
		if (object1 == sun)
			elongStr = dash;
		else
		{
			if (withDecimalDegree)
				elongStr = StelUtils::radToDecDegStr(object1->getElongation(core->getObserverHeliocentricEclipticPos()), 5, false, true);
			else
				elongStr = StelUtils::radToDmsStr(object1->getElongation(core->getObserverHeliocentricEclipticPos()), true);
		}

		QString angDistStr = "";
		if (planet != earth)
			angDistStr = dash;
		else
		{
			if (object1 == moon)
				angDistStr = dash;
			else
			{
				double angularDistance = object1->getJ2000EquatorialPos(core).angle(moon->getJ2000EquatorialPos(core));
				if (withDecimalDegree)
					angDistStr = StelUtils::radToDecDegStr(angularDistance, 5, false, true);
				else
					angDistStr = StelUtils::radToDmsStr(angularDistance, true);
			}
		}

		QString commonName = object2->getNameI18n();
		if (commonName.isEmpty())
			commonName = object2->getDSODesignation();

		QString separationStr = dash;
		float magnitude = object2->getVMagnitude(core);
		if (!occultation)
		{
			if (withDecimalDegree)
				separationStr = StelUtils::radToDecDegStr(separation, 5, false, true);
			else
				separationStr = StelUtils::radToDmsStr(separation, true);
		}
		else
			magnitude = 99.f; // Let's hide obviously wrong data

		QString elevationStr = dash;
		StelUtils::rectToSphe(&az, &alt, object1->getAltAzPosAuto(core));
		if (withDecimalDegree)
			elevationStr = StelUtils::radToDecDegStr(alt, 5, false, true);
		else
			elevationStr = StelUtils::radToDmsPStr(alt, 2);

		fillPhenomenaTableVis(phenomenType, it.key(), object1->getNameI18n(), object1->getVMagnitude(core), commonName, magnitude, separationStr, elevationStr, elongStr, angDistStr);
	}
}

void AstroCalcDialog::fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const StelObjectP object2, int mode = 0)
{
	QMap<double, double>::ConstIterator it;
	PlanetP sun = solarSystem->getSun();
	PlanetP moon = solarSystem->getMoon();
	PlanetP earth = solarSystem->getEarth();
	PlanetP planet = core->getCurrentPlanet();
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	double az, alt;
	for (it = list.constBegin(); it != list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		bool occultation = false;		
		const double s1 = object1->getSpheroidAngularSize(core);
		const double s2 = object2->getAngularSize(core);
		const double d1 = object1->getJ2000EquatorialPos(core).length();
		const double d2 = object2->getJ2000EquatorialPos(core).length();
		if (mode==PhenomenaTypeIndex::Opposition)
		{
			phenomenType = q_("Opposition");
			// Added a special case - lunar eclipse
			if (qAbs(separation) <= 0.02 && ((object1 == moon  && object2 == sun) || (object1 == sun  && object2 == moon)))
				phenomenType = q_("Eclipse");

			separation = M_PI - separation;
		}
		else if (separation < (s2 * M_PI / 180.) || separation < (s1 * M_PI / 180.))
		{
			if ((d1 < d2 && s1 <= s2) || (d1 > d2 && s1 > s2))
			{
				// The passage of the celestial body in front of another of greater apparent diameter
				phenomenType = qc_("Transit", "passage of the celestial body");
			}
			else
				phenomenType = q_("Occultation");

			// Added a special case - solar eclipse
			if (qAbs(s1 - s2) <= 0.05 && (object1 == sun || object2 == sun)) // 5% error of difference of sizes
				phenomenType = q_("Eclipse");

			occultation = true;
		}
		else if (qAbs(separation) <= 0.0087 && ((object1 == moon  && object2 == sun) || (object1 == sun  && object2 == moon))) // Added a special case - partial solar eclipse
		{
			phenomenType = q_("Eclipse");
		}
		else if (object1 == sun || object2 == sun) // this is may be superior of inferior conjuction for inner planet
		{
			double dcp = (planet->getEquinoxEquatorialPos(core) - sun->getEquinoxEquatorialPos(core)).length();
			double dp;
			if (object1 == sun)
				dp = (object2->getEquinoxEquatorialPos(core) - sun->getEquinoxEquatorialPos(core)).length();
			else
				dp = (object1->getEquinoxEquatorialPos(core) - sun->getEquinoxEquatorialPos(core)).length();
			if (dp < dcp) // OK, it's inner planet
			{
				if (object1 == sun)
				{
					if (d1<d2)
						phenomenType = q_("Superior conjunction");
					else
						phenomenType = q_("Inferior conjunction");
				}
				else
				{
					if (d2<d1)
						phenomenType = q_("Superior conjunction");
					else
						phenomenType = q_("Inferior conjunction");
				}
			}
		}

		QString elongStr = "";
		if (object1 == sun)
			elongStr = dash;
		else
		{
			if (withDecimalDegree)
				elongStr = StelUtils::radToDecDegStr(object1->getElongation(core->getObserverHeliocentricEclipticPos()), 5, false, true);
			else
				elongStr = StelUtils::radToDmsStr(object1->getElongation(core->getObserverHeliocentricEclipticPos()), true);
		}

		QString angDistStr = "";
		if (planet != earth)
			angDistStr = dash;
		else
		{
			if (object1 == moon)
				angDistStr = dash;
			else
			{
				double angularDistance = object1->getJ2000EquatorialPos(core).angle(moon->getJ2000EquatorialPos(core));
				if (withDecimalDegree)
					angDistStr = StelUtils::radToDecDegStr(angularDistance, 5, false, true);
				else
					angDistStr = StelUtils::radToDmsStr(angularDistance, true);
			}
		}

		QString commonName = object2->getNameI18n();
		if (commonName.isEmpty())
			commonName = object2->getID();

		QString separationStr = dash;
		float magnitude = object2->getVMagnitude(core);
		if (!occultation)
		{
			if (withDecimalDegree)
				separationStr = StelUtils::radToDecDegStr(separation, 5, false, true);
			else
				separationStr = StelUtils::radToDmsStr(separation, true);
		}
		else
			magnitude = 99.f; // Let's hide obviously wrong data

		QString elevationStr = dash;
		StelUtils::rectToSphe(&az, &alt, object1->getAltAzPosAuto(core));
		if (withDecimalDegree)
			elevationStr = StelUtils::radToDecDegStr(alt, 5, false, true);
		else
			elevationStr = StelUtils::radToDmsPStr(alt, 2);

		fillPhenomenaTableVis(phenomenType, it.key(), object1->getNameI18n(), object1->getVMagnitude(core), commonName, magnitude, separationStr, elevationStr, elongStr, angDistStr);
	}
}

double AstroCalcDialog::findInitialStep(double startJD, double stopJD, QStringList objects)
{
	double step = (stopJD - startJD) / 16.0;
	double limit = 24.8 * 365.25;

	if (objects.contains("Moon", Qt::CaseInsensitive) || objects.contains("Sun", Qt::CaseInsensitive))
		limit = 0.25;
	else if (objects.contains("C/",Qt::CaseInsensitive) || objects.contains("P/",Qt::CaseInsensitive))
		limit = 0.5;
	else if (objects.contains("Earth",Qt::CaseInsensitive))
		limit = 1.;
	else if (objects.contains("Venus",Qt::CaseInsensitive) || objects.contains("Mercury", Qt::CaseInsensitive))
		limit = 2.5;
	else if (objects.contains("Mars",Qt::CaseInsensitive))
		limit = 5.;
	else if (objects.contains("Ceres",Qt::CaseInsensitive) || objects.contains("Juno",Qt::CaseInsensitive) || objects.contains("Pallas",Qt::CaseInsensitive) || objects.contains("Vesta",Qt::CaseInsensitive))
		limit = 45.28125;
	else if (objects.contains("Jupiter", Qt::CaseInsensitive) || objects.contains("Saturn", Qt::CaseInsensitive))
		limit = 90.5625;
	else if (objects.contains("Neptune", Qt::CaseInsensitive) || objects.contains("Uranus", Qt::CaseInsensitive) || objects.contains("Pluto",Qt::CaseInsensitive))
		limit = 181.125;

	if (step > limit)
		step = limit;

	return step;
}

QMap<double, double> AstroCalcDialog::findClosestApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD, double maxSeparation, int mode)
{
	double dist, prevDist, step, step0;
	int sgn, prevSgn = 0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	QStringList objects;
	objects.clear();
	objects.append(object1->getEnglishName());
	objects.append(object2->getEnglishName());
	step0 = findInitialStep(startJD, stopJD, objects);
	step = step0;
	double jd = startJD;
	prevDist = findDistance(jd, object1, object2, mode);
	jd += step;
	while (jd <= stopJD)
	{
		dist = findDistance(jd, object1, object2, mode);
		sgn = StelUtils::sign(dist - prevDist);

		double factor = qAbs((dist - prevDist) / dist);
		if (factor > 10.)
			step = step0 * factor / 10.;
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
					dist = findDistance(jd, object1, object2, mode);
					sgn = StelUtils::sign(dist - prevDist);
					if (sgn != prevSgn)
						break;

					prevDist = dist;
					prevSgn = sgn;
					jd += step;
				}
			}

			if (findPrecise(&extremum, object1, object2, jd, step, sgn, mode))
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

bool AstroCalcDialog::findPrecise(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double step, int prevSign, int mode)
{
	int sgn;
	double dist, prevDist;

	if (out == Q_NULLPTR)
		return false;

	prevDist = findDistance(JD, object1, object2, mode);
	step = -step / 2.;
	prevSign = -prevSign;

	while (true)
	{
		JD += step;
		dist = findDistance(JD, object1, object2, mode);

		if (qAbs(step) < 1. / 1440.)
		{
			out->first = JD - step / 2.0;
			out->second = findDistance(JD - step / 2.0, object1, object2, mode);
			if (out->second < findDistance(JD - 5.0, object1, object2, mode))
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

double AstroCalcDialog::findDistance(double JD, PlanetP object1, StelObjectP object2, int mode)
{
	core->setJD(JD);
	core->update(0);
	double angle = object1->getJ2000EquatorialPos(core).angle(object2->getJ2000EquatorialPos(core));
	if (mode==PhenomenaTypeIndex::Opposition)
		angle = M_PI - angle;	
	return angle;
}

QMap<double, double> AstroCalcDialog::findGreatestElongationApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD)
{
	double dist, prevDist, step, step0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	QStringList objects;
	objects.clear();
	objects.append(object1->getEnglishName());
	objects.append(object2->getEnglishName());
	step0 = findInitialStep(startJD, stopJD, objects);
	step = step0;
	double jd = startJD;
	prevDist = findDistance(jd, object1, object2, PhenomenaTypeIndex::Conjuction);
	jd += step;
	while (jd <= stopJD)
	{
		dist = findDistance(jd, object1, object2, PhenomenaTypeIndex::Conjuction);
		double factor = qAbs((dist - prevDist) / dist);
		if (factor > 10.)
			step = step0 * factor / 10.;
		else
			step = step0;

		if (dist>prevDist)
		{
			if (step > step0)
			{
				jd -= step;
				step = step0;
				while (jd <= stopJD)
				{
					dist = findDistance(jd, object1, object2, PhenomenaTypeIndex::Conjuction);
					if (dist<prevDist)
						break;

					prevDist = dist;
					jd += step;
				}
			}

			if (findPreciseGreatestElongation(&extremum, object1, object2, jd, stopJD, step))
			{
				separations.insert(extremum.first, extremum.second);
			}
		}

		prevDist = dist;
		jd += step;
	}
	return separations;
}

bool AstroCalcDialog::findPreciseGreatestElongation(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double stopJD, double step)
{
	double dist, prevDist;

	if (out == Q_NULLPTR)
		return false;

	prevDist = findDistance(JD, object1, object2, PhenomenaTypeIndex::Conjuction);
	step = -step / 2.;

	while (true)
	{
		JD += step;
		dist = findDistance(JD, object1, object2, PhenomenaTypeIndex::Conjuction);

		if (qAbs(step) < 1. / 1440.)
		{
			out->first = JD - step / 2.0;
			out->second = findDistance(JD - step / 2.0, object1, object2, PhenomenaTypeIndex::Conjuction);
			if (out->second > findDistance(JD - 5.0, object1, object2, PhenomenaTypeIndex::Conjuction))
			{
				if (object1->getJ2000EquatorialPos(core).longitude()>object2->getJ2000EquatorialPos(core).longitude())
					out->second *= -1.0; // let's use negative value for eastern elongations
				return true;
			}
			else
				return false;
		}
		if (dist<prevDist)
		{
			step = -step / 2.0;
		}
		prevDist = dist;

		if (JD > stopJD)
			return false;
	}
}

QMap<double, double> AstroCalcDialog::findStationaryPointApproach(PlanetP &object1, double startJD, double stopJD)
{
	double RA, prevRA, step, step0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	QStringList objects;
	objects.clear();
	objects.append(object1->getEnglishName());
	step0 = findInitialStep(startJD, stopJD, objects);
	step = step0;
	double jd = startJD;
	prevRA = findRightAscension(jd, object1);
	jd += step;
	while (jd <= stopJD)
	{
		RA = findRightAscension(jd, object1);
		double factor = qAbs((RA - prevRA) / RA);
		if (factor > 10.)
			step = step0 * factor / 10.;
		else
			step = step0;

		if (RA>prevRA && qAbs(RA - prevRA)<180.)
		{
			if (step > step0)
			{
				jd -= step;
				step = step0;
				while (jd <= stopJD)
				{
					RA = findRightAscension(jd, object1);
					if (RA<prevRA)
						break;

					prevRA = RA;
					jd += step;
				}
			}

			if (findPreciseStationaryPoint(&extremum, object1, jd, stopJD, step, true))
			{
				separations.insert(extremum.first, extremum.second);
			}
		}
		prevRA = RA;
		jd += step;
	}

	step0 = findInitialStep(startJD, stopJD, objects);
	step = step0;
	jd = startJD;
	prevRA = findRightAscension(jd, object1);
	jd += step;
	while (jd <= stopJD)
	{
		RA = findRightAscension(jd, object1);
		double factor = qAbs((RA - prevRA) / RA);
		if (factor > 10.)
			step = step0 * factor / 10.;
		else
			step = step0;

		if (RA<prevRA && qAbs(RA - prevRA)<180.)
		{
			if (step > step0)
			{
				jd -= step;
				step = step0;
				while (jd <= stopJD)
				{
					RA = findRightAscension(jd, object1);
					if (RA>prevRA)
						break;

					prevRA = RA;
					jd += step;
				}
			}

			if (findPreciseStationaryPoint(&extremum, object1, jd, stopJD, step, false))
			{
				separations.insert(extremum.first, extremum.second);
			}
		}
		prevRA = RA;
		jd += step;
	}

	return separations;
}

bool AstroCalcDialog::findPreciseStationaryPoint(QPair<double, double> *out, PlanetP object, double JD, double stopJD, double step, bool retrograde)
{
	double RA, prevRA;

	if (out == Q_NULLPTR)
		return false;

	prevRA = findRightAscension(JD, object);
	step = -step / 2.;

	while (true)
	{
		JD += step;
		RA = findRightAscension(JD, object);

		if (qAbs(step) < 1. / 1440.)
		{
			out->first = JD - step / 2.0;
			out->second = findRightAscension(JD - step / 2.0, object);
			if (retrograde) // begin retrograde motion
			{
				if (out->second > findRightAscension(JD - 5.0, object))
				{
					out->second = -1.0;
					return true;
				}
				else
					return false;
			}
			else
			{
				if (out->second < findRightAscension(JD - 5.0, object))
				{
					out->second = 1.0;
					return true;
				}
				else
					return false;
			}
		}
		if (retrograde)
		{
			if (RA<prevRA)
				step = -step / 2.0;
		}
		else
		{
			if (RA>prevRA)
				step = -step / 2.0;
		}
		prevRA = RA;

		if (JD > stopJD)
			return false;
	}
}

double AstroCalcDialog::findRightAscension(double JD, PlanetP object)
{
	core->setJD(JD);
	core->update(0);
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, object->getJ2000EquatorialPos(core));
	return ra*M_180_PI;
}

QMap<double, double> AstroCalcDialog::findOrbitalPointApproach(PlanetP &object1, double startJD, double stopJD)
{
	double distance, prevDistance, step, step0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	QStringList objects;
	objects.clear();
	objects.append(object1->getEnglishName());
	step0 = findInitialStep(startJD, stopJD, objects);
	step = step0;
	double jd = startJD - step;
	prevDistance = findRightAscension(jd, object1);
	jd += step;
	double stopJDfx = stopJD + step;
	while (jd <= stopJDfx)
	{
		distance = findHeliocentricDistance(jd, object1);
		double factor = qAbs((distance - prevDistance) / distance);
		if (factor > 10.)
			step = step0 * factor / 10.;
		else
			step = step0;

		if (distance>prevDistance)
		{
			if (step > step0)
			{
				jd -= step;
				step = step0;
				while (jd <= stopJDfx)
				{
					distance = findHeliocentricDistance(jd, object1);
					if (distance<prevDistance)
						break;

					prevDistance = distance;
					jd += step;
				}
			}

			if (findPreciseOrbitalPoint(&extremum, object1, jd, stopJDfx, step, false))
			{
				if (extremum.first>startJD && extremum.first<stopJD)
					separations.insert(extremum.first, extremum.second);
			}
		}

		prevDistance = distance;
		jd += step;
	}

	step0 = findInitialStep(startJD, stopJD, objects);
	step = step0;
	jd = startJD - step;
	prevDistance = findRightAscension(jd, object1);
	jd += step;
	while (jd <= stopJDfx)
	{
		distance = findHeliocentricDistance(jd, object1);
		double factor = qAbs((distance - prevDistance) / distance);
		if (factor > 10.)
			step = step0 * factor / 10.;
		else
			step = step0;

		if (distance<prevDistance)
		{
			if (step > step0)
			{
				jd -= step;
				step = step0;
				while (jd <= stopJDfx)
				{
					distance = findHeliocentricDistance(jd, object1);
					if (distance>prevDistance)
						break;

					prevDistance = distance;
					jd += step;
				}
			}

			if (findPreciseOrbitalPoint(&extremum, object1, jd, stopJDfx, step, true))
			{
				if (extremum.first>startJD && extremum.first<stopJD)
					separations.insert(extremum.first, extremum.second);
			}
		}

		prevDistance = distance;
		jd += step;
	}

	return separations;
}

bool AstroCalcDialog::findPreciseOrbitalPoint(QPair<double, double>* out, PlanetP object1, double JD, double stopJD, double step, bool minimal)
{
	double dist, prevDist;

	if (out == Q_NULLPTR)
		return false;

	prevDist = findHeliocentricDistance(JD, object1);
	step = -step / 2.;

	while (true)
	{
		JD += step;
		dist = findHeliocentricDistance(JD, object1);

		if (qAbs(step) < 1. / 1440.)
		{
			out->first = JD - step / 2.0;
			out->second = findHeliocentricDistance(JD - step / 2.0, object1);
			if (minimal)
			{
				if (out->second > findHeliocentricDistance(JD - step / 5.0, object1))
				{
					out->second *= -1;
					return true;
				}
				else
					return false;
			}
			else
			{
				if (out->second < findHeliocentricDistance(JD - step / 5.0, object1))
					return true;
				else
					return false;
			}
		}
		if (minimal)
		{
			if (dist>prevDist)
				step = -step / 2.0;
		}
		else
		{
			if (dist<prevDist)
				step = -step / 2.0;
		}
		prevDist = dist;

		if (JD > stopJD)
			return false;
	}
}

double AstroCalcDialog::findHeliocentricDistance(double JD, PlanetP object1)
{
	core->setJD(JD);
	core->update(0);
	return object1->getHeliocentricEclipticPos().length();
}

void AstroCalcDialog::changePage(QListWidgetItem* current, QListWidgetItem* previous)
{
	if (!current)
		current = previous;

	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));

	// special case
	if (ui->stackListWidget->row(current) == 0)
		currentCelestialPositions();

	// special case - ephemeris
	if (ui->stackListWidget->row(current) == 1)
	{
		double JD = core->getJD() + core->getUTCOffset(core->getJD()) / 24;
		QDateTime currentDT = StelUtils::jdToQDateTime(JD);
		ui->dateFromDateTimeEdit->setDateTime(currentDT);
		ui->dateToDateTimeEdit->setDateTime(currentDT.addMonths(1));
	}

	// special case - transits
	if (ui->stackListWidget->row(current) == 2)
		setTransitCelestialBodyName();

	// special case - graphs
	if (ui->stackListWidget->row(current) == 4)
	{
		int idx = ui->tabWidgetGraphs->currentIndex();
		if (idx==0) // 'Alt. vs Time' is visible
		{
			plotAltVsTime = true;
			drawAltVsTimeDiagram(); // Is object already selected?
		}
		else
			plotAltVsTime = false;

		if (idx==1) //  'Azi. vs Time' is visible
		{
			plotAziVsTime = true;
			drawAziVsTimeDiagram(); // Is object already selected?
		}
		else
			plotAziVsTime = false;

		if (idx==2) // 'Monthly Elevation' is visible
		{
			plotMonthlyElevation = true;
			drawMonthlyElevationGraph(); // Is object already selected?
		}
		else
			plotMonthlyElevation = false;

		if(idx==4) // 'Angular distance' is visible
		{
			plotAngularDistanceGraph = true;
			drawAngularDistanceGraph();
		}
		else
			plotAngularDistanceGraph = false;
	}

	// special case (PCalc)
	if (ui->stackListWidget->row(current) == 6)
	{
		int index = ui->tabWidgetPC->currentIndex();
		if (index==0) // First tab: Data
		{
			plotDistanceGraph = false;
			computePlanetaryData();
		}
		if (index==1) // Second tab: Graphs
		{
			plotDistanceGraph = true;
			drawDistanceGraph();
		}
	}
}

void AstroCalcDialog::changePCTab(int index)
{
	if (index==0) // First tab: Data
	{
		plotDistanceGraph = false;
		computePlanetaryData();
	}
	if (index==1) // Second tab: Graphs
	{
		plotDistanceGraph = true;
		drawDistanceGraph();
	}
}

void AstroCalcDialog::changeGraphsTab(int index)
{
	// reset all flags to make sure only one is set
	plotAltVsTime = false;
	plotAziVsTime = false;
	plotMonthlyElevation = false;
	plotAngularDistanceGraph = false;

	if (index==0) // Altitude vs. Time
	{
		plotAltVsTime = true;
		drawAltVsTimeDiagram(); // Is object already selected?
	}
	if (index==1) // Azimuth vs. Time
	{
		plotAziVsTime = true;
		drawAziVsTimeDiagram(); // Is object already selected?
	}
	if (index==2) // Monthly Elevation
	{
		plotMonthlyElevation = true;
		drawMonthlyElevationGraph(); // Is object already selected?
	}
	if (index==4) // Angular Distance
	{
		plotAngularDistanceGraph = true;
		drawAngularDistanceGraph(); // Is object already selected?
	}
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
		width += 24; // margin - 12px left and 12px right
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
	StelModuleMgr& moduleMgr = StelApp::getInstance().getModuleMgr();

	QListWidget* category = ui->wutCategoryListWidget;
	category->blockSignals(true);

	wutCategories.clear();
	wutCategories.insert(q_("Planets"), 0);
	wutCategories.insert(q_("Bright stars"), 1);
	wutCategories.insert(q_("Bright nebulae"), 2);
	wutCategories.insert(q_("Dark nebulae"), 3);
	wutCategories.insert(q_("Galaxies"), 4);
	wutCategories.insert(q_("Open star clusters"), 5);
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
	wutCategories.insert(q_("Interstellar objects"), 24);
	wutCategories.insert(q_("Globular star clusters"), 25);
	wutCategories.insert(q_("Regions of the sky"), 26);
	wutCategories.insert(q_("Active galaxies"), 27);
	if (moduleMgr.isPluginLoaded("Pulsars"))
	{
		// Add the category when pulsars is visible
		if (propMgr->getProperty("Pulsars.pulsarsVisible")->getValue().toBool())
			wutCategories.insert(q_("Pulsars"), 28);
	}
	if (moduleMgr.isPluginLoaded("Exoplanets"))
	{
		// Add the category when exoplanets is visible
		if (propMgr->getProperty("Exoplanets.showExoplanets")->getValue().toBool())
			wutCategories.insert(q_("Exoplanetary systems"), 29);
	}
	if (moduleMgr.isPluginLoaded("Novae"))
		wutCategories.insert(q_("Bright nova stars"), 30);
	if (moduleMgr.isPluginLoaded("Supernovae"))
		wutCategories.insert(q_("Bright supernova stars"), 31);	
	wutCategories.insert(q_("Interacting galaxies"), 32);

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

void AstroCalcDialog::saveWutMinAngularSizeLimit()
{
	// Convert to angular minutes
	conf->setValue("astrocalc/wut_angular_limit_min", QString::number(ui->wutAngularSizeLimitMinSpinBox->valueDegrees()*60.0, 'f', 2));
	calculateWutObjects();
}

void AstroCalcDialog::saveWutMaxAngularSizeLimit()
{
	// Convert to angular minutes
	conf->setValue("astrocalc/wut_angular_limit_max", QString::number(ui->wutAngularSizeLimitMaxSpinBox->valueDegrees()*60.0, 'f', 2));
	calculateWutObjects();
}

void AstroCalcDialog::saveWutAngularSizeFlag(bool state)
{
	conf->setValue("astrocalc/wut_angular_limit_flag", state);
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

void AstroCalcDialog::setWUTHeaderNames(const bool magnitude, const bool separation)
{
	wutHeader.clear();
	wutHeader << q_("Name");
	if (magnitude)
	{
		// TRANSLATORS: magnitude
		wutHeader << q_("Mag.");
	}
	else
	{
		// TRANSLATORS: opacity
		wutHeader << q_("Opac.");
	}
	wutHeader << qc_("Rise", "celestial event");
	wutHeader << qc_("Transit", "celestial event; passage across a meridian");
	// TRANSLATORS: elevation
	wutHeader << q_("Elev.");
	wutHeader << qc_("Set", "celestial event");
	if (separation)
	{
		// TRANSLATORS: separation
		wutHeader << q_("Sep.");
	}
	else
	{
		// TRANSLATORS: angular size
		wutHeader << q_("Ang. Size");
	}
	ui->wutMatchingObjectsTreeWidget->setHeaderLabels(wutHeader);

	adjustWUTColumns();
}

void AstroCalcDialog::adjustWUTColumns()
{
	// adjust the column width
	for (int i = 0; i < WUTCount; ++i)
	{
		ui->wutMatchingObjectsTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListWUT(const bool magnitude, const bool separation)
{
	ui->wutMatchingObjectsTreeWidget->clear();
	ui->wutMatchingObjectsTreeWidget->setColumnCount(WUTCount);
	setWUTHeaderNames(magnitude, separation);
	ui->wutMatchingObjectsTreeWidget->header()->setSectionsMovable(false);
	ui->wutMatchingObjectsTreeWidget->header()->setDefaultAlignment(Qt::AlignHCenter);
}

void AstroCalcDialog::enableVisibilityAngularLimits(bool visible)
{
	ui->wutAngularSizeLimitCheckBox->setVisible(visible);
	ui->wutAngularSizeLimitMinSpinBox->setVisible(visible);
	ui->wutAngularSizeLimitMaxSpinBox->setVisible(visible);
}

void AstroCalcDialog::fillWUTTable(QString objectName, QString designation, float magnitude, Vec3f RTSTime, double maxElevation, double angularSize, bool decimalDegrees)
{
	QString sAngularSize = dash;
	QString sRise = dash;
	QString sTransit = dash;
	QString sSet = dash;
	QString sMaxElevation = dash;

	WUTTreeWidgetItem* treeItem =  new WUTTreeWidgetItem(ui->wutMatchingObjectsTreeWidget);
	treeItem->setData(WUTObjectName, Qt::DisplayRole, objectName);
	treeItem->setData(WUTObjectName, Qt::UserRole, designation);
	if (magnitude > 98.f)
		treeItem->setText(WUTMagnitude, dash);
	else
		treeItem->setText(WUTMagnitude, QString::number(magnitude, 'f', 2));
	treeItem->setTextAlignment(WUTMagnitude, Qt::AlignRight);

	if (RTSTime[0]>-99.f && RTSTime[0]<100.f)
		sRise = StelUtils::hoursToHmsStr(RTSTime[0], true);
	if (RTSTime[1]>=0.f)
		sTransit = StelUtils::hoursToHmsStr(RTSTime[1], true);
	if (RTSTime[2]>-99.f && RTSTime[2]<100.f)
		sSet = StelUtils::hoursToHmsStr(RTSTime[2], true);

	treeItem->setText(WUTRiseTime, sRise);
	treeItem->setTextAlignment(WUTRiseTime, Qt::AlignRight);
	treeItem->setText(WUTTransitTime, sTransit);
	treeItem->setTextAlignment(WUTTransitTime, Qt::AlignRight);

	if (decimalDegrees)
		sMaxElevation = StelUtils::radToDecDegStr(maxElevation, 5, false, true);
	else
		sMaxElevation = StelUtils::radToDmsPStr(maxElevation, 2);
	treeItem->setText(WUTMaxElevation, sMaxElevation);
	treeItem->setTextAlignment(WUTMaxElevation, Qt::AlignRight);

	treeItem->setText(WUTSetTime, sSet);
	treeItem->setTextAlignment(WUTSetTime, Qt::AlignRight);

	double angularSizeRad = angularSize * M_PI / 180.;
	if (angularSize>0.0)
	{
		if (decimalDegrees)
			sAngularSize = StelUtils::radToDecDegStr(angularSizeRad, 5, false, true);
		else
			sAngularSize = StelUtils::radToDmsPStr(angularSizeRad, 2);
	}
	treeItem->setText(WUTAngularSize, sAngularSize);
	treeItem->setTextAlignment(WUTAngularSize, Qt::AlignRight);
}

void AstroCalcDialog::calculateWutObjects()
{
	if (ui->wutCategoryListWidget->currentItem())
	{
		QString categoryName = ui->wutCategoryListWidget->currentItem()->text();
		int categoryId = wutCategories.value(categoryName);

		QList<PlanetP> allObjects = solarSystem->getAllPlanets();
		QVector<NebulaP> allDSO = dsoMgr->getAllDeepSkyObjects();
		QList<StelObjectP> hipStars = starMgr->getHipparcosStars();
		QList<StelACStarData> dblHipStars = starMgr->getHipparcosDoubleStars();
		QList<StelACStarData> varHipStars = starMgr->getHipparcosVariableStars();
		QList<StelACStarData> hpmHipStars = starMgr->getHipparcosHighPMStars();

		const Nebula::TypeGroup& tflags = dsoMgr->getTypeFilters();
		bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
		bool angularSizeLimit = ui->wutAngularSizeLimitCheckBox->isChecked();
		bool passByType, visible;
		double angularSizeLimitMin = ui->wutAngularSizeLimitMinSpinBox->valueDegrees();
		double angularSizeLimitMax = ui->wutAngularSizeLimitMaxSpinBox->valueDegrees();
		float magLimit = static_cast<float>(ui->wutMagnitudeDoubleSpinBox->value());
		double JD = core->getJD();
		double UTCOffset = core->getUTCOffset(JD) / 24.;
		double wutJD, az, alt;
		float mag;
		QSet<QString> objectsList;
		QString designation, starName;
		Vec3f rts;

		ui->wutAngularSizeLimitCheckBox->setText(q_("Limit angular size:"));
		ui->wutAngularSizeLimitCheckBox->setToolTip(q_("Set limits for angular size for visible celestial objects"));
		ui->wutAngularSizeLimitMinSpinBox->setToolTip(q_("Minimal angular size for visible celestial objects"));
		ui->wutAngularSizeLimitMaxSpinBox->setToolTip(q_("Maximum angular size for visible celestial objects"));

		enableVisibilityAngularLimits(true);

		// Direct calculate sunrise/sunset
		PlanetP sun = GETSTELMODULE(SolarSystem)->getSun();
		double sunset = -1, sunrise = -1, midnight = -1, lc = 100.0;
		bool flag = false;
		for (int i = 0; i < 288; i++) // Check position every 5 minutes...
		{
			wutJD = static_cast<int>(JD) - UTCOffset + i * 0.0034722;
			core->setJD(wutJD);
			core->update(0);
			StelUtils::rectToSphe(&az, &alt, sun->getAltAzPosAuto(core));
			alt = std::fmod(alt, 2.0 * M_PI) * 180. / M_PI;
			if (alt >= -7. && alt <= -5.)
			{
				if (!flag)
				{
					sunset = wutJD;
					flag = true;
				}
				else
					sunrise = wutJD;
			}

			if (alt < lc)
			{
				midnight = wutJD;
				lc = alt;
			}
		}
		core->setJD(JD);

		if (sunset<0.)
			sunset = midnight - 0.25;
		if (sunrise<0.)
			sunrise = midnight + 0.25;

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

		initListWUT();
		ui->wutMatchingObjectsTreeWidget->showColumn(WUTMagnitude);
		ui->wutMatchingObjectsTreeWidget->showColumn(WUTAngularSize);		
		objectsList.clear();
		for (int i = 0; i < wutJDList.count(); i++)
		{
			core->setJD(wutJDList.at(i));
			core->update(0);

			switch (categoryId)
			{
				case 1: // Bright stars
					enableVisibilityAngularLimits(false);
					for (const auto& object : hipStars)
					{
						// Filter for angular size is not applicable
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isAboveRealHorizon(core))
						{
							designation = object->getEnglishName();
							if (designation.isEmpty())
								designation = object->getID();

							if (!objectsList.contains(designation))
							{
								starName = object->getNameI18n();
								if (starName.isEmpty())
									starName = designation;

								rts = object->getRTSTime(core);
								alt = computeMaxElevation(object);

								fillWUTTable(starName, designation, mag, rts, alt, 0.0, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}
					ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!
					break;
				case 2: // DSO
				case 3:
				case 4:
				case 5:
				case 14:
				case 18:
				case 19:
				case 20:
				case 21:
				case 22:
				case 23:
				case 25:
				case 26:
				case 32:				
				{
					if (categoryId==3)
						initListWUT(false, false); // special case!
					if (categoryId==18 || categoryId==19 || categoryId==20)
						enableVisibilityAngularLimits(false);

					for (const auto& object : allDSO)
					{
						passByType = false;
						mag = object->getVMagnitude(core);
						Nebula::NebulaType ntype = object->getDSOType();						
						switch (categoryId)
						{
							case 2: // Bright nebulae
								if (static_cast<bool>(tflags & Nebula::TypeBrightNebulae) && (ntype == Nebula::NebN || ntype == Nebula::NebBn || ntype == Nebula::NebEn || ntype == Nebula::NebRn || ntype == Nebula::NebHII || ntype == Nebula::NebISM || ntype == Nebula::NebCn || ntype == Nebula::NebSNR) && mag <= magLimit)
									passByType = true;
								break;
							case 3: // Dark nebulae
								if (static_cast<bool>(tflags & Nebula::TypeDarkNebulae) && (ntype == Nebula::NebDn || ntype == Nebula::NebMolCld	 || ntype == Nebula::NebYSO))
									passByType = true;
								break;
							case 4: // Galaxies
								if (static_cast<bool>(tflags & Nebula::TypeGalaxies) && (ntype == Nebula::NebGx) && mag <= magLimit)
									passByType = true;
								break;
							case 5: // Open Star clusters
								if (static_cast<bool>(tflags & Nebula::TypeOpenStarClusters) && (ntype == Nebula::NebCl || ntype == Nebula::NebOc || ntype == Nebula::NebSA || ntype == Nebula::NebSC || ntype == Nebula::NebCn) && mag <= magLimit)
									passByType = true;
								break;
							case 14: // Planetary nebulae
								if (static_cast<bool>(tflags & Nebula::TypePlanetaryNebulae) && (ntype == Nebula::NebPn || ntype == Nebula::NebPossPN || ntype == Nebula::NebPPN) && mag <= magLimit)
									passByType = true;
								break;
							case 18: // Symbiotic stars
								if (static_cast<bool>(tflags & Nebula::TypeOther) && (ntype == Nebula::NebSymbioticStar) && mag <= magLimit)
									passByType = true;
								break;
							case 19: // Emission-line stars
								if (static_cast<bool>(tflags & Nebula::TypeOther) && (ntype == Nebula::NebEmissionLineStar) && mag <= magLimit)
									passByType = true;
								break;
							case 20: // Supernova candidates
							{
								visible = ((mag <= magLimit) || (mag > 90.f && magLimit >= 19.f));
								if (static_cast<bool>(tflags & Nebula::TypeSupernovaRemnants) && (ntype == Nebula::NebSNC) && visible)
									passByType = true;
								break;
							}
							case 21: // Supernova remnant candidates
							{
								visible = ((mag <= magLimit) || (mag > 90.f && magLimit >= 19.f));
								if (static_cast<bool>(tflags & Nebula::TypeSupernovaRemnants) && (ntype == Nebula::NebSNRC) && visible)
									passByType = true;
								break;
							}
							case 22: // Supernova remnants
							{
								visible = ((mag <= magLimit) || (mag > 90.f && magLimit >= 19.f));
								if (static_cast<bool>(tflags & Nebula::TypeSupernovaRemnants) && (ntype == Nebula::NebSNR) && visible)
									passByType = true;
								break;
							}
							case 23: // Clusters of galaxies
								if (static_cast<bool>(tflags & Nebula::TypeGalaxyClusters) && (ntype == Nebula::NebGxCl) && mag <= magLimit)
									passByType = true;
								break;
							case 25: // Globular Star clusters
								if ((static_cast<bool>(tflags & Nebula::TypeGlobularStarClusters) && ntype == Nebula::NebGc) && mag <= magLimit)
									passByType = true;
								break;
							case 26: // Regions
								if (static_cast<bool>(tflags & Nebula::TypeOther) && ntype == Nebula::NebRegion)
									passByType = true;
								break;
							case 32: // Interacting Galaxies
								if (static_cast<bool>(tflags & Nebula::TypeInteractingGalaxies) && (ntype == Nebula::NebIGx) && mag <= magLimit)
									passByType = true;
								break;
						}

						if (passByType && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if ((angularSizeLimit) && (!StelUtils::isWithin(object->getAngularSize(core), angularSizeLimitMin, angularSizeLimitMax)))
								continue;

							if (d.isEmpty() && n.isEmpty())
								continue;

							designation = QString("%1:%2").arg(d, n);
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));

								if (d.isEmpty())
									fillWUTTable(n, n, mag, rts, alt, object->getAngularSize(core), withDecimalDegree);
								else if (n.isEmpty())
									fillWUTTable(d, d, mag, rts, alt, object->getAngularSize(core), withDecimalDegree);
								else
									fillWUTTable(QString("%1 (%2)").arg(d, n), d, mag, rts, alt, object->getAngularSize(core), withDecimalDegree);

								objectsList.insert(designation);
							}
						}
					}

					if (categoryId==18 || categoryId==19 || categoryId==23)
						ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!

					if (categoryId==26) // special case!
					{
						ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize);
						ui->wutMatchingObjectsTreeWidget->hideColumn(WUTMagnitude);
					}

					break;
				}
				case 0:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
				case 12:
				case 13:
				case 24:
				{
					static const QMap<int, Planet::PlanetType>map = {
						{0, Planet::isPlanet},
						{6, Planet::isAsteroid},
						{7, Planet::isComet},
						{8, Planet::isPlutino},
						{9, Planet::isDwarfPlanet},
						{10, Planet::isCubewano},
						{11, Planet::isSDO},
						{12, Planet::isOCO},
						{13, Planet::isSednoid},
						{24, Planet::isInterstellar}};
					const Planet::PlanetType pType = map.value(categoryId, Planet::isInterstellar);

					for (const auto& object : allObjects)
					{
						mag = object->getVMagnitude(core);
						if (object->getPlanetType() == pType && mag <= magLimit && object->isAboveRealHorizon(core))
						{
							if ((angularSizeLimit) && (!StelUtils::isWithin(object->getAngularSize(core), angularSizeLimitMin, angularSizeLimitMax)))
								continue;

							designation = object->getEnglishName();
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));

								fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 2.0*object->getAngularSize(core), withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}

					if (pType==Planet::isComet)
						ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!

					break;
				}
				case 15: // Bright double stars
					// Special case for double stars
					ui->wutAngularSizeLimitCheckBox->setText(q_("Limit angular separation:"));
					ui->wutAngularSizeLimitCheckBox->setToolTip(q_("Set limits for angular separation for visible double stars"));
					ui->wutAngularSizeLimitMinSpinBox->setToolTip(q_("Minimal angular separation for visible double stars"));
					ui->wutAngularSizeLimitMaxSpinBox->setToolTip(q_("Maximum angular separation for visible double stars"));
					if (i==0)
						initListWUT(true, true); // special case!

					for (const auto& dblStar : dblHipStars)
					{
						StelObjectP object = dblStar.firstKey();
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isAboveRealHorizon(core))
						{
							// convert from arcseconds to degrees
							if ((angularSizeLimit) && (!StelUtils::isWithin(static_cast<double>(dblStar.value(object))/3600.0, angularSizeLimitMin, angularSizeLimitMax)))
								continue;

							designation = object->getEnglishName();
							if (designation.isEmpty())
								designation = object->getID();

							if (!objectsList.contains(designation))
							{
								starName = object->getNameI18n();
								if (starName.isEmpty())
									starName = designation;

								rts = object->getRTSTime(core);
								alt = computeMaxElevation(object);

								fillWUTTable(starName, designation, mag, rts, alt, dblStar.value(object)/3600.0, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}
					break;
				case 16: // Bright variable stars
					enableVisibilityAngularLimits(false);
					for (const auto& varStar : varHipStars)
					{
						StelObjectP object = varStar.firstKey();
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isAboveRealHorizon(core))
						{
							designation = object->getEnglishName();
							if (designation.isEmpty())
								designation = object->getID();

							if (!objectsList.contains(designation))
							{
								starName = object->getNameI18n();
								if (starName.isEmpty())
									starName = designation;

								rts = object->getRTSTime(core);
								alt = computeMaxElevation(object);

								fillWUTTable(starName, designation, mag, rts, alt, 0.0, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}
					ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!
					break;
				case 17: // Bright stars with high proper motion
					enableVisibilityAngularLimits(false);
					for (const auto& hpmStar : hpmHipStars)
					{
						StelObjectP object = hpmStar.firstKey();
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isAboveRealHorizon(core))
						{
							designation = object->getEnglishName();
							if (designation.isEmpty())
								designation = object->getID();

							if (!objectsList.contains(designation))
							{
								starName = object->getNameI18n();
								if (starName.isEmpty())
									starName = designation;

								rts = object->getRTSTime(core);
								alt = computeMaxElevation(object);

								fillWUTTable(starName, designation, mag, rts, alt, 0.0, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}
					ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!
					break;
				case 27: // Active galaxies
					enableVisibilityAngularLimits(false);
					for (const auto& object : allDSO)
					{
						passByType = false;
						mag = object->getVMagnitude(core);
						Nebula::NebulaType ntype = object->getDSOType();
						if (static_cast<bool>(tflags & Nebula::TypeActiveGalaxies) && (ntype == Nebula::NebQSO || ntype == Nebula::NebPossQSO || ntype == Nebula::NebAGx || ntype == Nebula::NebRGx || ntype == Nebula::NebBLA || ntype == Nebula::NebBLL) && mag <= magLimit && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							QString n = object->getNameI18n();

							if ((angularSizeLimit) && (!StelUtils::isWithin(object->getAngularSize(core), angularSizeLimitMin, angularSizeLimitMax)))
								continue;

							if (d.isEmpty() && n.isEmpty())
								continue;

							designation = QString("%1:%2").arg(d, n);
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));

								if (d.isEmpty())
									fillWUTTable(n, n, mag, rts, alt, object->getAngularSize(core), withDecimalDegree);
								else if (n.isEmpty())
									fillWUTTable(d, d, mag, rts, alt, object->getAngularSize(core), withDecimalDegree);
								else
									fillWUTTable(QString("%1 (%2)").arg(d, n), d, mag, rts, alt, object->getAngularSize(core), withDecimalDegree);

								objectsList.insert(designation);
							}
						}
					}

					#ifdef USE_STATIC_PLUGIN_QUASARS
					if (StelApp::getInstance().getModuleMgr().isPluginLoaded("Quasars"))
					{
						if (propMgr->getProperty("Quasars.quasarsVisible")->getValue().toBool())
						{
							for (const auto& object : GETSTELMODULE(Quasars)->getAllQuasars())
							{
								mag = object->getVMagnitude(core);
								if (mag <= magLimit && object->isAboveRealHorizon(core))
								{
									designation = object->getEnglishName();
									if (!objectsList.contains(designation) && !designation.isEmpty())
									{
										rts = object->getRTSTime(core);
										alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
										fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 0.0, withDecimalDegree);
										objectsList.insert(designation);
									}
								}
							}
						}
					}
					#endif
					ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!
					break;
				case 28: // Pulsars
					enableVisibilityAngularLimits(false);
					#ifdef USE_STATIC_PLUGIN_PULSARS					
					for (const auto& object : GETSTELMODULE(Pulsars)->getAllPulsars())
					{
						if (object->isAboveRealHorizon(core))
						{
							designation = object->getEnglishName();
							if (designation.isEmpty())
								designation = object->getDesignation();

							if (!objectsList.contains(designation) && !designation.isEmpty())
							{
								starName = object->getNameI18n(); // Just re-use variable
								if (starName.isEmpty())
									starName = designation;

								rts = object->getRTSTime(core);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								fillWUTTable(starName, designation, 99.f, rts, alt, 0.0, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}
					ui->wutMatchingObjectsTreeWidget->hideColumn(WUTMagnitude); // special case!
					ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!
					#endif
					break;
				case 29: // Exoplanetary systems
					enableVisibilityAngularLimits(false);
					#ifdef USE_STATIC_PLUGIN_EXOPLANETS					
					for (const auto& object : GETSTELMODULE(Exoplanets)->getAllExoplanetarySystems())
					{
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isVMagnitudeDefined() && object->isAboveRealHorizon(core))
						{
							designation = object->getEnglishName();
							if (!objectsList.contains(designation) && !designation.isEmpty())
							{
								rts = object->getRTSTime(core);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								fillWUTTable(object->getNameI18n().trimmed(), designation, mag, rts, alt, 0.0, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}
					ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!
					#endif
					break;
				case 30: // Bright novae
					enableVisibilityAngularLimits(false);
					#ifdef USE_STATIC_PLUGIN_NOVAE					
					for (const auto& object : GETSTELMODULE(Novae)->getAllBrightNovae())
					{
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isAboveRealHorizon(core))
						{
							designation = object->getEnglishName();
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 0.0, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}
					ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!
					#endif
					break;
				case 31: // Bright supernovae
					enableVisibilityAngularLimits(false);
					#ifdef USE_STATIC_PLUGIN_SUPERNOVAE					
					for (const auto& object : GETSTELMODULE(Supernovae)->getAllBrightSupernovae())
					{
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isAboveRealHorizon(core))
						{
							designation = object->getEnglishName();
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 0.0, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}
					ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!
					#endif
					break;
			}
		}

		core->setJD(JD);
		adjustWUTColumns();
		objectsList.clear();		
	}
}

void AstroCalcDialog::selectWutObject(const QModelIndex &index)
{
	if (index.isValid())
	{
		// Find the object
		QString wutObjectEnglisName = index.sibling(index.row(),WUTObjectName).data(Qt::UserRole).toString();
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
					GETSTELMODULE(StelObjectMgr)->unSelect();
			}
		}
	}
}

void AstroCalcDialog::saveWutObjects()
{
	QString filter = q_("Microsoft Excel Open XML Spreadsheet");
	filter.append(" (*.xlsx);;");
	filter.append(q_("CSV (Comma delimited)"));
	filter.append(" (*.csv)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save list of objects as..."),
							QDir::homePath() + "/wut-objects.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->wutMatchingObjectsTreeWidget, wutHeader);
	else
	{
		int count = ui->wutMatchingObjectsTreeWidget->topLevelItemCount();
		int columns = wutHeader.size();

		int *width;
		width = new int[columns];
		QString sData;
		int w;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("What's Up Tonight"));
		xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
		if (ui->wutCategoryListWidget->currentRow()>0) // Fixed crash when category of objects is not selected
			xlsx.addSheet(ui->wutCategoryListWidget->currentItem()->text(), AbstractSheet::ST_WorkSheet);

		QXlsx::Format header;
		header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		header.setPatternBackgroundColor(Qt::yellow);
		header.setBorderStyle(QXlsx::Format::BorderThin);
		header.setBorderColor(Qt::black);
		header.setFontBold(true);
		for (int i = 0; i < columns; i++)
		{
			// Row 1: Names of columns
			sData = wutHeader.at(i).trimmed();
			xlsx.write(1, i + 1, sData, header);
			width[i] = sData.size();
		}

		QXlsx::Format data;
		data.setHorizontalAlignment(QXlsx::Format::AlignRight);
		QXlsx::Format left;
		left.setHorizontalAlignment(QXlsx::Format::AlignLeft);
		for (int i = 0; i < count; i++)
		{
			for (int j = 0; j < columns; j++)
			{
				// Row 2 and next: the data
				sData = ui->wutMatchingObjectsTreeWidget->topLevelItem(i)->text(j).trimmed();
				if (j==0)
					xlsx.write(i + 2, j + 1, sData, left);
				else
					xlsx.write(i + 2, j + 1, sData, data);
				w = sData.size();
				if (w > width[j])
				{
					width[j] = w;
				}
			}
		}

		for (int i = 0; i < columns; i++)
		{
			xlsx.setColumnWidth(i+1, width[i]+2);
		}

		delete[] width;
		xlsx.saveAs(filePath);
	}
}

void AstroCalcDialog::saveFirstCelestialBody(int index)
{
	Q_ASSERT(ui->firstCelestialBodyComboBox);
	QComboBox* celestialBody = ui->firstCelestialBodyComboBox;
	conf->setValue("astrocalc/first_celestial_body", celestialBody->itemData(index).toString());

	computePlanetaryData();
	drawDistanceGraph();
}

void AstroCalcDialog::saveSecondCelestialBody(int index)
{
	Q_ASSERT(ui->secondCelestialBodyComboBox);
	QComboBox* celestialBody = ui->secondCelestialBodyComboBox;
	conf->setValue("astrocalc/second_celestial_body", celestialBody->itemData(index).toString());

	computePlanetaryData();
	drawDistanceGraph();
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

	const double distanceAu = (posFCB - posSCB).length();
	const double distanceKm = AU * distanceAu;
	// TRANSLATORS: Unit of measure for distance - kilometers
	QString km = qc_("km", "distance");
	// TRANSLATORS: Unit of measure for distance - milliones kilometers
	QString Mkm = qc_("M km", "distance");
	QString distAU, distKM;
	const bool useKM = (distanceAu < 0.1);
	if (useKM)
	{
		distAU = QString::number(distanceAu, 'f', 5);
		distKM = QString::number(distanceKm, 'f', 3);
	}
	else
	{
		distAU = QString::number(distanceAu, 'f', 5);
		distKM = QString::number(distanceKm / 1.0e6, 'f', 3);
	}

	double r = std::acos(sin(posFCB.latitude()) * sin(posSCB.latitude()) + cos(posFCB.latitude()) * cos(posSCB.latitude()) * cos(posFCB.longitude() - posSCB.longitude()));

	unsigned int d, m;
	double s, dd;
	bool sign;

	StelUtils::radToDms(r, sign, d, m, s);
	StelUtils::radToDecDeg(r, sign, dd);

	double spcb1 = firstCBId->getSiderealPeriod();
	double spcb2 = secondCBId->getSiderealPeriod();
	int cb1 = qRound(spcb1);
	int cb2 = qRound(spcb2);
	QString orbitalResonance = dash;
	QString orbitalPeriodsRatio = dash;
	bool spin = false;
	QString parentFCBName = "";
	if (firstCelestialBody != "Sun")
		parentFCBName = firstCBId->getParent()->getEnglishName();
	QString parentSCBName = "";
	if (secondCelestialBody != "Sun")
		parentSCBName = secondCBId->getParent()->getEnglishName();

	if (firstCelestialBody == parentSCBName)
	{
		cb1 = qRound(secondCBId->getSiderealPeriod());
		cb2 = qRound(secondCBId->getSiderealDay());
		spin = true;
	}
	else if (secondCelestialBody == parentFCBName)
	{
		cb1 = qRound(firstCBId->getSiderealPeriod());
		cb2 = qRound(firstCBId->getSiderealDay());
		spin = true;
	}
	int gcd = StelUtils::gcd(cb1, cb2);

	QString distanceUM = qc_("AU", "distance, astronomical unit");
	ui->labelLinearDistanceValue->setText(QString("%1 %2 (%3 %4)").arg(distAU).arg(distanceUM).arg(distKM).arg(useKM ? km : Mkm));

	QString angularDistance = dash;
	if (firstCelestialBody != currentPlanet && secondCelestialBody != currentPlanet)
		angularDistance = QString("%1%2 %3' %4\" (%5%2)").arg(d).arg(QChar(0x00B0)).arg(m).arg(s, 0, 'f', 2).arg(dd, 0, 'f', 5);
	ui->labelAngularDistanceValue->setText(angularDistance);

	if (cb1 > 0 && cb2 > 0)
	{
		double r1 = qAbs(cb1 / gcd);
		double r2 = qAbs(cb2 / gcd);
		orbitalResonance = QString("%1:%2").arg(qRound(r1)).arg(qRound(r2));
		if (spin)
			orbitalResonance.append(QString(" (%1)").arg(q_("spin-orbit resonance")));
	}
	ui->labelOrbitalResonanceValue->setText(orbitalResonance);

	if (spcb1 > 0. && spcb2 > 0.)
	{
		double minp = spcb2;
		if (qAbs(spcb1)<=qAbs(spcb2)) { minp = spcb1; }
		orbitalPeriodsRatio = QString("%1:%2").arg(QString::number(qAbs(spcb1/minp), 'f', 2)).arg(QString::number(qAbs(spcb2/minp), 'f', 2));
	}
	ui->labelOrbitalPeriodsRatioValue->setText(orbitalPeriodsRatio);

	// TRANSLATORS: Unit of measure for speed - kilometers per second
	QString kms = qc_("km/s", "speed");

	double orbVelFCB = firstCBId->getEclipticVelocity().length();
	QString orbitalVelocityFCB = dash;
	if (orbVelFCB > 0.)
		orbitalVelocityFCB = QString("%1 %2").arg(QString::number(orbVelFCB * AU/86400., 'f', 3)).arg(kms);

	ui->labelOrbitalVelocityFCBValue->setText(orbitalVelocityFCB);

	double orbVelSCB = secondCBId->getEclipticVelocity().length();
	QString orbitalVelocitySCB = dash;
	if (orbVelSCB>0.)
		orbitalVelocitySCB = QString("%1 %2").arg(QString::number(orbVelSCB * AU/86400., 'f', 3)).arg(kms);

	ui->labelOrbitalVelocitySCBValue->setText(orbitalVelocitySCB);

	// TRANSLATORS: Unit of measure for period - days
	QString days = qc_("days", "duration");
	QString synodicPeriod = dash;
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

	double fcbs = 2.0 * AU * firstCBId->getEquatorialRadius();
	double scbs = 2.0 * AU * secondCBId->getEquatorialRadius();
	double sratio = fcbs/scbs;

	int ss = 2;
	if (sratio < 1.0)
		ss = 6;

	QString sizeRatio = QString("%1 (%2 %4 / %3 %4)").arg(QString::number(sratio, 'f', ss), QString::number(fcbs, 'f', 1), QString::number(scbs, 'f', 1) , km);
	ui->labelEquatorialRadiiRatioValue->setText(sizeRatio);
}

void AstroCalcDialog::prepareDistanceAxesAndGraph()
{
	QString xAxisStr = q_("Days from today");
	QString yAxisLegend1 = QString("%1, %2").arg(q_("Linear distance"), qc_("AU", "distance, astronomical unit"));
	QString yAxisLegend2 = QString("%1, %2").arg(q_("Angular distance"), QChar(0x00B0)); // decimal degrees

	QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);
	QColor axisColorL(Qt::green);
	QPen axisPenL(axisColorL, 1);
	QColor axisColorR(Qt::yellow);
	QPen axisPenR(axisColorR, 1);

	ui->pcDistanceGraphPlot->xAxis->setLabel(xAxisStr);
	ui->pcDistanceGraphPlot->yAxis->setLabel(yAxisLegend1);
	ui->pcDistanceGraphPlot->yAxis2->setLabel(yAxisLegend2);

	ui->pcDistanceGraphPlot->xAxis->setRange(-300, 300);
	ui->pcDistanceGraphPlot->xAxis->setScaleType(QCPAxis::stLinear);
	ui->pcDistanceGraphPlot->xAxis->setLabelColor(axisColor);
	ui->pcDistanceGraphPlot->xAxis->setTickLabelColor(axisColor);
	ui->pcDistanceGraphPlot->xAxis->setBasePen(axisPen);
	ui->pcDistanceGraphPlot->xAxis->setTickPen(axisPen);
	ui->pcDistanceGraphPlot->xAxis->setSubTickPen(axisPen);
	ui->pcDistanceGraphPlot->xAxis->setAutoTicks(true);
	ui->pcDistanceGraphPlot->xAxis->setAutoTickCount(15);

	ui->pcDistanceGraphPlot->yAxis->setRange(minYld, maxYld);
	ui->pcDistanceGraphPlot->yAxis->setScaleType(QCPAxis::stLinear);
	ui->pcDistanceGraphPlot->yAxis->setLabelColor(axisColorL);
	ui->pcDistanceGraphPlot->yAxis->setTickLabelColor(axisColorL);
	ui->pcDistanceGraphPlot->yAxis->setBasePen(axisPenL);
	ui->pcDistanceGraphPlot->yAxis->setTickPen(axisPenL);
	ui->pcDistanceGraphPlot->yAxis->setSubTickPen(axisPenL);

	ui->pcDistanceGraphPlot->yAxis2->setRange(minYad, maxYad);
	ui->pcDistanceGraphPlot->yAxis2->setScaleType(QCPAxis::stLinear);
	ui->pcDistanceGraphPlot->yAxis2->setLabelColor(axisColorR);
	ui->pcDistanceGraphPlot->yAxis2->setTickLabelColor(axisColorR);
	ui->pcDistanceGraphPlot->yAxis2->setBasePen(axisPenR);
	ui->pcDistanceGraphPlot->yAxis2->setTickPen(axisPenR);
	ui->pcDistanceGraphPlot->yAxis2->setSubTickPen(axisPenR);
	ui->pcDistanceGraphPlot->yAxis2->setVisible(true);

	ui->pcDistanceGraphPlot->clearGraphs();
	ui->pcDistanceGraphPlot->addGraph(ui->pcDistanceGraphPlot->xAxis, ui->pcDistanceGraphPlot->yAxis);
	ui->pcDistanceGraphPlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->pcDistanceGraphPlot->graph(0)->setPen(axisPenL);
	ui->pcDistanceGraphPlot->graph(0)->setLineStyle(QCPGraph::lsLine);
	ui->pcDistanceGraphPlot->graph(0)->rescaleAxes(true);

	ui->pcDistanceGraphPlot->addGraph(ui->pcDistanceGraphPlot->xAxis, ui->pcDistanceGraphPlot->yAxis2);
	ui->pcDistanceGraphPlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->pcDistanceGraphPlot->graph(1)->setPen(axisPenR);
	ui->pcDistanceGraphPlot->graph(1)->setLineStyle(QCPGraph::lsLine);
	ui->pcDistanceGraphPlot->graph(1)->rescaleAxes(true);
}

void AstroCalcDialog::drawDistanceGraph()
{
	// special case - plot the graph when tab is visible
	if (!plotDistanceGraph || !dialog->isVisible())
		return;

	Q_ASSERT(ui->firstCelestialBodyComboBox);
	Q_ASSERT(ui->secondCelestialBodyComboBox);

	QComboBox* fbody = ui->firstCelestialBodyComboBox;
	QComboBox* sbody = ui->secondCelestialBodyComboBox;

	QString firstCelestialBody = fbody->currentData(Qt::UserRole).toString();
	QString secondCelestialBody = sbody->currentData(Qt::UserRole).toString();
	QString currentPlanet = core->getCurrentPlanet()->getEnglishName();

	PlanetP firstCBId = solarSystem->searchByEnglishName(firstCelestialBody);
	PlanetP secondCBId = solarSystem->searchByEnglishName(secondCelestialBody);

	if (firstCBId==secondCBId)
	{
		ui->pcDistanceGraphPlot->graph(0)->clearData();
		ui->pcDistanceGraphPlot->graph(1)->clearData();
		ui->pcDistanceGraphPlot->replot();
		return;
	}

	QList<double> aX, aY1, aY2;
	Vec3d posFCB, posSCB;
	double currentJD = core->getJD();
	double JD, distanceAu, r, dd;
	bool sign;
	for (int i = -305; i <= 305; i++)
	{
		JD = currentJD + i;
		core->setJD(JD);
		posFCB = firstCBId->getJ2000EquatorialPos(core);
		posSCB = secondCBId->getJ2000EquatorialPos(core);
		distanceAu = (posFCB - posSCB).length();
		r = std::acos(sin(posFCB.latitude()) * sin(posSCB.latitude()) + cos(posFCB.latitude()) * cos(posSCB.latitude()) * cos(posFCB.longitude() - posSCB.longitude()));
		StelUtils::radToDecDeg(r, sign, dd);
		aX.append(i);
		aY1.append(distanceAu);
		if (firstCelestialBody != currentPlanet && secondCelestialBody != currentPlanet)
			aY2.append(dd);
		core->update(0.0);
	}
	core->setJD(currentJD);

	QVector<double> x = aX.toVector(), y1 = aY1.toVector(), y2;
	minYld = *std::min_element(aY1.begin(), aY1.end());
	maxYld = *std::max_element(aY1.begin(), aY1.end());

	if (!aY2.isEmpty()) // mistake-proofing!
	{
		y2 = aY2.toVector();
		minYad = *std::min_element(aY2.begin(), aY2.end());
		maxYad = *std::max_element(aY2.begin(), aY2.end());
	}

	prepareDistanceAxesAndGraph();

	ui->pcDistanceGraphPlot->graph(0)->setData(x, y1);
	ui->pcDistanceGraphPlot->graph(0)->setName("[LD]");
	if (!aY2.isEmpty()) // mistake-proofing!
	{
		ui->pcDistanceGraphPlot->graph(1)->setData(x, y2);
		ui->pcDistanceGraphPlot->graph(1)->setName("[AD]");
	}
	ui->pcDistanceGraphPlot->replot();
}

void AstroCalcDialog::mouseOverDistanceGraph(QMouseEvent* event)
{
	double x = ui->pcDistanceGraphPlot->xAxis->pixelToCoord(event->pos().x());
	double y = ui->pcDistanceGraphPlot->yAxis->pixelToCoord(event->pos().y());
	double y2 = ui->pcDistanceGraphPlot->yAxis2->pixelToCoord(event->pos().y());

	QCPAbstractPlottable* abstractGraph = ui->pcDistanceGraphPlot->plottableAt(event->pos(), false);
	QCPGraph* graph = qobject_cast<QCPGraph*>(abstractGraph);

	if (x > ui->pcDistanceGraphPlot->xAxis->range().lower && x < ui->pcDistanceGraphPlot->xAxis->range().upper
	    && y > ui->pcDistanceGraphPlot->yAxis->range().lower && y < ui->pcDistanceGraphPlot->yAxis->range().upper)
	{
		if (graph)
		{
			QString info;
			if (graph->name()=="[LD]")
				info = QString("%1: %2 %3<br />%7: %8").arg(q_("Linear distance"), QString::number(y), qc_("AU", "distance, astronomical unit"), q_("Day"), QString::number(x));

			if (graph->name()=="[AD]")
				info = QString("%1: %2%3<br />%7: %8").arg(q_("Angular distance"), QString::number(y2), QChar(0x00B0), q_("Day"), QString::number(x));

			QToolTip::hideText();
			QToolTip::showText(event->globalPos(), info, ui->pcDistanceGraphPlot, ui->pcDistanceGraphPlot->rect());
		}
		else
			QToolTip::hideText();
	}

	ui->pcDistanceGraphPlot->update();
	ui->pcDistanceGraphPlot->rescaleAxes();
	ui->pcDistanceGraphPlot->replot();
}

void AstroCalcDialog::prepareAngularDistanceAxesAndGraph()
{
	QString xAxisStr = q_("Days from today");
	QString yAxisLegend = QString("%1, %2").arg(q_("Angular distance"), QChar(0x00B0)); // decimal degrees

	QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);

	ui->angularDistancePlot->xAxis->setLabel(xAxisStr);
	ui->angularDistancePlot->yAxis->setLabel(yAxisLegend);

	ui->angularDistancePlot->xAxis->setRange(-2, 31);
	ui->angularDistancePlot->xAxis->setScaleType(QCPAxis::stLinear);
	ui->angularDistancePlot->xAxis->setLabelColor(axisColor);
	ui->angularDistancePlot->xAxis->setTickLabelColor(axisColor);
	ui->angularDistancePlot->xAxis->setBasePen(axisPen);
	ui->angularDistancePlot->xAxis->setTickPen(axisPen);
	ui->angularDistancePlot->xAxis->setSubTickPen(axisPen);
	ui->angularDistancePlot->xAxis->setAutoTicks(true);

	ui->angularDistancePlot->yAxis->setRange(minYadm, maxYadm);
	ui->angularDistancePlot->yAxis->setScaleType(QCPAxis::stLinear);
	ui->angularDistancePlot->yAxis->setLabelColor(axisColor);
	ui->angularDistancePlot->yAxis->setTickLabelColor(axisColor);
	ui->angularDistancePlot->yAxis->setBasePen(axisPen);
	ui->angularDistancePlot->yAxis->setTickPen(axisPen);
	ui->angularDistancePlot->yAxis->setSubTickPen(axisPen);

	ui->angularDistancePlot->clearGraphs();
	ui->angularDistancePlot->addGraph(ui->angularDistancePlot->xAxis, ui->angularDistancePlot->yAxis);
	ui->angularDistancePlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->angularDistancePlot->graph(0)->setPen(QPen(Qt::red, 1));
	ui->angularDistancePlot->graph(0)->setLineStyle(QCPGraph::lsLine);
	ui->angularDistancePlot->graph(0)->rescaleAxes(true);

	ui->angularDistancePlot->addGraph(ui->angularDistancePlot->xAxis, ui->angularDistancePlot->yAxis);
	ui->angularDistancePlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->angularDistancePlot->graph(1)->setPen(QPen(Qt::yellow, 1));
	ui->angularDistancePlot->graph(1)->setLineStyle(QCPGraph::lsLine);
	ui->angularDistancePlot->graph(1)->rescaleAxes(true);
}

void AstroCalcDialog::drawAngularDistanceGraph()
{
	QString label = q_("Change of angular distance between the Moon and selected object");
	ui->angularDistanceLabel->setText(label);

	// special case - plot the graph when tab is visible
	//..
	// we got notified about a reason to redraw the plot, but dialog was
	// not visible. which means we must redraw when becoming visible again!
	if (!dialog->isVisible() && !plotAngularDistanceGraph)
	{
		graphPlotNeedsRefresh = true;
		return;
	}

	if (!plotAngularDistanceGraph) return;

	// special case - the tool is not applicable on non-Earth locations
	if (core->getCurrentPlanet()!=solarSystem->getEarth())
		return;

	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();
	if (!selectedObjects.isEmpty())
	{
		PlanetP moon = solarSystem->getMoon();
		StelObjectP selectedObject = selectedObjects[0];
		if (selectedObject==moon || selectedObject->getType() == "Satellite")
		{
			ui->angularDistancePlot->graph(0)->clearData();
			ui->angularDistancePlot->replot();
			return;
		}

		QList<double> aX, aY;
		Vec3d selectedObjectPosition, moonPosition;
		double currentJD = core->getJD();
		double JD, distance, dd;
		bool sign;
		for (int i = -5; i <= 35; i++)
		{
			JD = currentJD + i;
			core->setJD(JD);
			moonPosition = moon->getJ2000EquatorialPos(core);
			selectedObjectPosition = selectedObject->getJ2000EquatorialPos(core);
			distance = std::acos(sin(moonPosition.latitude()) * sin(selectedObjectPosition.latitude()) + cos(moonPosition.latitude()) * cos(selectedObjectPosition.latitude()) * cos(moonPosition.longitude() - selectedObjectPosition.longitude()));
			StelUtils::radToDecDeg(distance, sign, dd);
			aX.append(i);
			aY.append(dd);
			core->update(0.0);
		}
		core->setJD(currentJD);

		QVector<double> x = aX.toVector(), y = aY.toVector();
		minYadm = *std::min_element(aY.begin(), aY.end()) - 5.0;
		maxYadm = *std::max_element(aY.begin(), aY.end()) + 5.0;
		int limit = ui->angularDistanceLimitSpinBox->value();
		if (minYadm > limit)
			minYadm = limit - 5.0;
		if (maxYadm < limit)
			maxYadm = limit + 5.0;

		QString name = selectedObject->getNameI18n();
		if (name.isEmpty())
		{
			QString otype = selectedObject->getType();
			if (otype == "Nebula")
				name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
			if (otype == "Star" || otype=="Pulsar")
				selectedObject->getID().isEmpty() ? name = q_("Unnamed star") : name = selectedObject->getID();
		}
		ui->angularDistanceLabel->setText(QString("%1 (%2)").arg(label, name));

		prepareAngularDistanceAxesAndGraph();

		ui->angularDistancePlot->graph(0)->setData(x, y);
		ui->angularDistancePlot->graph(0)->setName("[Angular distance]");
		ui->angularDistancePlot->replot();
	}

	// clean up the data when selection is removed
	if (!objectMgr->getWasSelected())
	{
		ui->angularDistancePlot->graph(0)->clearData();
		ui->angularDistancePlot->replot();
	}
	drawAngularDistanceLimitLine();
}

void AstroCalcDialog::saveAngularDistanceLimit(int limit)
{
	conf->setValue("astrocalc/angular_distance_limit", limit);
	drawAngularDistanceLimitLine();
}

void AstroCalcDialog::drawAngularDistanceLimitLine()
{
	// special case - plot the graph when tab is visible
	if (!plotAngularDistanceGraph || !dialog->isVisible())
		return;

	int limit = ui->angularDistanceLimitSpinBox->value();

	QList<double> ax, ay;
	ax.append(-5);
	ax.append(35);
	ay.append(limit);
	ay.append(limit);
	QVector<double> x = ax.toVector(), y = ay.toVector();
	ui->angularDistancePlot->graph(1)->setData(x, y);
	ui->angularDistancePlot->replot();
}

void AstroCalcDialog::saveTableAsCSV(const QString &fileName, QTreeWidget* tWidget, QStringList &headers)
{
	int count = tWidget->topLevelItemCount();
	int columns = headers.size();

	QFile table(fileName);
	if (!table.open(QFile::WriteOnly | QFile::Truncate))
	{
		qWarning() << "AstroCalc: Unable to open file" << QDir::toNativeSeparators(fileName);
		return;
	}

	QTextStream tableData(&table);
	tableData.setCodec("UTF-8");

	for (int i = 0; i < columns; i++)
	{
		QString h = headers.at(i).trimmed();
		if (h.contains(","))
			tableData << QString("\"%1\"").arg(h);
		else
			tableData << h;

		if (i < columns - 1)
			tableData << delimiter;
		else
			tableData << StelUtils::getEndLineChar();
	}

	for (int i = 0; i < count; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			tableData << tWidget->topLevelItem(i)->text(j);
			if (j < columns - 1)
				tableData << delimiter;
			else
				tableData << StelUtils::getEndLineChar();
		}
	}

	table.close();
}

