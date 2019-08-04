/*
 * Copyright (C) 2017 Alexander Wolf
 * Copyright (C) 2017 Teresa Huertas Roldán
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

#include "NomenclatureItem.hpp"
#include "NomenclatureMgr.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelProjector.hpp"
#include "StelUtils.hpp"

const QString NomenclatureItem::NOMENCLATURE_TYPE = QStringLiteral("NomenclatureItem");
Vec3f NomenclatureItem::color = Vec3f(0.1f,1.0f,0.1f);
bool NomenclatureItem::hideLocalNomenclature = false;

NomenclatureItem::NomenclatureItem(PlanetP nPlanet,
				   int nId,
				   const QString& nName,
				   const QString& nContext,
				   NomenclatureItemType nItemType,
				   float nLatitude,
				   float nLongitude,
				   float nSize)
	: XYZ(0.0)
	, jde(0.0)
	, planet(nPlanet)
	, identificator(nId)
	, englishName(nName)
	, context(nContext)
	, nameI18n(nName)
	, nType(nItemType)
	, latitude(nLatitude)
	, longitude(nLongitude)
	, size(nSize)
{
	StelUtils::spheToRect((longitude /*+ planet->getAxisRotation()*/) * M_PI/180.0, latitude * M_PI/180.0, XYZpc);
}

NomenclatureItem::~NomenclatureItem()
{
}

