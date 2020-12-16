/*
 * Stellarium
 * Copyright (C) 2017 Alexander Wolf
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
#include "Asterism.hpp"
#include "StarMgr.hpp"

#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "AsterismMgr.hpp"

#include <algorithm>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QFontMetrics>

Vec3f Asterism::lineColor = Vec3f(0.4f,0.4f,0.8f);
Vec3f Asterism::rayHelperColor = Vec3f(1.0f,1.0f,0.0f);
Vec3f Asterism::labelColor = Vec3f(0.4f,0.4f,0.8f);
const QString Asterism::ASTERISM_TYPE = QStringLiteral("Asterism");

Asterism::Asterism()
	: numberOfSegments(0)
	, typeOfAsterism(1)
	, flagAsterism(true)
	, asterism(Q_NULLPTR)
{
}

Asterism::~Asterism()
{
	delete[] asterism;
	asterism = Q_NULLPTR;
}

bool Asterism::read(const QString& record, StarMgr *starMgr)
{
	abbreviation.clear();
	numberOfSegments = 0;
	typeOfAsterism = 1;
	flagAsterism = true;

	QString buf(record);
	QTextStream istr(&buf, QIODevice::ReadOnly);
	// We allow mixed-case abbreviations now that they can be displayed on screen. We then need toUpper() in comparisons.
	istr >> abbreviation >> typeOfAsterism >> numberOfSegments;
	if (istr.status()!=QTextStream::Ok)
		return false;

	StelCore *core = StelApp::getInstance().getCore();
	asterism = new StelObjectP[numberOfSegments*2];
	for (unsigned int i=0;i<numberOfSegments*2;++i)
	{
		switch (typeOfAsterism)
		{
			case 0: // Ray helpers
			case 1: // A big asterism with lines by HIP stars
			{
				unsigned int HP = 0;
				istr >> HP;
				if(HP == 0)
				{
					return false;
				}
				asterism[i]=starMgr->searchHP(static_cast<int>(HP));
				if (!asterism[i])
				{
					qWarning() << "Error in Asterism " << abbreviation << ": can't find star HIP" << HP;
					return false;
				}
				break;
			}
			case 2: // A small asterism with lines by J2000.0 coordinates
			{
				double RA, DE;
				Vec3d coords;

				istr >> RA >> DE;				
				StelUtils::spheToRect(RA*M_PI/12., DE*M_PI/180., coords);
				QList<StelObjectP> stars = starMgr->searchAround(coords, 0.1, core);
				StelObjectP s = Q_NULLPTR;
				double d = 10.;
				for (const auto& p : stars)
				{
					double a = coords.angle(p->getJ2000EquatorialPos(core));
					if (a<d)
					{
						d = a;
						s = p;
					}
				}
				asterism[i] = s;
				if (!asterism[i])
				{
					qWarning() << "Error in Asterism " << abbreviation << ": can't find star with coordinates" << RA << "/" << DE;
					return false;
				}
				break;
			}
		}
	}

	if (typeOfAsterism>0)
	{
		XYZname.set(0.,0.,0.);
		for(unsigned int j=0;j<numberOfSegments*2;++j)
			XYZname+= asterism[j]->getJ2000EquatorialPos(core);
		XYZname.normalize();
	}
	else
		flagAsterism = false;

	return true;
}

void Asterism::drawOptim(StelPainter& sPainter, const StelCore* core, const SphericalCap& viewportHalfspace) const
{
	if (flagAsterism)
	{
		if (lineFader.getInterstate()<=0.0001f)
			return;

		sPainter.setColor(lineColor, lineFader.getInterstate());
	}
	else
	{
		if (rayHelperFader.getInterstate()<=0.0001f)
			return;

		sPainter.setColor(rayHelperColor, rayHelperFader.getInterstate());
	}

	Vec3d star1;
	Vec3d star2;
	for (unsigned int i=0;i<numberOfSegments;++i)
	{
		star1=asterism[2*i]->getJ2000EquatorialPos(core);
		star2=asterism[2*i+1]->getJ2000EquatorialPos(core);
		star1.normalize();
		star2.normalize();
		sPainter.drawGreatCircleArc(star1, star2, &viewportHalfspace);
	}
}

void Asterism::drawName(StelPainter& sPainter) const
{
	if ((nameFader.getInterstate()==0.0f) || !flagAsterism)
		return;

	if (typeOfAsterism==2 && sPainter.getProjector()->getFov()>60.f)
		return;

	QString name = getNameI18n();
	sPainter.setColor(labelColor, nameFader.getInterstate());
	sPainter.drawText(static_cast<float>(XYname[0]), static_cast<float>(XYname[1]), name, 0., -sPainter.getFontMetrics().boundingRect(name).width()/2, 0, false);
}

const Asterism* Asterism::isStarIn(const StelObject* s) const
{
	for(unsigned int i=0;i<numberOfSegments*2;++i)
	{
		if (asterism[i]==s)
			return this;
	}
	return Q_NULLPTR;
}

void Asterism::update(int deltaTime)
{
	lineFader.update(deltaTime);
	rayHelperFader.update(deltaTime);
	nameFader.update(deltaTime);
}

QString Asterism::getInfoString(const StelCore *core, const InfoStringGroup &flags) const
{
	Q_UNUSED(core);
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
		oss << "<h2>" << getNameI18n() << "</h2>";

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_("asterism")) << "<br />";

	postProcessInfoString(str, flags);

	return str;
}

