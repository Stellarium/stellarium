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

#include "AstroCalcAltVsTimeChart.hpp"
#include "StelTranslator.hpp"
#include "qdebug.h"
#include <QAbstractSeries>



AstroCalcAltVsTimeChart::AstroCalcAltVsTimeChart() : QChart()
{
	qDebug() << "chart constructor";
	altVsTime        = new QtCharts::QSplineSeries(this);
	currentTime      = new QtCharts::QLineSeries(this);
	transitTime      = new QtCharts::QLineSeries(this);
	sunElevation     = new QtCharts::QSplineSeries(this);
	civilTwilight    = new QtCharts::QSplineSeries(this);
	nauticalTwilight = new QtCharts::QSplineSeries(this);
	astroTwilight    = new QtCharts::QSplineSeries(this);
	moon             = new QtCharts::QSplineSeries(this);

	map={
		{AltVsTime,        dynamic_cast<QtCharts::QLineSeries*>(altVsTime)},
		{CurrentTime,      currentTime},
		{TransitTime,      transitTime},
		{SunElevation,     dynamic_cast<QtCharts::QLineSeries*>(sunElevation)},
		{CivilTwilight,    dynamic_cast<QtCharts::QLineSeries*>(civilTwilight)},
		{NauticalTwilight, dynamic_cast<QtCharts::QLineSeries*>(nauticalTwilight)},
		{AstroTwilight,    dynamic_cast<QtCharts::QLineSeries*>(astroTwilight)},
		{Moon,             dynamic_cast<QtCharts::QLineSeries*>(moon)}
	};
//	setTitle(q_("Altitude vs. Time"));
	setBackgroundBrush(QBrush(QColor(86, 87, 90)));

	qDebug() << "2";
	// Do not add empty series here!
	//addSeries(altVsTime);
	//addSeries(currentTime);
	//addSeries(transitTime);
	//addSeries(sunElevation);
	//addSeries(civilTwilight);
	//addSeries(nauticalTwilight);
	//addSeries(astroTwilight);
	//addSeries(moon);

	altVsTime       ->setName("alt"); // no name in old solution. Maybe remove again.
	currentTime     ->setName("[Now]");
	transitTime     ->setName("[Transit]");
	sunElevation    ->setName("[Sun]");
	civilTwilight   ->setName("[Civil Twilight]");
	nauticalTwilight->setName("[Nautical Twilight]");
	astroTwilight   ->setName("[Astronomical Twilight]");
	moon            ->setName("[Moon]");
	//createDefaultAxes(); // Not sure if possible with empty series?

	qDebug() << "3";

	setupAxes();
	qDebug() << "c'tor done";

}

AstroCalcAltVsTimeChart::~AstroCalcAltVsTimeChart()
{
	removeAllSeries(); // Also deletes!
	//delete altVsTime;
	//delete currentTime;
	//delete transitTime;
	//delete sunElevation;
	//delete civilTwilight;
	//delete nauticalTwilight;
	//delete astroTwilight;
	//delete moon;

}

//void AstroCalcAltVsTimeChart::retranslate(){}

void AstroCalcAltVsTimeChart::append(Series s, qreal x, qreal y)
{
	map.value(s)->append(x, y);
}

void AstroCalcAltVsTimeChart::show(Series s)
{
	addSeries(map.value(s));
	// TODO: Adjust axis?
}

void AstroCalcAltVsTimeChart::clear(Series s)
{
	removeSeries(map.value(s));
	map.value(s)->clear();
}


void AstroCalcAltVsTimeChart::setupAxes()
{
	qDebug() << "setupAxes()...";
	xAxis=new QtCharts::QValueAxis(this);
	yAxis=new QtCharts::QValueAxis(this);
	QString xAxisStr = q_("Local Time");
	QString yAxisStr = QString("%1, %2").arg(q_("Altitude"), QChar(0x00B0));

	static const QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);

	//xAxis->setLabel(xAxisStr);
	//yAxis->setLabel(yAxisStr);
	xAxis->setRange(43200, 129600); // 24 hours since 12h00m (range in seconds)
	xAxis->setTickCount(12); // step is 2 hours

	yAxis->setRange(yMin, yMax);
	//yAxis->setTickCount(10);
	yAxis->applyNiceNumbers();

	addAxis(xAxis, Qt::AlignBottom);
	addAxis(yAxis, Qt::AlignLeft);

	const QList<QtCharts::QAbstractSeries *> ser=series();

	if ((altVsTime)        && ser.contains(altVsTime))       { altVsTime->attachAxis(xAxis);         altVsTime->attachAxis(yAxis); }
	if ((currentTime)      && ser.contains(currentTime))     { currentTime->attachAxis(xAxis);       currentTime->attachAxis(yAxis); }
	if ((transitTime)      && ser.contains(transitTime))     { transitTime->attachAxis(xAxis);       transitTime->attachAxis(yAxis); }
	if ((sunElevation)     && ser.contains(sunElevation))    { sunElevation->attachAxis(xAxis);      sunElevation->attachAxis(yAxis); }
	if ((civilTwilight)    && ser.contains(civilTwilight))   { civilTwilight->attachAxis(xAxis);     civilTwilight->attachAxis(yAxis); }
	if ((nauticalTwilight) && ser.contains(nauticalTwilight)){ nauticalTwilight->attachAxis(xAxis);  nauticalTwilight->attachAxis(yAxis); }
	if ((astroTwilight)    && ser.contains(astroTwilight))   { astroTwilight->attachAxis(xAxis);     astroTwilight->attachAxis(yAxis); }
	if ((moon)             && ser.contains(moon))            { moon->attachAxis(xAxis);              moon->attachAxis(yAxis); }
	qDebug() << "setupAxes()...done";
}

void AstroCalcAltVsTimeChart::setYrange(qreal min, qreal max)
{
	yMin=min;
	yMax=max;
	yAxis->setRange(yMin, yMax);
	yAxis->applyNiceNumbers();
}
