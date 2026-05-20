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

#ifndef VISIBILITYMAP_DIALOG_HPP
#define VISIBILITYMAP_DIALOG_HPP

#include "StelDialog.hpp"

class EarthShadowMapWidget;
class SunriseSunsetMapWidget;
class StarVisibilityMapWidget;
class StarCalendarWidget;
class DaylightLengthMapWidget;
class QLabel;
class QComboBox;
class QCheckBox;
class QPushButton;
class QStackedWidget;
class QTabWidget;
class QSpinBox;
class TitleBar;
class StelCore;

class VisibilityMapDialog : public StelDialog
{
	Q_OBJECT

public:
	VisibilityMapDialog();
	~VisibilityMapDialog() override;

public slots:
	void retranslate() override;

protected:
	void createDialogContent() override;
	bool eventFilter(QObject* object, QEvent* event) override;

private slots:
	void stepBackward();
	void stepForward();
	void setToCurrentSystemTime();
	void refresh();
	void setShowGeographicGrid(bool show);
	void resetIsolineView();
	void zoomIsolineViewToCurrentLocation();
	void syncIsolineFromCore();
	void sendIsolineToCore();
	void sendTwilightToCore();
	void onTwilightAutoSyncToggled(bool checked);
	void previousIsolineDay();
	void nextIsolineDay();

	// Daylight length tab
	void onDaylightDateSpinChanged(int value);
	void syncDaylightDateFromCore();
	void onDaylightAltitudePresetChanged(int index);
	void resetStarVisibilityView();
	void zoomStarVisibilityToCurrentLocation();
	void onStarVisSelectionChanged(bool objectAvailable);
	void onCalculateStarVisibility();       //!< Forwards data to StarCalendarWidget after Calculate.
	void onSolarSystemObjectSelected();     //!< Called when Calculate is rejected for a solar system object.
	void toggleStarView();                  //!< Switch between world map and calendar diagram.
	void syncStarVisibility();

private:
	double currentStepDays() const;
	void stepBy(int multiplier);
	void updateLabels();

	TitleBar*               titleBar;
	EarthShadowMapWidget*   mapWidget;
	SunriseSunsetMapWidget* isolineMapWidget;
	StarVisibilityMapWidget* starVisibilityMapWidget;
	StarCalendarWidget*     starCalendarWidget;
	QStackedWidget*         starViewStack;
	DaylightLengthMapWidget* daylightLengthMapWidget;
	QTabWidget*             tabWidget;
	QComboBox*              stepCombo;
	QComboBox*              isolineBodyCombo;
	QComboBox*              isolineModeCombo;
	QCheckBox*              twilightGridCheckBox;
	QCheckBox*              twilightAutoSyncCheckBox;
	QCheckBox*              isolineGridCheckBox;
	QCheckBox*              isolineCitiesCheckBox;
	QCheckBox*              starVisGridCheckBox;
	QCheckBox*              starVisCitiesCheckBox;
	QCheckBox*              daylightGridCheckBox;
	QCheckBox*              daylightCitiesCheckBox;
	QSpinBox*               daylightYearSpin;
	QSpinBox*               daylightMonthSpin;
	QSpinBox*               daylightDaySpin;
	QComboBox*              daylightAltitudeCombo;
	QPushButton*            calculateButton;   //!< "Calculate visibility" — enabled only when an object is selected
	QPushButton*            starViewToggleButton; //!< Switch between world map and calendar diagram
	QLabel*                 currentTimeLabel;
	QLabel*                 stepValueLabel;
	StelCore*               core;
};

#endif /* VISIBILITYMAP_DIALOG_HPP */
