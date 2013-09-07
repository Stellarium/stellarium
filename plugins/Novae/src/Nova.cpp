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
#include "StelRenderer.hpp"
#include "StelLocaleMgr.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QVariant>
#include <QList>

Nova::Nova(const QVariantMap& map)
		: initialized(false)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation"))
		return;
		
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
	distance = map.value("distance").toDouble();

	initialized = true;
}

Nova::~Nova()
{
	//
}

QVariantMap Nova::getMap(void)
{
	QVariantMap map;
	map["designation"] = designation;
	map["name"] = novaName;
	map["type"] = novaType;
	map["maxMagnitude"] = maxMagnitude;
	map["minMagnitude"] = minMagnitude;
	map["peakJD"] = peakJD;
	map["m2"] = m2;
	map["m3"] = m3;
	map["m6"] = m6;
	map["m9"] = m9;
	map["RA"] = RA;
	map["Dec"] = Dec;	
	map["distance"] = distance;

	return map;
}

float Nova::getSelectPriority(const StelCore* core) const
{
	//Same as StarWrapper::getSelectPriority()
        return getVMagnitude(core, false);
}

QString Nova::getEnglishName() const
{
	return novaName;
}

QString Nova::getNameI18n() const
{
	return novaName;
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
	double mag = getVMagnitude(core, false);

	if (flags&Name)
	{
		QString name = novaName.isEmpty() ? QString("<h2>%1</h2>").arg(designation) : QString("<h2>%1 (%2)</h2>").arg(novaName).arg(designation);
		oss << name;
	}

	if (flags&Extra1)
		oss << q_("Type: <b>%1</b> (%2)").arg(q_("nova")).arg(novaType) << "<br />";

	if (flags&Magnitude)
	{
	    if (core->getSkyDrawer()->getFlagHasAtmosphere())
		oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(getVMagnitude(core, false), 'f', 2),
									       QString::number(getVMagnitude(core, true), 'f', 2)) << "<br>";
	    else
		oss << q_("Magnitude: <b>%1</b>").arg(mag, 0, 'f', 2) << "<br>";
	}

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Extra1)
	{
		oss << q_("Maximum brightness: %1").arg(getMaxBrightnessDate(peakJD)) << "<br>";
		if (distance>0)
			oss << q_("Distance: %1 Light Years").arg(distance*1000) << "<br>";
	}

	postProcessInfoString(str, flags);
	return str;
}

Vec3f Nova::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : Vec3f(1.0, 1.0, 1.0);
}

float Nova::getVMagnitude(const StelCore* core, bool withExtinction) const
{
	float extinctionMag=0.0; // track magnitude loss
	if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
	{
		Vec3d altAz=getAltAzPosApparent(core);
		altAz.normalize();
		core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
	}

	// OK, start from minimal brightness
	double vmag = minMagnitude;
	double currentJD = core->getJDay();
	double deltaJD = std::abs(peakJD-currentJD);
    
	// Fill "default" values for mX
	int t2;
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
	else
		t2 = m2;

	int t3;
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
	else
		t3 = m3;

	int t6;
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
	else
		t6 = m6;

	int t9;
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
	else
		t9 = m9;

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

	return vmag + extinctionMag;
}

double Nova::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void Nova::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void Nova::draw(StelCore* core, StelRenderer* renderer, StelProjectorP projector)
{
	StelSkyDrawer* sd = core->getSkyDrawer();
	StarMgr* smgr = GETSTELMODULE(StarMgr); // It's need for checking displaying of labels for stars

	Vec3f color = Vec3f(1.f,1.f,1.f);
	if (StelApp::getInstance().getVisionModeNight())
		color = StelUtils::getNightColor(color);

	float rcMag[2], size, shift;
	double mag;

	StelUtils::spheToRect(RA, Dec, XYZ);
	mag = getVMagnitude(core, true);
	sd->preDrawPointSource();
	float mlimit = sd->getLimitMagnitude();

	if (mag <= mlimit)
	{
		sd->computeRCMag(mag, rcMag);
		const Vec3f XYZf(XYZ[0], XYZ[1], XYZ[2]);
		Vec3f win;
		if(sd->pointSourceVisible(&(*projector), XYZf, rcMag, false, win))
		{
			sd->drawPointSource(win, rcMag, color);
		}
		renderer->setGlobalColor(color[0], color[1], color[2], 1);
		size = getAngularSize(NULL)*M_PI/180.*projector->getPixelPerRadAtCenter();
		shift = 6.f + size/1.8f;
		if (labelsFader.getInterstate()<=0.f && (mag+5.f)<mlimit && smgr->getFlagLabels())
		{
			QString name = novaName.isEmpty() ? designation : novaName;
			renderer->drawText(TextParams(XYZ, projector, name).shift(shift, shift).useGravity());
		}
	}

	sd->postDrawPointSource(projector);
}
