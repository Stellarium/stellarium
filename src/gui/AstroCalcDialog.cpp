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
using namespace QtCharts;

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
double AstroCalcDialog::brightLimit = 10.;
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
	Q_ASSERT(ephemerisHeader.isEmpty());
	Q_ASSERT(phenomenaHeader.isEmpty());
	Q_ASSERT(positionsHeader.isEmpty());
	Q_ASSERT(wutHeader.isEmpty());
	Q_ASSERT(rtsHeader.isEmpty());
	Q_ASSERT(lunareclipseHeader.isEmpty());
	Q_ASSERT(solareclipseHeader.isEmpty());
	Q_ASSERT(solareclipselocalHeader.isEmpty());
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
		setHECPositionsHeaderNames();
		setEphemerisHeaderNames();
		setRTSHeaderNames();
		setPhenomenaHeaderNames();
		setWUTHeaderNames();
		setLunarEclipseHeaderNames();
		setSolarEclipseHeaderNames();
		setSolarEclipseLocalHeaderNames();
		populateCelestialBodyList();
		populateCelestialCategoryList();
		populateEphemerisTimeStepsList();
		populatePlanetList();
		populateGroupCelestialBodyList();
		currentCelestialPositions();
		currentHECPositions();
		prepareAxesAndGraph();
		prepareAziVsTimeAxesAndGraph();
		populateFunctionsList();
		prepareXVsTimeAxesAndGraph();
		prepareMonthlyElevationAxesAndGraph();
		prepareDistanceAxesAndGraph();
		prepareAngularDistanceAxesAndGraph();
		drawAltVsTimeDiagram();
		drawAziVsTimeDiagram();
		populateTimeIntervalsList();
		populateWutGroups();
		// Hack to shrink the tabs to optimal size after language change
		// by causing the list items to be laid out again.
		updateTabBarListWidgetWidth();
		// TODO: make a new private function and call that here and in createDialogContent?
		QString validDates = QString("%1 1582/10/15 - 9999/12/31").arg(q_("Gregorian dates. Valid range:"));
		ui->dateFromDateTimeEdit->setToolTip(validDates);
		ui->dateToDateTimeEdit->setToolTip(validDates);
		ui->phenomenFromDateEdit->setToolTip(validDates);
		ui->phenomenToDateEdit->setToolTip(validDates);
		ui->rtsFromDateEdit->setToolTip(validDates);
		ui->rtsToDateEdit->setToolTip(validDates);
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

	// prepare default background gradient for all graphs
	graphBackgroundGradient.setStart(QPointF(0, 0));
	graphBackgroundGradient.setFinalStop(QPointF(0, 1));
	// Colors for gradiaent taken from QSS's QWidget
	graphBackgroundGradient.setColorAt(0.0, QColor(90, 90, 90));
	graphBackgroundGradient.setColorAt(1.0, QColor(70, 70, 70));
	graphBackgroundGradient.setCoordinateMode(QGradient::ObjectBoundingMode);

	initListCelestialPositions();
	initListHECPositions();
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
	prepareMonthlyElevationAxesAndGraph();
	// WUT
	initListWUT();
	populateTimeIntervalsList();
	populateWutGroups();
	// PC
	prepareDistanceAxesAndGraph();
	prepareAngularDistanceAxesAndGraph();

	ui->genericMarkerColor->setText("1");
	ui->secondaryMarkerColor->setText("2");
	ui->mercuryMarkerColor->setText(QChar(0x263F));
	ui->venusMarkerColor->setText(QChar(0x2640));
	ui->marsMarkerColor->setText(QChar(0x2642));
	ui->jupiterMarkerColor->setText(QChar(0x2643));
	ui->saturnMarkerColor->setText(QChar(0x2644));

	const double JD = core->getJD() + core->getUTCOffset(core->getJD()) / 24;
	QDateTime currentDT		= StelUtils::jdToQDateTime(JD);	
	ui->dateFromDateTimeEdit->setDateTime(currentDT);
	ui->dateToDateTimeEdit->setDateTime(currentDT.addMonths(1));
	ui->phenomenFromDateEdit->setDateTime(currentDT);
	ui->phenomenToDateEdit->setDateTime(currentDT.addMonths(1));
	ui->rtsFromDateEdit->setDateTime(currentDT);
	ui->rtsToDateEdit->setDateTime(currentDT.addMonths(1));
	ui->eclipseFromYearSpinBox->setValue(currentDT.date().year());
	ui->monthlyElevationTimeInfo->setStyleSheet("font-size: 18pt; color: rgb(238, 238, 238);");

	// TODO: Replace QDateTimeEdit by a new StelDateTimeEdit widget to apply full range of dates
	// NOTE: https://github.com/Stellarium/stellarium/issues/711
	const QDate minDate = QDate(1582, 10, 15); // QtDateTime's minimum date is 1.1.100AD, but appears to be always Gregorian.
	QString validDates = QString("%1 1582/10/15 - 9999/12/31").arg(q_("Gregorian dates. Valid range:"));
	ui->dateFromDateTimeEdit->setMinimumDate(minDate);
	ui->dateFromDateTimeEdit->setToolTip(validDates);
	ui->dateToDateTimeEdit->setMinimumDate(minDate);
	ui->dateToDateTimeEdit->setToolTip(validDates);
	ui->phenomenFromDateEdit->setMinimumDate(minDate);
	ui->phenomenFromDateEdit->setToolTip(validDates);
	ui->phenomenToDateEdit->setMinimumDate(minDate);
	ui->phenomenToDateEdit->setToolTip(validDates);
	ui->rtsFromDateEdit->setMinimumDate(minDate);
	ui->rtsFromDateEdit->setToolTip(validDates);
	ui->rtsToDateEdit->setMinimumDate(minDate);
	ui->rtsToDateEdit->setToolTip(validDates);
	ui->eclipseFromYearSpinBox->setToolTip(QString("%1 %2..%3").arg(q_("Valid range years:"), QString::number(ui->eclipseFromYearSpinBox->minimum()), QString::number(ui->eclipseFromYearSpinBox->maximum())));
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

	// Tab: Transits
	initListRTS();
	connect(ui->rtsCalculateButton, SIGNAL(clicked()), this, SLOT(generateRTS()));
	connect(ui->rtsCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupRTS()));
	connect(ui->rtsSaveButton, SIGNAL(clicked()), this, SLOT(saveRTS()));
	connect(ui->rtsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentRTS(QModelIndex)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(setRTSCelestialBodyName()));

	// Tab: Eclipses
	initListLunarEclipse();
	connect(ui->lunareclipsesCalculateButton, SIGNAL(clicked()), this, SLOT(generateLunarEclipses()));
	connect(ui->lunareclipsesCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupLunarEclipses()));
	connect(ui->lunareclipsesSaveButton, SIGNAL(clicked()), this, SLOT(saveLunarEclipses()));
	connect(ui->lunareclipseTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentLunarEclipse(QModelIndex)));
	initListSolarEclipse();
	connect(ui->solareclipsesCalculateButton, SIGNAL(clicked()), this, SLOT(generateSolarEclipses()));
	connect(ui->solareclipsesCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupSolarEclipses()));
	connect(ui->solareclipsesSaveButton, SIGNAL(clicked()), this, SLOT(saveSolarEclipses()));
	connect(ui->solareclipseTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentSolarEclipse(QModelIndex)));
	initListSolarEclipseLocal();
	connect(ui->solareclipseslocalCalculateButton, SIGNAL(clicked()), this, SLOT(generateSolarEclipsesLocal()));
	connect(ui->solareclipseslocalCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupSolarEclipsesLocal()));
	connect(ui->solareclipseslocalSaveButton, SIGNAL(clicked()), this, SLOT(saveSolarEclipsesLocal()));
	connect(ui->solareclipselocalTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentSolarEclipseLocal(QModelIndex)));

	// Let's use DMS and decimal degrees as acceptable values for "Maximum allowed separation" input box
	ui->allowedSeparationSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->allowedSeparationSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
	ui->allowedSeparationSpinBox->setMinimum(0.0, true);
	ui->allowedSeparationSpinBox->setMaximum(20.0, true);
	ui->allowedSeparationSpinBox->setWrapping(false);
	ui->allowedSeparationSpinBox->setToolTip(QString("%1: %2 - %3").arg(q_("Valid range"), StelUtils::decDegToDmsStr(ui->allowedSeparationSpinBox->getMinimum(true)), StelUtils::decDegToDmsStr(ui->allowedSeparationSpinBox->getMaximum(true))));

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
	connect(ui->graphsPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseOverGraphs(QMouseEvent*)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(updateXVsTimeGraphs()));	

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
	ui->wutAltitudeMinSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->wutAltitudeMinSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
	ui->wutAltitudeMinSpinBox->setMinimum(0.0, true);
	ui->wutAltitudeMinSpinBox->setMaximum(90.0, true);
	ui->wutAltitudeMinSpinBox->setWrapping(false);

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
	connect(dsoMgr, SIGNAL(catalogFiltersChanged(Nebula::CatalogGroup)), this, SLOT(calculateWutObjects()));
	connect(dsoMgr, SIGNAL(typeFiltersChanged(Nebula::TypeGroup)), this, SLOT(calculateWutObjects()));
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
	connect(ui->pcDistanceGraphPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseOverDistanceGraph(QMouseEvent*)));
	connect(core, SIGNAL(dateChanged()), this, SLOT(drawDistanceGraph()));

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

	// Let's improve visibility of the text
	QString style = "QLabel { color: rgb(238, 238, 238); }";
	ui->celestialPositionsTimeLabel->setStyleSheet(style);
	ui->hecPositionsTimeLabel->setStyleSheet(style);
	ui->altVsTimeLabel->setStyleSheet(style);
	//ui->altVsTimeTitle->setStyleSheet(style);
	ui->aziVsTimeLabel->setStyleSheet(style);
	//ui->aziVsTimeTitle->setStyleSheet(style);
	ui->monthlyElevationLabel->setStyleSheet(style);
	//ui->monthlyElevationTitle->setStyleSheet(style);
	ui->graphsFirstLabel->setStyleSheet(style);	
	ui->graphsSecondLabel->setStyleSheet(style);	
	ui->graphsDurationLabel->setStyleSheet(style);
	ui->graphsYearsLabel->setStyleSheet(style);
	ui->angularDistanceNote->setStyleSheet(style);
	ui->angularDistanceLimitLabel->setStyleSheet(style);	
	//ui->angularDistanceTitle->setStyleSheet(style);
	ui->graphsNoteLabel->setStyleSheet(style);
	ui->rtsNotificationLabel->setStyleSheet(style);
	ui->gammaNoteLabel->setStyleSheet(style);
	ui->gammaNoteSolarEclipseLabel->setStyleSheet(style);
	ui->UncertaintiesNoteLabel->setStyleSheet(style);
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
		ui->aziVsTimeTitle->setText(selectedObject->getNameI18n());
		double currentJD = core->getJD();
		double shift = core->getUTCOffset(currentJD) / 24.0;
		double noon = static_cast<int>(currentJD + shift);
		double az, alt, deg;
		bool sign;

		int step = 180;
		int limit = 485;
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

		for (int i = -5; i <= limit; i++) // 24 hours + 15 minutes in both directions
		{
			// A new point on the graph every 3 minutes with shift to right 12 hours
			// to get midnight at the center of diagram (i.e. accuracy is 3 minutes)
			double ltime = i * step + 43200;
			aX.append(ltime);
			double JD = noon + ltime / 86400 - shift - 0.5;
			core->setJD(JD);
			
#ifdef USE_STATIC_PLUGIN_SATELLITES
			if (isSatellite)
			{
				sat->update(0.0);
			}
			else
#endif
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
			{
				name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
				if (name.isEmpty())
					name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignationWIC();
			}

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

	if (ui->aziVsTimePlot->xAxis->range().contains(x) && ui->aziVsTimePlot->yAxis->range().contains(y))
	{
		QString info = "";
		if (graph)
		{
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
		}
		ui->aziVsTimePlot->setToolTip(info);
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
	// TRANSLATORS: angular size, arc-minutes
	positionsHeader << QString("%1, '").arg(q_("A.S."));
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
	hecPositionsHeader.clear();
	// TRANSLATORS: name of object
	hecPositionsHeader << q_("Name");
	// TRANSLATORS: ecliptic latitude
	hecPositionsHeader << q_("Latitude");
	// TRANSLATORS: ecliptic longitude
	hecPositionsHeader << q_("Longitude");
	// TRANSLATORS: distance
	hecPositionsHeader << q_("Distance");

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
	const double lat = static_cast<double>(core->getCurrentLocation().latitude);
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

void AstroCalcDialog::fillCelestialPositionTable(QString objectName, QString RA, QString Dec, double magnitude,
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
				if (rts[1]>=0.)
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

				fillCelestialPositionTable(dsoName, coordStrings.first, coordStrings.second, magOp, angularSize, asToolTip, extra, mu, sTransit, sMaxElevation, elongStr, q_(obj->getTypeString()));
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
		QString asToolTip = QString("%1, %2").arg(q_("Angular size (with rings, if any)"), q_("arc-min"));
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

				QString extra = QString::number(pos.length(), 'f', 5); // A.U.

				// Convert to arc-seconds the angular size of Solar system object (with rings, if any)
				QString angularSize = QString::number(planet->getAngularRadius(core) * 120., 'f', 4);
				if (angularSize.toFloat() < 1e-4f || planet->getPlanetType() == Planet::isComet)
					angularSize = dash;

				QString sTransit = dash;
				QString sMaxElevation = dash;
				QString elongStr;
				rts = planet->getRTSTime(core);
				if (rts[1]>=0.)
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

				fillCelestialPositionTable(planet->getNameI18n(), coordStrings.first, coordStrings.second, planet->getVMagnitudeWithExtinction(core), angularSize, asToolTip, extra, sToolTip, sTransit, sMaxElevation, elongStr, q_(planet->getPlanetTypeString()));
			}
		}		
	}
	else
	{
		// stars
		QString sType = q_("star");
		QString commonName, sToolTip = "";
		QList<StelACStarData> celestialObjects;
		if (celTypeId == 170)
		{
			// double stars
			celestialObjects = starMgr->getHipparcosDoubleStars();
			sType = q_("double star");
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
			sType = q_("variable star");
		}
		else
		{
			// stars with high proper motion
			celestialObjects = starMgr->getHipparcosHighPMStars();
			sType = q_("star with high proper motion");
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

				fillCelestialPositionTable(commonName, coordStrings.first, coordStrings.second, obj->getVMagnitudeWithExtinction(core), dash, "", extra, sToolTip, sTransit, sMaxElevation, elongStr, sType);
			}
		}
		ui->celestialPositionsTreeWidget->hideColumn(CColumnAngularSize);
	}

	adjustCelestialPositionsColumns();
	// sort-by-name
	ui->celestialPositionsTreeWidget->sortItems(CColumnName, Qt::AscendingOrder);
}

