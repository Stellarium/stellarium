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

#include "EarthShadowMapWidget.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocation.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "planetsephems/sidereal_time.h"

#include <QPainter>
#include <QPaintEvent>
#include <QtMath>
#include <algorithm>

namespace
{
constexpr double sunriseSunsetAltitudeDeg = -0.833;
constexpr double civilTwilightAltitudeDeg = -6.0;
constexpr double nauticalTwilightAltitudeDeg = -12.0;
constexpr double astronomicalTwilightAltitudeDeg = -18.0;
constexpr double mapCenterLongitudeDeg = 0.0;

double normalizeLongitudeDeg(double longitudeDeg)
{
	while (longitudeDeg < -180.0)
		longitudeDeg += 360.0;
	while (longitudeDeg > 180.0)
		longitudeDeg -= 360.0;
	return longitudeDeg;
}

QPointF interpolatedPoint(const QPointF& a, const QPointF& b, double valueA, double valueB)
{
	const double denom = valueA - valueB;
	const double t = qFuzzyIsNull(denom) ? 0.5 : valueA / denom;
	return a + (b - a) * qBound(0.0, t, 1.0);
}
}

EarthShadowMapWidget::EarthShadowMapWidget(QWidget* parent)
	: QWidget(parent)
	, earthMap(QStringLiteral(":/DaylightMap/earth_map.jpg"))
	, core(StelApp::getInstance().getCore())
	, showGrid(false)
	, localJD(core ? core->getJD() : 0.)
	, localJDE(core ? core->getJDE() : 0.)
	, cachedJD(0.)
{
	setAutoFillBackground(false);
	setMinimumSize(640, 320);
}

void EarthShadowMapWidget::syncFromCore()
{
	if (!core) return;
	localJD  = core->getJD();
	localJDE = core->getJDE();
	cachedJD = -1.;
	update();
}

void EarthShadowMapWidget::syncToCore()
{
	if (!core) return;
	core->setJD(localJD);
}

void EarthShadowMapWidget::addDays(double days)
{
	if (!core) return;
	localJD += days;
	localJDE = localJD + core->computeDeltaT(localJD) / 86400.0;
	cachedJD = -1.;
	update();
}

void EarthShadowMapWidget::updateFromCore()
{
	syncFromCore();
}

void EarthShadowMapWidget::setFlagShowGrid(bool show)
{
	if (showGrid == show)
		return;

	showGrid = show;
	update();
}






void EarthShadowMapWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(rect(), QColor(5, 8, 15));

	QRectF available = rect().adjusted(10, 10, -10, -10);
	const double targetRatio = 2.0;
	double mapWidth = available.width();
	double mapHeight = mapWidth / targetRatio;
	if (mapHeight > available.height())
	{
		mapHeight = available.height();
		mapWidth = mapHeight * targetRatio;
	}
	const QRect mapPixelRect(qRound(available.center().x() - mapWidth / 2.0),
	                         qRound(available.center().y() - mapHeight / 2.0),
	                         qMax(1, qRound(mapWidth)),
	                         qMax(1, qRound(mapHeight)));
	const QRectF mapRect(mapPixelRect);
	const QSize mapSize = mapPixelRect.size();
	rebuildSceneCache(mapSize);

	painter.setPen(QPen(QColor(120, 135, 150), 1));
	painter.setBrush(QColor(18, 26, 38));
	painter.drawRect(mapRect);

	if (!baseMapCache.isNull())
	{
		painter.drawPixmap(mapPixelRect, baseMapCache);
	}
	else
	{
		painter.fillRect(mapRect, QColor(26, 55, 80));
		painter.setPen(QColor(170, 185, 195));
		painter.drawText(mapRect, Qt::AlignCenter, q_("Earth map texture not found"));
	}

	if (cachedSunPoint.valid)
	{
		if (!twilightOverlayCache.isNull())
			painter.drawImage(mapPixelRect, twilightOverlayCache);
		if (!contourCache.isNull())
			painter.drawPixmap(mapPixelRect, contourCache);
		if (showGrid)
			drawGeographicGrid(painter, mapRect);
		drawBodyMarker(painter, mapRect, cachedSunPoint, QColor(255, 218, 72), true);
	}
	else if (showGrid)
	{
		drawGeographicGrid(painter, mapRect);
	}

	if (cachedMoonPoint.valid)
		drawBodyMarker(painter, mapRect, cachedMoonPoint, QColor(220, 225, 230), false);

	const BodyPoint locationPoint = computeCurrentLocationPoint();
	if (locationPoint.valid)
		drawLocationMarker(painter, mapRect, locationPoint);

}

