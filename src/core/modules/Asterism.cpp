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
#include "StelUtils.hpp"
#include "ZoneArray.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyCultureMgr.hpp"

#include <algorithm>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QFontMetrics>
#include <QIODevice>

Vec3f Asterism::lineColor = Vec3f(0.4f,0.4f,0.8f);
Vec3f Asterism::rayHelperColor = Vec3f(1.0f,1.0f,0.0f);
Vec3f Asterism::labelColor = Vec3f(0.4f,0.4f,0.8f);
const QString Asterism::ASTERISM_TYPE = QStringLiteral("Asterism");

Asterism::Asterism()
	: flagAsterism(true)
	, singleStarAsterismRadius(cos(M_PI/360.)) // default radius of 1/2 degrees
{
}

Asterism::~Asterism()
{
}

bool Asterism::read(const QJsonObject& data, StarMgr *starMgr)
{
	abbreviation = data["id"].toString();
	const auto commonName = data["common_name"];
	if (commonName.isObject())
		englishName = commonName.toObject()["english"].toString();
	const auto polylines = data["lines"].toArray();
	asterism.clear();

	flagAsterism = !data["is_ray_helper"].toBool();
	typeOfAsterism = flagAsterism ? Type::Asterism : Type::RayHelper;

	if (polylines.isEmpty())
	{
		qWarning().nospace() << "Empty asterism lines array found for asterism " << abbreviation << " (" << englishName << ")";
		return false;
	}

	if (!polylines[0].toArray().isEmpty() && polylines[0].toArray()[0].isArray())
	{
		if (polylines[0].toArray()[0].toArray().size() != 2)
		{
			qWarning().nospace() << "Bad asterism point entry for asterism " << abbreviation
			                     << " (" << englishName << "): expected size 2, got " << polylines[0].toArray()[0].toArray().size();
			return false;
		}
		if (typeOfAsterism == Type::RayHelper)
		{
			qWarning() << "Mismatch between asterism type and line data: got a ray helper, "
			              "but the line points contain two entries instead of one";
			return false;
		}
		// The entry contains RA and dec instead of HIP catalog number
		typeOfAsterism = Type::TelescopicAsterism;
	}

	StelCore *core = StelApp::getInstance().getCore();
	for (int lineIndex = 0; lineIndex < polylines.size(); ++lineIndex)
	{
		if (!polylines[lineIndex].isArray())
		{
			qWarning().nospace() << "Bad asterism line #" << lineIndex << ": is not an array";
			return false;
		}
		const auto polyline = polylines[lineIndex].toArray();
		for (int pointIndex = 0; pointIndex < polyline.size(); ++pointIndex)
		{
			const auto point = polyline[pointIndex];
			switch (typeOfAsterism)
			{
			case Type::RayHelper:
			case Type::Asterism:
			{
				if (!point.isDouble() && !point.isString())
				{
					qWarning().nospace() << "Error in asterism " << abbreviation << ": bad point at line #"
					                     << lineIndex << ": isn't a number";
					return false;
				}
				const StarId HIP = StelUtils::getLongLong(point);
				if (HIP <= NR_OF_HIP)
					asterism.push_back(starMgr->searchHP(HIP));
				else
					asterism.push_back(starMgr->searchGaia(HIP));
				if (!asterism.back())
				{
					asterism.pop_back();
					qWarning().nospace() << "Error in asterism " << abbreviation <<
								": can't find star " << (HIP <= NR_OF_HIP ? "HIP ":"DR3 ") << HIP <<
								" (Skipping asterism. Install more catalogs?)";
					return false;
				}
				break;
			}
			case Type::TelescopicAsterism:
			{
				if (!point.isArray())
				{
					qWarning().nospace() << "Error in asterism " << abbreviation << ": bad point at line #"
					                     << lineIndex << ": isn't an array of two numbers (RA and dec)";
					return false;
				}
				const auto arr = point.toArray();
				if (arr.size() != 2 || !arr[0].isDouble() || !arr[1].isDouble())
				{
					qWarning().nospace() << "Error in asterism " << abbreviation << ": bad point at line #"
					                     << lineIndex << ": isn't an array of two numbers (RA and dec)";
					return false;
				}

				const double RA = arr[0].toDouble();
				const double DE = arr[1].toDouble();

				Vec3d coords;
				StelUtils::spheToRect(RA*M_PI/12., DE*M_PI/180., coords);
				const QList<StelObjectP> stars = starMgr->searchAround(coords, 0.25, core);
				// Find star closest to coordinates
				StelObjectP s = nullptr;
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
				asterism.push_back(s);
				if (!asterism.back())
				{
					qWarning() << "Error in asterism" << abbreviation << "- can't find star with coordinates" << RA << "/" << DE;
					return false;
				}
				break;
			}
			}
			// Expand the polyline into a sequence of segments
			if (pointIndex != 0 && pointIndex != polyline.size() - 1)
				asterism.push_back(asterism.back());
		}
	}

	if (data.contains("single_star_radius"))
	{
		double rd = data["single_star_radius"].toDouble(0.5);
		singleStarAsterismRadius = cos(rd*M_PI/180.);
	}

	if (typeOfAsterism != Type::RayHelper)
	{
		XYZname.set(0.,0.,0.);
		for(const auto& point : asterism)
			XYZname += point->getJ2000EquatorialPos(core);
		XYZname.normalize();
	}

	return true;
}

