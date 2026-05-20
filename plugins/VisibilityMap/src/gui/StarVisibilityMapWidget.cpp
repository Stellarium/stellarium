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
 * The concept for the "Object visibility" map was inspired by the article:
 *
 *   "The 49 Brightest Stars in the Night Sky — When and Where Can We See Them?"
 *   https://astro-geo-gis.com/the-49-brightest-stars-in-the-night-sky-when-and-where-can-we-see-them/
 *
 * The article visualises, for each bright star, the latitude band from which
 * the star is visible at some point during the year, and the approximate
 * dates when it is best placed for observation.  This widget generalises
 * that idea to any object selectable in Stellarium, computing five
 * characteristic latitude lines for the current simulation epoch:
 *
 *   - Southern visibility limit  (object just reaches the horizon at due south)
 *   - Extinction-free southern belt  (object clears ~10° above the horizon)
 *   - Zenith passage latitude  (object passes through the zenith)
 *   - Extinction-free northern belt  (object clears ~10° above the horizon)
 *   - Northern visibility limit  (object just reaches the horizon at due north)
 *
 * Precession is accounted for via getEquinoxEquatorialPos(), so the map
 * is correct for any epoch within the DE441 window.
 */

#include "StarVisibilityMapWidget.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocation.hpp"
#include "StelLocationMgr.hpp"
#include "StelObject.hpp"
#include "StelObjectMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPolygonF>
#include <QWheelEvent>
#include <QtMath>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace
{
constexpr double mapCenterLongitudeDeg = 0.0;

double normalizeLongitudeDeg(double lon)
{
	while (lon < -180.0) lon += 360.0;
	while (lon >  180.0) lon -= 360.0;
	return lon;
}

// Clamp a latitude that may fall outside [-90, 90] – if it does the line is
// off the map (invisible), so we return qQNaN() as a sentinel.
double validLat(double lat)
{
	if (lat < -90.0 || lat > 90.0)
		return qQNaN();
	return lat;
}
} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
StarVisibilityMapWidget::StarVisibilityMapWidget(QWidget* parent)
	: QWidget(parent)
	, earthMap(QStringLiteral(":/DaylightMap/earth_map.jpg"))
	, core(StelApp::getInstance().getCore())
	, showGrid(false)
	, showCities(true)
	, centerLongitudeDeg(mapCenterLongitudeDeg)
	, centerLatitudeDeg(0.)
	, longitudeSpanDeg(360.)
	, dragging(false)
	, sceneCacheDirty(true)
	, pendingObjectAvailable(false)
	, hasCalculation(false)
	, m_objectDecDeg(0.)
	, m_objectRaDeg(0.)
	, calcYear(2000)
	, calcMonth(1)
	, calcDay(1)
{
	setMinimumSize(720, 360);
	setMouseTracking(true);
	// Check whether something is already selected when the widget is created.
	onSelectionChanged();
}

// ---------------------------------------------------------------------------
// Public slots
// ---------------------------------------------------------------------------

//! Called whenever StelObjectMgr::selectedObjectChanged fires.
//! Only updates the button-enable state; never touches the drawn map.
void StarVisibilityMapWidget::onSelectionChanged()
{
	const QList<StelObjectP> selected =
	        StelApp::getInstance().getStelObjectMgr().getSelectedObject();
	const bool nowAvailable = !selected.isEmpty();

	if (nowAvailable != pendingObjectAvailable)
	{
		pendingObjectAvailable = nowAvailable;
		emit selectionStateChanged(pendingObjectAvailable);
	}
}

