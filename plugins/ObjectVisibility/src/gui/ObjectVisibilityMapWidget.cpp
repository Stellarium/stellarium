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

#include "ObjectVisibilityMapWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QRectF>
#include <QSizeF>
#include <cmath>
#include <algorithm>
#include <utility>

namespace
{
double normalizeLongitudeDeg(double longitude);
double longitudeDifferenceDeg(double a, double b);
}

ObjectVisibilityMapWidget::ObjectVisibilityMapWidget(QWidget* parent)
	: MapWidget(parent)
{
	// We show the observer-location marker so the user can always
	// tell where they currently are on Earth.  The dialog updates
	// its position whenever the observer moves.
	setMarkerVisible(true);

	// Listen to the base class's clicks so we can decide whether to
	// re-emit them upwards.
	connect(this, &MapWidget::positionChanged,
	        this, &ObjectVisibilityMapWidget::onPositionChanged);
}

void ObjectVisibilityMapWidget::setOverlayMode(OverlayMode mode)
{
	if (currentOverlayMode == mode) return;
	currentOverlayMode = mode;
	update();
}

void ObjectVisibilityMapWidget::setPlaceLabels(const QVector<PlaceLabel>& labels)
{
	placeLabels = labels;
	update();
}

void ObjectVisibilityMapWidget::setPlaceLabelsVisible(bool visible)
{
	if (showPlaceLabels == visible) return;
	showPlaceLabels = visible;
	update();
}

void ObjectVisibilityMapWidget::setPlaceLabelMinimumPopulation(int population)
{
	if (population < 0) population = 0;
	if (placeLabelMinimumPopulation == population) return;
	placeLabelMinimumPopulation = population;
	update();
}

void ObjectVisibilityMapWidget::setPlaceLabelsNearLinesOnly(bool nearLinesOnly)
{
	if (placeLabelsNearLinesOnly == nearLinesOnly) return;
	placeLabelsNearLinesOnly = nearLinesOnly;
	update();
}

void ObjectVisibilityMapWidget::setClickSetsLocationMode(bool on)
{
	if (clickToSetMode == on) return;
	clickToSetMode = on;
	setCursor(on ? Qt::CrossCursor : Qt::ArrowCursor);
}

void ObjectVisibilityMapWidget::setDeclination(double declinationDeg_)
{
	if (declinationDeg_ < -90.0 || declinationDeg_ > 90.0)
	{
		clearVisibility();
		return;
	}
	hasDeclination  = true;
	declinationDeg  = declinationDeg_;
	update();
}

void ObjectVisibilityMapWidget::setGoodVisibilityAltitude(int degrees)
{
	if (degrees < 1)  degrees = 1;
	if (degrees > 89) degrees = 89;
	if (goodVisibilityDeg == degrees) return;
	goodVisibilityDeg = degrees;
	if (hasDeclination)
		update();
}

void ObjectVisibilityMapWidget::clearVisibility()
{
	if (!hasDeclination) return;
	hasDeclination = false;
	update();
}

void ObjectVisibilityMapWidget::setTwilightObliquity(double obliquityDeg)
{
	if (obliquityDeg <= 0.0 || obliquityDeg >= 90.0)
	{
		clearTwilightLimits();
		return;
	}
	hasTwilightObliquity = true;
	twilightObliquityDeg = obliquityDeg;
	update();
}

void ObjectVisibilityMapWidget::clearTwilightLimits()
{
	if (!hasTwilightObliquity) return;
	hasTwilightObliquity = false;
	update();
}

void ObjectVisibilityMapWidget::setTwilightMapData(double sunLongitudeDeg,
                                                   double sunLatitudeDeg,
                                                   double moonLongitudeDeg,
                                                   double moonLatitudeDeg)
{
	sunLongitudeDeg = normalizeLongitudeDeg(sunLongitudeDeg);
	moonLongitudeDeg = normalizeLongitudeDeg(moonLongitudeDeg);

	const bool unchanged = hasTwilightMap &&
		longitudeDifferenceDeg(twilightSunLongitudeDeg, sunLongitudeDeg) < 0.001 &&
		std::abs(twilightSunLatitudeDeg - sunLatitudeDeg) < 0.001 &&
		longitudeDifferenceDeg(twilightMoonLongitudeDeg, moonLongitudeDeg) < 0.002 &&
		std::abs(twilightMoonLatitudeDeg - moonLatitudeDeg) < 0.002;

	if (unchanged)
		return;

	hasTwilightMap = true;
	twilightSunLongitudeDeg = sunLongitudeDeg;
	twilightSunLatitudeDeg = sunLatitudeDeg;
	twilightMoonLongitudeDeg = moonLongitudeDeg;
	twilightMoonLatitudeDeg = moonLatitudeDeg;
	invalidateTwilightOverlayCache();
	update();
}

void ObjectVisibilityMapWidget::setTwilightMapFullTwilight(bool enabled)
{
	if (twilightMapFullTwilight == enabled) return;
	twilightMapFullTwilight = enabled;
	if (hasTwilightMap)
	{
		invalidateTwilightOverlayCache();
		update();
	}
}

void ObjectVisibilityMapWidget::clearTwilightMap()
{
	if (!hasTwilightMap) return;
	hasTwilightMap = false;
	invalidateTwilightOverlayCache();
	update();
}

void ObjectVisibilityMapWidget::onPositionChanged(double longitude,
                                                  double latitude,
                                                  const QColor& color)
{
	if (!clickToSetMode) return;
	emit locationPicked(longitude, latitude, color);
	// We deliberately stay in click-to-set mode so the user can pick
	// multiple locations in a row.  The dialog's checkbox is the
	// canonical control; we only react to its state.
}

//
// =================== Drawing ====================
//