QString NomenclatureItem::getNomenclatureTypeString() const
{
	QString nTypeStr = "";
	switch (nType)
	{
		case niAlbedoFeature:
			// TRANSLATORS: Geographic area distinguished by amount of reflected light
			nTypeStr = qc_("albedo feature", "landform");
			break;
		case niArcus:
			// TRANSLATORS: Arc-shaped feature
			nTypeStr = qc_("arcus", "landform");
			break;
		case niAstrum:
			// TRANSLATORS: Radial-patterned features on Venus
			nTypeStr = qc_("astrum", "landform");
			break;
		case niCatena:
			// TRANSLATORS: Chain of craters
			nTypeStr = qc_("catena", "landform");
			break;
		case niCavus:
			// TRANSLATORS: Hollows, irregular steep-sided depressions usually in arrays or clusters
			nTypeStr = qc_("cavus", "landform");
			break;
		case niChaos:
			// TRANSLATORS: Distinctive area of broken terrain
			nTypeStr = qc_("chaos", "landform");
			break;
		case niChasma:
			// TRANSLATORS: A deep, elongated, steep-sided depression
			nTypeStr = qc_("chasma", "landform");
			break;
		case niCollis:
			// TRANSLATORS: Small hills or knobs
			nTypeStr = qc_("collis", "landform");
			break;
		case niCorona:
			// TRANSLATORS: Ovoid-shaped feature
			nTypeStr = qc_("corona", "landform");
			break;
		case niCrater:
			// TRANSLATORS: A circular depression
			nTypeStr = qc_("crater", "landform");
			break;
		case niDorsum:
			// TRANSLATORS: Ridge
			nTypeStr = qc_("dorsum", "landform");
			break;
		case niEruptiveCenter:
			// TRANSLATORS: Active volcanic centers on Io
			nTypeStr = qc_("eruptive center", "landform");
			break;
		case niFacula:
			// TRANSLATORS: Bright spot
			nTypeStr = qc_("facula", "landform");
			break;
		case niFarrum:
			// TRANSLATORS: Pancake-like structure, or a row of such structures
			nTypeStr = qc_("farrum", "landform");
			break;
		case niFlexus:
			// TRANSLATORS: A very low curvilinear ridge with a scalloped pattern
			nTypeStr = qc_("flexus", "landform");
			break;
		case niFluctus:
			// TRANSLATORS: Flow terrain
			nTypeStr = qc_("fluctus", "landform");
			break;
		case niFlumen:
			// TRANSLATORS: Channel on Titan that might carry liquid
			nTypeStr = qc_("flumen", "landform");
			break;
		case niFretum:
			// TRANSLATORS: Strait, a narrow passage of liquid connecting two larger areas of liquid
			nTypeStr = qc_("fretum", "landform");
			break;
		case niFossa:
			// TRANSLATORS: Long, narrow depression
			nTypeStr = qc_("fossa", "landform");
			break;
		case niInsula:
			// TRANSLATORS: Island (islands), an isolated land area (or group of such areas) surrounded by, or nearly surrounded by, a liquid area (sea or lake)
			nTypeStr = qc_("insula", "landform");
			break;
		case niLabes:
			// TRANSLATORS: Landslide
			nTypeStr = qc_("labes", "landform");
			break;
		case niLabyrinthus:
			// TRANSLATORS: Complex of intersecting valleys or ridges
			nTypeStr = qc_("labyrinthus", "landform");
			break;
		case niLacuna:
			// TRANSLATORS: Irregularly shaped depression on Titan having the appearance of a dry lake bed
			nTypeStr = qc_("lacuna", "landform");
			break;
		case niLacus:
			// TRANSLATORS: "Lake" or small plain; on Titan, a "lake" or small, dark plain with discrete, sharp boundaries
			nTypeStr = qc_("lacus", "landform");
			break;
		case niLargeRingedFeature:
			// TRANSLATORS: Cryptic ringed feature
			nTypeStr = qc_("large ringed feature", "landform");
			break;
		case niLenticula:
			// TRANSLATORS: Small dark spots on Europa
			nTypeStr = qc_("lenticula", "landform");
			break;
		case niLinea:
			// TRANSLATORS: A dark or bright elongate marking, may be curved or straight
			nTypeStr = qc_("linea", "landform");
			break;
		case niLingula:
			// TRANSLATORS: Extension of plateau having rounded lobate or tongue-like boundaries
			nTypeStr = qc_("lingula", "landform");
			break;
		case niMacula:
			// TRANSLATORS: Dark spot, may be irregular
			nTypeStr = qc_("macula", "landform");
			break;
		case niMare:
			// TRANSLATORS: "Sea"; on the Moon, low albedo, relatively smooth plain, generally of large extent; on Mars, dark albedo areas of no known geological significance; on Titan, large expanses of dark materials thought to be liquid hydrocarbons
			nTypeStr = qc_("mare", "landform");
			break;
		case niMensa:
			// TRANSLATORS: A flat-topped prominence with cliff-like edges
			nTypeStr = qc_("mensa", "landform");
			break;
		case niMons:
			// TRANSLATORS: Mountain
			nTypeStr = qc_("mons", "landform");
			break;
		case niOceanus:
			// TRANSLATORS: A very large dark area on the Moon
			nTypeStr = qc_("oceanus", "landform");
			break;
		case niPalus:
			// TRANSLATORS: "Swamp"; small plain
			nTypeStr = qc_("palus", "landform");
			break;
		case niPatera:
			// TRANSLATORS: An irregular crater, or a complex one with scalloped edges
			nTypeStr = qc_("patera", "landform");
			break;
		case niPlanitia:
			// TRANSLATORS: Low plain
			nTypeStr = qc_("planitia", "landform");
			break;
		case niPlanum:
			// TRANSLATORS: Plateau or high plain
			nTypeStr = qc_("planum", "landform");
			break;
		case niPlume:
			// TRANSLATORS: Cryo-volcanic features on Triton
			nTypeStr = qc_("plume", "landform");
			break;
		case niPromontorium:
			// TRANSLATORS: "Cape"; headland promontoria
			nTypeStr = qc_("promontorium", "landform");
			break;
		case niRegio:
			// TRANSLATORS: A large area marked by reflectivity or color distinctions from adjacent areas, or a broad geographic region
			nTypeStr = qc_("regio", "landform");
			break;
		case niReticulum:
			// TRANSLATORS: Reticular (netlike) pattern on Venus
			nTypeStr = qc_("reticulum", "landform");
			break;
		case niRima:
			// TRANSLATORS: Fissure
			nTypeStr = qc_("rima", "landform");
			break;
		case niRupes:
			// TRANSLATORS: Scarp
			nTypeStr = qc_("rupes", "landform");
			break;
		case niSatelliteFeature:
			// TRANSLATORS: A feature that shares the name of an associated feature.
			nTypeStr = qc_("satellite feature", "landform");
			break;
		case niScopulus:
			// TRANSLATORS: Lobate or irregular scarp
			nTypeStr = qc_("scopulus", "landform");
			break;
		case niSerpens:
			// TRANSLATORS: Sinuous feature with segments of positive and negative relief along its length
			nTypeStr = qc_("serpens", "landform");
			break;
		case niSulcus:
			// TRANSLATORS: Subparallel furrows and ridges
			nTypeStr = qc_("sulcus", "landform");
			break;
		case niSinus:
			// TRANSLATORS: "Bay"; small plain; on Titan, bays within seas or lakes of liquid hydrocarbons
			nTypeStr = qc_("sinus", "landform");
			break;
		case niTerra:
			// TRANSLATORS: Extensive land mass
			nTypeStr = qc_("terra", "landform");
			break;
		case niTessera:
			// TRANSLATORS: Tile-like, polygonal terrain
			nTypeStr = qc_("tessera", "landform");
			break;
		case niTholus:
			// TRANSLATORS: Small domical mountain or hill
			nTypeStr = qc_("tholus", "landform");
			break;
		case niUnda:
			// TRANSLATORS: Dunes
			nTypeStr = qc_("unda", "landform");
			break;
		case niVallis:
			// TRANSLATORS: Valley
			nTypeStr = qc_("vallis", "landform");
			break;
		case niVastitas:
			// TRANSLATORS: Extensive plain
			nTypeStr = qc_("vastitas", "landform");
			break;
		case niVirga:
			// TRANSLATORS: A streak or stripe of color
			nTypeStr = qc_("virga", "landform");
			break;
		case niLandingSite:
			// TRANSLATORS: Lunar features at or near Apollo landing sites
			nTypeStr = qc_("landing site name", "landform");
			break;
		case niUNDEFINED:
		default:
			nTypeStr = qc_("undocumented landform type", "landform");
			break;
	}
	return nTypeStr;
}