//! Locks in the current selection and JD, then redraws.
void StarVisibilityMapWidget::calculateVisibility()
{
	if (!core) return;

	const QList<StelObjectP> selected =
	        StelApp::getInstance().getStelObjectMgr().getSelectedObject();
	if (selected.isEmpty()) return;

	const StelObjectP obj = selected.first();

	// Only stars and DSOs have a fixed enough position for the calendar
	// diagram to be meaningful.  Solar system bodies (Planet, Moon, Sun,
	// comets, asteroids — all return "Planet" from getType()) are rejected.
	const QString objType = obj->getType();
	if (objType != QStringLiteral("Star") && objType != QStringLiteral("Nebula"))
	{
		emit solarSystemObjectSelected();
		return;
	}

	// getEquinoxEquatorialPos() applies precession AND proper motion for the
	// current JD, giving true-of-date coordinates. This is what we want for
	// computing visibility from Earth at an arbitrary historical or future epoch.
	const Vec3d posEquinox = obj->getEquinoxEquatorialPos(core);
	double raRad = 0., decRad = 0.;
	StelUtils::rectToSphe(&raRad, &decRad, posEquinox);

	m_objectDecDeg = decRad * M_180_PI;
	m_objectRaDeg  = StelUtils::fmodpos(raRad, 2.0 * M_PI) * M_180_PI;
	objectName   = obj->getNameI18n();
	if (objectName.isEmpty())
		objectName = obj->getID();

	// Date at calculation time
	{
		const double jd = core->getJD();
		const double utcOffset = core->getUTCOffset(jd);
		int hour = 0, minute = 0, second = 0, millis = 0;
		StelUtils::getDateTimeFromJulianDay(jd + utcOffset / 24.0,
		                                    &calcYear, &calcMonth, &calcDay,
		                                    &hour, &minute, &second, &millis);
	}

	hasCalculation = true;
	invalidateSceneCache();
	update();
}

void StarVisibilityMapWidget::setFlagShowGrid(bool show)
{
	if (showGrid == show) return;
	showGrid = show;
	invalidateSceneCache();
	update();
}

void StarVisibilityMapWidget::setFlagShowCities(bool show)
{
	if (showCities == show) return;
	showCities = show;
	invalidateSceneCache();
	update();
}

void StarVisibilityMapWidget::resetView()
{
	centerLongitudeDeg = mapCenterLongitudeDeg;
	centerLatitudeDeg  = 0.;
	longitudeSpanDeg   = 360.;
	invalidateSceneCache();
	update();
}

void StarVisibilityMapWidget::zoomToCurrentLocation()
{
	if (!core || core->getCurrentLocation().planetName != "Earth")
		return;
	centerLongitudeDeg = static_cast<double>(core->getCurrentLocation().getLongitude());
	centerLatitudeDeg  = static_cast<double>(core->getCurrentLocation().getLatitude());
	longitudeSpanDeg   = 70.;
	constrainView();
	invalidateSceneCache();
	update();
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------
void StarVisibilityMapWidget::paintEvent(QPaintEvent* event)
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
	painter.setClipping(false);

	painter.setPen(QPen(Qt::black, 2));
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(mapRect);

	// Caption above the map — inside the map area at the top
	{
		painter.save();
		painter.setClipRect(mapRect);
		QString caption;
		if (hasCalculation)
			caption = q_("Visibility of") + QStringLiteral(" ") + objectName +
			          QStringLiteral("  –  ") +
			          QString::asprintf("%04d-%02d-%02d", calcYear, calcMonth, calcDay) +
			          QStringLiteral("  (dec ") +
			          QString::asprintf("%+.2f°", m_objectDecDeg) +
			          QStringLiteral(")");
		else
			caption = q_("Select an object and click \"Calculate visibility\"");

		const QRectF captionRect(mapRect.left() + 4, mapRect.top() + 4,
		                          mapRect.width() - 8, 22);
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(255, 255, 255, 190));
		painter.drawRect(captionRect);
		painter.setPen(Qt::black);
		painter.setFont(QFont(painter.font().family(), 8, QFont::Bold));
		painter.drawText(captionRect, Qt::AlignVCenter | Qt::AlignHCenter, caption);
		painter.setClipping(false);
		painter.restore();
	}
}

void StarVisibilityMapWidget::mousePressEvent(QMouseEvent* event)
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

void StarVisibilityMapWidget::mouseMoveEvent(QMouseEvent* event)
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

void StarVisibilityMapWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		dragging = false;
		event->accept();
		return;
	}
	QWidget::mouseReleaseEvent(event);
}