namespace
{
// Colour matching the screenshots in the article: solid blue line, a
// slightly lighter dashed line, blue plus marks and triangle markers.
const QColor LINE_COLOR     = QColor(0,  60, 220, 255);
const QColor DASHED_COLOR   = QColor(0,  60, 220, 220);
const QColor ZENITH_COLOR   = QColor(0,  60, 220, 240);
const QColor TRIANGLE_COLOR = QColor(0,  60, 220, 255);

const QColor EQUATOR_COLOR  = QColor(235, 235, 235, 245);
const QColor TROPIC_COLOR   = QColor(220, 155, 30, 245);
const QColor POLAR_COLOR    = QColor(0, 120, 145, 245);

const QColor CIVIL_MIN_COLOR = QColor(245, 110, 45, 245);
const QColor NAUT_MIN_COLOR  = QColor(160, 90, 210, 245);
const QColor ASTRO_MIN_COLOR = QColor(60, 105, 220, 245);

const QColor CIVIL_MAX_COLOR = QColor(205, 60, 60, 245);
const QColor NAUT_MAX_COLOR  = QColor(120, 65, 175, 245);
const QColor ASTRO_MAX_COLOR = QColor(35, 70, 160, 245);

const QColor TWILIGHT_SUNSET_COLOR = QColor(255, 205, 90, 245);
const QColor TWILIGHT_CIVIL_COLOR  = QColor(110, 175, 255, 235);
const QColor TWILIGHT_NAUT_COLOR   = QColor(65, 125, 230, 235);
const QColor TWILIGHT_ASTRO_COLOR  = QColor(55, 80, 175, 235);

const QColor TWILIGHT_CIVIL_SHADE = QColor(95, 155, 255, 58);
const QColor TWILIGHT_NAUT_SHADE  = QColor(55, 105, 225, 88);
const QColor TWILIGHT_ASTRO_SHADE = QColor(30, 60, 165, 118);
const QColor EARTH_NIGHT_SHADE    = QColor(5, 18, 58, 148);
const QColor OTHER_NIGHT_SHADE    = QColor(0, 0, 0, 128);

constexpr double DEG_TO_RAD = M_PI / 180.0;
constexpr double RAD_TO_DEG = 180.0 / M_PI;
constexpr double EARTH_APPARENT_SUNRISE_ALTITUDE_DEG = -50.0 / 60.0;

double clampUnit(double value)
{
	return std::max(-1.0, std::min(1.0, value));
}

double normalizeLongitudeDeg(double longitude)
{
	double result = std::fmod(longitude + 180.0, 360.0);
	if (result < 0.0)
		result += 360.0;
	return result - 180.0;
}

double longitudeDifferenceDeg(double a, double b)
{
	return std::abs(normalizeLongitudeDeg(a - b));
}

} // namespace

void ObjectVisibilityMapWidget::drawLatitudeLine(QPainter& painter,
                                                 double latitudeDeg,
                                                 const QPen& pen) const
{
	if (latitudeDeg < -90.0 || latitudeDeg > 90.0) return;

	// Use the inherited helper from MapWidget to convert lat/lon to
	// pixel space.  The helper returns coordinates in DEVICE pixels.
	const auto p = lonLatToMapPoint(0.0, latitudeDeg);
	const double ratio = devicePixelRatioF();
	// MapWidget::paintEvent applies painter.scale(1/ratio, 1/ratio)
	// and then works in device pixels.  We do the same so our
	// coordinate space matches the base class exactly.
	const double y = p.y;

	painter.setPen(pen);
	// Draw across the entire widget width (in device pixels) so the
	// line still appears continuous over map wrap-arounds.
	painter.drawLine(QPointF(0.0,                       y),
	                 QPointF(width() * ratio,           y));
}

void ObjectVisibilityMapWidget::drawLatitudeLineCopies(QPainter& painter,
                                                       double latitudeDeg,
                                                       const QPen& pen) const
{
	drawLatitudeLine(painter, latitudeDeg, pen);
}

void ObjectVisibilityMapWidget::drawLatitudeMarkers(QPainter& painter,
                                                    double latitudeDeg,
                                                    const QChar& marker,
                                                    const QColor& color) const
{
	if (latitudeDeg < -90.0 || latitudeDeg > 90.0) return;

	const auto p = lonLatToMapPoint(0.0, latitudeDeg);
	const double ratio = devicePixelRatioF();
	const double y = p.y;

	// Pick a font size that scales with the widget height (and HiDPI).
	// 12-14 pt at standard DPI looks good; we target ~14 logical pixels.
	QFont font = painter.font();
	const int pixelSize = static_cast<int>(std::round(14.0 * ratio));
	font.setPixelSize(std::max(8, pixelSize));
	font.setBold(true);
	painter.setFont(font);
	painter.setPen(color);

	const QFontMetrics fm(font);
	const QString s(marker);
	const int advance =
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		fm.horizontalAdvance(s);
#else
		fm.width(s);
#endif
	// Spacing between marks: roughly 1.6x the advance so they "kiss"
	// without overlapping.  Matches the article's screenshot pretty
	// well across a wide range of zoom levels.
	const double step = std::max<double>(advance * 1.6,
	                                     12.0 * ratio);
	const double totalWidth = width() * ratio;

	// We draw the marker glyph centred on the latitude line.  Qt's
	// text baseline is offset from the y coordinate by the font
	// ascent, so we add ascent/2 to roughly centre the glyph.
	const double yText = y + fm.ascent() * 0.35;

	for (double x = step * 0.5; x < totalWidth; x += step)
	{
		painter.drawText(QPointF(x - advance * 0.5, yText), s);
	}
}

