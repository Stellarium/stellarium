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
#include <cmath>

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
constexpr double EPS = 1e-9;
// Colour matching the screenshots in the article: solid blue line, a
// slightly lighter dashed line, blue plus marks and triangle markers.
const QColor LINE_COLOR     = QColor(0,  60, 220, 255);
const QColor DASHED_COLOR   = QColor(0,  60, 220, 220);
const QColor ZENITH_COLOR   = QColor(0,  60, 220, 240);
const QColor TRIANGLE_COLOR = QColor(0,  60, 220, 255);
} // namespace

void ObjectVisibilityMapWidget::drawLatitudeLine(QPainter& painter,
                                                 double latitudeDeg,
                                                 const QPen& pen) const
{
	if (latitudeDeg < -90.0 - EPS || latitudeDeg > 90.0 + EPS) return;

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

void ObjectVisibilityMapWidget::drawLatitudeMarkers(QPainter& painter,
                                                    double latitudeDeg,
                                                    const QChar& marker,
                                                    const QColor& color) const
{
	if (latitudeDeg < -90.0 - EPS || latitudeDeg > 90.0 + EPS) return;

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

void ObjectVisibilityMapWidget::paintEvent(QPaintEvent* event)
{
	// Step 1: let MapWidget render the world map, the (hidden) marker,
	// and any location filter.  This is the cheapest way to keep
	// pan/zoom/HiDPI behaviour identical to LocationDialog's map.
	MapWidget::paintEvent(event);

	if (!hasDeclination) return;

	// Step 2: overlay our visibility lines.  Like the base class, we
	// work in device-pixel space.
	QPainter painter(this);
	const double ratio = devicePixelRatioF();
	painter.scale(1.0 / ratio, 1.0 / ratio);
	painter.setRenderHint(QPainter::Antialiasing, true);

	const double dec = declinationDeg;
	const double h   = static_cast<double>(goodVisibilityDeg);

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
