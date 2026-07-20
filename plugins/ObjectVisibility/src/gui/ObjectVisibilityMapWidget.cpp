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
#include <QPolygonF>
#include <QRectF>
#include <cmath>
#include <algorithm>

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
	rebuildTwilightMapCache();
	update();
}

void ObjectVisibilityMapWidget::setTwilightMapFullTwilight(bool enabled)
{
	if (twilightMapFullTwilight == enabled) return;
	twilightMapFullTwilight = enabled;
	if (hasTwilightMap)
	{
		rebuildTwilightMapCache();
		update();
	}
}

void ObjectVisibilityMapWidget::clearTwilightMap()
{
	if (!hasTwilightMap && twilightShadeImage.isNull()) return;
	hasTwilightMap = false;
	twilightShadeImage = QImage();
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

void ObjectVisibilityMapWidget::rebuildTwilightMapCache()
{
	constexpr int shadeWidth = 720;
	constexpr int shadeHeight = 360;

	QImage image(shadeWidth, shadeHeight, QImage::Format_ARGB32);
	image.fill(Qt::transparent);

	const double sunLatRad = twilightSunLatitudeDeg * DEG_TO_RAD;
	const double sinSunLat = std::sin(sunLatRad);
	const double cosSunLat = std::cos(sunLatRad);
	const double horizonAltitudeDeg = twilightHorizonAltitudeDeg();

	for (int y = 0; y < shadeHeight; ++y)
	{
		const double latDeg = 90.0 - (static_cast<double>(y) + 0.5) *
		                      180.0 / static_cast<double>(shadeHeight);
		const double latRad = latDeg * DEG_TO_RAD;
		const double sinLat = std::sin(latRad);
		const double cosLat = std::cos(latRad);
		QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));

		for (int x = 0; x < shadeWidth; ++x)
		{
			const double lonDeg = -180.0 + (static_cast<double>(x) + 0.5) *
			                      360.0 / static_cast<double>(shadeWidth);
			const double hourAngle = (lonDeg - twilightSunLongitudeDeg) * DEG_TO_RAD;
			const double sinAlt = sinLat * sinSunLat +
			                      cosLat * cosSunLat * std::cos(hourAngle);
			const double altDeg = std::asin(clampUnit(sinAlt)) * RAD_TO_DEG;

			if (altDeg >= horizonAltitudeDeg)
				line[x] = qRgba(0, 0, 0, 0);
			else if (!twilightMapFullTwilight)
				line[x] = qRgba(0, 0, 0, 128);
			else if (altDeg >= -6.0)
				line[x] = qRgba(95, 155, 255, 58);
			else if (altDeg >= -12.0)
				line[x] = qRgba(55, 105, 225, 88);
			else if (altDeg >= -18.0)
				line[x] = qRgba(30, 60, 165, 118);
			else
				line[x] = qRgba(5, 18, 58, 148);
		}
	}

	twilightShadeImage = image;
}

void ObjectVisibilityMapWidget::drawMapImageCopies(QPainter& painter,
                                                   const QImage& image,
                                                   double opacity) const
{
	if (image.isNull()) return;

	const auto topLeft = lonLatToMapPoint(-180.0, 90.0);
	const auto bottomRight = lonLatToMapPoint(180.0, -90.0);
	const double mapWidth = bottomRight.x - topLeft.x;
	const double mapHeight = bottomRight.y - topLeft.y;
	if (mapWidth <= 0.0 || mapHeight <= 0.0) return;

	const double ratio = devicePixelRatioF();
	const double logicalWidth = width() * ratio;
	const double oldOpacity = painter.opacity();
	painter.setOpacity(oldOpacity * opacity);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

	for (double x = topLeft.x; x < logicalWidth + mapWidth; x += mapWidth)
		painter.drawImage(QRectF(x, topLeft.y, mapWidth, mapHeight), image);
	for (double x = topLeft.x - mapWidth; x > -mapWidth; x -= mapWidth)
		painter.drawImage(QRectF(x, topLeft.y, mapWidth, mapHeight), image);

	painter.setOpacity(oldOpacity);
}

void ObjectVisibilityMapWidget::drawTwilightContour(QPainter& painter,
                                                    double altitudeDeg,
                                                    const QPen& pen) const
{
	const auto leftEdge = lonLatToMapPoint(-180.0, 0.0);
	const auto rightEdge = lonLatToMapPoint(180.0, 0.0);
	const double mapWidth = rightEdge.x - leftEdge.x;
	if (mapWidth <= 0.0) return;

	const double ratio = devicePixelRatioF();
	const double logicalWidth = width() * ratio;
	const double sunLatRad = twilightSunLatitudeDeg * DEG_TO_RAD;
	const double sunLonRad = twilightSunLongitudeDeg * DEG_TO_RAD;
	const double sinSunLat = std::sin(sunLatRad);
	const double cosSunLat = std::cos(sunLatRad);
	const double angularDistance = (90.0 - altitudeDeg) * DEG_TO_RAD;
	const double sinDistance = std::sin(angularDistance);
	const double cosDistance = std::cos(angularDistance);

	painter.setPen(pen);

	auto drawPathCopies = [&painter, mapWidth, logicalWidth](const QVector<QPointF>& points)
	{
		if (points.size() < 2) return;

		QPainterPath path(points.first());
		for (int i = 1; i < points.size(); ++i)
			path.lineTo(points.at(i));

		const QRectF bounds = path.boundingRect();
		const int firstCopy = static_cast<int>(
			std::floor((0.0 - bounds.right()) / mapWidth)) - 1;
		const int lastCopy = static_cast<int>(
			std::ceil((logicalWidth - bounds.left()) / mapWidth)) + 1;
		for (int k = firstCopy; k <= lastCopy; ++k)
		{
			QPainterPath shifted = path;
			shifted.translate(k * mapWidth, 0.0);
			painter.drawPath(shifted);
		}
	};

	QVector<QPointF> points;
	points.reserve(721);
	double previousX = 0.0;
	bool hasPrevious = false;

	for (int i = 0; i <= 720; ++i)
	{
		const double bearing = static_cast<double>(i) * 0.5 * DEG_TO_RAD;
		const double latRad = std::asin(clampUnit(
			sinSunLat * cosDistance +
			cosSunLat * sinDistance * std::cos(bearing)));
		const double lonRad = sunLonRad + std::atan2(
			std::sin(bearing) * sinDistance * cosSunLat,
			cosDistance - sinSunLat * std::sin(latRad));
		const double lonDeg = normalizeLongitudeDeg(lonRad * RAD_TO_DEG);
		const double latDeg = latRad * RAD_TO_DEG;
		const auto mapPoint = lonLatToMapPoint(lonDeg, latDeg);
		double x = mapPoint.x;
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

	drawPathCopies(points);
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

void ObjectVisibilityMapWidget::drawTwilightMapOverlay(QPainter& painter) const
{
	drawMapImageCopies(painter, twilightShadeImage);

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
	candidates.reserve(static_cast<int>(std::min<qint64>(
		static_cast<qint64>(placeLabels.size()), 2048)));
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
