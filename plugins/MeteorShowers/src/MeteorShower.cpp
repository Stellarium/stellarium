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

#include "LandscapeMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "MeteorShower.hpp"
#include "MeteorShowers.hpp"
#include "SporadicMeteorMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"

const QString MeteorShower::METEORSHOWER_TYPE = QStringLiteral("MeteorShower");

MeteorShower::MeteorShower(MeteorShowersMgr* mgr, const QVariantMap& map)
	: m_mgr(mgr)
	, m_status(INVALID)
	, m_speed(0)
	, m_rAlphaPeak(0)
	, m_rDeltaPeak(0)
	, m_driftAlpha(0)
	, m_driftDelta(0)
	, m_pidx(0)
	, m_radiantAlpha(0)
	, m_radiantDelta(0)
{
	if(!map.contains("showerID") || !map.contains("activity")
		|| !map.contains("radiantAlpha") || !map.contains("radiantDelta"))
	{
		qWarning() << "MeteorShower: INVALID meteor shower!" << map.value("showerID").toString();
		qWarning() << "MeteorShower: Please, check your 'showers.json' catalog!";
		return;
	}

	m_showerID = map.value("showerID").toString();
	m_designation  = map.value("designation").toString();
	m_speed = map.value("speed").toInt();
	m_radiantAlpha = StelUtils::getDecAngle(map.value("radiantAlpha").toString());
	m_radiantDelta = StelUtils::getDecAngle(map.value("radiantDelta").toString());
	// initialize position to keep valgrind happy
	StelUtils::spheToRect(m_radiantAlpha, m_radiantDelta, m_position);
	m_parentObj = map.value("parentObj").toString();
	m_pidx = map.value("pidx").toFloat();

	// the catalog (IMO) will give us the drift for a five-day interval from peak
	m_driftAlpha = static_cast<float>(StelUtils::getDecAngle(map.value("driftAlpha").toString())) *0.2f;
	m_driftDelta = static_cast<float>(StelUtils::getDecAngle(map.value("driftDelta").toString())) *0.2f;

	m_rAlphaPeak = static_cast<float>(m_radiantAlpha);
	m_rDeltaPeak = static_cast<float>(m_radiantDelta);

	const int genericYear = 1000;

	// build the activity list
	QList<QVariant> activities = map.value("activity").toList();
	for (const auto& ms : activities)
	{
		QVariantMap activityMap = ms.toMap();
		Activity d;
		d.zhr = activityMap.value("zhr").toInt();

		//
		// 'variable'field
		//
		QStringList variable = activityMap.value("variable").toString().split("-");
		if (d.zhr == -1) // is variable
		{
			bool ok = variable.size() == 2;
			for (int i=0; i < 2 && ok; i++)
			{
				d.variable.append(variable.at(i).toInt(&ok));
			}

			if (!ok)
			{
				qWarning() << "[MeteorShower] INVALID data for " << m_showerID;
				qWarning() << "[MeteorShower] Please, check your 'showers.json' catalog!";
				return;
			}
		}

		//
		// 'start', 'finish' and 'peak' fields
		//
		d.year = activityMap.value("year").toInt();
		QString year = QString::number(d.year == 0 ? genericYear : d.year);

		QString start = activityMap.value("start").toString();
		start = start.isEmpty() ? "" : start + " " + year;
		d.start = QDate::fromString(start, "MM.dd yyyy");

		QString finish = activityMap.value("finish").toString();
		finish = finish.isEmpty() ? "" : finish + " " + year;
		d.finish = QDate::fromString(finish, "MM.dd yyyy");

		QString peak = activityMap.value("peak").toString();
		peak = peak.isEmpty() ? "" : peak + " " + year;
		d.peak = QDate::fromString(peak, "MM.dd yyyy");

		m_activities.append(d);
	}

	// filling null values of the activity list with generic data
	// and fixing the edge cases (showers between december and january)
	const Activity& g = m_activities.at(0);
	const int activitiesSize = m_activities.size();
	for (int i = 0; i < activitiesSize; ++i)
	{
		Activity a = m_activities.at(i);
		if (i != 0)
		{
			if (a.zhr == 0)
			{
				a.zhr = g.zhr;
				a.variable = g.variable;
			}

			int aux = a.year - genericYear;
			a.start = a.start.isValid() ? a.start : g.start.addYears(aux);
			a.finish = a.finish.isValid() ? a.finish : g.finish.addYears(aux);
			a.peak = a.peak.isValid() ? a.peak : g.peak.addYears(aux);
		}

		if (a.start.isValid() && a.finish.isValid() && a.peak.isValid())
		{
			// Fix the 'finish' year! Handling cases when the shower starts on
			// the current year and ends on the next year!
			if(a.start.operator >(a.finish))
			{
				a.finish = a.finish.addYears(1);
			}
			// Fix the 'peak' year
			if(a.start.operator >(a.peak))
			{
				a.peak = a.peak.addYears(1);
			}
		}
		else
		{
			qWarning() << "[MeteorShower] INVALID data for "
				   << m_showerID << "Unable to read some dates!";
			qWarning() << "[MeteorShower] Please, check your 'showers.json' catalog!";
			return;
		}

		m_activities.replace(i, a);
	}

	if(map.contains("colors"))
	{
		int totalIntensity = 0;
		for (const auto& ms : map.value("colors").toList())
		{
			QVariantMap colorMap = ms.toMap();
			QString color = colorMap.value("color").toString();
			int intensity = colorMap.value("intensity").toInt();
			m_colors.append(Meteor::ColorPair(color, intensity));
			totalIntensity += intensity;
		}

		// the total intensity must be 100
		if (totalIntensity != 100) {
			qWarning() << "[MeteorShower] INVALID data for "
				   << m_showerID << "The total intensity must be equal to 100";
			qWarning() << "[MeteorShower] Please, check your 'showers.json' catalog!";
			m_colors.clear();
		}
	}

	if (m_colors.isEmpty()) {
		m_colors.push_back(Meteor::ColorPair("white", 100));
	}

	m_status = UNDEFINED;
}

