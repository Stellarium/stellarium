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

const QString Nebula::NEBULA_TYPE = QStringLiteral("Nebula");

StelTextureSP Nebula::texCircle;
StelTextureSP Nebula::texGalaxy;
StelTextureSP Nebula::texOpenCluster;
StelTextureSP Nebula::texGlobularCluster;
StelTextureSP Nebula::texPlanetaryNebula;
StelTextureSP Nebula::texDiffuseNebula;
StelTextureSP Nebula::texDarkNebula;
StelTextureSP Nebula::texOpenClusterWithNebulosity;
bool  Nebula::drawHintProportional = false;
bool  Nebula::surfaceBrightnessUsage = false;
bool  Nebula::designationUsage = false;
float Nebula::hintsBrightness = 0.f;
Vec3f Nebula::labelColor = Vec3f(0.4f,0.3f,0.5f);
Vec3f Nebula::circleColor = Vec3f(0.8f,0.8f,0.1f);
Vec3f Nebula::galaxyColor = Vec3f(1.0f,0.2f,0.2f);
Vec3f Nebula::radioGalaxyColor = Vec3f(0.3f,0.3f,0.3f);
Vec3f Nebula::activeGalaxyColor = Vec3f(0.8f,0.8f,0.1f);
Vec3f Nebula::interactingGalaxyColor = Vec3f(0.8f,0.8f,0.1f);
Vec3f Nebula::quasarColor = Vec3f(1.0f,0.2f,0.2f);
Vec3f Nebula::nebulaColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::planetaryNebulaColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::reflectionNebulaColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::bipolarNebulaColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::emissionNebulaColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::darkNebulaColor = Vec3f(0.3f,0.3f,0.3f);
Vec3f Nebula::hydrogenRegionColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::supernovaRemnantColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::interstellarMatterColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::clusterWithNebulosityColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::clusterColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::openClusterColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::globularClusterColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::stellarAssociationColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::starCloudColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::emissionObjectColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::blLacObjectColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::blazarColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::molecularCloudColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::youngStellarObjectColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::possibleQuasarColor = Vec3f(1.0f,0.2f,0.2f);
Vec3f Nebula::possiblePlanetaryNebulaColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::protoplanetaryNebulaColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::starColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::symbioticStarColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::emissionLineStarColor = Vec3f(1.0f,1.0f,0.1f);
Vec3f Nebula::supernovaCandidateColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::supernovaRemnantCandidateColor = Vec3f(0.1f,1.0f,0.1f);
Vec3f Nebula::galaxyClusterColor = Vec3f(0.8f,0.8f,0.5f);
bool Nebula::flagUseTypeFilters = false;
Nebula::CatalogGroup Nebula::catalogFilters = Nebula::CatalogGroup(0);
Nebula::TypeGroup Nebula::typeFilters = Nebula::TypeGroup(Nebula::AllTypes);
bool Nebula::flagUseArcsecSurfaceBrightness = false;
bool Nebula::flagUseShortNotationSurfaceBrightness = true;
bool Nebula::flagUseOutlines = false;
bool Nebula::flagShowAdditionalNames = true;
bool Nebula::flagUseSizeLimits = false;
double Nebula::minSizeLimit = 1.0f;
double Nebula::maxSizeLimit = 600.0f;

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
	, Arp_nb(0)
	, VV_nb(0)	
	, Ced_nb("")
	, PK_nb("")
	, PNG_nb("")
	, SNRG_nb("")
	, ACO_nb("")
	, withoutID(false)
	, nameI18("")
	, mTypeString()
	, bMag(99.)
	, vMag(99.)
	, majorAxisSize(0.)
	, minorAxisSize(0.)
	, orientationAngle(0)
	, oDistance(0.)
	, oDistanceErr(0.)
	, redshift(99.)
	, redshiftErr(0.)
	, parallax(0.)
	, parallaxErr(0.)
	, nType()
{
	outlineSegments.clear();
}

Nebula::~Nebula()
{
}

