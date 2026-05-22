/*
 * Visibility Map plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "VisibilityMapDialog.hpp"
#include "EarthShadowMapWidget.hpp"
#include "SunriseSunsetMapWidget.hpp"
#include "StarVisibilityMapWidget.hpp"
#include "StarCalendarWidget.hpp"
#include "DaylightLengthMapWidget.hpp"

#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelObjectMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QListView>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
enum StepType
{
	StepMinute = 0,
	StepHour,
	StepDay,
	StepCalendarMonth,
	StepCalendarYear
};
}

VisibilityMapDialog::VisibilityMapDialog()
	: StelDialog("VisibilityMap")
	, titleBar(Q_NULLPTR)
	, mapWidget(Q_NULLPTR)
	, isolineMapWidget(Q_NULLPTR)
	, starVisibilityMapWidget(Q_NULLPTR)
	, starCalendarWidget(Q_NULLPTR)
	, starViewStack(Q_NULLPTR)
	, daylightLengthMapWidget(Q_NULLPTR)
	, tabWidget(Q_NULLPTR)
	, stepCombo(Q_NULLPTR)
	, isolineBodyCombo(Q_NULLPTR)
	, isolineModeCombo(Q_NULLPTR)
	, isolineUtcSpin(Q_NULLPTR)
	, isolineSyncButton(Q_NULLPTR)
	, isolineSendButton(Q_NULLPTR)
	, isolinePrevDayButton(Q_NULLPTR)
	, isolineNextDayButton(Q_NULLPTR)
	, isolineLocationButton(Q_NULLPTR)
	, isolineResetButton(Q_NULLPTR)
	, twilightGridCheckBox(Q_NULLPTR)
	, twilightAutoSyncCheckBox(Q_NULLPTR)
	, isolineGridCheckBox(Q_NULLPTR)
	, isolineCitiesCheckBox(Q_NULLPTR)
	, starVisGridCheckBox(Q_NULLPTR)
	, starVisCitiesCheckBox(Q_NULLPTR)
	, daylightGridCheckBox(Q_NULLPTR)
	, daylightCitiesCheckBox(Q_NULLPTR)
	, setLocationCheckBox(Q_NULLPTR)
	, daylightYearSpin(Q_NULLPTR)
	, daylightMonthSpin(Q_NULLPTR)
	, daylightDaySpin(Q_NULLPTR)
	, daylightAltitudeCombo(Q_NULLPTR)
	, calculateButton(Q_NULLPTR)
	, starViewToggleButton(Q_NULLPTR)
	, starVisLocationButton(Q_NULLPTR)
	, starVisResetButton(Q_NULLPTR)
	, daylightSyncButton(Q_NULLPTR)
	, daylightLocationButton(Q_NULLPTR)
	, daylightResetButton(Q_NULLPTR)
	, currentTimeLabel(Q_NULLPTR)
	, stepValueLabel(Q_NULLPTR)
	, stepLabel(Q_NULLPTR)
	, isolineUtcLabel(Q_NULLPTR)
	, starVisAltLabel(Q_NULLPTR)
	, nowButton(Q_NULLPTR)
	, sendTwilightButton(Q_NULLPTR)
	, core(Q_NULLPTR)
{
}

VisibilityMapDialog::~VisibilityMapDialog()
{
}

void VisibilityMapDialog::retranslate()
{
	if (!dialog)
		return;

	// Note: q_() strings will only be translated once this plugin is added
	// to Stellarium's po/stellarium-visibilitymap/ translation catalogue.
	// Until then, retranslate() ensures the UI is consistent after a language
	// change even if strings remain in English.
	titleBar->setTitle(q_("Visibility Map"));

	// Tab labels
	if (tabWidget)
	{
		tabWidget->setTabText(0, q_("Twilight zones"));
		tabWidget->setTabText(1, q_("Rise/set isolines"));
		tabWidget->setTabText(2, q_("Object visibility"));
		tabWidget->setTabText(3, q_("Daylight length"));
	}

	// Shared checkbox
	if (setLocationCheckBox)
		setLocationCheckBox->setText(q_("Click map to set location"));

	// ── Twilight tab ─────────────────────────────────────────────────────────
	{
		const int idx = stepCombo->currentIndex();
		stepCombo->blockSignals(true);
		stepCombo->clear();
		stepCombo->addItem(q_("Minute"),         StepMinute);
		stepCombo->addItem(q_("Hour"),           StepHour);
		stepCombo->addItem(q_("Day"),            StepDay);
		stepCombo->addItem(q_("Calendar month"), StepCalendarMonth);
		stepCombo->addItem(q_("Calendar year"),  StepCalendarYear);
		stepCombo->setCurrentIndex(qBound(0, idx, stepCombo->count() - 1));
		stepCombo->blockSignals(false);
	}
	if (twilightGridCheckBox)    twilightGridCheckBox->setText(q_("Grid"));
	if (twilightAutoSyncCheckBox) twilightAutoSyncCheckBox->setText(q_("Sync with Stellarium"));
	if (stepLabel)               stepLabel->setText(q_("Step"));
	if (nowButton)               nowButton->setText(q_("Now"));
	if (sendTwilightButton)      sendTwilightButton->setText(q_("Send to Stellarium"));

	// ── Isolines tab ─────────────────────────────────────────────────────────
	if (isolineBodyCombo)
	{
		const int bodyIdx = isolineBodyCombo->currentIndex();
		isolineBodyCombo->blockSignals(true);
		isolineBodyCombo->clear();
		isolineBodyCombo->addItem(q_("Sun"),  SunriseSunsetMapWidget::Sun);
		isolineBodyCombo->addItem(q_("Moon"), SunriseSunsetMapWidget::Moon);
		isolineBodyCombo->setCurrentIndex(qBound(0, bodyIdx, isolineBodyCombo->count() - 1));
		isolineBodyCombo->blockSignals(false);
	}
	if (isolineModeCombo)
	{
		const int modeIdx = isolineModeCombo->currentIndex();
		isolineModeCombo->blockSignals(true);
		isolineModeCombo->clear();
		isolineModeCombo->addItem(q_("Rise"),                  SunriseSunsetMapWidget::Sunrise);
		isolineModeCombo->addItem(q_("Set"),                   SunriseSunsetMapWidget::Sunset);
		isolineModeCombo->addItem(q_("Civil dawn (−6°)"),      SunriseSunsetMapWidget::CivilDawn);
		isolineModeCombo->addItem(q_("Civil dusk (−6°)"),      SunriseSunsetMapWidget::CivilDusk);
		isolineModeCombo->addItem(q_("Nautical dawn (−12°)"),  SunriseSunsetMapWidget::NauticalDawn);
		isolineModeCombo->addItem(q_("Nautical dusk (−12°)"),  SunriseSunsetMapWidget::NauticalDusk);
		isolineModeCombo->addItem(q_("Astro. dawn (−18°)"),    SunriseSunsetMapWidget::AstronomicalDawn);
		isolineModeCombo->addItem(q_("Astro. dusk (−18°)"),    SunriseSunsetMapWidget::AstronomicalDusk);
		isolineModeCombo->setCurrentIndex(qBound(0, modeIdx, isolineModeCombo->count() - 1));
		isolineModeCombo->blockSignals(false);
	}
	if (isolineSyncButton)     isolineSyncButton->setText(q_("Sync from Stellarium"));
	if (isolineSendButton)     isolineSendButton->setText(q_("Send to Stellarium"));
	if (isolineUtcLabel)       isolineUtcLabel->setText(q_("UTC:"));
	if (isolineGridCheckBox)   isolineGridCheckBox->setText(q_("Grid"));
	if (isolineCitiesCheckBox) isolineCitiesCheckBox->setText(q_("Cities"));
	if (isolineLocationButton) isolineLocationButton->setText(q_("Current location"));
	if (isolineResetButton)    isolineResetButton->setText(q_("World"));

	// ── Object visibility tab ────────────────────────────────────────────────
	if (calculateButton)       calculateButton->setText(q_("Calculate"));
	if (starVisGridCheckBox)   starVisGridCheckBox->setText(q_("Grid"));
	if (starVisCitiesCheckBox) starVisCitiesCheckBox->setText(q_("Cities"));
	if (starVisLocationButton) starVisLocationButton->setText(q_("Current location"));
	if (starVisResetButton)    starVisResetButton->setText(q_("World"));
	if (starVisAltLabel)       starVisAltLabel->setText(q_("Min. altitude:"));
	if (starViewToggleButton)
		starViewToggleButton->setText(starViewToggleButton->isChecked()
		    ? q_("World map") : q_("Best visibility calendar"));

	// ── Daylight length tab ──────────────────────────────────────────────────
	if (daylightAltitudeCombo)
	{
		const int altIdx = daylightAltitudeCombo->currentIndex();
		daylightAltitudeCombo->blockSignals(true);
		daylightAltitudeCombo->clear();
		daylightAltitudeCombo->addItem(q_("Sunrise/sunset (standard)"),    DaylightLengthMapWidget::PresetSunrise);
		daylightAltitudeCombo->addItem(q_("Civil twilight (−6°)"),         DaylightLengthMapWidget::PresetCivil);
		daylightAltitudeCombo->addItem(q_("Nautical twilight (−12°)"),     DaylightLengthMapWidget::PresetNautical);
		daylightAltitudeCombo->addItem(q_("Astronomical twilight (−18°)"), DaylightLengthMapWidget::PresetAstronomical);
		daylightAltitudeCombo->setCurrentIndex(qBound(0, altIdx, daylightAltitudeCombo->count() - 1));
		daylightAltitudeCombo->blockSignals(false);
	}
	if (daylightGridCheckBox)   daylightGridCheckBox->setText(q_("Grid"));
	if (daylightCitiesCheckBox) daylightCitiesCheckBox->setText(q_("Cities"));
	if (daylightSyncButton)     daylightSyncButton->setText(q_("Use Stellarium date"));
	if (daylightLocationButton) daylightLocationButton->setText(q_("Current location"));
	if (daylightResetButton)    daylightResetButton->setText(q_("World"));

	// Invalidate all widget caches — translated strings are baked into them.
	if (mapWidget)               mapWidget->update();
	if (isolineMapWidget)        isolineMapWidget->invalidateCache();
	if (starVisibilityMapWidget) starVisibilityMapWidget->invalidateCache();
	if (starCalendarWidget)    { starCalendarWidget->setCacheDirty(); starCalendarWidget->update(); }
	if (daylightLengthMapWidget) daylightLengthMapWidget->invalidateCache();

	updateLabels();
}

void VisibilityMapDialog::createDialogContent()
{
	core = StelApp::getInstance().getCore();

	dialog->setMinimumSize(760, 520);
	dialog->installEventFilter(this);
	dialog->setFocusPolicy(Qt::StrongFocus);

	QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	titleBar = new TitleBar(dialog);
	titleBar->setObjectName(QStringLiteral("titleBar"));
	titleBar->setTitle(q_("Visibility Map"));
	mainLayout->addWidget(titleBar);
	connect(titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	tabWidget = new QTabWidget(dialog);
	tabWidget->setTabPosition(QTabWidget::South);

	// "Click to set location" checkbox — shared across all map tabs.
	setLocationCheckBox = new QCheckBox(q_("Click map to set location"), dialog);
	setLocationCheckBox->setToolTip(
	        q_("When checked, clicking anywhere on the map instantly moves the "
	           "Stellarium observer to that geographic location."));
	setLocationCheckBox->setChecked(false);
	tabWidget->setCornerWidget(setLocationCheckBox, Qt::TopRightCorner);

	mainLayout->addWidget(tabWidget, 1);

	// ── Tab 1: Twilight zones ─────────────────────────────────────────────
	QWidget* twilightTab = new QWidget(tabWidget);
	QVBoxLayout* twilightLayout = new QVBoxLayout(twilightTab);
	twilightLayout->setContentsMargins(0, 0, 0, 0);
	twilightLayout->setSpacing(6);

	mapWidget = new EarthShadowMapWidget(dialog);
	mapWidget->setMinimumSize(720, 360);
	mapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	twilightLayout->addWidget(mapWidget, 1);

	QHBoxLayout* timeLayout = new QHBoxLayout;
	timeLayout->setSpacing(6);
	twilightLayout->addLayout(timeLayout);

	stepLabel = new QLabel(q_("Step"), dialog);
	timeLayout->addWidget(stepLabel);

	stepCombo = new QComboBox(dialog);
	timeLayout->addWidget(stepCombo);
	connect(stepCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(refresh()));

	QToolButton* backButton = new QToolButton(dialog);
	backButton->setText(QStringLiteral("<"));
	backButton->setToolTip(q_("Step backward"));
	backButton->setAutoRepeat(true);
	backButton->setAutoRepeatDelay(350);
	backButton->setAutoRepeatInterval(70);
	timeLayout->addWidget(backButton);
	connect(backButton, &QToolButton::clicked, this, &VisibilityMapDialog::stepBackward);

	stepValueLabel = new QLabel(dialog);
	stepValueLabel->setAlignment(Qt::AlignCenter);
	stepValueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	timeLayout->addWidget(stepValueLabel);

	QToolButton* forwardButton = new QToolButton(dialog);
	forwardButton->setText(QStringLiteral(">"));
	forwardButton->setToolTip(q_("Step forward"));
	forwardButton->setAutoRepeat(true);
	forwardButton->setAutoRepeatDelay(350);
	forwardButton->setAutoRepeatInterval(70);
	timeLayout->addWidget(forwardButton);
	connect(forwardButton, &QToolButton::clicked, this, &VisibilityMapDialog::stepForward);

	nowButton = new QPushButton(q_("Now"), dialog);
	timeLayout->addWidget(nowButton);
	connect(nowButton, &QPushButton::clicked, this, &VisibilityMapDialog::setToCurrentSystemTime);

	sendTwilightButton = new QPushButton(q_("Send to Stellarium"), dialog);
	sendTwilightButton->setToolTip(q_("Push the twilight map's current time to the planetarium"));
	timeLayout->addWidget(sendTwilightButton);
	connect(sendTwilightButton, &QPushButton::clicked, this, &VisibilityMapDialog::sendTwilightToCore);

	twilightGridCheckBox = new QCheckBox(q_("Grid"), dialog);
	twilightGridCheckBox->setToolTip(q_("Show geographic grid, tropics, and polar circles"));
	timeLayout->addWidget(twilightGridCheckBox);
	connect(twilightGridCheckBox, &QCheckBox::toggled, this, &VisibilityMapDialog::setShowGeographicGrid);

	twilightAutoSyncCheckBox = new QCheckBox(q_("Sync with Stellarium"), dialog);
	twilightAutoSyncCheckBox->setToolTip(
	        q_("When checked, the twilight map follows Stellarium's clock in real time.\n"
	           "Uncheck to decouple the map from the planetarium (recommended during time animation)."));
	twilightAutoSyncCheckBox->setChecked(false);
	timeLayout->addWidget(twilightAutoSyncCheckBox);
	connect(twilightAutoSyncCheckBox, &QCheckBox::toggled,
	        this, &VisibilityMapDialog::onTwilightAutoSyncToggled);

	timeLayout->addStretch(1);

	QHBoxLayout* labelLayout = new QHBoxLayout;
	labelLayout->setSpacing(8);
	twilightLayout->addLayout(labelLayout);

	currentTimeLabel = new QLabel(dialog);
	currentTimeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	labelLayout->addWidget(currentTimeLabel, 1);

	tabWidget->addTab(twilightTab, q_("Twilight zones"));

	// ── Tab 2: Rise/set isolines ──────────────────────────────────────────
	QWidget* isolineTab = new QWidget(tabWidget);
	QVBoxLayout* isolineLayout = new QVBoxLayout(isolineTab);
	isolineLayout->setContentsMargins(0, 0, 0, 0);
	isolineLayout->setSpacing(6);

	isolineMapWidget = new SunriseSunsetMapWidget(isolineTab);
	isolineMapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	isolineLayout->addWidget(isolineMapWidget, 1);

	QHBoxLayout* isolineControls = new QHBoxLayout;
	isolineControls->setSpacing(6);
	isolineLayout->addLayout(isolineControls);

	isolineControls->addWidget(new QLabel(q_("Body"), isolineTab));
	isolineBodyCombo = new QComboBox(isolineTab);
	isolineBodyCombo->addItem(q_("Sun"), SunriseSunsetMapWidget::Sun);
	isolineBodyCombo->addItem(q_("Moon"), SunriseSunsetMapWidget::Moon);
	isolineControls->addWidget(isolineBodyCombo);
	connect(isolineBodyCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(refresh()));

	isolineControls->addWidget(new QLabel(q_("Event"), isolineTab));
	isolineModeCombo = new QComboBox(isolineTab);
	isolineModeCombo->addItem(q_("Rise"),                  SunriseSunsetMapWidget::Sunrise);
	isolineModeCombo->addItem(q_("Set"),                   SunriseSunsetMapWidget::Sunset);
	isolineModeCombo->addItem(q_("Civil dawn (−6°)"),      SunriseSunsetMapWidget::CivilDawn);
	isolineModeCombo->addItem(q_("Civil dusk (−6°)"),      SunriseSunsetMapWidget::CivilDusk);
	isolineModeCombo->addItem(q_("Nautical dawn (−12°)"),  SunriseSunsetMapWidget::NauticalDawn);
	isolineModeCombo->addItem(q_("Nautical dusk (−12°)"),  SunriseSunsetMapWidget::NauticalDusk);
	isolineModeCombo->addItem(q_("Astro. dawn (−18°)"),    SunriseSunsetMapWidget::AstronomicalDawn);
	isolineModeCombo->addItem(q_("Astro. dusk (−18°)"),    SunriseSunsetMapWidget::AstronomicalDusk);
	isolineControls->addWidget(isolineModeCombo);
	connect(isolineModeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(refresh()));

	// Twilight options only apply to the Sun — hide them when Moon is selected.
	connect(isolineBodyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this, [this](int) {
		        const bool sunSelected =
		            isolineBodyCombo->currentData().toInt() == SunriseSunsetMapWidget::Sun;
		        // Rows 2–7 are twilight items
		        auto* view = qobject_cast<QListView*>(isolineModeCombo->view());
		        if (view)
		        	for (int i = 2; i < isolineModeCombo->count(); ++i)
		        		view->setRowHidden(i, !sunSelected);
		        // If Moon is selected and a twilight mode is active, revert to Rise
		        if (!sunSelected && isolineModeCombo->currentIndex() >= 2)
		        	isolineModeCombo->setCurrentIndex(0);
	        });

	isolineSyncButton = new QPushButton(q_("Sync from Stellarium"), isolineTab);
	isolineSyncButton->setToolTip(q_("Copy current Stellarium time and location timezone to this map"));
	isolineControls->addWidget(isolineSyncButton);
	connect(isolineSyncButton, &QPushButton::clicked, this, &VisibilityMapDialog::syncIsolineFromCore);

	isolineSendButton = new QPushButton(q_("Send to Stellarium"), isolineTab);
	isolineSendButton->setToolTip(q_("Push this map's time back to the planetarium"));
	isolineControls->addWidget(isolineSendButton);
	connect(isolineSendButton, &QPushButton::clicked, this, &VisibilityMapDialog::sendIsolineToCore);

	// UTC offset override — lets users explore times in any timezone
	// without changing their Stellarium observer location.
	isolineUtcLabel = new QLabel(q_("UTC:"), isolineTab);
	isolineControls->addWidget(isolineUtcLabel);
	isolineUtcSpin = new QDoubleSpinBox(isolineTab);
	isolineUtcSpin->setRange(-12.0, 14.0);
	isolineUtcSpin->setSingleStep(0.5);
	isolineUtcSpin->setDecimals(1);
	isolineUtcSpin->setSuffix(QStringLiteral("h"));
	isolineUtcSpin->setValue(0.0);
	isolineUtcSpin->setToolTip(q_("UTC offset for isoline time labels.\n"
	                               "Automatically updated when syncing from Stellarium or changing location.\n"
	                               "Override manually to see times in a different timezone."));
	isolineControls->addWidget(isolineUtcSpin);
	connect(isolineUtcSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
	        isolineMapWidget, &SunriseSunsetMapWidget::setUtcOffsetHours);
	// Keep spinbox in sync when auto-update changes the offset
	connect(isolineMapWidget, &SunriseSunsetMapWidget::utcOffsetChanged,
	        this, [this](double h) {
		        isolineUtcSpin->blockSignals(true);
		        isolineUtcSpin->setValue(h);
		        isolineUtcSpin->blockSignals(false);
	        });
	// Auto-update UTC offset when Stellarium location changes
	{
		StelCore* core = StelApp::getInstance().getCore();
		connect(core, &StelCore::locationChanged,
		        this, [this](const StelLocation&) {
			        if (isolineMapWidget)
			        	isolineMapWidget->resetUtcOffsetToLocation();
		        });
	}

	isolineGridCheckBox = new QCheckBox(q_("Grid"), isolineTab);
	isolineGridCheckBox->setToolTip(q_("Show geographic grid, tropics, and polar circles"));
	isolineControls->addWidget(isolineGridCheckBox);
	connect(isolineGridCheckBox, &QCheckBox::toggled, this, &VisibilityMapDialog::setShowGeographicGrid);

	isolineCitiesCheckBox = new QCheckBox(q_("Cities"), isolineTab);
	isolineCitiesCheckBox->setChecked(true);
	isolineCitiesCheckBox->setToolTip(q_("Show city labels when zoomed in"));
	isolineControls->addWidget(isolineCitiesCheckBox);
	connect(isolineCitiesCheckBox, &QCheckBox::toggled,
	        isolineMapWidget, &SunriseSunsetMapWidget::setFlagShowCities);

	isolinePrevDayButton = new QToolButton(isolineTab);
	isolinePrevDayButton->setText(QStringLiteral("<"));
	isolinePrevDayButton->setToolTip(q_("Previous day"));
	isolinePrevDayButton->setAutoRepeat(true);
	isolinePrevDayButton->setAutoRepeatDelay(350);
	isolinePrevDayButton->setAutoRepeatInterval(90);
	isolineControls->addWidget(isolinePrevDayButton);
	connect(isolinePrevDayButton, &QToolButton::clicked, this, &VisibilityMapDialog::previousIsolineDay);

	QLabel* dayLabel = new QLabel(q_("Day"), isolineTab);
	dayLabel->setAlignment(Qt::AlignCenter);
	isolineControls->addWidget(dayLabel);

	isolineNextDayButton = new QToolButton(isolineTab);
	isolineNextDayButton->setText(QStringLiteral(">"));
	isolineNextDayButton->setToolTip(q_("Next day"));
	isolineNextDayButton->setAutoRepeat(true);
	isolineNextDayButton->setAutoRepeatDelay(350);
	isolineNextDayButton->setAutoRepeatInterval(90);
	isolineControls->addWidget(isolineNextDayButton);
	connect(isolineNextDayButton, &QToolButton::clicked, this, &VisibilityMapDialog::nextIsolineDay);

	isolineLocationButton = new QPushButton(q_("Current location"), isolineTab);
	isolineControls->addWidget(isolineLocationButton);
	connect(isolineLocationButton, &QPushButton::clicked, this, &VisibilityMapDialog::zoomIsolineViewToCurrentLocation);

	isolineResetButton = new QPushButton(q_("World"), isolineTab);
	isolineControls->addWidget(isolineResetButton);
	connect(isolineResetButton, &QPushButton::clicked, this, &VisibilityMapDialog::resetIsolineView);
	isolineControls->addStretch(1);

	tabWidget->addTab(isolineTab, q_("Rise/set isolines"));

	// ── Tab 3: Object visibility ──────────────────────────────────────────
	QWidget* starVisTab = new QWidget(tabWidget);
	QVBoxLayout* starVisLayout = new QVBoxLayout(starVisTab);
	starVisLayout->setContentsMargins(0, 0, 0, 0);
	starVisLayout->setSpacing(6);

	// Stacked widget: page 0 = world map, page 1 = calendar diagram
	starViewStack = new QStackedWidget(starVisTab);

	starVisibilityMapWidget = new StarVisibilityMapWidget(starViewStack);
	starVisibilityMapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	starViewStack->addWidget(starVisibilityMapWidget);   // index 0

	starCalendarWidget = new StarCalendarWidget(starViewStack);
	starCalendarWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	starViewStack->addWidget(starCalendarWidget);        // index 1

	starViewStack->setCurrentIndex(0);
	starVisLayout->addWidget(starViewStack, 1);

	QHBoxLayout* starVisControls = new QHBoxLayout;
	starVisControls->setSpacing(6);
	starVisLayout->addLayout(starVisControls);

	// "Calculate" — triggers both map and calendar simultaneously.
	// Enabled only when Stellarium has a selected star.
	calculateButton = new QPushButton(q_("Calculate"), starVisTab);
	calculateButton->setToolTip(
	        q_("Compute and display visibility for the selected object at the current simulation date"));
	calculateButton->setEnabled(starVisibilityMapWidget->hasSelection());
	starVisControls->addWidget(calculateButton);
	connect(calculateButton, &QPushButton::clicked,
	        starVisibilityMapWidget, &StarVisibilityMapWidget::calculateVisibility);
	connect(calculateButton, &QPushButton::clicked,
	        this, &VisibilityMapDialog::onCalculateStarVisibility);

	// Toggle between world map and "best visibility" calendar diagram
	starViewToggleButton = new QPushButton(q_("Best visibility calendar"), starVisTab);
	starViewToggleButton->setToolTip(
	        q_("Switch to the latitude/calendar diagram showing when the sky is dark at culmination"));
	starViewToggleButton->setCheckable(true);
	starViewToggleButton->setChecked(false);
	starVisControls->addWidget(starViewToggleButton);
	connect(starViewToggleButton, &QPushButton::toggled,
	        this, &VisibilityMapDialog::toggleStarView);

	// Grid checkbox (world map only)
	starVisGridCheckBox = new QCheckBox(q_("Grid"), starVisTab);
	starVisGridCheckBox->setToolTip(q_("Show geographic grid (world map view)"));
	starVisControls->addWidget(starVisGridCheckBox);
	connect(starVisGridCheckBox, &QCheckBox::toggled,
	        starVisibilityMapWidget, &StarVisibilityMapWidget::setFlagShowGrid);

	starVisCitiesCheckBox = new QCheckBox(q_("Cities"), starVisTab);
	starVisCitiesCheckBox->setChecked(true);
	starVisCitiesCheckBox->setToolTip(q_("Show city labels when zoomed in (world map view)"));
	starVisControls->addWidget(starVisCitiesCheckBox);
	connect(starVisCitiesCheckBox, &QCheckBox::toggled,
	        starVisibilityMapWidget, &StarVisibilityMapWidget::setFlagShowCities);

	// Zoom to current location
	starVisLocationButton = new QPushButton(q_("Current location"), starVisTab);
	starVisControls->addWidget(starVisLocationButton);
	connect(starVisLocationButton, &QPushButton::clicked,
	        this, &VisibilityMapDialog::zoomStarVisibilityToCurrentLocation);

	// Reset / world view
	starVisResetButton = new QPushButton(q_("World"), starVisTab);
	starVisControls->addWidget(starVisResetButton);
	connect(starVisResetButton, &QPushButton::clicked,
	        this, &VisibilityMapDialog::resetStarVisibilityView);

	starVisAltLabel = new QLabel(q_("Min. altitude:"), starVisTab);
	starVisControls->addWidget(starVisAltLabel);
	QSpinBox* goodAltSpin = new QSpinBox(starVisTab);
	goodAltSpin->setRange(1, 89);
	goodAltSpin->setValue(5);
	goodAltSpin->setSuffix(QStringLiteral("°"));
	goodAltSpin->setToolTip(q_("Minimum altitude for the \"good visibility\" dash-dot line"));
	starVisControls->addWidget(goodAltSpin);
	connect(goodAltSpin, QOverload<int>::of(&QSpinBox::valueChanged),
	        starVisibilityMapWidget, &StarVisibilityMapWidget::setGoodVisibilityAltitude);

	starVisControls->addStretch(1);

	tabWidget->addTab(starVisTab, q_("Object visibility"));

	// ── Tab 4: Daylight length ────────────────────────────────────────────
	QWidget* daylightLenTab = new QWidget(tabWidget);
	QVBoxLayout* daylightLenLayout = new QVBoxLayout(daylightLenTab);
	daylightLenLayout->setContentsMargins(0, 0, 0, 0);
	daylightLenLayout->setSpacing(6);

	daylightLengthMapWidget = new DaylightLengthMapWidget(daylightLenTab);
	daylightLengthMapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	daylightLenLayout->addWidget(daylightLengthMapWidget, 1);

	// ── Controls row 1: date ─────────────────────────────────────────────
	QHBoxLayout* daylightDateRow = new QHBoxLayout;
	daylightDateRow->setSpacing(6);
	daylightLenLayout->addLayout(daylightDateRow);

	daylightDateRow->addWidget(new QLabel(q_("Date"), daylightLenTab));

	// Three spinboxes instead of QDateEdit — no calendar widget date range limit.
	// This allows dates across the full DE441 window (±15,000 years).
	const QDate initialDate = daylightLengthMapWidget->currentDate();

	daylightYearSpin = new QSpinBox(daylightLenTab);
	daylightYearSpin->setRange(-9999, 9999);
	daylightYearSpin->setValue(initialDate.year());
	daylightYearSpin->setPrefix(q_("Y "));
	daylightYearSpin->setMinimumWidth(90);
	daylightYearSpin->setToolTip(q_("Year (negative = BCE)"));
	daylightYearSpin->setKeyboardTracking(false);
	daylightDateRow->addWidget(daylightYearSpin);

	daylightMonthSpin = new QSpinBox(daylightLenTab);
	daylightMonthSpin->setRange(1, 12);
	daylightMonthSpin->setValue(initialDate.month());
	daylightMonthSpin->setPrefix(q_("M "));
	daylightMonthSpin->setMinimumWidth(60);
	daylightMonthSpin->setToolTip(q_("Month (1–12)"));
	daylightMonthSpin->setKeyboardTracking(false);
	daylightDateRow->addWidget(daylightMonthSpin);

	daylightDaySpin = new QSpinBox(daylightLenTab);
	daylightDaySpin->setRange(1, 31);
	daylightDaySpin->setValue(initialDate.day());
	daylightDaySpin->setPrefix(q_("D "));
	daylightDaySpin->setMinimumWidth(60);
	daylightDaySpin->setToolTip(q_("Day of month"));
	daylightDaySpin->setKeyboardTracking(false);
	daylightDateRow->addWidget(daylightDaySpin);

	// Connect all three spinboxes to the same slot
	connect(daylightYearSpin,  QOverload<int>::of(&QSpinBox::valueChanged),
	        this, &VisibilityMapDialog::onDaylightDateSpinChanged);
	connect(daylightMonthSpin, QOverload<int>::of(&QSpinBox::valueChanged),
	        this, &VisibilityMapDialog::onDaylightDateSpinChanged);
	connect(daylightDaySpin,   QOverload<int>::of(&QSpinBox::valueChanged),
	        this, &VisibilityMapDialog::onDaylightDateSpinChanged);

	// Keep spinboxes in sync when the widget steps the date internally
	connect(daylightLengthMapWidget, &DaylightLengthMapWidget::dateChanged,
	        this, [this](int y, int m, int d) {
		        daylightYearSpin ->blockSignals(true);
		        daylightMonthSpin->blockSignals(true);
		        daylightDaySpin  ->blockSignals(true);
		        daylightYearSpin ->setValue(y);
		        daylightMonthSpin->setValue(m);
		        // Compute days in month — use abs(year) for QDate since it
		        // only handles positive years; we just need the month length.
		        const QDate test(qAbs(y) > 0 ? qAbs(y) : 1, m, 1);
		        const int dim = test.isValid() ? test.daysInMonth() : 31;
		        daylightDaySpin->setMaximum(dim > 0 ? dim : 31);
		        daylightDaySpin  ->setValue(d);
		        daylightYearSpin ->blockSignals(false);
		        daylightMonthSpin->blockSignals(false);
		        daylightDaySpin  ->blockSignals(false);
	        });

	// Day step buttons
	QToolButton* dlPrevDay = new QToolButton(daylightLenTab);
	dlPrevDay->setText(QStringLiteral("−1d"));
	dlPrevDay->setAutoRepeat(true);
	dlPrevDay->setAutoRepeatDelay(350);
	dlPrevDay->setAutoRepeatInterval(80);
	daylightDateRow->addWidget(dlPrevDay);
	connect(dlPrevDay, &QToolButton::clicked, this,
	        [this]() { daylightLengthMapWidget->stepDays(-1); });

	QToolButton* dlNextDay = new QToolButton(daylightLenTab);
	dlNextDay->setText(QStringLiteral("+1d"));
	dlNextDay->setAutoRepeat(true);
	dlNextDay->setAutoRepeatDelay(350);
	dlNextDay->setAutoRepeatInterval(80);
	daylightDateRow->addWidget(dlNextDay);
	connect(dlNextDay, &QToolButton::clicked, this,
	        [this]() { daylightLengthMapWidget->stepDays(1); });

	// Month step buttons
	QToolButton* dlPrevMonth = new QToolButton(daylightLenTab);
	dlPrevMonth->setText(QStringLiteral("−1m"));
	daylightDateRow->addWidget(dlPrevMonth);
	connect(dlPrevMonth, &QToolButton::clicked, this,
	        [this]() { daylightLengthMapWidget->stepMonths(-1); });

	QToolButton* dlNextMonth = new QToolButton(daylightLenTab);
	dlNextMonth->setText(QStringLiteral("+1m"));
	daylightDateRow->addWidget(dlNextMonth);
	connect(dlNextMonth, &QToolButton::clicked, this,
	        [this]() { daylightLengthMapWidget->stepMonths(1); });

	// Sync from Stellarium
	daylightSyncButton = new QPushButton(q_("Use Stellarium date"), daylightLenTab);
	daylightSyncButton->setToolTip(q_("Copy today's date from the Stellarium clock (does not change the planetarium view)"));
	daylightDateRow->addWidget(daylightSyncButton);
	connect(daylightSyncButton, &QPushButton::clicked,
	        this, &VisibilityMapDialog::syncDaylightDateFromCore);

	daylightDateRow->addStretch(1);

	// ── Controls row 2: threshold + view ─────────────────────────────────
	QHBoxLayout* daylightOptRow = new QHBoxLayout;
	daylightOptRow->setSpacing(6);
	daylightLenLayout->addLayout(daylightOptRow);

	daylightOptRow->addWidget(new QLabel(q_("Above altitude"), daylightLenTab));

	daylightAltitudeCombo = new QComboBox(daylightLenTab);
	daylightAltitudeCombo->addItem(q_("Sunrise/sunset (standard)"),
	                               DaylightLengthMapWidget::PresetSunrise);
	daylightAltitudeCombo->addItem(q_("Civil twilight (−6°)"),
	                               DaylightLengthMapWidget::PresetCivil);
	daylightAltitudeCombo->addItem(q_("Nautical twilight (−12°)"),
	                               DaylightLengthMapWidget::PresetNautical);
	daylightAltitudeCombo->addItem(q_("Astronomical twilight (−18°)"),
	                               DaylightLengthMapWidget::PresetAstronomical);
	daylightOptRow->addWidget(daylightAltitudeCombo);
	connect(daylightAltitudeCombo, SIGNAL(currentIndexChanged(int)),
	        this, SLOT(onDaylightAltitudePresetChanged(int)));

	daylightGridCheckBox = new QCheckBox(q_("Grid"), daylightLenTab);
	daylightOptRow->addWidget(daylightGridCheckBox);
	connect(daylightGridCheckBox, &QCheckBox::toggled,
	        daylightLengthMapWidget, &DaylightLengthMapWidget::setFlagShowGrid);

	daylightCitiesCheckBox = new QCheckBox(q_("Cities"), daylightLenTab);
	daylightCitiesCheckBox->setChecked(true);
	daylightOptRow->addWidget(daylightCitiesCheckBox);
	connect(daylightCitiesCheckBox, &QCheckBox::toggled,
	        daylightLengthMapWidget, &DaylightLengthMapWidget::setFlagShowCities);

	daylightLocationButton = new QPushButton(q_("Current location"), daylightLenTab);
	daylightOptRow->addWidget(daylightLocationButton);
	connect(daylightLocationButton, &QPushButton::clicked,
	        daylightLengthMapWidget, &DaylightLengthMapWidget::zoomToCurrentLocation);

	daylightResetButton = new QPushButton(q_("World"), daylightLenTab);
	daylightOptRow->addWidget(daylightResetButton);
	connect(daylightResetButton, &QPushButton::clicked,
	        daylightLengthMapWidget, &DaylightLengthMapWidget::resetView);

	daylightOptRow->addStretch(1);

	tabWidget->addTab(daylightLenTab, q_("Daylight length"));

	// Connect "click to set location" checkbox to all map widgets.
	connect(setLocationCheckBox, &QCheckBox::toggled,
	        mapWidget,              &EarthShadowMapWidget::setFlagSetLocationOnClick);
	connect(setLocationCheckBox, &QCheckBox::toggled,
	        isolineMapWidget,       &SunriseSunsetMapWidget::setFlagSetLocationOnClick);
	connect(setLocationCheckBox, &QCheckBox::toggled,
	        starVisibilityMapWidget,&StarVisibilityMapWidget::setFlagSetLocationOnClick);
	connect(setLocationCheckBox, &QCheckBox::toggled,
	        daylightLengthMapWidget,&DaylightLengthMapWidget::setFlagSetLocationOnClick);

	// ── Connections for object-selection changes ──────────────────────────
	// When the user clicks a different object, only the button enable-state
	// changes. The map itself is only redrawn when the user clicks Calculate.
	connect(&StelApp::getInstance().getStelObjectMgr(),
	        SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)),
	        starVisibilityMapWidget,
	        SLOT(onSelectionChanged()));
	connect(starVisibilityMapWidget, &StarVisibilityMapWidget::selectionStateChanged,
	        this, &VisibilityMapDialog::onStarVisSelectionChanged);
	connect(starVisibilityMapWidget, &StarVisibilityMapWidget::solarSystemObjectSelected,
	        this, &VisibilityMapDialog::onSolarSystemObjectSelected);

	// ── Global signals ────────────────────────────────────────────────────
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

	retranslate();
	refresh();
}

bool VisibilityMapDialog::eventFilter(QObject* object, QEvent* event)
{
	if (object == dialog && event->type() == QEvent::KeyPress)
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		switch (keyEvent->key())
		{
			case Qt::Key_Left:
				stepBy(-1);
				return true;
			case Qt::Key_Right:
				stepBy(1);
				return true;
			case Qt::Key_PageUp:
				stepBy(10);
				return true;
			case Qt::Key_PageDown:
				stepBy(-10);
				return true;
			default:
				break;
		}
	}

	return StelDialog::eventFilter(object, event);
}

double VisibilityMapDialog::currentStepDays() const
{
	switch (stepCombo->currentData().toInt())
	{
		case StepMinute:
			return 1.0 / 1440.0;
		case StepHour:
			return 1.0 / 24.0;
		case StepDay:
			return 1.0;
		default:
			return 0.;
	}
}

void VisibilityMapDialog::stepBackward()
{
	stepBy(-1);
}

void VisibilityMapDialog::stepForward()
{
	stepBy(1);
}

void VisibilityMapDialog::setToCurrentSystemTime()
{
	const double systemJD = StelUtils::getJDFromSystem();
	const bool autoSync = twilightAutoSyncCheckBox && twilightAutoSyncCheckBox->isChecked();
	if (autoSync)
		core->setJD(systemJD);
	else if (mapWidget)
		mapWidget->syncFromCore(); // just pull current Stellarium time into widget
	refresh();
}

void VisibilityMapDialog::stepBy(int multiplier)
{
	const int step = stepCombo->currentData().toInt();
	const bool autoSync = twilightAutoSyncCheckBox && twilightAutoSyncCheckBox->isChecked();

	if (autoSync)
	{
		// Twilight tab is driving Stellarium's clock.
		switch (step)
		{
			case StepCalendarMonth:
				for (int i = 0; i < qAbs(multiplier); ++i)
				{
					if (multiplier > 0) core->addCalendarMonth();
					else                core->subtractCalendarMonth();
				}
				break;
			case StepCalendarYear:
				for (int i = 0; i < qAbs(multiplier); ++i)
				{
					if (multiplier > 0) core->addCalendarYear();
					else                core->subtractCalendarYear();
				}
				break;
			default:
				core->setJD(core->getJD() + multiplier * currentStepDays());
				break;
		}
		// mapWidget will update via the dateChanged signal connection.
	}
	else
	{
		// Decoupled mode — step the widget's local clock only.
		if (!mapWidget) return;
		switch (step)
		{
			case StepCalendarMonth:
				for (int i = 0; i < qAbs(multiplier); ++i)
					mapWidget->addDays(multiplier > 0 ? 30.0 : -30.0);
				break;
			case StepCalendarYear:
				for (int i = 0; i < qAbs(multiplier); ++i)
					mapWidget->addDays(multiplier > 0 ? 365.0 : -365.0);
				break;
			default:
				mapWidget->addDays(multiplier * currentStepDays());
				break;
		}
	}

	refresh();
}

void VisibilityMapDialog::refresh()
{
	if (mapWidget)
	{
		// Only pull from core when auto-sync is enabled.
		const bool autoSync = twilightAutoSyncCheckBox && twilightAutoSyncCheckBox->isChecked();
		if (autoSync)
			mapWidget->updateFromCore();
		mapWidget->setFlagShowGrid(twilightGridCheckBox && twilightGridCheckBox->isChecked());
	}
	if (isolineMapWidget)
	{
		isolineMapWidget->setBodyMode(isolineBodyCombo->currentData().toInt());
		isolineMapWidget->setEventMode(isolineModeCombo->currentData().toInt());
		isolineMapWidget->setFlagShowGrid(isolineGridCheckBox && isolineGridCheckBox->isChecked());
		isolineMapWidget->updateFromCore();
	}
	if (starVisibilityMapWidget)
	{
		// The visibility map is not time-driven; it only updates on Calculate.
		// Nothing to do here.
	}

	updateLabels();
}

void VisibilityMapDialog::setShowGeographicGrid(bool show)
{
	if (twilightGridCheckBox && twilightGridCheckBox->isChecked() != show)
	{
		twilightGridCheckBox->blockSignals(true);
		twilightGridCheckBox->setChecked(show);
		twilightGridCheckBox->blockSignals(false);
	}

	if (isolineGridCheckBox && isolineGridCheckBox->isChecked() != show)
	{
		isolineGridCheckBox->blockSignals(true);
		isolineGridCheckBox->setChecked(show);
		isolineGridCheckBox->blockSignals(false);
	}

	if (mapWidget)
		mapWidget->setFlagShowGrid(show);
	if (isolineMapWidget)
		isolineMapWidget->setFlagShowGrid(show);
}

void VisibilityMapDialog::resetIsolineView()
{
	if (isolineMapWidget)
		isolineMapWidget->resetView();
}

void VisibilityMapDialog::zoomIsolineViewToCurrentLocation()
{
	if (isolineMapWidget)
		isolineMapWidget->zoomToCurrentLocation();
}

void VisibilityMapDialog::syncIsolineFromCore()
{
	if (mapWidget)
		mapWidget->updateFromCore();

	if (isolineMapWidget)
	{
		isolineMapWidget->setBodyMode(isolineBodyCombo->currentData().toInt());
		isolineMapWidget->setEventMode(isolineModeCombo->currentData().toInt());
		isolineMapWidget->syncFromCore();
		// Reset to location timezone — clears any manual override.
		isolineMapWidget->resetUtcOffsetToLocation();
		isolineMapWidget->zoomToCurrentLocation();
	}

	// Update the UTC spinbox to reflect the (now synced) offset.
	if (isolineUtcSpin && isolineMapWidget)
	{
		isolineUtcSpin->blockSignals(true);
		isolineUtcSpin->setValue(isolineMapWidget->getUtcOffsetHours());
		isolineUtcSpin->blockSignals(false);
	}

	updateLabels();
}

void VisibilityMapDialog::sendIsolineToCore()
{
	if (isolineMapWidget)
		isolineMapWidget->syncToCore();
	updateLabels();
}

void VisibilityMapDialog::sendTwilightToCore()
{
	if (mapWidget)
		mapWidget->syncToCore();
	updateLabels();
}

void VisibilityMapDialog::onTwilightAutoSyncToggled(bool checked)
{
	if (!mapWidget || !core) return;

	if (checked)
	{
		// timeSyncOccurred fires on any JD change (time panel, animation,
		// scripts) — more responsive than dateChanged which only fires on
		// calendar-date boundaries.
		connect(core, &StelCore::timeSyncOccurred,
		        mapWidget, [this](double) { mapWidget->syncFromCore(); },
		        Qt::UniqueConnection);
		mapWidget->syncFromCore();
	}
	else
	{
		disconnect(core, &StelCore::timeSyncOccurred,
		           mapWidget, nullptr);
	}
}

void VisibilityMapDialog::previousIsolineDay()
{
	if (isolineMapWidget)
		isolineMapWidget->addDays(-1);
}

void VisibilityMapDialog::nextIsolineDay()
{
	if (isolineMapWidget)
		isolineMapWidget->addDays(1);
}

// ── Star visibility tab slots ─────────────────────────────────────────────

void VisibilityMapDialog::syncStarVisibility()
{
	// kept for potential future use; currently Calculate does the job
	if (starVisibilityMapWidget)
		starVisibilityMapWidget->calculateVisibility();
}

void VisibilityMapDialog::onSolarSystemObjectSelected()
{
	// The user clicked Calculate on a solar system object.
	// The world map view still works (it only needs declination),
	// but the calendar diagram is meaningless for moving objects.
	if (starViewToggleButton)
	{
		starViewToggleButton->setEnabled(false);
		starViewToggleButton->setToolTip(
		        q_("The calendar diagram is only available for stars and deep-sky objects.\n"
		           "Solar system objects move too fast for the diagram to be meaningful."));
		// If we were in calendar mode, switch back to the world map.
		if (starViewToggleButton->isChecked())
		{
			starViewToggleButton->setChecked(false);
			// toggleStarView() fires automatically via the toggled signal
		}
	}
	if (starCalendarWidget)
		starCalendarWidget->clearObject();
}

void VisibilityMapDialog::onCalculateStarVisibility()
{
	// Re-enable the calendar toggle (it may have been disabled for a solar system object).
	if (starViewToggleButton)
	{
		starViewToggleButton->setEnabled(true);
		starViewToggleButton->setToolTip(
		        q_("Switch to the latitude/calendar diagram showing when the sky is dark at culmination"));
	}

	// Forward the calculated result to the calendar widget.
	if (!starVisibilityMapWidget || !starCalendarWidget) return;
	if (!starVisibilityMapWidget->hasVisibilityResult()) return;

	starCalendarWidget->setObject(starVisibilityMapWidget->objectDecDeg(),
	                               starVisibilityMapWidget->objectRaDeg(),
	                               starVisibilityMapWidget->objectNameStr(),
	                               starVisibilityMapWidget->calcYearVal());
}

void VisibilityMapDialog::toggleStarView()
{
	if (!starViewStack || !starViewToggleButton) return;
	const bool calendarMode = starViewToggleButton->isChecked();
	starViewStack->setCurrentIndex(calendarMode ? 1 : 0);
	starViewToggleButton->setText(calendarMode
	    ? q_("World map")
	    : q_("Best visibility calendar"));

	// Hide the map navigation buttons in calendar mode — they act on the
	// world map which is not visible.
	if (starVisLocationButton) starVisLocationButton->setVisible(!calendarMode);
	if (starVisResetButton)    starVisResetButton->setVisible(!calendarMode);
	if (starVisGridCheckBox)   starVisGridCheckBox->setVisible(!calendarMode);
	if (starVisCitiesCheckBox) starVisCitiesCheckBox->setVisible(!calendarMode);
}

void VisibilityMapDialog::onStarVisSelectionChanged(bool objectAvailable)
{
	if (calculateButton)
		calculateButton->setEnabled(objectAvailable);
}

void VisibilityMapDialog::resetStarVisibilityView()
{
	if (starVisibilityMapWidget)
		starVisibilityMapWidget->resetView();
}

void VisibilityMapDialog::zoomStarVisibilityToCurrentLocation()
{
	if (starVisibilityMapWidget)
		starVisibilityMapWidget->zoomToCurrentLocation();
}

// ── Labels ────────────────────────────────────────────────────────────────

void VisibilityMapDialog::updateLabels()
{
	if (!core || !currentTimeLabel || !stepValueLabel)
		return;

	currentTimeLabel->setText(q_("UTC") + QStringLiteral(": ") +
	                          StelUtils::julianDayToISO8601String(core->getJD(), false));
	stepValueLabel->setText(QStringLiteral("1 ") + stepCombo->currentText());
}

// ── Daylight length tab slots ─────────────────────────────────────────────

void VisibilityMapDialog::onDaylightDateSpinChanged(int /*value*/)
{
	if (!daylightLengthMapWidget || !daylightYearSpin || !daylightMonthSpin || !daylightDaySpin)
		return;

	const int y = daylightYearSpin->value();
	const int m = daylightMonthSpin->value();

	// Compute days in month without QDate — QDate is unreliable for negative years.
	// Use the same helper logic as DaylightLengthMapWidget::daysInMonthForYear().
	// We call through to the widget so the calculation is in one place.
	// For the spinbox max we just need an upper bound — clamp to what the widget accepts.
	const QDate testDate(qAbs(y) > 0 ? qAbs(y) : 1, m, 1);
	const int daysInMonth = testDate.isValid() ? testDate.daysInMonth() : 31;

	daylightDaySpin->blockSignals(true);
	daylightDaySpin->setMaximum(daysInMonth > 0 ? daysInMonth : 31);
	daylightDaySpin->blockSignals(false);

	const int d = qBound(1, daylightDaySpin->value(), daysInMonth > 0 ? daysInMonth : 31);

	// Use the year/month/day overload which works for any year including BCE.
	daylightLengthMapWidget->setDate(y, m, d);
}

void VisibilityMapDialog::syncDaylightDateFromCore()
{
	if (daylightLengthMapWidget)
		daylightLengthMapWidget->syncDateFromCore();
}

void VisibilityMapDialog::onDaylightAltitudePresetChanged(int /*index*/)
{
	if (!daylightLengthMapWidget || !daylightAltitudeCombo)
		return;
	daylightLengthMapWidget->setAltitudeThreshold(
	        daylightAltitudeCombo->currentData().toInt());
}
