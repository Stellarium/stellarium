/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#include <QTextStream>
#include <QFile>
#include <QString>

#include "Nebula.hpp"
#include "NebulaMgr.hpp"
#include "STexture.hpp"
#include "SFont.hpp"
#include "Navigator.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "SFont.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"

#include <QDebug>

STextureSP Nebula::texCircle;
SFont* Nebula::nebulaFont = NULL;
float Nebula::circleScale = 1.f;
float Nebula::hintsBrightness = 0;
Vec3f Nebula::labelColor = Vec3f(0.4,0.3,0.5);
Vec3f Nebula::circleColor = Vec3f(0.8,0.8,0.1);

Nebula::Nebula() :
		M_nb(0),
		NGC_nb(0),
		IC_nb(0)
{
	nameI18 = "";
	angularSize = -1;
}

Nebula::~Nebula()
{
}

QString Nebula::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (!(flags&PlainText))
		oss << QString("<font color=%1>").arg(StelUtils::vec3fToHtmlColor(getInfoColor()));

	if (flags&Name || flags&CatalogNumber)
		oss << "<h2>";

	if (nameI18!="" && flags&Name)
	{
		oss << getNameI18n() << " (";
	}

	if (flags&CatalogNumber)
	{
		if (nameI18!="" && flags&Name)
			oss << " (";

		QStringList catIds;
		if ((M_nb > 0) && (M_nb < 111))
			catIds << QString("M %1").arg(M_nb);
		if (NGC_nb > 0)
			catIds << QString("NGC %1").arg(NGC_nb);
		if (IC_nb > 0)
			catIds << QString("IC %1").arg(IC_nb);
		oss << catIds.join(" - ");

		if (nameI18!="" && flags&Name)
			oss << ")";
	}

	if (flags&Name || flags&CatalogNumber)
		oss << "</h2>";
	
	if (flags&Extra1)
		oss << q_("Type: <b>%1</b>").arg(getTypeString()) << "<br>";

	if (mag < 50 && flags&Magnitude) 
		oss << q_("Magnitude: <b>%1</b>").arg(mag, 0, 'f', 2) << "<br>";

	oss << getPositionInfoString(core, flags);

	if (angularSize>0 && flags&Size)
		oss << q_("Size: %1").arg(StelUtils::radToDmsStr(angularSize*M_PI/180.)) << "<br>";

	postProcessInfoString(str, flags);

	return str;
}

float Nebula::getSelectPriority(const Navigator *nav) const
{
	if( ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getFlagHints() )
	{
		// make very easy to select IF LABELED
		return -10;
	}
	else
	{
		if (getMagnitude(nav)>20) return 20;
		return getMagnitude(nav);
	}
}

Vec3f Nebula::getInfoColor(void) const
{
	return ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getLabelsColor();
}

double Nebula::getCloseViewFov(const Navigator*) const
{
	return angularSize * 4;
}
						   
void Nebula::drawHints(const StelCore* core, float maxMagHints)
{
	if (mag>maxMagHints)
		return;
	//if (4.f/getOnScreenSize(core)<0.1) return;
	glBlendFunc(GL_ONE, GL_ONE);
	float lum = 1.;//qMin(1,4.f/getOnScreenSize(core))*0.8;
	glColor3f(circleColor[0]*lum*hintsBrightness, circleColor[1]*lum*hintsBrightness, circleColor[2]*lum*hintsBrightness);
	Nebula::texCircle->bind();
	core->getProjection()->drawSprite2dMode(XY[0], XY[1], 8);
}

void Nebula::drawLabel(const StelCore* core, float maxMagLabel)
{
	if (mag>maxMagLabel)
		return;
	glColor4f(labelColor[0], labelColor[1], labelColor[2], hintsBrightness);
	float size = getOnScreenSize(core);
	float shift = 4.f + size/1.8f;
	QString str;
	if (nameI18!="")
		str = getNameI18n();
	else
	{
		if (M_nb > 0)
			str = QString("M %1").arg(M_nb);
		else if (NGC_nb > 0)
			str = QString("NGC %1").arg(NGC_nb);
		else if (IC_nb > 0)
			str = QString("IC %1").arg(IC_nb);
	}
	
	core->getProjection()->drawText(nebulaFont,XY[0]+shift, XY[1]+shift, str, 0, 0, 0, false);
}

bool Nebula::readNGC(char *recordstr)
{
	int rahr;
	float ramin;
	int dedeg;
	float demin;
	int nb;

	sscanf(&recordstr[1],"%d",&nb);

	if (recordstr[0] == 'I')
	{
		IC_nb = nb;
	}
	else
	{
		NGC_nb = nb;
	}

	sscanf(&recordstr[12],"%d %f",&rahr, &ramin);
	sscanf(&recordstr[22],"%d %f",&dedeg, &demin);
	float RaRad = (double)rahr+ramin/60;
	float DecRad = (float)dedeg+demin/60;
	if (recordstr[21] == '-') DecRad *= -1.;

	RaRad*=M_PI/12.;     // Convert from hours to rad
	DecRad*=M_PI/180.;    // Convert from deg to rad

	// Calc the Cartesian coord with RA and DE
	StelUtils::spheToRect(RaRad,DecRad,XYZ);

	sscanf(&recordstr[47],"%f",&mag);
	if (mag < 1) mag = 99;

	// Calc the angular size in radian : TODO this should be independant of tex_angular_size
	float size;
	sscanf(&recordstr[40],"%f",&size);

	angularSize = size/60;
	if (angularSize<0)
		angularSize=0;
	
	if (size < 0)
		size = 1;

	// this is a huge performance drag if called every frame, so cache here
	if (!strncmp(&recordstr[8],"Gx",2)) { nType = NebGx;}
	else if (!strncmp(&recordstr[8],"OC",2)) { nType = NebOc;}
	else if (!strncmp(&recordstr[8],"Gb",2)) { nType = NebGc;}
	else if (!strncmp(&recordstr[8],"Nb",2)) { nType = NebN;}
	else if (!strncmp(&recordstr[8],"Pl",2)) { nType = NebPn;}
	else if (!strncmp(&recordstr[8],"  ",2)) { return false;}
	else if (!strncmp(&recordstr[8]," -",2)) { return false;}
	else if (!strncmp(&recordstr[8]," *",2)) { return false;}
	else if (!strncmp(&recordstr[8],"D*",2)) { return false;}
	else if (!strncmp(&recordstr[7],"***",3)) { return false;}
	else if (!strncmp(&recordstr[7],"C+N",3)) { nType = NebCn;}
	else if (!strncmp(&recordstr[8]," ?",2)) { nType = NebUnknown;}
	else { nType = NebUnknown;}

	return true;
}

QString Nebula::getTypeString(void) const
{
	QString wsType;

	switch(nType)
	{
		case NebGx:
			wsType = q_("Galaxy");
			break;
		case NebOc:
			wsType = q_("Open cluster");
			break;
		case NebGc:
			wsType = q_("Globular cluster");
			break;
		case NebN:
			wsType = q_("Nebula");
			break;
		case NebPn:
			wsType = q_("Planetary nebula");
			break;
		case NebCn:
			wsType = q_("Cluster associated with nebulosity");
			break;
		case NebUnknown:
			wsType = q_("Unknown");
			break;
		default:
			wsType = q_("Undocumented type");
			break;
	}
	return wsType;
}

