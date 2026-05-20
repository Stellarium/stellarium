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

#include "DaylightLengthMapWidget.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocation.hpp"
#include "StelLocationMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "SolarSystem.hpp"
#include "StelModuleMgr.hpp"
#include "Planet.hpp"
#include "VecMath.hpp"

#include <algorithm>
#include <QDate>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QTime>
#include <QWheelEvent>
#include <QtMath>

// ---------------------------------------------------------------------------
// Anonymous helpers
// ---------------------------------------------------------------------------
namespace
{
constexpr double mapCenterLongitudeDeg = 0.0;

// Standard sunrise/sunset threshold: refraction (≈0.57°) + solar semi-diameter (≈0.27°)
constexpr double standardSunriseDeg = -0.8333;

double normalizeLongitudeDeg(double lon)
{
	while (lon < -180.0) lon += 360.0;
	while (lon >  180.0) lon -= 360.0;
	return lon;
}
} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
DaylightLengthMapWidget::DaylightLengthMapWidget(QWidget* parent)
	: QWidget(parent)
	, earthMap(QStringLiteral(":/DaylightMap/earth_map.jpg"))
	, core(StelApp::getInstance().getCore())
	, year(2000)
	, month(1)
	, day(1)
	, dayOfYear(1)
	, declinationDeg(0.)
	, thresholdDeg(standardSunriseDeg)
	, altitudePreset(PresetSunrise)
	, showGrid(false)
	, showCities(true)
	, centerLongitudeDeg(mapCenterLongitudeDeg)
	, centerLatitudeDeg(0.)
	, longitudeSpanDeg(360.)
	, dragging(false)
	, sceneCacheDirty(true)
{
	setMinimumSize(720, 360);
	setMouseTracking(true);
	syncDateFromCore();
}

// ---------------------------------------------------------------------------
// Helper — days in month, works for any year including BCE (year <= 0).
// Uses the proleptic Gregorian calendar as StelUtils does.
// ---------------------------------------------------------------------------
static int daysInMonthForYear(int year, int month)
{
	// Adjust for astronomical year numbering (year 0 = 1 BCE)
	const int y = (year <= 0) ? year + 1 : year;  // convert to proleptic Gregorian
	switch (month)
	{
		case 1: case 3: case 5: case 7: case 8: case 10: case 12: return 31;
		case 4: case 6: case 9: case 11: return 30;
		case 2:
		{
			// Gregorian leap year rule
			const bool leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
			return leap ? 29 : 28;
		}
		default: return 30;
	}
}

// ---------------------------------------------------------------------------
// Public slots
// ---------------------------------------------------------------------------
void DaylightLengthMapWidget::setDate(QDate date)
{
	if (!date.isValid()) return;
	setDate(date.year(), date.month(), date.day());
}

void DaylightLengthMapWidget::setDate(int y, int m, int d)
{
	const int maxDay = daysInMonthForYear(y, m);
	d = qBound(1, d, maxDay);

	if (y == year && m == month && d == day) return;

	year      = y;
	month     = m;
	day       = d;
	// QDate::dayOfYear is unreliable for negative years — compute manually
	int doy = d;
	for (int i = 1; i < m; ++i)
		doy += daysInMonthForYear(y, i);
	dayOfYear = doy;

	declinationDeg = solarDeclinationDeg();
	invalidateSceneCache();
	update();

	emit dateChanged(year, month, day);
}

void DaylightLengthMapWidget::syncDateFromCore()
{
	if (!core) return;

	const double jd = core->getJD();
	const double utcOffset = core->getUTCOffset(jd);
	int hour = 0, minute = 0, second = 0, millis = 0;
	int y = year, m = month, d = day;
	StelUtils::getDateTimeFromJulianDay(jd + utcOffset / 24.0,
	                                    &y, &m, &d,
	                                    &hour, &minute, &second, &millis);
	// Use setDate(int,int,int) which handles BCE years correctly
	setDate(y, m, d);
}

void DaylightLengthMapWidget::setAltitudeThreshold(int preset)
{
	altitudePreset = preset;
	thresholdDeg   = effectiveThresholdDeg();
	invalidateSceneCache();
	update();
}

void DaylightLengthMapWidget::setFlagShowCities(bool show)
{
	if (showCities == show) return;
	showCities = show;
	invalidateSceneCache();
	update();
}

void DaylightLengthMapWidget::setFlagShowGrid(bool show)
{
	if (showGrid == show) return;
	showGrid = show;
	invalidateSceneCache();
	update();
}

void DaylightLengthMapWidget::resetView()
{
	centerLongitudeDeg = mapCenterLongitudeDeg;
	centerLatitudeDeg  = 0.;
	longitudeSpanDeg   = 360.;
	invalidateSceneCache();
	update();
}

