/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#include "Nebula.hpp"
#include "NebulaMgr.hpp"

#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QString>
#include <QTextStream>

float Nebula::circleScale = 1.f;
float Nebula::hintsBrightness = 0;
Vec3f Nebula::labelColor = Vec3f(0.4,0.3,0.5);
Vec3f Nebula::circleColor = Vec3f(0.8,0.8,0.1);

Nebula::NebulaHintTextures::~NebulaHintTextures()
{
	if(!initialized){return;}

	delete texCircle; 
	delete texOpenCluster;    
	delete texGlobularCluster;
	delete texPlanetaryNebula;   
	
	initialized = false;
}

void Nebula::NebulaHintTextures::lazyInit(StelRenderer* renderer)
{
	if(initialized){return;}

	texCircle          = renderer->createTexture("textures/neb.png");   // Load circle texture
	texOpenCluster     = renderer->createTexture("textures/ocl.png");   // Load open cluster marker texture
	texGlobularCluster = renderer->createTexture("textures/gcl.png");   // Load globular cluster marker texture
	texPlanetaryNebula = renderer->createTexture("textures/pnb.png");   // Load planetary nebula marker texture

	initialized = true;
}


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

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "<h2>";

	if (nameI18!="" && flags&Name)
	{
		oss << getNameI18n();
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

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "</h2>";

	if (flags&Extra1)
		oss << q_("Type: <b>%1</b>").arg(getTypeString()) << "<br>";

	if (mag < 50 && flags&Magnitude)
	{
	    if (core->getSkyDrawer()->getFlagHasAtmosphere())
		oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(getVMagnitude(core, false), 'f', 2),
										QString::number(getVMagnitude(core, true), 'f', 2)) << "<br>";
	    else
		oss << q_("Magnitude: <b>%1</b>").arg(getVMagnitude(core, false), 0, 'f', 2) << "<br>";
	}
	oss << getPositionInfoString(core, flags);

	if (angularSize>0 && flags&Size)
		oss << q_("Size: %1").arg(StelUtils::radToDmsStr(angularSize*M_PI/180.)) << "<br>";

	postProcessInfoString(str, flags);

	return str;
}

float Nebula::getVMagnitude(const StelCore* core, bool withExtinction) const
{
    float extinctionMag=0.0; // track magnitude loss
    if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
    {
	Vec3d altAz=getAltAzPosApparent(core);
	altAz.normalize();
	core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
    }

    return mag+extinctionMag;
}


float Nebula::getSelectPriority(const StelCore* core) const
{
	if( ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getFlagHints() )
	{
		// make very easy to select IF LABELED
		return -10.f;
	}
	else
	{
		if (getVMagnitude(core, false)>20.f) return 20.f;
		return getVMagnitude(core, false);
	}
}

Vec3f Nebula::getInfoColor(void) const
{
	Vec3f col = ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getLabelsColor();
	return StelApp::getInstance().getVisionModeNight() ? StelUtils::getNightColor(col) : col;
}

double Nebula::getCloseViewFov(const StelCore*) const
{
	return angularSize>0 ? angularSize * 4 : 1;
}

void Nebula::drawHints(StelRenderer* renderer, float maxMagHints, NebulaHintTextures& hintTextures)
{
	float lim = mag;
	if (lim > 50) lim = 15.f;
	if (lim>maxMagHints)
		return;
	renderer->setBlendMode(BlendMode_Add);
	float lum = 1.f;//qMin(1,4.f/getOnScreenSize(core))*0.8;
	Vec3f col(circleColor[0]*lum*hintsBrightness, circleColor[1]*lum*hintsBrightness, circleColor[2]*lum*hintsBrightness);
	if (StelApp::getInstance().getVisionModeNight())
		col = StelUtils::getNightColor(col);

	renderer->setGlobalColor(col[0], col[1], col[2], 1);
	if (nType == 1)
	{
		hintTextures.texOpenCluster->bind();
	}
	else if (nType == 2)
	{
		hintTextures.texGlobularCluster->bind();
	}
	else if (nType == 4)
	{
		hintTextures.texPlanetaryNebula->bind();
	}
	else
	{
		hintTextures.texCircle->bind();
	}

	renderer->drawTexturedRect(XY[0] - 6, XY[1] - 6, 12, 12);
}


void Nebula::drawLabel(StelRenderer* renderer, StelProjectorP projector, float maxMagLabel)
{
	float lim = mag;
	if (lim > 50) lim = 15.f;
	if (lim>maxMagLabel)
		return;

	Vec3f col(labelColor[0], labelColor[1], labelColor[2]);
	if (StelApp::getInstance().getVisionModeNight())
		col = StelUtils::getNightColor(col);
	renderer->setGlobalColor(col[0], col[1], col[2], hintsBrightness);

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

	float size = getAngularSize(NULL) * M_PI / 180.0 * projector->getPixelPerRadAtCenter();
	float shift = 4.f + size / 1.8f;

	renderer->drawText(TextParams(XY[0] + shift, XY[1] + shift, str).useGravity().projector(projector));
}


void Nebula::readNGC(QDataStream& in)
{
	bool isIc;
	int nb;
	float ra, dec;
	unsigned int type;
	in >> isIc >> nb >> ra >> dec >> mag >> angularSize >> type;
	if (isIc)
	{
		IC_nb = nb;
	}
	else
	{
		NGC_nb = nb;
	}
	StelUtils::spheToRect(ra,dec,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);
	nType = (Nebula::NebulaType)type;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));
}

#if 0
QFile filess("filess.dat");
QDataStream out;
out.setVersion(QDataStream::Qt_4_5);
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

	if (!filess.isOpen())
	{
		filess.open(QIODevice::WriteOnly);
		out.setDevice(&filess);
	}
	out << ((bool)(recordstr[0] == 'I')) << nb << RaRad << DecRad << mag << angularSize << ((unsigned int)nType);
	if (nb==5369 && recordstr[0] == 'I')
		filess.close();

	return true;
}
#endif

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

