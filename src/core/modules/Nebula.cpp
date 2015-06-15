/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2011 Alexander Wolf
 * Copyright (C) 2015 Georg Zotti
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
#include "StelTexture.hpp"

#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"

#include <QTextStream>
#include <QFile>
#include <QString>

#include <QDebug>
#include <QBuffer>

StelTextureSP Nebula::texCircle;
StelTextureSP Nebula::texGalaxy;
StelTextureSP Nebula::texOpenCluster;
StelTextureSP Nebula::texGlobularCluster;
StelTextureSP Nebula::texPlanetaryNebula;
StelTextureSP Nebula::texDiffuseNebula;
StelTextureSP Nebula::texDarkNebula;
StelTextureSP Nebula::texOpenClusterWithNebulosity;
float Nebula::circleScale = 1.f;
bool  Nebula::drawHintProportional = false;
float Nebula::hintsBrightness = 0;
Vec3f Nebula::labelColor = Vec3f(0.4,0.3,0.5);
Vec3f Nebula::circleColor = Vec3f(0.8,0.8,0.1);
Vec3f Nebula::galaxyColor = Vec3f(1.0,0.2,0.2);
Vec3f Nebula::brightNebulaColor = Vec3f(0.1,1.0,0.1);
Vec3f Nebula::darkNebulaColor = Vec3f(0.3,0.3,0.3);
Vec3f Nebula::clusterColor = Vec3f(1.0,1.0,0.1);

Nebula::Nebula()
	: M_nb(0)
	, NGC_nb(0)
	, IC_nb(0)
	, C_nb(0)
	, B_nb(0)
	, Sh2_nb(0)
	, VdB_nb(0)
	, RCW_nb(0)
	, LDN_nb(0)
	, LBN_nb(0)
	, Cr_nb(0)
	, Mel_nb(0)
	, mag(99.)
	, nType()
	, formType()
	, structureType()
	, brightnessType()
	, rcwBrightnessType()
{
	nameI18 = "";
	angularSize = -1;
	brightnessClass = -1;
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
		if ((C_nb > 0) && (C_nb < 110))
			catIds << QString("C %1").arg(C_nb);
		if (NGC_nb > 0)
			catIds << QString("NGC %1").arg(NGC_nb);
		if (IC_nb > 0)
			catIds << QString("IC %1").arg(IC_nb);		
		if ((B_nb > 0) && (B_nb <= 370))
			catIds << QString("B %1").arg(B_nb);
		if ((Sh2_nb > 0) && (Sh2_nb <= 313))
			catIds << QString("Sh 2-%1").arg(Sh2_nb);
		if ((VdB_nb > 0) && (VdB_nb <= 158))
			catIds << QString("VdB %1").arg(VdB_nb);
		if ((RCW_nb > 0) && (RCW_nb <= 182))
			catIds << QString("RCW %1").arg(RCW_nb);
		if ((LDN_nb > 0) && (LDN_nb <= 1802))
			catIds << QString("LDN %1").arg(LDN_nb);
		if ((LBN_nb > 0) && (LBN_nb <= 1125))
			catIds << QString("LBN %1").arg(LBN_nb);
		if ((Cr_nb > 0) && (Cr_nb <= 471))
			catIds << QString("Cr %1").arg(Cr_nb);
		if ((Mel_nb > 0) && (Mel_nb <= 245))
			catIds << QString("Mel %1").arg(Mel_nb);
		oss << catIds.join(" - ");

		if (nameI18!="" && flags&Name)
			oss << ")";
	}

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "</h2>";

	if (flags&ObjectType)
		oss << q_("Type: <b>%1</b>").arg(getTypeString()) << "<br>";

	if (mag < 50 && flags&Magnitude)
	{
		if (nType == NebDn)
		{
			oss << q_("Opacity: <b>%1</b>").arg(getVMagnitude(core), 0, 'f', 2) << "<br>";
		}
		else
		{
			if (core->getSkyDrawer()->getFlagHasAtmosphere())
				oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(getVMagnitude(core), 'f', 2),
												QString::number(getVMagnitudeWithExtinction(core), 'f', 2)) << "<br>";
			else
				oss << q_("Magnitude: <b>%1</b>").arg(getVMagnitude(core), 0, 'f', 2) << "<br>";
		}
	}
	if (nType != NebDn && mag < 50 && flags&Extra)
	{
		if (core->getSkyDrawer()->getFlagHasAtmosphere())
		{
			if (getSurfaceBrightness(core)<99 && getSurfaceBrightnessWithExtinction(core)<99)
				oss << q_("Surface brightness: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(getSurfaceBrightness(core), 'f', 2),
													 QString::number(getSurfaceBrightnessWithExtinction(core), 'f', 2)) << "<br>";
		}
		else
		{
			if (getSurfaceBrightness(core)<99)
				oss << q_("Surface brightness: <b>%1</b>").arg(QString::number(getSurfaceBrightness(core), 'f', 2)) << "<br>";
		}
	}
	if (flags&Extra)
	{
		if (nType==NebHII)
		{
			if (LBN_nb!=0)
				oss << q_("Brightness: %1").arg(brightnessClass) << "<br>";
			else
			{
				oss << qc_("Form: %1","HII Region").arg(getHIIFormTypeString()) << "<br>";
				oss << qc_("Structure: %1","HII Region").arg(getHIIStructureTypeString()) << "<br>";
				oss << q_("Brightness: %1").arg(getHIIBrightnessTypeString()) << "<br>";
			}
		}
		if (nType==NebHa)
		{
			oss << q_("Brightness: %1").arg(getHaBrightnessTypeString()) << "<br>";
		}
	}
	oss << getPositionInfoString(core, flags);

	if (angularSize>0 && flags&Size)
		oss << q_("Size: %1").arg(StelUtils::radToDmsStr(angularSize*M_PI/180.)) << "<br>";

	postProcessInfoString(str, flags);

	return str;
}

