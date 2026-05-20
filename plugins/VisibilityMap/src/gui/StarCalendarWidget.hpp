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

/*
 * StarCalendarWidget — "Best visibility" calendar diagram.
 *
 * For a given star (identified by its declination δ and right ascension α),
 * this widget plots a 2-D diagram with:
 *
 *   X-axis : day of year  (1 – 365)
 *   Y-axis : observer latitude  (−90° … +90°)
 *
 * Each pixel is coloured according to the Sun's altitude at the moment the
 * star transits the meridian at that latitude and on that day.  The same four
 * twilight-zone colours used in EarthShadowMapWidget are applied:
 *
 *   Night            (Sun < −18°)  deep navy
 *   Astronomical tw. (−18° … −12°) dark blue
 *   Nautical tw.     (−12° … −6°)  medium blue
 *   Civil tw.        (−6° … −0.83°) pale blue
 *   Day              (Sun > −0.83°) transparent / white
 *
 * Three horizontal contour lines mark the twilight thresholds at the current
 * observer latitude so the user can read off their own "window" directly.
 * A right-hand Y-axis shows the star's altitude at culmination for reference.
 *
 * The astronomical formula is:
 *
 *   1. Day d, year y → JD → solar longitude via the standard solar series
 *      (geometric mean longitude + equation of centre, truncated at T³).
 *      Accurate to ~1° across the full DE441 window (−15000 to +17000 CE).
 *      This avoids the century-scale drift of purely day-of-year formulas,
 *      making ancient-epoch diagrams (e.g. Sirius in Ancient Egypt) reliable.
 *
 *   2. The star transits when LST = α★.  At that moment the Sun's hour angle
 *      is  H☉ = α★ − α☉  (both in degrees).
 *
 *   3. Sun altitude at transit:
 *        alt☉ = arcsin( sin φ · sin δ☉  +  cos φ · cos δ☉ · cos H☉ )
 *
 * No planetary ephemeris is called; the calculation is a tight analytic loop
 * and renders a 365 × 180 pixel image in a few milliseconds.
 */

#ifndef STAR_CALENDAR_WIDGET_HPP
#define STAR_CALENDAR_WIDGET_HPP

#include <QImage>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QWidget>

class StelCore;

class StarCalendarWidget : public QWidget
{
	Q_OBJECT

public:
	explicit StarCalendarWidget(QWidget* parent = Q_NULLPTR);

	//! Set the star parameters and trigger a full redraw.
	//! @param decDeg   Declination in degrees (epoch-of-date).
	//! @param raDeg    Right ascension in degrees (epoch-of-date).
	//! @param name     Localised object name for the title.
	//! @param year     Year at calculation time (for X-axis label).
	void setObject(double decDeg, double raDeg,
	               const QString& name, int year);

	//! Show the "no object" placeholder.
	void clearObject();

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	// ── astronomical helpers ────────────────────────────────────────────────

	//! Low-precision solar longitude (degrees) for day-of-year d in year y,
	//! with optional sub-day fractional offset for smooth interpolation.
	static double solarLongitudeDeg(int dayOfYear, int year, double fracDay = 0.0);

	//! Solar declination (degrees) from solar longitude (degrees).
	static double solarDeclinationDeg(double solarLonDeg);

	//! Solar right ascension (degrees) from solar longitude (degrees).
	static double solarRaDeg(double solarLonDeg);

	//! Sun altitude (degrees) when the star transits at observer latitude φ
	//! on the given (fractional) day of year.
	double sunAltAtStarTransit(double latDeg, double dayOfYear) const;

	// ── drawing ─────────────────────────────────────────────────────────────
	void rebuildCache();
	void drawAxes(QPainter& painter, const QRectF& plotRect) const;
	void drawContourLines(QPainter& painter, const QRectF& plotRect) const;
	void drawObserverLine(QPainter& painter, const QRectF& plotRect) const;
	void drawCaption(QPainter& painter, const QRectF& plotRect) const;

	//! Map a (dayOfYear, latitudeDeg) pair to a pixel position in plotRect.
	QPointF toPixel(const QRectF& plotRect, double dayOfYear, double latitudeDeg) const;

	// ── state ────────────────────────────────────────────────────────────────
	StelCore* core;

	bool    hasObject;
	double  objectDecDeg;
	double  objectRaDeg;
	QString objectName;
	int     objectYear;

	// Cached background image (twilight shading), rebuilt on resize or new object.
	QImage  bgCache;
	QSize   bgCacheSize;
	bool    cacheDirty;
};

#endif /* STAR_CALENDAR_WIDGET_HPP */
