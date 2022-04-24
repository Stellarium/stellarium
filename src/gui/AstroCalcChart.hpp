/*
 * Stellarium
 * Copyright (C) 2019 Alexander Wolf
 * Copyright (C) 2022 Georg Zotti (QtCustomPlot->QtChart and this new class)
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
//using namespace QtCharts;

//! @class AstroCalcAltVsTimeChart
//! This class encapsulates data for the Altitude vs. Time Chart in AstroCalc.
//! The next iterations will try to re-use the chart in place of other plots.
class AstroCalcChart : public QtCharts::QChart
{
	Q_OBJECT

public:
	// Series names. Better than int numbers for the old plots.
	// This graph can be used in several scenarios. In 0.22 there are 6 plots which can be recreated by creating a chart which activates 1-2 lines of these:
	enum Series {AltVsTime, CurrentTime, TransitTime, SunElevation, CivilTwilight, NauticalTwilight, AstroTwilight, Moon,
		     AzVsTime,
		     MonthlyElevation,
		     AngularSize1, Declination1, Distance1, Elongation1, HeliocentricDistance1, Magnitude1, PhaseAngle1, Phase1, RightAscension1, TransitAltitude1,
		     AngularSize2, Declination2, Distance2, Elongation2, HeliocentricDistance2, Magnitude2, PhaseAngle2, Phase2, RightAscension2, TransitAltitude2,
		     LunarElongation, LunarElongationLimit
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

	//! Activate series s. It must have been filled using append(s, ., .) before.
	void show(Series s);


	//! set range of Y axis
	void setYrange(qreal min, qreal max);
	//! set range of Y axis on the right side
	void setYrangeR(qreal min, qreal max);

	//! Find range of dates for the respective series plot.
	//! Valid vlues for series are AltVsTime, AzVsTime, MonthlyElevation, LunarDistance. All other values are interpreted as 2-curve plots extending over the current year.
	//! @arg periods number of periods (defauts to 1; currently applicable for number of years in the 2-curves plot)
	QPair<QDateTime, QDateTime>findXRange(const double JD, const Series series, const int periods=1);
	//! Setup axes and appearance. Call this at the end of drawing, just before display.
	void setupAxes(const double jd, const int periods);

	//! Draw a 2-point vertical line at x which extends over the whole graph
	//! Note: This shall replace AstroCalcDialog::drawTransitTimeDiagram() and AstroCalcDialog::drawCurrentTimeDiagram()
	void drawTrivialLineX(Series s, const qreal x);

	//! Draw a 2-point horizontal line at y which extends over the whole graph
	//! Note: This shall replace AstroCalcDialog::drawAngularDistanceLimitLine()
	void drawTrivialLineY(Series s, const qreal y);

public slots:
	//virtual void retranslate();

private slots:
	//void setOptionStatus();

private:
	QtCharts::QDateTimeAxis *xAxis; // running along bottom
	//QtCharts::QDateTimeAxis *xAxis; // Maybe change to use this!
	QtCharts::QValueAxis *yAxis; // running on the left side
	QtCharts::QValueAxis *yAxisR; // running on the right side for some charts only
	qreal yMin, yMax; // store range of Y values
	qreal yMinR, yMaxR; // store range of Y values for right axis


	//! The QMap holds enums and series for principally existing graphs. These can be made active by show(key)
	QMap<AstroCalcChart::Series, QtCharts::QSplineSeries*> map;
	static const QMap<AstroCalcChart::Series, QPen> penMap;
};

#endif // ASTROCALCCHART_H
