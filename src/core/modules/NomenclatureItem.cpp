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
	Q_ASSERT(niTypeStringMap.size()>20); // should be filled, not sure how many.
	return niTypeStringMap.value(nType, qc_("undocumented landform type", "landform"));
}

QString NomenclatureItem::getNomenclatureTypeLatinString(NomenclatureItemType nType)
{
	static const QMap<NomenclatureItemType, QString>map = {
		{ niArcus  , "arcus" },
		{ niAstrum , "astrum" },
		{ niCatena , "catena" },
		{ niCavus  , "cavus" },
		{ niChaos  , "chaos" },
		{ niChasma , "chasma" },
		{ niCollis , "collis" },
		{ niCorona , "corona" },
		{ niCrater , "crater" },
		{ niDorsum , "dorsum" },
		{ niFacula , "facula" },
		{ niFarrum , "farrum" },
		{ niFlexus , "flexus" },
		{ niFluctus, "fluctus" },
		{ niFlumen , "flumen" },
		{ niFretum , "fretum" },
		{ niFossa  , "fossa" },
		{ niInsula , "insula" },
		{ niLabes  , "labes" },
		{ niLabyrinthus, "labyrinthus" },
		{ niLacuna   , "lacuna" },
		{ niLacus    , "lacus" },
		{ niLenticula, "lenticula" },
		{ niLinea    , "linea" },
		{ niLingula  , "lingula" },
		{ niMacula   , "macula" },
		{ niMare     , "mare" },
		{ niMensa    , "mensa" },
		{ niMons     , "mons" },
		{ niOceanus  , "oceanus" },
		{ niPalus    , "palus" },
		{ niPatera   , "patera" },
		{ niPlanitia , "planitia" },
		{ niPlanum   , "planum" },
		{ niPlume    , "plume" },
		{ niPromontorium, "promontorium" },
		{ niRegio    , "regio" },
		{ niReticulum, "reticulum" },
		{ niRima     , "rima" },
		{ niRupes    , "rupes" },
		{ niSaxum  , "saxum" },
		{ niScopulus , "scopulus" },
		{ niSerpens  , "serpens" },
		{ niSulcus   , "sulcus" },
		{ niSinus    , "sinus" },
		{ niTerra    , "terra" },
		{ niTessera  , "tessera" },
		{ niTholus   , "tholus" },
		{ niUnda     , "unda" },
		{ niVallis   , "vallis" },
		{ niVastitas , "vastitas" },
		{ niVirga    , "virga" }};
		return map.value(nType, "");
}

QString NomenclatureItem::getNomenclatureTypeDescription(NomenclatureItemType nType, QString englishName)
{
	Q_ASSERT(niTypeDescriptionMap.size()>20); // should be filled, not sure how many.
	QString ret=niTypeDescriptionMap.value(nType, q_("Undocumented landform type."));
	// Fix a few exceptions
	if((englishName=="Mars") && (nType==niMare))
		// TRANSLATORS: Description for landform 'mare' on Mars
		return q_("'Sea'; dark albedo areas of no known geological significance.");
	if (englishName=="Titan")
	{
		if (nType==niLacus)
			// TRANSLATORS: Description for landform 'lacus' on Titan
			return q_("'Lake' or small, dark plain with discrete, sharp boundaries.");
		if (nType==niMare)
			// TRANSLATORS: Description for landform 'mare' on Titan
			return q_("'Sea'; large expanses of dark materials thought to be liquid hydrocarbons");
		if (nType==niSinus)
			// TRANSLATORS: Description for landform 'sinus' on Titan
			return q_("'Bay'; bays within seas or lakes of liquid hydrocarbons.");
	}
	return ret;
}