void StarVisibilityMapWidget::wheelEvent(QWheelEvent* event)
{
	const QRectF mapRect = rect().adjusted(10, 10, -10, -34);
	double cursorLon = centerLongitudeDeg;
	double cursorLat = centerLatitudeDeg;
	screenToLonLat(event->position(), mapRect, cursorLon, cursorLat);

	const double factor = event->angleDelta().y() > 0 ? 0.78 : 1.28;
	longitudeSpanDeg = qBound(20.0, longitudeSpanDeg * factor, 360.0);

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
QPointF StarVisibilityMapWidget::lonLatToPoint(double longitudeDeg, double latitudeDeg,
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

void StarVisibilityMapWidget::screenToLonLat(const QPointF& point, const QRectF& mapRect,
                                             double& longitudeDeg, double& latitudeDeg) const
{
	const double latSpan = longitudeSpanDeg * mapRect.height() / mapRect.width();
	longitudeDeg = normalizeLongitudeDeg(
	        centerLongitudeDeg + (point.x() - mapRect.center().x()) *
	        longitudeSpanDeg / mapRect.width());
	latitudeDeg  = centerLatitudeDeg -
	               (point.y() - mapRect.center().y()) * latSpan / mapRect.height();
	latitudeDeg  = qBound(-89.0, latitudeDeg, 89.0);
}

// ---------------------------------------------------------------------------
// Cache management
// ---------------------------------------------------------------------------
void StarVisibilityMapWidget::rebuildSceneCache(const QRectF& mapRect)
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
	if (!hasCalculation)
		drawNoObjectMessage(cp, mapRect);
	else
		drawVisibilityLines(cp, mapRect);
	drawLegend(cp, mapRect);
	if (showCities)
		drawLocationLabels(cp, mapRect);
	cp.setClipping(false);

	sceneCacheSize  = size();
	sceneCacheDirty = false;
}

void StarVisibilityMapWidget::invalidateSceneCache()
{
	sceneCacheDirty = true;
}

// ---------------------------------------------------------------------------
// Base map
// ---------------------------------------------------------------------------
void StarVisibilityMapWidget::drawBaseMap(QPainter& painter, const QRectF& mapRect)
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
		                       (lonLeft - centerLongitudeDeg) / longitudeSpanDeg * mapRect.width();
		QRectF dest = destBase;
		dest.moveLeft(xLeft);
		if (!earthMap.isNull())
			painter.drawPixmap(dest.toRect(), earthMap);
	}
}

// ---------------------------------------------------------------------------
// Geographic grid (same style as SunriseSunsetMapWidget)
// ---------------------------------------------------------------------------
void StarVisibilityMapWidget::drawGeographicGrid(QPainter& painter, const QRectF& mapRect) const
{
	painter.save();
	painter.setClipRect(mapRect);

	const QPen ordinaryPen(QColor(0, 0, 0, 70), 0.8);
	const QPen equatorPen (QColor(0, 0, 0, 115), 1.1);

	const int stepDeg = longitudeSpanDeg > 120.0 ? 30 : (longitudeSpanDeg > 50.0 ? 15 : 5);
	const double latSpan = longitudeSpanDeg * mapRect.height() / mapRect.width();
	const double minLat  = qMax(-90.0, centerLatitudeDeg - latSpan / 2.0);
	const double maxLat  = qMin( 90.0, centerLatitudeDeg + latSpan / 2.0);
	const int firstLat   = static_cast<int>(qCeil(minLat / stepDeg)) * stepDeg;
	for (int lat = firstLat; lat <= maxLat; lat += stepDeg)
		drawLatitudeLine(painter, mapRect, lat, lat == 0 ? equatorPen : ordinaryPen);

	const double leftLon  = centerLongitudeDeg - longitudeSpanDeg / 2.0;
	const double rightLon = centerLongitudeDeg + longitudeSpanDeg / 2.0;
	const int firstLon    = static_cast<int>(qCeil(leftLon / stepDeg)) * stepDeg;
	for (int lon = firstLon; lon <= rightLon; lon += stepDeg)
	{
		painter.setPen(ordinaryPen);
		const int segments = qBound(18, qRound(mapRect.height() / 18.0), 120);
		QVector<QPointF> pts;
		pts.reserve(segments + 1);
		for (int i = 0; i <= segments; ++i)
		{
			const double lat2 = maxLat - static_cast<double>(i) * (maxLat - minLat) / segments;
			pts << lonLatToPoint(normalizeLongitudeDeg(lon), lat2, mapRect);
		}
		painter.drawPolyline(pts.constData(), pts.size());
	}

	painter.restore();
}

// ---------------------------------------------------------------------------
// Visibility line drawing
// ---------------------------------------------------------------------------