QString NomenclatureItem::getNomenclatureTypeLatinString() const
{
	QString nTypeStr = "";
	switch (nType)
	{
		case niArcus:
			nTypeStr = "arcus";
			break;
		case niAstrum:
			nTypeStr = "astrum";
			break;
		case niCatena:
			nTypeStr = "catena";
			break;
		case niCavus:
			nTypeStr = "cavus";
			break;
		case niChaos:
			nTypeStr = "chaos";
			break;
		case niChasma:
			nTypeStr = "chasma";
			break;
		case niCollis:
			nTypeStr = "collis";
			break;
		case niCorona:
			nTypeStr = "corona";
			break;
		case niCrater:
			nTypeStr = "crater";
			break;
		case niDorsum:
			nTypeStr = "dorsum";
			break;
		case niFacula:
			nTypeStr = "facula";
			break;
		case niFarrum:
			nTypeStr = "farrum";
			break;
		case niFlexus:
			nTypeStr = "flexus";
			break;
		case niFluctus:
			nTypeStr = "fluctus";
			break;
		case niFlumen:
			nTypeStr = "flumen";
			break;
		case niFretum:
			nTypeStr = "fretum";
			break;
		case niFossa:
			nTypeStr = "fossa";
			break;
		case niInsula:
			nTypeStr = "insula";
			break;
		case niLabes:
			nTypeStr = "labes";
			break;
		case niLabyrinthus:
			nTypeStr = "labyrinthus";
			break;
		case niLacuna:
			nTypeStr = "lacuna";
			break;
		case niLacus:
			nTypeStr = "lacus";
			break;
		case niLenticula:
			nTypeStr = "lenticula";
			break;
		case niLinea:
			nTypeStr = "linea";
			break;
		case niLingula:
			nTypeStr = "lingula";
			break;
		case niMacula:
			nTypeStr = "macula";
			break;
		case niMare:
			nTypeStr = "mare";
			break;
		case niMensa:
			nTypeStr = "mensa";
			break;
		case niMons:
			nTypeStr = "mons";
			break;
		case niOceanus:
			nTypeStr = "oceanus";
			break;
		case niPalus:
			nTypeStr = "palus";
			break;
		case niPatera:
			nTypeStr = "patera";
			break;
		case niPlanitia:
			nTypeStr = "planitia";
			break;
		case niPlanum:
			nTypeStr = "planum";
			break;
		case niPlume:
			nTypeStr = "plume";
			break;
		case niPromontorium:
			nTypeStr = "promontorium";
			break;
		case niRegio:
			nTypeStr = "regio";
			break;
		case niReticulum:
			nTypeStr = "reticulum";
			break;
		case niRima:
			nTypeStr = "rima";
			break;
		case niRupes:
			nTypeStr = "rupes";
			break;
		case niScopulus:
			nTypeStr = "scopulus";
			break;
		case niSerpens:
			nTypeStr = "serpens";
			break;
		case niSulcus:
			nTypeStr = "sulcus";
			break;
		case niSinus:
			nTypeStr = "sinus";
			break;
		case niTerra:
			nTypeStr = "terra";
			break;
		case niTessera:
			nTypeStr = "tessera";
			break;
		case niTholus:
			nTypeStr = "tholus";
			break;
		case niUnda:
			nTypeStr = "unda";
			break;
		case niVallis:
			nTypeStr = "vallis";
			break;
		case niVastitas:
			nTypeStr = "vastitas";
			break;
		case niVirga:
			nTypeStr = "virga";
			break;
		default:
			nTypeStr = "";
			break;
	}
	return nTypeStr;
}

