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
	maxMagnitude = map.value("maxMagnitude").toFloat();
	peakJD = map.value("peakJD").toDouble();
	snra = StelUtils::getDecAngle(map.value("alpha").toString());
	snde = StelUtils::getDecAngle(map.value("delta").toString());
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
	QVariantMap map;
	map["designation"] = designation;
	map["sntype"] = sntype;
	map["maxMagnitude"] = maxMagnitude;
	map["peakJD"] = peakJD;
	map["snra"] = snra;
	map["snde"] = snde;
	map["note"] = note;
	map["distance"] = distance;

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

QString Supernova::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	float maglimit = 21.f;
	QString str, mag = "--", mage = "--";
	QTextStream oss(&str);
	if (getVMagnitude(core) <= maglimit)
	{
		mag  = QString::number(getVMagnitude(core), 'f', 2);
		mage = QString::number(getVMagnitudeWithExtinction(core), 'f', 2);
	}

	if (flags&Name)
	{
		oss << "<h2>" << getNameI18n() << "</h2>";
	}

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_("supernova")) << "<br />";

	if (flags&Magnitude)
	{
	    QString emag = "";
	    if (core->getSkyDrawer()->getFlagHasAtmosphere())
		    emag = QString(" (%1: <b>%2</b>)").arg(q_("extincted to"), mage);

	    oss << QString("%1: <b>%2</b>%3").arg(q_("Magnitude"), mag, emag) << "<br />";

	}

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
	return Vec3f(1.0, 1.0, 1.0);
}

float Supernova::getVMagnitude(const StelCore* core) const
{
	double vmag = 20;
	double currentJD = core->getJDE(); // GZ JDfix for 0.14. I hope the JD in the list is JDE? (Usually difference should be negligible)
	double deltaJD = qAbs(peakJD-currentJD);

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

	return vmag;
}

double Supernova::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void Supernova::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void Supernova::draw(StelCore* core, StelPainter& painter)
{
	StelSkyDrawer* sd = core->getSkyDrawer();
	StarMgr* smgr = GETSTELMODULE(StarMgr); // It's need for checking displaying of labels for stars

	Vec3f color = Vec3f(1.f,1.f,1.f);
	RCMag rcMag;
	float size, shift;
	double mag;

	StelUtils::spheToRect(snra, snde, XYZ);
	mag = getVMagnitudeWithExtinction(core);
	sd->preDrawPointSource(&painter);
	float mlimit = sd->getLimitMagnitude();
	
	if (mag <= mlimit)
	{
		sd->computeRCMag(mag, &rcMag);		
		sd->drawPointSource(&painter, Vec3f(XYZ[0],XYZ[1],XYZ[2]), rcMag, color, false);
		painter.setColor(color[0], color[1], color[2], 1.f);
		size = getAngularSize(Q_NULLPTR)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
		shift = 6.f + size/1.8f;
		if (labelsFader.getInterstate()<=0.f && (mag+5.f)<mlimit && smgr->getFlagLabels())
		{
			painter.drawText(XYZ, designation, 0, shift, shift, false);
		}
	}

	sd->postDrawPointSource(&painter);
}