void ObjectVisibilityMapWidget::drawVisibilityOverlay(QPainter& painter) const
{
	const double dec = declinationDeg;
	const double h   = static_cast<double>(goodVisibilityDeg);
	const double ratio = devicePixelRatioF();

	// Pen widths scale with HiDPI so lines look the same physical
	// thickness on any monitor.
	const double penW = std::max(1.0, 1.5 * ratio);

	//
	// (1) Limit of visibility (solid).  phi = dec +/- 90.
	//
	{
		QPen pen(LINE_COLOR);
		pen.setWidthF(penW);
		pen.setCapStyle(Qt::FlatCap);
		// One of phi_north / phi_south will be off-map; we just try
		// both and the helper silently does nothing for the invalid
		// one.
		drawLatitudeLine(painter, dec + 90.0, pen);
		drawLatitudeLine(painter, dec - 90.0, pen);
	}

	//
	// (2) Good-visibility (extinction-free) limit (dashed).
	//     phi = dec +/- (90 - h).
	//
	{
		QPen pen(DASHED_COLOR);
		pen.setWidthF(penW);
		// A pattern that scales with line width so the dashes look
		// right at any DPI.  Numbers are in units of pen widths.
		pen.setStyle(Qt::CustomDashLine);
		pen.setDashPattern({6.0, 4.0});
		pen.setCapStyle(Qt::FlatCap);
		drawLatitudeLine(painter, dec + (90.0 - h), pen);
		drawLatitudeLine(painter, dec - (90.0 - h), pen);
	}

	//
	// (3) Passes zenith (+ marks).  phi = dec.
	//
	drawLatitudeMarkers(painter, dec, QChar('+'), ZENITH_COLOR);

	//
	// (4) Circumpolar limit, northern hemisphere (filled up-triangle).
	//     phi = 90 - dec.   Only meaningful when 0 <= phi <= 90.
	//
	drawLatitudeMarkers(painter, 90.0 - dec,
	                    QChar(0x25B2),  // BLACK UP-POINTING TRIANGLE
	                    TRIANGLE_COLOR);

	//
	// (5) Circumpolar limit, southern hemisphere (filled down-triangle).
	//     phi = -90 - dec.  Only meaningful when -90 <= phi <= 0.
	//
	drawLatitudeMarkers(painter, -90.0 - dec,
	                    QChar(0x25BC),  // BLACK DOWN-POINTING TRIANGLE
	                    TRIANGLE_COLOR);
}

void ObjectVisibilityMapWidget::drawTwilightLimitsOverlay(QPainter& painter) const
{
	const double ratio = devicePixelRatioF();
	const double penW = std::max(1.0, 1.35 * ratio);
	const double eps = twilightObliquityDeg;
	const double polar = 90.0 - eps;

	auto penFor = [penW](const QColor& color,
	                     Qt::PenStyle style = Qt::SolidLine,
	                     double width = 0.0)
	{
		QPen pen(color);
		pen.setWidthF(width > 0.0 ? width : penW);
		pen.setCapStyle(Qt::FlatCap);
		pen.setStyle(style);
		if (style == Qt::CustomDashLine)
			pen.setDashPattern({6.0, 4.0});
		return pen;
	};

	auto drawSymmetric = [this, &painter](double latitude, const QPen& pen)
	{
		drawLatitudeLineCopies(painter,  latitude, pen);
		if (std::abs(latitude) > 1e-9)
			drawLatitudeLineCopies(painter, -latitude, pen);
	};

	drawLatitudeLineCopies(painter, 0.0, penFor(EQUATOR_COLOR,
	                                            Qt::SolidLine,
	                                            std::max(1.0, 1.8 * ratio)));
	drawSymmetric(eps, penFor(TROPIC_COLOR));
	drawSymmetric(polar, penFor(POLAR_COLOR,
	                            Qt::DotLine,
	                            std::max(1.0, 1.9 * ratio)));

	// At solstice the Sun's declination is +/-eps.  For the
	// summer-solstice midnight Sun, h_min = phi + eps - 90, giving
	// phi = 90 - eps - |h|.  For the winter-solstice noon Sun,
	// h_max = 90 - phi - eps, giving phi = 90 - eps + |h|.
	drawSymmetric(polar - 6.0,  penFor(CIVIL_MIN_COLOR));
	drawSymmetric(polar - 12.0, penFor(NAUT_MIN_COLOR));
	drawSymmetric(polar - 18.0, penFor(ASTRO_MIN_COLOR));

	drawSymmetric(polar + 6.0,  penFor(CIVIL_MAX_COLOR, Qt::CustomDashLine));
	drawSymmetric(polar + 12.0, penFor(NAUT_MAX_COLOR, Qt::CustomDashLine));
	drawSymmetric(polar + 18.0, penFor(ASTRO_MAX_COLOR, Qt::CustomDashLine));
}

QVector<QPointF> ObjectVisibilityMapWidget::twilightSmallCirclePoints(
	double centerLongitudeDeg,
	double centerLatitudeDeg,
	double angularDistanceDeg,
	double xShift) const
{
	const auto leftEdge = lonLatToMapPoint(-180.0, 0.0);
	const auto rightEdge = lonLatToMapPoint(180.0, 0.0);
	const double mapWidth = rightEdge.x - leftEdge.x;
	if (mapWidth <= 0.0) return {};

	const double centerLatRad = centerLatitudeDeg * DEG_TO_RAD;
	const double centerLonRad = centerLongitudeDeg * DEG_TO_RAD;
	const double sinCenterLat = std::sin(centerLatRad);
	const double cosCenterLat = std::cos(centerLatRad);
	const double angularDistance = angularDistanceDeg * DEG_TO_RAD;
	const double sinDistance = std::sin(angularDistance);
	const double cosDistance = std::cos(angularDistance);

	QVector<QPointF> points;
	const int contourSteps = static_cast<int>(std::clamp(
		std::ceil(mapWidth / 4.0), 180.0, 720.0));
	points.reserve(contourSteps + 1);
	double previousX = 0.0;
	bool hasPrevious = false;

	for (int i = 0; i <= contourSteps; ++i)
	{
		const double bearing = static_cast<double>(i) *
		                       360.0 / static_cast<double>(contourSteps) *
		                       DEG_TO_RAD;
		const double latRad = std::asin(clampUnit(
			sinCenterLat * cosDistance +
			cosCenterLat * sinDistance * std::cos(bearing)));
		const double lonRad = centerLonRad + std::atan2(
			std::sin(bearing) * sinDistance * cosCenterLat,
			cosDistance - sinCenterLat * std::sin(latRad));
		const double lonDeg = normalizeLongitudeDeg(lonRad * RAD_TO_DEG);
		const double latDeg = latRad * RAD_TO_DEG;
		const auto mapPoint = lonLatToMapPoint(lonDeg, latDeg);
		double x = mapPoint.x + xShift;
		if (hasPrevious)
		{
			while (x - previousX > mapWidth * 0.5)
				x -= mapWidth;
			while (previousX - x > mapWidth * 0.5)
				x += mapWidth;
		}

		points.append(QPointF(x, mapPoint.y));
		previousX = x;
		hasPrevious = true;
	}

	return points;
}