QString NomenclatureItem::getNomenclatureTypeDescription() const
{
	QString nTypeStr = "";
	switch (nType)
	{
		case niAlbedoFeature:
			// TRANSLATORS: Description for landform 'albedo feature'
			nTypeStr = q_("Geographic area distinguished by amount of reflected light.");
			break;
		case niArcus:
			// TRANSLATORS: Description for landform 'arcus'
			nTypeStr = q_("Arc-shaped feature.");
			break;
		case niAstrum:
			// TRANSLATORS: Description for landform 'astrum'
			nTypeStr = q_("Radial-patterned feature.");
			break;
		case niCatena:
			// TRANSLATORS: Description for landform 'catena'
			nTypeStr = q_("Chain of craters.");
			break;
		case niCavus:
			// TRANSLATORS: Description for landform 'cavus'
			nTypeStr = q_("Hollows, irregular steep-sided depressions usually in arrays or clusters.");
			break;
		case niChaos:
			// TRANSLATORS: Description for landform 'chaos'
			nTypeStr = q_("Distinctive area of broken terrain.");
			break;
		case niChasma:
			// TRANSLATORS: Description for landform 'chasma'
			nTypeStr = q_("A deep, elongated, steep-sided depression.");
			break;
		case niCollis:
			// TRANSLATORS: Description for landform 'collis'
			nTypeStr = q_("Small hills or knobs.");
			break;
		case niCorona:
			// TRANSLATORS: Description for landform 'corona'
			nTypeStr = q_("Ovoid-shaped feature.");
			break;
		case niCrater:
			// TRANSLATORS: Description for landform 'crater'
			nTypeStr = q_("A circular depression.");
			break;
		case niDorsum:
			// TRANSLATORS: Description for landform 'dorsum'
			nTypeStr = q_("Ridge.");
			break;
		case niEruptiveCenter:
			// TRANSLATORS: Description for landform 'eruptive center'
			nTypeStr = q_("Active volcanic center.");
			break;
		case niFacula:
			// TRANSLATORS: Description for landform 'facula'
			nTypeStr = q_("Bright spot.");
			break;
		case niFarrum:
			// TRANSLATORS: Description for landform 'farrum'
			nTypeStr = q_("Pancake-like structure, or a row of such structures.");
			break;
		case niFlexus:
			// TRANSLATORS: Description for landform 'flexus'
			nTypeStr = q_("A very low curvilinear ridge with a scalloped pattern.");
			break;
		case niFluctus:
			// TRANSLATORS: Description for landform 'fluctus'
			nTypeStr = q_("Flow terrain.");
			break;
		case niFlumen:
			// TRANSLATORS: Description for landform 'flumen'
			nTypeStr = q_("Channel, that might carry liquid.");
			break;
		case niFretum:
			// TRANSLATORS: Description for landform 'fretum'
			nTypeStr = q_("Strait, a narrow passage of liquid connecting two larger areas of liquid.");
			break;
		case niFossa:
			// TRANSLATORS: Description for landform 'fossa'
			nTypeStr = q_("Long, narrow depression.");
			break;
		case niInsula:
			// TRANSLATORS: Description for landform 'insula'
			nTypeStr = q_("Island, an isolated land area surrounded by, or nearly surrounded by, a liquid area (sea or lake).");
			break;
		case niLabes:
			// TRANSLATORS: Description for landform 'labes'
			nTypeStr = q_("Landslide.");
			break;
		case niLabyrinthus:
			// TRANSLATORS: Description for landform 'labyrinthus'
			nTypeStr = q_("Complex of intersecting valleys or ridges.");
			break;
		case niLacuna:
			// TRANSLATORS: Description for landform 'lacuna'
			nTypeStr = q_("Irregularly shaped depression, having the appearance of a dry lake bed.");
			break;
		case niLacus:
			if (planet->getEnglishName()=="Titan")
			{
				// TRANSLATORS: Description for landform 'lacus' on Titan
				nTypeStr = q_("'Lake' or small, dark plain with discrete, sharp boundaries.");
			}
			else
			{
				// TRANSLATORS: Description for landform 'lacus'
				nTypeStr = q_("'Lake' or small plain.");
			}
			break;
		case niLargeRingedFeature:
			// TRANSLATORS: Description for landform 'large ringed feature'
			nTypeStr = q_("Cryptic ringed feature.");
			break;
		case niLenticula:
			// TRANSLATORS: Description for landform 'lenticula'
			nTypeStr = q_("Small dark spot.");
			break;
		case niLinea:
			// TRANSLATORS: Description for landform 'linea'
			nTypeStr = q_("A dark or bright elongate marking, may be curved or straight.");
			break;
		case niLingula:
			// TRANSLATORS: Description for landform 'lingula'
			nTypeStr = q_("Extension of plateau having rounded lobate or tongue-like boundaries.");
			break;
		case niMacula:
			// TRANSLATORS: Description for landform 'macula'
			nTypeStr = q_("Dark spot, may be irregular");
			break;
		case niMare:
		{
			QString p = planet->getEnglishName();
			if (p=="Moon")
			{
				// TRANSLATORS: Description for landform 'mare' on the Moon
				nTypeStr = q_("'Sea'; low albedo, relatively smooth plain, generally of large extent.");
			}
			if (p=="Mars")
			{
				// TRANSLATORS: Description for landform 'mare' on Mars
				nTypeStr = q_("'Sea'; dark albedo areas of no known geological significance.");
			}
			if (p=="Titan")
			{
				// TRANSLATORS: Description for landform 'mare' on Titan
				nTypeStr = q_("'Sea'; large expanses of dark materials thought to be liquid hydrocarbons");
			}
			break;
		}
		case niMensa:
			// TRANSLATORS: Description for landform 'mensa'
			nTypeStr = q_("A flat-topped prominence with cliff-like edges.");
			break;
		case niMons:
			// TRANSLATORS: Description for landform 'mons'
			nTypeStr = q_("Mountain.");
			break;
		case niOceanus:
			// TRANSLATORS: Description for landform 'oceanus'
			nTypeStr = q_("A very large dark area.");
			break;
		case niPalus:
			// TRANSLATORS: Description for landform 'palus'
			nTypeStr = q_("'Swamp'; small plain.");
			break;
		case niPatera:
			// TRANSLATORS: Description for landform 'patera'
			nTypeStr = q_("An irregular crater, or a complex one with scalloped edges.");
			break;
		case niPlanitia:
			// TRANSLATORS: Description for landform 'planitia'
			nTypeStr = q_("Low plain.");
			break;
		case niPlanum:
			// TRANSLATORS: Description for landform 'planum'
			nTypeStr = q_("Plateau or high plain.");
			break;
		case niPlume:
			// TRANSLATORS: Description for landform 'plume'
			nTypeStr = q_("Cryo-volcanic feature.");
			break;
		case niPromontorium:
			// TRANSLATORS: Description for landform 'promontorium'
			nTypeStr = q_("'Cape'; headland promontoria.");
			break;
		case niRegio:
			// TRANSLATORS: Description for landform 'regio'
			nTypeStr = q_("A large area marked by reflectivity or color distinctions from adjacent areas, or a broad geographic region.");
			break;
		case niReticulum:
			// TRANSLATORS: Description for landform 'reticulum'
			nTypeStr = q_("Reticular (netlike) pattern.");
			break;
		case niRima:
			// TRANSLATORS: Description for landform 'rima'
			nTypeStr = q_("Fissure.");
			break;
		case niRupes:
			// TRANSLATORS: Description for landform 'rupes'
			nTypeStr = q_("Scarp.");
			break;
		case niSatelliteFeature:
			// TRANSLATORS: Description for landform 'satellite feature'
			nTypeStr = q_("A feature that shares the name of an associated feature.");
			break;
		case niScopulus:
			// TRANSLATORS: Description for landform 'scopulus'
			nTypeStr = q_("Lobate or irregular scarp.");
			break;
		case niSerpens:
			// TRANSLATORS: Description for landform 'serpens'
			nTypeStr = q_("Sinuous feature with segments of positive and negative relief along its length.");
			break;
		case niSulcus:
			// TRANSLATORS: Description for landform 'sulcus'
			nTypeStr = q_("Subparallel furrows and ridges.");
			break;
		case niSinus:
			if (planet->getEnglishName()=="Titan")
			{
				// TRANSLATORS: Description for landform 'sinus' on Titan
				nTypeStr = q_("'Bay'; bays within seas or lakes of liquid hydrocarbons.");
			}
			else
			{
				// TRANSLATORS: Description for landform 'sinus'
				nTypeStr = q_("'Bay'; small plain.");
			}
			break;
		case niTerra:
			// TRANSLATORS: Description for landform 'terra'
			nTypeStr = q_("Extensive land mass.");
			break;
		case niTessera:
			// TRANSLATORS: Description for landform 'tessera'
			nTypeStr = q_("Tile-like, polygonal terrain.");
			break;
		case niTholus:
			// TRANSLATORS: Description for landform 'tholus'
			nTypeStr = q_("Small domical mountain or hill.");
			break;
		case niUnda:
			// TRANSLATORS: Description for landform 'unda'
			nTypeStr = q_("Dunes.");
			break;
		case niVallis:
			// TRANSLATORS: Description for landform 'vallis'
			nTypeStr = q_("Valley.");
			break;
		case niVastitas:
			// TRANSLATORS: Description for landform 'vastitas'
			nTypeStr = q_("Extensive plain.");
			break;
		case niVirga:
			// TRANSLATORS: Description for landform 'virga'
			nTypeStr = q_("A streak or stripe of color.");
			break;
		case niLandingSite:			
			nTypeStr = "";
			break;
		case niUNDEFINED:
		default:
			nTypeStr = q_("Undocumented landform type.");
			break;
	}
	return nTypeStr;
}