void DaylightLengthMapWidget::zoomToCurrentLocation()
{
	if (!core || core->getCurrentLocation().planetName != "Earth") return;
	centerLongitudeDeg = static_cast<double>(core->getCurrentLocation().getLongitude());
	centerLatitudeDeg  = static_cast<double>(core->getCurrentLocation().getLatitude());
	longitudeSpanDeg   = 70.;
	constrainView();
	invalidateSceneCache();
	update();
}

void DaylightLengthMapWidget::stepDays(int days)
{
	// Use JD arithmetic — works for any year including BCE.
	double jd = 0.;
	StelUtils::getJDFromDate(&jd, year, month, day, 12, 0, 0.f);
	jd += days;
	int y = 0, m = 0, d = 0;
	StelUtils::getDateFromJulianDay(jd, &y, &m, &d);
	setDate(y, m, d);
}

void DaylightLengthMapWidget::stepMonths(int months)
{
	int y = year;
	int m = month + months;
	// Normalise month overflow/underflow
	while (m > 12) { m -= 12; ++y; }
	while (m < 1)  { m += 12; --y; }
	const int maxDay = daysInMonthForYear(y, m);
	setDate(y, m, qMin(day, maxDay));
}

// ---------------------------------------------------------------------------
// Solar geometry
// ---------------------------------------------------------------------------

// Spencer's Fourier series was deliberately NOT used here.  It is accurate
// to ±0.3° near 2000 CE but diverges badly for historical or far-future
// dates.  Instead we ask Stellarium's Solar System module for the true
// heliocentric position of the Sun at solar noon on the selected date,
// which is consistent with the DE441 ephemeris across the full ±15,000-year
// window that Stellarium supports.  One ephemeris call per map update is
// sufficient because the Sun's declination changes by less than 0.5° per day
// at the solstices and negligibly during the map's lifetime.
double DaylightLengthMapWidget::solarDeclinationDeg() const
{
	SolarSystem* ss = GETSTELMODULE(SolarSystem);
	if (!core || !ss || ss->getSun().isNull() || ss->getEarth().isNull())
	{
		// Safe analytical fallback (modern dates only) if the Solar System
		// module is somehow unavailable.
		const double B = 2.0 * M_PI * (dayOfYear - 1) / 365.0;
		const double decRad =  0.006918
		                     - 0.399912 * qCos(B)   + 0.070257 * qSin(B)
		                     - 0.006758 * qCos(2*B)  + 0.000907 * qSin(2*B)
		                     - 0.002697 * qCos(3*B)  + 0.001480 * qSin(3*B);
		return decRad * M_180_PI;
	}

	// Sample at UTC solar noon on the selected date.  The exact time of day
	// is unimportant: the Sun's declination varies by < 0.03° over a single
	// day (< 0.5° near the solstices when the rate is highest), which is
	// completely negligible for a world-map overview.
	double jd = 0.;
	StelUtils::getJDFromDate(&jd, year, month, day, 12, 0, 0.f);
	const double jde = jd + core->computeDeltaT(jd) / 86400.0;

	// Geocentric Sun direction in VSOP87 ecliptic frame, then converted to
	// equinox-of-date equatorial coordinates.  We must use equinox-of-date
	// (not J2000) so that declination is correct at the target epoch —
	// for 2000 BCE the precession offset is ~30° which shifts the summer
	// solstice declination significantly.
	//
	// The matrix chain mirrors StelCore::updateTransformMatrices():
	//   matEquinoxEquDateToJ2000 = matVsop87ToJ2000 * earth->getRotEquatorialToVsop87()
	//   matJ2000ToEquinoxEqu     = matEquinoxEquDateToJ2000.transpose()
	// Both rotation matrices are updated as a side effect of calling
	// getHeliocentricEclipticPos(jde).
	const Vec3d earthPos = ss->getEarth()->getHeliocentricEclipticPos(jde);
	const Vec3d sunVsop  = ss->getSun() ->getHeliocentricEclipticPos(jde) - earthPos;

	// computeTransMatrix updates earth->rotLocalToParent for this JDE,
	// which is what getRotEquatorialToVsop87() returns.  Without this call,
	// rotLocalToParent still holds the value from the main simulation loop
	// (i.e. the current core JDE), giving wrong precession for historical dates.
	ss->getEarth()->computeTransMatrix(jd, jde);

	// VSOP87 → equinox-of-date:
	// matEquinoxEquDateToJ2000 = matVsop87ToJ2000 * earth->getRotEquatorialToVsop87()
	// matJ2000ToEquinoxEqu     = matEquinoxEquDateToJ2000.transpose()
	// matHeliocentricEclipticToEquinoxEqu = matJ2000ToEquinoxEqu * matVsop87ToJ2000
	const Mat4d matEquDateToJ2000 = StelCore::matVsop87ToJ2000
	                                * ss->getEarth()->getRotEquatorialToVsop87();
	const Vec3d equOfDate = matEquDateToJ2000.transpose()
	                              .multiplyWithoutTranslation(
	                              StelCore::matVsop87ToJ2000
	                              .multiplyWithoutTranslation(sunVsop));

	double ra = 0., dec = 0.;
	StelUtils::rectToSphe(&ra, &dec, equOfDate);
	return dec * M_180_PI;
}