MeteorShower::~MeteorShower()
{
	qDeleteAll(m_activeMeteors);
	m_activeMeteors.clear();
	m_colors.clear();
}

bool MeteorShower::enabled() const
{
	if (m_status == INVALID)
	{
		return false;
	}
	else if (m_status == UNDEFINED)
	{
		return true;
	}
	else if (m_mgr->getActiveRadiantOnly())
	{
		return m_status == ACTIVE_GENERIC || m_status == ACTIVE_CONFIRMED;
	}
	else
	{
		return true;
	}
}

void MeteorShower::update(StelCore* core, double deltaTime)
{
	if (m_status == INVALID)
	{
		return;
	}

	// gets the current UTC date
	double currentJD = core->getJD();
	QDate currentDate = QDate::fromJulianDay(currentJD);

	// updating status and activity
	bool found = false;
	m_status = INACTIVE;
	m_activity = hasConfirmedShower(currentDate, found);
	if (found)
	{
		m_status = ACTIVE_CONFIRMED;
	}
	else
	{
		m_activity = hasGenericShower(currentDate, found);
		if (found)
		{
			m_status = ACTIVE_GENERIC;
		}
	}

	// will be displayed?
	if (!enabled())
	{
		return;
	}

	// fix the radiant position (considering drift)
	m_radiantAlpha = static_cast<double>(m_rAlphaPeak);
	m_radiantDelta = static_cast<double>(m_rDeltaPeak);
	if (m_status != INACTIVE)
	{
		double daysToPeak = currentJD - m_activity.peak.toJulianDay();
		m_radiantAlpha += static_cast<double>(m_driftAlpha) * daysToPeak;
		m_radiantDelta += static_cast<double>(m_driftDelta) * daysToPeak;
	}

	// step through and update all active meteors
	foreach (MeteorObj* m, m_activeMeteors)
	{
		if (!m->update(deltaTime))
		{
			//important to delete when no longer active
			m_activeMeteors.removeOne(m);
			delete m;
		}
	}

	// paused | forward | backward ?
	// don't create new meteors
	if(!core->getRealTimeSpeed())
	{
		return;
	}

	// calculates a ZHR for the current date
	int currentZHR = calculateZHR(currentJD);
	if (currentZHR < 1)
	{
		return;
	}

	// average meteors per frame
	const float mpf = currentZHR * static_cast<float>(deltaTime) / 3600.f;

	// maximum amount of meteors for the current frame
	int maxMpf = qRound(mpf);
	maxMpf = maxMpf < 1 ? 1 : maxMpf;

	float rate = mpf / static_cast<float>(maxMpf);
	for (int i = 0; i < maxMpf; ++i)
	{
		float prob = static_cast<float>(qrand()) / static_cast<float>(RAND_MAX);
		if (prob < rate)
		{
			MeteorObj *m = new MeteorObj(core, m_speed, static_cast<float>(m_radiantAlpha), static_cast<float>(m_radiantDelta),
						     m_pidx, m_colors, m_mgr->getBolideTexture());
			if (m->isAlive())
			{
				m_activeMeteors.append(m);
			}
			else
			{
				delete m;
			}
		}
	}
}

