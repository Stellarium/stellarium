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
#include <QtOpenGL>
#include <QVariantMap>
#include <QVariant>
#include <QList>

StelTextureSP Quasar::markerTexture;

Quasar::Quasar(const QVariantMap& map)
		: initialized(false)
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

float Quasar::getSelectPriority(const StelCore* core) const
{
	//Same as StarWrapper::getSelectPriority()
        return getVMagnitude(core, false);
}

QString Quasar::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
        double mag = getVMagnitude(core, false);

	if (flags&Name)
	{
		oss << "<h2>" << designation << "</h2>";
	}
	if (flags&Extra1)
		oss << q_("Type: <b>%1</b>").arg(q_("quasar")) << "<br />";

	if (flags&Magnitude)
	{
		if (core->getSkyDrawer()->getFlagHasAtmosphere())
		{
			if (bV!=0)
			{
				oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>. B-V: <b>%3</b>)").arg(QString::number(mag, 'f', 2),
														QString::number(getVMagnitude(core, true),  'f', 2),
														QString::number(bV, 'f', 2)) << "<br />";
			}
			else
			{
				oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(mag, 'f', 2),
												QString::number(getVMagnitude(core, true),  'f', 2)) << "<br />";
			}
		}
		else
		{
			if (bV!=0)
			{
				oss << q_("Magnitude: <b>%1</b> (B-V: <b>%2</b>)").arg(mag, 0, 'f', 2).arg(bV, 0, 'f', 2) << "<br />";
			}
			else
			{
				oss << q_("Magnitude: <b>%1</b>").arg(mag, 0, 'f', 2) << "<br />";
			}
		}
		if (AMagnitude!=0)
		{
			oss << q_("Absolute Magnitude: %1").arg(AMagnitude, 0, 'f', 2) << "<br />";
		}
	}

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Extra1)
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
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : Vec3f(1.0, 1.0, 1.0);
}

float Quasar::getVMagnitude(const StelCore* core, bool withExtinction) const
{
	float extinctionMag=0.0; // track magnitude loss
	if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
	{
		Vec3d altAz=getAltAzPosApparent(core);
		altAz.normalize();
		core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
	}

	return VMagnitude + extinctionMag;
}

double Quasar::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void Quasar::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void Quasar::draw(StelCore* core, StelPainter& painter)
{
	StelSkyDrawer* sd = core->getSkyDrawer();

	Vec3f color = sd->indexToColor(BvToColorIndex(bV))*0.75f;
	Vec3f dcolor = Vec3f(1.2f,0.5f,0.4f);
	float rcMag[2], size, shift;
	double mag;

	StelUtils::spheToRect(qRA, qDE, XYZ);
        mag = getVMagnitude(core, true);	

	if (GETSTELMODULE(Quasars)->getDisplayMode())
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		painter.setColor(dcolor[0], dcolor[1], dcolor[2], 1);

		Quasar::markerTexture->bind();
		size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
		shift = 5.f + size/1.6f;
		if (labelsFader.getInterstate()<=0.f)
		{
			painter.drawSprite2dMode(XYZ, 4);
			painter.drawText(XYZ, " ", 0, shift, shift, false);
		}
	}
	else
	{
		sd->preDrawPointSource(&painter);
	
		if (mag <= sd->getLimitMagnitude())
		{
			sd->computeRCMag(mag, rcMag);
			//sd->drawPointSource(&painter, Vec3f(XYZ[0], XYZ[1], XYZ[2]), rcMag, sd->indexToColor(BvToColorIndex(bV)), false);
			sd->drawPointSource(&painter, XYZ, rcMag, sd->indexToColor(BvToColorIndex(bV)), false);
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
