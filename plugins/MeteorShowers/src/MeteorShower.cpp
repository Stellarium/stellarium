/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013-2015 Marcos Cardinot
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

#include <QtMath>

#include "MeteorShower.hpp"
#include "MeteorShowers.hpp"
#include "SporadicMeteorMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"

MeteorShower::MeteorShower(const QVariantMap& map)
	: m_status(INVALID)
	, m_enabled(false)
	, m_speed(0)
	, m_rAlphaPeak(0)
	, m_rDeltaPeak(0)
	, m_driftAlpha(0)
	, m_driftDelta(0)
	, m_pidx(0)
	, m_radiantAlpha(0)
	, m_radiantDelta(0)
	, m_zhr(0)
{
	// return initialized if the mandatory fields are not present
	if(!map.contains("showerID"))
	{
		return;
	}

	m_showerID = map.value("showerID").toString();
	m_designation  = map.value("designation").toString();
	m_speed = map.value("speed").toInt();
	m_radiantAlpha = StelUtils::getDecAngle(map.value("radiantAlpha").toString());
	m_radiantDelta = StelUtils::getDecAngle(map.value("radiantDelta").toString());
	m_driftAlpha = StelUtils::getDecAngle(map.value("driftAlpha").toString());
	m_driftDelta = StelUtils::getDecAngle(map.value("driftDelta").toString());
	m_parentObj = map.value("parentObj").toString();
	m_pidx = map.value("pidx").toFloat();

	m_rAlphaPeak = m_radiantAlpha;
	m_rDeltaPeak = m_radiantDelta;

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
			m_activity.append(d);
		}
	}

	if(map.contains("colors"))
	{
		foreach(const QVariant &ms, map.value("colors").toList())
		{
			QVariantMap colorMap = ms.toMap();
			QString color = colorMap.value("color").toString();
			int intensity = colorMap.value("intensity").toInt();
			m_colors.append(Meteor::colorPair(color, intensity));
		}
	}

	StelCore* core = StelApp::getInstance().getCore();
	updateCurrentData(getSkyQDateTime(core));

	qsrand(QDateTime::currentMSecsSinceEpoch());
}

MeteorShower::~MeteorShower()
{
}

void MeteorShower::update(double deltaTime)
{
	if (!m_enabled)
	{
		return;
	}

	StelCore* core = StelApp::getInstance().getCore();

	updateCurrentData(getSkyQDateTime(core));

	// step through and update all active meteors
	foreach (MeteorObj* m, m_activeMeteors)
	{
		if (!m->update(deltaTime))
		{
			m_activeMeteors.removeOne(m);
		}
	}

	double tspeed = core->getTimeRate() * 86400;  // sky seconds per actual second
	if(tspeed < 0. || fabs(tspeed) > 1.)
	{
		return; // don't create new meteors
	}

	int currentZHR = calculateZHR(m_zhr, m_variable, m_start, m_finish, m_peak);

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
			MeteorObj *m = new MeteorObj(core, m_speed, m_radiantAlpha, m_radiantDelta, m_pidx,
					m_colors, GETSTELMODULE(MeteorShowers)->getBolideTexture());
			m_activeMeteors.append(m);
		}
	}
}

void MeteorShower::updateCurrentData(QDateTime skyDate)
{
	//Check if we have real data for the current sky year
	int index = searchRealData(skyDate.toString("yyyy"));

	/**************************
	 *ZHR info
	 **************************/
	m_zhr = m_activity[index].zhr == 0 ? m_activity[0].zhr : m_activity[index].zhr;

	if (m_zhr == -1)
	{
		m_variable = m_activity[index].variable.isEmpty() ? m_activity[0].variable : m_activity[index].variable;
	}
	else
	{
		m_variable = "";
	}

	/***************************
	 *Dates - start/finish/peak
	 ***************************/
	QString dateStart = m_activity[index].start.isEmpty() ? m_activity[0].start : m_activity[index].start;
	QString dateFinish = m_activity[index].finish.isEmpty() ? m_activity[0].finish : m_activity[index].finish;
	QString datePeak = m_activity[index].peak.isEmpty() ? m_activity[0].peak : m_activity[index].peak;
	QString yearBase = m_activity[index].year == "generic" ? skyDate.toString("yyyy") : m_activity[index].year;
	QString yearS, yearF;

	int monthStart = dateStart.split(".").at(0).toInt();
	int monthFinish = dateFinish.split(".").at(0).toInt();

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

	m_start = QDateTime::fromString(dateStart + " " + yearS, "MM.dd yyyy");
	m_finish = QDateTime::fromString(dateFinish + " " + yearF, "MM.dd yyyy");
	m_peak = QDateTime::fromString(datePeak + " " + yearS, "MM.dd yyyy");

	if (m_peak.operator <(m_start) || m_peak.operator >(m_finish))
		m_peak = QDateTime::fromString(datePeak + " " + yearF, "MM.dd yyyy");

	/***************************
	 *Activity - is active?
	 ***************************/
	if(skyDate.operator >=(m_start) && skyDate.operator <=(m_finish))
	{
		if(index)
			m_status = ACTIVE_REAL; // real data
		else
			m_status = ACTIVE_GENERIC; // generic data
	}
	else
	{
		m_status = INACTIVE; // isn't active
	}
	// TODO: m_enabled = (m_status != INACTIVE) || !showActiveRadiantsOnly;

	/**************************
	 *Radiant drift
	 *************************/
	m_radiantAlpha = m_rAlphaPeak;
	m_radiantDelta = m_rDeltaPeak;

	if (m_status != INACTIVE)
	{
		double time = (StelUtils::qDateTimeToJd(skyDate) - StelUtils::qDateTimeToJd(m_peak))*24;
		m_radiantAlpha += (m_driftAlpha/120)*time;
		m_radiantDelta += (m_driftDelta/120)*time;
	}
}

