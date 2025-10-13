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
#include <QFont>
#include <QFontMetrics>

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
	extent(QList<double>{90.}),
	centerLine(nullptr),
	linkStars(),
	offset(0.0),
	eclObl(0.0),
        offsetFromAries(0.0),
        context()
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

	// Find the context data for localization support
	if (json.contains("context"))
		context = json["context"].toString();

	// Parse extent: either one value defining north/south caps, or per-station limits.
	if (json.contains("extent") && json["extent"].isDouble())
	{
		extent.clear();
		extent.append(json["extent"].toDouble());
	}
	else if (json.contains("extent") && json["extent"].isArray())
	{
		extent.clear();
		QJsonArray ext=json["extent"].toArray();
		for (unsigned int i=0; i<ext.size(); ++i)
			extent.append(ext.at(i).toDouble());
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
	fontSize = StelApp::getInstance().getScreenFontSize()+1;
	updateLabels();

	// Recapitulate what we have loaded:
	qDebug() << "Cultural Sky Partition: Loaded partitions:" << partitions << names.length() << "names, " << symbols.length() << "symbols." <<
		    linkStars.length() << "link stars. " << (linkStars.length()==1 ? QString("Offset %1 at star %2").arg(QString::number(offset), QString::number(linkStars.first())) : QString());
}

StelSkyCultureSkyPartition::~StelSkyCultureSkyPartition()
{
//	if (centerLine) delete centerLine;
}

void StelSkyCultureSkyPartition::update(double deltaTime)
{
	static StelCore *core=StelApp::getInstance().getCore();
	static SolarSystem *solSys=GETSTELMODULE(SolarSystem);
	eclObl = solSys->getEarth()->getRotObliquity(core->getJDE());
	centerLine->update(deltaTime);
}