QPainterPath ObjectVisibilityMapWidget::twilightCapPath(
	double centerLongitudeDeg,
	double centerLatitudeDeg,
	double angularDistanceDeg,
	const QRectF& mapRect) const
{
	QPainterPath path;
	path.setFillRule(Qt::OddEvenFill);
	if (angularDistanceDeg <= 0.0)
		return path;

	if (angularDistanceDeg >= 180.0)
	{
		path.addRect(mapRect.adjusted(-1.0, -1.0, 1.0, 1.0));
		return path;
	}

	const auto baseTopLeft = lonLatToMapPoint(-180.0, 90.0);
	const double xShift = mapRect.left() - baseTopLeft.x;
	const auto baseLeft = lonLatToMapPoint(-180.0, 0.0);
	const auto baseRight = lonLatToMapPoint(180.0, 0.0);
	const double mapWidth = baseRight.x - baseLeft.x;
	if (mapWidth <= 0.0)
		return path;

	constexpr double poleEpsilonDeg = 1e-7;
	const bool includesNorthPole = centerLatitudeDeg > 0.0 &&
		centerLatitudeDeg + angularDistanceDeg > 90.0 - poleEpsilonDeg;
	const bool includesSouthPole = centerLatitudeDeg < 0.0 &&
		centerLatitudeDeg - angularDistanceDeg < -90.0 + poleEpsilonDeg;

	if (includesNorthPole || includesSouthPole)
	{
		const double centerLatRad = centerLatitudeDeg * DEG_TO_RAD;
		const double sinCenterLat = std::sin(centerLatRad);
		const double cosCenterLat = std::cos(centerLatRad);
		const double cosDistance = std::cos(angularDistanceDeg * DEG_TO_RAD);
		const int steps = static_cast<int>(std::clamp(
			std::ceil(mapWidth / 4.0), 180.0, 720.0));
		QVector<QPointF> boundary;
		boundary.reserve(steps + 1);

		for (int i = 0; i <= steps; ++i)
		{
			const double t = static_cast<double>(i) /
			                 static_cast<double>(steps);
			const double lonDeg = -180.0 + 360.0 * t;
			const double hourAngle =
				normalizeLongitudeDeg(lonDeg - centerLongitudeDeg) *
				DEG_TO_RAD;
			const double b = cosCenterLat * std::cos(hourAngle);
			const double r = std::hypot(sinCenterLat, b);
			if (r <= 1e-12)
				continue;

			const double rawValue = cosDistance / r;
			if (rawValue < -1.0 - 1e-12 || rawValue > 1.0 + 1e-12)
				continue;
			const double value = clampUnit(rawValue);
			const double alpha = std::atan2(b, sinCenterLat);
			const double solution1 = std::asin(value) - alpha;
			const double solution2 = M_PI - std::asin(value) - alpha;
			const double lat1 = solution1 * RAD_TO_DEG;
			const double lat2 = solution2 * RAD_TO_DEG;
			bool hasBoundary = false;
			double boundaryLat = includesNorthPole ? -90.0 : 90.0;
			auto acceptLatitude = [&](double latitudeDeg)
			{
				constexpr double latitudeEpsilonDeg = 1e-9;
				while (latitudeDeg > 180.0)
					latitudeDeg -= 360.0;
				while (latitudeDeg <= -180.0)
					latitudeDeg += 360.0;
				if (latitudeDeg < -90.0 - latitudeEpsilonDeg ||
				    latitudeDeg > 90.0 + latitudeEpsilonDeg)
					return;

				latitudeDeg = std::max(-90.0, std::min(90.0, latitudeDeg));
				if (!hasBoundary)
					boundaryLat = latitudeDeg;
				else if (includesNorthPole)
					boundaryLat = std::min(boundaryLat, latitudeDeg);
				else
					boundaryLat = std::max(boundaryLat, latitudeDeg);
				hasBoundary = true;
			};
			acceptLatitude(lat1);
			acceptLatitude(lat2);
			if (!hasBoundary)
				continue;

			const auto p = lonLatToMapPoint(lonDeg, boundaryLat);
			boundary.append(QPointF(mapRect.left() + mapWidth * t, p.y));
		}

		if (boundary.size() < 2)
			return path;

		if (includesNorthPole)
		{
			path.moveTo(mapRect.topLeft() + QPointF(-1.0, -1.0));
			path.lineTo(mapRect.topRight() + QPointF(1.0, -1.0));
			for (int i = boundary.size() - 1; i >= 0; --i)
				path.lineTo(boundary.at(i));
		}
		else
		{
			path.moveTo(mapRect.bottomLeft() + QPointF(-1.0, 1.0));
			path.lineTo(mapRect.bottomRight() + QPointF(1.0, 1.0));
			for (int i = boundary.size() - 1; i >= 0; --i)
				path.lineTo(boundary.at(i));
		}
		path.closeSubpath();
		return path;
	}

	const QVector<QPointF> points = twilightSmallCirclePoints(
		centerLongitudeDeg, centerLatitudeDeg, angularDistanceDeg, xShift);
	if (points.size() < 3)
		return path;

	QPainterPath basePath(points.first());
	for (const QPointF& p : std::as_const(points))
		basePath.lineTo(p);
	basePath.closeSubpath();

	const QRectF bounds = basePath.boundingRect();
	const int firstCopy = static_cast<int>(
		std::floor((mapRect.left() - bounds.right()) / mapWidth)) - 1;
	const int lastCopy = static_cast<int>(
		std::ceil((mapRect.right() - bounds.left()) / mapWidth)) + 1;
	for (int k = firstCopy; k <= lastCopy; ++k)
	{
		QPainterPath shifted = basePath;
		shifted.translate(k * mapWidth, 0.0);
		path.addPath(shifted);
	}
	return path;
}

