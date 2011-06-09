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
#include "StelLocation.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "VecMath.hpp"
#include "StelUtils.hpp"

#include <QTextStream>
#include <QRegExp>
#include <QDebug>
#include <QVariant>
#include <QtOpenGL>
#include <QSettings>
#include <QByteArray>

#include "gsatellite/gTime.hpp"

#include <cmath>

// static data members - will be initialised in the Satallites class (the StelObjectMgr)
StelTextureSP Satellite::hintTexture;
float Satellite::showLabels = true;
float Satellite::hintBrightness = 0.0;
float Satellite::hintScale = 1.f;
SphericalCap Satellite::viewportHalfspace = SphericalCap();
int Satellite::orbitLineSegments = 90;
int Satellite::orbitLineFadeSegments = 4;
int Satellite::orbitLineSegmentDuration = 20;
bool Satellite::orbitLinesFlag = true;


Satellite::Satellite(const QVariantMap& map)
		: initialized(false), visible(true), hintColor(0.0,0.0,0.0), lastUpdated(), pSatWrapper(NULL)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation") || !map.contains("tle1") || !map.contains("tle2"))
		return;

	font.setPixelSize(16);

	designation  = map.value("designation").toString();
	if (map.contains("description")) description = map.value("description").toString();
	if (map.contains("visible")) visible = map.value("visible").toBool();
	if (map.contains("orbitVisible")) orbitVisible = map.value("orbitVisible").toBool();

	if (map.contains("hintColor"))
	{
		if (map.value("hintColor").toList().count() == 3)
		{
			hintColor[0] = map.value("hintColor").toList().at(0).toDouble();
			hintColor[1] = map.value("hintColor").toList().at(1).toDouble();
			hintColor[2] = map.value("hintColor").toList().at(2).toDouble();
		}
	}

	if (map.contains("orbitColor"))
	{
		if (map.value("orbitColor").toList().count() == 3)
		{
			orbitColorNormal[0] = map.value("orbitColor").toList().at(0).toDouble();
			orbitColorNormal[1] = map.value("orbitColor").toList().at(1).toDouble();
			orbitColorNormal[2] = map.value("orbitColor").toList().at(2).toDouble();
		}
	}
	else
	{
		orbitColorNormal = hintColor;
	}

	// Set the night color of orbit lines to red with the
	// intensity of the average of the RGB for the day color.
	float orbitColorBrightness = (orbitColorNormal[0] + orbitColorNormal[1] + orbitColorNormal[2])/3;
	orbitColorNight[0] = orbitColorBrightness;
	orbitColorNight[1] = 0;
	orbitColorNight[2] = 0;

	if (StelApp::getInstance().getVisionModeNight())
		orbitColor = &orbitColorNight;
	else
		orbitColor = &orbitColorNormal;

	if (map.contains("comms"))
	{
                foreach(const QVariant &comm, map.value("comms").toList())
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
                foreach(const QVariant &group, map.value("groups").toList())
		{
			if (!groupIDs.contains(group.toString()))
				groupIDs << group.toString();
		}
	}

	setNewTleElements(map.value("tle1").toString(), map.value("tle2").toString());

	if (map.contains("lastUpdated"))
	{
		lastUpdated = map.value("lastUpdated").toDateTime();
	}
	initialized = true;
}

Satellite::~Satellite()
{
	if (pSatWrapper != NULL)
		delete pSatWrapper;
}

double Satellite::roundToDp(float n, int dp)
{
	// round n to dp decimal places
	return floor(n * pow(10., dp) + .5) / pow(10., dp);
}

QVariantMap Satellite::getMap(void)
{
	QVariantMap map;
	map["designation"] = designation;
	map["tle1"] = tleElements.first.data();
	map["tle2"] = tleElements.second.data();

	if (!description.isEmpty() && description!="")
		map["description"] = description;

	map["visible"]     = visible;
	map["orbitVisible"] = orbitVisible;
	QVariantList col, orbitCol;
	col << roundToDp(hintColor[0],3) << roundToDp(hintColor[1], 3) << roundToDp(hintColor[2], 3);
	orbitCol << roundToDp(orbitColorNormal[0], 3) << roundToDp(orbitColorNormal[1], 3) << roundToDp(orbitColorNormal[2],3);
	map["hintColor"] = col;
	map["orbitColor"] = orbitCol;
	QVariantList commList;
        foreach(const commLink &c, comms)
	{
		QVariantMap commMap;
		commMap["frequency"] = c.frequency;
		if (!c.modulation.isEmpty() && c.modulation != "") commMap["modulation"] = c.modulation;
		if (!c.description.isEmpty() && c.description != "") commMap["description"] = c.description;
		commList << commMap;
	}
	map["comms"] = commList;
	QVariantList groupList;
        foreach(const QString &g, groupIDs)
	{
		groupList << g;
	}
	map["groups"] = groupList;

	if (!lastUpdated.isNull())
	{
		map["lastUpdated"] = lastUpdated;
	}

	return map;
}

