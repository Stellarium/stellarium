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

#ifndef ASTROCALCALTVSTIMECHART_HPP
#define ASTROCALCALTVSTIMECHART_HPP


#include <QChart>
#include <QSplineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>
//using namespace QtCharts;

//! @class AstroCalcAltVsTimeChart
//! This class encapsulates data for the Altitude vs. Time Chart in AstroCalc.
class AstroCalcAltVsTimeChart : public QtCharts::QChart
{
	Q_OBJECT

public:
	// Series names. Better than int numbers for the old plots.
	enum Series {AltVsTime, CurrentTime, TransitTime, SunElevation, CivilTwilight, NauticalTwilight, AstroTwilight, Moon};
	Q_ENUM(Series)

	AstroCalcAltVsTimeChart();
	virtual ~AstroCalcAltVsTimeChart() Q_DECL_OVERRIDE;
	//! Setup axes and appearance
	void setupAxes();

	//! Append one pair of data values to series s.
	void append(Series s, qreal x, qreal y);
	//! Replace one pair of data values to series s at position index.
	void replace(Series s, int index, qreal x, qreal y);
	//! Reset (delete and create new empty) series s
	void clear(Series s);

	//! Activate series s. It must have been filled using append(s, ., .) before.
	void show(Series s);


	//! set range of Y axis
	void setYrange(qreal min, qreal max);

public slots:
	//virtual void retranslate();

private slots:
	//void setOptionStatus();

private:
	QtCharts::QSplineSeries *altVsTime;
	QtCharts::QLineSeries *currentTime;
	QtCharts::QLineSeries *transitTime;
	QtCharts::QSplineSeries *sunElevation;
	QtCharts::QSplineSeries *civilTwilight;
	QtCharts::QSplineSeries *nauticalTwilight;
	QtCharts::QSplineSeries *astroTwilight;
	QtCharts::QSplineSeries *moon;

	QtCharts::QValueAxis *xAxis;
	//QtCharts::QDateTimeAxis *xAxis; // Maybe change to use this!
	QtCharts::QValueAxis *yAxis;
	qreal yMin, yMax;


	QMap<AstroCalcAltVsTimeChart::Series, QtCharts::QLineSeries*> map; //! Make access to series easier.
};

#endif // ASTROCALCALTVSTIMECHART_HPP