double DaylightLengthMapWidget::daylightHours(double latitudeDeg) const
{
	const double latRad = latitudeDeg   * M_PI / 180.0;
	const double decRad = declinationDeg * M_PI / 180.0;
	const double thrRad = thresholdDeg   * M_PI / 180.0;

	// cos H = (sin α − sin φ sin δ) / (cos φ cos δ)
	const double cosH = (qSin(thrRad) - qSin(latRad) * qSin(decRad)) /
	                    (qCos(latRad) * qCos(decRad));

	if (cosH <= -1.0) return 24.0;  // polar day
	if (cosH >=  1.0) return  0.0;  // polar night
	return 2.0 * qAcos(cosH) * 12.0 / M_PI;  // hours
}

double DaylightLengthMapWidget::effectiveThresholdDeg() const
{
	switch (altitudePreset)
	{
		case PresetSunrise:      return standardSunriseDeg;
		case PresetCivil:        return -6.0;
		case PresetNautical:     return -12.0;
		case PresetAstronomical: return -18.0;
		default:                 return standardSunriseDeg;
	}
}

double DaylightLengthMapWidget::axialTiltDeg() const
{
	SolarSystem* ss = GETSTELMODULE(SolarSystem);
	if (!core || !ss || ss->getEarth().isNull())
		return 23.4392911;
	return ss->getEarth()->getRotObliquity(core->getJDE()) * M_180_PI;
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------
void DaylightLengthMapWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	const QRectF mapRect = rect().adjusted(10, 10, -10, -34);
	if (sceneCacheDirty || sceneCacheSize != size())
		rebuildSceneCache(mapRect);

	painter.drawPixmap(0, 0, sceneCache);
	painter.setClipRect(mapRect);
	drawCurrentLocation(painter, mapRect);
	drawPolarAnnotations(painter, mapRect);
	painter.setClipping(false);

	painter.setPen(QPen(Qt::black, 2));
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(mapRect);

	// Title below the map border
	const QString title =
	        q_("Daylight length") + QStringLiteral("  –  ") +
	        QString::asprintf("%04d-%02d-%02d", year, month, day) +
	        QStringLiteral("  (") + thresholdLabel() + QStringLiteral(")");
	painter.setPen(Qt::black);
	painter.drawText(QRectF(mapRect.left(), mapRect.bottom() + 5,
	                        mapRect.width(), height() - mapRect.bottom() - 6),
	                 Qt::AlignCenter, title);
}

void DaylightLengthMapWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		dragging     = true;
		lastMousePos = event->pos();
		event->accept();
		return;
	}
	QWidget::mousePressEvent(event);
}

void DaylightLengthMapWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (dragging)
	{
		const QPoint delta = event->pos() - lastMousePos;
		lastMousePos = event->pos();

		const QRectF mapRect = rect().adjusted(10, 10, -10, -34);
		const double latSpan = longitudeSpanDeg * mapRect.height() / mapRect.width();
		centerLongitudeDeg = normalizeLongitudeDeg(
		        centerLongitudeDeg - delta.x() * longitudeSpanDeg / mapRect.width());
		centerLatitudeDeg += delta.y() * latSpan / mapRect.height();
		constrainView();
		invalidateSceneCache();
		update();
		event->accept();
		return;
	}
	QWidget::mouseMoveEvent(event);
}

void DaylightLengthMapWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		dragging = false;
		event->accept();
		return;
	}
	QWidget::mouseReleaseEvent(event);
}

