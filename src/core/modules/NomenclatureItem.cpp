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
				   double nLatitude,
				   double nLongitude,
				   double nSize)
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
	StelUtils::spheToRect(longitude * M_PI_180, latitude * M_PI_180, XYZpc);
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
		{ niSpecialPointPole, "point" },
		{ niSpecialPointEast, "point" },
		{ niSpecialPointWest, "point" },
		{ niSpecialPointCenter, "point" },
		{ niSpecialPointSubSolar, "point" },
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
	float priority = StelObject::getSelectPriority(core);
	if (getFlagLabels())
	{
		const float scale = getAngularSizeRatio(core);
		if (planet->getVMagnitude(core)>=20.f || (planet->getJ2000EquatorialPos(core).lengthSquared() < getJ2000EquatorialPos(core).lengthSquared()))
		{
			// The feature on far-side of the planet or the planet is too faint for view
			// (in deep shadow for example), so let's disable selecting the nomenclature
			priority += 25.f;
		}
		else if (scale>=0.015f)
		{
			// The item may be good selectable when it over 37px size only
			//priority -= (20.f + scale*100.f);
			priority = planet->getSelectPriority(core) - scale*100.f;
			if (nType>=NomenclatureItemType::niSpecialPointPole)
				priority -= 10.f;
		}
		else
			priority -= 2.f;
	}
	else
		priority += 1.e12f; // suppress selection if switched off.

	return priority;
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

	if (flags&ObjectType && nType!=NomenclatureItem::niUNDEFINED)
	{
		QString tstr  = getNomenclatureTypeString(nType);
		QString latin = getNomenclatureTypeLatinString(nType); // not always latin!
		QString ts    = q_("Type");
		if (tstr!=latin && !latin.isEmpty())
			oss << QString("%1: <b>%2</b> (%3: %4)<br/>").arg(ts, tstr, q_("geologic term"), latin);
		else
			oss << QString("%1: <b>%2</b><br/>").arg(ts, tstr);
	}

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Size && size>0. && nType<NomenclatureItem::niSpecialPointPole)
	{
		QString sz = q_("Linear size");
		// Satellite Features are almost(?) exclusively lettered craters, and all are on the Moon. Assume craters.
		if ((nType==NomenclatureItem::niCrater) || (nType==NomenclatureItem::niSatelliteFeature))
			sz = q_("Diameter");
		oss << QString("%1: %2 %3<br/>").arg(sz, QString::number(size, 'f', 2), qc_("km", "distance"));
	}

	if (flags&Extra)
	{
		if (nType < NomenclatureItemType::niSpecialPointPole)
		{
			// TODO for 0.22.0 It seems the (numerical) planetary longitudes of NomenclatureItems are off by (usually) 360-longitude.
			// For 0.21.3 we must now hide the coordinates for the special points, but fix this ASAP.
			QString sLong = StelUtils::decDegToDmsStr(longitude), sLat = StelUtils::decDegToDmsStr(latitude);
			if (nType>=NomenclatureItemType::niSpecialPointEast && planet->getEnglishName()=="Jupiter")
			{
				// Due to Jupiter's issues around GRS shift we must repeat some calculations here.
				double lng=0., lat=0.;
				// East/West points are assumed to be along the equator, on the planet rim. Start with sub-observer point
				if (nType==NomenclatureItemType::niSpecialPointEast || nType==NomenclatureItemType::niSpecialPointWest)
				{
					QPair<Vec4d, Vec3d> subObs = planet->getSubSolarObserverPoints(core, false);
					lng = - subObs.first[2]  * M_180_PI + ((nType==NomenclatureItemType::niSpecialPointEast) ? 90. : -90.);
					Q_ASSERT(lat==0.);
				}
				// Center and Subsolar points are similar.
				if (nType==NomenclatureItemType::niSpecialPointCenter || nType==NomenclatureItemType::niSpecialPointSubSolar)
				{
					QPair<Vec4d, Vec3d> subObs = planet->getSubSolarObserverPoints(core, false);
					lat =   M_180_PI * (nType==NomenclatureItemType::niSpecialPointCenter ? subObs.first[1]: subObs.second[1]);
					lng = - M_180_PI * (nType==NomenclatureItemType::niSpecialPointCenter ? subObs.first[2]: subObs.second[2]);
				}
				lng   = StelUtils::fmodpos(lng, 360.);
				sLong = StelUtils::decDegToDmsStr(360.-lng);
				sLat  = StelUtils::decDegToDmsStr(lat);
			}
			oss << QString("%1: %2/%3<br/>").arg(q_("Planetographic long./lat."), sLong, sLat);
		}
		oss << QString("%1: %2<br/>").arg(q_("Celestial body"), planet->getNameI18n());
		QString description = getNomenclatureTypeDescription(nType, planet->getEnglishName());
		if (nType!=NomenclatureItem::niUNDEFINED && nType<NomenclatureItem::niSpecialPointPole && !description.isEmpty())
			oss << QString("%1: %2<br/>").arg(q_("Landform description"), description);
		if (planet->getEnglishName()!="Jupiter") // we must exclude this for now due to Jupiter's "off" rotation
			oss << QString("%1: %2°<br/>").arg(q_("Solar altitude"), QString::number(getSolarAltitude(core), 'f', 1));
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
	// East/West points are assumed to be along the equator, on the planet rim. Start with sub-observer point
	if (nType==NomenclatureItemType::niSpecialPointEast || nType==NomenclatureItemType::niSpecialPointWest)
	{
		QPair<Vec4d, Vec3d> subObs = planet->getSubSolarObserverPoints(core, true);

		longitude = - subObs.first[2]  * M_180_PI;
		if (planet->isRotatingRetrograde()) longitude *= -1;
		longitude += ((nType==NomenclatureItemType::niSpecialPointEast) ? 90. : -90.);
		longitude=StelUtils::fmodpos(longitude, 360.);
		Q_ASSERT(latitude==0.);
		// rebuild XYZpc
		StelUtils::spheToRect(longitude * M_PI_180, latitude * M_PI_180, XYZpc);
	}
	// Center and Subsolar points are similar.
	if (nType==NomenclatureItemType::niSpecialPointCenter || nType==NomenclatureItemType::niSpecialPointSubSolar)
	{
		QPair<Vec4d, Vec3d> subObs = planet->getSubSolarObserverPoints(core, true);

		if (nType==NomenclatureItemType::niSpecialPointCenter)
		{
			longitude = - subObs.first[2] * M_180_PI;
			if (planet->isRotatingRetrograde()) longitude *= -1;
			latitude = subObs.first[1] * M_180_PI; // or first[0]???
		}
		else
		{
			longitude = - subObs.second[2] * M_180_PI;
			if (planet->isRotatingRetrograde()) longitude *= -1;
			latitude = subObs.second[1] * M_180_PI; // or second[0]???
		}
		longitude=StelUtils::fmodpos(longitude, 360.);
		// rebuild XYZpc
		StelUtils::spheToRect(longitude * M_PI_180, latitude * M_PI_180, XYZpc);
	}
	// Calculate the radius of the planet at the item's position. It is necessary to re-scale it
	Vec4d rect = planet->getRectangularCoordinates(longitude, latitude);
	Vec3d XYZ0 = XYZpc * (rect[3] * static_cast<double>(planet->getSphereScale()));
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
	return std::atan2(size*planet->getSphereScale()/AU, getJ2000EquatorialPos(core).length()) * M_180_PI;
}