QPainterPath ObjectVisibilityMapWidget::twilightBelowAltitudePath(
	double altitudeDeg,
	const QRectF& mapRect) const
{
	return twilightCapPath(normalizeLongitudeDeg(twilightSunLongitudeDeg + 180.0),
	                       -twilightSunLatitudeDeg,
	                       90.0 + altitudeDeg,
	                       mapRect);
}

QVector<QPointF> ObjectVisibilityMapWidget::twilightContourPoints(
	double altitudeDeg) const
{
	return twilightSmallCirclePoints(twilightSunLongitudeDeg,
	                                 twilightSunLatitudeDeg,
	                                 90.0 - altitudeDeg);
}

void ObjectVisibilityMapWidget::invalidateTwilightShadeCache()
{
	twilightShadeCache = QImage();
	twilightShadeCacheImageSize = QSize();
	twilightShadeCacheRatio = 0.0;
	twilightShadeCacheLeft = 0.0;
	twilightShadeCacheTop = 0.0;
	twilightShadeCacheMapWidth = 0.0;
	twilightShadeCacheMapHeight = 0.0;
}

void ObjectVisibilityMapWidget::invalidateTwilightOverlayCache()
{
	invalidateTwilightShadeCache();
	twilightOverlayCache = QImage();
	twilightOverlayCacheImageSize = QSize();
	twilightOverlayCacheRatio = 0.0;
	twilightOverlayCacheLeft = 0.0;
	twilightOverlayCacheTop = 0.0;
	twilightOverlayCacheMapWidth = 0.0;
	twilightOverlayCacheMapHeight = 0.0;
}

void ObjectVisibilityMapWidget::drawTwilightShade(QPainter& painter)
{
	const double ratio = devicePixelRatioF();
	const QSize imageSize(std::max(1, static_cast<int>(std::ceil(width() * ratio))),
	                      std::max(1, static_cast<int>(std::ceil(height() * ratio))));
	const auto topLeft = lonLatToMapPoint(-180.0, 90.0);
	const auto bottomRight = lonLatToMapPoint(180.0, -90.0);
	const double mapWidth = bottomRight.x - topLeft.x;
	const double mapHeight = bottomRight.y - topLeft.y;
	if (mapWidth <= 0.0 || mapHeight <= 0.0) return;

	const bool cacheMatches =
		!twilightShadeCache.isNull() &&
		twilightShadeCacheImageSize == imageSize &&
		qFuzzyCompare(twilightShadeCacheRatio, ratio) &&
		qFuzzyCompare(twilightShadeCacheLeft, topLeft.x) &&
		qFuzzyCompare(twilightShadeCacheTop, topLeft.y) &&
		qFuzzyCompare(twilightShadeCacheMapWidth, mapWidth) &&
		qFuzzyCompare(twilightShadeCacheMapHeight, mapHeight);

	if (!cacheMatches)
	{
		twilightShadeCache = QImage(imageSize, QImage::Format_ARGB32_Premultiplied);
		twilightShadeCache.fill(Qt::transparent);

		QPainter cachePainter(&twilightShadeCache);
		cachePainter.setRenderHint(QPainter::Antialiasing, true);
		renderTwilightShadePaths(cachePainter);

		twilightShadeCacheImageSize = imageSize;
		twilightShadeCacheRatio = ratio;
		twilightShadeCacheLeft = topLeft.x;
		twilightShadeCacheTop = topLeft.y;
		twilightShadeCacheMapWidth = mapWidth;
		twilightShadeCacheMapHeight = mapHeight;
	}

	painter.drawImage(QPointF(0.0, 0.0), twilightShadeCache);
}

void ObjectVisibilityMapWidget::renderTwilightShadePaths(QPainter& painter) const
{
	const auto topLeft = lonLatToMapPoint(-180.0, 90.0);
	const auto bottomRight = lonLatToMapPoint(180.0, -90.0);
	const double mapWidth = bottomRight.x - topLeft.x;
	const double mapHeight = bottomRight.y - topLeft.y;
	if (mapWidth <= 0.0 || mapHeight <= 0.0) return;

	const double ratio = devicePixelRatioF();
	const double logicalWidth = width() * ratio;
	const QRectF baseMapRect(QPointF(topLeft.x, topLeft.y),
	                         QSizeF(mapWidth, mapHeight));
	const int firstCopy = static_cast<int>(
		std::floor((0.0 - baseMapRect.right()) / mapWidth)) - 1;
	const int lastCopy = static_cast<int>(
		std::ceil((logicalWidth - baseMapRect.left()) / mapWidth)) + 1;

	auto ringPath = [](const QPainterPath& outer,
	                   const QPainterPath& inner)
	{
		QPainterPath path;
		path.setFillRule(Qt::OddEvenFill);
		path.addPath(outer);
		path.addPath(inner);
		return path;
	};

	const QPen oldPen = painter.pen();
	const QBrush oldBrush = painter.brush();
	const bool oldAntialiasing = painter.testRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(Qt::NoPen);

	for (int k = firstCopy; k <= lastCopy; ++k)
	{
		const QRectF mapRect = baseMapRect.translated(k * mapWidth, 0.0);
		painter.save();
		painter.setClipRect(mapRect);

		const QPainterPath horizon = twilightBelowAltitudePath(
			twilightHorizonAltitudeDeg(), mapRect);

		if (!twilightMapFullTwilight)
		{
			painter.setBrush(OTHER_NIGHT_SHADE);
			painter.drawPath(horizon);
			painter.restore();
			continue;
		}

		const QPainterPath civil = twilightBelowAltitudePath(-6.0, mapRect);
		const QPainterPath nautical = twilightBelowAltitudePath(-12.0, mapRect);
		const QPainterPath astronomical = twilightBelowAltitudePath(-18.0, mapRect);

		painter.setBrush(EARTH_NIGHT_SHADE);
		painter.drawPath(astronomical);
		painter.setBrush(TWILIGHT_ASTRO_SHADE);
		painter.drawPath(ringPath(nautical, astronomical));
		painter.setBrush(TWILIGHT_NAUT_SHADE);
		painter.drawPath(ringPath(civil, nautical));
		painter.setBrush(TWILIGHT_CIVIL_SHADE);
		painter.drawPath(ringPath(horizon, civil));

		painter.restore();
	}

	painter.setRenderHint(QPainter::Antialiasing, oldAntialiasing);
	painter.setPen(oldPen);
	painter.setBrush(oldBrush);
}

