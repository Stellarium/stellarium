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

#include "AstroCalcChart.hpp"
#include "StelTranslator.hpp"
#include <math.h>
#include <QDebug>
#include <QAbstractSeries>
#include <QPen>
#include <QColor>

AstroCalcChart::AstroCalcChart(QList<Series> which) : QChart(), yAxisR(Q_NULLPTR), yMin(-90), yMax(90.)
{
	qDebug() << "chart constructor";

	// Configure all series you want to potentially use later.
	for (Series s: which)
	{
		map.insert(s,         new QtCharts::QSplineSeries(this));
	}

	setBackgroundBrush(QBrush(QColor(86, 87, 90)));

	// We need to configure every enum Series here!
	if (map.contains(AltVsTime            )) map.value(AltVsTime            )->setName(q_("Altitude")); // no name in old solution, but used for legend!
	if (map.contains(CurrentTime          )) map.value(CurrentTime          )->setName(q_("Now"));
	if (map.contains(TransitTime          )) map.value(TransitTime          )->setName(q_("Culmination"));
	if (map.contains(SunElevation         )) map.value(SunElevation         )->setName(q_("Sun"));
	if (map.contains(CivilTwilight        )) map.value(CivilTwilight        )->setName(q_("Civil Twilight"));
	if (map.contains(NauticalTwilight     )) map.value(NauticalTwilight     )->setName(q_("Nautical Twilight"));
	if (map.contains(AstroTwilight        )) map.value(AstroTwilight        )->setName(q_("Astronomical Twilight"));
	if (map.contains(Moon                 )) map.value(Moon                 )->setName(q_("Moon"));
	if (map.contains(AzVsTime             )) map.value(AzVsTime             )->setName(q_("Azimuth"));
	if (map.contains(MonthlyElevation     )) map.value(MonthlyElevation     )->setName(q_("Monthly Elevation"));
	if (map.contains(AngularSize1         )) map.value(AngularSize1         )->setName(q_("Angular Size"));
	if (map.contains(Declination1         )) map.value(Declination1         )->setName(q_("Declination"));
	if (map.contains(Distance1            )) map.value(Distance1            )->setName(q_("Distance"));
	if (map.contains(Elongation1          )) map.value(Elongation1          )->setName(q_("Elongation"));
	if (map.contains(HeliocentricDistance1)) map.value(HeliocentricDistance1)->setName(q_("Heliocentric Distance"));
	if (map.contains(Magnitude1           )) map.value(Magnitude1           )->setName(q_("Magnitude"));
	if (map.contains(PhaseAngle1          )) map.value(PhaseAngle1          )->setName(q_("Phase angle"));
	if (map.contains(Phase1               )) map.value(Phase1               )->setName(q_("Phase"));
	if (map.contains(RightAscension1      )) map.value(RightAscension1      )->setName(q_("Right Ascension"));
	if (map.contains(TransitAltitude1     )) map.value(TransitAltitude1     )->setName(q_("Transit Altitude"));
	if (map.contains(AngularSize2         )) map.value(AngularSize2         )->setName(q_("Angular Size"));
	if (map.contains(Declination2         )) map.value(Declination2         )->setName(q_("Declination"));
	if (map.contains(Distance2            )) map.value(Distance2            )->setName(q_("Distance"));
	if (map.contains(Elongation2          )) map.value(Elongation2          )->setName(q_("Elongation"));
	if (map.contains(HeliocentricDistance2)) map.value(HeliocentricDistance2)->setName(q_("Heliocentric Distance"));
	if (map.contains(Magnitude2           )) map.value(Magnitude2           )->setName(q_("Magnitude"));
	if (map.contains(PhaseAngle2          )) map.value(PhaseAngle2          )->setName(q_("Phase angle"));
	if (map.contains(Phase2               )) map.value(Phase2               )->setName(q_("Phase"));
	if (map.contains(RightAscension2      )) map.value(RightAscension2      )->setName(q_("Right Ascension"));
	if (map.contains(TransitAltitude2     )) map.value(TransitAltitude2     )->setName(q_("Transit Altitude"));
	if (map.contains(LunarDistance        )) map.value(LunarDistance        )->setName(q_("Lunar Elongation"));
	if (map.contains(DistanceLimit        )) map.value(DistanceLimit        )->setName(q_("Elongation Limit"));

	xAxis=new QtCharts::QValueAxis(this);
	yAxis=new QtCharts::QValueAxis(this);
	legend()->setAlignment(Qt::AlignBottom);
	legend()->setLabelColor(Qt::white);
	setTitleBrush(QBrush(Qt::white));

	qDebug() << "c'tor done";
}