QPair<QString, QString> AstroCalcDialog::getStringCoordinates(const Vec3d coord, const bool horizontal, const bool southAzimuth, const bool decimalDegrees)
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
		int *width = new int[static_cast<unsigned int>(columns)];
		QString sData;

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

void AstroCalcDialog::fillHECPositionTable(QString objectName, QString latitude, QString longitude, double distance)
{
	AHECPosTreeWidgetItem* treeItem = new AHECPosTreeWidgetItem(ui->hecPositionsTreeWidget);
	treeItem->setText(HECColumnName, objectName);
	treeItem->setTextAlignment(HECColumnName, Qt::AlignLeft);
	treeItem->setText(HECColumnLatitude, latitude);
	treeItem->setTextAlignment(HECColumnLatitude, Qt::AlignRight);
	treeItem->setText(HECColumnLongitude, longitude);
	treeItem->setTextAlignment(HECColumnLongitude, Qt::AlignRight);
	treeItem->setText(HECColumnDistance, QString("%1 %2").arg(QString::number(distance, 'f', 2), qc_("AU", "distance, astronomical unit")));
	treeItem->setData(HECColumnDistance, Qt::UserRole, distance);
	treeItem->setTextAlignment(HECColumnDistance, Qt::AlignRight);
	treeItem->setToolTip(HECColumnDistance, q_("Distance from the Sun at the moment of computation of position"));
}

void AstroCalcDialog::currentHECPositions()
{
	QPair<QString, QString> coordStrings;
	hecObjects.clear();
	initListHECPositions();
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

	double distance, longitude, latitude, dl;
	Vec3d pos;
	bool sign;
	HECPosition object;
	const double JD = core->getJD();
	ui->hecPositionsTimeLabel->setText(q_("Positions on %1").arg(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))));

	QList<PlanetP> planets = solarSystem->getAllPlanets();
	for (const auto& planet : qAsConst(planets))
	{
		if (planet->getPlanetType() == Planet::isPlanet)
		{
			pos = planet->getHeliocentricEclipticPos();
			distance = pos.length();
			StelUtils::rectToSphe(&longitude, &latitude, pos);
			if (longitude<0) longitude+=2.0*M_PI;
			StelUtils::radToDecDeg(longitude, sign, dl);
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

			fillHECPositionTable(planet->getNameI18n(), coordStrings.first, coordStrings.second, distance);
			object.objectName = planet->getNameI18n();
			object.x = 360.-dl;
			object.y = log(distance);
			hecObjects.append(object);
		}
	}

	adjustHECPositionsColumns();
	// sort-by-distance
	ui->hecPositionsTreeWidget->sortItems(HECColumnDistance, Qt::AscendingOrder);

	drawHECGraph();
}

void AstroCalcDialog::drawHECGraph(QString selectedObject)
{
	QScatterSeries *seriesPlanets = new QScatterSeries();
	QScatterSeries *seriesSelectedPlanet = new QScatterSeries();
	QScatterSeries *seriesSun = new QScatterSeries();
	seriesSun->append(0., -1.5);

	for (const auto& planet : qAsConst(hecObjects))
	{
		seriesPlanets->append(planet.x, planet.y);
		if (!selectedObject.isEmpty() && planet.objectName==selectedObject)
			seriesSelectedPlanet->append(planet.x, planet.y);
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
	radialAxis->append("0.5", log(.5));  // a few stop marks for AU values
	radialAxis->append("1", log(1));
	radialAxis->append("2", log(2));
	radialAxis->append("5", log(5));
	radialAxis->append("10", log(10));
	radialAxis->append("20", log(20));
	radialAxis->append("30", log(30));
	radialAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
	radialAxis->setLabelsColor(labelColor);
	radialAxis->setGridLineColor(axisColor);
	radialAxis->setLineVisible(false);
	radialAxis->setRange(-1.5, log(32));
	chart->addAxis(radialAxis, QPolarChart::PolarOrientationRadial);

	seriesPlanets->attachAxis(angularAxis);
	seriesPlanets->attachAxis(radialAxis);
	seriesPlanets->setMarkerSize(5);
	seriesPlanets->setColor(Qt::cyan);
	seriesPlanets->setBorderColor(Qt::transparent);

	seriesSelectedPlanet->attachAxis(angularAxis);
	seriesSelectedPlanet->attachAxis(radialAxis);
	seriesSelectedPlanet->setMarkerSize(7);
	seriesSelectedPlanet->setColor(Qt::green);
	seriesSelectedPlanet->setBorderColor(Qt::transparent);

	seriesSun->attachAxis(angularAxis);
	seriesSun->attachAxis(radialAxis);
	seriesSun->setMarkerSize(9);
	seriesSun->setColor(Qt::yellow);
	seriesSun->setBorderColor(Qt::red);

	ui->hecPositionsGraph->setChart(chart);
	ui->hecPositionsGraph->setBackgroundBrush(graphBackgroundGradient);
}

void AstroCalcDialog::saveHECPositions()
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
		saveTableAsCSV(filePath, ui->hecPositionsTreeWidget, hecPositionsHeader);
	else
	{
		int count = ui->hecPositionsTreeWidget->topLevelItemCount();
		int columns = hecPositionsHeader.size();
		int *width = new int[static_cast<unsigned int>(columns)];
		QString sData;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("Heliocentric ecliptic positions of the major planets"));
		xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
		xlsx.addSheet(q_("Major planets"), AbstractSheet::ST_WorkSheet);

		QXlsx::Format header;
		header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		header.setPatternBackgroundColor(Qt::yellow);
		header.setBorderStyle(QXlsx::Format::BorderThin);
		header.setBorderColor(Qt::black);
		header.setFontBold(true);
		for (int i = 0; i < columns; i++)
		{
			// Row 1: Names of columns
			sData = hecPositionsHeader.at(i).trimmed();
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
				sData = ui->hecPositionsTreeWidget->topLevelItem(i)->text(j).trimmed();
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

		// Add the date and time info for celestial positions
		xlsx.write(count + 2, 1, ui->hecPositionsTimeLabel->text());
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
	const QString currentPlanet = ui->celestialBodyComboBox->currentData(Qt::UserRole).toString();
	const QString secondaryPlanet = ui->secondaryCelestialBodyComboBox->currentData(Qt::UserRole).toString();
	const QString distanceInfo = (core->getUseTopocentricCoordinates() ? q_("Topocentric distance") : q_("Planetocentric distance"));
	const QString distanceUM = qc_("AU", "distance, astronomical unit");
	QString englishName, nameI18n, elongStr = "", phaseStr = "";
	const bool useHorizontalCoords = ui->ephemerisHorizontalCoordinatesCheckBox->isChecked();
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

			if (!obj->hasValidPositionalData(JD, Planet::PositionQuality::OrbitPlotting))
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
			treeItem->setText(EphemerisDistance, QString::number(obj->getJ2000EquatorialPos(core).length(), 'f', 6));
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
	QString filter = q_("Microsoft Excel Open XML Spreadsheet");
	filter.append(" (*.xlsx);;");
	filter.append(q_("CSV (Comma delimited)"));
	filter.append(" (*.csv)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save calculated ephemeris as..."),
							QDir::homePath() + "/ephemeris.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->ephemerisTreeWidget, ephemerisHeader);
	else
	{
		int count = ui->ephemerisTreeWidget->topLevelItemCount();
		int columns = ephemerisHeader.size();
		int *width = new int[static_cast<unsigned int>(columns)];
		QString sData;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("Ephemeris"));
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
		xlsx.saveAs(filePath);
	}
}