EarthShadowMapWidget::BodyPoint EarthShadowMapWidget::computeSubPoint(bool sun) const
{
	BodyPoint result;
	if (!core)
		return result;

	SolarSystem* solarSystem = GETSTELMODULE(SolarSystem);
	if (!solarSystem)
		return result;

	const PlanetP body = sun ? solarSystem->getSun() : solarSystem->getMoon();
	const PlanetP earth = solarSystem->getEarth();
	if (body.isNull() || earth.isNull())
		return result;

	const Vec3d geocentricVsop87 = body->getHeliocentricEclipticPos() - earth->getHeliocentricEclipticPos();
	const Vec3d geocentricJ2000 = StelCore::matVsop87ToJ2000.multiplyWithoutTranslation(geocentricVsop87);
	const Vec3d equinoxEqu = core->j2000ToEquinoxEqu(geocentricJ2000, StelCore::RefractionOff);

	double ra = 0.;
	double dec = 0.;
	StelUtils::rectToSphe(&ra, &dec, equinoxEqu);

	const double gastDeg = get_apparent_sidereal_time(localJD, localJDE);
	result.longitudeDeg = normalizeLongitudeDeg(ra * M_180_PI - gastDeg);
	result.latitudeDeg = dec * M_180_PI;
	result.valid = true;
	return result;
}

EarthShadowMapWidget::BodyPoint EarthShadowMapWidget::computeCurrentLocationPoint() const
{
	BodyPoint result;
	if (!core)
		return result;

	const StelLocation& location = core->getCurrentLocation();
	if (location.planetName != "Earth")
		return result;

	result.longitudeDeg = static_cast<double>(location.getLongitude());
	result.latitudeDeg = static_cast<double>(location.getLatitude());
	result.valid = true;
	return result;
}

double EarthShadowMapWidget::solarAltitudeDeg(double latitudeDeg, double longitudeDeg, const BodyPoint& sun) const
{
	const double lat = latitudeDeg * M_PI / 180.0;
	const double sunLat = sun.latitudeDeg * M_PI / 180.0;
	const double hourAngle = normalizeLongitudeDeg(longitudeDeg - sun.longitudeDeg) * M_PI / 180.0;
	const double sinAlt = qSin(lat) * qSin(sunLat) +
	                      qCos(lat) * qCos(sunLat) * qCos(hourAngle);
	return qAsin(qBound(-1.0, sinAlt, 1.0)) * M_180_PI;
}

double EarthShadowMapWidget::axialTiltDeg() const
{
	SolarSystem* solarSystem = GETSTELMODULE(SolarSystem);
	if (!core || !solarSystem || solarSystem->getEarth().isNull())
		return 23.4392911;

	return solarSystem->getEarth()->getRotObliquity(localJDE) * M_180_PI;
}

QPointF EarthShadowMapWidget::lonLatToPoint(const QRectF& mapRect, double longitudeDeg, double latitudeDeg) const
{
	const double normLon = normalizeLongitudeDeg(longitudeDeg);
	return QPointF(mapRect.left() + (normLon + 180.0) / 360.0 * mapRect.width(),
	               mapRect.top() + (90.0 - qBound(-90.0, latitudeDeg, 90.0)) / 180.0 * mapRect.height());
}



