/*
 * Copyright (C) 2023 Alexander Wolf
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

#include "MissingStar.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelLocaleMgr.hpp"
#include "Planet.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QVariant>
#include <QList>

const QString MissingStar::MISSINGSTAR_TYPE = QStringLiteral("MissingStar");
bool MissingStar::flagShowLabels = true;

MissingStar::MissingStar(const QVariantMap& map)
	: initialized(false)
	, designation("")
	, RA(0.)
	, DEC(0.)
	, pmRA(0.f)
	, pmDEC(0.f)
	, bMag(-99.f)
	, vMag(-99.f)
	, parallax(0.f)
	, parallaxErr(0.f)
	, spType("")
	, bvFlag(false)
{
	if (!map.contains("designation") || !map.contains("RA") || !map.contains("DEC") || !map.contains("vMag"))
	{
		qWarning() << "MissingStar: INVALID star!" << map.value("designation").toString();
		return;
	}

	designation = map.value("designation").toString();
	RA          = StelUtils::getDecAngle(map.value("RA").toString());
	DEC         = StelUtils::getDecAngle(map.value("DEC").toString());
	pmRA        = map.value("pmRA", 0.f).toFloat();
	pmDEC       = map.value("pmDEC", 0.f).toFloat();
	bMag        = map.value("bMag", -99.f).toFloat();
	vMag        = map.value("vMag", -99.f).toFloat();
	parallax    = map.value("parallax", 0.f).toFloat();
	parallaxErr = map.value("parallaxErr", 0.f).toFloat();
	spType      = map.value("SpType", "").toString();

	double b_v = 0.;
	if (bMag>-99.f && vMag>-99.f)
	{
		b_v = (bMag-vMag)*1000.0;
		if (b_v < -500.) {
			b_v = -500.;
		} else if (b_v > 3499.) {
			b_v = 3499.;
		}
		bvFlag = true;
	}
	colorIndex = (unsigned int)floor(0.5+127.0*((500.0+b_v)/4000.0));

	initialized = true;
}

MissingStar::~MissingStar()
{
	//
}

QVariantMap MissingStar::getMap(void) const
{
	const QVariantMap map = {
		{"designation", designation},
		{"RA", RA},
		{"DEC", DEC},
		{"pmRA", pmRA},
		{"pmDEC", pmDEC},
		{"bMag", bMag},
		{"vMag", vMag},
		{"parallax", parallax},
		{"parallaxErr", parallaxErr},
		{"SpType", spType}
	};

	return map;
}

QString MissingStar::getNameI18n(void) const
{
	return designation;
}

QString MissingStar::getEnglishName(void) const
{
	return designation;
}

QString MissingStar::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
		oss << "<h2>" << getNameI18n() << "</h2>";

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), getObjectTypeI18n()) << "<br />";

	if (flags&Magnitude)
		oss << getMagnitudeInfoString(core, flags, 2);

	if (flags&AbsoluteMagnitude)
	{
		if (parallax>0.f)
			oss << QString("%1: %2").arg(q_("Absolute Magnitude")).arg(getVMagnitude(core)+5.*(1.+std::log10(0.001*parallax)), 0, 'f', 2) << "<br />";
		oss << getExtraInfoStrings(AbsoluteMagnitude).join("");
	}

	if (flags&Extra && bvFlag)
		oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(bMag-vMag, 'f', 2)) << "<br />";

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Distance)
	{
		if (parallax>0.f)
		{
			//TRANSLATORS: Unit of measure for distance - Light Years
			QString ly = qc_("ly", "distance");
			double k = AU/(SPEED_OF_LIGHT*86400*365.25);
			double d = ((0.001/3600.)*(M_PI/180));
			double distance = k/(parallax*d);
			if (parallaxErr>0.f && parallax>parallaxErr) // No distance when error of parallax is bigger than parallax!
				oss << QString("%1: %2%3%4 %5").arg(q_("Distance"), QString::number(distance, 'f', 2), QChar(0x00B1), QString::number(qAbs(k/((parallaxErr + parallax)*d) - distance), 'f', 2), ly) << "<br />";
			else
				oss << QString("%1: %2 %3").arg(q_("Distance"), QString::number(distance, 'f', 2), ly) << "<br />";
		}
		oss << getExtraInfoStrings(Distance).join("");
	}

	if (flags&ProperMotion && (pmRA!=0.0 && pmDEC!=0.0))
	{
		double pa = std::atan2(pmRA, pmDEC)*M_180_PI;
		if (pa<0)
			pa += 360.;
		oss << QString("%1: %2 %3 %4 %5&deg;<br />").arg(q_("Proper motion"),
			       QString::number(std::sqrt(pmRA*pmRA + pmDEC*pmDEC), 'f', 1), qc_("mas/yr", "milliarc second per year"),
			       qc_("towards", "into the direction of"), QString::number(pa, 'f', 1));
		oss << QString("%1: %2 %3 (%4)<br />").arg(q_("Proper motions by axes"), QString::number(pmRA, 'f', 1), QString::number(pmDEC, 'f', 1), qc_("mas/yr", "milliarc second per year"));
	}

	if (flags&Extra)
	{
		if (parallax>0.f)
		{
			QString plx = q_("Parallax");
			if (parallaxErr>0.f)
				oss <<  QString("%1: %2%3%4 ").arg(plx, QString::number(parallax, 'f', 3), QChar(0x00B1), QString::number(parallaxErr, 'f', 3));
			else
				oss << QString("%1: %2 ").arg(plx, QString::number(parallax, 'f', 3));
			oss  << qc_("mas", "parallax") << "<br />";
		}

		if (!spType.isEmpty())
			oss << QString("%1: %2").arg(q_("Spectral Type"), spType) << "<br />";
	}

	oss << getSolarLunarInfoString(core, flags);
	postProcessInfoString(str, flags);
	return str;
}

float MissingStar::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core)
	return vMag;
}

Vec3f MissingStar::getInfoColor(void) const
{
	return StelSkyDrawer::indexToColor(colorIndex);
}

void MissingStar::draw(StelCore* core, StelPainter *painter)
{
	Vec3d pos = getJ2000EquatorialPos(core);
	Vec3d win;
	// Check visibility of missing star
	if(!(painter->getProjector()->projectCheck(pos, win)))
		return;

	StelSkyDrawer* sd = core->getSkyDrawer();
	const float mlimit = sd->getFlagStarMagnitudeLimit() ? sd->getCustomStarMagnitudeLimit() : sd->getLimitMagnitude();
	const float mag = getVMagnitudeWithExtinction(core);
	const bool visibleStar = (mag <= mlimit);
	const bool visibleHint = ((mag+5.f)<mlimit);
	
	if (visibleStar)
	{
		static const float shift = 8.f;
		const Vec3f color = getInfoColor();
		Vec3d altAz(pos);
		altAz.normalize();
		core->j2000ToAltAzInPlaceNoRefraction(&altAz);
		RCMag rcMag;
		sd->computeRCMag(mag, &rcMag);
		sd->preDrawPointSource(painter);
		// allow height-dependent twinkle and suppress twinkling in higher altitudes. Keep 0.1 twinkle amount in zenith.
		sd->drawPointSource(painter, pos, rcMag, color, true, qMin(1.0f, 1.0f-0.9f*static_cast<float>(altAz[2])));
		sd->postDrawPointSource(painter);
		painter->setColor(color, 1.f);
		if (flagShowLabels && visibleHint)
			painter->drawText(pos, designation, 0, shift, shift, false);
	}	
}

Vec3d MissingStar::getJ2000EquatorialPos(const StelCore* core) const
{
	Vec3d v;
	static const double d2000 = 2451545.0;
	const double movementFactor = (M_PI/180.)*(0.0001/3600.) * (core->getJDE()-d2000)/365.25;
	const double cRA = RA + movementFactor*pmRA/::cos(DEC*M_180_PI);
	const double cDE = DEC + movementFactor*pmDEC;
	StelUtils::spheToRect(cRA, cDE, v);

	if ((core) && (core->getUseAberration()) && (core->getCurrentPlanet()))
	{
		Vec3d vel=core->getCurrentPlanet()->getHeliocentricEclipticVelocity();
		vel=StelCore::matVsop87ToJ2000*vel*core->getAberrationFactor()*(AU/(86400.0*SPEED_OF_LIGHT));
		Vec3d pos=v+vel;
		pos.normalize();
		return pos;
	}
	else
		return v;
}