float Nebula::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core);
	return mag;
}


float Nebula::getSelectPriority(const StelCore* core) const
{
	const NebulaMgr* nebMgr = ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"));
	if (!nebMgr->getFlagHints())
		return StelObject::getSelectPriority(core)-2.f;
	
	const float maxMagHint = nebMgr->computeMaxMagHint(core->getSkyDrawer());
	// make very easy to select if labeled
	float lim=getVMagnitude(core);
	if (nType==NebDn)
		lim=15.0f - mag - 2.0f*angularSize; // Note that "mag" field is used for opacity in this catalog!
	else if (nType==NebHII)
		lim=10.0f - 2.0f*angularSize; // Unfortunately, in Sh catalog, we always have mag=99=unknown!

	if (std::min(15.f, lim)<maxMagHint)
		return -10.f;
	else
		return StelObject::getSelectPriority(core)-2.f;

}

Vec3f Nebula::getInfoColor(void) const
{
	return ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getLabelsColor();
}

double Nebula::getCloseViewFov(const StelCore*) const
{
	return angularSize>0 ? angularSize * 4 : 1;
}

float Nebula::getSurfaceBrightness(const StelCore* core) const
{
	if (getVMagnitude(core)<99 && angularSize>0 && nType!=NebDn && nType!=NebHII)
		return getVMagnitude(core) + 2.5*log10(M_PI*pow((angularSize*M_PI/180.)*1800,2));
	else
		return 99;
}

float Nebula::getSurfaceBrightnessWithExtinction(const StelCore* core) const
{
	if (getVMagnitudeWithExtinction(core)<99 && angularSize>0 && nType!=NebDn && nType!=NebHII)
		return getVMagnitudeWithExtinction(core) + 2.5*log10(M_PI*pow((angularSize*M_PI/180.)*1800,2));
	else
		return 99;
}

