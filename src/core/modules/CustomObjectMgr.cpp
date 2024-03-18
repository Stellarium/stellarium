/*
 * Copyright (C) 2012 Alexander Wolf
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
#include "StelTranslator.hpp"
#include "CustomObjectMgr.hpp"

#include <QSettings>
#include <QKeyEvent>

CustomObjectMgr::CustomObjectMgr()
	: countMarkers(0)
	, radiusLimit(15)
{
	setObjectName("CustomObjectMgr");
	conf = StelApp::getInstance().getSettings();
	setFontSize(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSize(int)));
}

CustomObjectMgr::~CustomObjectMgr()
{
	StelApp::getInstance().getStelObjectMgr().unSelect();
}

double CustomObjectMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	if (actionName==StelModule::ActionHandleMouseClicks)
		return -11;	
	return 0;
}

void CustomObjectMgr::handleMouseClicks(class QMouseEvent* e)
{
	StelCore *core = StelApp::getInstance().getCore();
	Vec3d mousePosition = core->getMouseJ2000Pos();
	// Make sure the modifiers are exact, not just "Shift is pressed", but "Shift is pressed while Ctrl and Alt aren't"
	const auto modifiers = e->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
	const bool modifierIsShift = modifiers == Qt::ShiftModifier;
	const bool modifierIsShiftAlt = modifiers == (Qt::ShiftModifier | Qt::AltModifier);
	// Shift + LeftClick -- Add custom marker
	if (modifierIsShift && e->button()==Qt::LeftButton && e->type()==QEvent::MouseButtonPress)
	{
		addCustomObject(QString("%1 %2").arg(N_("Marker")).arg(countMarkers + 1), mousePosition, true);

		e->setAccepted(true);
		return;
	}
	// Shift + Alt + RightClick -- Removes all custom markers
	if(modifierIsShiftAlt && e->button() == Qt::RightButton && e->type() == QEvent::MouseButtonPress)
	{
		//Delete ALL custom markers
		removeCustomObjects();
		e->setAccepted(true);
		return;
	}
	// Shift + RightClick -- Removes the closest marker within a radius specified within
	if (modifierIsShift && e->button()==Qt::RightButton && e->type()==QEvent::MouseButtonPress)
	{
		const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000, StelCore::RefractionAuto);
		Vec3d winpos;
		prj->project(mousePosition, winpos);
		double xpos = winpos[0];
		double ypos = winpos[1];

		CustomObjectP closest;
		//Smallest valid radius will be at most `radiusLimit`, so radiusLimit + 10 is plenty as the default
		float smallestRad = radiusLimit + 10;
		for (const auto& cObj : std::as_const(customObjects))
		{
			//Get the position of the custom object
			Vec3d a = cObj->getJ2000EquatorialPos(core);
			prj->project(a, winpos);
			//Distance formula to determine how close we clicked to each of the custom objects
			float dist = static_cast<float>(std::sqrt(((xpos-winpos[0])*(xpos-winpos[0])) + ((ypos-winpos[1])*(ypos-winpos[1]))));
			//If the position of the object is within our click radius
			if(dist <= radiusLimit && dist < smallestRad)
			{
				//Update the closest object and the smallest distance.
				closest = cObj;
				smallestRad = dist;
			}
		}
		//If there was a custom object within `radiusLimit` pixels...
		if(smallestRad <= radiusLimit)
		{
			//Remove it and return
			removeCustomObject(closest);
			e->setAccepted(true);
			return;
		}
	}
	e->setAccepted(false);
}

void CustomObjectMgr::init()
{
	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");

	customObjects.clear();

	setMarkersColor(Vec3f(conf->value("color/custom_marker_color", "0.1,1.0,0.1").toString()));
	setMarkersSize(conf->value("gui/custom_marker_size", 5.f).toFloat());
	// Limit the click radius to 15px in any direction
	setActiveRadiusLimit(conf->value("gui/custom_marker_radius_limit", 15).toInt());
	setSelectPriority(conf->value("gui/custom_marker_priority", -2.f).toFloat());

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

void CustomObjectMgr::deinit()
{
	customObjects.clear();	
	texPointer.clear();
}

void CustomObjectMgr::setSelectPriority(float priority)
{
	CustomObject::selectPriority = priority;
}

float CustomObjectMgr::getSelectPriority() const
{
	return CustomObject::selectPriority;
}

void CustomObjectMgr::addCustomObject(QString designation, Vec3d coordinates, bool isVisible)
{
	if (!designation.isEmpty())
	{
		CustomObjectP custObj(new CustomObject(designation, coordinates, isVisible));
		if (custObj->initialized)
			customObjects.append(custObj);

		if (isVisible)
			countMarkers++;

		emit StelApp::getInstance().getCore()->updateSearchLists();
	}
}

void CustomObjectMgr::addCustomObject(QString designation, const QString &ra, const QString &dec, bool isVisible)
{
	Vec3d j2000;
	double dRa = StelUtils::getDecAngle(ra);
	double dDec = StelUtils::getDecAngle(dec);
	StelUtils::spheToRect(dRa,dDec,j2000);

	addCustomObject(designation, j2000, isVisible);
}

void CustomObjectMgr::addCustomObjectRaDec(QString designation, const QString &ra, const QString &dec, bool isVisible)
{
	Vec3d aim;
	double dRa = StelUtils::getDecAngle(ra);
	double dDec = StelUtils::getDecAngle(dec);
	StelUtils::spheToRect(dRa, dDec, aim);

	addCustomObject(designation, StelApp::getInstance().getCore()->equinoxEquToJ2000(aim, StelCore::RefractionOff), isVisible);
}

void CustomObjectMgr::addCustomObjectAltAzi(QString designation, const QString &alt, const QString &azi, bool isVisible)
{
	Vec3d aim;
	double dAlt = StelUtils::getDecAngle(alt);
	double dAzi = M_PI - StelUtils::getDecAngle(azi);

	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		dAzi -= M_PI;

	StelUtils::spheToRect(dAzi, dAlt, aim);

	addCustomObject(designation, StelApp::getInstance().getCore()->altAzToJ2000(aim, StelCore::RefractionAuto), isVisible);
}

void CustomObjectMgr::removeCustomObjects()
{
	setSelected("");
	customObjects.clear();
	//This marker count can be set to 0 because there will be no markers left and a duplicate will be impossible
	countMarkers = 0;
	emit StelApp::getInstance().getCore()->updateSearchLists();
}

void CustomObjectMgr::removeCustomObject(CustomObjectP obj)
{
	setSelected("");
	customObjects.removeOne(obj);
	emit StelApp::getInstance().getCore()->updateSearchLists();
}

void CustomObjectMgr::removeCustomObject(QString englishName)
{
	setSelected("");
	for (const auto& cObj : std::as_const(customObjects))
	{
		//If we have a match for the thing we want to delete
		if(cObj && cObj->getEnglishName()==englishName && cObj->initialized)
			customObjects.removeOne(cObj);
	}	
}

void CustomObjectMgr::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);

	for (const auto& cObj : std::as_const(customObjects))
	{
		if (cObj && cObj->initialized)
			cObj->draw(core, &painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);
}

void CustomObjectMgr::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("CustomObject");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3f pos=obj->getJ2000EquatorialPos(core).toVec3f();

		Vec3f screenpos;
		// Compute 2D pos and return if outside screen
		if (!painter.getProjector()->project(pos, screenpos))
			return;

		painter.setColor(obj->getInfoColor());
		texPointer->bind();
		painter.setBlending(true);
		painter.drawSprite2dMode(screenpos[0], screenpos[1], 13.f, static_cast<float>(StelApp::getInstance().getTotalRunTime()*40.));
	}
}

QList<StelObjectP> CustomObjectMgr::searchAround(const Vec3d& av, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	const double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	for (const auto& cObj : customObjects)
	{
		if (cObj->initialized)
		{
			equPos = cObj->getJ2000EquatorialPos(core);
			equPos.normalize();
			if (equPos.dot(v) >= cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(cObj));
			}
		}
	}

	return result;
}

StelObjectP CustomObjectMgr::searchByName(const QString& englishName) const
{
	for (const auto& cObj : customObjects)
	{
		if (cObj->getEnglishName().toUpper() == englishName.toUpper())
			return qSharedPointerCast<StelObject>(cObj);
	}

	return Q_NULLPTR;
}

StelObjectP CustomObjectMgr::searchByNameI18n(const QString& nameI18n) const
{
	for (const auto& cObj : customObjects)
	{
		if (cObj->getNameI18n().toUpper() == nameI18n.toUpper())
			return qSharedPointerCast<StelObject>(cObj);
	}

	return Q_NULLPTR;
}

QStringList CustomObjectMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;

	if (inEnglish)
	{
		for (const auto& cObj : customObjects)
		{
			result << cObj->getEnglishName();
		}
	}
	else
	{
		for (const auto& cObj : customObjects)
		{
			result << cObj->getNameI18n();
		}
	}
	return result;
}

void CustomObjectMgr::selectedObjectChange(StelModule::StelModuleSelectAction)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("CustomObject");
	if (!newSelected.empty())
	{
		setSelected(qSharedPointerCast<CustomObject>(newSelected[0]));
	}
	else
		setSelected("");
}

// Set selected planets by englishName
void CustomObjectMgr::setSelected(const QString& englishName)
{
	setSelected(searchByEnglishName(englishName));
}

void CustomObjectMgr::setSelected(CustomObjectP obj)
{
	if (obj && obj->getType() == "CustomObject")
		selected = obj;
	else
		selected.clear();
}

CustomObjectP CustomObjectMgr::searchByEnglishName(QString customObjectEnglishName) const
{
	for (const auto& p : customObjects)
	{
		if (p->getEnglishName() == customObjectEnglishName)
			return p;
	}
	return CustomObjectP();
}

// Set/Get planets names color
void CustomObjectMgr::setMarkersColor(const Vec3f& c)
{
	CustomObject::markerColor = c;
}

Vec3f CustomObjectMgr::getMarkersColor(void) const
{
	return CustomObject::markerColor;
}

void CustomObjectMgr::setMarkersSize(const float size)
{
	CustomObject::markerSize = size;
}

float CustomObjectMgr::getMarkersSize() const
{
	return CustomObject::markerSize;
}

void CustomObjectMgr::setActiveRadiusLimit(const int radius)
{
	radiusLimit = radius;
}
