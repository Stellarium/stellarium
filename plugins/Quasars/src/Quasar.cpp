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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Quasar.hpp"
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

Quasar::Quasar(const QVariantMap& map)
		: initialized(false)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation"))
		return;
		
	designation  = map.value("designation").toString();
	VMagnitude = map.value("Vmag").toFloat();
	bV = map.value("BV").toFloat();
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
	map["BV"] = bV;
	map["RA"] = qRA;
	map["DE"] = qDE;	
	map["z"] = redshift;

	return map;
}

float Quasar::getSelectPriority(const StelCore* core) const
{
	//Same as StarWrapper::getSelectPriority()
	return getVMagnitude(core);
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

	if (flags&Magnitude && mag <= core->getSkyDrawer()->getLimitMagnitude())
	{
		if (bV!=0)
		{
			oss << q_("Magnitude: <b>%1</b> (B-V: %2)").arg(mag, 0, 'f', 2).arg(bV, 0, 'f', 2) << "<br>";
		}
		else
		{
			oss << q_("Magnitude: <b>%1</b>").arg(mag, 0, 'f', 2) << "<br>";
		}
	}

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Extra1)
	{
		oss << q_("Z (redshift): %1").arg(redshift) << "<br>";
	}

	postProcessInfoString(str, flags);
	return str;
}

Vec3f Quasar::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : Vec3f(1.0, 1.0, 1.0);
}

float Quasar::getVMagnitude(const StelCore* core) const
{
	return VMagnitude;	
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

	Vec3f color = Vec3f(1.f,1.f,1.f);
	float rcMag[2], size, shift;
	double mag;

	StelUtils::spheToRect(qRA, qDE, XYZ);
	mag = getVMagnitude(core);
	
	if (mag <= sd->getLimitMagnitude())
	{
		sd->computeRCMag(mag, rcMag);
		sd->drawPointSource(&painter, Vec3f(XYZ[0], XYZ[1], XYZ[2]), rcMag, color, false);
		painter.setColor(color[0], color[1], color[2], 1);
		size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
		shift = 6.f + size/1.8f;
		if (labelsFader.getInterstate()<=0.f)
		{
			painter.drawText(XYZ, designation, 0, shift, shift, false);
		}
	}
}
