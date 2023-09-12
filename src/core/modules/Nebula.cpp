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
#include "Planet.hpp"
#include "StelTexture.hpp"

#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "RefractionExtinction.hpp"

#include <QTextStream>
#include <QFile>
#include <QString>
#include <QRegularExpression>

#include <QDebug>
#include <QBuffer>

const QString Nebula::NEBULA_TYPE = QStringLiteral("Nebula");

StelTextureSP Nebula::texRegion;
StelTextureSP Nebula::texPointElement;
StelTextureSP Nebula::texPlanetaryNebula;
bool  Nebula::drawHintProportional = false;
bool  Nebula::surfaceBrightnessUsage = false;
bool  Nebula::designationUsage = false;
float Nebula::hintsBrightness = 0.f;
Vec3f Nebula::labelColor = Vec3f(0.4f,0.3f,0.5f);
QMap<Nebula::NebulaType, Vec3f>Nebula::hintColorMap;
bool Nebula::flagUseTypeFilters = false;
Nebula::CatalogGroup Nebula::catalogFilters = Nebula::CatalogGroup(Nebula::CatNone);
Nebula::TypeGroup Nebula::typeFilters = Nebula::TypeGroup(Nebula::TypeAll);
bool Nebula::flagUseArcsecSurfaceBrightness = false;
bool Nebula::flagUseShortNotationSurfaceBrightness = true;
bool Nebula::flagUseOutlines = false;
bool Nebula::flagShowAdditionalNames = true;
bool Nebula::flagUseSizeLimits = false;
double Nebula::minSizeLimit = 1.0;
double Nebula::maxSizeLimit = 600.0;

const QMap<Nebula::NebulaType, QString> Nebula::typeEnglishStringMap = // Maps type to english name.
{
	{ NebGx     , N_("galaxy") },
	{ NebAGx    , N_("active galaxy") },
	{ NebRGx    , N_("radio galaxy") },
	{ NebIGx    , N_("interacting galaxy") },
	{ NebQSO    , N_("quasar") },
	{ NebCl     , N_("star cluster") },
	{ NebOc     , N_("open star cluster") },
	{ NebGc     , N_("globular star cluster") },
	{ NebSA     , N_("stellar association") },
	{ NebSC     , N_("star cloud") },
	{ NebN      , N_("nebula") },
	{ NebPn     , N_("planetary nebula") },
	{ NebDn     , N_("dark nebula") },
	{ NebRn     , N_("reflection nebula") },
	{ NebBn     , N_("bipolar nebula") },
	{ NebEn     , N_("emission nebula") },
	{ NebCn     , N_("cluster associated with nebulosity") },
	{ NebHII    , N_("HII region") },
	{ NebSNR    , N_("supernova remnant") },
	{ NebISM    , N_("interstellar matter") },
	{ NebEMO    , N_("emission object") },
	{ NebBLL    , N_("BL Lac object") },
	{ NebBLA    , N_("blazar") },
	{ NebMolCld , N_("molecular cloud") },
	{ NebYSO    , N_("young stellar object") },
	{ NebPossQSO, N_("possible quasar") },
	{ NebPossPN , N_("possible planetary nebula") },
	{ NebPPN    , N_("protoplanetary nebula") },
	{ NebStar   , N_("star") },
	{ NebSymbioticStar   , N_("symbiotic star") },
	{ NebEmissionLineStar, N_("emission-line star") },
	{ NebSNC    , N_("supernova candidate") },
	{ NebSNRC   , N_("supernova remnant candidate") },
	{ NebGxCl   , N_("cluster of galaxies") },
	{ NebPartOfGx, N_("part of a galaxy") },
	{ NebRegion , N_("region of the sky") },
	{ NebUnknown, N_("object of unknown nature") }
};

Nebula::Nebula()
	: StelObject()
	, DSO_nb(0)
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
	, DWB_nb(0)
	, Tr_nb(0)
	, St_nb(0)
	, Ru_nb(0)
	, VdBHa_nb(0)
	, Ced_nb("")
	, PK_nb("")
	, PNG_nb("")
	, SNRG_nb("")
	, ACO_nb("")
	, HCG_nb("")
	, ESO_nb("")
	, VdBH_nb("")
	, withoutID(false)
	, nameI18("")
	, discoverer("")
	, discoveryYear("")
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
	designations.clear();
}

Nebula::~Nebula()
{
}

QString Nebula::getMagnitudeInfoString(const StelCore *core, const InfoStringGroup& flags, const int decimals) const
{
	QString res;
	const float mmag = qMin(vMag, bMag);
	if (mmag < 50.f && flags&Magnitude)
	{
		QString emag = "";
		QString fsys = "";
		bool bmag = false;
		float mag = getVMagnitude(core);
		float mage = getVMagnitudeWithExtinction(core);
		bool hasAtmosphere = core->getSkyDrawer()->getFlagHasAtmosphere();
		QString tmag = q_("Magnitude");
		if (nType == NebDn || B_nb>0) // Dark nebulae or objects from Barnard catalog
			tmag = q_("Opacity");

		if (bMag < 50.f && vMag > 50.f)
		{
			fsys = QString("(%1 B").arg(q_("photometric passband"));
			if (hasAtmosphere)
				fsys.append(";");
			else
				fsys.append(")");
			mag = getBMagnitude(core);
			mage = getBMagnitudeWithExtinction(core);
			bmag = true;
		}

		const float airmass = getAirmass(core);
		if (nType != NebDn && B_nb==0 && airmass>-1.f) // Don't show extincted magnitude much below horizon where model is meaningless.
		{
			emag = QString("%1 <b>%2</b> %3 <b>%4</b> %5)").arg(q_("reduced to"), QString::number(mage, 'f', decimals), q_("by"), QString::number(airmass, 'f', 2), q_("Airmasses"));
			if (!bmag)
				emag = QString("(%1").arg(emag);
		}
		res = QString("%1: <b>%2</b> %3 %4<br />").arg(tmag, QString::number(mag, 'f', decimals), fsys, emag);
	}
	res += getExtraInfoStrings(Magnitude).join("");
	return res;
}