QString Nebula::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));
	Q_UNUSED(az_app);

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "<h2>";

	if (!nameI18.isEmpty() && flags&Name)
	{
		oss << getNameI18n();
		QString aliases = getI18nAliases();
		if (!aliases.isEmpty() && flagShowAdditionalNames)
			oss << " (" << aliases << ")";
	}

	if (flags&CatalogNumber)
	{
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
			catIds << QString("SH 2-%1").arg(Sh2_nb);
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
		if (Arp_nb > 0)
			catIds << QString("Arp %1").arg(Arp_nb);
		if (VV_nb > 0)
			catIds << QString("VV %1").arg(VV_nb);
		if (!PK_nb.isEmpty())
			catIds << QString("PK %1").arg(PK_nb);
		if (!PNG_nb.isEmpty())
			catIds << QString("PN G%1").arg(PNG_nb);
		if (!SNRG_nb.isEmpty())
			catIds << QString("SNR G%1").arg(SNRG_nb);
		if (!ACO_nb.isEmpty())
			catIds << QString("ACO %1").arg(ACO_nb);

		if (!nameI18.isEmpty() && !catIds.isEmpty() && flags&Name)
			oss << "<br>";

		oss << catIds.join(" - ");
	}

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "</h2>";

	if (flags&ObjectType)
	{
		QString mt = getMorphologicalTypeString();
		if (mt.isEmpty())
			oss << QString("%1: <b>%2</b>").arg(q_("Type"), getTypeString()) << "<br>";
		else
			oss << QString("%1: <b>%2</b> (%3)").arg(q_("Type"), getTypeString(), mt) << "<br>";
	}

	if (vMag < 50.f && flags&Magnitude)
	{
		QString emag = "";
		QString tmag = q_("Magnitude");
		if (nType == NebDn)
			tmag = q_("Opacity");

		if (nType != NebDn && core->getSkyDrawer()->getFlagHasAtmosphere() && (alt_app>-3.0*M_PI/180.0)) // Don't show extincted magnitude much below horizon where model is meaningless.
			emag = QString(" (%1: <b>%2</b>)").arg(q_("extincted to"), QString::number(getVMagnitudeWithExtinction(core), 'f', 2));

		oss << QString("%1: <b>%2</b>%3").arg(tmag, QString::number(getVMagnitude(core), 'f', 2), emag) << "<br />";
	}
	if (bMag < 50.f && vMag > 50.f && flags&Magnitude)
		oss << QString("%1: <b>%2</b> (%3: B)").arg(q_("Magnitude"), QString::number(bMag, 'f', 2), q_("Photometric system")) << "<br />";

	if (flags&Extra)
	{
		if (vMag < 50 && bMag < 50)
			oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(bMag-vMag, 'f', 2)) << "<br />";
	}
	float mmag = qMin(vMag,bMag);
	if (nType != NebDn && mmag < 50 && flags&Extra)
	{
		QString sb = q_("Surface brightness");
		QString ae = q_("after extinction");
		QString mu;
		if (flagUseShortNotationSurfaceBrightness)
		{
			mu = QString("<sup>m</sup>/%1'").arg(QChar(0x2B1C));
			if (flagUseArcsecSurfaceBrightness)
				mu = QString("<sup>m</sup>/%1\"").arg(QChar(0x2B1C));
		}
		else
		{
			mu = QString("%1/%2<sup>2</sup>").arg(qc_("mag", "magnitude"), q_("arcmin"));
			if (flagUseArcsecSurfaceBrightness)
				mu = QString("%1/%2<sup>2</sup>").arg(qc_("mag", "magnitude"), q_("arcsec"));

		}

		if (getSurfaceBrightness(core)<99)
		{
			if (core->getSkyDrawer()->getFlagHasAtmosphere() && (alt_app>-3.0*M_PI/180.0) && getSurfaceBrightnessWithExtinction(core)<99) // Don't show extincted surface brightness much below horizon where model is meaningless.
			{
				oss << QString("%1: <b>%2</b> %5 (%3: <b>%4</b> %5)").arg(sb, QString::number(getSurfaceBrightness(core, flagUseArcsecSurfaceBrightness), 'f', 2),
											  ae, QString::number(getSurfaceBrightnessWithExtinction(core, flagUseArcsecSurfaceBrightness), 'f', 2), mu) << "<br />";
			}
			else
				oss << QString("%1: <b>%2</b> %3").arg(sb, QString::number(getSurfaceBrightness(core, flagUseArcsecSurfaceBrightness), 'f', 2), mu) << "<br />";

			oss << QString("%1: %2").arg(q_("Contrast index"), QString::number(getContrastIndex(core), 'f', 2)) << "<br />";
		}
	}

	oss << getCommonInfoString(core, flags);

	if (majorAxisSize>0 && flags&Size)
	{
		if (majorAxisSize==minorAxisSize || minorAxisSize==0.f)
			oss << QString("%1: %2").arg(q_("Size"), StelUtils::radToDmsStr(majorAxisSize*M_PI/180.)) << "<br />";
		else
		{
			oss << QString("%1: %2 x %3").arg(q_("Size"), StelUtils::radToDmsStr(majorAxisSize*M_PI/180.), StelUtils::radToDmsStr(minorAxisSize*M_PI/180.)) << "<br />";
			if (orientationAngle>0)
				oss << QString("%1: %2%3").arg(q_("Orientation angle")).arg(orientationAngle).arg(QChar(0x00B0)) << "<br />";
		}
	}

	if (flags&Distance)
	{
		if (parallax!=0.f)
		{
			QString dx;
			// distance in light years from parallax
			float distance = 3.162e-5/(qAbs(parallax)*4.848e-9);
			float distanceErr = 0.f;

			if (parallaxErr>0.f)
				distanceErr = 3.162e-5/(qAbs(parallaxErr)*4.848e-9);

			if (distanceErr>0.f)
				dx = QString("%1%2%3").arg(QString::number(distance, 'f', 3)).arg(QChar(0x00B1)).arg(QString::number(distanceErr, 'f', 3));
			else
				dx = QString("%1").arg(QString::number(distance, 'f', 3));

			if (oDistance==0.f)
			{
				// TRANSLATORS: Unit of measure for distance - Light Years
				QString ly = qc_("ly", "distance");
				oss << QString("%1: %2 %3").arg(q_("Distance"), dx, ly) << "<br />";
			}
		}

		if (oDistance>0.f)
		{
			QString dx, dy;
			float dc = 3262.f;
			int ms = 1;
			//TRANSLATORS: Unit of measure for distance - kiloparsecs
			QString dupc = qc_("kpc", "distance");
			//TRANSLATORS: Unit of measure for distance - Light Years
			QString duly = qc_("ly", "distance");

			if (nType==NebAGx || nType==NebGx || nType==NebRGx || nType==NebIGx || nType==NebQSO || nType==NebPossQSO)
			{
				dc = 3.262f;
				ms = 3;
				//TRANSLATORS: Unit of measure for distance - Megaparsecs
				dupc = qc_("Mpc", "distance");
				//TRANSLATORS: Unit of measure for distance - Millions of Light Years
				duly = qc_("M ly", "distance");
			}

			if (oDistanceErr>0.f)
			{
				dx = QString("%1%2%3").arg(QString::number(oDistance, 'f', 3)).arg(QChar(0x00B1)).arg(QString::number(oDistanceErr, 'f', 3));
				dy = QString("%1%2%3").arg(QString::number(oDistance*dc, 'f', ms)).arg(QChar(0x00B1)).arg(QString::number(oDistanceErr*dc, 'f', ms));
			}
			else
			{
				dx = QString("%1").arg(QString::number(oDistance, 'f', 3));
				dy = QString("%1").arg(QString::number(oDistance*dc, 'f', ms));
			}

			oss << QString("%1: %2 %3 (%4 %5)").arg(q_("Distance"), dx, dupc, dy, duly) << "<br />";
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

			oss << QString("%1: %2").arg(q_("Redshift"), z) << "<br />";
		}
		if (parallax!=0.f)
		{
			QString px;

			if (parallaxErr>0.f)
				px = QString("%1%2%3").arg(QString::number(qAbs(parallax)*0.001, 'f', 5)).arg(QChar(0x00B1)).arg(QString::number(parallaxErr*0.001, 'f', 5));
			else
				px = QString("%1").arg(QString::number(qAbs(parallax)*0.001, 'f', 5));

			oss << QString("%1: %2\"").arg(q_("Parallax"), px) << "<br />";
		}

		if (!getMorphologicalTypeDescription().isEmpty())
			oss << QString("%1: %2.").arg(q_("Morphological description"), getMorphologicalTypeDescription()) << "<br />";

	}

	postProcessInfoString(str, flags);

	return str;
}

