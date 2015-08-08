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
bool Nebula::flagUsageTypeFilter = false;
Nebula::CatalogGroup Nebula::catalogFilters = Nebula::CatalogGroup(0);
Nebula::TypeGroup Nebula::typeFilters = Nebula::TypeGroup(Nebula::AllTypes);

Nebula::Nebula()
	: DSO_nb(0)
	, M_nb(0)
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
	, PGC_nb(0)
	, UGC_nb(0)
	, Ced_nb()
	, bMag(99.)
	, vMag(99.)
	, nType()	
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

	if (!nameI18.isEmpty() && flags&Name)
	{
		oss << getNameI18n();
	}

	if (flags&CatalogNumber)
	{
		if (!nameI18.isEmpty() && flags&Name)
			oss << "<br>";

		QStringList catIds;
		if (M_nb > 0)
			catIds << QString("M %1").arg(M_nb);
		if (C_nb > 0)
			catIds << QString("C %1").arg(C_nb);
		if (NGC_nb > 0)
			catIds << QString("NGC %1").arg(NGC_nb);
		if (IC_nb > 0)
			catIds << QString("IC %1").arg(IC_nb);		
		if (B_nb > 0)
			catIds << QString("B %1").arg(B_nb);
		if (Sh2_nb > 0)
			catIds << QString("Sh 2-%1").arg(Sh2_nb);
		if (VdB_nb > 0)
			catIds << QString("VdB %1").arg(VdB_nb);
		if (RCW_nb > 0)
			catIds << QString("RCW %1").arg(RCW_nb);
		if (LDN_nb > 0)
			catIds << QString("LDN %1").arg(LDN_nb);
		if (LBN_nb > 0)
			catIds << QString("LBN %1").arg(LBN_nb);
		if (Cr_nb > 0)
			catIds << QString("Cr %1").arg(Cr_nb);
		if (Mel_nb > 0)
			catIds << QString("Mel %1").arg(Mel_nb);
		if (PGC_nb > 0)
			catIds << QString("PGC %1").arg(PGC_nb);
		if (UGC_nb > 0)
			catIds << QString("UGC %1").arg(UGC_nb);
		if (!Ced_nb.isEmpty())
			catIds << QString("Ced %1").arg(Ced_nb);		
		oss << catIds.join(" - ");

		//if (!nameI18.isEmpty() && flags&Name)
		//	oss << ")";
	}

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "</h2>";

	if (flags&ObjectType)
	{
		QString mt = getMorphologicalTypeString();
		if (mt.isEmpty())
			oss << q_("Type: <b>%1</b>").arg(getTypeString()) << "<br>";
		else
			oss << q_("Type: <b>%1</b> (%2)").arg(getTypeString()).arg(mt) << "<br>";
	}

	if (vMag < 50.f && flags&Magnitude)
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
	if (bMag < 50.f && vMag > 50.f && flags&Magnitude)
	{
		oss << q_("Magnitude: <b>%1</b>").arg(bMag, 0, 'f', 2)
		    << q_(" (Photometric system: B)")
		    << "<br>";
	}
	if (flags&Extra)
	{
		if (vMag < 50 && bMag < 50)
			oss << q_("Color Index (B-V): <b>%1</b>").arg(QString::number(bMag-vMag, 'f', 2)) << "<br>";
	}
	if (nType != NebDn && vMag < 50 && flags&Extra)
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

	oss << getPositionInfoString(core, flags);

	if (angularSize>0 && flags&Size)
	{
		if (majorAxisSize==minorAxisSize || minorAxisSize==0.f)
			oss << q_("Size: %1").arg(StelUtils::radToDmsStr(majorAxisSize*M_PI/180.)) << "<br>";
		else
		{
			oss << q_("Size: %1 x %2").arg(StelUtils::radToDmsStr(majorAxisSize*M_PI/180.)).arg(StelUtils::radToDmsStr(minorAxisSize*M_PI/180.)) << "<br>";
			if (orientationAngle>0.f)
				oss << q_("Orientation angle: %1%2").arg(orientationAngle).arg(QChar(0x00B0)) << "<br>";
		}
	}

	if (flags&Extra)
	{
		if (redshift<99.f)
		{
			QString z;
			if (redshiftErr>0.f)
				z = QString("%1%2%3").arg(QString::number(redshift, 'f', 6)).arg(QChar(0x00B1)).arg(QString::number(redshiftErr, 'f', 6));
			else
				z = QString("%1").arg(QString::number(redshift, 'f', 6));

			oss << q_("Redshift: %1").arg(z) << "<br>";
		}
		if (parallax!=0.f)
		{
			QString px, dx;
			// distance in light years from parallax
			float distance = 3.162e-5/(qAbs(parallax)*4.848e-9);
			float distanceErr = 0.f;

			if (parallaxErr>0.f)
			{
				px = QString("%1%2%3").arg(QString::number(qAbs(parallax)*0.001, 'f', 5)).arg(QChar(0x00B1)).arg(QString::number(parallaxErr*0.001, 'f', 5));
				distanceErr = 3.162e-5/(qAbs(parallaxErr)*4.848e-9);
			}
			else
				px = QString("%1").arg(QString::number(qAbs(parallax)*0.001, 'f', 5));

			if (distanceErr>0.f)
				dx = QString("%1%2%3").arg(QString::number(distance, 'f', 2)).arg(QChar(0x00B1)).arg(QString::number(distanceErr, 'f', 2));
			else
				dx = QString("%1").arg(QString::number(distance, 'f', 2));

			oss << q_("Parallax: %1\"").arg(px) << "<br>";
			if (oDistance==0.f)
				oss << q_("Distance: %1 ly").arg(dx) << "<br>";
		}

		if (oDistance>0.f)
		{
			QString dx, du;
			if (oDistanceErr>0.f)
				dx = QString("%1%2%3").arg(QString::number(oDistance, 'f', 2)).arg(QChar(0x00B1)).arg(QString::number(oDistanceErr, 'f', 2));
			else
				dx = QString("%1").arg(QString::number(oDistance, 'f', 2));

			if (nType==NebAGx || nType==NebGx || nType==NebRGx || nType==NebIGx || nType==NebQSO || nType==NebISM)
				du = q_("Mpc");
			else
				du = q_("kpc");

			oss << q_("Distance: %1 %2").arg(dx).arg(du) << "<br>";
		}
	}

	postProcessInfoString(str, flags);

	return str;
}