void MeteorShower::draw(StelCore* core)
{
	if (!enabled())
	{
		return;
	}
	drawRadiant(core);
	drawMeteors(core);
}

void MeteorShower::drawRadiant(StelCore *core)
{
	StelPainter painter(core->getProjection(StelCore::FrameJ2000));

	Vec3d XY;
	StelUtils::spheToRect(m_radiantAlpha, m_radiantDelta, m_position);
	painter.getProjector()->project(m_position, XY);

	painter.setBlending(true, GL_SRC_ALPHA, GL_ONE);

	Vec3f rgb;
	float alpha = 0.85f + (static_cast<float>(qrand()) / static_cast<float>(RAND_MAX)) / 10.f;
	switch(m_status)
	{
		case ACTIVE_CONFIRMED: //Active, confirmed data
			rgb = m_mgr->getColorARC();
			break;
		case ACTIVE_GENERIC: //Active, generic data
			rgb = m_mgr->getColorARG();
			break;
		default: //Inactive
			rgb = m_mgr->getColorIR();
	}	
	painter.setColor(rgb[0], rgb[1], rgb[2], alpha);

	// Hide the radiant markers at during day light and make it visible
	// when first stars will shine on the sky.
	float mlimit = core->getSkyDrawer()->getLimitMagnitude();
	float mag = 2.0f;

	Vec3d win;
	if (m_mgr->getEnableMarker() && painter.getProjector()->projectCheck(m_position, win) && mag<=mlimit)
	{
		m_mgr->getRadiantTexture()->bind();
		painter.drawSprite2dMode(static_cast<float>(XY[0]), static_cast<float>(XY[1]), 45);

		if (m_mgr->getEnableLabels())
		{
			painter.setFont(m_mgr->getFont());
			const float size = static_cast<float>(getAngularSize(Q_NULLPTR))*M_PI_180f*static_cast<float>(painter.getProjector()->getPixelPerRadAtCenter());
			const float shift = 8.f + size/1.8f;
			if ((mag+1.f)<mlimit)
				painter.drawText(static_cast<float>(XY[0])+shift, static_cast<float>(XY[1])+shift, getNameI18n(), 0, 0, 0, false);
		}
	}
}

void MeteorShower::drawMeteors(StelCore *core)
{
	if (!core->getSkyDrawer()->getFlagHasAtmosphere())
	{
		return;
	}

	LandscapeMgr* landmgr = GETSTELMODULE(LandscapeMgr);
	if (landmgr->getFlagAtmosphere() && landmgr->getLuminance() > 5.f)
	{
		return;
	}

	// step through and draw all active meteors
	StelPainter painter(core->getProjection(StelCore::FrameAltAz));
	for (auto* m : m_activeMeteors)
	{
		m->draw(core, painter);
	}
}