void ObjectVisibilityMapWidget::drawTwilightContour(QPainter& painter,
                                                    double altitudeDeg,
                                                    const QPen& pen) const
{
	const auto leftEdge = lonLatToMapPoint(-180.0, 0.0);
	const auto rightEdge = lonLatToMapPoint(180.0, 0.0);
	const double mapWidth = rightEdge.x - leftEdge.x;
	if (mapWidth <= 0.0) return;

	const QVector<QPointF> points = twilightContourPoints(altitudeDeg);
	if (points.size() < 2) return;

	QPainterPath path(points.first());
	for (int i = 1; i < points.size(); ++i)
		path.lineTo(points.at(i));

	const double ratio = devicePixelRatioF();
	const double logicalWidth = width() * ratio;
	const QRectF bounds = path.boundingRect();
	const int firstCopy = static_cast<int>(
		std::floor((0.0 - bounds.right()) / mapWidth)) - 1;
	const int lastCopy = static_cast<int>(
		std::ceil((logicalWidth - bounds.left()) / mapWidth)) + 1;

	painter.setPen(pen);
	for (int k = firstCopy; k <= lastCopy; ++k)
	{
		QPainterPath shifted = path;
		shifted.translate(k * mapWidth, 0.0);
		painter.drawPath(shifted);
	}
}

void ObjectVisibilityMapWidget::drawSubPointSymbol(QPainter& painter,
                                                   double longitudeDeg,
                                                   double latitudeDeg,
                                                   bool sun) const
{
	if (latitudeDeg < -90.0 || latitudeDeg > 90.0) return;

	const auto leftEdge = lonLatToMapPoint(-180.0, 0.0);
	const auto rightEdge = lonLatToMapPoint(180.0, 0.0);
	const double mapWidth = rightEdge.x - leftEdge.x;
	if (mapWidth <= 0.0) return;

	const double ratio = devicePixelRatioF();
	const double logicalWidth = width() * ratio;
	const auto p = lonLatToMapPoint(normalizeLongitudeDeg(longitudeDeg),
	                                latitudeDeg);
	const double r = std::max(4.5, 6.0 * ratio);

	auto drawOne = [&](double x)
	{
		const QPointF center(x, p.y);
		if (sun)
		{
			QPen rayPen(QColor(255, 215, 75, 240));
			rayPen.setWidthF(std::max(1.0, 1.2 * ratio));
			rayPen.setCapStyle(Qt::RoundCap);
			painter.setPen(rayPen);
			for (int i = 0; i < 8; ++i)
			{
				const double angle = i * M_PI / 4.0;
				const QPointF a(center.x() + std::cos(angle) * r * 1.25,
				                center.y() + std::sin(angle) * r * 1.25);
				const QPointF b(center.x() + std::cos(angle) * r * 1.85,
				                center.y() + std::sin(angle) * r * 1.85);
				painter.drawLine(a, b);
			}
			painter.setBrush(QColor(255, 218, 80, 245));
			painter.setPen(QPen(QColor(175, 95, 20, 245),
			                    std::max(1.0, 1.0 * ratio)));
			painter.drawEllipse(center, r, r);
		}
		else
		{
			painter.setBrush(QColor(235, 235, 220, 235));
			painter.setPen(QPen(QColor(70, 75, 95, 230),
			                    std::max(1.0, 1.0 * ratio)));
			painter.drawEllipse(center, r, r);
			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor(20, 34, 75, 190));
			painter.drawEllipse(QPointF(center.x() + r * 0.42,
			                            center.y() - r * 0.08),
			                    r * 0.86, r * 0.86);
		}
	};

	for (double x = p.x; x < logicalWidth + mapWidth; x += mapWidth)
		drawOne(x);
	for (double x = p.x - mapWidth; x > -mapWidth; x -= mapWidth)
		drawOne(x);
}

void ObjectVisibilityMapWidget::drawTwilightMapOverlay(QPainter& painter)
{
	const double ratio = devicePixelRatioF();
	const QSize imageSize(std::max(1, static_cast<int>(std::ceil(width() * ratio))),
	                      std::max(1, static_cast<int>(std::ceil(height() * ratio))));
	const auto topLeft = lonLatToMapPoint(-180.0, 90.0);
	const auto bottomRight = lonLatToMapPoint(180.0, -90.0);
	const double mapWidth = bottomRight.x - topLeft.x;
	const double mapHeight = bottomRight.y - topLeft.y;
	if (mapWidth <= 0.0 || mapHeight <= 0.0) return;

	const bool cacheMatches =
		!twilightOverlayCache.isNull() &&
		twilightOverlayCacheImageSize == imageSize &&
		qFuzzyCompare(twilightOverlayCacheRatio, ratio) &&
		qFuzzyCompare(twilightOverlayCacheLeft, topLeft.x) &&
		qFuzzyCompare(twilightOverlayCacheTop, topLeft.y) &&
		qFuzzyCompare(twilightOverlayCacheMapWidth, mapWidth) &&
		qFuzzyCompare(twilightOverlayCacheMapHeight, mapHeight);

	if (!cacheMatches)
	{
		twilightOverlayCache = QImage(imageSize, QImage::Format_ARGB32_Premultiplied);
		twilightOverlayCache.fill(Qt::transparent);

		QPainter cachePainter(&twilightOverlayCache);
		cachePainter.setRenderHint(QPainter::Antialiasing, true);
		renderTwilightMapOverlay(cachePainter);

		twilightOverlayCacheImageSize = imageSize;
		twilightOverlayCacheRatio = ratio;
		twilightOverlayCacheLeft = topLeft.x;
		twilightOverlayCacheTop = topLeft.y;
		twilightOverlayCacheMapWidth = mapWidth;
		twilightOverlayCacheMapHeight = mapHeight;
	}

	painter.drawImage(QPointF(0.0, 0.0), twilightOverlayCache);
}

