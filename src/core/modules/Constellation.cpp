/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
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

#include "StelProjector.hpp"
#include "Constellation.hpp"
#include "StarMgr.hpp"
#include "StelModuleMgr.hpp"
#include "NebulaMgr.hpp"

#include "StelTexture.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "ConstellationMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "ZoneArray.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyCultureMgr.hpp"

#include <QString>
#include <QJsonArray>
#include <QTextStream>
#include <QDebug>
#include <QFontMetrics>
#include <QIODevice>

const QString Constellation::CONSTELLATION_TYPE = QStringLiteral("Constellation");

Vec3f Constellation::lineColor = Vec3f(0.4f,0.4f,0.8f);
Vec3f Constellation::labelColor = Vec3f(0.4f,0.4f,0.8f);
Vec3f Constellation::boundaryColor = Vec3f(0.8f,0.3f,0.3f);
Vec3f Constellation::hullColor = Vec3f(0.6f,0.2f,0.2f);
bool Constellation::singleSelected = false;
bool Constellation::seasonalRuleEnabled = false;
float Constellation::artIntensityFovScale = 1.0f;

Constellation::Constellation()
	: numberOfSegments(0)
	, beginSeason(0)
	, endSeason(0)
	, singleStarConstellationRadius(cos(M_PI/360.)) // default radius of 1/2 degrees
	, convexHull(nullptr)
	, artOpacity(1.f)
{
}

Constellation::~Constellation()
{
}