QString Nebula::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

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
		if (!nameI18.isEmpty() && !withoutID && flags&Name)
			oss << "<br>";

		oss << designations.join(" - ");
	}

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "</h2>";

	if (flags&Name)
	{
		QStringList extraNames=getExtraInfoStrings(Name);
		if (!extraNames.isEmpty())
			oss << q_("Additional names: ") << extraNames.join(", ") << "<br/>";
	}
	if (flags&CatalogNumber)
	{
		QStringList extraCat=getExtraInfoStrings(CatalogNumber);
		if (!extraCat.isEmpty())
			oss << q_("Additional catalog numbers: ") << extraCat.join(", ") << "<br/>";
	}

	if (flags&ObjectType)
	{
		QString mt = getMorphologicalTypeString();
		if (mt.isEmpty())
			oss << QString("%1: <b>%2</b>").arg(q_("Type"), getObjectTypeI18n()) << "<br>";
		else
			oss << QString("%1: <b>%2</b> (%3)").arg(q_("Type"), getObjectTypeI18n(), mt) << "<br>";
		oss << getExtraInfoStrings(ObjectType).join("");
	}

	oss << getMagnitudeInfoString(core, flags, 2);

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
			mu = QString("<sup>m</sup>/□'");
			if (flagUseArcsecSurfaceBrightness)
				mu = QString("<sup>m</sup>/□\"");
		}
		else
		{
			mu = QString("%1/%2<sup>2</sup>").arg(qc_("mag", "magnitude"), q_("arc-min"));
			if (flagUseArcsecSurfaceBrightness)
				mu = QString("%1/%2<sup>2</sup>").arg(qc_("mag", "magnitude"), q_("arc-sec"));
		}

		if (getSurfaceBrightness(core)<99.f)
		{
			if (getAirmass(core)>-1.f && getSurfaceBrightnessWithExtinction(core)<99.f) // Don't show extincted surface brightness much below horizon where model is meaningless.
			{
				oss << QString("%1: <b>%2</b> %5 (%3: <b>%4</b> %5)").arg(sb, QString::number(getSurfaceBrightness(core, flagUseArcsecSurfaceBrightness), 'f', 2),
											  ae, QString::number(getSurfaceBrightnessWithExtinction(core, flagUseArcsecSurfaceBrightness), 'f', 2), mu) << "<br />";
			}
			else
				oss << QString("%1: <b>%2</b> %3").arg(sb, QString::number(getSurfaceBrightness(core, flagUseArcsecSurfaceBrightness), 'f', 2), mu) << "<br />";

			if (getContrastIndex(core)<99.f)
				oss << QString("%1: %2").arg(q_("Contrast index"), QString::number(getContrastIndex(core), 'f', 2)) << "<br />";
		}
	}

	oss << getCommonInfoString(core, flags);

	if (flags&Size && majorAxisSize>0.f)
	{
		QString majorAxS, minorAxS, sizeAx = q_("Size");
		if (withDecimalDegree)
		{
			majorAxS = StelUtils::radToDecDegStr(static_cast<double>(majorAxisSize)*M_PI/180., 5, false, true);
			minorAxS = StelUtils::radToDecDegStr(static_cast<double>(minorAxisSize)*M_PI/180., 5, false, true);
		}
		else
		{
			majorAxS = StelUtils::radToDmsPStr(static_cast<double>(majorAxisSize)*M_PI/180., 2);
			minorAxS = StelUtils::radToDmsPStr(static_cast<double>(minorAxisSize)*M_PI/180., 2);
		}

		if (fuzzyEquals(majorAxisSize, minorAxisSize) || minorAxisSize==0.f)
			oss << QString("%1: %2").arg(sizeAx, majorAxS) << "<br />";
		else
		{
			oss << QString("%1: %2 x %3").arg(sizeAx, majorAxS, minorAxS) << "<br />";
			if (orientationAngle>0)
				oss << QString("%1: %2%3").arg(q_("Orientation angle")).arg(orientationAngle).arg(QChar(0x00B0)) << "<br />";
		}
	}
	if (flags&Size)
		oss << getExtraInfoStrings(Size).join("");

	if (flags&Distance)
	{
		float distance, distanceErr, distanceLY, distanceErrLY;
		if (qAbs(parallax)>0.f)
		{
			QString dx;
			// distance in light years from parallax
			distance = 3.162e-5f/(qAbs(parallax)*4.848e-9f);
			distanceErr = 0.f;

			if (parallaxErr>0.f)
				distanceErr = qAbs(3.162e-5f/(qAbs(parallaxErr + parallax)*4.848e-9f) - distance);

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

			distance = oDistance;
			distanceErr = oDistanceErr;
			distanceLY = oDistance*dc;
			distanceErrLY= oDistanceErr*dc;
			if (oDistance>=1000.f)
			{
				distance = oDistance/1000.f;
				distanceErr = oDistanceErr/1000.f;
				//TRANSLATORS: Unit of measure for distance - Megaparsecs
				dupc = qc_("Mpc", "distance");
			}

			if (distanceLY>=1e6f)
			{
				distanceLY /= 1e6f;
				distanceErrLY /= 1e6f;
				ms = 3;
				//TRANSLATORS: Unit of measure for distance - Millions of Light Years
				duly = qc_("M ly", "distance");
			}

			if (oDistanceErr>0.f)
			{
				dx = QString("%1%2%3").arg(QString::number(distance, 'f', 3)).arg(QChar(0x00B1)).arg(QString::number(distanceErr, 'f', 3));
				dy = QString("%1%2%3").arg(QString::number(distanceLY, 'f', ms)).arg(QChar(0x00B1)).arg(QString::number(distanceErrLY, 'f', ms));
			}
			else
			{
				dx = QString("%1").arg(QString::number(distance, 'f', 3));
				dy = QString("%1").arg(QString::number(distanceLY, 'f', ms));
			}

			oss << QString("%1: %2 %3 (%4 %5)").arg(q_("Distance"), dx, dupc, dy, duly) << "<br />";
		}
		oss << getExtraInfoStrings(Distance).join("");
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
		if (qAbs(parallax)>0.f)
		{
			QString px;
			if (parallaxErr>0.f)
				px = QString("%1%2%3").arg(QString::number(qAbs(parallax), 'f', 3)).arg(QChar(0x00B1)).arg(QString::number(parallaxErr, 'f', 3));
			else
				px = QString("%1").arg(QString::number(qAbs(parallax), 'f', 3));

			oss << QString("%1: %2 %3").arg(q_("Parallax"), px, qc_("mas", "parallax")) << "<br />";
		}
		if (!discoverer.isEmpty())
			oss << QString("%1: %2 (%3)").arg(q_("Discoverer"), discoverer, StelUtils::localeDiscoveryDateString(discoveryYear)) << "<br />";
		if (!getMorphologicalTypeDescription().isEmpty())
			oss << QString("%1: %2.").arg(q_("Morphological description"), getMorphologicalTypeDescription()) << "<br />";
	}

	oss << getSolarLunarInfoString(core, flags);

	postProcessInfoString(str, flags);

	return str;
}

