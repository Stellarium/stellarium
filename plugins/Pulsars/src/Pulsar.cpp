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
#include "StelModuleMgr.hpp"
#include "StelSkyDrawer.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QtOpenGL>
#include <QVariantMap>
#include <QVariant>
#include <QList>

StelTextureSP Pulsar::markerTexture;

Pulsar::Pulsar(const QVariantMap& map)
		: initialized(false)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation"))
		return;
		
	designation  = map.value("designation").toString();
	distance = map.value("distance").toFloat();
	period = map.value("period").toDouble();
	RA = StelUtils::getDecAngle(map.value("RA").toString());
	DE = StelUtils::getDecAngle(map.value("DE").toString());
	ntype = map.value("ntype").toInt();
	We = map.value("We").toFloat();
	w50 = map.value("w50").toFloat();
	s400 = map.value("s400").toFloat();
	s600 = map.value("s600").toFloat();
	s1400 = map.value("s1400").toFloat();

	initialized = true;
}

Pulsar::~Pulsar()
{
	//
}

QVariantMap Pulsar::getMap(void)
{
	QVariantMap map;
	map["designation"] = designation;
	map["distance"] = distance;
	map["RA"] = RA;
	map["DE"] = DE;
	map["period"] = period;
	map["ntype"] = ntype;	
	map["We"] = We;
	map["w50"] = w50;
	map["s400"] = s400;
	map["s600"] = s600;
	map["s1400"] = s1400;

	return map;
}

float Pulsar::getSelectPriority(const StelCore* core) const
{
	//Same as StarWrapper::getSelectPriority()
        return getVMagnitude(core, false);
}

QString Pulsar::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>" << designation << "</h2>";
	}

	if (flags&Extra1)
	    oss << q_("Type: <b>%1</b>").arg(q_("pulsar")) << "<br />";

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Extra1)
	{
		oss << q_("Barycentric period: %1 s").arg(QString::number(period, 'f', 16)) << "<br>";
		if (distance>0)
		{
			oss << q_("Distance: %1 kpc (%2 ly)").arg(distance).arg(distance*3261.563777) << "<br>";
		}
		if (ntype>0)
		{
			oss << q_("Type of pulsar: %1").arg(getPulsarTypeInfoString(ntype)) << "<br>";
		}
		if (We>0)
		{
			oss << q_("Equivalent width of the integrated pulse profile: %1 ms").arg(QString::number(We, 'f', 2)) << "<br>";
		}
		if (w50>0)
		{
			oss << q_("Profile width at 50% of peak: %1 ms").arg(QString::number(w50, 'f', 2)) << "<br>";
		}
		if (s400>0)
		{
			oss << QString("%1 %2%3: %4 %5")
			       // TRANSLATORS: Full phrase is "Time averaged flux density at XXXMHz"
			       .arg(q_("Time averaged flux density at"))
			       .arg(400)
			       // TRANSLATORS: Unit of measurement of frequency
			       .arg(q_("MHz"))
			       .arg(QString::number(s400, 'f', 2))
			       // TRANSLATORS: mJy is milliJansky(10-26W/m2/Hz)
			       .arg(q_("mJy")) << "<br>";
		}
		if (s600>0)
		{
			oss << QString("%1 %2%3: %4 %5")
			       // TRANSLATORS: Full phrase is "Time averaged flux density at XXXMHz"
			       .arg(q_("Time averaged flux density at"))
			       .arg(600)
			       // TRANSLATORS: Unit of measurement of frequency
			       .arg(q_("MHz"))
			       .arg(QString::number(s600, 'f', 2))
			       // TRANSLATORS: mJy is milliJansky(10-26W/m2/Hz)
			       .arg(q_("mJy")) << "<br>";
		}
		if (s1400>0)
		{
			oss << QString("%1 %2%3: %4 %5")
			       // TRANSLATORS: Full phrase is "Time averaged flux density at XXXMHz"
			       .arg(q_("Time averaged flux density at"))
			       .arg(1400)
			       // TRANSLATORS: Unit of measurement of frequency
			       .arg(q_("MHz"))
			       .arg(QString::number(s1400, 'f', 2))
			       // TRANSLATORS: mJy is milliJansky(10-26W/m2/Hz)
			       .arg(q_("mJy")) << "<br>";
		}
	}

	postProcessInfoString(str, flags);
	return str;
}

Vec3f Pulsar::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : Vec3f(1.0, 1.0, 1.0);
}

float Pulsar::getVMagnitude(const StelCore* core, bool withExtinction) const
{
	float extinctionMag=0.0; // track magnitude loss
	if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
	{
	    Vec3d altAz=getAltAzPosApparent(core);
	    altAz.normalize();
	    core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
	}

	// Calculate fake visual magnitude as function by distance - minimal magnitude is 6
	float vmag = distance + 6.f;

	return vmag + extinctionMag;
}

double Pulsar::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void Pulsar::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void Pulsar::draw(StelCore* core, StelPainter& painter)
{
	StelSkyDrawer* sd = core->getSkyDrawer();	

	Vec3f color = Vec3f(0.4f,0.5f,1.2f);
	double mag = getVMagnitude(core, true);

	StelUtils::spheToRect(RA, DE, XYZ);			
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	painter.setColor(color[0], color[1], color[2], 1);

	if (mag <= sd->getLimitMagnitude())
	{

		Pulsar::markerTexture->bind();
		float size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
		float shift = 5.f + size/1.6f;
		if (labelsFader.getInterstate()<=0.f)
		{
			painter.drawSprite2dMode(XYZ, 5);
			painter.drawText(XYZ, designation, 0, shift, shift, false);
		}
	}
}

QString Pulsar::getPulsarTypeInfoString(const int flags) const
{
	QStringList out;

	if (flags&C)
	{
		// TRANSLATORS: Type of pulsar
		out.append(q_("globular cluster association"));
	}

	if (flags&S)
	{
		// TRANSLATORS: Type of pulsar
		out.append(q_("SNR association"));
	}

	if (flags&G)
	{
		// TRANSLATORS: Type of pulsar
		out.append(q_("glitches in period"));
	}

	if (flags&B)
	{
		// TRANSLATORS: Type of pulsar
		out.append(q_("binary or multiple pulsar"));
	}

	if (flags&M)
	{
		// TRANSLATORS: Type of pulsar
		out.append(q_("millisecond pulsar"));
	}

	if (flags&R)
	{
		// TRANSLATORS: Type of pulsar
		out.append(q_("recycled pulsar"));
	}

	if (flags&I)
	{
		// TRANSLATORS: Type of pulsar
		out.append(q_("radio interpulse"));
	}

	if (flags&H)
	{
		// TRANSLATORS: Type of pulsar
		out.append(q_("optical, X-ray or Gamma-ray pulsed emission (high energy)"));
	}

	if (flags&E)
	{
		// TRANSLATORS: Type of pulsar
		out.append(q_("extragalactic (in MC) pulsar"));
	}

	return out.join(", ");
}
