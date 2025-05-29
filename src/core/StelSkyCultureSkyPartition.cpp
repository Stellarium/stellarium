/*
 * Stellarium
 * Copyright (C) 2025 Georg Zotti
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


#include <QJsonObject>
#include <QJsonArray>

#include "StelSkyCultureSkyPartition.hpp"
#include "GridLinesMgr.hpp"

StelSkyCultureSkyPartition::StelSkyCultureSkyPartition(const QJsonObject &json):
	frameType(StelCore::FrameObservercentricEclipticOfDate),
	partitions(),
	extent(90.),
	centerLine(nullptr), northernCap(nullptr), southernCap(nullptr),
	color(1., 1., 0),
	lineThickness(1)
{
	// Parse defining centerline type
	SkyLine::SKY_LINE_TYPE skylineType=SkyLine::ECLIPTIC_CULTURAL;
	if (json.contains("coordsys"))
	{
		static const QMap<QString, SkyLine::SKY_LINE_TYPE>map={
			{"ecliptical", SkyLine::ECLIPTIC_CULTURAL},
			{"equatorial", SkyLine::EQUATORIAL_CULTURAL}};
		skylineType=map.value(json["coordsys"].toString(), SkyLine::ECLIPTIC_CULTURAL);
	}
	centerLine=new SkyLine(skylineType);

	// Parse extent, create polar caps where needed.
	if (json.contains("extent") && json["extent"].isDouble())
	{
		extent=json["extent"].toDouble();
		northernCap = new SphericalCap(Vec3d(0., 0.,  1.), cos((90.-extent)*M_PI_180));
		southernCap = new SphericalCap(Vec3d(0., 0., -1.), cos((90.-extent)*M_PI_180));
	}
	else
		qWarning() << "Bad \"extent\" given in JSON file.";

	if (json.contains("partitions") && json["partitions"].isArray())
	{
		QJsonArray p = json["partitions"].toArray();
		for (unsigned int i=0; i<p.size(); ++i)
			partitions.append(p.at(i).toDouble());
	}
	else
	{
		qWarning() << "No \"partitions\" array found in JSON data for zodiac or lunar description";
	}
	// TODO: Parse names
	// TODO: Parse symbols

	// Font size is 14
	font.setPixelSize(StelApp::getInstance().getScreenFontSize()+1);
	updateLabels();
}



void StelSkyCultureSkyPartition::draw(StelCore *core) const
{
	if (fader.getInterstate() <= 0.f)
		return;

	StelProjectorP prj = core->getProjection(frameType, (frameType!=StelCore::FrameAltAz && frameType!=StelCore::FrameFixedEquatorial) ? StelCore::RefractionAuto : StelCore::RefractionOff);

	// Get the bounding halfspace
	const SphericalCap& viewPortSphericalCap = prj->getBoundingCap();

	// Initialize a painter and set openGL state
	StelPainter sPainter(prj);
	sPainter.setColor(color, fader.getInterstate());
	sPainter.setBlending(true);
	const float oldLineWidth=sPainter.getLineWidth();
	sPainter.setLineWidth(lineThickness); // set line thickness
	sPainter.setLineSmooth(true);

	//ViewportEdgeIntersectCallbackData userData(&sPainter);
	//sPainter.setFont(font);
	//userData.textColor = Vec4f(color, fader.getInterstate());
	//userData.text = label;





	if (northernCap)
		drawCap(sPainter, viewPortSphericalCap, northernCap);
	if (southernCap)
		drawCap(sPainter, viewPortSphericalCap, southernCap);
}


// shamelessly copied from SkyLine::draw (small circles part)
void StelSkyCultureSkyPartition::drawCap(StelPainter &sPainter, const SphericalCap& viewPortSphericalCap, SphericalCap *cap) const
{
	const Vec3d rotCenter(0,0,cap->d);

	const double lat=std::acos(cap->d);
	Vec3d p1, p2;
	if (!SphericalCap::intersectionPoints(viewPortSphericalCap, *cap, p1, p2))
	{
		if ((viewPortSphericalCap.d<cap->d && viewPortSphericalCap.contains(cap->n))
				|| (viewPortSphericalCap.d<-cap->d && viewPortSphericalCap.contains(-cap->n)))
		{
			// The line is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
			Vec3d pt1;
			Vec3d pt2;
			Vec3d pt3;
			const double lon1=0.0;
			const double lon2=120.0*M_PI_180;
			const double lon3=240.0*M_PI_180;
			StelUtils::spheToRect(lon1, lat, pt1); //pt1.normalize();
			StelUtils::spheToRect(lon2, lat, pt2); //pt2.normalize();
			StelUtils::spheToRect(lon3, lat, pt3); //pt3.normalize();

			sPainter.drawSmallCircleArc(pt1, pt2, rotCenter, nullptr, nullptr);
			sPainter.drawSmallCircleArc(pt2, pt3, rotCenter, nullptr, nullptr);
			sPainter.drawSmallCircleArc(pt3, pt1, rotCenter, nullptr, nullptr);
		}
	}
	else
	{
		// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
		Vec3d middlePoint = p1-rotCenter+p2-rotCenter;
		middlePoint.normalize();
		middlePoint*=(p1-rotCenter).norm();
		middlePoint+=rotCenter;
		if (!viewPortSphericalCap.contains(middlePoint))
		{
			middlePoint-=rotCenter;
			middlePoint*=-1.;
			middlePoint+=rotCenter;
		}

		sPainter.drawSmallCircleArc(p1, middlePoint, rotCenter, nullptr, nullptr);
		sPainter.drawSmallCircleArc(p2, middlePoint, rotCenter, nullptr, nullptr);
	}
}

void StelSkyCultureSkyPartition::setFontSize(int newFontSize)
{
	font.setPixelSize(newFontSize);
}

void StelSkyCultureSkyPartition::updateLabels()
{
	// TODO: Extract short cultural labels and apply here.
}
