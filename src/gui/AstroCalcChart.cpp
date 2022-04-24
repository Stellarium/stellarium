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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include <QDateTime>
#include <math.h>
#include <QDebug>
#include <QAbstractSeries>
#include <QPen>
#include <QColor>

AstroCalcChart::AstroCalcChart(QSet<Series> which) : QChart(), yAxisR(Q_NULLPTR), yMin(-90), yMax(90.)
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

	xAxis=new QtCharts::QDateTimeAxis(this);
	yAxis=new QtCharts::QValueAxis(this);
	if (QSet<AstroCalcChart::Series>({AstroCalcChart::AngularSize2, AstroCalcChart::Declination2, AstroCalcChart::Distance2,
					  AstroCalcChart::Elongation2, AstroCalcChart::HeliocentricDistance2, AstroCalcChart::Magnitude2,
					  AstroCalcChart::PhaseAngle2, AstroCalcChart::Phase2, AstroCalcChart::RightAscension2, AstroCalcChart::TransitAltitude2}).intersect(which).count())
		yAxisR=new QtCharts::QValueAxis(this);
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

void AstroCalcChart::append(Series s, qint64 x, qreal y)
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

void AstroCalcChart::drawTrivialLine(Series s, const qreal x)
{
	if (map.value(s))
	{
		//replace(s, 0, x, yAxis->min());
		//replace(s, 1, x, yAxis->max());
		replace(s, 0, x, -180.);
		replace(s, 1, x, 360.);
		map.value(s)->setPen(penMap.value(s));
		qDebug() << "Trivial line in " << s << "from " << yAxis->min() << "to" << yAxis->max();
	}
	else
		qDebug() << "No series" << s << "to add trivial line";
}



