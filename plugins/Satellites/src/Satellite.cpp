/*
 * Stellarium
 * Copyright (C) 2009, 2012 Matthew Gates
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

#include "Satellite.hpp"
#include "StelObject.hpp"
#include "StelApp.hpp"
#include "StelLocation.hpp"
#include "StelCore.hpp"
#include "VecMath.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "renderer/StelCircleArcRenderer.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"


#include <QTextStream>
#include <QRegExp>
#include <QDebug>
#include <QVariant>
#include <QSettings>
#include <QByteArray>

#include "gsatellite/gTime.hpp"

#include <cmath>

// static data members - will be initialised in the Satallites class (the StelObjectMgr)
float Satellite::showLabels = true;
float Satellite::hintBrightness = 0.0;
float Satellite::hintScale = 1.f;
SphericalCap Satellite::viewportHalfspace = SphericalCap();
int Satellite::orbitLineSegments = 90;
int Satellite::orbitLineFadeSegments = 4;
int Satellite::orbitLineSegmentDuration = 20;
bool Satellite::orbitLinesFlag = true;


Satellite::Satellite(const QString& identifier, const QVariantMap& map)
		: initialized(false), visible(true), newlyAdded(false), orbitValid(false), hintColor(0.0,0.0,0.0), lastUpdated(), pSatWrapper(NULL)
{
	// return initialized if the mandatory fields are not present
	if (identifier.isEmpty())
		return;
	if (!map.contains("name") || !map.contains("tle1") || !map.contains("tle2"))
		return;

	font.setPixelSize(16);

	id = identifier;
	name  = map.value("name").toString();
	if (name.isEmpty())
		return;
	
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

	QString line1 = map.value("tle1").toString();
	QString line2 = map.value("tle2").toString();
	setNewTleElements(line1, line2);
	internationalDesignator = extractInternationalDesignator(line1);
	StelUtils::getJDFromDate(&jdLaunchYearJan1, extractLaunchYear(line1), 1, 1, 0, 0, 0);

	if (map.contains("lastUpdated"))
	{
		lastUpdated = QDateTime::fromString(map.value("lastUpdated").toString(),
		                                    Qt::ISODate);
	}
	orbitValid = true;
	initialized = true;

	update(0.);
}

Satellite::~Satellite()
{
	if (pSatWrapper != NULL)
	{

		delete pSatWrapper;
		pSatWrapper = NULL;
	}
}

double Satellite::roundToDp(float n, int dp)
{
	// round n to dp decimal places
	return floor(n * pow(10., dp) + .5) / pow(10., dp);
}

QVariantMap Satellite::getMap(void)
{
	QVariantMap map;
	map["name"] = name;
	map["tle1"] = tleElements.first.data();
	map["tle2"] = tleElements.second.data();

	if (!description.isEmpty() && description!="")
		map["description"] = description;

	map["visible"] = visible;
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
		// A raw QDateTime is not a recognised JSON data type. --BM
		map["lastUpdated"] = lastUpdated.toString(Qt::ISODate);
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
	
	if (flags & Name)
	{
		oss << "<h2>" << name << "</h2>";
		if (!description.isEmpty())
			oss << description << "<br/>";
	}
	
	if (flags & CatalogNumber)
	{
		QString catalogNumbers;
		if (internationalDesignator.isEmpty())
			catalogNumbers = QString("%1: %2")
					 .arg(q_("Catalog #"))
					 .arg(id);
		else
			catalogNumbers = QString("%1: %2; %3: %4")
					 .arg(q_("Catalog #"))
			                 .arg(id)
					 .arg(q_("International Designator"))
			                 .arg(internationalDesignator);
		oss << catalogNumbers << "<br/><br/>";
	}

	if (flags & Extra1)
	{
		oss << q_("Type: <b>%1</b>").arg(q_("artificial satellite")) << "<br/>";
	}
	
	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);
	
	if (flags & Extra1)
	{
		oss << "<br/>";
		// TRANSLATORS: Slant range: distance between the satellite and the observer
		oss << QString(q_("Range (km): %1")).arg(range, 5, 'f', 2);
		oss << "<br/>";
		// TRANSLATORS: Rate at which the distance changes
		oss << QString(q_("Range rate (km/s): %1")).arg(rangeRate, 5, 'f', 3);
		oss << "<br/>";
		// TRANSLATORS: Satellite altitude
		oss << QString(q_("Altitude (km): %1")).arg(height, 5, 'f', 2);
		oss << "<br/>";
		// TRANSLATORS: %1 and %3 are numbers, %2 and %4 - degree signs.
		oss << QString(q_("SubPoint (Lat./Long.): %1%2/%3%4"))
		       .arg(latLongSubPointPosition[0], 5, 'f', 2)
		       .arg(QChar(0x00B0))
		       .arg(latLongSubPointPosition[1], 5, 'f', 3)
		       .arg(QChar(0x00B0));
		oss << "<br/><br/>";
		
		//TODO: This one can be done better
		const char* xyz = "<b>X:</b> %1, <b>Y:</b> %2, <b>Z:</b> %3";
		QString temeCoords = QString(xyz)
		        .arg(position[0], 5, 'f', 2)
		        .arg(position[1], 5, 'f', 2)
		        .arg(position[2], 5, 'f', 2);
		// TRANSLATORS: TEME is an Earth-centered inertial coordinate system
		oss << QString(q_("TEME coordinates (km): %1")).arg(temeCoords);
		oss << "<br/>";
		
		QString temeVel = QString(xyz)
		        .arg(velocity[0], 5, 'f', 2)
		        .arg(velocity[1], 5, 'f', 2)
		        .arg(velocity[2], 5, 'f', 2);
		// TRANSLATORS: TEME is an Earth-centered inertial coordinate system
		oss << QString(q_("TEME velocity (km/s): %1")).arg(temeVel);
		oss << "<br/>";
		
		//Visibility: Full text
		//TODO: Move to a more prominent place.
		switch (visibility)
		{
		case RADAR_SUN:
			oss << q_("The satellite and the observer are in sunlight.") << "<br/>";
			break;
		case VISIBLE:
			oss << q_("The satellite is visible.") << "<br/>";
			break;
		case RADAR_NIGHT:
			oss << q_("The satellite is eclipsed.") << "<br/>";
			break;
		case NOT_VISIBLE:
			oss << q_("The satellite is not visible") << "<br/>";
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

			oss << "<br/>";
			if (!c.modulation.isEmpty() && c.modulation != "") oss << "  " << c.modulation;
			if (!c.description.isEmpty() && c.description != "") oss << "  " << c.description;
			if ((!c.modulation.isEmpty() && c.modulation != "") || (!c.description.isEmpty() && c.description != "")) oss << "<br/>";
			oss << QString(q_("%1 MHz (%2%3 kHz)"))
			       .arg(c.frequency, 8, 'f', 5)
			       .arg(sign)
			       .arg(ddop, 6, 'f', 3);
			oss << "<br/>";
		}
	}

	postProcessInfoString(str, flags);
	return str;
}

Vec3d Satellite::getJ2000EquatorialPos(const StelCore* core) const
{
	return core->altAzToJ2000(elAzPosition);;
}

Vec3f Satellite::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : hintColor;
}

float Satellite::getVMagnitude(const StelCore* core, bool withExtinction) const
{
    float extinctionMag=0.0; // track magnitude loss
    if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
    {
	Vec3d altAz=getAltAzPosApparent(core);
	altAz.normalize();
	core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
    }

    return 5.0 + extinctionMag;
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

	pSatWrapper = new gSatWrapper(id, tle1, tle2);
	orbitPoints.clear();
}

void Satellite::update(double)
{
	if (pSatWrapper && orbitValid)
	{
		epochTime = StelApp::getInstance().getCore()->getJDay();

		pSatWrapper->setEpoch(epochTime);
		position                 = pSatWrapper->getTEMEPos();
		velocity                 = pSatWrapper->getTEMEVel();
		latLongSubPointPosition  = pSatWrapper->getSubPoint();
		height                   = latLongSubPointPosition[2];
		if (height <= 0.0)
		{
			// The orbit is no longer valid.  Causes include very out of date
			// TLE, system date and time out of a reasonable range, and orbital
			// degradation and re-entry of a satellite.  In any of these cases
			// we might end up with a problem - usually a crash of Stellarium
			// because of a div/0 or something.  To prevent this, we turn off
			// the satellite.
			qWarning() << "Satellite has invalid orbit:" << name << id;
			orbitValid = false;
			return;
		}

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
	result = -f*((rangeRate*1000.0)/SPEED_OF_LIGHT);
	return result/1000000;
}

void Satellite::recalculateOrbitLines(void)
{
	orbitPoints.clear();
}

QString Satellite::extractInternationalDesignator(const QString& tle1)
{
	QString result;
	if (tle1.isEmpty())
		return result;
	
	// The designator is encoded as the 3rd group on the first line
	QString rawString = tle1.split(' ').at(2);
	if (rawString.isEmpty())
		return result;
	
	//TODO: Use a regular expression?
	bool ok;
	int year = rawString.left(2).toInt(&ok);
	if (!ok)
		return result;
	
	// Y2K bug :) I wonder what NORAD will do in 2057. :)
	if (year < 57)
		year += 2000;
	else
		year += 1900;
	
	result = QString::number(year) + "-" + rawString.right(4);
	return result;
}

int Satellite::extractLaunchYear(const QString& tle1)
{
	if (tle1.isEmpty())
		return 1957;

	// The designator is encoded as the 3rd group on the first line
	QString rawString = tle1.split(' ').at(2);
	if (rawString.isEmpty())
		return 1957;

	//TODO: Use a regular expression?
	bool ok;
	int year = rawString.left(2).toInt(&ok);
	if (!ok)
		return 1957;

	// Y2K bug :) I wonder what NORAD will do in 2057. :)
	if (year < 57)
		return year + 2000;
	else
		return year + 1900;
}

void Satellite::draw(const StelCore* core, StelRenderer* renderer, 
                     StelProjectorP projector, StelTextureNew* hintTexture)
{
	if (core->getJDay() < jdLaunchYearJan1) return;

	XYZ = getJ2000EquatorialPos(core);
	Vec3f drawColor;
	(visibility==RADAR_NIGHT) ? drawColor = Vec3f(0.2f,0.2f,0.2f) : drawColor = hintColor;
	StelApp::getInstance().getVisionModeNight() 
		? renderer->setGlobalColor(0.6,0.0,0.0,1.0) 
		: renderer->setGlobalColor(drawColor[0],drawColor[1],drawColor[2], Satellite::hintBrightness);

	Vec3d xy;
	if (core->getProjection(StelCore::FrameJ2000)->project(XYZ,xy))
	{
		if (Satellite::showLabels)
		{
			renderer->drawText(TextParams(xy[0], xy[1], name).shift(10, 10).useGravity());
		}
		renderer->drawTexturedRect(xy[0] - 11, xy[1] - 11, 22, 22);

		if (orbitVisible && Satellite::orbitLinesFlag) {drawOrbit(renderer, projector);}
	}
}


void Satellite::drawOrbit(StelRenderer* renderer, StelProjectorP projector)
{
	Vec3d position,previousPosition;

	QList<Vec3d>::iterator it= orbitPoints.begin();

	//First point projection calculation
	previousPosition.set(it->operator [](0), it->operator [](1), it->operator [](2));

	it++;

	QVector<Vec3d> orbitArcPoints;
	StelCircleArcRenderer circleArcRenderer(renderer, projector);
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
			renderer->setGlobalColor((*orbitColor)[0], (*orbitColor)[1], (*orbitColor)[2],
			                         hintBrightness * calculateOrbitSegmentIntensity(i));
			circleArcRenderer.drawGreatCircleArc(previousPosition, position, &viewportHalfspace);
		}
		else
		{
			orbitArcPoints << previousPosition << position;
		}
		previousPosition = position;
	}

	// Draw center section of orbit in one go
	renderer->setGlobalColor((*orbitColor)[0], (*orbitColor)[1], (*orbitColor)[2],
	                         hintBrightness);
	circleArcRenderer.drawGreatCircleArcs(orbitArcPoints, PrimitiveType_Lines, &viewportHalfspace);
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
	gTime epochTm;
	gTime epoch(epochTime);
	gTime lastEpochComp(lastEpochCompForOrbit);
	Vec3d elAzVector;
	int diffSlots;


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
			{
				//remove points at beginning of list and add points at end.
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

