/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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

#include "Satellite.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelNavigator.hpp"
#include "StelLocation.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "VecMath.hpp"
#include "StelUtils.hpp"
#include "sgp4sdp4/sgp4sdp4.h"

#include <QTextStream>
#include <QRegExp>
#include <QDebug>
#include <QVariant>

// static data members - will be initialised in the Satallites class (the StelObjectMgr)
StelTextureSP Satellite::hintTexture;
float Satellite::showLabels = true;
float Satellite::hintBrightness = 0.0;
float Satellite::hintScale = 1.f;

Satellite::Satellite(const QVariantMap& map)
	: initialized(false), visible(true), hintColor(0.0,0.0,0.0)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation") || !map.contains("tle1") || !map.contains("tle2"))
		return;
	
		
	designation  = map.value("designation").toString();
	strncpy(elements[0], "DUMMY", 5);
	strncpy(elements[1], qPrintable(map.value("tle1").toString()), 80);
	strncpy(elements[2], qPrintable(map.value("tle2").toString()), 80);
	if (map.contains("description")) description = map.value("description").toString();
	if (map.contains("visible")) visible = map.value("visible").toBool();

	if (map.contains("hintColor"))
	{
		if (map.value("hintColor").toList().count() == 3)
		{
			hintColor[0] = map.value("hintColor").toList().at(0).toDouble();
			hintColor[1] = map.value("hintColor").toList().at(1).toDouble();
			hintColor[2] = map.value("hintColor").toList().at(2).toDouble();
		}
	}

	if (map.contains("comms"))
	{
		foreach(QVariant comm, map.value("comms").toList())
		{
			QVariantMap commMap = comm.toMap();
			commLink c;
			if (commMap.contains("frequency")) c.frequency = commMap.value("frequency").toDouble();
			if (commMap.contains("modulation")) c.modulation = commMap.value("modulation").toString();
			if (commMap.contains("description")) c.description = commMap.value("description").toString();
			comms.append(c);
		}
	}

	if (map.contains("groups"))
	{
		foreach(QVariant group, map.value("groups").toList())
		{
			if (!groupIDs.contains(group.toString()))
				groupIDs << group.toString();
		}
	}

	setObserverLocation();
	initialized = true;
}

Satellite::~Satellite()
{
}

QVariantMap Satellite::getMap(void)
{
	QVariantMap map;
	map["designation"] = designation;
	map["visible"] = visible;
	map["tle1"] = QString(elements[1]);
	map["tle2"] = QString(elements[2]);
	QVariantList col;
	col << (double)hintColor[0] << (double)hintColor[1] << (double)hintColor[2];
	map["hintColor"] = col;
	QVariantList commList;
	foreach(commLink c, comms)
	{
		QVariantMap commMap;
		commMap["frequency"] = c.frequency;
		if (!c.modulation.isEmpty() && c.modulation != "") commMap["modulation"] = c.modulation;
		if (!c.description.isEmpty() && c.description != "") commMap["description"] = c.description;
		commList << commMap;
	}
	map["comms"] = commList;
	return map;
}

float Satellite::getSelectPriority(const StelNavigator *nav) const
{
	return -10.;
}

QString Satellite::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name) 
	{
		oss << "<h2>" << designation << "</h2><br>";
		if (description!="")
			oss << description << "<br>";
	}

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Extra1)
	{
		oss << QString("Range (km): <b>%1</b>").arg(range, 5, 'f', 2) << "<br>";
		oss << QString("Range rate (km/s): <b>%1</b>").arg(rangeRate, 5, 'f', 3) << "<br>";
		oss << QString("Altitude (km): <b>%1</b>").arg(height, 5, 'f', 2) << "<br>";
	}

	if (flags&Extra2 && comms.size() > 0)
	{
		foreach (commLink c, comms)
		{
			double dop = getDoppler(c.frequency);
			double ddop = dop;
			char sign;
			if (dop<0.)
			{
				sign='-';
				ddop*=-1;
			}
			else
				sign='+';

			oss << "<p>";
			if (!c.modulation.isEmpty() && c.modulation != "") oss << "  " << c.modulation;
			if (!c.description.isEmpty() && c.description != "") oss << "  " << c.description;
			if ((!c.modulation.isEmpty() && c.modulation != "") || (!c.description.isEmpty() && c.description != "")) oss << "<br>";
			oss << QString("%1 MHz (%2%3 kHz)</p>").arg(c.frequency, 8, 'f', 5)
			                                       .arg(sign)
			                                       .arg(ddop, 6, 'f', 3);
		}		
	}

	postProcessInfoString(str, flags);
	return str;
}