void AstroCalcDialog::cleanupEphemeris()
{
	EphemerisList.clear();
	ui->ephemerisTreeWidget->clear();
}

void AstroCalcDialog::setRTSHeaderNames()
{
	rtsHeader.clear();
	rtsHeader << q_("Name");
	rtsHeader << qc_("Rise", "celestial event");
	rtsHeader << qc_("Transit", "celestial event; passage across a meridian");
	rtsHeader << qc_("Set", "celestial event");
	// TRANSLATORS: altitude
	rtsHeader << q_("Altitude");
	// TRANSLATORS: magnitude
	rtsHeader << q_("Mag.");
	rtsHeader << q_("Solar Elongation");
	rtsHeader << q_("Lunar Elongation");
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
			startJD = startJD - core->getUTCOffset(startJD) / 24.;
			stopJD = stopJD - core->getUTCOffset(stopJD) / 24.;
			int elements = static_cast<int>((stopJD - startJD) / currentStep);
			double JD, az, alt;
			float magnitude;
			QString riseStr, setStr, altStr, magStr, elongSStr = dash, elongLStr = dash;
			for (int i = 0; i <= elements; i++)
			{
				JD = startJD + i * currentStep + 1;
				core->setJD(JD);
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

				if (rts[3]==0.)
				{
					riseStr = QString("%1 %2").arg(localeMgr->getPrintableDateLocal(rts[0]), localeMgr->getPrintableTimeLocal(rts[0]));
					setStr = QString("%1 %2").arg(localeMgr->getPrintableDateLocal(rts[2]), localeMgr->getPrintableTimeLocal(rts[2]));
				}
				else
					riseStr = setStr = dash;

				ACRTSTreeWidgetItem* treeItem = new ACRTSTreeWidgetItem(ui->rtsTreeWidget);
				treeItem->setText(RTSCOName, name);
				treeItem->setData(RTSCOName, Qt::UserRole, englishName);
				treeItem->setText(RTSRiseDate, riseStr); // local date and time
				treeItem->setData(RTSRiseDate, Qt::UserRole, rts[0]);
				treeItem->setTextAlignment(RTSRiseDate, Qt::AlignRight);
				treeItem->setText(RTSTransitDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))); // local date and time
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
		}
		else
			cleanupRTS();
	}
}

void AstroCalcDialog::cleanupRTS()
{
	ui->rtsTreeWidget->clear();
}

void AstroCalcDialog::selectCurrentRTS(const QModelIndex& modelIndex)
{
	// Find the object
	QString name = modelIndex.sibling(modelIndex.row(), RTSCOName).data(Qt::UserRole).toString();
	double JD = modelIndex.sibling(modelIndex.row(), RTSTransitDate).data(Qt::UserRole).toDouble();

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
	QString filter = q_("Microsoft Excel Open XML Spreadsheet");
	filter.append(" (*.xlsx);;");
	filter.append(q_("CSV (Comma delimited)"));
	filter.append(" (*.csv)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save calculated data as..."),
							QDir::homePath() + "/RTS.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->rtsTreeWidget, ephemerisHeader);
	else
	{
		int count = ui->rtsTreeWidget->topLevelItemCount();
		int columns = rtsHeader.size();
		int *width = new int[static_cast<unsigned int>(columns)];
		QString sData;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("Transits"));
		xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
		xlsx.addSheet(ui->rtsCelestialBodyNameLabel->text(), AbstractSheet::ST_WorkSheet);

		QXlsx::Format header;
		header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		header.setPatternBackgroundColor(Qt::yellow);
		header.setBorderStyle(QXlsx::Format::BorderThin);
		header.setBorderColor(Qt::black);
		header.setFontBold(true);
		for (int i = 0; i < columns; i++)
		{
			// Row 1: Names of columns
			sData = rtsHeader.at(i).trimmed();
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
				sData = ui->rtsTreeWidget->topLevelItem(i)->text(j).trimmed();
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
		xlsx.saveAs(filePath);
	}
}

