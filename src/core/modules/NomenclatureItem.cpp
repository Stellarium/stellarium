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
	StelUtils::spheToRect((static_cast<double>(longitude) /*+ planet->getAxisRotation()*/) * M_PI/180.0, static_cast<double>(latitude) * M_PI/180.0, XYZpc);
}

NomenclatureItem::~NomenclatureItem()
{
}

QString NomenclatureItem::getNomenclatureTypeString(NomenclatureItemType nType)
{
	switch (nType)
	{
		case niAlbedoFeature:
			// TRANSLATORS: Geographic area distinguished by amount of reflected light
			return qc_("albedo feature", "landform");
		case niArcus:
			// TRANSLATORS: Arc-shaped feature
			return qc_("arcus", "landform");
		case niAstrum:
			// TRANSLATORS: Radial-patterned features on Venus
			return qc_("astrum", "landform");
		case niCatena:
			// TRANSLATORS: Chain of craters
			return qc_("catena", "landform");
		case niCavus:
			// TRANSLATORS: Hollows, irregular steep-sided depressions usually in arrays or clusters
			return qc_("cavus", "landform");
		case niChaos:
			// TRANSLATORS: Distinctive area of broken terrain
			return qc_("chaos", "landform");
		case niChasma:
			// TRANSLATORS: A deep, elongated, steep-sided depression
			return qc_("chasma", "landform");
		case niCollis:
			// TRANSLATORS: Small hills or knobs
			return qc_("collis", "landform");
		case niCorona:
			// TRANSLATORS: Ovoid-shaped feature
			return qc_("corona", "landform");
		case niCrater:
			// TRANSLATORS: A circular depression
			return qc_("crater", "landform");
		case niDorsum:
			// TRANSLATORS: Ridge
			return qc_("dorsum", "landform");
		case niEruptiveCenter:
			// TRANSLATORS: Active volcanic centers on Io
			return qc_("eruptive center", "landform");
		case niFacula:
			// TRANSLATORS: Bright spot
			return qc_("facula", "landform");
		case niFarrum:
			// TRANSLATORS: Pancake-like structure, or a row of such structures
			return qc_("farrum", "landform");
		case niFlexus:
			// TRANSLATORS: A very low curvilinear ridge with a scalloped pattern
			return qc_("flexus", "landform");
		case niFluctus:
			// TRANSLATORS: Flow terrain
			return qc_("fluctus", "landform");
		case niFlumen:
			// TRANSLATORS: Channel on Titan that might carry liquid
			return qc_("flumen", "landform");
		case niFretum:
			// TRANSLATORS: Strait, a narrow passage of liquid connecting two larger areas of liquid
			return qc_("fretum", "landform");
		case niFossa:
			// TRANSLATORS: Long, narrow depression
			return qc_("fossa", "landform");
		case niInsula:
			// TRANSLATORS: Island (islands), an isolated land area (or group of such areas) surrounded by, or nearly surrounded by, a liquid area (sea or lake)
			return qc_("insula", "landform");
		case niLabes:
			// TRANSLATORS: Landslide
			return qc_("labes", "landform");
		case niLabyrinthus:
			// TRANSLATORS: Complex of intersecting valleys or ridges
			return qc_("labyrinthus", "landform");
		case niLacuna:
			// TRANSLATORS: Irregularly shaped depression on Titan having the appearance of a dry lake bed
			return qc_("lacuna", "landform");
		case niLacus:
			// TRANSLATORS: "Lake" or small plain; on Titan, a "lake" or small, dark plain with discrete, sharp boundaries
			return qc_("lacus", "landform");
		case niLargeRingedFeature:
			// TRANSLATORS: Cryptic ringed feature
			return qc_("large ringed feature", "landform");
		case niLenticula:
			// TRANSLATORS: Small dark spots on Europa
			return qc_("lenticula", "landform");
		case niLinea:
			// TRANSLATORS: A dark or bright elongate marking, may be curved or straight
			return qc_("linea", "landform");
		case niLingula:
			// TRANSLATORS: Extension of plateau having rounded lobate or tongue-like boundaries
			return qc_("lingula", "landform");
		case niMacula:
			// TRANSLATORS: Dark spot, may be irregular
			return qc_("macula", "landform");
		case niMare:
			// TRANSLATORS: "Sea"; on the Moon, low albedo, relatively smooth plain, generally of large extent; on Mars, dark albedo areas of no known geological significance; on Titan, large expanses of dark materials thought to be liquid hydrocarbons
			return qc_("mare", "landform");
		case niMensa:
			// TRANSLATORS: A flat-topped prominence with cliff-like edges
			return qc_("mensa", "landform");
		case niMons:
			// TRANSLATORS: Mountain
			return qc_("mons", "landform");
		case niOceanus:
			// TRANSLATORS: A very large dark area on the Moon
			return qc_("oceanus", "landform");
		case niPalus:
			// TRANSLATORS: "Swamp"; small plain
			return qc_("palus", "landform");
		case niPatera:
			// TRANSLATORS: An irregular crater, or a complex one with scalloped edges
			return qc_("patera", "landform");
		case niPlanitia:
			// TRANSLATORS: Low plain
			return qc_("planitia", "landform");
		case niPlanum:
			// TRANSLATORS: Plateau or high plain
			return qc_("planum", "landform");
		case niPlume:
			// TRANSLATORS: Cryo-volcanic features on Triton
			return qc_("plume", "landform");
		case niPromontorium:
			// TRANSLATORS: "Cape"; headland promontoria
			return qc_("promontorium", "landform");
		case niRegio:
			// TRANSLATORS: A large area marked by reflectivity or color distinctions from adjacent areas, or a broad geographic region
			return qc_("regio", "landform");
		case niReticulum:
			// TRANSLATORS: Reticular (netlike) pattern on Venus
			return qc_("reticulum", "landform");
		case niRima:
			// TRANSLATORS: Fissure
			return qc_("rima", "landform");
		case niRupes:
			// TRANSLATORS: Scarp
			return qc_("rupes", "landform");
		case niSatelliteFeature:
			// TRANSLATORS: A feature that shares the name of an associated feature.
			return qc_("satellite feature", "landform");
		case niScopulus:
			// TRANSLATORS: Lobate or irregular scarp
			return qc_("scopulus", "landform");
		case niSerpens:
			// TRANSLATORS: Sinuous feature with segments of positive and negative relief along its length
			return qc_("serpens", "landform");
		case niSulcus:
			// TRANSLATORS: Subparallel furrows and ridges
			return qc_("sulcus", "landform");
		case niSinus:
			// TRANSLATORS: "Bay"; small plain; on Titan, bays within seas or lakes of liquid hydrocarbons
			return qc_("sinus", "landform");
		case niTerra:
			// TRANSLATORS: Extensive land mass
			return qc_("terra", "landform");
		case niTessera:
			// TRANSLATORS: Tile-like, polygonal terrain
			return qc_("tessera", "landform");
		case niTholus:
			// TRANSLATORS: Small domical mountain or hill
			return qc_("tholus", "landform");
		case niUnda:
			// TRANSLATORS: Dunes
			return qc_("unda", "landform");
		case niVallis:
			// TRANSLATORS: Valley
			return qc_("vallis", "landform");
		case niVastitas:
			// TRANSLATORS: Extensive plain
			return qc_("vastitas", "landform");
		case niVirga:
			// TRANSLATORS: A streak or stripe of color
			return qc_("virga", "landform");
		case niLandingSite:
			// TRANSLATORS: Lunar features at or near Apollo landing sites
			return qc_("landing site name", "landform");
		case niUNDEFINED:
		default:
			return qc_("undocumented landform type", "landform");
	}
}

