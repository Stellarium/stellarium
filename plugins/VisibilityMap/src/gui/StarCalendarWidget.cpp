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

#include "StarCalendarWidget.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocation.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"

#include <QDate>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QtMath>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
namespace
{
// Twilight altitude thresholds (degrees) – same as EarthShadowMapWidget.
constexpr double kCivil         = -0.8333;  // standard sunrise/sunset
constexpr double kCivilTw       = -6.0;
constexpr double kNauticalTw    = -12.0;
constexpr double kAstronomicalTw= -18.0;

// Plot Y range
constexpr double kLatMin = -90.0;
constexpr double kLatMax =  90.0;

// Margin sizes (pixels)
constexpr int kMarginLeft   = 52;  // for left Y-axis labels
constexpr int kMarginRight  = 52;  // for right Y-axis labels
constexpr int kMarginTop    = 38;  // for title
constexpr int kMarginBottom = 32;  // for X-axis labels
} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
StarCalendarWidget::StarCalendarWidget(QWidget* parent)
	: QWidget(parent)
	, core(StelApp::getInstance().getCore())
	, hasObject(false)
	, objectDecDeg(0.)
	, objectRaDeg(0.)
	, objectYear(2000)
	, cacheDirty(true)
{
	setMinimumSize(560, 320);
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------
void StarCalendarWidget::setObject(double decDeg, double raDeg,
                                   const QString& name, int year)
{
	objectDecDeg = decDeg;
	objectRaDeg  = raDeg;
	objectName   = name;
	objectYear   = year;
	hasObject    = true;
	cacheDirty   = true;
	update();
}

void StarCalendarWidget::clearObject()
{
	hasObject  = false;
	cacheDirty = true;
	update();
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------
void StarCalendarWidget::resizeEvent(QResizeEvent*)
{
	cacheDirty = true;
}

void StarCalendarWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// Overall background
	painter.fillRect(rect(), QColor(15, 18, 30));

	const QRectF plotRect(kMarginLeft, kMarginTop,
	                      width()  - kMarginLeft - kMarginRight,
	                      height() - kMarginTop  - kMarginBottom);
	if (plotRect.width() < 10 || plotRect.height() < 10)
		return;

	if (!hasObject)
	{
		painter.setPen(QColor(160, 165, 180));
		painter.drawText(rect(), Qt::AlignCenter,
		                 q_("Select a star and click \"Calculate visibility\""));
		return;
	}

	// Rebuild the pixel cache if needed
	if (cacheDirty || bgCacheSize != size())
		rebuildCache();

	// Draw cached background
	painter.drawImage(QPoint(kMarginLeft, kMarginTop), bgCache);

	// Border around plot area
	painter.setPen(QPen(QColor(180, 185, 200), 1.0));
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(plotRect);

	drawAxes(painter, plotRect);
	drawContourLines(painter, plotRect);
	drawObserverLine(painter, plotRect);
	drawCaption(painter, plotRect);
}

// ---------------------------------------------------------------------------
// Background image — the expensive pixel loop (run once, cached)
// ---------------------------------------------------------------------------
void StarCalendarWidget::rebuildCache()
{
	const QRectF plotRect(kMarginLeft, kMarginTop,
	                      width()  - kMarginLeft - kMarginRight,
	                      height() - kMarginTop  - kMarginBottom);
	const int pw = qMax(1, qRound(plotRect.width()));
	const int ph = qMax(1, qRound(plotRect.height()));

	// Use Format_ARGB32 (straight alpha) — premultiplied format causes colour
	// distortion when qRgba() straight values are used.
	bgCache     = QImage(pw, ph, QImage::Format_ARGB32);
	bgCacheSize = size();
	cacheDirty  = false;

	for (int py = 0; py < ph; ++py)
	{
		QRgb* line = reinterpret_cast<QRgb*>(bgCache.scanLine(py));
		// Map pixel row to latitude (top = kLatMax, bottom = kLatMin)
		const double lat = kLatMax - (py + 0.5) * (kLatMax - kLatMin) / ph;

		// Altitude of star at culmination for this latitude — right Y-axis info
		const double culminationAlt = 90.0 - qAbs(lat - objectDecDeg);

		for (int px = 0; px < pw; ++px)
		{
			// Map pixel column to fractional day of year (1.0 … 365.0).
			// Using a fractional day avoids the banding that occurs when
			// multiple pixels map to the same integer day.
			const double dayFrac = 1.0 + (px + 0.5) * 364.0 / pw;

			// If the star never rises at this latitude, shade as night
			if (culminationAlt <= 0.0)
			{
				line[px] = qRgba(8, 12, 48, 255);
				continue;
			}

			const double sunAlt = sunAltAtStarTransit(lat, dayFrac);

			// Apply twilight colour palette (same as EarthShadowMapWidget blue shading)
			if (sunAlt < kAstronomicalTw)
				line[px] = qRgba(8,  12,  48,  255);   // night — deep navy
			else if (sunAlt < kNauticalTw)
				line[px] = qRgba(15, 30,  95,  255);   // astronomical twilight
			else if (sunAlt < kCivilTw)
				line[px] = qRgba(35, 65,  145, 255);   // nautical twilight
			else if (sunAlt < kCivil)
				line[px] = qRgba(70, 115, 185, 255);   // civil twilight
			else
				line[px] = qRgba(200, 195, 175, 255);  // daylight — warm grey
		}
	}
}

// ---------------------------------------------------------------------------
// Astronomical helpers
// ---------------------------------------------------------------------------

// Accurate solar longitude (degrees) for day-of-year d in year y.
// Uses the Julian Day as the time argument, which avoids the century-scale
// drift of purely day-of-year approximations.  This gives ~1° accuracy
// across the full DE441 window (−15000 to +17000 CE).
double StarCalendarWidget::solarLongitudeDeg(int dayOfYear, int year, double fracDay)
{
	// Compute JD for the given day (with optional fractional part for sub-day precision)
	double jd = 0.;
	StelUtils::getJDFromDate(&jd, year, 1, dayOfYear, 12, 0, 0.f);
	jd += fracDay;  // add fractional day offset for smooth interpolation

	// Julian centuries from J2000.0
	const double T = (jd - 2451545.0) / 36525.0;

	// Geometric mean longitude of the Sun (degrees), corrected for aberration
	const double L0 = StelUtils::fmodpos(280.46646 + 36000.76983 * T + 0.0003032 * T * T, 360.0);

	// Mean anomaly of the Sun (degrees)
	const double M = StelUtils::fmodpos(357.52911 + 35999.05029 * T - 0.0001537 * T * T, 360.0);
	const double Mrad = M * M_PI / 180.0;

	// Equation of centre
	const double C = (1.914602 - 0.004817 * T - 0.000014 * T * T) * qSin(Mrad)
	               + (0.019993 - 0.000101 * T) * qSin(2.0 * Mrad)
	               + 0.000289 * qSin(3.0 * Mrad);

	return StelUtils::fmodpos(L0 + C, 360.0);
}

double StarCalendarWidget::solarDeclinationDeg(double solarLonDeg)
{
	// Obliquity of ecliptic (degrees) — fixed at J2000 value for this purpose
	constexpr double eps = 23.439;
	return qAsin(qSin(eps * M_PI / 180.0) *
	             qSin(solarLonDeg * M_PI / 180.0)) * M_180_PI;
}

double StarCalendarWidget::solarRaDeg(double solarLonDeg)
{
	// Right ascension from ecliptic longitude, using obliquity ε
	constexpr double eps = 23.439;
	const double lonRad  = solarLonDeg * M_PI / 180.0;
	const double epsRad  = eps * M_PI / 180.0;
	const double ra = qAtan2(qCos(epsRad) * qSin(lonRad), qCos(lonRad));
	return StelUtils::fmodpos(ra * M_180_PI, 360.0);
}

double StarCalendarWidget::sunAltAtStarTransit(double latDeg, double dayOfYear) const
{
	// 1. Solar position for this (fractional) day
	const double solarLon = solarLongitudeDeg(static_cast<int>(dayOfYear), objectYear,
	                                           dayOfYear - static_cast<int>(dayOfYear));
	const double sunDec   = solarDeclinationDeg(solarLon);
	const double sunRa    = solarRaDeg(solarLon);

	// 2. Hour angle of Sun when star transits (LST = starRA at transit)
	//    H☉ = starRA − sunRA  (in degrees, converted to radians for trig)
	const double H = (objectRaDeg - sunRa) * M_PI / 180.0;

	// 3. Solar altitude
	const double phiRad    = latDeg    * M_PI / 180.0;
	const double sunDecRad = sunDec    * M_PI / 180.0;
	const double sinAlt    = qSin(phiRad) * qSin(sunDecRad) +
	                         qCos(phiRad) * qCos(sunDecRad) * qCos(H);
	return qAsin(qBound(-1.0, sinAlt, 1.0)) * M_180_PI;
}

// ---------------------------------------------------------------------------
// Map helpers
// ---------------------------------------------------------------------------
QPointF StarCalendarWidget::toPixel(const QRectF& plotRect,
                                    double dayOfYear, double latitudeDeg) const
{
	const double x = plotRect.left() + (dayOfYear - 1.0) / 364.0 * plotRect.width();
	const double y = plotRect.top()  + (kLatMax - latitudeDeg) / (kLatMax - kLatMin) * plotRect.height();
	return QPointF(x, y);
}

// ---------------------------------------------------------------------------
// Axes
// ---------------------------------------------------------------------------
void StarCalendarWidget::drawAxes(QPainter& painter, const QRectF& plotRect) const
{
	painter.save();
	painter.setPen(QColor(200, 205, 215));
	painter.setFont(QFont(painter.font().family(), 9));
	const QFontMetrics fm(painter.font());

	// ── Left Y-axis: observer latitude ──────────────────────────────────────
	for (int lat = -80; lat <= 80; lat += 20)
	{
		const QPointF p = toPixel(plotRect, 1, static_cast<double>(lat));
		// Tick
		painter.drawLine(QPointF(plotRect.left() - 4, p.y()),
		                 QPointF(plotRect.left(),      p.y()));
		// Label
		const QString lbl = QString::number(lat) + QStringLiteral("°");
		painter.drawText(QRectF(0, p.y() - 8, kMarginLeft - 6, 16),
		                 Qt::AlignRight | Qt::AlignVCenter, lbl);
		// Light grid line
		painter.setPen(QPen(QColor(255, 255, 255, 30), 0.5));
		painter.drawLine(QPointF(plotRect.left(),  p.y()),
		                 QPointF(plotRect.right(), p.y()));
		painter.setPen(QColor(200, 205, 215));
	}

	// ── Right Y-axis: star's culmination altitude ─────────────────────────
	// alt = 90 − |lat − dec|  →  lat = dec ± (90 − alt)
	// We label altitudes 0°, 10°, 20°, ... 90° for the northern hemisphere
	// (where lat > dec) if they fall in the plot range.
	painter.setPen(QColor(180, 220, 255));
	for (int alt = 0; alt <= 90; alt += 10)
	{
		// For northern latitudes: lat = dec + (90 - alt)
		const double lat = objectDecDeg + (90.0 - alt);
		if (lat < kLatMin || lat > kLatMax) continue;
		const QPointF p = toPixel(plotRect, 365, lat);
		painter.drawLine(QPointF(plotRect.right(),     p.y()),
		                 QPointF(plotRect.right() + 4,  p.y()));
		const QString lbl = QString::number(alt) + QStringLiteral("°");
		painter.drawText(QRectF(plotRect.right() + 6, p.y() - 8,
		                        kMarginRight - 8, 16),
		                 Qt::AlignLeft | Qt::AlignVCenter, lbl);
	}

	// Right Y-axis title — "Culmination altitude"
	painter.save();
	painter.translate(width() - 10, plotRect.center().y());
	painter.rotate(90);
	painter.setFont(QFont(painter.font().family(), 9));
	painter.drawText(QRectF(-50, -8, 100, 16), Qt::AlignCenter,
	                 q_("Culmination altitude"));
	painter.restore();

	// ── X-axis: months ──────────────────────────────────────────────────────
	painter.setPen(QColor(200, 205, 215));
	// Month start days (non-leap year)
	const int monthStart[12] = {1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
	const char* monthAbbr[12] = {
	    QT_TR_NOOP("Jan"), QT_TR_NOOP("Feb"), QT_TR_NOOP("Mar"),
	    QT_TR_NOOP("Apr"), QT_TR_NOOP("May"), QT_TR_NOOP("Jun"),
	    QT_TR_NOOP("Jul"), QT_TR_NOOP("Aug"), QT_TR_NOOP("Sep"),
	    QT_TR_NOOP("Oct"), QT_TR_NOOP("Nov"), QT_TR_NOOP("Dec")
	};

	for (int m = 0; m < 12; ++m)
	{
		const QPointF p = toPixel(plotRect, monthStart[m], kLatMin);
		painter.drawLine(QPointF(p.x(), plotRect.bottom()),
		                 QPointF(p.x(), plotRect.bottom() + 4));
		// Light vertical grid line
		painter.setPen(QPen(QColor(255, 255, 255, 30), 0.5));
		painter.drawLine(QPointF(p.x(), plotRect.top()),
		                 QPointF(p.x(), plotRect.bottom()));
		painter.setPen(QColor(200, 205, 215));

		// Month label centred in the month
		const int midDay = monthStart[m] + (m < 11 ? (monthStart[m+1] - monthStart[m]) / 2 : 15);
		const QPointF mid = toPixel(plotRect, midDay, kLatMin);
		painter.drawText(QRectF(mid.x() - 16, plotRect.bottom() + 5, 32, 18),
		                 Qt::AlignCenter, q_(monthAbbr[m]));
	}

	// Year label — use BCE/CE notation for historical/future epochs
	{
		const QPointF p = toPixel(plotRect, 355, kLatMin);
		painter.setFont(QFont(painter.font().family(), 9));
		QString yearStr;
		if (objectYear <= 0)
			yearStr = QString::number(1 - objectYear) + QStringLiteral(" BCE");
		else
			yearStr = QString::number(objectYear);
		painter.drawText(QRectF(p.x() - 25, plotRect.bottom() + 14, 50, 12),
		                 Qt::AlignCenter, yearStr);
	}

	painter.restore();
}

// ---------------------------------------------------------------------------
// Twilight contour lines at the three thresholds
// ---------------------------------------------------------------------------
void StarCalendarWidget::drawContourLines(QPainter& painter, const QRectF& plotRect) const
{
	// For each threshold altitude (degrees), trace the iso-contour across the plot.
	// We sample day by day and connect the latitude at which sunAlt == threshold.
	// Since sunAlt varies smoothly, a simple linear interpolation between adjacent
	// latitudes is sufficient.

	struct Contour
	{
		double altDeg;
		QColor color;
		QString label;
	};

	const QVector<Contour> contours = {
	    {kCivil,          QColor(255, 220, 120), q_("Sunrise/sunset")},
	    {kCivilTw,        QColor(140, 200, 255), q_("Civil (−6°)")},
	    {kNauticalTw,     QColor( 80, 150, 255), q_("Nautical (−12°)")},
	    {kAstronomicalTw, QColor( 30,  90, 220), q_("Astronomical (−18°)")},
	};

	const int latSteps = 180;
	const double latStep = (kLatMax - kLatMin) / latSteps;

	painter.save();
	painter.setClipRect(plotRect);

	for (const Contour& c : contours)
	{
		painter.setPen(QPen(c.color, 1.2));

		for (int day = 1; day <= 365; ++day)
		{
			// Find latitude crossing by binary search
			double lo = kLatMin, hi = kLatMax;
			const double loAlt = sunAltAtStarTransit(lo, day);
			const double hiAlt = sunAltAtStarTransit(hi, day);

			// Check if threshold is crossed at all
			const double mn = qMin(loAlt, hiAlt);
			const double mx = qMax(loAlt, hiAlt);
			if (c.altDeg < mn || c.altDeg > mx)
				continue;

			// Binary search for the crossing latitude
			for (int iter = 0; iter < 20; ++iter)
			{
				const double mid    = (lo + hi) / 2.0;
				const double midAlt = sunAltAtStarTransit(mid, day);
				if ((midAlt - c.altDeg) * (loAlt - c.altDeg) <= 0.0)
					hi = mid;
				else
					lo = mid;
			}
			const double crossLat = (lo + hi) / 2.0;

			if (day > 1)
			{
				const QPointF prev = toPixel(plotRect, day - 1, crossLat);
				const QPointF curr = toPixel(plotRect, day,     crossLat);
				painter.drawLine(prev, curr);
			}
		}

		// Label at the right edge
		{
			const int labelDay = 360;
			double lo2 = kLatMin, hi2 = kLatMax;
			const double lo2Alt = sunAltAtStarTransit(lo2, labelDay);
			const double hi2Alt = sunAltAtStarTransit(hi2, labelDay);
			const double mn2 = qMin(lo2Alt, hi2Alt);
			const double mx2 = qMax(lo2Alt, hi2Alt);
			if (c.altDeg >= mn2 && c.altDeg <= mx2)
			{
				for (int iter = 0; iter < 20; ++iter)
				{
					const double mid    = (lo2 + hi2) / 2.0;
					const double midAlt = sunAltAtStarTransit(mid, labelDay);
					if ((midAlt - c.altDeg) * (lo2Alt - c.altDeg) <= 0.0)
						hi2 = mid;
					else
						lo2 = mid;
				}
				const QPointF lp = toPixel(plotRect, labelDay, (lo2 + hi2) / 2.0);
				painter.setFont(QFont(painter.font().family(), 9, QFont::Bold));
				painter.setPen(c.color);
				painter.drawText(lp + QPointF(2, -3), c.label);
			}
		}
	}

	painter.restore();
}

// ---------------------------------------------------------------------------
// Observer latitude line
// ---------------------------------------------------------------------------
void StarCalendarWidget::drawObserverLine(QPainter& painter,
                                          const QRectF& plotRect) const
{
	if (!core || core->getCurrentLocation().planetName != "Earth")
		return;

	const double obsLat = static_cast<double>(core->getCurrentLocation().getLatitude());
	if (obsLat < kLatMin || obsLat > kLatMax)
		return;

	const QPointF left  = toPixel(plotRect,   1, obsLat);
	const QPointF right = toPixel(plotRect, 365, obsLat);

	painter.save();
	QPen pen(QColor(255, 255, 80), 1.5, Qt::DashLine);
	painter.setPen(pen);
	painter.setClipRect(plotRect);
	painter.drawLine(left, right);

	// Label on the left margin
	painter.setClipping(false);
	painter.setFont(QFont(painter.font().family(), 9, QFont::Bold));
	painter.setPen(QColor(255, 255, 80));
	const QString lbl = QString::asprintf("%.1f°", obsLat);
	painter.drawText(QRectF(0, left.y() - 8, kMarginLeft - 2, 16),
	                 Qt::AlignRight | Qt::AlignVCenter, lbl);
	painter.restore();
}

// ---------------------------------------------------------------------------
// Title caption
// ---------------------------------------------------------------------------
void StarCalendarWidget::drawCaption(QPainter& painter,
                                     const QRectF& plotRect) const
{
	painter.save();
	painter.setFont(QFont(painter.font().family(), 11, QFont::Bold));
	painter.setPen(QColor(230, 235, 245));

	// Show BCE/CE suffix for clarity at historical/future epochs
	QString yearStr;
	if (objectYear <= 0)
		// Astronomical year 0 = 1 BCE, year -1 = 2 BCE, etc.
		yearStr = QString::number(1 - objectYear) + QStringLiteral(" BCE");
	else
		yearStr = QString::number(objectYear) + QStringLiteral(" CE");

	const QString title = q_("Best visibility of") + QStringLiteral(" ") + objectName +
	                      QStringLiteral("  (dec ") +
	                      QString::asprintf("%+.2f°", objectDecDeg) +
	                      QStringLiteral(")  —  ") + yearStr;
	painter.drawText(QRectF(plotRect.left(), 4, plotRect.width(), kMarginTop - 6),
	                 Qt::AlignCenter | Qt::AlignVCenter, title);
	painter.restore();
}
