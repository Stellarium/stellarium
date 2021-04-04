/*
 * Copyright (C) 2018 Alexander Wolf
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

#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "HighlightMgr.hpp"

#include <QSettings>

HighlightMgr::HighlightMgr()
	: hightlightColor(Vec3f(0.f,1.f,1.f))
	, markerSize(11.f)
{
	setObjectName("HighlightMgr");
	conf = StelApp::getInstance().getSettings();
}

HighlightMgr::~HighlightMgr()
{
	//
}

double HighlightMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("StarMgr")->getCallOrder(actionName)+10.;
	return 0;
}

void HighlightMgr::init()
{
	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");

	// Highlights
	setColor(Vec3f(conf->value("gui/highlight_marker_color", "0.0,1.0,1.0").toString()));
	setMarkersSize(conf->value("gui/highlight_marker_size", 11.f).toFloat());

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

void HighlightMgr::deinit()
{
	highlightList.clear();
	texPointer.clear();
}

void HighlightMgr::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);

	// draw all highlights
	drawHighlights(core, painter);
}

QList<StelObjectP> HighlightMgr::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	Q_UNUSED(av)
	Q_UNUSED(limitFov)
	return QList<StelObjectP>();
}

StelObjectP HighlightMgr::searchByName(const QString& englishName) const
{
	Q_UNUSED(englishName)
	return Q_NULLPTR;
}

StelObjectP HighlightMgr::searchByNameI18n(const QString& nameI18n) const
{
	Q_UNUSED(nameI18n)
	return Q_NULLPTR;
}

StelObjectP HighlightMgr::searchByID(const QString& id) const
{
	Q_UNUSED(id)
	return Q_NULLPTR;
}

QStringList HighlightMgr::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	Q_UNUSED(objPrefix)
	Q_UNUSED(maxNbItem)
	Q_UNUSED(useStartOfWords)	
	return QStringList();
}

QStringList HighlightMgr::listAllObjects(bool inEnglish) const
{
	Q_UNUSED(inEnglish)
	return QStringList();
}

void HighlightMgr::setColor(const Vec3f& c)
{
	hightlightColor = c;
}

Vec3f HighlightMgr::getColor(void) const
{
	return hightlightColor;
}

void HighlightMgr::fillHighlightList(QList<Vec3d> list)
{
	highlightList = list;
}

void HighlightMgr::cleanHighlightList()
{
	highlightList.clear();
}

void HighlightMgr::setMarkersSize(const float size)
{
	markerSize = size;
}

float HighlightMgr::getMarkersSize() const
{
	return markerSize;
}

void HighlightMgr::drawHighlights(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	if (!highlightList.empty())
	{
		Vec3d screenpos;
		for (const auto& hlObj : qAsConst(highlightList))
		{
			// Compute 2D pos and return if outside screen
			if (!painter.getProjector()->project(hlObj, screenpos))
				continue;

			painter.setColor(hightlightColor);
			texPointer->bind();
			painter.setBlending(true);
			painter.drawSprite2dMode(screenpos[0], screenpos[1], markerSize, StelApp::getInstance().getTotalRunTime()*40.f);
		}
	}
}

void HighlightMgr::addPoint(const QString &ra, const QString &dec)
{
	Vec3d J2000;
	double dRa = StelUtils::getDecAngle(ra);
	double dDec = StelUtils::getDecAngle(dec);
	StelUtils::spheToRect(dRa, dDec, J2000);

	highlightList.append(J2000);
}

void HighlightMgr::addPointRaDec(const QString& ra, const QString& dec)
{
	Vec3d aim;
	double dRa = StelUtils::getDecAngle(ra);
	double dDec = StelUtils::getDecAngle(dec);
	StelUtils::spheToRect(dRa, dDec, aim);

	highlightList.append(StelApp::getInstance().getCore()->equinoxEquToJ2000(aim, StelCore::RefractionOff));
}

void HighlightMgr::addPointAltAzi(const QString &alt, const QString &azi)
{
	Vec3d aim;
	double dAlt = StelUtils::getDecAngle(alt);
	double dAzi = M_PI - StelUtils::getDecAngle(azi);

	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		dAzi -= M_PI;

	StelUtils::spheToRect(dAzi, dAlt, aim);

	highlightList.append(StelApp::getInstance().getCore()->altAzToJ2000(aim, StelCore::RefractionAuto));
}
