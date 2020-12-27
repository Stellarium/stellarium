/*
 * Stellarium
 * Copyright (C) 2019 Alexander Wolf
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "MarkerMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelProjector.hpp"
#include "StelModule.hpp"
#include "StelObject.hpp"
#include "StelObjectType.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"
#include "StelPainter.hpp"
#include "StelTextureTypes.hpp"
#include "StelFader.hpp"

#include <QString>
#include <QDebug>
#include <QTimer>

// Base class from which other marker types inherit
class StelMarker
{
public:
	StelMarker(const float& mSize, const Vec3f& mColor);
	virtual ~StelMarker() {}

	//! draw the marker on the sky
	//! @param core the StelCore object
	virtual bool draw(StelCore* core, StelPainter& sPainter) = 0;
	//! update fade for on/off action
	virtual void update(double deltaTime);
	//! Set the duration used for the fade in / fade out of the label.
	void setFadeDuration(float duration);
	//! Set the marker color
	void setColor(const Vec3f& color);
	//! Show or hide the marker.  It will fade in/out.
	void setFlagShow(bool b);
	//! Get value of flag used to turn on and off the marker
	bool getFlagShow(void) const;

	float markerSize;
	Vec3f markerColor;
	LinearFader markerFader;
	bool autoDelete;
	int id;
	QTimer* timer;
};

//! @class SkyMarker
//! Used to create user markers which are bound to some object on the celestial sphere.
//! The object in question can be any existing StelObject or celestial coordinates.
class SkyMarker : public StelMarker
{
public:
	//! @enum MarkerType determined the way the object to which the marker is bound
	//! is indicated.
	enum MarkerType
	{
		Cross		= 1,
		Circle		= 2,
		Ellipse		= 3,
		Square		= 4,
		DottedCircle	= 5,
		CircledCross	= 6,
		DashedSquare	= 7,
		SquaredDCircle	= 8,
		CrossedCircle	= 9,
		Target		= 10,
		Gear			= 11,
		Disk			= 12
	};

	//! Constructor of a SkyMarker which is "attached" to the equatorial coordinates for epoch J2000.0
	//! @param pos the vector to the equatorial coordinates for epoch J2000.0
	//! @param size the size of the marker
	//! @param color choose of a color for the marker
	//! @param style determines type of marker
	SkyMarker(Vec3d pos, const float& size, const Vec3f& color, SkyMarker::MarkerType style=Cross);

	virtual ~SkyMarker();

	//! Draw the marker on the sky
	//! @param core the StelCore object
	//! @param sPainter the StelPainter to use for drawing operations
	virtual bool draw(StelCore* core, StelPainter& sPainter);

	static SkyMarker::MarkerType stringToMarkerType(const QString& s);

private:
	StelTextureSP markerTexture;
	Vec3d markerPosition;
	SkyMarker::MarkerType markerType;
};

//! @class HorizonMarker
//! Used to create user markers which are bound to azimuthal coordinates.
class HorizonMarker : public StelMarker
{
public:
	//! Constructor of a HorizonMarker which is to be displayed on an alt-azimuthal position.
	//! @param az  the azimuth, degrees
	//! @param alt the altitude, degrees
	//! @param size the size of the marker
	//! @param color the color for the label
	//! @param style determines type of marker
	HorizonMarker(const float az, const float alt, const float& size, const Vec3f& color, SkyMarker::MarkerType style=SkyMarker::Cross);
	virtual ~HorizonMarker();

	//! draw the marker on the screen
	//! @param core the StelCore object
	//! @param sPainter the StelPainter to use for drawing operations
	virtual bool draw(StelCore* core, StelPainter& sPainter);
private:
	StelTextureSP markerTexture;
	SkyMarker::MarkerType markerType;
	Vec3d altaz; // the vector to the coordinates
};


/////////////////////
// StelMarker class //
/////////////////////
StelMarker::StelMarker(const float& mSize, const Vec3f& mColor)
	: markerSize(mSize),
	  markerColor(mColor),
	  autoDelete(false),
	  id(0),
	  timer(Q_NULLPTR)
{
}

void StelMarker::update(double deltaTime)
{
	markerFader.update(static_cast<int>(deltaTime*1000));
}

void StelMarker::setFadeDuration(float duration)
{
	markerFader.setDuration(static_cast<int>(1000.f*duration));
}

void StelMarker::setColor(const Vec3f& color)
{
	markerColor = color;
}

void StelMarker::setFlagShow(bool b)
{
	markerFader = b;
}

bool StelMarker::getFlagShow(void) const
{
	return markerFader;
}

////////////////////
// SkyMarker class //
////////////////////
SkyMarker::SkyMarker(Vec3d pos, const float& size, const Vec3f& color, SkyMarker::MarkerType style)
	: StelMarker(size, color)
	, markerTexture(Q_NULLPTR)
	, markerPosition(pos)
	, markerType(style)
{
	// TODO: Use unicode chars for markers or SVG instead?
	static const QMap<SkyMarker::MarkerType, QString>map={
		{ Cross,          "cross.png"},
		{ Circle,         "neb_lrg.png"},
		{ Ellipse,        "neb_gal_lrg.png"},
		{ Square,         "neb_dif_lrg.png"},
		{ DottedCircle,   "neb_ocl_lrg.png"},
		{ CircledCross,   "neb_gcl_lrg.png"},
		{ DashedSquare,   "neb_drk_lrg.png"},
		{ SquaredDCircle, "neb_ocln_lrg.png"},
		{ CrossedCircle,  "neb_pnb.png"},
		{ Target,         "pointeur2.png"},
		{ Gear,           "gear.png"},
		{ Disk,           "disk.png"}};
	const QString fileName = map.value(markerType, "cross.png");
	markerTexture = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/"+fileName);
}

SkyMarker::~SkyMarker()
{
	markerTexture.clear();
}

bool SkyMarker::draw(StelCore* core, StelPainter& sPainter)
{
	Q_UNUSED(core);
	if(markerFader.getInterstate() <= 0.f)
		return false;

	Vec3d xyPos;
	sPainter.getProjector()->project(markerPosition, xyPos);

	markerTexture->bind();

	sPainter.setBlending(true, GL_ONE, GL_ONE);
	sPainter.setColor(markerColor, markerFader.getInterstate());
	sPainter.drawSprite2dMode(static_cast<float>(xyPos[0]), static_cast<float>(xyPos[1]), markerSize);

	return true;
}

SkyMarker::MarkerType SkyMarker::stringToMarkerType(const QString &s)
{
	static const QMap<QString, SkyMarker::MarkerType>map={
		{ "circle",                SkyMarker::Circle},
		{ "ellipse",               SkyMarker::Ellipse},
		{ "square",                SkyMarker::Square},
		{ "dotted-circle",         SkyMarker::DottedCircle},
		{ "circled-cross",         SkyMarker::CircledCross},
		{ "circled-plus",          SkyMarker::CircledCross},
		{ "dashed-square",         SkyMarker::DashedSquare},
		{ "squared-dotted-circle", SkyMarker::SquaredDCircle},
		{ "squared-dcircle",       SkyMarker::SquaredDCircle},
		{ "crossed-circle",        SkyMarker::CrossedCircle},
		{ "target",                SkyMarker::Target},
		{ "gear",                  SkyMarker::Gear},
		{ "disk",                  SkyMarker::Disk}};
		return map.value(s.toLower(), SkyMarker::Cross);
}

///////////////////////
// HorizonMarker class //
///////////////////////
HorizonMarker::HorizonMarker(const float az, const float alt, const float& size, const Vec3f& color, SkyMarker::MarkerType style)
	: StelMarker(size, color)
	, markerTexture(Q_NULLPTR)
	, markerType(style)
{
	StelUtils::spheToRect((180.0-static_cast<double>(az))*M_PI/180.0, static_cast<double>(alt)*M_PI/180.0, altaz);
	// TODO: Use unicode chars for markers or SVG instead?
	static const QMap<SkyMarker::MarkerType, QString>map={
		{ SkyMarker::Cross,          "cross.png"},
		{ SkyMarker::Circle,         "neb_lrg.png"},
		{ SkyMarker::Ellipse,        "neb_gal_lrg.png"},
		{ SkyMarker::Square,         "neb_dif_lrg.png"},
		{ SkyMarker::DottedCircle,   "neb_ocl_lrg.png"},
		{ SkyMarker::CircledCross,   "neb_gcl_lrg.png"},
		{ SkyMarker::DashedSquare,   "neb_drk_lrg.png"},
		{ SkyMarker::SquaredDCircle, "neb_ocln_lrg.png"},
		{ SkyMarker::CrossedCircle,  "neb_pnb.png"},
		{ SkyMarker::Target,         "pointeur2.png"},
		{ SkyMarker::Gear,           "gear.png"},
		{ SkyMarker::Disk,           "disk.png"}};
	const QString fileName = map.value(markerType, "cross.png");
	markerTexture = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/"+fileName);
}

HorizonMarker::~HorizonMarker()
{
	markerTexture.clear();
}

bool HorizonMarker::draw(StelCore *core, StelPainter& sPainter)
{
	if (markerFader.getInterstate() <= 0.f)
		return false;

	Vec3d xyPos;
	StelProjectorP keepProj=sPainter.getProjector(); // we must reset after painting!
	StelProjectorP altazProjector=core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);

	sPainter.setProjector(altazProjector);
	sPainter.getProjector()->project(altaz, xyPos);
	markerTexture->bind();
	sPainter.setBlending(true, GL_ONE, GL_ONE);
	sPainter.setColor(markerColor, markerFader.getInterstate());
	sPainter.drawSprite2dMode(static_cast<float>(xyPos[0]), static_cast<float>(xyPos[1]), markerSize);
	sPainter.setProjector(keepProj);
	return true;
}

///////////////////////
// MarkerMgr class //
///////////////////////
MarkerMgr::MarkerMgr() : counter(0)
{
	setObjectName("MarkerMgr");
}
 
MarkerMgr::~MarkerMgr()
{
}

void MarkerMgr::init()
{
}

void MarkerMgr::draw(StelCore* core)
{
	StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
	for (auto* m : allMarkers)
	{
		m->draw(core, sPainter);
	}
}

void MarkerMgr::markerDeleteTimeout()
{
	QObject* obj = QObject::sender();
	for (auto* m : allMarkers)
	{
		if (m->timer == obj)
		{
			deleteMarker(m->id);
			return;
		}
	}
}

void MarkerMgr::markerVisibleTimeout()
{
	QObject* obj = QObject::sender();
	for (auto* m : allMarkers)
	{
		if (m->timer == obj)
		{
			disconnect(m->timer, SIGNAL(timeout()), this, SLOT(markerVisibleTimeout()));
			connect(m->timer, SIGNAL(timeout()), this, SLOT(markerDeleteTimeout()));
			m->setFlagShow(false);
			m->timer->setInterval(qRound(m->markerFader.getDuration()*1000.f));
			m->timer->start();
			return;
		}
	}
}

int MarkerMgr::appendMarker(class StelMarker *m, int autoDeleteTimeoutMs)
{
	if (autoDeleteTimeoutMs > 0)
	{
		QTimer* timer = new QTimer(this);
		m->timer = timer;
		timer->setSingleShot(true);
		timer->setInterval(autoDeleteTimeoutMs);
		connect(timer, SIGNAL(timeout()), this, SLOT(markerVisibleTimeout()));
		timer->start();
	}

	counter++;
	m->id = counter;
	allMarkers[counter] = m;
	return counter;
}

int MarkerMgr::markerObject(const QString& objectName,
			    bool visible,
			    const QString& mtype,
			    const QString& color,
			    const float size,
			    bool autoDelete,
			    int autoDeleteTimeoutMs)
{
	StelObjectP obj = GETSTELMODULE(StelObjectMgr)->searchByName(objectName);
	if (!obj)
	{
		qWarning() << "MarkerMgr::markerObject object not found: " << objectName;
		return -1;
	}
	
	StelMarker* m = new SkyMarker(obj->getJ2000EquatorialPos(StelApp::getInstance().getCore()), size, Vec3f().setFromHtmlColor(color), SkyMarker::stringToMarkerType(mtype));
	if (m==Q_NULLPTR)
		return -1;

	if (visible)
		m->setFlagShow(true);

	m->autoDelete = autoDelete;

	return appendMarker(m, autoDeleteTimeoutMs);
}

int MarkerMgr::markerEquatorial(const QString& RA,
				const QString& Dec,
				bool j2000epoch,
				bool visible,
				const QString& mtype,
				const QString& color,
				const float size,
				bool autoDelete,
				int autoDeleteTimeoutMs)
{
	double dRA	= StelUtils::getDecAngle(RA);
	double dDec	= StelUtils::getDecAngle(Dec);
	Vec3d pos;
	StelUtils::spheToRect(dRA, dDec, pos);
	if (!j2000epoch)
		pos = StelApp::getInstance().getCore()->equinoxEquToJ2000(pos);

	StelMarker* m = new SkyMarker(pos, size, Vec3f().setFromHtmlColor(color), SkyMarker::stringToMarkerType(mtype));
	if (m==Q_NULLPTR)
		return -1;

	if (visible)
		m->setFlagShow(true);

	m->autoDelete = autoDelete;

	return appendMarker(m, autoDeleteTimeoutMs);
}

int MarkerMgr::markerHorizon(const QString& az,
			     const QString& alt,
			     bool visible,
			     const QString& mtype,
			     const QString& color,
			     const float size,
			     bool autoDelete,
			     int autoDeleteTimeoutMs)
{
	float dAzi	= static_cast<float>(StelUtils::getDecAngle(az)*M_180_PI);
	float dAlt	= static_cast<float>(StelUtils::getDecAngle(alt)*M_180_PI);

	StelMarker* m = new HorizonMarker(dAzi, dAlt, size, Vec3f().setFromHtmlColor(color), SkyMarker::stringToMarkerType(mtype));
	if (m==Q_NULLPTR)
		return -1;

	if (visible)
		m->setFlagShow(true);

	m->autoDelete = autoDelete;

	return appendMarker(m, autoDeleteTimeoutMs);
}

bool MarkerMgr::getMarkerShow(int id) const
{
	if (allMarkers.contains(id)) // mistake-proofing!
		return allMarkers[id]->getFlagShow();
	else
		return false;
}
	
void MarkerMgr::setMarkerShow(int id, bool show)
{
	if (allMarkers.contains(id))  // mistake-proofing!
		allMarkers[id]->setFlagShow(show);
}
	
void MarkerMgr::deleteMarker(int id)
{
	if (id<0 || !allMarkers.contains(id))
		return;

	if (allMarkers[id]->timer != Q_NULLPTR)
		allMarkers[id]->timer->deleteLater();

	delete allMarkers[id];
	allMarkers.remove(id);
}
	
void MarkerMgr::update(double deltaTime)
{
	for (auto* m : allMarkers)
		m->update(deltaTime);
}
	
double MarkerMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+11;
        return 0;
}

int MarkerMgr::deleteAllMarkers(void)
{
	int count=0;
	for (auto* m : allMarkers)
	{
		delete m;
		count++;
	}
	allMarkers.clear();
	return count;
}
