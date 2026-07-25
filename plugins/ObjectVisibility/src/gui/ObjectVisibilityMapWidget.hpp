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

#ifndef OBJECTVISIBILITYMAPWIDGET_HPP
#define OBJECTVISIBILITYMAPWIDGET_HPP

#include "MapWidget.hpp"

#include <QString>
#include <QVector>

//! A MapWidget subclass that:
//!  1. Optionally gates emission of positionChanged() through a "click
//!     to set location" mode.  Outside of that mode, the user can pan
//!     and zoom freely without ever moving the observer.
//!  2. Overlays "visibility lines" on top of the map, computed from
//!     the declination of a star/DSO.
//!  3. Optionally overlays Earth-only twilight/solstice latitude
//!     limits, computed from Earth's obliquity of date.
//!
//! All five line types are simply parallels of geographic latitude:
//!   - limit-of-visibility:   phi = dec +/- 90
//!   - good-visibility:       phi = dec +/- (90 - h_good)
//!   - passes zenith:         phi = dec
//!   - circumpolar (N):       phi = 90 - dec
//!   - circumpolar (S):       phi = -90 - dec
//!
//! A latitude that falls outside [-90, +90] is simply not drawn.
//!
//! @ingroup objectVisibility
class ObjectVisibilityMapWidget : public MapWidget
{
	Q_OBJECT

public:
	explicit ObjectVisibilityMapWidget(QWidget *parent = nullptr);

	enum OverlayMode
	{
		VisibilityOverlay,
		TwilightLimitsOverlay
	};

	void setOverlayMode(OverlayMode mode);
	OverlayMode overlayMode() const { return currentOverlayMode; }

	struct PlaceLabel
	{
		QString name;
		double longitude = 0.0;
		double latitude = 0.0;
		int population = 0;
		QChar role;
	};

	void setPlaceLabels(const QVector<PlaceLabel>& labels);
	void setPlaceLabelsVisible(bool visible);
	void setPlaceLabelMinimumPopulation(int population);
	void setPlaceLabelsNearLinesOnly(bool nearLinesOnly);

	//! Enable/disable "click on map to set observer location" mode.
	//! In normal mode, clicks on the map do nothing (panning/zooming
	//! still work).
	void setClickSetsLocationMode(bool on);
	bool clickSetsLocationMode() const { return clickToSetMode; }

	//! Set the declination (degrees, current epoch) of the object to
	//! be plotted.  Pass any value outside [-90, 90] to clear and stop
	//! drawing visibility lines.
	void setDeclination(double declinationDeg);

	//! Configure the "good visibility" altitude limit (degrees).
	//! 1..89.  Default is 5.
	void setGoodVisibilityAltitude(int degrees);

	//! Hide all visibility lines (e.g. when no object is selected).
	void clearVisibility();

	//! Set Earth obliquity of date (degrees) and draw the twilight
	//! solstice limits. Pass a value outside (0, 90) to clear.
	void setTwilightObliquity(double obliquityDeg);

	//! Hide all twilight/solstice limit lines.
	void clearTwilightLimits();

signals:
	//! Forwarded to the dialog when the user clicked on the map and we
	//! were in click-to-set mode.  Mirrors MapWidget::positionChanged.
	void locationPicked(double longitude, double latitude, const QColor &color);

protected:
	void paintEvent(QPaintEvent* event) override;

private slots:
	void onPositionChanged(double longitude, double latitude, const QColor &color);

private:
	void drawLatitudeLine(QPainter& painter, double latitudeDeg,
	                      const QPen& pen) const;
	void drawLatitudeLineCopies(QPainter& painter, double latitudeDeg,
	                            const QPen& pen) const;
	void drawLatitudeMarkers(QPainter& painter, double latitudeDeg,
	                         const QChar& marker, const QColor& color) const;
	void drawVisibilityOverlay(QPainter& painter) const;
	void drawTwilightLimitsOverlay(QPainter& painter) const;
	void drawPlaceLabels(QPainter& painter) const;
	QVector<double> currentOverlayLatitudes() const;

	OverlayMode currentOverlayMode = VisibilityOverlay;
	bool   clickToSetMode = false;

	bool   hasDeclination = false;
	double declinationDeg = 0.0;
	int    goodVisibilityDeg = 5;

	bool   hasTwilightObliquity = false;
	double twilightObliquityDeg = 0.0;

	QVector<PlaceLabel> placeLabels;
	bool showPlaceLabels = false;
	int placeLabelMinimumPopulation = 1000000;
	bool placeLabelsNearLinesOnly = true;
};

#endif // OBJECTVISIBILITYMAPWIDGET_HPP
