/*
 * Stellarium
 * Copyright (C) 2015-2022 Alexander Wolf
 * Copyright (C) 2016 Nick Fedoseev (visualization of ephemeris)
 * Copyright (C) 2022 Georg Zotti
 * Copyright (C) 2022 Worachate Boonplod (Eclipses)
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
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelLocaleMgr.hpp"
#include "StelLocationMgr.hpp"
#include "StelFileMgr.hpp"
#include "AngleSpinBox.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "NebulaMgr.hpp"
#include "Nebula.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelJsonParser.hpp"
#include "planetsephems/sidereal_time.h"

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

#include <QtCharts/QtCharts>
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
using namespace QtCharts;
#endif

#include "AstroCalcDialog.hpp"
#include "AstroCalcExtraEphemerisDialog.hpp"
#include "AstroCalcCustomStepsDialog.hpp"
#include "ui_astroCalcDialog.h"

#ifdef ENABLE_XLSX
#include <xlsxdocument.h>
#include <xlsxcellrange.h>
using namespace QXlsx;
#endif

QVector<Ephemeris> AstroCalcDialog::EphemerisList;
int AstroCalcDialog::DisplayedPositionIndex = -1;
double AstroCalcDialog::brightLimit = 10.;
const QString AstroCalcDialog::dash = QChar(0x2014);
const QString AstroCalcDialog::delimiter(", ");

AstroCalcDialog::AstroCalcDialog(QObject* parent)
	: StelDialog("AstroCalc", parent)
	, extraEphemerisDialog(nullptr)
	, customStepsDialog(nullptr)
	, altVsTimeChart(nullptr)
	, azVsTimeChart(nullptr)
	, monthlyElevationChart(nullptr)
	, curvesChart(nullptr)
	, lunarElongationChart(nullptr)
	, pcChart(nullptr)
	//, wutModel(nullptr)
	//, proxyModel(nullptr)
	, currentTimeLine(nullptr)
	, plotAltVsTime(false)	
	, plotAltVsTimeSun(false)
	, plotAltVsTimeMoon(false)
	, plotAltVsTimePositive(false)
	, plotMonthlyElevation(false)
	, plotMonthlyElevationPositive(false)
	, plotDistanceGraph(false)
	, plotLunarElongationGraph(false)
	, plotAziVsTime(false)	
	, altVsTimePositiveLimit(0)
	, monthlyElevationPositiveLimit(0)
	, graphsDuration(1)
	, graphsStep(24)
	, oldGraphJD(0)
	, graphPlotNeedsRefresh(false)
{
	ui = new Ui_astroCalcDialogForm;
	core = StelApp::getInstance().getCore();
	solarSystem = GETSTELMODULE(SolarSystem);
	dsoMgr = GETSTELMODULE(NebulaMgr);
	starMgr = GETSTELMODULE(StarMgr);
	objectMgr = GETSTELMODULE(StelObjectMgr);
	localeMgr = &StelApp::getInstance().getLocaleMgr();
	mvMgr = GETSTELMODULE(StelMovementMgr);
	propMgr = StelApp::getInstance().getStelPropertyManager();
	conf = StelApp::getInstance().getSettings();
	Q_ASSERT(ephemerisHeader.isEmpty());
	Q_ASSERT(phenomenaHeader.isEmpty());
	Q_ASSERT(positionsHeader.isEmpty());
	Q_ASSERT(wutHeader.isEmpty());
	Q_ASSERT(rtsHeader.isEmpty());
	Q_ASSERT(lunareclipseHeader.isEmpty());
	Q_ASSERT(lunareclipsecontactsHeader.isEmpty());
	Q_ASSERT(solareclipseHeader.isEmpty());
	Q_ASSERT(solareclipsecontactsHeader.isEmpty());
	Q_ASSERT(solareclipselocalHeader.isEmpty());
	Q_ASSERT(transitHeader.isEmpty());
}

AstroCalcDialog::~AstroCalcDialog()
{
	if (currentTimeLine)
	{
		currentTimeLine->stop();
		delete currentTimeLine;
		currentTimeLine = nullptr;
	}
	delete ui;
	delete extraEphemerisDialog;
	delete customStepsDialog;
	if (altVsTimeChart)
		delete altVsTimeChart;
	if (azVsTimeChart)
		delete azVsTimeChart;
	if (monthlyElevationChart)
		delete monthlyElevationChart;
	if (curvesChart)
		delete curvesChart;
	if (lunarElongationChart)
		delete lunarElongationChart;
	if (pcChart)
		delete pcChart;
}

void AstroCalcDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setCelestialPositionsHeaderNames();
		setHECPositionsHeaderNames();
		setEphemerisHeaderNames();
		setRTSHeaderNames();
		setPhenomenaHeaderNames();
		setWUTHeaderNames();
		setLunarEclipseHeaderNames();
		setLunarEclipseContactsHeaderNames();
		setSolarEclipseHeaderNames();
		setSolarEclipseContactsHeaderNames();
		setSolarEclipseLocalHeaderNames();
		setTransitHeaderNames();
		populateCelestialBodyList();
		populateCelestialCategoryList();
		populateEphemerisTimeStepsList();
		populatePlanetList();
		populateGroupCelestialBodyList();
		currentCelestialPositions();
		currentHECPositions();
		populateFunctionsList();
		drawAltVsTimeDiagram();
		drawAziVsTimeDiagram();
		drawMonthlyElevationGraph();
		drawLunarElongationGraph();
		drawDistanceGraph();
		drawXVsTimeGraphs();
		populateTimeIntervalsList();
		populateWutGroups();
		// Hack to shrink the tabs to optimal size after language change
		// by causing the list items to be laid out again.
		updateTabBarListWidgetWidth();
		populateToolTips();
	}
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

	// prepare default background gradient for all charts. This is used for the outer frame, not the actual chart!
	QLinearGradient graphBackgroundGradient(QPointF(0, 0), QPointF(0, 1));
	// Colors for gradient taken from QSS's QWidget
	graphBackgroundGradient.setColorAt(0.0, QColor(90, 90, 90));
	graphBackgroundGradient.setColorAt(1.0, QColor(70, 70, 70));
	graphBackgroundGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
	ui->hecPositionsChartView->setBackgroundBrush(graphBackgroundGradient);

	initListCelestialPositions();
	initListHECPositions();
	initListPhenomena();	
	populateCelestialBodyList();
	populateCelestialCategoryList();
	populateEphemerisTimeStepsList();
	populatePlanetList();
	populateGroupCelestialBodyList();
	// Altitude vs. Time feature
	drawCurrentTimeDiagram();
	// Graphs feature
	populateFunctionsList();
	// WUT
	initListWUT();
	populateTimeIntervalsList();
	populateWutGroups();
	populateToolTips();
	// Default buttons state
	bool buttonState = false;

	ui->lunarElongationLimitSpinBox->setSuffix("°");
	ui->positiveAltitudeLimitSpinBox->setSuffix("°");
	ui->monthlyElevationPositiveLimitSpinBox->setSuffix("°");

	ui->genericMarkerColor->setText("1");
	ui->secondaryMarkerColor->setText("2");
	ui->mercuryMarkerColor->setText(QChar(0x263F));
	ui->venusMarkerColor->setText(QChar(0x2640));
	ui->marsMarkerColor->setText(QChar(0x2642));
	ui->jupiterMarkerColor->setText(QChar(0x2643));
	ui->saturnMarkerColor->setText(QChar(0x2644));

	const double JD = core->getJD() + core->getUTCOffset(core->getJD()) / 24;
	QDateTime currentDT = StelUtils::jdToQDateTime(JD, Qt::LocalTime);
	ui->dateFromDateTimeEdit->setDateTime(currentDT);
	ui->dateToDateTimeEdit->setDateTime(currentDT.addMonths(1));
	ui->phenomenFromDateEdit->setDateTime(currentDT);
	ui->phenomenToDateEdit->setDateTime(currentDT.addMonths(1));
	ui->rtsFromDateEdit->setDateTime(currentDT);
	ui->rtsToDateEdit->setDateTime(currentDT.addMonths(1));
	ui->eclipseFromYearSpinBox->setValue(currentDT.date().year());

	// TODO: Replace QDateTimeEdit by a new StelDateTimeEdit widget to apply full range of dates
	// NOTE: https://github.com/Stellarium/stellarium/issues/711
	const QDate minDate = QDate(1582, 10, 15); // QtDateTime's minimum date is 1.1.100AD, but appears to be always Gregorian.
	ui->dateFromDateTimeEdit->setMinimumDate(minDate);
	ui->dateToDateTimeEdit->setMinimumDate(minDate);
	ui->phenomenFromDateEdit->setMinimumDate(minDate);
	ui->phenomenToDateEdit->setMinimumDate(minDate);
	ui->rtsFromDateEdit->setMinimumDate(minDate);
	ui->rtsToDateEdit->setMinimumDate(minDate);
	ui->pushButtonExtraEphemerisDialog->setFixedSize(QSize(20, 20));
	ui->pushButtonCustomStepsDialog->setFixedSize(QSize(26, 26));
	ui->pushButtonCustomStepsDialog->setIconSize(QSize(20, 20));
	ui->pushButtonNow->setFixedSize(QSize(26, 26));
	ui->pushButtonNow->setIconSize(QSize(20, 20));

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
	connect(dsoMgr, SIGNAL(catalogFiltersChanged(int)), this, SLOT(populateCelestialCategoryList()));
	connect(dsoMgr, SIGNAL(catalogFiltersChanged(int)), this, SLOT(currentCelestialPositions()));
	connect(dsoMgr, SIGNAL(flagSizeLimitsUsageChanged(bool)), this, SLOT(currentCelestialPositions()));
	connect(dsoMgr, SIGNAL(minSizeLimitChanged(double)), this, SLOT(currentCelestialPositions()));
	connect(dsoMgr, SIGNAL(maxSizeLimitChanged(double)), this, SLOT(currentCelestialPositions()));

	ui->hecSelectedMinorPlanetsCheckBox->setChecked(conf->value("astrocalc/flag_hec_minor_planets", false).toBool());
	connect(ui->hecSelectedMinorPlanetsCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveHECFlagMinorPlanets(bool)));

	const bool brightCometsState = conf->value("astrocalc/flag_hec_bright_comets", false).toBool();
	ui->hecBrightCometsCheckBox->setChecked(brightCometsState);
	connect(ui->hecBrightCometsCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveHECFlagBrightComets(bool)));
	ui->hecMagnitudeLimitSpinBox->setValue(conf->value("astrocalc/hec_magnitude_limit", 9.0).toDouble());
	ui->hecMagnitudeLimitSpinBox->setEnabled(brightCometsState);
	connect(ui->hecMagnitudeLimitSpinBox, SIGNAL(valueChanged(double)), this,  SLOT(saveHECBrightCometMagnitudeLimit(double)));
	connect(ui->hecPositionsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentHECPosition(QModelIndex)));
	connect(ui->hecPositionsTreeWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(markCurrentHECPosition(QModelIndex)));
	connect(ui->hecPositionsUpdateButton, SIGNAL(clicked()), this, SLOT(currentHECPositions()));
	connect(ui->hecPositionsSaveButton, SIGNAL(clicked()), this, SLOT(saveHECPositions()));
	connect(ui->tabWidgetPositions, SIGNAL(currentChanged(int)), this, SLOT(changePositionsTab(int)));

	connectBoolProperty(ui->ephemerisShowLineCheckBox, "SolarSystem.ephemerisLineDisplayed");
	connectBoolProperty(ui->ephemerisShowMarkersCheckBox, "SolarSystem.ephemerisMarkersDisplayed");
	connectBoolProperty(ui->ephemerisShowDatesCheckBox, "SolarSystem.ephemerisDatesDisplayed");
	connectBoolProperty(ui->ephemerisShowMagnitudesCheckBox, "SolarSystem.ephemerisMagnitudesDisplayed");
	connectBoolProperty(ui->ephemerisHorizontalCoordinatesCheckBox, "SolarSystem.ephemerisHorizontalCoordinates");
	initListEphemeris();
	initEphemerisFlagNakedEyePlanets();
	enableEphemerisButtons(buttonState);
	ui->ephemerisIgnoreDateTestCheckBox->setChecked(conf->value("astrocalc/flag_ephemeris_ignore_date_test", true).toBool());
	connect(ui->ephemerisIgnoreDateTestCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveIgnoreDateTestFlag(bool)));
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
	connect(ui->pushButtonNow, SIGNAL(clicked()), this, SLOT(setDateTimeNow()));

	connectColorButton(ui->genericMarkerColor, "SolarSystem.ephemerisGenericMarkerColor", "color/ephemeris_generic_marker_color");
	connectColorButton(ui->secondaryMarkerColor, "SolarSystem.ephemerisSecondaryMarkerColor", "color/ephemeris_secondary_marker_color");
	connectColorButton(ui->selectedMarkerColor, "SolarSystem.ephemerisSelectedMarkerColor", "color/ephemeris_selected_marker_color");
	connectColorButton(ui->mercuryMarkerColor, "SolarSystem.ephemerisMercuryMarkerColor", "color/ephemeris_mercury_marker_color");
	connectColorButton(ui->venusMarkerColor, "SolarSystem.ephemerisVenusMarkerColor", "color/ephemeris_venus_marker_color");
	connectColorButton(ui->marsMarkerColor, "SolarSystem.ephemerisMarsMarkerColor", "color/ephemeris_mars_marker_color");
	connectColorButton(ui->jupiterMarkerColor, "SolarSystem.ephemerisJupiterMarkerColor", "color/ephemeris_jupiter_marker_color");
	connectColorButton(ui->saturnMarkerColor, "SolarSystem.ephemerisSaturnMarkerColor", "color/ephemeris_saturn_marker_color");

	// Tab: Transits
	initListRTS();
	enableRTSButtons(buttonState);
	connect(ui->rtsCalculateButton, SIGNAL(clicked()), this, SLOT(generateRTS()));
	connect(ui->rtsCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupRTS()));
	connect(ui->rtsSaveButton, SIGNAL(clicked()), this, SLOT(saveRTS()));
	connect(ui->rtsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentRTS(QModelIndex)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(setRTSCelestialBodyName()));

	// Tab: Eclipses
	ui->eclipseYearsSpinBox->setValue(conf->value("astrocalc/eclipse_future_years", 10).toInt());
	connect(ui->eclipseYearsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val){conf->setValue("astrocalc/eclipse_future_years", val);}); // Make that a permanent decision.
	initListLunarEclipse();
	enableLunarEclipsesButtons(buttonState);
	connect(ui->lunareclipsesCalculateButton, SIGNAL(clicked()), this, SLOT(generateLunarEclipses()));
	connect(ui->lunareclipsesCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupLunarEclipses()));
	connect(ui->lunareclipsesSaveButton, SIGNAL(clicked()), this, SLOT(saveLunarEclipses()));
	initListLunarEclipseContact();
	enableLunarEclipsesCircumstancesButtons(buttonState);
	connect(ui->lunareclipsescontactsSaveButton, SIGNAL(clicked()), this, SLOT(saveLunarEclipseCircumstances()));
	connect(ui->lunareclipseTreeWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(selectCurrentLunarEclipse(QModelIndex)));
	connect(ui->lunareclipseTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentLunarEclipseDate(QModelIndex)));
	connect(ui->lunareclipsecontactsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentLunarEclipseContact(QModelIndex)));
	initListSolarEclipse();
	enableSolarEclipsesButtons(buttonState);
	connect(ui->solareclipsesCalculateButton, SIGNAL(clicked()), this, SLOT(generateSolarEclipses()));
	connect(ui->solareclipsesCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupSolarEclipses()));
	connect(ui->solareclipsesSaveButton, SIGNAL(clicked()), this, SLOT(saveSolarEclipses()));
	initListSolarEclipseContact();
	enableSolarEclipsesCircumstancesButtons(buttonState);
	connect(ui->solareclipsescontactsSaveButton, SIGNAL(clicked()), this, SLOT(saveSolarEclipseCircumstances()));
	connect(ui->solareclipsesKMLSaveButton, SIGNAL(clicked()), this, SLOT(saveSolarEclipseKML()));
	connect(ui->solareclipseTreeWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(selectCurrentSolarEclipse(QModelIndex)));
	connect(ui->solareclipseTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentSolarEclipseDate(QModelIndex)));
	connect(ui->solareclipsecontactsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentSolarEclipseContact(QModelIndex)));
	initListSolarEclipseLocal();
	enableSolarEclipsesLocalButtons(buttonState);
	connect(ui->solareclipseslocalCalculateButton, SIGNAL(clicked()), this, SLOT(generateSolarEclipsesLocal()));
	connect(ui->solareclipseslocalCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupSolarEclipsesLocal()));
	connect(ui->solareclipseslocalSaveButton, SIGNAL(clicked()), this, SLOT(saveSolarEclipsesLocal()));
	connect(ui->solareclipselocalTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentSolarEclipseLocal(QModelIndex)));
	initListTransit();
	enableTransitsButtons(buttonState);
	connect(ui->transitsCalculateButton, SIGNAL(clicked()), this, SLOT(generateTransits()));
	connect(ui->transitsCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupTransits()));
	connect(ui->transitsSaveButton, SIGNAL(clicked()), this, SLOT(saveTransits()));
	connect(ui->transitTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentTransit(QModelIndex)));

	// Let's use DMS and decimal degrees as acceptable values for "Maximum allowed separation" input box
	ui->allowedSeparationSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->allowedSeparationSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
	ui->allowedSeparationSpinBox->setMinimum(0.0, true);
	ui->allowedSeparationSpinBox->setMaximum(20.0, true);
	ui->allowedSeparationSpinBox->setWrapping(false);
	enablePhenomenaButtons(buttonState);

	ui->phenomenaOppositionCheckBox->setChecked(conf->value("astrocalc/flag_phenomena_opposition", false).toBool());
	connect(ui->phenomenaOppositionCheckBox, SIGNAL(toggled(bool)), this, SLOT(savePhenomenaOppositionFlag(bool)));
	ui->phenomenaPerihelionAphelionCheckBox->setChecked(conf->value("astrocalc/flag_phenomena_perihelion", false).toBool());
	connect(ui->phenomenaPerihelionAphelionCheckBox, SIGNAL(toggled(bool)), this, SLOT(savePhenomenaPerihelionAphelionFlag(bool)));
	ui->phenomenaElongationQuadratureCheckBox->setChecked(conf->value("astrocalc/flag_phenomena_quadratures", false).toBool());
	connect(ui->phenomenaElongationQuadratureCheckBox, SIGNAL(toggled(bool)), this, SLOT(savePhenomenaElongationsQuadraturesFlag(bool)));
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
	altVsTimePositiveLimit = conf->value("astrocalc/altvstime_positive_limit", 0).toInt();
	ui->sunAltitudeCheckBox->setChecked(plotAltVsTimeSun);
	ui->moonAltitudeCheckBox->setChecked(plotAltVsTimeMoon);
	ui->positiveAltitudeOnlyCheckBox->setChecked(plotAltVsTimePositive);
	ui->positiveAltitudeLimitSpinBox->setValue(conf->value("astrocalc/altvstime_positive_limit", 0).toInt());
	connect(ui->sunAltitudeCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveAltVsTimeSunFlag(bool)));
	connect(ui->moonAltitudeCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveAltVsTimeMoonFlag(bool)));
	connect(ui->positiveAltitudeOnlyCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveAltVsTimePositiveFlag(bool)));
	connect(ui->positiveAltitudeLimitSpinBox, SIGNAL(valueChanged(int)), this, SLOT(saveAltVsTimePositiveLimit(int)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawAltVsTimeDiagram()));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawAziVsTimeDiagram()));
	connect(core, SIGNAL(dateChanged()), this, SLOT(drawCurrentTimeDiagram()));
	connect(this, SIGNAL(graphDayChanged()), this, SLOT(drawAltVsTimeDiagram()));
	connect(this, SIGNAL(graphDayChanged()), this, SLOT(drawAziVsTimeDiagram()));
	connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(handleVisibleEnabled()));

	// Monthly Elevation
	plotMonthlyElevationPositive = conf->value("astrocalc/me_positive_only", false).toBool();
	ui->monthlyElevationPositiveCheckBox->setChecked(plotMonthlyElevationPositive);
	monthlyElevationPositiveLimit=(conf->value("astrocalc/me_positive_limit", 0).toInt());
	ui->monthlyElevationPositiveLimitSpinBox->setValue(monthlyElevationPositiveLimit);
	ui->monthlyElevationTime->setValue(conf->value("astrocalc/me_time", 0).toInt());
	syncMonthlyElevationTime();
	connect(ui->monthlyElevationTime, SIGNAL(valueChanged(int)), this, SLOT(updateMonthlyElevationTime()));
	connect(ui->monthlyElevationPositiveCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveMonthlyElevationPositiveFlag(bool)));
	connect(ui->monthlyElevationPositiveLimitSpinBox, SIGNAL(valueChanged(int)), this, SLOT(saveMonthlyElevationPositiveLimit(int)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawMonthlyElevationGraph()));
	connect(core, SIGNAL(dateChangedByYear(const int)), this, SLOT(drawMonthlyElevationGraph()));

	connect(ui->graphsCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveGraphsCelestialBody(int)));
	connect(ui->graphsFirstComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveGraphsFirstId(int)));
	connect(ui->graphsSecondComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveGraphsSecondId(int)));
	graphsDuration = qBound(1, conf->value("astrocalc/graphs_duration",1).toInt(), 600);
	ui->graphsDurationSpinBox->setValue(graphsDuration);
	connect(ui->graphsDurationSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateGraphsDuration(int)));
	graphsStep = qBound(1, conf->value("astrocalc/graphs_step",1).toInt(), 240);
	ui->graphsHourStepsSpinBox->setValue(graphsStep);
	connect(ui->graphsHourStepsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateGraphsStep(int)));
	connect(ui->drawGraphsPushButton, SIGNAL(clicked()), this, SLOT(drawXVsTimeGraphs()));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(updateXVsTimeGraphs()));	

	ui->lunarElongationLimitSpinBox->setValue(conf->value("astrocalc/angular_distance_limit", 40).toInt());
	connect(ui->lunarElongationLimitSpinBox, SIGNAL(valueChanged(int)), this, SLOT(saveLunarElongationLimit(int)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawLunarElongationGraph()));
	connect(core, SIGNAL(dateChanged()), this, SLOT(drawLunarElongationGraph()));

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
	ui->wutAltitudeMinSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->wutAltitudeMinSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
	ui->wutAltitudeMinSpinBox->setMinimum(0.0, true);
	ui->wutAltitudeMinSpinBox->setMaximum(90.0, true);
	ui->wutAltitudeMinSpinBox->setWrapping(false);
	ui->saveObjectsButton->setEnabled(buttonState);

	// Convert from angular minutes
	ui->wutAngularSizeLimitMinSpinBox->setDegrees(conf->value("astrocalc/wut_angular_limit_min", 10.0).toDouble()/60.0);
	ui->wutAngularSizeLimitMaxSpinBox->setDegrees(conf->value("astrocalc/wut_angular_limit_max", 600.0).toDouble()/60.0);
	ui->wutAltitudeMinSpinBox->setDegrees(conf->value("astrocalc/wut_altitude_min", 0.0).toDouble());
	connect(ui->wutAngularSizeLimitCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveWutAngularSizeFlag(bool)));
	connect(ui->wutAngularSizeLimitMinSpinBox, SIGNAL(valueChanged()), this, SLOT(saveWutMinAngularSizeLimit()));
	connect(ui->wutAngularSizeLimitMaxSpinBox, SIGNAL(valueChanged()), this, SLOT(saveWutMaxAngularSizeLimit()));
	connect(ui->wutAltitudeMinSpinBox, SIGNAL(valueChanged()), this, SLOT(saveWutMinAltitude()));

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
	connect(dsoMgr, SIGNAL(catalogFiltersChanged(int)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(typeFiltersChanged(int)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(flagSizeLimitsUsageChanged(bool)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(minSizeLimitChanged(double)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(maxSizeLimitChanged(double)), this, SLOT(calculateWutObjects()));
	connect(core, SIGNAL(dateChanged()), this, SLOT(calculateWutObjects()));

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
	currentTimeLine->start(1000); // Update 'now' line position every second

	connect(ui->firstCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveFirstCelestialBody(int)));
	connect(ui->secondCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveSecondCelestialBody(int)));
	connect(core, SIGNAL(dateChanged()), this, SLOT(drawDistanceGraph()));

	connect(solarSystem, SIGNAL(solarSystemDataReloaded()), this, SLOT(updateSolarSystemData()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(updateAstroCalcData()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawAltVsTimeDiagram()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawMonthlyElevationGraph()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawDistanceGraph()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawLunarElongationGraph()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(initEphemerisFlagNakedEyePlanets()));

	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(changePage(QListWidgetItem*, QListWidgetItem*)));
	connect(ui->tabWidgetGraphs, SIGNAL(currentChanged(int)), this, SLOT(changeGraphsTab(int)));
	connect(ui->tabWidgetPC, SIGNAL(currentChanged(int)), this, SLOT(changePCTab(int)));
	connect(ui->tabWidgetEclipses, SIGNAL(currentChanged(int)), this, SLOT(changeEclipsesTab(int)));

	connect(ui->pushButtonExtraEphemerisDialog, SIGNAL(clicked()), this, SLOT(showExtraEphemerisDialog()));
	connect(ui->pushButtonCustomStepsDialog, SIGNAL(clicked()), this, SLOT(showCustomStepsDialog()));

	updateTabBarListWidgetWidth();

	ui->celestialPositionsUpdateButton->setShortcut(QKeySequence("Shift+F10"));
	ui->hecPositionsUpdateButton->setShortcut(QKeySequence("Shift+F10"));
	ui->ephemerisPushButton->setShortcut(QKeySequence("Shift+F10"));
	ui->rtsCalculateButton->setShortcut(QKeySequence("Shift+F10"));
	ui->phenomenaPushButton->setShortcut(QKeySequence("Shift+F10"));
	ui->solareclipsesCalculateButton->setShortcut(QKeySequence("Shift+F10"));
	ui->solareclipseslocalCalculateButton->setShortcut(QKeySequence("Shift+F10"));
	ui->lunareclipsesCalculateButton->setShortcut(QKeySequence("Shift+F10"));
	ui->transitsCalculateButton->setShortcut(QKeySequence("Shift+F10"));

	// chartview exports:
	connect(ui->hecPositionsExportButton, &QPushButton::clicked, this, [=]{ saveGraph(ui->hecPositionsChartView); });
	connect(ui->exportAltVsTimePushButton, &QPushButton::clicked, this, [=]{ saveGraph(ui->altVsTimeChartView); });
	connect(ui->exportAziVsTimePushButton, &QPushButton::clicked, this, [=]{ saveGraph(ui->aziVsTimeChartView); });
	connect(ui->exportMonthlyElevationPushButton, &QPushButton::clicked, this, [=]{ saveGraph(ui->monthlyElevationChartView); });
	connect(ui->exportGraphsPushButton, &QPushButton::clicked, this, [=]{ saveGraph(ui->twoGraphsChartView); });
	connect(ui->exportLunarElongationPushButton, &QPushButton::clicked, this, [=]{ saveGraph(ui->lunarElongationChartView); });
	connect(ui->exportPCPushButton, &QPushButton::clicked, this, [=]{ saveGraph(ui->pcChartView); });
}

void AstroCalcDialog::populateToolTips()
{
	QString validDates = QString("%1 1582/10/15 - 9999/12/31").arg(q_("Gregorian dates. Valid range:"));
	ui->dateFromDateTimeEdit->setToolTip(validDates);
	ui->dateToDateTimeEdit->setToolTip(validDates);
	ui->phenomenFromDateEdit->setToolTip(validDates);
	ui->phenomenToDateEdit->setToolTip(validDates);
	ui->rtsFromDateEdit->setToolTip(validDates);
	ui->rtsToDateEdit->setToolTip(validDates);
	ui->eclipseFromYearSpinBox->setToolTip(QString("%1 %2..%3").arg(q_("Valid range years:"), QString::number(ui->eclipseFromYearSpinBox->minimum()), QString::number(ui->eclipseFromYearSpinBox->maximum())));
	ui->allowedSeparationSpinBox->setToolTip(QString("%1: %2..%3").arg(q_("Valid range"), StelUtils::decDegToDmsStr(ui->allowedSeparationSpinBox->getMinimum(true)), StelUtils::decDegToDmsStr(ui->allowedSeparationSpinBox->getMaximum(true))));
	ui->allowedSeparationLabel->setToolTip(QString("<p>%1</p>").arg(q_("This is a tolerance for the angular distance for conjunctions and oppositions from 0 and 180 degrees respectively.")));
	ui->hecMagnitudeLimitSpinBox->setToolTip(QString("%1 %2..%3").arg(q_("Valid range magnitudes:"), QString::number(ui->hecMagnitudeLimitSpinBox->minimum(), 'f', 2), QString::number(ui->hecMagnitudeLimitSpinBox->maximum(), 'f', 2)));
}

void AstroCalcDialog::saveGraph(QChartView *graph)
{
	QString dir = StelFileMgr::getScreenshotDir();
	if (dir.isEmpty())
		dir = StelFileMgr::getUserDir();
	QString filter = q_("Images");
	filter.append(" (*.png)");
	QString filename = QFileDialog::getSaveFileName(nullptr, q_("Save graph"), dir + "/stellarium-graph.png", filter);
	if (!filename.isEmpty())
	{
		QPixmap p = graph->grab();
		p.save(filename, "PNG");
	}
}

void AstroCalcDialog::populateCelestialNames(QString)
{
	populateCelestialBodyList();
	populatePlanetList();
}

void AstroCalcDialog::showExtraEphemerisDialog()
{
	if (extraEphemerisDialog == nullptr)
		extraEphemerisDialog = new AstroCalcExtraEphemerisDialog();

	extraEphemerisDialog->setVisible(true);
}

void AstroCalcDialog::showCustomStepsDialog()
{
	if (customStepsDialog == nullptr)
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

void AstroCalcDialog::drawAziVsTimeDiagram()
{
	if (!azVsTimeChartMutex.tryLock()) return; // Avoid calling parallel from various sides. (called by signals/slots)
	// Avoid crash!
	if (core->getCurrentPlanet()->getEnglishName().contains("->")) // We are on the spaceship!
	{
		azVsTimeChartMutex.unlock();
		return;
	}

	// special case - plot the graph when tab is visible
	//..
	// we got notified about a reason to redraw the plot, but dialog was
	// not visible. which means we must redraw when becoming visible again!
	if (!dialog->isVisible() && plotAziVsTime)
	{
		graphPlotNeedsRefresh = true;
		azVsTimeChartMutex.unlock();
		return;
	}

	if (!plotAziVsTime)
	{
		azVsTimeChartMutex.unlock();
		return;
	}

	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();
	azVsTimeChart = new AstroCalcChart({AstroCalcChart::AzVsTime, AstroCalcChart::CurrentTime});

	if (!selectedObjects.isEmpty())
	{
		const bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
		StelObjectP selectedObject = selectedObjects[0];
		azVsTimeChart->setTitle(QString("%1 (%2)").arg(getSelectedObjectNameI18n(selectedObject), selectedObject->getObjectTypeI18n()));
		const double currentJD = core->getJD();
		const double shift = core->getUTCOffset(currentJD) / 24.0;
		const double noon = std::floor(currentJD + shift); // Integral JD of this day

#ifdef USE_STATIC_PLUGIN_SATELLITES
		bool isSatellite = false;

		SatelliteP sat;		
		if (selectedObject->getType() == "Satellite") 
		{
			// get reference to satellite
			isSatellite = true;
			sat = GETSTELMODULE(Satellites)->getById(selectedObject.staticCast<Satellite>()->getCatalogNumberString());
		}
#endif

		for (int i = 0; i <= 86400; i+=180) // 24 hours in 3-min steps
		{
			// A new point on the graph every 3 minutes with shift to right 12 hours
			// to get midnight at the center of diagram (i.e. accuracy is 3 minutes)
			double JD = noon + i / 86400. - shift;
			core->setJD(JD);
			
#ifdef USE_STATIC_PLUGIN_SATELLITES
			if (isSatellite)
			{
				sat->update(0.0);
			}
			else
#endif
				core->update(0.0);

			double az, alt;
			StelUtils::rectToSphe(&az, &alt, selectedObject->getAltAzPosAuto(core));
			const double direction = useSouthAzimuth ? 2. : 3.; // N is zero, E is 90 degrees
			az = direction*M_PI - az;
			if (az > M_PI*2)
				az -= M_PI*2;

			// We must shift the JD value to trick the display into zone time.
			azVsTimeChart->append(AstroCalcChart::AzVsTime, StelUtils::jdToQDateTime(JD, Qt::UTC).toMSecsSinceEpoch(), az*M_180_PI);
		}
		azVsTimeChart->show(AstroCalcChart::AzVsTime);
		core->setJD(currentJD);

		const QPair<double, double>&yRange=azVsTimeChart->findYRange(AstroCalcChart::AzVsTime);
		azVsTimeChart->setYrange(AstroCalcChart::AzVsTime, yRange);

		drawCurrentTimeDiagram();
	}
	else
	{
		azVsTimeChart->setYrange(AstroCalcChart::AzVsTime, 0., 360.);
		azVsTimeChart->clear(AstroCalcChart::AzVsTime);
		azVsTimeChart->setTitle(q_("Please select object to plot its graph 'Azimuth vs. Time'."));
	}

	azVsTimeChart->setupAxes(core->getJD(), 1, "");
	QChart *oldChart=ui->aziVsTimeChartView->chart();
	if (oldChart) oldChart->deleteLater();
	ui->aziVsTimeChartView->setChart(azVsTimeChart);
	ui->aziVsTimeChartView->setRenderHint(QPainter::Antialiasing);

	azVsTimeChartMutex.unlock();
}

void AstroCalcDialog::initListCelestialPositions()
{
	ui->celestialPositionsTreeWidget->clear();
	ui->celestialPositionsTreeWidget->setColumnCount(CColumnCount);
	setCelestialPositionsHeaderNames();
	ui->celestialPositionsTreeWidget->header()->setSectionsMovable(false);
	ui->celestialPositionsTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
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
	if (celType >= 200)
	{
		// TRANSLATORS: angular size, arc-seconds
		positionsHeader << QString("%1, \"").arg(q_("A.S."));
	}
	else
	{
		// TRANSLATORS: angular size, arc-minutes
		positionsHeader << QString("%1, '").arg(q_("A.S."));
	}

	if (celType == 170)
	{
		// TRANSLATORS: separation, arc-seconds
		positionsHeader << QString("%1, \"").arg(q_("Sep."));
	}
	else if (celType == 171 || celType == 173 || celType == 174)
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
		// TRANSLATORS: proper motion, arc-second per year
		positionsHeader << QString("%1, %2").arg(q_("P.M."), qc_("\"/yr", "arc-second per year"));
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

void AstroCalcDialog::initListHECPositions()
{
	ui->hecPositionsTreeWidget->clear();
	ui->hecPositionsTreeWidget->setColumnCount(HECColumnCount);
	setHECPositionsHeaderNames();
	ui->hecPositionsTreeWidget->header()->setSectionsMovable(false);
	ui->hecPositionsTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
}

void AstroCalcDialog::setHECPositionsHeaderNames()
{
	hecPositionsHeader = QStringList({
		// TRANSLATORS: name of object
		q_("Name"),
		q_("Symbol"),
		// TRANSLATORS: ecliptic latitude
		q_("Latitude"),
		// TRANSLATORS: ecliptic longitude
		q_("Longitude"),
		// TRANSLATORS: distance
		q_("Distance")});

	ui->hecPositionsTreeWidget->setHeaderLabels(hecPositionsHeader);
	adjustHECPositionsColumns();
}

void AstroCalcDialog::adjustHECPositionsColumns()
{
	// adjust the column width
	for (int i = 0; i < HECColumnCount; ++i)
	{
		ui->hecPositionsTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::onChangedEphemerisPosition()
{
	DisplayedPositionIndex =ui->ephemerisTreeWidget->currentItem()->data(EphemerisRA, Qt::UserRole).toInt();
}

double AstroCalcDialog::computeMaxElevation(StelObjectP obj)
{
	// NOTE: Without refraction!
	double ra, dec, elevation;
	const double lat = static_cast<double>(core->getCurrentLocation().getLatitude());
	StelUtils::rectToSphe(&ra, &dec, obj->getEquinoxEquatorialPos(core));
	dec *= M_180_PI;
	if (lat>=0.)
	{
		// star between zenith and southern horizon, or between N pole and zenith
		elevation = (dec<=lat) ? (90. - lat + dec) : (90. + lat - dec);
	}
	else
	{
		// star between zenith and north horizon, or between S pole and zenith
		elevation = (dec>=lat) ? (90. + lat - dec) : (90. - lat + dec);
	}
	return elevation * M_PI_180;
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
	while(it.hasNext())
	{
		it.next();
		QString key = it.key();
		if (key.startsWith("NebulaMgr") && key.contains(":"))
			category->addItem(q_(it.value()), key.remove("NebulaMgr:"));

		if (key.startsWith("StarMgr") && key.contains(":"))
		{
			int kn = key.remove("StarMgr:").toInt();
			if (kn>1 && kn<=6) // Original IDs: 2, 3, 4, 5, 6
				category->addItem(q_(it.value()), QString::number(kn + 168)); // AstroCalc IDs: 170, 171, 172, 173, 174
		}
	}
	category->addItem(q_("Deep-sky objects"), "169");
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

void AstroCalcDialog::fillCelestialPositionTable(const QString &objectName, const QString &RA, const QString &Dec, double magnitude,
						 const QString &angularSize, const QString &angularSizeToolTip, const QString &extraData,
						 const QString &extraDataToolTip, const QString &transitTime, const QString &maxElevation,
						  QString &sElongation, const QString &objectType)
{
	ACCelPosTreeWidgetItem* treeItem = new ACCelPosTreeWidgetItem(ui->celestialPositionsTreeWidget);
	treeItem->setText(CColumnName, objectName);
	treeItem->setText(CColumnRA, RA);
	treeItem->setTextAlignment(CColumnRA, Qt::AlignRight);
	treeItem->setText(CColumnDec, Dec);
	treeItem->setTextAlignment(CColumnDec, Qt::AlignRight);
	if (magnitude>90.)
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
	treeItem->setText(CColumnElongation, sElongation.replace("+","",Qt::CaseInsensitive)); // remove sign
	treeItem->setTextAlignment(CColumnElongation, Qt::AlignRight);
	treeItem->setToolTip(CColumnElongation, q_("Angular distance from the Sun at the moment of computation of position"));
	treeItem->setText(CColumnType, objectType);
}

void AstroCalcDialog::currentCelestialPositions()
{
	QPair<QString, QString> coordStrings;

	initListCelestialPositions();
	ui->celestialPositionsTreeWidget->showColumn(CColumnAngularSize);

	const double mag = ui->celestialMagnitudeDoubleSpinBox->value();
	const bool horizon = ui->horizontalCoordinatesCheckBox->isChecked();
	const bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

	const double JD = core->getJD();
	const double utcShift = core->getUTCOffset(core->getJD()) / 24.; // Fix DST shift...
	ui->celestialPositionsTimeLabel->setText(q_("Positions on %1").arg(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))));
	Vec4d rts;
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
			mu = QString("%1/%2<sup>2</sup>").arg(qc_("mag", "magnitude"), q_("arc-min"));
			if (dsoMgr->getFlagSurfaceBrightnessArcsecUsage())
				mu = QString("%1/%2<sup>2</sup>").arg(qc_("mag", "magnitude"), q_("arc-sec"));
		}
		double magOp;
		bool passByBrightness;
		QString dsoName;
		QString asToolTip = QString("%1, %2").arg(q_("Average angular size"), q_("arc-min"));
		// Deep-sky objects
		QList<NebulaP> celestialObjects;
		if (celTypeId==169)
			celestialObjects = dsoMgr->getAllDeepSkyObjects().toList();
		else
			celestialObjects = dsoMgr->getDeepSkyObjectsByType(celType);
		for (const auto& obj : qAsConst(celestialObjects))
		{
			if (celTypeId == 12 || celTypeId == 102 || celTypeId == 111) // opacity cannot be extincted
				magOp = static_cast<double>(obj->getVMagnitude(core));
			else
				magOp = static_cast<double>(obj->getVMagnitudeWithExtinction(core));

			if (celTypeId==35 || (celTypeId==169 && obj->getDSOType()==Nebula::NebDn)) // Regions of the sky
			{
				passByBrightness = true;
				magOp = 99.;
			}
			else
				passByBrightness = (magOp <= mag);

			if (obj->objectInDisplayedCatalog() && obj->objectInAllowedSizeRangeLimits() && passByBrightness && obj->isAboveRealHorizon(core))
			{
				coordStrings = getStringCoordinates(horizon ? obj->getAltAzPosAuto(core) : obj->getJ2000EquatorialPos(core), horizon, useSouthAzimuth, withDecimalDegree);

				QString celObjName = obj->getNameI18n();
				QString celObjId = obj->getDSODesignation();
				QString elongStr;
				if (celObjId.isEmpty())
					dsoName = celObjName;
				else if (celObjName.isEmpty())
					dsoName = celObjId;
				else
					dsoName = QString("%1 (%2)").arg(celObjId, celObjName);

				QString extra = QString::number(static_cast<double>(obj->getSurfaceBrightnessWithExtinction(core)), 'f', 2);
				if (extra.toFloat() > 90.f)
					extra = dash;

				// Convert to arc-minutes the average angular size of deep-sky object
				QString angularSize = QString::number(obj->getAngularRadius(core) * 120., 'f', 3);
				if (angularSize.toFloat() < 0.01f)
					angularSize = dash;

				QString sTransit = dash;
				QString sMaxElevation = dash;
				rts = obj->getRTSTime(core);
				//if (rts[1]>=0.)
				if (rts[3]!=20)
				{
					sTransit = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(rts[1]+utcShift), true);
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

				fillCelestialPositionTable(dsoName, coordStrings.first, coordStrings.second, magOp, angularSize, asToolTip, extra, mu, sTransit, sMaxElevation, elongStr, obj->getObjectTypeI18n());
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
		QString asToolTip = QString("%1, %2").arg(q_("Angular size (with rings, if any)"), q_("arc-sec"));
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

		for (const auto& planet : qAsConst(planets))
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

			if (!planet->hasValidPositionalData(JD, Planet::PositionQuality::OrbitPlotting))
				passByType = false;

			if (passByType && planet != core->getCurrentPlanet() && static_cast<double>(planet->getVMagnitudeWithExtinction(core)) <= mag && planet->isAboveRealHorizon(core))
			{
				pos = planet->getJ2000EquatorialPos(core);

				if (horizon)
					coordStrings = getStringCoordinates(planet->getAltAzPosAuto(core), horizon, useSouthAzimuth, withDecimalDegree);
				else
					coordStrings = getStringCoordinates(pos, horizon, useSouthAzimuth, withDecimalDegree);

				QString extra = QString::number(pos.norm(), 'f', 5); // A.U.

				// Convert to arc-seconds the angular size of Solar system object (with rings, if any)
				QString angularSize = QString::number(planet->getAngularRadius(core) * 7200., 'f', 2);
				if (angularSize.toFloat() < 1e-4f || planet->getPlanetType() == Planet::isComet)
					angularSize = dash;

				QString sTransit = dash;
				QString sMaxElevation = dash;
				QString elongStr;
				rts = planet->getRTSTime(core);
				//if (rts[1]>=0.)
				if (rts[3]!=20)
				{
					sTransit = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(rts[1]+utcShift), true);
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

				fillCelestialPositionTable(planet->getNameI18n(), coordStrings.first, coordStrings.second, planet->getVMagnitudeWithExtinction(core), angularSize, asToolTip, extra, sToolTip, sTransit, sMaxElevation, elongStr, planet->getObjectTypeI18n());
			}
		}		
	}
	else
	{
		// stars
		QString commonName, sToolTip = "";
		QList<StelACStarData> celestialObjects;
		if (celTypeId == 170)
		{
			// double stars
			celestialObjects = starMgr->getHipparcosDoubleStars();
		}
		else if (celTypeId == 171 || celTypeId == 173 || celTypeId == 174)
		{
			// variable stars
			switch (celTypeId)
			{
				case 171:
					celestialObjects = starMgr->getHipparcosVariableStars();
					break;
				case 173:
					celestialObjects = starMgr->getHipparcosAlgolTypeStars();
					break;
				case 174:
					celestialObjects = starMgr->getHipparcosClassicalCepheidsTypeStars();
					break;
			}
		}
		else
		{
			// stars with high proper motion
			celestialObjects = starMgr->getHipparcosHighPMStars();
		}

		for (const auto& star : qAsConst(celestialObjects))
		{
			StelObjectP obj = star.firstKey();
			QString extra, elongStr;
			if (static_cast<double>(obj->getVMagnitudeWithExtinction(core)) <= mag && obj->isAboveRealHorizon(core))
			{
				if (horizon)
					coordStrings = getStringCoordinates(obj->getAltAzPosAuto(core), horizon, useSouthAzimuth, withDecimalDegree);
				else
					coordStrings = getStringCoordinates(obj->getJ2000EquatorialPos(core), horizon, useSouthAzimuth, withDecimalDegree);

				if (celTypeId == 170) // double stars
				{
					double wdsSep = static_cast<double>(star.value(obj));
					extra = QString::number(wdsSep, 'f', 3); // arc-seconds
					sToolTip = StelUtils::decDegToDmsStr(wdsSep / 3600.);
				}
				else if (celTypeId == 171 || celTypeId == 173 || celTypeId == 174) // variable stars
				{
					if (star.value(obj) > 0.f)
						extra = QString::number(star.value(obj), 'f', 5); // days
					else
						extra = dash;
				}
				else	// stars with high proper motion
					extra = QString::number(star.value(obj), 'f', 5); // "/yr

				QString sTransit = dash;
				QString sMaxElevation = dash;
				rts = obj->getRTSTime(core);
				if (rts[1]>=0.)
				{
					sTransit = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(rts[1]+utcShift), true);
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

				fillCelestialPositionTable(commonName, coordStrings.first, coordStrings.second, obj->getVMagnitudeWithExtinction(core), dash, "", extra, sToolTip, sTransit, sMaxElevation, elongStr, obj->getObjectTypeI18n());
			}
		}
		ui->celestialPositionsTreeWidget->hideColumn(CColumnAngularSize);
	}

	adjustCelestialPositionsColumns();
	// sort-by-name
	ui->celestialPositionsTreeWidget->sortItems(CColumnName, Qt::AscendingOrder);
}

QPair<QString, QString> AstroCalcDialog::getStringCoordinates(const Vec3d &coord, const bool horizontal, const bool southAzimuth, const bool decimalDegrees)
{
	double lng, lat;
	QString lngStr, latStr;
	StelUtils::rectToSphe(&lng, &lat, coord);
	if (horizontal)
	{
		// Azimuth is counted in reverse sense. N is zero, E is 90 degrees
		lng = (southAzimuth ? 2. : 3.) * M_PI - lng;
		if (lng > M_PI * 2)
			lng -= M_PI * 2;
		if (decimalDegrees)
		{
			lngStr = StelUtils::radToDecDegStr(lng, 5, false, true);
			latStr = StelUtils::radToDecDegStr(lat, 5);
		}
		else
		{
			lngStr = StelUtils::radToDmsStr(lng, true);
			latStr = StelUtils::radToDmsStr(lat, true);
		}
	}
	else
	{
		if (decimalDegrees)
		{
			lngStr = StelUtils::radToDecDegStr(lng, 5, false, true);
			latStr = StelUtils::radToDecDegStr(lat, 5);
		}
		else
		{
			lngStr = StelUtils::radToHmsStr(lng);
			latStr = StelUtils::radToDmsStr(lat, true);
		}
	}

	return QPair<QString, QString> (lngStr, latStr);
}

void AstroCalcDialog::saveCelestialPositions()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save celestial positions of objects as..."), "positions");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->celestialPositionsTreeWidget, positionsHeader);
	else
		saveTableAsXLSX(fileData.first, ui->celestialPositionsTreeWidget, positionsHeader, q_("Celestial positions of objects"), ui->celestialCategoryComboBox->currentData(Qt::DisplayRole).toString(), ui->celestialPositionsTimeLabel->text());
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

QString AstroCalcDialog::getSelectedObjectNameI18n(StelObjectP selectedObject)
{
	QString name = selectedObject->getNameI18n();
	if (name.isEmpty())
	{
		QString otype = selectedObject->getType();
		if (otype == "Nebula")
		{
			name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
			if (name.isEmpty())
				name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignationWIC();
		}
		else if (otype == "Star" || otype=="Pulsar")
			name = (selectedObject->getID().isEmpty() ? q_("Unnamed star") : selectedObject->getID());
	}
	return name;
}

void AstroCalcDialog::fillHECPositionTable(const QString &objectName, const QChar objectSymbol, const QString &latitude, const QString &longitude, const double distance)
{
	AHECPosTreeWidgetItem* treeItem = new AHECPosTreeWidgetItem(ui->hecPositionsTreeWidget);
	treeItem->setText(HECColumnName, objectName);
	treeItem->setTextAlignment(HECColumnName, Qt::AlignLeft);
	treeItem->setText(HECColumnSymbol, objectSymbol);
	treeItem->setTextAlignment(HECColumnSymbol, Qt::AlignHCenter);
	treeItem->setText(HECColumnLatitude, latitude);
	treeItem->setTextAlignment(HECColumnLatitude, Qt::AlignRight);
	treeItem->setText(HECColumnLongitude, longitude);
	treeItem->setTextAlignment(HECColumnLongitude, Qt::AlignRight);
	treeItem->setText(HECColumnDistance, QString("%1 %2").arg(QString::number(distance, 'f', 2), qc_("AU", "distance, astronomical unit")));
	treeItem->setData(HECColumnDistance, Qt::UserRole, distance);
	treeItem->setTextAlignment(HECColumnDistance, Qt::AlignRight);
	treeItem->setToolTip(HECColumnDistance, q_("Distance from the Sun at the moment of computation of position"));
}

void AstroCalcDialog::saveHECFlagMinorPlanets(bool b)
{
	conf->setValue("astrocalc/flag_hec_minor_planets", b);
	currentHECPositions();
}

void AstroCalcDialog::saveHECFlagBrightComets(bool b)
{
	conf->setValue("astrocalc/flag_hec_bright_comets", b);
	ui->hecMagnitudeLimitSpinBox->setEnabled(b);
	currentHECPositions();
}

void AstroCalcDialog::saveHECBrightCometMagnitudeLimit(double mag)
{
	conf->setValue("astrocalc/hec_magnitude_limit", QString::number(mag, 'f', 2));
	currentHECPositions();
}

void AstroCalcDialog::currentHECPositions()
{
	QPair<QString, QString> coordStrings;
	hecObjects.clear();
	initListHECPositions();
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	const bool minorPlanets = ui->hecSelectedMinorPlanetsCheckBox->isChecked();
	const bool brightComets = ui->hecBrightCometsCheckBox->isChecked();
	const double magLimit = ui->hecMagnitudeLimitSpinBox->value();

	QMap<QString, QChar> symbol = {
		{ "Mercury", QChar(0x263F) }, { "Venus",   QChar(0x2640) }, { "Earth",   QChar(0x2641) },
		{ "Mars",    QChar(0x2642) }, { "Jupiter", QChar(0x2643) }, { "Saturn",  QChar(0x2644) },
		{ "Uranus",  QChar(0x2645) }, { "Neptune", QChar(0x2646) }, { "Pluto",   QChar(0x2647) }
	};

	HECPosition object;
	const double JD = core->getJD();
	ui->hecPositionsTimeLabel->setText(q_("Positions on %1").arg(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))));

	QList<PlanetP> planets;
	QList<PlanetP> allplanets = solarSystem->getAllPlanets();
	planets.clear();
	for (const auto& planet : qAsConst(allplanets))
	{
		if (planet->getPlanetType() == Planet::isPlanet)
			planets.append(planet);
		if (brightComets && planet->getPlanetType() == Planet::isComet && planet->getVMagnitude(core)<=magLimit)
		{
			planets.append(planet);
			symbol.insert(planet->getEnglishName(), QChar(0x2604));
		}
	}
	if (minorPlanets)
		planets.append(getSelectedMinorPlanets());

	for (const auto& planet : qAsConst(planets))
	{
		if (!planet.isNull())
		{
			Vec3d pos = planet->getHeliocentricEclipticPos();
			double distance = pos.norm();
			double longitude, latitude;
			StelUtils::rectToSphe(&longitude, &latitude, pos);
			if (longitude<0) longitude+=2.0*M_PI;
			double dl=longitude*M_180_PI;
			if (withDecimalDegree)
			{
				coordStrings.first  = StelUtils::radToDecDegStr(latitude);
				coordStrings.second = StelUtils::radToDecDegStr(longitude);
			}
			else
			{
				coordStrings.first  = StelUtils::radToDmsStr(latitude, true);
				coordStrings.second = StelUtils::radToDmsStr(longitude, true);
			}

			fillHECPositionTable(planet->getNameI18n(), symbol.value(planet->getCommonEnglishName(), QChar(0x200B)), coordStrings.first, coordStrings.second, distance);
			object.objectName = planet->getNameI18n();
			object.angle = 360.-dl;
			object.dist = log(distance);
			hecObjects.append(object);
		}
	}

	adjustHECPositionsColumns();
	// sort-by-distance
	ui->hecPositionsTreeWidget->sortItems(HECColumnDistance, Qt::AscendingOrder);

	drawHECGraph();
}

void AstroCalcDialog::drawHECGraph(const QString &selectedObject)
{
	QScatterSeries *seriesPlanets = new QScatterSeries();
	QScatterSeries *seriesSelectedPlanet = new QScatterSeries();
	QScatterSeries *seriesSun = new QScatterSeries();
	seriesSun->append(0., -1.5);
	const bool minorPlanets = ui->hecSelectedMinorPlanetsCheckBox->isChecked();

	for (const auto& planet : qAsConst(hecObjects))
	{
		seriesPlanets->append(planet.angle, planet.dist);
		if (!selectedObject.isEmpty() && planet.objectName==selectedObject)
			seriesSelectedPlanet->append(planet.angle, planet.dist);
	}

	QColor axisColor(Qt::lightGray);
	QColor labelColor(Qt::white);

	QPolarChart *chart = new QPolarChart();
	chart->addSeries(seriesPlanets);
	chart->addSeries(seriesSelectedPlanet);
	chart->addSeries(seriesSun);
	chart->legend()->hide();
	chart->setMargins(QMargins(0, 0, 0, 0));
	chart->setBackgroundVisible(false);

	// QPolarChart only has clockwise counting.
	// We prevent a view from below with a manually labeled category axis.
	QCategoryAxis *angularAxis = new QCategoryAxis();
	angularAxis->setTickCount(13); // First and last ticks are co-located on 0/360 angle (30 degrees per tick).
	angularAxis->append("330&deg;", 30);
	angularAxis->append("300&deg;", 60);
	angularAxis->append("270&deg;", 90);
	angularAxis->append("240&deg;", 120);
	angularAxis->append("210&deg;", 150);
	angularAxis->append("180&deg;", 180);
	angularAxis->append("150&deg;", 210);
	angularAxis->append("120&deg;", 240);
	angularAxis->append("90&deg;", 270);
	angularAxis->append("60&deg;", 300);
	angularAxis->append("30&deg;", 330);
	angularAxis->append("0&deg;", 360);
	angularAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
	angularAxis->setLabelsColor(labelColor);
	angularAxis->setGridLineColor(axisColor);
	angularAxis->setRange(0, 360);
	chart->addAxis(angularAxis, QPolarChart::PolarOrientationAngular);

	// The QLogValueAxis has a stupid limit for the low end. Again, circumvent it with a CategoryAxis.
	QCategoryAxis *radialAxis = new QCategoryAxis();
	radialAxis->append("0.5", log(0.5));  // a few stop marks for AU values
	radialAxis->append("1", log(1.0));
	radialAxis->append("2", log(2.0));
	radialAxis->append("5", log(5.0));
	radialAxis->append("10", log(10.0));
	radialAxis->append("20", log(20.0));
	if (minorPlanets)
		radialAxis->append("40", log(40.0));
	else
		radialAxis->append("30", log(30.0));
	radialAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
	radialAxis->setLabelsColor(labelColor);
	radialAxis->setGridLineColor(axisColor);
	radialAxis->setLineVisible(false);
	if (minorPlanets)
		radialAxis->setRange(-1.5, log(52.0));
	else
		radialAxis->setRange(-1.5, log(32.0));
	chart->addAxis(radialAxis, QPolarChart::PolarOrientationRadial);

	seriesPlanets->attachAxis(angularAxis);
	seriesPlanets->attachAxis(radialAxis);
	seriesPlanets->setMarkerSize(5);
	seriesPlanets->setColor(Qt::cyan);
	seriesPlanets->setBorderColor(Qt::transparent);

	seriesSelectedPlanet->attachAxis(angularAxis);
	seriesSelectedPlanet->attachAxis(radialAxis);
	seriesSelectedPlanet->setMarkerSize(9);
	seriesSelectedPlanet->setColor(Qt::green);
	seriesSelectedPlanet->setBorderColor(Qt::transparent);

	seriesSun->attachAxis(angularAxis);
	seriesSun->attachAxis(radialAxis);
	seriesSun->setMarkerSize(9);
	seriesSun->setColor(Qt::yellow);
	seriesSun->setBorderColor(Qt::red);

	QChart* oldChart=ui->hecPositionsChartView->chart();

	ui->hecPositionsChartView->setChart(chart);
	if (oldChart) delete oldChart;
}

void AstroCalcDialog::saveHECPositions()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save celestial positions of objects as..."), "positions");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->hecPositionsTreeWidget, hecPositionsHeader);
	else
		saveTableAsXLSX(fileData.first, ui->hecPositionsTreeWidget, hecPositionsHeader, q_("Heliocentric ecliptic positions of the major planets"), q_("Major planets"), ui->hecPositionsTimeLabel->text());
}

void AstroCalcDialog::selectCurrentHECPosition(const QModelIndex& modelIndex)
{
	// Find the object
	QString nameI18n = modelIndex.sibling(modelIndex.row(), HECColumnName).data().toString();
	if (nameI18n==core->getCurrentPlanet()->getNameI18n())
		return;
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

void AstroCalcDialog::markCurrentHECPosition(const QModelIndex& modelIndex)
{
	drawHECGraph(modelIndex.sibling(modelIndex.row(), HECColumnName).data().toString());
}

void AstroCalcDialog::selectCurrentEphemeride(const QModelIndex& modelIndex)
{
	// Find the object
	const QString name = modelIndex.sibling(modelIndex.row(), EphemerisCOName).data(Qt::UserRole).toString();
	const double JD = modelIndex.sibling(modelIndex.row(), EphemerisDate).data(Qt::UserRole).toDouble();
	goToObject(name, JD);
}

void AstroCalcDialog::setEphemerisHeaderNames()
{
	const bool horizontalCoords = ui->ephemerisHorizontalCoordinatesCheckBox->isChecked();

	ephemerisHeader.clear();
	ephemerisHeader << q_("Name");
	ephemerisHeader << q_("Date and Time");	
	if (horizontalCoords)
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
	ui->ephemerisTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
}

void AstroCalcDialog::reGenerateEphemeris()
{
	reGenerateEphemeris(true);
}

void AstroCalcDialog::setDateTimeNow()
{
	const double JD = core->getJD() + core->getUTCOffset(core->getJD()) / 24;
	ui->dateFromDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(JD, Qt::LocalTime));
}

void AstroCalcDialog::saveIgnoreDateTestFlag(bool b)
{
	conf->setValue("astrocalc/flag_ephemeris_ignore_date_test", b);
	reGenerateEphemeris(true);
}

void AstroCalcDialog::reGenerateEphemeris(bool withSelection)
{
	if (!EphemerisList.isEmpty())
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
	const QString currentPlanet = ui->celestialBodyComboBox->currentData(Qt::UserRole).toString();
	const QString secondaryPlanet = ui->secondaryCelestialBodyComboBox->currentData(Qt::UserRole).toString();
	const QString distanceInfo = (core->getUseTopocentricCoordinates() ? q_("Topocentric distance") : q_("Planetocentric distance"));
	const QString distanceUM = qc_("AU", "distance, astronomical unit");
	QString englishName, nameI18n, elongStr = "", phaseStr = "";
	const bool useHorizontalCoords = ui->ephemerisHorizontalCoordinatesCheckBox->isChecked();
	const bool ignoreDateTest = ui->ephemerisIgnoreDateTestCheckBox->isChecked();
	const bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

	DisplayedPositionIndex = -1; // deselect an ephemeris marker
	initListEphemeris();

	if (currentPlanet.isEmpty()) // avoid crash
		return;

	int idxRow = 0, colorIndex = 0;
	double solarDay = 1.0, siderealDay = 1.0, siderealYear = 365.256363004; // days
	const PlanetP& cplanet = core->getCurrentPlanet();
	const PlanetP& sun = solarSystem->getSun();
	if (!cplanet->getEnglishName().contains("observer", Qt::CaseInsensitive))
	{
		if (cplanet==solarSystem->getEarth())
			solarDay = 1.0; // Special case: OK, it's Earth, let's use standard duration of the solar day
		else
			solarDay = cplanet->getMeanSolarDay();
		siderealDay = cplanet->getSiderealDay();
		siderealYear = cplanet->getSiderealPeriod();
	}
	const QMap<int, double>timeStepMap = {
		{ 0, getCustomTimeStep() },		// custom time step
		{ 1, 10. * StelCore::JD_MINUTE },
		{ 2, 30. * StelCore::JD_MINUTE },
		{ 3, StelCore::JD_HOUR },
		{ 4, 6. * StelCore::JD_HOUR },
		{ 5, 12. * StelCore::JD_HOUR },
		{ 6, solarDay },
		{ 7, 5. * solarDay },
		{ 8, 10. * solarDay },
		{ 9, 15. * solarDay },
		{10, 30. * solarDay },
		{11, 60. * solarDay },
		{12, StelCore::JD_DAY },
		{13, 5. * StelCore::JD_DAY },
		{14, 10. * StelCore::JD_DAY },
		{15, 15. * StelCore::JD_DAY },
		{16, 30. * StelCore::JD_DAY },
		{17, 60. * StelCore::JD_DAY },
		{18, siderealDay },
		{19, 5. * siderealDay },
		{20, 10. * siderealDay },
		{21, 15. * siderealDay },
		{22, 30. * siderealDay },
		{23, 60. * siderealDay },
		{24, 100. * solarDay },
		{25, 100. * siderealDay },
		{26, 100. * StelCore::JD_DAY },
		{27, siderealYear*solarDay },
		{28, 365.25*solarDay },			// 1 Julian year
		{29, 365.2568983*solarDay },		// 1 Gaussian year
		{30, 29.530588853*solarDay },	// 1 synodic month
		{31, 27.212220817*solarDay },	// 1 draconic month
		{32, 27.321582241*solarDay },	// 1 mean tropical month
		{33, 27.554549878*solarDay },	// 1 anomalistic month
		{34, 365.259636*solarDay },		// 1 anomalistic year
		{35, 6585.321314219*solarDay },	// 1 saros (223 synodic months)
		{36, 500. * siderealDay },
		{37, 500. * solarDay },
		{38, StelCore::JD_MINUTE }
	};
	double currentStep = timeStepMap.value(ui->ephemerisStepComboBox->currentData().toInt(), solarDay);

	const double currentJD = core->getJD(); // save current JD
	double firstJD = StelUtils::qDateTimeToJd(ui->dateFromDateTimeEdit->dateTime());	
	firstJD -= core->getUTCOffset(firstJD) / 24.;
	double secondJD = StelUtils::qDateTimeToJd(ui->dateToDateTimeEdit->dateTime());	
	secondJD -= core->getUTCOffset(secondJD) / 24.;
	const int elements = static_cast<int>((secondJD - firstJD) / currentStep);
	EphemerisList.clear();
	const bool allNakedEyePlanets = (ui->allNakedEyePlanetsCheckBox->isChecked() && cplanet==solarSystem->getEarth());

	QVector<PlanetP> celestialObjects;
	Q_ASSERT(celestialObjects.isEmpty());

	int n = 1;
	if (allNakedEyePlanets)
	{
		const QStringList planets = { "Mercury", "Venus", "Mars", "Jupiter", "Saturn"};
		for (auto &planet: planets)
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

	for (auto &obj: celestialObjects)
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
			colorIndex = (secondaryPlanet==englishName ? 1 : 0);
		}

		if (obj == solarSystem->getSun())
		{
			phaseStr = dash;
			elongStr = dash;
		}

		for (int i = 0; i <= elements; i++)
		{
			Vec3d pos, sunPos;
			double JD = firstJD + i * currentStep;
			core->setJD(JD);
			core->update(0); // force update to get new coordinates

			if (!ignoreDateTest && !obj->hasValidPositionalData(JD, Planet::PositionQuality::OrbitPlotting))
				continue;

			if (useHorizontalCoords)
			{
				pos = obj->getAltAzPosAuto(core);
				sunPos = sun->getAltAzPosAuto(core);
			}
			else
			{
				pos = obj->getJ2000EquatorialPos(core);
				sunPos = sun->getJ2000EquatorialPos(core);
			}
			QPair<QString, QString> coordStrings = getStringCoordinates(pos, useHorizontalCoords, useSouthAzimuth, withDecimalDegree);

			Ephemeris item;
			item.coord = pos;
			item.sunCoord = sunPos;
			item.colorIndex = colorIndex;			
			item.objDate = JD;
			item.magnitude = obj->getVMagnitudeWithExtinction(core);
			item.isComet = obj->getPlanetType()==Planet::isComet;
			EphemerisList.append(item);

			Vec3d observerHelioPos = core->getObserverHeliocentricEclipticPos();
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
			treeItem->setText(EphemerisRA, coordStrings.first);
			treeItem->setData(EphemerisRA, Qt::UserRole, idxRow);
			treeItem->setTextAlignment(EphemerisRA, Qt::AlignRight);
			treeItem->setText(EphemerisDec, coordStrings.second);
			treeItem->setTextAlignment(EphemerisDec, Qt::AlignRight);
			treeItem->setText(EphemerisMagnitude, QString::number(obj->getVMagnitudeWithExtinction(core), 'f', 2));
			treeItem->setTextAlignment(EphemerisMagnitude, Qt::AlignRight);
			treeItem->setText(EphemerisPhase, phaseStr);
			treeItem->setTextAlignment(EphemerisPhase, Qt::AlignRight);
			treeItem->setText(EphemerisDistance, QString::number(obj->getJ2000EquatorialPos(core).norm(), 'f', 6));
			treeItem->setTextAlignment(EphemerisDistance, Qt::AlignRight);
			treeItem->setToolTip(EphemerisDistance, QString("%1, %2").arg(distanceInfo, distanceUM));
			treeItem->setText(EphemerisElongation, elongStr.replace("+","",Qt::CaseInsensitive)); // remove sign
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
	enableEphemerisButtons(true);

	emit solarSystem->requestEphemerisVisualization();
}

double AstroCalcDialog::getCustomTimeStep()
{
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
	const double timeStep = conf->value("astrocalc/custom_time_step", 1.0).toDouble();
	// NOTE: Sync units with AstroCalcCustomStepsDialog::populateUnitMeasurementsList()!
	const int customTimeStepKey=conf->value("astrocalc/custom_time_step_unit", 3).toInt();
	const QMap<int, double>customTimeStepMap={
			{ 1, StelCore::JD_MINUTE},	// minutes
			{ 2, StelCore::JD_HOUR},	// hours
			{ 3, solarDay},			// solar days
			{ 4, siderealDay},		// sidereal days
			{ 5, StelCore::JD_DAY},		// Julian days
			{ 6, 29.530588853*solarDay},	// synodic months
			{ 7, 27.212220817*solarDay},	// draconic months
			{ 8, 27.321582241*solarDay},	// mean tropical months
			{ 9, 27.554549878*solarDay},	// anomalistic months
			{10, siderealYear},		// sidereal years
			{11, 365.25*solarDay},		// Julian years
			{12, 365.2568983*solarDay},	// Gaussian years
			{13, 365.259636*solarDay},	// Anomalistic years
			{14, 6585.321314219*solarDay}};	// 1 saros (223 synodic months)
	return timeStep*customTimeStepMap.value(customTimeStepKey);
}

void AstroCalcDialog::saveEphemeris()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save calculated ephemeris as..."), "ephemeris");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->ephemerisTreeWidget, ephemerisHeader);
	else
		saveTableAsXLSX(fileData.first, ui->ephemerisTreeWidget, ephemerisHeader, q_("Ephemeris"), ui->celestialBodyComboBox->currentData(Qt::DisplayRole).toString());
}

void AstroCalcDialog::cleanupEphemeris()
{
	EphemerisList.clear();
	ui->ephemerisTreeWidget->clear();
	enableEphemerisButtons(false);
}

void AstroCalcDialog::setRTSHeaderNames()
{
	rtsHeader = QStringList({
		q_("Name"),
		qc_("Rise", "celestial event"),
		qc_("Transit", "celestial event; passage across a meridian"),
		qc_("Set", "celestial event"),
		// TRANSLATORS: altitude
		q_("Altitude"),
		// TRANSLATORS: magnitude
		q_("Mag."),
		q_("Solar Elongation"),
		q_("Lunar Elongation")});
	ui->rtsTreeWidget->setHeaderLabels(rtsHeader);

	// adjust the column width
	for (int i = 0; i < RTSCount; ++i)
	{
		ui->rtsTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListRTS()
{
	ui->rtsTreeWidget->clear();
	ui->rtsTreeWidget->setColumnCount(RTSCount);
	setRTSHeaderNames();
	ui->rtsTreeWidget->header()->setSectionsMovable(false);
	ui->rtsTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
}

void AstroCalcDialog::generateRTS()
{
	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();
	if (!selectedObjects.isEmpty())
	{
		QString name, englishName;
		StelObjectP selectedObject = selectedObjects[0];
		name = ui->rtsCelestialBodyNameLabel->text();
		selectedObject->getEnglishName().isEmpty() ? englishName = name : englishName = selectedObject->getEnglishName();
		//const bool isPlanet = (selectedObject->getType() == "Planet");

		if (!name.isEmpty()) // OK, let's calculate!
		{
			const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

			initListRTS();

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

			const double currentJD = core->getJD();   // save current JD

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			// Note: It may even be possible to configure the time zone corrections into this call.
			double startJD = StelUtils::qDateTimeToJd(ui->rtsFromDateEdit->date().startOfDay(Qt::UTC));
			double stopJD = StelUtils::qDateTimeToJd(ui->rtsToDateEdit->date().startOfDay(Qt::UTC));
 #else
			double startJD = StelUtils::qDateTimeToJd(QDateTime(ui->rtsFromDateEdit->date()));
			double stopJD = StelUtils::qDateTimeToJd(QDateTime(ui->rtsToDateEdit->date()));
 #endif

			if (stopJD<startJD) // Stop warming atmosphere!..
				return;

			int elements = static_cast<int>((stopJD - startJD) / currentStep);
			double JD, JDn, az, alt;
			float magnitude;
			QString riseStr, setStr, transitStr, altStr, magStr, elongSStr = dash, elongLStr = dash;
			for (int i = 0; i <= elements; i++)
			{
				JD = startJD + i * currentStep;
				JDn = JD +.5 - core->getUTCOffset(JD) / 24.; // JD at noon of local date
				core->setJD(JDn); // start the calculation 
				core->update(0); // force update to get new coordinates
				Vec4d rts = selectedObject->getRTSTime(core);

				JD = rts[1];
				core->setJD(JD);
				core->update(0); // force update to get new coordinates

				StelUtils::rectToSphe(&az, &alt, selectedObject->getAltAzPosAuto(core));
				if (withDecimalDegree)
				{
					altStr = StelUtils::radToDecDegStr(alt, 5, false, true);
					elongSStr = (selectedObject==sun) ?                   dash : StelUtils::radToDecDegStr(selectedObject->getJ2000EquatorialPos(core).angle(sun->getJ2000EquatorialPos(core)), 5, false, true);
					elongLStr = (selectedObject==moon && planet==earth) ? dash : StelUtils::radToDecDegStr(selectedObject->getJ2000EquatorialPos(core).angle(moon->getJ2000EquatorialPos(core)), 5, false, true);
				}
				else
				{
					altStr = StelUtils::radToDmsStr(alt, true);
					elongSStr = (selectedObject==sun)                   ? dash : StelUtils::radToDmsStr(selectedObject->getJ2000EquatorialPos(core).angle(sun->getJ2000EquatorialPos(core)), true);
					elongLStr = (selectedObject==moon && planet==earth) ? dash : StelUtils::radToDmsStr(selectedObject->getJ2000EquatorialPos(core).angle(moon->getJ2000EquatorialPos(core)), true);
				}
				magnitude = selectedObject->getVMagnitudeWithExtinction(core);
				magStr = (magnitude > 50.f || selectedObject->getEnglishName().contains("marker", Qt::CaseInsensitive)? dash : QString::number(magnitude, 'f', 2));

				int year, month, day, currentdate;
				const double utcShift = core->getUTCOffset(rts[1]) / 24.;
				StelUtils::getDateFromJulianDay(JDn+utcShift, &year, &month, &currentdate);
				StelUtils::getDateFromJulianDay(rts[1]+utcShift, &year, &month, &day);

				if (rts[3]==20 || day != currentdate) // no transit time
				{
					transitStr = dash;
					altStr = dash;
					magStr = dash;
					elongSStr = dash;
					elongLStr = dash;
				}
				else
				{
					transitStr = QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD));
				}

				if (rts[3]==30 || rts[3]<0 || rts[3]>50) // no rise time
					riseStr = dash;
				else
					riseStr = QString("%1 %2").arg(localeMgr->getPrintableDateLocal(rts[0]), localeMgr->getPrintableTimeLocal(rts[0]));

				if (rts[3]==40 || rts[3]<0 || rts[3]>50) // no set time
					setStr = dash;
				else
					setStr = QString("%1 %2").arg(localeMgr->getPrintableDateLocal(rts[2]), localeMgr->getPrintableTimeLocal(rts[2]));

				ACRTSTreeWidgetItem* treeItem = new ACRTSTreeWidgetItem(ui->rtsTreeWidget);
				treeItem->setText(RTSCOName, name);
				treeItem->setData(RTSCOName, Qt::UserRole, englishName);
				treeItem->setText(RTSRiseDate, riseStr); // local date and time
				treeItem->setData(RTSRiseDate, Qt::UserRole, rts[0]);
				treeItem->setTextAlignment(RTSRiseDate, Qt::AlignRight);
				treeItem->setText(RTSTransitDate, transitStr); // local date and time
				treeItem->setData(RTSTransitDate, Qt::UserRole, JD);
				treeItem->setTextAlignment(RTSTransitDate, Qt::AlignRight);
				treeItem->setText(RTSSetDate, setStr); // local date and time
				treeItem->setData(RTSSetDate, Qt::UserRole, rts[2]);
				treeItem->setTextAlignment(RTSSetDate, Qt::AlignRight);
				treeItem->setText(RTSTransitAltitude, altStr);
				treeItem->setToolTip(RTSTransitAltitude, q_("Altitude of celestial object at transit"));
				treeItem->setTextAlignment(RTSTransitAltitude, Qt::AlignRight);
				treeItem->setText(RTSMagnitude, magStr);
				treeItem->setTextAlignment(RTSMagnitude, Qt::AlignRight);
				treeItem->setToolTip(RTSMagnitude, q_("Magnitude of celestial object at transit"));
				treeItem->setText(RTSElongation, elongSStr.replace("+","",Qt::CaseInsensitive)); // remove sign
				treeItem->setTextAlignment(RTSElongation, Qt::AlignRight);
				treeItem->setToolTip(RTSElongation, q_("Celestial object's angular distance from the Sun at transit"));
				treeItem->setText(RTSAngularDistance, elongLStr.replace("+","",Qt::CaseInsensitive)); // remove sign
				treeItem->setTextAlignment(RTSAngularDistance, Qt::AlignRight);
				treeItem->setToolTip(RTSAngularDistance, q_("Celestial object's angular distance from the Moon at transit"));
			}
			core->setJD(currentJD);

			// adjust the column width
			for (int i = 0; i < RTSCount; ++i)
			{
				ui->rtsTreeWidget->resizeColumnToContents(i);
			}

			// sort-by-date
			ui->rtsTreeWidget->sortItems(RTSTransitDate, Qt::AscendingOrder);
			enableRTSButtons(true);
		}
		else
			cleanupRTS();
	}
}

void AstroCalcDialog::cleanupRTS()
{
	ui->rtsTreeWidget->clear();
	enableRTSButtons(false);
}

void AstroCalcDialog::selectCurrentRTS(const QModelIndex& modelIndex)
{
	// Find the object
	const QString name = modelIndex.sibling(modelIndex.row(), RTSCOName).data(Qt::UserRole).toString();
	const double JD = modelIndex.sibling(modelIndex.row(), RTSTransitDate).data(Qt::UserRole).toDouble();
	goToObject(name, JD);
}

void AstroCalcDialog::setRTSCelestialBodyName()
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
			{
				name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
				if (name.isEmpty())
					name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignationWIC();
			}
			if (otype == "Star" || otype=="Pulsar")
				name = selectedObject->getID();
		}
		if (selectedObject->getType()=="Satellite")
			name = QString();
	}
	ui->rtsCelestialBodyNameLabel->setText(name);
}

void AstroCalcDialog::saveRTS()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save calculated data as..."), "RTS");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->rtsTreeWidget, rtsHeader);
	else
		saveTableAsXLSX(fileData.first, ui->rtsTreeWidget, rtsHeader, q_("Risings, Transits, and Settings"), ui->rtsCelestialBodyNameLabel->text());
}

void AstroCalcDialog::setLunarEclipseHeaderNames()
{
	lunareclipseHeader = QStringList({
		q_("Date and Time"),
		q_("Saros"),
		q_("Type"),
		q_("Gamma"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Penumbral eclipse magnitude", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Umbral eclipse magnitude", "column name"),
		// TRANSLATORS: Visibility conditions; the name of column in AstroCalc/Eclipses tool
		qc_("Vis. Cond.", "column name")});
	ui->lunareclipseTreeWidget->setHeaderLabels(lunareclipseHeader);

	// adjust the column width
	for (int i = 0; i < LunarEclipseCount; ++i)
	{
		ui->lunareclipseTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::setLunarEclipseContactsHeaderNames()
{
	lunareclipsecontactsHeader = QStringList({
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Circumstances", "column name"),
		q_("Date and Time"),
		q_("Altitude"),
		q_("Azimuth"),
		q_("Latitude"),
		q_("Longitude"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Position Angle", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Axis Distance", "column name")});
	ui->lunareclipsecontactsTreeWidget->setHeaderLabels(lunareclipsecontactsHeader);

	// adjust the column width
	for (int i = 0; i < LunarEclipseContactCount; ++i)
	{
		ui->lunareclipsecontactsTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListLunarEclipse()
{
	ui->lunareclipseTreeWidget->clear();
	ui->lunareclipseTreeWidget->setColumnCount(LunarEclipseCount);
	setLunarEclipseHeaderNames();
	ui->lunareclipseTreeWidget->header()->setSectionsMovable(false);
	ui->lunareclipseTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
	initListLunarEclipseContact();
}

void AstroCalcDialog::initListLunarEclipseContact()
{
	ui->lunareclipsecontactsTreeWidget->clear();
	ui->lunareclipsecontactsTreeWidget->setColumnCount(LunarEclipseContactCount);
	setLunarEclipseContactsHeaderNames();
	ui->lunareclipsecontactsTreeWidget->header()->setSectionsMovable(false);
	ui->lunareclipsecontactsTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
}

LunarEclipseBessel::LunarEclipseBessel(double &besX, double &besY, double &besL1, double &besL2, double &besL3, double &latDeg, double &lngDeg)
{
	// Besselian elements
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	// Algorithm adapted from Planet::getLunarEclipseMagnitudes()

	StelCore* core = StelApp::getInstance().getCore();
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	PlanetP moon = ssystem->getMoon();
	// Use geocentric coordinates
	core->setUseTopocentricCoordinates(false);
	core->update(0);

	double raMoon, deMoon, raSun, deSun;
	StelUtils::rectToSphe(&raSun, &deSun, ssystem->getSun()->getEquinoxEquatorialPos(core));
	StelUtils::rectToSphe(&raMoon, &deMoon, moon->getEquinoxEquatorialPos(core));

	// R.A./Dec of Earth's shadow
	const double raShadow = StelUtils::fmodpos(raSun+M_PI, 2.*M_PI);
	const double deShadow = -(deSun);
	const double raDiff = StelUtils::fmodpos(raMoon-raShadow, 2.*M_PI);
	besX = std::cos(deMoon)*std::sin(raDiff)*3600.*M_180_PI;
	besY = (std::cos(deShadow)*std::sin(deMoon)-std::sin(deShadow)*std::cos(deMoon)*std::cos(raDiff))*3600.* M_180_PI;
	const double dist=moon->getEclipticPos().norm();  // geocentric Lunar distance [AU]
	const double mSD=atan(moon->getEquatorialRadius()/dist)*M_180_PI*3600.; // arcsec
	const QPair<Vec3d,Vec3d>shadowRadii=ssystem->getEarthShadowRadiiAtLunarDistance();
	const double f1 = shadowRadii.second[0]; // radius of penumbra at the distance of the Moon
	const double f2 = shadowRadii.first[0];  // radius of umbra at the distance of the Moon
	besL1 = f1+mSD; // distance between center of the Moon and shadow at beginning and end of penumbral eclipse
	besL2 = f2+mSD; // distance between center of the Moon and shadow at beginning and end of partial eclipse
	besL3 = f2-mSD; // distance between center of the Moon and shadow at beginning and end of total eclipse

	// Sublunar point
	const double gast = get_apparent_sidereal_time(core->getJD(), core->getJDE());
	latDeg = deMoon*M_180_PI;
	lngDeg = StelUtils::fmodpos((raMoon*M_180_PI)-gast, 360.);
	if (lngDeg>180.) lngDeg-=360.;
};

// Lunar eclipse parameters
struct LunarEclipseParameters {
	double dt;
	double positionAngle;
	double axisDistance;
};

LunarEclipseParameters lunarEclipseContacts(double JD, bool beforeMaximum, int eclipseType) {
	LunarEclipseParameters result;

	StelCore* core = StelApp::getInstance().getCore();
	core->setJD(JD);
	core->update(0);
	double x,y,L1,L2,L3,latitude,longitude,L=0.;
	LunarEclipseBessel(x,y,L1,L2,L3,latitude,longitude);
	switch (eclipseType)
	{
		case 0: // penumbral
			L = L1;
			break;
		case 1: // partial
			L = L2;
			break;
		case 2: // total
			L = L3;
			break;
	}

	core->setJD(JD - 5./1440.);
	core->update(0);
	double x1,y1;
	LunarEclipseBessel(x1,y1,L1,L2,L3,latitude,longitude);
	core->setJD(JD + 5./1440.);
	core->update(0);
	double x2,y2;
	LunarEclipseBessel(x2,y2,L1,L2,L3,latitude,longitude);

	const double xdot = (x2 - x1) * 6.;
	const double ydot = (y2 - y1) * 6.;
	const double n2 = xdot*xdot+ydot*ydot;
	const double delta = (x*ydot-y*xdot)/sqrt(n2);
	const double semiDuration = sqrt((L*L-delta*delta)/n2);
	result.dt = -(x*xdot+y*ydot)/n2;
	if (beforeMaximum)
		result.dt-=semiDuration;
	else
		result.dt+=semiDuration;

	result.positionAngle = StelUtils::fmodpos(atan2(x, y), 2.*M_PI);
	result.axisDistance = sqrt(x*x+y*y)*M_PI_180/3600.;
	core->setJD(JD);
	core->update(0);

	return result;
}

void AstroCalcDialog::generateLunarEclipses()
{
	const bool onEarth = core->getCurrentPlanet()==solarSystem->getEarth();
	if (onEarth)
	{
		initListLunarEclipse();

		const double currentJD = core->getJD();   // save current JD
		double startyear = ui->eclipseFromYearSpinBox->value();
		double years = ui->eclipseYearsSpinBox->value();
		double startJD, stopJD;
		StelUtils::getJDFromDate(&startJD, startyear, 1, 1, 0, 0, 0);
		StelUtils::getJDFromDate(&stopJD, startyear+years, 12, 31, 23, 59, 59);
		startJD = startJD - core->getUTCOffset(startJD) / 24.;
		stopJD = stopJD - core->getUTCOffset(stopJD) / 24.;
		int elements = static_cast<int>((stopJD - startJD) / 29.530588853);
		QString sarosStr, eclipseTypeStr, uMagStr, pMagStr, gammaStr, visibilityConditionsStr, visibilityConditionsTooltip;

		const bool saveTopocentric = core->getUseTopocentricCoordinates();
		static const double approxJD = 2451550.09765;
		static const double synodicMonth = 29.530588853;

		static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
		PlanetP moon = ssystem->getMoon();
		PlanetP sun = ssystem->getSun();
		PlanetP earth = ssystem->getEarth();

		// Find approximate JD of Full Moon = Geocentric opposition in longitude
		double tmp = (startJD - approxJD - (synodicMonth * 0.5)) / synodicMonth;
		double InitJD = approxJD + int(tmp) * synodicMonth - (synodicMonth * 0.5);

		// Search for lunar eclipses at each full Moon
		for (int i = 0; i <= elements+2; i++)
		{
			double JD = InitJD + synodicMonth * i;
			if (JD > startJD)
			{
				core->setJD(JD);
				core->setUseTopocentricCoordinates(false);
				core->update(0);

				double az, alt;

				// Find exact time of closest approach between the Moon and shadow centre
				double dt = 1.;
				int iteration = 0;
				while (abs(dt)>(0.1/86400.) && (iteration < 20)) // 0.1 second of accuracy
				{
					core->setJD(JD);
					core->update(0);
					double x,y,L1,L2,L3,latitude,longitude;
					LunarEclipseBessel(x,y,L1,L2,L3,latitude,longitude);
					core->setJD(JD - 5./1440.);
					core->update(0);
					double x1,y1;
					LunarEclipseBessel(x1,y1,L1,L2,L3,latitude,longitude);
					core->setJD(JD + 5./1440.);
					core->update(0);
					double x2,y2;
					LunarEclipseBessel(x2,y2,L1,L2,L3,latitude,longitude);
					double xdot = (x2 - x1) * 6.;
					double ydot = (y2 - y1) * 6.;
					double n2 = xdot * xdot + ydot * ydot;
					dt  = -(x * xdot + y * ydot) / n2;
					JD += dt / 24.;
					iteration += 1;
				}

				core->setJD(JD);
				core->update(0);

				// Check for eclipse
				// Source: Explanatory Supplement to the Astronomical Ephemeris 
				// and the American Ephemeris and Nautical Almanac (1961)
				// Algorithm taken from Planet::getLunarEclipseMagnitudes()

				const double dist=moon->getEclipticPos().norm();  // geocentric Lunar distance [AU]
				const double mSD=atan(moon->getEquatorialRadius()/dist) * M_180_PI*3600.; // arcsec
				double x,y,L1,L2,L3,latitude,longitude;
				LunarEclipseBessel(x,y,L1,L2,L3,latitude,longitude);
				const double m = sqrt(x * x + y * y); // distance between lunar centre and shadow centre
				const double pMag = (L1 - m) / (2. * mSD); // penumbral magnitude
				const double uMag = (L2 - m) / (2. * mSD); // umbral magnitude
				
				if (pMag>0.)
				{
					if (uMag>=1.)
						eclipseTypeStr = qc_("Total", "eclipse type");
					else if (uMag>0.)
						eclipseTypeStr = qc_("Partial", "eclipse type");
					else
						eclipseTypeStr = qc_("Penumbral", "eclipse type");

					// Visibility conditions / Elevation of the Moon at max. phase of eclipse
					StelUtils::rectToSphe(&az, &alt, moon->getAltAzPosAuto(core));
					double altitude=alt*M_180_PI;

					if (altitude >= 45.) // Perfect conditions - 45+ degrees
					{
						visibilityConditionsStr = qc_("Perfect", "visibility conditions");
						visibilityConditionsTooltip = q_("Perfect visibility conditions for current location");
					}
					else if (altitude >= 30.) // "Photometric altitude" - 30+ degrees
					{
						visibilityConditionsStr = qc_("Good", "visibility conditions");
						visibilityConditionsTooltip = q_("Good visibility conditions for current location");
					}
					else
					{
						visibilityConditionsStr = qc_("Bad", "visibility conditions");
						visibilityConditionsTooltip = q_("Bad visibility conditions for current location");
					}

					// Our rule of thumb is that a partial penumbral eclipse is detectable with
					// the unaided eye if penumbral magnitude>0.7
					if (uMag < 1.0 && pMag < 0.7)
					{
						// TRANSLATORS: Not obs. = Not observable
						visibilityConditionsStr = qc_("Not obs.", "visibility conditions");
						visibilityConditionsTooltip = q_("Not observable eclipse");
					}

					if (altitude<0.)
					{
						visibilityConditionsStr = qc_("Invisible", "visibility conditions");
						visibilityConditionsTooltip = q_("The greatest eclipse is invisible in current location");
					}

					// Saros series calculations - useful to search for eclipses in the same Saros
					// Adapted from Saros calculations for solar eclipses in Sky & Telescope (October 1985)
					// Saros numbers calculated here are matching well with NASA's Five Millennium Catalog of Lunar Eclipses

					// ln = Brown Lunation number : = 1 at the first New Moon of 1923
					double q = round((JD-2423436.40347)/29.530588-0.25);
					int ln = int(q) + 1 - 953;
					int nd = ln + 105;
					int s = 148 + 38 * nd;
					int nx = -61 * nd;
					int nc = floor(nx / 358. + 0.5 - nd / (12. * 358 * 358));
					int saros = 1 + ((s + nc * 223 - 1) % 223);
					if ((s + nc * 223 - 1) < 0) saros -= 223;
					if (saros < -223) saros += 223;

					// gamma = minimum distance from the center of the Moon to the axis of Earth’s umbral shadow cone
					// in units of Earth’s equatorial radius. Positive when the Moon passes north of the shadow cone axis.
					// Source: https://eclipse.gsfc.nasa.gov/5MCLE/5MCLE-Text10.pdf
					double gamma = m*0.2725076/mSD;
					if (y<0.) gamma *= -1.;

					sarosStr = QString("%1").arg(QString::number(saros));
					gammaStr = QString("%1").arg(QString::number(gamma, 'f', 3));
					pMagStr = QString("%1").arg(QString::number(pMag, 'f', 3));
					if (uMag<0.)
						uMagStr = dash;
					else
						uMagStr = QString("%1").arg(QString::number(uMag, 'f', 3));

					ACLunarEclipseTreeWidgetItem* treeItem = new ACLunarEclipseTreeWidgetItem(ui->lunareclipseTreeWidget);
					treeItem->setText(LunarEclipseDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))); // local date and time
					treeItem->setData(LunarEclipseDate, Qt::UserRole, JD);
					treeItem->setText(LunarEclipseSaros, sarosStr);
					treeItem->setToolTip(LunarEclipseSaros, q_("Saros series number of eclipse (each eclipse in a Saros is separated by an interval of 18 years 11.3 days)"));
					treeItem->setText(LunarEclipseType, eclipseTypeStr);
					treeItem->setText(LunarEclipseGamma, gammaStr);
					treeItem->setText(LunarEclipsePMag, pMagStr);
					treeItem->setToolTip(LunarEclipsePMag, q_("Penumbral magnitude is the fraction of the Moon's diameter immersed in the penumbra"));
					treeItem->setData(LunarEclipsePMag, Qt::UserRole, pMag);
					treeItem->setText(LunarEclipseUMag, uMagStr);
					treeItem->setToolTip(LunarEclipseUMag, q_("Umbral magnitude is the fraction of the Moon's diameter immersed in the umbra"));
					treeItem->setData(LunarEclipseUMag, Qt::UserRole, uMag);
					treeItem->setText(LunarEclipseVisConditions, visibilityConditionsStr);
					treeItem->setData(LunarEclipseVisConditions, Qt::UserRole, altitude);
					treeItem->setToolTip(LunarEclipseVisConditions, visibilityConditionsTooltip);
					treeItem->setTextAlignment(LunarEclipseDate, Qt::AlignRight);
					treeItem->setTextAlignment(LunarEclipseSaros, Qt::AlignRight);
					treeItem->setTextAlignment(LunarEclipseGamma, Qt::AlignRight);
					treeItem->setTextAlignment(LunarEclipsePMag, Qt::AlignRight);
					treeItem->setTextAlignment(LunarEclipseUMag, Qt::AlignRight);
				}
			}
		}
		core->setJD(currentJD);
		core->setUseTopocentricCoordinates(saveTopocentric);
		core->update(0); // enforce update cache to avoid odd selection of Moon details!

		// adjust the column width
		for (int i = 0; i < LunarEclipseCount; ++i)
		{
			ui->lunareclipseTreeWidget->resizeColumnToContents(i);
		}

		// sort-by-date
		ui->lunareclipseTreeWidget->sortItems(LunarEclipseDate, Qt::AscendingOrder);
		enableLunarEclipsesButtons(true);
		enableLunarEclipsesCircumstancesButtons(false);
	}
	else
		cleanupLunarEclipses();
}

void AstroCalcDialog::cleanupLunarEclipses()
{
	ui->lunareclipseTreeWidget->clear();
	ui->lunareclipsecontactsTreeWidget->clear();
	enableLunarEclipsesButtons(false);
	enableLunarEclipsesCircumstancesButtons(false);
}

void AstroCalcDialog::enableLunarEclipsesButtons(bool enable)
{
	ui->lunareclipsesCleanupButton->setEnabled(enable);
	ui->lunareclipsesSaveButton->setEnabled(enable);
}

void AstroCalcDialog::enableLunarEclipsesCircumstancesButtons(bool enable)
{
	ui->lunareclipsescontactsSaveButton->setEnabled(enable);
}

LunarEclipseIteration::LunarEclipseIteration(double &JD, double &positionAngle, double &axisDistance, bool beforeMaximum, int eclipseType)
{
	double dt = 1.;
	int iterations = 0;
	LunarEclipseParameters eclipseData;
	while (abs(dt) > 0.00002 && (iterations < 10))
	{
		eclipseData = lunarEclipseContacts(JD,beforeMaximum,eclipseType);
		dt = eclipseData.dt;
		JD += dt/24.;
		iterations += 1;
	}
	positionAngle = eclipseData.positionAngle;
	if (eclipseType < 2)
	{
		positionAngle -= M_PI;
		if (positionAngle < 0) positionAngle += 2.*M_PI;
	}
	axisDistance = eclipseData.axisDistance;
};

void AstroCalcDialog::selectCurrentLunarEclipseDate(const QModelIndex& modelIndex)
{
	const double JDMid = modelIndex.sibling(modelIndex.row(), LunarEclipseDate).data(Qt::UserRole).toDouble();
	goToObject("Moon", JDMid);
}

void AstroCalcDialog::selectCurrentLunarEclipse(const QModelIndex& modelIndex)
{
	initListLunarEclipseContact();
	const bool saveTopocentric = core->getUseTopocentricCoordinates();
	const double currentJD = core->getJD();
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	PlanetP moon = ssystem->getMoon();
	const bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	const double JDMid = modelIndex.sibling(modelIndex.row(), LunarEclipseDate).data(Qt::UserRole).toDouble();
	const double uMag = modelIndex.sibling(modelIndex.row(), LunarEclipseUMag).data(Qt::UserRole).toDouble();

	// Compute time and other data of contacts
	LunarEclipseParameters eclipseData;
	for (int i = 0; i < 7; i++)
	{
		double x,y,L1,L2,L3,latitude,longitude, positionAngle=0, axisDistance=0;
		bool event = false;
		double JD = JDMid;
		if (i==0)
		{
			LunarEclipseIteration(JD,positionAngle,axisDistance,true,0);
			LunarEclipseBessel(x,y,L1,L2,L3,latitude,longitude);
			event = true;
		}
		else if (i==1 && uMag>0.)
		{
			LunarEclipseIteration(JD,positionAngle,axisDistance,true,1);
			LunarEclipseBessel(x,y,L1,L2,L3,latitude,longitude);
			event = true;
		}
		else if (i==2 && uMag>=1.)
		{
			LunarEclipseIteration(JD,positionAngle,axisDistance,true,2);
			LunarEclipseBessel(x,y,L1,L2,L3,latitude,longitude);
			event = true;
		}
		else if (i==3)
		{
			eclipseData = lunarEclipseContacts(JD,true,2);
			LunarEclipseBessel(x,y,L1,L2,L3,latitude,longitude);
			positionAngle = eclipseData.positionAngle;
			axisDistance = eclipseData.axisDistance;
			event = true;
		}
		else if (i==4 && uMag>=1.)
		{
			LunarEclipseIteration(JD,positionAngle,axisDistance,false,2);
			LunarEclipseBessel(x,y,L1,L2,L3,latitude,longitude);
			event = true;
		}
		else if (i==5 && uMag>0.)
		{
			LunarEclipseIteration(JD,positionAngle,axisDistance,false,1);
			LunarEclipseBessel(x,y,L1,L2,L3,latitude,longitude);
			event = true;
		}
		else if (i==6)
		{
			LunarEclipseIteration(JD,positionAngle,axisDistance,false,0);
			LunarEclipseBessel(x,y,L1,L2,L3,latitude,longitude);
			event = true;
		}
		if (event)
		{
			ACLunarEclipseContactsTreeWidgetItem* treeItem = new ACLunarEclipseContactsTreeWidgetItem(ui->lunareclipsecontactsTreeWidget);
			QStringList events={
				q_("Moon enters penumbra"),
				q_("Moon enters umbra"),
				q_("Total eclipse begins"),
				q_("Maximum eclipse"),
				q_("Total eclipse ends"),
				q_("Moon leaves umbra"),
				q_("Moon leaves penumbra")};
			treeItem->setText(LunarEclipseContact, events.at(i));
			treeItem->setText(LunarEclipseContactDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD)));
			treeItem->setData(LunarEclipseContactDate, Qt::UserRole, JD);
			core->setJD(JD);
			core->setUseTopocentricCoordinates(saveTopocentric);
			core->update(0);
			double az, alt;
			StelUtils::rectToSphe(&az, &alt, moon->getAltAzPosAuto(core));
			QPair<QString, QString> coordStrings = getStringCoordinates(moon->getAltAzPosAuto(core), true, useSouthAzimuth, withDecimalDegree);
			QString azimuthStr = coordStrings.first;
			QString altitudeStr = coordStrings.second;
			treeItem->setText(LunarEclipseContactAltitude, altitudeStr);
			treeItem->setData(LunarEclipseContactAltitude, Qt::UserRole, alt);
			treeItem->setText(LunarEclipseContactAzimuth, azimuthStr);
			treeItem->setData(LunarEclipseContactAzimuth, Qt::UserRole, az);
			QString latitudeStr = StelUtils::decDegToLatitudeStr(latitude, !withDecimalDegree);
			QString longitudeStr = StelUtils::decDegToLongitudeStr(longitude, true, false, !withDecimalDegree);
			treeItem->setText(LunarEclipseContactLatitude, latitudeStr);
			treeItem->setData(LunarEclipseContactLatitude, Qt::UserRole, latitude);
			treeItem->setToolTip(LunarEclipseContactLatitude, q_("Geographic latitude where the Moon appears in the zenith"));
			treeItem->setText(LunarEclipseContactLongitude, longitudeStr);
			treeItem->setData(LunarEclipseContactLatitude, Qt::UserRole, longitude);
			treeItem->setToolTip(LunarEclipseContactLongitude, q_("Geographic longitude where the Moon appears in the zenith"));
			QString positionAngleStr, distanceStr;
			if (withDecimalDegree)
			{
				positionAngleStr = StelUtils::radToDecDegStr(positionAngle, 3, false, true);
				distanceStr = StelUtils::radToDecDegStr(axisDistance, 5, false, true);
			}
			else
			{
				positionAngleStr = StelUtils::radToDmsStr(positionAngle, true);
				distanceStr = StelUtils::radToDmsStr(axisDistance, true);
			}
			treeItem->setText(LunarEclipseContactPA, positionAngleStr);
			treeItem->setData(LunarEclipseContactPA, Qt::UserRole, positionAngle);
			treeItem->setToolTip(LunarEclipseContactPA, q_("Position angle of contact point with respect to center of the Moon measured counter-clockwise from celestial north"));
			treeItem->setText(LunarEclipseContactDistance, distanceStr);
			treeItem->setData(LunarEclipseContactDistance, Qt::UserRole, axisDistance);
			treeItem->setToolTip(LunarEclipseContactDistance, q_("Geocentric angular distance of center of the Moon from the axis or center of the Earth's shadow"));
			treeItem->setTextAlignment(LunarEclipseContactDate, Qt::AlignRight);
			if (alt<0.)
			{
				for (auto column : {LunarEclipseContact,         LunarEclipseContactDate,      LunarEclipseContactAltitude, LunarEclipseContactAzimuth,
						    LunarEclipseContactLatitude, LunarEclipseContactLongitude, LunarEclipseContactPA,       LunarEclipseContactDistance})
#if (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
					treeItem->setForeground(column, Qt::gray);
#else
					treeItem->setTextColor(column, Qt::gray);
#endif
			}
			treeItem->setTextAlignment(LunarEclipseContact, Qt::AlignLeft);
			for (auto column : {LunarEclipseContactDate, LunarEclipseContactAltitude, LunarEclipseContactAzimuth, LunarEclipseContactLatitude,
					    LunarEclipseContactLongitude, LunarEclipseContactPA, LunarEclipseContactDistance})
				treeItem->setTextAlignment(column, Qt::AlignRight);
		}
	}

	// adjust the column width
	for (int i = 0; i < LunarEclipseContactCount; ++i)
	{
		ui->lunareclipsecontactsTreeWidget->resizeColumnToContents(i);
	}
	core->setJD(currentJD);
	enableLunarEclipsesCircumstancesButtons(true);
}

void AstroCalcDialog::selectCurrentLunarEclipseContact(const QModelIndex& modelIndex)
{
	const double JD = modelIndex.sibling(modelIndex.row(), LunarEclipseContactDate).data(Qt::UserRole).toDouble();
	if (JD!=0)
		goToObject("Moon", JD);
}

void AstroCalcDialog::saveLunarEclipses()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save calculated lunar eclipses as..."), "lunareclipses");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->lunareclipseTreeWidget, lunareclipseHeader);
	else
		saveTableAsXLSX(fileData.first, ui->lunareclipseTreeWidget, lunareclipseHeader, q_("Lunar Eclipses"), q_("Lunar Eclipses"), q_("Note: Local circumstances for eclipses during thousands of years in the past and future are not reliable due to uncertainty in ΔT which is caused by fluctuations in Earth's rotation."));
}

void AstroCalcDialog::saveLunarEclipseCircumstances()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save lunar eclipse circumstances as..."), "lunareclipse-circumstances");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->lunareclipsecontactsTreeWidget, lunareclipsecontactsHeader);
	else
		saveTableAsXLSX(fileData.first, ui->lunareclipsecontactsTreeWidget, lunareclipsecontactsHeader, q_("Circumstances of Lunar Eclipse"), q_("Circumstances of Lunar Eclipse"), q_("Note: Local circumstances for eclipses during thousands of years in the past and future are not reliable due to uncertainty in ΔT which is caused by fluctuations in Earth's rotation."));
}

void AstroCalcDialog::setSolarEclipseHeaderNames()
{
	solareclipseHeader = QStringList({
		q_("Date and Time"),
		q_("Saros"),
		q_("Type"),
		q_("Gamma"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Eclipse Magnitude", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Latitude", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Longitude", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Altitude", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Path Width", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Central Duration", "column name")});
	ui->solareclipseTreeWidget->setHeaderLabels(solareclipseHeader);

	// adjust the column width
	for (int i = 0; i < SolarEclipseCount; ++i)
	{
		ui->solareclipseTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::setSolarEclipseContactsHeaderNames()
{
	solareclipsecontactsHeader = QStringList({
		qc_("Circumstances", "column name"),
		q_("Date and Time"),
		q_("Latitude"),
		q_("Longitude"),
		qc_("Path Width", "column name"),
		qc_("Central Duration", "column name"),
		q_("Type")});
	ui->solareclipsecontactsTreeWidget->setHeaderLabels(solareclipsecontactsHeader);

	// adjust the column width
	for (int i = 0; i < SolarEclipseContactCount; ++i)
	{
		ui->solareclipsecontactsTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListSolarEclipse()
{
	ui->solareclipseTreeWidget->clear();
	ui->solareclipseTreeWidget->setColumnCount(SolarEclipseCount);
	setSolarEclipseHeaderNames();
	ui->solareclipseTreeWidget->header()->setSectionsMovable(false);
	ui->solareclipseTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
	initListSolarEclipseContact();
}

void AstroCalcDialog::initListSolarEclipseContact()
{
	ui->solareclipsecontactsTreeWidget->clear();
	ui->solareclipsecontactsTreeWidget->setColumnCount(SolarEclipseContactCount);
	setSolarEclipseContactsHeaderNames();
	ui->solareclipsecontactsTreeWidget->header()->setSectionsMovable(false);
	ui->solareclipsecontactsTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);	
}

// Local solar eclipse parameters
struct LocalSEparams {
	double dt;
	double L1;
	double L2;
	double ce;
	double magnitude;
	double altitude;
};

LocalSEparams localSolarEclipse(double JD,int contact,bool central) {
	LocalSEparams result;
	// contact : -1 for beginning, 0 for mid-eclipse, 1 for the end of partial or annular
	// contact : -1 for the end, 0 for mid-eclipse, 1 for beginning of total
	// central : true for total/annular eclipse

	// Besselian elements
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)

	StelCore* core = StelApp::getInstance().getCore();
	const double lat = static_cast<double>(core->getCurrentLocation().getLatitude());
	const double lon = static_cast<double>(core->getCurrentLocation().getLongitude());
	const double elevation = static_cast<double>(core->getCurrentLocation().altitude);

	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	Vec4d geocentricCoords = ssystem->getEarth()->getRectangularCoordinates(lon,lat,elevation);
	static const double earthRadius = ssystem->getEarth()->getEquatorialRadius();
	const double rc = geocentricCoords[0]/earthRadius; // rhoCosPhiPrime
	const double rs = geocentricCoords[1]/earthRadius; // rhoSinPhiPrime

	core->setUseTopocentricCoordinates(false);
	core->setJD(JD);
	core->update(0);

	double xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot;
	BesselParameters(xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot,true);
	double x,y,d,tf1,tf2,L1,L2,mu;
	SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
	double theta = (mu + lon) * M_PI_180;
	theta = StelUtils::fmodpos(theta, 2.*M_PI);
	const double xi = rc*std::sin(theta);
	const double eta = rs*std::cos(d)-rc*std::sin(d)*std::cos(theta);
	const double zeta = rs*std::sin(d)+rc*std::cos(d)*std::cos(theta);
	const double xidot = mudot*rc*std::cos(theta);
	etadot = mudot*xi*std::sin(d)-zeta*ddot;
	const double u = x - xi;
	const double v = y - eta;
	const double udot = xdot - xidot;
	const double vdot = ydot - etadot;
	const double n2 = udot * udot + vdot * vdot;
	const double delta = (u*vdot - udot*v)/std::sqrt(n2);
	L1 = L1 - zeta * tf1;
	L2 = L2 - zeta * tf2;
	const double L = central ? L2 : L1;
	const double sfi = delta/L;
	const double ce = 1.- sfi*sfi;
	double cfi = 0.; 
	if (ce > 0.)
		cfi = contact * std::sqrt(ce);
	const double m = sqrt(u * u + v * v);

	result.magnitude = (L1 - m) / (L1 + L2);
	result.altitude = std::asin(rc*std::cos(d)*std::cos(theta)+rs*std::sin(d))*M_180_PI;
	result.dt = (L * cfi / std::sqrt(udot*udot + vdot*vdot)) - (u*udot + v*vdot)/n2;
	result.L1 = L1;
	result.L2 = L2;
	result.ce = ce;

	return result;
}

double AstroCalcDialog::getDeltaTimeofContact(double JD, bool beginning, bool penumbra, bool external, bool outerContact)
{
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	const int sign = outerContact ? 1 : -1; // there are outer & inner contacts
	core->setJD(JD);
	core->update(0);
	double xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot;
	BesselParameters(xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot,penumbra);
	double x,y,d,tf1,tf2,L1,L2,mu;
	SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
	const double rho1 = std::sqrt(1.-e2*std::cos(d)*std::cos(d));
	double s,dt;
	if (!penumbra)
		ydot = ydot/rho1;
	const double n = std::sqrt(xdot*xdot+ydot*ydot);
	const double y1 = y/rho1;
	const double m = std::sqrt(x*x+y*y);
	const double m1 = std::sqrt(x*x+y1*y1);
	const double rho = m/m1;
	const double L = penumbra ? L1 : L2;

	if (external)
	{
		s = (x*ydot-y*xdot)/(n*(L+sign*rho)); // shadow's limb
	}
	else
	{
		s = (x*ydot-xdot*y1)/n; // center of shadow
	}
	double cs = std::sqrt(1.-s*s);
	if (beginning)
		cs *= -1.;
	if (external)
	{
		dt = (L+sign*rho)*cs/n;
		if (outerContact)
			dt -= ((x*xdot+y*ydot)/(n*n));
		else
			dt = -((x*xdot+y*ydot)/(n*n))-dt;
	}
	else
		dt = cs/n-((x*xdot+y1*ydot)/(n*n));
	return dt;
}

double AstroCalcDialog::getJDofContact(double JD, bool beginning, bool penumbra, bool external, bool outerContact)
{
	double dt = 1.;
	int iterations = 0;
	while (std::abs(dt)>(0.1/86400.) && (iterations < 10))
	{
		dt = getDeltaTimeofContact(JD,beginning,penumbra,external,outerContact);
		JD += dt/24.;
		iterations++;
	}
	return JD;
}

double AstroCalcDialog::getJDofMinimumDistance(double JD)
{
	const double currentJD = core->getJD(); // save current JD
	double dt = 1.;
	int iterations = 0;
	double xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot;
	double x,y,d,tf1,tf2,L1,L2,mu;
	while (std::abs(dt)>(0.1/86400.) && (iterations < 20)) // 0.1 second of accuracy
	{
		core->setJD(JD);
		core->update(0);
		BesselParameters(xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot,false);
		SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
		double n2 = xdot*xdot + ydot*ydot;
		dt = -(x*xdot + y*ydot)/n2;
		JD += dt/24.;
		iterations++;
	}
	core->setJD(currentJD);
	core->update(0);
	return JD;
}

void AstroCalcDialog::generateSolarEclipses()
{
	const bool onEarth = core->getCurrentPlanet()==solarSystem->getEarth();
	if (onEarth)
	{
		initListSolarEclipse();

		const double currentJD = core->getJD();   // save current JD
		int startyear = ui->eclipseFromYearSpinBox->value();
		int years = ui->eclipseYearsSpinBox->value();
		double startJD, stopJD;
		StelUtils::getJDFromDate(&startJD, startyear, 1, 1, 0, 0, 0);
		StelUtils::getJDFromDate(&stopJD, startyear+years, 12, 31, 23, 59, 59);
		startJD = startJD - core->getUTCOffset(startJD) / 24.;
		stopJD = stopJD - core->getUTCOffset(stopJD) / 24.;
		QString sarosStr, eclipseTypeStr, gammaStr, magStr, latitudeStr, longitudeStr, altitudeStr, pathWidthStr, durationStr;
		// TRANSLATORS: Unit of measure for distance - kilometers
		QString km = qc_("km", "distance");

		const bool saveTopocentric = core->getUseTopocentricCoordinates();
		const double approxJD = 2451550.09765;
		const double synodicMonth = 29.530588853;
		int elements = static_cast<int>((stopJD - startJD) / synodicMonth);
		const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

		// Find approximate JD of New Moon = Geocentric conjunction in longitude
		// Source: Astronomical Algorithms (1991), Jean Meeus
		double tmp = (startJD - approxJD - synodicMonth) / synodicMonth;
		double InitJD = approxJD + int(tmp) * synodicMonth;

		// Search for solar eclipses at each New Moon
		for (int i = 0; i <= elements+2; i++)
		{
			double JD = InitJD + synodicMonth * i;
			if (JD > startJD)
			{
				core->setUseTopocentricCoordinates(false);
				core->update(0);

				// Find exact time of minimum distance between axis of lunar shadow cone to the center of Earth
				JD = getJDofMinimumDistance(JD);
				core->setJD(JD);
				core->update(0);

				double x,y,d,tf1,tf2,L1,L2,mu;
				SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
				double gamma = sqrt(x * x + y * y);
				if (y<0.) gamma = -(gamma);
				double dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude;
				SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);

				bool noncentraleclipse = false; // Non-central includes partial and total/annular eclipses that shadow axis miss Earth

				// Determine the type of solar eclipse
				// Source: Astronomical Algorithms (1991), Jean Meeus
				if (abs(gamma) <= (1.5433 + L2))
				{
					if (abs(gamma) > 0.9972 && abs(gamma) < (1.5433 + L2))
					{
						if (abs(gamma) < 0.9972 + abs(L2) && dRatio > 1.)
						{
							eclipseTypeStr = qc_("Total", "eclipse type"); // Non-central total eclipse
						}
						else if (abs(gamma) < 0.9972 + abs(L2) && dRatio < 1.)
						{
							eclipseTypeStr = qc_("Annular", "eclipse type"); // Non-central annular eclipse
						}
						else
							eclipseTypeStr = qc_("Partial", "eclipse type");
						noncentraleclipse = true;
					}
					else
					{
						if (L2 < 0.)
						{
							eclipseTypeStr = qc_("Total", "eclipse type");
						}
						else if (L2 > 0.0047)
						{
							eclipseTypeStr = qc_("Annular", "eclipse type");
						}
						else if (L2 > 0. && L2 < 0.0047)
						{
							if (L2 < (0.00464 * sqrt(1. - gamma * gamma)))
							{
								eclipseTypeStr = qc_("Hybrid", "eclipse type");
							}
							else
							{
								eclipseTypeStr = qc_("Annular", "eclipse type");
							}
						}
					}

					// Saros series calculations - useful to search for eclipses in the same Saros
					// Adapted from Saros calculations for solar eclipses in Sky & Telescope (October 1985)
					// Saros numbers calculated here are matching well with NASA's Five Millennium Catalog of Solar Eclipses

					// ln = Brown Lunation number : = 1 at the first New Moon of 1923
					const double q = round ((JD-2423436.40347)/29.530588);
					const int ln = int(q) + 1 - 953;
					const int nd = ln + 105;
					const int s = 136 + 38 * nd;
					const int nx = -61 * nd;
					const int nc = qFloor(nx / 358. + 0.5 - nd / (12. * 358 * 358));
					int saros = 1 + ((s + nc * 223 - 1) % 223);
					if ((s + nc * 223 - 1) < 0) saros -= 223;
					if (saros < -223) saros += 223;

					sarosStr = QString("%1").arg(QString::number(saros));
					gammaStr = QString("%1").arg(QString::number(gamma, 'f', 3));
					double eclipseLatitude = 0.;
					double eclipseLongitude = 0.;
					double eclipseAltitude = 0.;

					if (noncentraleclipse)
					{
						magStr = QString("%1").arg(QString::number(magnitude, 'f', 3));
						eclipseLatitude = latDeg;
						eclipseLongitude = lngDeg;
						altitudeStr = "0°";
						durationStr = dash;
						pathWidthStr = dash;
					}
					else
					{
						magStr = QString("%1").arg(QString::number(dRatio, 'f', 3));
						eclipseAltitude = altitude;
						altitudeStr = QString("%1°").arg(QString::number(round(eclipseAltitude)));
						pathWidthStr = QString("%1 %2").arg(QString::number(round(pathWidth)), km);
						eclipseLatitude = latDeg;
						eclipseLongitude = lngDeg;
						double centralDuration = abs(duration);
						int durationMinute = int(centralDuration);
						int durationSecond = qRound((centralDuration - durationMinute) * 60.);
						if (durationSecond>59)
						{
							durationMinute += 1;
							durationSecond = 0;
						}
						if (durationSecond>9)
							durationStr = QString("%1m %2s").arg(QString::number(durationMinute), QString::number(durationSecond));
						else
							durationStr = QString("%1m 0%2s").arg(QString::number(durationMinute), QString::number(durationSecond));
					}

					latitudeStr = StelUtils::decDegToLatitudeStr(eclipseLatitude, !withDecimalDegree);
					longitudeStr = StelUtils::decDegToLongitudeStr(eclipseLongitude, true, false, !withDecimalDegree);

					ACSolarEclipseTreeWidgetItem* treeItem = new ACSolarEclipseTreeWidgetItem(ui->solareclipseTreeWidget);
					treeItem->setText(SolarEclipseDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))); // local date and time
					treeItem->setData(SolarEclipseDate, Qt::UserRole, JD);
					treeItem->setText(SolarEclipseSaros, sarosStr);
					treeItem->setToolTip(SolarEclipseSaros, q_("Saros series number of eclipse (each eclipse in a Saros is separated by an interval of 18 years 11.3 days)"));
					treeItem->setText(SolarEclipseType, eclipseTypeStr);
					treeItem->setText(SolarEclipseGamma, gammaStr);
					treeItem->setText(SolarEclipseMag, magStr);
					treeItem->setToolTip(SolarEclipseMag, q_("Eclipse magnitude is the fraction of the Sun's diameter obscured by the Moon"));
					treeItem->setText(SolarEclipseLatitude, latitudeStr);
					treeItem->setData(SolarEclipseLatitude, Qt::UserRole, eclipseLatitude);
					treeItem->setText(SolarEclipseLongitude, longitudeStr);
					treeItem->setData(SolarEclipseLongitude, Qt::UserRole, eclipseLongitude);
					treeItem->setText(SolarEclipseAltitude, altitudeStr);
					treeItem->setData(SolarEclipseAltitude, Qt::UserRole, eclipseAltitude);
					treeItem->setToolTip(SolarEclipseAltitude, q_("Sun's altitude at greatest eclipse"));
					treeItem->setText(SolarEclipsePathwidth, pathWidthStr);
					treeItem->setData(SolarEclipsePathwidth, Qt::UserRole, pathWidth);
					treeItem->setToolTip(SolarEclipsePathwidth, q_("Width of the path of totality or annularity at greatest eclipse"));
					treeItem->setText(SolarEclipseDuration, durationStr);
					treeItem->setData(SolarEclipseDuration, Qt::UserRole, abs(duration));
					treeItem->setToolTip(SolarEclipseDuration, q_("Duration of total or annular phase at greatest eclipse"));
					for (auto column: {SolarEclipseDate,      SolarEclipseSaros,    SolarEclipseGamma,     SolarEclipseMag,      SolarEclipseLatitude,
							   SolarEclipseLongitude, SolarEclipseAltitude, SolarEclipsePathwidth, SolarEclipseDuration})
						treeItem->setTextAlignment(column, Qt::AlignRight);
				}
			}
		}
		core->setJD(currentJD);
		core->setUseTopocentricCoordinates(saveTopocentric);
		core->update(0); // enforce update cache to avoid odd selection of Moon details!

		// adjust the column width
		for (int i = 0; i < SolarEclipseCount; ++i)
		{
			ui->solareclipseTreeWidget->resizeColumnToContents(i);
		}

		// sort-by-date
		ui->solareclipseTreeWidget->sortItems(SolarEclipseDate, Qt::AscendingOrder);
		enableSolarEclipsesButtons(true);
		enableSolarEclipsesCircumstancesButtons(false);
	}
	else
		cleanupSolarEclipses();
}

void AstroCalcDialog::setSolarEclipseLocalHeaderNames()
{
	solareclipselocalHeader = QStringList({
		q_("Date"),
		q_("Type"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Partial Eclipse Begins", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Central Eclipse Begins", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Maximum Eclipse", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Eclipse Magnitude", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Central Eclipse Ends", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Partial Eclipse Ends", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
		qc_("Duration", "column name")});
	ui->solareclipselocalTreeWidget->setHeaderLabels(solareclipselocalHeader);

	// adjust the column width
	for (int i = 0; i < SolarEclipseLocalCount; ++i)
	{
		ui->solareclipselocalTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListSolarEclipseLocal()
{
	ui->solareclipselocalTreeWidget->clear();
	ui->solareclipselocalTreeWidget->setColumnCount(SolarEclipseLocalCount);
	setSolarEclipseLocalHeaderNames();
	ui->solareclipselocalTreeWidget->header()->setSectionsMovable(false);
	ui->solareclipselocalTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
}

void AstroCalcDialog::generateSolarEclipsesLocal()
{
	const bool onEarth = core->getCurrentPlanet()==solarSystem->getEarth();
	if (onEarth)
	{
		initListSolarEclipseLocal();

		const double currentJD = core->getJD();   // save current JD
		int startyear = ui->eclipseFromYearSpinBox->value();
		int years = ui->eclipseYearsSpinBox->value();
		double startJD, stopJD;
		StelUtils::getJDFromDate(&startJD, startyear, 1, 1, 0, 0, 0);
		StelUtils::getJDFromDate(&stopJD, startyear+years, 12, 31, 23, 59, 59);
		startJD = startJD - core->getUTCOffset(startJD) / 24.;
		stopJD = stopJD - core->getUTCOffset(stopJD) / 24.;
		QString eclipseTypeStr, magStr, durationStr;
		bool centraleclipse = false;

		const bool saveTopocentric = core->getUseTopocentricCoordinates();
		const double approxJD = 2451550.09765;
		const double synodicMonth = 29.530588853;
		int elements = static_cast<int>((stopJD - startJD) / synodicMonth);

		// Find approximate JD of New Moon = Geocentric conjunction in longitude
		double tmp = (startJD - approxJD - synodicMonth) / synodicMonth;
		double InitJD = approxJD + int(tmp) * synodicMonth;

		// Search for solar eclipses at each New Moon
		for (int i = 0; i <= elements+2; i++)
		{
			double JD = InitJD + synodicMonth * i;
			if (JD > startJD)
			{
				core->setUseTopocentricCoordinates(false);
				core->update(0);

				// Find exact time of minimum distance between axis of lunar shadow cone to the center of Earth
				JD = getJDofMinimumDistance(JD);
				core->setJD(JD);
				core->update(0);
				double x,y,d,tf1,tf2,L1,L2,mu;
				SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
				double gamma = sqrt(x * x + y * y);
				if (y<0.) gamma *= -1.;
				if (abs(gamma) <= (1.5433 + L2)) // Solar eclipse occurs on this date
				{
					double magLocal = 0., altitudeMideclipse = 0.;
					double altitudeFirstcontact = 0.;
					double altitudeLastcontact = 0.;

					// Find time of maximum eclipse for current location
					double dt = 1.;
					int iteration = 0;
					LocalSEparams eclipseData = localSolarEclipse(JD,0,false);
					while (abs(dt) > 0.000001 && (iteration < 20))
					{
						eclipseData = localSolarEclipse(JD,0,false);
						dt = eclipseData.dt;
						JD += dt / 24.;
						iteration += 1;
						magLocal = eclipseData.magnitude;
						altitudeMideclipse = eclipseData.altitude;
					}
					magStr = QString("%1").arg(QString::number(magLocal, 'f', 3));

					if (magLocal > 0.) // Eclipse occurs for transparent Earth, need to check altitude of the Sun at each contacts
					{
						double JD1 = JD;
						double JD2 = JD;
						double JDmax = JD;
						double JD3 = JD;
						double JD4 = JD;
						// First contact
						iteration = 0;
						eclipseData = localSolarEclipse(JD1,-1,false);
						dt = eclipseData.dt;
						while (abs(dt) > 0.000001 && (iteration < 20))
						{
							eclipseData = localSolarEclipse(JD1,-1,false);
							dt = eclipseData.dt;
							JD1 += dt / 24.;
							iteration += 1;
							altitudeFirstcontact = eclipseData.altitude;
						}
						// Last contact
						iteration = 0;
						eclipseData = localSolarEclipse(JD4,1,false);
						dt = eclipseData.dt;
						while (abs(dt) > 0.000001 && (iteration < 20))
						{
							eclipseData = localSolarEclipse(JD4,1,false);
							dt = eclipseData.dt;
							JD4 += dt / 24.;
							iteration += 1;
							altitudeLastcontact = eclipseData.altitude;
						}

						if (altitudeMideclipse > -.3 || altitudeFirstcontact > -.3 || altitudeLastcontact > -.3) // Eclipse certainly visible at this location
						{
							// 0.3 deg. is approx. correction for atmospheric refraction at horizon	            		
							if ((altitudeFirstcontact < -.3) && (altitudeLastcontact > -.3)) // Eclipse occurs at Sunrise
							{
								// find time of Sunrise (not exactly, we want the time when lower limb is at the horizon)
								for (int j = 0; j <= 5; j++)
								{
									eclipseData = localSolarEclipse(JD - 5./1440.,0,false);
									double alt1 = eclipseData.altitude+.3;
									eclipseData = localSolarEclipse(JD + 5./1440.,0,false);
									double alt2 = eclipseData.altitude+.3;
									double dt = .006944444 * alt1 / (alt2 - alt1);
									JD = JD - 5./1440. - dt;
									eclipseData = localSolarEclipse(JD,0,false);
								}
								if (altitudeMideclipse > -.3) // Eclipse begins at Sunrise before Mid-eclipse
								{
									JD1 = JD; // time of first contact at Sunrise
								}
								else
								{
									// Eclipse begins at Sunrise, after Mid-eclipse
									JD1 = JD; // time of first contact at Sunrise
									JDmax = JD; // time of maximum eclipse
									magLocal = eclipseData.magnitude;
									magStr = QString("%1").arg(QString::number(eclipseData.magnitude, 'f', 3));
								}
							}

							if ((altitudeFirstcontact > -.3) && (altitudeLastcontact < -.3)) // Eclipse occurs at Sunset
							{
								// find time of Sunset (not exactly, we want the time when lower limb is at the horizon)
								for (int j = 0; j <= 5; j++)
								{
									eclipseData = localSolarEclipse(JD - 5./1440.,0,false);
									double alt1 = eclipseData.altitude+.3;
									eclipseData = localSolarEclipse(JD + 5./1440.,0,false);
									double alt2 = eclipseData.altitude+.3;
									double dt = .006944444 * alt1 / (alt2 - alt1);
									JD = JD - 5./1440. - dt;
									eclipseData = localSolarEclipse(JD,0,false);
								}
								if (altitudeMideclipse > -.3) // Eclipse ends at Sunset after Mid-eclipse
									JD4 = JD; // time of last contact at Sunset
								else
									{
										// Eclipse ends at Sunset before Mid-eclipse
										JD4 = JD; // time of last contact at Sunset
										JDmax = JD; // time of maximum eclipse
										magLocal = eclipseData.magnitude;
										magStr = QString("%1").arg(QString::number(eclipseData.magnitude, 'f', 3));
									}
							}

							// 2nd contact - start of totality/annularity
							iteration = 0;
							JD2 = JD;
							eclipseData = localSolarEclipse(JD2,-1,true);
							dt = eclipseData.dt;
							while (abs(dt) > 0.000001 && (iteration < 20))
							{
								eclipseData = localSolarEclipse(JD2,-1,true);
								dt = eclipseData.dt;
								JD2 += dt / 24.;
								iteration += 1;
							}
							double C2altitude = eclipseData.altitude;
							// 3rd contact - end of totality/annularity
							iteration = 0;
							JD3 = JD;
							eclipseData = localSolarEclipse(JD3,1,true);
							dt = eclipseData.dt;
							while (abs(dt) > 0.000001 && (iteration < 20))
							{
								eclipseData = localSolarEclipse(JD3,1,true);
								dt = eclipseData.dt;
								JD3 += dt / 24.;
								iteration += 1;
							}
							double C3altitude = eclipseData.altitude;

							if (eclipseData.ce > 0. && ((C2altitude > -.3) || (C3altitude > -.3))) // Central eclipse occurs
							{
								centraleclipse = true;
								if (eclipseData.L2 < 0.)
								{
									eclipseTypeStr = qc_("Total", "eclipse type");
									qSwap(JD2, JD3);
								}
								else
									eclipseTypeStr = qc_("Annular", "eclipse type");
							}
							else
								{
									eclipseTypeStr = qc_("Partial", "eclipse type");
									centraleclipse = false;
								}

							if (centraleclipse)
							{
								double duration = abs(JD3-JD2)*1440.;
								int durationMinute = int(duration);
								int durationSecond = qRound((duration - durationMinute) * 60.);
								if (durationSecond>59)
								{
									durationMinute += 1;
									durationSecond = 0;
								}
								if (durationSecond>9)
									durationStr = QString("%1m %2s").arg(QString::number(durationMinute), QString::number(durationSecond));
								else
									durationStr = QString("%1m 0%2s").arg(QString::number(durationMinute), QString::number(durationSecond));
							}

							ACSolarEclipseLocalTreeWidgetItem* treeItem = new ACSolarEclipseLocalTreeWidgetItem(ui->solareclipselocalTreeWidget);
							treeItem->setText(SolarEclipseLocalDate, QString("%1").arg(localeMgr->getPrintableDateLocal(JDmax))); // local date
							treeItem->setData(SolarEclipseLocalDate, Qt::UserRole, JDmax);
							treeItem->setText(SolarEclipseLocalType, eclipseTypeStr);
							treeItem->setText(SolarEclipseLocalFirstContact, QString("%1").arg(localeMgr->getPrintableTimeLocal(JD1)));
							if (centraleclipse && JD2<JD1) // central eclipse  in progress at Sunrise
								treeItem->setText(SolarEclipseLocalFirstContact, dash);
							treeItem->setToolTip(SolarEclipseLocalFirstContact, q_("The time of first contact"));
							
							if (centraleclipse)
								treeItem->setText(SolarEclipseLocal2ndContact, QString("%1").arg(localeMgr->getPrintableTimeLocal(JD2)));
							else
								treeItem->setText(SolarEclipseLocal2ndContact, dash);
							treeItem->setToolTip(SolarEclipseLocal2ndContact, q_("The time of second contact"));
							treeItem->setText(SolarEclipseLocalMaximum, QString("%1").arg(localeMgr->getPrintableTimeLocal(JDmax)));
							treeItem->setToolTip(SolarEclipseLocalMaximum, q_("The time of greatest eclipse"));
							treeItem->setText(SolarEclipseLocalMagnitude, magStr);
							if (centraleclipse)
								treeItem->setText(SolarEclipseLocal3rdContact, QString("%1").arg(localeMgr->getPrintableTimeLocal(JD3)));
							else
								treeItem->setText(SolarEclipseLocal3rdContact, dash);
							treeItem->setToolTip(SolarEclipseLocal3rdContact, q_("The time of third contact"));
							treeItem->setText(SolarEclipseLocalLastContact, QString("%1").arg(localeMgr->getPrintableTimeLocal(JD4)));
							if (centraleclipse && JD3>JD4) // central eclipse in progress at Sunset
								treeItem->setText(SolarEclipseLocalLastContact, dash);
							treeItem->setToolTip(SolarEclipseLocalLastContact, q_("The time of fourth contact"));
							if (centraleclipse)
								treeItem->setText(SolarEclipseLocalDuration, durationStr);
							else
								treeItem->setText(SolarEclipseLocalDuration, dash);
							treeItem->setToolTip(SolarEclipseLocalDuration, q_("Duration of total or annular eclipse"));
							for (auto column: {SolarEclipseLocalDate,    SolarEclipseLocalMagnitude,  SolarEclipseLocalFirstContact, SolarEclipseLocal2ndContact,
									   SolarEclipseLocalMaximum, SolarEclipseLocal3rdContact, SolarEclipseLocalLastContact,  SolarEclipseLocalDuration})
								treeItem->setTextAlignment(column, Qt::AlignRight);
						}
					}
				}
			}
		}
		core->setJD(currentJD);
		core->setUseTopocentricCoordinates(saveTopocentric);
		core->update(0); // enforce update cache to avoid odd selection of Moon details!

		// adjust the column width
		for (int i = 0; i < SolarEclipseLocalCount; ++i)
		{
			ui->solareclipselocalTreeWidget->resizeColumnToContents(i);
		}

		// sort-by-date
		ui->solareclipselocalTreeWidget->sortItems(SolarEclipseLocalDate, Qt::AscendingOrder);
		enableSolarEclipsesLocalButtons(true);
	}
	else
		cleanupSolarEclipsesLocal();
}

void AstroCalcDialog::cleanupSolarEclipses()
{
	ui->solareclipseTreeWidget->clear();
	ui->solareclipsecontactsTreeWidget->clear();
	enableSolarEclipsesButtons(false);
	enableSolarEclipsesCircumstancesButtons(false);
}

void AstroCalcDialog::enableSolarEclipsesButtons(bool enable)
{
	ui->solareclipsesCleanupButton->setEnabled(enable);
	ui->solareclipsesSaveButton->setEnabled(enable);
}

void AstroCalcDialog::enableSolarEclipsesCircumstancesButtons(bool enable)
{
	ui->solareclipsesKMLSaveButton->setEnabled(enable);
	ui->solareclipsescontactsSaveButton->setEnabled(enable);
}

void AstroCalcDialog::selectCurrentSolarEclipse(const QModelIndex& modelIndex)
{
	initListSolarEclipseContact();
	const bool saveTopocentric = core->getUseTopocentricCoordinates();
	const double currentJD = core->getJD();
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	QPair<QString, QString> coordStrings;
	QString pathWidthStr, durationStr, eclipseTypeStr;
	const QString km = qc_("km", "distance");
	double JDMid = modelIndex.sibling(modelIndex.row(), SolarEclipseDate).data(Qt::UserRole).toDouble();
	double JD = JDMid;

	// Compute time and other data of contacts
	bool nonCentralEclipse = false;
	core->setJD(JD);
	core->update(0);
	double x,y,d,tf1,tf2,L1,L2,mu;
	SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
	double gamma = std::sqrt(x*x+y*y);
	QPair<double, double> coordinates;
	if ((gamma > 0.9972) && (gamma < (1.5433 + L2)))
		nonCentralEclipse = true;

	for (int i = 0; i < 5; i++)
	{
		double dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude;
		bool event = false;
		if (i==0) // P1
		{
			JD = getJDofContact(JDMid,true,true,true,true);
			SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			event = true;
		}
		if (i==1 && !nonCentralEclipse) // C1
		{
			JD = getJDofContact(JDMid,true,false,false,true);
			// Workaround to mostly eliminate 0.1 second of fluctuation
			// that can noticebly move coordinates of shadow.
			JD = int(JD)+(int((JD-int(JD))*86400.)-1)/86400.;
			SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			// Make sure that the shadow axis is really touching Earth,
			// otherwise path width and duration will be zero.
			int steps = 0;
			while (pathWidth<0.0001 && steps<20)
			{
				JD += .1/86400.;
				SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				steps += 1;
			}
			core->setJD(JD);
			core->update(0);
			SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
			coordinates = getContactCoordinates(x,y,d,mu);
			latDeg = coordinates.first;
			lngDeg = coordinates.second;
			event = true;
		}
		else if (i==2) // Greatest Eclipse
		{
			JD = JDMid;
			SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			event = true;
		}
		else if (i==3 && !nonCentralEclipse) // C2
		{
			JD = getJDofContact(JDMid,false,false,false,true);
			// Workaround to mostly eliminate 0.1 second of fluctuation
			// that can noticebly move coordinates of shadow.
			JD = int(JD)+(int((JD-int(JD))*86400.)+1)/86400.;
			SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			// Make sure that the shadow axis is really touching Earth,
			// otherwise path width and duration will be zero.
			int steps = 0;
			while (pathWidth<0.0001 && steps<20)
			{
				JD -= .1/86400.;
				SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				steps += 1;
			}
			core->setJD(JD);
			core->update(0);
			SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
			coordinates = getContactCoordinates(x,y,d,mu);
			latDeg = coordinates.first;
			lngDeg = coordinates.second;
			event = true;
		}
		else if (i==4) // P4
		{
			JD = getJDofContact(JDMid,false,true,true,true);
			SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			event = true;
		}	
		if (event)
		{
			ACSolarEclipseContactsTreeWidgetItem* treeItem = new ACSolarEclipseContactsTreeWidgetItem(ui->solareclipsecontactsTreeWidget);
			switch (i)
			{
				case 0:
					treeItem->setText(SolarEclipseContact, QString(q_("Eclipse begins; first contact with Earth")));
					treeItem->setData(SolarEclipseContact, Qt::UserRole, false);
					break;
				case 1:
					treeItem->setText(SolarEclipseContact, QString(q_("Beginning of center line; central eclipse begins")));
					treeItem->setData(SolarEclipseContact, Qt::UserRole, false);
					break;
				case 2:
					treeItem->setText(SolarEclipseContact, QString(q_("Greatest eclipse")));
					treeItem->setData(SolarEclipseContact, Qt::UserRole, true);
					break;
				case 3:
					treeItem->setText(SolarEclipseContact, QString(q_("End of center line; central eclipse ends")));
					treeItem->setData(SolarEclipseContact, Qt::UserRole, false);
					break;
				case 4:
					treeItem->setText(SolarEclipseContact, QString(q_("Eclipse ends; last contact with Earth")));
					treeItem->setData(SolarEclipseContact, Qt::UserRole, false);
					break;
			}
			treeItem->setText(SolarEclipseContactDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD)));
			treeItem->setData(SolarEclipseContactDate, Qt::UserRole, JD);
			treeItem->setText(SolarEclipseContactLatitude, StelUtils::decDegToLatitudeStr(latDeg, !withDecimalDegree));
			treeItem->setData(SolarEclipseContactLatitude, Qt::UserRole, latDeg);
			treeItem->setToolTip(SolarEclipseContactLatitude, q_("Geographic latitude of contact point"));
			treeItem->setText(SolarEclipseContactLongitude, StelUtils::decDegToLongitudeStr(lngDeg, true, false, !withDecimalDegree));
			treeItem->setData(SolarEclipseContactLongitude, Qt::UserRole, lngDeg);
			treeItem->setToolTip(SolarEclipseContactLongitude, q_("Geographic longitude of contact point"));
			switch (i)
			{
				case 0:
				case 4:
				{
					pathWidthStr = dash;
					durationStr = dash;
					eclipseTypeStr = qc_("Partial", "eclipse type");
					break;
				}
				case 1:
				case 2:
				case 3:
				{
					if (nonCentralEclipse)
					{
						if (abs(gamma) < 0.9972 + abs(L2) && dRatio > 1.)
						{
							eclipseTypeStr = qc_("Total", "eclipse type"); // Non-central total eclipse
						}
						else if (abs(gamma) < 0.9972 + abs(L2) && dRatio < 1.)
						{
							eclipseTypeStr = qc_("Annular", "eclipse type"); // Non-central annular eclipse
						}
						else
						{
							eclipseTypeStr = qc_("Partial", "eclipse type");
						}
						pathWidthStr = dash;
						durationStr = dash;
					}
					else
					{
						pathWidthStr = QString("%1 %2").arg(QString::number(round(pathWidth)), km);
						double centralDuration = abs(duration);
						int durationMinute = int(centralDuration);
						int durationSecond = qRound((centralDuration - durationMinute) * 60.);
						if (durationSecond>59)
						{
							durationMinute += 1;
							durationSecond = 0;
						}
						if (durationSecond>9)
							durationStr = QString("%1m %2s").arg(QString::number(durationMinute), QString::number(durationSecond));
						else
							durationStr = QString("%1m 0%2s").arg(QString::number(durationMinute), QString::number(durationSecond));
						if (duration<=0)
							eclipseTypeStr = qc_("Total", "eclipse type");
						else
							eclipseTypeStr = qc_("Annular", "eclipse type");
					}
					break;
				}
			}
			treeItem->setText(SolarEclipseContactPathwidth, pathWidthStr);
			treeItem->setData(SolarEclipseContactPathwidth, Qt::UserRole, pathWidth);
			treeItem->setToolTip(SolarEclipseContactPathwidth, q_("Width of the path of totality or annularity"));
			treeItem->setText(SolarEclipseContactDuration, durationStr);
			treeItem->setToolTip(SolarEclipseContactDuration, q_("Duration of total or annular phase"));
			treeItem->setText(SolarEclipseContactType, eclipseTypeStr);			
			treeItem->setTextAlignment(SolarEclipseContact, Qt::AlignLeft);
			treeItem->setTextAlignment(SolarEclipseContactDate, Qt::AlignRight);
			treeItem->setTextAlignment(SolarEclipseContactLatitude, Qt::AlignRight);
			treeItem->setTextAlignment(SolarEclipseContactLongitude, Qt::AlignRight);
			treeItem->setTextAlignment(SolarEclipseContactPathwidth, Qt::AlignRight);
			treeItem->setTextAlignment(SolarEclipseContactDuration, Qt::AlignRight);
			treeItem->setTextAlignment(SolarEclipseContactType, Qt::AlignLeft);
		}
		ui->solareclipsecontactsTreeWidget->setColumnHidden(7,true);
	}
	core->setJD(currentJD);
	core->setUseTopocentricCoordinates(saveTopocentric);
	core->update(0);

	// adjust the column width
	for (int i = 0; i < SolarEclipseContactCount; ++i)
	{
		ui->solareclipsecontactsTreeWidget->resizeColumnToContents(i);
	}

	// sort-by-date
	ui->solareclipsecontactsTreeWidget->sortItems(SolarEclipseContactDate, Qt::AscendingOrder);
	enableSolarEclipsesCircumstancesButtons(true);
}

void AstroCalcDialog::selectCurrentSolarEclipseContact(const QModelIndex& modelIndex)
{
	const double JD = modelIndex.sibling(modelIndex.row(), SolarEclipseContactDate).data(Qt::UserRole).toDouble();
	const float lat = modelIndex.sibling(modelIndex.row(), SolarEclipseContactLatitude).data(Qt::UserRole).toFloat();
	const float lon = modelIndex.sibling(modelIndex.row(), SolarEclipseContactLongitude).data(Qt::UserRole).toFloat();
	const bool greatest = modelIndex.sibling(modelIndex.row(), SolarEclipseContact).data(Qt::UserRole).toBool();

	StelLocation contactLoc(greatest ? q_("Greatest eclipse’s point") : q_("Eclipse’s contact point"), "", "", lon, lat, 10, 0, "LMST", 1, 'X');
	// Find landscape color at the spot
	StelLocationMgr* locationMgr = &StelApp::getInstance().getLocationMgr();
	QColor color=locationMgr->getColorForCoordinates(lon, lat);
	core->moveObserverTo(contactLoc, 2., 2., QString("ZeroColor(%1)").arg(Vec3f(color).toStr())); // use a neutral horizon but environmental color to avoid confusion.
	goToObject("Sun", JD);
}

void AstroCalcDialog::saveSolarEclipses()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save calculated solar eclipses as..."), "solareclipses");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->solareclipseTreeWidget, solareclipseHeader);
	else
		saveTableAsXLSX(fileData.first, ui->solareclipseTreeWidget, solareclipseHeader, q_("Solar Eclipses"), q_("Solar Eclipses"), q_("Note: Path of eclipses during thousands of years in the past and future are not reliable due to uncertainty in ΔT which is caused by fluctuations in Earth's rotation."));
}

void AstroCalcDialog::saveSolarEclipseCircumstances()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save solar eclipse circumstances as..."), "solareclipse-circumstances");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->solareclipsecontactsTreeWidget, solareclipsecontactsHeader);
	else
		saveTableAsXLSX(fileData.first, ui->solareclipsecontactsTreeWidget, solareclipsecontactsHeader, q_("Circumstances of Solar Eclipse"), q_("Circumstances of Solar Eclipse"), q_("Note: Path of eclipses during thousands of years in the past and future are not reliable due to uncertainty in ΔT which is caused by fluctuations in Earth's rotation."));
}

void AstroCalcDialog::selectCurrentSolarEclipseDate(const QModelIndex& modelIndex)
{
	const double JD = modelIndex.sibling(modelIndex.row(), SolarEclipseDate).data(Qt::UserRole).toDouble();
	const float lat = modelIndex.sibling(modelIndex.row(), SolarEclipseLatitude).data(Qt::UserRole).toFloat();
	const float lon = modelIndex.sibling(modelIndex.row(), SolarEclipseLongitude).data(Qt::UserRole).toFloat();

	StelLocation maxLoc(q_("Greatest eclipse’s point"), "", "", lon, lat, 10, 0, "LMST", 1, 'X');
	qDebug() << "AstroCalcDialog::selectCurrentSolarEclipseDate(" << modelIndex;
	qDebug() << "Moving to MaxLoc ...";
	// Find landscape color at the spot
	StelLocationMgr* locationMgr = &StelApp::getInstance().getLocationMgr();
	QColor color=locationMgr->getColorForCoordinates(lon, lat);
	core->moveObserverTo(maxLoc, 2., 2., QString("ZeroColor(%1)").arg(Vec3f(color).toStr())); // use a neutral horizon but environmental color to avoid confusion.
	goToObject("Sun", JD);
	qDebug() << "Moving to MaxLoc ... done";
}

QPair<double, double> AstroCalcDialog::getRiseSetLineCoordinates(bool first, double x,double y,double d,double L,double mu)
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double ff = 1./(1.-f);
	const double m2 = x*x+y*y;
	const double cgm = (m2+1.-L*L)/(2.*std::sqrt(m2));
	QPair<double, double> coordinates(99., 0.);
	if (std::abs(cgm)<=1.)
	{
		double gamma = std::acos(cgm)+std::atan2(x,y);
		if (!first)
			gamma = M_PI*2.-std::acos(cgm)+std::atan2(x,y);
		double xi = std::sin(gamma);
		double eta = std::cos(gamma);
		double b = -eta*std::sin(d);
		double theta = std::atan2(xi,b)*M_180_PI;
		double lngDeg = theta-mu;
		lngDeg = StelUtils::fmodpos(lngDeg, 360.);
		if (lngDeg > 180.) lngDeg -= 360.;
		double sfn1 = eta*std::cos(d);
		double cfn1 = std::sqrt(1.-sfn1*sfn1);
		double latDeg = ff*sfn1/cfn1;
		coordinates.first = std::atan(latDeg)*M_180_PI;
		coordinates.second = lngDeg;
	}
	return coordinates;
}

QPair<double, double> AstroCalcDialog::getShadowOutlineCoordinates(double angle,double x,double y,double d,double L,double tf,double mu)
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	static const double ff = 1./(1.-f);
	const double rho1 = std::sqrt(1.-e2*std::cos(d)*std::cos(d));
	const double sd1 = std::sin(d)/rho1;
	const double cd1 = std::sqrt(1.-e2)*std::cos(d)/rho1;
	const double xi0 = x-L*std::sin(angle);
	const double eta0 = y-L*std::cos(angle);
	const double zeta0 = 1.-xi0*xi0-eta0*eta0;

	QPair<double, double> coordinates(99., 0.);
	if (zeta0 >= 0)
	{
		const double L1 = L-zeta0*tf;
		const double xi = x-L1*std::sin(angle);
		const double eta1 = (y-L1*std::cos(angle))/rho1;
		const double pp = 1.-xi*xi-eta1*eta1;

		if (pp >= 0)
		{
			const double zeta0 = std::sqrt(pp);
			const double L1 = L-zeta0*tf;
			const double xi = x-L1*std::sin(angle);
			const double eta1 = (y-L1*std::cos(angle))/rho1;
			const double pp = 1.-xi*xi-eta1*eta1;

			if (pp >= 0)
			{
				double zeta1 = std::sqrt(pp);
				double b = -eta1*sd1+zeta1*cd1;
				double theta = std::atan2(xi,b)*M_180_PI;
				double lngDeg = theta-mu;
				lngDeg = StelUtils::fmodpos(lngDeg, 360.);
				if (lngDeg > 180.) lngDeg -= 360.;

				double sfn1 = eta1*cd1+zeta1*sd1;
				double cfn1 = std::sqrt(1.-sfn1*sfn1);
				double latDeg = ff*sfn1/cfn1;
				coordinates.first = std::atan(latDeg)*M_180_PI;
				coordinates.second = lngDeg;
			}
		}
	}
	return coordinates;
}

QPair<double, double> AstroCalcDialog::getMaximumEclipseAtRiseSet(bool first, double JD)
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double ff = 1./(1.-f);
	core->setJD(JD);
	core->update(0);
	double xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot;
	BesselParameters(xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot,true);
	double x,y,d,tf1,tf2,L1,L2,mu;
	SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);

	double qa = std::atan2(bdot,cdot);
	if (!first) // there are two parts of the curve
		qa += M_PI;
	const double sgqa = x*std::cos(qa)-y*std::sin(qa);

	QPair<double, double> coordinates(99., 0.);
	if (std::abs(sgqa)<1.)
	{
		const double gqa = std::asin(sgqa);
		const double ga = gqa+qa;
		const double xxia = x-std::sin(ga);
		const double yetaa = y-std::cos(ga);
		if (xxia*xxia+yetaa*yetaa < L1*L1)
		{
			double b = -std::cos(ga)*std::sin(d);
			double theta = std::atan2(std::sin(ga),b)*M_180_PI;
			double lngDeg = StelUtils::fmodpos(theta-mu, 360.);
			if (lngDeg > 180.) lngDeg -= 360.;
			double sfn1 = std::cos(ga)*std::cos(d);
			double cfn1 = std::sqrt(1.-sfn1*sfn1);
			double latDeg = ff*sfn1/cfn1;
			coordinates.first = std::atan(latDeg)*M_180_PI;
			coordinates.second = lngDeg;
		}
	}
	return coordinates;
}

BesselParameters::BesselParameters(double &xdot, double &ydot, double &ddot, double &mudot,
	double &ldot, double &etadot, double &bdot, double &cdot, bool penumbra)
{
	StelCore* core = StelApp::getInstance().getCore();
	double JD = core->getJD();
	double tf,tf1,tf2,L;
	core->setJD(JD - 5./1440.);
	core->update(0);
	double x1,y1,d1,mu1,L11,L21;
	SolarEclipseBessel(x1,y1,d1,tf1,tf2,L11,L21,mu1);
	core->setJD(JD + 5./1440.);
	core->update(0);
	double x2,y2,d2,mu2,L12,L22;
	SolarEclipseBessel(x2,y2,d2,tf1,tf2,L12,L22,mu2);

	xdot = (x2-x1)*6.;
	ydot = (y2-y1)*6.;
	ddot = (d2-d1)*6.*M_PI_180;
	mudot = (mu2-mu1);
	if (mudot<0.) mudot += 360.; // make sure it is positive in case mu2 < mu1
	mudot = mudot*6.* M_PI_180;
	core->setJD(JD);
	core->update(0);
	double x,y,d,L1,L2,mu;
	SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
	if (penumbra)
	{
		L = L1;
		tf = tf1;
		ldot = (L12-L11)*6.;
	}
	else
	{
		L = L2;
		tf = tf2;
		ldot = (L22-L21)*6.;
	}
	etadot = mudot*x*std::sin(d);
	bdot = -(ydot-etadot);
	cdot = xdot+mudot*y*std::sin(d)+mudot*L*tf*std::cos(d);
}

void AstroCalcDialog::saveSolarEclipseKML()
{
	int count = ui->solareclipsecontactsTreeWidget->topLevelItemCount();
	if (count>0) // Should be a way to make sure that we have circumstances of an eclipse in the table
	{
		const double currentJD = core->getJD(); // save current JD
		const bool saveTopocentric = core->getUseTopocentricCoordinates();
		core->setUseTopocentricCoordinates(false);
		core->update(0);		
		double JD = ui->solareclipsecontactsTreeWidget->topLevelItem(1)->data(SolarEclipseContactDate, Qt::UserRole).toDouble();
		// Find exact time of minimum distance between axis of lunar shadow cone to the center of Earth
		JD = getJDofMinimumDistance(JD);
		double JDMid = JD;
		int Year, Month, Day;
		StelUtils::getDateFromJulianDay(JDMid, &Year, &Month, &Day);
		// Use year-month-day in the file name
		QString eclipseDateStr = QString("-%1-%2-%3").arg(QString::number(Year), QString::number(Month), QString::number(Day));
		QString filter = "KML";
		filter.append(" (*.kml)");
		QString defaultFilter("(*.kml)");
		QString filePath = QFileDialog::getSaveFileName(nullptr,
								q_("Save KML as..."),
								QDir::homePath() + "/solareclipse"+eclipseDateStr+".kml",
								filter,
								&defaultFilter);

		if (filePath!=nullptr)
			QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		bool partialEclipse = false;
		bool nonCentralEclipse = false;
		double x,y,d,tf1,tf2,L1,L2,mu;
		double dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude;
		core->setJD(JDMid);
		core->update(0);
		SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
		double gamma = std::sqrt(x*x+y*y);
		// Type of eclipse
		if (abs(gamma) > 0.9972 && abs(gamma) < (1.5433 + L2))
		{
			if (abs(gamma) < 0.9972 + abs(L2))
			{
				partialEclipse = false;
				nonCentralEclipse = true; // non-central total/annular eclipse
			}
			else
				partialEclipse = true;
		}
		const double JDP1 = getJDofContact(JDMid,true,true,true,true);
		const double JDP4 = getJDofContact(JDMid,false,true,true,true);

		double JDP2 = 0., JDP3 = 0.;
		QPair<double, double> coordinates;
		// Check northern/southern limits of penumbra at greatest eclipse
		bool bothPenumbralLimits = false;
		coordinates = getNSLimitofShadow(JDMid,true,true);
		double latPL1 = coordinates.first;
		coordinates = getNSLimitofShadow(JDMid,false,true);
		double latPL2 = coordinates.first;
		if (latPL1 <= 90. && latPL2 <= 90.)
			bothPenumbralLimits = true;

		if (bothPenumbralLimits)
		{
			JDP2 = getJDofContact(JDMid,true,true,true,false);
			JDP3 = getJDofContact(JDMid,false,true,true,false);
		}

		QFile file(filePath);
		if(file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream stream(&file);
			stream << "<?xml version='1.0' encoding='UTF-8'?>\n<kml xmlns='http://www.opengis.net/kml/2.2'>\n<Document>" << '\n';
			stream << "<name>"+q_("Solar Eclipse")+eclipseDateStr+"</name>\n<description>"+q_("Created by Stellarium")+"</description>\n";
			stream << "<Style id='Hybrid'>\n<LineStyle>\n<color>ff800080</color>\n<width>1</width>\n</LineStyle>\n";
			stream << "<PolyStyle>\n<color>ff800080</color>\n</PolyStyle>\n</Style>\n";
			stream << "<Style id='Total'>\n<LineStyle>\n<color>ff0000ff</color>\n<width>1</width>\n</LineStyle>\n";
			stream << "<PolyStyle>\n<color>ff0000ff</color>\n</PolyStyle>\n</Style>\n";
			stream << "<Style id='Annular'>\n<LineStyle>\n<color>ffff0000</color>\n<width>1</width>\n</LineStyle>\n";
			stream << "<PolyStyle>\n<color>ffff0000</color>\n</PolyStyle>\n</Style>\n";
			stream << "<Style id='PLimits'>\n<LineStyle>\n<color>ff00ff00</color>\n<width>1</width>\n</LineStyle>\n";
			stream << "<PolyStyle>\n<color>ff00ff00</color>\n</PolyStyle>\n</Style>\n";

			// Plot GE
			SolarEclipseData(JDMid,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			QString eclipseTime = localeMgr->getPrintableTimeLocal(JDMid);
			stream << "<Placemark>\n<name>"+q_("Greatest eclipse")+" ("+eclipseTime+")</name>\n<Point>\n<coordinates>";
			stream << lngDeg << "," << latDeg << ",0.0\n";
			stream << "</coordinates>\n</Point>\n</Placemark>\n";

			// Plot P1
			SolarEclipseData(JDP1,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			double latP1 = latDeg, lngP1 = lngDeg;
			eclipseTime = localeMgr->getPrintableTimeLocal(JDP1);
			stream << "<Placemark>\n<name>"+q_("First contact with Earth")+" ("+eclipseTime+")</name>\n<Point>\n<coordinates>";
			stream << lngDeg << "," << latDeg << ",0.0\n";
			stream << "</coordinates>\n</Point>\n</Placemark>\n";

			// Plot P4
			SolarEclipseData(JDP4,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
			double latP4 = latDeg, lngP4 = lngDeg;
			eclipseTime = localeMgr->getPrintableTimeLocal(JDP4);
			stream << "<Placemark>\n<name>"+q_("Last contact with Earth")+" ("+eclipseTime+")</name>\n<Point>\n<coordinates>";
			stream << lngDeg << "," << latDeg << ",0.0\n";
			stream << "</coordinates>\n</Point>\n</Placemark>\n";

			// Northern/southern Limits of penumbra
			bool north = true;
			for (int j = 0; j < 2; j++)
			{
				if (j != 0) north = false;
				stream << "<Placemark>\n<name>PenumbraLimit</name>\n<styleUrl>#PLimits</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
				stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
				JD = JDP1;
				int i = 0;
				while (JD < JDP4)
				{
					JD = JDP1 + i/1440.0;
					coordinates = getNSLimitofShadow(JD,north,true);
					if (coordinates.first <= 90.)
						stream << coordinates.second << "," << coordinates.first << ",0.0\n";
					i++;
				}
				stream << "</coordinates>\n</LineString>\n</Placemark>\n";
			}

			// Eclipse begins/ends at sunrise/sunset curve
			if (bothPenumbralLimits)
			{
				double latP2, lngP2, latP3, lngP3;
				bool first = true;
				for (int j = 0; j < 2; j++)
				{
					if (j != 0) first = false;
					// P1 to P2 curve
					core->setJD(JDP2);
					core->update(0);
					SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
					coordinates = getContactCoordinates(x,y,d,mu);
					latP2 = coordinates.first;
					lngP2 = coordinates.second;
					stream << "<Placemark>\n<name>RiseSetLimit</name>\n<styleUrl>#PLimits</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
					stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
					stream << lngP1 << "," << latP1 << ",0.0\n";
					JD = JDP1;
					int i = 0;
					while (JD < JDP2)
					{
						JD = JDP1 + i/1440.0;
						core->setJD(JD);
						core->update(0);
						SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
						coordinates = getRiseSetLineCoordinates(first,x,y,d,L1,mu);
						if (coordinates.first <= 90.)
						{
							stream << coordinates.second << "," << coordinates.first << ",0.0\n";
						}
						i++;
					}
					stream << lngP2 << "," << latP2 << ",0.0\n";
					stream << "</coordinates>\n</LineString>\n</Placemark>\n";

					// P3 to P4 curve
					core->setJD(JDP3);
					core->update(0);
					SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
					coordinates = getContactCoordinates(x,y,d,mu);
					latP3 = coordinates.first;
					lngP3 = coordinates.second;
					stream << "<Placemark>\n<name>RiseSetLimit</name>\n<styleUrl>#PLimits</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
					stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
					stream << lngP3 << "," << latP3 << ",0.0\n";
					JD = JDP3;
					i = 0;
					while (JD < JDP4)
					{
						JD = JDP3 + i/1440.0;
						core->setJD(JD);
						core->update(0);
						SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
						coordinates = getRiseSetLineCoordinates(first,x,y,d,L1,mu);
						if (coordinates.first <= 90.)
						{
							stream << coordinates.second << "," << coordinates.first << ",0.0\n";
						}
						i++;
					}
					stream << lngP4 << "," << latP4 << ",0.0\n";
					stream << "</coordinates>\n</LineString>\n</Placemark>\n";
				}
			}
			else
			{
				// Only northern or southern limit exists
				// Draw the curve between P1 to P4
				bool first = true;
				for (int j = 0; j < 2; j++)
				{
					if (j != 0) first = false;
					stream << "<Placemark>\n<name>RiseSetLimit</name>\n<styleUrl>#PLimits</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
					stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
					stream << lngP1 << "," << latP1 << ",0.0\n";
					JD = JDP1;
					int i = 0;
					while (JD < JDP4)
					{
						JD = JDP1 + i/1440.0;
						core->setJD(JD);
						core->update(0);
						SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
						coordinates = getRiseSetLineCoordinates(first,x,y,d,L1,mu);
						if (coordinates.first <= 90.)
						{
							stream << coordinates.second << "," << coordinates.first << ",0.0\n";
						}
						i++;
					}
					stream << lngP4 << "," << latP4 << ",0.0\n";
					stream << "</coordinates>\n</LineString>\n</Placemark>\n";
				}
			}

			// Curve of maximum eclipse at sunrise/sunset
			// There are two parts of the curve
			bool first = true;
			double startLat1 = 0., startLon1 = 0., endLat1 = 0., endLon1 = 0.;
			double endJD1 = JDMid, startJD1 = JDMid, endJD2 = JDMid, startJD2 = JDMid;
			for (int j = 0; j < 2; j++)
			{
				if ( j!= 0) first = false;
				stream << "<Placemark>\n<name>MaxEclipseSunriseSunset</name>\n<styleUrl>#PLimits</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
				stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
				JD = JDP1;
				int i = 0, count = 0;
				double lat0 = 0., lon0 = 0., dlat, dlon, diff;
				while (JD < JDP4)
				{
					JD = JDP1 + i/1440.0;
					coordinates = getMaximumEclipseAtRiseSet(first,JD);
					if (abs(coordinates.first) <= 90.)
					{
						count += 1;
						dlat = lat0-coordinates.first;
						dlon = lon0-coordinates.second;
						diff = std::sqrt(dlat*dlat+dlon*dlon);
						if (diff>10.)
						{
							stream << "</coordinates>\n</LineString>\n</Placemark>\n";
							stream << "<Placemark>\n<name>MaxEclipseSunriseSunset</name>\n<styleUrl>#PLimits</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
							stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
						}
						if (!first && count == 1) startJD2 = JD;
						if (!first && count == 1 && bothPenumbralLimits && (startJD1 < JDMid) && (startJD2 < JDMid))
						{
							stream << startLon1 << "," << startLat1 << ",0.0\n"; // connect start of part 1 to start of part 2
						}
						stream << coordinates.second << "," << coordinates.first << ",0.0\n";
						if (first && bothPenumbralLimits)
						{
							endJD1 = JD;
							endLat1 = coordinates.first;
							endLon1 = coordinates.second;
						}
						if (first && count == 1 && bothPenumbralLimits)
						{
							startJD1 = JD;
							startLat1 = coordinates.first;
							startLon1 = coordinates.second;
						}
						lat0 = coordinates.first;
						lon0 = coordinates.second;
					}
					i++;
				}
				if (!first && bothPenumbralLimits) endJD2 = JD;
				if (!first && bothPenumbralLimits && (endJD1 > JDMid) && (endJD2 > JDMid))
				{
					stream << endLon1 << "," << endLat1 << ",0.0\n"; // connect end of part 2 to end of part 1
				}
				stream << "</coordinates>\n</LineString>\n</Placemark>\n";
			}

			if (!partialEclipse)
			{
				double JDC1 = JDMid, JDC2 = JDMid;
				const double JDU1 = getJDofContact(JDMid,true,false,true,true); // beginning of external (ant)umbral contact
				const double JDU4 = getJDofContact(JDMid,false,false,true,true); // end of external (ant)umbral contact
				SolarEclipseData(JDC1,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				if (!nonCentralEclipse)
				{
					// C1
					JD = getJDofContact(JDMid,true,false,false,true);
					JD = int(JD)+(int((JD-int(JD))*86400.)-1)/86400.;
					SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
					int steps = 0;
					while (pathWidth<0.0001 && steps<20)
					{
						JD += .1/86400.;
						SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
						steps += 1;
					}
					JDC1 = JD;
					core->setJD(JDC1);
					core->update(0);
					SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
					coordinates = getContactCoordinates(x,y,d,mu);
					double latC1 = coordinates.first;
					double lngC1 = coordinates.second;

					// Plot C1
					eclipseTime = localeMgr->getPrintableTimeLocal(JDC1);
					stream << "<Placemark>\n<name>"+q_("Central eclipse begins")+" ("+eclipseTime+")</name>\n<Point>\n<coordinates>";
					stream << lngC1 << "," << latC1 << ",0.0\n";
					stream << "</coordinates>\n</Point>\n</Placemark>\n";

					// C2
					JD = getJDofContact(JDMid,false,false,false,true);
					JD = int(JD)+(int((JD-int(JD))*86400.)+1)/86400.;
					SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
					steps = 0;
					while (pathWidth<0.0001 && steps<20)
					{
						JD -= .1/86400.;
						SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
						steps += 1;
					}
					JDC2 = JD;
					core->setJD(JDC2);
					core->update(0);
					SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
					coordinates = getContactCoordinates(x,y,d,mu);
					double latC2 = coordinates.first;
					double lngC2 = coordinates.second;

					// Plot C2
					eclipseTime = localeMgr->getPrintableTimeLocal(JDC2);
					stream << "<Placemark>\n<name>"+q_("Central eclipse ends")+" ("+eclipseTime+")</name>\n<Point>\n<coordinates>";
					stream << lngC2 << "," << latC2 << ",0.0\n";
					stream << "</coordinates>\n</Point>\n</Placemark>\n";

					// Center line
					JD = JDC1;
					int i = 0;
					double dRatioC1 = dRatio;
					SolarEclipseData(JDMid,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
					double dRatioMid = dRatio;
					SolarEclipseData(JDC2,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
					double dRatioC2 = dRatio;
					if (dRatioC1 >= 1. && dRatioMid >= 1. && dRatioC2 >= 1.)
						stream << "<Placemark>\n<name>Center line</name>\n<styleUrl>#Total</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
					else if (dRatioC1 < 1. && dRatioMid < 1. && dRatioC2 < 1.)
						stream << "<Placemark>\n<name>Center line</name>\n<styleUrl>#Annular</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
					else
						stream << "<Placemark>\n<name>Center line</name>\n<styleUrl>#Hybrid</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
					stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
					stream << lngC1 << "," << latC1 << ",0.0\n";
					while (JD+(1./1440.) < JDC2)
					{
						JD = JDC1 + i/1440.; // plot every one minute
						SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
						stream << lngDeg << "," << latDeg << ",0.0\n";
						i++;
					}
					stream << lngC2 << "," << latC2 << ",0.0\n";
					stream << "</coordinates>\n</LineString>\n</Placemark>\n";
				}
				else
				{
					JDC1 = JDMid;
					JDC2 = JDMid;
				}

				double dRatioC1 = dRatio;
				SolarEclipseData(JDMid,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				double dRatioMid = dRatio;
				SolarEclipseData(JDC2,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				double dRatioC2 = dRatio;
				// Umbra/antumbra outline
				// we want to draw (ant)umbral shadow on world map at exact times like 09:00, 09:10, 09:20, ...
				double beginJD = int(JDU1)+(10.*int(1440.*(JDU1-int(JDU1))/10.)+10.)/1440.;
				double endJD = int(JDU4)+(10.*int(1440.*(JDU4-int(JDU4))/10.))/1440.;
				JD = beginJD;
				int i = 0;
				double lat0 = 0., lon0 = 0.;
				while (JD < endJD)
				{
					JD = beginJD + i/144.; // plot every 10 minutes
					core->setJD(JD);
					core->update(0);
					SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
					SolarEclipseData(JD,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
					double angle = 0.;
					bool firstPoint = false;
					QString eclipseTime = localeMgr->getPrintableTimeLocal(JD);
					if (dRatio>=1.)
						stream << "<Placemark>\n<name>"+eclipseTime+"</name>\n<styleUrl>#Total</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
					else
						stream << "<Placemark>\n<name>"+eclipseTime+"</name>\n<styleUrl>#Annular</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
					stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
					int pointNumber = 0;
					while (pointNumber < 60)
					{
						angle = pointNumber*M_PI*2./60.;
						coordinates = getShadowOutlineCoordinates(angle,x,y,d,L2,tf2,mu);
						if (coordinates.first <= 90.)
						{
							stream << coordinates.second << "," << coordinates.first << ",0.0\n";
							if (!firstPoint)
							{
								lat0 = coordinates.first;
								lon0 = coordinates.second;
								firstPoint = true;
							}
						}
						pointNumber++;
					}
					stream << lon0 << "," << lat0 << ",0.0\n"; // completing the circle
					stream << "</coordinates>\n</LineString>\n</Placemark>\n";
					i++;
				}

				// Extreme northern/southern limits of umbra/antumbra at C1
				QPair<double, double> C1a = getExtremeNSLimitofShadow(JDC1,true,false,true);
				QPair<double, double> C1b = getExtremeNSLimitofShadow(JDC1,false,false,true);

				// Extreme northern/southern limits of umbra/antumbra at C2
				QPair<double, double> C2a = getExtremeNSLimitofShadow(JDC2,true,false,false);
				QPair<double, double> C2b = getExtremeNSLimitofShadow(JDC2,false,false,false);

				double dRatio,altitude,pathWidth,duration,magnitude;
				SolarEclipseData(JDC1,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);

				if (dRatioC1 >= 1. && dRatioMid >= 1. && dRatioC2 >= 1.)
					stream << "<Placemark>\n<name>Limit</name>\n<styleUrl>#Total</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
				else if (dRatioC1 < 1. && dRatioMid < 1. && dRatioC2 < 1.)
					stream << "<Placemark>\n<name>Limit</name>\n<styleUrl>#Annular</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
				else
					stream << "<Placemark>\n<name>Limit</name>\n<styleUrl>#Hybrid</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
				stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
				// 1st extreme limit at C1
				if (C1a.first <= 90. || C1b.first <= 90.)
				{
					if (dRatio>=1.)
						stream << C1a.second << "," << C1a.first << ",0.0\n";
					else
						stream << C1b.second << "," << C1b.first << ",0.0\n";
				}
				JD = JDC1-20./1440.;
				i = 0;
				while (JD < JDC2+20./1440.)
				{
					JD = JDC1+(i-20.)/1440.;
					coordinates = getNSLimitofShadow(JD,true,false);
					if (coordinates.first <= 90.)
						stream << coordinates.second << "," << coordinates.first << ",0.0\n";
					i++;
				}

				SolarEclipseData(JDC2,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				// 1st extreme limit at C2
				if (C2a.first <= 90. || C2b.first <= 90.)
				{
					if (dRatio>=1.)
						stream << C2a.second << "," << C2a.first << ",0.0\n";
					else
						stream << C2b.second << "," << C2b.first << ",0.0\n";
				}
				stream << "</coordinates>\n</LineString>\n</Placemark>\n";

				// 2nd extreme limit at C1
				if (dRatioC1 >= 1. && dRatioMid >= 1. && dRatioC2 >= 1.)
					stream << "<Placemark>\n<name>Limit</name>\n<styleUrl>#Total</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
				else if (dRatioC1 < 1. && dRatioMid < 1. && dRatioC2 < 1.)
					stream << "<Placemark>\n<name>Limit</name>\n<styleUrl>#Annular</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
				else
					stream << "<Placemark>\n<name>Limit</name>\n<styleUrl>#Hybrid</styleUrl>\n<LineString>\n<extrude>1</extrude>\n";
				stream << "<tessellate>1</tessellate>\n<altitudeMode>absoluto</altitudeMode>\n<coordinates>\n";
				SolarEclipseData(JDC1,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				if (C1a.first <= 90. || C1b.first <= 90.)
				{
					if (dRatio>=1.)
						stream << C1b.second << "," << C1b.first << ",0.0\n";
					else
						stream << C1a.second << "," << C1a.first << ",0.0\n";
				}
				JD = JDC1-20./1440.;
				i = 0;
				while (JD < JDC2+20./1440.)
				{
					JD = JDC1+(i-20.)/1440.;
					coordinates = getNSLimitofShadow(JD,false,false);
					if (coordinates.first <= 90.)
						stream << coordinates.second << "," << coordinates.first << ",0.0\n";
					i++;
				}
				SolarEclipseData(JDC2,dRatio,latDeg,lngDeg,altitude,pathWidth,duration,magnitude);
				// 2nd extreme limit at C2
				if (C2a.first <= 90. || C2b.first <= 90.)
				{
					if (dRatio>=1.)
						stream << C2b.second << "," << C2b.first << ",0.0\n";
					else
						stream << C2a.second << "," << C2a.first << ",0.0\n";
				}
				stream << "</coordinates>\n</LineString>\n</Placemark>\n";
			}
			stream << "</Document>\n</kml>\n";
			file.close();
			QGuiApplication::restoreOverrideCursor();
		}
		core->setJD(currentJD);
		core->setUseTopocentricCoordinates(saveTopocentric);
		core->update(0);
	}
}

QPair<double, double> AstroCalcDialog::getNSLimitofShadow(double JD, bool northernLimit, bool penumbra)
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	QPair<double, double> coordinates;
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	static const double ff = 1./(1.-f);
	double tf1,tf2,L;
	core->setJD(JD);
	core->update(0);
	double xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot;
	BesselParameters(xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot,penumbra);
	double x,y,d,L1,L2,mu,tf;
	SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
	const double rho1 = std::sqrt(1.-e2*std::cos(d)*std::cos(d));
	const double y1 = y/rho1;
	double eta1 = y1;
	const double sd1 = std::sin(d)/rho1;
	const double cd1 = std::sqrt(1.-e2)*std::cos(d)/rho1;
	const double rho2 = std::sqrt(1.-e2*std::sin(d)*std::sin(d));
	const double sd1d2 = e2*std::sin(d)*std::cos(d)/(rho1*rho2);
	const double cd1d2 = std::sqrt(1.-sd1d2*sd1d2);
	double zeta = rho2*(-eta1*sd1d2);
	if (penumbra)
	{
		L = L1;
		tf = tf1;
	}
	else
	{
		L = L2;
		tf = tf2;
	}
	double Lb = L-zeta*tf;
	double xidot = mudot*(-y*std::sin(d)+zeta*std::cos(d));
	double tq = -(ydot-etadot)/(xdot-xidot);
	double sq = std::sin(std::atan(tq));
	double cq = std::cos(std::atan(tq));
	if (!northernLimit)
	{
		sq *= -1.;
		cq *= -1.;
	}
	double xi = x-Lb*sq;
	eta1 = y1-Lb*cq/rho1;
	zeta = 1.-xi*xi-eta1*eta1;

	if (zeta < 0.)
	{
		coordinates.first = 99.;
		coordinates.second = 0.;
	}
	else
	{
		double zeta1 = std::sqrt(zeta);
		zeta = rho2*(zeta1*cd1d2-eta1*sd1d2);
		double adot = -ldot-mudot*x*tf*std::cos(d);
		double tq = (bdot-zeta*ddot-(adot/cq))/(cdot-zeta*mudot*std::cos(d));
		double Lb = L-zeta*tf;
		sq = std::sin(std::atan(tq));
		cq = std::cos(std::atan(tq));
		if (!northernLimit)
		{
			sq *= -1.;
			cq *= -1.;
		}
		xi = x-Lb*sq;
		eta1 = y1-Lb*cq/rho1;
		zeta = 1.-xi*xi-eta1*eta1;
		if (zeta < 0.)
		{
			coordinates.first = 99.;
			coordinates.second = 0.;
		}	
		else
		{
			zeta1 = std::sqrt(zeta);
			zeta = rho2*(zeta1*cd1d2-eta1*sd1d2);
			//tq = bdot-zeta*ddot-adot/cq;
			//tq = tq/(cdot-zeta*mudot*std::cos(d));
			double b = -eta1*sd1+zeta1*cd1;
			double lngDeg = StelUtils::fmodpos(std::atan2(xi,b)*M_180_PI - mu + 180., 360.) - 180.;
			double sfn1 = eta1*cd1+zeta1*sd1;
			double cfn1 = std::sqrt(1.-sfn1*sfn1);
			double latDeg = ff*sfn1/cfn1;
			coordinates.first = std::atan(latDeg)*M_180_PI;
			coordinates.second = lngDeg;
		}
	}
	return coordinates;
}

QPair<double, double> AstroCalcDialog::getExtremeNSLimitofShadow(double JD, bool northernLimit, bool penumbra, bool begin)
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	QPair<double, double> coordinates;
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	static const double ff = 1./(1.-f);
	core->setJD(JD+0.1);
	core->update(0);
	double xdot,ydot,ddot,mudot,ldot,etadot,bdot1,cdot1,bdot2,cdot2;
	BesselParameters(xdot,ydot,ddot,mudot,ldot,etadot,bdot1,cdot1,penumbra);
	core->setJD(JD-0.1);
	core->update(0);
	BesselParameters(xdot,ydot,ddot,mudot,ldot,etadot,bdot2,cdot2,penumbra);
	const double bdd = 5.*(bdot1-bdot2);
	const double cdd = 5.*(cdot1-cdot2);
	core->setJD(JD);
	core->update(0);
	double x,y,d,tf1,tf2,L1,L2,mu;
	SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
	double bdot,cdot,xidot;
	BesselParameters(xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot,penumbra);
	double e = std::sqrt(bdot*bdot+cdot*cdot);
	double rho1 = std::sqrt(1-e2*std::cos(d)*std::cos(d));
	double scq = e/cdot;
	double tq = bdot/cdot;
	double cq = 1./scq;
	const double L = penumbra ? L1 : L2;
	if (northernLimit)
	{
		if (L<0.)
			cq = std::abs(cq);
		else
			cq = -std::abs(cq);
	}
	else
	{
		if (L<0.)
			cq = -std::abs(cq);
		else
			cq = std::abs(cq);
	}
	double sq = tq*cq;
	if (cq>0.)
	{
		xidot = xdot-L*bdd/e;
		etadot = (ydot-L*cdd/e)/rho1;
	}
	else
	{
		xidot = xdot+L*bdd/e;
		etadot = (ydot+L*cdd/e)/rho1;
	}
	const double n2 = xidot*xidot+etadot*etadot;
	double xi = x-L*sq;
	double eta = (y-L*cq)/rho1;
	double szi = (xi*etadot-xidot*eta)/std::sqrt(n2);
	if (std::abs(szi)<=1.)
	{
		double czi = -std::sqrt(1.-szi*szi);
		if (!begin) czi *= -1.;
		double tc = (czi/std::sqrt(n2))-(xi*xidot+eta*etadot)/n2;
		core->setJD(JD+tc/24.);
		core->update(0);
		SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
		BesselParameters(xdot,ydot,ddot,mudot,ldot,etadot,bdot,cdot,penumbra);
		//tq = bdot/cdot;
		e = std::sqrt(bdot*bdot+cdot*cdot);
		rho1 = std::sqrt(1.-e2*std::cos(d)*std::cos(d));
		scq = e/cdot;
		tq = bdot/cdot;
		cq = 1./scq;
		const double L = penumbra ? L1 : L2;
		if (northernLimit)
		{
			if (L<0.)
				cq = std::abs(cq);
			else
				cq = -std::abs(cq);
		}
		else
		{
			if (L<0.)
				cq = -std::abs(cq);
			else
				cq = std::abs(cq);
		}
		sq = tq*cq;
		// clazy warns n2 below and therefore this all is unused!
		//if (cq>0.)
		//{
		//	xidot = xdot-L*bdd/e;
		//	etadot = (ydot-L*cdd/e)/rho1;
		//}
		//else
		//{
		//	xidot = xdot+L*bdd/e;
		//	etadot = (ydot+L*cdd/e)/rho1;
		//}
		//n2 = xidot*xidot+etadot*etadot;
		xi = x-L*sq;
		eta = (y-L*cq)/rho1;
		double sd1 = std::sin(d)/rho1;
		double cd1 = std::sqrt(1.-e2)*std::cos(d)/rho1;
		double b = -eta*sd1;
		double lngDeg = StelUtils::fmodpos(std::atan2(xi,b)*M_180_PI - mu +180., 360.) - 180.;
		double sfn1 = eta*cd1;
		double cfn1 = std::sqrt(1.-sfn1*sfn1);
		double latDeg = ff*sfn1/cfn1;
		coordinates.first = std::atan(latDeg)*M_180_PI;
		coordinates.second = lngDeg;
	}
	else
	{
		coordinates.first = 99.;
		coordinates.second = 0.;
	}
	return coordinates;
}

QPair<double, double> AstroCalcDialog::getContactCoordinates(double x, double y, double d, double mu)
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	QPair<double, double> coordinates;
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	static const double f = 1.0 - ssystem->getEarth()->getOneMinusOblateness(); // flattening
	static const double e2 = f*(2.-f);
	static const double ff = 1./(1.-f);
	const double rho1 = std::sqrt(1.-e2*std::cos(d)*std::cos(d));
	const double yy1 = y/rho1;
	const double m1 = std::sqrt(x*x+yy1*yy1);
	const double eta1 = yy1/m1;
	const double sd1 = std::sin(d)/rho1;
	const double cd1 = std::sqrt(1.-e2)*std::cos(d)/rho1;
	const double theta = std::atan2(x/m1,-eta1*sd1)*M_180_PI;
	double lngDeg = StelUtils::fmodpos(theta - mu + 180., 360.) - 180.;
	double latDeg = ff*std::tan(std::asin(eta1*cd1));
	coordinates.first = std::atan(latDeg)*M_180_PI;
	coordinates.second = lngDeg;
	return coordinates;
}

void AstroCalcDialog::cleanupSolarEclipsesLocal()
{
	ui->solareclipselocalTreeWidget->clear();
	enableSolarEclipsesLocalButtons(false);
}

void AstroCalcDialog::enableSolarEclipsesLocalButtons(bool enable)
{
	ui->solareclipseslocalCleanupButton->setEnabled(enable);
	ui->solareclipseslocalSaveButton->setEnabled(enable);
}

void AstroCalcDialog::selectCurrentSolarEclipseLocal(const QModelIndex& modelIndex)
{
	// Find the Sun
	const double JD = modelIndex.sibling(modelIndex.row(), SolarEclipseLocalDate).data(Qt::UserRole).toDouble();
	goToObject("Sun", JD);
}

void AstroCalcDialog::saveSolarEclipsesLocal()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save calculated solar eclipses as..."), "solareclipses-local");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->solareclipselocalTreeWidget, solareclipselocalHeader);
	else
		saveTableAsXLSX(fileData.first, ui->solareclipselocalTreeWidget, solareclipselocalHeader, q_("Solar Eclipses"), q_("Solar Eclipses"), q_("Note: Local circumstances for eclipses during thousands of years in the past and future are not reliable due to uncertainty in ΔT which is caused by fluctuations in Earth's rotation."));
}

void AstroCalcDialog::setTransitHeaderNames()
{
	transitHeader = QStringList({
		qc_("Date of mid-transit", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses/Transits tool
		q_("Planet"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses/Transits tool
		qc_("Exterior Ingress", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses/Transits tool
		qc_("Interior Ingress", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses/Transits tool
		qc_("Mid-transit", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses/Transits tool
		qc_("Angular Distance", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses/Transits tool
		qc_("Interior Egress", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses/Transits tool
		qc_("Exterior Egress", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses/Transits tool
		qc_("Duration", "column name"),
		// TRANSLATORS: The name of column in AstroCalc/Eclipses/Transits tool
		qc_("Observable Duration", "column name")});
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
	ui->transitTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
}

// Local transit parameters
struct LocalTransitparams {
	double dt;
	double L1;
	double L2;
	double ce;
	double magnitude;
	double altitude;
};

LocalTransitparams localTransit(double JD, int contact, bool central, PlanetP object, bool topocentric) {
	LocalTransitparams result;
	// contact : -1 for beginning, 0 for mid-transit, 1 for the ending
	// central : true for full transit

	// Besselian elements
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)

	StelCore* core = StelApp::getInstance().getCore();
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	const double lat = static_cast<double>(core->getCurrentLocation().getLatitude());
	const double lon = static_cast<double>(core->getCurrentLocation().getLongitude());
	const double elevation = static_cast<double>(core->getCurrentLocation().altitude);
	double rc = 0., rs = 0.;
	if (topocentric)
	{
		Vec4d geocentricCoords = ssystem->getEarth()->getRectangularCoordinates(lon,lat,elevation);
		static const double earthRadius = ssystem->getEarth()->getEquatorialRadius();
		rc = geocentricCoords[0]/earthRadius; // rhoCosPhiPrime
		rs = geocentricCoords[1]/earthRadius; // rhoSinPhiPrime
	}
	core->setUseTopocentricCoordinates(false);
	core->setJD(JD);
	core->update(0);

	double x,y,d,tf1,tf2,L1,L2,mu;
	TransitBessel(object,x,y,d,tf1,tf2,L1,L2,mu);

	core->setJD(JD - 5./1440.);
	core->update(0);
	double x1,y1,d1,mu1;
	TransitBessel(object,x1,y1,d1,tf1,tf2,L1,L2,mu1);

	core->setJD(JD + 5./1440.);
	core->update(0);
	double x2,y2,d2,mu2;
	TransitBessel(object,x2,y2,d2,tf1,tf2,L1,L2,mu2);

	// Hourly rate of changes
	const double xdot = (x2 - x1) * 6.;
	const double ydot = (y2 - y1) * 6.;
	const double ddot = (d2 - d1) * 6.;
	double mudot = mu2 - mu1;
	if (mudot < 0.) mudot += 360.; // make sure it is positive in case mu2 < mu1
	mudot = mudot * 6. * M_PI_180;
	double theta = (mu + lon) * M_PI_180;
	theta = StelUtils::fmodpos(theta, 2.*M_PI);
	const double xi = rc*sin(theta);
	const double eta = rs*cos(d)-rc*sin(d)*cos(theta);
	const double zeta = rs*sin(d)+rc*cos(d)*cos(theta);
	const double xidot = mudot * rc * cos(theta);
	const double etadot = mudot*xi*sin(d)-zeta*ddot;
	const double u = x - xi;
	const double v = y - eta;
	const double udot = xdot - xidot;
	const double vdot = ydot - etadot;
	const double n2 = udot * udot + vdot * vdot;
	const double delta = (u * vdot - udot * v) / sqrt(udot * udot + vdot * vdot);
	L1 -= zeta * tf1;
	L2 -= zeta * tf2;
	const double L = central ? L2 : L1;
	const double sfi = delta/L;
	const double ce = 1.- sfi*sfi;
	const double cfi = (ce > 0.) ? contact * sqrt(ce) : 0.;
	const double m = sqrt(u * u + v * v);
	const double magnitude = (L1 - m) / (L1 + L2);
	const double altitude = asin(rc*cos(d)*cos(theta)+rs*sin(d)) / M_PI_180;
	const double dt = (L * cfi / sqrt(udot * udot + vdot * vdot)) - (u * udot + v * vdot) / n2;
	result.dt = dt;
	result.L1 = L1;
	result.L2 = L2;
	result.ce = ce;
	result.magnitude = magnitude;
	result.altitude = altitude;
	return result;
}

TransitBessel::TransitBessel(PlanetP object, double &besX, double &besY,
	double &besD, double &bestf1, double &bestf2, double &besL1, double &besL2, double &besMu)
{
	// Besselian elements (adapted from solar eclipse)
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)

	StelCore* core = StelApp::getInstance().getCore();
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	core->setUseTopocentricCoordinates(false);
	core->update(0);

	double raPlanet, dePlanet, raSun, deSun;
	StelUtils::rectToSphe(&raSun, &deSun, ssystem->getSun()->getEquinoxEquatorialPos(core));
	StelUtils::rectToSphe(&raPlanet, &dePlanet, object->getEquinoxEquatorialPos(core));

	const double sdistanceAu = ssystem->getSun()->getEquinoxEquatorialPos(core).norm();
	static const double earthRadius = ssystem->getEarth()->getEquatorialRadius()*AU;
	// Planet's distance in Earth's radius
	const double pdistanceER = object->getEquinoxEquatorialPos(core).norm() * AU / earthRadius;
	// Greenwich Apparent Sidereal Time
	const double gast = get_apparent_sidereal_time(core->getJD(), core->getJDE());
	// Avoid bug for special cases happen around Vernal Equinox
	double raDiff = StelUtils::fmodpos(raPlanet-raSun, 2.*M_PI);
	if (raDiff>M_PI) raDiff-=2.*M_PI;

	static constexpr double SunEarth = 109.12278; // ratio of Sun-Earth radius : 109.12278 = 696000/6378.1366
	const double rss = sdistanceAu * 23454.7925; // from 1 AU/Earth's radius : 149597870.8/6378.1366
	const double b = pdistanceER / rss;
	const double a = raSun - ((b * cos(dePlanet) * raDiff) / ((1 - b) * cos(deSun)));
	besD = deSun - (b * (dePlanet - deSun) / (1 - b));
	besX = cos(dePlanet) * sin((raPlanet - a));
	besX *= pdistanceER;
	besY = cos(besD) * sin(dePlanet);
	besY -= cos(dePlanet) * sin(besD) * cos((raPlanet - a));
	besY *= pdistanceER;
	double z = sin(dePlanet) * sin(besD);
	z += cos(dePlanet) * cos(besD) * cos((raPlanet - a));
	z *= pdistanceER;

	// Ratio of Planet/Earth's radius
	double k = object->getEquatorialRadius()/ssystem->getEarth()->getEquatorialRadius();
	// Parameters of the shadow cone
	const double f1 = asin((SunEarth + k) / (rss * (1. - b)));
	bestf1 = tan(f1);
	const double f2 = asin((SunEarth - k) / (rss * (1. - b)));  
	bestf2 = tan(f2);
	besL1 = z * bestf1 + (k / cos(f1));
	besL2 = z * bestf2 - (k / cos(f2));
	besMu = gast - a * M_180_PI;
	besMu = StelUtils::fmodpos(besMu, 360.);
};

void AstroCalcDialog::generateTransits()
{
	const bool onEarth = core->getCurrentPlanet()==solarSystem->getEarth();
	if (onEarth)
	{
		initListTransit();
		const double currentJD = core->getJD(); // save current JD
		const bool saveTopocentric = core->getUseTopocentricCoordinates();
		const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
		for (int p = 0; p < 2; p++)
		{
			double startyear = ui->eclipseFromYearSpinBox->value();
			double years = ui->eclipseYearsSpinBox->value();
			double startJD, stopJD;
			StelUtils::getJDFromDate(&startJD, startyear, 1, 1, 0, 0, 0);
			StelUtils::getJDFromDate(&stopJD, startyear+years, 12, 31, 23, 59, 59);
			startJD -= core->getUTCOffset(startJD) / 24.;
			stopJD  -= core->getUTCOffset(stopJD)  / 24.;
			QString planetStr, separationStr, durationStr, observableDurationStr;
			double approxJD, synodicPeriod;
			if (p == 0)
			{
				 // Mercury
				approxJD = 2451612.023;
				synodicPeriod = 115.8774771;
			}
			else
			{
				// Venus
				approxJD = 2451996.706;
				synodicPeriod = 583.921361;
			}
			int elements = static_cast<int>((stopJD - startJD) / synodicPeriod);
			// Find approximate JD of Inferior conjunction
			double tmp = (startJD - approxJD - synodicPeriod) / synodicPeriod;
			double InitJD = approxJD + int(tmp) * synodicPeriod;
			
			// Search for transits at each inferior conjunction
			for (int i = 0; i <= elements+2; i++)
			{
				double JD = InitJD + synodicPeriod * i;
				if (JD > startJD)
				{
					core->setUseTopocentricCoordinates(false);
					core->update(0);

					// Find exact time of minimum distance between Mercury/Venus and the Sun
					double dt = 1.;
					int iteration = 0;
					PlanetP object;
					if (p==0)
					{
						object = GETSTELMODULE(SolarSystem)->searchByEnglishName("Mercury");
						planetStr = q_("Mercury");
					}
					else
					{
						object = GETSTELMODULE(SolarSystem)->searchByEnglishName("Venus");
						planetStr = q_("Venus");
					}

					while (abs(dt)>(0.1/86400.) && (iteration < 20)) // 0.1 second of accuracy
					{
						core->setJD(JD);
						core->update(0);
						double x,y,d,tf1,tf2,L1,L2,mu;
						TransitBessel(object,x,y,d,tf1,tf2,L1,L2,mu);

						core->setJD(JD - 5./1440.);
						core->update(0);
						double x1,y1;
						TransitBessel(object,x1,y1,d,tf1,tf2,L1,L2,mu);

						core->setJD(JD + 5./1440.);
						core->update(0);
						double x2,y2;
						TransitBessel(object,x2,y2,d,tf1,tf2,L1,L2,mu);

						double xdot1 = (x - x1) * 12.;
						double xdot2 = (x2 - x) * 12.;
						double xdot = (xdot1 + xdot2) / 2.;
						double ydot1 = (y - y1) * 12.;
						double ydot2 = (y2 - y) * 12.;
						double ydot = (ydot1 + ydot2) / 2.;
						double n2 = xdot * xdot + ydot * ydot;
						dt  = -(x * xdot + y * ydot) / n2;
						JD += dt / 24.;
						iteration += 1;
					}
					core->setJD(JD);
					core->update(0);
					
					double x,y,d,tf1,tf2,L1,L2,mu;
					TransitBessel(object,x,y,d,tf1,tf2,L1,L2,mu);
					double gamma = sqrt(x * x + y * y);
					if (gamma <= (0.9972+L1) && (JD <= stopJD)) // Transit occurs on this JD
					{
						double dt = 1.;
						int iteration = 0;
						double JDMid = JD;
						double az, altitudeMidtransit = -1.;
						double altitudeContact1 = -1., altitudeContact2 = -1., altitudeContact3 = -1., altitudeContact4 = -1.;
						double JD1 = 0., JD2 = 0., JD3 = 0., JD4 = 0., JDc1 = 0., JDc4 = 0.;
						double transitMagnitude;
						Vec4d rts;
						// Time of mid-transit
						LocalTransitparams transitData = localTransit(JD,0,false,object,saveTopocentric);
						while (abs(dt) > 0.000001 && (iteration < 20))
						{
							transitData = localTransit(JD,0,false,object,saveTopocentric);
							dt = transitData.dt;
							JD += dt / 24.;
							iteration += 1;
						}
						JDMid = JD;
						transitMagnitude = transitData.magnitude;
						core->setJD(JDMid);
						core->update(0);
						StelUtils::rectToSphe(&az,&altitudeMidtransit,object->getAltAzPosAuto(core));

						if (transitMagnitude > 0.)
						{
							// 1st contact = Exterior Ingress
							iteration = 0;
							JD1 = JDMid;
							transitData = localTransit(JD1,-1,false,object,saveTopocentric);
							dt = transitData.dt;
							while (abs(dt) > 0.000001 && (iteration < 20))
							{
								transitData = localTransit(JD1,-1,false,object,saveTopocentric);
								dt = transitData.dt;
								JD1 += dt / 24.;
								iteration += 1;
							}
							core->setJD(JD1);
							core->update(0);
							StelUtils::rectToSphe(&az,&altitudeContact1,object->getAltAzPosAuto(core));

							// 4th contact = Exterior Egress
							iteration = 0;
							JD4 = JDMid;
							transitData = localTransit(JD4,1,false,object,saveTopocentric);
							dt = transitData.dt;
							while (abs(dt) > 0.000001 && (iteration < 20))
							{
								transitData = localTransit(JD4,1,false,object,saveTopocentric);
								dt = transitData.dt;
								JD4 += dt / 24.;
								iteration += 1;
							}
							core->setJD(JD4);
							core->update(0);
							StelUtils::rectToSphe(&az,&altitudeContact4,object->getAltAzPosAuto(core));
							JDc1=JD1;
							JDc4=JD4;

							// 2nd contact = Interior Ingress
							iteration = 0;
							JD2 = JDMid;
							transitData = localTransit(JD2,-1,true,object,saveTopocentric);
							dt = transitData.dt;
							while (abs(dt) > 0.000001 && (iteration < 20))
							{
								transitData = localTransit(JD2,-1,true,object,saveTopocentric);
								dt = transitData.dt;
								JD2 += dt / 24.;
								iteration += 1;
							}
							core->setJD(JD2);
							core->update(0);
							StelUtils::rectToSphe(&az,&altitudeContact2,object->getAltAzPosAuto(core));
							// 3rd contact = Interior Egress
							iteration = 0;
							JD3 = JDMid;
							transitData = localTransit(JD3,1,true,object,saveTopocentric);
							dt = transitData.dt;
							while (abs(dt) > 0.000001 && (iteration < 20))
							{
								transitData = localTransit(JD3,1,true,object,saveTopocentric);
								dt = transitData.dt;
								JD3 += dt / 24.;
								iteration += 1;
							}
							core->setJD(JD3);
							core->update(0);
							StelUtils::rectToSphe(&az,&altitudeContact3,object->getAltAzPosAuto(core));

							// Transit in progress at sunrise/sunset
							if (saveTopocentric && transitMagnitude > 0.)
							{
								if (altitudeContact1 < 0. && altitudeContact4 > 0.) // Transit in progress at sunrise
								{
									// find rising time of planet
									for (int j = 0; j <= 5; j++)
									{
										transitData = localTransit(JD - 5./1440.,0,false,object,true);
										double alt1 = transitData.altitude+.3;
										transitData = localTransit(JD + 5./1440.,0,false,object,true);
										double alt2 = transitData.altitude+.3;
										double dt = .006944444 * alt1 / (alt2 - alt1);
										JD = JD - 5./1440. - dt;
										transitData = localTransit(JD,0,false,object,true);
									}
									core->setJD(JD);
									core->update(0);
									rts = object->getRTSTime(core);
									JD = rts[0];
									if (JD<JD1) // make sure it's after C1
									{
										core->setJD(JD+1.);
										core->update(0);
										rts = object->getRTSTime(core);
									}
									else if (JD>JD4) // make sure it's before C4
									{
										core->setJD(JD-1.);
										core->update(0);
										rts = object->getRTSTime(core);
									}
									JDc1 = rts[0];
								}

								if (altitudeContact1 > 0. && altitudeContact4 < 0.) // Transit in progress at sunset
								{
									// find setting time of planet
									for (int j = 0; j <= 5; j++)
									{
										transitData = localTransit(JD - 5./1440.,0,false,object,true);
										double alt1 = transitData.altitude+.3;
										transitData = localTransit(JD + 5./1440.,0,false,object,true);
										double alt2 = transitData.altitude+.3;
										double dt = .006944444 * alt1 / (alt2 - alt1);
										JD = JD - 5./1440. - dt;
										transitData = localTransit(JD,0,false,object,true);
									}
									core->setJD(JD);
									core->update(0);
									rts = object->getRTSTime(core);
									JD = rts[2];
									if (JD<JD1) // make sure it's after C1
									{
										core->setJD(JD+1.);
										core->update(0);
										rts = object->getRTSTime(core);
									}
									else if (JD>JD4) // make sure it's before C4
									{
										core->setJD(JD-1.);
										core->update(0);
										rts = object->getRTSTime(core);
									}
									JDc4 = rts[2];
								}
							}
						}
						const double shift = core->getUTCOffset(JDMid)/24.;
						ACTransitTreeWidgetItem* treeItem = new ACTransitTreeWidgetItem(ui->transitTreeWidget);
						treeItem->setText(TransitDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JDMid), localeMgr->getPrintableTimeLocal(JDMid))); // local date and time
						treeItem->setData(TransitDate, Qt::UserRole, JDMid);
						treeItem->setText(TransitPlanet, planetStr);
						treeItem->setData(TransitPlanet, Qt::UserRole, planetStr);

						if (transitMagnitude > 0.)
						{
							if (saveTopocentric && altitudeContact1 < 0.)
								{
									treeItem->setText(TransitContact1, QString("(%1)").arg(localeMgr->getPrintableTimeLocal(JD1)));
#if (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
									treeItem->setForeground(TransitContact1, Qt::gray);
#else
									treeItem->setTextColor(TransitContact1, Qt::gray);
#endif
								}
							else
								treeItem->setText(TransitContact1, QString("%1").arg(localeMgr->getPrintableTimeLocal(JD1)));
						}
						else
							treeItem->setText(TransitContact1, dash);
						treeItem->setData(TransitContact1, Qt::UserRole, StelUtils::getHoursFromJulianDay(JD1 + shift));
						treeItem->setToolTip(TransitContact1, q_("The time of first contact, the instant when the planet's disk is externally tangent to the Sun (transit begins)"));
						if (transitData.ce <= 0.)
								treeItem->setText(TransitContact2, dash);
						else if (saveTopocentric && altitudeContact2 < 0.)
						{
							treeItem->setText(TransitContact2, QString("(%1)").arg(localeMgr->getPrintableTimeLocal(JD2)));
#if (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
							treeItem->setForeground(TransitContact2, Qt::gray);
#else
							treeItem->setTextColor(TransitContact2, Qt::gray);
#endif
						}
						else
							treeItem->setText(TransitContact2, QString("%1").arg(localeMgr->getPrintableTimeLocal(JD2)));
						treeItem->setData(TransitContact2, Qt::UserRole, StelUtils::getHoursFromJulianDay(JD2 + shift));
						treeItem->setToolTip(TransitContact2, q_("The time of second contact, the entire disk of the planet is internally tangent to the Sun"));
						if (transitMagnitude > 0.)
						{
							if (saveTopocentric && altitudeMidtransit < 0.)
								{
									treeItem->setText(TransitMid, QString("(%1)").arg(localeMgr->getPrintableTimeLocal(JDMid)));
#if (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
									treeItem->setForeground(TransitMid, Qt::gray);
#else
									treeItem->setTextColor(TransitMid, Qt::gray);
#endif
								}
							else
								treeItem->setText(TransitMid, QString("%1").arg(localeMgr->getPrintableTimeLocal(JDMid)));
						}
						else
							treeItem->setText(TransitMid, dash);
						treeItem->setData(TransitMid, Qt::UserRole, StelUtils::getHoursFromJulianDay(JDMid + shift));
						treeItem->setToolTip(TransitMid, q_("The time of minimum angular distance of planet to Sun's center"));
						core->setUseTopocentricCoordinates(saveTopocentric);
						core->setJD(JDMid);
						core->update(0);
						double elongation = object->getElongation(core->getObserverHeliocentricEclipticPos());
						if (withDecimalDegree)
							separationStr = StelUtils::radToDecDegStr(elongation, 5, false, true);
						else
							separationStr = StelUtils::radToDmsStr(elongation, true);
						treeItem->setText(TransitSeparation, separationStr);
						if ((saveTopocentric && altitudeMidtransit < 0.) || transitMagnitude < 0.) 
#if (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
							treeItem->setForeground(TransitSeparation, Qt::gray);
#else
							treeItem->setTextColor(TransitSeparation, Qt::gray);
#endif
						treeItem->setData(TransitSeparation, Qt::UserRole, elongation);
						treeItem->setToolTip(TransitSeparation, q_("Minimum angular distance of planet to Sun's center"));
						if (transitData.ce <= 0.)
								treeItem->setText(TransitContact3, dash);
						else if (saveTopocentric && altitudeContact3 < 0.)
						{
							treeItem->setText(TransitContact3, QString("(%1)").arg(localeMgr->getPrintableTimeLocal(JD3)));
#if (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
							treeItem->setForeground(TransitContact3, Qt::gray);
#else
							treeItem->setTextColor(TransitContact3, Qt::gray);
#endif
						}
						else
							treeItem->setText(TransitContact3, QString("%1").arg(localeMgr->getPrintableTimeLocal(JD3)));
						treeItem->setData(TransitContact3, Qt::UserRole, StelUtils::getHoursFromJulianDay(JD3 + shift));
						treeItem->setToolTip(TransitContact3, q_("The time of third contact, the planet reaches the opposite limb and is once again internally tangent to the Sun"));
						if (transitMagnitude > 0.)
						{
							if (saveTopocentric && altitudeContact4 < 0.)
							{
								treeItem->setText(TransitContact4, QString("(%1)").arg(localeMgr->getPrintableTimeLocal(JD4)));
#if (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
								treeItem->setForeground(TransitContact4, Qt::gray);
#else
								treeItem->setTextColor(TransitContact4, Qt::gray);
#endif
							}
							else
								treeItem->setText(TransitContact4, QString("%1").arg(localeMgr->getPrintableTimeLocal(JD4)));
						}
						else
							treeItem->setText(TransitContact4, dash);
						treeItem->setData(TransitContact4, Qt::UserRole, StelUtils::getHoursFromJulianDay(JD4 + shift));
						treeItem->setToolTip(TransitContact4, q_("The time of fourth contact, the planet's disk is externally tangent to the Sun (transit ends)"));
						double totalDuration = 0.;
						if (transitMagnitude > 0.)
						{
							totalDuration = (JD4-JD1)*24.;
							durationStr = StelUtils::hoursToHmsStr(totalDuration,true);
							treeItem->setText(TransitDuration, durationStr);
						}
						else
							treeItem->setText(TransitDuration, dash);
						treeItem->setData(TransitDuration, Qt::UserRole, totalDuration);
						treeItem->setToolTip(TransitDuration, q_("Total duration of transit"));
						// Observable duration (rise/set are taken into account)
						double observableDuration = 0.;
						if (saveTopocentric && transitMagnitude > 0.)
						{
							if (altitudeContact1 < 0. && altitudeContact4 < 0.)
							{
								observableDurationStr = dash;
								// Special case: C1,C4 occur below horizon but visible around mid-transit
								// Example: 2019 November 11 at Lat. +70, Long. -55
								if (altitudeMidtransit > 0.)
								{
									core->setJD(JDMid);
									core->update(0);
									rts = object->getRTSTime(core);
									if (rts[0]>JD1 && rts[2]<JD4)
									{
										observableDuration = (rts[2]-rts[0])*24.;
										observableDurationStr = StelUtils::hoursToHmsStr(observableDuration,true);
									}
								}
							}
							else if (altitudeContact1 > 0. && altitudeMidtransit < 0. && altitudeContact4 > 0.)
							{
								// Special case: C1,C4 occur above horizon but invisible around mid-transit
								// Example: 2012 June 5/6 Lat. +60, Long. -20
								// Overall duration is combination of two separate durations 
								core->setJD(JD1);
								core->update(0);
								rts = object->getRTSTime(core);
								if (rts[2]>JD1)
									observableDuration = (rts[2]-JD1)*24.;
								core->setJD(JD4);
								core->update(0);
								rts = object->getRTSTime(core);
								if (JD4>rts[0])
									observableDuration += (JD4-rts[0])*24.;
								observableDurationStr = StelUtils::hoursToHmsStr(observableDuration,true);
							}
							else
							{
								observableDuration = (JDc4-JDc1)*24.;
								observableDurationStr = StelUtils::hoursToHmsStr(observableDuration,true);
							}
						}
						else
						{
							observableDurationStr = dash; // geocentric - no observable duration
						}
						treeItem->setText(TransitObservableDuration, observableDurationStr);
						treeItem->setData(TransitObservableDuration, Qt::UserRole, observableDuration);
						treeItem->setToolTip(TransitObservableDuration, q_("Observable duration of transit"));
						treeItem->setTextAlignment(TransitDate, Qt::AlignRight);
						treeItem->setTextAlignment(TransitPlanet, Qt::AlignRight);
						treeItem->setTextAlignment(TransitContact1, Qt::AlignCenter);
						treeItem->setTextAlignment(TransitContact2, Qt::AlignCenter);
						treeItem->setTextAlignment(TransitMid, Qt::AlignCenter);
						treeItem->setTextAlignment(TransitSeparation, Qt::AlignCenter);
						treeItem->setTextAlignment(TransitContact3, Qt::AlignCenter);
						treeItem->setTextAlignment(TransitContact4, Qt::AlignCenter);
						treeItem->setTextAlignment(TransitDuration, Qt::AlignCenter);
						treeItem->setTextAlignment(TransitObservableDuration, Qt::AlignCenter);
					}
				}
			}
		}
		core->setJD(currentJD);
		core->setUseTopocentricCoordinates(saveTopocentric);
		core->update(0); // enforce update

		// adjust the column width
		for (int i = 0; i < TransitCount; ++i)
		{
			ui->transitTreeWidget->resizeColumnToContents(i);
		}

		// sort-by-date
		ui->transitTreeWidget->sortItems(TransitDate, Qt::AscendingOrder);
		enableTransitsButtons(true);
	}
	else
		cleanupTransits();
}

void AstroCalcDialog::enableRTSButtons(bool enable)
{
	ui->rtsCleanupButton->setEnabled(enable);
	ui->rtsSaveButton->setEnabled(enable);
}

void AstroCalcDialog::cleanupTransits()
{
	ui->transitTreeWidget->clear();
	enableTransitsButtons(false);
}

void AstroCalcDialog::enableTransitsButtons(bool enable)
{
	ui->transitsCleanupButton->setEnabled(enable);
	ui->transitsSaveButton->setEnabled(enable);
}

void AstroCalcDialog::selectCurrentTransit(const QModelIndex& modelIndex)
{
	// Find the planet
	const QString name = modelIndex.sibling(modelIndex.row(), TransitPlanet).data(Qt::UserRole).toString();
	const double JD = modelIndex.sibling(modelIndex.row(), TransitDate).data(Qt::UserRole).toDouble();
	goToObject(name, JD);
}

void AstroCalcDialog::saveTransits()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save calculated transits as..."), "transits");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->transitTreeWidget, transitHeader);
	else
		saveTableAsXLSX(fileData.first, ui->transitTreeWidget, transitHeader, q_("Transits across the Sun"), q_("Transits across the Sun"), q_("Notes: Time in parentheses means the contact is invisible at current location. Transit times during thousands of years in the past and future are not reliable due to uncertainty in ΔT which is caused by fluctuations in Earth's rotation."));
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
		indexP = qMax(0, planets->findData(conf->value("astrocalc/ephemeris_celestial_body", "Moon").toString(), Qt::UserRole, Qt::MatchCaseSensitive));
	planets->setCurrentIndex(indexP);
	planets->model()->sort(0);

	// Restore the selection
	indexP2 = planets2->findData(selectedPlanet2Id, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexP2 < 0)
		indexP2 = qMax(0, planets2->findData(conf->value("astrocalc/ephemeris_second_celestial_body", "none").toString(), Qt::UserRole, Qt::MatchCaseSensitive));
	planets2->setCurrentIndex(indexP2);
	planets2->model()->sort(0);

	indexG = graphsp->findData(selectedGraphsPId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexG < 0)
		indexG = qMax(0, graphsp->findData(conf->value("astrocalc/graphs_celestial_body", "Moon").toString(), Qt::UserRole, Qt::MatchCaseSensitive));
	graphsp->setCurrentIndex(indexG);
	graphsp->model()->sort(0);

	indexFCB = firstCB->findData(selectedFirstCelestialBodyId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexFCB < 0)
		indexFCB = qMax(0, firstCB->findData(conf->value("astrocalc/first_celestial_body", "Sun").toString(), Qt::UserRole, Qt::MatchCaseSensitive));
	firstCB->setCurrentIndex(indexFCB);
	firstCB->model()->sort(0);

	indexSCB = secondCB->findData(selectedSecondCelestialBodyId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexSCB < 0)
		indexSCB = qMax(0, secondCB->findData(conf->value("astrocalc/second_celestial_body", "Earth").toString(), Qt::UserRole, Qt::MatchCaseSensitive));
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

void AstroCalcDialog::updateGraphsStep(int step)
{
	if (graphsStep!=step)
	{
		graphsStep = step;
		conf->setValue("astrocalc/graphs_step", step);
	}
}

void AstroCalcDialog::populateEphemerisTimeStepsList()
{
	typedef QPair<QString, QString> itemPairs;
	const QList<itemPairs> items = {
		{q_("1 minute"), "38"}, {q_("10 minutes"), "1"}, {q_("30 minutes"), "2"}, {q_("1 hour"), "3"}, {q_("6 hours"), "4"}, {q_("12 hours"), "5"},
		{q_("1 solar day"), "6"}, {q_("5 solar days"), "7"}, {q_("10 solar days"), "8"}, {q_("15 solar days"), "9"}, {q_("30 solar days"), "10"},
		{q_("60 solar days"), "11"}, {q_("100 solar days"), "24"},{q_("500 solar days"), "37"},{q_("1 sidereal day"), "18"},{q_("5 sidereal days"), "19"},
		{q_("10 sidereal days"), "20"},{q_("15 sidereal days"), "21"},{q_("30 sidereal days"), "22"},{q_("60 sidereal days"), "23"},{q_("100 sidereal days"), "25"},
		{q_("500 sidereal days"), "36"},{q_("1 sidereal year"), "27"},{q_("1 Julian day"), "12"},{q_("5 Julian days"), "13"},{q_("10 Julian days"), "14"},
		{q_("15 Julian days"), "15"},{q_("30 Julian days"), "16"},{q_("60 Julian days"), "17"},{q_("100 Julian days"), "26"},{q_("1 Julian year"), "28"},
		{q_("1 Gaussian year"), "29"},{q_("1 synodic month"), "30"},{q_("1 draconic month"), "31"},{q_("1 mean tropical month"), "32"},
		{q_("1 anomalistic month"), "33"},{q_("1 anomalistic year"), "34"},{q_("1 saros"), "35"},{q_("custom interval"), "0"}
	};
	Q_ASSERT(ui->ephemerisStepComboBox);
	QComboBox* steps = ui->ephemerisStepComboBox;
	steps->blockSignals(true);
	steps->clear();
	int index = steps->currentIndex();
	QVariant selectedStepId = steps->itemData(index);	
	for (const auto& f : items)
	{
		steps->addItem(f.first, f.second);
	}

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

void AstroCalcDialog::enableEphemerisButtons(bool enable)
{
	ui->ephemerisSaveButton->setEnabled(enable);
	ui->ephemerisCleanupButton->setEnabled(enable);
}

void AstroCalcDialog::populatePlanetList()
{
	Q_ASSERT(ui->object1ComboBox); // object 1 is always major planet

	QComboBox* planetList = ui->object1ComboBox;
	QList<PlanetP> planets = solarSystem->getAllPlanets();
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
			planetList->addItem(planet->getNameI18n(), planet->getEnglishName());

		// moons of the current planet
		if (planet->getPlanetType() == Planet::isMoon && planet->getEnglishName() != cpName && planet->getParent() == core->getCurrentPlanet())
			planetList->addItem(planet->getNameI18n(), planet->getEnglishName());
	}
	// special case: selected dwarf and minor planets
	planets.clear();
	planets = getSelectedMinorPlanets();
	for (const auto& planet : qAsConst(planets))
	{
		if (!planet.isNull() && planet->getEnglishName()!=cpName)
			planetList->addItem(planet->getNameI18n(), planet->getEnglishName());
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
	const QMap<QString, int>itemsMap={
		{q_("Latest selected object"), PHCLatestSelectedObject}, {q_("Solar system"), PHCSolarSystem}, {q_("Planets"), PHCPlanets},
		{q_("Asteroids"), PHCAsteroids}, {q_("Plutinos"), PHCPlutinos}, {q_("Comets"), PHCComets}, {q_("Dwarf planets"), PHCDwarfPlanets},
		{q_("Cubewanos"), PHCCubewanos}, {q_("Scattered disc objects"), PHCScatteredDiscObjects}, {q_("Oort cloud objects"), PHCOortCloudObjects},
		{q_("Sednoids"), PHCSednoids}, {q_("Bright stars (<%1 mag)").arg(QString::number(brightLimit - 5.0, 'f', 1)), PHCBrightStars},
		{q_("Bright double stars (<%1 mag)").arg(QString::number(brightLimit - 5.0, 'f', 1)), PHCBrightDoubleStars},
		{q_("Bright variable stars (<%1 mag)").arg(QString::number(brightLimit - 5.0, 'f', 1)), PHCBrightVariableStars},
		{q_("Bright star clusters (<%1 mag)").arg(brLimit), PHCBrightStarClusters}, {q_("Planetary nebulae (<%1 mag)").arg(brLimit), PHCPlanetaryNebulae},
		{q_("Bright nebulae (<%1 mag)").arg(brLimit), PHCBrightNebulae}, {q_("Dark nebulae"), PHCDarkNebulae},
		{q_("Bright galaxies (<%1 mag)").arg(brLimit), PHCBrightGalaxies}, {q_("Symbiotic stars"), PHCSymbioticStars},
		{q_("Emission-line stars"), PHCEmissionLineStars}, {q_("Interstellar objects"), PHCInterstellarObjects},
		{q_("Planets and Sun"), PHCPlanetsSun}, {q_("Sun, planets and moons of observer's planet"), PHCSunPlanetsMoons},
		{q_("Bright Solar system objects (<%1 mag)").arg(QString::number(brightLimit + 2.0, 'f', 1)), PHCBrightSolarSystemObjects},
		{q_("Solar system objects: minor bodies"), PHCSolarSystemMinorBodies}, {q_("Moons of first body"), PHCMoonsFirstBody},
		{q_("Bright carbon stars"), PHCBrightCarbonStars}, {q_("Bright barium stars"), PHCBrightBariumStars},
		{q_("Sun, planets and moons of first body and observer's planet"), PHCSunPlanetsTheirMoons}
	};
	QMapIterator<QString, int> i(itemsMap);
	groups->clear();
	while (i.hasNext())
	{
		i.next();
		groups->addItem(i.key(), QString::number(i.value()));
	}

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
	enablePhenomenaButtons(false);
}

void AstroCalcDialog::savePhenomenaOppositionFlag(bool b)
{
	conf->setValue("astrocalc/flag_phenomena_opposition", b);
}

void AstroCalcDialog::savePhenomenaPerihelionAphelionFlag(bool b)
{
	conf->setValue("astrocalc/flag_phenomena_perihelion", b);
}

void AstroCalcDialog::savePhenomenaElongationsQuadraturesFlag(bool b)
{
	conf->setValue("astrocalc/flag_phenomena_quadratures", b);
}

void AstroCalcDialog::savePhenomenaAngularSeparation()
{
	conf->setValue("astrocalc/phenomena_angular_separation", QString::number(ui->allowedSeparationSpinBox->valueDegrees(), 'f', 5));
}

void AstroCalcDialog::drawAltVsTimeDiagram()
{
	if (!altVsTimeChartMutex.tryLock()) return; // Avoid calling parallel from various sides. (called by signals/slots)
	// Avoid crash!
	if (core->getCurrentPlanet()->getEnglishName().contains("->")) // We are on the spaceship!
	{
		altVsTimeChartMutex.unlock();
		return;
	}
	// special case - plot the graph when tab is visible
	//..
	// we got notified about a reason to redraw the plot, but dialog was
	// not visible. which means we must redraw when becoming visible again!
	if (!dialog->isVisible() && plotAltVsTime)
	{
		graphPlotNeedsRefresh = true;
		altVsTimeChartMutex.unlock();
		return;
	}

	if (!plotAltVsTime)
	{
		altVsTimeChartMutex.unlock();
		return;
	}

	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();

	altVsTimeChart = new AstroCalcChart({AstroCalcChart::AltVsTime, AstroCalcChart::CurrentTime, AstroCalcChart::TransitTime, AstroCalcChart::SunElevation, AstroCalcChart::CivilTwilight, AstroCalcChart::NauticalTwilight, AstroCalcChart::AstroTwilight, AstroCalcChart::Moon});
	QPair<double, double>yRange(-90., 90.);

	if (!selectedObjects.isEmpty())
	{
		// X axis - time; Y axis - altitude
		StelObjectP selectedObject = selectedObjects[0];
		altVsTimeChart->setTitle(QString("%1 (%2)").arg(getSelectedObjectNameI18n(selectedObject), selectedObject->getObjectTypeI18n()));
		PlanetP sun = solarSystem->getSun();
		PlanetP moon = solarSystem->getMoon();

		const bool onEarth = core->getCurrentPlanet()==solarSystem->getEarth();
		const double currentJD = core->getJD();
		const double shift = core->getUTCOffset(currentJD) / 24.0;
		const double noon = std::floor(currentJD + shift); // Integral JD of this day
		double az, alt;

		bool isSatellite = false;
#ifdef USE_STATIC_PLUGIN_SATELLITES

		SatelliteP sat;		
		if (selectedObject->getType() == "Satellite") 
		{
			// get reference to satellite
			isSatellite = true;
			sat = GETSTELMODULE(Satellites)->getById(selectedObject.staticCast<Satellite>()->getCatalogNumberString());
		}
#endif
		for (int i = 0; i <= 86400; i+= isSatellite? 600 : 3600) // 24 hours in hourly or 600s steps
		{
			// A new point on the graph every 600s with shift to right 12 hours
			// to get midnight at the center of diagram (i.e. accuracy is 10 minutes, but smoothed by Spline interpolation)
			double JD = noon + i / 86400. - shift;
			core->setJD(JD);
			core->update(0.0);
#ifdef USE_STATIC_PLUGIN_SATELLITES
			if (isSatellite)
			{
				// update data for that single satellite only
				sat->update(0.0);
			}
#endif
			StelUtils::rectToSphe(&az, &alt, selectedObject->getAltAzPosAuto(core));
			altVsTimeChart->append(AstroCalcChart::AltVsTime, StelUtils::jdToQDateTime(JD, Qt::UTC).toMSecsSinceEpoch(), alt*M_180_PI);

			if (plotAltVsTimeSun)
			{
				StelUtils::rectToSphe(&az, &alt, sun->getAltAzPosAuto(core));
				double deg=alt*M_180_PI;

				altVsTimeChart->append(AstroCalcChart::SunElevation,     StelUtils::jdToQDateTime(JD, Qt::UTC).toMSecsSinceEpoch(), deg);
				altVsTimeChart->append(AstroCalcChart::CivilTwilight,    StelUtils::jdToQDateTime(JD, Qt::UTC).toMSecsSinceEpoch(), deg+6);
				altVsTimeChart->append(AstroCalcChart::NauticalTwilight, StelUtils::jdToQDateTime(JD, Qt::UTC).toMSecsSinceEpoch(), deg+12);
				altVsTimeChart->append(AstroCalcChart::AstroTwilight,    StelUtils::jdToQDateTime(JD, Qt::UTC).toMSecsSinceEpoch(), deg+18);
			}
			if (plotAltVsTimeMoon && onEarth)
			{
				StelUtils::rectToSphe(&az, &alt, moon->getAltAzPosAuto(core));
				altVsTimeChart->append(AstroCalcChart::Moon, StelUtils::jdToQDateTime(JD, Qt::UTC).toMSecsSinceEpoch(), alt*M_180_PI);
			}
		}
		altVsTimeChart->show(AstroCalcChart::AltVsTime);
		// configure series shown and also find real limits for all plots.
		yRange=altVsTimeChart->findYRange(AstroCalcChart::AltVsTime);
		if (plotAltVsTimeSun)
		{
			altVsTimeChart->show(AstroCalcChart::SunElevation);
			altVsTimeChart->show(AstroCalcChart::CivilTwilight);
			altVsTimeChart->show(AstroCalcChart::NauticalTwilight);
			altVsTimeChart->show(AstroCalcChart::AstroTwilight);
			QPair<double, double>yRangeSun=altVsTimeChart->findYRange(AstroCalcChart::SunElevation);
			yRange.first =qMin(yRangeSun.first,  yRange.first);
			yRange.second=qMax(yRangeSun.second, yRange.second);
		}
		if (plotAltVsTimeMoon && onEarth)
		{
			altVsTimeChart->show(AstroCalcChart::Moon);
			QPair<double, double>yRangeMoon=altVsTimeChart->findYRange(AstroCalcChart::Moon);
			yRange.first =qMin(yRangeMoon.first,  yRange.first);
			yRange.second=qMax(yRangeMoon.second, yRange.second);
		}

		core->setJD(currentJD);

		drawCurrentTimeDiagram();

		// Transit line
		QPair<double, double>transit=altVsTimeChart->findYMax(AstroCalcChart::AltVsTime);
		altVsTimeChart->drawTrivialLineX(AstroCalcChart::TransitTime, transit.first);
	}
	else
	{
		altVsTimeChart->clear(AstroCalcChart::AltVsTime);
		altVsTimeChart->clear(AstroCalcChart::TransitTime);
		altVsTimeChart->clear(AstroCalcChart::SunElevation);
		altVsTimeChart->clear(AstroCalcChart::CivilTwilight);
		altVsTimeChart->clear(AstroCalcChart::NauticalTwilight);
		altVsTimeChart->clear(AstroCalcChart::AstroTwilight);
		altVsTimeChart->clear(AstroCalcChart::Moon);
		altVsTimeChart->setTitle(q_("Please select object to plot its graph 'Altitude vs. Time'."));
	}

	if (plotAltVsTimePositive && yRange.first<altVsTimePositiveLimit)
		yRange.first = altVsTimePositiveLimit;
	altVsTimeChart->setYrange(AstroCalcChart::AltVsTime, yRange, plotAltVsTimePositive && qFuzzyCompare(yRange.first, altVsTimePositiveLimit));
	altVsTimeChart->setupAxes(core->getJD(), 1, "");
	QChart *oldChart=ui->altVsTimeChartView->chart();
	if (oldChart) oldChart->deleteLater();
	ui->altVsTimeChartView->setChart(altVsTimeChart);
	ui->altVsTimeChartView->setRenderHint(QPainter::Antialiasing);

	altVsTimeChartMutex.unlock();
}

// Added vertical line indicating "now" to the AltVsTime and AzVsTime plots
void AstroCalcDialog::drawCurrentTimeDiagram()
{
	// special case - plot the graph when tab is visible
	// and only if dialog is visible at all
	if (!dialog->isVisible() || (!plotAltVsTime && !plotAziVsTime)) return;

	const double currentJD = core->getJD();
	const double UTCOffset = core->getUTCOffset(currentJD)/24.;

	if (plotAltVsTime)
	{
		if (altVsTimeChart){
			altVsTimeChart->drawTrivialLineX(AstroCalcChart::CurrentTime, qreal(StelUtils::jdToQDateTime(currentJD, Qt::UTC).toMSecsSinceEpoch()));
		}
		else
			qWarning() << "no alt chart to add CT line!";
	}
	if (plotAziVsTime)
	{
		if (azVsTimeChart)
		{
			azVsTimeChart->drawTrivialLineX(AstroCalcChart::CurrentTime, qreal(StelUtils::jdToQDateTime(currentJD, Qt::UTC).toMSecsSinceEpoch()));
		}
		else
			qWarning() << "no azi chart to add CT line!";
	}

	// detect roll over graph day limits.
	// if so, update the graph
	int graphJD = static_cast<int>(currentJD + UTCOffset);
	if (oldGraphJD != graphJD || graphPlotNeedsRefresh)
	{
		oldGraphJD = graphJD;
		graphPlotNeedsRefresh = false;
		emit graphDayChanged();
	}
}

void AstroCalcDialog::drawXVsTimeGraphs()
{
	if (!curvesChartMutex.tryLock()) return;
	PlanetP ssObj = solarSystem->searchByEnglishName(ui->graphsCelestialBodyComboBox->currentData().toString());	
	// added special case - the tool is not applicable on non-Earth locations
	if (!ssObj.isNull() && core->getCurrentPlanet()==solarSystem->getEarth())
	{
		// X axis - time; Y axis - altitude
		const double currentJD = core->getJD();
		int year, month, day;
		double startJD;
		StelUtils::getDateFromJulianDay(currentJD, &year, &month, &day);
		StelUtils::getJDFromDate(&startJD, year, month, 1, 0, 0, 0);
		const double utcOffset = core->getUTCOffset(startJD)/24.;
		startJD+=utcOffset;

		AstroCalcChart::Series firstGraph  = AstroCalcChart::Series(ui->graphsFirstComboBox->currentData().toInt());
		AstroCalcChart::Series secondGraph = AstroCalcChart::Series(ui->graphsSecondComboBox->currentData().toInt());

		// It may be that we come from 0.22.1 for the first time. Apply some useful default graphs
		if (firstGraph==AstroCalcChart::AltVsTime) firstGraph=AstroCalcChart::AngularSize1;
		if (secondGraph==AstroCalcChart::AltVsTime) secondGraph=AstroCalcChart::Magnitude2;
		curvesChart = new AstroCalcChart({firstGraph, secondGraph});

		// Our principal counter is now hours.
		//qDebug() << "X time range" << StelUtils::julianDayToISO8601String(startJD) << "to" << StelUtils::julianDayToISO8601String(startJD+(30*24)*graphsDuration/24.);
		for (int i = -24; i <= (30*24)*graphsDuration+24; i+=graphsStep)
		{
			double JD = startJD + i/24.;

			if (firstGraph==AstroCalcChart::TransitAltitude1 || secondGraph==AstroCalcChart::TransitAltitude2)
			{
				core->setJD(JD);
				core->update(0.0);
				Vec4d rts = ssObj->getRTSTime(core);
				JD = rts[1];
			}
			core->setJD(JD);
			core->update(0.0);

			curvesChart->append(firstGraph,  StelUtils::jdToQDateTime(JD+utcOffset, Qt::UTC).toMSecsSinceEpoch(), computeGraphValue(ssObj, firstGraph));
			curvesChart->append(secondGraph, StelUtils::jdToQDateTime(JD+utcOffset, Qt::UTC).toMSecsSinceEpoch(), computeGraphValue(ssObj, secondGraph));
			//qDebug() << secondGraph << ": JD" << QString::number(JD+utcOffset, 'f', 18) << "=" << "=" << StelUtils::julianDayToISO8601String(JD+utcOffset) << StelUtils::jdToQDateTime(JD+utcOffset, Qt::UTC) << ":" << computeGraphValue(ssObj, secondGraph);
		}
		core->setJD(currentJD);

		// Work out scaling
		QPair<double, double>yRangeLeft  = curvesChart->findYRange(firstGraph);
		QPair<double, double>yRangeRight = curvesChart->findYRange(secondGraph);

		QString englishName=ui->graphsCelestialBodyComboBox->currentData().toString();
		curvesChart->show(firstGraph);
		curvesChart->show(secondGraph);
		curvesChart->setYrange(firstGraph, yRangeLeft);
		curvesChart->setYrangeR(secondGraph, yRangeRight);
		curvesChart->setupAxes(core->getJD(), graphsDuration, englishName);
	}
	else
	{
		curvesChart = new AstroCalcChart({AstroCalcChart::AngularSize1, AstroCalcChart::Distance2}); // May be wrong as it does not protect display from other planets...
		curvesChart->setYrange(AstroCalcChart::AngularSize1, 0., 10.);
		curvesChart->setYrangeR(AstroCalcChart::Magnitude2, 0., 10.);
		curvesChart->setupAxes(core->getJD(), graphsDuration, "");
	}

	QChart *oldChart=ui->twoGraphsChartView->chart();
	if (oldChart) oldChart->deleteLater();
	ui->twoGraphsChartView->setChart(curvesChart);
	ui->twoGraphsChartView->setRenderHint(QPainter::Antialiasing);
	curvesChartMutex.unlock();
}

void AstroCalcDialog::updateXVsTimeGraphs()
{
	// Do all that only if the dialog is visible!
	if (!dialog->isVisible())
		return;

	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();
	if (!selectedObjects.isEmpty())
	{
		QComboBox* celestialBody = ui->graphsCelestialBodyComboBox;
		celestialBody->blockSignals(true);
		int index = celestialBody->findData(selectedObjects[0]->getEnglishName(), Qt::UserRole, Qt::MatchCaseSensitive);
		if (index>=0)
			celestialBody->setCurrentIndex(index);
		celestialBody->blockSignals(false);
	}

	if (ui->tabWidgetGraphs->currentIndex()==3)
		drawXVsTimeGraphs();
}

double AstroCalcDialog::computeGraphValue(const PlanetP &ssObj, const AstroCalcChart::Series graphType)
{
	double value = 0.;
	switch (graphType)
	{
		case AstroCalcChart::Magnitude1:
		case AstroCalcChart::Magnitude2:
			value = static_cast<double>(ssObj->getVMagnitude(core));
			break;
		case AstroCalcChart::Phase1:
		case AstroCalcChart::Phase2:
			value = static_cast<double>(ssObj->getPhase(core->getObserverHeliocentricEclipticPos())) * 100.;
			break;
		case AstroCalcChart::Distance1:
		case AstroCalcChart::Distance2:
			value =  ssObj->getJ2000EquatorialPos(core).norm();
			if (ssObj->getEnglishName()=="Moon")
				value*=(AU*0.001);
			break;
		case AstroCalcChart::Elongation1:
		case AstroCalcChart::Elongation2:
			value = ssObj->getElongation(core->getObserverHeliocentricEclipticPos()) * M_180_PI;
			break;
		case AstroCalcChart::AngularSize1:
		case AstroCalcChart::AngularSize2:
		{
			// angular radius without rings
			value = ssObj->getSpheroidAngularRadius(core) * (360. / M_PI);
			if (value < 1.)
				value *= 60.;
			break;
		}
		case AstroCalcChart::PhaseAngle1:
		case AstroCalcChart::PhaseAngle2:
			value = ssObj->getPhaseAngle(core->getObserverHeliocentricEclipticPos()) * M_180_PI;
			break;
		case AstroCalcChart::HeliocentricDistance1:
		case AstroCalcChart::HeliocentricDistance2:
			value =  ssObj->getHeliocentricEclipticPos().norm();
			break;
		case AstroCalcChart::TransitAltitude1:
		case AstroCalcChart::TransitAltitude2:
		{
			double az, alt;
			StelUtils::rectToSphe(&az, &alt, ssObj->getAltAzPosAuto(core));
			value=alt*M_180_PI;
			break;
		}
		case AstroCalcChart::RightAscension1:
		case AstroCalcChart::RightAscension2:
		{
			double dec_equ, ra_equ;
			StelUtils::rectToSphe(&ra_equ, &dec_equ, ssObj->getEquinoxEquatorialPos(core));
			ra_equ = 2.*M_PI-ra_equ;
			value = ra_equ*12./M_PI;
			if (value>24.)
				value -= 24.;
			break;
		}
		case AstroCalcChart::Declination1:
		case AstroCalcChart::Declination2:
		{
			double dec_equ, ra_equ;
			StelUtils::rectToSphe(&ra_equ, &dec_equ, ssObj->getEquinoxEquatorialPos(core));
			value=dec_equ*M_180_PI;
			break;
		}
		default:
			qWarning() << "AstroCalcDialog::computeGraphValue() Wrong call for " << graphType;
	}
	return value;
}

void AstroCalcDialog::populateFunctionsList()
{
	Q_ASSERT(ui->graphsFirstComboBox);
	Q_ASSERT(ui->graphsSecondComboBox);

	typedef QPair<QString, AstroCalcChart::Series> graph;
	const QList<graph> functionsF = {
		{ q_("Magnitude vs. Time"),    AstroCalcChart::Magnitude1},
		{ q_("Phase vs. Time"),        AstroCalcChart::Phase1},
		{ q_("Distance vs. Time"),     AstroCalcChart::Distance1},
		{ q_("Elongation vs. Time"),   AstroCalcChart::Elongation1},
		{ q_("Angular size vs. Time"), AstroCalcChart::AngularSize1},
		{ q_("Phase angle vs. Time"),  AstroCalcChart::PhaseAngle1},
		// TRANSLATORS: The phrase "Heliocentric distance" may be long in some languages and you can short it to use in the drop-down list.
		{ q_("Heliocentric distance vs. Time"), AstroCalcChart::HeliocentricDistance1},
		// TRANSLATORS: The phrase "Transit altitude" may be long in some languages and you can short it to use in the drop-down list.
		{ q_("Transit altitude vs. Time"), AstroCalcChart::TransitAltitude1},
		// TRANSLATORS: The phrase "Right ascension" may be long in some languages and you can short it to use in the drop-down list.
		{ q_("Right ascension vs. Time"), AstroCalcChart::RightAscension1},
		{ q_("Declination vs. Time"), AstroCalcChart::Declination1}};
	const QList<graph> functionsS = {
		{ q_("Magnitude vs. Time"),    AstroCalcChart::Magnitude2},
		{ q_("Phase vs. Time"),        AstroCalcChart::Phase2},
		{ q_("Distance vs. Time"),     AstroCalcChart::Distance2},
		{ q_("Elongation vs. Time"),   AstroCalcChart::Elongation2},
		{ q_("Angular size vs. Time"), AstroCalcChart::AngularSize2},
		{ q_("Phase angle vs. Time"),  AstroCalcChart::PhaseAngle2},
		// TRANSLATORS: The phrase "Heliocentric distance" may be long in some languages and you can short it to use in the drop-down list.
		{ q_("Heliocentric distance vs. Time"), AstroCalcChart::HeliocentricDistance2},
		// TRANSLATORS: The phrase "Transit altitude" may be long in some languages and you can short it to use in the drop-down list.
		{ q_("Transit altitude vs. Time"), AstroCalcChart::TransitAltitude2},
		// TRANSLATORS: The phrase "Right ascension" may be long in some languages and you can short it to use in the drop-down list.
		{ q_("Right ascension vs. Time"), AstroCalcChart::RightAscension2},
		{ q_("Declination vs. Time"), AstroCalcChart::Declination2}};

	QComboBox* firstCB = ui->graphsFirstComboBox;
	QComboBox* secondCB = ui->graphsSecondComboBox;
	firstCB->blockSignals(true);
	secondCB->blockSignals(true);

	int indexF = firstCB->currentIndex();
	QVariant selectedFirstId = firstCB->itemData(indexF);
	int indexS = secondCB->currentIndex();
	QVariant selectedSecondId = secondCB->itemData(indexS);

	firstCB->clear();
	secondCB->clear();

	for (const auto& f : functionsF)
	{
		firstCB->addItem(f.first, f.second);
	}
	for (const auto& f : functionsS)
	{
		secondCB->addItem(f.first, f.second);
	}

	indexF = firstCB->findData(selectedFirstId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexF < 0)
		indexF = firstCB->findData(conf->value("astrocalc/graphs_first_id", AstroCalcChart::Magnitude1).toInt(), Qt::UserRole, Qt::MatchCaseSensitive);
	firstCB->setCurrentIndex(indexF);
	firstCB->model()->sort(0);

	indexS = secondCB->findData(selectedSecondId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexS < 0)
		indexS = secondCB->findData(conf->value("astrocalc/graphs_second_id", AstroCalcChart::Phase2).toInt(), Qt::UserRole, Qt::MatchCaseSensitive);
	secondCB->setCurrentIndex(indexS);
	secondCB->model()->sort(0);

	firstCB->blockSignals(false);
	secondCB->blockSignals(false);
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
	if (!monthlyElevationChartMutex.tryLock()) return; // Avoid calling parallel from various sides. (called by signals/slots)

	// Avoid crash!
	if (core->getCurrentPlanet()->getEnglishName().contains("->")) // We are on the spaceship!
	{
		monthlyElevationChartMutex.unlock();
		return;
	}

	// special case - plot the graph when tab is visible
	//..
	// we got notified about a reason to redraw the plot, but dialog was
	// not visible. which means we must redraw when becoming visible again!
	if (!dialog->isVisible() && plotMonthlyElevation)
	{
		graphPlotNeedsRefresh = true;
		monthlyElevationChartMutex.unlock();
		return;
	}

	if (!plotMonthlyElevation)
	{
		monthlyElevationChartMutex.unlock();
		return;
	}

	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();

	monthlyElevationChart = new AstroCalcChart({AstroCalcChart::MonthlyElevation});
	QPair<double, double>yRangeME(-90., 90.);

	if (!selectedObjects.isEmpty() && (selectedObjects[0]->getType() != "Satellite"))
	{
		// X axis - time; Y axis - altitude
		StelObjectP selectedObject = selectedObjects[0];
		monthlyElevationChart->setTitle(QString("%1 (%2)").arg(getSelectedObjectNameI18n(selectedObject), selectedObject->getObjectTypeI18n()));

		const double currentJD = core->getJD();
		int year, month, day;		
		StelUtils::getDateFromJulianDay(currentJD, &year, &month, &day);
		int hour = ui->monthlyElevationTime->value();
		double startJD;
		StelUtils::getJDFromDate(&startJD, year, 1, 1, hour, 0, 0);
		// If we want to take DST effects into account, move this line into the loop and make a denser plot!
		const double utOffset=core->getUTCOffset(currentJD)/24;
		startJD -= utOffset; // Time zone correction
		int dYear = static_cast<int>(core->getCurrentPlanet()->getSiderealPeriod()/5.) + 3;
		for (int i = 0; i <= dYear; i+=3)
		{
			double JD = startJD + i*5;
			core->setJD(JD);
			core->update(0.0);
			double az, alt;
			StelUtils::rectToSphe(&az, &alt, selectedObject->getAltAzPosAuto(core));
			// The first point shall be plotted at the sharp edge date of Jan1 at midnight. The plot should more correctly be a point plot, but the spline is good enough.
			monthlyElevationChart->append(AstroCalcChart::MonthlyElevation, StelUtils::jdToQDateTime(JD-hour/24.+utOffset, Qt::UTC).toMSecsSinceEpoch(), alt*M_180_PI);
		}
		core->setJD(currentJD);
		yRangeME=monthlyElevationChart->findYRange(AstroCalcChart::MonthlyElevation);
	}
	else
		monthlyElevationChart->setTitle(q_("Please select object to plot its 'Monthly Elevation' graph for the current year at selected time."));

	if (plotMonthlyElevationPositive && yRangeME.first<monthlyElevationPositiveLimit)
		yRangeME.first = monthlyElevationPositiveLimit;
	monthlyElevationChart->show(AstroCalcChart::MonthlyElevation);
	monthlyElevationChart->setYrange(AstroCalcChart::MonthlyElevation, yRangeME, plotMonthlyElevationPositive && qFuzzyCompare(yRangeME.first, monthlyElevationPositiveLimit));
	monthlyElevationChart->setupAxes(core->getJD(), 1, "");
	QChart *oldChart=ui->monthlyElevationChartView->chart();
	if (oldChart) oldChart->deleteLater();
	ui->monthlyElevationChartView->setChart(monthlyElevationChart);
	ui->monthlyElevationChartView->setRenderHint(QPainter::Antialiasing);

	monthlyElevationChartMutex.unlock();
}

// When dialog becomes visible: check if there is a graph plot to refresh
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
			if (plotLunarElongationGraph)
				drawLunarElongationGraph();
		}
		else
			drawCurrentTimeDiagram();
	}
	graphPlotNeedsRefresh = false;
}

void AstroCalcDialog::setPhenomenaHeaderNames()
{
	phenomenaHeader = QStringList({
		q_("Phenomenon"),
		q_("Date and Time"),
		q_("Object 1"),
		// TRANSLATORS: Magnitude of object 1
		q_("Mag. 1"),
		q_("Object 2"),
		// TRANSLATORS: Magnitude of object 2
		q_("Mag. 2"),
		q_("Separation"),
		q_("Elevation"),
		q_("Solar Elongation"),
		q_("Lunar Elongation")});
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
	ui->phenomenaTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
}

void AstroCalcDialog::selectCurrentPhenomen(const QModelIndex& modelIndex)
{
	// Find the object
	QString name = ui->object1ComboBox->currentData().toString();
	if (modelIndex.sibling(modelIndex.row(), PhenomenaType).data().toString().contains(q_("Opposition"), Qt::CaseInsensitive))
		name = modelIndex.sibling(modelIndex.row(), PhenomenaObject2).data().toString();
	const double JD = modelIndex.sibling(modelIndex.row(), PhenomenaDate).data(Qt::UserRole).toDouble();
	goToObject(name, JD);
}

void AstroCalcDialog::calculatePhenomena()
{
	const QString currentPlanet = ui->object1ComboBox->currentData().toString();
	const double separation = ui->allowedSeparationSpinBox->valueDegrees();
	const bool opposition = ui->phenomenaOppositionCheckBox->isChecked();
	const bool perihelion = ui->phenomenaPerihelionAphelionCheckBox->isChecked();
	const bool quadrature = ui->phenomenaElongationQuadratureCheckBox->isChecked();

	initListPhenomena();

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	double startJD = StelUtils::qDateTimeToJd(ui->phenomenFromDateEdit->date().startOfDay(Qt::UTC));
	double stopJD = StelUtils::qDateTimeToJd(ui->phenomenToDateEdit->date().endOfDay(Qt::UTC));
#else
	double startJD = StelUtils::qDateTimeToJd(QDateTime(ui->phenomenFromDateEdit->date()));
	double stopJD = StelUtils::qDateTimeToJd(QDateTime(ui->phenomenToDateEdit->date().addDays(1)));
#endif
	if (stopJD<=startJD) // Stop warming atmosphere!..
		return;

	QVector<PlanetP> objects;
	Q_ASSERT(objects.isEmpty());
	QList<PlanetP> allObjects = solarSystem->getAllPlanets();

	QVector<NebulaP> dso;
	Q_ASSERT(dso.isEmpty());
	QVector<NebulaP> allDSO = dsoMgr->getAllDeepSkyObjects();

	QVector<StelObjectP> star, doubleStar, variableStar;
	Q_ASSERT(star.isEmpty());
	Q_ASSERT(doubleStar.isEmpty());
	Q_ASSERT(variableStar.isEmpty());
	QVector<StelObjectP> hipStars = starMgr->getHipparcosStars().toVector();
	QVector<StelObjectP> carbonStars = starMgr->getHipparcosCarbonStars().toVector();
	QVector<StelObjectP> bariumStars = starMgr->getHipparcosBariumStars().toVector();
	QVector<StelACStarData> doubleHipStars = starMgr->getHipparcosDoubleStars().toVector();
	QVector<StelACStarData> variableHipStars = starMgr->getHipparcosVariableStars().toVector();

	int obj2Type = ui->object2ComboBox->currentData().toInt();
	switch (obj2Type)
	{
		case PHCSolarSystem: // All Solar system objects
			for (const auto& object : allObjects)
			{
				if (object->getPlanetType() != Planet::isUNDEFINED && object->getPlanetType() != Planet::isObserver)
					objects.append(object);
			}
			break;
		case PHCPlanets: // Planets
			for (const auto& object : allObjects)
			{
				if (object->getPlanetType() == Planet::isPlanet && object->getEnglishName() != core->getCurrentPlanet()->getEnglishName() && object->getEnglishName() != currentPlanet)
					objects.append(object);
			}
			break;
		case PHCAsteroids:
		case PHCPlutinos:
		case PHCComets:
		case PHCDwarfPlanets:
		case PHCCubewanos:
		case PHCScatteredDiscObjects:
		case PHCOortCloudObjects:
		case PHCSednoids:
		case PHCInterstellarObjects:
		{
			static const QMap<int, Planet::PlanetType>map = {
				{PHCAsteroids,            Planet::isAsteroid},
				{PHCPlutinos,             Planet::isPlutino},
				{PHCComets,               Planet::isComet},
				{PHCDwarfPlanets,         Planet::isDwarfPlanet},
				{PHCCubewanos,            Planet::isCubewano},
				{PHCScatteredDiscObjects, Planet::isSDO},
				{PHCOortCloudObjects,     Planet::isOCO},
				{PHCSednoids,             Planet::isSednoid},
				{PHCInterstellarObjects,  Planet::isInterstellar}};
			const Planet::PlanetType pType = map.value(obj2Type, Planet::isUNDEFINED);

			for (const auto& object : allObjects)
			{
				if (object->getPlanetType() == pType)
					objects.append(object);
			}
			break;
		}
		case PHCBrightStars: // Stars
		case PHCBrightCarbonStars: // Bright carbon stars
		case PHCBrightBariumStars: // Bright barium stars
		{
			QVector<StelObjectP> stars;
			if (obj2Type==PHCBrightStars)
				stars = hipStars;
			else if (obj2Type==PHCBrightBariumStars)
				stars = bariumStars;
			else
				stars = carbonStars;
			for (const auto& object : qAsConst(stars))
			{
				if (object->getVMagnitude(core) < static_cast<float>(brightLimit - 5.0))
					star.append(object);
			}
			break;
		}
		case PHCBrightDoubleStars: // Double stars
			for (const auto& object : doubleHipStars)
			{
				if (object.firstKey()->getVMagnitude(core) < static_cast<float>(brightLimit - 5.0))
					star.append(object.firstKey());
			}
			break;
		case PHCBrightVariableStars: // Variable stars
			for (const auto& object : variableHipStars)
			{
				if (object.firstKey()->getVMagnitude(core) < static_cast<float>(brightLimit - 5.0))
					star.append(object.firstKey());
			}
			break;
		case PHCBrightStarClusters: // Star clusters
			for (const auto& object : allDSO)
			{
			    if (object->getVMagnitude(core) < static_cast<float>(brightLimit) && (object->getDSOType() == Nebula::NebCl || object->getDSOType() == Nebula::NebOc || object->getDSOType() == Nebula::NebGc || object->getDSOType() == Nebula::NebSA || object->getDSOType() == Nebula::NebSC || object->getDSOType() == Nebula::NebCn))
					dso.append(object);
			}
			break;
		case PHCPlanetaryNebulae: // Planetary nebulae
			for (const auto& object : allDSO)
			{
				if (object->getVMagnitude(core) < brightLimit && (object->getDSOType() == Nebula::NebPn || object->getDSOType() == Nebula::NebPossPN || object->getDSOType() == Nebula::NebPPN))
					dso.append(object);
			}
			break;
		case PHCBrightNebulae: // Bright nebulae
			for (const auto& object : allDSO)
			{
				if (object->getVMagnitude(core) < brightLimit && (object->getDSOType() == Nebula::NebN || object->getDSOType() == Nebula::NebBn || object->getDSOType() == Nebula::NebEn || object->getDSOType() == Nebula::NebRn || object->getDSOType() == Nebula::NebHII || object->getDSOType() == Nebula::NebISM || object->getDSOType() == Nebula::NebCn || object->getDSOType() == Nebula::NebSNR))
					dso.append(object);
			}
			break;
		case PHCDarkNebulae: // Dark nebulae
			for (const auto& object : allDSO)
			{
				if (object->getDSOType() == Nebula::NebDn || object->getDSOType() == Nebula::NebMolCld || object->getDSOType() == Nebula::NebYSO)
					dso.append(object);
			}
			break;
		case PHCBrightGalaxies: // Galaxies
			for (const auto& object : allDSO)
			{
				if (object->getVMagnitude(core) < brightLimit && (object->getDSOType() == Nebula::NebGx || object->getDSOType() == Nebula::NebAGx || object->getDSOType() == Nebula::NebRGx || object->getDSOType() == Nebula::NebQSO || object->getDSOType() == Nebula::NebPossQSO || object->getDSOType() == Nebula::NebBLL || object->getDSOType() == Nebula::NebBLA || object->getDSOType() == Nebula::NebIGx))
					dso.append(object);
			}
			break;
		case PHCSymbioticStars: // Symbiotic stars
			for (const auto& object : allDSO)
			{
				if (object->getDSOType() == Nebula::NebSymbioticStar)
					dso.append(object);
			}
			break;
		case PHCEmissionLineStars: // Emission-line stars
			for (const auto& object : allDSO)
			{
				if (object->getDSOType() == Nebula::NebEmissionLineStar)
					dso.append(object);
			}
			break;
		case PHCPlanetsSun: // Planets and Sun
			for (const auto& object : allObjects)
			{
				if ((object->getPlanetType() == Planet::isPlanet || object->getPlanetType() == Planet::isStar) && object->getEnglishName() != core->getCurrentPlanet()->getEnglishName() && object->getEnglishName() != currentPlanet)
					objects.append(object);
			}
			break;
		case PHCSunPlanetsMoons: // Sun, planets and moons of observer's planet
		{
			PlanetP cp = core->getCurrentPlanet();
			for (const auto& object : allObjects)
			{
				if ((object->getPlanetType() == Planet::isPlanet || object->getPlanetType() == Planet::isStar || (object->getParent()==cp && object->getPlanetType()==Planet::isMoon)) && object->getEnglishName() != cp->getEnglishName() && object->getEnglishName() != currentPlanet)
					objects.append(object);
			}
			break;
		}
		case PHCBrightSolarSystemObjects: // Bright Solar system objects
			for (const auto& object : allObjects)
			{
				if (object->getVMagnitude(core) < (brightLimit + 2.0) && object->getPlanetType() != Planet::isUNDEFINED)
					objects.append(object);
			}
			break;
		case PHCSolarSystemMinorBodies: // Minor bodies of Solar system
			for (const auto& object : allObjects)
			{
				if (object->getPlanetType() != Planet::isUNDEFINED && object->getPlanetType() != Planet::isPlanet && object->getPlanetType() != Planet::isStar && object->getPlanetType() != Planet::isMoon && object->getPlanetType() != Planet::isComet && object->getPlanetType() != Planet::isArtificial && object->getPlanetType() != Planet::isObserver && !object->getEnglishName().contains("Pluto", Qt::CaseInsensitive))
					objects.append(object);
			}
			break;
		case PHCMoonsFirstBody: // Moons of first body
		{
			PlanetP firstPlanet = solarSystem->searchByEnglishName(currentPlanet);
			for (const auto& object : allObjects)
			{
				if (object->getParent()==firstPlanet && object->getPlanetType() == Planet::isMoon)
					objects.append(object);
			}
			break;
		}
		case PHCSunPlanetsTheirMoons: // Sun, planets and moons of first body and observer's planet
		{
			PlanetP firstPlanet = solarSystem->searchByEnglishName(currentPlanet);
			PlanetP cp = core->getCurrentPlanet();
			for (const auto& object : allObjects)
			{
				if (object->getEnglishName() != cp->getEnglishName() && object->getEnglishName() != currentPlanet)
				{
					// planets and stars
					if (object->getPlanetType() == Planet::isPlanet || object->getPlanetType() == Planet::isStar)
						objects.append(object);
					// moons of first body and observer's planet
					if ((object->getParent()==cp || object->getParent()==firstPlanet) && object->getPlanetType()==Planet::isMoon)
						objects.append(object);
				}
			}
			break;
		}
	}

	PlanetP planet = solarSystem->searchByEnglishName(currentPlanet);
	PlanetP sun = solarSystem->getSun();
	if (planet)
	{
		const double currentJD = core->getJD();   // save current JD
		startJD = startJD - core->getUTCOffset(startJD) / 24.;
		stopJD = stopJD - core->getUTCOffset(stopJD) / 24.;

		if (obj2Type == PHCLatestSelectedObject)
		{
			QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();
			if (!selectedObjects.isEmpty())
			{
				StelObjectP selectedObject = selectedObjects[0];
				if (selectedObject!=planet && selectedObject->getType() != "Satellite")
				{
					// conjunction
					fillPhenomenaTable(findClosestApproach(planet, selectedObject, startJD, stopJD, separation, PhenomenaTypeIndex::Conjunction), planet, selectedObject, PhenomenaTypeIndex::Conjunction);
					// opposition
					if (opposition)
						fillPhenomenaTable(findClosestApproach(planet, selectedObject, startJD, stopJD, separation, PhenomenaTypeIndex::Opposition), planet, selectedObject, PhenomenaTypeIndex::Opposition);
				}
			}
		}
		else if ((obj2Type >= PHCSolarSystem && obj2Type < PHCBrightStars) || (obj2Type >= PHCInterstellarObjects && obj2Type <= PHCMoonsFirstBody) || (obj2Type==PHCSunPlanetsTheirMoons))
		{
			// Solar system objects
			for (auto& obj : objects)
			{
				// conjunction
				StelObjectP mObj = qSharedPointerCast<StelObject>(obj);
				fillPhenomenaTable(findClosestApproach(planet, mObj, startJD, stopJD, separation, PhenomenaTypeIndex::Conjunction), planet, obj, PhenomenaTypeIndex::Conjunction);
				// opposition
				if (opposition)
					fillPhenomenaTable(findClosestApproach(planet, mObj, startJD, stopJD, separation, PhenomenaTypeIndex::Opposition), planet, obj, PhenomenaTypeIndex::Opposition);
				// shadows from moons
				if (obj2Type==PHCMoonsFirstBody || obj2Type==PHCSolarSystem || obj2Type==PHCBrightSolarSystemObjects || obj2Type==PHCSunPlanetsTheirMoons)
					fillPhenomenaTable(findClosestApproach(planet, mObj, startJD, stopJD, separation, PhenomenaTypeIndex::Shadows), planet, obj, PhenomenaTypeIndex::Shadows);
			}
		}
		else if (obj2Type == PHCBrightStars || obj2Type == PHCBrightDoubleStars || obj2Type == PHCBrightVariableStars || obj2Type == PHCBrightCarbonStars || obj2Type == PHCBrightBariumStars)
		{
			// Stars
			for (auto& obj : star)
			{
				// conjunction
				StelObjectP mObj = qSharedPointerCast<StelObject>(obj);
				fillPhenomenaTable(findClosestApproach(planet, mObj, startJD, stopJD, separation, PhenomenaTypeIndex::Conjunction), planet, obj, PhenomenaTypeIndex::Conjunction);
			}
		}
		else
		{
			// Deep-sky objects
			for (auto& obj : dso)
			{
				// conjunction
				StelObjectP mObj = qSharedPointerCast<StelObject>(obj);
				fillPhenomenaTable(findClosestApproach(planet, mObj, startJD, stopJD, separation, PhenomenaTypeIndex::Conjunction), planet, obj);
			}
		}

		if (planet!=sun && planet->getPlanetType()!=Planet::isMoon)
		{
			StelObjectP mObj = qSharedPointerCast<StelObject>(sun);
			if (quadrature)
			{
				if (planet->getHeliocentricEclipticPos().norm()<core->getCurrentPlanet()->getHeliocentricEclipticPos().norm())
				{
					// greatest elongations for inner planets
					fillPhenomenaTable(findGreatestElongationApproach(planet, mObj, startJD, stopJD), planet, sun, PhenomenaTypeIndex::GreatestElongation);
				}
				else
				{
					// quadratures for outer planets
					fillPhenomenaTable(findQuadratureApproach(planet, mObj, startJD, stopJD), planet, sun, PhenomenaTypeIndex::Quadrature);
				}
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
	enablePhenomenaButtons(true);
}

void AstroCalcDialog::enablePhenomenaButtons(bool enable)
{
	ui->phenomenaCleanupButton->setEnabled(enable);
	ui->phenomenaSaveButton->setEnabled(enable);
}

void AstroCalcDialog::savePhenomena()
{
	QPair<QString, QString> fileData = askTableFilePath(q_("Save calculated phenomena as..."), "phenomena");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->phenomenaTreeWidget, phenomenaHeader);
	else
		saveTableAsXLSX(fileData.first, ui->phenomenaTreeWidget, phenomenaHeader, q_("Phenomena"), q_("Phenomena"));
}

void AstroCalcDialog::fillPhenomenaTableVis(const QString &phenomenType, double JD, const QString &firstObjectName, float firstObjectMagnitude,
					    const QString &secondObjectName, float secondObjectMagnitude, const QString &separation, const QString &elevation,
					    QString &elongation, const QString &angularDistance, const QString &elongTooltip, const QString &angDistTooltip)
{
	ACPhenTreeWidgetItem* treeItem = new ACPhenTreeWidgetItem(ui->phenomenaTreeWidget);
	treeItem->setText(PhenomenaType, phenomenType);
	// local date and time
	treeItem->setText(PhenomenaDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD)));
	treeItem->setData(PhenomenaDate, Qt::UserRole, JD);
	treeItem->setText(PhenomenaObject1, firstObjectName);
	treeItem->setText(PhenomenaMagnitude1, (firstObjectMagnitude > 90.f ? dash : QString::number(firstObjectMagnitude, 'f', 2)));
	treeItem->setTextAlignment(PhenomenaMagnitude1, Qt::AlignRight);
	treeItem->setToolTip(PhenomenaMagnitude1, q_("Magnitude of first object"));
	treeItem->setText(PhenomenaObject2, secondObjectName);
	treeItem->setText(PhenomenaMagnitude2, (secondObjectMagnitude > 90.f ? dash : QString::number(secondObjectMagnitude, 'f', 2)));
	treeItem->setToolTip(PhenomenaMagnitude2, q_("Magnitude of second object"));
	treeItem->setTextAlignment(PhenomenaMagnitude2, Qt::AlignRight);	
	treeItem->setText(PhenomenaSeparation, separation);
	treeItem->setTextAlignment(PhenomenaSeparation, Qt::AlignRight);
	treeItem->setText(PhenomenaElevation, elevation);
	treeItem->setTextAlignment(PhenomenaElevation, Qt::AlignRight);
	treeItem->setToolTip(PhenomenaElevation, q_("Elevation of first object at moment of phenomena"));
	treeItem->setText(PhenomenaElongation, elongation.replace("+","",Qt::CaseInsensitive)); // remove sign
	treeItem->setToolTip(PhenomenaElongation, elongTooltip.isEmpty() ? q_("Angular distance from the Sun") : elongTooltip);
	treeItem->setTextAlignment(PhenomenaElongation, Qt::AlignRight);
	treeItem->setText(PhenomenaAngularDistance, angularDistance);
	treeItem->setToolTip(PhenomenaAngularDistance, angDistTooltip.isEmpty() ? q_("Angular distance from the Moon") : angDistTooltip);
	treeItem->setTextAlignment(PhenomenaAngularDistance, Qt::AlignRight);
}

void AstroCalcDialog::fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const PlanetP object2, int mode)
{
	QMap<double, double>::ConstIterator it;
	PlanetP sun = solarSystem->getSun();
	PlanetP moon = solarSystem->getMoon();
	PlanetP earth = solarSystem->getEarth();
	PlanetP planet = core->getCurrentPlanet();
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	double az, alt;
	for (it = list.constBegin(); it != list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		bool occultation = false;
		const double s1 = object1->getSpheroidAngularRadius(core);
		const double s2 = object2->getSpheroidAngularRadius(core);
		const double d1 = object1->getJ2000EquatorialPos(core).norm();
		const double d2 = object2->getJ2000EquatorialPos(core).norm();
		if (mode==PhenomenaTypeIndex::Shadows) // shadows
		{
			phenomenType = q_("Shadow transit");
			separation = object1->getJ2000EquatorialPos(core).angle(object2->getJ2000EquatorialPos(core));
			if (d1<d2) // the moon is behind planet, so the moon in shadow
				phenomenType = q_("Eclipse");
		}
		else if (mode==PhenomenaTypeIndex::Opposition) // opposition
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
		else if (mode==PhenomenaTypeIndex::Quadrature) // quadratures
		{
			if (separation < 0.0) // we use negative value for eastern quadratures!
			{
				separation *= -1.0;
				phenomenType = q_("Eastern quadrature");
			}
			else
				phenomenType = q_("Western quadrature");
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
				// The passage of a celestial body in front of another of greater apparent diameter
				phenomenType = qc_("Transit", "passage of a celestial body in front of another");
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
		else if (object1 == sun || object2 == sun) // this is may be superior of inferior conjunction for inner planet
		{
			double dcp = planet->getHeliocentricEclipticPos().norm();
			double dp  = (object1 == sun) ? object2->getHeliocentricEclipticPos().norm() :
							object1->getHeliocentricEclipticPos().norm();
			if (dp < dcp) // OK, it's inner planet
			{
				if (object1 == sun)
					phenomenType = d1<d2 ? q_("Superior conjunction") : q_("Inferior conjunction");
				else
					phenomenType = d2<d1 ? q_("Superior conjunction") : q_("Inferior conjunction");
			}
		}

		QString elongStr;
		if (((object1 == sun || object2 == sun) && mode==PhenomenaTypeIndex::Conjunction) || (object2 == sun && mode==PhenomenaTypeIndex::Opposition))
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

		QString angDistStr;
		if (planet != earth || object1 == moon || object2 == moon)
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
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	double az, alt;
	for (it = list.constBegin(); it != list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		bool occultation = false;
		if (separation < (object2->getAngularRadius(core) * M_PI / 180.) || separation < (object1->getSpheroidAngularRadius(core) * M_PI / 180.))
		{
			phenomenType = q_("Occultation");
			occultation = true;
		}

		QString elongStr;
		if (object1 == sun)
			elongStr = dash;
		else
		{
			if (withDecimalDegree)
				elongStr = StelUtils::radToDecDegStr(object1->getElongation(core->getObserverHeliocentricEclipticPos()), 5, false, true);
			else
				elongStr = StelUtils::radToDmsStr(object1->getElongation(core->getObserverHeliocentricEclipticPos()), true);
		}

		QString angDistStr;
		if (planet != earth || object1 == moon)
			angDistStr = dash;
		else
		{
			double angularDistance = object1->getJ2000EquatorialPos(core).angle(moon->getJ2000EquatorialPos(core));
			if (withDecimalDegree)
				angDistStr = StelUtils::radToDecDegStr(angularDistance, 5, false, true);
			else
				angDistStr = StelUtils::radToDmsStr(angularDistance, true);
		}

		QString commonName = object2->getNameI18n();
		if (commonName.isEmpty())
			commonName = object2->getDSODesignation();
		if (commonName.isEmpty())
			commonName = object2->getDSODesignationWIC();

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
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	double az, alt;
	for (it = list.constBegin(); it != list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		bool occultation = false;		
		const double s1 = object1->getSpheroidAngularRadius(core);
		const double s2 = object2->getAngularRadius(core);
		const double d1 = object1->getJ2000EquatorialPos(core).norm();
		const double d2 = object2->getJ2000EquatorialPos(core).norm();
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
				// The passage of a celestial body in front of another of greater apparent diameter
				phenomenType = qc_("Transit", "passage of a celestial body in front of another");
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
		else if (object1 == sun || object2 == sun) // this is may be superior of inferior conjunction for inner planet
		{
			double dcp = (planet->getEquinoxEquatorialPos(core) - sun->getEquinoxEquatorialPos(core)).norm();
			double dp;
			if (object1 == sun)
				dp = (object2->getEquinoxEquatorialPos(core) - sun->getEquinoxEquatorialPos(core)).norm();
			else
				dp = (object1->getEquinoxEquatorialPos(core) - sun->getEquinoxEquatorialPos(core)).norm();
			if (dp < dcp) // OK, it's inner planet
			{
				if (object1 == sun)
					phenomenType = d1<d2 ? q_("Superior conjunction") : q_("Inferior conjunction");
				else
					phenomenType = d2<d1 ? q_("Superior conjunction") : q_("Inferior conjunction");
			}
		}

		QString elongStr;
		if (object1 == sun)
			elongStr = dash;
		else
		{
			if (withDecimalDegree)
				elongStr = StelUtils::radToDecDegStr(object1->getElongation(core->getObserverHeliocentricEclipticPos()), 5, false, true);
			else
				elongStr = StelUtils::radToDmsStr(object1->getElongation(core->getObserverHeliocentricEclipticPos()), true);
		}

		QString angDistStr;
		if (planet != earth || object1 == moon)
			angDistStr = dash;
		else
		{
			double angularDistance = object1->getJ2000EquatorialPos(core).angle(moon->getJ2000EquatorialPos(core));
			if (withDecimalDegree)
				angDistStr = StelUtils::radToDecDegStr(angularDistance, 5, false, true);
			else
				angDistStr = StelUtils::radToDmsStr(angularDistance, true);
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

double AstroCalcDialog::findInitialStep(double startJD, double stopJD, QStringList &objects)
{
	static const QRegularExpression mp("^[(](\\d+)[)]\\s(.+)$");
	static const QMap<QString, double> steps = {
		{ "Moon",     0.25 },
		{ "C/",       0.5  },
		{ "P/",       0.5  },
		{ "Earth",    1.0  },
		{ "Sun",      1.0  },
		{ "Venus",    2.5  },
		{ "Mercury",  2.5  },
		{ "Mars",     5.0  },
		{ "Jupiter", 15.0  },
		{ "Saturn",  15.0  },
		{ "Neptune", 20.0  },
		{ "Uranus",  20.0  },
		{ "Pluto",   20.0  },
	};

	double limit=10.;
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	if (objects.contains(mp))
#else
	if (objects.indexOf(mp)>=0)
#endif
	{
		limit = 24.8 * 365.25;
		QMap<QString, double>::const_iterator it=steps.constBegin();
		while (it != steps.constEnd())
		{
			if (objects.contains(it.key(), Qt::CaseInsensitive))
				limit = qMin(it.value(), limit);
			it++;
		}
	}

	double step = (stopJD - startJD) / 16.0;
	if (step > limit)
		step = limit;

	return step;
}

QMap<double, double> AstroCalcDialog::findClosestApproach(PlanetP& object1, StelObjectP& object2, const double startJD, const double stopJD, const double maxSeparation, const int mode)
{
	QMap<double, double> separations;
	QPair<double, double> extremum;

	if (object2->getType()=="Planet")
	{
		PlanetP planet = qSharedPointerCast<Planet>(object2);
		// We shouldn't compute eclipses and shadow transits not for planets and their moons
		if (mode==PhenomenaTypeIndex::Shadows && object2->getEnglishName()!="Sun" && planet->getParent()!=object1)
			return separations;

		// If we don't have at least partial overlap between planet valid dates and our interval, skip by returning an empty map.
		const Vec2d planetValidityLimits=planet->getValidPositionalDataRange(Planet::PositionQuality::Position);
		if ( (planetValidityLimits[0] > stopJD) || (planetValidityLimits[1] < startJD) )
			return separations;
	}

	QStringList objects;
	Q_ASSERT(objects.isEmpty());
	objects.append(object1->getEnglishName());
	objects.append(object2->getEnglishName());
	double step0 = findInitialStep(startJD, stopJD, objects);
	double step = step0;
	double jd = startJD;
	double prevDist = findDistance(jd, object1, object2, mode);
	int prevSgn = 0;
	jd += step;
	while (jd <= stopJD)
	{
		double dist = findDistance(jd, object1, object2, mode);
		int sgn = StelUtils::sign(dist - prevDist);

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
	if (out == nullptr)
		return false;

	double prevDist = findDistance(JD, object1, object2, mode);
	step = -step / 2.;
	prevSign = -prevSign;

	while (true)
	{
		JD += step;
		double dist = findDistance(JD, object1, object2, mode);

		if (qAbs(step) < 1. / 1440.)
		{
			out->first = JD - step / 2.0;
			out->second = findDistance(JD - step / 2.0, object1, object2, mode);
			if (out->second < findDistance(JD - 5.0, object1, object2, mode))
				return true;
			else
				return false;
		}
		int sgn = StelUtils::sign(dist - prevDist);
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
	else if (mode==PhenomenaTypeIndex::Shadows)
		angle = object1->getHeliocentricEclipticPos().angle(qSharedPointerCast<Planet>(object2)->getHeliocentricEclipticPos());
	return angle;
}

bool AstroCalcDialog::isSecondObjectRight(double JD, PlanetP object1, StelObjectP object2)
{
	core->setJD(JD);
	core->update(0);
	const double angle1 = object1->getJ2000EquatorialPos(core).longitude() * M_180_PI;
	const double angle2 = object2->getJ2000EquatorialPos(core).longitude() * M_180_PI;
	return (StelUtils::fmodpos(angle2-angle1, 360.)<180.);
}

QMap<double, double> AstroCalcDialog::findGreatestElongationApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD)
{
	QMap<double, double> separations;
	QPair<double, double> extremum;

	QStringList objects;
	Q_ASSERT(objects.isEmpty());
	objects.append(object1->getEnglishName());
	objects.append(object2->getEnglishName());
	double step0 = findInitialStep(startJD, stopJD, objects);
	double step = step0;
	double jd = startJD;
	double prevDist = findDistance(jd, object1, object2, PhenomenaTypeIndex::Conjunction);
	jd += step;
	while (jd <= stopJD)
	{
		double dist = findDistance(jd, object1, object2, PhenomenaTypeIndex::Conjunction);
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
					dist = findDistance(jd, object1, object2, PhenomenaTypeIndex::Conjunction);
					if (dist<prevDist)
						break;

					prevDist = dist;
					jd += step;
				}
			}

			double steps = step;
			if (findPreciseGreatestElongation(&extremum, object1, object2, jd, stopJD, step))
			{
				separations.insert(extremum.first, extremum.second);
				jd += 2.*steps;
			}
		}

		prevDist = dist;
		jd += step;
	}
	return separations;
}

bool AstroCalcDialog::findPreciseGreatestElongation(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double stopJD, double step)
{
	if (out == nullptr)
		return false;

	double prevDist = findDistance(JD, object1, object2, PhenomenaTypeIndex::Conjunction);
	step = -step / 2.;

	while (true)
	{
		JD += step;
		double dist = findDistance(JD, object1, object2, PhenomenaTypeIndex::Conjunction);

		if (qAbs(step) < 1. / 1440.)
		{
			out->first = JD - step / 2.0;
			out->second = findDistance(JD - step / 2.0, object1, object2, PhenomenaTypeIndex::Conjunction);
			if (out->second > findDistance(JD - 5.0, object1, object2, PhenomenaTypeIndex::Conjunction))
			{
				if (!isSecondObjectRight(out->first, object1, object2))
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

QMap<double, double> AstroCalcDialog::findQuadratureApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD)
{
	QMap<double, double> separations;
	QPair<double, double> extremum;

	QStringList objects;
	Q_ASSERT(objects.isEmpty());
	objects.append(object1->getEnglishName());
	objects.append(object2->getEnglishName());
	double step0 = findInitialStep(startJD, stopJD, objects);
	double step = step0;
	double jd = startJD;
	double prevDist = findDistance(jd, object1, object2, PhenomenaTypeIndex::Conjunction);
	double limit = M_PI_2 + 0.02;
	jd += step;
	while (jd <= stopJD)
	{
		double dist = findDistance(jd, object1, object2, PhenomenaTypeIndex::Conjunction);
		double factor = qAbs((dist - prevDist) / dist);
		if (factor > 10.)
			step = step0 * factor / 10.;
		else
			step = step0;

		if (dist<limit)
		{
			if (step > step0)
			{
				jd -= step;
				step = step0;
				while (jd <= stopJD)
				{
					dist = findDistance(jd, object1, object2, PhenomenaTypeIndex::Conjunction);
					if (dist>limit)
						break;

					jd += step;
				}
			}

			double steps = step;
			if (findPreciseQuadrature(&extremum, object1, object2, jd, stopJD, step))
			{
				separations.insert(extremum.first, extremum.second);
				jd += 2.0*steps;
			}
		}

		prevDist = dist;
		jd += step;
	}
	return separations;
}

bool AstroCalcDialog::findPreciseQuadrature(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double stopJD, double step)
{
	if (out == nullptr)
		return false;

	step = -step / 288.;

	while (true)
	{
		JD += step;
		double dist = findDistance(JD, object1, object2, PhenomenaTypeIndex::Conjunction);
		double distNext = findDistance(JD + 1., object1, object2, PhenomenaTypeIndex::Conjunction);
		if (((dist-M_PI_2) < 0. && (distNext-M_PI_2) > 0.) || ((dist-M_PI_2) > 0. && (distNext-M_PI_2) < 0.))
		{
			double quadratureJD = JD+(qAbs(dist-M_PI_2)/qAbs(dist-distNext));
			out->first = quadratureJD;
			out->second = findDistance(quadratureJD, object1, object2, PhenomenaTypeIndex::Conjunction);
			if (!isSecondObjectRight(quadratureJD, object1, object2))
				out->second *= -1.0; // let's use negative value for eastern quadratures
			return true;
		}
		else
			return false;

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
	Q_ASSERT(objects.isEmpty());
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

	if (out == nullptr)
		return false;

	prevRA = findRightAscension(JD, object);
	step /= -2.;

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
				step /= -2.0;
		}
		else
		{
			if (RA>prevRA)
				step /= -2.0;
		}
		prevRA = RA;

		if (JD > stopJD)
			return false;
	}
}

// return J2000 RA of a planetary object (without aberration)
double AstroCalcDialog::findRightAscension(double JD, PlanetP object)
{
	const double JDE=JD+core->computeDeltaT(JD)/86400.;
	const Vec3d obs=core->getCurrentPlanet()->getHeliocentricEclipticPos(JDE);
	Vec3d body=object->getHeliocentricEclipticPos(JDE);
	const double distanceCorrection=(body-obs).norm() * (AU / (SPEED_OF_LIGHT * 86400.));
	body=object->getHeliocentricEclipticPos(JDE-distanceCorrection);
	Vec3d bodyJ2000=StelCore::matVsop87ToJ2000.multiplyWithoutTranslation(body - obs);
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, bodyJ2000);
	return ra*M_180_PI;
}

QMap<double, double> AstroCalcDialog::findOrbitalPointApproach(PlanetP &object1, double startJD, double stopJD)
{
	QMap<double, double> separations;
	QPair<double, double> extremum;

	QStringList objects;
	Q_ASSERT(objects.isEmpty());
	objects.append(object1->getEnglishName());
	double step0 = findInitialStep(startJD, stopJD, objects);
	double step = step0;
	double jd = startJD - step;
	double prevDistance = findRightAscension(jd, object1);
	jd += step;
	double stopJDfx = stopJD + step;
	while (jd <= stopJDfx)
	{
		double distance = findHeliocentricDistance(jd, object1);
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
		double distance = findHeliocentricDistance(jd, object1);
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
	double dist, prevDist, timeDist = qAbs(stopJD-JD);

	if (out == nullptr)
		return false;

	prevDist = findHeliocentricDistance(JD, object1);
	step /= -2.;

	bool result;
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
				result = (out->second > findHeliocentricDistance(JD - step / 5.0, object1));
				if (result)
					out->second *= -1;
			}
			else
				result = (out->second < findHeliocentricDistance(JD - step / 5.0, object1));

			return result;
		}
		if (minimal)
		{
			if (dist>prevDist)
				step /= -2.0;
		}
		else
		{
			if (dist<prevDist)
				step /= -2.0;
		}
		prevDist = dist;

		if (JD > stopJD || JD < (stopJD - 2*timeDist))
			return false;
	}
}

void AstroCalcDialog::changePage(QListWidgetItem* current, QListWidgetItem* previous)
{
	if (!current)
		current = previous;

	// reset all flags to make sure only one is set
	plotAltVsTime = false;
	plotAziVsTime = false;
	plotMonthlyElevation = false;
	plotLunarElongationGraph = false;
	plotDistanceGraph = false;

	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));

	// special case
	if (ui->stackListWidget->row(current) == 0)
		currentCelestialPositions();

	// special case - ephemeris
	if (ui->stackListWidget->row(current) == 1)
	{
		double JD = core->getJD() + core->getUTCOffset(core->getJD()) / 24;
		QDateTime currentDT = StelUtils::jdToQDateTime(JD, Qt::LocalTime);
		ui->dateFromDateTimeEdit->setDateTime(currentDT);
		ui->dateToDateTimeEdit->setDateTime(currentDT.addMonths(1));
	}

	// special case - RTS
	if (ui->stackListWidget->row(current) == 2)
		setRTSCelestialBodyName();

	// special case - graphs
	if (ui->stackListWidget->row(current) == 4)
	{
		const int idx = ui->tabWidgetGraphs->currentIndex();
		switch (idx){
			case 0: // 'Alt. vs Time' is visible
				plotAltVsTime = true;
				//qDebug() << "calling drawAltVsTimeDiagram() in changePage";
				drawAltVsTimeDiagram(); // Is object already selected?
				break;
			case 1: //  'Azi. vs Time' is visible
				plotAziVsTime = true;
				drawAziVsTimeDiagram(); // Is object already selected?
				break;
			case 2: // 'Monthly Elevation' is visible
				plotMonthlyElevation = true;
				drawMonthlyElevationGraph(); // Is object already selected?
				break;
			case 3: // 'Graphs' is visible
				updateXVsTimeGraphs();
				break;
			case 4: // 'Angular distance' is visible
				plotLunarElongationGraph = true;
				drawLunarElongationGraph();
				break;
			default:
				qWarning() << "Bad index.";
		}
	}

	// special case (PCalc)
	if (ui->stackListWidget->row(current) == 6)
	{
		int index = ui->tabWidgetPC->currentIndex();
		if (index==0) // First tab: Data
			computePlanetaryData();

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
	plotLunarElongationGraph = false;

	switch (index){
		case 0: // Altitude vs. Time
			plotAltVsTime = true;
			drawAltVsTimeDiagram(); // Is object already selected?
			break;
		case 1: // Azimuth vs. Time
			plotAziVsTime = true;
			drawAziVsTimeDiagram(); // Is object already selected?
			break;
		case 2: // Monthly Elevation
			plotMonthlyElevation = true;
			drawMonthlyElevationGraph(); // Is object already selected?
			break;
		case 3: // Graphs
			updateXVsTimeGraphs();
			break;
		case 4: // Angular Distance
			plotLunarElongationGraph = true;
			drawLunarElongationGraph(); // Is object already selected?
			break;
		default:
			qWarning() << "AstroCalc::changeGraphsTab(): illegal index";
	}
}

void AstroCalcDialog::changePositionsTab(int index)
{
	if (index==1)
		currentHECPositions();
}

void AstroCalcDialog::changeEclipsesTab(int index)
{
	const QMap<int, QString> headermap = {
		{0,	q_("Table of solar eclipses")},
		{1,	q_("Table of solar eclipses visible in current location")},
		{2,	q_("Table of lunar eclipses")},
		{3,	q_("Transits of Mercury and Venus across the Sun")}
		};
	ui->eclipseHeaderLabel->setText(headermap.value(index, q_("Table of solar eclipses")));
}

void AstroCalcDialog::updateTabBarListWidgetWidth()
{
	ui->stackListWidget->setWrapping(false);

	// Update list item sizes after translation
	ui->stackListWidget->adjustSize();

	QAbstractItemModel* model = ui->stackListWidget->model();
	if (!model)
		return;

	// stackListWidget->font() does not work properly!
	// It has a incorrect fontSize in the first loading, which produces the bug#995107.
	QFont font;
	font.setPixelSize(14);
	font.setWeight(QFont::Bold);
	QFontMetrics fontMetrics(font);

	int iconSize = ui->stackListWidget->iconSize().width();

	int width = 0;
	for (int row = 0; row < model->rowCount(); row++)
	{
		int textWidth = fontMetrics.boundingRect(ui->stackListWidget->item(row)->text()).width();
		width += qMax(iconSize, textWidth);
		width += 24; // margin - 12px left and 12px right
	}

	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
	ui->stackListWidget->updateGeometry();
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

	wutCategories = {
		{ q_("Planets"),                              EWPlanets},
		{ q_("Bright stars"),                         EWBrightStars},
		{ q_("Bright nebulae"),                       EWBrightNebulae},
		{ q_("Dark nebulae"),                         EWDarkNebulae},
		{ q_("Galaxies"),                             EWGalaxies},
		{ q_("Open star clusters"),                   EWOpenStarClusters},
		{ q_("Asteroids"),                            EWAsteroids},
		{ q_("Comets"),                               EWComets},
		{ q_("Plutinos"),                             EWPlutinos},
		{ q_("Dwarf planets"),                        EWDwarfPlanets},
		{ q_("Cubewanos"),                            EWCubewanos},
		{ q_("Scattered disc objects"),               EWScatteredDiscObjects},
		{ q_("Oort cloud objects"),                   EWOortCloudObjects},
		{ q_("Sednoids"),                             EWSednoids},
		{ q_("Planetary nebulae"),                    EWPlanetaryNebulae},
		{ q_("Bright double stars"),                  EWBrightDoubleStars},
		{ q_("Bright variable stars"),                EWBrightVariableStars},
		{ q_("Bright stars with high proper motion"), EWBrightStarsWithHighProperMotion},
		{ q_("Symbiotic stars"),                      EWSymbioticStars},
		{ q_("Emission-line stars"),                  EWEmissionLineStars},
		{ q_("Supernova candidates"),                 EWSupernovaeCandidates},
		{ q_("Supernova remnant candidates"),         EWSupernovaeRemnantCandidates},
		{ q_("Supernova remnants"),                   EWSupernovaeRemnants},
		{ q_("Clusters of galaxies"),                 EWClustersOfGalaxies},
		{ q_("Interstellar objects"),                 EWInterstellarObjects},
		{ q_("Globular star clusters"),               EWGlobularStarClusters},
		{ q_("Regions of the sky"),                   EWRegionsOfTheSky},
		{ q_("Active galaxies"),                      EWActiveGalaxies},
		{ q_("Interacting galaxies"),                 EWInteractingGalaxies},
		{ q_("Deep-sky objects"),                     EWDeepSkyObjects},
		{ q_("Messier objects"),                      EWMessierObjects},
		{ q_("NGC/IC objects"),                       EWNGCICObjects},
		{ q_("Caldwell objects"),                     EWCaldwellObjects},
		{ q_("Herschel 400 objects"),                 EWHerschel400Objects},
		{ q_("Algol-type eclipsing systems"),         EWAlgolTypeVariableStars},
		{ q_("The classical cepheids"),               EWClassicalCepheidsTypeVariableStars},
		{ q_("Bright carbon stars"),                  EWCarbonStars},
		{ q_("Bright barium stars"),                  EWBariumStars}};
	if (moduleMgr.isPluginLoaded("Novae"))
		wutCategories.insert(q_("Bright nova stars"), EWBrightNovaStars);
	if (moduleMgr.isPluginLoaded("Supernovae"))
		wutCategories.insert(q_("Bright supernova stars"), EWBrightSupernovaStars);
	// Add the Pulsars category when pulsars are visible
	if (moduleMgr.isPluginLoaded("Pulsars") && propMgr->getProperty("Pulsars.pulsarsVisible")->getValue().toBool())
		wutCategories.insert(q_("Pulsars"), EWPulsars);
	// Add the Exoplanets category when exoplanets are visible
	if (moduleMgr.isPluginLoaded("Exoplanets") && propMgr->getProperty("Exoplanets.showExoplanets")->getValue().toBool())
		wutCategories.insert(q_("Exoplanetary systems"), EWExoplanetarySystems);

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

void AstroCalcDialog::saveWutMinAltitude()
{
	conf->setValue("astrocalc/wut_altitude_min", QString::number(ui->wutAltitudeMinSpinBox->valueDegrees(), 'f', 2));
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
	// TRANSLATORS: IAU Constellation
	wutHeader << qc_("Const.", "IAU Constellation");
	wutHeader << q_("Type");
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
	ui->wutMatchingObjectsTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
}

void AstroCalcDialog::enableAngularLimits(bool enable)
{
	ui->wutAngularSizeLimitCheckBox->setEnabled(enable);
	ui->wutAngularSizeLimitMinSpinBox->setEnabled(enable);
	ui->wutAngularSizeLimitMaxSpinBox->setEnabled(enable);
	if (!enable)
		ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!
}

void AstroCalcDialog::fillWUTTable(const QString &objectName, const QString &designation, float magnitude, const Vec4d &RTSTime, double maxElevation, double angularSize, const QString &constellation, const QString &otype, bool decimalDegrees)
{
	QString sAngularSize = dash;
	QString sRise = dash;
	QString sTransit = dash;
	QString sSet = dash;
	QString sMaxElevation = dash;
	const double utcShift = core->getUTCOffset(core->getJD()) / 24.; // Fix DST shift...

	WUTTreeWidgetItem* treeItem =  new WUTTreeWidgetItem(ui->wutMatchingObjectsTreeWidget);
	treeItem->setData(WUTObjectName, Qt::DisplayRole, objectName);
	treeItem->setData(WUTObjectName, Qt::UserRole, designation);
	treeItem->setText(WUTMagnitude, magnitude > 98.f ? dash : QString::number(magnitude, 'f', 2));
	treeItem->setTextAlignment(WUTMagnitude, Qt::AlignRight);

	sTransit = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(RTSTime[1] + utcShift), true);
	if (RTSTime[3]==0.)
	{
		sRise = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(RTSTime[0] + utcShift), true);
		sSet  = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(RTSTime[2] + utcShift), true);
	}

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

	double angularSizeRad = angularSize * M_PI_180;
	if (angularSize>0.0)
	{
		if (decimalDegrees)
			sAngularSize = StelUtils::radToDecDegStr(angularSizeRad, 5, false, true);
		else
			sAngularSize = StelUtils::radToDmsPStr(angularSizeRad, 2);
	}
	treeItem->setText(WUTAngularSize, sAngularSize);
	treeItem->setTextAlignment(WUTAngularSize, Qt::AlignRight);
	treeItem->setText(WUTConstellation, constellation);
	treeItem->setTextAlignment(WUTConstellation, Qt::AlignCenter);
	treeItem->setToolTip(WUTConstellation, q_("IAU Constellation"));
	treeItem->setText(WUTObjectType, otype);
}

void AstroCalcDialog::calculateWutObjects()
{
	if (ui->wutCategoryListWidget->currentItem())
	{
		QString categoryName = ui->wutCategoryListWidget->currentItem()->text();
		const WUTCategory categoryId = static_cast<WUTCategory> (wutCategories.value(categoryName));

		QList<PlanetP> allObjects = solarSystem->getAllPlanets();
		QVector<NebulaP> allDSO = dsoMgr->getAllDeepSkyObjects();
		QList<StelObjectP> hipStars = starMgr->getHipparcosStars();
		QList<StelObjectP> carbonStars = starMgr->getHipparcosCarbonStars();
		QList<StelObjectP> bariumStars = starMgr->getHipparcosBariumStars();
		QList<StelACStarData> dblHipStars = starMgr->getHipparcosDoubleStars();
		QList<StelACStarData> varHipStars = starMgr->getHipparcosVariableStars();
		QList<StelACStarData> algolTypeStars = starMgr->getHipparcosAlgolTypeStars();
		QList<StelACStarData> classicalCepheidsTypeStars = starMgr->getHipparcosClassicalCepheidsTypeStars();
		QList<StelACStarData> hpmHipStars = starMgr->getHipparcosHighPMStars();

		const Nebula::TypeGroup tflags = static_cast<Nebula::TypeGroup>(dsoMgr->getTypeFilters());
		const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
		const bool angularSizeLimit = ui->wutAngularSizeLimitCheckBox->isChecked();
		bool passByType, visible = true;
		bool enableAngular = true;
		const double angularSizeLimitMin = ui->wutAngularSizeLimitMinSpinBox->valueDegrees();
		const double angularSizeLimitMax = ui->wutAngularSizeLimitMaxSpinBox->valueDegrees();
		const double altitudeLimitMin = ui->wutAltitudeMinSpinBox->valueDegrees();
		const float magLimit = static_cast<float>(ui->wutMagnitudeDoubleSpinBox->value());
		const double JD = core->getJD();
		double alt;
		float mag;
		QSet<QString> objectsList;
		QString designation, starName, constellation, otype;

		ui->wutAngularSizeLimitCheckBox->setText(q_("Limit angular size:"));
		ui->wutAngularSizeLimitCheckBox->setToolTip(q_("Set limits for angular size for visible celestial objects"));
		ui->wutAngularSizeLimitMinSpinBox->setToolTip(q_("Minimal angular size for visible celestial objects"));
		ui->wutAngularSizeLimitMaxSpinBox->setToolTip(q_("Maximum angular size for visible celestial objects"));

		// Direct calculate sunrise/sunset (civil twilight)
		Vec4d rts = GETSTELMODULE(SolarSystem)->getSun()->getRTSTime(core, -6.);
		QList<double> wutJDList;
		wutJDList.clear();

		QComboBox* wut = ui->wutComboBox;
		switch (wut->itemData(wut->currentIndex()).toInt())
		{
			case 1: // Morning
				wutJDList << rts[0];
				break;
			case 2: // Night
				wutJDList << rts[1] + 0.5;
				break;
			case 3:
				wutJDList << rts[0] << rts[1] + 0.5 << rts[2];
				break;
			default: // Evening
				wutJDList << rts[2];
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
				case EWBrightStars:
				case EWCarbonStars:
				case EWBariumStars:
				{
					enableAngular = false;
					QList<StelObjectP> stars;
					if (categoryId==EWBrightStars)
						stars = hipStars;
					else if (categoryId==EWBariumStars)
						stars = bariumStars;
					else
						stars = carbonStars;
					for (const auto& object : qAsConst(stars))
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

								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(object);
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								fillWUTTable(starName, designation, mag, rts, alt, 0.0, constellation, otype, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}					
					break;
				}
				case EWBrightNebulae:
				case EWDarkNebulae:
				case EWGalaxies:
				case EWOpenStarClusters:
				case EWPlanetaryNebulae:
				case EWSymbioticStars:
				case EWEmissionLineStars:
				case EWSupernovaeCandidates:
				case EWSupernovaeRemnantCandidates:
				case EWSupernovaeRemnants:
				case EWClustersOfGalaxies:
				case EWGlobularStarClusters:
				case EWRegionsOfTheSky:
				case EWInteractingGalaxies:
				case EWDeepSkyObjects:
				{
					if (categoryId==EWDarkNebulae)
						initListWUT(false, false); // special case!
					if (categoryId==EWSymbioticStars || categoryId==EWEmissionLineStars || categoryId==EWSupernovaeCandidates)
						enableAngular = false;

					for (const auto& object : allDSO)
					{
						passByType = false;
						mag = object->getVMagnitude(core);
						Nebula::NebulaType ntype = object->getDSOType();						
						switch (categoryId)
						{
							case EWBrightNebulae:
								if (static_cast<bool>(tflags & Nebula::TypeBrightNebulae) && (ntype == Nebula::NebN || ntype == Nebula::NebBn || ntype == Nebula::NebEn || ntype == Nebula::NebRn || ntype == Nebula::NebHII || ntype == Nebula::NebISM || ntype == Nebula::NebCn || ntype == Nebula::NebSNR) && mag <= magLimit)
									passByType = true;
								break;
							case EWDarkNebulae:
								if (static_cast<bool>(tflags & Nebula::TypeDarkNebulae) && (ntype == Nebula::NebDn || ntype == Nebula::NebMolCld	 || ntype == Nebula::NebYSO))
									passByType = true;
								break;
							case EWGalaxies:
								if (static_cast<bool>(tflags & Nebula::TypeGalaxies) && (ntype == Nebula::NebGx) && mag <= magLimit)
									passByType = true;
								break;
							case EWOpenStarClusters:
								if (static_cast<bool>(tflags & Nebula::TypeOpenStarClusters) && (ntype == Nebula::NebCl || ntype == Nebula::NebOc || ntype == Nebula::NebSA || ntype == Nebula::NebSC || ntype == Nebula::NebCn) && mag <= magLimit)
									passByType = true;
								break;
							case EWPlanetaryNebulae:
								if (static_cast<bool>(tflags & Nebula::TypePlanetaryNebulae) && (ntype == Nebula::NebPn || ntype == Nebula::NebPossPN || ntype == Nebula::NebPPN) && mag <= magLimit)
									passByType = true;
								break;
							case EWSymbioticStars:
								if (static_cast<bool>(tflags & Nebula::TypeOther) && (ntype == Nebula::NebSymbioticStar) && mag <= magLimit)
									passByType = true;
								break;
							case EWEmissionLineStars:
								if (static_cast<bool>(tflags & Nebula::TypeOther) && (ntype == Nebula::NebEmissionLineStar) && mag <= magLimit)
									passByType = true;
								break;
							case EWSupernovaeCandidates:
							{
								visible = ((mag <= magLimit) || (mag > 90.f && magLimit >= 19.f));
								if (static_cast<bool>(tflags & Nebula::TypeSupernovaRemnants) && (ntype == Nebula::NebSNC) && visible)
									passByType = true;
								break;
							}
							case EWSupernovaeRemnantCandidates:
							{
								visible = ((mag <= magLimit) || (mag > 90.f && magLimit >= 19.f));
								if (static_cast<bool>(tflags & Nebula::TypeSupernovaRemnants) && (ntype == Nebula::NebSNRC) && visible)
									passByType = true;
								break;
							}
							case EWSupernovaeRemnants:
							{
								visible = ((mag <= magLimit) || (mag > 90.f && magLimit >= 19.f));
								if (static_cast<bool>(tflags & Nebula::TypeSupernovaRemnants) && (ntype == Nebula::NebSNR) && visible)
									passByType = true;
								break;
							}
							case EWClustersOfGalaxies:
								if (static_cast<bool>(tflags & Nebula::TypeGalaxyClusters) && (ntype == Nebula::NebGxCl) && mag <= magLimit)
									passByType = true;
								break;
							case EWGlobularStarClusters:
								if ((static_cast<bool>(tflags & Nebula::TypeGlobularStarClusters) && ntype == Nebula::NebGc) && mag <= magLimit)
									passByType = true;
								break;
							case EWRegionsOfTheSky:
								if (static_cast<bool>(tflags & Nebula::TypeOther) && ntype == Nebula::NebRegion)
									passByType = true;
								break;
							case EWInteractingGalaxies:
								if (static_cast<bool>(tflags & Nebula::TypeInteractingGalaxies) && (ntype == Nebula::NebIGx) && mag <= magLimit)
									passByType = true;
								break;
							case EWDeepSkyObjects:
								if (mag <= magLimit)
									passByType = true;
								if (ntype == Nebula::NebDn)
									mag = 99.f;
								break;
							default:
								break;
						}

						if (passByType && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							if (d.isEmpty())
								d = object->getDSODesignationWIC();
							QString n = object->getNameI18n();

							if ((angularSizeLimit) && (!StelUtils::isWithin(object->getAngularRadius(core), angularSizeLimitMin, angularSizeLimitMax)))
								continue;

							if (d.isEmpty() && n.isEmpty())
								continue;

							designation = QString("%1:%2").arg(d, n);
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								if (d.isEmpty())
									fillWUTTable(n, n, mag, rts, alt, object->getAngularRadius(core), constellation, otype, withDecimalDegree);
								else if (n.isEmpty())
									fillWUTTable(d, d, mag, rts, alt, object->getAngularRadius(core), constellation, otype, withDecimalDegree);
								else
									fillWUTTable(QString("%1 (%2)").arg(d, n), d, mag, rts, alt, object->getAngularRadius(core), constellation, otype, withDecimalDegree);

								objectsList.insert(designation);
							}
						}
					}

					if (categoryId==EWRegionsOfTheSky) // special case!
					{
						ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize);
						ui->wutMatchingObjectsTreeWidget->hideColumn(WUTMagnitude);
					}

					break;
				}
				case EWPlanets:
				case EWAsteroids:
				case EWComets:
				case EWPlutinos:
				case EWDwarfPlanets:
				case EWCubewanos:
				case EWScatteredDiscObjects:
				case EWOortCloudObjects:
				case EWSednoids:
				case EWInterstellarObjects:
				{
					static const QMap<int, Planet::PlanetType>map = {
						{EWPlanets,			Planet::isPlanet},
						{EWAsteroids,			Planet::isAsteroid},
						{EWComets,			Planet::isComet},
						{EWPlutinos,			Planet::isPlutino},
						{EWDwarfPlanets,		Planet::isDwarfPlanet},
						{EWCubewanos,			Planet::isCubewano},
						{EWScatteredDiscObjects,	Planet::isSDO},
						{EWOortCloudObjects,		Planet::isOCO},
						{EWSednoids,			Planet::isSednoid},
						{EWInterstellarObjects,		Planet::isInterstellar}};
					const Planet::PlanetType pType = map.value(categoryId, Planet::isInterstellar);

					for (const auto& object : allObjects)
					{
						mag = object->getVMagnitude(core);
						if (object->getPlanetType() == pType && mag <= magLimit && object->isAboveRealHorizon(core))
						{
							if ((angularSizeLimit) && (!StelUtils::isWithin(object->getAngularRadius(core), angularSizeLimitMin, angularSizeLimitMax)))
								continue;

							designation = object->getEnglishName();
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 2.0*object->getAngularRadius(core), constellation, otype, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}

					if (pType==Planet::isComet)
						ui->wutMatchingObjectsTreeWidget->hideColumn(WUTAngularSize); // special case!

					break;
				}
				case EWBrightDoubleStars:
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
							// convert from arc-seconds to degrees
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

								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(object);
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								fillWUTTable(starName, designation, mag, rts, alt, dblStar.value(object)/3600.0, constellation, otype, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}
					break;
				case EWAlgolTypeVariableStars:
				case EWClassicalCepheidsTypeVariableStars:
				case EWBrightVariableStars:
				{
					enableAngular = false;					
					static QMap<int, QList<StelACStarData>>map = {
						{EWAlgolTypeVariableStars,			algolTypeStars},
						{EWClassicalCepheidsTypeVariableStars,	classicalCepheidsTypeStars},
						{EWBrightVariableStars,				varHipStars}};
					for (const auto& varStar : map.value(categoryId, varHipStars))
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

								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(object);
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								fillWUTTable(starName, designation, mag, rts, alt, 0.0, constellation, otype, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}					
					break;
				}
				case EWBrightStarsWithHighProperMotion:
					enableAngular = false;
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

								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(object);
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								fillWUTTable(starName, designation, mag, rts, alt, 0.0, constellation, otype, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}					
					break;
				case EWActiveGalaxies:
					enableAngular = false;
					for (const auto& object : allDSO)
					{
						passByType = false;
						mag = object->getVMagnitude(core);
						Nebula::NebulaType ntype = object->getDSOType();
						if (static_cast<bool>(tflags & Nebula::TypeActiveGalaxies) && (ntype == Nebula::NebQSO || ntype == Nebula::NebPossQSO || ntype == Nebula::NebAGx || ntype == Nebula::NebRGx || ntype == Nebula::NebBLA || ntype == Nebula::NebBLL) && mag <= magLimit && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							if (d.isEmpty())
								d = object->getDSODesignationWIC();
							QString n = object->getNameI18n();

							if ((angularSizeLimit) && (!StelUtils::isWithin(object->getAngularRadius(core), angularSizeLimitMin, angularSizeLimitMax)))
								continue;

							if (d.isEmpty() && n.isEmpty())
								continue;

							designation = QString("%1:%2").arg(d, n);
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								if (d.isEmpty())
									fillWUTTable(n, n, mag, rts, alt, object->getAngularRadius(core), constellation, otype, withDecimalDegree);
								else if (n.isEmpty())
									fillWUTTable(d, d, mag, rts, alt, object->getAngularRadius(core), constellation, otype, withDecimalDegree);
								else
									fillWUTTable(QString("%1 (%2)").arg(d, n), d, mag, rts, alt, object->getAngularRadius(core), constellation, otype, withDecimalDegree);

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
										rts = object->getRTSTime(core, altitudeLimitMin);
										alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
										constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
										otype = object->getObjectTypeI18n();

										fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 0.0, constellation, otype, withDecimalDegree);
										objectsList.insert(designation);
									}
								}
							}
						}
					}
					#endif					
					break;
				case EWPulsars:
					enableAngular = false;
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

								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								fillWUTTable(starName, designation, 99.f, rts, alt, 0.0, constellation, otype, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}
					ui->wutMatchingObjectsTreeWidget->hideColumn(WUTMagnitude); // special case!					
					#endif
					break;
				case EWExoplanetarySystems:
					enableAngular = false;
					#ifdef USE_STATIC_PLUGIN_EXOPLANETS					
					for (const auto& object : GETSTELMODULE(Exoplanets)->getAllExoplanetarySystems())
					{
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isVMagnitudeDefined() && object->isAboveRealHorizon(core))
						{
							designation = object->getEnglishName();
							if (!objectsList.contains(designation) && !designation.isEmpty())
							{
								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								fillWUTTable(object->getNameI18n().trimmed(), designation, mag, rts, alt, 0.0, constellation, otype, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}					
					#endif
					break;
				case EWBrightNovaStars:
					enableAngular = false;
					#ifdef USE_STATIC_PLUGIN_NOVAE					
					for (const auto& object : GETSTELMODULE(Novae)->getAllBrightNovae())
					{
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isAboveRealHorizon(core))
						{
							designation = object->getEnglishName();
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 0.0, constellation, otype, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}					
					#endif
					break;
				case EWBrightSupernovaStars:
					enableAngular = false;
					#ifdef USE_STATIC_PLUGIN_SUPERNOVAE					
					for (const auto& object : GETSTELMODULE(Supernovae)->getAllBrightSupernovae())
					{
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isAboveRealHorizon(core))
						{
							designation = object->getEnglishName();
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 0.0, constellation, otype, withDecimalDegree);
								objectsList.insert(designation);
							}
						}
					}					
					#endif
					break;
				case EWMessierObjects:
				case EWNGCICObjects:
				case EWCaldwellObjects:
				case EWHerschel400Objects:
				{
					QList<NebulaP> catDSO;
					switch (categoryId)
					{
						case EWMessierObjects:
							catDSO = dsoMgr->getDeepSkyObjectsByType("100");
							break;
						case EWNGCICObjects:
							catDSO = dsoMgr->getDeepSkyObjectsByType("108");
							catDSO.append(dsoMgr->getDeepSkyObjectsByType("109"));
							break;
						case EWCaldwellObjects:
							catDSO = dsoMgr->getDeepSkyObjectsByType("101");
							break;
						case EWHerschel400Objects:
							catDSO = dsoMgr->getDeepSkyObjectsByType("151");
							break;
						default:
							qWarning() << "catDSO: should never come here";
							break;
					}

					for (const auto& object : qAsConst(catDSO))
					{
						mag = object->getVMagnitude(core);
						if (mag <= magLimit && object->isAboveRealHorizon(core))
						{
							QString d = object->getDSODesignation();
							if (d.isEmpty())
								d = object->getDSODesignationWIC();
							QString n = object->getNameI18n();

							if ((angularSizeLimit) && (!StelUtils::isWithin(object->getAngularRadius(core), angularSizeLimitMin, angularSizeLimitMax)))
								continue;

							if (d.isEmpty() && n.isEmpty())
								continue;

							designation = QString("%1:%2").arg(d, n);
							if (!objectsList.contains(designation))
							{
								rts = object->getRTSTime(core, altitudeLimitMin);
								alt = computeMaxElevation(qSharedPointerCast<StelObject>(object));
								constellation = core->getIAUConstellation(object->getEquinoxEquatorialPos(core));
								otype = object->getObjectTypeI18n();

								if (d.isEmpty())
									fillWUTTable(n, n, mag, rts, alt, object->getAngularRadius(core), constellation, otype, withDecimalDegree);
								else if (n.isEmpty())
									fillWUTTable(d, d, mag, rts, alt, object->getAngularRadius(core), constellation, otype, withDecimalDegree);
								else
									fillWUTTable(QString("%1 (%2)").arg(d, n), d, mag, rts, alt, object->getAngularRadius(core), constellation, otype, withDecimalDegree);

								objectsList.insert(designation);
							}
						}
					}
					break;
				}
				default:
					qWarning() << "unknown WUTCategory " << categoryId;
					break;
			}
		}

		enableAngularLimits(enableAngular);
		core->setJD(JD);
		adjustWUTColumns();
		if (!objectsList.isEmpty())
			ui->saveObjectsButton->setEnabled(true);
		else
			ui->saveObjectsButton->setEnabled(false);
		objectsList.clear();		
	}
	else
		ui->saveObjectsButton->setEnabled(false);
}

void AstroCalcDialog::selectWutObject(const QModelIndex &index)
{
	if (index.isValid())
	{
		// Find the object
		QString wutObjectEnglishName = index.sibling(index.row(), WUTObjectName).data(Qt::UserRole).toString();
		if (objectMgr->findAndSelect(wutObjectEnglishName))
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
	QPair<QString, QString> fileData = askTableFilePath(q_("Save list of objects as..."), "wut-objects");
	if (fileData.second.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(fileData.first, ui->wutMatchingObjectsTreeWidget, wutHeader);
	else if (fileData.second.contains(".json", Qt::CaseInsensitive))
		saveTableAsBookmarks(fileData.first, ui->wutMatchingObjectsTreeWidget);
	else
		saveTableAsXLSX(fileData.first, ui->wutMatchingObjectsTreeWidget, wutHeader, q_("What's Up Tonight"), ui->wutCategoryListWidget->currentItem()->text());
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

	const double distanceAu = (posFCB - posSCB).norm();
	const double distanceKm = AU * distanceAu;
	QString degree = QChar(0x00B0);
	// TRANSLATORS: Unit of measure for distance - kilometers
	QString km = qc_("km", "distance");
	// TRANSLATORS: Unit of measure for distance - millions kilometers
	QString Mkm = qc_("M km", "distance");
	const bool useKM = (distanceAu < 0.1);
	QString distAU = QString::number(distanceAu, 'f', 5);
	QString distKM = useKM ? QString::number(distanceKm, 'f', 3) : QString::number(distanceKm / 1.0e6, 'f', 3);

	const double r= posFCB.angle(posSCB);

	unsigned int d, m;
	double s;
	bool sign;

	StelUtils::radToDms(r, sign, d, m, s);
	double dd=r*M_180_PI;

	double spcb1 = firstCBId->getSiderealPeriod();
	double spcb2 = secondCBId->getSiderealPeriod();
	QString parentFCBName = "";
	if (firstCelestialBody != "Sun")
		parentFCBName = firstCBId->getParent()->getEnglishName();
	QString parentSCBName = "";
	if (secondCelestialBody != "Sun")
		parentSCBName = secondCBId->getParent()->getEnglishName();

	QString distanceUM = qc_("AU", "distance, astronomical unit");
	ui->labelLinearDistanceValue->setText(QString("%1 %2 (%3 %4)").arg(distAU, distanceUM, distKM, useKM ? km : Mkm));

	QString angularDistance = dash;
	if (firstCelestialBody != currentPlanet && secondCelestialBody != currentPlanet)
		angularDistance = QString("%1%2 %3' %4\" (%5%2)").arg(d).arg(degree).arg(m).arg(s, 0, 'f', 2).arg(dd, 0, 'f', 5);
	ui->labelAngularDistanceValue->setText(angularDistance);

	// TRANSLATORS: Part of unit of measure for mean motion - degrees per day
	QString day = qc_("day", "mean motion");
	// TRANSLATORS: Unit of measure for period - days
	QString days = qc_("days", "duration");
	QString synodicPeriod = dash;
	QString orbitalPeriodsRatio = dash;
	if (spcb1 > 0.0 && spcb2 > 0.0 && parentFCBName==parentSCBName && firstCelestialBody!="Sun")
	{
		double sp = qAbs(1/(1/spcb1 - 1/spcb2));
		synodicPeriod = QString("%1 %2 (%3 a)").arg(QString::number(sp, 'f', 3), days, QString::number(sp/365.25, 'f', 5));

		double minp = spcb2;
		if (qAbs(spcb1)<=qAbs(spcb2)) { minp = spcb1; }
		int a = qRound(qAbs(spcb1/minp)*10);
		int b = qRound(qAbs(spcb2/minp)*10);
		int lcm = qAbs(a*b)/StelUtils::gcd(a, b);
		orbitalPeriodsRatio = QString("%1:%2").arg(lcm/a).arg(lcm/b);
	}
	ui->labelSynodicPeriodValue->setText(synodicPeriod);
	ui->labelOrbitalPeriodsRatioValue->setText(orbitalPeriodsRatio);

	if (spcb1>0. && firstCelestialBody!="Sun")
		ui->labelMeanMotionFCBValue->setText(QString("%1 %2/%3").arg(QString::number(360./spcb1, 'f', 5), degree, day));
	else
		ui->labelMeanMotionFCBValue->setText(dash);

	if (spcb2>0. && secondCelestialBody!="Sun")
		ui->labelMeanMotionSCBValue->setText(QString("%1 %2/%3").arg(QString::number(360./spcb2, 'f', 5), degree, day));
	else
		ui->labelMeanMotionSCBValue->setText(dash);

	// TRANSLATORS: Unit of measure for speed - kilometers per second
	QString kms = qc_("km/s", "speed");

	double orbVelFCB = firstCBId->getEclipticVelocity().norm();
	QString orbitalVelocityFCB = orbVelFCB<=0. ? dash : QString("%1 %2").arg(QString::number(orbVelFCB * AU/86400., 'f', 3), kms);
	ui->labelOrbitalVelocityFCBValue->setText(orbitalVelocityFCB);

	double orbVelSCB = secondCBId->getEclipticVelocity().norm();
	QString orbitalVelocitySCB = orbVelSCB<=0. ? dash : QString("%1 %2").arg(QString::number(orbVelSCB * AU/86400., 'f', 3), kms);
	ui->labelOrbitalVelocitySCBValue->setText(orbitalVelocitySCB);

	double fcbs = 2.0 * AU * firstCBId->getEquatorialRadius();
	double scbs = 2.0 * AU * secondCBId->getEquatorialRadius();
	double sratio = fcbs/scbs;
	int ss = (sratio < 1.0 ? 6 : 2);

	QString sizeRatio = QString("%1 (%2 %4 / %3 %4)").arg(QString::number(sratio, 'f', ss), QString::number(fcbs, 'f', 1), QString::number(scbs, 'f', 1), km);
	ui->labelEquatorialRadiiRatioValue->setText(sizeRatio);
}

void AstroCalcDialog::drawDistanceGraph()
{
	if (!pcChartMutex.tryLock()) return; // Avoid calling parallel from various sides. (called by signals/slots)

	// special case - plot the graph when tab is visible
	if (!plotDistanceGraph || !dialog->isVisible())
	{
		pcChartMutex.unlock();
		return;
	}

	pcChart = new AstroCalcChart({AstroCalcChart::pcDistanceAU, AstroCalcChart::pcDistanceDeg});
	pcChart->setYrange(AstroCalcChart::pcDistanceAU, 0., 10.); // start with something in case it remains empty.
	pcChart->setYrangeR(AstroCalcChart::pcDistanceDeg, 0., 180.);
	pcChart->setTitle(q_("Linear and angular distances between selected objects"));

	Q_ASSERT(ui->firstCelestialBodyComboBox);
	Q_ASSERT(ui->secondCelestialBodyComboBox);

	QComboBox* fbody = ui->firstCelestialBodyComboBox;
	QComboBox* sbody = ui->secondCelestialBodyComboBox;

	PlanetP currentPlanet = core->getCurrentPlanet();
	PlanetP firstCBId = solarSystem->searchByEnglishName(fbody->currentData(Qt::UserRole).toString());
	PlanetP secondCBId = solarSystem->searchByEnglishName(sbody->currentData(Qt::UserRole).toString());

	if (firstCBId==secondCBId)
	{
		ui->pcChartView->setChart(pcChart);
		pcChartMutex.unlock();
		return;
	}

	int limit = 76, step = 4;
	if (firstCBId->getParent() == currentPlanet || secondCBId->getParent() == currentPlanet)
	{
		limit = 151; step = 2;
	}

	// use full calendar days, not offsets from current JD.
	const double currentJD = core->getJD();
	const double utcOffset=core->getUTCOffset(currentJD)/24.;
	const double baseJD=std::floor(currentJD)+0.5-utcOffset;
	for (int i = -limit; i <= limit; i++)
	{
		double JD = baseJD + i*step;
		core->setJD(JD);
		core->update(0.0);
		Vec3d posFCB = firstCBId->getJ2000EquatorialPos(core);
		Vec3d posSCB = secondCBId->getJ2000EquatorialPos(core);
		double distanceAu = (posFCB - posSCB).norm();
		pcChart->append(AstroCalcChart::pcDistanceAU, StelUtils::jdToQDateTime(JD+utcOffset, Qt::UTC).toMSecsSinceEpoch(), distanceAu);
		if (firstCBId != currentPlanet && secondCBId != currentPlanet)
		{
			double r= posFCB.angle(posSCB);
			pcChart->append(AstroCalcChart::pcDistanceDeg, StelUtils::jdToQDateTime(JD+utcOffset, Qt::UTC).toMSecsSinceEpoch(), r*M_180_PI);
		}
	}
	core->setJD(currentJD);

	QPair<double, double>yRange=pcChart->findYRange(AstroCalcChart::pcDistanceAU);
	pcChart->setYrange(AstroCalcChart::pcDistanceAU, yRange);
	pcChart->show(AstroCalcChart::pcDistanceAU);

	if (pcChart->lengthOfSeries(AstroCalcChart::pcDistanceDeg)>0) // mistake-proofing!
	{
		yRange=pcChart->findYRange(AstroCalcChart::pcDistanceDeg);
		pcChart->setYrangeR(AstroCalcChart::pcDistanceDeg, yRange);
		pcChart->show(AstroCalcChart::pcDistanceDeg);
	}

	pcChart->setupAxes(core->getJD(), 1, "");
	QChart *oldChart=ui->pcChartView->chart();
	if (oldChart) oldChart->deleteLater();
	ui->pcChartView->setChart(pcChart);
	ui->pcChartView->setRenderHint(QPainter::Antialiasing);

	pcChartMutex.unlock();
}

void AstroCalcDialog::drawLunarElongationGraph()
{
	if (!lunarElongationChartMutex.tryLock()) return; // Avoid calling parallel from various sides. (called by signals/slots)

	const QString label = q_("Angular distance between the Moon and selected object");
	ui->lunarElongationChartView->setToolTip(label);

	// special case - plot the graph when tab is visible
	//..
	// we got notified about a reason to redraw the plot, but dialog was
	// not visible. which means we must redraw when becoming visible again!
	if (!dialog->isVisible() && !plotLunarElongationGraph)
	{
		graphPlotNeedsRefresh = true;
		lunarElongationChartMutex.unlock();
		return;
	}

	if (!plotLunarElongationGraph)
	{
		lunarElongationChartMutex.unlock();
		return;
	}

	// special case - the tool is not applicable on non-Earth locations
	if (core->getCurrentPlanet()!=solarSystem->getEarth())
	{
		lunarElongationChartMutex.unlock();
		return;
	}

	lunarElongationChart = new AstroCalcChart({AstroCalcChart::LunarElongation, AstroCalcChart::LunarElongationLimit});
	lunarElongationChart->setTitle(label);
	QPair<double, double>yRange(0., 180.);

	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();
	if (!selectedObjects.isEmpty())
	{
		PlanetP moon = solarSystem->getMoon();
		StelObjectP selectedObject = selectedObjects[0];
		if (selectedObject==moon || selectedObject->getType() == "Satellite")
		{
			// Chart: Do something with the freshly generated but useless chart?
			QChart *oldChart=ui->lunarElongationChartView->chart();
			if (oldChart) oldChart->deleteLater();
			ui->lunarElongationChartView->setChart(lunarElongationChart);
			lunarElongationChartMutex.unlock();
			return;
		}
		const double currentJD = core->getJD();
		int day, month, year;
		StelUtils::getDateFromJulianDay(currentJD, &year, &month, &day);
		double firstJD;
		StelUtils::getJDFromDate(&firstJD, year, month, day, 0, 0, 0.f);
		const double utcOffset=core->getUTCOffset(firstJD)/24.;
		firstJD+=utcOffset;
		// With SplineSeries in charts, even i+=3 may be enough! For finer detail, you may want to split the day in 4-6 parts at cost of speed.
		for (int i = -2; i <= 32; i+=2)
		{
			double JD = firstJD + i;
			core->setJD(JD);
			core->update(0.0);
			const Vec3d moonPosition = moon->getJ2000EquatorialPos(core);
			const Vec3d selectedObjectPosition = selectedObject->getJ2000EquatorialPos(core);
			double distance = moonPosition.angle(selectedObjectPosition);
			lunarElongationChart->append(AstroCalcChart::LunarElongation, StelUtils::jdToQDateTime(JD+utcOffset, Qt::UTC).toMSecsSinceEpoch(), distance*M_180_PI);
		}
		core->setJD(currentJD);
		QString name = getSelectedObjectNameI18n(selectedObject);
		QString otype = selectedObject->getObjectTypeI18n();
		ui->lunarElongationChartView->setToolTip(QString("<p>%1 &quot;%2&quot; (%3)</p>").arg(q_("Angular distance between the Moon and celestial object"), name, otype));
		lunarElongationChart->setTitle(QString("%1: %2 &mdash; %3 (%4)").arg(q_("Angular distance"), moon->getNameI18n(), name, otype));

		yRange=lunarElongationChart->findYRange(AstroCalcChart::LunarElongation);
	}
	else
	{
		lunarElongationChart->clear(AstroCalcChart::LunarElongation);
		lunarElongationChart->setTitle(label);
	}

	lunarElongationChart->setYrange(AstroCalcChart::LunarElongation, yRange);
	lunarElongationChart->show(AstroCalcChart::LunarElongation);
	lunarElongationChart->setupAxes(core->getJD(), 1, "");
	QChart *oldChart=ui->lunarElongationChartView->chart();
	if (oldChart) oldChart->deleteLater();
	ui->lunarElongationChartView->setChart(lunarElongationChart);
	ui->lunarElongationChartView->setRenderHint(QPainter::Antialiasing);

	drawLunarElongationLimitLine();

	lunarElongationChartMutex.unlock();
}

void AstroCalcDialog::saveLunarElongationLimit(int limit)
{
	conf->setValue("astrocalc/angular_distance_limit", limit);
	drawLunarElongationLimitLine();
}

void AstroCalcDialog::drawLunarElongationLimitLine()
{
	// special case - plot the graph when tab is visible
	if (!plotLunarElongationGraph || !dialog->isVisible())
		return;

	double limit = ui->lunarElongationLimitSpinBox->value();
	if(lunarElongationChart)
	{
		lunarElongationChart->drawTrivialLineY(AstroCalcChart::LunarElongationLimit, limit);
		lunarElongationChart->show(AstroCalcChart::LunarElongationLimit);
	}
}

void AstroCalcDialog::saveTableAsCSV(const QString &fileName, QTreeWidget* tWidget, QStringList &headers)
{
	const int count = tWidget->topLevelItemCount();
	const int columns = headers.size();

	QFile table(fileName);
	if (!table.open(QFile::WriteOnly | QFile::Truncate))
	{
		qWarning() << "[AstroCalc] Unable to open file" << QDir::toNativeSeparators(fileName);
		return;
	}

	QTextStream tableData(&table);
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	tableData.setEncoding(QStringConverter::Utf8);
#else
	tableData.setCodec("UTF-8");
#endif
	for (int i = 0; i < columns; i++)
	{
		QString h = headers.at(i).trimmed();
		tableData << ((h.contains(",")) ? QString("\"%1\"").arg(h) : h);
		tableData << ((i < columns - 1) ? delimiter : StelUtils::getEndLineChar());
	}

	for (int i = 0; i < count; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			tableData << tWidget->topLevelItem(i)->text(j);
			tableData << ((j < columns - 1) ? delimiter : StelUtils::getEndLineChar());
		}
	}

	table.close();
}

void AstroCalcDialog::saveTableAsXLSX(const QString& fileName, QTreeWidget* tWidget, QStringList& headers, const QString &title, const QString &sheetName, const QString &note)
{
#ifdef ENABLE_XLSX
	int count = tWidget->topLevelItemCount();
	int columns = headers.size();
	int *width = new int[static_cast<unsigned int>(columns)];
	QString sData;

	QXlsx::Document xlsx;
	xlsx.setDocumentProperty("title", title);
	xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
	xlsx.addSheet(sheetName, AbstractSheet::ST_WorkSheet);

	QXlsx::Format header;
	header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
	header.setPatternBackgroundColor(Qt::yellow);
	header.setBorderStyle(QXlsx::Format::BorderThin);
	header.setBorderColor(Qt::black);
	header.setFontBold(true);
	for (int i = 0; i < columns; i++)
	{
		// Row 1: Names of columns
		sData = headers.at(i).trimmed();
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
			sData = tWidget->topLevelItem(i)->text(j).trimmed();
			xlsx.write(i + 2, j + 1, sData, data);
			int w = sData.size();
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

	if (!note.isEmpty())
	{
		// Add the important note
		xlsx.write(count + 2, 1, note);
		QXlsx::CellRange range = CellRange(count+2, 1, count+2, columns);
		QXlsx::Format extraData;
		extraData.setBorderStyle(QXlsx::Format::BorderThin);
		extraData.setBorderColor(Qt::black);
		extraData.setPatternBackgroundColor(Qt::yellow);
		extraData.setHorizontalAlignment(QXlsx::Format::AlignLeft);
		xlsx.mergeCells(range, extraData);
	}

	xlsx.saveAs(fileName);
#endif
}

QPair<QString, QString> AstroCalcDialog::askTableFilePath(const QString &caption, const QString& fileName)
{
	QString csv  = QString("%1 (*.csv)").arg(q_("CSV (Comma delimited)"));

	#ifdef ENABLE_XLSX
	QString xlsx = QString("%1 (*.xlsx)").arg(q_("Microsoft Excel Open XML Spreadsheet"));
	QString defaultExtension = "xlsx";
	QString filter = QString("%1;;%2").arg(xlsx, csv);
	#else
	QString defaultExtension = "csv";
	QString filter = csv;
	#endif

	QString dir = QString("%1/%2.%3").arg(QDir::homePath(), fileName, defaultExtension);
	QString defaultFilter = QString("(*.%1)").arg(defaultExtension);
	QString filePath = QFileDialog::getSaveFileName(nullptr, caption, dir, filter, &defaultFilter);

	return QPair<QString, QString>(filePath, defaultFilter);
}

void AstroCalcDialog::saveTableAsBookmarks(const QString &fileName, QTreeWidget* tWidget)
{
	const int count = tWidget->topLevelItemCount();

	QFile bookmarksFile(fileName);
	if (!bookmarksFile.open(QFile::WriteOnly | QFile::Truncate))
	{
		qWarning() << "[AstroCalc] Unable to open file" << QDir::toNativeSeparators(fileName);
		return;
	}

	QVariantMap bookmarksDataList;
	double fov = GETSTELMODULE(StelMovementMgr)->getCurrentFov();
	for (int i = 0; i < count; i++)
	{
		QString uuid = QUuid::createUuid().toString();
		QVariantMap bm;
		bm.insert("name", tWidget->topLevelItem(i)->data(0, Qt::UserRole).toString());
		bm.insert("nameI18n", tWidget->topLevelItem(i)->data(0, Qt::DisplayRole).toString());
		bm.insert("fov", fov);
		bookmarksDataList.insert(uuid, bm);
	}

	QVariantMap bmList;
	bmList.insert("bookmarks", bookmarksDataList);

	//Convert the tree to JSON
	StelJsonParser::write(bmList, &bookmarksFile);
	bookmarksFile.flush();
	bookmarksFile.close();
}

QList<PlanetP> AstroCalcDialog::getSelectedMinorPlanets()
{
	// The list of selected dwarf and minor planets is obtained from Astronomical Almanacs
	const QStringList minorPlanets = { "Ceres", "Pallas", "Juno", "Vesta" };
	QList<PlanetP> planets;
	// special case: Pluto
	planets.append(solarSystem->searchByEnglishName("Pluto"));
	for (auto &planet: minorPlanets)
	{
		PlanetP mp = solarSystem->searchMinorPlanetByEnglishName(planet);
		if (!mp.isNull())
			planets.append(mp);
	}

	return planets;
}

void AstroCalcDialog::goToObject(const QString &name, const double JD)
{
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
