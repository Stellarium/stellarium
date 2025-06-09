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

#include "StarMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelSkyCultureSkyPartition.hpp"
#include "GridLinesMgr.hpp"

StelSkyCultureSkyPartition::StelSkyCultureSkyPartition(const QJsonObject &json):
	frameType(StelCore::FrameObservercentricEclipticOfDate),
	partitions(),
	extent(90.),
	centerLine(nullptr),
	linkStars(),
	offset(0.0)
{
	// Parse defining centerline type
	SkyLine::SKY_LINE_TYPE skylineType=SkyLine::ECLIPTIC_CULTURAL;
	if (json.contains("coordsys"))
	{
		static const QMap<QString, SkyLine::SKY_LINE_TYPE>map={
			{"ecliptical", SkyLine::ECLIPTIC_CULTURAL},
			{"equatorial", SkyLine::EQUATORIAL_CULTURAL}};
		skylineType=map.value(json["coordsys"].toString(), SkyLine::ECLIPTIC_CULTURAL);
		static const QMap<SkyLine::SKY_LINE_TYPE, StelCore::FrameType>frameMap={
			{SkyLine::ECLIPTIC_CULTURAL   , StelCore::FrameObservercentricEclipticOfDate},
			{SkyLine::EQUATORIAL_CULTURAL , StelCore::FrameEquinoxEqu}};
		frameType=frameMap.value(skylineType, StelCore::FrameObservercentricEclipticOfDate);
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

		// From this definition, build the list of sub-tick lists for the central line.
		QList<QList<double>>cParts;
		for (int i=0; i<partitions.length(); ++i)
		{
			QList<double> partList;
			double angle=0;
			double partNum=1.;
			for (int j=0; j<=i; ++j)
			{
				partNum*=partitions[j];
			}
			//qDebug() << "partNum" << partNum;
			while (angle <360.)
			{
				partList.append(angle);
				angle+=360./partNum;
				bool take=true;
				// build test...
				for (int j=0; j<=i; ++j)
				{
					if (cParts.length() > j && cParts.at(j).contains(angle))
					{
						take=false;
						//qDebug() << "Angle taken:" << QString::number(angle);
					}
				}
				if (take)
					partList.append(angle);
			}
			cParts.append(partList);
		}
		centerLine->setCulturalPartitions(cParts);
	}
	else if (json.contains("defining_stars"))
	{
		QJsonArray jStars=json["defining_stars"].toArray();
		for (unsigned int i=0; i<jStars.size(); ++i)
			linkStars.append(jStars.at(i).toInt());
	}
	else
	{
		qWarning() << "Neither \"partitions\" nor \"defining_stars\" array found in JSON data for zodiac or lunarSystem description";
	}

	// Parse names
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

	if (json.contains("link"))
	{
		Q_ASSERT(linkStars.isEmpty());
		QJsonObject obj=json["link"].toObject();
		linkStars.append(obj["star"].toInt());
		offset=obj["offset"].toDouble();
	}

	// Font size is 14
	font.setPixelSize(StelApp::getInstance().getScreenFontSize()+1);
	updateLabels();

	// Recapitulate what we have loaded:
	qDebug() << "Cultural Sky Partition: Loaded partitions:" << partitions << names.length() << "names, " << symbols.length() << "symbols." <<
		    linkStars.length() << "link stars. " << (linkStars.length()==1 ? QString("Offset %1 at star %2").arg(QString::number(offset), QString::number(linkStars.first())) : QString());
}

StelSkyCultureSkyPartition::~StelSkyCultureSkyPartition()
{
//	if (centerLine) delete centerLine;
}