QVariantMap Nebula::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	map["type"]=getTypeString(); // replace "Nebula" type by detail. This is localized. Maybe add argument such as getTypeString(bool translated=true)?
	map.insert("morpho", getMorphologicalTypeString());
	map.insert("surface-brightness", getSurfaceBrightness(core));
	map.insert("bmag", bMag);
	if (vMag < 50 && bMag < 50)
		map.insert("bV", bMag-vMag);
	if (redshift<99.f)
		map.insert("redshift", redshift);

	// TODO: more? Names? Data?
	return map;
}

QString Nebula::getEnglishAliases() const
{
	QString aliases = "";	
	int asize = englishAliases.size();
	if (asize!=0)
	{
		if (asize>3) // Special case for many AKA
		{
			for(int i=0; i<asize; i++)
			{
				aliases.append(englishAliases.at(i));
				if (i<asize-1)
					aliases.append(" - ");

				if (i==1) // 2 AKA-items on first line!
					aliases.append("<br />");
			}
		}
		else
			aliases = nameI18Aliases.join(" - ");
	}
	return aliases;
}

QString Nebula::getI18nAliases() const
{
	QString aliases = "";
	int asize = nameI18Aliases.size();
	if (asize!=0)
	{
		if (asize>3) // Special case for many AKA
		{
			for(int i=0; i<asize; i++)
			{
				aliases.append(nameI18Aliases.at(i));
				if (i<asize-1)
					aliases.append(" - ");

				if (i==1) // 2 AKA-items on first line!
					aliases.append("<br />");
			}
		}
		else
			aliases = nameI18Aliases.join(" - ");
	}
	return aliases;
}

float Nebula::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core);	
	return vMag;
}

double Nebula::getAngularSize(const StelCore *) const
{
	float size = majorAxisSize;
	if (majorAxisSize!=minorAxisSize || minorAxisSize>0)
		size = (majorAxisSize+minorAxisSize)*0.5f;
	return size;
}

float Nebula::getSelectPriority(const StelCore* core) const
{
	const NebulaMgr* nebMgr = ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"));
	// minimize unwanted selection of the deep-sky objects
	if (!nebMgr->getFlagHints())
		return StelObject::getSelectPriority(core)+3.f;

	float lim, mag;
	lim = mag = getVMagnitude(core);
	float mLim = 15.0f;

	if (!objectInDisplayedCatalog() || !objectInDisplayedType())
		return StelObject::getSelectPriority(core)+mLim;

	const StelSkyDrawer* drawer = core->getSkyDrawer();

	if (drawer->getFlagNebulaMagnitudeLimit() && (mag>drawer->getCustomNebulaMagnitudeLimit()))
		return StelObject::getSelectPriority(core)+mLim;
	
	const float maxMagHint = nebMgr->computeMaxMagHint(drawer);
	// make very easy to select if labeled	
	if (surfaceBrightnessUsage)
	{
		lim = mag = getSurfaceBrightness(core);
		mLim += 1.f;
	}

	if (nType==NebDn)
		lim=mLim - mag - 2.0f*qMin(1.5f, majorAxisSize); // Note that "mag" field is used for opacity in this catalog!
	else if (nType==NebHII) // Sharpless and LBN
		lim=10.0f - 2.0f*qMin(1.5f, majorAxisSize); // Unfortunately, in Sh catalog, we always have mag=99=unknown!

	if (std::min(mLim, lim)<maxMagHint || outlineSegments.size()>0) // High priority for big DSO (with outlines)
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
	return majorAxisSize>0 ? majorAxisSize * 4 : 1;
}

float Nebula::getSurfaceBrightness(const StelCore* core, bool arcsec) const
{
	float mag = getVMagnitude(core);
	float sq = 3600.f; // arcmin^2
	if (arcsec)
		sq = 12.96e6; // 3600.f*3600.f, i.e. arcsec^2
	if (bMag < 90.f && mag > 90.f)
		mag = bMag;
	if (mag<99.f && majorAxisSize>0 && nType!=NebDn)
		return mag + 2.5*log10(getSurfaceArea()*sq);
	else
		return 99.f;
}

float Nebula::getSurfaceBrightnessWithExtinction(const StelCore* core, bool arcsec) const
{
	float sq = 3600.f; // arcmin^2
	if (arcsec)
		sq = 12.96e6; // 3600.f*3600.f, i.e. arcsec^2
	if (getVMagnitudeWithExtinction(core)<99.f && majorAxisSize>0 && nType!=NebDn)
		return getVMagnitudeWithExtinction(core) + 2.5*log10(getSurfaceArea()*sq);
	else
		return 99.f;
}