QVariantMap Nebula::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	map["type"]=getObjectTypeI18n(); // replace "Nebula" type by detail. This is localized.
	map.insert("morpho", getMorphologicalTypeString());
	map.insert("surface-brightness", getSurfaceBrightness(core));
	map.insert("designations", withoutID ? QString() : designations.join(" - "));
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
		if (asize>2) // Special case for many AKA
		{
			bool firstLine = true;
			for(int i=1; i<=asize; i++)
			{
				aliases.append(englishAliases.at(i-1));
				if (i<asize)
					aliases.append(" - ");

				if ((i % 2)==0 && firstLine) // 2 AKA-items on first line!
				{
					aliases.append("<br />");
					firstLine = false;
				}
				if (i>3 && ((i-2) % 4)==0 && !firstLine &&  i<asize)
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
		if (asize>2) // Special case for many AKA; NOTE: Should we add size to the config data for skyculture?
		{
			bool firstLine = true;
			for(int i=1; i<=asize; i++)
			{
				aliases.append(nameI18Aliases.at(i-1));
				if (i<asize)
					aliases.append(" - ");

				if ((i % 2)==0 && firstLine) // 2 AKA-items on first line!
				{
					aliases.append("<br />");
					firstLine = false;
				}
				if (i>3 && ((i-2) % 4)==0 && !firstLine &&  i<asize)
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
	Q_UNUSED(core)
	return vMag;
}

float Nebula::getBMagnitude(const StelCore* core) const
{
	Q_UNUSED(core)
	return bMag;
}

float Nebula::getBMagnitudeWithExtinction(const StelCore* core) const
{
	Vec3d altAzPos = getAltAzPosGeometric(core);
	altAzPos.normalize();
	float mag = getBMagnitude(core);
	// without the test, planets flicker stupidly in fullsky atmosphere-less view.
	if (core->getSkyDrawer()->getFlagHasAtmosphere())
		core->getSkyDrawer()->getExtinction().forward(altAzPos, &mag);
	return mag;
}

double Nebula::getAngularRadius(const StelCore *) const
{
	return static_cast<double>(0.5f*majorAxisSize);
}

float Nebula::getSelectPriority(const StelCore* core) const
{
	float selectPriority = StelObject::getSelectPriority(core);
	const NebulaMgr* nebMgr = (static_cast<NebulaMgr*>(StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")));
	// minimize unwanted selection of the deep-sky objects
	if (!nebMgr->getFlagHints())
		return selectPriority+3.f;

	float mag = qMin(getVMagnitude(core), getBMagnitude(core));
	float lim = mag;
	float mLim = 15.0f;

	if (nType==NebRegion) // special case for regions
		mag = 3.f;

	if (!objectInDisplayedCatalog() || !objectInDisplayedType())
		return selectPriority+mLim;

	const StelSkyDrawer* drawer = core->getSkyDrawer();

	if (drawer->getFlagNebulaMagnitudeLimit() && (mag>static_cast<float>(drawer->getCustomNebulaMagnitudeLimit())))
		return selectPriority+mLim;
	
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

	if (std::min(mLim, lim)<=maxMagHint || !outlineSegments.empty() || nType==NebRegion) // High priority for big DSO (with outlines) or regions
		selectPriority = -10.f;
	else
		selectPriority -= 5.f;

	return selectPriority;
}

Vec3f Nebula::getInfoColor(void) const
{
	return (static_cast<NebulaMgr*>(StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")))->getLabelsColor();
}

double Nebula::getCloseViewFov(const StelCore*) const
{
	return majorAxisSize>0.f ? static_cast<double>(majorAxisSize) * 4. : 1.;
}

float Nebula::getSurfaceBrightness(const StelCore* core, bool arcsec) const
{
	const float sq = (arcsec ? 3600.f*3600.f : 3600.f); // arcsec^2 or arcmin^2
	const float mag = qMin(getVMagnitude(core), getBMagnitude(core));
	if (mag<99.f && majorAxisSize>0.f && nType!=NebDn)
		return mag + 2.5f*log10f(getSurfaceArea()*sq);
	else
		return 99.f;
}

float Nebula::getSurfaceBrightnessWithExtinction(const StelCore* core, bool arcsec) const
{
	const float sq = (arcsec ? 3600.f*3600.f : 3600.f); // arcsec^2 or arcmin^2
	const float mag = qMin(getVMagnitudeWithExtinction(core), getBMagnitudeWithExtinction(core));
	if (mag<99.f && majorAxisSize>0.f && nType!=NebDn)
		return mag + 2.5f*log10f(getSurfaceArea()*sq);
	else
		return 99.f;
}

float Nebula::getContrastIndex(const StelCore* core) const
{
	// Compute an extended object's contrast index: http://www.unihedron.com/projects/darksky/NELM2BCalc.html

	// Sky brightness
	const auto luminance = core->getSkyDrawer()->getLightPollutionLuminance();
	const float B_mpsas = StelCore::luminanceToMPSAS(luminance);
	// Compute an extended object's contrast index
	// Source: Clark, R.N., 1990. Appendix E in Visual Astronomy of the Deep Sky, Cambridge University Press and Sky Publishing.
	// URL: http://www.clarkvision.com/visastro/appendix-e.html
	const float emag = getSurfaceBrightnessWithExtinction(core, true);
	if (emag<99.f)
		return -0.4f * (emag - B_mpsas);
	else
		return 99.f;
}

float Nebula::getSurfaceArea(void) const
{
	if (minorAxisSize==0.f)
		return M_PIf*(majorAxisSize/2.f)*(majorAxisSize/2.f); // S = pi*R^2 = pi*(D/2)^2
	else
		return M_PIf*(majorAxisSize/2.f)*(minorAxisSize/2.f); // S = pi*a*b
}

Vec3f Nebula::getHintColor(Nebula::NebulaType nType)
{
	return hintColorMap.value(nType, hintColorMap.value(NebUnknown));
}

float Nebula::getVisibilityLevelByMagnitude(void) const
{
	StelCore* core = StelApp::getInstance().getCore();

	const float mLim = 15.0f;
	float lim = qMin(vMag, bMag);

	if (surfaceBrightnessUsage)
	{
		lim = getSurfaceBrightness(core) - 3.f;
		if (lim > 90.f) lim = mLim + 1.f;		
	}
	else
	{
		if (lim > 90.f) lim = mLim;

		// Dark nebulae. Not sure how to assess visibility from opacity? --GZ
		if (nType==NebDn)
		{
			const float mag = getVMagnitude(core);
			// GZ: ad-hoc visibility formula: assuming good visibility if objects of mag9 are visible, "usual" opacity 5 and size 30', better visibility (discernability) comes with higher opacity and larger size,
			// 9-(opac-5)-2*(angularSize-0.5)
			// GZ Not good for non-Barnards. weak opacity and large surface are antagonists. (some LDN are huge, but opacity 2 is not much to discern).
			// The qMin() maximized the visibility gain for large objects.
			if (majorAxisSize>0.f && mag<90.f)
				lim = mLim - mag - 2.0f*qMin(majorAxisSize, 1.5f);
			else
				lim = (B_nb>0 ? 9.0f : 12.0f); // GZ I assume LDN objects are rather elusive.
		}
		else if (nType==NebHII) // NebHII={Sharpless, LBN, RCW} but also M42.
		{
			// artificially increase visibility of (most) Sharpless and LBN objects. No magnitude recorded:-(
			lim=qMin(lim, 10.0f);
		}
	}

	if (nType==NebRegion) // special case for regions
		lim=3.0f;

	return lim;
}

void Nebula::drawOutlines(StelPainter &sPainter, float maxMagHints) const
{
	size_t segments = outlineSegments.size();
	Vec3f color = getHintColor(nType);

	// tune limits for outlines
	float oLim = getVisibilityLevelByMagnitude() - 3.f;

	float lum = 1.f;
	Vec3f col(color*lum*hintsBrightness);
	if (!objectInDisplayedType())
		col.set(0.f,0.f,0.f);
	sPainter.setColor(col, 1);

	StelCore *core=StelApp::getInstance().getCore();
	Vec3d vel=core->getCurrentPlanet()->getHeliocentricEclipticVelocity();
	vel=StelCore::matVsop87ToJ2000*vel;
	vel*=core->getAberrationFactor() * (AU/(86400.0*SPEED_OF_LIGHT));

	// Show outlines
	if (segments>0 && flagUseOutlines && oLim<=maxMagHints)
	{
		unsigned int i, j;
		std::vector<Vec3d> *points;

		sPainter.setBlending(true);
		sPainter.setLineSmooth(true);
		const SphericalCap& viewportHalfspace = sPainter.getProjector()->getBoundingCap();

		for (i=0;i<segments;i++)
		{
			points = outlineSegments[i];

			for (j=0;j<points->size()-1;j++)
			{
				Vec3d point1=points->at(j);
				Vec3d point2=points->at(j+1);
				if (core->getUseAberration())
				{
					point1+=vel;
					point1.normalize();
					point2+=vel;
					point2.normalize();
				}
				sPainter.drawGreatCircleArc(point1, point2, &viewportHalfspace);
			}
		}
		sPainter.setLineSmooth(false);
	}
}

void Nebula::renderDarkNebulaMarker(StelPainter& sPainter, const float x, const float y,
									float size, const Vec3f color) const
{
	// Take into account device pixel density and global scale ratio, as we are drawing 2D stuff.
	const auto pixelRatio = sPainter.getProjector()->getDevicePixelsPerPixel();
	const auto scale = pixelRatio * StelApp::getInstance().getGlobalScalingRatio();
	size *= scale;

	const float roundRadius = 0.35 * size;
	const int numPointsInArc = std::lround(std::clamp(5*size/35, 5.f, 16.f));
	std::vector<float> vertexData;
	vertexData.reserve(numPointsInArc*2*4);
	const float leftOuterX = x - size;
	const float leftInnerX = leftOuterX + roundRadius;
	const float bottomOuterY = y - size;
	const float bottomInnerY = bottomOuterY + roundRadius;
	const float rightOuterX = x + size;
	const float rightInnerX = rightOuterX - roundRadius;
	const float topOuterY = y + size;
	const float topInnerY = topOuterY - roundRadius;
	const float gap = 0.15*size;
	const float*const cossin = StelUtils::ComputeCosSinRhoZone((M_PIf/2)/(numPointsInArc-1),
															   numPointsInArc-1, 0);
	sPainter.setLineSmooth(true);
	sPainter.setLineWidth(scale * std::clamp(2*size/35, 1.f, 2.5f));
	sPainter.setColor(color);
	sPainter.enableClientStates(true);

	vertexData.clear();
	vertexData.push_back(x-gap);
	vertexData.push_back(bottomOuterY);
	for(int n = 0; n < numPointsInArc; ++n)
	{
		const auto cosa = cossin[2*n], sina = cossin[2*n+1];
		vertexData.push_back(leftInnerX   - roundRadius*sina);
		vertexData.push_back(bottomInnerY - roundRadius*cosa);
	}
	vertexData.push_back(leftOuterX);
	vertexData.push_back(y-gap);
	sPainter.setVertexPointer(2, GL_FLOAT, vertexData.data());
	sPainter.drawFromArray(StelPainter::LineStrip, vertexData.size() / 2, 0, false);

	vertexData.clear();
	vertexData.push_back(leftOuterX);
	vertexData.push_back(y+gap);
	for(int n = 0; n < numPointsInArc; ++n)
	{
		const auto cosa = cossin[2*n], sina = cossin[2*n+1];
		vertexData.push_back(leftInnerX - roundRadius*cosa);
		vertexData.push_back(topInnerY  + roundRadius*sina);
	}
	vertexData.push_back(x-gap);
	vertexData.push_back(topOuterY);
	sPainter.setVertexPointer(2, GL_FLOAT, vertexData.data());
	sPainter.drawFromArray(StelPainter::LineStrip, vertexData.size() / 2, 0, false);

	vertexData.clear();
	vertexData.push_back(x+gap);
	vertexData.push_back(topOuterY);
	for(int n = 0; n < numPointsInArc; ++n)
	{
		const auto cosa = cossin[2*n], sina = cossin[2*n+1];
		vertexData.push_back(rightInnerX + roundRadius*sina);
		vertexData.push_back(topInnerY   + roundRadius*cosa);
	}
	vertexData.push_back(rightOuterX);
	vertexData.push_back(y+gap);
	sPainter.setVertexPointer(2, GL_FLOAT, vertexData.data());
	sPainter.drawFromArray(StelPainter::LineStrip, vertexData.size() / 2, 0, false);

	vertexData.clear();
	vertexData.push_back(rightOuterX);
	vertexData.push_back(y-gap);
	for(int n = 0; n < numPointsInArc; ++n)
	{
		const auto cosa = cossin[2*n], sina = cossin[2*n+1];
		vertexData.push_back(rightInnerX  + roundRadius*cosa);
		vertexData.push_back(bottomInnerY - roundRadius*sina);
	}
	vertexData.push_back(x+gap);
	vertexData.push_back(bottomOuterY);
	sPainter.setVertexPointer(2, GL_FLOAT, vertexData.data());
	sPainter.drawFromArray(StelPainter::LineStrip, vertexData.size() / 2, 0, false);

	sPainter.enableClientStates(false);
}

void Nebula::renderMarkerRoundedRect(StelPainter& sPainter, const float x, const float y,
									 float size, const Vec3f color) const
{
	// Take into account device pixel density and global scale ratio, as we are drawing 2D stuff.
	const auto pixelRatio = sPainter.getProjector()->getDevicePixelsPerPixel();
	const auto scale = pixelRatio * StelApp::getInstance().getGlobalScalingRatio();
	size *= scale;

	const float roundRadius = 0.35 * size;
	const int numPointsInArc = std::lround(std::clamp(5*size/35, 5.f, 16.f));
	std::vector<float> vertexData;
	vertexData.reserve(numPointsInArc*2*4);
	const float leftOuterX = x - size;
	const float leftInnerX = leftOuterX + roundRadius;
	const float bottomOuterY = y - size;
	const float bottomInnerY = bottomOuterY + roundRadius;
	const float rightOuterX = x + size;
	const float rightInnerX = rightOuterX - roundRadius;
	const float topOuterY = y + size;
	const float topInnerY = topOuterY - roundRadius;
	const float*const cossin = StelUtils::ComputeCosSinRhoZone((M_PIf/2)/(numPointsInArc-1),
															   numPointsInArc-1, 0);
	for(int n = 0; n < numPointsInArc; ++n)
	{
		const auto cosa = cossin[2*n], sina = cossin[2*n+1];
		vertexData.push_back(leftInnerX   - roundRadius*sina);
		vertexData.push_back(bottomInnerY - roundRadius*cosa);
	}
	for(int n = 0; n < numPointsInArc; ++n)
	{
		const auto cosa = cossin[2*n], sina = cossin[2*n+1];
		vertexData.push_back(leftInnerX - roundRadius*cosa);
		vertexData.push_back(topInnerY  + roundRadius*sina);
	}
	for(int n = 0; n < numPointsInArc; ++n)
	{
		const auto cosa = cossin[2*n], sina = cossin[2*n+1];
		vertexData.push_back(rightInnerX + roundRadius*sina);
		vertexData.push_back(topInnerY   + roundRadius*cosa);
	}
	for(int n = 0; n < numPointsInArc; ++n)
	{
		const auto cosa = cossin[2*n], sina = cossin[2*n+1];
		vertexData.push_back(rightInnerX  + roundRadius*cosa);
		vertexData.push_back(bottomInnerY - roundRadius*sina);
	}
	const auto vertCount = vertexData.size() / 2;
	sPainter.setLineSmooth(true);
	sPainter.setLineWidth(scale * std::clamp(2*size/35, 1.f, 2.5f));
	sPainter.setColor(color);
	sPainter.enableClientStates(true);
	sPainter.setVertexPointer(2, GL_FLOAT, vertexData.data());
	sPainter.drawFromArray(StelPainter::LineLoop, vertCount, 0, false);
	sPainter.enableClientStates(false);
}

void Nebula::renderRoundMarker(StelPainter& sPainter, const float x, const float y,
							   float size, const Vec3f color, const bool crossed) const
{
	// Take into account device pixel density and global scale ratio, as we are drawing 2D stuff.
	const auto pixelRatio = sPainter.getProjector()->getDevicePixelsPerPixel();
	const auto scale = pixelRatio * StelApp::getInstance().getGlobalScalingRatio();
	size *= scale;

	sPainter.setLineSmooth(true);
	sPainter.setLineWidth(scale * std::clamp(size/7, 1.f, 2.5f));
	sPainter.setColor(color);

	sPainter.drawCircle(x, y, size);
	if(!crossed) return;

	sPainter.enableClientStates(true);
	const float vertexData[] = {x-size, y,
								x+size, y,
								x, y-size,
								x, y+size};
	const auto vertCount = std::size(vertexData) / 2;
	sPainter.setVertexPointer(2, GL_FLOAT, vertexData);
	sPainter.drawFromArray(StelPainter::Lines, vertCount, 0, false);
	sPainter.enableClientStates(false);
}

void Nebula::renderEllipticMarker(StelPainter& sPainter, const float x, const float y, float size,
								  const float aspectRatio, const float angle, const Vec3f color) const
{
	// Take into account device pixel density and global scale ratio, as we are drawing 2D stuff.
	const auto pixelRatio = sPainter.getProjector()->getDevicePixelsPerPixel();
	const auto scale = pixelRatio * StelApp::getInstance().getGlobalScalingRatio();
	size *= scale;

	const float radiusY = 0.35 * size;
	const float radiusX = aspectRatio * radiusY;
	const int numPoints = std::lround(std::clamp(size/3, 32.f, 4096.f));
	std::vector<float> vertexData;
	vertexData.reserve(numPoints*2);
	const float*const cossin = StelUtils::ComputeCosSinTheta(numPoints);
	const auto cosa = std::cos(angle);
	const auto sina = std::sin(angle);
	for(int n = 0; n < numPoints; ++n)
	{
		const auto cosb = cossin[2*n], sinb = cossin[2*n+1];
		const auto pointX = radiusX*sinb;
		const auto pointY = radiusY*cosb;
		vertexData.push_back(x + pointX*cosa - pointY*sina);
		vertexData.push_back(y + pointY*cosa + pointX*sina);
	}
	const auto vertCount = vertexData.size() / 2;
	sPainter.setLineSmooth(true);
	sPainter.setLineWidth(scale * std::clamp(size/40, 1.f, 2.f));
	sPainter.setColor(color);
	sPainter.enableClientStates(true);
	sPainter.setVertexPointer(2, GL_FLOAT, vertexData.data());
	sPainter.drawFromArray(StelPainter::LineLoop, vertCount, 0, false);
	sPainter.enableClientStates(false);
}

void Nebula::renderMarkerPointedCircle(StelPainter& sPainter, const float x, const float y,
									   float size, const Vec3f color, const bool insideRect) const
{
	// Take into account device pixel density and global scale ratio, as we are drawing 2D stuff.
	const auto pixelRatio = sPainter.getProjector()->getDevicePixelsPerPixel();
	const auto scale = pixelRatio * StelApp::getInstance().getGlobalScalingRatio();
	size *= scale;

	texPointElement->bind();
	sPainter.setColor(color);
	sPainter.setBlending(true, GL_ONE, GL_ONE);
	const auto numPoints = StelUtils::getSmallerPowerOfTwo(std::clamp(int(0.4f*size), 8, 4096));
	const auto spriteSize = std::min(0.25f * 2*M_PIf*size / numPoints, 5.f);
	if(insideRect)
		size -= spriteSize*2;
	const float*const cossin = StelUtils::ComputeCosSinRhoZone((2*M_PIf)/numPoints, numPoints, 0);
	for(int n = 0; n < numPoints; ++n)
	{
		const auto cosa = cossin[2*n], sina = cossin[2*n+1];
		sPainter.drawSprite2dMode(x - size*sina, y - size*cosa, spriteSize);
	}
}

void Nebula::drawHints(StelPainter& sPainter, float maxMagHints, StelCore *core) const
{
	size_t segments = outlineSegments.size();
	if (segments>0 && flagUseOutlines)
		return;
	Vec3d win;
	// Check visibility of DSO hints
	if (!(sPainter.getProjector()->projectCheck(XYZ, win)))
		return;

	if (getVisibilityLevelByMagnitude()>maxMagHints)
		return;

	Vec3f color = getHintColor(nType);

	const float size = 6.0f;
	float scaledSize = 0.0f;
	if (drawHintProportional)
		scaledSize = static_cast<float>(getAngularRadius(Q_NULLPTR)) *(M_PI_180f*2.f)*static_cast<float>(sPainter.getProjector()->getPixelPerRadAtCenter());
	float finalSize=qMax(size, scaledSize);

	float lum = 1.f;
	Vec3f col(color*lum*hintsBrightness);
	if (!objectInDisplayedType())
		return;

	switch (nType)
	{
		case NebGx:
		case NebIGx:
		case NebAGx:
		case NebQSO:
		case NebPossQSO:
		case NebBLL:
		case NebBLA:
		case NebRGx:
		case NebGxCl:
		{
			// The rotation angle in renderEllipticMarker() is relative to screen. Make sure to compute correct angle from 90+orientationAngle.
			// Find an on-screen direction vector from a point offset somewhat in declination from our object.
			Vec3d XYZrel(getJ2000EquatorialPos(core));
			XYZrel[2]*=0.95; XYZrel.normalize();
			Vec3d XYrel;
			sPainter.getProjector()->project(XYZrel, XYrel);
			const auto screenAngle = atan2(XYrel[1]-XY[1], XYrel[0]-XY[0]);
			renderEllipticMarker(sPainter, XY[0], XY[1], finalSize, 2, screenAngle + orientationAngle*M_PI_180f, col);
			return;
		}
		case NebOc:
		case NebSA:
		case NebSC:
		case NebCl:
			renderMarkerPointedCircle(sPainter, XY[0], XY[1], finalSize, col, false);
			return;
		case NebGc:
			renderRoundMarker(sPainter, XY[0], XY[1], finalSize, col, true);
			return;
		case NebN:
		case NebHII:
		case NebMolCld:
		case NebYSO:
		case NebRn:		
		case NebSNR:
		case NebBn:
		case NebEn:
		case NebSNC:
		case NebSNRC:
			renderMarkerRoundedRect(sPainter, XY[0], XY[1], finalSize, col);
			return;
		case NebPn:
		case NebPossPN:
		case NebPPN:
			Nebula::texPlanetaryNebula->bind();
			break;
		case NebDn:
			renderDarkNebulaMarker(sPainter, XY[0], XY[1], finalSize, col);
			return;
		case NebCn:
		{
			col = getHintColor(NebN)*lum*hintsBrightness;
			renderMarkerRoundedRect(sPainter, XY[0], XY[1], finalSize, col);
			col = getHintColor(NebCl)*lum*hintsBrightness;
			renderMarkerPointedCircle(sPainter, XY[0], XY[1], finalSize, col, true);
			return;
		}
		case NebRegion:
			finalSize = size*2.f;
			Nebula::texRegion->bind();
			break;
		//case NebEMO:
		//case NebStar:
		//case NebSymbioticStar:
		//case NebEmissionLineStar:
		default:
			renderRoundMarker(sPainter, XY[0], XY[1], finalSize, col, false);
			return;
	}

	sPainter.setColor(col, 1);
	sPainter.setBlending(true, GL_ONE, GL_ONE);

	sPainter.drawSprite2dMode(static_cast<float>(XY[0]), static_cast<float>(XY[1]), finalSize);
}

void Nebula::drawLabel(StelPainter& sPainter, float maxMagLabel) const
{
	Vec3d win;
	// Check visibility of DSO labels
	if (!(sPainter.getProjector()->projectCheck(XYZ, win)))
		return;

	if (getVisibilityLevelByMagnitude()>maxMagLabel)
		return;

	sPainter.setColor(labelColor, objectInDisplayedType() ? hintsBrightness : 0.f);

	const float size = static_cast<float>(getAngularRadius(Q_NULLPTR))*(M_PI_180f*2.f)*sPainter.getProjector()->getPixelPerRadAtCenter();
	const float shift = 5.f + (drawHintProportional ? size*0.9f : 0.f);

	QString str = getNameI18n();
	if (str.isEmpty() || designationUsage)
		str = getDSODesignation();

	sPainter.drawText(static_cast<float>(XY[0])+shift, static_cast<float>(XY[1])+shift, str, 0, 0, 0, false);
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
		str = QString("vdB %1").arg(VdB_nb);
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
		str = QString("Abell %1").arg(ACO_nb);
	else if (catalogFilters&CatHCG && !HCG_nb.isEmpty())
		str = QString("HCG %1").arg(HCG_nb);	
	else if (catalogFilters&CatESO && !ESO_nb.isEmpty())
		str = QString("ESO %1").arg(ESO_nb);
	else if (catalogFilters&CatVdBH && !VdBH_nb.isEmpty())
		str = QString("vdBH %1").arg(VdBH_nb);
	else if (catalogFilters&CatDWB && DWB_nb > 0)
		str = QString("DWB %1").arg(DWB_nb);
	else if (catalogFilters&CatTr && Tr_nb > 0)
		str = QString("Tr %1").arg(Tr_nb);
	else if (catalogFilters&CatSt && St_nb > 0)
		str = QString("St %1").arg(St_nb);
	else if (catalogFilters&CatRu && Ru_nb > 0)
		str = QString("Ru %1").arg(Ru_nb);
	else if (catalogFilters&CatVdBHa && VdBHa_nb > 0)
		str = QString("vdB-Ha %1").arg(VdBHa_nb);

	return str;
}

QString Nebula::getDSODesignationWIC() const
{
	if (!withoutID)
		return designations.first();
	else
		return QString();
}


void Nebula::readDSO(QDataStream &in)
{
	float	ra, dec;
	unsigned int oType;

	in	>> DSO_nb >> ra >> dec >> bMag >> vMag >> oType >> mTypeString >> majorAxisSize >> minorAxisSize
		>> orientationAngle >> redshift >> redshiftErr >> parallax >> parallaxErr >> oDistance >> oDistanceErr
		>> NGC_nb >> IC_nb >> M_nb >> C_nb >> B_nb >> Sh2_nb >> VdB_nb >> RCW_nb >> LDN_nb >> LBN_nb >> Cr_nb
		>> Mel_nb >> PGC_nb >> UGC_nb >> Ced_nb >> Arp_nb >> VV_nb >> PK_nb >> PNG_nb >> SNRG_nb >> ACO_nb
		>> HCG_nb >> ESO_nb >> VdBH_nb >> DWB_nb >> Tr_nb >> St_nb >> Ru_nb >> VdBHa_nb;

	const unsigned int f = NGC_nb + IC_nb + M_nb + C_nb + B_nb + Sh2_nb + VdB_nb + RCW_nb + LDN_nb + LBN_nb + Cr_nb + Mel_nb + PGC_nb + UGC_nb + Arp_nb + VV_nb + DWB_nb + Tr_nb + St_nb + Ru_nb + VdBHa_nb;
	if (f==0 && Ced_nb.isEmpty() && PK_nb.isEmpty() && PNG_nb.isEmpty() && SNRG_nb.isEmpty() && ACO_nb.isEmpty() && HCG_nb.isEmpty() && ESO_nb.isEmpty() && VdBH_nb.isEmpty())
		withoutID = true;

	if (M_nb > 0) designations << QString("M %1").arg(M_nb);
	if (C_nb > 0)  designations << QString("C %1").arg(C_nb);
	if (NGC_nb > 0) designations << QString("NGC %1").arg(NGC_nb);
	if (IC_nb > 0) designations << QString("IC %1").arg(IC_nb);
	if (B_nb > 0) designations << QString("B %1").arg(B_nb);
	if (Sh2_nb > 0) designations << QString("SH 2-%1").arg(Sh2_nb);
	if (VdB_nb > 0) designations << QString("vdB %1").arg(VdB_nb);
	if (RCW_nb > 0) designations << QString("RCW %1").arg(RCW_nb);
	if (LDN_nb > 0) designations << QString("LDN %1").arg(LDN_nb);
	if (LBN_nb > 0) designations << QString("LBN %1").arg(LBN_nb);
	if (Cr_nb > 0) designations << QString("Cr %1").arg(Cr_nb);
	if (Mel_nb > 0) designations << QString("Mel %1").arg(Mel_nb);
	if (PGC_nb > 0) designations << QString("PGC %1").arg(PGC_nb);
	if (UGC_nb > 0) designations << QString("UGC %1").arg(UGC_nb);
	if (!Ced_nb.isEmpty()) designations << QString("Ced %1").arg(Ced_nb);
	if (Arp_nb > 0) designations << QString("Arp %1").arg(Arp_nb);
	if (VV_nb > 0) designations << QString("VV %1").arg(VV_nb);
	if (!PK_nb.isEmpty()) designations << QString("PK %1").arg(PK_nb);
	if (!PNG_nb.isEmpty()) designations << QString("PN G%1").arg(PNG_nb);
	if (!SNRG_nb.isEmpty()) designations << QString("SNR G%1").arg(SNRG_nb);
	if (!ACO_nb.isEmpty()) designations << QString("Abell %1").arg(ACO_nb);
	if (!HCG_nb.isEmpty()) designations << QString("HCG %1").arg(HCG_nb);
	if (!ESO_nb.isEmpty()) designations << QString("ESO %1").arg(ESO_nb);
	if (!VdBH_nb.isEmpty()) designations << QString("vdBH %1").arg(VdBH_nb);
	if (DWB_nb > 0) designations << QString("DWB %1").arg(DWB_nb);
	if (Tr_nb > 0) designations << QString("Tr %1").arg(Tr_nb);
	if (St_nb > 0) designations << QString("St %1").arg(St_nb);
	if (Ru_nb > 0) designations << QString("Ru %1").arg(Ru_nb);
	if (VdBHa_nb > 0) designations << QString("vdB-Ha %1").arg(VdBHa_nb);

	StelUtils::spheToRect(ra,dec,XYZ);
	Q_ASSERT(fabs(XYZ.normSquared()-1.)<1e-9);
	nType = static_cast<Nebula::NebulaType>(oType);
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(Q_NULLPTR)));
}

bool Nebula::objectInDisplayedType() const
{
	if (!flagUseTypeFilters)
		return true;

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
		case NebCl:
		case NebSA:
		case NebSC:
			cntype = 3; // Open Star Clusters
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
		case NebGc:
			cntype = 11;
			break;
		default:
			cntype = 12;
			break;
	}
	bool r = ( (typeFilters&TypeGalaxies             && cntype==0)
		|| (typeFilters&TypeActiveGalaxies       && cntype==1)
		|| (typeFilters&TypeInteractingGalaxies  && cntype==2)
		|| (typeFilters&TypeOpenStarClusters     && cntype==3)
		|| (typeFilters&TypeGlobularStarClusters && cntype==11)
		|| (typeFilters&TypeHydrogenRegions      && cntype==4)
		|| (typeFilters&TypeBrightNebulae        && cntype==5)
		|| (typeFilters&TypeDarkNebulae          && cntype==6)
		|| (typeFilters&TypePlanetaryNebulae     && cntype==7)
		|| (typeFilters&TypeSupernovaRemnants    && cntype==8)
		|| ((typeFilters&TypeOpenStarClusters || typeFilters&TypeBrightNebulae || typeFilters&TypeHydrogenRegions) && cntype==9)
		|| (typeFilters&TypeGalaxyClusters       && cntype==10)
		|| (typeFilters&TypeOther                && cntype==12));

	return r;
}

bool Nebula::objectInDisplayedCatalog() const
{
	bool r = ( ((catalogFilters&CatM)     && (M_nb>0))
		|| ((catalogFilters&CatC)     && (C_nb>0))
		|| ((catalogFilters&CatNGC)   && (NGC_nb>0))
		|| ((catalogFilters&CatIC)    && (IC_nb>0))
		|| ((catalogFilters&CatB)     && (B_nb>0))
		|| ((catalogFilters&CatSh2)   && (Sh2_nb>0))
		|| ((catalogFilters&CatVdB)   && (VdB_nb>0))
		|| ((catalogFilters&CatRCW)   && (RCW_nb>0))
		|| ((catalogFilters&CatLDN)   && (LDN_nb>0))
		|| ((catalogFilters&CatLBN)   && (LBN_nb>0))
		|| ((catalogFilters&CatCr)    && (Cr_nb>0))
		|| ((catalogFilters&CatMel)   && (Mel_nb>0))
		|| ((catalogFilters&CatPGC)   && (PGC_nb>0))
		|| ((catalogFilters&CatUGC)   && (UGC_nb>0))
		|| ((catalogFilters&CatCed)   && !(Ced_nb.isEmpty()))
		|| ((catalogFilters&CatArp)   && (Arp_nb>0))
		|| ((catalogFilters&CatVV)    && (VV_nb>0))
		|| ((catalogFilters&CatPK)    && !(PK_nb.isEmpty()))
		|| ((catalogFilters&CatPNG)   && !(PNG_nb.isEmpty()))
		|| ((catalogFilters&CatSNRG)  && !(SNRG_nb.isEmpty()))
		|| ((catalogFilters&CatACO)   && (!ACO_nb.isEmpty()))
		|| ((catalogFilters&CatHCG)   && (!HCG_nb.isEmpty()))		
		|| ((catalogFilters&CatESO)   && (!ESO_nb.isEmpty()))
		|| ((catalogFilters&CatVdBH)  && (!VdBH_nb.isEmpty()))
		|| ((catalogFilters&CatDWB)   && (DWB_nb>0))
		|| ((catalogFilters&CatTr)	   && (Tr_nb>0))
		|| ((catalogFilters&CatSt)	   && (St_nb>0))
		|| ((catalogFilters&CatRu	)   && (Ru_nb>0))
		|| ((catalogFilters&CatVdBHa)   && (VdBHa_nb>0)))

		// Special case: objects without ID from current catalogs
		|| ((catalogFilters&CatOther)   && withoutID);

	return r;
}

bool Nebula::objectInAllowedSizeRangeLimits(void) const
{
	bool r = true;
	if (flagUseSizeLimits)
	{
		const double size = 60. * static_cast<double>(qMax(majorAxisSize, minorAxisSize));
		r = (size>=minSizeLimit && size<=maxSizeLimit);
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

	static const QRegularExpression GlClRx("\\.*(I|II|III|IV|V|VI|VI|VII|VIII|IX|X|XI|XII)\\.*");
	int idx = mTypeString.indexOf(GlClRx);
	if (idx>0)
		m = mTypeString.mid(idx);
	else
		m = mTypeString;

	static const QStringList glclass = {"I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII"};

	QRegularExpressionMatch GlClMatch=GlClRx.match(m);
	if (GlClMatch.hasMatch()) // Globular Clusters
	{
		switch(glclass.indexOf(GlClMatch.captured(1).trimmed()))
		{
			case 0:
				r = qc_("high concentration of stars toward the center", "Shapley-Sawyer Concentration Class");
				break;
			case 1:
				r = qc_("dense central concentration of stars", "Shapley-Sawyer Concentration Class");
				break;
			case 2:
				r = qc_("strong inner core of stars", "Shapley-Sawyer Concentration Class");
				break;
			case 3:
				r = qc_("intermediate rich concentrations of stars", "Shapley-Sawyer Concentration Class");
				break;
			case 4:
			case 5:
			case 6:
				r = qc_("intermediate concentrations of stars", "Shapley-Sawyer Concentration Class");
				break;
			case 7:
				r = qc_("rather loose concentration of stars towards the center", "Shapley-Sawyer Concentration Class");
				break;
			case 8:
				r = qc_("loose concentration of stars towards the center", "Shapley-Sawyer Concentration Class");
				break;
			case 9:
				r = qc_("loose concentration of stars", "Shapley-Sawyer Concentration Class");
				break;
			case 10:
				r = qc_("very loose concentration of stars towards the center", "Shapley-Sawyer Concentration Class");
				break;
			case 11:
				r = qc_("almost no concentration towards the center", "Shapley-Sawyer Concentration Class");
				break;
			default:
				r = qc_("undocumented concentration class", "Shapley-Sawyer Concentration Class");
				break;
		}
	}

	static const QRegularExpression OClRx("\\.*(I|II|III|IV)(\\d)(p|m|r)(n*|N*|u*|U*|e*|E*)\\.*");
	idx = mTypeString.indexOf(OClRx);
	if (idx>0)
		m = mTypeString.mid(idx);
	else
		m = mTypeString;

	QRegularExpressionMatch OClMatch=OClRx.match(m);
	if (OClMatch.hasMatch()) // Open Clusters
	{
		QStringList rtxt;
		static const QStringList occlass = { "I", "II", "III", "IV"};
		static const QStringList ocrich = { "p", "m", "r"};
		switch(occlass.indexOf(OClMatch.captured(1).trimmed()))
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
		switch(OClMatch.captured(2).toInt())
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
		switch(ocrich.indexOf(OClMatch.captured(3).trimmed()))
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
		if (!OClMatch.captured(4).trimmed().isEmpty())
			rtxt << qc_("the cluster lies within nebulosity", "nebulosity factor of open clusters");

		r = rtxt.join(",<br />");
	}

	static const QRegularExpression VdBRx("\\.*(I|II|I-II|II P|P),\\s+(VBR|VB|BR|M|F|VF|:)\\.*");
	idx = mTypeString.indexOf(VdBRx);
	if (idx>0)
		m = mTypeString.mid(idx);
	else
		m = mTypeString;

	QRegularExpressionMatch VdBMatch=VdBRx.match(m);
	if (VdBMatch.hasMatch()) // Reflection Nebulae
	{
		QStringList rtx;
		static const QStringList rnclass = { "I", "II", "I-II", "II P", "P"};
		static const QStringList rnbrightness = { "VBR", "VB", "BR", "M", "F", "VF", ":"};
		switch(rnbrightness.indexOf(VdBMatch.captured(2).trimmed()))
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
		switch(rnclass.indexOf(VdBMatch.captured(1).trimmed()))
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


	static const QRegularExpression HIIRx("\\.*(\\d+),\\s+(\\d+),\\s+(\\d+)\\.*");
	idx = mTypeString.indexOf(HIIRx);
	if (idx>0)
		m = mTypeString.mid(idx);
	else
		m = mTypeString;

	QRegularExpressionMatch HIIMatch=HIIRx.match(m);
	if (HIIMatch.hasMatch()) // HII regions
	{
		const int form	     = HIIMatch.captured(1).toInt();
		const int structure  = HIIMatch.captured(2).toInt();
		const int brightness = HIIMatch.captured(3).toInt();
		const QStringList formList={
			q_("circular form"),
			q_("elliptical form"),
			q_("irregular form")};
		const QStringList structureList={
			q_("amorphous structure"),
			q_("conventional structure"),
			q_("filamentary structure")};
		const QStringList brightnessList={
			qc_("faintest", "HII region brightness"),
			qc_("moderate brightness", "HII region brightness"),
			qc_("brightest", "HII region brightness")};

		QStringList morph;
		morph << formList.value(form-1, q_("undocumented form"));
		morph << structureList.value(structure-1, q_("undocumented structure"));
		morph << brightnessList.value(brightness-1, q_("undocumented brightness"));

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

QString Nebula::getTypeStringI18n(Nebula::NebulaType nType)
{
	return q_(typeEnglishStringMap.value(nType, q_("undocumented type")));
}

Vec3d Nebula::getJ2000EquatorialPos(const StelCore* core) const
{
	if ((core) && (core->getUseAberration()) && (core->getCurrentPlanet()))
	{
		Vec3d pos=XYZ;
		Q_ASSERT_X(fabs(pos.normSquared()-1.0)<0.0001, "Nebula aberration", "vertex length not unity");
		//pos.normalize(); // Yay - not required!
		Vec3d vel=core->getCurrentPlanet()->getHeliocentricEclipticVelocity();
		vel=StelCore::matVsop87ToJ2000*vel*core->getAberrationFactor()*(AU/(86400.0*SPEED_OF_LIGHT));
		pos+=vel;
		pos.normalize();
		return pos;
	}
	else
	{
		return XYZ;
	}
}
