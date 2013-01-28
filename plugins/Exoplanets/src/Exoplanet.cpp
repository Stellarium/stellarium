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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QVariant>
#include <QList>

Exoplanet::Exoplanet(const QVariantMap& map)
		: initialized(false)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation"))
		return;
		
	designation  = map.value("designation").toString();
	RA = StelUtils::getDecAngle(map.value("RA").toString());
	DE = StelUtils::getDecAngle(map.value("DE").toString());
	distance = map.value("distance").toFloat();
	stype = map.value("stype").toString();
	smass = map.value("smass").toFloat();
	smetal = map.value("smetal").toFloat();
	if (map.contains("Vmag"))
	{
		Vmag = map.value("Vmag").toFloat();
	}
	else
	{
		Vmag = 99;
	}
	sradius = map.value("sradius").toFloat();
	effectiveTemp = map.value("effectiveTemp").toInt();

	if (map.contains("exoplanets"))
	{
		foreach(const QVariant &expl, map.value("exoplanets").toList())
		{
			QVariantMap exoplanetMap = expl.toMap();
			exoplanetData p;
			if (exoplanetMap.contains("planetName")) p.planetName = exoplanetMap.value("planetName").toString();
			if (exoplanetMap.contains("period"))
				p.period = exoplanetMap.value("period").toFloat();
			else
				p.period = -1.f;
			if (exoplanetMap.contains("mass"))
				p.mass = exoplanetMap.value("mass").toFloat();
			else
				p.mass = -1.f;
			if (exoplanetMap.contains("radius"))
				p.radius = exoplanetMap.value("radius").toFloat();
			else
				p.radius = -1.f;
			if (exoplanetMap.contains("semiAxis"))
				p.semiAxis = exoplanetMap.value("semiAxis").toFloat();
			else
				p.semiAxis = -1.f;
			if (exoplanetMap.contains("eccentricity"))
				p.eccentricity = exoplanetMap.value("eccentricity").toFloat();
			else
				p.eccentricity = -1.f;
			if (exoplanetMap.contains("inclination"))
				p.inclination = exoplanetMap.value("inclination").toFloat();
			else
				p.inclination = -1.f;
			if (exoplanetMap.contains("angleDistance"))
				p.angleDistance = exoplanetMap.value("angleDistance").toFloat();
			else
				p.angleDistance = -1.f;
			if (exoplanetMap.contains("discovered"))
				p.discovered = exoplanetMap.value("discovered").toInt();
			else
				p.discovered = 0;
			exoplanets.append(p);
		}
	}

	initialized = true;
}

Exoplanet::~Exoplanet()
{
	//
}

QVariantMap Exoplanet::getMap(void)
{
	QVariantMap map;
	map["designation"] = designation;
	map["RA"] = RA;
	map["DE"] = DE;
	map["distance"] = distance;
	map["stype"] = stype;
	map["smass"] = smass;
	map["smetal"] = smetal;
	map["Vmag"] = Vmag;
	map["sradius"] = sradius;
	map["effectiveTemp"] = effectiveTemp;
	QVariantList exoplanetList;
	foreach(const exoplanetData &p, exoplanets)
	{
		QVariantMap explMap;
		explMap["planetName"] = p.planetName;
		if (p.mass > -1.f) explMap["mass"] = p.mass;
		if (p.period > -1.f) explMap["period"] = p.period;
		if (p.radius > -1.f) explMap["radius"] = p.radius;
		if (p.semiAxis > -1.f) explMap["semiAxis"] = p.semiAxis;
		if (p.inclination > -1.f) explMap["inclination"] = p.inclination;
		if (p.eccentricity > -1.f) explMap["eccentricity"] = p.eccentricity;
		if (p.angleDistance > -1.f) explMap["angleDistance"] = p.angleDistance;
		if (p.discovered > 0) explMap["discovered"] = p.discovered;
		exoplanetList << explMap;
	}
	map["exoplanets"] = exoplanetList;

	return map;
}

float Exoplanet::getSelectPriority(const StelCore* core) const
{
	if (getVMagnitude(core, false)>20.f)
	{
		return 20.f;
	}
	else
	{
		return getVMagnitude(core, false) - 1.f;
	}
}

