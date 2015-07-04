/*
 * Stellarium: Meteor Showers Plug-in
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
#include "MeteorShowers.hpp"
#include "SporadicMeteorMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"

#include <QtMath>

StelTextureSP MeteorShower::radiantTexture;
bool MeteorShower::showLabels = true;
bool MeteorShower::radiantMarkerEnabled = true;
bool MeteorShower::showActiveRadiantsOnly = true;

MeteorShower::MeteorShower(const QVariantMap& map)
	: initialized(false)
	, active(false)
	, speed(0)
	, rAlphaPeak(0)
	, rDeltaPeak(0)
	, driftAlpha(0)
	, driftDelta(0)
	, pidx(0)
	, radiantAlpha(0)
	, radiantDelta(0)
	, zhr(0)
	, status(0)
{
	// return initialized if the mandatory fields are not present
	if(!map.contains("showerID"))
	{
		return;
	}

	showerID = map.value("showerID").toString();
	designation  = map.value("designation").toString();
	speed = map.value("speed").toInt();
	radiantAlpha = StelUtils::getDecAngle(map.value("radiantAlpha").toString());
	radiantDelta = StelUtils::getDecAngle(map.value("radiantDelta").toString());
	driftAlpha = StelUtils::getDecAngle(map.value("driftAlpha").toString());
	driftDelta = StelUtils::getDecAngle(map.value("driftDelta").toString());
	parentObj = map.value("parentObj").toString();
	pidx = map.value("pidx").toFloat();

	rAlphaPeak = radiantAlpha;
	rDeltaPeak = radiantDelta;

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

	if(map.contains("colors"))
	{
		foreach(const QVariant &ms, map.value("colors").toList())
		{
			QVariantMap colorMap = ms.toMap();
			QString color = colorMap.value("color").toString();
			int intensity = colorMap.value("intensity").toInt();
			colors.append(Meteor::colorPair(color, intensity));
		}
	}

	StelCore* core = StelApp::getInstance().getCore();
	updateCurrentData(getSkyQDateTime(core));
	// ensures that all objects will be drawn once
	// that's to avoid crashes by trying select a nonexistent object
	draw(core);

	qsrand(QDateTime::currentMSecsSinceEpoch());

	initialized = true;
}

MeteorShower::~MeteorShower()
{
}

QVariantMap MeteorShower::getMap()
{
	QVariantMap map;
	map["showerID"] = showerID;
	map["designation"] = designation;
	map["speed"] = speed;
	map["radiantAlpha"] = rAlphaPeak;
	map["radiantDelta"] = rDeltaPeak;
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

	QVariantList colorList;
	foreach(const Meteor::colorPair &c, colors)
	{
		QVariantMap colorMap;
		colorMap["color"] = c.first;
		colorMap["intensity"] = c.second;
		colorList << colorMap;
	}
	map["colors"] = colorList;

	return map;
}

float MeteorShower::getSelectPriority(const StelCore*) const
{
	return -4.0;
}

QString MeteorShower::getDesignation() const
{
	if (showerID.toInt()) // if showerID is a number
	{
		return "";
	}
	return showerID;
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

QDateTime MeteorShower::getSkyQDateTime(StelCore* core) const
{
	//get the current sky date
	double JD = core->getJDay();
	return StelUtils::jdToQDateTime(JD+StelUtils::getGMTShiftFromQT(JD)/24-core->getDeltaT(JD)/86400);
}

void MeteorShower::updateCurrentData(QDateTime skyDate)
{
	//Check if we have real data for the current sky year
	int index = searchRealData(skyDate.toString("yyyy"));

	/**************************
	 *ZHR info
	 **************************/
	zhr = activity[index].zhr == 0 ? activity[0].zhr : activity[index].zhr;

	if (zhr == -1)
	{
		variable = activity[index].variable.isEmpty() ? activity[0].variable : activity[index].variable;
	}
	else
	{
		variable = "";
	}

	/***************************
	 *Dates - start/finish/peak
	 ***************************/
	QString dateStart = activity[index].start.isEmpty() ? activity[0].start : activity[index].start;
	QString dateFinish = activity[index].finish.isEmpty() ? activity[0].finish : activity[index].finish;
	QString datePeak = activity[index].peak.isEmpty() ? activity[0].peak : activity[index].peak;
	QString yearBase = activity[index].year == "generic" ? skyDate.toString("yyyy") : activity[index].year;
	QString yearS, yearF;

	int monthStart = getMonthFromJSON(dateStart);
	int monthFinish = getMonthFromJSON(dateFinish);

	if(monthStart > monthFinish)
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

	start = QDateTime::fromString(dateStart + " " + yearS, "MM.dd yyyy");
	finish = QDateTime::fromString(dateFinish + " " + yearF, "MM.dd yyyy");
	peak = QDateTime::fromString(datePeak + " " + yearS, "MM.dd yyyy");

	if (peak.operator <(start) || peak.operator >(finish))
		peak = QDateTime::fromString(datePeak + " " + yearF, "MM.dd yyyy");

	/***************************
	 *Activity - is active?
	 ***************************/
	if(skyDate.operator >=(start) && skyDate.operator <=(finish))
	{
		if(index)
			status = ACTIVE_REAL; // real data
		else
			status = ACTIVE_GENERIC; // generic data
	}
	else
	{
		status = INACTIVE; // isn't active
	}
	active = (status != INACTIVE) || !showActiveRadiantsOnly;

	/**************************
	 *Radiant drift
	 *************************/
	radiantAlpha = rAlphaPeak;
	radiantDelta = rDeltaPeak;

	if (status != INACTIVE)
	{
		double time = (StelUtils::qDateTimeToJd(skyDate) - StelUtils::qDateTimeToJd(peak))*24;
		radiantAlpha += (driftAlpha/120)*time;
		radiantDelta += (driftDelta/120)*time;
	}
}

