/*
 * Copyright (C) 2016 Alexander Wolf
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

#include "CustomObject.hpp"
#include "CustomObjectMgr.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelTexture.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelProjector.hpp"
#include "StelUtils.hpp"

const QString CustomObject::CUSTOMOBJECT_TYPE = QStringLiteral("CustomObject");
Vec3f CustomObject::markerColor = Vec3f(0.1f,1.0f,0.1f);
float CustomObject::markerSize = 1.f;
float CustomObject::selectPriority = 0.f;

CustomObject::CustomObject(const QString& codesignation, const Vec3d& coordinates, const bool isVisible)
	: initialized(false)
	, XYZ(coordinates)
	, markerTexture(Q_NULLPTR)
	, designation(codesignation)
	, isMarker(isVisible)
{
	markerTexture = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/cross.png");	
	initialized = true;
}

CustomObject::~CustomObject()
{
	markerTexture.clear();
}

float CustomObject::getSelectPriority(const StelCore* core) const
{
	Q_UNUSED(core);
	return selectPriority;
}

QString CustomObject::getNameI18n() const
{
	QString r = designation;
	if (isMarker)
	{
		QStringList cod = designation.split(" ");
		if (cod.count()>1)
			r = QString("%1 %2").arg(q_(cod.at(0))).arg(cod.at(1));
		else
			r = q_(r);
	}
	return r;
}

QString CustomObject::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
		oss << "<h2>" << getNameI18n() << "</h2>";

	if (flags&ObjectType)
	{
		QString type = q_("custom object");
		if (isMarker)
			type = q_("custom marker");

		oss << QString("%1: <b>%2</b>").arg(q_("Type"), type) << "<br />";
	}

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);
	postProcessInfoString(str, flags);
	return str;
}

Vec3f CustomObject::getInfoColor(void) const
{
	return Vec3f(1.f, 1.f, 1.f);
}

float CustomObject::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core);
	if (isMarker)
		return 3.f;
	else
		return 99.f;
}

double CustomObject::getAngularSize(const StelCore*) const
{
	return 0.00001;
}

void CustomObject::update(double deltaTime)
{
	labelsFader.update(static_cast<int>(deltaTime*1000));
}

void CustomObject::draw(StelCore* core, StelPainter *painter)
{
	Q_UNUSED(core);
	Vec3d pos;
	// Check visibility of custom object
	if (!(painter->getProjector()->projectCheck(XYZ, pos)))
		return;

	painter->setBlending(true, GL_ONE, GL_ONE);
	painter->setColor(markerColor, 1.f);

	if (isMarker)
	{
		markerTexture->bind();
		const float size = static_cast<float>(getAngularSize(Q_NULLPTR))*M_PI_180f*painter->getProjector()->getPixelPerRadAtCenter();
		const float shift = markerSize + size/1.6f;

		painter->drawSprite2dMode(static_cast<float>(pos[0]), static_cast<float>(pos[1]), markerSize);

		if (labelsFader.getInterstate()<=0.f)
		{
			painter->drawText(static_cast<float>(pos[0]), static_cast<float>(pos[1]), getNameI18n(), 0, shift, shift, false);
		}
	}
}