QString Exoplanet::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>" << designation << "</h2>";
	}

	if (flags&Magnitude)
	{
		if (Vmag<99)
		{
			if (core->getSkyDrawer()->getFlagHasAtmosphere())
			{
				oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(getVMagnitude(core, false), 'f', 2),
												QString::number(getVMagnitude(core, true), 'f', 2)) << "<br>";
			}
			else
			{
				oss << q_("Magnitude: <b>%1</b>").arg(QString::number(getVMagnitude(core, false), 'f', 2)) << "<br>";
			}
		}
	}

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Extra1)
	{
		oss << q_("Spectral Type: %1").arg(stype) << "<br>";
	}

	if (flags&Distance)
	{
		oss << q_("Distance: %1 Light Years").arg(QString::number(distance/0.306601, 'f', 2)) << "<br>";
	}

	if (flags&Extra1)
	{
		if (smetal!=0)
		{
			oss << QString("%1 [Fe/H]: %2").arg(q_("Metallicity")).arg(smetal) << "<br>";
		}
		if (smass>0)
		{
			oss << QString("%1: %2 M<sub>%3</sub>").arg(q_("Mass")).arg(QString::number(smass, 'f', 3)).arg(q_("Sun")) << "<br>";
		}
		if (sradius>0)
		{
			oss << QString("%1: %2 R<sub>%3</sub>").arg(q_("Radius")).arg(QString::number(sradius, 'f', 5)).arg(q_("Sun")) << "<br>";
		}
		if (effectiveTemp>0)
		{
			oss << q_("Effective temperature: %1 K").arg(effectiveTemp) << "<br>";
		}
	}

	if (flags&Extra1 && exoplanets.size() > 0)
	{
		QString planetNameLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Exoplanet"));
		QString periodLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (%2)</td>").arg(q_("Period")).arg(q_("days"));
		QString massLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (M<sub>%2</sub>)</td>").arg(q_("Mass")).arg(q_("Jup"));
		QString radiusLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (R<sub>%2</sub>)</td>").arg(q_("Radius")).arg(q_("Jup"));
		QString semiAxisLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (%2)</td>").arg(q_("Semi-Major Axis")).arg(q_("AU"));
		QString eccentricityLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Eccentricity"));
		QString inclinationLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (%2)</td>").arg(q_("Inclination")).arg(QChar(0x00B0));		
		QString angleDistanceLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (\")</td>").arg(q_("Angle Distance"));
		QString discoveredLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Discovered year"));
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
		}
		oss << "<table>";
		oss << "<tr>" << planetNameLabel << "</tr>";
		oss << "<tr>" << periodLabel << "</tr>";
		oss << "<tr>" << massLabel << "</tr>";
		oss << "<tr>" << radiusLabel << "</tr>";
		oss << "<tr>" << semiAxisLabel << "</tr>";
		oss << "<tr>" << eccentricityLabel << "</tr>";
		oss << "<tr>" << inclinationLabel << "</tr>";
		oss << "<tr>" << angleDistanceLabel << "</tr>";
		oss << "<tr>" << discoveredLabel << "</tr>";
		oss << "</table>";
	}

	postProcessInfoString(str, flags);
	return str;
}

Vec3f Exoplanet::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.0) : Vec3f(1.0, 1.0, 1.0);
}

float Exoplanet::getVMagnitude(const StelCore* core, bool withExtinction) const
{
	float extinctionMag=0.0; // track magnitude loss
	if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
	{
	    Vec3d altAz=getAltAzPosApparent(core);
	    altAz.normalize();
	    core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
	}

	if (GETSTELMODULE(Exoplanets)->getDisplayMode())
	{
		return 4.f;
	}
	else
	{
		if (Vmag<99)
		{
			return Vmag + extinctionMag;
		}
		else
		{
			return 6.f + extinctionMag;
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
	StelUtils::getDateFromJulianDay(core->getJDay()+0.5, &year, &month, &day);
	discovery.clear();
	foreach(const exoplanetData &p, exoplanets)
	{
		if (p.discovered>0)
		{
			discovery.append(p.discovered);
		}
	}
	qSort(discovery.begin(),discovery.end());	
	if (discovery.at(0)<=year && discovery.at(0)>0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Exoplanet::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void Exoplanet::draw(StelCore* core, StelRenderer* renderer, StelProjectorP projector, 
                     StelTextureNew* markerTexture)
{
	bool visible;
	StelSkyDrawer* sd = core->getSkyDrawer();	

	Vec3f color = Vec3f(0.4f,1.2f,0.5f);
	if (StelApp::getInstance().getVisionModeNight())
		color = StelUtils::getNightColor(color);
	const double mag = getVMagnitude(core, true);

	StelUtils::spheToRect(RA, DE, XYZ);
	renderer->setBlendMode(BlendMode_Add);
	renderer->setGlobalColor(color[0], color[1], color[2]);

	if (GETSTELMODULE(Exoplanets)->getTimelineMode())
	{
		visible = isDiscovered(core);
	}
	else
	{
		visible = true;
	}

	if(!visible) {return;}

	if (mag <= sd->getLimitMagnitude())
	{
		markerTexture->bind();
		const float size = getAngularSize(NULL)*M_PI/180.*projector->getPixelPerRadAtCenter();
		const float shift = 5.f + size/1.6f;
		if (labelsFader.getInterstate()<=0.f)
		{
			Vec3d win;
			if(!projector->project(XYZ, win)){return;}			
			if (GETSTELMODULE(Exoplanets)->getDisplayMode())
			{
				renderer->drawTexturedRect(win[0] - 4, win[1] - 4, 8, 8);
			}
			else
			{
				renderer->drawTexturedRect(win[0] - 5, win[1] - 5, 10, 10);
				renderer->drawText(TextParams(XYZ, projector, designation).shift(shift, shift).useGravity());
			}
		}
	}
}