float NomenclatureItem::getSelectPriority(const StelCore* core) const
{
	if (planet->getVMagnitude(core)>=20.f)
	{
		// The planet is too faint for view (in the deep shadow for example), so let's disable select the nomenclature
		return StelObject::getSelectPriority(core)+25.f;
	}
	else if (getFlagLabels() && (getAngularSize(core)*M_PI/180.*core->getProjection(StelCore::FrameJ2000)->getPixelPerRadAtCenter()>=25.))
	{
		// The item may be good selectable when it over 25px size only
		return StelObject::getSelectPriority(core)-25.f;
	}
	else
		return StelObject::getSelectPriority(core)-2.f;
}

QString NomenclatureItem::getNameI18n() const
{
	if (getNomenclatureType()==niCrater)
		return QString("%1 (%2)").arg(nameI18n, getNomenclatureTypeString());
	else
		return nameI18n;
}

QString NomenclatureItem::getEnglishName() const
{
	if (getNomenclatureType()==niCrater)
		return QString("%1 (%2)").arg(englishName, getNomenclatureTypeLatinString());
	else
		return englishName;
}

void NomenclatureItem::translateName(const StelTranslator& trans)
{
	nameI18n = trans.qtranslate(englishName, context);
}

QString NomenclatureItem::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>" << nameI18n;
		// englishName here is the original scientific term, usually latin but could be plain english like "landing site".
		if (nameI18n!=englishName)
			oss << " (" << englishName << ")";
		oss << "</h2>";
	}

	if (flags&ObjectType && getNomenclatureType()!=NomenclatureItem::niUNDEFINED)
	{
		QString tstr  = getNomenclatureTypeString();
		QString latin = getNomenclatureTypeLatinString(); // not always latin!
		QString ts    = q_("Type");
		if (tstr!=latin && !latin.isEmpty())
			oss << QString("%1: <b>%2</b> (%3: %4)").arg(ts).arg(tstr).arg(q_("geologic term")).arg(latin) << "<br />";
		else
			oss << QString("%1: <b>%2</b>").arg(ts).arg(tstr) << "<br />";
	}

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Size && size>0.f)
	{
		QString sz = q_("Linear size");
		// Satellite Features are almost(?) exclusively lettered craters, and all are on the Moon. Assume craters.
		if ((getNomenclatureType()==NomenclatureItem::niCrater) || (getNomenclatureType()==NomenclatureItem::niSatelliteFeature))
			sz = q_("Diameter");
		oss << QString("%1: %2 %3").arg(sz).arg(QString::number(size, 'f', 2)).arg(qc_("km", "distance")) << "<br />";
	}

	if (flags&Extra)
	{
		oss << QString("%1: %2/%3").arg(q_("Planetographic long./lat.")).arg(StelUtils::decDegToDmsStr(longitude)).arg(StelUtils::decDegToDmsStr(latitude)) << "<br />";
		oss << QString("%1: %2").arg(q_("Celestial body")).arg(planet->getNameI18n()) << "<br />";
		QString description = getNomenclatureTypeDescription();
		if (getNomenclatureType()!=NomenclatureItem::niUNDEFINED && !description.isEmpty())
			oss << QString("%1: %2").arg(q_("Landform description"), description) << "<br />";
	}

	postProcessInfoString(str, flags);
	return str;
}

