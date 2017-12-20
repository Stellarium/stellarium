/*
 * Copyright (C) 2012 Alexander Wolf
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


#include "Exoplanet.hpp"
#include "Exoplanets.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelLocaleMgr.hpp"
#include "StarMgr.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QVariant>
#include <QList>

const QString Exoplanet::EXOPLANET_TYPE=QStringLiteral("Exoplanet");
StelTextureSP Exoplanet::markerTexture;
bool Exoplanet::distributionMode = false;
bool Exoplanet::timelineMode = false;
bool Exoplanet::habitableMode = false;
bool Exoplanet::showDesignations = false;
Vec3f Exoplanet::exoplanetMarkerColor = Vec3f(0.4f,0.9f,0.5f);
Vec3f Exoplanet::habitableExoplanetMarkerColor = Vec3f(1.f,0.5f,0.f);
int Exoplanet::temperatureScaleID = 1;

Exoplanet::Exoplanet(const QVariantMap& map)
	: initialized(false)
	, EPCount(0)
	, PHEPCount(0)
	, designation("")
	, RA(0.)
	, DE(0.)
	, distance(0.)
	, stype("")
	, smass(0.)
	, smetal(0.)
	, Vmag(99.)
	, sradius(0.)
	, effectiveTemp(0)
	, hasHabitableExoplanets(false)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation"))
		return;

	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
		
	designation  = map.value("designation").toString();
	starProperName = map.value("starProperName").toString();
	RA = StelUtils::getDecAngle(map.value("RA").toString());
	DE = StelUtils::getDecAngle(map.value("DE").toString());
	distance = map.value("distance").toFloat();
	stype = map.value("stype").toString();
	smass = map.value("smass").toFloat();
	smetal = map.value("smetal").toFloat();
	Vmag = map.value("Vmag", 99.f).toFloat();
	sradius = map.value("sradius").toFloat();
	effectiveTemp = map.value("effectiveTemp").toInt();
	hasHabitableExoplanets = map.value("hasHP", false).toBool();

	EPCount=0;
	PHEPCount=0;
	eccentricityList.clear();
	semiAxisList.clear();
	massList.clear();
	radiusList.clear();
	periodList.clear();
	angleDistanceList.clear();
	englishNames.clear();
	translatedNames.clear();
	exoplanetDesignations.clear();
	effectiveTempHostStarList.clear();
	yearDiscoveryList.clear();
	metallicityHostStarList.clear();
	vMagHostStarList.clear();
	raHostStarList.clear();
	decHostStarList.clear();
	distanceHostStarList.clear();
	massHostStarList.clear();
	radiusHostStarList.clear();
	if (map.contains("exoplanets"))
	{
		foreach(const QVariant &expl, map.value("exoplanets").toList())
		{
			QVariantMap exoplanetMap = expl.toMap();
			exoplanetData p;
			EPCount++;
			if (exoplanetMap.contains("planetName"))
			{
				p.planetName = exoplanetMap.value("planetName").toString();
				QString epd = QString("%1 %2").arg(designation, p.planetName);
				exoplanetDesignations.append(epd);
			}
			if (exoplanetMap.contains("planetProperName"))
			{
				p.planetProperName = exoplanetMap.value("planetProperName").toString();
				englishNames.append(p.planetProperName);
				translatedNames.append(trans.qtranslate(p.planetProperName));
			}
			p.period = exoplanetMap.value("period", -1.f).toFloat();
			p.mass = exoplanetMap.value("mass", -1.f).toFloat();
			p.radius = exoplanetMap.value("radius", -1.f).toFloat();
			p.semiAxis = exoplanetMap.value("semiAxis", -1.f).toFloat();
			p.eccentricity = exoplanetMap.value("eccentricity", -1.f).toFloat();
			p.inclination = exoplanetMap.value("inclination", -1.f).toFloat();
			p.angleDistance = exoplanetMap.value("angleDistance", -1.f).toFloat();
			p.discovered = exoplanetMap.value("discovered", 0).toInt();
			p.pclass = exoplanetMap.value("pclass", "").toString();			
			if (!p.pclass.isEmpty())
				PHEPCount++;			
			p.EqTemp = exoplanetMap.value("EqTemp", -1).toInt();
			p.flux = exoplanetMap.value("flux", -1).toInt();
			p.ESI = exoplanetMap.value("ESI", -1).toInt();
			p.detectionMethod = exoplanetMap.value("detectionMethod", "").toString();
			p.conservative = exoplanetMap.value("conservative", false).toBool();

			exoplanets.append(p);

			if (p.eccentricity>0)
				eccentricityList.append(p.eccentricity);
			else
				eccentricityList.append(0);

			if (p.semiAxis>0)
				semiAxisList.append(p.semiAxis);
			else
				semiAxisList.append(0);

			if (p.mass>0)
				massList.append(p.mass);
			else
				massList.append(0);

			if (p.radius>0)
				radiusList.append(p.radius);
			else
				radiusList.append(0);

			if (p.angleDistance>0)
				angleDistanceList.append(p.angleDistance);
			else
				angleDistanceList.append(0);

			if (p.period>0)
				periodList.append(p.period);
			else
				periodList.append(0);

			if (p.discovered>0)
				yearDiscoveryList.append(p.discovered);

			effectiveTempHostStarList.append(effectiveTemp);
			metallicityHostStarList.append(smetal);
			if (Vmag<99)
				vMagHostStarList.append(Vmag);
			raHostStarList.append(RA);
			decHostStarList.append(DE);
			distanceHostStarList.append(distance);
			massHostStarList.append(smass);
			radiusHostStarList.append(sradius);
		}
	}

	initialized = true;
}

Exoplanet::~Exoplanet()
{
	//
}

QVariantMap Exoplanet::getMap(void) const
{
	QVariantMap map;
	map["designation"] = designation;
	if (!starProperName.isEmpty()) map["starProperName"] = starProperName;
	map["RA"] = RA;
	map["DE"] = DE;
	map["distance"] = distance;
	map["stype"] = stype;
	map["smass"] = smass;
	map["smetal"] = smetal;
	map["Vmag"] = Vmag;
	map["sradius"] = sradius;
	map["effectiveTemp"] = effectiveTemp;
	map["hasHP"] = hasHabitableExoplanets;
	QVariantList exoplanetList;
	foreach(const exoplanetData &p, exoplanets)
	{
		QVariantMap explMap;
		explMap["planetName"] = p.planetName;
		if (!p.planetProperName.isEmpty()) explMap["planetProperName"] = p.planetProperName;
		if (p.mass > -1.f) explMap["mass"] = p.mass;
		if (p.period > -1.f) explMap["period"] = p.period;
		if (p.radius > -1.f) explMap["radius"] = p.radius;
		if (p.semiAxis > -1.f) explMap["semiAxis"] = p.semiAxis;
		if (p.inclination > -1.f) explMap["inclination"] = p.inclination;
		if (p.eccentricity > -1.f) explMap["eccentricity"] = p.eccentricity;
		if (p.angleDistance > -1.f) explMap["angleDistance"] = p.angleDistance;
		if (p.discovered > 0) explMap["discovered"] = p.discovered;
		if (!p.pclass.isEmpty()) explMap["pclass"] = p.pclass;		
		if (p.EqTemp > 0) explMap["EqTemp"] = p.EqTemp;
		if (p.flux > 0) explMap["flux"] = p.flux;
		if (p.ESI > 0) explMap["ESI"] = p.ESI;
		if (!p.detectionMethod.isEmpty()) explMap["detectionMethod"] = p.detectionMethod;
		if (p.conservative) explMap["conservative"] = p.conservative;
		exoplanetList << explMap;
	}
	map["exoplanets"] = exoplanetList;

	return map;
}

float Exoplanet::getSelectPriority(const StelCore *core) const
{
	return StelObject::getSelectPriority(core)-2.f;
}

QString Exoplanet::getNameI18n(void) const
{
	// Use SkyTranslator for translation star names
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	QString name = trans.qtranslate(designation);
	if (!starProperName.isEmpty())
		name = trans.qtranslate(starProperName);

	return name;
}

QString Exoplanet::getEnglishName(void) const
{
	QString name = designation;
	if (!starProperName.isEmpty())
		name = starProperName;

	return name;
}

QString Exoplanet::getDesignation(void) const
{
	return designation;
}

QStringList Exoplanet::getExoplanetsEnglishNames() const
{
	return englishNames;
}

QStringList Exoplanet::getExoplanetsNamesI18n() const
{
	return translatedNames;
}

QStringList Exoplanet::getExoplanetsDesignations() const
{
	return exoplanetDesignations;
}

QString Exoplanet::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	if (flags&Name)
	{
		QString systemName = getNameI18n();
		if (!starProperName.isEmpty())
			systemName.append(QString(" (%1)").arg(designation));

		oss << "<h2>" << systemName << "</h2>";
	}
	
	if (flags&ObjectType)
	{
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_("planetary system")) << "<br />";
	}

	if (flags&Magnitude)
	{
		if (Vmag<99 && !distributionMode)
		{
			QString emag = "";
			if (core->getSkyDrawer()->getFlagHasAtmosphere())
				emag = QString(" (%1: <b>%2</b>)").arg(q_("extincted to"), QString::number(getVMagnitudeWithExtinction(core), 'f', 2));

			oss << QString("%1: <b>%2</b>%3").arg(q_("Magnitude"), QString::number(getVMagnitude(core), 'f', 2), emag) << "<br />";
		}
	}

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Extra && !stype.isEmpty())
		oss <<  QString("%1: <b>%2</b>").arg(q_("Spectral Type"), stype) << "<br />";

	if (flags&Distance && distance>0)
	{
		//TRANSLATORS: Unit of measure for distance - Light Years
		QString ly = qc_("ly", "distance");
		oss << QString("%1: %2 %3").arg(q_("Distance"), QString::number(distance/0.306601, 'f', 2), ly) << "<br />";
	}

	if (flags&Extra)
	{
		if (smetal!=0)
		{
			oss << QString("%1 [Fe/H]: %2").arg(q_("Metallicity"), QString::number(smetal, 'f', 3)) << "<br />";
		}
		if (smass>0)
		{
			oss << QString("%1: %2 M<sub>%3</sub>").arg(q_("Mass"), QString::number(smass, 'f', 3), q_("Sun")) << "<br />";
		}
		if (sradius>0)
		{
			oss << QString("%1: %2 R<sub>%3</sub>").arg(q_("Radius"), QString::number(sradius, 'f', 5), q_("Sun")) << "<br />";
		}
		if (effectiveTemp>0)
		{
			oss << QString("%1: %2 %3").arg(q_("Effective temperature")).arg(effectiveTemp).arg(qc_("K", "temperature")) << "<br />";
		}
		if (exoplanets.size() > 0)
		{
			QString planetNameLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Exoplanet"));
			QString planetProperNameLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Name"));
			QString periodLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (%2)</td>").arg(q_("Period")).arg(q_("days"));
			QString massLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (M<sub>%2</sub>)</td>").arg(q_("Mass")).arg(q_("Jup"));
			QString radiusLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (R<sub>%2</sub>)</td>").arg(q_("Radius")).arg(q_("Jup"));
			QString semiAxisLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (%2)</td>").arg(q_("Semi-Major Axis")).arg(qc_("AU", "distance, astronomical unit"));
			QString eccentricityLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Eccentricity"));
			QString inclinationLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (%2)</td>").arg(q_("Inclination")).arg(QChar(0x00B0));
			QString angleDistanceLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (\")</td>").arg(q_("Angle Distance"));
			QString discoveredLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Discovered year"));
			QString detectionMethodLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Detection method"));
			QString pClassLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Planetary class"));
			//TRANSLATORS: Full phrase is "Equilibrium Temperature"
			QString equilibriumTempLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (%2)</td>").arg(q_("Equilibrium temp.")).arg(getTemperatureScaleUnit());
			//TRANSLATORS: Average stellar flux of the planet
			QString fluxLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (S<sub>E</sub>)</td>").arg(q_("Flux"));
			//TRANSLATORS: ESI = Earth Similarity Index
			QString ESILabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("ESI"));			
			foreach(const exoplanetData &p, exoplanets)
			{
				if (!p.planetName.isEmpty())
				{
					planetNameLabel.append("<td style=\"padding:0 2px;\">").append(p.planetName).append("</td>");
				}
				else
				{
					planetNameLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (!p.planetProperName.isEmpty())
				{
					planetProperNameLabel.append("<td style=\"padding:0 2px;\">").append(trans.qtranslate(p.planetProperName)).append("</td>");
				}
				else
				{
					planetProperNameLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.period > -1.f)
				{
					periodLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.period, 'f', 2)).append("</td>");
				}
				else
				{
					periodLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.mass > -1.f)
				{
					massLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.mass, 'f', 2)).append("</td>");
				}
				else
				{
					massLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.radius > -1.f)
				{
					radiusLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.radius, 'f', 1)).append("</td>");
				}
				else
				{
					radiusLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.eccentricity > -1.f)
				{
					eccentricityLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.eccentricity, 'f', 3)).append("</td>");
				}
				else
				{
					eccentricityLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.inclination > -1.f)
				{
					inclinationLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.inclination, 'f', 1)).append("</td>");
				}
				else
				{
					inclinationLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.semiAxis > -1.f)
				{
					semiAxisLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.semiAxis, 'f', 4)).append("</td>");
				}
				else
				{
					semiAxisLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.angleDistance > -1.f)
				{
					angleDistanceLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.angleDistance, 'f', 6)).append("</td>");
				}
				else
				{
					angleDistanceLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.discovered > 0)
				{
					discoveredLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.discovered)).append("</td>");
				}
				else
				{
					discoveredLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (!p.pclass.isEmpty())
				{
					if (!p.conservative)
						pClassLabel.append("<td style=\"padding:0 2px;\"><em>").append(getPlanetaryClassI18n(p.pclass)).append("</em></td>");
					else
						pClassLabel.append("<td style=\"padding:0 2px;\">").append(getPlanetaryClassI18n(p.pclass)).append("</td>");
				}
				else
				{
					pClassLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}				
				if (p.EqTemp > 0)
				{
					if (!p.conservative)
						equilibriumTempLabel.append("<td style=\"padding:0 2px;\"><em>").append(QString::number(getTemperature(p.EqTemp), 'f', 2)).append("</em></td>");
					else
						equilibriumTempLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(getTemperature(p.EqTemp), 'f', 2)).append("</td>");
				}
				else
				{
					equilibriumTempLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.flux > 0)
				{
					if (!p.conservative)
						fluxLabel.append("<td style=\"padding:0 2px;\"><em>").append(QString::number(p.flux * 0.01, 'f', 2)).append("</em></td>");
					else
						fluxLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.flux * 0.01, 'f', 2)).append("</td>");
				}
				else
				{
					fluxLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.ESI > 0)
				{
					if (!p.conservative)
						ESILabel.append("<td style=\"padding:0 2px;\"><em>").append(QString::number(p.ESI * 0.01, 'f', 2)).append("</em></td>");
					else
						ESILabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.ESI * 0.01, 'f', 2)).append("</td>");
				}
				else
				{
					ESILabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				if (p.detectionMethod.isEmpty())
				{
					detectionMethodLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
				}
				else
				{
					detectionMethodLabel.append("<td style=\"padding:0 2px;\">").append(q_(p.detectionMethod)).append("</td>");
				}
			}
			oss << "<table>";
			oss << "<tr>" << planetNameLabel << "</tr>";
			oss << "<tr>" << planetProperNameLabel << "</tr>";
			oss << "<tr>" << periodLabel << "</tr>";
			oss << "<tr>" << massLabel << "</tr>";
			oss << "<tr>" << radiusLabel << "</tr>";
			oss << "<tr>" << semiAxisLabel << "</tr>";
			oss << "<tr>" << eccentricityLabel << "</tr>";
			oss << "<tr>" << inclinationLabel << "</tr>";
			oss << "<tr>" << angleDistanceLabel << "</tr>";
			oss << "<tr>" << discoveredLabel << "</tr>";
			oss << "<tr>" << detectionMethodLabel << "</tr>";
			if (hasHabitableExoplanets)
			{
				oss << "<tr>" << pClassLabel << "</tr>";				
				oss << "<tr>" << equilibriumTempLabel << "</tr>";
				oss << "<tr>" << fluxLabel << "</tr>";
				oss << "<tr>" << ESILabel << "</tr>";				
			}
			oss << "</table>";
			if (hasHabitableExoplanets)
				oss << QString("%1: %2%3").arg(q_("Equilibrium temperature on Earth")).arg(QString::number(getTemperature(255), 'f', 2)).arg(getTemperatureScaleUnit()) << "<br />";
		}
	}

	postProcessInfoString(str, flags);
	return str;
}

QString Exoplanet::getTemperatureScaleUnit() const
{
	QString um = "";
	switch (temperatureScaleID) {
		case 0:
			um = "K";
			break;
		case 2:
			um = QString("%1F").arg(QChar(0x00B0));
			break;
		case 1:
		default:
			um = QString("%1C").arg(QChar(0x00B0));
			break;
	}

	return um;
}

float Exoplanet::getTemperature(float temperature) const
{
	float rt = 0.f;
	switch (temperatureScaleID) {
		case 0: // Kelvins
			rt = temperature;
			break;
		case 2: //
			rt = (temperature - 273.15f)*1.8f + 32.f;
			break;
		case 1: // Celsius
		default:
			rt = temperature - 273.15f;
			break;
	}

	return rt;
}

QVariantMap Exoplanet::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	// Tentatively add a few more strings. Details are left to the plugin author.
	if (!starProperName.isEmpty()) map["starProperName"] = starProperName;
	map["distance"] = distance;
	map["stype"] = stype;
	map["smass"] = smass;
	map["smetal"] = smetal;
	// map["Vmag"] = Vmag; // maybe same as getVmagnitude?
	map["sradius"] = sradius;
	map["effectiveTemp"] = effectiveTemp;
	map["hasHabitablePlanets"] = hasHabitableExoplanets;
	map["type"] = "ExoplanetSystem"; // Replace default but confusing "Exoplanet" from class name.
	// TODO: Maybe add number of habitables? Add details?
	return map;
}

QString Exoplanet::getPlanetaryClassI18n(QString ptype) const
{
	QString result = "";
	QRegExp dataRx("^(\\w)-(\\w+)\\s(\\w+)$");
	if (dataRx.exactMatch(ptype))
	{
		QString spectral = dataRx.capturedTexts().at(1).trimmed();
		QString zone = dataRx.capturedTexts().at(2).trimmed();
		QString size = dataRx.capturedTexts().at(3).trimmed();

		result = QString("%1-%2 %3")
				.arg(spectral)
				.arg(q_(zone))
				.arg(q_(size));
	}
	return result;
}

Vec3f Exoplanet::getInfoColor(void) const
{
	return Vec3f(1.0, 1.0, 1.0);
}

float Exoplanet::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core);
	if (distributionMode)
	{
		return 4.f;
	}
	else
	{
		if (Vmag<99)
		{
			return Vmag;
		}
		else
		{
			return 6.f;
		}
	}
}

double Exoplanet::getAngularSize(const StelCore*) const
{
	return 0.0001;
}

bool Exoplanet::isDiscovered(const StelCore *core)
{
	int year, month, day;
	QList<int> discovery;
	// For getting value of new year from midnight at 1 Jan we need increase a value of JD on 0.5.
	// This hack need for correct display of discovery mode of exoplanets.
	StelUtils::getDateFromJulianDay(core->getJD()+0.5, &year, &month, &day);
	discovery.clear();
	foreach(const exoplanetData &p, exoplanets)
	{
		if (p.discovered>0)
		{
			discovery.append(p.discovered);
		}
	}
	qSort(discovery.begin(),discovery.end());
	if (!discovery.isEmpty()) 
	{
		if (discovery.at(0)<=year && discovery.at(0)>0)
		{
			return true;
		}
	}
	return false;
}

void Exoplanet::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void Exoplanet::draw(StelCore* core, StelPainter *painter)
{
	bool visible;
	StelSkyDrawer* sd = core->getSkyDrawer();
	StarMgr* smgr = GETSTELMODULE(StarMgr); // It's need for checking displaying of labels for stars

	Vec3f color = exoplanetMarkerColor;
	if (hasHabitableExoplanets)
		color = habitableExoplanetMarkerColor;

	StelUtils::spheToRect(RA, DE, XYZ);
	double mag = getVMagnitudeWithExtinction(core);

	painter->setBlending(true, GL_ONE, GL_ONE);
	painter->setColor(color[0], color[1], color[2], 1);

	if (timelineMode)
	{
		visible = isDiscovered(core);
	}
	else
	{
		visible = true;
	}

	if (habitableMode)
	{
		if (!hasHabitableExoplanets)
			return;
	}

	Vec3d win;
	// Check visibility of exoplanet system
	if(!visible || !(painter->getProjector()->projectCheck(XYZ, win))) {return;}

	float mlimit = sd->getLimitMagnitude();

	if (mag <= mlimit)
	{		
		Exoplanet::markerTexture->bind();
		float size = getAngularSize(Q_NULLPTR)*M_PI/180.*painter->getProjector()->getPixelPerRadAtCenter();
		float shift = 5.f + size/1.6f;

		painter->drawSprite2dMode(XYZ, distributionMode ? 4.f : 5.f);

		float coeff = 4.5f + std::log10(sradius + 0.1f);
		if (labelsFader.getInterstate()<=0.f && !distributionMode && (mag+coeff)<mlimit && smgr->getFlagLabels() && showDesignations)
		{
			painter->drawText(XYZ, getNameI18n(), 0, shift, shift, false);
		}
	}
}
