/*
 * Copyright (C) 2012 Alexander Wolf
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

#include "Pulsar.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelSkyDrawer.hpp"
#include "StelProjector.hpp"
#include "StelLocaleMgr.hpp"
#include "StelTranslator.hpp"
#include "Planet.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QVariant>
#include <QList>

#define PSR_INERTIA 1.0e45 /* Typical moment of inertia for a pulsar */

const QString Pulsar::PULSAR_TYPE = QStringLiteral("Pulsar");
StelTextureSP Pulsar::markerTexture;

bool Pulsar::distributionMode = false;
bool Pulsar::glitchFlag = false;
bool Pulsar::filteredMode = false;
float Pulsar::filterValue = 150.f;
Vec3f Pulsar::markerColor = Vec3f(0.4f,0.5f,1.0f);
Vec3f Pulsar::glitchColor = Vec3f(0.2f,0.3f,1.0f);

Pulsar::Pulsar(const QVariantMap& map)
	: initialized(false)
	, designation("")
	, bdesignation("")
	, pulsarName("")
	, RA(0.)
	, DE(0.)
	, parallax(0.)
	, period(0.)
	, frequency(0.)
	, pfrequency(0.)
	, pderivative(0.)
	, dmeasure(0.)
	, bperiod(0.)
	, eccentricity(0.)
	, w50(0.)
	, s400(0.)
	, s600(0.)
	, s1400(0.)
	, distance(0.)
	, glitch(-1)
	, notes("")
{
	if (!map.contains("designation") || !map.contains("RA") || !map.contains("DE"))
	{
		qWarning() << "Pulsar: INVALID pulsar!" << map.value("designation").toString();
		qWarning() << "Pulsar: Please, check your 'pulsars.json' catalog!";
		return;
	}

	designation  = map.value("designation").toString();
	bdesignation  = map.value("bdesignation").toString();
	pulsarName = map.value("name").toString();
	parallax = map.value("parallax").toFloat();
	period = map.value("period").toDouble();
	bperiod = map.value("bperiod").toDouble();
	frequency = map.value("frequency").toDouble();
	pfrequency = map.value("pfrequency").toDouble();
	pderivative = map.value("pderivative").toDouble();
	dmeasure = map.value("dmeasure").toDouble();
	eccentricity = map.value("eccentricity").toDouble();
	RA = StelUtils::getDecAngle(map.value("RA").toString());
	DE = StelUtils::getDecAngle(map.value("DE").toString());	
	pmRA = map.value("pmRA").toDouble();
	pmDE = map.value("pmDE").toDouble();
	w50 = map.value("w50").toFloat();
	s400 = map.value("s400").toFloat();
	s600 = map.value("s600").toFloat();
	s1400 = map.value("s1400").toFloat();
	distance = map.value("distance").toFloat();
	glitch = map.value("glitch").toInt();
	notes = map.value("notes").toString();

	// If barycentric period not set then calculate it
	if (qFuzzyCompare(period,0) && frequency>0)
	{
		period = 1/frequency;
	}
	// If barycentric period derivative not set then calculate it
	if (qFuzzyCompare(pderivative,0))
	{
		pderivative = getP1(period, pfrequency);
	}

	initialized = true;
}

Pulsar::~Pulsar()
{
	//
}

QVariantMap Pulsar::getMap(void) const
{
	QVariantMap map;
	map["designation"] = designation;
	map["bdesignation"] = bdesignation;
	map["name"] = pulsarName;
	map["parallax"] = parallax;
	map["bperiod"] = bperiod;
	map["frequency"] = frequency;
	map["pfrequency"] = pfrequency;
	map["pderivative"] = pderivative;
	map["dmeasure"] = dmeasure;
	map["eccentricity"] = eccentricity;	
	map["RA"] = RA;
	map["DE"] = DE;
	map["pmRA"] = pmRA;
	map["pmDE"] = pmDE;
	map["period"] = period;	
	map["w50"] = w50;
	map["s400"] = s400;
	map["s600"] = s600;
	map["s1400"] = s1400;
	map["distance"] = distance;
	map["glitch"] = glitch;
	map["notes"] = notes;

	return map;
}

float Pulsar::getSelectPriority(const StelCore* core) const
{
	return StelObject::getSelectPriority(core)-2.f;
}

QString Pulsar::getNameI18n(void) const
{
	QString name = getEnglishName();
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	if (!pulsarName.isEmpty())
		name = trans.qtranslate(pulsarName, "pulsar");

	return name;
}

QString Pulsar::getEnglishName(void) const
{
	return pulsarName;
}

