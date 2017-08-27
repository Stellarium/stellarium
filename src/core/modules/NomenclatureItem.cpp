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

QMap<NomenclatureItem::NomenclatureItemType, QString> NomenclatureItem::niTypeMap;

NomenclatureItem::NomenclatureItem(PlanetP nPlanet,
				   const QString& nId,
				   const QString& nName,
				   const QString& nItemType,
				   float nLatitude,
				   float nLongitude,
				   float nSize)
	: initialized(false)
	, planet(nPlanet)
	, identificator(nId)
	, englishName(nName)
	, nameI18n(nName)
	, nType(nItemType)
	, latitude(nLatitude)
	, longitude(nLongitude)
	, size(nSize)
{
	niType = niTypeMap.key(nType, NomenclatureItemType::niUNDEFINED);
	initialized = true;
}

NomenclatureItem::~NomenclatureItem()
{
}

void NomenclatureItem::init()
{
	if (niTypeMap.count() > 0 )
	{
		// This should never happen. But it's uncritical.
		qDebug() << "NomenclatureItem::init(): Non-empty static map. This is a programming error, but we can fix that.";
		niTypeMap.clear();
	}
	niTypeMap.insert(NomenclatureItem::niAlbedoFeature,	 "albedo feature");
	niTypeMap.insert(NomenclatureItem::niArcus,		 "arcus");
	niTypeMap.insert(NomenclatureItem::niCatena,		 "catena");
	niTypeMap.insert(NomenclatureItem::niCavus,		 "cavus");
	niTypeMap.insert(NomenclatureItem::niChaos,		 "chaos");
	niTypeMap.insert(NomenclatureItem::niChasma,		 "chasma");
	niTypeMap.insert(NomenclatureItem::niCollis,		 "collis");
	niTypeMap.insert(NomenclatureItem::niCorona,		 "corona");
	niTypeMap.insert(NomenclatureItem::niCrater,		 "crater");
	niTypeMap.insert(NomenclatureItem::niDorsum,		 "dorsum");
	niTypeMap.insert(NomenclatureItem::niEruptiveCenter,	 "eruptive center");
	niTypeMap.insert(NomenclatureItem::niFlexus,		 "flexus");
	niTypeMap.insert(NomenclatureItem::niFluctus,		 "fluctus");
	niTypeMap.insert(NomenclatureItem::niFlumen,		 "flumen");
	niTypeMap.insert(NomenclatureItem::niFretum,		 "fretum");
	niTypeMap.insert(NomenclatureItem::niFossa,		 "fossa");
	niTypeMap.insert(NomenclatureItem::niInsula,		 "insula");
	niTypeMap.insert(NomenclatureItem::niLabes,		 "labes");
	niTypeMap.insert(NomenclatureItem::niLabyrinthus,	 "labyrinthus");
	niTypeMap.insert(NomenclatureItem::niLacuna,		 "lacuna");
	niTypeMap.insert(NomenclatureItem::niLacus,		 "lacus");
	niTypeMap.insert(NomenclatureItem::niLargeRingedFeature, "large ringed feature");
	niTypeMap.insert(NomenclatureItem::niLinea,		 "linea");
	niTypeMap.insert(NomenclatureItem::niLingula,		 "lingula");
	niTypeMap.insert(NomenclatureItem::niMacula,		 "macula");
	niTypeMap.insert(NomenclatureItem::niMare,		 "mare");
	niTypeMap.insert(NomenclatureItem::niMensa,		 "mensa");
	niTypeMap.insert(NomenclatureItem::niMons,		 "mons");
	niTypeMap.insert(NomenclatureItem::niOceanus,		 "oceanus");
	niTypeMap.insert(NomenclatureItem::niPalus,		 "palus");
	niTypeMap.insert(NomenclatureItem::niPatera,		 "patera");
	niTypeMap.insert(NomenclatureItem::niPlanitia,		 "planitia");
	niTypeMap.insert(NomenclatureItem::niPlanum,		 "planum");
	niTypeMap.insert(NomenclatureItem::niPlume,		 "plume");
	niTypeMap.insert(NomenclatureItem::niPromontorium,	 "promontorium");
	niTypeMap.insert(NomenclatureItem::niRegio,		 "regio");
	niTypeMap.insert(NomenclatureItem::niRima,		 "rima");
	niTypeMap.insert(NomenclatureItem::niRupes,		 "rupes");
	niTypeMap.insert(NomenclatureItem::niScopulus,		 "scopulus");
	niTypeMap.insert(NomenclatureItem::niSerpens,		 "serpens");
	niTypeMap.insert(NomenclatureItem::niSulcus,		 "sulcus");
	niTypeMap.insert(NomenclatureItem::niSinus,		 "sinus");
	niTypeMap.insert(NomenclatureItem::niTerra,		 "terra");
	niTypeMap.insert(NomenclatureItem::niTholus,		 "tholus");
	niTypeMap.insert(NomenclatureItem::niUnda,		 "unda");
	niTypeMap.insert(NomenclatureItem::niVallis,		 "vallis");
	niTypeMap.insert(NomenclatureItem::niVastitas,		 "vastitas");
	niTypeMap.insert(NomenclatureItem::niVirga,		 "virga");
	niTypeMap.insert(NomenclatureItem::niLandingSite,	 "landing site");
	niTypeMap.insert(NomenclatureItem::niUNDEFINED,		 "UNDEFINED"); // something must be broken before we ever see this!
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
	nameI18n = trans.qtranslate(englishName);
}

