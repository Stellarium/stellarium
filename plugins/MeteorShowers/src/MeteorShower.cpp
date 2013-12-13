/*
 * Copyright (C) 2013 Marcos Cardinot
 * Copyright (C) 2011 Alexander Wolf
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

#include "MeteorShower.hpp"
#include "Meteor.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelModuleMgr.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QOpenGLFunctions>
#include <QVariantMap>
#include <QVariant>
#include <QList>

StelTextureSP MeteorShower::radiantTexture;

MeteorShower::MeteorShower(const QVariantMap& map)
	: initialized(false)
{
	// return initialized if the mandatory fields are not present
	if(!map.contains("showerID"))
		return;

	showerID = map.value("showerID").toString();
	designation  = map.value("designation").toString();
	speed = map.value("speed").toInt();
	radiantAlpha = StelUtils::getDecAngle(map.value("radiantAlpha").toString());
	radiantDelta = StelUtils::getDecAngle(map.value("radiantDelta").toString());
	driftAlpha = StelUtils::getDecAngle(map.value("driftAlpha").toString());
	driftDelta = StelUtils::getDecAngle(map.value("driftDelta").toString());
	parentObj = map.value("parentObj").toString();
	pidx = map.value("pidx").toFloat();

	if(map.contains("activity"))
	{
		foreach(const QVariant &ms, map.value("activity").toList())
		{
			QVariantMap activityMap = ms.toMap();
			activityData d;
			d.year = activityMap.value("year").toString();
			d.zhr = activityMap.value("zhr").toInt();
			d.variable = activityMap.value("variable").toString();
			d.peak = activityMap.value("peak").toString();
			d.start = activityMap.value("start").toString();
			d.finish = activityMap.value("finish").toString();
			activity.append(d);
		}
	}

	initialized = true;
}

MeteorShower::~MeteorShower()
{
	//
}

QVariantMap MeteorShower::getMap(void)
{
	QVariantMap map;
	map["showerID"] = showerID;
	map["designation"] = designation;
	map["speed"] = speed;
	map["radiantAlpha"] = radiantAlpha;
	map["radiantDelta"] = radiantDelta;
	map["driftAlpha"] = driftAlpha;
	map["driftDelta"] = driftDelta;
	map["parentObj"] = parentObj;
	map["pidx"] = pidx;

	QVariantList activityList;
	foreach(const activityData &p, activity)
	{
		QVariantMap activityMap;
		activityMap["year"] = p.year;
		activityMap["zhr"] = p.zhr;
		activityMap["variable"] = p.variable;
		activityMap["start"] = p.start;
		activityMap["finish"] = p.finish;
		activityMap["peak"] = p.peak;
		activityList << activityMap;
	}
	map["activity"] = activityList;

	return map;
}

float MeteorShower::getSelectPriority(const StelCore*) const
{
	return 5.0;
}

QString MeteorShower::getDesignation() const
{
	return designation;
}

QString MeteorShower::getDateFromJSON(QString jsondate) const
{
	QStringList parsedDate = jsondate.split(".");

	return QString("%1 %2").arg(parsedDate.at(1).toInt()).arg(getMonthName(parsedDate.at(0).toInt()));
}

QString MeteorShower::getDayFromJSON(QString jsondate) const
{
	QStringList parsedDate = jsondate.split(".");

	return QString("%1").arg(parsedDate.at(1).toInt());
}

int MeteorShower::getMonthFromJSON(QString jsondate) const
{
	QStringList parsedDate = jsondate.split(".");

	return parsedDate.at(0).toInt();
}

QString MeteorShower::getMonthNameFromJSON(QString jsondate) const
{
	QStringList parsedDate = jsondate.split(".");

	return QString("%1").arg(getMonthName(parsedDate.at(0).toInt()));
}

QString MeteorShower::getMonthName(int number) const
{
	QStringList monthList;
	monthList.append(N_("January"));
	monthList.append(N_("February"));
	monthList.append(N_("March"));
	monthList.append(N_("April"));
	monthList.append(N_("May"));
	monthList.append(N_("June"));
	monthList.append(N_("July"));
	monthList.append(N_("August"));
	monthList.append(N_("September"));
	monthList.append(N_("October"));
	monthList.append(N_("November"));
	monthList.append(N_("December"));

	return q_(monthList.at(number-1));
}

int MeteorShower::isActive() const
{
	StelCore* core = StelApp::getInstance().getCore();

	//get the current sky date
	double JD = core->getJDay();
	QDateTime skyDate = StelUtils::jdToQDateTime(JD+StelUtils::getGMTShiftFromQT(JD)/24-core->getDeltaT(JD)/86400);

	//Check if we have real data for the current sky year
	int index = checkYear(skyDate.toString("yyyy"));

	QString dateStart = activity[index].start.isEmpty() ? activity[0].start : activity[index].start;
	QString dateFinish = activity[index].finish.isEmpty() ? activity[0].finish : activity[index].finish;
	QString yearBase = activity[index].year == "generic" ? skyDate.toString("yyyy") : activity[index].year;
	QString yearS, yearF;

	int monthStart = getMonthFromJSON(dateStart);

	if(monthStart > getMonthFromJSON(dateFinish))
	{
		if(monthStart == skyDate.toString("MM").toInt())
		{
			yearS = yearBase;
			yearF = QString("%1").arg(yearBase.toInt() + 1);
		}
		else
		{
			yearS = QString("%1").arg(yearBase.toInt() - 1);
			yearF = yearBase;
		}
	}
	else
	{
		yearS = yearF = yearBase;
	}

	QDateTime start = QDateTime::fromString(dateStart + " " + yearS, "MM.dd yyyy");
	QDateTime finish = QDateTime::fromString(dateFinish + " " + yearF, "MM.dd yyyy");

	if(skyDate.operator >=(start) && skyDate.operator <=(finish))
	{
		if(index)
			return 1; // real data
		else
			return 2; // generic data
	}

	return 0; // isn't active
}

int MeteorShower::checkYear(QString yyyy) const
{
	int index = -1;
	foreach(const activityData &p, activity)
	{
		index++;
		if(p.year == yyyy)
			return index;
	}

	return 0;
}

QString MeteorShower::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if(flags&Name)
		oss << "<h2>" << getNameI18n() << "</h2>";

	if(flags&Extra)
		oss << q_("Type: <b>%1</b>").arg(q_("meteor shower")) << "<br />";

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if(flags&Extra)
	{
		oss << QString("%1: %2/%3")
		    .arg(q_("Radiant drift"))
		    .arg(StelUtils::radToHmsStr(driftAlpha))
		    .arg(StelUtils::radToDmsStr(driftDelta));
		oss << "<br />";

		oss << q_("Geocentric meteoric velocity: %1 km/s").arg(speed) << "<br />";
		if(pidx>0)
		{
			oss << q_("The population index: %1").arg(pidx) << "<br />";
		}

		if(!parentObj.isEmpty())
		{
			oss << q_("Parent body: %1").arg(parentObj) << "<br />";
		}

		double JD = core->getJDay();
		QString skyYear = StelUtils::jdToQDateTime(JD+StelUtils::getGMTShiftFromQT(JD)/24-core->getDeltaT(JD)/86400).toString("yyyy");

		if(activity.size() > 0)
		{
			int index = checkYear(skyYear);

			if(index == 0)
				oss << "<b>" << q_("Generic data for the year %1").arg(skyYear) << "</b> <br />";
			else
				oss << "<b>" << q_("Real data for the year %1").arg(activity[index].year) << "</b> <br />";

			QString c_start = activity[index].start.isEmpty() ? activity[0].start : activity[index].start;
			QString c_finish = activity[index].finish.isEmpty() ? activity[0].finish : activity[index].finish;
			QString c_peak = activity[index].peak.isEmpty() ? activity[0].peak : activity[index].peak;
			int c_zhr = activity[index].zhr;

			if(getMonthNameFromJSON(c_start)==getMonthNameFromJSON(c_finish))
			{
				oss << QString("%1: %2 - %3 %4")
				    .arg(q_("Active"))
				    .arg(getDayFromJSON(c_start))
				    .arg(getDayFromJSON(c_finish))
				    .arg(getMonthNameFromJSON(c_finish));
			}
			else
			{
				oss << QString("%1: %2 - %3")
				    .arg(q_("Active"))
				    .arg(getDateFromJSON(c_start))
				    .arg(getDateFromJSON(c_finish));
			}
			oss << "<br />";
			oss << q_("Maximum: %1").arg(getDateFromJSON(c_peak));
			oss << "<br />";
			if(c_zhr>0)
			{
				oss << QString("ZHR<sub>max</sub>: %1").arg(c_zhr) << "<br />";
			}
			else
			{
				oss << QString("ZHR<sub>max</sub>: %1").arg(q_("variable"));
				if(!activity[index].variable.isEmpty())
				{
					oss << "; " << activity[index].variable;
				}
				oss << "<br />";
			}
		}
	}

	postProcessInfoString(str, flags);

	return str;
}

Vec3f MeteorShower::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : Vec3f(1.0, 1.0, 1.0);
}

double MeteorShower::getAngularSize(const StelCore*) const
{
	return 0.001;
}

void MeteorShower::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void MeteorShower::draw(StelPainter& painter)
{
	StelUtils::spheToRect(radiantAlpha, radiantDelta, XYZ);
	painter.getProjector()->project(XYZ, XY);

	Vec3f color = getInfoColor();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	float alpha = 0.85f + ((double) rand() / (RAND_MAX))/10;

	switch(isActive())
	{
	case 1: //Active, real data
		painter.setColor(1.0f, 0.94f, 0.0f, alpha);
		break;
	case 2: //Active, generic data
		painter.setColor(0.0f, 1.0f, 0.94f, alpha);
		break;
	default: //Inactive
		painter.setColor(1.0f, 1.0f, 1.0f, alpha);
	}

	MeteorShower::radiantTexture->bind();
	painter.drawSprite2dMode(XY[0], XY[1], 10);

	float size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
	float shift = 8.f + size/1.8f;
	painter.setColor(color[0], color[1], color[2], 1.0f);
	painter.drawText(XY[0]+shift, XY[1]+shift, getNameI18n(), 0, 0, 0, false);
	painter.setColor(1,1,1,0);
}
