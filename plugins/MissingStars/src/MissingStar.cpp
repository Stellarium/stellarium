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
#include "StarMgr.hpp"
#include "Planet.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QVariant>
#include <QList>

const QString MissingStar::MISSINGSTAR_TYPE = QStringLiteral("MissingStar");

MissingStar::MissingStar(const QVariantMap& map)
	: initialized(false)
	, designation("")
	, RA(0.)
	, DEC(0.)
	, pmRA(0.f)
	, pmDEC(0.f)
	, bMag(-99.f)
	, vMag(-99.f)
	, colorIndex(0)
{
	if (!map.contains("designation") || !map.contains("RA") || !map.contains("DEC") || !map.contains("vMag"))
	{
		qWarning() << "MissingStar: INVALID star!" << map.value("designation").toString();
		return;
	}

	designation = map.value("designation").toString();
	RA          = StelUtils::getDecAngle(map.value("RA").toString());
	DEC         = StelUtils::getDecAngle(map.value("DEC").toString());
	StelUtils::spheToRect(RA, DEC, XYZ);
	pmRA        = map.value("pmRA", 0.f).toFloat();
	pmDEC       = map.value("pmDEC", 0.f).toFloat();
	bMag        = map.value("bMag", -99.f).toFloat();
	vMag        = map.value("vMag", -99.f).toFloat();

	if (bMag>-99.f && vMag>-99.f)
	{
		double b_v = (bMag-vMag)*1000.0;
		if (b_v < -500.) {
			b_v = -500.;
		} else if (b_v > 3499.) {
			b_v = 3499.;
		}
		colorIndex = (unsigned int)floor(0.5+127.0*((500.0+b_v)/4000.0));
	}

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
		{"vMag", vMag}
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

	if (flags&Extra)
		oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(bMag-vMag, 'f', 2)) << "<br />";

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

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

void MissingStar::update(double deltaTime)
{
	labelsFader.update(static_cast<int>(deltaTime*1000));
}

void MissingStar::draw(StelCore* core, StelPainter& painter)
{
	StelSkyDrawer* sd = core->getSkyDrawer();
	const float mlimit = sd->getLimitMagnitude();
	const float mag = getVMagnitudeWithExtinction(core);
	const float shift = 8.f;
	
	if (mag <= mlimit)
	{
		const Vec3f color = getInfoColor();
		Vec3f vf(getJ2000EquatorialPos(core).toVec3f());
		Vec3f altAz(vf);
		altAz.normalize();
		core->j2000ToAltAzInPlaceNoRefraction(&altAz);
		RCMag rcMag;
		sd->computeRCMag(mag, &rcMag);
		sd->preDrawPointSource(&painter);
		// allow height-dependent twinkle and suppress twinkling in higher altitudes. Keep 0.1 twinkle amount in zenith.
		sd->drawPointSource(&painter, vf.toVec3d(), rcMag, color, true, qMin(1.0f, 1.0f-0.9f*altAz[2]));
		sd->postDrawPointSource(&painter);
		painter.setColor(color, 1.f);
		StarMgr* smgr = GETSTELMODULE(StarMgr); // It's need for checking displaying of labels for stars
		if (labelsFader.getInterstate()<=0.f && (mag+5.f)<mlimit && smgr->getFlagLabels())
			painter.drawText(getJ2000EquatorialPos(core), designation, 0, shift, shift, false);
	}	
}

Vec3d MissingStar::getJ2000EquatorialPos(const StelCore* core) const
{
	if ((core) && (core->getUseAberration()) && (core->getCurrentPlanet()))
	{
		Vec3d vel=core->getCurrentPlanet()->getHeliocentricEclipticVelocity();
		vel=StelCore::matVsop87ToJ2000*vel*core->getAberrationFactor()*(AU/(86400.0*SPEED_OF_LIGHT));
		Vec3d pos=XYZ+vel;
		pos.normalize();
		return pos;
	}
	else
	{
		return XYZ;
	}
}
