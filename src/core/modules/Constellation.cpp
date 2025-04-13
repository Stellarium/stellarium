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

#include "StelTexture.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "ConstellationMgr.hpp"
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
bool Constellation::singleSelected = false;
bool Constellation::seasonalRuleEnabled = false;
float Constellation::artIntensityFovScale = 1.0f;

Constellation::Constellation()
	: numberOfSegments(0)
	, beginSeason(0)
	, endSeason(0)
	, singleStarConstellationRadius(cos(M_PI/360.)) // default radius of 1/2 degrees
	, artOpacity(1.f)
{
}

Constellation::~Constellation()
{
}

bool Constellation::read(const QJsonObject& data, StarMgr *starMgr)
{
	const QString id = data["id"].toString();
	const QStringList idParts = id.split(" ");
	if (idParts.size() == 3 && idParts[0] == "CON")
	{
		abbreviation = idParts[2];
	}
	else
	{
		qWarning().nospace() << "Bad constellation id: expected \"CON cultureName Abbrev\", got " << id;
		return false;
	}

	const QJsonValue names = data["common_name"].toObject();
	culturalName.translated = names["english"].toString().trimmed();
	culturalName.native = names["native"].toString().trimmed();
	culturalName.pronounce = names["pronounce"].toString().trimmed();
	//if (culturalName.native.isEmpty())
	//{
	//	if (culturalName.pronounce.isEmpty())
	//		culturalName.native=culturalName.translated;
	//	else
	//		culturalName.native=culturalName.pronounce;
	//}
	culturalName.IPA = names["IPA"].toString().trimmed();
	culturalName.transliteration = names["transliteration"].toString().trimmed();

	context = names["context"].toString().trimmed();
	if (culturalName.translated.isEmpty() && culturalName.native.isEmpty() && culturalName.pronounce.isEmpty())
		qWarning() << "No name for constellation" << id;

	constellation.clear();
	const QJsonArray &linesArray=data["lines"].toArray();
	for (const auto& polyLineObj : linesArray)
	{
		const auto& polyLine = polyLineObj.toArray();
		if (polyLine.size() < 2) continue; // one point doesn't define a segment

		const auto numSegments = polyLine.size() - 1;
		constellation.reserve(constellation.size() + 2 * numSegments);

		StelObjectP prevPoint = nullptr;
		for (qsizetype i = 0; i < polyLine.size(); ++i)
		{
			if (polyLine[i].isString())
			{
				// Can be "thin" or "bold", but we don't support these modifiers yet, so ignore this entry
				const auto s = polyLine[i].toString();
				if (s == "thin" || s == "bold")
					continue;
			}
			const StarId HP = StelUtils::getLongLong(polyLine[i]);
			if (HP == 0)
			{
				qWarning().nospace() << "Error in constellation " << abbreviation << ": bad HIP " << HP;
				return false;
			}

			const auto newPoint = HP <= NR_OF_HIP ? starMgr->searchHP(HP)
			                                      : starMgr->searchGaia(HP);
			if (!newPoint)
			{
				qWarning().nospace() << "Error in constellation " << abbreviation << ": can't find star HIP " << HP;
				return false;
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
	XYZname.set(0.,0.,0.);
	for(unsigned int ii=0;ii<numberOfSegments*2;++ii)
	{
		XYZname+= constellation[ii]->getJ2000EquatorialPos(StelApp::getInstance().getCore());
	}
	XYZname.normalize();
	// Sometimes label placement is suboptimal. Allow a correction from the automatic solution in label_offset:[dRA_deg, dDec_deg]
	if (data.contains("label_offset"))
	{
		QJsonArray offset=data["label_offset"].toArray();
		if (offset.size()!=2)
			qWarning() << "Bad constellation label offset given for " << id << ", ignoring";
		else
		{
			double ra, dec;
			StelUtils::rectToSphe(&ra, &dec, XYZname);
			ra  += offset[0].toDouble()*M_PI_180;
			dec += offset[1].toDouble()*M_PI_180;
			StelUtils::spheToRect(ra, dec, XYZname);
		}
	}

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
	return getCultureLabel(GETSTELMODULE(StelSkyCultureMgr)->getScreenLabelStyle());
}
QString Constellation::getInfoLabel() const
{
	return getCultureLabel(GETSTELMODULE(StelSkyCultureMgr)->getInfoLabelStyle());
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

	if (checkVisibility())
	{
		sPainter.setColor(lineColor, lineFader.getInterstate());

		Vec3d star1;
		Vec3d star2;
		for (unsigned int i=0;i<numberOfSegments;++i)
		{
			star1=constellation[2*i]->getJ2000EquatorialPos(core);
			star2=constellation[2*i+1]->getJ2000EquatorialPos(core);
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
}

void Constellation::drawName(StelPainter& sPainter, bool abbreviateLabel) const
{
	if (nameFader.getInterstate()==0.0f)
		return;

	// TODO: Find a solution of fallbacks when components are missing?
	if (checkVisibility())
	{
		QString name = abbreviateLabel ? abbreviationI18n : getScreenLabel();
		sPainter.setColor(labelColor, nameFader.getInterstate());
		sPainter.drawText(static_cast<float>(XYname[0]), static_cast<float>(XYname[1]), name, 0., -sPainter.getFontMetrics().boundingRect(name).width()/2, 0, false);
	}
}

void Constellation::drawArtOptim(StelPainter& sPainter, const SphericalRegion& region, const Vec3d& obsVelocity) const
{
	if (checkVisibility())
	{
		const float intensity = artFader.getInterstate() * artOpacity * artIntensityFovScale;
		if (artTexture && intensity > 0.0f && region.intersects(boundingCap))
		{
			sPainter.setColor(intensity,intensity,intensity);

			// The texture is not fully loaded
			if (artTexture->bind()==false)
				return;

#ifdef Q_OS_LINUX
			// Unfortunately applying aberration to the constellation artwork causes ugly artifacts visible on Linux.
			// It is better to disable aberration in this case and have a tiny texture shift where it usually does not need to critically match.
			sPainter.drawStelVertexArray(artPolygon, false, Vec3d(0.));
#else
			sPainter.drawStelVertexArray(artPolygon, false, obsVelocity);
#endif
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

bool Constellation::checkVisibility() const
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

	getSolarLunarInfoString(core, flags);
	postProcessInfoString(str, flags);

	return str;
}


StelObjectP Constellation::getBrightestStarInConstellation(void) const
{
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
	return brightest;
}
