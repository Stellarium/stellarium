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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelPainter.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "SolarSystem.hpp"

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
		qWarning() << "MeteorShower: Please, check your 'MeteorShowers.json' catalog!";
		return;
	}

	m_showerID = map.value("showerID").toString();
	m_designation  = map.value("designation").toString();
	m_IAUNumber = map.value("IAUNo").toString();
	m_speed = map.value("speed").toInt();
	m_radiantAlpha = map.value("radiantAlpha").toFloat()*M_PI_180;
	m_radiantDelta = map.value("radiantDelta").toFloat()*M_PI_180;
	// initialize position to keep valgrind happy
	StelUtils::spheToRect(m_radiantAlpha, m_radiantDelta, m_position);
	m_parentObj = map.value("parentObj").toString();
	m_pidx = map.value("pidx").toFloat();

	// Rate of radiant drift from IAU MDC database (degree per solar longitude)
	// https://www.ta3.sk/IAUC22DB/MDC2007/
	m_driftAlpha = static_cast<float>(map.value("driftAlpha").toFloat()*M_PI_180);
	m_driftDelta = static_cast<float>(map.value("driftDelta").toFloat()*M_PI_180);

	// Antihelion radiant
	if (m_showerID == "ANT")
	{
		StelCore* core = StelApp::getInstance().getCore();
		static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
		double ra_equ, dec_equ, currentLambda, beta;
		
		StelUtils::rectToSphe(&ra_equ,&dec_equ, ssystem->getSun()->getJ2000EquatorialPos(core));
		StelUtils::equToEcl(ra_equ, dec_equ, eclJ2000, &currentLambda, &beta);

		// 15 deg. east of opposition point along the ecliptic
		// Lunsford, R. (2004), "The anthelion radiant", WGN, Journal of the International Meteor Organization, vol. 32, no.3, p.81-83, July 2004
		StelUtils::eclToEqu(currentLambda+(195.*M_PI_180), 0., eclJ2000, &ra_equ, &dec_equ);
		m_radiantAlpha = ra_equ;
		m_radiantDelta = dec_equ;
	}

	m_rAlphaPeak = static_cast<float>(m_radiantAlpha);
	m_rDeltaPeak = static_cast<float>(m_radiantDelta);

	const int genericYear = 1000;
	eclJ2000 = GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(2451545.0);

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
				qWarning() << "[MeteorShower] Please, check your 'MeteorShowers.json' catalog!";
				return;
			}
		}

		//
		// 'start', 'finish' and 'peak' fields
		//
		d.year = activityMap.value("year").toInt();
		QString year = QString::number(d.year == 0 ? genericYear : d.year);
		d.start = activityMap.value("start").toDouble(); // solar longitude at start
		d.finish = activityMap.value("finish").toDouble(); // solar longitude at finish
		
		//
		// 'sollong', '"disttype', 'b1' and 'b2' fields
		//
		d.peak = activityMap.value("peak").toDouble(); // solar longitude at ZHRmax
		d.disttype = activityMap.value("disttype").toInt();
		d.b1 = activityMap.value("b1").toDouble();
		d.b2 = activityMap.value("b2").toDouble();
		// set default value if there is no value in the database
		if (d.b1 == 0) d.b1 = 0.19;
		if (d.b2 == 0) d.b2 = 0.19;

		m_activities.append(d);
	}

	// filling null values of the activity list with generic data
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

			if (a.start == 0) a.start = g.start;
			if (a.finish == 0) a.finish = g.finish;
			if (a.peak == 0) a.peak = g.peak;
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
			qWarning() << "[MeteorShower] Please, check your 'MeteorShowers.json' catalog!";
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

	static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
	double ra_equ, dec_equ, currentSolLong, beta;
	StelUtils::rectToSphe(&ra_equ,&dec_equ, ssystem->getSun()->getJ2000EquatorialPos(core));
	StelUtils::equToEcl(ra_equ, dec_equ, eclJ2000, &currentSolLong, &beta);

	// updating status and activity
	bool found = false;
	currentSolLong = StelUtils::fmodpos(currentSolLong, 2.*M_PI) / M_PI_180; // solar longitude in degrees
	m_status = INACTIVE;
	m_activity = hasConfirmedShower(currentSolLong, found);
	if (found)
	{
		m_status = ACTIVE_CONFIRMED;
	}
	else
	{
		m_activity = hasGenericShower(currentSolLong, found);
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
		double dLong = currentSolLong - m_activity.peak;
		if (dLong<-180.) dLong += 360.;
		m_radiantAlpha += static_cast<double>(m_driftAlpha) * dLong;
		m_radiantDelta += static_cast<double>(m_driftDelta) * dLong;
	}

	// special case of antihelion radiant
	if (m_showerID == "ANT")
	{
		// Antihelion radiant is 15 deg. east of the solar opposition point along the ecliptic
		// Source: The anthelion radiant
		// Lunsford, R., WGN, Journal of the International Meteor Organization, vol. 32, no.3, p.81-83, July 2004
		StelUtils::eclToEqu((currentSolLong+195.)*M_PI_180, 0., eclJ2000, &ra_equ, &dec_equ);
		m_radiantAlpha = ra_equ;
		m_radiantDelta = dec_equ;
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
	int currentZHR = calculateZHR(core);
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
		#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
		float prob = StelApp::getInstance().getRandF();
		#else
		float prob = static_cast<float>(qrand()) / static_cast<float>(RAND_MAX);
		#endif
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	float alpha = 0.85f + StelApp::getInstance().getRandF() / 10.f;
#else
	float alpha = 0.85f + (static_cast<float>(qrand()) / static_cast<float>(RAND_MAX)) / 10.f;
#endif
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
	painter.setColor(rgb, alpha);

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
			painter.setColor(rgb);
			painter.setFont(m_mgr->getFont());
			const float shift = 8.f;
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
	for (auto* m : qAsConst(m_activeMeteors))
	{
		m->draw(core, painter);
	}
}