bool Constellation::read(const QJsonObject& data, StarMgr *starMgr)
{
	static NebulaMgr *nebulaMgr=GETSTELMODULE(NebulaMgr);
	static StelCore *core=StelApp::getInstance().getCore();
	const QString id = data["id"].toString();
	const QStringList idParts = id.split(" ");
	if (idParts.size() == 3 && idParts[0] == "CON")
	{
		abbreviation = idParts[2].trimmed();
	}
	else
	{
		qWarning().nospace() << "Bad constellation id: expected \"CON cultureName Abbrev\", got " << id;
		return false;
	}

	const QJsonValue names = data["common_name"].toObject();
	culturalName.translated = names["english"].toString().trimmed();
	culturalName.byname = names["byname"].toString().trimmed();
	culturalName.native = names["native"].toString().trimmed();
	culturalName.pronounce = names["pronounce"].toString().trimmed();
	culturalName.IPA = names["IPA"].toString().trimmed();
	culturalName.transliteration = names["transliteration"].toString().trimmed();

	context = names["context"].toString().trimmed();
	if (culturalName.translated.isEmpty() && culturalName.native.isEmpty() && culturalName.pronounce.isEmpty())
		qWarning() << "No name for constellation" << id;

	constellation.clear();
	const QJsonArray &linesArray=data["lines"].toArray();
	if (linesArray.isEmpty())
	{
		qWarning().nospace() << "Empty lines array found for constellation " << id << " (" << culturalName.native << ")";
		return false;
	}

	const bool isDarkConstellation = (!linesArray[0].toArray().isEmpty() && linesArray[0].toArray()[0].isArray());
	if (isDarkConstellation)
	{
		for (const auto& polyLineObj : linesArray) // auto=[[ra, dec], [ra, dec], ...]
		{
			const auto& polyLine = polyLineObj.toArray();
			if (polyLine.size() < 2) continue; // one point doesn't define a segment

			const auto numSegments = polyLine.size() - 1;
			dark_constellation.reserve(dark_constellation.size() + 2 * numSegments);

			Vec3d prevPoint = Vec3d(0.);
			int darkConstLineIndex=0;
			for (qsizetype i = 0; i < polyLine.size(); ++i)
			{
				if (polyLine[i].isString())
				{
					// Can be "thin" or "bold", but we don't support these modifiers yet, so ignore this entry
					const auto s = polyLine[i].toString();
					if (s == "thin" || s == "bold")
						continue;
				}

				if (!polyLine[i].isArray())
				{
					qWarning().nospace() << "Error in constellation " << id << ": bad point"
							     << polyLine[i].toString() << ": isn't an array of two numbers (RA and dec)";
					return false;
				}
				const QJsonArray arr = polyLine[i].toArray();
				if (arr.size() != 2 || !arr[0].isDouble() || !arr[1].isDouble())
				{
					qWarning().nospace() << "Error in constellation " << id << ": bad point"
							     << polyLine[i].toString() << ": isn't an array of two numbers (RA and dec)";
					return false;
				}

				const double RA = arr[0].toDouble() * (M_PI_180*15.);
				const double DE = arr[1].toDouble() * M_PI_180;
				Vec3d newPoint;
				StelUtils::spheToRect(RA, DE, newPoint);

				if (prevPoint != Vec3d(0.))
				{
					dark_constellation.push_back(QSharedPointer<StelObject>(new CoordObject(QString("%1_%2.A").arg(abbreviation, QString::number(darkConstLineIndex)), prevPoint)));
					dark_constellation.push_back(QSharedPointer<StelObject>(new CoordObject(QString("%1_%2.B").arg(abbreviation, QString::number(darkConstLineIndex)), newPoint)));
				}
				prevPoint = newPoint;
				++darkConstLineIndex;
			}
		}
		numberOfSegments = dark_constellation.size() / 2;
		// Name tag should go to constellation's centre of gravity
		Vec3d XYZname1(0.);
		for(unsigned int ii=0;ii<numberOfSegments*2;ii+=2)
		{
			XYZname1 += dark_constellation[ii]->getJ2000EquatorialPos(nullptr);
		}
		XYZname1.normalize();
		XYZname.append(XYZname1);
	}
	else
	{
		for (const auto& polyLineObj : linesArray)
		{
			const auto& polyLine = polyLineObj.toArray();
			if (polyLine.size() < 2) continue; // one point doesn't define a segment

			const auto numSegments = polyLine.size() - 1;
			constellation.reserve(constellation.size() + 2 * numSegments);

			StelObjectP prevPoint = nullptr;
			for (qsizetype i = 0; i < polyLine.size(); ++i)
			{
				StelObjectP newPoint;
				if (polyLine[i].isString() && polyLine[i].toString().startsWith("DSO:"))
				{
					QString DSOname=polyLine[i].toString().remove(0,4);
					newPoint = nebulaMgr->searchByID(DSOname);
					if (!newPoint)
					{
						qWarning().nospace() << "Error in constellation " << abbreviation << ": can't find DSO " << DSOname << "... skipping constellation";
						return false;
					}
				}
				else
				{
					if (polyLine[i].isString())
					{
						// Can be "thin" or "bold", but we don't support these modifiers yet, so ignore this entry
						const auto s = polyLine[i].toString();
						if (s == "thin" || s == "bold")
							continue;
					}
					const StarId HP = StelUtils::getLongLong(polyLine[i]);
					if (HP > 0)
					{
						newPoint = HP <= NR_OF_HIP ? starMgr->searchHP(HP)
									   : starMgr->searchGaia(HP);
						if (!newPoint)
						{
							qWarning().nospace() << "Error in constellation " << abbreviation << ": can't find star HIP " << HP << "... skipping constellation";
							return false;
						}
					}
				}

				if (prevPoint)
				{
					constellation.push_back(prevPoint);
					constellation.push_back(newPoint);
				}
				prevPoint = newPoint;
			}
		}

		numberOfSegments = constellation.size() / 2;
		if (data.contains("single_star_radius"))
		{
			double rd = data["single_star_radius"].toDouble(0.5);
			singleStarConstellationRadius = cos(rd*M_PI/180.);
		}

		// Name tag should go to constellation's centre of gravity
		Vec3d XYZname1(0.);
		for(unsigned int ii=0;ii<numberOfSegments*2;++ii)
		{
			XYZname1 += constellation[ii]->getJ2000EquatorialPos(core);
		}
		XYZname1.normalize();
		XYZname.append(XYZname1);
	}

	// At this point we have either a constellation or a dark_constellation filled

	// Sometimes label placement is suboptimal. Allow a correction from the automatic solution in label_offset:[dRA_deg, dDec_deg]
	if (data.contains("label_offset"))
	{
		QJsonArray offset=data["label_offset"].toArray();
		if (offset.size()!=2)
			qWarning() << "Bad constellation label offset given for " << id << ", ignoring";
		else
		{
			double ra, dec;
			StelUtils::rectToSphe(&ra, &dec, XYZname[0]);
			ra  += offset[0].toDouble()*M_PI_180;
			dec += offset[1].toDouble()*M_PI_180;
			StelUtils::spheToRect(ra, dec, XYZname[0]);
		}
	}
	// Manual label placement: Manual positions can have more than one (e.g., Serpens has two parts). In this case, discard automatically derived position.
	if (data.contains("label_positions"))
	{
		XYZname.clear();
		const QJsonArray &labelPosArray=data["label_positions"].toArray();
		for (const auto& labelPos : labelPosArray)
		{
			const auto& labelArray=labelPos.toArray();
			if (labelArray.size() != 2)
			{
				qWarning() << "Bad label position given for constellation" << id << "... skipping";
				continue;
			}
			const double RA = labelArray[0].toDouble() * (M_PI_180*15.);
			const double DE = labelArray[1].toDouble() * M_PI_180;
			Vec3d newPoint;
			StelUtils::spheToRect(RA, DE, newPoint);
			XYZname.append(newPoint);
		}
	}

	//qDebug() << "Convex hull for " << englishName;
	if (data.contains("hull_extension"))
	{
		const QJsonArray &hullExtraArray=data["hull_extension"].toArray();

		for (qsizetype i = 0; i < hullExtraArray.size(); ++i)
		{
			if (hullExtraArray[i].isString() && hullExtraArray[i].toString().startsWith("DSO:"))
			{
				QString DSOname=hullExtraArray[i].toString().remove(0,4);
				const StelObjectP newDSO = nebulaMgr->searchByID(DSOname);
				if (!newDSO)
				{
					qWarning().nospace() << "Error in hull_extension for constellation " << abbreviation << ": can't find DSO " << DSOname << "... skipping";
				}
				else
					hullExtension.push_back(newDSO);
			}
			else
			{
				const StarId HP = StelUtils::getLongLong(hullExtraArray[i]);
				if (HP > 0)
				{
					const StelObjectP newStar = HP <= NR_OF_HIP ? starMgr->searchHP(HP)
										    : starMgr->searchGaia(HP);
					if (!newStar)
					{
						qWarning().nospace() << "Error in hull_extension for constellation " << abbreviation << ": can't find StarId " << HP << "... skipping";
					}
					else
						hullExtension.push_back(newStar);
				}
				else
				{
					qWarning().nospace() << "Error in hull_extension for constellation " << abbreviation << ": bad element: " << hullExtraArray[i].toString() << "... skipping";
				}
			}
		}
	}
	hullRadius=data["hull_radius"].toDouble(data["single_star_radius"].toDouble(0.5));

	convexHull=makeConvexHull(constellation, hullExtension, dark_constellation, XYZname.constFirst(), hullRadius);

	beginSeason = 1;
	endSeason = 12;
	const auto visib = data["visibility"];
	if (visib.isUndefined()) return true;
	const auto visibility = visib.toObject();
	const auto months = visibility["months"].toArray();
	if (months.size() != 2)
	{
		qWarning() << "Unexpected format of \"visibility\" entry in constellation" << id;
		return true; // not critical
	}
	beginSeason = months[0].toInt();
	endSeason = months[1].toInt();
	seasonalRuleEnabled = true;

	return true;
}