QString Asterism::getScreenLabel() const
{
	return getCultureLabel(GETSTELMODULE(StelSkyCultureMgr)->getScreenLabelStyle());
}
QString Asterism::getInfoLabel() const
{
	return getCultureLabel(GETSTELMODULE(StelSkyCultureMgr)->getInfoLabelStyle());
}

QString Asterism::getCultureLabel(StelObject::CulturalDisplayStyle style) const
{
	QString label;
	switch (style)
	{
		case CulturalDisplayStyle::Abbreviated:
			label=(abbreviationI18n.startsWith('.') ? "" : abbreviationI18n);
			break;
		case CulturalDisplayStyle::Native:
			label=nativeName;
			break;
		case CulturalDisplayStyle::Translated:
			label=nameI18;
			break;
		case CulturalDisplayStyle::Modern:
			label=englishName;
			break;
		case CulturalDisplayStyle::Pronounce:
			label=getNamePronounce();
			break;
		case CulturalDisplayStyle::Translit:
			label=nativeNameTranslit;
			break;
		case CulturalDisplayStyle::IPA:
			label=nativeNameIPA;
			break;
		case CulturalDisplayStyle::Pronounce_Translated:
			label=QString("%1 (%2)").arg(getNamePronounce(), nameI18);
			break;
		case CulturalDisplayStyle::Pronounce_IPA_Translated:
			label=QString("%1 [%2] (%3)").arg(getNamePronounce(), nativeNameIPA, nameI18);
			break;
		case CulturalDisplayStyle::Pronounce_Translated_Modern:
			label=QString("%1 (%2, %3)").arg(getNamePronounce(), nameI18, englishName);
			break;
		case CulturalDisplayStyle::Pronounce_IPA_Translated_Modern:
			label=QString("%1 [%2] (%3, %4)").arg(getNamePronounce(), nativeNameIPA, nameI18, englishName);
			break;
		case CulturalDisplayStyle::Native_Pronounce:
			label=QString("%1 [%2]").arg(nativeName, getNamePronounce());
			break;
		case CulturalDisplayStyle::Native_Pronounce_Translated:
			label=QString("%1 [%2] (%3)").arg(nativeName, getNamePronounce(), nameI18);
			break;
		case CulturalDisplayStyle::Native_Pronounce_IPA_Translated:
			label=QString("%1 [%2%3] (%4)").arg(nativeName, getNamePronounce(), nativeNameIPA.length() > 0 ? QString(", %1").arg(nativeNameIPA) : "", nameI18);
			break;
		case  CulturalDisplayStyle::Native_Translated:
			label=QString("%1 (%2)").arg(nativeName, nameI18);
			break;
		case  CulturalDisplayStyle::Native_Translit_Translated:
			label=QString("%1 [%2] (%3)").arg(nativeName, nativeNameTranslit, nameI18);
			break;
		case  CulturalDisplayStyle::Native_Translit_Pronounce_Translated:
			label=QString("%1 [%2, %3] (%4)").arg(nativeName, nativeNameTranslit, getNamePronounce(), nameI18);
			break;
		case  CulturalDisplayStyle::Native_Translit_Pronounce_IPA_Translated:
			label=QString("%1 [%2, %3, %4] (%5)").arg(nativeName, nativeNameTranslit, getNamePronounce(), nativeNameIPA, nameI18);
			break;
		case  CulturalDisplayStyle::Native_Translit_IPA_Translated:
			label=QString("%1 [%2, %3] (%4)").arg(nativeName, nativeNameTranslit, nativeNameIPA, nameI18);
			break;
		case  CulturalDisplayStyle::Translit_Translated:
			label=QString("%1 (%2)").arg(nativeNameTranslit, nameI18);
			break;
		case  CulturalDisplayStyle::Translit_Pronounce_Translated:
			label=QString("%1 [%2] (%3)").arg(nativeNameTranslit, getNamePronounce(), nameI18);
			break;
		case  CulturalDisplayStyle::Translit_Pronounce_IPA_Translated:
			label=QString("%1 [%2, %3] (%4)").arg(nativeNameTranslit, getNamePronounce(), nativeNameIPA, nameI18);
			break;
		case  CulturalDisplayStyle::Translit_IPA_Translated:
			label=QString("%1 [%2] (%4)").arg(nativeNameTranslit, nativeNameIPA, nameI18);
			break;
		// NO default here, else we may forget one.
	}
	return label;
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
	for (unsigned int i = 0; i < asterism.size() / 2; ++i)
	{
		star1=asterism[2*i]->getJ2000EquatorialPos(core);
		star2=asterism[2*i+1]->getJ2000EquatorialPos(core);
		star1.normalize();
		star2.normalize();
		if (star1.fuzzyEquals(star2))
		{
			// draw single-star segment as circle
			SphericalCap saCircle(star1, singleStarAsterismRadius);
			sPainter.drawSphericalRegion(&saCircle, StelPainter::SphericalPolygonDrawModeBoundary);
		}
		else
			sPainter.drawGreatCircleArc(star1, star2, &viewportHalfspace);
	}
}

void Asterism::drawName(StelPainter& sPainter) const
{
	if ((nameFader.getInterstate()==0.0f) || !flagAsterism)
		return;

	if (typeOfAsterism==Type::TelescopicAsterism && sPainter.getProjector()->getFov()>60.f)
		return;

	QString name = getNameI18n();
	sPainter.setColor(labelColor, nameFader.getInterstate());
	sPainter.drawText(static_cast<float>(XYname[0]), static_cast<float>(XYname[1]), name, 0., -sPainter.getFontMetrics().boundingRect(name).width()/2, 0, false);
}

void Asterism::update(int deltaTime)
{
	lineFader.update(deltaTime);
	rayHelperFader.update(deltaTime);
	nameFader.update(deltaTime);
}

QString Asterism::getInfoString(const StelCore *core, const InfoStringGroup &flags) const
{
	Q_UNUSED(core)
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
		oss << "<h2>" << getInfoLabel() << "</h2>";

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), getObjectTypeI18n()) << "<br />";

	oss << getSolarLunarInfoString(core, flags);
	postProcessInfoString(str, flags);

	return str;
}

