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

#include "StelModuleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelSkyCultureSkyPartition.hpp"
#include "GridLinesMgr.hpp"

StelSkyCultureSkyPartition::StelSkyCultureSkyPartition(const QJsonObject &json):
	frameType(StelCore::FrameObservercentricEclipticOfDate),
	partitions(),
	extent(90.),
	centerLine(nullptr)
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
	centerLine->setDisplayed(true);

	// Parse extent, create polar caps where needed.
	if (json.contains("extent") && json["extent"].isDouble())
	{
		extent=json["extent"].toDouble();
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
		qWarning() << "No \"partitions\" array found in JSON data for zodiac or lunarSystem description";
	}
	// TODO: Parse names
	if (json.contains("name"))
	{
		QJsonObject nameObj = json["name"].toObject();
		name.native = nameObj["native"].toString();
		name.pronounce = nameObj["pronounce"].toString();
		name.transliteration = nameObj["transliteration"].toString();
		name.translated = nameObj["english"].toString();
	}
	else
	{
		qWarning() << "No \"name\" found in JSON data for zodiac or lunarSystem description";
	}
	if (json.contains("names") && json["names"].isArray())
	{
		QJsonArray jNames = json["names"].toArray();
		for (unsigned int i=0; i<jNames.size(); ++i)
		{
			const QJsonObject jName = jNames[i].toObject();
			StelObject::CulturalName cName;
			cName.native=jName["native"].toString();
			cName.pronounce=jName["pronounce"].toString();
			cName.transliteration=jName["transliteration"].toString();
			cName.translated=jName["english"].toString();
			names.append(cName);
			symbols.append(jName["symbol"].toString());
		}
	}
	else
	{
		qWarning() << "No \"names\" array found in JSON data for zodiac or lunarSystem description";
	}
	Q_ASSERT(symbols.length() == names.length());


	// Font size is 14
	font.setPixelSize(StelApp::getInstance().getScreenFontSize()+1);
	updateLabels();
}

StelSkyCultureSkyPartition::~StelSkyCultureSkyPartition()
{
//	if (centerLine) delete centerLine;
}


void StelSkyCultureSkyPartition::draw(StelPainter& sPainter, const Vec3d &obsVelocity) const
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	static StelCore *core=StelApp::getInstance().getCore();
	StelProjectorP prj = core->getProjection(frameType, (frameType!=StelCore::FrameAltAz && frameType!=StelCore::FrameFixedEquatorial) ? StelCore::RefractionAuto : StelCore::RefractionOff);
	sPainter.setProjector(prj);	

	// Draw the major partitions
	for (int p=0; p<partitions[0]; ++p)
	{
		const double lng=360./partitions[0]*p *M_PI_180;
		Vec3d eqPt, nPt, sPt;
		StelUtils::spheToRect(lng, 0., eqPt);
		StelUtils::spheToRect(lng, extent*M_PI_180, nPt);
		StelUtils::spheToRect(lng, -extent*M_PI_180, sPt);
		sPainter.drawGreatCircleArc(eqPt, nPt);
		sPainter.drawGreatCircleArc(eqPt, sPt);
	}

	// Draw top/bottom lines where applicable
	if (extent<90.)
	{
		// Get the bounding halfspace
		const SphericalCap& viewPortSphericalCap = sPainter.getProjector()->getBoundingCap();
		drawCap(sPainter, viewPortSphericalCap, extent);
		drawCap(sPainter, viewPortSphericalCap, -extent);
	}

	centerLine->draw(sPainter, 1.f); // The second arg. is irrelevant, will be restored again in the caller...

	// Symbols: At the beginning of the respective zone.
	if (symbols.length())
	{
		for (int i=0; i<partitions[0]; ++i)
		{
			// To have tilted labels, we project a point 0.1deg from the actual label point and derive screen-based angle.
			double lng  = (360./partitions[0]*i + 2.)*M_PI_180;
			double lng1 = (360./partitions[0]*i + 1.9)*M_PI_180;
			double lat  = (extent<50. ? -extent+0.2 : -10.) *M_PI_180;
			Vec3d pos, pos1, scr, scr1;
			StelUtils::spheToRect(lng, lat, pos);
			StelUtils::spheToRect(lng1, lat, pos1);
			prj->project(pos, scr);
			prj->project(pos1, scr1);
			double angle=atan2(scr1[1]-scr[1], scr1[0]-scr[0])*M_180_PI;
			sPainter.drawText(pos, symbols.at(i), angle);
		}
	}

	// Labels:
	if (names.length())
	{
		for (int i=0; i<partitions[0]; ++i)
		{
			QString label=scMgr->createCulturalLabel(names.at(i), StelObject::CulturalDisplayStyle::Pronounce,names.at(i).pronounceI18n);
			// To have tilted labels, we project a point 0.1deg from the actual label point and derive screen-based angle.
			double lng  = (360./partitions[0]*(double(i)+0.5) + 2.)*M_PI_180;
			double lng1 = (360./partitions[0]*(double(i)+0.5) + 1.9)*M_PI_180;
			double lat  = (extent<50. ? -extent+0.2 : -10.) *M_PI_180;
			Vec3d pos, pos1, scr, scr1;
			StelUtils::spheToRect(lng, lat, pos);
			StelUtils::spheToRect(lng1, lat, pos1);
			prj->project(pos, scr);
			prj->project(pos1, scr1);
			float angle=atan2(scr1[1]-scr[1], scr1[0]-scr[0])*M_180_PIf;
			QFontMetrics metrics(font);
			float xShift= -0.5 * metrics.boundingRect(label).width();

			sPainter.drawText(pos, label, angle, xShift);
		}
	}

}


// shamelessly copied from SkyLine::draw (small circles part)
void StelSkyCultureSkyPartition::drawCap(StelPainter &sPainter, const SphericalCap& viewPortSphericalCap, double latDeg) const
{
	double lat=latDeg*M_PI_180;
	SphericalCap declinationCap(Vec3d(0.,0.,1.), std::sin(lat));
	const Vec3d rotCenter(0,0,declinationCap.d);

	Vec3d p1, p2;
	if (!SphericalCap::intersectionPoints(viewPortSphericalCap, declinationCap, p1, p2))
	{
		if ((viewPortSphericalCap.d<declinationCap.d && viewPortSphericalCap.contains(declinationCap.n))
				|| (viewPortSphericalCap.d<-declinationCap.d && viewPortSphericalCap.contains(-declinationCap.n)))
		{
			// The line is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
			Vec3d pt1, pt2, pt3;
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