float Satellite::getSelectPriority(const StelCore*) const
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
		oss << "<p>";
		oss << QString("Range (km): <b>%1</b>").arg(range, 5, 'f', 2) << "<br>";
		oss << QString("Range rate (km/s): <b>%1</b>").arg(rangeRate, 5, 'f', 3) << "<br>";
		oss << QString("Altitude (km): <b>%1</b>").arg(height, 5, 'f', 2) << "<br>";
                oss << QString("SubPoint Lat/Long(Deg): <b>%1</b>").arg(latLongSubPointPosition[0], 5, 'f', 2) << "/";
                oss << QString("<b>%1</b>").arg(latLongSubPointPosition[1], 5, 'f', 3);
		oss << "</p>";

		oss << "TEME Coordinates(km):  ";
                oss << QString("<b>X:</b> %1 ").arg(position[0], 5, 'f', 2);
                oss << QString("<b>Y:</b> %1 ").arg(position[1], 5, 'f', 2);
                oss << QString("<b>Z:</b> %1 ").arg(position[2], 5, 'f', 2) << "<br>";
		oss << "TEME Vel(km/s):  ";
                oss << QString("<b>X:</b> %1 ").arg(velocity[0], 5, 'f', 2);
                oss << QString("<b>Y:</b> %1 ").arg(velocity[1], 5, 'f', 2);
                oss << QString("<b>Z:</b> %1 ").arg(velocity[2], 5, 'f', 2) << "<br>";

		oss << "Visibility:  ";
                switch (visibility)
		{
		case RADAR_SUN:
			oss << "Sat&Observer in Sunlit" << "<br>";
			break;
		case VISIBLE:
			oss << "Visible" << "<br>";
			break;
		case RADAR_NIGHT:
			oss << "Sat. Eclipsed" << "<br>";
			break;
		case NOT_VISIBLE:
			oss << "Sat. Not Visible" << "<br>";
			break;
		default:
			break;

		}
	}

	if (flags&Extra2 && comms.size() > 0)
	{
                foreach(const commLink &c, comms)
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

Vec3f Satellite::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : hintColor;
}

float Satellite::getVMagnitude(const StelCore*) const
{
	return 5.0;
}

double Satellite::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void Satellite::setNewTleElements(const QString& tle1, const QString& tle2)
{
	if (pSatWrapper)
	{
		gSatWrapper *old = pSatWrapper;
		pSatWrapper = NULL;
		delete old;
	}

	tleElements.first.clear();
	tleElements.first.append(tle1);
	tleElements.second.clear();
	tleElements.second.append(tle2);

	pSatWrapper = new gSatWrapper(designation, tle1, tle2);
	orbitPoints.clear();
}

void Satellite::update(double)
{
	if (pSatWrapper)
	{
		epochTime = StelApp::getInstance().getCore()->getJDay();

		pSatWrapper->setEpoch(epochTime);
                position                 = pSatWrapper->getTEMEPos();
                velocity                 = pSatWrapper->getTEMEVel();
                latLongSubPointPosition  = pSatWrapper->getSubPoint();
                height                   = latLongSubPointPosition[2];
                elAzPosition             = pSatWrapper->getAltAz();
                elAzPosition.normalize();

		pSatWrapper->getSlantRange(range, rangeRate);
                visibility = pSatWrapper->getVisibilityPredict();

		// Compute orbit points to draw orbit line.
		if (orbitVisible) computeOrbitPoints();
	}
}

double Satellite::getDoppler(double freq) const
{
	double result;
	double f = freq * 1000000;
        result   = -f*((rangeRate*1000.0)/SPEED_OF_LIGHT);
	return result/1000000;
}

void Satellite::recalculateOrbitLines(void)
{
	orbitPoints.clear();
}

void Satellite::draw(const StelCore* core, StelPainter& painter, float)
{
		XYZ = core->altAzToJ2000(elAzPosition);
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

		if (orbitVisible && Satellite::orbitLinesFlag) drawOrbit(painter);
	}
}