QString Pulsar::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>";
		if (!getNameI18n().isEmpty())
			oss << getNameI18n() << "< br />";
		oss << (getBDesignation().isEmpty() ? getDesignation() : QString("%1 - %2").arg(getDesignation(), getBDesignation())) << "</h2>";
	}

	if (flags&ObjectType)
	{
		if (glitch==0)
			oss << QString("%1: <b>%2</b><br />").arg(q_("Type"), getObjectTypeI18n());
		else
			oss << QString("%1: <b>%2</b> (%3: %4)<br />").arg(q_("Type"), getObjectTypeI18n(), q_("registered glitches"), QString::number(glitch));
	}

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&ProperMotion && (pmRA!=0.0 && pmDE!=0.0))
	{
		double pa = std::atan2(pmRA, pmDE)*M_180_PI;
		if (pa<0)
			pa += 360.;
		oss << QString("%1: %2 %3 %4 %5&deg;<br />").arg(q_("Proper motion"),
			       QString::number(std::sqrt(pmRA*pmRA + pmDE*pmDE), 'f', 1), qc_("mas/yr", "milliarc second per year"),
			       qc_("towards", "into the direction of"), QString::number(pa, 'f', 1));
		oss << QString("%1: %2 %3 (%4)<br />").arg(q_("Proper motions by axes"), QString::number(pmRA, 'f', 1), QString::number(pmDE, 'f', 1), qc_("mas/yr", "milliarc second per year"));
	}

	if (flags&Extra)
	{
		if (period>0)
		{
			oss << QString("%1: %2 %3<br />").arg(
			       q_("Barycentric period"),
			       QString::number(period, 'f', 16),
			       //TRANSLATORS: Unit of measure for period - seconds
			       qc_("s", "period"));
		}
		if (pderivative>0)
			oss << QString("%1: %2<br />").arg(q_("Time derivative of barycentric period"), QString::number(pderivative, 'e', 5));

		if (dmeasure>0)
		{
			oss << QString("%1: %2 %3/%4<sup>3</sup><br />").arg(
				       q_("Dispersion measure"),
				       QString::number(dmeasure, 'f', 3),
				       //TRANSLATORS: Unit of measure for distance - parsecs
				       qc_("pc", "distance"),
				       //TRANSLATORS: Unit of measure for distance - centimeters
				       qc_("cm", "distance"));
		}
		double edot = getEdot(period, pderivative);
		if (edot>0)
		{
			oss << QString("%1: %2 %3<br />").arg(
				       q_("Spin down energy loss rate"),
				       QString::number(edot, 'e', 2),
				       //TRANSLATORS: Unit of measure for power - erg per second
				       qc_("ergs/s", "power"));
		}
		if (bperiod>0)
		{
			oss << QString("%1: %2 %3<br />").arg(
				       q_("Binary period of pulsar"),
				       QString::number(bperiod, 'f', 12),
				       //TRANSLATORS: Unit of measure for period - days
				       qc_("days", "period"));
		}
		if (eccentricity>0)
			oss << QString("%1: %2<br />").arg(q_("Eccentricity"), QString::number(eccentricity, 'f', 10));

		if (parallax>0)
		{
			oss << QString("%1: %2 %3<br />").arg(
				       q_("Annual parallax"),
				       QString::number(parallax),
				       //TRANSLATORS: Unit of measure for annual parallax - milliarcseconds
				       qc_("mas", "parallax"));
		}
		if (distance>0)
		{
			oss << QString("%1: %2 %3 (%4 %5)<br />")
			       .arg(q_("Distance based on electron density model"))
			       .arg(distance)
			       //TRANSLATORS: Unit of measure for distance - kiloparsecs
			       .arg(qc_("kpc", "distance"))
			       .arg(distance*3261.563777)
			       //TRANSLATORS: Unit of measure for distance - light years
			       .arg(qc_("ly", "distance"));
		}
		if (w50>0)
		{
			oss << QString("%1: %2 %3<br />").arg(
				       // xgettext:no-c-format
				       q_("Profile width at 50% of peak"),
				       QString::number(w50, 'f', 2),
				       //TRANSLATORS: Unit of measure for time - milliseconds
				       qc_("ms", "time"));
		}

		// TRANSLATORS: Full phrase is "Time averaged flux density at XXXMHz"
		QString flux = q_("Time averaged flux density at");
		// TRANSLATORS: Unit of measurement of frequency
		QString freq = qc_("MHz", "frequency");
		// TRANSLATORS: mJy is milliJansky(10-26W/m2/Hz)
		QString sfd  = qc_("mJy", "spectral flux density");

		if (s400>0)
			oss << QString("%1 %2%3: %4 %5<br />").arg(flux, QString::number(400), freq, QString::number(s400, 'f', 2), sfd);

		if (s600>0)
			oss << QString("%1 %2%3: %4 %5<br />").arg(flux, QString::number(600), freq, QString::number(s600, 'f', 2), sfd);

		if (s1400>0)
			oss << QString("%1 %2%3: %4 %5<br />").arg(flux, QString::number(1400), freq, QString::number(s1400, 'f', 2), sfd);

		if (!notes.isEmpty())
			oss << QString("<br />%1: %2<br />").arg(q_("Notes"), getPulsarTypeInfoString(notes));
	}

	postProcessInfoString(str, flags);
	return str;
}