void EarthShadowMapWidget::rebuildSceneCache(const QSize& mapSize)
{
	if (cachedMapSize != mapSize || baseMapCache.isNull())
		rebuildBaseMapCache(mapSize);

	const double jd = localJD;
	const bool timeChanged = qAbs(jd - cachedJD) > 1.0e-9;
	const QSize overlaySize = mapSize;

	if (!timeChanged && cachedOverlaySize == overlaySize
	        && !twilightOverlayCache.isNull() && !contourCache.isNull())
		return;

	cachedJD          = jd;
	cachedOverlaySize = overlaySize;
	cachedSunPoint    = computeSubPoint(true);
	cachedMoonPoint   = computeSubPoint(false);

	if (cachedSunPoint.valid)
	{
		rebuildTwilightOverlayCache(overlaySize, cachedSunPoint);
		rebuildContourCache(overlaySize, cachedSunPoint);
	}
	else
	{
		twilightOverlayCache = QImage();
		contourCache = QPixmap();
	}
}

void EarthShadowMapWidget::rebuildBaseMapCache(const QSize& mapSize)
{
	cachedMapSize = mapSize;
	baseMapCache = QPixmap(mapSize);
	baseMapCache.fill(Qt::transparent);

	QPainter painter(&baseMapCache);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	if (!earthMap.isNull())
		painter.drawPixmap(QRect(QPoint(0, 0), mapSize),
		                   earthMap.scaled(mapSize, Qt::IgnoreAspectRatio,
		                                   Qt::SmoothTransformation));
}

void EarthShadowMapWidget::rebuildTwilightOverlayCache(const QSize& cacheSize, const BodyPoint& sun)
{
	// Use Format_ARGB32 (straight alpha), NOT Format_ARGB32_Premultiplied.
	// With premultiplied format, qRgba() straight values are misinterpreted
	// by Qt's compositor, causing colour shifts (e.g. civil twilight appears red).
	twilightOverlayCache = QImage(cacheSize, QImage::Format_ARGB32);
	twilightOverlayCache.fill(Qt::transparent);

	for (int y = 0; y < cacheSize.height(); ++y)
	{
		QRgb* line = reinterpret_cast<QRgb*>(twilightOverlayCache.scanLine(y));
		const double lat = 90.0 - (static_cast<double>(y) + 0.5) * 180.0 / cacheSize.height();
		for (int x = 0; x < cacheSize.width(); ++x)
		{
			const double lon = -180.0 + (static_cast<double>(x) + 0.5) * 360.0 / cacheSize.width();
			const double alt = solarAltitudeDeg(lat, lon, sun);

			// Grey shading — four zones of increasing darkness toward night.
			// Straight ARGB: grey with varying alpha.
			if (alt < astronomicalTwilightAltitudeDeg)
				line[x] = qRgba(0, 0, 0, 200);    // full night
			else if (alt < nauticalTwilightAltitudeDeg)
				line[x] = qRgba(0, 0, 0, 150);    // astronomical twilight
			else if (alt < civilTwilightAltitudeDeg)
				line[x] = qRgba(0, 0, 0, 100);    // nautical twilight
			else if (alt < sunriseSunsetAltitudeDeg)
				line[x] = qRgba(0, 0, 0, 50);     // civil twilight
			// else: day — transparent (already set by fill)
		}
	}
}

void EarthShadowMapWidget::rebuildContourCache(const QSize& cacheSize, const BodyPoint& sun)
{
	contourCache = QPixmap(cacheSize);
	contourCache.fill(Qt::transparent);

	QPainter painter(&contourCache);
	painter.setRenderHint(QPainter::Antialiasing, true);
	const QRectF cacheRect(0., 0., cacheSize.width(), cacheSize.height());
	drawAltitudeContour(painter, cacheRect, sun, astronomicalTwilightAltitudeDeg,
	                     QPen(QColor( 30,  90, 220, 210), 1.0));
	drawAltitudeContour(painter, cacheRect, sun, nauticalTwilightAltitudeDeg,
	                     QPen(QColor( 80, 150, 255, 210), 1.0));
	drawAltitudeContour(painter, cacheRect, sun, civilTwilightAltitudeDeg,
	                     QPen(QColor(140, 200, 255, 210), 1.0));
	drawAltitudeContour(painter, cacheRect, sun, sunriseSunsetAltitudeDeg,
	                     QPen(QColor(255, 220, 120, 230), 1.2));
}

