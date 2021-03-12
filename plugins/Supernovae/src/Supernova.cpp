/*
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

#include "Supernova.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelLocaleMgr.hpp"
#include "StarMgr.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QVariant>
#include <QList>

const QString Supernova::SUPERNOVA_TYPE = QStringLiteral("Supernova");

Supernova::Supernova(const QVariantMap& map)
	: initialized(false)
	, designation("")
	, sntype("")
	, maxMagnitude(21.)
	, peakJD(0.)
	, snra(0.)
	, snde(0.)
	, note("")
	, distance(0.)
{
	if (!map.contains("designation") || !map.contains("alpha") || !map.contains("delta"))
	{
		qWarning() << "Supernova: INVALID supernova!" << map.value("designation").toString();
		qWarning() << "Supernova: Please, check your 'supernovae.json' catalog!";
		return;
	}

	designation  = map.value("designation").toString();
	sntype = map.value("type").toString();
	maxMagnitude = map.value("maxMagnitude").toDouble();
	peakJD = map.value("peakJD").toDouble();
	snra = StelUtils::getDecAngle(map.value("alpha").toString());
	snde = StelUtils::getDecAngle(map.value("delta").toString());
	StelUtils::spheToRect(snra, snde, XYZ);
	note = map.value("note").toString();
	distance = map.value("distance").toDouble();

	initialized = true;
}

Supernova::~Supernova()
{
	//
}

QVariantMap Supernova::getMap(void) const
{
	const QVariantMap map = {
	{"designation", designation},
	{"sntype", sntype},
	{"maxMagnitude", maxMagnitude},
	{"peakJD", peakJD},
	{"snra", snra},
	{"snde", snde},
	{"note", note},
	{"distance", distance}};

	return map;
}

QString Supernova::getNameI18n(void) const
{
	QString name = designation;
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	if (note.size()!=0)
		name = QString("%1 (%2)").arg(name).arg(trans.qtranslate(note));

	return name;
}

QString Supernova::getEnglishName(void) const
{
	QString name = designation;
	if (note.size()!=0)
		name = QString("%1 (%2)").arg(name).arg(note);	

	return name;
}

QString Supernova::getMaxBrightnessDate(const double JD) const
{
	return StelApp::getInstance().getLocaleMgr().getPrintableDateLocal(JD);
}

QString Supernova::getMagnitudeInfoString(const StelCore *core, const InfoStringGroup& flags, const int decimals) const
{
	const float maglimit = 21.f;
	QString res;

	if (flags&Magnitude)
	{
		if (getVMagnitude(core) <= maglimit)
			res = StelObject::getMagnitudeInfoString(core, flags, decimals);
		else
		{
			res = QString("%1: <b>--</b><br />").arg(q_("Magnitude"));
			res += getExtraInfoStrings(Magnitude).join("");
		}
	}
	return res;
}

QString Supernova::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
		oss << "<h2>" << getNameI18n() << "</h2>";

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_("supernova")) << "<br />";

	if (flags&Magnitude)
		oss << getMagnitudeInfoString(core, flags, 2);

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Extra)
	{
		oss << QString("%1: %2").arg(q_("Type of supernova"), sntype) << "<br />";
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


QVariantMap Supernova::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	map["sntype"] = sntype;
	map["max-magnitude"] = maxMagnitude;
	map["peakJD"] = peakJD;
	map["note"] = note;
	map["distance"] = distance;

	return map;
}

Vec3f Supernova::getInfoColor(void) const
{
	return Vec3f(1.f, 1.f, 1.f);
}

float Supernova::getVMagnitude(const StelCore* core) const
{
	double vmag = 20;
	const double currentJD = core->getJDE(); // GZ JDfix for 0.14. I hope the JD in the list is JDE? (Usually difference should be negligible)
	const double deltaJD = qAbs(peakJD-currentJD);

	// Use supernova light curve model from here - http://www.astronet.ru/db/msg/1188703

	if (sntype.contains("II", Qt::CaseSensitive))
	{
		// Type II
		if (peakJD<=currentJD)
		{
			vmag = maxMagnitude;
			if (deltaJD>0 && deltaJD<=30)
				vmag = maxMagnitude + 0.05 * deltaJD;

			if (deltaJD>30 && deltaJD<=80)
				vmag = maxMagnitude + 0.013 * (deltaJD - 30) + 1.5;

			if (deltaJD>80 && deltaJD<=100)
				vmag = maxMagnitude + 0.075 * (deltaJD - 80) + 2.15;

			if (deltaJD>100)
				vmag = maxMagnitude + 0.025 * (deltaJD - 100) + 3.65;
		}
		else
		{
			if (deltaJD<=20)
				vmag = maxMagnitude + 0.75 * deltaJD;
		}
	}
	else
	{
		// Type I
		if (peakJD<=currentJD)
		{
			vmag = maxMagnitude;
			if (deltaJD>0 && deltaJD<=25)
				vmag = maxMagnitude + 0.1 * deltaJD;

			if (deltaJD>25)
				vmag = maxMagnitude + 0.016 * (deltaJD - 25) + 2.5;
		}
		else
		{
			if (deltaJD<=15)
				vmag = maxMagnitude + 1.13 * deltaJD;
		}
	}

	if (vmag<maxMagnitude)
		vmag = maxMagnitude;

	return static_cast<float>(vmag);
}

double Supernova::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void Supernova::update(double deltaTime)
{
	labelsFader.update(static_cast<int>(deltaTime*1000));
}

void Supernova::draw(StelCore* core, StelPainter& painter)
{
	StelSkyDrawer* sd = core->getSkyDrawer();
	const float mlimit = sd->getLimitMagnitude();
	const float mag = getVMagnitudeWithExtinction(core);
	
	if (mag <= mlimit)
	{
		const Vec3f color(1.f);
		Vec3f vf(XYZ.toVec3f());
		Vec3f altAz(vf);
		altAz.normalize();
		core->j2000ToAltAzInPlaceNoRefraction(&altAz);
		RCMag rcMag;
		sd->computeRCMag(mag, &rcMag);
		sd->preDrawPointSource(&painter);
		// allow height-dependent twinkle and suppress twinkling in higher altitudes. Keep 0.1 twinkle amount in zenith.
		sd->drawPointSource(&painter, vf, rcMag, color, true, qMin(1.0f, 1.0f-0.9f*altAz[2]));
		sd->postDrawPointSource(&painter);
		painter.setColor(color, 1.f);
		float size = static_cast<float>(getAngularSize(Q_NULLPTR))*M_PI_180f*painter.getProjector()->getPixelPerRadAtCenter();
		float shift = 6.f + size/1.8f;
		StarMgr* smgr = GETSTELMODULE(StarMgr); // It's need for checking displaying of labels for stars
		if (labelsFader.getInterstate()<=0.f && (mag+5.f)<mlimit && smgr->getFlagLabels())
			painter.drawText(XYZ, designation, 0, shift, shift, false);
	}	
}