QString Constellation::getScreenLabel() const
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	return getCultureLabel(scMgr->getScreenLabelStyle());
}
QString Constellation::getInfoLabel() const
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	return getCultureLabel(scMgr->getInfoLabelStyle());
}

QString Constellation::getCultureLabel(StelObject::CulturalDisplayStyle style) const
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	return scMgr->createCulturalLabel(culturalName, style, getNameI18n(), abbreviationI18n);
}

void Constellation::drawOptim(StelPainter& sPainter, const StelCore* core, const SphericalCap& viewportHalfspace) const
{
	if (lineFader.getInterstate()<=0.0001f)
		return;
	if (!isSeasonallyVisible())
		return;

	const bool isDarkConstellation = !(dark_constellation.empty());
	const float darkFactor = (isDarkConstellation? 0.6667f : 1.0f);
	sPainter.setColor(lineColor*darkFactor, lineFader.getInterstate());

	if (isDarkConstellation)
		for (unsigned int i=0;i<numberOfSegments;++i)
		{
			Vec3d pos1=dark_constellation[2*i]->getJ2000EquatorialPos(core);
			Vec3d pos2=dark_constellation[2*i+1]->getJ2000EquatorialPos(core);
			//pos1.normalize();
			//pos2.normalize();
			sPainter.drawGreatCircleArc(pos1, pos2, &viewportHalfspace);
		}
	else
		for (unsigned int i=0;i<numberOfSegments;++i)
		{
			Vec3d star1=constellation[2*i]->getJ2000EquatorialPos(core);
			Vec3d star2=constellation[2*i+1]->getJ2000EquatorialPos(core);
			star1.normalize();
			star2.normalize();
			if (star1.fuzzyEquals(star2))
			{
				// draw single-star segment as circle
				SphericalCap scCircle(star1, singleStarConstellationRadius);
				sPainter.drawSphericalRegion(&scCircle, StelPainter::SphericalPolygonDrawModeBoundary);
			}
			else
				sPainter.drawGreatCircleArc(star1, star2, &viewportHalfspace);
		}
}