float NomenclatureItem::getSelectPriority(const StelCore* core) const
{
	if (planet->getVMagnitude(core)>=20.f)
	{
		// The planet is too faint for view (in deep shadow for example), so let's disable selecting the nomenclature
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
	if (nType==niCrater)
		return QString("%1 (%2, %3)").arg(nameI18n, getNomenclatureTypeString(nType), getPlanet()->getNameI18n());
	else
		return QString("%1 (%2)").arg(nameI18n, getPlanet()->getNameI18n());
}

QString NomenclatureItem::getEnglishName() const
{
	if (nType==niCrater)
		return QString("%1 (%2, %3)").arg(englishName, getNomenclatureTypeLatinString(nType), getPlanet()->getEnglishName());
	else
		return QString("%1 (%2)").arg(englishName, getPlanet()->getEnglishName());
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
			oss << QString("%1: <b>%2</b> (%3: %4)<br/>").arg(ts).arg(tstr).arg(q_("geologic term")).arg(latin);
		else
			oss << QString("%1: <b>%2</b><br/>").arg(ts).arg(tstr);
	}

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Size && size>0.f)
	{
		QString sz = q_("Linear size");
		// Satellite Features are almost(?) exclusively lettered craters, and all are on the Moon. Assume craters.
		if ((getNomenclatureType()==NomenclatureItem::niCrater) || (getNomenclatureType()==NomenclatureItem::niSatelliteFeature))
			sz = q_("Diameter");
		oss << QString("%1: %2 %3<br/>").arg(sz).arg(QString::number(size, 'f', 2)).arg(qc_("km", "distance"));
	}

	if (flags&Extra)
	{
		oss << QString("%1: %2/%3<br/>").arg(q_("Planetographic long./lat.")).arg(StelUtils::decDegToDmsStr(longitude)).arg(StelUtils::decDegToDmsStr(latitude));
		oss << QString("%1: %2<br/>").arg(q_("Celestial body")).arg(planet->getNameI18n());
		QString description = getNomenclatureTypeDescription(nType, planet->getEnglishName());
		if (getNomenclatureType()!=NomenclatureItem::niUNDEFINED && !description.isEmpty())
			oss << QString("%1: %2<br/>").arg(q_("Landform description"), description);
		oss << QString("%1: %2°<br/>").arg(q_("Solar altitude")).arg(QString::number(getSolarAltitude(core), 'f', 1));
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
		float brightness=(getSolarAltitude(core)<0. ? 0.25f : 1.0f);
		painter->setColor(color*brightness, 1.0f);
		painter->drawCircle(static_cast<float>(srcPos[0]), static_cast<float>(srcPos[1]), 2.f);
		painter->drawText(static_cast<float>(srcPos[0]), static_cast<float>(srcPos[1]), nameI18n, 0, 5.f, 5.f, false);
	}
}

double NomenclatureItem::getSolarAltitude(const StelCore *core) const
{
	QPair<Vec4d, Vec3d> ssop=planet->getSubSolarObserverPoints(core);
	double sign=planet->isRotatingRetrograde() ? -1. : 1.;
	double colongitude=450.*M_PI_180-sign*ssop.second[2];
	double h=asin(sin(ssop.second[0])*sin(static_cast<double>(latitude)*M_PI_180) +
		      cos(ssop.second[0])*cos(static_cast<double>(latitude)*M_PI_180)*sin(colongitude-static_cast<double>(longitude)*M_PI_180));
	return h*M_180_PI;
}