void ObjectVisibilityMapWidget::renderTwilightMapOverlay(QPainter& painter)
{
	drawTwilightShade(painter);

	const double ratio = devicePixelRatioF();
	auto penFor = [ratio](const QColor& color, double width)
	{
		QPen pen(color);
		pen.setWidthF(std::max(1.0, width * ratio));
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		return pen;
	};

	drawTwilightContour(painter, twilightHorizonAltitudeDeg(),
	                    penFor(TWILIGHT_SUNSET_COLOR, 1.7));
	if (twilightMapFullTwilight)
	{
		drawTwilightContour(painter, -6.0,  penFor(TWILIGHT_CIVIL_COLOR, 1.35));
		drawTwilightContour(painter, -12.0, penFor(TWILIGHT_NAUT_COLOR, 1.35));
		drawTwilightContour(painter, -18.0, penFor(TWILIGHT_ASTRO_COLOR, 1.45));
	}

	drawSubPointSymbol(painter, twilightSunLongitudeDeg,
	                   twilightSunLatitudeDeg, true);
	if (twilightMapFullTwilight)
		drawSubPointSymbol(painter, twilightMoonLongitudeDeg,
		                   twilightMoonLatitudeDeg, false);
}

double ObjectVisibilityMapWidget::twilightHorizonAltitudeDeg() const
{
	return twilightMapFullTwilight ? EARTH_APPARENT_SUNRISE_ALTITUDE_DEG : 0.0;
}

double ObjectVisibilityMapWidget::sunAltitudeDegAt(double longitudeDeg,
                                                   double latitudeDeg) const
{
	const double sunLatRad = twilightSunLatitudeDeg * DEG_TO_RAD;
	const double latRad = latitudeDeg * DEG_TO_RAD;
	const double hourAngle = (normalizeLongitudeDeg(longitudeDeg) -
	                          twilightSunLongitudeDeg) * DEG_TO_RAD;
	const double sinAlt = std::sin(latRad) * std::sin(sunLatRad) +
	                      std::cos(latRad) * std::cos(sunLatRad) *
	                      std::cos(hourAngle);
	return std::asin(clampUnit(sinAlt)) * RAD_TO_DEG;
}

bool ObjectVisibilityMapWidget::isPlaceLabelNearOverlay(
	const PlaceLabel& label,
	double toleranceDeg,
	const QVector<double>& lineLatitudes) const
{
	for (double latitude : lineLatitudes)
	{
		if (std::abs(label.latitude - latitude) <= toleranceDeg)
			return true;
	}

	if (currentOverlayMode != LiveTwilightMapOverlay || !hasTwilightMap)
		return false;

	const double altitude = sunAltitudeDegAt(label.longitude, label.latitude);
	const double levels[] = {twilightHorizonAltitudeDeg(), -6.0, -12.0, -18.0};
	for (double level : levels)
	{
		if (!twilightMapFullTwilight && level != 0.0)
			continue;
		if (std::abs(altitude - level) <= toleranceDeg)
			return true;
	}

	return false;
}

QVector<double> ObjectVisibilityMapWidget::currentOverlayLatitudes() const
{
	QVector<double> latitudes;

	auto addLatitude = [&latitudes](double latitude)
	{
		if (latitude < -90.0 || latitude > 90.0) return;
		for (double existing : latitudes)
		{
			if (std::abs(existing - latitude) < 1e-6)
				return;
		}
		latitudes.append(latitude);
	};

	auto addSymmetric = [&addLatitude](double latitude)
	{
		addLatitude(latitude);
		if (std::abs(latitude) > 1e-9)
			addLatitude(-latitude);
	};

	if (currentOverlayMode == VisibilityOverlay && hasDeclination)
	{
		const double dec = declinationDeg;
		const double h = static_cast<double>(goodVisibilityDeg);
		addLatitude(dec + 90.0);
		addLatitude(dec - 90.0);
		addLatitude(dec + (90.0 - h));
		addLatitude(dec - (90.0 - h));
		addLatitude(dec);
		addLatitude(90.0 - dec);
		addLatitude(-90.0 - dec);
	}
	else if (currentOverlayMode == TwilightLimitsOverlay && hasTwilightObliquity)
	{
		const double eps = twilightObliquityDeg;
		const double polar = 90.0 - eps;
		addLatitude(0.0);
		addSymmetric(eps);
		addSymmetric(polar);
		addSymmetric(polar - 6.0);
		addSymmetric(polar - 12.0);
		addSymmetric(polar - 18.0);
		addSymmetric(polar + 6.0);
		addSymmetric(polar + 12.0);
		addSymmetric(polar + 18.0);
	}

	return latitudes;
}

