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

#include "AstroCalcChart.hpp"
#include "StelTranslator.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include <QGraphicsLayout>
#include <QDateTime>
#include <math.h>
#include <QDebug>
#include <QAbstractSeries>
#include <QLegendMarker>
#include <QPen>
#include <QColor>

AstroCalcChart::AstroCalcChart(QSet<Series> which) : QChart(), yAxisR(Q_NULLPTR), yMin(-90), yMax(90.)
{
	//qDebug() << "chart constructor";

	// Configure all series you want to potentially use later.
	for (Series s: which)
	{
		// To avoid "spline tremor" we need line series for azimuths (0/360° rollover) and magnitudes (in case of shadow transits)
		if (QList<Series>({CurrentTime, TransitTime, AzVsTime, RightAscension1, RightAscension2, Magnitude1, Magnitude2}).contains(s))
			map.insert(s, new QtCharts::QLineSeries(this));
			else
		map.insert(s,         new QtCharts::QSplineSeries(this));
	}

	setBackgroundBrush(QBrush(QColor(86, 87, 90)));
	AstroCalcChart::retranslate();

	xAxis=new QtCharts::QDateTimeAxis(this);
	yAxis=new QtCharts::QValueAxis(this);
	if (QSet<AstroCalcChart::Series>({AstroCalcChart::AngularSize2, AstroCalcChart::Declination2, AstroCalcChart::Distance2,
					  AstroCalcChart::Elongation2, AstroCalcChart::HeliocentricDistance2, AstroCalcChart::Magnitude2,
					  AstroCalcChart::PhaseAngle2, AstroCalcChart::Phase2, AstroCalcChart::RightAscension2, AstroCalcChart::TransitAltitude2,
					  AstroCalcChart::pcDistanceDeg}).intersect(which).count())
		yAxisR=new QtCharts::QValueAxis(this);
	if (QSet<AstroCalcChart::Series>({AstroCalcChart::pcDistanceAU, AstroCalcChart::pcDistanceDeg, AstroCalcChart::MonthlyElevation}).intersect(which).count())
	{
		legend()->hide();
	}
	else
	{
		legend()->setAlignment(Qt::AlignBottom);
		legend()->setLabelColor(Qt::white);
	}
	setTitleBrush(QBrush(Qt::white));
	setMargins(QMargins(2, 2, 2, 2)); // set to 0/0/0/0 for max space usage. This is between the title/axis labels and the enclosing QChartView.
	layout()->setContentsMargins(0, 0, 0, 0);
	setBackgroundRoundness(0); // remove rounded corners

	//qDebug() << "c'tor done";
}

AstroCalcChart::~AstroCalcChart()
{
	removeAllSeries(); // Also deletes!
}

void AstroCalcChart::retranslate(){
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
	if (map.contains(LunarElongation      )) map.value(LunarElongation      )->setName(q_("Lunar Elongation"));
	if (map.contains(LunarElongationLimit )) map.value(LunarElongationLimit )->setName(q_("Elongation Limit"));
	if (map.contains(pcDistanceAU         )) map.value(pcDistanceAU         )->setName(q_("Distance, AU"));
	if (map.contains(pcDistanceDeg        )) map.value(pcDistanceDeg        )->setName(q_("Distance, °"));
}

