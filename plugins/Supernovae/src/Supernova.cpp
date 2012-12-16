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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelRenderer.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QVariant>
#include <QList>

Supernova::Supernova(const QVariantMap& map)
		: initialized(false)
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

float Supernova::getSelectPriority(const StelCore* core) const
{
	//Same as StarWrapper::getSelectPriority()
        return getVMagnitude(core, false);
}

QString Supernova::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	double mag = getVMagnitude(core, false);

	if (flags&Name)
	{
		oss << "<h2>" << designation;
		if (note.size()!=0)
		    oss << " (" << q_(note) << ")";
		
		oss << "</h2>";
	}

	if (flags&Extra1)
		oss << q_("Type: <b>%1</b>").arg(q_("supernova")) << "<br />";

	if (flags&Magnitude && mag <= core->getSkyDrawer()->getLimitMagnitude())
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
		oss << q_("Type of supernova: %1").arg(sntype) << "<br>";
		if (distance>0)
			oss << q_("Distance: %1 Light Years").arg(distance*1000) << "<br>";
	}

	postProcessInfoString(str, flags);
	return str;
}

Vec3f Supernova::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : Vec3f(1.0, 1.0, 1.0);
}

float Supernova::getVMagnitude(const StelCore* core, bool withExtinction) const
{
	float extinctionMag=0.0; // track magnitude loss
	if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
	{
	    Vec3d altAz=getAltAzPosApparent(core);
	    altAz.normalize();
	    core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
	}

	double vmag = 20;
	double currentJD = core->getJDay();
	double deltaJD = std::abs(peakJD-currentJD);

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

	return vmag + extinctionMag;
}

double Supernova::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void Supernova::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void Supernova::draw(StelCore* core, StelRenderer* renderer, StelProjectorP projector)
{
	StelSkyDrawer* sd = core->getSkyDrawer();

	Vec3f color = Vec3f(1.f,1.f,1.f);
	if (StelApp::getInstance().getVisionModeNight())
		color = StelUtils::getNightColor(color);

	float rcMag[2], size, shift;
	double mag;

	StelUtils::spheToRect(snra, snde, XYZ);
	mag = getVMagnitude(core, true);
	sd->preDrawPointSource();
	
	if (mag <= sd->getLimitMagnitude())
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
		if (labelsFader.getInterstate()<=0.f)
		{
			renderer->drawText(TextParams(XYZ, projector, designation).shift(shift, shift).useGravity());
		}
	}

	sd->postDrawPointSource(projector);
}
