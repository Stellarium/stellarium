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
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "ConstellationMgr.hpp"

#include <algorithm>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QFontMetrics>

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
	, constellation(Q_NULLPTR)
	, artOpacity(1.f)
{
}

Constellation::~Constellation()
{
	delete[] constellation;
	constellation = Q_NULLPTR;
}

bool Constellation::read(const QString& record, StarMgr *starMgr)
{
	unsigned int HP;

	abbreviation.clear();
	numberOfSegments = 0;

	QString buf(record);
	QTextStream istr(&buf, QIODevice::ReadOnly);
	// allow mixed-case abbreviations now that they can be displayed on screen. We then need toUpper() in comparisons.
	istr >> abbreviation >> numberOfSegments;
	if (istr.status()!=QTextStream::Ok)
		return false;

	constellation = new StelObjectP[numberOfSegments*2];
	for (unsigned int i=0;i<numberOfSegments*2;++i)
	{
		HP = 0;
		istr >> HP;
		if(HP == 0)
		{
			return false;
		}

		constellation[i]=starMgr->searchHP(static_cast<int>(HP));
		if (!constellation[i])
		{
			qWarning() << "Error in Constellation " << abbreviation << ": can't find star HIP" << HP;
			return false;
		}
	}

	// Name tag should go to constellation's centre of gravity
	XYZname.set(0.,0.,0.);
	for(unsigned int ii=0;ii<numberOfSegments*2;++ii)
	{
		XYZname+= constellation[ii]->getJ2000EquatorialPos(StelApp::getInstance().getCore());
	}
	XYZname.normalize();

	return true;
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
			sPainter.drawGreatCircleArc(star1, star2, &viewportHalfspace);			
		}
	}
}

void Constellation::drawName(StelPainter& sPainter, ConstellationMgr::ConstellationDisplayStyle style) const
{
	if (nameFader.getInterstate()==0.0f)
		return;

	if (checkVisibility())
	{
		QString name;
		switch (style)
		{
			case ConstellationMgr::constellationsTranslated:
				name=nameI18;
				break;
			case ConstellationMgr::constellationsNative:
				name=nativeName;
				break;
			case ConstellationMgr::constellationsEnglish:
				name=englishName;
				break;
			case ConstellationMgr::constellationsAbbreviated:
				name=(abbreviation.startsWith('.') ? "" : abbreviation);
				break;
		}

		sPainter.setColor(labelColor, nameFader.getInterstate());
		sPainter.drawText(static_cast<float>(XYname[0]), static_cast<float>(XYname[1]), name, 0., -sPainter.getFontMetrics().boundingRect(name).width()/2, 0, false);
	}
}

void Constellation::drawArtOptim(StelPainter& sPainter, const SphericalRegion& region) const
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

			sPainter.drawStelVertexArray(artPolygon);
		}
	}
}

// Draw the art texture
void Constellation::drawArt(StelPainter& sPainter) const
{
	sPainter.setBlending(true, GL_ONE, GL_ONE);
	sPainter.setCullFace(true);
	SphericalRegionP region = sPainter.getProjector()->getViewportConvexPolygon();
	drawArtOptim(sPainter, *region);
	sPainter.setCullFace(false);
}

const Constellation* Constellation::isStarIn(const StelObject* s) const
{
	for(unsigned int i=0;i<numberOfSegments*2;++i)
	{
		// constellation[i]==s test was not working
		if (constellation[i]->getEnglishName()==s->getEnglishName())
		{
			// qDebug() << "Const matched. " << getEnglishName();
			return this;
		}
	}
	return Q_NULLPTR;
}

void Constellation::update(int deltaTime)
{
	lineFader.update(deltaTime);
	nameFader.update(deltaTime);
	artFader.update(deltaTime);
	boundaryFader.update(deltaTime);
}

void Constellation::drawBoundaryOptim(StelPainter& sPainter) const
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
			sPainter.drawGreatCircleArc(points->at(j), points->at(j+1), &viewportHalfspace);
		}
	}
}

bool Constellation::checkVisibility() const
{
	// Is supported seasonal rules by current starlore?
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
		QString shortname = getShortName();
		oss << "<h2>" << getNameI18n();
		if (!shortname.isEmpty() && shortname.toInt()==0)
			oss << " (" << shortname << ")";
		oss << "</h2>";
	}

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_("constellation")) << "<br />";

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
