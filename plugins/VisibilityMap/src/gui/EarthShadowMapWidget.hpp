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

#ifndef EARTH_SHADOW_MAP_WIDGET_HPP
#define EARTH_SHADOW_MAP_WIDGET_HPP

#include <QImage>
#include <QPixmap>
#include <QSize>
#include <QWidget>

class StelCore;

class EarthShadowMapWidget : public QWidget
{
	Q_OBJECT

public:
	explicit EarthShadowMapWidget(QWidget* parent = Q_NULLPTR);

public slots:
	void updateFromCore();
	void syncFromCore();
	void syncToCore();
	void addDays(double days);
	void setFlagShowGrid(bool show);
	void setFlagSetLocationOnClick(bool enable) { flagSetLocationOnClick = enable; }

protected:
	void paintEvent(QPaintEvent* event) override;
	void changeEvent(QEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	struct BodyPoint
	{
		double longitudeDeg = 0.;
		double latitudeDeg = 0.;
		bool valid = false;
	};

	QRectF mapRect() const;
	QPointF lonLatToPoint(const QRectF& mapRect, double longitudeDeg, double latitudeDeg) const;
	void pointToLonLat(const QPointF& point, const QRectF& mapRect,
	                   double& longitudeDeg, double& latitudeDeg) const;

	BodyPoint computeSubPoint(bool sun) const;
	BodyPoint computeCurrentLocationPoint() const;
	double solarAltitudeDeg(double latitudeDeg, double longitudeDeg, const BodyPoint& sun) const;
	double axialTiltDeg() const;
	void rebuildSceneCache(const QSize& mapSize);
	void rebuildBaseMapCache(const QSize& mapSize);
	void rebuildTwilightOverlayCache(const QSize& cacheSize, const BodyPoint& sun);
	void rebuildContourCache(const QSize& cacheSize, const BodyPoint& sun);
	void drawAltitudeContour(QPainter& painter, const QRectF& mapRect, const BodyPoint& sun,
	                         double altitudeDeg, const QPen& pen) const;
	void drawBodyMarker(QPainter& painter, const QRectF& mapRect, const BodyPoint& point,
	                    const QColor& color, bool sun) const;
	void drawLocationMarker(QPainter& painter, const QRectF& mapRect, const BodyPoint& point) const;
	void drawGeographicGrid(QPainter& painter, const QRectF& mapRect) const;
	void drawLatitudeLine(QPainter& painter, const QRectF& mapRect, double latitudeDeg,
	                      const QPen& pen) const;

	QPixmap earthMap;
	StelCore* core;
	bool showGrid;
	bool flagSetLocationOnClick;
	QPoint pressPos;
	double localJD;
	double localJDE;

	QPixmap baseMapCache;
	QImage twilightOverlayCache;
	QPixmap contourCache;
	QSize cachedMapSize;
	QSize cachedOverlaySize;
	double cachedJD;
	BodyPoint cachedSunPoint;
	BodyPoint cachedMoonPoint;
};

#endif /* EARTH_SHADOW_MAP_WIDGET_HPP */
