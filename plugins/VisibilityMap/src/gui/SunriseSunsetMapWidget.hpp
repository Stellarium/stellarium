/*
 * Daylight Map plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef SUNRISE_SUNSET_MAP_WIDGET_HPP
#define SUNRISE_SUNSET_MAP_WIDGET_HPP

#include <QPoint>
#include <QPixmap>
#include <QSize>
#include <QSvgRenderer>
#include <QWidget>

class StelCore;

class SunriseSunsetMapWidget : public QWidget
{
	Q_OBJECT

public:
	enum BodyMode
	{
		Sun = 0,
		Moon
	};
	Q_ENUM(BodyMode)

	enum EventMode
	{
		Sunrise = 0,
		Sunset,
		// Solar twilight thresholds — Sun only (meaningless for Moon)
		CivilDawn,
		CivilDusk,
		NauticalDawn,
		NauticalDusk,
		AstronomicalDawn,
		AstronomicalDusk
	};
	Q_ENUM(EventMode)

	explicit SunriseSunsetMapWidget(QWidget* parent = Q_NULLPTR);

public slots:
	void updateFromCore();
	//! Copy the current Stellarium JD into the widget's local clock.
	void syncFromCore();
	//! Push the widget's local JD back into Stellarium.
	void syncToCore();
	void setBodyMode(int mode);
	void setEventMode(int mode);
	void setFlagShowGrid(bool show);
	void setFlagShowCities(bool show);
	void resetView();
	void zoomToCurrentLocation();
	void addDays(int days);
	void addMonths(int months);

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	struct Sample
	{
		double value = 0.;
		bool valid = false;
	};

	struct BodySample
	{
		double minutes = 0.;
		double rightAscensionRad = 0.;
		double declinationRad = 0.;
		double siderealRad = 0.;
		double horizonAltitudeRad = 0.;
	};

	double localEventMinutes(double latitudeDeg, double longitudeDeg, const QVector<BodySample>& bodySamples) const;
	double localSampledEventMinutes(double latitudeDeg, double longitudeDeg, const QVector<BodySample>& bodySamples) const;
	double sampledBodyAltitudeMinusHorizon(double latitudeRad, double longitudeRad, const BodySample& sample) const;
	QVector<BodySample> buildBodySamples() const;
	double axialTiltDeg() const;

	//! Returns the altitude threshold (radians) for the current event mode.
	//! For Sunrise/Sunset this is 0 (crossing is relative to horizonAltitudeRad).
	//! For twilight modes it is the twilight depression angle in radians.
	double eventThresholdAltitudeRad() const;

	//! True when the current event mode represents a rising event (dawn/rise).
	bool isRisingEvent() const;
	QString minutesToString(double minutes) const;
	QString titleText() const;
	QPointF lonLatToPoint(double longitudeDeg, double latitudeDeg, const QRectF& mapRect) const;
	void screenToLonLat(const QPointF& point, const QRectF& mapRect, double& longitudeDeg, double& latitudeDeg) const;
	void drawBaseMap(QPainter& painter, const QRectF& mapRect);
	void drawIsolines(QPainter& painter, const QRectF& mapRect);
	void drawPolarShading(QPainter& painter, const QRectF& mapRect);
	void drawContour(QPainter& painter, const QVector<QVector<Sample> >& grid, const QRectF& mapRect,
	                 double level, const QPen& pen, bool labelEdges);
	void drawEdgeLabel(QPainter& painter, const QPointF& point, const QRectF& mapRect, const QString& text);
	void rebuildSceneCache(const QRectF& mapRect);
	void invalidateSceneCache();
	void drawCurrentLocation(QPainter& painter, const QRectF& mapRect) const;
	void drawLocationLabels(QPainter& painter, const QRectF& mapRect) const;
	void drawGeographicGrid(QPainter& painter, const QRectF& mapRect) const;
	void drawLatitudeLine(QPainter& painter, const QRectF& mapRect, double latitudeDeg,
	                      const QPen& pen) const;
	void drawLongitudeLine(QPainter& painter, const QRectF& mapRect, double longitudeDeg,
	                       const QPen& pen) const;
	void updateDateCache();
	void constrainView();

	QPixmap earthMap;    //!< Bundled raster base map (:/DaylightMap/earth_map.png).
	StelCore* core;

	BodyMode bodyMode;
	EventMode eventMode;
	bool showGrid;
	bool showCities;
	double centerLongitudeDeg;
	double centerLatitudeDeg;
	double longitudeSpanDeg;
	bool dragging;
	QPoint lastMousePos;
	QPixmap sceneCache;
	QSize sceneCacheSize;
	bool sceneCacheDirty;

	double localJD;        //!< Widget's own clock — decoupled from the planetarium.
	double currentJD;      //!< Alias for localJD used internally (kept for compatibility).
	double utcOffsetHours;
	int year;
	int month;
	int day;
	int dayOfYear;
};

#endif /* SUNRISE_SUNSET_MAP_WIDGET_HPP */