// observer centered J2000 coordinates.
// These are either automatically computed from all stars forming the lines,
// or from the manually defined label point(s).
Vec3d Constellation::getJ2000EquatorialPos(const StelCore*) const
{
	if (XYZname.length() ==1)
		return XYZname.first();
	else
	{
		Vec3d point(0.0);
		for (Vec3d namePoint: XYZname)
		{
			point += namePoint;
		}
		point.normalize();
		return point;
	}
}

void Constellation::drawName(const Vec3d &xyName, StelPainter& sPainter) const
{
	if (nameFader.getInterstate()==0.0f)
		return;

	// TODO: Find a solution of fallbacks when components are missing?
	if (isSeasonallyVisible())
	{
		QString name = getScreenLabel();
		sPainter.setColor(labelColor, nameFader.getInterstate());
		sPainter.drawText(static_cast<float>(xyName[0]), static_cast<float>(xyName[1]), name, 0., -sPainter.getFontMetrics().boundingRect(name).width()/2, 0, false);
	}
}

void Constellation::drawArtOptim(StelPainter& sPainter, const SphericalRegion& region, const Vec3d& obsVelocity) const
{
	if (isSeasonallyVisible())
	{
		const float intensity = artFader.getInterstate() * artOpacity * artIntensityFovScale;
		if (artTexture && intensity > 0.0f && region.intersects(boundingCap))
		{
			sPainter.setColor(intensity,intensity,intensity);

			// The texture is not fully loaded
			if (artTexture->bind()==false)
				return;

			sPainter.drawStelVertexArray(artPolygon, false, obsVelocity);
		}
	}
}

// Draw the art texture
void Constellation::drawArt(StelPainter& sPainter) const
{
	// Is this ever used?
	Q_ASSERT(0);
	sPainter.setBlending(true, GL_ONE, GL_ONE);
	sPainter.setCullFace(true);
	SphericalRegionP region = sPainter.getProjector()->getViewportConvexPolygon();
	drawArtOptim(sPainter, *region, Vec3d(0.));
	sPainter.setCullFace(false);
}

const Constellation* Constellation::isStarIn(const StelObject* s) const
{
	if (constellation.empty())
		return nullptr;

	for(unsigned int i=0;i<numberOfSegments*2;++i)
	{
		// constellation[i]==s test was not working
		if (constellation[i]->getID()==s->getID()) // don't compare englishNames, we may have duplicate names!
		{
			// qDebug() << "Const matched. " << getEnglishName();
			return this;
		}
	}
	return nullptr;
}

void Constellation::update(int deltaTime)
{
	lineFader.update(deltaTime);
	nameFader.update(deltaTime);
	artFader.update(deltaTime);
	boundaryFader.update(deltaTime);
	hullFader.update(deltaTime);
}