float Nebula::getContrastIndex(const StelCore* core) const
{
	// Compute an extended object's contrast index: http://www.unihedron.com/projects/darksky/NELM2BCalc.html

	// Sky brightness
	// Source: Schaefer, B.E. Feb. 1990. Telescopic Limiting Magnitude. PASP 102:212-229
	// URL: http://adsbit.harvard.edu/cgi-bin/nph-iarticle_query?bibcode=1990PASP..102..212S [1990PASP..102..212S]
	float B_mpsas = 21.58 - 5*log10(std::pow(10, 1.586 - core->getSkyDrawer()->getNELMFromBortleScale()/5)-1);
	// Compute an extended object's contrast index
	// Source: Clark, R.N., 1990. Appendix E in Visual Astronomy of the Deep Sky, Cambridge University Press and Sky Publishing.
	// URL: http://www.clarkvision.com/visastro/appendix-e.html
	return -0.4 * (getSurfaceBrightnessWithExtinction(core, true) - B_mpsas);
}

float Nebula::getSurfaceArea(void) const
{
	if (majorAxisSize==minorAxisSize || minorAxisSize==0)
		return M_PI*(majorAxisSize/2.f)*(majorAxisSize/2.f); // S = pi*R^2 = pi*(D/2)^2
	else
		return M_PI*(majorAxisSize/2.f)*(minorAxisSize/2.f); // S = pi*a*b
}

Vec3f Nebula::getHintColor(void) const
{
	Vec3f color=circleColor;
	switch (nType)
	{
		case NebGx:
			color=galaxyColor;
			break;
		case NebIGx:
			color=interactingGalaxyColor;
			break;
		case NebAGx:
			color=activeGalaxyColor;
			break;
		case NebQSO:
			color=quasarColor;
			break;
		case NebPossQSO:
			color=possibleQuasarColor;
			break;
		case NebBLL:
			color=blLacObjectColor;
			break;
		case NebBLA:
			color=blazarColor;
			break;
		case NebRGx:
			color=radioGalaxyColor;
			break;
		case NebOc:
			color=openClusterColor;
			break;
		case NebSA:
			color=stellarAssociationColor;
			break;
		case NebSC:
			color=starCloudColor;
			break;
		case NebCl:
			color=clusterColor;
			break;
		case NebGc:
			color=globularClusterColor;
			break;
		case NebN:
			color=nebulaColor;
			break;
		case NebHII:
			color=hydrogenRegionColor;
			break;
		case NebMolCld:
			color=molecularCloudColor;
			break;
		case NebYSO:
			color=youngStellarObjectColor;
			break;
		case NebRn:
			color=reflectionNebulaColor;
			break;
		case NebSNR:
			color=supernovaRemnantColor;
			break;
		case NebBn:
			color=bipolarNebulaColor;
			break;
		case NebEn:
			color=emissionNebulaColor;
			break;
		case NebPn:
			color=planetaryNebulaColor;
			break;
		case NebPossPN:
			color=possiblePlanetaryNebulaColor;
			break;
		case NebPPN:
			color=protoplanetaryNebulaColor;
			break;
		case NebDn:
			color=darkNebulaColor;
			break;
		case NebCn:
			color=clusterWithNebulosityColor;
			break;
		case NebEMO:
			color=emissionObjectColor;
			break;
		case NebStar:
			color=starColor;
			break;
		case NebSymbioticStar:
			color=symbioticStarColor;
			break;
		case NebEmissionLineStar:
			color=emissionLineStarColor;
			break;
		case NebSNC:
			color=supernovaCandidateColor;
			break;
		case NebSNRC:
			color=supernovaRemnantCandidateColor;
			break;
		case NebGxCl:
			color=galaxyClusterColor;
			break;
		default:
			color=circleColor;
	}

	return color;
}

float Nebula::getVisibilityLevelByMagnitude(void) const
{
	StelCore* core = StelApp::getInstance().getCore();

	float lim = qMin(vMag, bMag);
	float mLim = 15.0f;

	if (surfaceBrightnessUsage)
	{
		lim = getSurfaceBrightness(core) - 3.f;
		if (lim > 90) lim = mLim + 1.f;
	}
	else
	{
		float mag = getVMagnitude(core);
		if (lim > 90) lim = mLim;

		// Dark nebulae. Not sure how to assess visibility from opacity? --GZ
		if (nType==NebDn)
		{
			// GZ: ad-hoc visibility formula: assuming good visibility if objects of mag9 are visible, "usual" opacity 5 and size 30', better visibility (discernability) comes with higher opacity and larger size,
			// 9-(opac-5)-2*(angularSize-0.5)
			// GZ Not good for non-Barnards. weak opacity and large surface are antagonists. (some LDN are huge, but opacity 2 is not much to discern).
			// The qMin() maximized the visibility gain for large objects.
			if (majorAxisSize>0 && mag<90)
				lim = mLim - mag - 2.0f*qMin(majorAxisSize, 1.5f);
			else if (B_nb>0)
				lim = 9.0f;
			else
				lim= 12.0f; // GZ I assume LDN objects are rather elusive.
		}
		else if (nType==NebHII) // NebHII={Sharpless, LBN, RCW}
		{ // artificially increase visibility of (most) Sharpless objects? No magnitude recorded:-(
			lim=9.0f;
		}
	}

	return lim;
}

void Nebula::drawOutlines(StelPainter &sPainter, float maxMagHints) const
{
	size_t segments = outlineSegments.size();
	Vec3f color = getHintColor();

	// tune limits for outlines
	float oLim = getVisibilityLevelByMagnitude() - 3.f;

	float lum = 1.f;
	Vec3f col(color[0]*lum*hintsBrightness, color[1]*lum*hintsBrightness, color[2]*lum*hintsBrightness);
	if (!objectInDisplayedType())
		col = Vec3f(0.f,0.f,0.f);
	sPainter.setColor(col[0], col[1], col[2], 1);

	// Show outlines
	if (segments>0 && flagUseOutlines && oLim<=maxMagHints)
	{
		unsigned int i, j;
		Vec3f pt1, pt2;
		Vec3d ptd1, ptd2;
		std::vector<Vec3f> *points;

		sPainter.setBlending(true);
		sPainter.setLineSmooth(true);
		const SphericalCap& viewportHalfspace = sPainter.getProjector()->getBoundingCap();

		for (i=0;i<segments;i++)
		{
			points = outlineSegments[i];

			for (j=0;j<points->size()-1;j++)
			{
				pt1 = points->at(j);
				pt2 = points->at(j+1);
				ptd1.set(pt1[0], pt1[1], pt1[2]);
				ptd2.set(pt2[0], pt2[1], pt2[2]);
				sPainter.drawGreatCircleArc(ptd1, ptd2, &viewportHalfspace);
			}
		}
		sPainter.setLineSmooth(false);
	}

}

