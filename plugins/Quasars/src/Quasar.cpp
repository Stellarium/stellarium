/*
 * Copyright (C) 2011, 2018 Alexander Wolf
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

#include "Quasar.hpp"
#include "Quasars.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyDrawer.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QVariant>
#include <QList>

const QString Quasar::QUASAR_TYPE = QStringLiteral("Quasar");
StelTextureSP Quasar::markerTexture;

bool Quasar::distributionMode = false;
bool Quasar::useMarkers = false;
Vec3f Quasar::markerColor = Vec3f(1.0f,0.5f,0.4f);

Quasar::Quasar(const QVariantMap& map)
	: initialized(false)
	, shiftVisibility(3.f) // increase magnitude for better visibility markers of quasars (sync with DSO)
	, designation("")
	, VMagnitude(-99.f)
	, AMagnitude(-99.f)
	, bV(-99.f)
	, qRA(0.)
	, qDE(0.)
	, redshift(0.f)
	, f6(-9999.f)
	, f20(-9999.f)
	, sclass("")
{
	if (!map.contains("designation") || !map.contains("RA") || !map.contains("DE"))
	{
		qWarning() << "Quasar: INVALID quasar!" << map.value("designation").toString();
		qWarning() << "Quasar: Please, check your 'quasars.json' catalog!";
		return;
	}

	designation  = map.value("designation").toString();
	if (map.contains("Vmag"))
		VMagnitude = map.value("Vmag").toFloat();
	else
		VMagnitude = -99.f;
	if (map.contains("Amag"))
		AMagnitude = map.value("Amag").toFloat();
	else
		AMagnitude = -99.f;
	if (map.contains("bV"))
		bV = map.value("bV").toFloat();
	else
		bV = -99.f;
	qRA = StelUtils::getDecAngle(map.value("RA").toString());
	qDE = StelUtils::getDecAngle(map.value("DE").toString());
	StelUtils::spheToRect(qRA, qDE, XYZ);
	redshift = map.value("z").toFloat();
	if (map.contains("f6"))
		f6 = map.value("f6").toFloat();
	else
		f6 = -9999.f;
	if (map.contains("f20"))
		f20 = map.value("f20").toFloat();
	else
		f20 = -9999.f;
	sclass = map.value("sclass").toString();

	initialized = true;
}

Quasar::~Quasar()
{
	//
}

QVariantMap Quasar::getMap(void) const
{
	QVariantMap map;
	map["designation"] = designation;
	map["Vmag"] = VMagnitude;
	map["Amag"] = AMagnitude;
	map["bV"] = bV;
	map["RA"] = qRA;
	map["DE"] = qDE;	
	map["z"] = redshift;
	map["f6"] = f6;
	map["f20"] = f20;
	map["sclass"] = sclass;

	return map;
}

QString Quasar::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
		oss << "<h2>" << designation << "</h2>";

	if (flags&ObjectType)
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_("quasar")) << "<br />";

	if (flags&Magnitude && VMagnitude>-99.f)
		oss << getMagnitudeInfoString(core, flags, 2);

	if (flags&AbsoluteMagnitude && AMagnitude>-99.f)
		oss << QString("%1: %2").arg(q_("Absolute Magnitude"), QString::number(AMagnitude, 'f', 2)) << "<br />";

	if (flags&Extra && bV>-99.f)
		oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(bV, 'f', 2)) << "<br />";
	
	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Extra)
	{
		if (!sclass.isEmpty())
			oss <<  QString("%1: %2").arg(q_("Spectral Type"), sclass) << "<br />";
		// TRANSLATORS: Jy is Jansky(10-26W/m2/Hz)
		QString sfd  = qc_("Jy", "radio flux density");
		if (redshift>0.f)
			oss << QString("%1: %2").arg(q_("Redshift")).arg(redshift) << "<br />";
		if (f6>-9999.f)
			oss << QString("%1: %2 %3").arg(q_("Radio flux density around 5GHz (6cm)"), QString::number(f6, 'f', 3), sfd) << "<br />";
		if (f20>-9999.f)
			oss << QString("%1: %2 %3").arg(q_("Radio flux density around 1.4GHz (21cm)"), QString::number(f20, 'f', 3), sfd) << "<br />";
	}

	postProcessInfoString(str, flags);
	return str;
}

QVariantMap Quasar::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	map["amag"] = AMagnitude;
	map["bV"] = bV;
	map["redshift"] = redshift;
	map["f6"] = f6;
	map["f20"] = f20;
	map["sclass"] = sclass;

	return map;
}

Vec3f Quasar::getInfoColor(void) const
{
	return Vec3f(1.f, 1.f, 1.f);
}

float Quasar::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core);
	return VMagnitude;
}

double Quasar::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

float Quasar::getSelectPriority(const StelCore* core) const
{
	float mag = getVMagnitudeWithExtinction(core);
	if (useMarkers)
		mag -= shiftVisibility;

	if (distributionMode)
		mag = 4.f;

	return mag;
}

void Quasar::update(double deltaTime)
{
	labelsFader.update(static_cast<int>(deltaTime*1000));
}

void Quasar::draw(StelCore* core, StelPainter& painter)
{
	Vec3d win;
	// Check visibility of quasar
	if (!(painter.getProjector()->projectCheck(XYZ, win)))
		return;

	StelSkyDrawer* sd = core->getSkyDrawer();
	const float mlimit = sd->getLimitMagnitude();
	float mag = getVMagnitudeWithExtinction(core);
	if (useMarkers)
		mag -= shiftVisibility;

	if (distributionMode)
		mag = 3.f;

	if (mag <= mlimit)
	{
		float size, shift=0;
		if (distributionMode || useMarkers)
		{
			painter.setBlending(true, GL_ONE, GL_ONE);
			painter.setColor(markerColor[0], markerColor[1], markerColor[2], 1);

			Quasar::markerTexture->bind();
			size = getAngularSize(Q_NULLPTR)*M_PI/180.*painter.getProjector()->getPixelPerRadAtCenter();
			shift = 5.f + size/1.6f;

			painter.drawSprite2dMode(XYZ, distributionMode ? 4.f : 5.f);
		}
		else
		{
			Vec3f color = sd->indexToColor(BvToColorIndex(bV))*0.75f; // see ZoneArray.cpp:L490
			Vec3f vf(XYZ.toVec3f());
			Vec3f altAz(vf);
			altAz.normalize();
			core->j2000ToAltAzInPlaceNoRefraction(&altAz);
			RCMag rcMag;
			sd->preDrawPointSource(&painter);
			sd->computeRCMag(mag, &rcMag);
			// allow height-dependent twinkle and suppress twinkling in higher altitudes. Keep 0.1 twinkle amount in zenith.
			sd->drawPointSource(&painter, vf, rcMag, sd->indexToColor(BvToColorIndex(bV)), true, qMin(1.0f, 1.0f-0.9f*altAz[2]));
			sd->postDrawPointSource(&painter);
			painter.setColor(color[0], color[1], color[2], 1);
			size = getAngularSize(Q_NULLPTR)*M_PI_180f*painter.getProjector()->getPixelPerRadAtCenter();
			shift = 6.f + size/1.8f;
		}

		if (labelsFader.getInterstate()<=0.f && !distributionMode && (mag+2.f)<mlimit)
			painter.drawText(XYZ, designation, 0, shift, shift, false);
	}
}

unsigned char Quasar::BvToColorIndex(float b_v)
{
	if (b_v<-98.f)
		b_v = 0.f;
	double dBV = qBound(-500., static_cast<double>(b_v)*1000., 3499.);
	return static_cast<unsigned char>(floor(0.5+127.0*((500.0+dBV)/4000.0)));
}