void DaylightLengthMapWidget::wheelEvent(QWheelEvent* event)
{
	const QRectF mapRect = rect().adjusted(10, 10, -10, -34);
	double cursorLon = centerLongitudeDeg;
	double cursorLat = centerLatitudeDeg;
	screenToLonLat(event->position(), mapRect, cursorLon, cursorLat);

	const double factor = event->angleDelta().y() > 0 ? 0.78 : 1.28;
	longitudeSpanDeg = qBound(10.0, longitudeSpanDeg * factor, 360.0);

	const double latSpan = longitudeSpanDeg * mapRect.height() / mapRect.width();
	const double xFrac   = (event->position().x() - mapRect.left()) / mapRect.width() - 0.5;
	const double yFrac   = 0.5 - (event->position().y() - mapRect.top()) / mapRect.height();
	centerLongitudeDeg   = normalizeLongitudeDeg(cursorLon - xFrac * longitudeSpanDeg);
	centerLatitudeDeg    = cursorLat - yFrac * latSpan;
	constrainView();
	invalidateSceneCache();
	update();
	event->accept();
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------
QPointF DaylightLengthMapWidget::lonLatToPoint(double longitudeDeg, double latitudeDeg,
                                               const QRectF& mapRect) const
{
	const double latSpan = longitudeSpanDeg * mapRect.height() / mapRect.width();
	const double x = mapRect.center().x() +
	                 normalizeLongitudeDeg(longitudeDeg - centerLongitudeDeg) /
	                 longitudeSpanDeg * mapRect.width();
	const double y = mapRect.center().y() -
	                 (latitudeDeg - centerLatitudeDeg) / latSpan * mapRect.height();
	return QPointF(x, y);
}

void DaylightLengthMapWidget::screenToLonLat(const QPointF& point, const QRectF& mapRect,
                                             double& longitudeDeg, double& latitudeDeg) const
{
	const double latSpan = longitudeSpanDeg * mapRect.height() / mapRect.width();
	longitudeDeg = normalizeLongitudeDeg(
	        centerLongitudeDeg + (point.x() - mapRect.center().x()) *
	        longitudeSpanDeg / mapRect.width());
	latitudeDeg = centerLatitudeDeg -
	              (point.y() - mapRect.center().y()) * latSpan / mapRect.height();
	latitudeDeg = qBound(-89.0, latitudeDeg, 89.0);
}

// ---------------------------------------------------------------------------
// Cache management
// ---------------------------------------------------------------------------
void DaylightLengthMapWidget::rebuildSceneCache(const QRectF& mapRect)
{
	const double dpr = devicePixelRatioF();
	sceneCache = QPixmap(QSize(qCeil(width() * dpr), qCeil(height() * dpr)));
	sceneCache.setDevicePixelRatio(dpr);
	sceneCache.fill(Qt::transparent);

	QPainter cp(&sceneCache);
	cp.setRenderHint(QPainter::Antialiasing);
	cp.fillRect(rect(), QColor(245, 245, 242));
	cp.setClipRect(mapRect);
	drawBaseMap(cp, mapRect);
	if (showGrid)
		drawGeographicGrid(cp, mapRect);
	drawIsochrones(cp, mapRect);
	if (showCities)
		drawCities(cp, mapRect);
	drawLegend(cp, mapRect);
	cp.setClipping(false);

	sceneCacheSize  = size();
	sceneCacheDirty = false;
}

void DaylightLengthMapWidget::invalidateSceneCache()
{
	sceneCacheDirty = true;
}

// ---------------------------------------------------------------------------
// Base map (identical pattern to SunriseSunsetMapWidget)
// ---------------------------------------------------------------------------
void DaylightLengthMapWidget::drawBaseMap(QPainter& painter, const QRectF& mapRect)
{
	const double latSpan = longitudeSpanDeg * mapRect.height() / mapRect.width();
	const QRectF destBase(
	        0.,
	        mapRect.center().y() - (90.0 - centerLatitudeDeg) / latSpan * mapRect.height(),
	        mapRect.width() * 360.0 / longitudeSpanDeg,
	        mapRect.height() * 180.0 / latSpan);

	for (int wrap = -2; wrap <= 2; ++wrap)
	{
		const double lonLeft = mapCenterLongitudeDeg - 180.0 + wrap * 360.0;
		const double xLeft   = mapRect.center().x() +
		                       (lonLeft - centerLongitudeDeg) /
		                       longitudeSpanDeg * mapRect.width();
		QRectF dest = destBase;
		dest.moveLeft(xLeft);
		if (!earthMap.isNull())
			painter.drawPixmap(dest.toRect(), earthMap);
	}
}

// ---------------------------------------------------------------------------
// Geographic grid (identical to SunriseSunsetMapWidget)
// ---------------------------------------------------------------------------
void DaylightLengthMapWidget::drawGeographicGrid(QPainter& painter, const QRectF& mapRect) const
{
	painter.save();
	painter.setClipRect(mapRect);

	const QPen ordinaryPen(QColor(0, 0, 0, 70), 0.8);
	const QPen equatorPen (QColor(0, 0, 0, 115), 1.1);
	const QPen specialPen (QColor(120, 82, 0, 145), 1.2, Qt::DashLine);
	const int stepDeg = longitudeSpanDeg > 120.0 ? 30 : (longitudeSpanDeg > 50.0 ? 15 : 5);

	const double latSpan = longitudeSpanDeg * mapRect.height() / mapRect.width();
	const double minLat  = qMax(-90.0, centerLatitudeDeg - latSpan / 2.0);
	const double maxLat  = qMin( 90.0, centerLatitudeDeg + latSpan / 2.0);
	const int firstLat   = static_cast<int>(qCeil(minLat / stepDeg)) * stepDeg;
	for (int lat = firstLat; lat <= maxLat; lat += stepDeg)
		drawLatitudeLine(painter, mapRect, lat, lat == 0 ? equatorPen : ordinaryPen);

	// Vertical lines
	const double leftLon  = centerLongitudeDeg - longitudeSpanDeg / 2.0;
	const double rightLon = centerLongitudeDeg + longitudeSpanDeg / 2.0;
	const int firstLon    = static_cast<int>(qCeil(leftLon / stepDeg)) * stepDeg;
	for (int lon = firstLon; lon <= rightLon; lon += stepDeg)
	{
		painter.setPen(ordinaryPen);
		const int segs = qBound(18, qRound(mapRect.height() / 18.0), 120);
		QVector<QPointF> pts;
		pts.reserve(segs + 1);
		for (int i = 0; i <= segs; ++i)
		{
			const double lat2 = maxLat - static_cast<double>(i) * (maxLat - minLat) / segs;
			pts << lonLatToPoint(normalizeLongitudeDeg(lon), lat2, mapRect);
		}
		painter.drawPolyline(pts.constData(), pts.size());
	}

	// Tropics and polar circles
	const double tilt  = axialTiltDeg();
	const double polar = 90.0 - tilt;
	drawLatitudeLine(painter, mapRect,  tilt,  specialPen);
	drawLatitudeLine(painter, mapRect, -tilt,  specialPen);
	drawLatitudeLine(painter, mapRect,  polar, specialPen);
	drawLatitudeLine(painter, mapRect, -polar, specialPen);

	painter.restore();
}

void DaylightLengthMapWidget::drawLatitudeLine(QPainter& painter, const QRectF& mapRect,
                                               double latitudeDeg, const QPen& pen) const
{
	painter.setPen(pen);
	const int segs = qBound(24, qRound(mapRect.width() / 24.0), 160);
	QVector<QPointF> pts;
	pts.reserve(segs + 1);
	for (int i = 0; i <= segs; ++i)
	{
		const double lon = centerLongitudeDeg - longitudeSpanDeg / 2.0 +
		                   static_cast<double>(i) * longitudeSpanDeg / segs;
		pts << lonLatToPoint(normalizeLongitudeDeg(lon), latitudeDeg, mapRect);
	}
	painter.drawPolyline(pts.constData(), pts.size());
}

// ---------------------------------------------------------------------------
// Isochrone drawing
// ---------------------------------------------------------------------------

// Adaptive isochrone step (hours) based on zoom level.
// World view   (span > 160°): 1-hour steps,   labelled every 2 h
// Regional     (span  40–160°): 30-min steps,  labelled every 1 h
// Local        (span < 40°):  15-min steps,    labelled every 30 min
static void isochroneStep(double longitudeSpanDeg,
                           double& stepHours, double& majorStepHours)
{
	if (longitudeSpanDeg > 160.0)
	{
		stepHours      = 1.0;
		majorStepHours = 2.0;
	}
	else if (longitudeSpanDeg > 40.0)
	{
		stepHours      = 0.5;
		majorStepHours = 1.0;
	}
	else
	{
		stepHours      = 0.25;
		majorStepHours = 0.5;
	}
}

void DaylightLengthMapWidget::drawIsochrones(QPainter& painter, const QRectF& mapRect) const
{
	// We need to find the latitude for a given daylight-hours value.
	// Since daylightHours(lat) is monotonically increasing with latitude in
	// each hemisphere (for a given declination sign), we binary-search for
	// the latitude where daylightHours == target.

	const double latSpan = longitudeSpanDeg * mapRect.height() / mapRect.width();
	const double minVisLat = qMax(-90.0, centerLatitudeDeg - latSpan / 2.0);
	const double maxVisLat = qMin( 90.0, centerLatitudeDeg + latSpan / 2.0);

	double stepH = 1.0, majorH = 2.0;
	isochroneStep(longitudeSpanDeg, stepH, majorH);

	// Scan latitudes in fine steps to find where isochrones cross
	// the visible latitude band.  Because daylight hours is a smooth
	// monotonic function of latitude (in each hemisphere), we can
	// find crossings by walking from −90 to +90 and noting sign changes.
	const int latSteps = 1800; // 0.1° resolution
	const double latStep = 180.0 / latSteps;

	// Build a table of (lat, hours) for the visible range + a small margin
	const double marginDeg = 2.0;
	const double scanMin = qMax(-89.9, minVisLat - marginDeg);
	const double scanMax = qMin( 89.9, maxVisLat + marginDeg);

	// For each isochrone level, find the latitude where daylightHours == level
	// by scanning.  Since the function is monotonic, each level crosses at most
	// twice (once in each hemisphere for asymmetric cases around equinoxes).
	// We collect crossing latitudes and draw a horizontal line at each.

	const double minHours = daylightHours(scanMin);
	const double maxHours = daylightHours(scanMax);
	const double loH = qMin(minHours, maxHours);
	const double hiH = qMax(minHours, maxHours);

	const double startLevel = qFloor(loH / stepH) * stepH;
	const double stopLevel  = qCeil (hiH / stepH) * stepH;

	// We also need to handle the global min/max across ALL latitudes, since
	// polar regions may be visible.
	// daylightHours at the poles:
	const double hoursNorthPole = daylightHours( 89.9);
	const double hoursSouthPole = daylightHours(-89.9);
	const double globalMinH = std::min({minHours, maxHours, hoursNorthPole, hoursSouthPole, 0.0});
	const double globalMaxH = std::max({minHours, maxHours, hoursNorthPole, hoursSouthPole, 24.0});
	const double gStart = qFloor(globalMinH / stepH) * stepH;
	const double gStop  = qCeil (globalMaxH / stepH) * stepH;
	Q_UNUSED(startLevel) Q_UNUSED(stopLevel)

	painter.setClipRect(mapRect);

	for (double level = gStart; level <= gStop + 1e-9; level += stepH)
	{
		// Clamp to valid range
		if (level < 0.0 || level > 24.0) continue;

		const bool major = qFuzzyIsNull(std::fmod(qAbs(level), majorH));
		const QPen pen(major ? QColor(0, 0, 128, 220) : QColor(0, 0, 128, 120),
		               major ? 2.0 : 0.9);

		// Scan for latitude crossings of this level.
		// We scan the full sphere and filter to the visible range.
		double prevLat   = -89.9;
		double prevHours = daylightHours(prevLat);

		for (int i = 1; i <= latSteps; ++i)
		{
			const double lat   = -89.9 + i * latStep;
			const double hours = daylightHours(lat);

			// Crossing: linear interpolation between prevLat and lat
			const bool crossing = (prevHours < level && hours >= level) ||
			                      (prevHours > level && hours <= level);
			if (crossing)
			{
				const double denom = hours - prevHours;
				const double t     = qFuzzyIsNull(denom) ? 0.5 : (level - prevHours) / denom;
				const double crossLat = prevLat + t * latStep;

				// Only draw if this latitude is in the visible range
				if (crossLat >= minVisLat - 0.5 && crossLat <= maxVisLat + 0.5)
					drawIsochrone(painter, mapRect, crossLat, level, pen, major);
			}

			prevLat   = lat;
			prevHours = hours;
		}
	}

	painter.setClipping(false);
}

void DaylightLengthMapWidget::drawIsochrone(QPainter& painter, const QRectF& mapRect,
                                            double latitudeDeg, double hours,
                                            const QPen& pen, bool label) const
{
	drawLatitudeLine(painter, mapRect, latitudeDeg, pen);

	if (!label) return;

	// Draw labels on both left and right edges
	const QPointF leftPoint  = lonLatToPoint(centerLongitudeDeg - longitudeSpanDeg / 2.0,
	                                         latitudeDeg, mapRect);
	const QPointF rightPoint = lonLatToPoint(centerLongitudeDeg + longitudeSpanDeg / 2.0,
	                                         latitudeDeg, mapRect);

	if (leftPoint.y() >= mapRect.top() + 2 && leftPoint.y() <= mapRect.bottom() - 2)
		drawEdgeLabel(painter, leftPoint, mapRect, hoursToString(hours));
	if (rightPoint.y() >= mapRect.top() + 2 && rightPoint.y() <= mapRect.bottom() - 2)
		drawEdgeLabel(painter, rightPoint, mapRect, hoursToString(hours));
}

void DaylightLengthMapWidget::drawEdgeLabel(QPainter& painter, const QPointF& point,
                                            const QRectF& mapRect, const QString& text) const
{
	painter.save();
	painter.setFont(QFont(painter.font().family(), 7));
	const QFontMetrics fm(painter.font());
	const int tw = fm.horizontalAdvance(text) + 6;
	const int th = fm.height() + 2;

	QRectF labelRect(point.x() - tw / 2.0, point.y() - th / 2.0, tw, th);

	// Pin to left or right edge
	if (point.x() <= mapRect.left() + 2)
		labelRect.moveLeft(mapRect.left() + 2);
	else if (point.x() >= mapRect.right() - 2)
		labelRect.moveRight(mapRect.right() - 2);

	// Clip to map
	if (labelRect.top() < mapRect.top() || labelRect.bottom() > mapRect.bottom())
	{
		painter.restore();
		return;
	}

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(255, 255, 255, 210));
	painter.drawRect(labelRect);
	painter.setPen(QColor(0, 0, 128));
	painter.drawText(labelRect, Qt::AlignCenter, text);
	painter.restore();
}