void EarthShadowMapWidget::drawAltitudeContour(QPainter& painter, const QRectF& mapRect, const BodyPoint& sun,
                                               double altitudeDeg, const QPen& pen) const
{
	painter.save();
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);

	const int columns = qMax(80, qRound(mapRect.width() / 4.0));
	const int rows = qMax(40, qRound(mapRect.height() / 4.0));

	auto valueAt = [&](int ix, int iy) {
		const double lon = -180.0 + static_cast<double>(ix) * 360.0 / columns;
		const double lat = 90.0 - static_cast<double>(iy) * 180.0 / rows;
		return solarAltitudeDeg(lat, lon, sun) - altitudeDeg;
	};

	auto pointAt = [&](int ix, int iy) {
		return QPointF(mapRect.left() + static_cast<double>(ix) * mapRect.width() / columns,
		               mapRect.top() + static_cast<double>(iy) * mapRect.height() / rows);
	};

	for (int y = 0; y < rows; ++y)
	{
		for (int x = 0; x < columns; ++x)
		{
			const double v0 = valueAt(x, y);
			const double v1 = valueAt(x + 1, y);
			const double v2 = valueAt(x + 1, y + 1);
			const double v3 = valueAt(x, y + 1);

			const QPointF p0 = pointAt(x, y);
			const QPointF p1 = pointAt(x + 1, y);
			const QPointF p2 = pointAt(x + 1, y + 1);
			const QPointF p3 = pointAt(x, y + 1);

			QVector<QPointF> crossings;
			if ((v0 <= 0.0 && v1 > 0.0) || (v0 > 0.0 && v1 <= 0.0))
				crossings << interpolatedPoint(p0, p1, v0, v1);
			if ((v1 <= 0.0 && v2 > 0.0) || (v1 > 0.0 && v2 <= 0.0))
				crossings << interpolatedPoint(p1, p2, v1, v2);
			if ((v2 <= 0.0 && v3 > 0.0) || (v2 > 0.0 && v3 <= 0.0))
				crossings << interpolatedPoint(p2, p3, v2, v3);
			if ((v3 <= 0.0 && v0 > 0.0) || (v3 > 0.0 && v0 <= 0.0))
				crossings << interpolatedPoint(p3, p0, v3, v0);

			if (crossings.size() == 2)
				painter.drawLine(crossings.at(0), crossings.at(1));
			else if (crossings.size() == 4)
			{
				painter.drawLine(crossings.at(0), crossings.at(1));
				painter.drawLine(crossings.at(2), crossings.at(3));
			}
		}
	}

	painter.restore();
}

void EarthShadowMapWidget::drawGeographicGrid(QPainter& painter, const QRectF& mapRect) const
{
	painter.save();
	painter.setClipRect(mapRect);
	painter.setRenderHint(QPainter::Antialiasing, false);

	const QPen ordinaryPen(QColor(255, 255, 255, 75), 0.8);
	const QPen equatorPen(QColor(255, 255, 255, 125), 1.2);
	const QPen specialPen(QColor(255, 230, 170, 150), 1.3, Qt::DashLine);

	for (int lat = -60; lat <= 60; lat += 30)
		drawLatitudeLine(painter, mapRect, lat, lat == 0 ? equatorPen : ordinaryPen);

	painter.setPen(ordinaryPen);
	for (int lon = -180; lon <= 180; lon += 30)
	{
		const QPointF north = lonLatToPoint(mapRect, lon, 90.0);
		const QPointF south = lonLatToPoint(mapRect, lon, -90.0);
		painter.drawLine(north, south);
	}

	const double obliquity = axialTiltDeg();
	const double polarCircle = 90.0 - obliquity;
	drawLatitudeLine(painter, mapRect, obliquity, specialPen);
	drawLatitudeLine(painter, mapRect, -obliquity, specialPen);
	drawLatitudeLine(painter, mapRect, polarCircle, specialPen);
	drawLatitudeLine(painter, mapRect, -polarCircle, specialPen);

	painter.restore();
}