void StelSkyCultureSkyPartition::draw(StelPainter& sPainter, const Vec3d &obsVelocity)
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	static StarMgr *starMgr=GETSTELMODULE(StarMgr);
	static StelCore *core=StelApp::getInstance().getCore();
	StelProjectorP prj = core->getProjection(frameType, (frameType!=StelCore::FrameAltAz && frameType!=StelCore::FrameFixedEquatorial) ? StelCore::RefractionAuto : StelCore::RefractionOff);
	sPainter.setProjector(prj);
	QFont font=QGuiApplication::font();
	font.setPixelSize(fontSize);
	const bool lrFlipped=core->getFlipHorz();
	const bool vertFlipped=core->getFlipVert();
	const double txtOffset=lrFlipped ? 0.1 : -0.1; // coordinate offset for projecting text
	const float yShift = vertFlipped ? -fontSize : 0.f;
	const float screenScale = prj->getDevicePixelsPerPixel();

	// If defined, find the necessary shift from single linkStar and offset
	if (linkStars.length()==1)
	{
		const StelObjectP star=starMgr->searchHP(linkStars.first());
		Vec3d pos=star->getEquinoxEquatorialPosAuto(core);
		double ra, dec;
		StelUtils::rectToSphe(&ra, &dec, pos);
		if (frameType==StelCore::FrameObservercentricEclipticOfDate)
		{
			double lambda, beta;
			Vec3d eclPos;
			StelUtils::equToEcl(ra, dec, eclObl, &lambda, &beta);
			offsetFromAries=lambda*M_180_PI-offset;
		}
		else
			offsetFromAries=ra*M_180_PI-offset;
	}


	if (linkStars.length()>1)
	{
		// Chinese systems: Unequal partitions defined by stars. We can also safely assume extent has only one value.
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
			StelUtils::spheToRect(ra, extent.at(0)*M_PI_180, nPt);
			StelUtils::spheToRect(ra, -extent.at(0)*M_PI_180, sPt);
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
			double nExt, sExt;
			if (extent.length()==1)
			{
				nExt=extent.at(0);
				sExt=-extent.at(0);
			}
			else
			{
				Q_ASSERT(extent.length() == 2*partitions.length());
				nExt=qMax(extent.at(2*p),   extent.at(2*((p-1+partitions[0]) % partitions[0])));
				sExt=qMin(extent.at(2*p+1), extent.at(2*((p-1+partitions[0]) % partitions[0])+1));
			}
			StelUtils::spheToRect(lng, nExt*M_PI_180, nPt);
			StelUtils::spheToRect(lng, sExt*M_PI_180, sPt);
			sPainter.drawGreatCircleArc(eqPt, nPt);
			sPainter.drawGreatCircleArc(eqPt, sPt);
		}
	}

	// Draw top/bottom lines where applicable
	if ((extent.length()==1) && (extent.at(0)<90.))
	{
		// Get the bounding halfspace
		const SphericalCap& viewPortSphericalCap = sPainter.getProjector()->getBoundingCap();
		drawCap(sPainter, viewPortSphericalCap, extent.at(0));
		drawCap(sPainter, viewPortSphericalCap, -extent.at(0));
	}
	else if (extent.length()>1)
	{
		// TODO: Arab LM may show sectioned boxes.
		for (int p=0; p<partitions[0]; ++p)
		{
			// Get the bounding halfspace
			const SphericalCap& viewPortSphericalCap = sPainter.getProjector()->getBoundingCap();
			// TODO: draw 2 small arcs per LM in the given extent latitudes
			drawMansionCap(sPainter, viewPortSphericalCap, extent.at(2*p), p*360.0/partitions[0]+offsetFromAries, (p+1)*360.0/partitions[0]+offsetFromAries);
			drawMansionCap(sPainter, viewPortSphericalCap, extent.at(2*p+1), p*360.0/partitions[0]+offsetFromAries, (p+1)*360.0/partitions[0]+offsetFromAries);
		}
	}

	centerLine->setCulturalOffset(offsetFromAries);
	centerLine->draw(sPainter, 1.f); // The second arg. is irrelevant, will be restored again in the caller...

	// Symbols: At the beginning of the respective zone.
	if (symbols.length() && linkStars.length()<2)
	{
		for (int i=0; i<partitions[0]; ++i)
		{
			const QString symbol=symbols.at(i).trimmed();
			// To have tilted labels, we project a point 0.1deg from the actual label point and derive screen-based angle.
			double lng  = (360./partitions[0]*i + offsetFromAries)*M_PI_180;
			double lng1 = (360./partitions[0]*i + offsetFromAries + txtOffset)*M_PI_180;
			// the displayed latitude is defined in the first LM.
			double lat = (extent.at(0)<50. ? -extent.at(0)+0.2 : -10.) *M_PI_180;
			Vec3d pos, pos1, scr, scr1;
			StelUtils::spheToRect(lng, lat, pos);
			StelUtils::spheToRect(lng1, lat, pos1);
			prj->project(pos, scr);
			prj->project(pos1, scr1);
			double angle=atan2(scr1[1]-scr[1], scr1[0]-scr[0])*M_180_PI;
			QFontMetrics metrics(font);
			float xShift= -1.5 * metrics.boundingRect(symbol).width();
			sPainter.drawText(pos, symbol, angle, xShift*screenScale, yShift*screenScale);
		}
	}
	else if (symbols.length()==linkStars.length()  && linkStars.length()>1)
	{
		// Chinese Symbols
		for (int i=0; i<linkStars.length(); ++i)
		{
			const QString symbol=symbols.at(i).trimmed();
			StelObjectP starBegin = starMgr->searchHP(linkStars.at(i));

			Vec3d eq=starBegin->getEquinoxEquatorialPos(core);
			double ra, dec;
			StelUtils::rectToSphe(&ra, &dec, eq);

			//ra += 2.*M_PI_180; // push a bit into the mansion area. Problem: the narrow mansions...
			double ra1 = ra+txtOffset*M_PI_180;
			double dec1  = (extent.at(0)<50. ? -extent.at(0)+0.2 : -10.) *M_PI_180;
			Vec3d pos, pos1, scr, scr1;
			StelUtils::spheToRect(ra, dec1, pos);
			StelUtils::spheToRect(ra1, dec1, pos1);
			prj->project(pos, scr);
			prj->project(pos1, scr1);
			float angle=atan2(scr1[1]-scr[1], scr1[0]-scr[0])*M_180_PIf;
			QFontMetrics metrics(font);
			float xShift= -1.5 * metrics.boundingRect(symbol).width();

			sPainter.drawText(pos, symbol, angle, xShift*screenScale, yShift*screenScale);
		}
	}

	// Labels:
	if (names.length()  && linkStars.length()<2)
	{
		for (int i=0; i<partitions[0]; ++i)
		{
			const QString label=scMgr->createCulturalLabel(names.at(i), partitions[0]==12 ? scMgr->getZodiacLabelStyle() : scMgr->getLunarSystemLabelStyle(), QString()).trimmed();
			// To have tilted labels, we project a point 0.1deg from the actual label point and derive screen-based angle.
			const double lng  = (360./partitions[0]*(double(i)+0.5) + offsetFromAries)*M_PI_180;
			const double lng1 = (360./partitions[0]*(double(i)+0.5) + offsetFromAries+txtOffset)*M_PI_180;
			const double lat  = (extent.at(0)<50. ? -extent.at(0)+0.2 : -10.) *M_PI_180;
			Vec3d pos, pos1, scr, scr1;
			StelUtils::spheToRect(lng, lat, pos);
			StelUtils::spheToRect(lng1, lat, pos1);
			prj->project(pos, scr);
			prj->project(pos1, scr1);
			const float angle=atan2(scr1[1]-scr[1], scr1[0]-scr[0])*M_180_PIf;
			QFontMetrics metrics(font);
			float xShift= -0.5 * metrics.boundingRect(label).width();

			sPainter.drawText(pos, label, angle, xShift*screenScale, yShift*screenScale);
		}
	}
	else if (names.length()==linkStars.length()  && linkStars.length()>1)
	{
		for (int i=0; i<linkStars.length(); ++i)
		{
			QString label=scMgr->createCulturalLabel(names.at(i), scMgr->getLunarSystemLabelStyle(),names.at(i).pronounceI18n).trimmed();
			StelObjectP starBegin = starMgr->searchHP(linkStars.at(i));
			StelObjectP starEnd   = starMgr->searchHP(linkStars.at((i==linkStars.length()-1? 0 : i+1)));

			Vec3d mid=starBegin->getEquinoxEquatorialPos(core)+starEnd->getEquinoxEquatorialPos(core); // no need to normalize!
			double ra, dec;
			StelUtils::rectToSphe(&ra, &dec, mid);

			const double ra1 = ra+txtOffset*M_PI_180;
			const double dec1  = (extent.at(0)<50. ? -extent.at(0)+0.2 : -10.) *M_PI_180;
			Vec3d pos, pos1, scr, scr1;
			StelUtils::spheToRect(ra, dec1, pos);
			StelUtils::spheToRect(ra1, dec1, pos1);
			prj->project(pos, scr);
			prj->project(pos1, scr1);
			const float angle=atan2(scr1[1]-scr[1], scr1[0]-scr[0])*M_180_PIf;
			QFontMetrics metrics(font);
			float xShift= -0.5 * metrics.boundingRect(label).width();

			sPainter.drawText(pos, label, angle, xShift*screenScale, yShift*screenScale);
		}
	}
}