void Satellite::setObserverLocation(StelLocation* loc)
{
	StelLocation l;
	if (loc==NULL)
	{
		l = StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation();
	}
	else
	{
		l = *loc;
	}
	obs_geodetic.lon = l.longitude * de2ra;
	obs_geodetic.lat = l.latitude * de2ra;
	obs_geodetic.alt = l.altitude / 1000.0;
}

Vec3f Satellite::getInfoColor(void) const {
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : hintColor;
}

float Satellite::getVMagnitude(const StelNavigator* nav) const
{
	return 5.0;
}

double Satellite::getAngularSize(const StelCore *core) const
{
	return 0.00001;
}

void Satellite::update(double deltaTime)
{
	ClearFlag(ALL_FLAGS);
	Get_Next_Tle_Set(elements, &tle);
	memcpy(&localtle, &tle, sizeof(tle_t));
	select_ephemeris(&tle);
	double jul_epoch, jul_utc, tsince, phase;
	vector_t vel = {0,0,0,0};
	vector_t pos = {0,0,0,0};
	vector_t obs_set;
	geodetic_t sat_geodetic;
	jul_utc = StelApp::getInstance().getCore()->getNavigator()->getJDay();
	jul_epoch = Julian_Date_of_Epoch(tle.epoch);
	tsince = (jul_utc - jul_epoch) * xmnpda;

	if (isFlagSet(DEEP_SPACE_EPHEM_FLAG))
		SDP4(tsince, &tle, &pos, &vel, &phase);
	else
		SGP4(tsince, &tle, &pos, &vel, &phase);

	Convert_Sat_State(&pos, &vel);
	SgpMagnitude(&vel); // scalar magnitude, not brightness...
	velocity=vel.w;

	Calculate_Obs(jul_utc, &pos, &vel, &obs_geodetic, &obs_set);
	Calculate_LatLonAlt(jul_utc, &pos, &sat_geodetic);

	azimuth=Degrees(obs_set.x);
	elevation=Degrees(obs_set.y);
	range=obs_set.z;
	rangeRate=obs_set.w;
	height=sat_geodetic.alt;
}

double Satellite::getDoppler(double freq) const
{
	double result;
	double f = freq * 1000000;
	result = -f*((rangeRate*1000.0)/SPEED_OF_LIGHT);
	return result/1000000;
}

void Satellite::draw(const StelCore* core, StelPainter& painter, float maxMagLabel)
{
	float a = (azimuth-90)*M_PI/180;
	Vec3f pos(sin(a),cos(a), tan(elevation * M_PI / 180.));
	XYZ = core->getNavigator()->j2000ToEquinoxEqu(core->getNavigator()->altAzToEquinoxEqu(pos));
	StelApp::getInstance().getVisionModeNight() ? glColor4f(0.6,0.0,0.0,1.0) : glColor4f(hintColor[0],hintColor[1],hintColor[2], Satellite::hintBrightness);

	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	Vec3d xy;
	if (prj->project(XYZ,xy))
	{
		if (Satellite::showLabels)
		{
			painter.drawText(xy[0], xy[1], designation, 0, 10, 10, false);
			Satellite::hintTexture->bind();
		}
		painter.drawSprite2dMode(xy[0], xy[1], 11);
	}
}