MeteorShower::Activity MeteorShower::hasGenericShower(QDate date, bool &found) const
{
	int year = date.year();
	Activity g = m_activities.at(0);
	bool peakOnStart = g.peak.year() == g.start.year(); // 'peak' and 'start' on the same year ?

	//Fix the 'generic years'!
	// Handling cases when the shower starts on the current year and
	// ends on the next year; or when it started on the last year...
	if (g.start.year() != g.finish.year()) // edge case?
	{
		// trying the current year with the next year
		g.start.setDate(year, g.start.month(), g.start.day());
		g.finish.setDate(year + 1, g.finish.month(), g.finish.day());
		found = date.operator >=(g.start) && date.operator <=(g.finish);

		if (!found)
		{
			// trying the last year with the current year
			g.start.setDate(year - 1, g.start.month(), g.start.day());
			g.finish.setDate(year, g.finish.month(), g.finish.day());
			found = date.operator >=(g.start) && date.operator <=(g.finish);
		}
	}
	else
	{
		g.start.setDate(year, g.start.month(), g.start.day());
		g.finish.setDate(year, g.finish.month(), g.finish.day());
		found = date.operator >=(g.start) && date.operator <=(g.finish);
	}

	g.year = g.start.year();
	g.peak.setDate(peakOnStart ? g.start.year() : g.finish.year(), g.peak.month(), g.peak.day());
	return g;
}

MeteorShower::Activity MeteorShower::hasConfirmedShower(QDate date, bool& found) const
{
	const int activitiesSize = m_activities.size();
	for (int i = 1; i < activitiesSize; ++i)
	{
		const Activity& a = m_activities.at(i);
		if (date.operator >=(a.start) && date.operator <=(a.finish))
		{
			found = true;
			return a;
		}
	}
	return Activity();
}

int MeteorShower::calculateZHR(const double& currentJD)
{
	const double startJD = m_activity.start.toJulianDay();
	const double finishJD = m_activity.finish.toJulianDay();
	const double peakJD = m_activity.peak.toJulianDay();

	double sd; //standard deviation
	if (currentJD >= startJD && currentJD < peakJD) //left side of gaussian
	{
		sd = (peakJD - startJD) *0.5;
	}
	else
	{
		sd = (finishJD - peakJD) *0.5;
	}

	const float maxZHR = m_activity.zhr == -1 ? m_activity.variable.at(1) : m_activity.zhr;
	const float minZHR = m_activity.zhr == -1 ? m_activity.variable.at(0) : 0;

	float gaussian = maxZHR * static_cast<float>(qExp( - qPow(currentJD - peakJD, 2) / (2 * sd * sd) )) + minZHR;

	return qRound(gaussian);
}

QString MeteorShower::getSolarLongitude(QDate date)
{
	//The number of days (positive or negative) since Greenwich noon,
	//Terrestrial Time, on 1 January 2000 (J2000.0)
	const double n = date.toJulianDay() - 2451545.0;

	//The mean longitude of the Sun, corrected for the aberration of light
	double l = (280.460 + 0.9856474 * n);

	// put it in the range 0 to 360 degrees
	l /= 360.;
	l = (l - static_cast<int>(l)) * 360. - 1.;

	return QString::number(l, 'f', 2);
}

QString MeteorShower::getDesignation() const
{
	if (m_showerID.toInt()) // if showerID is a number
		return QString();
	return m_showerID;
}

Vec3f MeteorShower::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6f, 0.0f, 0.0f) : Vec3f(1.0f, 1.0f, 1.0f);
}