void Nebula::drawHints(StelPainter& sPainter, float maxMagHints) const
{
	size_t segments = outlineSegments.size();
	Vec3d win;
	// Check visibility of DSO hints
	if (!(sPainter.getProjector()->projectCheck(XYZ, win)))
		return;

	if (getVisibilityLevelByMagnitude()>maxMagHints)
		return;

	Vec3f color = getHintColor();
	switch (nType)
	{
		case NebGx:
			Nebula::texGalaxy->bind();
			break;
		case NebIGx:
			Nebula::texGalaxy->bind();
			break;
		case NebAGx:
			Nebula::texGalaxy->bind();
			break;
		case NebQSO:
			Nebula::texGalaxy->bind();
			break;
		case NebPossQSO:
			Nebula::texGalaxy->bind();
			break;
		case NebBLL:
			Nebula::texGalaxy->bind();
			break;
		case NebBLA:
			Nebula::texGalaxy->bind();
			break;
		case NebRGx:
			Nebula::texGalaxy->bind();
			break;
		case NebOc:
			Nebula::texOpenCluster->bind();
			break;
		case NebSA:
			Nebula::texOpenCluster->bind();
			break;
		case NebSC:
			Nebula::texOpenCluster->bind();
			break;
		case NebCl:
			Nebula::texOpenCluster->bind();
			break;
		case NebGc:
			Nebula::texGlobularCluster->bind();
			break;
		case NebN:
			Nebula::texDiffuseNebula->bind();
			break;
		case NebHII:
			Nebula::texDiffuseNebula->bind();
			break;
		case NebMolCld:
			Nebula::texDiffuseNebula->bind();
			break;
		case NebYSO:
			Nebula::texDiffuseNebula->bind();
			break;
		case NebRn:		
			Nebula::texDiffuseNebula->bind();
			break;
		case NebSNR:
			Nebula::texDiffuseNebula->bind();
			break;
		case NebBn:
			Nebula::texDiffuseNebula->bind();
			break;
		case NebEn:
			Nebula::texDiffuseNebula->bind();
			break;
		case NebPn:
			Nebula::texPlanetaryNebula->bind();
			break;
		case NebPossPN:
			Nebula::texPlanetaryNebula->bind();
			break;
		case NebPPN:
			Nebula::texPlanetaryNebula->bind();
			break;
		case NebDn:		
			Nebula::texDarkNebula->bind();
			break;
		case NebCn:
			Nebula::texOpenClusterWithNebulosity->bind();
			break;
		case NebEMO:
			Nebula::texCircle->bind();
			break;
		case NebStar:
			Nebula::texCircle->bind();
			break;
		case NebSymbioticStar:
			Nebula::texCircle->bind();
			break;
		case NebEmissionLineStar:
			Nebula::texCircle->bind();
			break;
		case NebSNC:
			Nebula::texDiffuseNebula->bind();
			break;
		case NebSNRC:
			Nebula::texDiffuseNebula->bind();
			break;
		case NebGxCl:
			Nebula::texGalaxy->bind();
			break;
		default:
			Nebula::texCircle->bind();
	}

	float lum = 1.f;
	Vec3f col(color[0]*lum*hintsBrightness, color[1]*lum*hintsBrightness, color[2]*lum*hintsBrightness);
	if (!objectInDisplayedType())
		col = Vec3f(0.f,0.f,0.f);
	sPainter.setColor(col[0], col[1], col[2], 1);

	float size = 6.0f;
	float scaledSize = 0.0f;
	if (drawHintProportional && segments==0)
	{
		if (majorAxisSize>0.)
			scaledSize = majorAxisSize *0.5 *M_PI/180.*sPainter.getProjector()->getPixelPerRadAtCenter();
		else
			scaledSize = minorAxisSize *0.5 *M_PI/180.*sPainter.getProjector()->getPixelPerRadAtCenter();
	}

	sPainter.setBlending(true, GL_ONE, GL_ONE);

	// Rotation looks good only for galaxies.
	if ((nType <=NebQSO) || (nType==NebBLA) || (nType==NebBLL) )
	{
		// The rotation angle in drawSprite2dMode() is relative to screen. Make sure to compute correct angle from 90+orientationAngle.
		// Find an on-screen direction vector from a point offset somewhat in declination from our object.
		Vec3d XYZrel(XYZ);
		XYZrel[2]*=0.99;
		Vec3d XYrel;
		sPainter.getProjector()->project(XYZrel, XYrel);
		float screenAngle=atan2(XYrel[1]-XY[1], XYrel[0]-XY[0]);
		sPainter.drawSprite2dMode(XY[0], XY[1], qMax(size, scaledSize), screenAngle*180./M_PI + orientationAngle);
	}
	else	// no galaxy
		sPainter.drawSprite2dMode(XY[0], XY[1], qMax(size, scaledSize));

}