float Nebula::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core);
	return vMag;
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
		lim=15.0f - vMag - 2.0f*qMin(1.5f, angularSize); // Note that "mag" field is used for opacity in this catalog!
	else if (nType==NebHII) // Sharpless and LBN
		lim=10.0f - 2.0f*qMin(1.5f, angularSize); // Unfortunately, in Sh catalog, we always have mag=99=unknown!

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
	float lim = qMin(vMag, bMag);
	if (lim > 50) lim = 15.f;

	// Dark nebulae. Not sure how to assess visibility from opacity? --GZ
	if (nType==NebDn)
	{
		// GZ: ad-hoc visibility formula: assuming good visibility if objects of mag9 are visible, "usual" opacity 5 and size 30', better visibility (discernability) comes with higher opacity and larger size,
		// 9-(opac-5)-2*(angularSize-0.5)
		// GZ Not good for non-Barnards. weak opacity and large surface are antagonists. (some LDN are huge, but opacity 2 is not much to discern).
		// The qMin() maximized the visibility gain for large objects.
		if (angularSize>0 && vMag<50)
			lim = 15.0f - vMag - 2.0f*qMin(angularSize, 1.5f);
		else if (B_nb>0)
			lim = 9.0f;
		else
			lim= 12.0f; // GZ I assume LDN objects are rather elusive.
	}
	else if (nType==NebHII) // NebHII={Sharpless, LBN, RCW}
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
		case NebIGx:
			Nebula::texGalaxy->bind();
			color=galaxyColor;
			break;
		case NebAGx:
		case NebQSO:
			Nebula::texGalaxy->bind();			
			break;
		case NebRGx:
			Nebula::texGalaxy->bind();
			color=darkNebulaColor;
			break;
		case NebOc:
		case NebSA:
		case NebSC:
		case NebCl:
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
		case NebSNR:
		case NebBn:
		case NebEn:
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
	if (!objectInDisplayedType())
		col = Vec3f(0.f,0.f,0.f);
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
	float lim = qMin(vMag, bMag);
	if (lim > 50) lim = 15.f;

	// Dark nebulae. Not sure how to assess visibility from opacity? --GZ
	if (nType==NebDn)
	{
		// GZ: ad-hoc visibility formula: assuming good visibility if objects of mag9 are visible, "usual" opacity 5 and size 30', better visibility (discernability) comes with higher opacity and larger size,
		// 9-(opac-5)-2*(angularSize-0.5)
		if (angularSize>0 && vMag<50)
			lim = 15.0f - vMag - 2.0f*qMin(angularSize, 2.5f);
		else if (B_nb>0)
			lim = 9.0f;
		else
			lim= 12.0f; // GZ I assume some LDN objects are rather elusive.
	}
	else if (nType==NebHII)
		lim=9.0f;

	if (lim>maxMagLabel)
		return;

	Vec3d win;
	// Check visibility of DSO labels
	if (!(sPainter.getProjector()->projectCheck(XYZ, win)))
		return;

	Vec3f col(labelColor[0], labelColor[1], labelColor[2]);
	if (objectInDisplayedType())
		sPainter.setColor(col[0], col[1], col[2], hintsBrightness);
	else
		sPainter.setColor(col[0], col[1], col[2], 0.f);

	float size = getAngularSize(NULL)*M_PI/180.*sPainter.getProjector()->getPixelPerRadAtCenter();
	float shift = 4.f + (drawHintProportional ? size : size/1.8f);

	QString str = getNameI18n();
	if (str.isEmpty())
	{
		// On screen label: one only, priority as given here.
		if (catalogFilters&CatM && M_nb>0)
			str = QString("M %1").arg(M_nb);
		else if (catalogFilters&CatC && C_nb>0)
			str = QString("C %1").arg(C_nb);
		else if (catalogFilters&CatNGC && NGC_nb>0)
			str = QString("NGC %1").arg(NGC_nb);
		else if (catalogFilters&CatIC && IC_nb>0)
			str = QString("IC %1").arg(IC_nb);
		else if (catalogFilters&CatB && B_nb>0)
			str = QString("B %1").arg(B_nb);
		else if (catalogFilters&CatSh2 && Sh2_nb>0)
			str = QString("Sh 2-%1").arg(Sh2_nb);
		else if (catalogFilters&CatVdB && VdB_nb>0)
			str = QString("VdB %1").arg(VdB_nb);
		else if (catalogFilters&CatRCW && RCW_nb>0)
			str = QString("RCW %1").arg(RCW_nb);
		else if (catalogFilters&CatLDN && LDN_nb>0)
			str = QString("LDN %1").arg(LDN_nb);
		else if (catalogFilters&CatLBN && LBN_nb > 0)
			str = QString("LBN %1").arg(LBN_nb);
		else if (catalogFilters&CatCr && Cr_nb > 0)
			str = QString("Cr %1").arg(Cr_nb);
		else if (catalogFilters&CatMel && Mel_nb > 0)
			str = QString("Mel %1").arg(Mel_nb);
		else if (catalogFilters&CatPGC && PGC_nb > 0)
			str = QString("PGC %1").arg(PGC_nb);
		else if (catalogFilters&CatUGC && UGC_nb > 0)
			str = QString("UGC %1").arg(UGC_nb);
		else if (catalogFilters&CatCed && !Ced_nb.isEmpty())
			str = QString("Ced %1").arg(Ced_nb);
	}

	sPainter.drawText(XY[0]+shift, XY[1]+shift, str, 0, 0, 0, false);
}