void Constellation::drawBoundaryOptim(StelPainter& sPainter, const Vec3d& obsVelocity) const
{
	if (boundaryFader.getInterstate()==0.0f)
		return;

	sPainter.setBlending(true);
	sPainter.setColor(boundaryColor, boundaryFader.getInterstate());

	unsigned int i, j;
	size_t size;
	std::vector<Vec3d> *points;

	if (singleSelected) size = isolatedBoundarySegments.size();
	else size = sharedBoundarySegments.size();

	const SphericalCap& viewportHalfspace = sPainter.getProjector()->getBoundingCap();

	for (i=0;i<size;i++)
	{
		if (singleSelected) points = isolatedBoundarySegments[i];
		else points = sharedBoundarySegments[i];

		for (j=0;j<points->size()-1;j++)
		{
			Vec3d point0=points->at(j)+obsVelocity;
			point0.normalize();
			Vec3d point1=points->at(j+1)+obsVelocity;
			point1.normalize();

			sPainter.drawGreatCircleArc(point0, point1, &viewportHalfspace);
		}
	}
}

void Constellation::drawHullOptim(StelPainter& sPainter, const Vec3d& obsVelocity) const
{
	if (hullFader.getInterstate()==0.0f || (!convexHull) )
		return;

	sPainter.setBlending(true);
	sPainter.setColor(hullColor, hullFader.getInterstate());

	sPainter.drawSphericalRegion(convexHull.data(), StelPainter::SphericalPolygonDrawModeBoundary, nullptr, true, 5, obsVelocity); // draw nice with aberration
}


bool Constellation::isSeasonallyVisible() const
{
	// Is supported seasonal rules by current sky culture?
	if (!seasonalRuleEnabled)
		return true;

	bool visible = false;
	int year, month, day;
	// Get the current month
	StelUtils::getDateFromJulianDay(StelApp::getInstance().getCore()->getJD(), &year, &month, &day);
	if (endSeason >= beginSeason)
	{
		// OK, it's a "normal" season rule...
		if ((month >= beginSeason) && (month <= endSeason))
			visible = true;
	}
	else
	{
		// ...oops, it's a "inverted" season rule
		if (((month>=1) && (month<=endSeason)) || ((month>=beginSeason) && (month<=12)))
			visible = true;
	}
	return visible;
}

QString Constellation::getInfoString(const StelCore *core, const InfoStringGroup &flags) const
{
	Q_UNUSED(core)
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>" << getInfoLabel() << "</h2>";
	}

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), getObjectTypeI18n()) << "<br />";

	oss << getSolarLunarInfoString(core, flags);
	postProcessInfoString(str, flags);

	return str;
}

StelObjectP Constellation::getBrightestStarInConstellation(void) const
{
	if (constellation.empty())
		return nullptr;

	float maxMag = 99.f;
	StelObjectP brightest;
	// maybe the brightest star has always odd index,
	// so check all segment endpoints:
	for (int i=2*static_cast<int>(numberOfSegments)-1;i>=0;i--)
	{
		const float Mag = constellation[i]->getVMagnitude(Q_NULLPTR);
		if (Mag < maxMag)
		{
			brightest = constellation[i];
			maxMag = Mag;
		}
	}
	for (unsigned int i=0; i<hullExtension.size();++i)
	{
		const float Mag = hullExtension[i]->getVMagnitude(Q_NULLPTR);
		if (Mag < maxMag)
		{
			brightest = hullExtension[i];
			maxMag = Mag;
		}
	}
	return brightest;
}

struct hullEntry
{
	StelObjectP obj;
	double x;
	double y;
};
// simple function only for ordering from Sedgewick 1990, Algorithms in C (p.353) used for sorting.
// The result fulfills the same purpose as some atan2, but with simpler operations.
static double theta(const hullEntry &p1, const hullEntry &p2)
{
	const double dx = p2.x-p1.x;
	const double ax = abs(dx);
	const double dy = p2.y-p1.y;
	const double ay = abs(dy);
	const double asum = ax+ay;
	double t = (asum==0) ? 0.0 : dy/asum;
	if (dx<0.)
		t=2-t;
	else if (dy<0.)
		t=4+t;
	return t*M_PI_2;
}