void Nebula::drawLabel(StelPainter& sPainter, float maxMagLabel) const
{
	Vec3d win;
	// Check visibility of DSO labels
	if (!(sPainter.getProjector()->projectCheck(XYZ, win)))
		return;

	if (getVisibilityLevelByMagnitude()>maxMagLabel)
		return;

	Vec3f col(labelColor[0], labelColor[1], labelColor[2]);
	if (objectInDisplayedType())
		sPainter.setColor(col[0], col[1], col[2], hintsBrightness);
	else
		sPainter.setColor(col[0], col[1], col[2], 0.f);

	float size = getAngularSize(Q_NULLPTR)*M_PI/180.*sPainter.getProjector()->getPixelPerRadAtCenter();
	float shift = 4.f + (drawHintProportional ? size : size/1.8f);

	QString str = getNameI18n();
	if (str.isEmpty() || designationUsage)
		str = getDSODesignation();

	sPainter.drawText(XY[0]+shift, XY[1]+shift, str, 0, 0, 0, false);
}

QString Nebula::getDSODesignation() const
{
	QString str = "";
	// Get designation for DSO with priority as given here.
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
		str = QString("SH 2-%1").arg(Sh2_nb);
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
	else if (catalogFilters&CatArp && Arp_nb > 0)
		str = QString("Arp %1").arg(Arp_nb);
	else if (catalogFilters&CatVV && VV_nb > 0)
		str = QString("VV %1").arg(VV_nb);
	else if (catalogFilters&CatPK && !PK_nb.isEmpty())
		str = QString("PK %1").arg(PK_nb);
	else if (catalogFilters&CatPNG && !PNG_nb.isEmpty())
		str = QString("PN G%1").arg(PNG_nb);
	else if (catalogFilters&CatSNRG && !SNRG_nb.isEmpty())
		str = QString("SNR G%1").arg(SNRG_nb);
	else if (catalogFilters&CatACO && !ACO_nb.isEmpty())
		str = QString("ACO %1").arg(ACO_nb);

	return str;
}

void Nebula::readDSO(QDataStream &in)
{
	float	ra, dec;
	unsigned int oType;

	in	>> DSO_nb >> ra >> dec >> bMag >> vMag >> oType >> mTypeString >> majorAxisSize >> minorAxisSize
		>> orientationAngle >> redshift >> redshiftErr >> parallax >> parallaxErr >> oDistance >> oDistanceErr
		>> NGC_nb >> IC_nb >> M_nb >> C_nb >> B_nb >> Sh2_nb >> VdB_nb >> RCW_nb >> LDN_nb >> LBN_nb >> Cr_nb
		>> Mel_nb >> PGC_nb >> UGC_nb >> Ced_nb >> Arp_nb >> VV_nb >> PK_nb >> PNG_nb >> SNRG_nb >> ACO_nb;

	int f = NGC_nb + IC_nb + M_nb + C_nb + B_nb + Sh2_nb + VdB_nb + RCW_nb + LDN_nb + LBN_nb + Cr_nb + Mel_nb + PGC_nb + UGC_nb + Arp_nb + VV_nb;
	if (f==0 && Ced_nb.isEmpty() && PK_nb.isEmpty() && PNG_nb.isEmpty() && SNRG_nb.isEmpty() && ACO_nb.isEmpty())
		withoutID = true;

	StelUtils::spheToRect(ra,dec,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);
	nType = (Nebula::NebulaType)oType;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(Q_NULLPTR)));
}

bool Nebula::objectInDisplayedType() const
{
	if (!flagUseTypeFilters)
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
		case NebPossQSO:
		case NebBLL:
		case NebBLA:
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
		case NebMolCld:
		case NebYSO:
			cntype = 6; // Dark Nebulae
			break;
		case NebPn:
		case NebPossPN:
		case NebPPN:
			cntype = 7; // Planetary Nebulae
			break;
		case NebSNR:
		case NebSNC:
		case NebSNRC:
			cntype = 8; // Supernova Remnants
			break;
		case NebCn:
			cntype = 9;
			break;
		case NebGxCl:
			cntype = 10;
			break;
		default:
			cntype = 11;
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
	else if (typeFilters&TypeGalaxyClusters && cntype==10)
		r = true;
	else if (typeFilters&TypeOther && cntype==11)
		r = true;

	return r;
}

bool Nebula::objectInDisplayedCatalog() const
{
	bool r = false;
	if ((catalogFilters&CatM) && (M_nb>0))
		r = true;
	else if ((catalogFilters&CatC) && (C_nb>0))
		r = true;
	else if ((catalogFilters&CatNGC) && (NGC_nb>0))
		r = true;
	else if ((catalogFilters&CatIC) && (IC_nb>0))
		r = true;
	else if ((catalogFilters&CatB) && (B_nb>0))
		r = true;
	else if ((catalogFilters&CatSh2) && (Sh2_nb>0))
		r = true;
	else if ((catalogFilters&CatVdB) && (VdB_nb>0))
		r = true;
	else if ((catalogFilters&CatRCW) && (RCW_nb>0))
		r = true;
	else if ((catalogFilters&CatLDN) && (LDN_nb>0))
		r = true;
	else if ((catalogFilters&CatLBN) && (LBN_nb>0))
		r = true;
	else if ((catalogFilters&CatCr) && (Cr_nb>0))
		r = true;
	else if ((catalogFilters&CatMel) && (Mel_nb>0))
		r = true;
	else if ((catalogFilters&CatPGC) && (PGC_nb>0))
		r = true;
	else if ((catalogFilters&CatUGC) && (UGC_nb>0))
		r = true;
	else if ((catalogFilters&CatCed) && !(Ced_nb.isEmpty()))
		r = true;
	else if ((catalogFilters&CatArp) && (Arp_nb>0))
		r = true;
	else if ((catalogFilters&CatVV) && (VV_nb>0))
		r = true;
	else if ((catalogFilters&CatPK) && !(PK_nb.isEmpty()))
		r = true;
	else if ((catalogFilters&CatPNG) && !(PNG_nb.isEmpty()))
		r = true;
	else if ((catalogFilters&CatSNRG) && !(SNRG_nb.isEmpty()))
		r = true;
	else if ((catalogFilters&CatACO) && (!ACO_nb.isEmpty()))
		r = true;

	// Special case: objects without ID from current catalogs
	if (withoutID)
		r = true;

	return r;
}