AstroCalcChart::~AstroCalcChart()
{
	removeAllSeries(); // Also deletes!
}

//void AstroCalcAltVsTimeChart::retranslate(){}

const QMap<AstroCalcChart::Series, QPen> AstroCalcChart::penMap=
{
	{AstroCalcChart::AltVsTime,         QPen(Qt::red,                             2, Qt::SolidLine)},
	{AstroCalcChart::CurrentTime,       QPen(Qt::yellow,                          2, Qt::SolidLine)},
	{AstroCalcChart::TransitTime,       QPen(Qt::cyan,                            2, Qt::SolidLine)},
	{AstroCalcChart::SunElevation,      QPen(QColorConstants::Svg::orange,        2, Qt::SolidLine)},
	{AstroCalcChart::CivilTwilight,     QPen(QColorConstants::Svg::paleturquoise, 1, Qt::DashLine)},
	{AstroCalcChart::NauticalTwilight,  QPen(QColorConstants::Svg::dodgerblue,    1, Qt::DashDotLine)},
	{AstroCalcChart::AstroTwilight,     QPen(Qt::darkBlue,                        1, Qt::DashDotDotLine)},
	{AstroCalcChart::Moon,              QPen(QColorConstants::Svg::springgreen,   2, Qt::DashLine)},
	{AstroCalcChart::AzVsTime,               QPen(Qt::red,                        2, Qt::SolidLine)},
	{AstroCalcChart::MonthlyElevation,       QPen(Qt::red,                        2, Qt::SolidLine)},
	{AstroCalcChart::AngularSize1,           QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::Declination1,           QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::Distance1,              QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::Elongation1,            QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::HeliocentricDistance1,  QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::Magnitude1,             QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::PhaseAngle1,            QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::Phase1,                 QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::RightAscension1,        QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::TransitAltitude1,       QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::AngularSize2,           QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::Declination2,           QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::Distance2,              QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::Elongation2,            QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::HeliocentricDistance2,  QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::Magnitude2,             QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::PhaseAngle2,            QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::Phase2,                 QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::RightAscension2,        QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::TransitAltitude2,       QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::LunarDistance,          QPen(Qt::red,                        2, Qt::SolidLine)},
	{AstroCalcChart::DistanceLimit,          QPen(Qt::yellow,                     2, Qt::SolidLine)}
};

void AstroCalcChart::append(Series s, qreal x, qreal y)
{
	if (map.value(s))
		map.value(s)->append(x, y);
	else
		qDebug() << "Series " << s << "invalid for append()!";
}

void AstroCalcChart::replace(Series s, int index, qreal x, qreal y)
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

void AstroCalcChart::show(Series s)
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

void AstroCalcChart::clear(Series s)
{
	qDebug() << "Clearing series " << s;
	qDebug() << "Before remove it has " << map.value(s)->points().length() << "entries";
//	if (series().contains(map.value(s)))
//		removeSeries(map.value(s));
	qDebug() << "clearing series with " << map.value(s)->points().length() << "entries";
	map.value(s)->clear();
	removeSeries(map.value(s)); // updates legend to not show entries

}


void AstroCalcChart::setupAxes()
{
	qDebug() << "setupAxes()...";

	static const QPen axisPen(       Qt::white,                         1,    Qt::SolidLine);
	static const QPen axisGridPen(   Qt::white,                         0.5,  Qt::SolidLine);
	static const QPen axisMinorGridPen(Qt::white,                       0.35, Qt::DotLine);

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

	const QList<QtCharts::QAbstractSeries *> ser=series(); // currently shown series. These may be fewer than the series in our map!

	for (Series s: {AltVsTime, CurrentTime, TransitTime, SunElevation, CivilTwilight, NauticalTwilight, AstroTwilight, Moon})
	{
		if ((map.value(s)) && ser.contains(map.value(s)))
		{
			map.value(s)->setPen(penMap.value(s));
			map.value(s)->attachAxis(xAxis);
			map.value(s)->attachAxis(yAxis);
		}
	}

	qDebug() << "setupAxes()...done";
}

void AstroCalcChart::setYrange(qreal min, qreal max)
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