void Nebula::drawHints(StelPainter& sPainter, float maxMagHints)
{
	float lim = mag;
	if (lim > 50) lim = 15.f;

	// temporary workaround of this bug: https://bugs.launchpad.net/stellarium/+bug/1115035 --AW
	if (getEnglishName().contains("Pleiades"))
		lim = 5.f;
	// Dark nebulae. Not sure how to assess visibility from opacity? --GZ
	if (nType==NebDn)
	{
		// GZ: ad-hoc visibility formula: assuming good visibility if objects of mag9 are visible, "usual" opacity 5 and size 30', better visibility (discernability) comes with higher opacity and larger size,
		// 9-(opac-5)-2*(angularSize-0.5)
		// GZ Not good for non-Barnards. weak opacity and large surface are antagonists. (some LDN are huge, but opacity 2 is not much to discern).
		// The qMin() maximized the visibility gain for large objects.
		if (angularSize>0 && mag<50)
			lim = 15.0f - mag - 2.0f*qMin(angularSize, 1.5f);
		else if (B_nb>0)
			lim = 9.0f;
		else
			lim= 12.0f; // GZ I assume LDN objects are rather elusive.
	}
	else if (nType==NebHII || nType==NebHa)
	{ // artificially increase visibility of (most) Sharpless objects? No magnitude recorded:-(
		lim=9.0f;
	}

	if (lim>maxMagHints)
		return;

	Vec3d win;
	// Check visibility of DSO hints
	if (!(sPainter.getProjector()->projectCheck(XYZ, win)))
		return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	float lum = 1.f;//qMin(1,4.f/getOnScreenSize(core))*0.8;

	Vec3f color=circleColor;
	switch (nType)
	{
		case NebGx:
			Nebula::texGalaxy->bind();
			color=galaxyColor;
			break;
		case NebOc:
			Nebula::texOpenCluster->bind();
			color=clusterColor;
			break;
		case NebGc:
			Nebula::texGlobularCluster->bind();
			color=clusterColor;
			break;
		case NebN:
		case NebHII:
		case NebRn:
		case NebHa:
			Nebula::texDiffuseNebula->bind();
			color=brightNebulaColor;
			break;
		case NebPn:
			Nebula::texPlanetaryNebula->bind();
			color=brightNebulaColor;
			break;
		case NebDn:
			Nebula::texDarkNebula->bind();
			color=darkNebulaColor;
			break;
		case NebCn:
			Nebula::texOpenClusterWithNebulosity->bind();
			color=clusterColor;
			break;
		default:
			Nebula::texCircle->bind();
	}
	Vec3f col(color[0]*lum*hintsBrightness, color[1]*lum*hintsBrightness, color[2]*lum*hintsBrightness);
	sPainter.setColor(col[0], col[1], col[2], 1);

	if (drawHintProportional)
	{
		float size = getAngularSize(NULL)*M_PI/180.*sPainter.getProjector()->getPixelPerRadAtCenter();
		sPainter.drawSprite2dMode(XY[0], XY[1], qMax(6.0f,size));
	}
	else
		sPainter.drawSprite2dMode(XY[0], XY[1], 6.0f);
}