void ObjectVisibilityMapWidget::drawPlaceLabels(QPainter& painter) const
{
	if (!showPlaceLabels || placeLabels.isEmpty()) return;

	const double ratio = devicePixelRatioF();
	const auto leftEdge = lonLatToMapPoint(-180.0, 0.0);
	const auto rightEdge = lonLatToMapPoint(180.0, 0.0);
	const double mapWidth = rightEdge.x - leftEdge.x;
	if (mapWidth <= 0.0) return;

	const QVector<double> lineLatitudes = currentOverlayLatitudes();
	const bool hasOverlayLineFilter = !lineLatitudes.isEmpty() ||
		(currentOverlayMode == LiveTwilightMapOverlay && hasTwilightMap);
	const bool filterNearLines = placeLabelsNearLinesOnly && hasOverlayLineFilter;

	constexpr double minNearLineDegrees = 1.0;
	constexpr double nearLinePixels = 24.0;
	const double logicalWidth = width() * ratio;
	const double logicalHeight = height() * ratio;
	const double degreesPerPixel =
		mapWidth > 0.0 ? 360.0 / mapWidth : 360.0;
	const double nearLineDegrees =
		std::max(minNearLineDegrees, nearLinePixels * ratio * degreesPerPixel);

	QVector<const PlaceLabel*> candidates;
	candidates.reserve(std::min<qsizetype>(placeLabels.size(), 2048));
	for (const PlaceLabel& label : placeLabels)
	{
		if (label.population < placeLabelMinimumPopulation)
			continue;

		if (filterNearLines &&
		    !isPlaceLabelNearOverlay(label, nearLineDegrees, lineLatitudes))
			continue;

		candidates.append(&label);
	}

	auto rolePriority = [](QChar role)
	{
		const QChar r = role.toUpper();
		if (r == QChar('C') || r == QChar('B')) return 0;
		if (r == QChar('R')) return 1;
		if (r == QChar('O')) return 2;
		return 3;
	};

	std::sort(candidates.begin(), candidates.end(),
	          [rolePriority](const PlaceLabel* a, const PlaceLabel* b)
	          {
		          const int ar = rolePriority(a->role);
		          const int br = rolePriority(b->role);
		          if (ar != br) return ar < br;
		          return a->population > b->population;
	          });

	QFont font = painter.font();
	font.setPixelSize(std::max(8, static_cast<int>(std::round(11.0 * ratio))));
	painter.setFont(font);
	const QFontMetrics fm(font);

	QVector<QRectF> occupied;
	occupied.reserve(candidates.size());

	for (const PlaceLabel* label : candidates)
	{
		if (label->latitude < -90.0 || label->latitude > 90.0) continue;

		const auto p = lonLatToMapPoint(label->longitude, label->latitude);
		const QString text = label->name;
		const int textWidth =
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
			fm.horizontalAdvance(text);
#else
			fm.width(text);
#endif
		const double dotR = std::max(2.0, 2.4 * ratio);
		const double xOffset = 6.0 * ratio;
		const double yOffset = -4.0 * ratio;
		const double labelPadX = 3.5 * ratio;
		const double labelPadY = 1.5 * ratio;

		struct Placement
		{
			double xSign;
			double ySign;
		};
		const Placement placements[] = {
			{  1.0, -1.0 },
			{  1.0,  1.0 },
			{ -1.0, -1.0 },
			{ -1.0,  1.0 }
		};

		auto drawCandidate = [&](double x)
		{
			const QPointF dotPos(x, p.y);
			for (const Placement& placement : placements)
			{
				const double textX = placement.xSign > 0.0
				                   ? dotPos.x() + xOffset
				                   : dotPos.x() - xOffset - textWidth;
				const double baselineY = placement.ySign < 0.0
				                       ? dotPos.y() + yOffset
				                       : dotPos.y() - yOffset + fm.ascent();
				const QPointF textPos(textX, baselineY);
				const QRectF glyphRect(textPos.x(),
				                       textPos.y() - fm.ascent(),
				                       textWidth,
				                       fm.height());
				const QRectF labelRect = glyphRect.adjusted(-labelPadX,
				                                            -labelPadY,
				                                            labelPadX,
				                                            labelPadY);
				const QRectF dotRect(dotPos.x() - dotR, dotPos.y() - dotR,
				                     dotR * 2.0, dotR * 2.0);
				const QRectF visibleRect = labelRect.united(dotRect);
				if (visibleRect.right() < 0.0 || visibleRect.left() > logicalWidth ||
				    visibleRect.bottom() < 0.0 || visibleRect.top() > logicalHeight)
					continue;

				const QRectF padded = labelRect.adjusted(-3.0 * ratio,
				                                         -1.5 * ratio,
				                                         3.0 * ratio,
				                                         1.5 * ratio);
				bool collides = false;
				for (const QRectF& rect : occupied)
				{
					if (rect.intersects(padded))
					{
						collides = true;
						break;
					}
				}
				if (collides)
					continue;

				painter.setPen(QPen(QColor(255, 255, 255, 210),
				                    1.2 * ratio));
				painter.setBrush(QColor(25, 25, 25, 215));
				painter.drawEllipse(dotPos, dotR, dotR);

				painter.setPen(Qt::NoPen);
				painter.setBrush(QColor(255, 255, 255, 188));
				painter.drawRoundedRect(labelRect, 2.0 * ratio, 2.0 * ratio);
				painter.setPen(QColor(15, 15, 15, 245));
				painter.drawText(textPos, text);

				occupied.append(padded);
				return true;
			}

			return false;
		};

		bool drawnThisPlace = false;
		for (double x = p.x; x < logicalWidth + mapWidth; x += mapWidth)
		{
			if (drawnThisPlace) break;
			if (x < -mapWidth) continue;
			drawnThisPlace = drawCandidate(x);
		}

		for (double x = p.x - mapWidth; x > -mapWidth; x -= mapWidth)
		{
			if (drawnThisPlace) break;
			drawnThisPlace = drawCandidate(x);
		}
	}
}

void ObjectVisibilityMapWidget::paintEvent(QPaintEvent* event)
{
	// Step 1: let MapWidget render the world map, the marker, and any
	// location filter.  This is the cheapest way to keep pan/zoom/HiDPI
	// behaviour identical to LocationDialog's map.
	MapWidget::paintEvent(event);

	// Step 2: overlay our latitude lines.  Like the base class, we
	// work in device-pixel space.
	QPainter painter(this);
	const double ratio = devicePixelRatioF();
	painter.scale(1.0 / ratio, 1.0 / ratio);
	painter.setRenderHint(QPainter::Antialiasing, true);

	switch (currentOverlayMode)
	{
	case VisibilityOverlay:
		if (hasDeclination)
			drawVisibilityOverlay(painter);
		break;
	case TwilightLimitsOverlay:
		if (hasTwilightObliquity)
			drawTwilightLimitsOverlay(painter);
		break;
	case LiveTwilightMapOverlay:
		if (hasTwilightMap)
			drawTwilightMapOverlay(painter);
		break;
	}

	drawPlaceLabels(painter);
}
