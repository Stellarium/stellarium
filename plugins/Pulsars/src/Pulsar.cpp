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
	survey = map.value("survey").toInt();

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
	map["survey"] = survey;

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
			oss << q_("Type: %1").arg(getPulsarTypeInfoString(ntype)) << "<br>";
		}
		if (survey>0)
		{
			oss << q_("Survey: %1").arg(getPulsarSurveyInfoString(survey)) << "<br>";
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
		out.append(q_("globular cluster association"));
	}

	if (flags&S)
	{
		out.append(q_("SNR association"));
	}

	if (flags&G)
	{
		out.append(q_("glitches in period"));
	}

	if (flags&B)
	{
		out.append(q_("binary or multiple pulsar"));
	}

	if (flags&M)
	{
		out.append(q_("millisecond pulsar"));
	}

	if (flags&R)
	{
		out.append(q_("recycled pulsar"));
	}

	if (flags&I)
	{
		out.append(q_("radio interpulse"));
	}

	if (flags&H)
	{
		out.append(q_("optical, X-ray or Gamma-ray pulsed emission (high energy)"));
	}

	if (flags&E)
	{
		out.append(q_("extragalactic (in MC) pulsar"));
	}

	return out.join(", ");
}

QString Pulsar::getPulsarSurveyInfoString(const int flags) const
{
	QStringList out;
	QString data = "";

	if (flags&Molonglo1)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Molonglo"))
			   // Survey
			   .arg(1)
			   // Frequency in MHz
			   .arg(408)
			   .arg(q_("MHz")));
	}

	if (flags&JodrellBank1)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Jodrell Bank"))
			   // Survey
			   .arg(1)
			   // Frequency in MHz
			   .arg(408)
			   .arg(q_("MHz")));
	}

	if (flags&Arecibo1)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Arecibo"))
			   // Survey
			   .arg(1)
			   // Frequency in MHz
			   .arg(430)
			   .arg(q_("MHz")));
	}

	if (flags&Molonglo2)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Molonglo"))
			   // Survey
			   .arg(2)
			   // Frequency in MHz
			   .arg(408)
			   .arg(q_("MHz")));
	}

	if (flags&GreenBank1)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Green Bank"))
			   // Survey
			   .arg(1)
			   // Frequency in MHz
			   .arg(400)
			   .arg(q_("MHz")));
	}

	if (flags&GreenBank2)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Green Bank"))
			   // Survey
			   .arg(2)
			   // Frequency in MHz
			   .arg(400)
			   .arg(q_("MHz")));
	}

	if (flags&JodrellBank2)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Jodrell Bank"))
			   // Survey
			   .arg(2)
			   // Frequency in MHz
			   .arg(1400)
			   .arg(q_("MHz")));
	}

	if (flags&GreenBank3)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Green Bank"))
			   // Survey
			   .arg(3)
			   // Frequency in MHz
			   .arg(400)
			   .arg(q_("MHz")));
	}

	if (flags&Arecibo2)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Arecibo"))
			   // Survey
			   .arg(2)
			   // Frequency in MHz
			   .arg(430)
			   .arg(q_("MHz")));
	}

	if (flags&Parkes1)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Parkes"))
			   // Survey
			   .arg(1)
			   // Frequency in MHz
			   .arg(1520)
			   .arg(q_("MHz")));
	}

	if (flags&Arecibo3)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Arecibo"))
			   // Survey
			   .arg(3)
			   // Frequency in MHz
			   .arg(430)
			   .arg(q_("MHz")));
	}

	if (flags&Parkes70)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Parkes"))
			   // Survey
			   .arg(70)
			   // Frequency in MHz
			   .arg(436)
			   .arg(q_("MHz")));
	}

	if (flags&Arecibo4a)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Arecibo"))
			   // Survey
			   .arg("4a")
			   // Frequency in MHz
			   .arg(430)
			   .arg(q_("MHz")));
	}

	if (flags&Arecibo4b)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Arecibo"))
			   // Survey
			   .arg("4b")
			   // Frequency in MHz
			   .arg(430)
			   .arg(q_("MHz")));
	}

	if (flags&Arecibo4c)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Arecibo"))
			   // Survey
			   .arg("4c")
			   // Frequency in MHz
			   .arg(430)
			   .arg(q_("MHz")));
	}

	if (flags&Arecibo4d)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Arecibo"))
			   // Survey
			   .arg("4d")
			   // Frequency in MHz
			   .arg(430)
			   .arg(q_("MHz")));
	}

	if (flags&Arecibo4e)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Arecibo"))
			   // Survey
			   .arg("4e")
			   // Frequency in MHz
			   .arg(430)
			   .arg(q_("MHz")));
	}

	if (flags&Arecibo4f)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Arecibo"))
			   // Survey
			   .arg("4f")
			   // Frequency in MHz
			   .arg(430)
			   .arg(q_("MHz")));
	}

	if (flags&GreenBank4)
	{
		out.append(QString("%1 %2 (%3 %4)")
			   // TRANSLATORS: Name of survey by radio observatory
			   .arg(q_("Green Bank"))
			   // Survey
			   .arg(4)
			   // Frequency in MHz
			   .arg(370)
			   .arg(q_("MHz")));
	}

	if (out.size()>3)
	{
		for (int i=0; i<out.size(); ++i)
		{
			data += QString(out.at(i).toLocal8Bit().constData());
			div_t r = div(i, 2);
			if (r.rem!=0)
			{
				if (i!=out.size()-1)
				{
					data += ",<br>";
				}
			}
			else
			{
				if (i!=out.size()-1)
				{
					data += ", ";
				}
			}
		}
	}
	else
	{
		data = out.join(", ");
	}

	return data;
}