void Satellite::drawOrbit(StelPainter& painter)
{
        Vec3d position,previousPosition;

	glDisable(GL_TEXTURE_2D);

	QList<Vec3d>::iterator it= orbitPoints.begin();

	//First point projection calculation
        previousPosition.set(it->operator [](0), it->operator [](1), it->operator [](2));

	it++;
	StelVertexArray vertexArray;
	vertexArray.primitiveType=StelVertexArray::Lines;

	//Rest of points
	for (int i=1; i<orbitPoints.size(); i++)
	{
                position.set(it->operator [](0), it->operator [](1), it->operator [](2));
		it++;
                position.normalize();
                previousPosition.normalize();

		// Draw end (fading) parts of orbit lines one segment at a time.
		if (i<=orbitLineFadeSegments || orbitLineSegments-i < orbitLineFadeSegments)
		{
			painter.setColor((*orbitColor)[0], (*orbitColor)[1], (*orbitColor)[2], hintBrightness * calculateOrbitSegmentIntensity(i));
                        painter.drawGreatCircleArc(previousPosition, position, &viewportHalfspace);
		}
		else
		{
                        vertexArray.vertex << previousPosition << position;
		}
                previousPosition = position;
	}

	// Draw center section of orbit in one go
	painter.setColor((*orbitColor)[0], (*orbitColor)[1], (*orbitColor)[2], hintBrightness);
	painter.drawGreatCircleArcs(vertexArray, &viewportHalfspace);

	glEnable(GL_TEXTURE_2D);
}



float Satellite::calculateOrbitSegmentIntensity(int segNum)
{
	int endDist = (orbitLineSegments/2) - abs(segNum-1 - (orbitLineSegments/2) % orbitLineSegments);
	if (endDist > orbitLineFadeSegments)
	{
		return 1.0;
	}
	else
	{
		return (endDist  + 1) / (orbitLineFadeSegments + 1.0);
	}
}

void Satellite::setNightColors(bool night)
{
	if (night)
		orbitColor = &orbitColorNight;
	else
		orbitColor = &orbitColorNormal;
}


void Satellite::computeOrbitPoints()
{
	gTimeSpan computeInterval(0, 0, 0, orbitLineSegmentDuration);
	gTimeSpan orbitSpan(0, 0, 0, orbitLineSegments*orbitLineSegmentDuration/2);
	gTime	  epochTm;
        gTime	  epoch(epochTime);
	gTime	  lastEpochComp(lastEpochCompForOrbit);
        Vec3d     elAzVector;
        int	  diffSlots;


	if (orbitPoints.isEmpty())//Setup orbitPoins
	{
                epochTm  = epoch - orbitSpan;

		for (int i=0; i<=orbitLineSegments; i++)
		{
			pSatWrapper->setEpoch(epochTm.getGmtTm());
                        elAzVector  = pSatWrapper->getAltAz();
                        orbitPoints.append(elAzVector);
			epochTm    += computeInterval;
		}
		lastEpochCompForOrbit = epochTime;
	}
	else if (epochTime > lastEpochCompForOrbit)
	{ // compute next orbit point when clock runs forward

                gTimeSpan diffTime = epoch - lastEpochComp;
		diffSlots          = (int)(diffTime.getDblSeconds()/orbitLineSegmentDuration);

		if (diffSlots > 0)
		{
			if (diffSlots > orbitLineSegments)
			{
				diffSlots = orbitLineSegments + 1;
                                epochTm  = epoch - orbitSpan;
			}
			else
			{
				epochTm   = lastEpochComp + orbitSpan + computeInterval;
			}

			for (int i=0; i<diffSlots; i++)
			{  //remove points at beginning of list and add points at end.
				orbitPoints.removeFirst();
				pSatWrapper->setEpoch(epochTm.getGmtTm());
                                elAzVector  = pSatWrapper->getAltAz();
                                orbitPoints.append(elAzVector);
				epochTm    += computeInterval;
			}

			lastEpochCompForOrbit = epochTime;
		}
	}
	else if (epochTime < lastEpochCompForOrbit)
	{ // compute next orbit point when clock runs backward
                gTimeSpan diffTime = lastEpochComp - epoch;
		diffSlots          = (int)(diffTime.getDblSeconds()/orbitLineSegmentDuration);

		if (diffSlots > 0)
		{
			if (diffSlots > orbitLineSegments)
			{
				diffSlots = orbitLineSegments + 1;
                                epochTm   = epoch + orbitSpan;
			}
			else
			{
                                epochTm   = epoch - orbitSpan - computeInterval;
			}
			for (int i=0; i<diffSlots; i++)
			{ //remove points at end of list and add points at beginning.
				orbitPoints.removeLast();
				pSatWrapper->setEpoch(epochTm.getGmtTm());
                                elAzVector  = pSatWrapper->getAltAz();
                                orbitPoints.push_front(elAzVector);
				epochTm -= computeInterval;

			}
			lastEpochCompForOrbit = epochTime;
		}
	}
}