void MeteorShower::draw(StelCore* core)
{
	if (!m_enabled)
	{
		return;
	}

	MeteorShowersMgr* mgr = GETSTELMODULE(MeteorShowers);
	StelPainter painter(core->getProjection(StelCore::FrameJ2000));

	Vec3d XY;
	StelUtils::spheToRect(m_radiantAlpha, m_radiantDelta, m_position);
	painter.getProjector()->project(m_position, XY);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	Vec3f rgb;
	float alpha = 0.85f + ((double) qrand() / (RAND_MAX))/10;
	switch(m_status)
	{
		case ACTIVE_REAL: //Active, real data
			rgb = mgr->getColorARR();
			break;
		case ACTIVE_GENERIC: //Active, generic data
			rgb = mgr->getColorARG();
			break;
		default: //Inactive
			rgb = mgr->getColorIR();
	}
	rgb /= 255.f;
	painter.setColor(rgb[0], rgb[1], rgb[2], alpha);

	Vec3d win;
	if (mgr->getEnableMarker() && painter.getProjector()->projectCheck(m_position, win))
	{
		mgr->getRadiantTexture()->bind();
		painter.drawSprite2dMode(XY[0], XY[1], 45);

		if (mgr->getEnableLabels())
		{
			painter.setFont(mgr->getFont());
			float size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
			float shift = 8.f + size/1.8f;
			painter.drawText(XY[0]+shift, XY[1]+shift, getNameI18n(), 0, 0, 0, false);
		}
	}

	// step through and draw all active meteors
	painter.setProjector(core->getProjection(StelCore::FrameAltAz));
	foreach (MeteorObj* m, m_activeMeteors)
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
	double currentJD = StelApp::getInstance().getCore()->getJDay();

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

int MeteorShower::searchRealData(QString yyyy) const
{
	int index = -1;
	foreach(const activityData &p, m_activity)
	{
		index++;
		if(p.year == yyyy)
		{
			return index;
		}
	}

	return 0;
}

QDateTime MeteorShower::getSkyQDateTime(StelCore* core) const
{
	//get the current sky date
	double JD = core->getJDay();
	return StelUtils::jdToQDateTime(JD+StelUtils::getGMTShiftFromQT(JD)/24-core->getDeltaT(JD)/86400);
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

QVariantMap MeteorShower::getMap()
{
	QVariantMap map;
	map["showerID"] = m_showerID;
	map["designation"] = m_designation;
	map["speed"] = m_speed;
	map["radiantAlpha"] = m_rAlphaPeak;
	map["radiantDelta"] = m_rDeltaPeak;
	map["driftAlpha"] = m_driftAlpha;
	map["driftDelta"] = m_driftDelta;
	map["parentObj"] = m_parentObj;
	map["pidx"] = m_pidx;

	QVariantList activityList;
	foreach(const activityData &p, m_activity)
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
	foreach(const Meteor::colorPair &c, m_colors)
	{
		QVariantMap colorMap;
		colorMap["color"] = c.first;
		colorMap["intensity"] = c.second;
		colorList << colorMap;
	}
	map["colors"] = colorList;

	return map;
}

QString MeteorShower::getDesignation() const
{
	if (m_showerID.toInt()) // if showerID is a number
	{
		return "";
	}
	return m_showerID;
}

Vec3f MeteorShower::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : Vec3f(1.0, 1.0, 1.0);
}

QString MeteorShower::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	QString mstdata = q_("generic data");
	if(m_status == ACTIVE_REAL)
		mstdata = q_("real data");

	if(flags&Name)
	{
		oss << "<h2>" << getNameI18n();
		if (!m_showerID.toInt())
		{
			oss << " (" << m_showerID  <<")</h2>";
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
			.arg(StelUtils::radToHmsStr(m_driftAlpha/5))
			.arg(StelUtils::radToDmsStr(m_driftDelta/5));
		oss << "<br />";

		if (m_speed > 0)
		{
			oss << q_("Geocentric meteoric velocity: %1 km/s").arg(m_speed) << "<br />";
		}

		if(m_pidx > 0)
		{
			oss << q_("The population index: %1").arg(m_pidx) << "<br />";
		}

		if(!m_parentObj.isEmpty())
		{
			oss << q_("Parent body: %1").arg(q_(m_parentObj)) << "<br />";
		}

		if(m_start.toString("M") == m_finish.toString("M"))
		{
			oss << QString("%1: %2 - %3 %4")
			       .arg(q_("Active"))
			       .arg(m_start.toString("d"))
			       .arg(m_finish.toString("d"))
			       .arg(m_start.toString("MMMM"));
		}
		else
		{
			oss << QString("%1: %2 - %3")
			       .arg(q_("Activity"))
			       .arg(m_start.toString("d MMMM"))
			       .arg(m_finish.toString("d MMMM"));
		}
		oss << "<br />";
		oss << q_("Maximum: %1").arg(m_peak.toString("d MMMM"));

		QString slong = QString::number(MeteorShower::getSolarLongitude(m_peak), 'f', 2);
		oss << QString(" (%1 %2&deg;)").arg(q_("Solar longitude is")).arg(slong);
		oss << "<br />";

		if(m_zhr > 0)
		{
			oss << QString("ZHR<sub>max</sub>: %1").arg(m_zhr) << "<br />";
		}
		else
		{
			oss << QString("ZHR<sub>max</sub>: %1").arg(q_("variable"));
			if(!m_variable.isEmpty())
			{
				oss << "; " << m_variable;
			}
			oss << "<br />";
		}
	}

	postProcessInfoString(str, flags);

	return str;
}