void Nebula::readDSO(QDataStream &in)
{
	float	ra, dec;
	unsigned int oType;

	in	>> DSO_nb >> ra >> dec >> bMag >> vMag >> oType >> mTypeString >> majorAxisSize >> minorAxisSize
		>> orientationAngle >> redshift >> redshiftErr >> parallax >> parallaxErr >> oDistance >> oDistanceErr
		>> NGC_nb >> IC_nb >> M_nb >> C_nb >> B_nb >> Sh2_nb >> VdB_nb >> RCW_nb >> LDN_nb >> LBN_nb >> Cr_nb
		>> Mel_nb >> PGC_nb >> UGC_nb >> Ced_nb;

	if (majorAxisSize!=minorAxisSize && minorAxisSize>0.f)
		angularSize = majorAxisSize*minorAxisSize;
	else
		angularSize = majorAxisSize;

	StelUtils::spheToRect(ra,dec,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);
	nType = (Nebula::NebulaType)oType;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));
}

bool Nebula::objectInDisplayedType()
{
	if (!flagUsageTypeFilter)
		return true;

	bool r = false;
	int cntype = -1;
	switch (nType)
	{
		case NebGx:
			cntype = 0; // Galaxies
			break;
		case NebAGx:
		case NebRGx:
		case NebQSO:
			cntype = 1; // Active Galaxies
			break;
		case NebIGx:
			cntype = 2; // Interacting Galaxies
			break;
		case NebOc:
		case NebGc:
		case NebCl:
		case NebSA:
		case NebSC:
			cntype = 3; // Star Clusters
			break;
		case NebHII:
		case NebISM:
			cntype = 4; // Hydrogen regions (include interstellar matter)
			break;
		case NebN:
		case NebBn:
		case NebEn:
		case NebRn:
			cntype = 5; // Bright Nebulae
			break;
		case NebDn:
			cntype = 6; // Dark Nebulae
			break;
		case NebPn:
			cntype = 7; // Planetary Nebulae
			break;
		case NebSNR:
			cntype = 8; // Supernova Remnants
			break;
		case NebCn:
			cntype = 9;
			break;
		default:
			cntype = 10;
			break;
	}
	if (typeFilters&TypeGalaxies && cntype==0)
		r = true;
	else if (typeFilters&TypeActiveGalaxies && cntype==1)
		r = true;
	else if (typeFilters&TypeInteractingGalaxies && cntype==2)
		r = true;
	else if (typeFilters&TypeStarClusters && cntype==3)
		r = true;
	else if (typeFilters&TypeHydrogenRegions && cntype==4)
		r = true;
	else if (typeFilters&TypeBrightNebulae && cntype==5)
		r = true;
	else if (typeFilters&TypeDarkNebulae && cntype==6)
		r = true;
	else if (typeFilters&TypePlanetaryNebulae && cntype==7)
		r = true;
	else if (typeFilters&TypeSupernovaRemnants && cntype==8)
		r = true;
	else if (typeFilters&TypeStarClusters && (typeFilters&TypeBrightNebulae || typeFilters&TypeHydrogenRegions) && cntype==9)
		r = true;
	else if (typeFilters&TypeOther && cntype==10)
		r = true;

	return r;
}

