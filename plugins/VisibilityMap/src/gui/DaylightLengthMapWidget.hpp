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

#ifndef DAYLIGHT_LENGTH_MAP_WIDGET_HPP
#define DAYLIGHT_LENGTH_MAP_WIDGET_HPP

#include <QDate>
#include <QPixmap>
#include <QPoint>
#include <QSize>

#include <QWidget>

class StelCore;

//! Displays a world map with isochrones of equal daylight duration (or equal
//! duration of a given twilight phase — civil, nautical, astronomical) for a
//! chosen date.
//!
//! The date is set independently of the Stellarium JD via setDate() /
//! syncDateFromCore(), so the planetarium view is never disturbed.
//!
//! Solar declination is obtained from Stellarium's Solar System module
//! (DE441 ephemeris) at solar noon on the selected date, making the widget
//! accurate across the full ±15,000-year window Stellarium supports.  The
//! hour angle H at which the Sun reaches threshold altitude α is:
//!
//!   cos H = (sin α − sin φ · sin δ) / (cos φ · cos δ)
//!
//! where φ = observer latitude, δ = solar declination.
//!
//! Isochrones are purely horizontal (daylight duration depends only on
//! latitude, not longitude) and are drawn as a family of horizontal lines
//! across the map at adaptive intervals that depend on zoom level.
//!
//! The threshold altitude defaults to −0.833° (standard sunrise/sunset,
//! accounting for refraction + solar semi-diameter).  Presets for civil
//! (−6°), nautical (−12°), and astronomical (−18°) twilight are provided.
//!
//! Cities are drawn as an optional overlay using the same population-filtered
//! StelLocationMgr logic as SunriseSunsetMapWidget::drawLocationLabels().
class DaylightLengthMapWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DaylightLengthMapWidget(QWidget* parent = Q_NULLPTR);

	QDate currentDate() const { return QDate(year, month, day); }

	//! Altitude threshold presets (degrees).
	//! The numeric values are used internally; the user sees the label strings.
	enum AltitudePreset
	{
		PresetSunrise      =  0,   //!< −0.833° (refraction + semi-diameter)
		PresetCivil        = -6,
		PresetNautical     = -12,
		PresetAstronomical = -18
	};

signals:
	//! Emitted after the date changes. Carries year/month/day as separate
	//! integers so BCE dates (negative year) are represented correctly
	//! — QDate cannot represent negative years in Qt6.
	void dateChanged(int year, int month, int day);

public slots:
	//! Set the date to display.  Does NOT change the Stellarium JD.
	void setDate(QDate date);
	//! Set date by individual components — works for any year including BCE.
	void setDate(int year, int month, int day);

	//! Copy today's date from StelCore (UTC + observer offset).  Does NOT
	//! push anything back to the planetarium.
	void syncDateFromCore();

	//! Set the solar altitude threshold.
	//! @param preset  One of the AltitudePreset enum values.
	void setAltitudeThreshold(int preset);

	void setFlagShowCities(bool show);
	void setFlagShowGrid(bool show);
	void resetView();
	void zoomToCurrentLocation();

	void stepDays(int days);    //!< Move date forward/backward by N days.
	void stepMonths(int months);//!< Move date forward/backward by N months.

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	// ── Solar geometry ───────────────────────────────────────────────────────

	//! Solar declination (degrees) for the currently set date, derived from
	//! Stellarium's Solar System ephemeris (DE441-compatible).  Accurate
	//! across the full ±15,000-year window Stellarium supports.  Falls back
	//! to Spencer's Fourier series only if the Solar System module is
	//! unavailable (should never happen in normal use).
	double solarDeclinationDeg() const;

	//! Hours of daylight at the given latitude (degrees) for the current
	//! solar declination and altitude threshold.
	//! Returns  24.0 for polar day, 0.0 for polar night.
	double daylightHours(double latitudeDeg) const;

	// ── Coordinate helpers ───────────────────────────────────────────────────
	QPointF lonLatToPoint(double longitudeDeg, double latitudeDeg,
	                      const QRectF& mapRect) const;
	void screenToLonLat(const QPointF& point, const QRectF& mapRect,
	                    double& longitudeDeg, double& latitudeDeg) const;

	// ── Drawing ──────────────────────────────────────────────────────────────
	void drawBaseMap(QPainter& painter, const QRectF& mapRect);
	void drawGeographicGrid(QPainter& painter, const QRectF& mapRect) const;
	void drawLatitudeLine(QPainter& painter, const QRectF& mapRect,
	                      double latitudeDeg, const QPen& pen) const;
	void drawIsochrones(QPainter& painter, const QRectF& mapRect) const;
	void drawCities(QPainter& painter, const QRectF& mapRect) const;
	void drawCurrentLocation(QPainter& painter, const QRectF& mapRect) const;
	void drawPolarAnnotations(QPainter& painter, const QRectF& mapRect) const;
	void drawLegend(QPainter& painter, const QRectF& mapRect) const;

	//! Draw a single isochrone (horizontal line) for the given daylight hours
	//! value, with edge labels.
	void drawIsochrone(QPainter& painter, const QRectF& mapRect,
	                   double latitudeDeg, double hours,
	                   const QPen& pen, bool label) const;

	//! Draw a small label on the left or right edge of the map.
	void drawEdgeLabel(QPainter& painter, const QPointF& point,
	                   const QRectF& mapRect, const QString& text) const;

	// ── Cache ────────────────────────────────────────────────────────────────
	void rebuildSceneCache(const QRectF& mapRect);
	void invalidateSceneCache();
	void constrainView();

	// ── Helpers ──────────────────────────────────────────────────────────────
	QString hoursToString(double hours) const;
	QString thresholdLabel() const;
	double effectiveThresholdDeg() const;  //!< actual degrees used in formula
	double axialTiltDeg() const;

	QPixmap earthMap;    //!< Bundled raster base map (:./DaylightMap/earth_map.png).
	StelCore*    core;

	// Date
	int year;
	int month;
	int day;
	int dayOfYear;  //!< cached day-of-year for the current date

	// Solar geometry cache (recomputed whenever date or threshold changes)
	double declinationDeg;     //!< solar declination for the current date
	double thresholdDeg;       //!< effective altitude threshold in degrees
	int    altitudePreset;     //!< AltitudePreset value

	// View
	bool   showGrid;
	bool   showCities;
	double centerLongitudeDeg;
	double centerLatitudeDeg;
	double longitudeSpanDeg;
	bool   dragging;
	QPoint lastMousePos;

	// Scene cache
	QPixmap sceneCache;
	QSize   sceneCacheSize;
	bool    sceneCacheDirty;
};

#endif /* DAYLIGHT_LENGTH_MAP_WIDGET_HPP */
