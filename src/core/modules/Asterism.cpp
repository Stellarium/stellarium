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
#include "StelModuleMgr.hpp"
#include "StarMgr.hpp"
#include "NebulaMgr.hpp"

#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "ZoneArray.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include <QMessageBox>

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

bool Asterism::read(const QJsonObject& data, StarMgr *starMgr, const QSet<int> &excludeRefs)
{
	static NebulaMgr *nebulaMgr=GETSTELMODULE(NebulaMgr);
	static QSettings *conf=StelApp::getInstance().getSettings();

	const QString id = data["id"].toString();
	const QStringList idParts = id.split(" ");
	QString scName;
	if (idParts.size() == 3 && idParts[0] == "AST")
	{
		abbreviation = idParts[2].trimmed();
		scName = idParts[1].trimmed();
		if (scName != GETSTELMODULE(StelSkyCultureMgr)->getCurrentSkyCultureID())
		{
			qWarning().nospace() << "Skyculture definition error (skyculture name mismatch) in asterism " << id
					     << " of skyculture " << GETSTELMODULE(StelSkyCultureMgr)->getCurrentSkyCultureID()
					     << ". Skipping this asterism.";
			return false;
		}
	}
	else
	{
		qWarning().nospace() << "Bad asterism id: expected \"AST cultureName Abbrev\", got " << id;
		return false;
	}

	// Allow exclusion by user configuration!
	QString exclude=conf->value(QString("SCExcludeReferences/%1").arg(scName), QString()).toString();
	if (!exclude.isEmpty())
	{
#if  (QT_VERSION<QT_VERSION_CHECK(5,14,0))
		QStringList excludeRefStrings=exclude.split(',', QString::SkipEmptyParts);
#else
		QStringList excludeRefStrings=exclude.split(',', Qt::SkipEmptyParts);
#endif
		// The list has potentially both, unwanted references (which we just take out) and additional asterism abbrevations.
		QMutableListIterator<QString> it(excludeRefStrings);
		while (it.hasNext())
		{
			bool ok;
			int numRef=it.next().toInt(&ok); // ok=false for strings e.g. from asterisms
			if (ok)
			{
				Q_UNUSED(numRef);
				it.remove();
			}
		}
		//qInfo() << "Skyculture" << scName << "configured to exclude asterisms referenced from" << excludeRefs;

		QVariantList refsVariants=data["references"].toArray().toVariantList();
		if (!refsVariants.isEmpty())
		{
			QSet<int> refs;
			foreach(const QVariant &v, refsVariants) {
			    refs << v.value<int>();
			}
			//qInfo() << "Asterism" << id << "has refs" << refs;
			if (refs.subtract(excludeRefs).isEmpty())
			{
				//qInfo() << "Asterism" << id << "has lost reference support. Skipping.";
				return false;
			}
		}

		// The remaining elements are (hopefully) unique asterism abbreviations.
		if (excludeRefStrings.contains(abbreviation))
		{
			qDebug().nospace() << "Asterism " << id << " excluded by user config. Skipping.";
			return false;
		}
	}

	const QJsonValue names = data["common_name"];
	if (names.isObject())
	{
		culturalName.translated = names["english"].toString().trimmed();
		culturalName.native = names["native"].toString().trimmed();
		culturalName.pronounce = names["pronounce"].toString().trimmed();
		culturalName.IPA = names["IPA"].toString().trimmed();
		culturalName.transliteration = names["transliteration"].toString().trimmed();
	}
	const QJsonArray polylines = data["lines"].toArray();
	asterism.clear();

	flagAsterism = !data["is_ray_helper"].toBool();
	typeOfAsterism = flagAsterism ? Type::Asterism : Type::RayHelper;

	// TODO: Apparently ray helpers have no name. This could act to autodetect all this.
	if(names.isObject()) Q_ASSERT(flagAsterism);

	if (polylines.isEmpty())
	{
		qWarning().nospace() << "Empty asterism lines array found for asterism " << id << " (" << culturalName.native << ")";
		return false;
	}

	if (!polylines[0].toArray().isEmpty() && polylines[0].toArray()[0].isArray())
	{
		qWarning().nospace() << "Coordinate array detected for asterism" << id << ". This is obsolete. Reformulate with Gaia stars. Skipping.";
		//if (polylines[0].toArray()[0].toArray().size() != 2)
		//{
		//	qWarning().nospace() << "Bad asterism point entry for asterism " << id
		//			     << " (" << culturalName.native << "): expected size 2, got " << polylines[0].toArray()[0].toArray().size();
		//	return false;
		//}
		//if (typeOfAsterism == Type::RayHelper)
		//{
		//	qWarning() << "Mismatch between asterism type and line data: got a ray helper, "
		//	              "but the line points contain two entries instead of one";
		//	return false;
		//}
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
			StelObjectP newObj;
			switch (typeOfAsterism)
			{
			case Type::RayHelper:
			case Type::Asterism:
			{
				if (point.isString() && point.toString().startsWith("DSO:"))
				{
					QString DSOname=point.toString().remove(0,4);
					newObj = nebulaMgr->searchByID(DSOname);
					if (!newObj)
					{
						qWarning().nospace() << "Error in asterism " << abbreviation << ": can't find DSO " << DSOname << "... skipping asterism";
						return false;
					}
				}
				else if (!point.isDouble() && !point.isString())
				{
					qWarning().nospace() << "Error in asterism " << abbreviation << ": bad point at line #"
					                     << lineIndex << ": isn't a number";
					return false;
				}
				else
				{
					const StarId HIP = StelUtils::getLongLong(point);
					if (HIP>0)
					{
						newObj = HIP <= NR_OF_HIP ? starMgr->searchHP(HIP) : starMgr->searchGaia(HIP);

						if (!newObj)
						{
							qWarning().nospace() << "Possible error in asterism " << abbreviation << ((HIP <= NR_OF_HIP) ? ": can't find star HIP " : ": can't find star Gaia DR3 ")  << HIP << "... skipping asterism, please install more star catalogs";
							return false;
						}
					}
					else
					{
						qWarning().nospace() << "Error in asterism " << abbreviation << ": bad element: " << point.toString() << "... skipping asterism";
						return false;
					}
				}
				asterism.push_back(newObj);

				if (!asterism.back())
				{
					asterism.pop_back();
					qWarning().nospace() << "Error in asterism " << abbreviation <<
								" (Skipping asterism. Install more catalogs?)";
					return false;
				}
				break;
			}
			case Type::TelescopicAsterism:
			{
				qWarning() << "Asterisms with coord list no longer supported. Skipping " << id;
				/*
				if (!point.isArray())
				{
					qWarning().nospace() << "Error in asterism " << id << ": bad point at line #"
					                     << lineIndex << ": isn't an array of two numbers (RA and dec)";
					return false;
				}
				const QJsonArray arr = point.toArray();
				if (arr.size() != 2 || !arr[0].isDouble() || !arr[1].isDouble())
				{
					qWarning().nospace() << "Error in asterism " << id << ": bad point at line #"
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
					qWarning() << "Error in asterism" << id << "- can't find star with coordinates" << RA << "/" << DE;
					return false;
				}
				*/
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
		Vec3d XYZname1(0.);
		for(const auto& point : asterism)
			XYZname1 += point->getJ2000EquatorialPos(core);
		XYZname1.normalize();
		XYZname.append(XYZname1);

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
		// Asterism label placement: Manual position can have more than one.
		if (data.contains("label_positions"))
		{
			XYZname.clear();
			const QJsonArray &labelPosArray=data["label_positions"].toArray();
			for (const auto& labelPos : labelPosArray)
			{
				const auto& labelArray=labelPos.toArray();
				if (labelArray.size() != 2)
				{
					qWarning() << "Bad label position given for asterism" << abbreviation << "... skipping";
					continue;
				}
				const double RA = labelArray[0].toDouble() * (M_PI_180*15.);
				const double DE = labelArray[1].toDouble() * M_PI_180;
				Vec3d newPoint;
				StelUtils::spheToRect(RA, DE, newPoint);
				XYZname.append(newPoint);
			}
		}
	}
	return true;
}

QString Asterism::getScreenLabel() const
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	return getCultureLabel(scMgr->getScreenLabelStyle());
}
QString Asterism::getInfoLabel() const
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	return getCultureLabel(scMgr->getInfoLabelStyle());
}

QString Asterism::getCultureLabel(StelObject::CulturalDisplayStyle style) const
{
	static StelSkyCultureMgr *scMgr=GETSTELMODULE(StelSkyCultureMgr);
	return scMgr->createCulturalLabel(culturalName, style, getNameI18n(), abbreviationI18n);
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

// observer centered J2000 coordinates.
// These are either automatically computed from all stars forming the lines,
// or from the manually defined label point(s).
Vec3d Asterism::getJ2000EquatorialPos(const StelCore*) const
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

void Asterism::drawName(const Vec3d &xyName, StelPainter& sPainter) const
{
	if ((nameFader.getInterstate()==0.0f) || !flagAsterism)
		return;

	if (typeOfAsterism==Type::TelescopicAsterism && sPainter.getProjector()->getFov()>60.f)
		return;

	QString name = getScreenLabel();
	sPainter.setColor(labelColor, nameFader.getInterstate());
	sPainter.drawText(static_cast<float>(xyName[0]), static_cast<float>(xyName[1]), name, 0., -sPainter.getFontMetrics().boundingRect(name).width()/2, 0, false);
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