// Helper: draw a plain horizontal latitude line across the full map.
void StarVisibilityMapWidget::drawLatitudeLine(QPainter& painter, const QRectF& mapRect,
                                               double latitudeDeg, const QPen& pen) const
{
	if (qIsNaN(latitudeDeg)) return;

	painter.setPen(pen);
	const int segments = qBound(24, qRound(mapRect.width() / 24.0), 160);
	QVector<QPointF> pts;
	pts.reserve(segments + 1);
	for (int i = 0; i <= segments; ++i)
	{
		const double lon = centerLongitudeDeg - longitudeSpanDeg / 2.0 +
		                   static_cast<double>(i) * longitudeSpanDeg / segments;
		pts << lonLatToPoint(normalizeLongitudeDeg(lon), latitudeDeg, mapRect);
	}
	painter.drawPolyline(pts.constData(), pts.size());
}

// Helper: draw a line of repeated symbols (plus signs or triangles) at a
// given latitude.  symbolType: SymbolPlus / SymbolTriangleUp / SymbolTriangleDown
void StarVisibilityMapWidget::drawSymbolLine(QPainter& painter, const QRectF& mapRect,
                                             double latitudeDeg, int symbolType) const
{
	if (qIsNaN(latitudeDeg)) return;

	const QPointF refPoint = lonLatToPoint(centerLongitudeDeg, latitudeDeg, mapRect);
	if (refPoint.y() < mapRect.top() - 10 || refPoint.y() > mapRect.bottom() + 10)
		return;

	const double y      = refPoint.y();
	const double startX = mapRect.left() + 4.;
	const double endX   = mapRect.right() - 4.;
	const double step   = 16.0;
	const double sz     = 5.0; // half-size of symbol

	painter.save();
	for (double x = startX; x <= endX; x += step)
	{
		switch (symbolType)
		{
			case SymbolPlus:
				painter.drawLine(QPointF(x - sz, y), QPointF(x + sz, y));
				painter.drawLine(QPointF(x, y - sz), QPointF(x, y + sz));
				break;

			case SymbolTriangleUp:
			{
				QPolygonF tri;
				tri << QPointF(x,       y - sz)
				    << QPointF(x + sz,  y + sz)
				    << QPointF(x - sz,  y + sz);
				painter.drawPolygon(tri);
				break;
			}

			case SymbolTriangleDown:
			{
				QPolygonF tri;
				tri << QPointF(x,       y + sz)
				    << QPointF(x + sz,  y - sz)
				    << QPointF(x - sz,  y - sz);
				painter.drawPolygon(tri);
				break;
			}
		}
	}
	painter.restore();
}

