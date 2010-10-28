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


#include "gsatellite/gSatTEME.hpp"
#include "gsatellite/gObserver.hpp"
#include "gsatellite/gTime.hpp"
#include "gsatellite/gVector.hpp"

#include <QTextStream>
#include <QRegExp>
#include <QDebug>
#include <QVariant>
#include <QtOpenGL>
#include <QSettings>

#include <cmath>
#include <cstdlib>

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
	
	font.setPixelSize(16);

	designation  = map.value("designation").toString();
	strncpy(elements[0], "DUMMY", 5);
	strncpy(elements[1], qPrintable(map.value("tle1").toString()), 80);
	strncpy(elements[2], qPrintable(map.value("tle2").toString()), 80);
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

	pSatellite = new gSatTEME( designation.toAscii().data(), elements[1], elements[2]);

	setObserverLocation();
	initialized = true;
}

Satellite::~Satellite()
{
	if(pSatellite != NULL)
		delete pSatellite;
}

QVariantMap Satellite::getMap(void)
{
	QVariantMap map;
	map["designation"] = designation;
	map["visible"] = visible;
	map["orbitVisible"] = orbitVisible;
	map["tle1"] = QString(elements[1]);
	map["tle2"] = QString(elements[2]);
	QVariantList col, orbitCol;;
	col << (double)hintColor[0] << (double)hintColor[1] << (double)hintColor[2];
	orbitCol << (double)orbitColor[0] << (double)orbitColor[1] << (double)orbitColor[2];
	map["hintColor"] = col;
	map["orbitColor"] = orbitCol;
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
		QVariantList groupList;
	foreach(QString g, groupIDs)
	{
		groupList << g;
	}
	map["groups"] = groupList;
	return map;
}

float Satellite::getSelectPriority(const StelNavigator*) const
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
		oss << QString("SubPoint Lat/Long(Deg): <b>%1</b>").arg(LatLong[0], 5, 'f', 2) << "/";
		oss << QString("<b>%1</b>").arg(LatLong[1], 5, 'f', 3);
		oss << "</p>";

		oss << "TEME Coordinates(km):  ";
		oss << QString("<b>X:</b> %1 ").arg(Position[0], 5, 'f', 2);
		oss << QString("<b>Y:</b> %1 ").arg(Position[1], 5, 'f', 2);
		oss << QString("<b>Z:</b> %1 ").arg(Position[2], 5, 'f', 2) << "<br>";
		oss << "TEME Vel(km/s):  ";
		oss << QString("<b>X:</b> %1 ").arg(Vel[0], 5, 'f', 2);
		oss << QString("<b>Y:</b> %1 ").arg(Vel[1], 5, 'f', 2);
		oss << QString("<b>Z:</b> %1 ").arg(Vel[2], 5, 'f', 2) << "<br>";

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


	observer.setPosition( l.latitude, l.longitude, l.altitude / 1000.0);

}

Vec3f Satellite::getInfoColor(void) const {
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : hintColor;
}

float Satellite::getVMagnitude(const StelNavigator*) const
{
	return 5.0;
}

double Satellite::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void Satellite::update(double)
{
	double jul_utc = StelApp::getInstance().getCore()->getNavigator()->getJDay();

	epochTime = jul_utc;

	pSatellite->setEpoch( epochTime);
	Position = pSatellite->getPos();
	Vel      = pSatellite->getVel();
	LatLong  = pSatellite->getSubPoint( epochTime);
	azElPos  = observer.calculateLook( *pSatellite, epochTime);


	azimuth   = azElPos[ AZIMUTH]/KDEG2RAD;
	elevation = azElPos[ ELEVATION]/KDEG2RAD;
	range     = azElPos[ RANGE];
	rangeRate = azElPos[ RANGERATE];
	height    = LatLong[2];

	// Compute orbit points to draw orbit line.
	if(orbitVisible) computeOrbitPoints();

}

double Satellite::getDoppler(double freq) const
{
	double result;
	double f = freq * 1000000;
	result = -f*((rangeRate*1000.0)/SPEED_OF_LIGHT);
	return result/1000000;
}

void Satellite::draw(const StelCore* core, StelPainter& painter, float)
{
	float a = (azimuth-90)*M_PI/180;
	Vec3d pos(sin(a),cos(a), tan(elevation * M_PI / 180.));
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

		if(orbitVisible) drawOrbit(core, prj, painter);
	}
}