void Nebula::drawLabel(StelPainter& sPainter, float maxMagLabel)
{
	float lim = mag;
	if (lim > 50) lim = 15.f;

	// temporary workaround of this bug: https://bugs.launchpad.net/stellarium/+bug/1115035 --AW
	if (getEnglishName().contains("Pleiades"))
		lim = 5.f;
	// Dark nebulae. Not sure how to assess visibility from opacity? --GZ
	if (nType==NebDn)
	{
		// GZ: ad-hoc visibility formula: assuming good visibility if objects of mag9 are visible, "usual" opacity 5 and size 30', better visibility (discernability) comes with higher opacity and larger size,
		// 9-(opac-5)-2*(angularSize-0.5)
		if (angularSize>0 && mag<50)
			lim = 15.0f - mag - 2.0f*qMin(angularSize, 2.5f);
		else if (B_nb>0)
			lim = 9.0f;
		else
			lim= 12.0f; // GZ I assume some LDN objects are rather elusive.
	}
	else if (nType==NebHII || nType==NebHa)
		lim=9.0f;

	if (lim>maxMagLabel)
		return;

	Vec3d win;
	// Check visibility of DSO labels
	if (!(sPainter.getProjector()->projectCheck(XYZ, win)))
		return;

	Vec3f col(labelColor[0], labelColor[1], labelColor[2]);

	sPainter.setColor(col[0], col[1], col[2], hintsBrightness);
	float size = getAngularSize(NULL)*M_PI/180.*sPainter.getProjector()->getPixelPerRadAtCenter();
	float shift = 4.f + (drawHintProportional ? size : size/1.8f);
	QString str;
	if (!nameI18.isEmpty())
		str = getNameI18n();
	else
	{
		// On screen label: one only, priority as given here. NGC should win over Sharpless. (GZ)
		if (M_nb > 0)
			str = QString("M %1").arg(M_nb);
		else if (C_nb > 0)
			str = QString("C %1").arg(C_nb);
		else if (NGC_nb > 0)
			str = QString("NGC %1").arg(NGC_nb);
		else if (IC_nb > 0)
			str = QString("IC %1").arg(IC_nb);
		else if (B_nb > 0)
			str = QString("B %1").arg(B_nb);		
		else if (Sh2_nb > 0)
			str = QString("Sh 2-%1").arg(Sh2_nb);
		else if (VdB_nb > 0)
			str = QString("VdB %1").arg(VdB_nb);
		else if (RCW_nb > 0)
			str = QString("RCW %1").arg(RCW_nb);
		else if (LDN_nb > 0)
			str = QString("LDN %1").arg(LDN_nb);
		else if (LBN_nb > 0)
			str = QString("LBN %1").arg(LBN_nb);
		else if (Cr_nb > 0)
			str = QString("Cr %1").arg(Cr_nb);
		else if (Mel_nb > 0)
			str = QString("Mel %1").arg(Mel_nb);
	}

	sPainter.drawText(XY[0]+shift, XY[1]+shift, str, 0, 0, 0, false);
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
	//if (type >= 5) {
	//	qDebug()<< (isIc?"IC" : "NGC") << nb << " type " << type ;
	//}
	// This confirms there are currently no dark nebulae in the NGC list.
	Q_ASSERT(type!=5);
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

bool Nebula::readBarnard(QString record)
{
	// Line Format: "<B>\t<RAh>\t<RAm>\t<RAs>\t[+-]DD MM\t<size>\t<obs>\t<comment>... ..."
	int rahr;
	float ramin;
	int dedeg;
	int demin;

	QStringList list=record.split("\t", QString::KeepEmptyParts);

	//qDebug() << "Barnard: " << list.at(0) << "RA " << list.at(1) << list.at(2) << list.at(3) <<
	//	    "Dec" << list.at(4) << "opac" << list.at(6) << "size" << list.at(5);

	B_nb=list.at(0).toInt();
	rahr=list.at(1).toInt();
	ramin=list.at(2).toInt() + list.at(3).toInt() / 60.0f;
	float RaRad = (double)rahr+ramin/60;

	QString degString=list.at(4);

	dedeg=degString.mid(1,2).toInt();
	demin=degString.mid(4,2).toInt();

	float DecRad = (float)dedeg+(float)demin/60.0f;

	if (degString.at(0) == '-') DecRad *= -1.f;

	RaRad*=M_PI/12.f;     // Convert from hours to rad
	DecRad*=M_PI/180.f;    // Convert from deg to rad

	// Calc the Cartesian coord with RA and DE
	StelUtils::spheToRect(RaRad,DecRad,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);

	// "mag" will receive opacity for dark nebulae.
	QString opacityStr=list.at(6);

	if (opacityStr.contains('?')) mag=99;
	else mag=opacityStr.toFloat();

	// Calc the angular size in degrees
	float size=list.at(5).toFloat();
	angularSize = size/60.0f;

	// Barnard are dark nebulae only, so at least type is easy:
	nType=NebDn;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));