MeteorShower::Activity MeteorShower::hasGenericShower(double currentSolLong, bool &found) const
{
	Activity g = m_activities.at(0);

	double finishLong = g.finish;
	if (g.start > finishLong) finishLong += 360.;
	if (currentSolLong>=(g.start) && currentSolLong<=(finishLong))
	{
		found = true;
	}
	return g;
}

MeteorShower::Activity MeteorShower::hasConfirmedShower(double currentSolLong, bool& found) const
{
	const int activitiesSize = m_activities.size();
	StelCore* core = StelApp::getInstance().getCore();
	const double currentJD = core->getJD();
	int Year, Month, Day;
	StelUtils::getDateFromJulianDay(currentJD, &Year, &Month, &Day);

	for (int i = 1; i < activitiesSize; ++i)
	{
		const Activity& a = m_activities.at(i);
		double finishLong = a.finish;
		if (a.start > finishLong) finishLong += 360.;
		if (currentSolLong>=(a.start) && currentSolLong<=(finishLong) && a.year==Year)
		{
			found = true;
			return a;
		}
	}
	return Activity();
}

int MeteorShower::calculateZHR(StelCore *core)
{
	static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
	double ra_equ, dec_equ, currentlambda, beta;
	StelUtils::rectToSphe(&ra_equ,&dec_equ, ssystem->getSun()->getJ2000EquatorialPos(core));
	StelUtils::equToEcl(ra_equ, dec_equ, eclJ2000, &currentlambda, &beta);
	currentlambda = StelUtils::fmodpos(currentlambda, 2.*M_PI);

	// current geocentric ZHR
	// B is the slope coefficient related to the full width at half maximum by FWHM = 2 log2/B
	// Jenniskens P. (1994), "Meteor stream activity I. The annual streams", Astron. Astrophys., 287, 990-1013.

	double diffLong = (currentlambda / M_PI_180) - m_activity.peak;
	double b = 0.19; // mean slope for some showers that we don't have this value

	if (m_activity.disttype == 0) // Gaussian distribution
	{
		if (diffLong<0.)
			b = m_activity.b1; // slope before peak
		else
			b = m_activity.b2; // slope after peak
	}
	else
	{
		b = m_activity.b1; // slope for Lorentz distribution
	}

	float currentZHR = 0.;
	if (m_activity.zhr > 0)
	{
		if (m_activity.disttype == 0)
		{
			// Gaussian ZHR-profile for normal meteor showers
			currentZHR = m_activity.zhr * pow(10.,-(b * abs(diffLong)));
		}
		else
		{
			// Lorentz ZHR-profile (better for meteor outbursts with very high ZHR)
			currentZHR = m_activity.zhr*(b*b/4.) / ((diffLong*diffLong)+(b*b/4.));
		}
	}
	else
	{
		currentZHR = m_activity.variable.at(0) * pow(10.,-(b * abs(diffLong)));
	}
	return qRound(currentZHR);
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
		oss << QString("%1: <b>%2</b> (%3)").arg(q_("Type"), getObjectTypeI18n(), mstdata) << "<br />";
	

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Extra)
	{
		QString sDriftRA;
		if (m_driftAlpha >= 0.)
			sDriftRA = StelUtils::radToHmsStr(static_cast<double>(m_driftAlpha));
		else
			sDriftRA = '-'+StelUtils::radToHmsStr(static_cast<double>(abs(m_driftAlpha)));
		QString sDriftDE = StelUtils::radToDmsStr(static_cast<double>(m_driftDelta));
		if (withDecimalDegree)
		{
			sDriftRA = StelUtils::radToDecDegStr(static_cast<double>(m_driftAlpha),4,false,true);
			sDriftDE = StelUtils::radToDecDegStr(static_cast<double>(m_driftDelta),4,false,true);
		}
		if (m_showerID != "ANT")
		{
			oss << QString("%1: %2").arg(q_("IAU shower number"), m_IAUNumber) << "<br />";
			oss << QString("%1: %2/%3").arg(q_("Radiant drift (per solar longitude)"), sDriftRA, sDriftDE) << "<br />";
		}

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
			oss << QString("%1: %2").arg(q_("Parent body"), q_(m_parentObj)) << "<br />";

		QString actStr = q_("Activity");
		static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
		double ra_equ, dec_equ, currentLambda, beta, az, alt;
		const double currentJD = core->getJD(); // save current JD
		
		if (m_showerID != "ANT")
		{
			const double utcShift = core->getUTCOffset(core->getJD()) / 24.;
			int Year, startMonth, startDay, finishMonth, finishDay;
			double activityJD, peakJD;

			StelUtils::getDateFromJulianDay(currentJD+utcShift, &Year, &startMonth, &startDay);
			// peak JD from solar longitude
			peakJD = MeteorShower::JDfromSolarLongitude(m_activity.peak, Year);
			StelUtils::getDateFromJulianDay(peakJD+utcShift, &Year, &startMonth, &startDay);

			// Find approximate period of activity from ranges of solar longitude
			// WB: IAU MDC have noticeable differences when compared with IMO, but peak dates are around the same
			// We will use data from IMO calendars by converting date to solar longtitude
			// This will allow them to be usable with more realistic activity period in the ancient and far future years
			// Example : Activity duration of Perseids in recent years ~ July 17 - August 24
			// They can converted to solar longitude 114-151.

			// Start JD
			activityJD = MeteorShower::JDfromSolarLongitude(m_activity.start, Year);
			StelUtils::getDateFromJulianDay(activityJD+utcShift, &Year, &startMonth, &startDay);
			// Finish JD
			activityJD = MeteorShower::JDfromSolarLongitude(m_activity.finish, Year);
			StelUtils::getDateFromJulianDay(activityJD+utcShift, &Year, &finishMonth, &finishDay);

			if(startMonth == finishMonth)
			{
				oss << QString("%1: %2 - %3 %4")
				       .arg(actStr)
				       .arg(startDay)
				       .arg(finishDay)
				       .arg(StelLocaleMgr::longGenitiveMonthName(startMonth));
			}
			else
			{
				oss << QString("%1: %2 %3 - %4 %5")
				       .arg(actStr)
				       .arg(startDay)
				       .arg(StelLocaleMgr::longGenitiveMonthName(startMonth))
				       .arg(finishDay)
				       .arg(StelLocaleMgr::longGenitiveMonthName(finishMonth));
			}
			oss << QString(" (%1 %2&deg; - %3&deg;)")
			       .arg(q_("Solar longitude "))
			       .arg(m_activity.start)
			       .arg(m_activity.finish);
			oss << "<br />";

			// Peak date/time for current year
			int peakMonth, peakDay;
			StelUtils::getDateFromJulianDay(peakJD+utcShift, &Year, &peakMonth, &peakDay);
			oss << QString("%1: %2 %3").arg(qc_("Maximum","meteor shower activity")).arg(peakDay)
			       .arg(StelLocaleMgr::longGenitiveMonthName(peakMonth));
			
			// Time of predicted peak
			// Meteor shower can not be predicted to this precision
			// but IMO provides this information in their annual calendars,
			// Stellarium should be able to do it too.
			// It helps users to identify the best night for observation which varies from year to year.
			// However, we use approximate formula to calculate JD from solar longitude (to avoid heavy calculations).
			// It's accurate to within a few minutes for present day but inaccurate in thousands of years from now (many hours of error).
			// So, it may be better to not show the time for now.
			//float peakHour = StelUtils::getHoursFromJulianDay(peakJD+utcShift);
			//oss << QString(" (%1) ").arg(StelUtils::hoursToHmsStr(peakHour, true));

			// Solar longitude at peak, a precise measure of the Earth's position on its orbit which is not dependent on the vagaries of the calendar.
			// This is the correct way and more realistic than earlier implementation.
			oss << QString(" (%1 %2&deg;)")
			       .arg(q_("Solar longitude "))
			       .arg(m_activity.peak);
			oss << QString("<br />");
		}
		StelUtils::rectToSphe(&ra_equ,&dec_equ, ssystem->getSun()->getJ2000EquatorialPos(core));
		StelUtils::equToEcl(ra_equ, dec_equ, eclJ2000, &currentLambda, &beta);
		currentLambda = StelUtils::fmodpos(currentLambda, 2.*M_PI);
		
		// current geocentric ZHR
		// B is the slope coefficient related to the full width at half maximum by FWHM = 2 log2/B
		// Jenniskens P. (1994), "Meteor stream activity I. The annual streams", Astron. Astrophys., 287, 990-1013.

		double diffLong = (currentLambda / M_PI_180) - m_activity.peak;
		double b;
		if (m_activity.disttype == 0) // Gaussian distribution
		{
			if (diffLong<0.)
				b = m_activity.b1; // slope before peak
			else
				b = m_activity.b2; // slope after peak
		}
		else
		{
			b = m_activity.b1; // slope for Lorentz distribution
		}

		// Altitude of radiant
		StelUtils::rectToSphe(&az,&alt,getAltAzPosGeometric(core));
		// Limiting magnitude
		float mlimit = core->getSkyDrawer()->getLimitMagnitude();
		if (mlimit>6.5) mlimit = 6.5;
		// Limiting magnitude could be higher than 6.5 but we can't see very faint meteors
		// and we should set the limit to prevent unrealistic super-high meteor rate.
		// Hourly rate that goes higher than ZHRmax may also confuse users if we allow it.
		
		if (m_activity.zhr > 0)
		{
			oss << QString("ZHR<sub>max</sub>: %1").arg(m_activity.zhr) << "<br />";
			if (m_status != INACTIVE)
			{
				int currentZHR=0, localHR=0;
				if (m_activity.disttype == 0)
				{
					// Gaussian ZHR-profile for normal meteor showers
					currentZHR = qRound(m_activity.zhr * pow(10.,-(b * abs(diffLong))));
				}
				else
				{
					// Lorentz ZHR-profile (better for meteor outbursts with very high ZHR)
					currentZHR = qRound(m_activity.zhr*(b*b/4.) / ((diffLong*diffLong)+(b*b/4.)));
				} 
				if (m_showerID == "ANT") currentZHR = 4;

				// Local hourly rate - radiant altitude and limiting magnitude are taken into account
				// Jenniskens P. (1994), "Meteor stream activity I. The annual streams", Astron. Astrophys., 287, 990-1013.
				localHR = qRound(currentZHR*sin(alt) / pow(m_pidx,6.5-mlimit));
				if (localHR<0) localHR = 0;
				oss << QString("%1: %2<br />")
						.arg(q_("Current ZHR"))
						.arg(currentZHR);
				oss << QString("%1: %2<br />")
						.arg(q_("Local Hourly Rate"))
						.arg(localHR);
			}
		}
		else
		{
			oss << QString("ZHR<sub>max</sub>: %1").arg(q_("variable"));
			if(m_activity.variable.size() == 2)
				oss << QString("; %1-%2").arg(m_activity.variable.at(0)).arg(m_activity.variable.at(1));
			oss << "<br />";

			if (m_status != INACTIVE)
			{
				int minvarZHR = qRound(m_activity.variable.at(0) * pow(10.,-(b * abs(diffLong))));
				int maxvarZHR = qRound(m_activity.variable.at(1) * pow(10.,-(b * abs(diffLong))));
				if (maxvarZHR > 0)
				{
					oss << QString("%1: %2-%3<br />")
						.arg(q_("Current ZHR"))
						.arg(minvarZHR)
						.arg(maxvarZHR);

					int localminvarHR = qRound(minvarZHR*sin(alt) / pow(m_pidx,6.5-mlimit));
					int localmaxvarHR = qRound(maxvarZHR*sin(alt) / pow(m_pidx,6.5-mlimit));

					if (localmaxvarHR > 0)
					{
						oss << QString("%1: %2-%3<br />")
						.arg(q_("Local Hourly Rate"))
						.arg(localminvarHR)
						.arg(localmaxvarHR);
					}
				}
			}
		}
	}

	oss << getSolarLunarInfoString(core, flags);
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

// Approximate Julian day from solar longitude
// The formula is very good for the years 1900-2100 (accurate to within a few minutes)
// but we can use it for thousands of years outside this range because we don't need very accurate result in that years
// However, to prevent greater than a day of errors, we will limit the calculation in search dialog between the years -1000 to 5000
// or around +/- 3,000 years from present day.
// Ofek, E. O. (2000), "Low-Precision Formulae for Calculating Julian Day from Solar Longitude",
// WGN, Journal of the International Meteor Organization, vol. 28, no. 5, p. 176-178

double MeteorShower::JDfromSolarLongitude(double solarLong, int year)
{
	double approxJD;
	StelUtils::getJDFromDate(&approxJD, year, 1, 1, 0, 0, 1);
	double diffJD = approxJD-2451545.0;
	solarLong = solarLong * M_PI_180;
	double dt = 1.94330*sin(solarLong - 1.798135) + 0.01305272*sin(2.*solarLong + 2.634232) + 78.195268 + 58.13165*solarLong - 0.0000089408*diffJD;
	double jd = 2451182.24736 + 365.25963575*(year-2000) + dt;
	if (jd<approxJD) jd += 365.25963575;
	return jd;
}