void AstroCalcDialog::setLunarEclipseHeaderNames()
{
	lunareclipseHeader.clear();
	lunareclipseHeader << q_("Date and Time");
	lunareclipseHeader << q_("Saros");
	lunareclipseHeader << q_("Type");
	lunareclipseHeader << q_("Gamma");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	lunareclipseHeader << qc_("Penumbral eclipse magnitude", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	lunareclipseHeader << qc_("Umbral eclipse magnitude", "column name");	
	// TRANSLATORS: Visibility conditions; the name of column in AstroCalc/Eclipses tool
	lunareclipseHeader << qc_("Vis. Cond.", "column name");
	ui->lunareclipseTreeWidget->setHeaderLabels(lunareclipseHeader);

	// adjust the column width
	for (int i = 0; i < LunarEclipseCount; ++i)
	{
		ui->lunareclipseTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListLunarEclipse()
{
	ui->lunareclipseTreeWidget->clear();
	ui->lunareclipseTreeWidget->setColumnCount(LunarEclipseCount);
	setLunarEclipseHeaderNames();
	ui->lunareclipseTreeWidget->header()->setSectionsMovable(false);
	ui->lunareclipseTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
}

QPair<double,double> AstroCalcDialog::getLunarEclipseXY() const
{
	// Source: Explanatory Supplement to the Astronomical Ephemeris 
	// and the American Ephemeris and Nautical Almanac (1961)
	// Algorithm adaped from Planet::getLunarEclipseMagnitudes() -- we need only x and y here.
	// Find x, y of Besselian elements
	QPair<double,double> LunarEclipseXY;
	// Use geocentric coordinates
	StelCore* core = StelApp::getInstance().getCore();
	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	core->setUseTopocentricCoordinates(false);
	core->update(0);

	double raMoon, deMoon, raSun, deSun;
	StelUtils::rectToSphe(&raSun, &deSun, ssystem->getSun()->getEquinoxEquatorialPos(core));
	StelUtils::rectToSphe(&raMoon, &deMoon, ssystem->getMoon()->getEquinoxEquatorialPos(core));

	// R.A./Dec of Earth's shadow
	const double raShadow = StelUtils::fmodpos(raSun + M_PI, 2.*M_PI);
	const double deShadow = -(deSun);
	const double raDiff = StelUtils::fmodpos(raMoon - raShadow, 2.*M_PI);

	double x = cos(deMoon) * sin(raDiff);
	x *= 3600. * M_180_PI;
	double y = cos(deShadow) * sin(deMoon) - sin(deShadow) * cos(deMoon) * cos(raDiff);
	y *= 3600. * M_180_PI;

	LunarEclipseXY.first = x;
	LunarEclipseXY.second = y;

	return LunarEclipseXY;
}

void AstroCalcDialog::generateLunarEclipses()
{
	const bool onEarth = core->getCurrentPlanet()==solarSystem->getEarth();
	if (onEarth) // Not sure it's right thing to do but should be ok.
	{
		initListLunarEclipse();

		const double currentJD = core->getJD();   // save current JD
		double startyear = ui->eclipseFromYearSpinBox->value();
		double years = ui->eclipseYearsSpinBox->value();
		double startJD, stopJD;
		StelUtils::getJDFromDate(&startJD, startyear, 1, 1, 0, 0, 1);
		StelUtils::getJDFromDate(&stopJD, startyear+years, 12, 31, 23, 59, 59);
		startJD = startJD - core->getUTCOffset(startJD) / 24.;
		stopJD = stopJD - core->getUTCOffset(stopJD) / 24.;
		int elements = static_cast<int>((stopJD - startJD) / 29.530588853);
		QString sarosStr, eclipseTypeStr, uMagStr, pMagStr, gammaStr, visibilityConditionsStr, visibilityConditionsTooltip;

		const bool saveTopocentric = core->getUseTopocentricCoordinates();
		const double approxJD = 2451550.09765;
		const double synodicMonth = 29.530588853;
		bool sign;

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

				double az, alt, altitude;

				// Find exact time of closest approach between the Moon and shadow centre
				double dt = 1.;
				int iteration = 0;
				while (abs(dt)>(0.1/86400.) && (iteration < 20)) // 0.1 second of accuracy
				{
					core->setJD(JD);
					core->update(0);
					QPair<double,double> XY = getLunarEclipseXY();
					double x = XY.first;
					double y = XY.second;
					core->setJD(JD - 5./1440.);
					core->update(0);
					XY = getLunarEclipseXY();
					double x1 = XY.first;
					double y1 = XY.second;
					core->setJD(JD + 5./1440.);
					core->update(0);
					XY = getLunarEclipseXY();
					double x2 = XY.first;
					double y2 = XY.second;

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

				// Check for eclipse
				// Source: Explanatory Supplement to the Astronomical Ephemeris 
				// and the American Ephemeris and Nautical Almanac (1961)
				// Algorithm taken from Planet::getLunarEclipseMagnitudes()
				
				QPair<double,double> XY = getLunarEclipseXY();				
				double x = XY.first;
				double y = XY.second;

				const double dist=moon->getEclipticPos().length();  // geocentric Lunar distance [AU]
				const double mSD=atan(moon->getEquatorialRadius()/dist) * M_180_PI*3600.; // arcsec
				const QPair<Vec3d,Vec3d>shadowRadii=ssystem->getEarthShadowRadiiAtLunarDistance();
				const double f1 = shadowRadii.second[0]; // radius of penumbra at the distance of the Moon
				const double f2 = shadowRadii.first[0];  // radius of umbra at the distance of the Moon

				const double m = sqrt(x * x + y * y); // distance between lunar centre and shadow centre
				const double L1 = f1 + mSD; // distance between center of the Moon and shadow at beginning and end of penumbral eclipse
				const double L2 = f2 + mSD; // distance between center of the Moon and shadow at beginning and end of partial eclipse
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
					StelUtils::radToDecDeg(alt, sign, altitude);

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

					if (!sign)
					{
						visibilityConditionsStr = qc_("Invisible", "visibility conditions");
						visibilityConditionsTooltip = q_("The greatest eclipse is invisible in current location");
						altitude *= -1.;
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
					double gamma = m*0.2725/mSD;
					if (y<0.) gamma = -(gamma);

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
	}
	else
		cleanupLunarEclipses();
}

void AstroCalcDialog::cleanupLunarEclipses()
{
	ui->lunareclipseTreeWidget->clear();
}

void AstroCalcDialog::selectCurrentLunarEclipse(const QModelIndex& modelIndex)
{
	// Find the Moon
	QString name = "Moon";
	double JD = modelIndex.sibling(modelIndex.row(), LunarEclipseDate).data(Qt::UserRole).toDouble();

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

void AstroCalcDialog::saveLunarEclipses()
{
	QString filter = q_("Microsoft Excel Open XML Spreadsheet");
	filter.append(" (*.xlsx);;");
	filter.append(q_("CSV (Comma delimited)"));
	filter.append(" (*.csv)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save calculated lunar eclipses as..."),
							QDir::homePath() + "/lunareclipses.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->lunareclipseTreeWidget, ephemerisHeader);
	else
	{
		int count = ui->lunareclipseTreeWidget->topLevelItemCount();
		int columns = lunareclipseHeader.size();
		int *width = new int[static_cast<unsigned int>(columns)];
		QString sData;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("Lunar Eclipses"));
		xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
		xlsx.addSheet("Lunar Eclipses", AbstractSheet::ST_WorkSheet);

		QXlsx::Format header;
		header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		header.setPatternBackgroundColor(Qt::yellow);
		header.setBorderStyle(QXlsx::Format::BorderThin);
		header.setBorderColor(Qt::black);
		header.setFontBold(true);
		for (int i = 0; i < columns; i++)
		{
			// Row 1: Names of columns
			sData = lunareclipseHeader.at(i).trimmed();
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
				sData = ui->lunareclipseTreeWidget->topLevelItem(i)->text(j).trimmed();
				xlsx.write(i + 2, j + 1, sData, data);
				int w = sData.size();
				if (w > width[j])
				{
					width[j] = w;
				}
			}
		}

		xlsx.write(count+3, 1, q_("Note: Local circumstances for eclipses during thousands of years in the past and future are not reliable due to uncertainty in ΔT which is caused by fluctuations in Earth's rotation."));

		for (int i = 0; i < columns; i++)
		{
			xlsx.setColumnWidth(i+1, width[i]+2);
		}

		delete[] width;
		xlsx.saveAs(filePath);
	}
}

void AstroCalcDialog::setSolarEclipseHeaderNames()
{
	solareclipseHeader.clear();
	solareclipseHeader << q_("Date and Time");
	solareclipseHeader << q_("Saros");
	solareclipseHeader << q_("Type");
	solareclipseHeader << q_("Gamma");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipseHeader << qc_("Eclipse Magnitude", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipseHeader << qc_("Latitude", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipseHeader << qc_("Longitude", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipseHeader << qc_("Altitude", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipseHeader << qc_("Path Width", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipseHeader << qc_("Central Duration", "column name");
	ui->solareclipseTreeWidget->setHeaderLabels(solareclipseHeader);

	// adjust the column width
	for (int i = 0; i < SolarEclipseCount; ++i)
	{
		ui->solareclipseTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListSolarEclipse()
{
	ui->solareclipseTreeWidget->clear();
	ui->solareclipseTreeWidget->setColumnCount(SolarEclipseCount);
	setSolarEclipseHeaderNames();
	ui->solareclipseTreeWidget->header()->setSectionsMovable(false);
	ui->solareclipseTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
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
	double lat = static_cast<double>(core->getCurrentLocation().latitude);
	double lon = static_cast<double>(core->getCurrentLocation().longitude);
	double elevation = static_cast<double>(core->getCurrentLocation().altitude);

	static SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	Vec4d geocentricCoords = ssystem->getEarth()->getRectangularCoordinates(lon,lat,elevation);
	static const double earthRadius = ssystem->getEarth()->getEquatorialRadius();
	const double rc = geocentricCoords[0]/earthRadius; // rhoCosPhiPrime
	const double rs = geocentricCoords[1]/earthRadius; // rhoSinPhiPrime

	core->setUseTopocentricCoordinates(false);
	core->setJD(JD);
	core->update(0);

	double x,y,d,tf1,tf2,L1,L2,mu;
	SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);

	core->setJD(JD - 5./1440.);
	core->update(0);
	double x1,y1,d1,bestf1,bestf2,besL1,besL2,mu1;
	SolarEclipseBessel(x1,y1,d1,bestf1,bestf2,besL1,besL2,mu1);

	core->setJD(JD + 5./1440.);
	core->update(0);
	double x2,y2,d2,mu2;
	SolarEclipseBessel(x2,y2,d2,bestf1,bestf2,besL1,besL2,mu2);

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
	L1 = L1 - zeta * tf1;
	L2 = L2 - zeta * tf2;
	double L = L1;
	if (central) L = L2;
	const double sfi = delta/L;
	const double ce = 1.- sfi*sfi;
	double cfi = 0.; 
	if (ce > 0.)
		cfi = contact * sqrt(ce);
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

void AstroCalcDialog::generateSolarEclipses()
{
	const bool onEarth = core->getCurrentPlanet()==solarSystem->getEarth();
	if (onEarth)
	{
		initListSolarEclipse();

		const double currentJD = core->getJD();   // save current JD
		double startyear = ui->eclipseFromYearSpinBox->value();
		double years = ui->eclipseYearsSpinBox->value();
		double startJD, stopJD;
		StelUtils::getJDFromDate(&startJD, startyear, 1, 1, 0, 0, 1);
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
				double dt = 1.;
				int iteration = 0;
				while (abs(dt)>(0.1/86400.) && (iteration < 20)) // 0.1 second of accuracy
				{
					core->setJD(JD);
					core->update(0);
					double x,y,d,tf1,tf2,L1,L2,mu;
					SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);

					core->setJD(JD - 5./1440.);
					core->update(0);
					double x1,y1;
					SolarEclipseBessel(x1,y1,d,tf1,tf2,L1,L2,mu);

					core->setJD(JD + 5./1440.);
					core->update(0);
					double x2,y2;
					SolarEclipseBessel(x2,y2,d,tf1,tf2,L1,L2,mu);

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
					double q = (JD-2423436.40347)/29.530588;
					q = round(q);
					int ln = int(q) + 1 - 953;
					int nd = ln + 105;
					int s = 136 + 38 * nd;
					int nx = -61 * nd;
					int nc = floor(nx / 358. + 0.5 - nd / (12. * 358 * 358));
					int saros = 1 + ((s + nc * 223 - 1) % 223);
					if ((s + nc * 223 - 1) < 0) saros -= 223;
					if (saros < -223) saros += 223;

					sarosStr = QString("%1").arg(QString::number(saros));
					gammaStr = QString("%1").arg(QString::number(gamma, 'f', 3));
					double eclipseLatitude = 0.;
					double eclipseLongitude = 0.;
					double eclipseAltitude = 0.;
					double pathwidth = 0.;

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
						int durationSecond = round((centralDuration - durationMinute) * 60.);
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
					treeItem->setData(SolarEclipsePathwidth, Qt::UserRole, pathwidth);
					treeItem->setToolTip(SolarEclipsePathwidth, q_("Width of the path of totality or annularity at greatest eclipse"));
					treeItem->setText(SolarEclipseDuration, durationStr);
					treeItem->setToolTip(SolarEclipseDuration, q_("Duration of total or annular phase at greatest eclipse"));
					treeItem->setTextAlignment(SolarEclipseDate, Qt::AlignRight);
					treeItem->setTextAlignment(SolarEclipseSaros, Qt::AlignRight);
					treeItem->setTextAlignment(SolarEclipseGamma, Qt::AlignRight);
					treeItem->setTextAlignment(SolarEclipseMag, Qt::AlignRight);
					treeItem->setTextAlignment(SolarEclipseLatitude, Qt::AlignRight);
					treeItem->setTextAlignment(SolarEclipseLongitude, Qt::AlignRight);
					treeItem->setTextAlignment(SolarEclipseAltitude, Qt::AlignRight);
					treeItem->setTextAlignment(SolarEclipsePathwidth, Qt::AlignRight);
					treeItem->setTextAlignment(SolarEclipseDuration, Qt::AlignRight);
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
	}
	else
		cleanupSolarEclipses();
}

void AstroCalcDialog::setSolarEclipseLocalHeaderNames()
{
	solareclipselocalHeader.clear();
	solareclipselocalHeader << q_("Date");
	solareclipselocalHeader << q_("Type");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipselocalHeader << qc_("Partial Eclipse Begins", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipselocalHeader << qc_("Central Eclipse Begins", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipselocalHeader << qc_("Maximum Eclipse", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipselocalHeader << qc_("Eclipse Magnitude", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipselocalHeader << qc_("Central Eclipse Ends", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipselocalHeader << qc_("Partial Eclipse Ends", "column name");
	// TRANSLATORS: The name of column in AstroCalc/Eclipses tool
	solareclipselocalHeader << qc_("Duration", "column name");
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
		double startyear = ui->eclipseFromYearSpinBox->value();
		double years = ui->eclipseYearsSpinBox->value();
		double startJD, stopJD;
		StelUtils::getJDFromDate(&startJD, startyear, 1, 1, 0, 0, 1);
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
				double dt = 1.;
				int iteration = 0;
				while (abs(dt)>(0.1/86400.) && (iteration < 20)) // 0.1 second of accuracy
				{
					core->setJD(JD);
					core->update(0);
					double x,y,d,tf1,tf2,L1,L2,mu;
					SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);

					core->setJD(JD - 5./1440.);
					core->update(0);
					double x1,y1;
					SolarEclipseBessel(x1,y1,d,tf1,tf2,L1,L2,mu);

					core->setJD(JD + 5./1440.);
					core->update(0);
					double x2,y2;
					SolarEclipseBessel(x2,y2,d,tf1,tf2,L1,L2,mu);

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
				SolarEclipseBessel(x,y,d,tf1,tf2,L1,L2,mu);
				double gamma = sqrt(x * x + y * y);
				if (y<0.) gamma = -(gamma);

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
						dt = 1.;
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
						dt = 1.;
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
							dt = 1.;
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
							dt = 1.;
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
									JD = JD3;
									JD3 = JD2;
									JD2 = JD;
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
								int durationSecond = round((duration - durationMinute) * 60.);
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
							treeItem->setTextAlignment(SolarEclipseLocalDate, Qt::AlignRight);
							treeItem->setTextAlignment(SolarEclipseLocalMagnitude, Qt::AlignRight);
							treeItem->setTextAlignment(SolarEclipseLocalFirstContact, Qt::AlignRight);
							treeItem->setTextAlignment(SolarEclipseLocal2ndContact, Qt::AlignRight);
							treeItem->setTextAlignment(SolarEclipseLocalMaximum, Qt::AlignRight);
							treeItem->setTextAlignment(SolarEclipseLocal3rdContact, Qt::AlignRight);
							treeItem->setTextAlignment(SolarEclipseLocalLastContact, Qt::AlignRight);
							treeItem->setTextAlignment(SolarEclipseLocalDuration, Qt::AlignRight);
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
	}
	else
		cleanupSolarEclipsesLocal();
}

void AstroCalcDialog::cleanupSolarEclipses()
{
	ui->solareclipseTreeWidget->clear();
}

void AstroCalcDialog::selectCurrentSolarEclipse(const QModelIndex& modelIndex)
{
	// Find the Sun
	QString name = "Sun";
	double JD = modelIndex.sibling(modelIndex.row(), SolarEclipseDate).data(Qt::UserRole).toDouble();
	double lat = modelIndex.sibling(modelIndex.row(), SolarEclipseLatitude).data(Qt::UserRole).toDouble();
	double lon = modelIndex.sibling(modelIndex.row(), SolarEclipseLongitude).data(Qt::UserRole).toDouble();

	StelLocation loc;
	loc.latitude = lat;
	loc.longitude = lon;
	loc.altitude = 10; // 10 meters above sea level
	loc.name = q_("Greatest eclipse’s point");
	loc.planetName = "Earth";
	loc.ianaTimeZone = "LMST";
	core->moveObserverTo(loc, 0.);

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

void AstroCalcDialog::saveSolarEclipses()
{
	QString filter = q_("Microsoft Excel Open XML Spreadsheet");
	filter.append(" (*.xlsx);;");
	filter.append(q_("CSV (Comma delimited)"));
	filter.append(" (*.csv)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save calculated solar eclipses as..."),
							QDir::homePath() + "/solareclipses.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->solareclipseTreeWidget, ephemerisHeader);
	else
	{
		int count = ui->solareclipseTreeWidget->topLevelItemCount();
		int columns = solareclipseHeader.size();
		int *width = new int[static_cast<unsigned int>(columns)];
		QString sData;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("Solar Eclipses"));
		xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
		xlsx.addSheet("Solar Eclipses", AbstractSheet::ST_WorkSheet);

		QXlsx::Format header;
		header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		header.setPatternBackgroundColor(Qt::yellow);
		header.setBorderStyle(QXlsx::Format::BorderThin);
		header.setBorderColor(Qt::black);
		header.setFontBold(true);
		for (int i = 0; i < columns; i++)
		{
			// Row 1: Names of columns
			sData = solareclipseHeader.at(i).trimmed();
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
				sData = ui->solareclipseTreeWidget->topLevelItem(i)->text(j).trimmed();
				xlsx.write(i + 2, j + 1, sData, data);
				int w = sData.size();
				if (w > width[j])
				{
					width[j] = w;
				}
			}
		}

		xlsx.write(count+3, 1, q_("Note: Path of eclipses during thousands of years in the past and future are not reliable due to uncertainty in ΔT which is caused by fluctuations in Earth's rotation."));

		for (int i = 0; i < columns; i++)
		{
			xlsx.setColumnWidth(i+1, width[i]+2);
		}

		delete[] width;
		xlsx.saveAs(filePath);
	}
}

void AstroCalcDialog::cleanupSolarEclipsesLocal()
{
	ui->solareclipselocalTreeWidget->clear();
}

void AstroCalcDialog::selectCurrentSolarEclipseLocal(const QModelIndex& modelIndex)
{
	// Find the Sun
	QString name = "Sun";
	double JD = modelIndex.sibling(modelIndex.row(), SolarEclipseLocalDate).data(Qt::UserRole).toDouble();

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

void AstroCalcDialog::saveSolarEclipsesLocal()
{
	QString filter = q_("Microsoft Excel Open XML Spreadsheet");
	filter.append(" (*.xlsx);;");
	filter.append(q_("CSV (Comma delimited)"));
	filter.append(" (*.csv)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save calculated solar eclipses as..."),
							QDir::homePath() + "/solareclipses-local.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->solareclipselocalTreeWidget, ephemerisHeader);
	else
	{
		int count = ui->solareclipselocalTreeWidget->topLevelItemCount();
		int columns = solareclipselocalHeader.size();
		int *width = new int[static_cast<unsigned int>(columns)];
		QString sData;

		QXlsx::Document xlsx;
		xlsx.setDocumentProperty("title", q_("Solar Eclipses"));
		xlsx.setDocumentProperty("creator", StelUtils::getApplicationName());
		xlsx.addSheet("Solar Eclipses", AbstractSheet::ST_WorkSheet);

		QXlsx::Format header;
		header.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
		header.setPatternBackgroundColor(Qt::yellow);
		header.setBorderStyle(QXlsx::Format::BorderThin);
		header.setBorderColor(Qt::black);
		header.setFontBold(true);
		for (int i = 0; i < columns; i++)
		{
			// Row 1: Names of columns
			sData = solareclipselocalHeader.at(i).trimmed();
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
				sData = ui->solareclipselocalTreeWidget->topLevelItem(i)->text(j).trimmed();
				xlsx.write(i + 2, j + 1, sData, data);
				int w = sData.size();
				if (w > width[j])
				{
					width[j] = w;
				}
			}
		}

		xlsx.write(count+3, 1, q_("Note: Local circumstances for eclipses during thousands of years in the past and future are not reliable due to uncertainty in ΔT which is caused by fluctuations in Earth's rotation."));

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
	// special case: selected dwarf and minor planets
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
	const QMap<QString, int>itemsMap={
		{q_("Latest selected object"), PHCLatestSelectedObject}, {q_("Solar system"), PHCSolarSystem}, {q_("Planets"), PHCPlanets}, {q_("Asteroids"), PHCAsteroids},
		{q_("Plutinos"), PHCPlutinos}, {q_("Comets"), PHCComets},	{q_("Dwarf planets"), PHCDwarfPlanets},{q_("Cubewanos"), PHCCubewanos},
		{q_("Scattered disc objects"), PHCScatteredDiscObjects},{q_("Oort cloud objects"), PHCOortCloudObjects},{q_("Sednoids"), PHCSednoids},
		{q_("Bright stars (<%1 mag)").arg(QString::number(brightLimit - 5.0, 'f', 1)), PHCBrightStars},{q_("Bright double stars (<%1 mag)").arg(QString::number(brightLimit - 5.0, 'f', 1)), PHCBrightDoubleStars},
		{q_("Bright variable stars (<%1 mag)").arg(QString::number(brightLimit - 5.0, 'f', 1)), PHCBrightVariableStars},{q_("Bright star clusters (<%1 mag)").arg(brLimit), PHCBrightStarClusters},
		{q_("Planetary nebulae (<%1 mag)").arg(brLimit), PHCPlanetaryNebulae},{q_("Bright nebulae (<%1 mag)").arg(brLimit), PHCBrightNebulae},{q_("Dark nebulae"), PHCDarkNebulae},
		{q_("Bright galaxies (<%1 mag)").arg(brLimit), PHCBrightGalaxies},{q_("Symbiotic stars"), PHCSymbioticStars},{q_("Emission-line stars"), PHCEmissionLineStars},
		{q_("Interstellar objects"), PHCInterstellarObjects},{q_("Planets and Sun"), PHCPlanetsSun},{q_("Sun, planets and moons of observer location"), PHCSunPlanetsMoons},
		{q_("Bright Solar system objects (<%1 mag)").arg(QString::number(brightLimit + 2.0, 'f', 1)), PHCBrightSolarSystemObjects},
		{q_("Solar system objects: minor bodies"), PHCSolarSystemMinorBodies},{q_("Moons of first body"), PHCMoonsFirstBody},
		{q_("Bright carbon stars"), PHCBrightCarbonStars},{q_("Bright barium stars"), PHCBrightBariumStars}
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
		ui->altVsTimeTitle->setText(selectedObject->getNameI18n());
		const bool onEarth = core->getCurrentPlanet()==solarSystem->getEarth();

		const double currentJD = core->getJD();
		const double shift = core->getUTCOffset(currentJD) / 24.0;
		const double noon = static_cast<int>(currentJD + shift);
		double az, alt, deg, ltime, JD;
		bool sign;

		double xMaxY = -100.;
		int step = 180;
		int limit = 485;		
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

		for (int i = -5; i <= limit; i++) // 24 hours + 15 minutes in both directions
		{
			// A new point on the graph every 3 minutes with shift to right 12 hours
			// to get midnight at the center of diagram (i.e. accuracy is 3 minutes)
			ltime = i * step + 43200;
			aX.append(ltime);
			JD = noon + ltime / 86400 - shift - 0.5;
			core->setJD(JD);
			
#ifdef USE_STATIC_PLUGIN_SATELLITES
			if (isSatellite)
			{
				// update data for that single satellite only
				sat->update(0.0);
			}
			else
#endif
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
			{
				name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
				if (name.isEmpty())
					name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignationWIC();
			}

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

	const double currentJD = core->getJD();
	const double UTCOffset = core->getUTCOffset(currentJD);
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
	// added special case - the tool is not applicable on non-Earth locations
	if (!ssObj.isNull() && core->getCurrentPlanet()==solarSystem->getEarth())
	{
		// X axis - time; Y axis - altitude
		QList<double> aX, aY, bY;

		const double currentJD = core->getJD();
		int year, month, day;
		double startJD, JD, ltime, /*UTCshift,*/ width = 1.0;
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
				//UTCshift = core->getUTCOffset(JD) / 24.; // Fix DST shift...
				Vec4d rts = ssObj->getRTSTime(core);
				//JD += (rts[1]/24. - UTCshift); // FIXME: New logic has JD, not hours, here.
				JD = rts[1]; // Maybe that's all?
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
	else
	{
		prepareXVsTimeAxesAndGraph();
		ui->graphsPlot->clearGraphs();
		ui->graphsPlot->replot();
	}
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

void AstroCalcDialog::mouseOverGraphs(QMouseEvent* event)
{
	double x = ui->graphsPlot->xAxis->pixelToCoord(event->pos().x());
	double y = ui->graphsPlot->yAxis->pixelToCoord(event->pos().y());
	double y2 = ui->graphsPlot->yAxis2->pixelToCoord(event->pos().y());

	QCPAbstractPlottable* abstractGraph = ui->graphsPlot->plottableAt(event->pos(), false);
	QCPGraph* graph = qobject_cast<QCPGraph*>(abstractGraph);

	int year, month, day;
	double startJD, ltime;
	StelUtils::getDateFromJulianDay(core->getJD(), &year, &month, &day);
	StelUtils::getJDFromDate(&startJD, year, 1, 1, 0, 0, 0);

	if (ui->graphsPlot->xAxis->range().contains(x) && ui->graphsPlot->yAxis->range().contains(y))
	{
		QString info = "";
		if (graph)
		{
			ltime = (x / StelCore::ONE_OVER_JD_SECOND) + startJD;

			if (graph->name() == "[0]")
				info = QString("%1<br />%2: %3").arg(StelUtils::julianDayToISO8601String(ltime).replace("T", " "), ui->graphsPlot->yAxis->label() , QString::number(y, 'f', 2));

			if (graph->name() == "[1]")
				info = QString("%1<br />%2: %3").arg(StelUtils::julianDayToISO8601String(ltime).replace("T", " "), ui->graphsPlot->yAxis2->label() , QString::number(y2, 'f', 2));
		}
		ui->graphsPlot->setToolTip(info);
	}

	ui->graphsPlot->update();
	ui->graphsPlot->replot();
}

double AstroCalcDialog::computeGraphValue(const PlanetP &ssObj, const int graphType)
{
	double value = 0.;
	switch (graphType)
	{
		case GraphMagnitudeVsTime:
			value = static_cast<double>(ssObj->getVMagnitude(core));
			break;
		case GraphPhaseVsTime:
			value = static_cast<double>(ssObj->getPhase(core->getObserverHeliocentricEclipticPos())) * 100.;
			break;
		case GraphDistanceVsTime:
			value =  ssObj->getJ2000EquatorialPos(core).length();
			break;
		case GraphElongationVsTime:
			value = ssObj->getElongation(core->getObserverHeliocentricEclipticPos()) * M_180_PI;
			break;
		case GraphAngularSizeVsTime:
		{
			value = ssObj->getAngularRadius(core) * (360. / M_PI);
			if (value < 1.)
				value *= 60.;
			break;
		}
		case GraphPhaseAngleVsTime:
			value = ssObj->getPhaseAngle(core->getObserverHeliocentricEclipticPos()) * M_180_PI;
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
	const QList<graph> functions = {
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
	QString distMU = qc_("AU", "distance, astronomical unit");
	QString asMU = QString("'");

	PlanetP ssObj = solarSystem->searchByEnglishName(ui->graphsCelestialBodyComboBox->currentData().toString());
	if (!ssObj.isNull())
	{
		if (ssObj->getJ2000EquatorialPos(core).length() < 0.1)
		{
			// TRANSLATORS: Mega-meter (SI symbol: Mm; Mega-meter is a unit of length in the metric system,
			// equal to one million meters)
			distMU = q_("Mm");
		}
		if ((ssObj->getAngularRadius(core) * 360. / M_PI) < 1.) asMU = QString("\"");
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
			// TRANSLATORS: The phrase "Heliocentric distance" may be long in some languages and you can abbreviate it.
			yAxis1Legend = QString("%1, %2").arg(qc_("Heliocentric distance","axis name"), distMU);
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 50.0;
			break;
		case GraphTransitAltitudeVsTime:
			// TRANSLATORS: The phrase "Transit altitude" may be long in some languages and you can abbreviate it.
			yAxis1Legend = QString("%1, %2").arg(qc_("Transit altitude","axis name"), QChar(0x00B0));
			if (minY1 < -1000.) minY1 = 0.0;
			if (maxY1 > 1000.) maxY1 = 90.0;
			break;
		case GraphRightAscensionVsTime:
			// TRANSLATORS: The phrase "Right ascension" may be long in some languages and you can abbreviate it.
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
	ui->graphsPlot->yAxis->setLabel(yAxis1Legend);
	ui->graphsPlot->yAxis2->setLabel(yAxis2Legend);

	int dYear = (static_cast<int>(solarSystem->getEarth()->getSiderealPeriod()*graphsDuration) + 1) * 86400;
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

void AstroCalcDialog::prepareMonthlyElevationAxesAndGraph()
{
	QString xAxisStr = q_("Date");
	QString yAxisStr = QString("%1, %2").arg(q_("Altitude"), QChar(0x00B0));

	QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);

	ui->monthlyElevationGraph->setLocale(QLocale(localeMgr->getAppLanguage()));
	ui->monthlyElevationGraph->xAxis->setLabel(xAxisStr);
	ui->monthlyElevationGraph->yAxis->setLabel(yAxisStr);

	int dYear = (static_cast<int>(solarSystem->getEarth()->getSiderealPeriod()) + 1) * 86400;
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
	ui->monthlyElevationGraph->setToolTip("");

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
		ui->monthlyElevationTitle->setText(selectedObject->getNameI18n());
		if (selectedObject->getType() == "Satellite")
		{
			ui->monthlyElevationGraph->graph(0)->data()->clear();
			ui->monthlyElevationGraph->replot();
			return;
		}

		const double currentJD = core->getJD();
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

		prepareMonthlyElevationAxesAndGraph();

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
			if (otype == "Star" || otype=="Pulsar")
				selectedObject->getID().isEmpty() ? name = q_("Unnamed star") : name = selectedObject->getID();
		}
		ui->monthlyElevationGraph->graph(0)->setData(x, y);
		ui->monthlyElevationGraph->graph(0)->setName(name);
		ui->monthlyElevationGraph->replot();
		ui->monthlyElevationGraph->setToolTip(name);
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

	if (ui->altVsTimePlot->xAxis->range().contains(x) && ui->altVsTimePlot->yAxis->range().contains(y) )
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

	if (ui->aziVsTimePlot->xAxis->range().contains(x) && ui->aziVsTimePlot->yAxis->range().contains(y))
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
	const double x = ui->altVsTimePlot->xAxis->pixelToCoord(event->pos().x());
	const double y = ui->altVsTimePlot->yAxis->pixelToCoord(event->pos().y());

	QCPAbstractPlottable* abstractGraph = ui->altVsTimePlot->plottableAt(event->pos(), false);
	QCPGraph* graph = qobject_cast<QCPGraph*>(abstractGraph);

	if (ui->altVsTimePlot->xAxis->range().contains(x) && ui->altVsTimePlot->yAxis->range().contains(y))
	{
		QString info = "";
		if (graph)
		{
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
		}
		ui->altVsTimePlot->setToolTip(info);
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
	ui->phenomenaTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
}

void AstroCalcDialog::selectCurrentPhenomen(const QModelIndex& modelIndex)
{
	// Find the object
	QString name = ui->object1ComboBox->currentData().toString();
	if (modelIndex.sibling(modelIndex.row(), PhenomenaType).data().toString().contains(q_("Opposition"), Qt::CaseInsensitive))
		name = modelIndex.sibling(modelIndex.row(), PhenomenaObject2).data().toString();
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
	const QString currentPlanet = ui->object1ComboBox->currentData().toString();
	const double separation = ui->allowedSeparationSpinBox->valueDegrees();
	const bool opposition = ui->phenomenaOppositionCheckBox->isChecked();
	const bool perihelion = ui->phenomenaPerihelionAphelionCheckBox->isChecked();

	initListPhenomena();

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	double startJD = StelUtils::qDateTimeToJd(ui->phenomenFromDateEdit->date().startOfDay(Qt::UTC));
	double stopJD = StelUtils::qDateTimeToJd(ui->phenomenFromDateEdit->date().addDays(1).startOfDay(Qt::UTC));
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
				{PHCAsteroids, Planet::isAsteroid},
				{PHCPlutinos, Planet::isPlutino},
				{PHCComets, Planet::isComet},
				{PHCDwarfPlanets, Planet::isDwarfPlanet},
				{PHCCubewanos, Planet::isCubewano},
				{PHCScatteredDiscObjects, Planet::isSDO},
				{PHCOortCloudObjects, Planet::isOCO},
				{PHCSednoids, Planet::isSednoid},
				{PHCInterstellarObjects, Planet::isInterstellar}};
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
		case PHCSunPlanetsMoons: // Sun, planets and moons of observer location
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
			PlanetP firstPplanet = solarSystem->searchByEnglishName(currentPlanet);
			for (const auto& object : allObjects)
			{
				if (object->getParent()==firstPplanet && object->getPlanetType() == Planet::isMoon)
					objects.append(object);
			}
			break;
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
		else if ((obj2Type >= PHCSolarSystem && obj2Type < PHCBrightStars) || (obj2Type >= PHCInterstellarObjects && obj2Type <= PHCMoonsFirstBody))
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
				if (obj2Type==PHCMoonsFirstBody || obj2Type==PHCSolarSystem || obj2Type==PHCBrightSolarSystemObjects)
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
		const double d1 = object1->getJ2000EquatorialPos(core).length();
		const double d2 = object2->getJ2000EquatorialPos(core).length();
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
		else if (object1 == sun || object2 == sun) // this is may be superior of inferior conjunction for inner planet
		{
			double dcp = planet->getHeliocentricEclipticPos().length();
			double dp  = (object1 == sun) ? object2->getHeliocentricEclipticPos().length() :
							object1->getHeliocentricEclipticPos().length();
			if (dp < dcp) // OK, it's inner planet
			{
				if (object1 == sun)
					phenomenType = d1<d2 ? q_("Superior conjunction") : q_("Inferior conjunction");
				else
					phenomenType = d2<d1 ? q_("Superior conjunction") : q_("Inferior conjunction");
			}
		}

		QString elongStr = "";
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
		else if (object1 == sun || object2 == sun) // this is may be superior of inferior conjunction for inner planet
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
					phenomenType = d1<d2 ? q_("Superior conjunction") : q_("Inferior conjunction");
				else
					phenomenType = d2<d1 ? q_("Superior conjunction") : q_("Inferior conjunction");
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
	QRegularExpression mp("^[(](\\d+)[)]\\s(.+)$");

	if (objects.contains("Moon", Qt::CaseInsensitive))
		limit = 0.25;
	else if (objects.contains("C/",Qt::CaseInsensitive) || objects.contains("P/",Qt::CaseInsensitive))
		limit = 0.5;
	else if (objects.contains("Earth",Qt::CaseInsensitive) || objects.contains("Sun", Qt::CaseInsensitive))
		limit = 1.;
	else if (objects.contains("Venus",Qt::CaseInsensitive) || objects.contains("Mercury", Qt::CaseInsensitive))
		limit = 2.5;
	else if (objects.contains("Mars",Qt::CaseInsensitive))
		limit = 5.;
	else if (objects.indexOf(mp)>=0)
		limit = 10.;	
	else if (objects.contains("Jupiter", Qt::CaseInsensitive) || objects.contains("Saturn", Qt::CaseInsensitive))
		limit = 15.;
	else if (objects.contains("Neptune", Qt::CaseInsensitive) || objects.contains("Uranus", Qt::CaseInsensitive) || objects.contains("Pluto",Qt::CaseInsensitive))
		limit = 20.;

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
	if (out == Q_NULLPTR)
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
	if (out == Q_NULLPTR)
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

	if (out == Q_NULLPTR)
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
	const double distanceCorrection=(body-obs).length() * (AU / (SPEED_OF_LIGHT * 86400.));
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

	if (out == Q_NULLPTR)
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
	plotAngularDistanceGraph = false;
	plotDistanceGraph = false;

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

	// special case - RTS
	if (ui->stackListWidget->row(current) == 2)
		setRTSCelestialBodyName();

	// special case - graphs
	if (ui->stackListWidget->row(current) == 4)
	{
		int idx = ui->tabWidgetGraphs->currentIndex();
		if (idx==0) // 'Alt. vs Time' is visible
		{
			plotAltVsTime = true;
			drawAltVsTimeDiagram(); // Is object already selected?
		}

		if (idx==1) //  'Azi. vs Time' is visible
		{
			plotAziVsTime = true;
			drawAziVsTimeDiagram(); // Is object already selected?
		}

		if (idx==2) // 'Monthly Elevation' is visible
		{
			plotMonthlyElevation = true;
			drawMonthlyElevationGraph(); // Is object already selected?
		}		

		if (idx==3) // 'Graphs' is visible
			updateXVsTimeGraphs();

		if(idx==4) // 'Angular distance' is visible
		{
			plotAngularDistanceGraph = true;
			drawAngularDistanceGraph();
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
	if (index==3) // Graphs
		updateXVsTimeGraphs();
	if (index==4) // Angular Distance
	{
		plotAngularDistanceGraph = true;
		drawAngularDistanceGraph(); // Is object already selected?
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
		{2,	q_("Table of lunar eclipses")}
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
	font.setWeight(75);
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

	wutCategories.clear();
	wutCategories.insert(q_("Planets"),				EWPlanets);
	wutCategories.insert(q_("Bright stars"),			EWBrightStars);
	wutCategories.insert(q_("Bright nebulae"),			EWBrightNebulae);
	wutCategories.insert(q_("Dark nebulae"),			EWDarkNebulae);
	wutCategories.insert(q_("Galaxies"),				EWGalaxies);
	wutCategories.insert(q_("Open star clusters"),		EWOpenStarClusters);
	wutCategories.insert(q_("Asteroids"),				EWAsteroids);
	wutCategories.insert(q_("Comets"),				EWComets);
	wutCategories.insert(q_("Plutinos"),				EWPlutinos);
	wutCategories.insert(q_("Dwarf planets"),			EWDwarfPlanets);
	wutCategories.insert(q_("Cubewanos"),			EWCubewanos);
	wutCategories.insert(q_("Scattered disc objects"), 	EWScatteredDiscObjects);
	wutCategories.insert(q_("Oort cloud objects"),		EWOortCloudObjects);
	wutCategories.insert(q_("Sednoids"),				EWSednoids);
	wutCategories.insert(q_("Planetary nebulae"),		EWPlanetaryNebulae);
	wutCategories.insert(q_("Bright double stars"),		EWBrightDoubleStars);
	wutCategories.insert(q_("Bright variable stars"),	EWBrightVariableStars);
	wutCategories.insert(q_("Bright stars with high proper motion"),	EWBrightStarsWithHighProperMotion);
	wutCategories.insert(q_("Symbiotic stars"),			EWSymbioticStars);
	wutCategories.insert(q_("Emission-line stars"),		EWEmissionLineStars);
	wutCategories.insert(q_("Supernova candidates"),	EWSupernovaeCandidates);
	wutCategories.insert(q_("Supernova remnant candidates"), EWSupernovaeRemnantCandidates);
	wutCategories.insert(q_("Supernova remnants"),	EWSupernovaeRemnants);
	wutCategories.insert(q_("Clusters of galaxies"), 		EWClustersOfGalaxies);
	wutCategories.insert(q_("Interstellar objects"),		EWInterstellarObjects);
	wutCategories.insert(q_("Globular star clusters"),	EWGlobularStarClusters);
	wutCategories.insert(q_("Regions of the sky"), 		EWRegionsOfTheSky);
	wutCategories.insert(q_("Active galaxies"), 			EWActiveGalaxies);
	if (moduleMgr.isPluginLoaded("Pulsars"))
	{
		// Add the category when pulsars is visible
		if (propMgr->getProperty("Pulsars.pulsarsVisible")->getValue().toBool())
			wutCategories.insert(q_("Pulsars"), EWPulsars);
	}
	if (moduleMgr.isPluginLoaded("Exoplanets"))
	{
		// Add the category when exoplanets is visible
		if (propMgr->getProperty("Exoplanets.showExoplanets")->getValue().toBool())
			wutCategories.insert(q_("Exoplanetary systems"), EWExoplanetarySystems);
	}
	if (moduleMgr.isPluginLoaded("Novae"))
		wutCategories.insert(q_("Bright nova stars"), EWBrightNovaStars);
	if (moduleMgr.isPluginLoaded("Supernovae"))
		wutCategories.insert(q_("Bright supernova stars"), 	EWBrightSupernovaStars);
	wutCategories.insert(q_("Interacting galaxies"), 	EWInteractingGalaxies);
	wutCategories.insert(q_("Deep-sky objects"), 		EWDeepSkyObjects);
	wutCategories.insert(q_("Messier objects"), 		EWMessierObjects);
	wutCategories.insert(q_("NGC/IC objects"), 		EWNGCICObjects);
	wutCategories.insert(q_("Caldwell objects"), 		EWCaldwellObjects);
	wutCategories.insert(q_("Herschel 400 objects"), 	EWHerschel400Objects);
	wutCategories.insert(q_("Algol-type eclipsing systems"),	EWAlgolTypeVariableStars);
	wutCategories.insert(q_("The classical cepheids"),	EWClassicalCepheidsTypeVariableStars);
	wutCategories.insert(q_("Bright carbon stars"),		EWCarbonStars);
	wutCategories.insert(q_("Bright barium stars"),		EWBariumStars);

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

void AstroCalcDialog::fillWUTTable(QString objectName, QString designation, float magnitude, Vec4d RTSTime, double maxElevation, double angularSize, QString constellation, bool decimalDegrees)
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
	treeItem->setText(WUTConstellation, constellation);
	treeItem->setTextAlignment(WUTConstellation, Qt::AlignCenter);
	treeItem->setToolTip(WUTConstellation, q_("IAU Constellation"));
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

		const Nebula::TypeGroup& tflags = dsoMgr->getTypeFilters();
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
		QString designation, starName, constellation;

		ui->wutAngularSizeLimitCheckBox->setText(q_("Limit angular size:"));
		ui->wutAngularSizeLimitCheckBox->setToolTip(q_("Set limits for angular size for visible celestial objects"));
		ui->wutAngularSizeLimitMinSpinBox->setToolTip(q_("Minimal angular size for visible celestial objects"));
		ui->wutAngularSizeLimitMaxSpinBox->setToolTip(q_("Maximum angular size for visible celestial objects"));

		// Direct calculate sunrise/sunset (civil twilight)
		Vec4d rts = GETSTELMODULE(SolarSystem)->getSun()->getRTSTime(core, -7.);
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

								fillWUTTable(starName, designation, mag, rts, alt, 0.0, constellation, withDecimalDegree);
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

								if (d.isEmpty())
									fillWUTTable(n, n, mag, rts, alt, object->getAngularRadius(core), constellation, withDecimalDegree);
								else if (n.isEmpty())
									fillWUTTable(d, d, mag, rts, alt, object->getAngularRadius(core), constellation, withDecimalDegree);
								else
									fillWUTTable(QString("%1 (%2)").arg(d, n), d, mag, rts, alt, object->getAngularRadius(core), constellation, withDecimalDegree);

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

								fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 2.0*object->getAngularRadius(core), constellation, withDecimalDegree);
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

								fillWUTTable(starName, designation, mag, rts, alt, dblStar.value(object)/3600.0, constellation, withDecimalDegree);
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

								fillWUTTable(starName, designation, mag, rts, alt, 0.0, constellation, withDecimalDegree);
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

								fillWUTTable(starName, designation, mag, rts, alt, 0.0, constellation, withDecimalDegree);
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

								if (d.isEmpty())
									fillWUTTable(n, n, mag, rts, alt, object->getAngularRadius(core), constellation, withDecimalDegree);
								else if (n.isEmpty())
									fillWUTTable(d, d, mag, rts, alt, object->getAngularRadius(core), constellation, withDecimalDegree);
								else
									fillWUTTable(QString("%1 (%2)").arg(d, n), d, mag, rts, alt, object->getAngularRadius(core), constellation, withDecimalDegree);

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

										fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 0.0, constellation, withDecimalDegree);
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

								fillWUTTable(starName, designation, 99.f, rts, alt, 0.0, constellation, withDecimalDegree);
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

								fillWUTTable(object->getNameI18n().trimmed(), designation, mag, rts, alt, 0.0, constellation, withDecimalDegree);
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

								fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 0.0, constellation, withDecimalDegree);
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

								fillWUTTable(object->getNameI18n(), designation, mag, rts, alt, 0.0, constellation, withDecimalDegree);
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

					for (const auto& object : catDSO)
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

								if (d.isEmpty())
									fillWUTTable(n, n, mag, rts, alt, object->getAngularRadius(core), constellation, withDecimalDegree);
								else if (n.isEmpty())
									fillWUTTable(d, d, mag, rts, alt, object->getAngularRadius(core), constellation, withDecimalDegree);
								else
									fillWUTTable(QString("%1 (%2)").arg(d, n), d, mag, rts, alt, object->getAngularRadius(core), constellation, withDecimalDegree);

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
	// filter.append(q_("JSON (Stellarium bookmarks)"));
	// filter.append(" (*.json)");
	QString defaultFilter("(*.xlsx)");
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save list of objects as..."),
							QDir::homePath() + "/wut-objects.xlsx",
							filter,
							&defaultFilter);

	if (defaultFilter.contains(".csv", Qt::CaseInsensitive))
		saveTableAsCSV(filePath, ui->wutMatchingObjectsTreeWidget, wutHeader);
	else if (defaultFilter.contains(".json", Qt::CaseInsensitive))
		saveTableAsBookmarks(filePath, ui->wutMatchingObjectsTreeWidget);
	else
	{
		int count = ui->wutMatchingObjectsTreeWidget->topLevelItemCount();
		int columns = wutHeader.size();

		int *width;
		width = new int[static_cast<unsigned int>(columns)];
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
				xlsx.write(i + 2, j + 1, sData, j==0 ? left : data);
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
	QString degree = QChar(0x00B0);
	// TRANSLATORS: Unit of measure for distance - kilometers
	QString km = qc_("km", "distance");
	// TRANSLATORS: Unit of measure for distance - millions kilometers
	QString Mkm = qc_("M km", "distance");
	const bool useKM = (distanceAu < 0.1);
	QString distAU = QString::number(distanceAu, 'f', 5);
	QString distKM = useKM ? QString::number(distanceKm, 'f', 3) : QString::number(distanceKm / 1.0e6, 'f', 3);

	//const double r = std::acos(sin(posFCB.latitude()) * sin(posSCB.latitude()) + cos(posFCB.latitude()) * cos(posSCB.latitude()) * cos(posFCB.longitude() - posSCB.longitude()));
	const double r= posFCB.angle(posSCB);

	unsigned int d, m;
	double s, dd;
	bool sign;

	StelUtils::radToDms(r, sign, d, m, s);
	StelUtils::radToDecDeg(r, sign, dd);

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

	double orbVelFCB = firstCBId->getEclipticVelocity().length();
	QString orbitalVelocityFCB = orbVelFCB<=0. ? dash : QString("%1 %2").arg(QString::number(orbVelFCB * AU/86400., 'f', 3), kms);
	ui->labelOrbitalVelocityFCBValue->setText(orbitalVelocityFCB);

	double orbVelSCB = secondCBId->getEclipticVelocity().length();
	QString orbitalVelocitySCB = orbVelSCB<=0. ? dash : QString("%1 %2").arg(QString::number(orbVelSCB * AU/86400., 'f', 3), kms);
	ui->labelOrbitalVelocitySCBValue->setText(orbitalVelocitySCB);

	double fcbs = 2.0 * AU * firstCBId->getEquatorialRadius();
	double scbs = 2.0 * AU * secondCBId->getEquatorialRadius();
	double sratio = fcbs/scbs;
	int ss = (sratio < 1.0 ? 6 : 2);

	QString sizeRatio = QString("%1 (%2 %4 / %3 %4)").arg(QString::number(sratio, 'f', ss), QString::number(fcbs, 'f', 1), QString::number(scbs, 'f', 1), km);
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

	ui->pcDistanceGraphPlot->addGraph(ui->pcDistanceGraphPlot->xAxis, ui->pcDistanceGraphPlot->yAxis2);
	ui->pcDistanceGraphPlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->pcDistanceGraphPlot->graph(1)->setPen(axisPenR);
	ui->pcDistanceGraphPlot->graph(1)->setLineStyle(QCPGraph::lsLine);
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

	PlanetP currentPlanet = core->getCurrentPlanet();
	PlanetP firstCBId = solarSystem->searchByEnglishName(fbody->currentData(Qt::UserRole).toString());
	PlanetP secondCBId = solarSystem->searchByEnglishName(sbody->currentData(Qt::UserRole).toString());

	if (firstCBId==secondCBId)
	{
		ui->pcDistanceGraphPlot->graph(0)->clearData();
		ui->pcDistanceGraphPlot->graph(1)->clearData();
		ui->pcDistanceGraphPlot->replot();
		return;
	}

	int limit = 76, step = 4;
	if (firstCBId->getParent() == currentPlanet || secondCBId->getParent() == currentPlanet)
	{
		limit = 151; step = 2;
	}

	QList<double> aX, aY1, aY2;
	const double currentJD = core->getJD();
	for (int i = (-1*limit); i <= limit; i++)
	{
		double JD = currentJD + i*step;
		core->setJD(JD);
		Vec3d posFCB = firstCBId->getJ2000EquatorialPos(core);
		Vec3d posSCB = secondCBId->getJ2000EquatorialPos(core);
		double distanceAu = (posFCB - posSCB).length();
		//r = std::acos(sin(posFCB.latitude()) * sin(posSCB.latitude()) + cos(posFCB.latitude()) * cos(posSCB.latitude()) * cos(posFCB.longitude() - posSCB.longitude()));
		double r= posFCB.angle(posSCB);
		double dd;
		bool sign;
		StelUtils::radToDecDeg(r, sign, dd);
		aX.append(i*step);
		aY1.append(distanceAu);
		if (firstCBId != currentPlanet && secondCBId != currentPlanet)
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
	const double x = ui->pcDistanceGraphPlot->xAxis->pixelToCoord(event->pos().x());
	const double y = ui->pcDistanceGraphPlot->yAxis->pixelToCoord(event->pos().y());
	const double y2 = ui->pcDistanceGraphPlot->yAxis2->pixelToCoord(event->pos().y());

	QCPAbstractPlottable* abstractGraph = ui->pcDistanceGraphPlot->plottableAt(event->pos(), false);
	QCPGraph* graph = qobject_cast<QCPGraph*>(abstractGraph);

	if (ui->pcDistanceGraphPlot->xAxis->range().contains(x) && ui->pcDistanceGraphPlot->yAxis->range().contains(y))
	{
		QString info;
		if (graph)
		{
			const double currentJD = core->getJD();
			if (graph->name()=="[LD]")
				info = QString("%1<br />%2: %3%4").arg(StelUtils::julianDayToISO8601String(currentJD + x).replace("T", " "), q_("Linear distance"), QString::number(y), qc_("AU", "distance, astronomical unit"));

			if (graph->name()=="[AD]")
				info = QString("%1<br />%2: %3%4").arg(StelUtils::julianDayToISO8601String(currentJD + x).replace("T", " "), q_("Angular distance"), QString::number(y2), QChar(0x00B0));
		}
		ui->pcDistanceGraphPlot->setToolTip(info);
	}

	ui->pcDistanceGraphPlot->update();
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
	QString label = q_("Angular distance between the Moon and selected object");
	ui->angularDistancePlot->setToolTip(label);

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
		const double currentJD = core->getJD();
		double JD, distance, dd;
		bool sign;
		for (int i = -5; i <= 35; i++)
		{
			JD = currentJD + i;
			core->setJD(JD);
			moonPosition = moon->getJ2000EquatorialPos(core);
			selectedObjectPosition = selectedObject->getJ2000EquatorialPos(core);
			distance = moonPosition.angle(selectedObjectPosition);
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
			{
				name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
				if (name.isEmpty())
					name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignationWIC();
			}
			if (otype == "Star" || otype=="Pulsar")
				selectedObject->getID().isEmpty() ? name = q_("Unnamed star") : name = selectedObject->getID();
		}
		ui->angularDistancePlot->setToolTip(QString("%1 (%2)").arg(label, name));
		ui->angularDistanceTitle->setText(QString("%1 (%2)").arg(label, name));

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

	double limit = ui->angularDistanceLimitSpinBox->value();
	QVector<double> x = {-5, 35};
	QVector<double> y = {limit, limit};
	ui->angularDistancePlot->graph(1)->setData(x, y);
	ui->angularDistancePlot->replot();
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
	tableData.setCodec("UTF-8");

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