int MeteorShower::searchRealData(QString yyyy) const
{
	int index = -1;
	foreach(const activityData &p, activity)
	{
		index++;
		if(p.year == yyyy)
		{
			return index;
		}
	}

	return 0;
}

float MeteorShower::getSolarLongitude(QDateTime QDT) const
{
	//The number of days (positive or negative) since Greenwich noon,
	//Terrestrial Time, on 1 January 2000 (J2000.0)
	double n = StelUtils::qDateTimeToJd(QDT) - 2451545.0;

	//The mean longitude of the Sun, corrected for the aberration of light
	float slong = (280.460 + 0.9856474*n) / 360;
	slong = (slong - (int) slong) * 360 - 1;

	return slong;
}

QString MeteorShower::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	QString mstdata = q_("generic data");
	if(status == ACTIVE_REAL)
		mstdata = q_("real data");

	if(flags&Name)
	{
		oss << "<h2>" << getNameI18n();
		if (!showerID.toInt())
		{
			oss << " (" << showerID  <<")</h2>";
		}
		else
		{
			oss << "</h2>";
		}
	}

	if(flags&Extra)
	{
		oss << q_("Type: <b>%1</b> (%2)").arg(q_("meteor shower"), mstdata) << "<br />";
	}

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if(flags&Extra)
	{
		oss << QString("%1: %2/%3")
			.arg(q_("Radiant drift (per day)"))
			.arg(StelUtils::radToHmsStr(driftAlpha/5))
			.arg(StelUtils::radToDmsStr(driftDelta/5));
		oss << "<br />";

		if (speed>0)
		{
			oss << q_("Geocentric meteoric velocity: %1 km/s").arg(speed) << "<br />";
		}

		if(pidx>0)
		{
			oss << q_("The population index: %1").arg(pidx) << "<br />";
		}

		if(!parentObj.isEmpty())
		{
			oss << q_("Parent body: %1").arg(q_(parentObj)) << "<br />";
		}

		if(start.toString("M") == finish.toString("M"))
		{
			oss << QString("%1: %2 - %3 %4")
			       .arg(q_("Active"))
			       .arg(start.toString("d"))
			       .arg(finish.toString("d"))
			       .arg(start.toString("MMMM"));
		}
		else
		{
			oss << QString("%1: %2 - %3")
			       .arg(q_("Activity"))
			       .arg(start.toString("d MMMM"))
			       .arg(finish.toString("d MMMM"));
		}
		oss << "<br />";
		oss << q_("Maximum: %1").arg(peak.toString("d MMMM"));

		QString slong = QString::number( MeteorShower::getSolarLongitude(peak), 'f', 2 );
		oss << QString(" (%1 %2&deg;)").arg(q_("Solar longitude is")).arg(slong);
		oss << "<br />";

		if(zhr>0)
		{
			oss << QString("ZHR<sub>max</sub>: %1").arg(zhr) << "<br />";
		}
		else
		{
			oss << QString("ZHR<sub>max</sub>: %1").arg(q_("variable"));
			if(!variable.isEmpty())
			{
				oss << "; " << variable;
			}
			oss << "<br />";
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
	StelCore* core = StelApp::getInstance().getCore();

	updateCurrentData(getSkyQDateTime(core));

	// step through and update all active meteors
	foreach (MeteorObj* m, activeMeteors)
	{
		if (!m->update(deltaTime))
		{
			activeMeteors.removeOne(m);
		}
	}

	double tspeed = core->getTimeRate() * 86400;  // sky seconds per actual second
	if(tspeed<0 || fabs(tspeed)>1.)
	{
		return; // don't create new meteors
	}

	int currentZHR = calculateZHR(zhr, variable, start, finish, peak);

	// determine average meteors per frame needing to be created
	int mpf = (int) ((double) currentZHR * ZHR_TO_WSR * deltaTime / 1000.0 + 0.5);
	if (mpf < 1)
	{
		mpf = 1;
	}

	for (int i = 0; i < mpf; ++i)
	{
		// start new meteor based on ZHR time probability
		double prob = ((double) qrand()) / RAND_MAX;
		double aux = (double) currentZHR * ZHR_TO_WSR * deltaTime / 1000.0 / (double) mpf;
		if (currentZHR > 0 && prob < aux)
		{
			MeteorObj *m = new MeteorObj(core, speed, radiantAlpha, radiantDelta, pidx,
					colors, GETSTELMODULE(MeteorShowers)->getBolideTexture());
			activeMeteors.append(m);
		}
	}
}

void MeteorShower::draw(StelCore* core)
{
	StelPainter painter(core->getProjection(StelCore::FrameJ2000));
	StelUtils::spheToRect(radiantAlpha, radiantDelta, XYZ);
	painter.getProjector()->project(XYZ, XY);
	painter.setFont(GETSTELMODULE(MeteorShowers)->getLabelFont());

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	qreal r, g, b;
	float alpha = 0.85f + ((double) qrand() / (RAND_MAX))/10;
	switch(status)
	{
		case ACTIVE_REAL: //Active, real data
			GETSTELMODULE(MeteorShowers)->getColorARR().getRgbF(&r,&g,&b);
			break;
		case ACTIVE_GENERIC: //Active, generic data
			GETSTELMODULE(MeteorShowers)->getColorARG().getRgbF(&r,&g,&b);
			break;
		default: //Inactive
			GETSTELMODULE(MeteorShowers)->getColorIR().getRgbF(&r,&g,&b);
	}

	painter.setColor(r, g, b, alpha);

	Vec3d win;
	if (MeteorShower::radiantMarkerEnabled && painter.getProjector()->projectCheck(XYZ, win))
	{
		MeteorShower::radiantTexture->bind();
		painter.drawSprite2dMode(XY[0], XY[1], 45);

		if (MeteorShower::showLabels)
		{
			float size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
			float shift = 8.f + size/1.8f;
			painter.drawText(XY[0]+shift, XY[1]+shift, getNameI18n(), 0, 0, 0, false);
		}
	}

	// step through and draw all active meteors
	painter.setProjector(core->getProjection(StelCore::FrameAltAz));
	foreach (MeteorObj* m, activeMeteors)
	{
		m->draw(core, painter);
	}
}

int MeteorShower::calculateZHR(int zhr, QString variable, QDateTime start, QDateTime finish, QDateTime peak)
{
	/***************************************
	 * Get ZHR ranges
	 ***************************************/
	int highZHR;
	int lowZHR;
	//bool multPeak = false; //multiple peaks
	if (zhr != -1)  //isn't variable
	{
		highZHR = zhr;
		lowZHR = 0;
	}
	else
	{
		QStringList varZHR = variable.split("-");
		lowZHR = varZHR.at(0).toInt();
		if (varZHR.at(1).contains("*"))
		{
			//multPeak = true;
			highZHR = varZHR[1].replace("*", "").toInt();
		}
		else
		{
			highZHR = varZHR.at(1).toInt();
		}
	}

	/***************************************
	 * Convert time intervals
	 ***************************************/
	double startJD = StelUtils::qDateTimeToJd(start);
	double finishJD = StelUtils::qDateTimeToJd(finish);
	double peakJD = StelUtils::qDateTimeToJd(peak);
	double currentJD = StelUtils::qDateTimeToJd(GETSTELMODULE(MeteorShowers)->getSkyDate());

	/***************************************
	 * Gaussian distribution
	 ***************************************/
	double sd; //standard deviation
	if (currentJD >= startJD && currentJD < peakJD) //left side of gaussian
		sd = (peakJD - startJD)/2;
	else
		sd = (finishJD - peakJD)/2;

	double gaussian = highZHR * qExp( - qPow(currentJD - peakJD, 2) / (sd*sd) ) + lowZHR;

	return (int) ((int) ((gaussian - (int) gaussian) * 10) >= 5 ? gaussian+1 : gaussian);
}