//	// Dark nebulae. Not sure how to assess visibility from opacity and size? --GZ
//	float lim;
//	// GZ: ad-hoc visibility formula: assuming good visibility if objects of mag9 are visible, "usual" opacity 5 and size 30', better visibility (discernability) comes with higher opacity and larger size,
//	// 9-(opac-5)-2*(angularSize-0.5)
//	if (angularSize>0 && mag<50)
//		lim = 15.0f - mag - 2.0f*angularSize;
//	else
//		lim = 9.0f;
//	qDebug() << "LIMIT:" << angularSize << "*" << mag << "=" << lim;

	return true;
}

bool Nebula::readSharpless(QString record)
{
	QStringList list=record.split("\t", QString::KeepEmptyParts);

	//qDebug() << "RA:" << list.at(0) << " DE:" << list.at(1) << " Sh2:" << list.at(2) << " size:" << list.at(3) << " F:" << list.at(4) << " S:" << list.at(5) << " B:" << list.at(6);

	float radeg=list.at(0).toFloat();
	float dedeg=list.at(1).toFloat();
	Sh2_nb=list.at(2).toInt();

	float RaRad=radeg*M_PI/180.f;     // Convert from degrees to rad
	float DecRad=dedeg*M_PI/180.f;    // Convert from degrees to rad

	// Calc the Cartesian coord with RA and DE
	StelUtils::spheToRect(RaRad,DecRad,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);

	mag=99.0f;

	// Calc the angular size in degrees
	int size=list.at(3).toFloat();
	angularSize = size/60.0f;

	formType = (Nebula::HIIFormType)list.at(4).toInt();
	structureType = (Nebula::HIIStructureType)list.at(5).toInt();
	brightnessType = (Nebula::HIIBrightnessType)list.at(6).toInt();

	nType=NebHII;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));

	return true;
}

bool Nebula::readVandenBergh(QString record)
{
	QStringList list=record.split("\t", QString::KeepEmptyParts);

	VdB_nb=list.at(0).toInt();
	mag = list.at(1).toFloat();
	if (mag==0.0f)
		mag=99.0f;

	// Calc the angular size in degrees
	float size=list.at(2).toFloat();
	angularSize = size/60.0f;

	float radeg=list.at(3).toFloat();
	float dedeg=list.at(4).toFloat();

	float RaRad=radeg*M_PI/180.f;     // Convert from degrees to rad
	float DecRad=dedeg*M_PI/180.f;    // Convert from degrees to rad

	// Calc the Cartesian coord with RA and DE
	StelUtils::spheToRect(RaRad,DecRad,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);

	nType=NebRn;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));

	return true;
}

bool Nebula::readRCW(QString record)
{
	QStringList list=record.split("\t", QString::KeepEmptyParts);

	float radeg=list.at(0).toFloat();
	float dedeg=list.at(1).toFloat();

	RCW_nb=list.at(2).toInt();
	mag = 99.;

	// Calc the angular size in degrees
	float size=list.at(3).toFloat();

	angularSize = size/60.0f;
	if (angularSize<0)
		angularSize=0;

	rcwBrightnessType = (Nebula::HaBrightnessType)list.at(5).toInt();

	float RaRad=radeg*M_PI/180.f;     // Convert from degrees to rad
	float DecRad=dedeg*M_PI/180.f;    // Convert from degrees to rad

	// Calc the Cartesian coord with RA and DE
	StelUtils::spheToRect(RaRad,DecRad,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);

	nType=NebHa;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));

	return true;
}