NomenclatureItem::NomenclatureItemType NomenclatureItem::getNomenclatureItemType(const QString abbrev)
{
	// codes for types of features (details: https://planetarynames.wr.usgs.gov/DescriptorTerms)
	static const QMap<QString, NomenclatureItem::NomenclatureItemType> niTypes = {
		{ "AL", NomenclatureItem::niAlbedoFeature	},
		{ "AR", NomenclatureItem::niArcus		},
		{ "AS", NomenclatureItem::niAstrum		},
		{ "CA", NomenclatureItem::niCatena		},
		{ "CB", NomenclatureItem::niCavus		},
		{ "CH", NomenclatureItem::niChaos		},
		{ "CM", NomenclatureItem::niChasma		},
		{ "CO", NomenclatureItem::niCollis		},
		{ "CR", NomenclatureItem::niCorona		},
		{ "AA", NomenclatureItem::niCrater		},
		{ "DO", NomenclatureItem::niDorsum		},
		{ "ER", NomenclatureItem::niEruptiveCenter	},
		{ "FA", NomenclatureItem::niFacula		},
		{ "FR", NomenclatureItem::niFarrum		},
		{ "FE", NomenclatureItem::niFlexus		},
		{ "FL", NomenclatureItem::niFluctus		},
		{ "FM", NomenclatureItem::niFlumen		},
		{ "FO", NomenclatureItem::niFossa		},
		{ "FT", NomenclatureItem::niFretum		},
		{ "IN", NomenclatureItem::niInsula		},
		{ "LA", NomenclatureItem::niLabes		},
		{ "LB", NomenclatureItem::niLabyrinthus		},
		{ "LU", NomenclatureItem::niLacuna		},
		{ "LC", NomenclatureItem::niLacus		},
		{ "LF", NomenclatureItem::niLandingSite		},
		{ "LG", NomenclatureItem::niLargeRingedFeature	},
		{ "LE", NomenclatureItem::niLenticula		},
		{ "LI", NomenclatureItem::niLinea		},
		{ "LN", NomenclatureItem::niLingula		},
		{ "MA", NomenclatureItem::niMacula		},
		{ "ME", NomenclatureItem::niMare		},
		{ "MN", NomenclatureItem::niMensa		},
		{ "MO", NomenclatureItem::niMons		},
		{ "OC", NomenclatureItem::niOceanus		},
		{ "PA", NomenclatureItem::niPalus		},
		{ "PE", NomenclatureItem::niPatera		},
		{ "PL", NomenclatureItem::niPlanitia		},
		{ "PM", NomenclatureItem::niPlanum		},
		{ "PU", NomenclatureItem::niPlume		},
		{ "PR", NomenclatureItem::niPromontorium	},
		{ "RE", NomenclatureItem::niRegio		},
		{ "RT", NomenclatureItem::niReticulum		},
		{ "RI", NomenclatureItem::niRima		},
		{ "RU", NomenclatureItem::niRupes		},
		{ "SA", NomenclatureItem::niSaxum		},
		{ "SF", NomenclatureItem::niSatelliteFeature	},
		{ "SC", NomenclatureItem::niScopulus		},
		{ "SE", NomenclatureItem::niSerpens		},
		{ "SI", NomenclatureItem::niSinus		},
		{ "SU", NomenclatureItem::niSulcus		},
		{ "TA", NomenclatureItem::niTerra		},
		{ "TE", NomenclatureItem::niTessera		},
		{ "TH", NomenclatureItem::niTholus		},
		{ "UN", NomenclatureItem::niUnda		},
		{ "VA", NomenclatureItem::niVallis		},
		{ "VS", NomenclatureItem::niVastitas		},
		{ "VI", NomenclatureItem::niVirga		}};
	return niTypes.value(abbrev, NomenclatureItem::niUNDEFINED);
}
QMap<NomenclatureItem::NomenclatureItemType, QString> NomenclatureItem::niTypeStringMap;
QMap<NomenclatureItem::NomenclatureItemType, QString> NomenclatureItem::niTypeDescriptionMap;
void NomenclatureItem::createNameLists()
{
	niTypeStringMap.clear();
	// TRANSLATORS: Geographic area distinguished by amount of reflected light
	niTypeStringMap.insert( niAlbedoFeature, qc_("albedo feature", "landform") );
	// TRANSLATORS: Arc-shaped feature
	niTypeStringMap.insert( niArcus, qc_("arcus", "landform") );
	// TRANSLATORS: Radial-patterned features on Venus
	niTypeStringMap.insert( niAstrum, qc_("astrum", "landform") );
	// TRANSLATORS: Chain of craters
	niTypeStringMap.insert( niCatena, qc_("catena", "landform") );
	// TRANSLATORS: Hollows, irregular steep-sided depressions usually in arrays or clusters
	niTypeStringMap.insert( niCavus, qc_("cavus", "landform") );
	// TRANSLATORS: Distinctive area of broken terrain
	niTypeStringMap.insert( niChaos, qc_("chaos", "landform") );
	// TRANSLATORS: A deep, elongated, steep-sided depression
	niTypeStringMap.insert( niChasma, qc_("chasma", "landform") );
	// TRANSLATORS: Small hills or knobs
	niTypeStringMap.insert( niCollis, qc_("collis", "landform") );
	// TRANSLATORS: Ovoid-shaped feature
	niTypeStringMap.insert( niCorona, qc_("corona", "landform") );
	// TRANSLATORS: A circular depression
	niTypeStringMap.insert( niCrater, qc_("crater", "landform") );
	// TRANSLATORS: Ridge
	niTypeStringMap.insert( niDorsum, qc_("dorsum", "landform") );
	// TRANSLATORS: Active volcanic centers on Io
	niTypeStringMap.insert( niEruptiveCenter, qc_("eruptive center", "landform") );
	// TRANSLATORS: Bright spot
	niTypeStringMap.insert( niFacula, qc_("facula", "landform") );
	// TRANSLATORS: Pancake-like structure, or a row of such structures
	niTypeStringMap.insert( niFarrum, qc_("farrum", "landform") );
	// TRANSLATORS: A very low curvilinear ridge with a scalloped pattern
	niTypeStringMap.insert( niFlexus, qc_("flexus", "landform") );
	// TRANSLATORS: Flow terrain
	niTypeStringMap.insert( niFluctus, qc_("fluctus", "landform") );
	// TRANSLATORS: Channel on Titan that might carry liquid
	niTypeStringMap.insert( niFlumen, qc_("flumen", "landform") );
	// TRANSLATORS: Strait, a narrow passage of liquid connecting two larger areas of liquid
	niTypeStringMap.insert( niFretum, qc_("fretum", "landform") );
	// TRANSLATORS: Long, narrow depression
	niTypeStringMap.insert( niFossa, qc_("fossa", "landform") );
	// TRANSLATORS: Island (islands), an isolated land area (or group of such areas) surrounded by, or nearly surrounded by, a liquid area (sea or lake)
	niTypeStringMap.insert( niInsula, qc_("insula", "landform") );
	// TRANSLATORS: Landslide
	niTypeStringMap.insert( niLabes, qc_("labes", "landform") );
	// TRANSLATORS: Complex of intersecting valleys or ridges
	niTypeStringMap.insert( niLabyrinthus, qc_("labyrinthus", "landform") );
	// TRANSLATORS: Irregularly shaped depression on Titan having the appearance of a dry lake bed
	niTypeStringMap.insert( niLacuna, qc_("lacuna", "landform") );
	// TRANSLATORS: "Lake" or small plain; on Titan, a "lake" or small, dark plain with discrete, sharp boundaries
	niTypeStringMap.insert( niLacus, qc_("lacus", "landform") );
	// TRANSLATORS: Cryptic ringed feature
	niTypeStringMap.insert( niLargeRingedFeature, qc_("large ringed feature", "landform") );
	// TRANSLATORS: Small dark spots on Europa
	niTypeStringMap.insert( niLenticula, qc_("lenticula", "landform") );
	// TRANSLATORS: A dark or bright elongate marking, may be curved or straight
	niTypeStringMap.insert( niLinea, qc_("linea", "landform") );
	// TRANSLATORS: Extension of plateau having rounded lobate or tongue-like boundaries
	niTypeStringMap.insert( niLingula, qc_("lingula", "landform") );
	// TRANSLATORS: Dark spot, may be irregular
	niTypeStringMap.insert( niMacula, qc_("macula", "landform") );
	// TRANSLATORS: "Sea"; on the Moon, low albedo, relatively smooth plain, generally of large extent; on Mars, dark albedo areas of no known geological significance; on Titan, large expanses of dark materials thought to be liquid hydrocarbons
	niTypeStringMap.insert( niMare, qc_("mare", "landform") );
	// TRANSLATORS: A flat-topped prominence with cliff-like edges
	niTypeStringMap.insert( niMensa, qc_("mensa", "landform") );
	// TRANSLATORS: Mountain
	niTypeStringMap.insert( niMons, qc_("mons", "landform") );
	// TRANSLATORS: A very large dark area on the Moon
	niTypeStringMap.insert( niOceanus, qc_("oceanus", "landform") );
	// TRANSLATORS: "Swamp"; small plain
	niTypeStringMap.insert( niPalus, qc_("palus", "landform") );
	// TRANSLATORS: An irregular crater, or a complex one with scalloped edges
	niTypeStringMap.insert( niPatera, qc_("patera", "landform") );
	// TRANSLATORS: Low plain
	niTypeStringMap.insert( niPlanitia, qc_("planitia", "landform") );
	// TRANSLATORS: Plateau or high plain
	niTypeStringMap.insert( niPlanum, qc_("planum", "landform") );
	// TRANSLATORS: Cryo-volcanic features on Triton
	niTypeStringMap.insert( niPlume, qc_("plume", "landform") );
	// TRANSLATORS: "Cape"; headland promontoria
	niTypeStringMap.insert( niPromontorium, qc_("promontorium", "landform") );
	// TRANSLATORS: A large area marked by reflectivity or color distinctions from adjacent areas, or a broad geographic region
	niTypeStringMap.insert( niRegio, qc_("regio", "landform") );
	// TRANSLATORS: Reticular (netlike) pattern on Venus
	niTypeStringMap.insert( niReticulum, qc_("reticulum", "landform") );
	// TRANSLATORS: Fissure
	niTypeStringMap.insert( niRima, qc_("rima", "landform") );
	// TRANSLATORS: Scarp
	niTypeStringMap.insert( niRupes, qc_("rupes", "landform") );
	// TRANSLATORS: A feature that shares the name of an associated feature.
	niTypeStringMap.insert( niSatelliteFeature, qc_("satellite feature", "landform") );
	// TRANSLATORS: Boulder or rock
	niTypeStringMap.insert( niSaxum, qc_("saxum", "landform") );
	// TRANSLATORS: Lobate or irregular scarp
	niTypeStringMap.insert( niScopulus, qc_("scopulus", "landform") );
	// TRANSLATORS: Sinuous feature with segments of positive and negative relief along its length
	niTypeStringMap.insert( niSerpens, qc_("serpens", "landform") );
	// TRANSLATORS: Subparallel furrows and ridges
	niTypeStringMap.insert( niSulcus, qc_("sulcus", "landform") );
	// TRANSLATORS: "Bay"; small plain; on Titan, bays within seas or lakes of liquid hydrocarbons
	niTypeStringMap.insert( niSinus, qc_("sinus", "landform") );
	// TRANSLATORS: Extensive land mass
	niTypeStringMap.insert( niTerra, qc_("terra", "landform") );
	// TRANSLATORS: Tile-like, polygonal terrain
	niTypeStringMap.insert( niTessera, qc_("tessera", "landform") );
	// TRANSLATORS: Small domical mountain or hill
	niTypeStringMap.insert( niTholus, qc_("tholus", "landform") );
	// TRANSLATORS: Dunes
	niTypeStringMap.insert( niUnda, qc_("unda", "landform") );
	// TRANSLATORS: Valley
	niTypeStringMap.insert( niVallis, qc_("vallis", "landform") );
	// TRANSLATORS: Extensive plain
	niTypeStringMap.insert( niVastitas, qc_("vastitas", "landform") );
	// TRANSLATORS: A streak or stripe of color
	niTypeStringMap.insert( niVirga, qc_("virga", "landform") );
	// TRANSLATORS: Lunar features at or near Apollo landing sites
	niTypeStringMap.insert( niLandingSite, qc_("landing site name", "landform") );

	niTypeDescriptionMap.clear();
	// TRANSLATORS: Description for landform 'albedo feature'
	niTypeDescriptionMap.insert( niAlbedoFeature, q_("Geographic area distinguished by amount of reflected light."));
	// TRANSLATORS: Description for landform 'arcus'
	niTypeDescriptionMap.insert( niArcus, q_("Arc-shaped feature."));
	// TRANSLATORS: Description for landform 'astrum'
	niTypeDescriptionMap.insert( niAstrum, q_("Radial-patterned feature."));
	// TRANSLATORS: Description for landform 'catena'
	niTypeDescriptionMap.insert( niCatena, q_("Chain of craters."));
	// TRANSLATORS: Description for landform 'cavus'
	niTypeDescriptionMap.insert( niCavus, q_("Hollows, irregular steep-sided depressions usually in arrays or clusters."));
	// TRANSLATORS: Description for landform 'chaos'
	niTypeDescriptionMap.insert( niChaos, q_("Distinctive area of broken terrain."));
	// TRANSLATORS: Description for landform 'chasma'
	niTypeDescriptionMap.insert( niChasma, q_("A deep, elongated, steep-sided depression."));
	// TRANSLATORS: Description for landform 'collis'
	niTypeDescriptionMap.insert( niCollis, q_("Small hills or knobs."));
	// TRANSLATORS: Description for landform 'corona'
	niTypeDescriptionMap.insert( niCorona, q_("Ovoid-shaped feature."));
	// TRANSLATORS: Description for landform 'crater'
	niTypeDescriptionMap.insert( niCrater, q_("A circular depression."));
	// TRANSLATORS: Description for landform 'dorsum'
	niTypeDescriptionMap.insert( niDorsum, q_("Ridge."));
	// TRANSLATORS: Description for landform 'eruptive center'
	niTypeDescriptionMap.insert( niEruptiveCenter, q_("Active volcanic center."));
	// TRANSLATORS: Description for landform 'facula'
	niTypeDescriptionMap.insert( niFacula, q_("Bright spot."));
	// TRANSLATORS: Description for landform 'farrum'
	niTypeDescriptionMap.insert( niFarrum, q_("Pancake-like structure, or a row of such structures."));
	// TRANSLATORS: Description for landform 'flexus'
	niTypeDescriptionMap.insert( niFlexus, q_("A very low curvilinear ridge with a scalloped pattern."));
	// TRANSLATORS: Description for landform 'fluctus'
	niTypeDescriptionMap.insert( niFluctus, q_("Flow terrain."));
	// TRANSLATORS: Description for landform 'flumen'
	niTypeDescriptionMap.insert( niFlumen, q_("Channel, that might carry liquid."));
	// TRANSLATORS: Description for landform 'fretum'
	niTypeDescriptionMap.insert( niFretum, q_("Strait, a narrow passage of liquid connecting two larger areas of liquid."));
	// TRANSLATORS: Description for landform 'fossa'
	niTypeDescriptionMap.insert( niFossa, q_("Long, narrow depression."));
	// TRANSLATORS: Description for landform 'insula'
	niTypeDescriptionMap.insert( niInsula, q_("Island, an isolated land area surrounded by, or nearly surrounded by, a liquid area (sea or lake)."));
	// TRANSLATORS: Description for landform 'labes'
	niTypeDescriptionMap.insert( niLabes, q_("Landslide."));
	// TRANSLATORS: Description for landform 'labyrinthus'
	niTypeDescriptionMap.insert( niLabyrinthus, q_("Complex of intersecting valleys or ridges."));
	// TRANSLATORS: Description for landform 'lacuna'
	niTypeDescriptionMap.insert( niLacuna, q_("Irregularly shaped depression, having the appearance of a dry lake bed."));
	// TRANSLATORS: Description for landform 'lacus'
	niTypeDescriptionMap.insert( niLacus, q_("'Lake' or small plain."));
	// TRANSLATORS: Description for landform 'large ringed feature'
	niTypeDescriptionMap.insert( niLargeRingedFeature, q_("Cryptic ringed feature."));
	// TRANSLATORS: Description for landform 'lenticula'
	niTypeDescriptionMap.insert( niLenticula, q_("Small dark spot."));
	// TRANSLATORS: Description for landform 'linea'
	niTypeDescriptionMap.insert( niLinea, q_("A dark or bright elongate marking, may be curved or straight."));
	// TRANSLATORS: Description for landform 'lingula'
	niTypeDescriptionMap.insert( niLingula, q_("Extension of plateau having rounded lobate or tongue-like boundaries."));
	// TRANSLATORS: Description for landform 'macula'
	niTypeDescriptionMap.insert( niMacula, q_("Dark spot, may be irregular"));
	// TRANSLATORS: Description for landform 'mare' on the Moon
	niTypeDescriptionMap.insert( niMare, q_("'Sea'; low albedo, relatively smooth plain, generally of large extent."));
	// TRANSLATORS: Description for landform 'mensa'
	niTypeDescriptionMap.insert( niMensa, q_("A flat-topped prominence with cliff-like edges."));
	// TRANSLATORS: Description for landform 'mons'
	niTypeDescriptionMap.insert( niMons, q_("Mountain."));
	// TRANSLATORS: Description for landform 'oceanus'
	niTypeDescriptionMap.insert( niOceanus, q_("A very large dark area."));
	// TRANSLATORS: Description for landform 'palus'
	niTypeDescriptionMap.insert( niPalus, q_("'Swamp'; small plain."));
	// TRANSLATORS: Description for landform 'patera'
	niTypeDescriptionMap.insert( niPatera, q_("An irregular crater, or a complex one with scalloped edges."));
	// TRANSLATORS: Description for landform 'planitia'
	niTypeDescriptionMap.insert( niPlanitia, q_("Low plain."));
	// TRANSLATORS: Description for landform 'planum'
	niTypeDescriptionMap.insert( niPlanum, q_("Plateau or high plain."));
	// TRANSLATORS: Description for landform 'plume'
	niTypeDescriptionMap.insert( niPlume, q_("Cryo-volcanic feature."));
	// TRANSLATORS: Description for landform 'promontorium'
	niTypeDescriptionMap.insert( niPromontorium, q_("'Cape'; headland promontoria."));
	// TRANSLATORS: Description for landform 'regio'
	niTypeDescriptionMap.insert( niRegio, q_("A large area marked by reflectivity or color distinctions from adjacent areas, or a broad geographic region."));
	// TRANSLATORS: Description for landform 'reticulum'
	niTypeDescriptionMap.insert( niReticulum, q_("Reticular (netlike) pattern."));
	// TRANSLATORS: Description for landform 'rima'
	niTypeDescriptionMap.insert( niRima, q_("Fissure."));
	// TRANSLATORS: Description for landform 'rupes'
	niTypeDescriptionMap.insert( niRupes, q_("Scarp."));
	// TRANSLATORS: Description for landform 'satellite feature'
	niTypeDescriptionMap.insert( niSatelliteFeature, q_("A feature that shares the name of an associated feature."));
	// TRANSLATORS: Description for landform 'saxum'
	niTypeDescriptionMap.insert( niSaxum, q_("Boulder or rock."));
	// TRANSLATORS: Description for landform 'scopulus'
	niTypeDescriptionMap.insert( niScopulus, q_("Lobate or irregular scarp."));
	// TRANSLATORS: Description for landform 'serpens'
	niTypeDescriptionMap.insert( niSerpens, q_("Sinuous feature with segments of positive and negative relief along its length."));
	// TRANSLATORS: Description for landform 'sinus'
	niTypeDescriptionMap.insert( niSinus, q_("'Bay'; small plain."));
	// TRANSLATORS: Description for landform 'sulcus'
	niTypeDescriptionMap.insert( niSulcus, q_("Subparallel furrows and ridges."));
	// TRANSLATORS: Description for landform 'terra'
	niTypeDescriptionMap.insert( niTerra, q_("Extensive land mass."));
	// TRANSLATORS: Description for landform 'tessera'
	niTypeDescriptionMap.insert( niTessera, q_("Tile-like, polygonal terrain."));
	// TRANSLATORS: Description for landform 'tholus'
	niTypeDescriptionMap.insert( niTholus, q_("Small domical mountain or hill."));
	// TRANSLATORS: Description for landform 'unda'
	niTypeDescriptionMap.insert( niUnda, q_("Dunes."));
	// TRANSLATORS: Description for landform 'vallis'
	niTypeDescriptionMap.insert( niVallis, q_("Valley."));
	// TRANSLATORS: Description for landform 'vastitas'
	niTypeDescriptionMap.insert( niVastitas, q_("Extensive plain."));
	// TRANSLATORS: Description for landform 'virga'
	niTypeDescriptionMap.insert( niVirga, q_("A streak or stripe of color."));
	niTypeDescriptionMap.insert( niLandingSite, "");
}