Vec3f NomenclatureItem::getInfoColor(void) const
{
	return color;
}

Vec3d NomenclatureItem::getJ2000EquatorialPos(const StelCore* core) const
{
	if (jde == core->getJDE()) return XYZ;
	jde = core->getJDE();
	const Vec3d equPos = planet->getJ2000EquatorialPos(core);
	// Calculate the radius of the planet. It is necessary to re-scale it
	const double r = planet->getEquatorialRadius() * planet->getSphereScale();

	Vec3d XYZ0;
//	// For now, assume spherical planets, simply scale by radius.
	XYZ0 = XYZpc*r;
	// TODO1: handle ellipsoid bodies
	// TODO2: intersect properly with OBJ bodies! (LP:1723742)

	/* We have to calculate feature's coordinates in VSOP87 (this is Ecliptic J2000 coordinates).
	   Feature's original coordinates are in planetocentric system, so we have to multiply it by the rotation matrix.
	   planet->getRotEquatorialToVsop87() gives us the rotation matrix between Equatorial (on date) coordinates and Ecliptic J2000 coordinates.
	   So we have to make another change to obtain the rotation matrix using Equatorial J2000: we have to multiplay by core->matVsop87ToJ2000 */
	// TODO: Maybe it is more efficient to add some getRotEquatorialToVsop87Zrotation() to the Planet class which returns a Mat4d computed in Planet::computeTransMatrix().
	XYZ = equPos + (core->matVsop87ToJ2000 * planet->getRotEquatorialToVsop87()) * Mat4d::zrotation(planet->getAxisRotation()* M_PI/180.0) * XYZ0;
	return XYZ;
}