// ---------------------------------------------------------------------------
// Polar day / polar night annotations
// ---------------------------------------------------------------------------
void DaylightLengthMapWidget::drawPolarAnnotations(QPainter& painter, const QRectF& mapRect) const
{
	auto findBoundary = [&](double startLat, double endLat, double target) -> double
	{
		double lo = qMin(startLat, endLat);
		double hi = qMax(startLat, endLat);
		for (int i = 0; i < 50; ++i)
		{
			const double mid  = (lo + hi) / 2.0;
			const double midH = daylightHours(mid);
			if ((midH >= target) == (daylightHours(lo) >= target))
				lo = mid;
			else
				hi = mid;
		}
		return (lo + hi) / 2.0;
	};

	painter.save();
	painter.setFont(QFont(painter.font().family(), 8, QFont::Bold));
	painter.setClipRect(mapRect);

	// Fill the band between a pole and its boundary latitude, draw the
	// boundary line, and label the region.
	auto shadeBand = [&](double poleLat, double boundaryLat,
	                     const QColor& fillColor, const QColor& lineColor,
	                     const QString& label)
	{
		const QPointF polePoint     = lonLatToPoint(centerLongitudeDeg, poleLat,     mapRect);
		const QPointF boundaryPoint = lonLatToPoint(centerLongitudeDeg, boundaryLat, mapRect);
		const double yTop    = qMin(polePoint.y(), boundaryPoint.y());
		const double yBottom = qMax(polePoint.y(), boundaryPoint.y());
		QRectF band(mapRect.left(), yTop, mapRect.width(), yBottom - yTop);
		band = band.intersected(mapRect);
		painter.save();
		painter.setOpacity(fillColor.alphaF());
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(fillColor.rgb()));
		painter.drawRect(band);
		painter.restore();
		drawLatitudeLine(painter, mapRect, boundaryLat, QPen(lineColor, 1.5));
		const double labelY = (yTop + yBottom) / 2.0;
		painter.setPen(lineColor);
		painter.drawText(QRectF(mapRect.left(), labelY - 10, mapRect.width(), 20),
		                 Qt::AlignHCenter | Qt::AlignVCenter, label);
	};

	// Northern polar day
	if (daylightHours(89.9) >= 23.99)
		shadeBand(90.0, findBoundary(89.9, 0.0, 23.99),
		          QColor(255, 200,  50,  60), QColor(220, 140, 0, 200), q_("Polar day"));

	// Southern polar day
	if (daylightHours(-89.9) >= 23.99)
		shadeBand(-90.0, findBoundary(-89.9, 0.0, 23.99),
		          QColor(255, 200,  50,  60), QColor(220, 140, 0, 200), q_("Polar day"));

	// Northern polar night
	if (daylightHours(89.9) <= 0.01)
		shadeBand(90.0, findBoundary(89.9, 0.0, 0.01),
		          QColor( 20,  30, 100,  80), QColor(60, 80, 200, 200), q_("Polar night"));

	// Southern polar night
	if (daylightHours(-89.9) <= 0.01)
		shadeBand(-90.0, findBoundary(-89.9, 0.0, 0.01),
		          QColor( 20,  30, 100,  80), QColor(60, 80, 200, 200), q_("Polar night"));

	painter.setClipping(false);
	painter.restore();
}
// ---------------------------------------------------------------------------
// City overlay — identical logic to SunriseSunsetMapWidget::drawLocationLabels
// ---------------------------------------------------------------------------
void DaylightLengthMapWidget::drawCities(QPainter& painter, const QRectF& mapRect) const
{
	if (longitudeSpanDeg > 95.0)
		return;

	const int popThreshold = longitudeSpanDeg > 55.0 ? 750 : 150;
	LocationList locations = StelApp::getInstance().getLocationMgr().getAll();
	locations.erase(
	    std::remove_if(locations.begin(), locations.end(),
	                   [popThreshold](const StelLocation& loc) {
		                   return loc.planetName != "Earth" ||
		                          loc.population < popThreshold ||
		                          (loc.role != 'C' && loc.role != 'B' && loc.role != 'R' &&
		                           loc.population < 1000);
	                   }),
	    locations.end());
	std::sort(locations.begin(), locations.end(),
	          [](const StelLocation& a, const StelLocation& b) {
		          return a.population > b.population;
	          });

	painter.save();
	painter.setFont(QFont(painter.font().family(), longitudeSpanDeg < 45.0 ? 8 : 7));
	QVector<QRectF> usedRects;
	int drawn = 0;
	const int maxLabels = longitudeSpanDeg < 45.0 ? 35 : 20;

	for (const StelLocation& loc : std::as_const(locations))
	{
		if (drawn >= maxLabels) break;

		const QPointF p = lonLatToPoint(static_cast<double>(loc.getLongitude()),
		                                static_cast<double>(loc.getLatitude()),
		                                mapRect);
		if (!mapRect.adjusted(4, 4, -4, -4).contains(p))
			continue;

		const QString name = loc.name;
		QRectF textRect(painter.fontMetrics().boundingRect(name).adjusted(-2, -1, 2, 1));
		textRect.translate(p + QPointF(5., -13.));
		bool overlaps = false;
		for (const QRectF& used : std::as_const(usedRects))
		{
			if (used.intersects(textRect)) { overlaps = true; break; }
		}
		if (overlaps || !mapRect.contains(textRect)) continue;

		painter.setPen(QPen(Qt::white, 3.0));
		painter.drawPoint(p);
		painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, name);
		painter.setPen(QPen(QColor(170, 0, 0), 1.2));
		painter.drawPoint(p);
		painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, name);
		usedRects << textRect.adjusted(-3, -2, 3, 2);
		++drawn;
	}
	painter.restore();
}