// Main drawing routine: compute the five characteristic latitudes from the
// object's declination and draw each with the appropriate symbology.
//
// All formulas assume a spherical Earth with no atmosphere (refraction at the
// horizon is roughly 0.57°, negligible for a world-map overview).
//
//  dec = object declination in degrees (already epoch-corrected by Stellarium)
//
//  Line 1 – Visibility limit (solid):
//    Northern: lat_N =  90° − |dec|   if dec ≥ 0, else  90° + dec  = 90° − |dec|
//             simplifies to  lat_N =  90° − |dec|
//    Southern: lat_S = -(90° − |dec|) = |dec| − 90°
//
//  Line 2 – Extinction-free (5° above horizon at upper culmination):
//    Upper culmination altitude  = 90° − |lat − dec|
//    Require altitude ≥ 5°  →  |lat − dec| ≤ 85°
//    Northern limit: lat = dec + 85°
//    Southern limit: lat = dec − 85°
//
//  Line 3 – Zenith passage: lat = dec  (object transits overhead)
//
//  Line 4 – Circumpolar north: lat = 90° − dec
//    (object never sets for lat > 90° − dec)
//
//  Line 5 – Circumpolar south: lat = -(90° + dec)
//    (object never rises for lat < -(90° + dec))
void StarVisibilityMapWidget::drawVisibilityLines(QPainter& painter, const QRectF& mapRect) const
{
	const double dec = m_objectDecDeg;

	// ── Line colours and styles (blue-dominant, matching reference charts) ──
	const QColor blue(30, 80, 200);

	// 1. Visibility limit  – solid blue, 2 px
	const QPen solidPen(blue, 2.0);

	// 2. Extinction-free   – dash-dot, 1.5 px
	QPen dashDotPen(blue, 1.5);
	dashDotPen.setStyle(Qt::DashDotLine);

	// 3/4/5. Symbol lines  – same blue, 1.5 px (set before each drawSymbolLine)
	const QPen symbolPen(blue, 1.5);

	// ── Compute latitudes ────────────────────────────────────────────────────
	const double visNorth = validLat( 90.0 - qAbs(dec));   // line 1 north
	const double visSouth = validLat(-90.0 + qAbs(dec));   // line 1 south
	const double extNorth = validLat(dec + 85.0);          // line 2 north
	const double extSouth = validLat(dec - 85.0);          // line 2 south
	const double zenith   = validLat(dec);                 // line 3
	const double circN    = validLat(90.0 - dec);          // line 4
	const double circS    = validLat(-90.0 - dec);         // line 5

	painter.setClipRect(mapRect);

	// ── Draw ─────────────────────────────────────────────────────────────────

	// 1a. Visibility limit north
	drawLatitudeLine(painter, mapRect, visNorth, solidPen);
	// 1b. Visibility limit south
	drawLatitudeLine(painter, mapRect, visSouth, solidPen);

	// 2a. Extinction-free north
	drawLatitudeLine(painter, mapRect, extNorth, dashDotPen);
	// 2b. Extinction-free south
	drawLatitudeLine(painter, mapRect, extSouth, dashDotPen);

	// 3. Zenith passage (plus symbols)
	painter.setPen(symbolPen);
	painter.setBrush(blue);
	drawSymbolLine(painter, mapRect, zenith, SymbolPlus);

	// 4. Circumpolar north (up-triangles)
	drawSymbolLine(painter, mapRect, circN, SymbolTriangleUp);

	// 5. Circumpolar south (down-triangles)
	drawSymbolLine(painter, mapRect, circS, SymbolTriangleDown);

	painter.setBrush(Qt::NoBrush);
	painter.setClipping(false);
}

// ---------------------------------------------------------------------------
// Legend
// ---------------------------------------------------------------------------
void StarVisibilityMapWidget::drawLegend(QPainter& painter, const QRectF& mapRect) const
{
	// Place legend in the bottom-right corner of the map area.
	const QColor blue(30, 80, 200);
	const QColor bg(255, 255, 255, 210);

	struct Entry
	{
		int   symbolType; // -1 = line (solid), -2 = dash-dot, else SymbolType
		bool  dashDot;
		QString label;
	};

	const QVector<Entry> entries = {
	    {-1,  false, q_("Limit of visibility")},
	    {-2,  true,  q_("Extinction free (5° above horizon)")},
	    {SymbolPlus,         false, q_("Passes zenith")},
	    {SymbolTriangleUp,   false, q_("Circumpolar limit (N hemisphere)")},
	    {SymbolTriangleDown, false, q_("Circumpolar limit (S hemisphere)")},
	};

	const int lineLen = 30;
	const int rowH    = 18;
	const int padding = 6;
	const int textOff = lineLen + padding + 4;

	painter.save();
	painter.setFont(QFont(painter.font().family(), 7));
	const QFontMetrics fm(painter.font());

	// Measure widest label
	int maxTextW = 0;
	for (const Entry& e : entries)
		maxTextW = qMax(maxTextW, fm.horizontalAdvance(e.label));

	const int totalW = textOff + maxTextW + padding;
	const int totalH = entries.size() * rowH + padding * 2;

	// Position: bottom-right of map, with small margin
	const QRectF legendRect(mapRect.right() - totalW - 6,
	                         mapRect.bottom() - totalH - 6,
	                         totalW, totalH);
	painter.setPen(QPen(Qt::gray, 0.8));
	painter.setBrush(bg);
	painter.drawRect(legendRect);

	for (int i = 0; i < entries.size(); ++i)
	{
		const Entry& e = entries.at(i);
		const double y = legendRect.top() + padding + i * rowH + rowH / 2.0;
		const double x0 = legendRect.left() + padding;
		const double x1 = x0 + lineLen;

		painter.setPen(QPen(blue, e.symbolType == -2 ? 1.5 : 2.0,
		                    e.symbolType == -2 ? Qt::DashDotLine : Qt::SolidLine));
		painter.setBrush(blue);

		if (e.symbolType == -1 || e.symbolType == -2)
		{
			// Line sample
			painter.drawLine(QPointF(x0, y), QPointF(x1, y));
		}
		else
		{
			// Symbol samples – draw 3 evenly spaced
			painter.setPen(QPen(blue, 1.5));
			const double sz = 4.0;
			for (int k = 0; k < 3; ++k)
			{
				const double sx = x0 + (lineLen * (k * 2 + 1)) / 6.0;
				switch (e.symbolType)
				{
					case SymbolPlus:
						painter.drawLine(QPointF(sx - sz, y), QPointF(sx + sz, y));
						painter.drawLine(QPointF(sx, y - sz), QPointF(sx, y + sz));
						break;
					case SymbolTriangleUp:
					{
						QPolygonF tri;
						tri << QPointF(sx, y - sz) << QPointF(sx + sz, y + sz) << QPointF(sx - sz, y + sz);
						painter.drawPolygon(tri);
						break;
					}
					case SymbolTriangleDown:
					{
						QPolygonF tri;
						tri << QPointF(sx, y + sz) << QPointF(sx + sz, y - sz) << QPointF(sx - sz, y - sz);
						painter.drawPolygon(tri);
						break;
					}
				}
			}
		}

		painter.setPen(Qt::black);
		painter.drawText(QPointF(legendRect.left() + textOff,
		                         y + fm.ascent() / 2.0 - 1),
		                 e.label);
	}

	painter.restore();
}

