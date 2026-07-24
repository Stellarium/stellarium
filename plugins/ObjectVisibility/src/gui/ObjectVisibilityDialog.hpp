/*
 * Object Visibility plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
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

#ifndef OBJECTVISIBILITYDIALOG_HPP
#define OBJECTVISIBILITYDIALOG_HPP

#include "StelDialog.hpp"
#include <QString>

class Ui_objectVisibilityDialog;
class ObjectVisibility;
class StelCore;
class QTimer;

//! Main window of the Object Visibility plug-in.
//! @ingroup objectVisibility
class ObjectVisibilityDialog : public StelDialog
{
	Q_OBJECT

public:
	ObjectVisibilityDialog();
	~ObjectVisibilityDialog() override;

public slots:
	void retranslate() override;

protected:
	void createDialogContent() override;

private slots:
	//! User pressed "Calculate".  Reads the current selection from
	//! StelObjectMgr, computes the declination at the current epoch
	//! and feeds it to the map widget.
	void calculate();

	//! Called whenever the user's selection in Stellarium changes; we
	//! use it only to enable/disable the Calculate button so the user
	//! gets immediate feedback that planets etc. are not supported.
	void onSelectedObjectChanged();

	//! Toggle "click on map to set location" mode on the map widget.
	void onSetLocationByClickToggled(bool on);

	//! Toggle "click on map to set location" mode on the twilight map.
	void onTwilightSetLocationByClickToggled(bool on);

	//! Toggle "click on map to set location" mode on the live twilight map.
	void onLiveTwilightSetLocationByClickToggled(bool on);

	//! The user clicked the map while in click-to-set mode.
	void onLocationPicked(double longitude, double latitude,
	                      const QColor &color);

	//! Spinbox value changed.
	void onGoodVisibilityLimitChanged(int degrees);

	//! Reset settings button.
	void onResetSettings();

	void onPlaceLabelsToggled(bool on);
	void onTwilightPlaceLabelsToggled(bool on);
	void onLiveTwilightPlaceLabelsToggled(bool on);
	void onPlaceLabelsPopulationChanged(int index);
	void onTwilightPlaceLabelsPopulationChanged(int index);
	void onLiveTwilightPlaceLabelsPopulationChanged(int index);
	void onPlaceLabelsNearLinesOnlyToggled(bool on);
	void onTwilightPlaceLabelsNearLinesOnlyToggled(bool on);
	void onLiveTwilightPlaceLabelsNearLinesOnlyToggled(bool on);
	void onSyncMapsToggled(bool on);
	void onMapViewChanged(double centerLongitude, double centerLatitude,
	                      double zoom);

	//! Re-sync the location marker AND the planet texture from
	//! StelCore's current observer.  Called on startup and whenever
	//! StelCore reports a location change.
	void syncMarkerToObserver();

	//! Recompute Earth-only solstice/twilight limits for the current
	//! epoch and update the twilight tab.
	void refreshTwilightLimits();

	//! Recompute the live Earth twilight map for Stellarium's current time.
	void refreshTwilightMap();

private:
	Ui_objectVisibilityDialog* ui;
	ObjectVisibility*          plugin;

	//! The object that was selected when Calculate was clicked.  We
	//! keep a reference so the title label can be refreshed (e.g. on
	//! retranslate) without re-running the geometry.  We hold it as
	//! a type + ID (which is more stable than the English name
	//! across releases).
	QString  lockedObjectId;
	QString  lockedObjectType;        // "Star" or "Nebula"
	QString  lockedObjectNameI18n;    // for label

	//! Cached english name of the planet whose texture is currently
	//! displayed in the map widget.  We compare against this to avoid
	//! reloading the same texture when the observer just changes
	//! geographic position on the same planet.
	QString  cachedPlanetName;

	QTimer* twilightMapTimer = nullptr;
	double  lastTwilightMapJd = 0.0;
	QString twilightMapCachedPlanet;

	bool placeLabelsVisible = false;
	int  placeLabelsMinimumPopulation = 1000000;
	bool placeLabelsNearLinesOnly = true;
	bool syncMaps = false;
	bool applyingMapSync = false;

	void setAboutHtml();
	void refreshTitleLabel();
	void updateCalculateButtonEnabled();
	void configurePlaceLabelControls();
	void syncPlaceLabelControls();
	void updatePlaceLabels();
	void syncMapControls();

	//! Compute the year label in astronomical convention from the
	//! StelCore's current JD.  E.g. 2026, or -10000 for 10001 BCE.
	static int currentYear(StelCore* core);

	//! True iff the given StelObject type is a star or a DSO (nebula).
	static bool isAcceptableType(const QString& type);

	//! True iff the given (English) planet name is in our supported
	//! list.  We restrict to bodies whose rotation-pole orientation is
	//! reliably known in Stellarium: Earth, Moon, the eight planets,
	//! Pluto, and the four Galilean moons.
	static bool isSupportedPlanet(const QString& englishName);
};

#endif // OBJECTVISIBILITYDIALOG_HPP