QString NomenclatureItem::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
		oss << "<h2>" << getNameI18n() << "</h2>";

	if (flags&ObjectType && getNomenclatureType()!=NomenclatureItem::niUNDEFINED)
		oss << QString("%1: <b>%2</b>").arg(q_("Type")).arg(q_(getNomenclatureTypeString())) << "<br />";

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Extra)
		oss << QString("%1: %2/%3").arg(q_("Planetocentric coordinates")).arg(StelUtils::decDegToDmsStr(latitude)).arg(StelUtils::decDegToDmsStr(longitude)) << "<br />";

	if (flags&Size)
		oss << QString("%1: %2 %3").arg(q_("Size")).arg(QString::number(size, 'f', 2)).arg(qc_("km", "distance")) << "<br />";

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
	//qWarning() << "Planet:" << planet->getEnglishName() << " LF:" << labelsFader;
	if (!getFlagLabels())
		return;

	// Get latitude and longitude of Moon in J2000
	Vec3d coord = planet->getJ2000EquatorialPos(core);
	Vec3d srcPos;

	// Change coordinates from planetocentric to equatorial
	/*
	- Distance from Earth to feature -> R
	- Latitude of feature -> latitude
	- Longitude of feature -> longitude
	*/

	double R = sqrt(coord.length()*coord.length() + planet->getRadius()*planet->getRadius());
	double clatitude = asin( (planet->getRadius()*sin(latitude) + coord.length()*sin(coord.latitude()))/R );
	double clongitude = atan( (planet->getRadius()*cos(latitude)*sin(longitude) + coord.length()*cos(coord.latitude())*sin(coord.longitude()))/(planet->getRadius()*cos(latitude)*cos(longitude) + coord.length()*cos(coord.latitude())*cos(coord.longitude())) );

	// From spherical to cartesian coordinates
	// The arguments of trigonometric functions must be in radians?
	XYZ[0] = planet->getRadius() * cos(clongitude) * cos(clatitude);
	XYZ[1] = planet->getRadius() * sin(clongitude) * cos(clatitude);
	XYZ[2] = planet->getRadius() * sin(clatitude);

	painter->setColor(color[0], color[1], color[2], 1.0);
	painter->getProjector()->project(XYZ, srcPos);
	painter->drawCircle(srcPos[0], srcPos[1], 2.f);
	painter->drawText(srcPos[0], srcPos[1], getNameI18n(), 0, 5.f, 5.f, false);
}