void StelSkyCultureSkyPartition::draw(StelPainter& sPainter, const Vec3d &obsVelocity) const
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	static StarMgr *starMgr=GETSTELMODULE(StarMgr);
	static StelCore *core=StelApp::getInstance().getCore();
	StelProjectorP prj = core->getProjection(frameType, (frameType!=StelCore::FrameAltAz && frameType!=StelCore::FrameFixedEquatorial) ? StelCore::RefractionAuto : StelCore::RefractionOff);
	sPainter.setProjector(prj);	

	double offsetFromAries=0.;
	// If defined, find the necessary shift from single linkStar and offset
	if (linkStars.length()==1)
	{
		const StelObjectP star=starMgr->searchHP(linkStars.first());
		Vec3d pos=star->getEquinoxEquatorialPosAuto(core);
		double ra, dec;
		StelUtils::rectToSphe(&ra, &dec, pos);
		if (frameType==StelCore::FrameObservercentricEclipticOfDate)
		{
			const double ecl = GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(core->getJDE());
			double lambda, beta;
			Vec3d eclPos;
			StelUtils::equToEcl(ra, dec, ecl, &lambda, &beta);
			offsetFromAries=lambda*M_180_PI-offset;
		}
		else
			offsetFromAries=ra*M_180_PI-offset;
	}


	if (linkStars.length()>1)
	{
		// Chinese systems: Unequal partitions defined by stars.
		foreach(const int starId, linkStars)
		{
			//qDebug() << "drawing line for HIP"  << starId;
			StelObjectP star=starMgr->searchHP(starId);
			//const double lng=(360./partitions[0]*p +offsetFromAries)*M_PI_180;
			Vec3d posDate=star->getEquinoxEquatorialPos(core);
			double ra, dec;
			StelUtils::rectToSphe(&ra, &dec, posDate);
			Vec3d eqPt, nPt, sPt;
			StelUtils::spheToRect(ra, 0., eqPt);
			StelUtils::spheToRect(ra, extent*M_PI_180, nPt);
			StelUtils::spheToRect(ra, -extent*M_PI_180, sPt);
			sPainter.drawGreatCircleArc(eqPt, nPt);
			sPainter.drawGreatCircleArc(eqPt, sPt);
			//qDebug() << "done line for HIP"  << starId;

			// Mark the defining star with a circlet.
			SphericalCap scCircle(posDate, cos(0.25*M_PI_180));
			sPainter.drawSphericalRegion(&scCircle, StelPainter::SphericalPolygonDrawModeBoundary);
		}
	}
	else
	{
		// Draw the equal-sized major partitions
		for (int p=0; p<partitions[0]; ++p)
		{
			const double lng=(360./partitions[0]*p +offsetFromAries)*M_PI_180;
			Vec3d eqPt, nPt, sPt;
			StelUtils::spheToRect(lng, 0., eqPt);
			StelUtils::spheToRect(lng, extent*M_PI_180, nPt);
			StelUtils::spheToRect(lng, -extent*M_PI_180, sPt);
			sPainter.drawGreatCircleArc(eqPt, nPt);
			sPainter.drawGreatCircleArc(eqPt, sPt);
		}
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
	if (symbols.length() && linkStars.length()<2)
	{
		for (int i=0; i<partitions[0]; ++i)
		{
			// To have tilted labels, we project a point 0.1deg from the actual label point and derive screen-based angle.
			double lng  = (360./partitions[0]*i + 2.+offsetFromAries)*M_PI_180;
			double lng1 = (360./partitions[0]*i + 1.9+offsetFromAries)*M_PI_180;
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
	else if (symbols.length()==linkStars.length()  && linkStars.length()>1)
	{
		// Chinese Symbols
		for (int i=0; i<linkStars.length(); ++i)
		{
			QString label=symbols.at(i);
			StelObjectP starBegin = starMgr->searchHP(linkStars.at(i));

			Vec3d eq=starBegin->getEquinoxEquatorialPos(core);
			double ra, dec;
			StelUtils::rectToSphe(&ra, &dec, eq);

			ra += 2.*M_PI_180; // push a bit into the mansion area. Problem: the narrow mansions...
			double ra1 = ra-0.1*M_PI_180;
			double dec1  = (extent<50. ? -extent+0.2 : -10.) *M_PI_180;
			Vec3d pos, pos1, scr, scr1;
			StelUtils::spheToRect(ra, dec1, pos);
			StelUtils::spheToRect(ra1, dec1, pos1);
			prj->project(pos, scr);
			prj->project(pos1, scr1);
			float angle=atan2(scr1[1]-scr[1], scr1[0]-scr[0])*M_180_PIf;
			QFontMetrics metrics(font);
			float xShift= -0.5 * metrics.boundingRect(label).width();

			sPainter.drawText(pos, label, angle, xShift);
		}
	}

	// Labels:
	if (names.length()  && linkStars.length()<2)
	{
		for (int i=0; i<partitions[0]; ++i)
		{
			QString label=scMgr->createCulturalLabel(names.at(i), scMgr->getScreenLabelStyle(), names.at(i).pronounceI18n);
			// To have tilted labels, we project a point 0.1deg from the actual label point and derive screen-based angle.
			double lng  = (360./partitions[0]*(double(i)+0.5) + 2.+offsetFromAries)*M_PI_180;
			double lng1 = (360./partitions[0]*(double(i)+0.5) + 1.9+offsetFromAries)*M_PI_180;
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
	else if (names.length()==linkStars.length()  && linkStars.length()>1)
	{
		for (int i=0; i<linkStars.length(); ++i)
		{
			QString label=scMgr->createCulturalLabel(names.at(i), scMgr->getScreenLabelStyle(),names.at(i).pronounceI18n);
			StelObjectP starBegin = starMgr->searchHP(linkStars.at(i));
			StelObjectP starEnd   = starMgr->searchHP(linkStars.at((i==linkStars.length()-1? 0 : i+1)));

			Vec3d mid=starBegin->getEquinoxEquatorialPos(core)+starEnd->getEquinoxEquatorialPos(core); // no need to normalize!
			double ra, dec;
			StelUtils::rectToSphe(&ra, &dec, mid);

			double ra1 = ra-0.1*M_PI_180;
			double dec1  = (extent<50. ? -extent+0.2 : -10.) *M_PI_180;
			Vec3d pos, pos1, scr, scr1;
			StelUtils::spheToRect(ra, dec1, pos);
			StelUtils::spheToRect(ra1, dec1, pos1);
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

void StelSkyCultureSkyPartition::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	for (auto &name : names)
	{
		QString context = name.translated;
		name.translatedI18n = trans.tryQtranslate(name.translated, context);
		if (name.translatedI18n.isEmpty())
		{
			if (context.isEmpty())
				name.translatedI18n = q_(name.translated);
			else
				name.translatedI18n = qc_(name.translated, context);
		}
		name.pronounceI18n = trans.tryQtranslate(name.pronounce, context);
		if (name.pronounceI18n.isEmpty())
		{
			if (context.isEmpty())
				name.pronounceI18n = q_(name.pronounce);
			else
				name.pronounceI18n = qc_(name.pronounce, context);
		}
	}
}