void AstroCalcChart::show(Series s)
{
	qDebug() << "About to add series " << s;
	if (!series().contains(map.value(s)))
	{
		addSeries(map.value(s));
		map.value(s)->attachAxis(xAxis);

		if (QList<AstroCalcChart::Series>({AstroCalcChart::AngularSize2, AstroCalcChart::Declination2, AstroCalcChart::Distance2,
						  AstroCalcChart::Elongation2, AstroCalcChart::HeliocentricDistance2, AstroCalcChart::Magnitude2,
						  AstroCalcChart::PhaseAngle2, AstroCalcChart::Phase2, AstroCalcChart::RightAscension2, AstroCalcChart::TransitAltitude2}).contains(s))
				map.value(s)->attachAxis(yAxisR);
		else
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

QPair<QDateTime, QDateTime> AstroCalcChart::findXRange(const double JD, const Series series, const int periods)
{
	//QtCharts::QSplineSeries *s;
	QDateTime startDate, endDate;
	switch (series){
		case AstroCalcChart::AltVsTime:
		case AstroCalcChart::AzVsTime:
			startDate=StelUtils::jdToQDateTime(JD);
			endDate=StelUtils::jdToQDateTime(JD+1);
			break;
		case AstroCalcChart::MonthlyElevation:
			int year, month, day;
			StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
			startDate=QDateTime(QDate(year, 1, 1), QTime(0, 0)); // TODO Work out timezone stuff
			endDate=QDateTime(QDate(year+1, 1, 1), QTime(0, 0)); // TODO Work out timezone stuff
			break;
		case AstroCalcChart::LunarDistance:
			break;
		default: // 2-curves page
			break;
	}
	return QPair(startDate, endDate);
}

void AstroCalcChart::setupAxes(const double jd, const int periods)
{
	qDebug() << "setupAxes()...";

	static const QPen axisPen(       Qt::white,                         1,    Qt::SolidLine);
	static const QPen axisGridPen(   Qt::white,                         0.5,  Qt::SolidLine);
	static const QPen axisMinorGridPen(Qt::white,                       0.35, Qt::DotLine);

	const double shift = StelApp::getInstance().getCore()->getUTCOffset(jd) / 24.0;
	qDebug() << "Why is thisshift not a full number of hours?: " << shift*24.;

	// Variables for scaling x axis
	//QtCharts::QSplineSeries *s;
	//double jdMin=0., jdMax=0.;
	//QDateTime startDate, endDate;
	QPair<QDateTime, QDateTime>xRange;
	// Maybe prefer to allow axis labeling from outside.
	if (map.contains(AstroCalcChart::AltVsTime) || map.contains(AstroCalcChart::AzVsTime))
	{
		//s=map.value(AstroCalcChart::AltVsTime, map.value(AstroCalcChart::AzVsTime));
		xAxis->setTitleText(q_("Local Time"));
		//xAxis->setRange(43200, 129600); // 24 hours since 12h00m (range in seconds)
		const double noon=floor(jd+shift);
		//double ltime=-5*180+43200;
		//jdMin= noon-shift; // noon+ltime/86400.-shift-0.5;
		//ltime=485*180+43200;
		//jdMax= jdMin+1.; // noon+ltime/86400.-shift-0.5;

		xAxis->setTickCount(13); // step is 2 hours
		//xAxis->setMinorTickCount(1); // substep is 1 hours. Unfortunately this axis type has no subticks.
		//xAxisRange=findXRange(jd, AstroCalcChart::AltVsTime, 1);
		xAxis->setFormat("dd\nh:mm");
		//setLocale(QLocale(localeMgr->getAppLanguage()));
		xRange=findXRange(floor(jd+shift), AstroCalcChart::AltVsTime, 1);
	}
	else if (map.contains(AstroCalcChart::MonthlyElevation))
	{
		//s=map.value(AstroCalcChart::MonthlyElevation);
		xAxis->setTitleText(q_("Date"));
		//xAxis->setRange(s->at(0).x(), s->at(s->count()-1).x()); // TODO-range in unknown units
		//qDebug() << "xAxis range is "  << s->at(0).x() << "/" << s->at(s->count()-1).x();
		xAxis->setTickCount(13); // about monthly
		xAxis->setFormat("dd.MM.");
		//xAxis->setMinorTickCount(2); // substep is 1 hours
		xRange=findXRange(jd, AstroCalcChart::MonthlyElevation, 1);
	}
	else if (map.contains(AstroCalcChart::LunarDistance))
	{
		//s=map.value(AstroCalcChart::LunarDistance);
		xAxis->setTitleText(q_("Days from today"));
		//xAxis->setRange(-2, 32); // 24 hours since 12h00m (range in seconds)
		xAxis->setTickCount(17); // step is 2 days
		//xAxis->setMinorTickCount(1); // substep is 1 day
		xRange=findXRange(jd, AstroCalcChart::LunarDistance, 1);
	}
	else
	{
		xRange=findXRange(jd, AstroCalcChart::AngularSize1, periods);
	}
	//startDate.setMSecsSinceEpoch(s->at(0).x());
	//endDate.setMSecsSinceEpoch(s->at(s->count()-1).x());
	//xAxis->setRange(startDate, endDate); // 24 hours since 12h00m (range in seconds)
	xAxis->setRange(xRange.first, xRange.second);
//	qDebug() << "xAxis range is "  << s->at(0).x() << "/" << s->at(s->count()-1).x();
//	qDebug() << "xAxis range is "  << jdMin << "/" << jdMax << "=" << StelUtils::julianDayToISO8601String(jdMin, true) << "/" << StelUtils::julianDayToISO8601String(jdMax, true);


	if (map.contains(AstroCalcChart::AltVsTime) || map.contains(AstroCalcChart::MonthlyElevation))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Altitude"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::AzVsTime))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Azimuth"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::LunarDistance))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Angular distance"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::AngularSize1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Angular size"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::Declination1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Declination"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::Distance1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Distance"), qc_("AU", "distance, astronomical unit")));
	else if (map.contains(AstroCalcChart::Elongation1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Elongation"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::HeliocentricDistance1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Heliocentric distance"), qc_("AU", "distance, astronomical unit")));
	else if (map.contains(AstroCalcChart::Magnitude1))
		yAxis->setTitleText(q_("Magnitude"));
	else if (map.contains(AstroCalcChart::PhaseAngle1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Phase angle"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::Phase1))
		yAxis->setTitleText(QString("%1, %").arg(q_("Phase")));
	else if (map.contains(AstroCalcChart::RightAscension1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Right ascension"), qc_("h", "time")));
	else if (map.contains(AstroCalcChart::TransitAltitude1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Transit altitude"), QChar(0x00B0)));

	if (map.contains(AstroCalcChart::AngularSize2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Angular size"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::Declination2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Declination"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::Distance2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Distance"), qc_("AU", "distance, astronomical unit")));
	else if (map.contains(AstroCalcChart::Elongation2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Elongation"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::HeliocentricDistance2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Heliocentric distance"), qc_("AU", "distance, astronomical unit")));
	else if (map.contains(AstroCalcChart::Magnitude2))
		yAxisR->setTitleText(q_("Magnitude"));
	else if (map.contains(AstroCalcChart::PhaseAngle2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Phase angle"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::Phase2))
		yAxisR->setTitleText(QString("%1, %").arg(q_("Phase")));
	else if (map.contains(AstroCalcChart::RightAscension2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Right ascension"), qc_("h", "time")));
	else if (map.contains(AstroCalcChart::TransitAltitude2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Transit altitude"), QChar(0x00B0)));

	xAxis->setTitleBrush(Qt::white);
	yAxis->setTitleBrush(Qt::white);
	xAxis->setLabelsBrush(Qt::white);
	yAxis->setLabelsBrush(Qt::white);
	xAxis->setLinePen(axisPen);
	xAxis->setGridLinePen(axisGridPen);
	xAxis->setMinorGridLinePen(axisMinorGridPen);

	setYrange(yMin, yMax);
	yAxis->setLinePen(axisPen);
	yAxis->setGridLinePen(axisGridPen);
	yAxis->setMinorGridLinePen(axisMinorGridPen);

	addAxis(xAxis, Qt::AlignBottom);
	addAxis(yAxis, Qt::AlignLeft);
	if (yAxisR)
		addAxis(yAxisR, Qt::AlignRight);

	const QList<QtCharts::QAbstractSeries *> ser=series(); // currently shown series. These may be fewer than the series in our map!

	for (Series s: {AltVsTime, CurrentTime, TransitTime, SunElevation, CivilTwilight, NauticalTwilight, AstroTwilight, Moon, AzVsTime, MonthlyElevation,
	     AngularSize1, Declination1, Distance1, Elongation1, HeliocentricDistance1, Magnitude1, PhaseAngle1, Phase1, RightAscension1, TransitAltitude1,
	     LunarDistance, DistanceLimit})
	{
		if ((map.value(s)) && ser.contains(map.value(s)))
		{
			map.value(s)->setPen(penMap.value(s));
			map.value(s)->attachAxis(xAxis);
			map.value(s)->attachAxis(yAxis);
		}
	}
	for (Series s: {AngularSize2, Declination2, Distance2, Elongation2, HeliocentricDistance2, Magnitude2, PhaseAngle2, Phase2, RightAscension2, TransitAltitude2})
	{
		if ((map.value(s)) && ser.contains(map.value(s)))
		{
			map.value(s)->setPen(penMap.value(s));
			map.value(s)->attachAxis(xAxis);
			map.value(s)->attachAxis(yAxisR);
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

	if (rMax-rMin > 30)
	{
		qDebug() << "Setting yrange from" << min << "/" << max << "-->" << rMin*10 << "/" << rMax*10;
		yAxis->setRange(rMin*10, rMax*10);
		//yAxis->applyNiceNumbers();
		yAxis->setTickCount(6+1);
		yAxis->setMinorTickCount(2);
	}
	else
	{
		qDebug() << "Setting yrange from" << min << "/" << max << "-->" << rMin*10 << "/" << rMax*10;
		yAxis->setRange(rMin*10, rMax*10);
		//yAxis->applyNiceNumbers();
		yAxis->setTickCount(qRound((rMax*10-rMin*10)/10.)+1);
		yAxis->setMinorTickCount(1);
	}
}

void AstroCalcChart::setYrangeR(qreal min, qreal max)
{
	yMinR=min;
	yMaxR=max;

	qreal rMin=floor(yMinR/10);
	qreal rMax=ceil(yMaxR/10);

	qDebug() << "Setting yrangeR from" << min << "/" << max << "-->" << rMin*10 << "/" << rMax*10;
	yAxisR->setRange(rMin*10, rMax*10);
	//yAxis->applyNiceNumbers();
	yAxisR->setTickCount(qRound((rMax*10-rMin*10)/10.)+1);
	yAxisR->setMinorTickCount(1);
}
