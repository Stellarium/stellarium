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

Supernova::Supernova(const QVariantMap& map)
		: initialized(false),
		  designation(""),
		  sntype(""),
		  maxMagnitude(21.),
		  peakJD(0.),
		  snra(0.),
		  snde(0.),
		  note(""),
		  distance(0.)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation"))
		return;
		
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

QVariantMap Supernova::getMap(void)
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
		oss << q_("Type: <b>%1</b>").arg(q_("supernova")) << "<br />";

	if (flags&Magnitude)
	{
	    if (core->getSkyDrawer()->getFlagHasAtmosphere() && getVMagnitude(core) <= maglimit)
		oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(mag, mage) << "<br>";
	    else
		oss << q_("Magnitude: <b>%1</b>").arg(mag) << "<br>";
	}

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Extra)
	{
		oss << q_("Type of supernova: %1").arg(sntype) << "<br>";
		oss << q_("Maximum brightness: %1").arg(getMaxBrightnessDate(peakJD)) << "<br>";
		if (distance>0)
		{
			//TRANSLATORS: Unit of measure for distance - Light Years
			oss << q_("Distance: %1 ly").arg(distance*1000) << "<br>";
		}
	}

	postProcessInfoString(str, flags);
	return str;
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
				vmag = maxMagnitude + 0.013 * deltaJD + 1.5;

			if (deltaJD>80)
				vmag = maxMagnitude + 0.05 * deltaJD + 2.15;

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
				vmag = maxMagnitude + 0.016 * deltaJD + 2.5;

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
		painter.setColor(color[0], color[1], color[2], 1);
		size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
		shift = 6.f + size/1.8f;
		if (labelsFader.getInterstate()<=0.f && (mag+5.f)<mlimit && smgr->getFlagLabels())
		{
			painter.drawText(XYZ, designation, 0, shift, shift, false);
		}
	}

	sd->postDrawPointSource(&painter);
}
