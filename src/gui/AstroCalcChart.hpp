/*
 * Stellarium
 * Copyright (C) 2022 Georg Zotti
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef ASTROCALCCHART_H
#define ASTROCALCCHART_H


#include <QChart>
#include <QSplineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>

#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
using namespace QtCharts;
#endif

//! @class AstroCalcChart
//! This class extends QChart and encapsulates data for the Altitude vs. Time and the other line Charts in AstroCalcDialog.
//! Most lines can be QSplineSeries, however the azimuth and magnitude lines must use QLineSeries to avoid a "spline tremor" at the 0/360° transition or where some moon enters the shadow of its planet.
//!
//! Additional features:
//! - Mouse-over on series provides date/value information in a tooltip
//! - Left mouse click sets date/time of click point.
//!
//! The correct sequence of use:
//! - create with the possible series you are going to use
//! - fill the series with values with append(). The X value is milliseconds from Epoch (1970).
//! - add horizontal/vertical lines with addTrivial[YX]Line(series)
//! - call show(series)
//! - scale the Y axes:
//! -- You can retrieve the actual min/max values from findYRange(series). Apply any manual limits last.
//! -- call setYrange(series, min, max) and setYrangeR(series, min, max) where needed. Use real min/max. A series-dependent buffer will be added.
//! - call setupAxes()
//! - use setChart() to display chart in a chartView. A previous chart must be retrieved and deleted!
//!
//! The time coordinate is displayed using a QDateTimeAxis. Given slight differences between QDateTime and Stellarium's understanding of timezones etc.,
//!  we must circumvent Qt's automatic handling of local timezones and work as if the time axis would display UTC dates, however, the actual data are in our own timezone frame.
//! This requires manipulation of the x values: From the data which are computed in JD, plot them in UT+zoneOffset. Clicked time is delivered as "UT" and must add zoneOffset as well.
//!
//! @note We are using QDateTime here. Currently we have no handling for years before 1AD. Qt has no year zero, we have it. Qt time scaling may also show use pf Proleptic Gregorian calendars.
//! We must expect to see a few weird dates before 1AD or even before 1582.
//!
class AstroCalcChart : public QChart
{
	Q_OBJECT

public:
	// Series names.
	// In 0.22 there are 6 charts created by activating 1-2 lines of these:
	enum Series {AltVsTime, CurrentTime, TransitTime, SunElevation, CivilTwilight, NauticalTwilight, AstroTwilight, Moon,
		     AzVsTime,
		     MonthlyElevation,
		     AngularSize1, Declination1, Distance1, Elongation1, HeliocentricDistance1, Magnitude1, PhaseAngle1, Phase1, RightAscension1, TransitAltitude1,
		     AngularSize2, Declination2, Distance2, Elongation2, HeliocentricDistance2, Magnitude2, PhaseAngle2, Phase2, RightAscension2, TransitAltitude2,
		     LunarElongation, LunarElongationLimit,
		     pcDistanceAU, pcDistanceDeg // PlanetaryCalculator: distance between two SSO in AU and angle(degrees).
		    };
	Q_ENUM(Series)

	AstroCalcChart(QSet<AstroCalcChart::Series> which);
	virtual ~AstroCalcChart() Q_DECL_OVERRIDE;

	//! Append one pair of data values to series s. x must be some QDateTime.toMSecsSinceEpoch()
	void append(Series s, qint64 x, qreal y);
	//! Replace one pair of data values to series s at position index.
	void replace(Series s, int index, qreal x, qreal y);
	//! Reset (delete and create new empty) series s
	void clear(Series s);
	//! @return length of series s. If chart does not contain s, returns -1.
	int lengthOfSeries(Series s) const;

	//! Activate series s. It must have been filled using append(s, ., .) before.
	//! Make sure to call setupAxes() after calling show!
	void show(Series s);

	//! Set range of Y axis required by the Y values of series. Do not set buffers or logical limits here, these are determined in this function.
	//! @arg strictMin: The altVsTime plot may require strict minimum setting without buffer. Use false (default) for other plots.
	void setYrange(Series series, qreal min, qreal max, bool strictMin=false);
	//! Convenience function
	void setYrange(Series series, QPair<double, double> yRange, bool strictMin=false){setYrange(series, yRange.first, yRange.second, strictMin);}
	//! Set range of Y axis on the right side required by the Y values of series. Do not set buffers or logical limits here, these are determined in this function.
	void setYrangeR(Series series, qreal min, qreal max);
	//! Convenience function
	void setYrangeR(Series series, QPair<double, double> yRangeR){setYrangeR(series, yRangeR.first, yRangeR.second);}

	//! Find range of values for the respective series plot.
	QPair<double, double>findYRange(const Series series) const;

	//! Find x/y values where y=max for the respective series plot.
	QPair<double, double>findYMax(const Series series) const;

	//! Setup axes and appearance. Call this at the end of drawing, just before display but after all series have been activated with show().
	//! @arg englishName used for details in the two-graph charts. Use an empty string otherwise.
	void setupAxes(const double jd, const int periods, const QString &englishName);

	//! Draw a 2-point vertical line at x which extends over the whole graph
	void drawTrivialLineX(Series s, const qreal x);

	//! Draw a 2-point horizontal line at y which extends over the whole graph
	void drawTrivialLineY(Series s, const qreal y);

public slots:
	virtual void retranslate();
	//! A slot to connect the hovered signal from a series to. It shows the value of the series in a Tooltip on mouse-over.
	//! @note it only shows values of a point where the mouse is over. The vertical lines for "now" and "transit" are currently unavailable.
	void showToolTip(const QPointF &point, bool show);

protected:
	//! Override. This sets the time from the click point to the main application.
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE;

private:
	//! Find range of dates for the respective series plot.
	//! Valid values for series are AltVsTime, AzVsTime, MonthlyElevation, LunarElongation. All other values are interpreted as 2-curve plots extending over the current year.
	//! @arg periods number of periods (defaults to 1; currently applicable for number of months in the 2-curves plot)
	QPair<QDateTime, QDateTime>findXRange(const double JD, const Series series, const int periods=1);

	//! Apply buffer to the determined required Y range of series
	//! This should add some space so that overshooting spline curves are not clipped off.
	//! @arg strictMin: The altVsTime plot may require strict minimum setting without buffer. Use false (default) for other plots.
	void bufferYrange(Series series, double *min, double *max, bool strictMin=false);

	QDateTimeAxis *xAxis; // running along bottom
	QValueAxis *yAxis; // running on the left side
	QValueAxis *yAxisR; // running on the right side for some charts only
	qreal yMin, yMax; // store range of Y values
	qreal yMinR, yMaxR; // store range of Y values for right axis

	//! The QMap holds enums and series for principally existing graphs. These can be made active by show(key)
	QMap<AstroCalcChart::Series, QLineSeries*> map;
	//! A simple map for line properties.
	static const QMap<AstroCalcChart::Series, QPen> penMap;
};

#endif // ASTROCALCCHART_H