// shamelessly copied from SkyLine::draw (small circles part)
void StelSkyCultureSkyPartition::drawCap(StelPainter &sPainter, const SphericalCap& viewPortSphericalCap, double latDeg) const
{
	const double lat=latDeg*M_PI_180;
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

void StelSkyCultureSkyPartition::drawMansionCap(StelPainter &sPainter, const SphericalCap& viewPortSphericalCap, double latDeg, double lon1, double lon2) const
{
	const double lat=latDeg*M_PI_180;
	SphericalCap declinationCap(Vec3d(0.,0.,1.), std::sin(lat));
	const Vec3d rotCenter(0,0,declinationCap.d);
	Vec3d pt1, pt2;
	StelUtils::spheToRect(lon1*M_PI_180, lat, pt1); //pt1.normalize();
	StelUtils::spheToRect(lon2*M_PI_180, lat, pt2); //pt2.normalize();
	sPainter.drawSmallCircleArc(pt1, pt2, rotCenter, nullptr, nullptr);
}


void StelSkyCultureSkyPartition::setFontSize(int newFontSize)
{
	fontSize = newFontSize;
}

void StelSkyCultureSkyPartition::updateLabels()
{
	// TODO: Extract short cultural labels and apply here.
}

void StelSkyCultureSkyPartition::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	// The name of the system
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

	// names array
	for (auto &name : names)
	{
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

QString StelSkyCultureSkyPartition::getCulturalName() const
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	return scMgr->createCulturalLabel(name, scMgr->getScreenLabelStyle(), names.length()==12 ? q_("Zodiac") : q_("Lunar Station"));
}

QString StelSkyCultureSkyPartition::getLongitudeCoordinate(Vec3d &eqPos) const
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, eqPos);
	if (frameType==StelCore::FrameObservercentricEclipticOfDate)
	{
		// We can for now assume this is a Zodiac or Indian Lunar system. However, e.g. both Indian are shifted by offsetFromAries.
		double lambda, beta;
		StelUtils::equToEcl(ra, dec, eclObl, &lambda, &beta);
		double cultureLambda=StelUtils::fmodpos((lambda*M_180_PI-offsetFromAries), 360.); // Degrees from zero point of cultural scale
		const double widthOfSign=(360./partitions.at(0));
		int sign=int(floor(cultureLambda/widthOfSign));
		double startOfSign=sign*widthOfSign;
		double degreeInSign=cultureLambda-startOfSign;

		if (partitions.at(0)==12)
		{
			double minuteInSign=(degreeInSign-floor(degreeInSign))*60.;
			return QString("%1 %2°%3'").arg(symbols.at(sign), QString::number(int(floor(degreeInSign))), QString::number(int(floor(minuteInSign))));
		}
		else if (partitions.at(0)==27)
		{
			// Indian Nakshatras
			int padaInSign=int(floor((degreeInSign/widthOfSign)*4.));
			return QString("%1: %2").arg(symbols.at(sign), QString::number(padaInSign+1));
		}
		else if (partitions.at(0)==28)
		{
			// Arab LM: Presumably a pragmatic three-partition is in use, like "not quite here yet", "central", "slightly past".
			static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);

			int thirdInSign=int(floor((degreeInSign/widthOfSign)*3.));
			return QString("%1: %2").arg(scMgr->createCulturalLabel(names.at(sign), scMgr->getScreenLabelStyle(), QString()), QString::number(thirdInSign+1));
		}
		else
		{
			static bool reported=false;
			if (!reported)
			{
				qInfo() << "Unknown ecliptical partition scheme for " << name.translated;
				reported=true;
			}
			return QString();
		}
	}
	else
	{
		// We only know Chinese mansions here. Provide Mansion symbol+name. So far I don't know any further subdivision.
		static StarMgr *starMgr=GETSTELMODULE(StarMgr);
		static StelCore *core=StelApp::getInstance().getCore();

		const double raDeg=StelUtils::fmodpos(ra*M_180_PI, 360.); // object RA of date, degrees
		const Vec3d starPos0=starMgr->searchHP(linkStars.at(0))->getEquinoxEquatorialPos(core);
		double raStar0, decStar0;
		StelUtils::rectToSphe(&raStar0, &decStar0, starPos0); // start of mansion 0.
		const double raDeg0=StelUtils::fmodpos(raStar0*M_180_PI, 360.); // start RA of mansion 0, date, degrees
		const double lngInMansions=StelUtils::fmodpos(raDeg-raDeg0, 360.); // object's 'longitude' counted from the start of mansion 0 along the equator, degrees.

		int mansion=0;
		while (mansion<linkStars.length()-1)
		{
			int nextStarIdx=StelUtils::imod(mansion+1, 28);
			Vec3d starPos=starMgr->searchHP(linkStars.at(nextStarIdx))->getEquinoxEquatorialPos(core);
			double raStar, decStar;
			StelUtils::rectToSphe(&raStar, &decStar, starPos); // start of next mansion.
			double lngNxtMansion=StelUtils::fmodpos(raStar*M_180_PI-raDeg0, 360.);
			if (lngNxtMansion>lngInMansions)
				break;
			++mansion;
		}
		QString mnName=scMgr->createCulturalLabel(names.at(mansion), scMgr->getScreenLabelStyle(), names.at(mansion).pronounceI18n);


		return QString("%1 - %2").arg(symbols.at(mansion), mnName);
	}
}
