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
	// return semi-initialized if the mandatory fields are not present
	if (!map.contains("designation"))
		return;

	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
		
	designation  = map.value("designation").toString();
	starProperName = map.value("starProperName").toString();	
	RA = StelUtils::getDecAngle(map.value("RA").toString());
	DE = StelUtils::getDecAngle(map.value("DE").toString());
	StelUtils::spheToRect(RA, DE, XYZ);	
	bool sign;
	double RAdd, DEdd;
	StelUtils::radToDecDeg(RA, sign, RAdd);
	StelUtils::radToDecDeg(DE, sign, DEdd);
	if (!sign) DEdd *= -1;
	distance = map.value("distance").toDouble();
	stype = map.value("stype").toString();
	smass = map.value("smass").toDouble();
	smetal = map.value("smetal").toDouble();
	Vmag = map.value("Vmag", 99.).toDouble();
	sradius = map.value("sradius").toDouble();
	effectiveTemp = map.value("effectiveTemp").toInt();
	hasHabitableExoplanets = map.value("hasHP", false).toBool();

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
		for (const auto& expl : map.value("exoplanets").toList())
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
			p.period = exoplanetMap.value("period", -1.f).toDouble();
			p.mass = exoplanetMap.value("mass", -1.f).toDouble();
			p.radius = exoplanetMap.value("radius", -1.f).toDouble();
			p.semiAxis = exoplanetMap.value("semiAxis", -1.f).toDouble();
			p.eccentricity = exoplanetMap.value("eccentricity", -1.f).toDouble();
			p.inclination = exoplanetMap.value("inclination", -1.f).toDouble();
			p.angleDistance = exoplanetMap.value("angleDistance", -1.f).toDouble();
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

			eccentricityList.append(qMax(0., p.eccentricity));
			semiAxisList.append(qMax(0., p.semiAxis));
			massList.append(qMax(0., p.mass));
			radiusList.append(qMax(0., p.radius));
			angleDistanceList.append(qMax(0., p.angleDistance));
			periodList.append(qMax(0., p.period));
			yearDiscoveryList.append(p.discovered);
			effectiveTempHostStarList.append(effectiveTemp);
			metallicityHostStarList.append(smetal);
			if (Vmag<99)
				vMagHostStarList.append(Vmag);
			raHostStarList.append(RAdd);
			decHostStarList.append(DEdd);
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
	for (const auto& p : exoplanets)
	{
		QVariantMap explMap;
		explMap["planetName"] = p.planetName;
		if (!p.planetProperName.isEmpty()) explMap["planetProperName"] = p.planetProperName;
		if (p.mass > -1.) explMap["mass"] = p.mass;
		if (p.period > -1.) explMap["period"] = p.period;
		if (p.radius > -1.) explMap["radius"] = p.radius;
		if (p.semiAxis > -1.) explMap["semiAxis"] = p.semiAxis;
		if (p.inclination > -1.) explMap["inclination"] = p.inclination;
		if (p.eccentricity > -1.) explMap["eccentricity"] = p.eccentricity;
		if (p.angleDistance > -1.) explMap["angleDistance"] = p.angleDistance;
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
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_("planetary system")) << "<br />";

	if (flags&Magnitude && isVMagnitudeDefined() && !distributionMode)
		oss << getMagnitudeInfoString(core, flags, 2);

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
			oss << QString("%1: %2 M<sub>%3</sub>").arg(q_("Mass"), QString::number(smass, 'f', 3), QChar(0x2609)) << "<br />";
		}
		if (sradius>0)
		{
			oss << QString("%1: %2 R<sub>%3</sub>").arg(q_("Radius"), QString::number(sradius, 'f', 5), QChar(0x2609)) << "<br />";
		}
		if (effectiveTemp>0)
		{
			oss << QString("%1: %2 %3").arg(q_("Effective temperature")).arg(effectiveTemp).arg(qc_("K", "temperature")) << "<br />";
		}
		if (exoplanets.size() > 0)
		{
			QString qss = "padding: 0 2px 0 0;";
			QString planetNameLabel = QString("<td style=\"%2\">%1</td>").arg(q_("Exoplanet")).arg(qss);
			QString planetProperNameLabel = QString("<td style=\"%2\">%1</td>").arg(q_("Name")).arg(qss);
			QString periodLabel = QString("<td style=\"%3\">%1 (%2)</td>").arg(q_("Period")).arg(qc_("days", "period")).arg(qss);
			QString massLabel = QString("<td style=\"%3\">%1 (M<sub>%2</sub>)</td>").arg(q_("Mass")).arg(QChar(0x2643)).arg(qss);
			QString radiusLabel = QString("<td style=\"%3\">%1 (R<sub>%2</sub>)</td>").arg(q_("Radius")).arg(QChar(0x2643)).arg(qss);
			QString semiAxisLabel = QString("<td style=\"%3\">%1 (%2)</td>").arg(q_("Semi-Major Axis")).arg(qc_("AU", "distance, astronomical unit")).arg(qss);
			QString eccentricityLabel = QString("<td style=\"%2\">%1</td>").arg(q_("Eccentricity")).arg(qss);
			QString inclinationLabel = QString("<td style=\"%3\">%1 (%2)</td>").arg(q_("Inclination")).arg(QChar(0x00B0)).arg(qss);
			QString angleDistanceLabel = QString("<td style=\"%2\">%1 (\")</td>").arg(q_("Angle Distance")).arg(qss);
			QString discoveredLabel = QString("<td style=\"%2\">%1</td>").arg(q_("Discovered year")).arg(qss);
			QString detectionMethodLabel = QString("<td style=\"%2\">%1</td>").arg(q_("Detection method")).arg(qss);
			QString pClassLabel = QString("<td style=\"%2\">%1</td>").arg(q_("Planetary class")).arg(qss);
			//TRANSLATORS: Full phrase is "Equilibrium Temperature"
			QString equilibriumTempLabel = QString("<td style=\"%3\">%1 (%2)</td>").arg(q_("Equilibrium temp.")).arg(getTemperatureScaleUnit()).arg(qss);
			//TRANSLATORS: Average stellar flux of the planet
			QString fluxLabel = QString("<td style=\"%2\">%1 (S<sub>E</sub>)</td>").arg(q_("Flux")).arg(qss);
			//TRANSLATORS: ESI = Earth Similarity Index
			QString ESILabel = QString("<td style=\"%2\">%1</td>").arg(q_("ESI")).arg(qss);

			QString row = "<td style=\"padding:0 2px;\">%1</td>";
			QString emRow = "<td style=\"padding:0 2px;\"><em>%1</em></td>";
			QString emptyRow = "<td style=\"padding:0 2px;\">&mdash;</td>";
			for (const auto& p : exoplanets)
			{
				if (!p.planetName.isEmpty())
					planetNameLabel.append(row.arg(p.planetName));
				else
					planetNameLabel.append(emptyRow);

				if (!p.planetProperName.isEmpty())
					planetProperNameLabel.append(row.arg(trans.qtranslate(p.planetProperName)));
				else
					planetProperNameLabel.append(emptyRow);

				if (p.period > -1.)
					periodLabel.append(row.arg(QString::number(p.period, 'f', 2)));
				else
					periodLabel.append(emptyRow);

				if (p.mass > -1.)
					massLabel.append(row.arg(QString::number(p.mass, 'f', 2)));
				else
					massLabel.append(emptyRow);

				if (p.radius > -1.)
					radiusLabel.append(row.arg(QString::number(p.radius, 'f', 1)));
				else
					radiusLabel.append(emptyRow);

				if (p.eccentricity > -1.)
					eccentricityLabel.append(row.arg(QString::number(p.eccentricity, 'f', 3)));
				else
					eccentricityLabel.append(emptyRow);

				if (p.inclination > -1.)
					inclinationLabel.append(row.arg(QString::number(p.inclination, 'f', 1)));
				else
					inclinationLabel.append(emptyRow);

				if (p.semiAxis > -1.)
					semiAxisLabel.append(row.arg(QString::number(p.semiAxis, 'f', 4)));
				else
					semiAxisLabel.append(emptyRow);

				if (p.angleDistance > -1.)
					angleDistanceLabel.append(row.arg(QString::number(p.angleDistance, 'f', 6)));
				else
					angleDistanceLabel.append(emptyRow);

				if (p.discovered > 0)
					discoveredLabel.append(row.arg(QString::number(p.discovered)));
				else
					discoveredLabel.append(emptyRow);

				if (!p.pclass.isEmpty())
				{
					if (!p.conservative)
						pClassLabel.append(emRow.arg(getPlanetaryClassI18n(p.pclass)));
					else
						pClassLabel.append(row.arg(getPlanetaryClassI18n(p.pclass)));
				}
				else
					pClassLabel.append(emptyRow);

				if (p.EqTemp > 0)
				{
					if (!p.conservative)
						equilibriumTempLabel.append(emRow.arg(QString::number(getTemperature(p.EqTemp), 'f', 2)));
					else
						equilibriumTempLabel.append(row.arg(QString::number(getTemperature(p.EqTemp), 'f', 2)));
				}
				else
					equilibriumTempLabel.append(emptyRow);

				if (p.flux > 0)
				{
					if (!p.conservative)
						fluxLabel.append(emRow.arg(QString::number(p.flux * 0.01, 'f', 2)));
					else
						fluxLabel.append(row.arg(QString::number(p.flux * 0.01, 'f', 2)));
				}
				else
					fluxLabel.append(emptyRow);

				if (p.ESI > 0)
				{
					if (!p.conservative)
						ESILabel.append(emRow.arg(QString::number(p.ESI * 0.01, 'f', 2)));
					else
						ESILabel.append(row.arg(QString::number(p.ESI * 0.01, 'f', 2)));
				}
				else
					ESILabel.append(emptyRow);

				if (p.detectionMethod.isEmpty())
					detectionMethodLabel.append(emptyRow);
				else
					detectionMethodLabel.append(row.arg(q_(p.detectionMethod)));
			}
			oss << "<table style='margin-left: -2px;'>"; // Cosmetic fix
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
		case 2: // Fahrenheit
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
		QString spectral = dataRx.cap(1).trimmed();
		QString zone = dataRx.cap(2).trimmed();
		QString size = dataRx.cap(3).trimmed();

		result = QString("%1-%2 %3")
				.arg(spectral)
				.arg(q_(zone))
				.arg(q_(size));
	}
	return result;
}