QString NomenclatureItem::getNomenclatureTypeLatinString(NomenclatureItemType nType)
{
	switch (nType)
	{
		case niArcus:
			return "arcus";
		case niAstrum:
			return "astrum";
		case niCatena:
			return "catena";
		case niCavus:
			return "cavus";
		case niChaos:
			return "chaos";
		case niChasma:
			return "chasma";
		case niCollis:
			return "collis";
		case niCorona:
			return "corona";
		case niCrater:
			return "crater";
		case niDorsum:
			return "dorsum";
		case niFacula:
			return "facula";
		case niFarrum:
			return "farrum";
		case niFlexus:
			return "flexus";
		case niFluctus:
			return "fluctus";
		case niFlumen:
			return "flumen";
		case niFretum:
			return "fretum";
		case niFossa:
			return "fossa";
		case niInsula:
			return "insula";
		case niLabes:
			return "labes";
		case niLabyrinthus:
			return "labyrinthus";
		case niLacuna:
			return "lacuna";
		case niLacus:
			return "lacus";
		case niLenticula:
			return "lenticula";
		case niLinea:
			return "linea";
		case niLingula:
			return "lingula";
		case niMacula:
			return "macula";
		case niMare:
			return "mare";
		case niMensa:
			return "mensa";
		case niMons:
			return "mons";
		case niOceanus:
			return "oceanus";
		case niPalus:
			return "palus";
		case niPatera:
			return "patera";
		case niPlanitia:
			return "planitia";
		case niPlanum:
			return "planum";
		case niPlume:
			return "plume";
		case niPromontorium:
			return "promontorium";
		case niRegio:
			return "regio";
		case niReticulum:
			return "reticulum";
		case niRima:
			return "rima";
		case niRupes:
			return "rupes";
		case niScopulus:
			return "scopulus";
		case niSerpens:
			return "serpens";
		case niSulcus:
			return "sulcus";
		case niSinus:
			return "sinus";
		case niTerra:
			return "terra";
		case niTessera:
			return "tessera";
		case niTholus:
			return "tholus";
		case niUnda:
			return "unda";
		case niVallis:
			return "vallis";
		case niVastitas:
			return "vastitas";
		case niVirga:
			return "virga";
		default:
			return "";
	}
}

