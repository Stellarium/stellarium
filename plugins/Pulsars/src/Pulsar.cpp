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

Pulsar::Pulsar(const QVariantMap& map)
		: initialized(false)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation"))
		return;
		
	designation  = map.value("designation").toString();
	distance = map.value("distance").toDouble();
	period = map.value("period").toDouble();
	RA = StelUtils::getDecAngle(map.value("RA").toString());
	DE = StelUtils::getDecAngle(map.value("DE").toString());
	ntype = map.value("ntype").toInt();

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
		oss << q_("Barycentric period: %1 s").arg(period) << "<br>";
		oss << q_("Distance: %1 kpc").arg(distance) << "<br>";
		// TODO: Calculate type of pulsar - see note on ntype on http://cdsarc.u-strasbg.fr/viz-bin/Cat?VII/189
		oss << q_("Pulsar type: %1").arg(ntype) << "<br>";
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

	float vmag = distance + 5.f; //Calculate fake magnitude as function by distance

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

	Vec3f color = Vec3f(1.f,1.f,1.f);
	float rcMag[2], size, shift;
	double mag;

	StelUtils::spheToRect(RA, DE, XYZ);
	mag = getVMagnitude(core, true);
	
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