const QMap<AstroCalcChart::Series, QPen> AstroCalcChart::penMap=
{
	{AstroCalcChart::AltVsTime,         QPen(Qt::red,                             2, Qt::SolidLine)},
	{AstroCalcChart::CurrentTime,       QPen(Qt::yellow,                          2, Qt::SolidLine)},
	{AstroCalcChart::TransitTime,       QPen(Qt::cyan,                            2, Qt::SolidLine)},
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	{AstroCalcChart::SunElevation,      QPen(QColorConstants::Svg::orange,        2, Qt::SolidLine)},
	{AstroCalcChart::CivilTwilight,     QPen(QColorConstants::Svg::paleturquoise, 1, Qt::DashLine)},
	{AstroCalcChart::NauticalTwilight,  QPen(QColorConstants::Svg::dodgerblue,    1, Qt::DashDotLine)},
	{AstroCalcChart::Moon,              QPen(QColorConstants::Svg::springgreen,   2, Qt::DashLine)},
#else
	{AstroCalcChart::SunElevation,      QPen(QColor(0xffa500),                    2, Qt::SolidLine)},
	{AstroCalcChart::CivilTwilight,     QPen(QColor(0xafeeee),                    1, Qt::DashLine)},
	{AstroCalcChart::NauticalTwilight,  QPen(QColor(0x1e90ff),                    1, Qt::DashDotLine)},
	{AstroCalcChart::Moon,              QPen(QColor(0x00ff7f),                    2, Qt::DashLine)},
#endif
	{AstroCalcChart::AstroTwilight,          QPen(Qt::darkBlue,                   1, Qt::DashDotDotLine)},
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
	{AstroCalcChart::LunarElongation,        QPen(Qt::red,                        2, Qt::SolidLine)},
	{AstroCalcChart::LunarElongationLimit,   QPen(Qt::yellow,                     2, Qt::SolidLine)},
	{AstroCalcChart::pcDistanceAU,           QPen(Qt::green,                      2, Qt::SolidLine)},
	{AstroCalcChart::pcDistanceDeg,          QPen(Qt::yellow,                     2, Qt::SolidLine)}
};

void AstroCalcChart::append(Series s, qint64 x, qreal y)
{
	if (map.value(s))
		map.value(s)->append(qreal(x), y);
	else
		qWarning() << "Series " << s << "invalid for append()!";
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

		// It seems we must remove/add series again to force a redraw.
		if (index!=0)
		{
			//qDebug() << "replace force redraw and attach axes: Removing series" << s << "at index" << index;
			if (series().contains(map.value(s)))
				removeSeries(map.value(s));
			addSeries(map.value(s));
			if (axes(Qt::Horizontal).contains(xAxis))
				map.value(s)->attachAxis(xAxis);
			if (axes(Qt::Vertical).contains(yAxis))
				map.value(s)->attachAxis(yAxis);
			//qDebug() << "replace force redraw and attach axes...done";
		}
	}
	else
		qWarning() << "Series " << s << "invalid for replace()!";
}

void AstroCalcChart::drawTrivialLineX(Series s, const qreal x)
{
	if (map.value(s))
	{
		replace(s, 0, x, -180.); // Good for alt and azi plots. axis values may not be available yet.
		replace(s, 1, x, 360.);
		map.value(s)->setPen(penMap.value(s));
		//qDebug() << "Trivial X line in" << s << "at" << x << "from" << yAxis->min() << "to" << yAxis->max();
	}
	else
		qWarning() << "No series" << s << "to add trivial X line";
}

void AstroCalcChart::drawTrivialLineY(Series s, const qreal y)
{
	if (map.value(s))
	{
		replace(s, 0, qreal(StelUtils::jdToQDateTime(StelUtils::qDateTimeToJd(xAxis->min())).toMSecsSinceEpoch()), y);
		replace(s, 1, qreal(StelUtils::jdToQDateTime(StelUtils::qDateTimeToJd(xAxis->max())).toMSecsSinceEpoch()), y);
		map.value(s)->setPen(penMap.value(s));
		//qDebug() << "Trivial Y line in" << s << "at" << y << "from" << xAxis->min() << "to" << xAxis->max();
	}
	else
		qWarning() << "No series" << s << "to add trivial Y line";
}

int AstroCalcChart::lengthOfSeries(Series s)
{
	if (!(map.value(s)))
		return -1;
	else return map.value(s)->count();
}

void AstroCalcChart::show(Series s)
{
	//qDebug() << "About to add series " << s;
	if (!series().contains(map.value(s)))
	{
		addSeries(map.value(s));
	}
	else
		qDebug() << "series" << s << "already shown.";

	// Set up tooltips on hover for all series
	connect(map.value(s), SIGNAL(hovered(const QPointF &, bool)), this, SLOT(showToolTip(const QPointF &, bool)));
}