QString NomenclatureItem::getNomenclatureTypeDescription(NomenclatureItemType nType, QString englishName)
{
	switch (nType)
	{
		case niAlbedoFeature:
			// TRANSLATORS: Description for landform 'albedo feature'
			return q_("Geographic area distinguished by amount of reflected light.");
		case niArcus:
			// TRANSLATORS: Description for landform 'arcus'
			return q_("Arc-shaped feature.");
		case niAstrum:
			// TRANSLATORS: Description for landform 'astrum'
			return q_("Radial-patterned feature.");
		case niCatena:
			// TRANSLATORS: Description for landform 'catena'
			return q_("Chain of craters.");
		case niCavus:
			// TRANSLATORS: Description for landform 'cavus'
			return q_("Hollows, irregular steep-sided depressions usually in arrays or clusters.");
		case niChaos:
			// TRANSLATORS: Description for landform 'chaos'
			return q_("Distinctive area of broken terrain.");
		case niChasma:
			// TRANSLATORS: Description for landform 'chasma'
			return q_("A deep, elongated, steep-sided depression.");
		case niCollis:
			// TRANSLATORS: Description for landform 'collis'
			return q_("Small hills or knobs.");
		case niCorona:
			// TRANSLATORS: Description for landform 'corona'
			return q_("Ovoid-shaped feature.");
		case niCrater:
			// TRANSLATORS: Description for landform 'crater'
			return q_("A circular depression.");
		case niDorsum:
			// TRANSLATORS: Description for landform 'dorsum'
			return q_("Ridge.");
		case niEruptiveCenter:
			// TRANSLATORS: Description for landform 'eruptive center'
			return q_("Active volcanic center.");
		case niFacula:
			// TRANSLATORS: Description for landform 'facula'
			return q_("Bright spot.");
		case niFarrum:
			// TRANSLATORS: Description for landform 'farrum'
			return q_("Pancake-like structure, or a row of such structures.");
		case niFlexus:
			// TRANSLATORS: Description for landform 'flexus'
			return q_("A very low curvilinear ridge with a scalloped pattern.");
		case niFluctus:
			// TRANSLATORS: Description for landform 'fluctus'
			return q_("Flow terrain.");
		case niFlumen:
			// TRANSLATORS: Description for landform 'flumen'
			return q_("Channel, that might carry liquid.");
		case niFretum:
			// TRANSLATORS: Description for landform 'fretum'
			return q_("Strait, a narrow passage of liquid connecting two larger areas of liquid.");
		case niFossa:
			// TRANSLATORS: Description for landform 'fossa'
			return q_("Long, narrow depression.");
		case niInsula:
			// TRANSLATORS: Description for landform 'insula'
			return q_("Island, an isolated land area surrounded by, or nearly surrounded by, a liquid area (sea or lake).");
		case niLabes:
			// TRANSLATORS: Description for landform 'labes'
			return q_("Landslide.");
		case niLabyrinthus:
			// TRANSLATORS: Description for landform 'labyrinthus'
			return q_("Complex of intersecting valleys or ridges.");
		case niLacuna:
			// TRANSLATORS: Description for landform 'lacuna'
			return q_("Irregularly shaped depression, having the appearance of a dry lake bed.");
		case niLacus:
			if (englishName=="Titan")
			{
				// TRANSLATORS: Description for landform 'lacus' on Titan
				return q_("'Lake' or small, dark plain with discrete, sharp boundaries.");
			}
			else
			{
				// TRANSLATORS: Description for landform 'lacus'
				return q_("'Lake' or small plain.");
			}
		case niLargeRingedFeature:
			// TRANSLATORS: Description for landform 'large ringed feature'
			return q_("Cryptic ringed feature.");
		case niLenticula:
			// TRANSLATORS: Description for landform 'lenticula'
			return q_("Small dark spot.");
		case niLinea:
			// TRANSLATORS: Description for landform 'linea'
			return q_("A dark or bright elongate marking, may be curved or straight.");
		case niLingula:
			// TRANSLATORS: Description for landform 'lingula'
			return q_("Extension of plateau having rounded lobate or tongue-like boundaries.");
		case niMacula:
			// TRANSLATORS: Description for landform 'macula'
			return q_("Dark spot, may be irregular");
		case niMare:
		{
			if (englishName=="Mars")
			{
				// TRANSLATORS: Description for landform 'mare' on Mars
				return q_("'Sea'; dark albedo areas of no known geological significance.");
			}
			if (englishName=="Titan")
			{
				// TRANSLATORS: Description for landform 'mare' on Titan
				return q_("'Sea'; large expanses of dark materials thought to be liquid hydrocarbons");
			}
			//if (englishName=="Moon")  // Presumably this should also be the default?
			//{
				// TRANSLATORS: Description for landform 'mare' on the Moon
				return q_("'Sea'; low albedo, relatively smooth plain, generally of large extent.");
			//}
		}
		case niMensa:
			// TRANSLATORS: Description for landform 'mensa'
			return q_("A flat-topped prominence with cliff-like edges.");
		case niMons:
			// TRANSLATORS: Description for landform 'mons'
			return q_("Mountain.");
		case niOceanus:
			// TRANSLATORS: Description for landform 'oceanus'
			return q_("A very large dark area.");
		case niPalus:
			// TRANSLATORS: Description for landform 'palus'
			return q_("'Swamp'; small plain.");
		case niPatera:
			// TRANSLATORS: Description for landform 'patera'
			return q_("An irregular crater, or a complex one with scalloped edges.");
		case niPlanitia:
			// TRANSLATORS: Description for landform 'planitia'
			return q_("Low plain.");
		case niPlanum:
			// TRANSLATORS: Description for landform 'planum'
			return q_("Plateau or high plain.");
		case niPlume:
			// TRANSLATORS: Description for landform 'plume'
			return q_("Cryo-volcanic feature.");
		case niPromontorium:
			// TRANSLATORS: Description for landform 'promontorium'
			return q_("'Cape'; headland promontoria.");
		case niRegio:
			// TRANSLATORS: Description for landform 'regio'
			return q_("A large area marked by reflectivity or color distinctions from adjacent areas, or a broad geographic region.");
		case niReticulum:
			// TRANSLATORS: Description for landform 'reticulum'
			return q_("Reticular (netlike) pattern.");
		case niRima:
			// TRANSLATORS: Description for landform 'rima'
			return q_("Fissure.");
		case niRupes:
			// TRANSLATORS: Description for landform 'rupes'
			return q_("Scarp.");
		case niSatelliteFeature:
			// TRANSLATORS: Description for landform 'satellite feature'
			return q_("A feature that shares the name of an associated feature.");
		case niScopulus:
			// TRANSLATORS: Description for landform 'scopulus'
			return q_("Lobate or irregular scarp.");
		case niSerpens:
			// TRANSLATORS: Description for landform 'serpens'
			return q_("Sinuous feature with segments of positive and negative relief along its length.");
		case niSulcus:
			// TRANSLATORS: Description for landform 'sulcus'
			return q_("Subparallel furrows and ridges.");
		case niSinus:
			if (englishName=="Titan")
			{
				// TRANSLATORS: Description for landform 'sinus' on Titan
				return q_("'Bay'; bays within seas or lakes of liquid hydrocarbons.");
			}
			else
			{
				// TRANSLATORS: Description for landform 'sinus'
				return q_("'Bay'; small plain.");
			}
		case niTerra:
			// TRANSLATORS: Description for landform 'terra'
			return q_("Extensive land mass.");
		case niTessera:
			// TRANSLATORS: Description for landform 'tessera'
			return q_("Tile-like, polygonal terrain.");
		case niTholus:
			// TRANSLATORS: Description for landform 'tholus'
			return q_("Small domical mountain or hill.");
		case niUnda:
			// TRANSLATORS: Description for landform 'unda'
			return q_("Dunes.");
		case niVallis:
			// TRANSLATORS: Description for landform 'vallis'
			return q_("Valley.");
		case niVastitas:
			// TRANSLATORS: Description for landform 'vastitas'
			return q_("Extensive plain.");
		case niVirga:
			// TRANSLATORS: Description for landform 'virga'
			return q_("A streak or stripe of color.");
		case niLandingSite:			
			return "";
		case niUNDEFINED:
		default:
			return q_("Undocumented landform type.");
	}
}