Vec3f Exoplanet::getInfoColor(void) const
{
	return Vec3f(1.f, 1.f, 1.f);
}

float Exoplanet::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core)
	return (distributionMode ? 4.f : (isVMagnitudeDefined() ? static_cast<float>(Vmag) : 6.f));
}

bool Exoplanet::isVMagnitudeDefined() const
{
	return Vmag<98.;
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
	for (const auto& p : exoplanets)
	{
		if (p.discovered>0)
		{
			discovery.append(p.discovered);
		}
	}
	std::sort(discovery.begin(), discovery.end());
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
	labelsFader.update(static_cast<int>(deltaTime*1000));
}

void Exoplanet::draw(StelCore* core, StelPainter *painter)
{
	if ((habitableMode) &&  (!hasHabitableExoplanets)) {return;}

	bool visible = (timelineMode? isDiscovered(core) : true);
	Vec3d win;
	// Check visibility of exoplanet system
	if(!visible || !(painter->getProjector()->projectCheck(XYZ, win))) {return;}

	StelSkyDrawer* sd = core->getSkyDrawer();
	const float mlimit = sd->getLimitMagnitude();
	const float mag = getVMagnitudeWithExtinction(core);

	if (mag <= mlimit)
	{		
		Exoplanet::markerTexture->bind();
		Vec3f color = (hasHabitableExoplanets ? habitableExoplanetMarkerColor : exoplanetMarkerColor);
		float size = static_cast<float>(getAngularSize(Q_NULLPTR))*M_PIf/180.f*painter->getProjector()->getPixelPerRadAtCenter();
		float shift = 5.f + size/1.6f;

		painter->setBlending(true, GL_ONE, GL_ONE);
		painter->setColor(color, 1);
		painter->drawSprite2dMode(XYZ, distributionMode ? 4.f : 5.f);

		float coeff = 4.5f + std::log10(static_cast<float>(sradius) + 0.1f);
		StarMgr* smgr = GETSTELMODULE(StarMgr); // It's need for checking displaying of labels for stars
		if (labelsFader.getInterstate()<=0.f && !distributionMode && (mag+coeff)<mlimit && smgr->getFlagLabels() && showDesignations)
		{
			painter->drawText(XYZ, getNameI18n(), 0, shift, shift, false);
		}
	}
}
