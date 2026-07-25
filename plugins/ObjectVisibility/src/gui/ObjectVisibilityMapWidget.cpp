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
#include <QPaintEvent>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QPolygonF>
#include <QRectF>
#include <cmath>
#include <algorithm>

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
	const bool filterNearLines = placeLabelsNearLinesOnly && !lineLatitudes.isEmpty();

	constexpr double nearLineDegrees = 1.0;
	constexpr int maxLabels = 120;
	const double logicalWidth = width() * ratio;
	const double logicalHeight = height() * ratio;

	QVector<const PlaceLabel*> candidates;
	candidates.reserve(static_cast<int>(std::min<qsizetype>(
		placeLabels.size(), 2048)));
	for (const PlaceLabel& label : placeLabels)
	{
		if (label.population < placeLabelMinimumPopulation)
			continue;

		if (filterNearLines)
		{
			bool near = false;
			for (double latitude : lineLatitudes)
			{
				if (std::abs(label.latitude - latitude) <= nearLineDegrees)
				{
					near = true;
					break;
				}
			}
			if (!near)
				continue;
		}

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
	occupied.reserve(maxLabels);
	int labelsDrawn = 0;

	for (const PlaceLabel* label : candidates)
	{
		if (labelsDrawn >= maxLabels) break;
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

		auto drawCandidate = [&](double x)
		{
			const QPointF dotPos(x, p.y);
			const QPointF textPos(dotPos.x() + xOffset,
			                      dotPos.y() + yOffset);
			const QRectF glyphRect(textPos.x(),
			                       textPos.y() - fm.ascent(),
			                       textWidth,
			                       fm.height());
			const QRectF labelRect = glyphRect.adjusted(-labelPadX,
			                                            -labelPadY,
			                                            labelPadX,
			                                            labelPadY);
			if (labelRect.right() < 0.0 || labelRect.left() > logicalWidth ||
			    labelRect.bottom() < 0.0 || labelRect.top() > logicalHeight)
				return false;

			const QRectF padded = labelRect.adjusted(-3.0 * ratio, -1.5 * ratio,
			                                         3.0 * ratio, 1.5 * ratio);
			for (const QRectF& rect : occupied)
			{
				if (rect.intersects(padded))
					return false;
			}

			painter.setPen(QPen(QColor(255, 255, 255, 210), 1.2 * ratio));
			painter.setBrush(QColor(25, 25, 25, 215));
			painter.drawEllipse(dotPos, dotR, dotR);

			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor(255, 255, 255, 188));
			painter.drawRoundedRect(labelRect, 2.0 * ratio, 2.0 * ratio);
			painter.setPen(QColor(15, 15, 15, 245));
			painter.drawText(textPos, text);

			occupied.append(padded);
			++labelsDrawn;
			return true;
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
	}

	drawPlaceLabels(painter);
}