void AstroCalcChart::showToolTip(const QPointF &point, bool show)
{
	if (show)
	{
		QtCharts::QLineSeries *series=dynamic_cast<QtCharts::QLineSeries *>(sender());
		AstroCalcChart::Series seriesCode=map.key(series);
		QString units("°");
		QDateTime date=QDateTime::fromMSecsSinceEpoch(qint64(point.x()));
		// Change units where required. No units for distances, phases!
		if (QList<AstroCalcChart::Series>({AstroCalcChart::CurrentTime, AstroCalcChart::TransitTime,
						  AstroCalcChart::Distance1, AstroCalcChart::Distance2,
						  AstroCalcChart::HeliocentricDistance1, AstroCalcChart::HeliocentricDistance2,
						  AstroCalcChart::RightAscension1, AstroCalcChart::RightAscension2,
						  AstroCalcChart::Phase1, AstroCalcChart::Phase2, AstroCalcChart::pcDistanceAU}).contains(seriesCode))
		{
			units="";
		}
		setToolTip(QString("%1\n%2: %3%4").arg(date.toString("yyyy.MM.dd hh:mm"), series->name(), QString::number(point.y(), 'f', 3), units));
	}
	else
		setToolTip(QString());
}


void AstroCalcChart::clear(Series s)
{
	//qDebug() << "Clearing series " << s;
	//qDebug() << "Before remove it has " << map.value(s)->points().length() << "entries";
//	if (series().contains(map.value(s)))
//		removeSeries(map.value(s));
	//qDebug() << "clearing series with " << map.value(s)->points().length() << "entries";
	map.value(s)->clear();
	//qDebug() << "clear(): Removing series " << s;
	if (series().contains(map.value(s)))
		removeSeries(map.value(s)); // updates legend to not show entries
}

QPair<QDateTime, QDateTime> AstroCalcChart::findXRange(const double JD, const Series series, const int periods)
{
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
		case AstroCalcChart::LunarElongation:
			startDate=StelUtils::jdToQDateTime(JD-2);
			endDate=StelUtils::jdToQDateTime(JD+30);
			break;
		case AstroCalcChart::pcDistanceAU:
		case AstroCalcChart::pcDistanceDeg:
			startDate=StelUtils::jdToQDateTime(JD-300);
			endDate=StelUtils::jdToQDateTime(JD+300);
			break;
		default: // 2-curves page
			break;
	}
	return QPair(startDate, endDate);
}

QPair<double, double> AstroCalcChart::findYRange(const Series series)
{
	const QList<QPointF> points=map.value(series)->points();
	double min=std::numeric_limits<double>::max(), max=std::numeric_limits<double>::min();
	for (int i=0; i<points.count(); i++)
	{
		const double y=points.at(i).y();
		if (y<min) min=y;
		if (y>max) max=y;
	}
	return QPair<double, double>(min, max);
}