float NomenclatureItem::getSelectPriority(const StelCore* core) const
{
	if (planet->getVMagnitude(core)>=20.f)
	{
		// The planet is too faint for view (in the deep shadow for example), so let's disable select the nomenclature
		return StelObject::getSelectPriority(core)+25.f;
	}
	else if (getFlagLabels() && (getAngularSize(core)*M_PI/180.*static_cast<double>(core->getProjection(StelCore::FrameJ2000)->getPixelPerRadAtCenter())>=25.))
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
		return QString("%1 (%2)").arg(nameI18n, getNomenclatureTypeString(nType));
	else
		return nameI18n;
}

QString NomenclatureItem::getEnglishName() const
{
	if (getNomenclatureType()==niCrater)
		return QString("%1 (%2)").arg(englishName, getNomenclatureTypeLatinString(nType));
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
		QString tstr  = getNomenclatureTypeString(nType);
		QString latin = getNomenclatureTypeLatinString(nType); // not always latin!
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
		QString description = getNomenclatureTypeDescription(nType, planet->getEnglishName());
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
	if (fuzzyEquals(jde, core->getJDE())) return XYZ;
	jde = core->getJDE();
	const Vec3d equPos = planet->getJ2000EquatorialPos(core);
	// Calculate the radius of the planet. It is necessary to re-scale it
	const double r = planet->getEquatorialRadius() * static_cast<double>(planet->getSphereScale());

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
	XYZ = equPos + (core->matVsop87ToJ2000 * planet->getRotEquatorialToVsop87()) * Mat4d::zrotation(static_cast<double>(planet->getAxisRotation())* M_PI/180.0) * XYZ0;
	return XYZ;
}

