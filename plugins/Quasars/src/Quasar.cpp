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

#include "Quasar.hpp"
#include "Quasars.hpp"
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
#include <QVariantMap>
#include <QVariant>
#include <QList>

StelTextureSP Quasar::markerTexture;

bool Quasar::distributionMode = false;
Vec3f Quasar::markerColor = Vec3f(1.0f,0.5f,0.4f);

Quasar::Quasar(const QVariantMap& map)
		: initialized(false),
		  designation(""),
		  VMagnitude(21.),
		  AMagnitude(21.),
		  bV(0.),
		  qRA(0.),
		  qDE(0.),
		  redshift(0.)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation"))
		return;
		
	designation  = map.value("designation").toString();
	VMagnitude = map.value("Vmag").toFloat();
	AMagnitude = map.value("Amag").toFloat();
	bV = map.value("bV").toFloat();
	qRA = StelUtils::getDecAngle(map.value("RA").toString());
	qDE = StelUtils::getDecAngle(map.value("DE").toString());	
	redshift = map.value("z").toFloat();

	initialized = true;
}

Quasar::~Quasar()
{
	//
}

QVariantMap Quasar::getMap(void)
{
	QVariantMap map;
	map["designation"] = designation;
	map["Vmag"] = VMagnitude;
	map["Amag"] = AMagnitude;
	map["bV"] = bV;
	map["RA"] = qRA;
	map["DE"] = qDE;	
	map["z"] = redshift;

	return map;
}

QString Quasar::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
        double mag = getVMagnitude(core);

	if (flags&Name)
	{
		oss << "<h2>" << designation << "</h2>";
	}
	if (flags&ObjectType)
		oss << q_("Type: <b>%1</b>").arg(q_("quasar")) << "<br />";

	if (flags&Magnitude)
	{
		if (core->getSkyDrawer()->getFlagHasAtmosphere())
		{
			oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(mag, 'f', 2),
											QString::number(getVMagnitudeWithExtinction(core),  'f', 2)) << "<br />";
		}
		else
		{
			oss << q_("Magnitude: <b>%1</b>").arg(mag, 0, 'f', 2) << "<br />";
		}
		if (AMagnitude!=0)
		{
			oss << q_("Absolute Magnitude: %1").arg(AMagnitude, 0, 'f', 2) << "<br />";
		}
	}

	if (flags&Extra)
	{
		oss << q_("Color Index (B-V): <b>%1</b>").arg(QString::number(bV, 'f', 2)) << "<br>";
	}
	
	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Extra)
	{
		if (redshift>0)
		{
			oss << q_("Z (redshift): %1").arg(redshift) << "<br />";
		}
	}

	postProcessInfoString(str, flags);
	return str;
}

Vec3f Quasar::getInfoColor(void) const
{
	return Vec3f(1.0, 1.0, 1.0);
}

float Quasar::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core);
	return VMagnitude;
}

double Quasar::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

float Quasar::getSelectPriority(const StelCore* core) const
{
	float mag = getVMagnitudeWithExtinction(core);
	if (distributionMode)
		mag = 4.f;
	return mag;
}

void Quasar::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void Quasar::draw(StelCore* core, StelPainter& painter)
{
	StelSkyDrawer* sd = core->getSkyDrawer();

	Vec3f color = sd->indexToColor(BvToColorIndex(bV))*0.75f;	
	RCMag rcMag;
	float size, shift=0;
	double mag;

	StelUtils::spheToRect(qRA, qDE, XYZ);
	mag = getVMagnitudeWithExtinction(core);	

	if (distributionMode)
	{
		painter.enableBlend(true, false, __FILE__, __LINE__);
		painter.setBlendFunc(GL_ONE, GL_ONE);
		painter.setColor(markerColor[0], markerColor[1], markerColor[2], 1);

		Quasar::markerTexture->bind();
		//size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
		if (labelsFader.getInterstate()<=0.f)
		{
			painter.drawSprite2dMode(XYZ, 4);			
		}
	}
	else
	{
		sd->preDrawPointSource(&painter);
	
		if (mag <= sd->getLimitMagnitude())
		{
			sd->computeRCMag(mag, &rcMag);
			sd->drawPointSource(&painter, Vec3f(XYZ[0],XYZ[1],XYZ[2]), rcMag, sd->indexToColor(BvToColorIndex(bV)), true);
			painter.setColor(color[0], color[1], color[2], 1);
			size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
			shift = 6.f + size/1.8f;
			if (labelsFader.getInterstate()<=0.f)
			{
				painter.drawText(XYZ, designation, 0, shift, shift, false);
			}
		}

		sd->postDrawPointSource(&painter);
	}
}

unsigned char Quasar::BvToColorIndex(float b_v)
{
	double dBV = b_v;
	dBV *= 1000.0;
	if (dBV < -500)
	{
		dBV = -500;
	}
	else if (dBV > 3499)
	{
		dBV = 3499;
	}
	return (unsigned int)floor(0.5+127.0*((500.0+dBV)/4000.0));
}