void AstroCalcChart::setupAxes(const double jd, const int periods, const QString &englishName)
{
	qDebug() << "setupAxes()...";

	static const QPen axisPen(          Qt::white,      1,    Qt::SolidLine);
	static const QPen axisGridPen(      Qt::white,      0.5,  Qt::SolidLine);
	static const QPen axisMinorGridPen( Qt::white,      0.35, Qt::DotLine);
	static const QPen axisPenL(         Qt::green,      1,    Qt::SolidLine);
	static const QPen axisGridPenL(     Qt::green,      0.5,  Qt::SolidLine);
	static const QPen axisMinorGridPenL(Qt::green,      0.35, Qt::DotLine);
	static const QPen axisPenR(         Qt::yellow,     1,    Qt::SolidLine);
	static const QPen axisGridPenR(     Qt::yellow,     0.5,  Qt::SolidLine);
	static const QPen axisMinorGridPenR(Qt::yellow,     0.35, Qt::DotLine);

	const double shift = StelApp::getInstance().getCore()->getUTCOffset(jd) / 24.0;
	//qDebug() << "Why is this shift not a full number of hours?: " << shift*24.;

	// Variables for scaling x axis
	QPair<QDateTime, QDateTime>xRange;
	// Maybe prefer to allow axis labeling from outside.
	xAxis->setTitleText(q_("Date")); // was Days from today, but real date is better. generally TODO: ticks on midnight, not "now".
	xAxis->setFormat("<div style=\"text-align:center\">dd.MM.</div>");
	if (map.contains(AstroCalcChart::AltVsTime) || map.contains(AstroCalcChart::AzVsTime))
	{
		xAxis->setTitleText(q_("Local Time"));
		xAxis->setTickCount(13); // step is 2 hours
		//xAxis->setFormat("<div style=\"text-align:center\">dd.MM.<br/>hh:mm</div>"); // This sets C locale with AM/PM hours! :-O
		xAxis->setFormat("dd.MM.<br/>hh:mm");
		xRange=findXRange(floor(jd+shift), AstroCalcChart::AltVsTime, 1);
	}
	else if (map.contains(AstroCalcChart::MonthlyElevation))
	{
		xAxis->setTickCount(13); // about monthly
		xRange=findXRange(jd, AstroCalcChart::MonthlyElevation, 1);
		xAxis->setFormat("<div style=\"text-align:center\">dd.<br/>MMM</div>");
	}
	else if (map.contains(AstroCalcChart::LunarElongation))
	{
		xAxis->setTickCount(17); // step is 2 days. 16 intervals.
		xRange=findXRange(jd, AstroCalcChart::LunarElongation, 1);
	}
	else if (map.contains(AstroCalcChart::pcDistanceAU))
	{
		xAxis->setTickCount(21); // step is 30 days. 20 intervals.
		xRange=findXRange(jd, AstroCalcChart::pcDistanceAU, 1);
	}
	else
	{
		xAxis->setTickCount(12+1); // step is ~30*periods days. We cannot have more, due to space reasons.
		xRange=findXRange(jd, AstroCalcChart::AngularSize1, periods);
		if (periods>1) xAxis->setFormat("<div style=\"text-align:center\">dd.MM.<br/>yyyy</div>");
	}
	xAxis->setRange(xRange.first, xRange.second);
//	qDebug() << "xAxis range is "  << xRange.first << "/" << xRange.second;

	if (map.contains(AstroCalcChart::AltVsTime) || map.contains(AstroCalcChart::MonthlyElevation))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Altitude"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::AzVsTime))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Azimuth"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::LunarElongation))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Lunar elongation"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::AngularSize1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Angular size"), (englishName=="Moon" || englishName=="Sun") ? QString("'") : QString("\"")));
	else if (map.contains(AstroCalcChart::Declination1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Declination"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::Distance1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Distance"), englishName=="Moon" ? qc_("Mm", "distance, Megameters") : qc_("AU", "distance, astronomical unit")));
	else if (map.contains(AstroCalcChart::Elongation1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Elongation"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::HeliocentricDistance1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Heliocentric distance"), qc_("AU", "distance, astronomical unit")));
	else if (map.contains(AstroCalcChart::Magnitude1))
	{
		yAxis->setTitleText(q_("Magnitude"));
		yAxis->setReverse();
	}
	else if (map.contains(AstroCalcChart::PhaseAngle1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Phase angle"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::Phase1))
		yAxis->setTitleText(QString("%1, %").arg(q_("Phase")));
	else if (map.contains(AstroCalcChart::RightAscension1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Right ascension"), qc_("h", "time")));
	else if (map.contains(AstroCalcChart::TransitAltitude1))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Culmination altitude"), QChar(0x00B0))); // Better term than transit!
	else if (map.contains(AstroCalcChart::pcDistanceAU))
		yAxis->setTitleText(QString("%1, %2").arg(q_("Linear Distance"), qc_("AU", "distance, astronomical unit")));

	if (map.contains(AstroCalcChart::AngularSize2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Angular size"), (englishName=="Moon" || englishName=="Sun") ? QString("'") : QString("\"")));
	else if (map.contains(AstroCalcChart::Declination2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Declination"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::Distance2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Distance"), englishName=="Moon" ? qc_("Mm", "distance, Megameters") : qc_("AU", "distance, astronomical unit")));
	else if (map.contains(AstroCalcChart::Elongation2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Elongation"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::HeliocentricDistance2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Heliocentric distance"), qc_("AU", "distance, astronomical unit")));
	else if (map.contains(AstroCalcChart::Magnitude2))
	{
		yAxisR->setTitleText(q_("Magnitude"));
		yAxisR->setReverse();
	}
	else if (map.contains(AstroCalcChart::PhaseAngle2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Phase angle"), QChar(0x00B0)));
	else if (map.contains(AstroCalcChart::Phase2))
		yAxisR->setTitleText(QString("%1, %").arg(q_("Phase")));
	else if (map.contains(AstroCalcChart::RightAscension2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Right ascension"), qc_("h", "time")));
	else if (map.contains(AstroCalcChart::TransitAltitude2))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Culmination altitude"), QChar(0x00B0))); // Better term than transit!
	else if (map.contains(AstroCalcChart::pcDistanceDeg))
		yAxisR->setTitleText(QString("%1, %2").arg(q_("Angular distance"), QChar(0x00B0)));

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

	// QChart axes are labeled bold by default. Set back to normal weight.
	QFont font=xAxis->titleFont();
	font.setBold(false);
	xAxis->setTitleFont(font);
	font=yAxis->titleFont();
	font.setBold(false);
	yAxis->setTitleFont(font);

	addAxis(xAxis, Qt::AlignBottom);
	addAxis(yAxis, Qt::AlignLeft);
	if (yAxisR)
	{
		yAxisR->setTitleBrush(Qt::yellow);
		yAxisR->setLabelsBrush(Qt::yellow);
		addAxis(yAxisR, Qt::AlignRight);
		yAxisR->setLinePen(axisPenR);
		yAxisR->setGridLinePen(axisGridPenR);
		yAxisR->setMinorGridLinePen(axisMinorGridPenR);

		// If we have a right axis, we always have green/yellow graphs!
		yAxis->setTitleBrush(Qt::green);
		yAxis->setLabelsBrush(Qt::green);
		yAxis->setLinePen(axisPenL);
		yAxis->setGridLinePen(axisGridPenL);
		yAxis->setMinorGridLinePen(axisMinorGridPenL);

		font=yAxisR->titleFont();
		font.setBold(false);
		yAxisR->setTitleFont(font);
	}
	const QList<QtCharts::QAbstractSeries *> ser=series(); // currently shown series. These may be fewer than the series in our map!

	for (Series s: {AltVsTime, CurrentTime, TransitTime, SunElevation, CivilTwilight, NauticalTwilight, AstroTwilight, Moon, AzVsTime, MonthlyElevation,
	     AngularSize1, Declination1, Distance1, Elongation1, HeliocentricDistance1, Magnitude1, PhaseAngle1, Phase1, RightAscension1, TransitAltitude1,
	     LunarElongation, LunarElongationLimit, pcDistanceAU})
	{
		if ((map.value(s)) && ser.contains(map.value(s)))
		{
			map.value(s)->setPen(penMap.value(s));
			map.value(s)->attachAxis(xAxis);
			map.value(s)->attachAxis(yAxis);
		}
	}
	for (Series s: {AngularSize2, Declination2, Distance2, Elongation2, HeliocentricDistance2, Magnitude2, PhaseAngle2, Phase2, RightAscension2, TransitAltitude2, pcDistanceDeg})
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

	// We have to find a scaling that has steps in an appropriate range.
	const double logYDiff=log10(yMax-yMin);
	const double s=pow(10., floor(logYDiff)-1.);

	qreal rMin=floor(yMin/s);
	qreal rMax=ceil(yMax/s);

	//qDebug() << "Setting yrange from" << min << "/" << max << "-->" << rMin*s << "/" << rMax*s;
	yAxis->setRange(rMin*s, rMax*s);
	//yAxis->applyNiceNumbers();
	if (rMax-rMin > 3*s)
	{
		yAxis->setTickCount(6+1);
		yAxis->setMinorTickCount(2);
	}
	else
	{
		yAxis->setTickCount(qRound((rMax*s-rMin*s)/s)+1);
		yAxis->setMinorTickCount(1);
	}
}

void AstroCalcChart::setYrangeR(qreal min, qreal max)
{
	yMinR=min;
	yMaxR=max;

	// We have to find a scaling that has steps in an appropriate range.
	const double logYDiff=log10(yMaxR-yMinR);
	const double s=pow(10., floor(logYDiff)-1.);

	qreal rMin=floor(yMinR/s);
	qreal rMax=ceil(yMaxR/s);

	//qDebug() << "Setting yrangeR from" << min << "/" << max << "-->" << rMin*s << "/" << rMax*s;
	yAxisR->setRange(rMin*s, rMax*s);
	//yAxisR->applyNiceNumbers();
	if (rMax-rMin > 3*s)
	{
		yAxis->setTickCount(6+1);
		yAxis->setMinorTickCount(2);
	}
	else
	{
		yAxis->setTickCount(qRound((rMax*s-rMin*s)/s)+1);
		yAxis->setMinorTickCount(1);
	}
}