float NomenclatureItem::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core);
	return 99.f;
}

double NomenclatureItem::getAngularSize(const StelCore* core) const
{
	return std::atan2(static_cast<double>(size)*planet->getSphereScale()/AU, getJ2000EquatorialPos(core).length()) * M_180_PI;
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

	const double screenSize = getAngularSize(core)*M_PI/180.*static_cast<double>(painter->getProjector()->getPixelPerRadAtCenter());

	// We can use ratio of angular size to the FOV to checking visibility of features also!
	// double scale = getAngularSize(core)/painter->getProjector()->getFov();
	// if (painter->getProjector()->projectCheck(XYZ, srcPos) && (dist >= XYZ.length()) && (scale>0.04 && scale<0.5))

	// check visibility of feature
	Vec3d srcPos;
	if (painter->getProjector()->projectCheck(XYZ, srcPos) && (equPos.length() >= XYZ.length()) && (screenSize>50. && screenSize<750.))
	{
		painter->setColor(color, 1.0);
		painter->drawCircle(static_cast<float>(srcPos[0]), static_cast<float>(srcPos[1]), 2.f);
		painter->drawText(static_cast<float>(srcPos[0]), static_cast<float>(srcPos[1]), nameI18n, 0, 5.f, 5.f, false);
	}
}