// ---------------------------------------------------------------------------
// Current location crosshair
// ---------------------------------------------------------------------------
void DaylightLengthMapWidget::drawCurrentLocation(QPainter& painter, const QRectF& mapRect) const
{
	if (!core || core->getCurrentLocation().planetName != "Earth") return;

	const QPointF p = lonLatToPoint(
	        static_cast<double>(core->getCurrentLocation().getLongitude()),
	        static_cast<double>(core->getCurrentLocation().getLatitude()),
	        mapRect);
	if (!mapRect.adjusted(-10, -10, 10, 10).contains(p)) return;

	painter.save();
	painter.setPen(QPen(Qt::white, 4.0));
	painter.drawEllipse(p, 5.5, 5.5);
	painter.drawLine(QPointF(p.x() - 8., p.y()), QPointF(p.x() + 8., p.y()));
	painter.drawLine(QPointF(p.x(), p.y() - 8.), QPointF(p.x(), p.y() + 8.));
	painter.setPen(QPen(Qt::black, 1.5));
	painter.drawEllipse(p, 5.5, 5.5);
	painter.drawLine(QPointF(p.x() - 8., p.y()), QPointF(p.x() + 8., p.y()));
	painter.drawLine(QPointF(p.x(), p.y() - 8.), QPointF(p.x(), p.y() + 8.));
	painter.restore();
}

