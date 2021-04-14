/*
 * Copyright (C) 2013 Alexander Wolf
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

#include "Nova.hpp"
#include "StelObject.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StarMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelPainter.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QVariant>
#include <QList>

const QString Nova::NOVA_TYPE = QStringLiteral("Nova");

Nova::Nova(const QVariantMap& map)
	: initialized(false)
	, designation("")
	, novaName("")
	, novaType("")
	, maxMagnitude(21.)
	, minMagnitude(21.)
	, peakJD(0.)
	, m2(-1)
	, m3(-1)
	, m6(-1)
	, m9(-1)
	, RA(0.)
	, Dec(0.)
	, distance(0.)
{
	if (!map.contains("designation") || !map.contains("RA") || !map.contains("Dec"))
	{
		qWarning() << "Nova: INVALID nova!" << map.value("designation").toString();
		qWarning() << "Nova: Please, check your 'novae.json' catalog!";
		return;
	}

	designation  = map.value("designation").toString();
	novaName = map.value("name").toString();
	novaType = map.value("type").toString();
	maxMagnitude = map.value("maxMagnitude").toFloat();
	minMagnitude = map.value("minMagnitude", 21).toFloat();
	peakJD = map.value("peakJD").toDouble();
	m2 = map.value("m2", -1).toInt();
	m3 = map.value("m3", -1).toInt();
	m6 = map.value("m6", -1).toInt();
	m9 = map.value("m9", -1).toInt();
	RA = StelUtils::getDecAngle(map.value("RA").toString());
	Dec = StelUtils::getDecAngle(map.value("Dec").toString());
	StelUtils::spheToRect(RA, Dec, XYZ);
	distance = map.value("distance").toDouble();

	initialized = true;
}

Nova::~Nova()
{
	//
}

QVariantMap Nova::getMap(void) const
{
	const QVariantMap map = {
	{"designation", designation},
	{"name", novaName},
	{"type", novaType},
	{"maxMagnitude", maxMagnitude},
	{"minMagnitude", minMagnitude},
	{"peakJD", peakJD},
	{"m2", m2},
	{"m3", m3},
	{"m6", m6},
	{"m9", m9},
	{"RA", RA},
	{"Dec", Dec},
	{"distance", distance}};

	return map;
}

QString Nova::getEnglishName() const
{
	return novaName;
}

QString Nova::getNameI18n() const
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	// Parse the nova name to get parts to translation
	QRegExp nn("^Nova\\s+(\\w+|\\w+\\s+\\w+)\\s+(\\d+|\\d+\\s+#\\d+)$");
	QString nameI18n = novaName;
	if (nn.exactMatch(novaName))
		nameI18n = QString("%1 %2 %3").arg(trans.qtranslate("Nova", "Nova template"), trans.qtranslate(nn.cap(1).trimmed(), "Genitive name of constellation"), nn.cap(2).trimmed());
	else
		nameI18n = trans.qtranslate(novaName);

	return nameI18n;
}

QString Nova::getDesignation() const
{
	return designation;
}

QString Nova::getMaxBrightnessDate(const double JD) const
{
	return StelApp::getInstance().getLocaleMgr().getPrintableDateLocal(JD);
}

QString Nova::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		QString name = novaName.isEmpty() ? QString("<h2>%1</h2>").arg(designation) : QString("<h2>%1 (%2)</h2>").arg(getNameI18n()).arg(designation);
		oss << name;
	}

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b> (%3)").arg(q_("Type"), q_("nova"), novaType) << "<br />";

	if (flags&Magnitude)
		oss << getMagnitudeInfoString(core, flags, 2);

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Extra)
	{
		oss << QString("%1: %2").arg(q_("Maximum brightness"), getMaxBrightnessDate(peakJD)) << "<br />";
		if (distance>0)
		{
			//TRANSLATORS: Unit of measure for distance - Light Years
			QString ly = qc_("ly", "distance");
			oss << QString("%1: %2 %3").arg(q_("Distance"), QString::number(distance*1000, 'f', 2), ly) << "<br />";
		}
	}

	postProcessInfoString(str, flags);
	return str;
}

QVariantMap Nova::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	map["designation"] = designation;
	map["name"] = novaName;
	map["nova-type"] = novaType;
	map["max-magnitude"] = maxMagnitude;
	map["min-magnitude"] = minMagnitude;
	map["peakJD"] = peakJD;
	map["m2"] = m2;
	map["m3"] = m3;
	map["m6"] = m6;
	map["m9"] = m9;
	map["distance"] = distance;

	return map;
}

Vec3f Nova::getInfoColor(void) const
{
	return Vec3f(1.f, 1.f, 1.f);
}

float Nova::getVMagnitude(const StelCore* core) const
{
	// OK, start from minimal brightness
	float vmag = minMagnitude;
	double currentJD = core->getJDE();
	float deltaJD = static_cast<float>(qAbs(peakJD-currentJD));
    
	// Fill "default" values for mX
	int t2 = m2;
	if (m2 < 0)
	{
		// m2 is unset, check type of nova
		if (novaType.contains("NA", Qt::CaseSensitive))
			t2 = 10; // Ok, "fast" nova

		if (novaType.contains("NB", Qt::CaseSensitive))
			t2 = 80; // Ok, "slow" nova

		if (novaType.contains("NC", Qt::CaseSensitive))
			t2 = 200; // Ok, "very slow" nova
	}	

	int t3 = m3;
	if (m3 < 0)
	{
		// m3 is unset, check type of nova
		if (novaType.contains("NA", Qt::CaseSensitive))
			t3 = 30; // Ok, "fast" nova

		if (novaType.contains("NB", Qt::CaseSensitive))
			t3 = 160; // Ok, "slow" nova

		if (novaType.contains("NC", Qt::CaseSensitive))
			t3 = 300; // Ok, "very slow" nova
	}	

	int t6 = m6;
	if (m6 < 0)
	{
		// m3 is unset, check type of nova
		if (novaType.contains("NA", Qt::CaseSensitive))
			t6 = 100; // Ok, "fast" nova

		if (novaType.contains("NB", Qt::CaseSensitive))
			t6 = 300; // Ok, "slow" nova

		if (novaType.contains("NC", Qt::CaseSensitive))
			t6 = 1200; // Ok, "very slow" nova
	}	

	int t9 = m9;
	if (m9 < 0)
	{
		// m3 is unset, check type of nova
		if (novaType.contains("NA", Qt::CaseSensitive))
			t9 = 400; // Ok, "fast" nova

		if (novaType.contains("NB", Qt::CaseSensitive))
			t9 = 1000; // Ok, "slow" nova

		if (novaType.contains("NC", Qt::CaseSensitive))
			t9 = 3000; // Ok, "very slow" nova
	}	

	// Calculate light curve
	if (peakJD<=currentJD)
	{
		float step;
		float d2 = maxMagnitude+2.f;
		float d3 = maxMagnitude+3.f;
		float d6 = maxMagnitude+6.f;		

		if (deltaJD>0 && deltaJD<=t2)
		{
			step = 2.f/t2;
			vmag = maxMagnitude + step*deltaJD;
		}

		if (deltaJD>t2 && deltaJD<=t3)
		{
			step = 3.f/t3;
			vmag = d2 + step*(deltaJD-t2);
		}

		if (deltaJD>t3 && deltaJD<=t6)
		{
			step = 6.f/t6;
			vmag = d3 + step*(deltaJD-t3);
		}

		if (deltaJD>t6 && deltaJD<=t9)
		{
			step = 9.f/t9;
			vmag = d6 + step*(deltaJD-t6);
		}

		if (deltaJD>t9)
			vmag = minMagnitude;
	}
	else
	{
		int dt = 3;
		if (deltaJD<=dt)
		{
			float step = (minMagnitude - maxMagnitude)/dt; // 3 days for outburst
			vmag = maxMagnitude + step*deltaJD;
		}
	}

	if (vmag>minMagnitude)
		vmag = minMagnitude;

	return vmag;
}

double Nova::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void Nova::update(double deltaTime)
{
	labelsFader.update(static_cast<int>(deltaTime*1000));
}

void Nova::draw(StelCore* core, StelPainter* painter)
{
	StelSkyDrawer* sd = core->getSkyDrawer();
	const float mlimit = sd->getLimitMagnitude();
	float mag = getVMagnitudeWithExtinction(core);

	if (mag <= mlimit)
	{
		const Vec3f color(1.f);
		Vec3f vf(XYZ.toVec3f());
		Vec3f altAz(vf);
		altAz.normalize();
		core->j2000ToAltAzInPlaceNoRefraction(&altAz);
		RCMag rcMag;
		sd->computeRCMag(mag, &rcMag);
		sd->preDrawPointSource(painter);
		// allow height-dependent twinkle and suppress twinkling in higher altitudes. Keep 0.1 twinkle amount in zenith.
		sd->drawPointSource(painter, vf, rcMag, color, true, qMin(1.0f, 1.0f-0.9f*altAz[2]));
		sd->postDrawPointSource(painter);
		painter->setColor(color, 1.f);
		float size = getAngularSize(Q_NULLPTR)*M_PI_180f*painter->getProjector()->getPixelPerRadAtCenter();
		float shift = 6.f + size/1.8f;
		StarMgr* smgr = GETSTELMODULE(StarMgr); // It's need for checking displaying of labels for stars
		if (labelsFader.getInterstate()<=0.f && (mag+5.f)<mlimit && smgr->getFlagLabels())
		{
			QString name = novaName.isEmpty() ? designation : novaName;
			painter->drawText(XYZ, name, 0, shift, shift, false);
		}
	}
}