// ---------------------------------------------------------------------------
// No-object placeholder
// ---------------------------------------------------------------------------
void StarVisibilityMapWidget::drawNoObjectMessage(QPainter& painter, const QRectF& mapRect) const
{
	painter.save();
	painter.setPen(QColor(60, 60, 60));
	painter.drawText(mapRect, Qt::AlignCenter,
	                 q_("Select an object in Stellarium, then click\n\"Calculate visibility\""));
	painter.restore();
}

// ---------------------------------------------------------------------------
// Current location crosshair (same as SunriseSunsetMapWidget)
// ---------------------------------------------------------------------------
void StarVisibilityMapWidget::drawCurrentLocation(QPainter& painter, const QRectF& mapRect) const
{
	if (!core || core->getCurrentLocation().planetName != "Earth")
		return;

	const QPointF p = lonLatToPoint(
	        static_cast<double>(core->getCurrentLocation().getLongitude()),
	        static_cast<double>(core->getCurrentLocation().getLatitude()),
	        mapRect);
	if (!mapRect.adjusted(-10, -10, 10, 10).contains(p))
		return;

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
// Constraint / view helpers
// ---------------------------------------------------------------------------
void StarVisibilityMapWidget::constrainView()
{
	const double latSpan      = longitudeSpanDeg * static_cast<double>(height()) / qMax(1, width());
	const double maxCenterLat = qMax(0.0, 90.0 - latSpan / 2.0);
	centerLatitudeDeg         = qBound(-maxCenterLat, centerLatitudeDeg, maxCenterLat);
	centerLongitudeDeg        = normalizeLongitudeDeg(centerLongitudeDeg);
}

void StarVisibilityMapWidget::drawLocationLabels(QPainter& painter, const QRectF& mapRect) const
{
	if (longitudeSpanDeg > 95.0)
		return;

	const int populationThreshold = longitudeSpanDeg > 55.0 ? 750 : 150;
	LocationList locations = StelApp::getInstance().getLocationMgr().getAll();
	locations.erase(std::remove_if(locations.begin(), locations.end(),
	        [populationThreshold](const StelLocation& loc) {
		        return loc.planetName != "Earth" || loc.population < populationThreshold ||
		               (loc.role != 'C' && loc.role != 'B' && loc.role != 'R' && loc.population < 1000);
	        }), locations.end());
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
		if (!mapRect.adjusted(4, 4, -4, -4).contains(p)) continue;
		const QString name = loc.name;
		QRectF textRect(painter.fontMetrics().boundingRect(name).adjusted(-2, -1, 2, 1));
		textRect.translate(p + QPointF(5., -13.));
		bool overlaps = false;
		for (const QRectF& used : std::as_const(usedRects))
			if (used.intersects(textRect)) { overlaps = true; break; }
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