// ---------------------------------------------------------------------------
// Legend (bottom-right corner)
// ---------------------------------------------------------------------------
void DaylightLengthMapWidget::drawLegend(QPainter& painter, const QRectF& mapRect) const
{
	const QColor bgColor(255, 255, 255, 210);
	struct Entry { QColor color; double width; Qt::PenStyle style; QString label; };
	const QVector<Entry> entries = {
	    { QColor(0,   0, 128, 220), 2.0, Qt::SolidLine, q_("Major isochrone (2 h / 1 h / 30 min)") },
	    { QColor(0,   0, 128, 120), 0.9, Qt::SolidLine, q_("Minor isochrone")                       },
	    { QColor(200, 100,  0, 180), 1.5, Qt::DotLine,  q_("Polar day boundary")                    },
	    { QColor(0,   0,  160, 180), 1.5, Qt::DotLine,  q_("Polar night boundary")                  },
	};

	const int lineLen = 28;
	const int rowH    = 17;
	const int padding = 5;

	painter.save();
	painter.setFont(QFont(painter.font().family(), 7));
	const QFontMetrics fm(painter.font());
	int maxW = 0;
	for (const Entry& e : entries)
		maxW = qMax(maxW, fm.horizontalAdvance(e.label));

	const int totalW = padding + lineLen + padding + maxW + padding;
	const int totalH = static_cast<int>(entries.size()) * rowH + padding * 2;
	const QRectF legendRect(mapRect.right() - totalW - 6,
	                         mapRect.bottom() - totalH - 6,
	                         totalW, totalH);

	painter.setPen(QPen(Qt::gray, 0.8));
	painter.setBrush(bgColor);
	painter.drawRect(legendRect);

	for (int i = 0; i < entries.size(); ++i)
	{
		const Entry& e = entries.at(i);
		const double y  = legendRect.top() + padding + i * rowH + rowH / 2.0;
		const double x0 = legendRect.left() + padding;
		const double x1 = x0 + lineLen;
		painter.setPen(QPen(e.color, e.width, e.style));
		painter.drawLine(QPointF(x0, y), QPointF(x1, y));
		painter.setPen(Qt::black);
		painter.drawText(QPointF(x1 + padding, y + fm.ascent() / 2.0 - 1), e.label);
	}
	painter.restore();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
QString DaylightLengthMapWidget::hoursToString(double hours) const
{
	// Format as H:MM
	const int totalMin = qRound(hours * 60.0);
	return QString::asprintf("%d:%02d", totalMin / 60, totalMin % 60);
}

QString DaylightLengthMapWidget::thresholdLabel() const
{
	switch (altitudePreset)
	{
		case PresetSunrise:      return q_("sunrise/sunset");
		case PresetCivil:        return q_("civil twilight");
		case PresetNautical:     return q_("nautical twilight");
		case PresetAstronomical: return q_("astronomical twilight");
		default: return QString();
	}
}

void DaylightLengthMapWidget::constrainView()
{
	const double latSpan      = longitudeSpanDeg * static_cast<double>(height()) / qMax(1, width());
	const double maxCenterLat = qMax(0.0, 90.0 - latSpan / 2.0);
	centerLatitudeDeg         = qBound(-maxCenterLat, centerLatitudeDeg, maxCenterLat);
	centerLongitudeDeg        = normalizeLongitudeDeg(centerLongitudeDeg);
}