bool Nebula::readLDN(QString record)
{
	QStringList list=record.split("\t", QString::KeepEmptyParts);

	float radeg=list.at(0).toFloat();
	float dedeg=list.at(1).toFloat();

	LDN_nb=list.at(2).toInt();

	// Area in square degrees
	angularSize = list.at(3).toFloat();
	if (angularSize<0)
		angularSize=0;

	mag = list.at(4).toInt();

	float RaRad=radeg*M_PI/180.f;     // Convert from degrees to rad
	float DecRad=dedeg*M_PI/180.f;    // Convert from degrees to rad

	// Calc the Cartesian coord with RA and DE
	StelUtils::spheToRect(RaRad,DecRad,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);

	nType=NebDn;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));

	return true;
}

bool Nebula::readLBN(QString record)
{
	QStringList list=record.split("\t", QString::KeepEmptyParts);

	float radeg=list.at(0).toFloat();
	float dedeg=list.at(1).toFloat();

	LBN_nb=list.at(2).toInt();

	// Area in square degrees
	angularSize = list.at(3).toFloat()/60.f;
	if (angularSize<0)
		angularSize=0;

	brightnessClass = list.at(4).toInt();

	float RaRad=radeg*M_PI/180.f;     // Convert from degrees to rad
	float DecRad=dedeg*M_PI/180.f;    // Convert from degrees to rad

	// Calc the Cartesian coord with RA and DE
	StelUtils::spheToRect(RaRad,DecRad,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);

	nType=NebHII;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));

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
		case NebDn:
			wsType = q_("Dark nebula");
			break;
		case NebIg:
			wsType = q_("Irregular galaxy");
			break;
		case NebCn:
			wsType = q_("Cluster associated with nebulosity");
			break;
		case NebUnknown:
			wsType = q_("Unknown");
			break;
		case NebHII:
			wsType = q_("HII region");
			break;
		case NebRn:
			wsType = q_("Reflection nebula");
			break;
		case NebHa:
			wsType = q_("H-α emission region");
			break;
		default:
			wsType = q_("Undocumented type");
			break;
	}
	return wsType;
}

QString Nebula::getHIIFormTypeString() const
{
	QString wsType;

	switch(formType)
	{
		case FormCir:
			wsType = qc_("circular","form");
			break;
		case FormEll:
			wsType = qc_("elliptical","form");
			break;
		case FormIrr:
			wsType = qc_("irregular","form");
			break;
		default:
			wsType = qc_("undocumented form","form");
			break;
	}
	return wsType;
}

QString Nebula::getHIIStructureTypeString() const
{
	QString wsType;

	switch(structureType)
	{
		case StructureAmo:
			wsType = qc_("amorphous","structure");
			break;
		case StructureCon:
			wsType = qc_("conventional","structure");
			break;
		case StructureFil:
			wsType = qc_("filamentary","structure");
			break;
		default:
			wsType = qc_("undocumented structure","structure");
			break;
	}
	return wsType;
}

QString Nebula::getHIIBrightnessTypeString() const
{
	QString wsType;

	switch(brightnessType)
	{
		case Faintest:
			wsType = qc_("faintest", "HII region brightness");
			break;
		case Moderate:
			wsType = qc_("moderate", "HII region brightness");
			break;
		case Brightest:
			wsType = qc_("brightest", "HII region brightness");
			break;
		default:
			wsType = q_("undocumented brightness");
			break;
	}
	return wsType;
}

QString Nebula::getHaBrightnessTypeString() const
{
	QString wsType;

	switch(rcwBrightnessType)
	{
		case HaVeryBright:
			wsType = qc_("very bright", "H-α emission region brightness");
			break;
		case HaBright:
			wsType = qc_("bright", "H-α emission region brightness");
			break;
		case HaMedium:
			wsType = qc_("medium", "H-α emission region brightness");
			break;
		case HaFaint:
			wsType = qc_("faint", "H-α emission region brightness");
			break;
		default:
			wsType = q_("undocumented brightness");
			break;
	}
	return wsType;
}