float NomenclatureItem::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core);
	return 99.f;
}

double NomenclatureItem::getAngularSize(const StelCore* core) const
{
	return std::atan2(size*planet->getSphereScale()/AU, getJ2000EquatorialPos(core).length()) * 180./M_PI;
}

void NomenclatureItem::update(double deltaTime)
{
	labelsFader.update(static_cast<int>(deltaTime*1000));
}

void NomenclatureItem::draw(StelCore* core, StelPainter *painter)
{
	if (!getFlagLabels())
		return;

	const Vec3d equPos = planet->getJ2000EquatorialPos(core);
	Vec3d XYZ = getJ2000EquatorialPos(core);

	// In case we are located at a labeled site, don't show this label or any labels within 150 km. Else we have bad flicker...
	if (XYZ.lengthSquared() < 150.*150.*AU_KM*AU_KM )
		return;

	if (getFlagHideLocalNomenclature())
	{
		// Check the state when needed only!
		if (planet==core->getCurrentPlanet())
			return;
	}

	double screenSize = getAngularSize(core)*M_PI/180.*painter->getProjector()->getPixelPerRadAtCenter();

	// We can use ratio of angular size to the FOV to checking visibility of features also!
	// double scale = getAngularSize(core)/painter->getProjector()->getFov();
	// if (painter->getProjector()->projectCheck(XYZ, srcPos) && (dist >= XYZ.length()) && (scale>0.04 && scale<0.5))

	// check visibility of feature
	Vec3d srcPos;
	if (painter->getProjector()->projectCheck(XYZ, srcPos) && (equPos.length() >= XYZ.length()) && (screenSize>50. && screenSize<750.))
	{
		painter->setColor(color[0], color[1], color[2], 1.0);
		painter->drawCircle(srcPos[0], srcPos[1], 2.f);
		painter->drawText(srcPos[0], srcPos[1], nameI18n, 0, 5.f, 5.f, false);
	}
}