void EarthShadowMapWidget::drawLatitudeLine(QPainter& painter, const QRectF& mapRect,
                                            double latitudeDeg, const QPen& pen) const
{
	painter.setPen(pen);
	const QPointF west = lonLatToPoint(mapRect, mapCenterLongitudeDeg - 180.0, latitudeDeg);
	const QPointF east = lonLatToPoint(mapRect, mapCenterLongitudeDeg + 180.0, latitudeDeg);
	painter.drawLine(west, east);
}

void EarthShadowMapWidget::drawBodyMarker(QPainter& painter, const QRectF& mapRect, const BodyPoint& point,
                                          const QColor& color, bool sun) const
{
	painter.save();
	painter.setRenderHint(QPainter::Antialiasing);

	const QPointF center = lonLatToPoint(mapRect, point.longitudeDeg, point.latitudeDeg);
	const QVector<double> xOffsets = {0.0, -mapRect.width(), mapRect.width()};
	for (double xOffset : xOffsets)
	{
		const QPointF c(center.x() + xOffset, center.y());
		if (c.x() < mapRect.left() - 14.0 || c.x() > mapRect.right() + 14.0)
			continue;

		painter.setPen(QPen(QColor(0, 0, 0, 190), 3.0));
		painter.setBrush(color);
		painter.drawEllipse(c, 6.0, 6.0);

		if (sun)
		{
			painter.setPen(QPen(color.lighter(135), 1.6));
			for (int i = 0; i < 8; ++i)
			{
				const double a = i * M_PI / 4.0;
				painter.drawLine(c + QPointF(qCos(a) * 9.0, qSin(a) * 9.0),
				                 c + QPointF(qCos(a) * 13.0, qSin(a) * 13.0));
			}
		}
		else
		{
			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor(45, 50, 62));
			painter.drawEllipse(c + QPointF(3.0, -1.0), 5.2, 5.2);
		}
	}

	painter.restore();
}

void EarthShadowMapWidget::drawLocationMarker(QPainter& painter, const QRectF& mapRect, const BodyPoint& point) const
{
	painter.save();
	painter.setRenderHint(QPainter::Antialiasing);

	const QPointF center = lonLatToPoint(mapRect, point.longitudeDeg, point.latitudeDeg);
	const QVector<double> xOffsets = {0.0, -mapRect.width(), mapRect.width()};
	for (double xOffset : xOffsets)
	{
		const QPointF c(center.x() + xOffset, center.y());
		if (c.x() < mapRect.left() - 12.0 || c.x() > mapRect.right() + 12.0)
			continue;

		painter.setPen(QPen(QColor(0, 0, 0, 210), 3.0));
		painter.setBrush(Qt::NoBrush);
		painter.drawEllipse(c, 5.5, 5.5);
		painter.drawLine(QPointF(c.x() - 8.5, c.y()), QPointF(c.x() + 8.5, c.y()));
		painter.drawLine(QPointF(c.x(), c.y() - 8.5), QPointF(c.x(), c.y() + 8.5));

		painter.setPen(QPen(QColor(255, 255, 255, 230), 1.4));
		painter.drawEllipse(c, 5.5, 5.5);
		painter.drawLine(QPointF(c.x() - 8.5, c.y()), QPointF(c.x() + 8.5, c.y()));
		painter.drawLine(QPointF(c.x(), c.y() - 8.5), QPointF(c.x(), c.y() + 8.5));
	}

	painter.restore();
}