float NomenclatureItem::getAngularSizeRatio(const StelCore *core) const
{
    return static_cast<float>(getAngularSize(core))/core->getProjection(StelCore::FrameJ2000)->getFov();
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

	// check visibility of feature
	Vec3d srcPos;
	const float scale = getAngularSizeRatio(core);
	NomenclatureItem::NomenclatureItemType niType = getNomenclatureType();
	if (painter->getProjector()->projectCheck(XYZ, srcPos) && (equPos.lengthSquared() >= XYZ.lengthSquared())
	    && (scale>0.04f && (scale<0.5f || niType>=NomenclatureItem::niSpecialPointPole )))
	{
		float brightness=(getSolarAltitude(core)<0. ? 0.25f : 1.0f);
		if (niType>=NomenclatureItem::niSpecialPointPole)
			brightness = 0.5f;
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
	double sinH= (sin(ssop.second[0])*sin(latitude*M_PI_180) +
		      cos(ssop.second[0])*cos(latitude*M_PI_180)*sin(colongitude-longitude*M_PI_180));
	double h = abs(sinH)<=1 ? asin(sinH) : M_PI_2 * StelUtils::sign(sinH);
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
    niTypeStringMap = {
	{ niSpecialPointPole, qc_("point", "special point") },
	{ niSpecialPointEast, qc_("point", "special point") },
	{ niSpecialPointWest, qc_("point", "special point") },
	{ niSpecialPointCenter, qc_("point", "special point") },
	{ niSpecialPointSubSolar, qc_("point", "special point") },
	// TRANSLATORS: Geographic area distinguished by amount of reflected light
	{ niAlbedoFeature, qc_("albedo feature", "landform") },
	// TRANSLATORS: Arc-shaped feature
	{ niArcus, qc_("arcus", "landform") },
	// TRANSLATORS: Radial-patterned features on Venus
	{ niAstrum, qc_("astrum", "landform") },
	// TRANSLATORS: Chain of craters
	{ niCatena, qc_("catena", "landform") },
	// TRANSLATORS: Hollows, irregular steep-sided depressions usually in arrays or clusters
	{ niCavus, qc_("cavus", "landform") },
	// TRANSLATORS: Distinctive area of broken terrain
	{ niChaos, qc_("chaos", "landform") },
	// TRANSLATORS: A deep, elongated, steep-sided depression
	{ niChasma, qc_("chasma", "landform") },
	// TRANSLATORS: Small hills or knobs
	{ niCollis, qc_("collis", "landform") },
	// TRANSLATORS: Ovoid-shaped feature
	{ niCorona, qc_("corona", "landform") },
	// TRANSLATORS: A circular depression
	{ niCrater, qc_("crater", "landform") },
	// TRANSLATORS: Ridge
	{ niDorsum, qc_("dorsum", "landform") },
	// TRANSLATORS: Active volcanic centers on Io
	{ niEruptiveCenter, qc_("eruptive center", "landform") },
	// TRANSLATORS: Bright spot
	{ niFacula, qc_("facula", "landform") },
	// TRANSLATORS: Pancake-like structure, or a row of such structures
	{ niFarrum, qc_("farrum", "landform") },
	// TRANSLATORS: A very low curvilinear ridge with a scalloped pattern
	{ niFlexus, qc_("flexus", "landform") },
	// TRANSLATORS: Flow terrain
	{ niFluctus, qc_("fluctus", "landform") },
	// TRANSLATORS: Channel on Titan that might carry liquid
	{ niFlumen, qc_("flumen", "landform") },
	// TRANSLATORS: Strait, a narrow passage of liquid connecting two larger areas of liquid
	{ niFretum, qc_("fretum", "landform") },
	// TRANSLATORS: Long, narrow depression
	{ niFossa, qc_("fossa", "landform") },
	// TRANSLATORS: Island (islands), an isolated land area (or group of such areas) surrounded by, or nearly surrounded by, a liquid area (sea or lake)
	{ niInsula, qc_("insula", "landform") },
	// TRANSLATORS: Landslide
	{ niLabes, qc_("labes", "landform") },
	// TRANSLATORS: Complex of intersecting valleys or ridges
	{ niLabyrinthus, qc_("labyrinthus", "landform") },
	// TRANSLATORS: Irregularly shaped depression on Titan having the appearance of a dry lake bed
	{ niLacuna, qc_("lacuna", "landform") },
	// TRANSLATORS: "Lake" or small plain; on Titan, a "lake" or small, dark plain with discrete, sharp boundaries
	{ niLacus, qc_("lacus", "landform") },
	// TRANSLATORS: Cryptic ringed feature
	{ niLargeRingedFeature, qc_("large ringed feature", "landform") },
	// TRANSLATORS: Small dark spots on Europa
	{ niLenticula, qc_("lenticula", "landform") },
	// TRANSLATORS: A dark or bright elongate marking, may be curved or straight
	{ niLinea, qc_("linea", "landform") },
	// TRANSLATORS: Extension of plateau having rounded lobate or tongue-like boundaries
	{ niLingula, qc_("lingula", "landform") },
	// TRANSLATORS: Dark spot, may be irregular
	{ niMacula, qc_("macula", "landform") },
	// TRANSLATORS: "Sea"; on the Moon, low albedo, relatively smooth plain, generally of large extent; on Mars, dark albedo areas of no known geological significance; on Titan, large expanses of dark materials thought to be liquid hydrocarbons
	{ niMare, qc_("mare", "landform") },
	// TRANSLATORS: A flat-topped prominence with cliff-like edges
	{ niMensa, qc_("mensa", "landform") },
	// TRANSLATORS: Mountain
	{ niMons, qc_("mons", "landform") },
	// TRANSLATORS: A very large dark area on the Moon
	{ niOceanus, qc_("oceanus", "landform") },
	// TRANSLATORS: "Swamp"; small plain
	{ niPalus, qc_("palus", "landform") },
	// TRANSLATORS: An irregular crater, or a complex one with scalloped edges
	{ niPatera, qc_("patera", "landform") },
	// TRANSLATORS: Low plain
	{ niPlanitia, qc_("planitia", "landform") },
	// TRANSLATORS: Plateau or high plain
	{ niPlanum, qc_("planum", "landform") },
	// TRANSLATORS: Cryo-volcanic features on Triton
	{ niPlume, qc_("plume", "landform") },
	// TRANSLATORS: "Cape"; headland promontoria
	{ niPromontorium, qc_("promontorium", "landform") },
	// TRANSLATORS: A large area marked by reflectivity or color distinctions from adjacent areas, or a broad geographic region
	{ niRegio, qc_("regio", "landform") },
	// TRANSLATORS: Reticular (netlike) pattern on Venus
	{ niReticulum, qc_("reticulum", "landform") },
	// TRANSLATORS: Fissure
	{ niRima, qc_("rima", "landform") },
	// TRANSLATORS: Scarp
	{ niRupes, qc_("rupes", "landform") },
	// TRANSLATORS: A feature that shares the name of an associated feature.
	{ niSatelliteFeature, qc_("satellite feature", "landform") },
	// TRANSLATORS: Boulder or rock
	{ niSaxum, qc_("saxum", "landform") },
	// TRANSLATORS: Lobate or irregular scarp
	{ niScopulus, qc_("scopulus", "landform") },
	// TRANSLATORS: Sinuous feature with segments of positive and negative relief along its length
	{ niSerpens, qc_("serpens", "landform") },
	// TRANSLATORS: Subparallel furrows and ridges
	{ niSulcus, qc_("sulcus", "landform") },
	// TRANSLATORS: "Bay"; small plain; on Titan, bays within seas or lakes of liquid hydrocarbons
	{ niSinus, qc_("sinus", "landform") },
	// TRANSLATORS: Extensive land mass
	{ niTerra, qc_("terra", "landform") },
	// TRANSLATORS: Tile-like, polygonal terrain
	{ niTessera, qc_("tessera", "landform") },
	// TRANSLATORS: Small domical mountain or hill
	{ niTholus, qc_("tholus", "landform") },
	// TRANSLATORS: Dunes
	{ niUnda, qc_("unda", "landform") },
	// TRANSLATORS: Valley
	{ niVallis, qc_("vallis", "landform") },
	// TRANSLATORS: Extensive plain
	{ niVastitas, qc_("vastitas", "landform") },
	// TRANSLATORS: A streak or stripe of color
	{ niVirga, qc_("virga", "landform") },
	// TRANSLATORS: Lunar features at or near Apollo landing sites
	{ niLandingSite, qc_("landing site name", "landform") }};

	niTypeDescriptionMap = {
	// TRANSLATORS: Description for landform 'albedo feature'
	{ niAlbedoFeature, q_("Geographic area distinguished by amount of reflected light.")},
	// TRANSLATORS: Description for landform 'arcus'
	{ niArcus, q_("Arc-shaped feature.")},
	// TRANSLATORS: Description for landform 'astrum'
	{ niAstrum, q_("Radial-patterned feature.")},
	// TRANSLATORS: Description for landform 'catena'
	{ niCatena, q_("Chain of craters.")},
	// TRANSLATORS: Description for landform 'cavus'
	{ niCavus, q_("Hollows, irregular steep-sided depressions usually in arrays or clusters.")},
	// TRANSLATORS: Description for landform 'chaos'
	{ niChaos, q_("Distinctive area of broken terrain.")},
	// TRANSLATORS: Description for landform 'chasma'
	{ niChasma, q_("A deep, elongated, steep-sided depression.")},
	// TRANSLATORS: Description for landform 'collis'
	{ niCollis, q_("Small hills or knobs.")},
	// TRANSLATORS: Description for landform 'corona'
	{ niCorona, q_("Ovoid-shaped feature.")},
	// TRANSLATORS: Description for landform 'crater'
	{ niCrater, q_("A circular depression.")},
	// TRANSLATORS: Description for landform 'dorsum'
	{ niDorsum, q_("Ridge.")},
	// TRANSLATORS: Description for landform 'eruptive center'
	{ niEruptiveCenter, q_("Active volcanic center.")},
	// TRANSLATORS: Description for landform 'facula'
	{ niFacula, q_("Bright spot.")},
	// TRANSLATORS: Description for landform 'farrum'
	{ niFarrum, q_("Pancake-like structure, or a row of such structures.")},
	// TRANSLATORS: Description for landform 'flexus'
	{ niFlexus, q_("A very low curvilinear ridge with a scalloped pattern.")},
	// TRANSLATORS: Description for landform 'fluctus'
	{ niFluctus, q_("Flow terrain.")},
	// TRANSLATORS: Description for landform 'flumen'
	{ niFlumen, q_("Channel, that might carry liquid.")},
	// TRANSLATORS: Description for landform 'fretum'
	{ niFretum, q_("Strait, a narrow passage of liquid connecting two larger areas of liquid.")},
	// TRANSLATORS: Description for landform 'fossa'
	{ niFossa, q_("Long, narrow depression.")},
	// TRANSLATORS: Description for landform 'insula'
	{ niInsula, q_("Island, an isolated land area surrounded by, or nearly surrounded by, a liquid area (sea or lake).")},
	// TRANSLATORS: Description for landform 'labes'
	{ niLabes, q_("Landslide.")},
	// TRANSLATORS: Description for landform 'labyrinthus'
	{ niLabyrinthus, q_("Complex of intersecting valleys or ridges.")},
	// TRANSLATORS: Description for landform 'lacuna'
	{ niLacuna, q_("Irregularly shaped depression, having the appearance of a dry lake bed.")},
	// TRANSLATORS: Description for landform 'lacus'
	{ niLacus, q_("'Lake' or small plain.")},
	// TRANSLATORS: Description for landform 'large ringed feature'
	{ niLargeRingedFeature, q_("Cryptic ringed feature.")},
	// TRANSLATORS: Description for landform 'lenticula'
	{ niLenticula, q_("Small dark spot.")},
	// TRANSLATORS: Description for landform 'linea'
	{ niLinea, q_("A dark or bright elongate marking, may be curved or straight.")},
	// TRANSLATORS: Description for landform 'lingula'
	{ niLingula, q_("Extension of plateau having rounded lobate or tongue-like boundaries.")},
	// TRANSLATORS: Description for landform 'macula'
	{ niMacula, q_("Dark spot, may be irregular")},
	// TRANSLATORS: Description for landform 'mare' on the Moon
	{ niMare, q_("'Sea'; low albedo, relatively smooth plain, generally of large extent.")},
	// TRANSLATORS: Description for landform 'mensa'
	{ niMensa, q_("A flat-topped prominence with cliff-like edges.")},
	// TRANSLATORS: Description for landform 'mons'
	{ niMons, q_("Mountain.")},
	// TRANSLATORS: Description for landform 'oceanus'
	{ niOceanus, q_("A very large dark area.")},
	// TRANSLATORS: Description for landform 'palus'
	{ niPalus, q_("'Swamp'; small plain.")},
	// TRANSLATORS: Description for landform 'patera'
	{ niPatera, q_("An irregular crater, or a complex one with scalloped edges.")},
	// TRANSLATORS: Description for landform 'planitia'
	{ niPlanitia, q_("Low plain.")},
	// TRANSLATORS: Description for landform 'planum'
	{ niPlanum, q_("Plateau or high plain.")},
	// TRANSLATORS: Description for landform 'plume'
	{ niPlume, q_("Cryo-volcanic feature.")},
	// TRANSLATORS: Description for landform 'promontorium'
	{ niPromontorium, q_("'Cape'; headland promontoria.")},
	// TRANSLATORS: Description for landform 'regio'
	{ niRegio, q_("A large area marked by reflectivity or color distinctions from adjacent areas, or a broad geographic region.")},
	// TRANSLATORS: Description for landform 'reticulum'
	{ niReticulum, q_("Reticular (netlike) pattern.")},
	// TRANSLATORS: Description for landform 'rima'
	{ niRima, q_("Fissure.")},
	// TRANSLATORS: Description for landform 'rupes'
	{ niRupes, q_("Scarp.")},
	// TRANSLATORS: Description for landform 'satellite feature'
	{ niSatelliteFeature, q_("A feature that shares the name of an associated feature.")},
	// TRANSLATORS: Description for landform 'saxum'
	{ niSaxum, q_("Boulder or rock.")},
	// TRANSLATORS: Description for landform 'scopulus'
	{ niScopulus, q_("Lobate or irregular scarp.")},
	// TRANSLATORS: Description for landform 'serpens'
	{ niSerpens, q_("Sinuous feature with segments of positive and negative relief along its length.")},
	// TRANSLATORS: Description for landform 'sinus'
	{ niSinus, q_("'Bay'; small plain.")},
	// TRANSLATORS: Description for landform 'sulcus'
	{ niSulcus, q_("Subparallel furrows and ridges.")},
	// TRANSLATORS: Description for landform 'terra'
	{ niTerra, q_("Extensive land mass.")},
	// TRANSLATORS: Description for landform 'tessera'
	{ niTessera, q_("Tile-like, polygonal terrain.")},
	// TRANSLATORS: Description for landform 'tholus'
	{ niTholus, q_("Small domical mountain or hill.")},
	// TRANSLATORS: Description for landform 'unda'
	{ niUnda, q_("Dunes.")},
	// TRANSLATORS: Description for landform 'vallis'
	{ niVallis, q_("Valley.")},
	// TRANSLATORS: Description for landform 'vastitas'
	{ niVastitas, q_("Extensive plain.")},
	// TRANSLATORS: Description for landform 'virga'
	{ niVirga, q_("A streak or stripe of color.")},
	{ niLandingSite, ""},
	{ niSpecialPointPole, ""},
	{ niSpecialPointEast, ""},
	{ niSpecialPointWest, ""},
	{ niSpecialPointCenter, ""},
	{ niSpecialPointSubSolar, ""}};
}
