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
#include <math.h>
#include <QDebug>
#include <QAbstractSeries>
#include <QPen>


AstroCalcAltVsTimeChart::AstroCalcAltVsTimeChart() : QChart(), yMin(-90), yMax(90.)
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
	setBackgroundBrush(QBrush(QColor(86, 87, 90)));

	altVsTime       ->setName(q_("Altitude")); // no name in old solution, but used for legend!
	currentTime     ->setName(q_("Now"));
	transitTime     ->setName(q_("Culmination"));
	sunElevation    ->setName(q_("Sun"));
	civilTwilight   ->setName(q_("Civil Twilight"));
	nauticalTwilight->setName(q_("Nautical Twilight"));
	astroTwilight   ->setName(q_("Astronomical Twilight"));
	moon            ->setName(q_("Moon"));

	xAxis=new QtCharts::QValueAxis(this);
	yAxis=new QtCharts::QValueAxis(this);
	legend()->setAlignment(Qt::AlignBottom);
	legend()->setLabelColor(Qt::white);
	setTitleBrush(QBrush(Qt::white));

	qDebug() << "c'tor done";
}

AstroCalcAltVsTimeChart::~AstroCalcAltVsTimeChart()
{
	removeAllSeries(); // Also deletes!
}

//void AstroCalcAltVsTimeChart::retranslate(){}

void AstroCalcAltVsTimeChart::append(Series s, qreal x, qreal y)
{
	if (map.value(s))
		map.value(s)->append(x, y);
	else
		qDebug() << "Series " << s << "invalid for append()!";
}

void AstroCalcAltVsTimeChart::replace(Series s, int index, qreal x, qreal y)
{
	if (map.value(s))
	{
		if (map.value(s)->points().length() >= index+1)
		{
			//qDebug() << "Replacing: Series" << s << " point " << index << ":" << x << "/" << y;
			map.value(s)->replace(index, x, y);
		}
		else
			map.value(s)->append(x, y);

	}
	else
		qDebug() << "Series " << s << "invalid for replace()!";

	// It seems we must remove/add series again to force a redraw.
	if (index!=0)
	{
		//qDebug() << "replace force redraw...";
		removeSeries(map.value(s));
		addSeries(map.value(s));
		map.value(s)->attachAxis(xAxis);
		map.value(s)->attachAxis(yAxis);
		//qDebug() << "replace force redraw...done";
	}
}

void AstroCalcAltVsTimeChart::show(Series s)
{
	qDebug() << "About to add series " << s;
	if (!series().contains(map.value(s)))
	{
		addSeries(map.value(s));
		map.value(s)->attachAxis(xAxis);
		map.value(s)->attachAxis(yAxis);
	}
	else
		qDebug() << "series" << s << "already shown.";
}

void AstroCalcAltVsTimeChart::clear(Series s)
{
	qDebug() << "Clearing series " << s;
	qDebug() << "Before remove it has " << map.value(s)->points().length() << "entries";
//	if (series().contains(map.value(s)))
//		removeSeries(map.value(s));
	qDebug() << "clearing series with " << map.value(s)->points().length() << "entries";
	map.value(s)->clear();
}


