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

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QtOpenGL>
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

	return map;
}

float Supernova::getSelectPriority(const StelCore*) const
{
	return -10.f;
}

QString Supernova::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	double mag = getVMagnitude(core);

	if (flags&Name)
	{
		oss << "<h2>" << designation;
		if (note.size()!=0)
		    oss << " (" << note << ")";
		
		oss << "</h2>";
	}

	if (flags&Magnitude && mag <= core->getSkyDrawer()->getLimitMagnitude())
		oss << q_("Magnitude: <b>%1</b>").arg(mag, 0, 'f', 2) << "<br>";

	if (flags&Extra1)
		oss << q_("Type: %1").arg(sntype) << "<br>";

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	postProcessInfoString(str, flags);
	return str;
}

Vec3f Supernova::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : Vec3f(1.0, 1.0, 1.0);
}

float Supernova::getVMagnitude(const StelCore* core) const
{
	double vmag = 20;
	double currentJD = core->getJDay();
	double deltaJD = std::abs(peakJD-currentJD);

	/*
	if (peakJD<=currentJD)
	{
		if (peakJD==std::floor(currentJD))
			vmag = maxMagnitude;

		else
			vmag = maxMagnitude - 2.5 * (-3) * std::log10(deltaJD);
	}
	else
	{
		if (std::abs(peakJD-currentJD)<=5)
			vmag = maxMagnitude - 2.5 * (-1.75) * std::log10(std::abs(deltaJD));

		if (vmag>maxMagnitude)
			vmag = maxMagnitude;
	}
	*/

	// Use supernova light curve model from here - http://www.astronet.ru/db/msg/1188703

	if (sntype.contains("II", Qt::CaseSensitive)) {
		// Type II
		if (peakJD<=currentJD)
		{
			if (deltaJD>=0 && deltaJD<=5)
				vmag = maxMagnitude;

			if (deltaJD>5 && deltaJD<=35)
				vmag = maxMagnitude + 0.05 * deltaJD;

			if (deltaJD>35 && deltaJD<=85)
				vmag = maxMagnitude + 0.013 * deltaJD + 1.5;

			if (deltaJD>85)
				vmag = maxMagnitude + 0.05 * deltaJD + 2.15;
		}
		else
		{
			if (deltaJD<=20)
				vmag = maxMagnitude - 2.5 * (-2) * std::log10(deltaJD);

		}
	}
	else
	{
		// Type I
		if (peakJD<=currentJD)
		{
			if (deltaJD>=0 && deltaJD<=5)
				vmag = maxMagnitude;

			if (deltaJD>5 && deltaJD<=30)
				vmag = maxMagnitude + 0.1 * deltaJD;

			if (deltaJD>30)
				vmag = maxMagnitude + 0.016 * deltaJD + 2.5;

		}
		else
		{
			if (deltaJD<=15)
				vmag = maxMagnitude - 2.5 * (-1.75) * std::log10(deltaJD);

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

void Supernova::update(double)
{
	//
}

void Supernova::draw(StelCore* core, StelPainter& painter)
{
	StelSkyDrawer* sd = core->getSkyDrawer();

	Vec3f color = Vec3f(1.f,1.f,1.f);
	float rcMag[2], size, shift;
	double mag;

	StelUtils::spheToRect(snra, snde, XYZ);
	mag = getVMagnitude(core);
	
	if (mag <= sd->getLimitMagnitude())
	{
		sd->computeRCMag(mag, rcMag);
		sd->drawPointSource(&painter, Vec3f(XYZ[0], XYZ[1], XYZ[2]), rcMag, color, false);
		painter.setColor(color[0], color[1], color[2], 1);
		size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
		shift = 6.f + size/1.8f;
		painter.drawText(XYZ, designation, 0, shift, shift, false);
	}
}