QString Nebula::getMorphologicalTypeString(void) const
{
	return mTypeString;
}

QString Nebula::getTypeString(void) const
{
	QString wsType;

	switch(nType)
	{
		case NebGx:
			wsType = q_("galaxy");
			break;
		case NebAGx:
			wsType = q_("active galaxy");
			break;
		case NebRGx:
			wsType = q_("radio galaxy");
			break;
		case NebIGx:
			wsType = q_("interacting galaxy");
			break;
		case NebQSO:
			wsType = q_("quasar");
			break;
		case NebCl:
			wsType = q_("star cluster");
			break;
		case NebOc:
			wsType = q_("open star cluster");
			break;
		case NebGc:
			wsType = q_("globular cluster");
			break;
		case NebN:
			wsType = q_("nebula");
			break;
		case NebPn:
			wsType = q_("planetary nebula");
			break;
		case NebDn:
			wsType = q_("dark nebula");
			break;
		case NebCn:
			wsType = q_("cluster associated with nebulosity");
			break;
		case NebBn:
			wsType = q_("bipolar nebula");
			break;
		case NebEn:
			wsType = q_("emission nebula");
			break;
		case NebHII:
			wsType = q_("HII region");
			break;
		case NebRn:
			wsType = q_("reflection nebula");
			break;		
		case NebSNR:
			wsType = q_("supernova remnant");
			break;
		case NebSA:
			wsType = q_("stellar association");
			break;
		case NebSC:
			wsType = q_("star cloud");
			break;
		case NebISM:
			wsType = q_("interstellar matter");
			break;
		case NebEMO:
			wsType = q_("emission object");
			break;
		case NebUnknown:
			wsType = q_("unknown");
			break;
		default:
			wsType = q_("undocumented type");
			break;
	}
	return wsType;
}