bool Nebula::objectInAllowedSizeRangeLimits(void) const
{
	bool r = true;
	if (flagUseSizeLimits)
	{
		float size = 60.f * qMax(majorAxisSize, minorAxisSize);
		if (size>=minSizeLimit && size<=maxSizeLimit)
			r = true;
		else
			r = false;
	}
	return r;
}

QString Nebula::getMorphologicalTypeString(void) const
{
	return mTypeString;
}

QString Nebula::getMorphologicalTypeDescription(void) const
{
	QString m, r = "";

	// Let's avoid showing a wrong morphological description for galaxies
	// NOTE: Is required the morphological description for galaxies?
	if (nType==NebGx || nType==NebAGx || nType==NebRGx || nType==NebIGx || nType==NebQSO || nType==NebPossQSO || nType==NebBLA || nType==NebBLL || nType==NebGxCl)
		return QString();

	QRegExp GlClRx("\\.*(I|II|III|IV|V|VI|VI|VII|VIII|IX|X|XI|XII)\\.*");
	int idx = GlClRx.indexIn(mTypeString);
	if (idx>0)
		m = mTypeString.mid(idx);
	else
		m = mTypeString;

	QStringList glclass;
	glclass << "I" << "II" << "III" << "IV" << "V" << "VI" << "VII" << "VIII" << "IX" << "X" << "XI" << "XII";

	if (GlClRx.exactMatch(m)) // Globular Clusters
	{
		switch(glclass.indexOf(GlClRx.capturedTexts().at(1).trimmed()))
		{
			case 0:
				r = qc_("high concentration of stars toward the center", "Shapley–Sawyer Concentration Class");
				break;
			case 1:
				r = qc_("dense central concentration of stars", "Shapley–Sawyer Concentration Class");
				break;
			case 2:
				r = qc_("strong inner core of stars", "Shapley–Sawyer Concentration Class");
				break;
			case 3:
				r = qc_("intermediate rich concentrations of stars", "Shapley–Sawyer Concentration Class");
				break;
			case 4:
			case 5:
			case 6:
				r = qc_("intermediate concentrations of stars", "Shapley–Sawyer Concentration Class");
				break;
			case 7:
				r = qc_("rather loosely concentration of stars towards the center", "Shapley–Sawyer Concentration Class");
				break;
			case 8:
				r = qc_("loose concentration of stars towards the center", "Shapley–Sawyer Concentration Class");
				break;
			case 9:
				r = qc_("loose concentration of stars", "Shapley–Sawyer Concentration Class");
				break;
			case 10:
				r = qc_("very loose concentration of stars towards the center", "Shapley–Sawyer Concentration Class");
				break;
			case 11:
				r = qc_("almost no concentration towards the center", "Shapley–Sawyer Concentration Class");
				break;
			default:
				r = qc_("undocumented concentration class", "Shapley–Sawyer Concentration Class");
				break;
		}
	}

	QRegExp OClRx("\\.*(I|II|III|IV)(\\d)(p|m|r)(n*)\\.*");
	idx = OClRx.indexIn(mTypeString);
	if (idx>0)
		m = mTypeString.mid(idx);
	else
		m = mTypeString;

	if (OClRx.exactMatch(m)) // Open Clusters
	{
		QStringList occlass, ocrich, rtxt;
		occlass << "I" << "II" << "III" << "IV";
		ocrich << "p" << "m" << "r";
		switch(occlass.indexOf(OClRx.capturedTexts().at(1).trimmed()))
		{
			case 0:
				rtxt << qc_("strong central concentration of stars", "Trumpler's Concentration Class");
				break;
			case 1:
				rtxt << qc_("little central concentration of stars", "Trumpler's Concentration Class");
				break;
			case 2:
				rtxt << qc_("no noticeable concentration of stars", "Trumpler's Concentration Class");
				break;
			case 3:
				rtxt << qc_("a star field condensation", "Trumpler's Concentration Class");
				break;
			default:
				rtxt << qc_("undocumented concentration class", "Trumpler's Concentration Class");
				break;
		}
		switch(OClRx.capturedTexts().at(2).toInt())
		{
			case 1:
				rtxt << qc_("small brightness range of cluster members", "Trumpler's Brightness Class");
				break;
			case 2:
				rtxt << qc_("medium brightness range of cluster members", "Trumpler's Brightness Class");
				break;
			case 3:
				rtxt << qc_("large brightness range of cluster members", "Trumpler's Brightness Class");
				break;
			default:
				rtxt << qc_("undocumented brightness range of cluster members", "Trumpler's Brightness Class");
				break;
		}
		switch(ocrich.indexOf(OClRx.capturedTexts().at(3).trimmed()))
		{
			case 0:
				rtxt << qc_("poor cluster with less than 50 stars", "Trumpler's Number of Members Class");
				break;
			case 1:
				rtxt << qc_("moderately rich cluster with 50-100 stars", "Trumpler's Number of Members Class");
				break;
			case 2:
				rtxt << qc_("rich cluster with more than 100 stars", "Trumpler's Number of Members Class");
				break;
			default:
				rtxt << qc_("undocumented number of members class", "Trumpler's Number of Members Class");
				break;
		}
		if (!OClRx.capturedTexts().at(4).trimmed().isEmpty())
			rtxt << qc_("the cluster lies within nebulosity", "nebulosity factor of open clusters");

		r = rtxt.join(",<br />");
	}

	QRegExp VdBRx("\\.*(I|II|I-II|II P|P),\\s+(VBR|VB|BR|M|F|VF|:)\\.*");
	idx = VdBRx.indexIn(mTypeString);
	if (idx>0)
		m = mTypeString.mid(idx);
	else
		m = mTypeString;

	if (VdBRx.exactMatch(m)) // Reflection Nebulae
	{
		QStringList rnclass, rnbrightness, rtx;
		rnclass << "I" << "II" << "I-II" << "II P" << "P";
		rnbrightness << "VBR" << "VB" << "BR" << "M" << "F" << "VF" << ":";
		switch(rnbrightness.indexOf(VdBRx.capturedTexts().at(2).trimmed()))
		{
			case 0:
			case 1:
				rtx << qc_("very bright", "Reflection Nebulae Brightness");
				break;
			case 2:
				rtx << qc_("bright", "Reflection Nebulae Brightness");
				break;
			case 3:
				rtx << qc_("moderate brightness", "Reflection Nebulae Brightness");
				break;
			case 4:
				rtx << qc_("faint", "Reflection Nebulae Brightness");
				break;
			case 5:
				rtx << qc_("very faint", "Reflection Nebulae Brightness");
				break;
			case 6:
				rtx << qc_("uncertain brightness", "Reflection Nebulae Brightness");
				break;
			default:
				rtx << qc_("undocumented brightness of reflection nebulae", "Reflection Nebulae Brightness");
				break;
		}
		switch(rnclass.indexOf(VdBRx.capturedTexts().at(1).trimmed()))
		{
			case 0:
				rtx << qc_("the illuminating star is embedded in the nebulosity", "Reflection Nebulae Classification");
				break;
			case 1:
				rtx << qc_("star is located outside the illuminated nebulosity", "Reflection Nebulae Classification");
				break;
			case 2:
				rtx << qc_("star is located on the corner of the illuminated nebulosity", "Reflection Nebulae Classification");
				break;
			case 3:
			{
				// TRANSLATORS: peculiar: odd or unusual, cf. peculiar star, peculiar galaxy
				rtx << qc_("star is located outside the illuminated peculiar nebulosity", "Reflection Nebulae Classification");
				break;
			}
			case 4:
			{
				// TRANSLATORS: peculiar: odd or unusual, cf. peculiar star, peculiar galaxy
				rtx << qc_("the illuminated peculiar nebulosity", "Reflection Nebulae Classification");
				break;
			}
			default:
				rtx << qc_("undocumented reflection nebulae", "Reflection Nebulae Classification");
				break;
		}
		r = rtx.join(",<br />");
	}


	QRegExp HIIRx("\\.*(\\d+),\\s+(\\d+),\\s+(\\d+)\\.*");
	idx = HIIRx.indexIn(mTypeString);
	if (idx>0)
		m = mTypeString.mid(idx);
	else
		m = mTypeString;

	if (HIIRx.exactMatch(m)) // HII regions
	{
		int form	= HIIRx.capturedTexts().at(1).toInt();
		int structure	= HIIRx.capturedTexts().at(2).toInt();
		int brightness	= HIIRx.capturedTexts().at(3).toInt();
		QStringList morph;
		switch(form)
		{
			case 1:
				morph << q_("circular form");
				break;
			case 2:
				morph << q_("elliptical form");
				break;
			case 3:
				morph << q_("irregular form");
				break;
			default:
				morph << q_("undocumented form");
				break;
		}
		switch(structure)
		{
			case 1:
				morph << q_("amorphous structure");
				break;
			case 2:
				morph << q_("conventional structure");
				break;
			case 3:
				morph << q_("filamentary structure");
				break;
			default:
				morph << q_("undocumented structure");
				break;
		}
		switch(brightness)
		{
			case 1:
				morph << qc_("faintest", "HII region brightness");
				break;
			case 2:
				morph << qc_("moderate brightness", "HII region brightness");
				break;
			case 3:
				morph << qc_("brightest", "HII region brightness");
				break;
			default:
				morph << q_("undocumented brightness");
				break;
		}
		r = morph.join(",<br />");
	}

	if (nType==NebSNR)
	{
		QString delim = "";
		if (!r.isEmpty())
			delim = ";<br />";

		if (mTypeString.contains("S") && !mTypeString.contains("S?"))
			r = qc_("remnant shows a shell radio structure", "supernova remnant structure classification") + delim + r;

		if (mTypeString.contains("F") && !mTypeString.contains("F?"))
			r = qc_("remnant shows a filled center ('plerion') radio structure", "supernova remnant structure classification") + delim + r;

		if (mTypeString.contains("C") && !mTypeString.contains("C?"))
			r = qc_("remnant shows a composite (or combination) radio structure", "supernova remnant structure classification") + delim + r;

		if (mTypeString.contains("S?"))
			r = qc_("remnant shows a shell radio structure with some uncertainty", "supernova remnant structure classification") + delim + r;

		if (mTypeString.contains("F?"))
			r = qc_("remnant shows a filled center ('plerion') radio structure with some uncertainty", "supernova remnant structure classification") + delim + r;

		if (mTypeString.contains("C?"))
			r = qc_("remnant shows a composite (or combination) radio structure with some uncertainty", "supernova remnant structure classification") + delim + r;
	}

	return r;
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
			wsType = q_("globular star cluster");
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
		case NebSNC:
			wsType = q_("supernova candidate");
			break;
		case NebSNRC:
			wsType = q_("supernova remnant candidate");
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
		case NebBLL:
			wsType = q_("BL Lac object");
			break;
		case NebBLA:
			wsType = q_("blazar");
			break;
		case NebMolCld:
			wsType = q_("molecular cloud");
			break;
		case NebYSO:
			wsType = q_("young stellar object");
			break;
		case NebPossQSO:
			wsType = q_("possible quasar");
			break;
		case NebPossPN:
			wsType = q_("possible planetary nebula");
			break;
		case NebPPN:
			wsType = q_("protoplanetary nebula");
			break;
		case NebStar:
			wsType = q_("star");
			break;
		case NebSymbioticStar:
			wsType = q_("symbiotic star");
			break;
		case NebEmissionLineStar:
			wsType = q_("emission-line star");
			break;
		case NebGxCl:
			wsType = q_("cluster of galaxies");
			break;
		case NebUnknown:
			wsType = q_("object of unknown nature");
			break;
		default:
			wsType = q_("undocumented type");
			break;
	}
	return wsType;
}