QString MeteorShower::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str, designation = getDesignation();
	QTextStream oss(&str);
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

	const QMap<MeteorShower::Status, QString>mstMap={
		{ ACTIVE_GENERIC, q_("generic data")},
		{ ACTIVE_CONFIRMED, q_("confirmed data")},
		{ INACTIVE, q_("inactive")}};
	QString mstdata = mstMap.value(m_status, "");

	if (flags&Name)
	{
		oss << "<h2>" << getNameI18n();
		if (!designation.isEmpty())
			oss << " (" << designation  <<")";
		oss << "</h2>";
	}

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b> (%3)").arg(q_("Type"), q_("meteor shower"), mstdata) << "<br />";
	

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Extra)
	{
		QString sDriftRA = StelUtils::radToHmsStr(static_cast<double>(m_driftAlpha));
		QString sDriftDE = StelUtils::radToDmsStr(static_cast<double>(m_driftDelta));
		if (withDecimalDegree)
		{
			sDriftRA = StelUtils::radToDecDegStr(static_cast<double>(m_driftAlpha),4,false,true);
			sDriftDE = StelUtils::radToDecDegStr(static_cast<double>(m_driftDelta),4,false,true);
		}
		oss << QString("%1: %2/%3").arg(q_("Radiant drift (per day)"), sDriftRA, sDriftDE) << "<br />";

		if (m_speed > 0)
		{
			oss << QString("%1: %2 %3")
			       .arg(q_("Geocentric meteoric velocity"))
			       .arg(m_speed)
			       // TRANSLATORS: Unit of measure for speed - kilometers per second
			       .arg(qc_("km/s", "speed"));
			oss << "<br />";
		}

		if (m_pidx > 0)
			oss << QString("%1: %2").arg(q_("The population index")).arg(m_pidx) << "<br />";

		if (!m_parentObj.isEmpty())
			oss << QString("%1: %2").arg(q_("Parent body")).arg(q_(m_parentObj)) << "<br />";

		QString actStr = q_("Activity");
		if(m_activity.start.month() == m_activity.finish.month())
		{
			oss << QString("%1: %2 - %3 %4")
			       .arg(actStr)
			       .arg(m_activity.start.day())
			       .arg(m_activity.finish.day())
			       .arg(StelLocaleMgr::longGenitiveMonthName(m_activity.start.month()));
		}
		else
		{
			oss << QString("%1: %2 %3 - %4 %5")
			       .arg(actStr)
			       .arg(m_activity.start.day())
			       .arg(StelLocaleMgr::longGenitiveMonthName(m_activity.start.month()))
			       .arg(m_activity.finish.day())
			       .arg(StelLocaleMgr::longGenitiveMonthName(m_activity.finish.month()));
		}
		oss << "<br />";
		oss << QString("%1: %2 %3").arg(qc_("Maximum","meteor shower activity")).arg(m_activity.peak.day())
		       .arg(StelLocaleMgr::longGenitiveMonthName(m_activity.peak.month()));
		oss << QString(" (%1 %2&deg;)").arg(q_("Solar longitude")).arg(getSolarLongitude(m_activity.peak)) << "<br />";

		if(m_activity.zhr > 0)
			oss << QString("ZHR<sub>max</sub>: %1").arg(m_activity.zhr) << "<br />";
		else
		{
			oss << QString("ZHR<sub>max</sub>: %1").arg(q_("variable"));
			if(m_activity.variable.size() == 2)
				oss << QString("; %1-%2").arg(m_activity.variable.at(0)).arg(m_activity.variable.at(1));
			oss << "<br />";
		}
	}

	postProcessInfoString(str, flags);
	return str;
}

QVariantMap MeteorShower::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	if (enabled())
	{
		const QMap<MeteorShower::Status, QString>mstMap={
			{ ACTIVE_GENERIC, "generic-data"},
			{ ACTIVE_CONFIRMED, "confirmed-data"},
			{ INACTIVE, "inactive"}};
		map.insert("status", mstMap.value(m_status, ""));

		QString designation = getDesignation();
		map.insert("id", designation.isEmpty() ? "?" : designation);
		map.insert("velocity", m_speed);
		map.insert("population-index", m_pidx);
		map.insert("parent", m_parentObj);

		if(m_activity.zhr > 0)
			map.insert("zhr-max", m_activity.zhr);
		else
		{
			QString varStr="variable";
			if(m_activity.variable.size() == 2)
			{
				 varStr=QString("%1; %2-%3")
					.arg("variable")
					.arg(m_activity.variable.at(0))
					.arg(m_activity.variable.at(1));
			}
			map.insert("zhr-max", varStr);
		}
	}

	return map;
}