QVariantMap Pulsar::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	map["parallax"] = parallax;
	map["bperiod"] = bperiod;
	map["frequency"] = frequency;
	map["pfrequency"] = pfrequency;
	map["pderivative"] = pderivative;
	map["dmeasure"] = dmeasure;
	map["eccentricity"] = eccentricity;
	map["period"] = period;
	map["w50"] = w50;
	map["s400"] = s400;
	map["s600"] = s600;
	map["s1400"] = s1400;
	map["distance"] = distance;
	map["glitch"] = glitch;
	map["notes"] = notes;
	return map;
}

Vec3f Pulsar::getInfoColor(void) const
{
	return Vec3f(1.f, 1.f, 1.f);
}

float Pulsar::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core)
	// Calculate fake visual magnitude as function by distance - minimal magnitude is 6
	float vmag = distance + 6.f;

	if (distributionMode)
	{
		return 3.f;
	}
	else
	{
		return vmag;
	}
}

float Pulsar::getVMagnitudeWithExtinction(const StelCore *core) const
{
	return getVMagnitude(core);
}

double Pulsar::getEdot(double p0, double p1) const
{
	if (p0>0 && !qFuzzyCompare(p1,0))
	{
		// Calculate spin down energy loss rate (ergs/s)
		return 4.0 * M_PI * M_PI * PSR_INERTIA * p1 / pow(p0,3);
	}
	else
	{
		return 0.0;
	}
}

double Pulsar::getP1(double p0, double f1) const
{
	if (p0>0 && !qFuzzyCompare(f1,0))
	{
		// Calculate derivative of barycentric period
		return -1.0 * p0 * p0 * f1;
	}
	else
	{
		return 0.0;
	}
}


QString Pulsar::getPulsarTypeInfoString(QString pcode) const
{
	QStringList out;

	if (pcode.contains("AXP"))
	{
		out.append(q_("anomalous X-ray pulsar or soft gamma-ray repeater with detected pulsations"));
	}

	if (pcode.contains("BINARY") || bperiod>0)
	{
		out.append(q_("has one or more binary companions"));
	}

	if (pcode.contains("HE"))
	{
		out.append(q_("with pulsed emission from radio to infrared or higher frequencies"));
	}

	if (pcode.contains("NRAD"))
	{
		out.append(q_("with pulsed emission only at infrared or higher frequencies"));
	}

	if (pcode.contains("RADIO"))
	{
		out.append(q_("with pulsed emission in the radio band"));
	}

	if (pcode.contains("RRAT"))
	{
		out.append(q_("with intermittently pulsed radio emission"));
	}

	if (pcode.contains("XINS"))
	{
		out.append(q_("isolated neutron star with pulsed thermal X-ray emission but no detectable radio emission"));
	}

	return out.join(",<br />");
}

void Pulsar::update(double deltaTime)
{
	labelsFader.update(static_cast<int>(deltaTime*1000));
}

void Pulsar::draw(StelCore* core, StelPainter *painter)
{
	Vec3d win, coord = getJ2000EquatorialPos(core);
	// Check visibility of pulsar
	if (!(painter->getProjector()->projectCheck(coord, win)))
		return;

	float mag = getVMagnitudeWithExtinction(core);
	StelSkyDrawer* sd = core->getSkyDrawer();
	const float mlimit = sd->getLimitMagnitude();
	const float shift = 8.f;
	bool visible = true;
	if (filteredMode && s400<filterValue)
		visible = false;

	if (mag <= mlimit && visible)
	{		
		Pulsar::markerTexture->bind();
		if (glitch>0 && glitchFlag)
			painter->setColor(glitchColor, 1.f);
		else
			painter->setColor(markerColor, 1.f);
		painter->setBlending(true, GL_ONE, GL_ONE);
		painter->drawSprite2dMode(coord, distributionMode ? 4.f : 5.f);

		if (labelsFader.getInterstate()<=0.f && !distributionMode && (mag+2.f)<mlimit)
		{
			QString name = getDesignation();
			if (!getNameI18n().isEmpty())
				name = getNameI18n();
			painter->drawText(coord, name, 0, shift, shift, false);
		}
	}
}

Vec3d Pulsar::getJ2000EquatorialPos(const StelCore* core) const
{
	static const double d2000 = 2451545.0;
	const double movementFactor = (M_PI/180.)*(0.0001/3600.) * (core->getJDE()-d2000)/365.25;
	Vec3d v;
	const double cRA = RA + movementFactor*pmRA;
	const double cDE = DE + movementFactor*pmDE;
	StelUtils::spheToRect(cRA, cDE, v);

	if ((core) && (core->getUseAberration()) && (core->getCurrentPlanet()))
	{
		Vec3d vel=core->getCurrentPlanet()->getHeliocentricEclipticVelocity();
		vel=StelCore::matVsop87ToJ2000*vel*core->getAberrationFactor()*(AU/(86400.0*SPEED_OF_LIGHT));
		Vec3d pos=v+vel;
		pos.normalize();
		return pos;
	}
	else
	{
		return v;
	}
}