void AstroCalcAltVsTimeChart::setupAxes()
{
	qDebug() << "setupAxes()...";

	static const QPen axisPen(       Qt::white,                         1,    Qt::SolidLine);
	static const QPen axisGridPen(   Qt::white,                         0.5,  Qt::SolidLine);
	static const QPen axisMinorGridPen(Qt::white,                       0.35, Qt::DotLine);
	static const QPen altPen(        Qt::red,                           2, Qt::SolidLine);
	static const QPen currentTimePen(Qt::yellow,                        2, Qt::SolidLine);
	static const QPen transitTimePen(Qt::cyan,                          2, Qt::SolidLine);
	static const QPen sunPen(        QColorConstants::Svg::orange,      2, Qt::SolidLine);
	static const QPen civilPen(      QColorConstants::Svg::paleturquoise, 1, Qt::DashLine);
	static const QPen nauticalPen(   QColorConstants::Svg::dodgerblue,   1, Qt::DashDotLine);
	static const QPen astroPen(      Qt::darkBlue,                      1, Qt::DashDotDotLine);
	static const QPen moonPen(       QColorConstants::Svg::springgreen, 2, Qt::DashLine);

	xAxis->setTitleText(q_("Local Time"));
	yAxis->setTitleText(QString("%1, %2").arg(q_("Altitude"), QChar(0x00B0)));
	xAxis->setTitleBrush(Qt::white);
	yAxis->setTitleBrush(Qt::white);
	xAxis->setLabelsBrush(Qt::white);
	yAxis->setLabelsBrush(Qt::white);
	xAxis->setLinePen(axisPen);
	xAxis->setGridLinePen(axisGridPen);
	xAxis->setMinorGridLinePen(axisMinorGridPen);
	xAxis->setRange(43200, 129600); // 24 hours since 12h00m (range in seconds)
	xAxis->setTickCount(9); // step is 3 hours
	xAxis->setMinorTickCount(2); // substep is 1 hours

	setYrange(yMin, yMax);
	yAxis->setLinePen(axisPen);
	yAxis->setGridLinePen(axisGridPen);
	yAxis->setMinorGridLinePen(axisMinorGridPen);

	addAxis(xAxis, Qt::AlignBottom);
	addAxis(yAxis, Qt::AlignLeft);

	const QList<QtCharts::QAbstractSeries *> ser=series();

	if ((altVsTime)        && ser.contains(altVsTime))       { altVsTime       ->setPen(altPen);         altVsTime->attachAxis(xAxis);         altVsTime->attachAxis(yAxis); }
	if ((currentTime)      && ser.contains(currentTime))     { currentTime     ->setPen(currentTimePen); currentTime->attachAxis(xAxis);       currentTime->attachAxis(yAxis); }
	if ((transitTime)      && ser.contains(transitTime))     { transitTime     ->setPen(transitTimePen); transitTime->attachAxis(xAxis);       transitTime->attachAxis(yAxis); }
	if ((sunElevation)     && ser.contains(sunElevation))    { sunElevation    ->setPen(sunPen);         sunElevation->attachAxis(xAxis);      sunElevation->attachAxis(yAxis); }
	if ((civilTwilight)    && ser.contains(civilTwilight))   { civilTwilight   ->setPen(civilPen);       civilTwilight->attachAxis(xAxis);     civilTwilight->attachAxis(yAxis); }
	if ((nauticalTwilight) && ser.contains(nauticalTwilight)){ nauticalTwilight->setPen(nauticalPen);    nauticalTwilight->attachAxis(xAxis);  nauticalTwilight->attachAxis(yAxis); }
	if ((astroTwilight)    && ser.contains(astroTwilight))   { astroTwilight   ->setPen(astroPen);       astroTwilight->attachAxis(xAxis);     astroTwilight->attachAxis(yAxis); }
	if ((moon)             && ser.contains(moon))            { moon            ->setPen(moonPen);        moon->attachAxis(xAxis);              moon->attachAxis(yAxis); }
	qDebug() << "setupAxes()...done";
}

void AstroCalcAltVsTimeChart::setYrange(qreal min, qreal max)
{
	yMin=min;
	yMax=max;

	qreal rMin=floor(yMin/10);
	qreal rMax=ceil(yMax/10);

	qDebug() << "Setting yrange from" << min << "/" << max << "-->" << rMin*10 << "/" << rMax*10;
	yAxis->setRange(rMin*10, rMax*10);
	//yAxis->applyNiceNumbers();
	yAxis->setTickCount(qRound((rMax*10-rMin*10)/10.)+1);
	yAxis->setMinorTickCount(1);
}