SphericalRegionP Constellation::makeConvexHull(const std::vector<StelObjectP> &starLines, const std::vector<StelObjectP> &hullExtension, const std::vector<StelObjectP> &darkLines, const Vec3d projectionCenter, const double hullRadius)
{
	static StelCore *core=StelApp::getInstance().getCore();
	// 1. Project every first star of a line pair (or just coordinates from a dark constellation) into a 2D tangential plane around projectionCenter.
	// Stars more than 90° from center cannot be projected of course and are skipped.
	double raC, deC;
	StelUtils::rectToSphe(&raC, &deC, projectionCenter);

	// starLines contains pairs of vertices, and some stars (or DSO) occur more than twice.
	QStringList idList;
	QList<StelObjectP>uniqueObjectList;
	foreach(auto &obj, starLines)
	{
		if (!idList.contains(obj->getID()))
		{
			// Take this star into consideration. However, the "star"s may be pointers to the same star, we must compare IDs.
			idList.append(obj->getID());
			uniqueObjectList.append(obj);
		}
	}
	foreach(auto &obj, hullExtension)
	{
		if (!idList.contains(obj->getID()))
		{
			// Take this star into consideration. However, the "star"s may be pointers to the same star, we must compare IDs.
			idList.append(obj->getID());
			uniqueObjectList.append(obj);
		}
	}
	foreach(auto &obj, darkLines)
	{
		if (!idList.contains(obj->getID()))
		{
			// Take this coordinate into consideration. However, the elements may be pointers to the same direction, we must compare IDs.
			idList.append(obj->getID());
			uniqueObjectList.append(obj);
		}
	}

	QList<hullEntry> hullList;
	// Perspective (gnomonic) projection from Snyder 1987, Map Projections: A Working Manual (USGS).
	// we must use an almost-dummy object type with unique name.
	foreach(auto &obj, uniqueObjectList)
	{
		hullEntry entry;
		entry.obj=obj;
		double ra, de;
		StelUtils::rectToSphe(&ra, &de, entry.obj->getJ2000EquatorialPos(core));
		const double cosC=sin(deC)*sin(de) + cos(deC)*cos(de)*cos(ra-raC);
		if (cosC<=0.) // distance 90° or more from projection center? Discard!
		{
			qWarning() << "Cannot include object" << entry.obj->getID() <<  "in convex hull: too far from projection center.";
			continue;
		}
		const double kP=1./cosC;
		entry.x = -kP*cos(de)*sin(ra-raC); // x must be negative here.
		entry.y =  kP*(cos(deC)*sin(de)-sin(deC)*cos(de)*cos(ra-raC));
		hullList.append(entry);
		//qDebug().noquote().nospace() << "[ " << entry.x << " " << entry.y << " " << ra*M_180_PI/15 << " " << de*M_180_PI << " (" << entry.obj->getID() << ") ]"; // allows Postscript graphics, looks OK.
	}
	//qDebug() << "Hull candidates: " << hullList.length();
	if (hullList.count() < 3)
	{
		//qDebug() << "List length" << hullList.count() << " not enough for a convex hull... create circular area";
		SphericalRegionP res;
		switch (hullList.count())
		{
			case 0:
				res = new EmptySphericalRegion;
				break;
			case 1:
				res = new SphericalCap(projectionCenter, cos(hullRadius*M_PI_180));
				break;
			case 2:
				double halfDist=0.5*acos(hullList.at(0).obj->getJ2000EquatorialPos(core).dot(hullList.at(1).obj->getJ2000EquatorialPos(core)));
				res = new SphericalCap(projectionCenter, cos(halfDist+hullRadius*M_PI_180));
				break;
		}
		return res;
	}

	// 2. Apply Package Wrapping from Sedgewick 1990, Algorithms in C to find the outer points wrapping all points.
	// find minY
	int min=0;
	for(int i=1; i<hullList.count(); ++i)
	{
		const hullEntry &entry=hullList.at(i);
		if (entry.y<hullList.at(min).y)
			min=i;
	}
	//qDebug() << "min entry is " << hullList.at(min).obj->getID();

	const int N=hullList.count(); // N...number of unique stars in constellation.
	//qDebug() << "unique stars N=" << N;
	hullList.append(hullList.at(min));

	//QStringList debugList;
	//// DUMP HULL LINE
	//for(int i=0; i<hullList.count(); ++i)
	//{
	//	const hullEntry &entry=hullList.at(i);
	//	debugList << entry.obj->getID();
	//}
	//qDebug() << "Hull candidate: " << debugList.join(" - ");
	//debugList.clear();

	int M=0;
	double th=0.0;
	for (M=0; M<N; ++M)
	{
#if (QT_VERSION<QT_VERSION_CHECK(5,13,0))
		std::swap(hullList[M], hullList[min]);
#else
		hullList.swapItemsAt(M, min);
#endif

		//// DUMP HULL LINE
		//for(int i=0; i<hullList.count(); ++i)
		//{
		//	const hullEntry &entry=hullList.at(i);
		//	debugList << entry.obj->getID();
		//	if (i==M)
		//		debugList << "|";
		//}
		//qDebug() << "Hull candidate after swap at M=" << M << debugList.join(" - ");
		//debugList.clear();

		min=N; double v=th; th=M_PI*2.0;
		for (int i=M+1; i<=N; ++i)
		{
			//qDebug() << "From M:" << M << "(" << hullList.at(M).obj->getID() << ") to i: " << i << "=" << hullList.at(i).obj->getID() << ": th=" << theta(hullList.at(M), hullList.at(i)) * M_180_PI ;

			if (theta(hullList.at(M), hullList.at(i)) > v)
				if(theta(hullList.at(M), hullList.at(i))< th)
				{
					min=i;
					th=theta(hullList.at(M), hullList.at(min));
					//qDebug() << "min:" << min << "th:" << th * M_180_PI;
				}
		}

		if (min==N)
		{
			//qDebug().nospace() << "min==N=" << N << ", we're done sorting. Hull should be of length M=" << M;
			break; // now M+1 is holds number of "valid" hull points.
		}
	}
	++M;
	//qDebug() << "Hull length" << M << "of" << hullList.count();
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
	for (int e=0; e<=N-M; ++e) hullList.pop_back();
#else
	hullList.remove(M, N-M);
#endif
	//hullList.remove(M-1, N-M+1);

	//// DUMP HULL LINE
	//for(int i=0; i<hullList.count(); ++i)
	//{
	//	const hullEntry &entry=hullList.at(i);
	//	debugList << entry.obj->getID();
	//}
	//qDebug() << "Final Hull, M=" << M << debugList.join(" - ");
	//debugList.clear();

	//Now create a SphericalRegion
	QList<Vec3d> hullPoints;
	foreach(const hullEntry &entry, hullList)
	{
		Vec3d pos=entry.obj->getJ2000EquatorialPos(core);
		hullPoints.append(pos);
	}
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
	SphericalPolygon *hull=new SphericalPolygon(hullPoints.toVector());
#else
	SphericalConvexPolygon *hull=new SphericalConvexPolygon(hullPoints);
#endif
	//qDebug() << "Successful hull:" << hull->toJSON();
	return hull;
}

void Constellation::makeConvexHull()
{
	// For 2-star automatic hulls, we must recreate the default XYZname (balance point) as hull circle center.
	if (constellation.size()==2 && XYZname.length()==1)
	{
		static StelCore *core=StelApp::getInstance().getCore();
		Vec3d XYZname1(0.);
		for(unsigned int i=0;i<constellation.size();++i)
		{
			XYZname1 += constellation.at(i)->getJ2000EquatorialPos(core);
		}
		XYZname1.normalize();
		convexHull=makeConvexHull(constellation, hullExtension, dark_constellation, XYZname1, hullRadius);
	}
	else
		convexHull=makeConvexHull(constellation, hullExtension, dark_constellation, XYZname.constFirst(), hullRadius);
}
