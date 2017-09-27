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

NomenclatureItem::NomenclatureItem(PlanetP nPlanet,
				   int nId,
				   const QString& nName,
				   const QString& nContext,
				   NomenclatureItemType nItemType,
				   float nLatitude,
				   float nLongitude,
				   float nSize)
	: initialized(false)
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
	initialized = true;
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
			// TRANSLATORS: “Sea”; on the Moon, low albedo, relatively smooth plain, generally of large extent; on Mars, dark albedo areas of no known geological significance; on Titan, large expanses of dark materials thought to be liquid hydrocarbons
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

QString NomenclatureItem::getNomenclatureTypeDescription() const
{
	QString nTypeStr = "";
	switch (nType)
	{
		case niAlbedoFeature:
			nTypeStr = q_("Geographic area distinguished by amount of reflected light.");
			break;
		case niArcus:
			nTypeStr = q_("Arc-shaped feature.");
			break;
		case niAstrum:
			nTypeStr = q_("Radial-patterned feature.");
			break;
		case niCatena:
			nTypeStr = q_("Chain of craters.");
			break;
		case niCavus:
			nTypeStr = q_("Hollows, irregular steep-sided depressions usually in arrays or clusters.");
			break;
		case niChaos:
			nTypeStr = q_("Distinctive area of broken terrain.");
			break;
		case niChasma:
			nTypeStr = q_("A deep, elongated, steep-sided depression.");
			break;
		case niCollis:
			nTypeStr = q_("Small hills or knobs.");
			break;
		case niCorona:
			nTypeStr = q_("Ovoid-shaped feature.");
			break;
		case niCrater:
			nTypeStr = q_("A circular depression.");
			break;
		case niDorsum:
			nTypeStr = q_("Ridge.");
			break;
		case niEruptiveCenter:
			nTypeStr = q_("Active volcanic center.");
			break;
		case niFacula:
			nTypeStr = q_("Bright spot.");
			break;
		case niFarrum:
			nTypeStr = q_("Pancake-like structure, or a row of such structures.");
			break;
		case niFlexus:
			nTypeStr = q_("A very low curvilinear ridge with a scalloped pattern.");
			break;
		case niFluctus:
			nTypeStr = q_("Flow terrain.");
			break;
		case niFlumen:
			nTypeStr = q_("Channel, that might carry liquid.");
			break;
		case niFretum:
			nTypeStr = q_("Strait, a narrow passage of liquid connecting two larger areas of liquid.");
			break;
		case niFossa:
			nTypeStr = q_("Long, narrow depression.");
			break;
		case niInsula:
			nTypeStr = q_("Island, an isolated land area surrounded by, or nearly surrounded by, a liquid area (sea or lake).");
			break;
		case niLabes:
			nTypeStr = q_("Landslide.");
			break;
		case niLabyrinthus:
			nTypeStr = q_("Complex of intersecting valleys or ridges.");
			break;
		case niLacuna:
			nTypeStr = q_("Irregularly shaped depression, which having the appearance of a dry lake bed.");
			break;
		case niLacus:
			if (planet->getEnglishName()=="Titan")
				nTypeStr = q_("'Lake' or small, dark plain with discrete, sharp boundaries.");
			else
				nTypeStr = q_("'Lake' or small plain.");
			break;
		case niLargeRingedFeature:
			nTypeStr = q_("Cryptic ringed feature.");
			break;
		case niLenticula:
			nTypeStr = q_("Small dark spot.");
			break;
		case niLinea:
			nTypeStr = q_("A dark or bright elongate marking, may be curved or straight.");
			break;
		case niLingula:
			nTypeStr = q_("Extension of plateau having rounded lobate or tongue-like boundaries.");
			break;
		case niMacula:
			nTypeStr = q_("Dark spot, may be irregular");
			break;
		case niMare:
		{
			QString p = planet->getEnglishName();
			if (p=="Moon")
				nTypeStr = q_("'Sea'; low albedo, relatively smooth plain, generally of large extent.");
			if (p=="Mars")
				nTypeStr = q_("'Sea'; dark albedo areas of no known geological significance.");
			if (p=="Titan")
				nTypeStr = q_("'Sea'; large expanses of dark materials thought to be liquid hydrocarbons");
			break;
		}
		case niMensa:
			nTypeStr = q_("A flat-topped prominence with cliff-like edges.");
			break;
		case niMons:
			nTypeStr = q_("Mountain.");
			break;
		case niOceanus:
			nTypeStr = q_("A very large dark area.");
			break;
		case niPalus:
			nTypeStr = q_("'Swamp'; small plain.");
			break;
		case niPatera:
			nTypeStr = q_("An irregular crater, or a complex one with scalloped edges.");
			break;
		case niPlanitia:
			nTypeStr = q_("Low plain.");
			break;
		case niPlanum:
			nTypeStr = q_("Plateau or high plain.");
			break;
		case niPlume:
			nTypeStr = q_("Cryo-volcanic feature.");
			break;
		case niPromontorium:
			nTypeStr = q_("'Cape'; headland promontoria.");
			break;
		case niRegio:
			nTypeStr = q_("A large area marked by reflectivity or color distinctions from adjacent areas, or a broad geographic region.");
			break;
		case niReticulum:
			nTypeStr = q_("Reticular (netlike) pattern.");
			break;
		case niRima:
			nTypeStr = q_("Fissure.");
			break;
		case niRupes:
			nTypeStr = q_("Scarp.");
			break;
		case niSatelliteFeature:
			nTypeStr = q_("A feature that shares the name of an associated feature.");
			break;
		case niScopulus:
			nTypeStr = q_("Lobate or irregular scarp.");
			break;
		case niSerpens:
			nTypeStr = q_("Sinuous feature with segments of positive and negative relief along its length.");
			break;
		case niSulcus:
			nTypeStr = q_("Subparallel furrows and ridges.");
			break;
		case niSinus:
			if (planet->getEnglishName()=="Titan")
				nTypeStr = q_("'Bay'; bays within seas or lakes of liquid hydrocarbons.");
			else
				nTypeStr = q_("'Bay'; small plain.");
			break;
		case niTerra:
			nTypeStr = q_("Extensive land mass.");
			break;
		case niTessera:
			nTypeStr = q_("Tile-like, polygonal terrain.");
			break;
		case niTholus:
			nTypeStr = q_("Small domical mountain or hill.");
			break;
		case niUnda:
			nTypeStr = q_("Dunes.");
			break;
		case niVallis:
			nTypeStr = q_("Valley.");
			break;
		case niVastitas:
			nTypeStr = q_("Extensive plain.");
			break;
		case niVirga:
			nTypeStr = q_("A streak or stripe of color.");
			break;
		case niLandingSite:
			nTypeStr = q_("Lunar features at or near Apollo landing sites.");
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
	if (getFlagLabels())
		return StelObject::getSelectPriority(core)-25.f;
	else
		return StelObject::getSelectPriority(core)-2.f;
}

QString NomenclatureItem::getNameI18n() const
{
	return nameI18n;
}

QString NomenclatureItem::getEnglishName() const
{
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
		oss << "<h2>" << getNameI18n() << "</h2>";

	if (flags&ObjectType && getNomenclatureType()!=NomenclatureItem::niUNDEFINED)
		oss << QString("%1: <b>%2</b>").arg(q_("Type")).arg(getNomenclatureTypeString()) << "<br />";

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Size && size>0.f)
		oss << QString("%1: %2 %3").arg(q_("Diameter")).arg(QString::number(size, 'f', 2)).arg(qc_("km", "distance")) << "<br />";

	if (flags&Extra)
	{
		oss << QString("%1: %2/%3").arg(q_("Planetographic longitude/latitude")).arg(StelUtils::decDegToDmsStr(longitude)).arg(StelUtils::decDegToDmsStr(latitude)) << "<br />";
		if (getNomenclatureType()!=NomenclatureItem::niUNDEFINED)
			oss << QString("%1: %2").arg(q_("Landform description"), getNomenclatureTypeDescription()) << "<br />";
	}

	postProcessInfoString(str, flags);
	return str;
}

