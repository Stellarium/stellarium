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
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyDrawer.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QtOpenGL>
#include <QVariantMap>
#include <QVariant>
#include <QList>

StelTextureSP Exoplanet::markerTexture;

Exoplanet::Exoplanet(const QVariantMap& map)
		: initialized(false)
{
	// return initialized if the mandatory fields are not present
	if (!map.contains("designation"))
		return;
		
	designation  = map.value("designation").toString();
	RA = StelUtils::getDecAngle(map.value("RA").toString());
	DE = StelUtils::getDecAngle(map.value("DE").toString());

	if (map.contains("exoplanets"))
	{
		foreach(const QVariant &expl, map.value("exoplanets").toList())
		{
			QVariantMap exoplanetMap = expl.toMap();
			exoplanetData p;
			if (exoplanetMap.contains("planetName")) p.planetName = exoplanetMap.value("planetName").toString();
			if (exoplanetMap.contains("period")) p.period = exoplanetMap.value("period").toString();
			if (exoplanetMap.contains("mass")) p.mass = exoplanetMap.value("mass").toString();
			if (exoplanetMap.contains("radius")) p.radius = exoplanetMap.value("radius").toString();
			if (exoplanetMap.contains("semiAxis")) p.semiAxis = exoplanetMap.value("semiAxis").toString();
			if (exoplanetMap.contains("eccentricity")) p.eccentricity = exoplanetMap.value("eccentricity").toString();
			if (exoplanetMap.contains("inclination")) p.inclination = exoplanetMap.value("inclination").toString();
			if (exoplanetMap.contains("year")) p.year = exoplanetMap.value("year").toInt();
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
	QVariantList exoplanetList;
	foreach(const exoplanetData &p, exoplanets)
	{
		QVariantMap explMap;
		explMap["planetName"] = p.planetName;
		if (!p.mass.isEmpty() && p.mass != "") explMap["mass"] = p.mass;
		if (!p.period.isEmpty() && p.period != "") explMap["period"] = p.period;
		if (!p.radius.isEmpty() && p.radius != "") explMap["radius"] = p.radius;
		if (!p.semiAxis.isEmpty() && p.semiAxis != "") explMap["semiAxis"] = p.semiAxis;
		if (!p.inclination.isEmpty() && p.inclination != "") explMap["inclination"] = p.inclination;
		if (!p.eccentricity.isEmpty() && p.eccentricity != "") explMap["eccentricity"] = p.eccentricity;
		if (p.year != 0) explMap["year"] = p.year;
		exoplanetList << explMap;
	}
	map["exoplanets"] = exoplanetList;

	return map;
}

float Exoplanet::getSelectPriority(const StelCore* core) const
{
	//Same as StarWrapper::getSelectPriority()
        return getVMagnitude(core, false);
}

QString Exoplanet::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>" << designation << "</h2>";
	}

	// Ra/Dec etc.
	oss << getPositionInfoString(core, flags);

	if (flags&Extra1 && exoplanets.size() > 0)
	{
		QString planetNameLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Exoplanet"));
		QString periodLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (%2)</td>").arg(q_("Period")).arg(q_("days"));
		QString massLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (M<sub>%2</sub>)</td>").arg(q_("Mass")).arg(q_("Jup"));
		QString radiusLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (R<sub>%2</sub>)</td>").arg(q_("Radius")).arg(q_("Jup"));
		QString semiAxisLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (%2)</td>").arg(q_("Semi-Major Axis")).arg(q_("AU"));
		QString eccentricityLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Eccentricity"));
		QString inclinationLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1 (%2)</td>").arg(q_("Inclination")).arg(QChar(0x00B0));
		QString yearLabel = QString("<td style=\"padding: 0 2px 0 0;\">%1</td>").arg(q_("Year of discovery"));
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
			if (!p.period.isEmpty())
			{
				periodLabel.append("<td style=\"padding:0 2px;\">").append(p.period).append("</td>");
			}
			else
			{
				periodLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
			}
			if (!p.mass.isEmpty())
			{
				massLabel.append("<td style=\"padding:0 2px;\">").append(p.mass).append("</td>");
			}
			else
			{
				massLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
			}
			if (!p.radius.isEmpty())
			{
				radiusLabel.append("<td style=\"padding:0 2px;\">").append(p.radius).append("</td>");
			}
			else
			{
				radiusLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
			}
			if (!p.eccentricity.isEmpty())
			{
				eccentricityLabel.append("<td style=\"padding:0 2px;\">").append(p.eccentricity).append("</td>");
			}
			else
			{
				eccentricityLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
			}
			if (!p.inclination.isEmpty())
			{
				inclinationLabel.append("<td style=\"padding:0 2px;\">").append(p.inclination).append("</td>");
			}
			else
			{
				inclinationLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
			}
			if (!p.semiAxis.isEmpty())
			{
				semiAxisLabel.append("<td style=\"padding:0 2px;\">").append(p.semiAxis).append("</td>");
			}
			else
			{
				semiAxisLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
			}
			if (p.year>0)
			{
				yearLabel.append("<td style=\"padding:0 2px;\">").append(QString::number(p.year)).append("</td>");
			}
			else
			{
				yearLabel.append("<td style=\"padding:0 2px;\">&mdash;</td>");
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
		oss << "<tr>" << yearLabel << "</tr></table>";
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

	// Calculate fake visual magnitude - minimal magnitude is 4
	float vmag = 4.f;

	return vmag + extinctionMag;
}

double Exoplanet::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void Exoplanet::update(double deltaTime)
{
	labelsFader.update((int)(deltaTime*1000));
}

void Exoplanet::draw(StelCore* core, StelPainter& painter)
{
	StelSkyDrawer* sd = core->getSkyDrawer();	

	Vec3f color = Vec3f(0.4f,1.2f,0.5f);
	double mag = getVMagnitude(core, true);

	StelUtils::spheToRect(RA, DE, XYZ);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	painter.setColor(color[0], color[1], color[2], 1);

	if (mag <= sd->getLimitMagnitude())
	{

		Exoplanet::markerTexture->bind();
		float size = getAngularSize(NULL)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
		float shift = 5.f + size/1.6f;
		if (labelsFader.getInterstate()<=0.f)
		{
			painter.drawSprite2dMode(XYZ, 5);
			painter.drawText(XYZ, designation, 0, shift, shift, false);
		}
	}
}