void Satellite::drawOrbit(const StelCore* core, StelProjectorP& prj, StelPainter& painter){
	Vec3d XYZPos, xy1;
	float a, azimth, elev;
	QVarLengthArray<float, 1024> vertexArray;


	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	painter.enableClientStates(true, false, false);

	QList<gVector>::iterator it= orbitPoints.begin();

	for(int i=0; i<orbitPoints.size();i++)
	{
		azimth = it->at( AZIMUTH);
		elev   = it->at( ELEVATION);
		a      = ( (azimth/KDEG2RAD)-90)*M_PI/180;
		Vec3d pos(sin(a),cos(a), tan( (elev/KDEG2RAD) * M_PI / 180.));
		XYZPos = core->getNavigator()->j2000ToEquinoxEqu(core->getNavigator()->altAzToEquinoxEqu(pos));

		if (prj->project(XYZPos,xy1))
		{
			vertexArray.append(xy1[0]);
			vertexArray.append(xy1[1]);
		}
		it++;
		if (i>0)
		{
			painter.setColor(hintColor[0], hintColor[1], hintColor[2], hintBrightness * calculateOrbitSegmentIntensity(i));
			painter.setVertexPointer(2, GL_FLOAT, vertexArray.constData());
			painter.drawFromArray(StelPainter::LineStrip, 2, i-1, false);
		}
	}

	vertexArray.clear();
	painter.enableClientStates(false);
	glEnable(GL_TEXTURE_2D);
}

float Satellite::calculateOrbitSegmentIntensity(int segNum)
{
	int endDist = (DRAWORBIT_SLOTS_NUMBER/2) - abs(segNum - (DRAWORBIT_SLOTS_NUMBER/2) % DRAWORBIT_SLOTS_NUMBER);
	if (endDist > DRAWORBIT_FADE_NUMBER) { return 1.0; }
	else { return (endDist  + 1) / (DRAWORBIT_FADE_NUMBER + 1.0); }
}

void Satellite::computeOrbitPoints(){

	gTimeSpan computeInterval(0,0,0,DRAWORBIT_SLOT_SECNUMBER);  //1/2 minute
	gTimeSpan orbitSpan(0,0,0, DRAWORBIT_SLOTS_NUMBER*DRAWORBIT_SLOT_SECNUMBER/2);       //+- 15 minutes range.
	gTime	  epochTm;
	gVector   azElVector;
	int	  diffSlots;


	if( orbitPoints.isEmpty())//Setup orbitPoins
	{
		epochTm  = epochTime - orbitSpan;

		for(int i=0; i< DRAWORBIT_SLOTS_NUMBER; i++)
		{
			pSatellite->setEpoch( epochTm);
			azElVector  = observer.calculateLook( *pSatellite, epochTm);
			orbitPoints.append(azElVector);
			epochTm += computeInterval;
		}
		lastEpochCompForOrbit = epochTime;
	}
	else if( epochTime > lastEpochCompForOrbit)
	{ // compute next orbit point when clock runs forward

		gTimeSpan diffTime = epochTime - lastEpochCompForOrbit;
		diffSlots          = (int)(diffTime.getDblSeconds()/DRAWORBIT_SLOT_SECNUMBER);

		if(diffSlots > 0)
		{
			if( diffSlots > DRAWORBIT_SLOTS_NUMBER)
			{
				diffSlots = DRAWORBIT_SLOTS_NUMBER;
				epochTm   = epochTime - orbitSpan;
			}
			else
			{
				epochTm   = lastEpochCompForOrbit + orbitSpan + computeInterval;
			}

			for( int i=0; i<diffSlots;i++)
			{  //remove points at beginning of list and add points at end.
				orbitPoints.removeFirst();
				pSatellite->setEpoch( epochTm);
				azElVector  = observer.calculateLook( *pSatellite, epochTm);
				orbitPoints.append(azElVector);
				epochTm += computeInterval;
			}

			lastEpochCompForOrbit = epochTime;
		}
	}
	else if(epochTime < lastEpochCompForOrbit)
	{ // compute next orbit point when clock runs backward
		gTimeSpan diffTime =  lastEpochCompForOrbit - epochTime;
		diffSlots          = (int)(diffTime.getDblSeconds()/DRAWORBIT_SLOT_SECNUMBER);

		if(diffSlots > 0)
		{
			if( diffSlots > DRAWORBIT_SLOTS_NUMBER)
			{
				diffSlots = DRAWORBIT_SLOTS_NUMBER;
				epochTm   = epochTime + orbitSpan;
			}
			else
			{
				epochTm   = epochTime - orbitSpan - computeInterval;
			}
			for( int i=0; i<diffSlots;i++)
			{ //remove points at end of list and add points at beginning.
				orbitPoints.removeLast();
				pSatellite->setEpoch( epochTm);
				azElVector  = observer.calculateLook( *pSatellite, epochTm);
				orbitPoints.push_front(azElVector);
				epochTm -= computeInterval;

			}
			lastEpochCompForOrbit = epochTime;
		}
	}
}