Vec3f NomenclatureItem::getInfoColor(void) const
{
	return color;
}

float NomenclatureItem::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core);
	return 99.f;
}

double NomenclatureItem::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void NomenclatureItem::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void NomenclatureItem::draw(StelCore* core, StelPainter *painter)
{
<<<<<<< TREE
    //qWarning() << "Planet:" << planet->getEnglishName() << " LF:" << labelsFader;
    if (!getFlagLabels())
        return;
    
    // GZ: I reject smaller craters for now, until geometric computations have been corrected.
    // TODO: Later, size should be adjusted with planet screen size so that not too many labels are visible.
    if ((nType==NomenclatureItemType::niSatelliteFeature) || (nType==NomenclatureItemType::niCrater && size<500))
        return;
    
    Vec3d srcPos;
    Vec3d XYZ0; // AW: XYZ is gobal variable with equatorial J2000.0 coordinates
    
    // Calculate the radius of the planet. It is necessary to re-scale it
    double r = planet->getRadius() * planet->getSphereScale();
    
    // Latitude and longitude of the feature must be in radians in order to use them in trigonometric functions
    double nlatitude = latitude * M_PI/180.0;
    double nlongitude = (longitude + planet->getSiderealTime(core->getJD(), core->getJDE())) * M_PI/180.0;
    
    // The data contains the latitude and longitude of features => angles => spherical coordinates. So, we have to convert them into cartesian coordinates
    XYZ0[0] = r * cos(nlatitude) * cos(nlongitude);
    XYZ0[1] = r * cos(nlatitude) * sin(nlongitude);
    XYZ0[2] = r * sin(nlatitude);
    
    /* We have to calculate feature's coordinates in VSOP87 (this is Ecliptic J2000 coordinates). Feature's original coordinates are in planetocentric system, so we have to multiply it by the rotation matrix.
     planet->getRotEquatorialToVsop87() gives us the rotation matrix between Equatorial (on date) coordinates and Ecliptic J2000 coordinates. So we have to make another change to obtain the rotation matrix using Equatorial J2000: we have to multiplay by core->matVsop87ToJ2000 */
    XYZ = planet->getJ2000EquatorialPos(core) + (core->matVsop87ToJ2000 * planet->getRotEquatorialToVsop87()) * XYZ0;
    
    Vec3d coord = planet->getEclipticPos();
    
    
    // Angular size of feature
    double angSizeFeature = 2*atan(size/(2*coord.length()));
    // Angular size of planet or moon
    double angSizePlanet = planet->getAngularSize(core);
    
    // It is necessary to "turn off" the names whose features are on the opposite face of the planet
    /* OPTION 1 */
    /*Vec3d XYZ1, XYZ2;
    // Cartesian coordinates of the planet
    XYZ1[0] = r * cos(coord.latitude()) * cos(coord.longitude());
    XYZ1[1] = r * cos(coord.latitude()) * sin(coord.longitude());
    XYZ1[2] = r * sin(coord.latitude());
    // Distance from the feature to the observer
    XYZ2[0] = XYZ[0] + XYZ1[0];
    XYZ2[1] = XYZ[1] + XYZ1[1];
    XYZ2[2] = XYZ[2] + XYZ1[2];
    double a = XYZ2.length();
    // If a is bigger than dist, then the feature is on the opposite face of the planet
    if (a > dist)
        return;*/
    /* OPTION 2 */
    // Distance from center of observer's planet to feature
    /*double dist = r/sin(0.5*planet->getAngularSize(core));
    if (XYZ.length() > dist)
        return;*/
    /* OPTION 3 */
    double dist = r/sin(0.5*planet->getAngularSize(core));
    if (XYZ.length() > coord.length())
        return;
    /* OPTION 4 */
    /*double dist = coord.length()/cos(atan(r/coord.length()));
    if (XYZ.length() > coord.length())
        return;*/
    else
    {
        if (painter->getProjector()->projectCheck(XYZ, srcPos))
        {
            painter->setColor(color[0], color[1], color[2], 1.0);
            painter->drawCircle(srcPos[0], srcPos[1], 2.f);
            painter->drawText(srcPos[0], srcPos[1], getNameI18n(), 0, 5.f, 5.f, false);
        }
    }
}
