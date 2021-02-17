/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#include "GridLinesMgr.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelProjector.hpp"
#include "StelFader.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelObserver.hpp"
#include "StelPainter.hpp"
#include "StelSkyDrawer.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelMovementMgr.hpp"
#include "precession.h"

#include <set>
#include <QSettings>
#include <QDebug>
#include <QFontMetrics>

//! @class SkyGrid
//! Class which manages a grid to display in the sky.
class SkyGrid
{
public:
	// Create and precompute positions of a SkyGrid
	SkyGrid(StelCore::FrameType frame);
	virtual ~SkyGrid();
	void draw(const StelCore* prj) const;
	void setFontSize(int newFontSize);
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() const {return color;}
	void update(double deltaTime) {fader.update(static_cast<int>(deltaTime*1000));}
	void setFadeDuration(float duration) {fader.setDuration(static_cast<int>(duration*1000.f));}
	void setDisplayed(const bool displayed){fader = displayed;}
	bool isDisplayed() const {return fader;}
	void setLineThickness(const int thickness) {lineThickness = thickness;}
	int getLineThickness() const {return lineThickness;}
private:
	Vec3f color;
	StelCore::FrameType frameType;
	QFont font;
	LinearFader fader;
	int lineThickness;
};

//! @class SkyPoint
//! Class which manages a point to display around the sky like the North Celestial Pole.
class SkyPoint
{
public:
	enum SKY_POINT_TYPE
	{
		CELESTIALPOLES_J2000,
		CELESTIALPOLES_OF_DATE,
		ZENITH_NADIR,
		ECLIPTICPOLES_J2000,
		ECLIPTICPOLES_OF_DATE,
		GALACTICPOLES,
		GALACTICCENTER,
		SUPERGALACTICPOLES,
		EQUINOXES_J2000,
		EQUINOXES_OF_DATE,
		SOLSTICES_J2000,
		SOLSTICES_OF_DATE,
		ANTISOLAR,
		APEX
	};
	// Create and precompute positions of a SkyGrid
	SkyPoint(SKY_POINT_TYPE _point_type = CELESTIALPOLES_J2000);
	virtual ~SkyPoint();
	void draw(StelCore* core) const;
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() const {return color;}
	void update(double deltaTime) {fader.update(static_cast<int>(deltaTime*1000));}
	void setFadeDuration(float duration) {fader.setDuration(static_cast<int>(duration*1000.f));}
	void setDisplayed(const bool displayed){fader = displayed;}
	bool isDisplayed() const {return fader;}
	void setFontSize(int newSize);
	//! Re-translates the label.
	void updateLabel();
private:
	QSharedPointer<Planet> earth, sun;
	SKY_POINT_TYPE point_type;
	Vec3f color;
	StelCore::FrameType frameType;
	LinearFader fader;
	QFont font;
	QString northernLabel, southernLabel;
	StelTextureSP texCross;
};


//! @class SkyLine
//! Class which manages a line to display around the sky like the ecliptic line.
class SkyLine
{
public:
	enum SKY_LINE_TYPE
	{
		EQUATOR_J2000,
		EQUATOR_OF_DATE,
		ECLIPTIC_J2000,
		ECLIPTIC_OF_DATE,
		PRECESSIONCIRCLE_N,
		PRECESSIONCIRCLE_S,
		MERIDIAN,
		HORIZON,
		GALACTICEQUATOR,
		SUPERGALACTICEQUATOR,
		LONGITUDE,
		PRIME_VERTICAL,
		CURRENT_VERTICAL,
		COLURE_1,
		COLURE_2,
		CIRCUMPOLARCIRCLE_N,
		CIRCUMPOLARCIRCLE_S,
		INVARIABLEPLANE,
		SOLAR_EQUATOR,
		EARTH_UMBRA,
		EARTH_PENUMBRA
	};
	// Create and precompute positions of a SkyGrid
	SkyLine(SKY_LINE_TYPE _line_type = EQUATOR_J2000);
	virtual ~SkyLine();
	static void init(); //! call once before creating the first line.
	static void deinit(); //! call once after deleting all lines.
	void draw(StelCore* core) const;
	void setColor(const Vec3f& c) {color = c;}
	void setPartitions(bool visible) {showPartitions = visible;}
	bool showsPartitions() const {return showPartitions;}
	const Vec3f& getColor() const {return color;}
	void update(double deltaTime) {fader.update(static_cast<int>(deltaTime*1000));}
	void setFadeDuration(float duration) {fader.setDuration(static_cast<int>(duration*1000.f));}
	void setDisplayed(const bool displayed){fader = displayed;}
	bool isDisplayed() const {return fader;}
	void setLabeled(const bool displayed){showLabel = displayed;}
	bool isLabeled() const {return showLabel;}
	void setFontSize(int newSize);
	void setLineThickness(const int thickness) {lineThickness = thickness;}
	int getLineThickness() const {return lineThickness;}
	void setPartThickness(const int thickness) {partThickness = thickness;}
	int getPartThickness() const {return partThickness;}
	//! Re-translates the label and sets the frameType. Must be called in the constructor!
	void updateLabel();
	static void setSolarSystem(SolarSystem* ss);
private:
	static QSharedPointer<Planet> earth, sun, moon;
	SKY_LINE_TYPE line_type;
	Vec3f color;
	StelCore::FrameType frameType;
	LinearFader fader;
	QFont font;
	QString label;
	int lineThickness;
	int partThickness;
	bool showPartitions;
	bool showLabel;
	static QMap<int, double> precessionPartitions;
};

// rms added color as parameter
SkyGrid::SkyGrid(StelCore::FrameType frame) : color(0.2f,0.2f,0.2f), frameType(frame), lineThickness(1)
{
	// Font size is 12
	font.setPixelSize(StelApp::getInstance().getScreenFontSize()-1);
}

SkyGrid::~SkyGrid()
{
}

void SkyGrid::setFontSize(int newFontSize)
{
	font.setPixelSize(newFontSize);
}

// Step sizes in arcsec
static const double STEP_SIZES_DMS[] = {0.05, 0.2, 1., 5., 10., 60., 300., 600., 1200., 3600., 3600.*5., 3600.*10.};
static const double STEP_SIZES_HMS[] = {0.05, 0.2, 1.5, 7.5, 15., 15.*5., 15.*10., 15.*60., 15.*60.*5., 15.*60*10., 15.*60*60};

//! Return the angular grid step in degree which best fits the given scale
static double getClosestResolutionDMS(double pixelPerRad)
{
	double minResolution = 80.;
	double minSizeArcsec = minResolution/pixelPerRad*M_180_PI*3600;
	for (unsigned int i=0;i<12;++i)
	{
		if (STEP_SIZES_DMS[i]>minSizeArcsec)
			return STEP_SIZES_DMS[i]/3600.;
	}
	return 10.;
}

//! Return the angular grid step in degree which best fits the given scale
static double getClosestResolutionHMS(double pixelPerRad)
{
	double minResolution = 80.;
	double minSizeArcsec = minResolution/pixelPerRad*M_180_PI*3600;
	for (unsigned int i=0;i<11;++i)
	{
		if (STEP_SIZES_HMS[i]>minSizeArcsec)
			return STEP_SIZES_HMS[i]/3600.;
	}
	return 15.;
}

struct ViewportEdgeIntersectCallbackData
{
	ViewportEdgeIntersectCallbackData(StelPainter* p)
		: sPainter(p)
		, raAngle(0.0)
		, frameType(StelCore::FrameUninitialized) {}
	StelPainter* sPainter;
	Vec4f textColor;
	QString text;		// Label to display at the intersection of the lines and screen side
	double raAngle;		// Used for meridians
	StelCore::FrameType frameType;
};

// Callback which draws the label of the grid
void viewportEdgeIntersectCallback(const Vec3d& screenPos, const Vec3d& direction, void* userData)
{
	ViewportEdgeIntersectCallbackData* d = static_cast<ViewportEdgeIntersectCallbackData*>(userData);
	const Vec4f tmpColor = d->sPainter->getColor();
	d->sPainter->setColor(d->textColor);
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	const bool useOldAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();
	const float ppx = static_cast<float>(d->sPainter->getProjector()->getDevicePixelsPerPixel());

	QString text;
	if (d->text.isEmpty())
	{
		// We are in the case of meridians, we need to determine which of the 2 labels (3h or 15h) to use
		Vec3d tmpV;
		d->sPainter->getProjector()->unProject(screenPos, tmpV);
		double lon, lat, textAngle, raAngle;
		StelUtils::rectToSphe(&lon, &lat, tmpV);
		switch (d->frameType)
		{
			case StelCore::FrameAltAz:
			{
				raAngle = ::fmod(M_PI-d->raAngle,2.*M_PI);
				lon = ::fmod(M_PI-lon,2.*M_PI);

				if (std::fabs(2.*M_PI-lon)<0.001) // We are at meridian 0
					lon = 0.;

				const double delta = raAngle<M_PI ? M_PI : -M_PI;				
				if (std::fabs(lon-raAngle) < 0.01 || (lon==0. && raAngle!=M_PI))					
					textAngle = raAngle;
				else
					textAngle = raAngle+delta;

				if (raAngle==2*M_PI && delta==-M_PI)
					textAngle = 0;

				if (useOldAzimuth)
					textAngle += M_PI;

				if (withDecimalDegree)
					text = StelUtils::radToDecDegStr(textAngle, 4, false, true);
				else
					text = StelUtils::radToDmsStrAdapt(textAngle);

				break;			
			}
			case StelCore::FrameObservercentricEclipticJ2000:
			case StelCore::FrameObservercentricEclipticOfDate:
			{
				raAngle = d->raAngle;
				if (raAngle<0.)
					raAngle += 2.*M_PI;

				if (lon<0.)
					lon += 2*M_PI;

				if (std::fabs(2.*M_PI-lon)<0.001) // We are at meridian 0
					lon = 0.;

				const double delta = raAngle<M_PI ? M_PI : -M_PI;
				if (std::fabs(lon-raAngle) < 1. || lon==0.)
					textAngle = raAngle;
				else
					textAngle = raAngle+delta;

				if (raAngle==2*M_PI && delta==-M_PI)
					textAngle = 0;

				if (withDecimalDegree)
					text = StelUtils::radToDecDegStr(textAngle, 4, false, true);
				else
					text = StelUtils::radToDmsStrAdapt(textAngle);

				break;			
			}			
			default:			
			{
				raAngle = M_PI-d->raAngle;
				lon = M_PI-lon;

				if (raAngle<0)
					raAngle+=2.*M_PI;

				if (lon<0)
					lon+=2.*M_PI;

				if (std::fabs(2.*M_PI-lon)<0.01) // We are at meridian 0
					lon = 0.;

				if (std::fabs(lon-raAngle) < 0.01)
					textAngle = -raAngle+M_PI;
				else
				{
					const double delta = raAngle<M_PI ? M_PI : -M_PI;
					textAngle = -raAngle-delta+M_PI;
				}


				if (withDecimalDegree)
					text = StelUtils::radToDecDegStr(textAngle, 4, false, true);
				else
				{
					if (d->frameType == StelCore::FrameGalactic || d->frameType == StelCore::FrameSupergalactic)
						text = StelUtils::radToDmsStrAdapt(textAngle);
					else
						text = StelUtils::radToHmsStrAdapt(textAngle);
				}
			}
		}
	}
	else
		text = d->text;

	Vec3f direc=direction.toVec3f();	
	direc.normalize();	
	float angleDeg = std::atan2(-direc[1], -direc[0])*M_180_PIf;
	float xshift=6.f;
	float yshift=6.f;
	if (angleDeg>90.f || angleDeg<-90.f)
	{
		angleDeg+=180.f;
		xshift=-(d->sPainter->getFontMetrics().boundingRect(text).width() + xshift*ppx);
	}

	d->sPainter->drawText(static_cast<float>(screenPos[0]), static_cast<float>(screenPos[1]), text, angleDeg, xshift*ppx, yshift*ppx);
	d->sPainter->setColor(tmpColor);
	d->sPainter->setBlending(true);
}

//! Draw the sky grid in the current frame
void SkyGrid::draw(const StelCore* core) const
{
	const StelProjectorP prj = core->getProjection(frameType, frameType!=StelCore::FrameAltAz ? StelCore::RefractionAuto : StelCore::RefractionOff);
	if (fader.getInterstate() <= 0.f)
		return;

	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

	// Look for all meridians and parallels intersecting with the disk bounding the viewport
	// Check whether the pole are in the viewport
	bool northPoleInViewport = false;
	bool southPoleInViewport = false;
	Vec3f win;
	if (prj->project(Vec3f(0,0,1), win) && prj->checkInViewport(win))
		northPoleInViewport = true;
	if (prj->project(Vec3f(0,0,-1), win) && prj->checkInViewport(win))
		southPoleInViewport = true;
	// Get the longitude and latitude resolution at the center of the viewport
	Vec3d centerV;
	prj->unProject(prj->getViewportPosX()+prj->getViewportWidth()/2., prj->getViewportPosY()+prj->getViewportHeight()/2.+1., centerV);
	double lon2, lat2;
	StelUtils::rectToSphe(&lon2, &lat2, centerV);

	const double gridStepParallelRad = M_PI_180*getClosestResolutionDMS(static_cast<double>(prj->getPixelPerRadAtCenter()));
	double gridStepMeridianRad;
	if (northPoleInViewport || southPoleInViewport)
		gridStepMeridianRad = (frameType==StelCore::FrameAltAz || frameType==StelCore::FrameGalactic || frameType==StelCore::FrameSupergalactic) ? M_PI/180.* 10. : M_PI/180.* 15.;
	else
	{
		const double closestResLon = (frameType==StelCore::FrameAltAz || frameType==StelCore::FrameGalactic || frameType==StelCore::FrameSupergalactic) ?
					getClosestResolutionDMS(static_cast<double>(prj->getPixelPerRadAtCenter())*std::cos(lat2)) :
					getClosestResolutionHMS(static_cast<double>(prj->getPixelPerRadAtCenter())*std::cos(lat2));
		gridStepMeridianRad = M_PI/180.* closestResLon;
	}

	// Get the bounding halfspace
	const SphericalCap& viewPortSphericalCap = prj->getBoundingCap();

	// Compute the first grid starting point. This point is close to the center of the screen
	// and lies at the intersection of a meridian and a parallel
	lon2 = gridStepMeridianRad*(static_cast<int>(lon2/gridStepMeridianRad+0.5));
	lat2 = gridStepParallelRad*(static_cast<int>(lat2/gridStepParallelRad+0.5));
	Vec3d firstPoint;
	StelUtils::spheToRect(lon2, lat2, firstPoint);
	firstPoint.normalize();

	// Q_ASSERT(viewPortSphericalCap.contains(firstPoint));

	// Initialize a painter and set OpenGL state
	StelPainter sPainter(prj);
	sPainter.setBlending(true);
	if (lineThickness>1)
		sPainter.setLineWidth(lineThickness); // set line thickness
	sPainter.setLineSmooth(true);

	// make text colors just a bit brighter. (But if >1, QColor::setRgb fails and makes text invisible.)
	Vec4f textColor(qMin(1.0f, 1.25f*color[0]), qMin(1.0f, 1.25f*color[1]), qMin(1.0f, 1.25f*color[2]), fader.getInterstate());
	sPainter.setColor(color, fader.getInterstate());

	sPainter.setFont(font);
	ViewportEdgeIntersectCallbackData userData(&sPainter);
	userData.textColor = textColor;
	userData.frameType = frameType;

	/////////////////////////////////////////////////
	// Draw all the meridians (great circles)
	SphericalCap meridianSphericalCap(Vec3d(1,0,0), 0);
	Mat4d rotLon = Mat4d::zrotation(gridStepMeridianRad);
	Vec3d fpt = firstPoint;
	Vec3d p1, p2;
	int maxNbIter = static_cast<int>(M_PI/gridStepMeridianRad);
	int i;
	for (i=0; i<maxNbIter; ++i)
	{
		StelUtils::rectToSphe(&lon2, &lat2, fpt);
		userData.raAngle = lon2;

		meridianSphericalCap.n = fpt^Vec3d(0,0,1);
		meridianSphericalCap.n.normalize();
		if (!SphericalCap::intersectionPoints(viewPortSphericalCap, meridianSphericalCap, p1, p2))
		{
			if (viewPortSphericalCap.d<meridianSphericalCap.d && viewPortSphericalCap.contains(meridianSphericalCap.n))
			{
				// The meridian is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
				const Mat4d& rotLon120 = Mat4d::rotation(meridianSphericalCap.n, 120.*M_PI/180.);
				Vec3d rotFpt=fpt;
				rotFpt.transfo4d(rotLon120);
				Vec3d rotFpt2=rotFpt;
				rotFpt2.transfo4d(rotLon120);
				sPainter.drawGreatCircleArc(fpt, rotFpt, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
				sPainter.drawGreatCircleArc(rotFpt, rotFpt2, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
				sPainter.drawGreatCircleArc(rotFpt2, fpt, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
				fpt.transfo4d(rotLon);
				continue;
			}
			else
				break;
		}

		Vec3d middlePoint = p1+p2;
		middlePoint.normalize();
		if (!viewPortSphericalCap.contains(middlePoint))
			middlePoint*=-1.;

		// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
		sPainter.drawGreatCircleArc(p1, middlePoint, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
		sPainter.drawGreatCircleArc(p2, middlePoint, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);

		fpt.transfo4d(rotLon);
	}

	if (i!=maxNbIter)
	{
		rotLon = Mat4d::zrotation(-gridStepMeridianRad);
		fpt = firstPoint;
		fpt.transfo4d(rotLon);
		for (int j=0; j<maxNbIter-i; ++j)
		{
			StelUtils::rectToSphe(&lon2, &lat2, fpt);
			userData.raAngle = lon2;

			meridianSphericalCap.n = fpt^Vec3d(0,0,1);
			meridianSphericalCap.n.normalize();
			if (!SphericalCap::intersectionPoints(viewPortSphericalCap, meridianSphericalCap, p1, p2))
				break;

			Vec3d middlePoint = p1+p2;
			middlePoint.normalize();
			if (!viewPortSphericalCap.contains(middlePoint))
				middlePoint*=-1;

			sPainter.drawGreatCircleArc(p1, middlePoint, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
			sPainter.drawGreatCircleArc(p2, middlePoint, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);

			fpt.transfo4d(rotLon);
		}
	}

	/////////////////////////////////////////////////
	// Draw all the parallels (small circles)
	SphericalCap parallelSphericalCap(Vec3d(0,0,1), 0);
	rotLon = Mat4d::rotation(firstPoint^Vec3d(0,0,1), gridStepParallelRad);
	fpt = firstPoint;
	maxNbIter = static_cast<int>(M_PI/gridStepParallelRad)-1;
	for (i=0; i<maxNbIter; ++i)
	{
		StelUtils::rectToSphe(&lon2, &lat2, fpt);
		if (withDecimalDegree)
			userData.text = StelUtils::radToDecDegStr(lat2);
		else
			userData.text = StelUtils::radToDmsStrAdapt(lat2);

		parallelSphericalCap.d = fpt[2];
		if (parallelSphericalCap.d>0.9999999)
			break;

		const Vec3d rotCenter(0,0,parallelSphericalCap.d);
		if (!SphericalCap::intersectionPoints(viewPortSphericalCap, parallelSphericalCap, p1, p2))
		{
			if ((viewPortSphericalCap.d<parallelSphericalCap.d && viewPortSphericalCap.contains(parallelSphericalCap.n))
				|| (viewPortSphericalCap.d<-parallelSphericalCap.d && viewPortSphericalCap.contains(-parallelSphericalCap.n)))
			{
				// The parallel is fully included in the viewport, draw it in 3 sub-arcs to avoid lengths >= 180 deg
				static const Mat4d rotLon120 = Mat4d::zrotation(120.*M_PI/180.);
				Vec3d rotFpt=fpt;
				rotFpt.transfo4d(rotLon120);
				Vec3d rotFpt2=rotFpt;
				rotFpt2.transfo4d(rotLon120);
				sPainter.drawSmallCircleArc(fpt, rotFpt, rotCenter, viewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(rotFpt, rotFpt2, rotCenter, viewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(rotFpt2, fpt, rotCenter, viewportEdgeIntersectCallback, &userData);
				fpt.transfo4d(rotLon);
				continue;
			}
			else
				break;
		}

		// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
		Vec3d middlePoint = p1-rotCenter+p2-rotCenter;
		middlePoint.normalize();
		middlePoint*=(p1-rotCenter).length();
		middlePoint+=rotCenter;
		if (!viewPortSphericalCap.contains(middlePoint))
		{
			middlePoint-=rotCenter;
			middlePoint*=-1.;
			middlePoint+=rotCenter;
		}

		sPainter.drawSmallCircleArc(p1, middlePoint, rotCenter, viewportEdgeIntersectCallback, &userData);
		sPainter.drawSmallCircleArc(p2, middlePoint, rotCenter, viewportEdgeIntersectCallback, &userData);

		fpt.transfo4d(rotLon);
	}

	if (i!=maxNbIter)
	{
		rotLon = Mat4d::rotation(firstPoint^Vec3d(0,0,1), -gridStepParallelRad);
		fpt = firstPoint;
		fpt.transfo4d(rotLon);
		for (int j=0; j<maxNbIter-i; ++j)
		{
			StelUtils::rectToSphe(&lon2, &lat2, fpt);
			if (withDecimalDegree)
				userData.text = StelUtils::radToDecDegStr(lat2);
			else
				userData.text = StelUtils::radToDmsStrAdapt(lat2);

			parallelSphericalCap.d = fpt[2];
			const Vec3d rotCenter(0,0,parallelSphericalCap.d);
			if (!SphericalCap::intersectionPoints(viewPortSphericalCap, parallelSphericalCap, p1, p2))
			{
				if ((viewPortSphericalCap.d<parallelSphericalCap.d && viewPortSphericalCap.contains(parallelSphericalCap.n))
					 || (viewPortSphericalCap.d<-parallelSphericalCap.d && viewPortSphericalCap.contains(-parallelSphericalCap.n)))
				{
					// The parallel is fully included in the viewport, draw it in 3 sub-arcs to avoid lengths >= 180 deg
					static const Mat4d rotLon120 = Mat4d::zrotation(120.*M_PI/180.);
					Vec3d rotFpt=fpt;
					rotFpt.transfo4d(rotLon120);
					Vec3d rotFpt2=rotFpt;
					rotFpt2.transfo4d(rotLon120);
					sPainter.drawSmallCircleArc(fpt, rotFpt, rotCenter, viewportEdgeIntersectCallback, &userData);
					sPainter.drawSmallCircleArc(rotFpt, rotFpt2, rotCenter, viewportEdgeIntersectCallback, &userData);
					sPainter.drawSmallCircleArc(rotFpt2, fpt, rotCenter, viewportEdgeIntersectCallback, &userData);
					fpt.transfo4d(rotLon);
					continue;
				}
				else
					break;
			}

			// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
			Vec3d middlePoint = p1-rotCenter+p2-rotCenter;
			middlePoint.normalize();
			middlePoint*=(p1-rotCenter).length();
			middlePoint+=rotCenter;
			if (!viewPortSphericalCap.contains(middlePoint))
			{
				middlePoint-=rotCenter;
				middlePoint*=-1.;
				middlePoint+=rotCenter;
			}

			sPainter.drawSmallCircleArc(p1, middlePoint, rotCenter, viewportEdgeIntersectCallback, &userData);
			sPainter.drawSmallCircleArc(p2, middlePoint, rotCenter, viewportEdgeIntersectCallback, &userData);

			fpt.transfo4d(rotLon);
		}
	}

	if (lineThickness>1)
		sPainter.setLineWidth(1); // reset tickness of line
	sPainter.setLineSmooth(false);
}

SkyLine::SkyLine(SKY_LINE_TYPE _line_type) : line_type(_line_type), color(0.f, 0.f, 1.f), lineThickness(1), partThickness(1), showPartitions(true), showLabel(true)
{
	// Font size is 14
	font.setPixelSize(StelApp::getInstance().getScreenFontSize()+1);
	updateLabel();
}

// Contains ecliptic rotations from -13000, -12900, ... , +13000
QMap<int, double> SkyLine::precessionPartitions;
QSharedPointer<Planet> SkyLine::earth, SkyLine::sun, SkyLine::moon;

//! call once before creating the first line.
void SkyLine::init()
{
	setSolarSystem(GETSTELMODULE(SolarSystem));
	// The years for the precession circles start in -13000, this is 15000 before J2000.
	for (int y=-13000; y<=17000; y+=100) // Range of DE431. Maybe extend to -50000..+50000?
	{
		double jdY0, epsilonA, chiA, omegaA, psiA;
		StelUtils::getJDFromDate(&jdY0, y, 1, 0, 0, 0, 0); // JD of Jan.0.
		getPrecessionAnglesVondrak(jdY0, &epsilonA, &chiA, &omegaA, &psiA);
		precessionPartitions.insert(y, psiA); // Store only the value of shift along the ecliptic.
	}
}

void SkyLine::setSolarSystem(SolarSystem* ss)
{
	earth = ss->getEarth();
	sun   = ss->getSun();
	moon  = ss->getMoon();
}

void SkyLine::deinit()
{
	earth = Q_NULLPTR;
	sun   = Q_NULLPTR;
	moon  = Q_NULLPTR;
}

SkyLine::~SkyLine()
{
}

void SkyLine::setFontSize(int newFontSize)
{
	font.setPixelSize(newFontSize);
}

void SkyLine::updateLabel()
{
	switch (line_type)
	{
		case MERIDIAN:
			frameType = StelCore::FrameAltAz;
			label = q_("Meridian");
			break;
		case ECLIPTIC_J2000:
			frameType = StelCore::FrameObservercentricEclipticJ2000;
			label = q_("Ecliptic of J2000.0");
			break;
		case ECLIPTIC_OF_DATE:
			frameType = StelCore::FrameObservercentricEclipticOfDate;
			label = q_("Ecliptic of Date");
			break;
		case EQUATOR_J2000:
			frameType = StelCore::FrameJ2000;
			label = q_("Equator of J2000.0");
			break;
		case EQUATOR_OF_DATE:
			frameType = StelCore::FrameEquinoxEqu;
			label = q_("Equator");
			break;
		case PRECESSIONCIRCLE_N:
		case PRECESSIONCIRCLE_S:
			frameType = StelCore::FrameObservercentricEclipticOfDate;
			label = q_("Precession Circle");
			break;
		case HORIZON:
			frameType = StelCore::FrameAltAz;
			label = q_("Horizon");
			break;
		case GALACTICEQUATOR:
			frameType = StelCore::FrameGalactic;
			label = q_("Galactic Equator");
			break;
		case SUPERGALACTICEQUATOR:
			frameType = StelCore::FrameSupergalactic;
			label = q_("Supergalactic Equator");
			break;
		case LONGITUDE:
			frameType = StelCore::FrameObservercentricEclipticOfDate;
			// TRANSLATORS: Full term is "opposition/conjunction longitude"
			label = q_("O./C. longitude");
			break;
		case PRIME_VERTICAL:
			frameType=StelCore::FrameAltAz;
			label = q_("Prime Vertical");
			break;
		case CURRENT_VERTICAL:
			frameType=StelCore::FrameAltAz;
			label = q_("Altitude");
			break;
		case COLURE_1:
			frameType=StelCore::FrameEquinoxEqu;
			label = q_("Equinoctial Colure");
			break;
		case COLURE_2:
			frameType=StelCore::FrameEquinoxEqu;
			label = q_("Solstitial Colure");
			break;
		case CIRCUMPOLARCIRCLE_N:
		case CIRCUMPOLARCIRCLE_S:
			frameType = StelCore::FrameEquinoxEqu;
			label = q_("Circumpolar Circle");
			break;
		case EARTH_UMBRA:
			frameType = StelCore::FrameHeliocentricEclipticJ2000;
			label = q_("Umbra");
			break;
		case EARTH_PENUMBRA:
			frameType = StelCore::FrameHeliocentricEclipticJ2000;
			label = q_("Penumbra");
			break;
		case INVARIABLEPLANE:
			frameType = StelCore::FrameJ2000;
			label = q_("Invariable Plane");
			break;
		case SOLAR_EQUATOR:
			frameType = StelCore::FrameJ2000;
			label = q_("Projected Solar Equator");
			break;
	}
}

void SkyLine::draw(StelCore *core) const
{
	if (fader.getInterstate() <= 0.f)
		return;

	StelProjectorP prj = core->getProjection(frameType, frameType!=StelCore::FrameAltAz ? StelCore::RefractionAuto : StelCore::RefractionOff);

	// Get the bounding halfspace
	const SphericalCap& viewPortSphericalCap = prj->getBoundingCap();

	// Initialize a painter and set openGL state
	StelPainter sPainter(prj);
	sPainter.setColor(color, fader.getInterstate());
	sPainter.setBlending(true);
	const float oldLineWidth=sPainter.getLineWidth();
	sPainter.setLineWidth(lineThickness); // set line thickness
	sPainter.setLineSmooth(true);

	ViewportEdgeIntersectCallbackData userData(&sPainter);	
	sPainter.setFont(font);
	userData.textColor = Vec4f(color, fader.getInterstate());
	userData.text = label;
	double alt=0, az=0; // Required only for CURRENT_VERTICAL line. Will contain alt/az of view.
	/////////////////////////////////////////////////
	// Draw the line

	// Precession and Circumpolar circles are Small Circles, all others are Great Circles.
	if (QList<SKY_LINE_TYPE>({PRECESSIONCIRCLE_N, PRECESSIONCIRCLE_S, CIRCUMPOLARCIRCLE_N, CIRCUMPOLARCIRCLE_S, EARTH_UMBRA, EARTH_PENUMBRA}).contains(line_type))
	{
		// partitions for precession. (mark millennia!)
		double lat=0.;
		if (line_type==PRECESSIONCIRCLE_N || line_type==PRECESSIONCIRCLE_S)
		{
			lat=(line_type==PRECESSIONCIRCLE_S ? -1.0 : 1.0) * (M_PI_2-getPrecessionAngleVondrakCurrentEpsilonA());
		}
		else if (line_type==CIRCUMPOLARCIRCLE_N || line_type==CIRCUMPOLARCIRCLE_S)
		{
			const double obsLatRad=core->getCurrentLocation().latitude * (M_PI_180);
			if (obsLatRad == 0.)
			{
				sPainter.setLineWidth(oldLineWidth); // restore painter state
				sPainter.setLineSmooth(false);
				sPainter.setBlending(false);
				return;
			}
			if (line_type==CIRCUMPOLARCIRCLE_N)
				lat=(obsLatRad>0 ? -1.0 : +1.0) * obsLatRad + (M_PI_2);
			else // southern circle
				lat=(obsLatRad>0 ? +1.0 : -1.0) * obsLatRad - (M_PI_2);
		}

		if ((line_type==EARTH_UMBRA) || (line_type==EARTH_PENUMBRA))
		{
			// resizing the shadow together with the Moon would require considerable trickery.
			// It seems better to just switch it off.
			if (GETSTELMODULE(SolarSystem)->getFlagMoonScale()) return;

			const Vec3d pos=earth->getEclipticPos();
			double lambda, beta;
			StelUtils::rectToSphe(&lambda, &beta, pos);
			const QPair<Vec3d, Vec3d> radii=GETSTELMODULE(SolarSystem)->getEarthShadowRadiiAtLunarDistance();
			const double radius=(line_type==EARTH_UMBRA ? radii.first[1] : radii.second[1]);
			const double dist=moon->getEclipticPos().length();  // geocentric Lunar distance [AU]
			const Mat4d rot=Mat4d::zrotation(lambda)*Mat4d::yrotation(-beta);
			StelVertexArray circle(StelVertexArray::LineLoop);
			for (int i=0; i<360; ++i)
			{
				Vec3d point(dist, cos(i*M_PI_180)*radius, sin(i*M_PI_180)*radius); // disk towards First Point of Aries
				rot.transfo(point);                                                // rotate towards earth position
				circle.vertex.append(pos+point);                                   // attach to earth centre
			}
			sPainter.drawStelVertexArray(circle, false); // setting true does not paint for cylindrical&friends :-(
			return;
		}
		else
		{
			SphericalCap declinationCap(Vec3d(0,0,1), std::sin(lat));
			const Vec3d rotCenter(0,0,declinationCap.d);

			Vec3d p1, p2;
			if (!SphericalCap::intersectionPoints(viewPortSphericalCap, declinationCap, p1, p2))
			{
				if ((viewPortSphericalCap.d<declinationCap.d && viewPortSphericalCap.contains(declinationCap.n))
						|| (viewPortSphericalCap.d<-declinationCap.d && viewPortSphericalCap.contains(-declinationCap.n)))
				{
					// The line is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
					Vec3d pt1;
					Vec3d pt2;
					Vec3d pt3;
					const double lon1=0.0;
					const double lon2=120.0*M_PI_180;
					const double lon3=240.0*M_PI_180;
					StelUtils::spheToRect(lon1, lat, pt1); pt1.normalize();
					StelUtils::spheToRect(lon2, lat, pt2); pt2.normalize();
					StelUtils::spheToRect(lon3, lat, pt3); pt3.normalize();

					sPainter.drawSmallCircleArc(pt1, pt2, rotCenter, viewportEdgeIntersectCallback, &userData);
					sPainter.drawSmallCircleArc(pt2, pt3, rotCenter, viewportEdgeIntersectCallback, &userData);
					sPainter.drawSmallCircleArc(pt3, pt1, rotCenter, viewportEdgeIntersectCallback, &userData);
				}
			}
			else
			{
				// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
				Vec3d middlePoint = p1-rotCenter+p2-rotCenter;
				middlePoint.normalize();
				middlePoint*=(p1-rotCenter).length();
				middlePoint+=rotCenter;
				if (!viewPortSphericalCap.contains(middlePoint))
				{
					middlePoint-=rotCenter;
					middlePoint*=-1.;
					middlePoint+=rotCenter;
				}

				sPainter.drawSmallCircleArc(p1, middlePoint, rotCenter,viewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(p2, middlePoint, rotCenter, viewportEdgeIntersectCallback, &userData);
			}

			if (showPartitions && (line_type==PRECESSIONCIRCLE_N))
			{
				const float lineThickness=sPainter.getLineWidth();
				sPainter.setLineWidth(partThickness);

				// Find current value of node rotation.
				double epsilonA, chiA, omegaA, psiA;
				getPrecessionAnglesVondrak(core->getJDE(), &epsilonA, &chiA, &omegaA, &psiA);
				// psiA is the current angle, counted from J2000. Other century years have been precomputed in precessionPartitions.
				// We cannot simply sum up the rotations, but must find the century locations one-by-one.

				Vec3d part0; // current pole point on the northern precession circle.
				StelUtils::spheToRect(0., M_PI/2.-core->getCurrentPlanet().data()->getRotObliquity(core->getJDE()), part0);
				Vec3d partAxis(0,1,0);
				Vec3d partZAxis = Vec3d(0,0,1); // rotation axis for the year partitions
				Vec3d part100=part0;  part100.transfo4d(Mat4d::rotation(partAxis, 0.10*M_PI/180)); // part1 should point to 0.05deg south of "equator"
				Vec3d part500=part0;  part500.transfo4d(Mat4d::rotation(partAxis, 0.25*M_PI/180));
				Vec3d part1000=part0; part1000.transfo4d(Mat4d::rotation(partAxis, 0.45*M_PI/180));
				Vec3d part1000l=part0; part1000l.transfo4d(Mat4d::rotation(partAxis, 0.475*M_PI/180)); // label

				Vec3d pt0, ptTgt;
				for (int y=-13000; y<13000; y+=100)
				{
					const double tickAngle=M_PI_2+psiA-precessionPartitions.value(y, 0.);
					pt0=part0; pt0.transfo4d(Mat4d::rotation(partZAxis, tickAngle));
					if (y%1000 == 0)
					{
						ptTgt=part1000; ptTgt.transfo4d(Mat4d::rotation(partZAxis, tickAngle));
						if (viewPortSphericalCap.contains(pt0) || viewPortSphericalCap.contains(ptTgt))
							sPainter.drawGreatCircleArc(pt0, ptTgt, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);
						if (showLabel)
						{
							Vec3d ptTgtL=part1000l; ptTgtL.transfo4d(Mat4d::rotation(partZAxis, tickAngle));
							QString label(QString::number(y));
							Vec3d screenPosTgt, screenPosTgtL;
							prj->project(ptTgt, screenPosTgt);
							prj->project(ptTgtL, screenPosTgtL);
							double dx=screenPosTgtL[0]-screenPosTgt[0];
							double dy=screenPosTgtL[1]-screenPosTgt[1];
							float textAngle=static_cast<float>(atan2(dy,dx));

							const float shiftx = 2.f;
							const float shifty = - static_cast<float>(sPainter.getFontMetrics().height()) / 4.f;
							sPainter.drawText(ptTgt, label, textAngle*M_180_PIf, shiftx, shifty, true);
						}
					}
					else
					{
						ptTgt=(y%500 == 0 ? part500 : part100);
						ptTgt.transfo4d(Mat4d::rotation(partZAxis, tickAngle));
						if (viewPortSphericalCap.contains(pt0) || viewPortSphericalCap.contains(ptTgt))
							sPainter.drawGreatCircleArc(pt0, ptTgt, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);
					}
				}

				sPainter.setLineWidth(lineThickness);
			}
			if (showPartitions && (line_type==PRECESSIONCIRCLE_S))
			{
				const float lineThickness=sPainter.getLineWidth();
				sPainter.setLineWidth(partThickness);

				// Find current value of node rotation.
				double epsilonA, chiA, omegaA, psiA;
				getPrecessionAnglesVondrak(core->getJDE(), &epsilonA, &chiA, &omegaA, &psiA);
				// psiA is the current angle, counted from J2000. Other century years have been precomputed in precessionPartitions.
				// We cannot simply sum up the rotations, but must find the century locations one-by-one.

				Vec3d part0; // current pole point on the northern precession circle.
				StelUtils::spheToRect(0., -M_PI/2.+core->getCurrentPlanet().data()->getRotObliquity(core->getJDE()), part0);
				Vec3d partAxis(0,1,0);
				Vec3d partZAxis = Vec3d(0,0,1); // rotation axis for the year partitions
				Vec3d part100=part0;  part100.transfo4d(Mat4d::rotation(partAxis, -0.10*M_PI/180)); // part1 should point to 0.05deg south of "equator"
				Vec3d part500=part0;  part500.transfo4d(Mat4d::rotation(partAxis, -0.25*M_PI/180));
				Vec3d part1000=part0; part1000.transfo4d(Mat4d::rotation(partAxis, -0.45*M_PI/180));
				Vec3d part1000l=part0; part1000.transfo4d(Mat4d::rotation(partAxis, -0.475*M_PI/180)); // label

				Vec3d pt0, ptTgt;
				for (int y=-13000; y<13000; y+=100)
				{
					const double tickAngle=-M_PI_2+psiA-precessionPartitions.value(y, 0.);
					pt0=part0; pt0.transfo4d(Mat4d::rotation(partZAxis, tickAngle));
					if (y%1000 == 0)
					{
						ptTgt=part1000; ptTgt.transfo4d(Mat4d::rotation(partZAxis, tickAngle));
						if (viewPortSphericalCap.contains(pt0) || viewPortSphericalCap.contains(ptTgt))
							sPainter.drawGreatCircleArc(pt0, ptTgt, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);
						if (showLabel)
						{
							Vec3d ptTgtL=part1000l; ptTgtL.transfo4d(Mat4d::rotation(partZAxis, tickAngle));
							QString label(QString::number(y));
							Vec3d screenPosTgt, screenPosTgtL;
							prj->project(ptTgt, screenPosTgt);
							prj->project(ptTgtL, screenPosTgtL);
							double dx=screenPosTgtL[0]-screenPosTgt[0];
							double dy=screenPosTgtL[1]-screenPosTgt[1];
							float textAngle=static_cast<float>(atan2(dy,dx));

							const float shiftx = -5.f - static_cast<float>(sPainter.getFontMetrics().boundingRect(label).width());
							const float shifty = - static_cast<float>(sPainter.getFontMetrics().height()) / 4.f;
							sPainter.drawText(ptTgt, label, textAngle*M_180_PIf, shiftx, shifty, true);
						}
					}
					else
					{
						ptTgt=(y%500 == 0 ? part500 : part100);
						ptTgt.transfo4d(Mat4d::rotation(partZAxis, tickAngle));
						if (viewPortSphericalCap.contains(pt0) || viewPortSphericalCap.contains(ptTgt))
							sPainter.drawGreatCircleArc(pt0, ptTgt, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);
					}
				}

				sPainter.setLineWidth(lineThickness);
			}
		}
		sPainter.setLineWidth(oldLineWidth); // restore line thickness
		sPainter.setLineSmooth(false);
		sPainter.setBlending(false);

		return;
	}

	// All the other "lines" are Great Circles
	SphericalCap sphericalCap(Vec3d(0,0,1), 0);
	Vec3d fpt(1,0,0); // First Point

	if ((line_type==MERIDIAN) || (line_type==COLURE_1))
	{
		sphericalCap.n.set(0,1,0);
	}
	else if ((line_type==PRIME_VERTICAL) || (line_type==COLURE_2))
	{
		sphericalCap.n.set(1,0,0);
		fpt.set(0,0,1);
	}
	else if (line_type==CURRENT_VERTICAL)
	{
		Vec3d coordJ2000=GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();
		Vec3d coordAltAz=core->j2000ToAltAz(coordJ2000, StelCore::RefractionAuto);
		StelUtils::rectToSphe(&az, &alt, coordAltAz);
		sphericalCap.n.set(sin(-az), cos(-az), 0.);
		fpt.set(0,0,1);
	}
	else if (line_type==LONGITUDE)
	{
		Vec3d coord;		
		const double eclJDE = earth->getRotObliquity(core->getJDE());
		double ra_equ, dec_equ, lambdaJDE, betaJDE;

		StelUtils::rectToSphe(&ra_equ,&dec_equ, sun->getEquinoxEquatorialPos(core));
		StelUtils::equToEcl(ra_equ, dec_equ, eclJDE, &lambdaJDE, &betaJDE);
		if (lambdaJDE<0) lambdaJDE+=2.0*M_PI;

		StelUtils::spheToRect(lambdaJDE + M_PI_2, 0., coord);
		sphericalCap.n=coord;
		fpt.set(0,0,1);
	}
	else if (line_type==INVARIABLEPLANE)
	{
		// RA, DEC of the Invariable Plane given in WGCCRE2015 report
		static const Mat4d mat=Mat4d::zrotation(M_PI_180*(273.85+90.))*Mat4d::xrotation(M_PI_180*(90.-66.99));
		static const Vec3d axis=mat*Vec3d(0, 0, 1);
		static const Vec3d ivFpt=mat*Vec3d(1, 0, 0);
		sphericalCap.n=axis;
		fpt=ivFpt;
	}
	else if (line_type==SOLAR_EQUATOR)
	{
		// Split out the const part of rotation: rotate along ICRS equator to ascending node
		static const Mat4d solarFrame = Mat4d::zrotation((286.13+90)*M_PI_180) * Mat4d::xrotation((90-63.87)*M_PI_180);
		// Axis rotation. N.B. By this formulation, we ignore any light time correction.
		Mat4d solarRot=solarFrame * Mat4d::zrotation((sun->getSiderealTime(core->getJD(), core->getJDE())*M_PI_180));

		sphericalCap.n=solarRot*sphericalCap.n;
		fpt=solarRot*fpt;
	}

	if (showPartitions && !(QList<SKY_LINE_TYPE>({INVARIABLEPLANE, EARTH_UMBRA, EARTH_PENUMBRA}).contains(line_type)))
	{
		const float lineThickness=sPainter.getLineWidth();
		sPainter.setLineWidth(partThickness);

		// TODO: Before drawing the lines themselves (and returning), draw the short partition lines
		// Define short lines from "equator" a bit "southwards"
		Vec3d part0 = fpt;
		Vec3d partAxis(0,1,0);
		Vec3d partZAxis = sphericalCap.n; // rotation axis for the 360 partitions
		if ((line_type==MERIDIAN) || (line_type==COLURE_1))
		{
			partAxis.set(0,0,1);
		}
		else if ((line_type==PRIME_VERTICAL) || (line_type==COLURE_2))
		{
			part0.set(0,1,0);
			partAxis.set(0,0,1);
		}
		else if ((line_type==LONGITUDE) || (line_type==CURRENT_VERTICAL) || (line_type==SOLAR_EQUATOR))
		{
			partAxis=sphericalCap.n ^ part0;
		}

		Vec3d part1=part0;  part1.transfo4d(Mat4d::rotation(partAxis, 0.10*M_PI/180)); // part1 should point to 0.05deg south of "equator"
		Vec3d part5=part0;  part5.transfo4d(Mat4d::rotation(partAxis, 0.25*M_PI/180));
		Vec3d part10=part0; part10.transfo4d(Mat4d::rotation(partAxis, 0.45*M_PI/180));
		Vec3d part30=part0; part30.transfo4d(Mat4d::rotation(partAxis, 0.75*M_PI/180));
		Vec3d part30l=part0; part30l.transfo4d(Mat4d::rotation(partAxis, 0.775*M_PI/180));
		const Mat4d& rotZ1 = Mat4d::rotation(partZAxis, 1.0*M_PI/180.);
		// Limit altitude marks to the displayed range
		int i_min= 0;
		int i_max=(line_type==CURRENT_VERTICAL ? 181 : 360);
		if ((line_type==CURRENT_VERTICAL) && (GETSTELMODULE(StelMovementMgr)->getMountMode()==StelMovementMgr::MountAltAzimuthal) &&
		    (core->getCurrentProjectionType()!=StelCore::ProjectionEqualArea) && (core->getCurrentProjectionType()!=StelCore::ProjectionStereographic) && (core->getCurrentProjectionType()!=StelCore::ProjectionFisheye))
		{
			// Avoid marks creeping sideways
			if (alt<= 2*M_PI_180) i_min =static_cast<int>(-alt*M_180_PI)+2;
			if (alt>=-2*M_PI_180) i_max-=static_cast<int>( alt*M_180_PI)+2;
		}

		for (int i=0; i<i_max; ++i)
		{
			if ((line_type==CURRENT_VERTICAL && i>=i_min) || (line_type!=CURRENT_VERTICAL)){

				if (i%30 == 0 && (viewPortSphericalCap.contains(part0) || viewPortSphericalCap.contains(part30)))
				{
					sPainter.drawGreatCircleArc(part0, part30, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);

					if (showLabel)
					{
						// we must adapt (rotate) some labels to observers on the southern hemisphere.
						const bool southernHemi = core->getCurrentLocation().latitude < 0.f;
						int value=i;
						float extraTextAngle=0.f;
						// shiftx/y is OK for equator, horizon, ecliptic.
						float shiftx = - static_cast<float>(sPainter.getFontMetrics().boundingRect(QString("%1°").arg(value)).width()) * 0.5f;
						float shifty = - static_cast<float>(sPainter.getFontMetrics().height());
						QString unit("°");
						QString label;
						switch (line_type) {
							case EQUATOR_J2000:
							case EQUATOR_OF_DATE:
								if (!StelApp::getInstance().getFlagShowDecimalDegrees())
								{
									value /= 15;
									unit="h";
								}
								extraTextAngle = southernHemi ? -90.f : 90.f;
								if (southernHemi) shifty*=-0.25f;
								break;
							case HORIZON:
								value=(360-i+(StelApp::getInstance().getFlagSouthAzimuthUsage() ? 0 : 180)) % 360;
								extraTextAngle=90.f;
								break;
							case MERIDIAN:
							case COLURE_1: // Equinoctial Colure
								shifty = - static_cast<float>(sPainter.getFontMetrics().height()) * 0.25f;
								if (i<90) // South..Nadir | ARI0..CSP
								{
									value=-i;
									extraTextAngle = (line_type==COLURE_1 && southernHemi) ? 0.f : 180.f;
									shiftx = (line_type==COLURE_1 && southernHemi) ? 3.f : - static_cast<float>(sPainter.getFontMetrics().boundingRect(QString("%1°").arg(value)).width()) - 3.f;
								}
								else if (i>270) // Zenith..South | CNP..ARI0
								{
									value=360-i;
									extraTextAngle = (line_type==COLURE_1 && southernHemi) ? 0.f : 180.f;
									shiftx = (line_type==COLURE_1 && southernHemi) ? 3.f : - static_cast<float>(sPainter.getFontMetrics().boundingRect(QString("%1°").arg(value)).width()) - 3.f;
								}
								else // Nadir..North..Zenith | CSP..Equator:12h..CNP
								{
									value=i-180;
									extraTextAngle = (line_type==COLURE_1 && southernHemi) ? 180.f : 0.f;
									shiftx = (line_type==COLURE_1 && southernHemi) ? - static_cast<float>(sPainter.getFontMetrics().boundingRect(QString("%1°").arg(value)).width()) - 3.f : 3.f;
								}
								break;
							case PRIME_VERTICAL:
							case COLURE_2: // Solstitial Colure
								shifty = - static_cast<float>(sPainter.getFontMetrics().height()) * 0.25f;
								if (i<90) // East..Zenith | Equator:6h..SummerSolstice..CNP
								{
									value=i;
									extraTextAngle = (line_type==COLURE_2 && southernHemi) ? 0.f : 180.f;
									shiftx = (line_type==COLURE_2 && southernHemi) ? 3.f : - static_cast<float>(sPainter.getFontMetrics().boundingRect(QString("%1°").arg(value)).width()) - 3.f;
								}
								else if (i<270) // Zenith..West..Nadir | CNP..WinterSolstice..CSP
								{
									value=180-i;
									extraTextAngle = (line_type==COLURE_2 && southernHemi) ? 180.f : 0.f;
									shiftx = (line_type==COLURE_2 && southernHemi) ? - static_cast<float>(sPainter.getFontMetrics().boundingRect(QString("%1°").arg(value)).width()) - 3.f : 3.f;
								}
								else // Nadir..East | CSP..Equator:6h
								{
									value=i-360;
									extraTextAngle = (line_type==COLURE_2 && southernHemi) ? 0.f : 180.f;
									shiftx = (line_type==COLURE_2 && southernHemi) ? 3.f : - static_cast<float>(sPainter.getFontMetrics().boundingRect(QString("%1°").arg(value)).width()) - 3.f;
								}
								break;
							case CURRENT_VERTICAL:
								shifty = - static_cast<float>(sPainter.getFontMetrics().height()) * 0.25f;
								value=90-i;
								shiftx = 3.0f;
								break;
							case LONGITUDE:
								value=( i<180 ? 90-i : i-270 );
								shifty = - static_cast<float>(sPainter.getFontMetrics().height()) * 0.25f;
								shiftx = (i<180) ^ southernHemi ? 3.f : -static_cast<float>(sPainter.getFontMetrics().boundingRect(QString("%1°").arg(value)).width()) - 3.f;
								extraTextAngle = (i<180) ^ southernHemi ? 0.f : 180.f;
								break;
							case GALACTICEQUATOR:
							case SUPERGALACTICEQUATOR:
								extraTextAngle = 90.f;
								break;
							default:
								extraTextAngle = southernHemi ? -90.f : 90.f;
								if (southernHemi) shifty*=-0.25f;
								break;
						}
						label = QString("%1%2").arg(value).arg(unit);
						Vec3d screenPosTgt, screenPosTgtL;
						prj->project(part30, screenPosTgt);
						prj->project(part30l, screenPosTgtL);
						double dx=screenPosTgtL[0]-screenPosTgt[0];
						double dy=screenPosTgtL[1]-screenPosTgt[1];
						float textAngle=static_cast<float>(atan2(dy,dx));
						sPainter.drawText(part30l, label, textAngle*M_180_PIf + extraTextAngle, shiftx, shifty, false);
					}
				}

				else if (i%10 == 0 && (viewPortSphericalCap.contains(part0) || viewPortSphericalCap.contains(part10)))
					sPainter.drawGreatCircleArc(part0, part10, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);
				else if (i%5 == 0 && (viewPortSphericalCap.contains(part0) || viewPortSphericalCap.contains(part5)))
					sPainter.drawGreatCircleArc(part0, part5, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);
				else if( viewPortSphericalCap.contains(part0) || viewPortSphericalCap.contains(part1))
					sPainter.drawGreatCircleArc(part0, part1, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);
			}
			part0.transfo4d(rotZ1);
			part1.transfo4d(rotZ1);
			part5.transfo4d(rotZ1);
			part10.transfo4d(rotZ1);
			part30.transfo4d(rotZ1);
			part30l.transfo4d(rotZ1);
		}
		sPainter.setLineWidth(lineThickness);
	}

	Vec3d p1, p2;
	if (line_type==CURRENT_VERTICAL)
	{
		// The usual handling should always projects this circle into a straight line. However, with some projections we see ugly artifacts. Better handle this line specially.
		p1.set(0.,0.,1.);
		p2.set(0.,0.,-1.);
		Vec3d pHori;
		StelUtils::spheToRect(az, 0., pHori);
		if (GETSTELMODULE(StelMovementMgr)->getMountMode()==StelMovementMgr::MountAltAzimuthal)
		{
			switch (core->getCurrentProjectionType())
			{
				case StelCore::ProjectionOrthographic:
					StelUtils::spheToRect(az, qMin(M_PI_2, alt+M_PI_2), p1);
					StelUtils::spheToRect(az, qMax(-M_PI_2, alt-M_PI_2), p2);
					break;
				case StelCore::ProjectionEqualArea:
					if (alt*M_180_PI<-89.0) StelUtils::spheToRect(az,  89.5*M_PI_180, p1);
					if (alt*M_180_PI> 89.0) StelUtils::spheToRect(az, -89.5*M_PI_180, p2);
					break;
				case StelCore::ProjectionHammer:
				case StelCore::ProjectionSinusoidal:
				case StelCore::ProjectionMercator:
				case StelCore::ProjectionMiller:
				case StelCore::ProjectionCylinder:
					StelUtils::spheToRect(az, qMin(M_PI_2, alt+M_PI_2)-0.05*M_PI_180, p1);
					StelUtils::spheToRect(az, qMax(-M_PI_2, alt-M_PI_2)+0.05*M_PI_180, p2);
					break;
				default:
					break;
			}
		}
		// Now draw through a middle point.
		sPainter.drawGreatCircleArc(p1, pHori, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
		sPainter.drawGreatCircleArc(p2, pHori, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
	}
	else
	{
		if (!SphericalCap::intersectionPoints(viewPortSphericalCap, sphericalCap, p1, p2))
		{
			if ((viewPortSphericalCap.d< sphericalCap.d && viewPortSphericalCap.contains( sphericalCap.n))
			 || (viewPortSphericalCap.d<-sphericalCap.d && viewPortSphericalCap.contains(-sphericalCap.n)))
			{
				// The meridian is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
				const Mat4d& rotLon120 = Mat4d::rotation(sphericalCap.n, 120.*M_PI_180);
				Vec3d rotFpt=fpt;
				rotFpt.transfo4d(rotLon120);
				Vec3d rotFpt2=rotFpt;
				rotFpt2.transfo4d(rotLon120);
				sPainter.drawGreatCircleArc(fpt, rotFpt, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
				sPainter.drawGreatCircleArc(rotFpt, rotFpt2, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
				sPainter.drawGreatCircleArc(rotFpt2, fpt, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
			}
		}
		else
		{
			Vec3d middlePoint = p1+p2;
			middlePoint.normalize();
			if (!viewPortSphericalCap.contains(middlePoint))
				middlePoint*=-1.;

			// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
			sPainter.drawGreatCircleArc(p1, middlePoint, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
			sPainter.drawGreatCircleArc(p2, middlePoint, Q_NULLPTR, viewportEdgeIntersectCallback, &userData);
		}
	}

	sPainter.setLineWidth(oldLineWidth); // restore line thickness
	sPainter.setLineSmooth(false);
	sPainter.setBlending(false);
}

SkyPoint::SkyPoint(SKY_POINT_TYPE _point_type) : point_type(_point_type), color(0.f, 0.f, 1.f)
{
	// Font size is 14
	font.setPixelSize(StelApp::getInstance().getScreenFontSize()+1);
	texCross = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/cross.png");

	earth = GETSTELMODULE(SolarSystem)->getEarth();
	sun = GETSTELMODULE(SolarSystem)->getSun();

	updateLabel();
}

SkyPoint::~SkyPoint()
{
	texCross.clear();
}

void SkyPoint::setFontSize(int newFontSize)
{
	font.setPixelSize(newFontSize);
}

void SkyPoint::updateLabel()
{
	switch (point_type)
	{
		case CELESTIALPOLES_J2000:
		{
			frameType = StelCore::FrameJ2000;
			// TRANSLATORS: North Celestial Pole
			northernLabel = q_("NCP");
			// TRANSLATORS: South Celestial Pole
			southernLabel = q_("SCP");
			break;
		}
		case CELESTIALPOLES_OF_DATE:
		{
			frameType = StelCore::FrameEquinoxEqu;
			// TRANSLATORS: North Celestial Pole
			northernLabel = q_("NCP");
			// TRANSLATORS: South Celestial Pole
			southernLabel = q_("SCP");
			break;
		}
		case ZENITH_NADIR:
		{
			frameType = StelCore::FrameAltAz;
			// TRANSLATORS: Zenith
			northernLabel = qc_("Z", "zenith");
			// TRANSLATORS: Nadir
			southernLabel = qc_("Z'", "nadir");
			break;
		}
		case ECLIPTICPOLES_J2000:
		{
			frameType = StelCore::FrameObservercentricEclipticJ2000;
			// TRANSLATORS: North Ecliptic Pole
			northernLabel = q_("NEP");
			// TRANSLATORS: South Ecliptic Pole
			southernLabel = q_("SEP");
			break;
		}
		case ECLIPTICPOLES_OF_DATE:
		{
			frameType = StelCore::FrameObservercentricEclipticOfDate;
			// TRANSLATORS: North Ecliptic Pole
			northernLabel = q_("NEP");
			// TRANSLATORS: South Ecliptic Pole
			southernLabel = q_("SEP");
			break;
		}
		case GALACTICPOLES:
		{
			frameType = StelCore::FrameGalactic;
			// TRANSLATORS: North Galactic Pole
			northernLabel = q_("NGP");
			// TRANSLATORS: South Galactic Pole
			southernLabel = q_("SGP");
			break;
		}
		case GALACTICCENTER:
		{
			frameType = StelCore::FrameGalactic;
			// TRANSLATORS: Galactic Center point
			northernLabel = q_("GC");
			// TRANSLATORS: Galactic Anticenter point
			southernLabel = q_("GA");
			break;
		}
		case SUPERGALACTICPOLES:
		{
			frameType = StelCore::FrameSupergalactic;
			// TRANSLATORS: North Supergalactic Pole
			northernLabel = q_("NSGP");
			// TRANSLATORS: South Supergalactic Pole
			southernLabel = q_("SSGP");
			break;
		}
		case EQUINOXES_J2000:
		{
			frameType = StelCore::FrameJ2000;
			northernLabel = QChar(0x2648); // Vernal equinox
			southernLabel = QChar(0x264E); // Autumnal equinox
			break;
		}
		case EQUINOXES_OF_DATE:
		{
			frameType = StelCore::FrameEquinoxEqu;
			northernLabel = QChar(0x2648); // Vernal equinox
			southernLabel = QChar(0x264E); // Autumnal equinox
			break;
		}
		case SOLSTICES_J2000:
		{
			frameType = StelCore::FrameObservercentricEclipticJ2000;
			northernLabel = QChar(0x264B); // Summer solstice
			southernLabel = QChar(0x2651); // Winter solstice
			break;
		}
		case SOLSTICES_OF_DATE:
		{
			frameType = StelCore::FrameObservercentricEclipticOfDate;
			northernLabel = QChar(0x264B); // Summer solstice
			southernLabel = QChar(0x2651); // Winter solstice
			break;
		}
		case ANTISOLAR:
		{
			frameType = StelCore::FrameObservercentricEclipticJ2000;
			// TRANSLATORS: Antisolar Point
			northernLabel = q_("ASP");
			break;
		}
		case APEX:
		{
			frameType = StelCore::FrameObservercentricEclipticJ2000;
			// TRANSLATORS: Apex Point, where the observer planet is heading to
			northernLabel = q_("Apex");
			// TRANSLATORS: Antapex Point, where the observer planet is receding from
			southernLabel = q_("Antapex");
			// add heliocentric speed
			StelCore *core=StelApp::getInstance().getCore();
			QSharedPointer<Planet> planet=core->getCurrentObserver()->getHomePlanet();
			Q_ASSERT(planet);
			const Vec3d dir=planet->getHeliocentricEclipticVelocity();
			const double speed=dir.length()*(AU/86400.0);
			// In some cases we don't have a valid speed vector
			if (speed>0.)
			{
				const QString kms = qc_("km/s", "speed");
				QString speedStr = QString(" (%1 %2)").arg(QString::number(speed, 'f', 2)).arg(kms);
				northernLabel += speedStr;
				speedStr = QString(" (-%1 %2)").arg(QString::number(speed, 'f', 2)).arg(kms);
				southernLabel += speedStr;
			}
			break;
		}		
		default:
			Q_ASSERT(0);
	}
}

void SkyPoint::draw(StelCore *core) const
{
	if (!fader.getInterstate())
		return;

	StelProjectorP prj = core->getProjection(frameType, frameType!=StelCore::FrameAltAz ? StelCore::RefractionAuto : StelCore::RefractionOff);

	// Initialize a painter and set openGL state
	StelPainter sPainter(prj);
	sPainter.setColor(color, fader.getInterstate());
	Vec4f textColor(color, fader.getInterstate());

	sPainter.setFont(font);
	/////////////////////////////////////////////////
	// Draw the point

	texCross->bind();
	const float size = 0.00001f*M_PI_180f*sPainter.getProjector()->getPixelPerRadAtCenter();
	const float shift = 4.f + size/1.8f;

	sPainter.setBlending(true, GL_ONE, GL_ONE);

	switch (point_type)
	{
		case CELESTIALPOLES_J2000:
		case CELESTIALPOLES_OF_DATE:
		case ZENITH_NADIR:
		case ECLIPTICPOLES_J2000:
		case ECLIPTICPOLES_OF_DATE:
		case GALACTICPOLES:		
		case SUPERGALACTICPOLES:
		{
			// North Pole
			sPainter.drawSprite2dMode(Vec3d(0,0,1), 5.f);
			sPainter.drawText(Vec3d(0,0,1), northernLabel, 0, shift, shift, false);

			// South Pole
			sPainter.drawSprite2dMode(Vec3d(0,0,-1), 5.f);
			sPainter.drawText(Vec3d(0,0,-1), southernLabel, 0, shift, shift, false);
			break;
		}
		case EQUINOXES_J2000:
		case EQUINOXES_OF_DATE:
		{
			// Vernal equinox
			sPainter.drawSprite2dMode(Vec3d(1,0,0), 5.f);
			sPainter.drawText(Vec3d(1,0,0), northernLabel, 0, shift, shift, false);

			// Autumnal equinox
			sPainter.drawSprite2dMode(Vec3d(-1,0,0), 5.f);
			sPainter.drawText(Vec3d(-1,0,0), southernLabel, 0, shift, shift, false);
			break;
		}
		case SOLSTICES_J2000:
		case SOLSTICES_OF_DATE:
		{
			// Summer solstice
			sPainter.drawSprite2dMode(Vec3d(0,1,0), 5.f);
			sPainter.drawText(Vec3d(0,1,0), northernLabel, 0, shift, shift, false);

			// Winter solstice
			sPainter.drawSprite2dMode(Vec3d(0,-1,0), 5.f);
			sPainter.drawText(Vec3d(0,-1,0), southernLabel, 0, shift, shift, false);
			break;
		}
		case GALACTICCENTER:
		{
			// Galactic Center point
			sPainter.drawSprite2dMode(Vec3d(1,0,0), 5.f);
			sPainter.drawText(Vec3d(1,0,0), northernLabel, 0, shift, shift, false);

			// Galactic Anticenter point
			sPainter.drawSprite2dMode(Vec3d(-1,0,0), 5.f);
			sPainter.drawText(Vec3d(-1,0,0), southernLabel, 0, shift, shift, false);
			break;
		}
		case ANTISOLAR:
		{
			// Antisolar Point
			Vec3d coord=core->getCurrentObserver()->getHomePlanet()->getHeliocentricEclipticPos();
			sPainter.drawSprite2dMode(coord, 5.f);
			sPainter.drawText(coord, northernLabel, 0, shift, shift, false);
			break;
		}
		case APEX:
		{
			// Observer planet apex (heading point)
			QSharedPointer<Planet> planet=core->getCurrentObserver()->getHomePlanet();
			Q_ASSERT(planet);
			const Vec3d dir=planet->getHeliocentricEclipticVelocity();
			// In some cases we don't have a valid speed vector
			if (dir.lengthSquared()>0.)
			{
				sPainter.drawSprite2dMode(dir, 5.f);
				sPainter.drawText(dir, northernLabel, 0, shift, shift, false);
				sPainter.drawSprite2dMode(-dir, 5.f);
				sPainter.drawText(-dir, southernLabel, 0, shift, shift, false);
			}
			break;
		}		
		default:
			Q_ASSERT(0);
	}
}


GridLinesMgr::GridLinesMgr()
	: gridlinesDisplayed(true)
{
	setObjectName("GridLinesMgr");
	SkyLine::init();

	equGrid = new SkyGrid(StelCore::FrameEquinoxEqu);
	equJ2000Grid = new SkyGrid(StelCore::FrameJ2000);
	eclJ2000Grid = new SkyGrid(StelCore::FrameObservercentricEclipticJ2000);
	eclGrid = new SkyGrid(StelCore::FrameObservercentricEclipticOfDate);
	galacticGrid = new SkyGrid(StelCore::FrameGalactic);
	supergalacticGrid = new SkyGrid(StelCore::FrameSupergalactic);
	aziGrid = new SkyGrid(StelCore::FrameAltAz);
	equatorLine = new SkyLine(SkyLine::EQUATOR_OF_DATE);
	equatorJ2000Line = new SkyLine(SkyLine::EQUATOR_J2000);
	eclipticJ2000Line = new SkyLine(SkyLine::ECLIPTIC_J2000);
	eclipticLine = new SkyLine(SkyLine::ECLIPTIC_OF_DATE);
	invariablePlaneLine = new SkyLine(SkyLine::INVARIABLEPLANE);
	solarEquatorLine = new SkyLine(SkyLine::SOLAR_EQUATOR);
	precessionCircleN = new SkyLine(SkyLine::PRECESSIONCIRCLE_N);
	precessionCircleS = new SkyLine(SkyLine::PRECESSIONCIRCLE_S);
	meridianLine = new SkyLine(SkyLine::MERIDIAN);
	horizonLine = new SkyLine(SkyLine::HORIZON);
	galacticEquatorLine = new SkyLine(SkyLine::GALACTICEQUATOR);
	supergalacticEquatorLine = new SkyLine(SkyLine::SUPERGALACTICEQUATOR);
	longitudeLine = new SkyLine(SkyLine::LONGITUDE);
	primeVerticalLine = new SkyLine(SkyLine::PRIME_VERTICAL);
	currentVerticalLine = new SkyLine(SkyLine::CURRENT_VERTICAL);
	colureLine_1 = new SkyLine(SkyLine::COLURE_1);
	colureLine_2 = new SkyLine(SkyLine::COLURE_2);
	circumpolarCircleN = new SkyLine(SkyLine::CIRCUMPOLARCIRCLE_N);
	circumpolarCircleS = new SkyLine(SkyLine::CIRCUMPOLARCIRCLE_S);
	umbraCircle = new SkyLine(SkyLine::EARTH_UMBRA);
	penumbraCircle = new SkyLine(SkyLine::EARTH_PENUMBRA);
	celestialJ2000Poles = new SkyPoint(SkyPoint::CELESTIALPOLES_J2000);
	celestialPoles = new SkyPoint(SkyPoint::CELESTIALPOLES_OF_DATE);
	zenithNadir = new SkyPoint(SkyPoint::ZENITH_NADIR);
	eclipticJ2000Poles = new SkyPoint(SkyPoint::ECLIPTICPOLES_J2000);
	eclipticPoles = new SkyPoint(SkyPoint::ECLIPTICPOLES_OF_DATE);
	galacticPoles = new SkyPoint(SkyPoint::GALACTICPOLES);
	galacticCenter = new SkyPoint(SkyPoint::GALACTICCENTER);
	supergalacticPoles = new SkyPoint(SkyPoint::SUPERGALACTICPOLES);
	equinoxJ2000Points = new SkyPoint(SkyPoint::EQUINOXES_J2000);
	equinoxPoints = new SkyPoint(SkyPoint::EQUINOXES_OF_DATE);
	solsticeJ2000Points = new SkyPoint(SkyPoint::SOLSTICES_J2000);
	solsticePoints = new SkyPoint(SkyPoint::SOLSTICES_OF_DATE);
	antisolarPoint = new SkyPoint(SkyPoint::ANTISOLAR);
	apexPoints = new SkyPoint(SkyPoint::APEX);	

	earth = GETSTELMODULE(SolarSystem)->getEarth();
	connect(GETSTELMODULE(SolarSystem), SIGNAL(solarSystemDataReloaded()), this, SLOT(connectSolarSystem()));
}

GridLinesMgr::~GridLinesMgr()
{
	delete equGrid;
	delete equJ2000Grid;
	delete eclJ2000Grid;
	delete eclGrid;
	delete galacticGrid;
	delete supergalacticGrid;
	delete aziGrid;
	delete equatorLine;
	delete equatorJ2000Line;
	delete eclipticLine;
	delete eclipticJ2000Line;
	delete invariablePlaneLine;
	delete solarEquatorLine;
	delete precessionCircleN;
	delete precessionCircleS;
	delete meridianLine;
	delete horizonLine;
	delete galacticEquatorLine;
	delete supergalacticEquatorLine;
	delete longitudeLine;
	delete primeVerticalLine;
	delete currentVerticalLine;
	delete colureLine_1;
	delete colureLine_2;
	delete circumpolarCircleN;
	delete circumpolarCircleS;
	delete umbraCircle;
	delete penumbraCircle;
	delete celestialJ2000Poles;
	delete celestialPoles;
	delete zenithNadir;
	delete eclipticJ2000Poles;
	delete eclipticPoles;
	delete galacticPoles;
	delete galacticCenter;
	delete supergalacticPoles;
	delete equinoxJ2000Points;
	delete equinoxPoints;
	delete solsticeJ2000Points;
	delete solsticePoints;
	delete antisolarPoint;
	delete apexPoints;	
	SkyLine::deinit();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double GridLinesMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.;
	return 0.;
}

void GridLinesMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	// Upgrade config keys
	if (conf->contains("color/longitude_color"))
	{
		conf->setValue("color/oc_longitude_color", conf->value("color/longitude_color", "0.2,0.4,0.4").toString());
		conf->remove("color/longitude_color");
	}

	setFlagGridlines(conf->value("viewing/flag_gridlines", true).toBool());
	setFlagAzimuthalGrid(conf->value("viewing/flag_azimuthal_grid").toBool());
	setFlagEquatorGrid(conf->value("viewing/flag_equatorial_grid").toBool());
	setFlagEquatorJ2000Grid(conf->value("viewing/flag_equatorial_J2000_grid").toBool());
	setFlagEclipticJ2000Grid(conf->value("viewing/flag_ecliptic_J2000_grid").toBool());
	setFlagEclipticGrid(conf->value("viewing/flag_ecliptic_grid").toBool());
	setFlagGalacticGrid(conf->value("viewing/flag_galactic_grid").toBool());
	setFlagSupergalacticGrid(conf->value("viewing/flag_supergalactic_grid").toBool());
	setFlagEquatorLine(conf->value("viewing/flag_equator_line").toBool());
	setFlagEquatorParts(conf->value("viewing/flag_equator_parts").toBool());
	setFlagEquatorLabeled(conf->value("viewing/flag_equator_labels").toBool());
	setFlagEquatorJ2000Line(conf->value("viewing/flag_equator_J2000_line").toBool());
	setFlagEquatorJ2000Parts(conf->value("viewing/flag_equator_J2000_parts").toBool());
	setFlagEquatorJ2000Labeled(conf->value("viewing/flag_equator_J2000_labels").toBool());
	setFlagEclipticLine(conf->value("viewing/flag_ecliptic_line").toBool());
	setFlagEclipticParts(conf->value("viewing/flag_ecliptic_parts").toBool());
	setFlagEclipticLabeled(conf->value("viewing/flag_ecliptic_labels").toBool());
	setFlagEclipticJ2000Line(conf->value("viewing/flag_ecliptic_J2000_line").toBool());
	setFlagEclipticJ2000Parts(conf->value("viewing/flag_ecliptic_J2000_parts").toBool());
	setFlagEclipticJ2000Labeled(conf->value("viewing/flag_ecliptic_J2000_labels").toBool());	
	setFlagInvariablePlaneLine(conf->value("viewing/flag_invariable_plane_line").toBool());
	setFlagSolarEquatorLine(conf->value("viewing/flag_solar_equator_line").toBool());
	setFlagSolarEquatorParts(conf->value("viewing/flag_solar_equator_parts").toBool());
	setFlagSolarEquatorLabeled(conf->value("viewing/flag_solar_equator_labels").toBool());
	setFlagPrecessionCircles(conf->value("viewing/flag_precession_circles").toBool());
	setFlagPrecessionParts(conf->value("viewing/flag_precession_parts").toBool());
	setFlagPrecessionLabeled(conf->value("viewing/flag_precession_labels").toBool());
	setFlagMeridianLine(conf->value("viewing/flag_meridian_line").toBool());
	setFlagMeridianParts(conf->value("viewing/flag_meridian_parts").toBool());
	setFlagMeridianLabeled(conf->value("viewing/flag_meridian_labels").toBool());
	setFlagHorizonLine(conf->value("viewing/flag_horizon_line").toBool());
	setFlagHorizonParts(conf->value("viewing/flag_horizon_parts").toBool());
	setFlagHorizonLabeled(conf->value("viewing/flag_horizon_labels").toBool());
	setFlagGalacticEquatorLine(conf->value("viewing/flag_galactic_equator_line").toBool());
	setFlagGalacticEquatorParts(conf->value("viewing/flag_galactic_equator_parts").toBool());
	setFlagGalacticEquatorLabeled(conf->value("viewing/flag_galactic_equator_labels").toBool());
	setFlagSupergalacticEquatorLine(conf->value("viewing/flag_supergalactic_equator_line").toBool());
	setFlagSupergalacticEquatorParts(conf->value("viewing/flag_supergalactic_equator_parts").toBool());
	setFlagSupergalacticEquatorLabeled(conf->value("viewing/flag_supergalactic_equator_labels").toBool());
	setFlagLongitudeLine(conf->value("viewing/flag_longitude_line").toBool());
	setFlagLongitudeParts(conf->value("viewing/flag_longitude_parts").toBool());
	setFlagLongitudeLabeled(conf->value("viewing/flag_longitude_labels").toBool());
	setFlagPrimeVerticalLine(conf->value("viewing/flag_prime_vertical_line").toBool());
	setFlagPrimeVerticalParts(conf->value("viewing/flag_prime_vertical_parts").toBool());
	setFlagPrimeVerticalLabeled(conf->value("viewing/flag_prime_vertical_labels").toBool());
	setFlagCurrentVerticalLine(conf->value("viewing/flag_current_vertical_line").toBool());
	setFlagCurrentVerticalParts(conf->value("viewing/flag_current_vertical_parts").toBool());
	setFlagCurrentVerticalLabeled(conf->value("viewing/flag_current_vertical_labels").toBool());
	setFlagColureLines(conf->value("viewing/flag_colure_lines").toBool());
	setFlagColureParts(conf->value("viewing/flag_colure_parts").toBool());
	setFlagColureLabeled(conf->value("viewing/flag_colure_labels").toBool());
	setFlagCircumpolarCircles(conf->value("viewing/flag_circumpolar_circles").toBool());
	setFlagUmbraCircle(conf->value("viewing/flag_umbra_circle").toBool());
	setFlagPenumbraCircle(conf->value("viewing/flag_penumbra_circle").toBool());
	setFlagCelestialJ2000Poles(conf->value("viewing/flag_celestial_J2000_poles").toBool());
	setFlagCelestialPoles(conf->value("viewing/flag_celestial_poles").toBool());
	setFlagZenithNadir(conf->value("viewing/flag_zenith_nadir").toBool());
	setFlagEclipticJ2000Poles(conf->value("viewing/flag_ecliptic_J2000_poles").toBool());
	setFlagEclipticPoles(conf->value("viewing/flag_ecliptic_poles").toBool());
	setFlagGalacticPoles(conf->value("viewing/flag_galactic_poles").toBool());
	setFlagGalacticCenter(conf->value("viewing/flag_galactic_center").toBool());
	setFlagSupergalacticPoles(conf->value("viewing/flag_supergalactic_poles").toBool());
	setFlagEquinoxJ2000Points(conf->value("viewing/flag_equinox_J2000_points").toBool());
	setFlagEquinoxPoints(conf->value("viewing/flag_equinox_points").toBool());
	setFlagSolsticeJ2000Points(conf->value("viewing/flag_solstice_J2000_points").toBool());
	setFlagSolsticePoints(conf->value("viewing/flag_solstice_points").toBool());
	setFlagAntisolarPoint(conf->value("viewing/flag_antisolar_point").toBool());
	setFlagApexPoints(conf->value("viewing/flag_apex_points").toBool());

	// Set the line thickness for grids and lines
	setLineThickness(conf->value("viewing/line_thickness", 1).toInt());
	setPartThickness(conf->value("viewing/part_thickness", 1).toInt());

	// Load colors from config file
	QString defaultColor = conf->value("color/default_color", "0.5,0.5,0.7").toString();
	setColorEquatorGrid(             Vec3f(conf->value("color/equatorial_color", defaultColor).toString()));
	setColorEquatorJ2000Grid(        Vec3f(conf->value("color/equatorial_J2000_color", defaultColor).toString()));
	setColorEclipticJ2000Grid(       Vec3f(conf->value("color/ecliptical_J2000_color", defaultColor).toString()));
	setColorEclipticGrid(            Vec3f(conf->value("color/ecliptical_color", defaultColor).toString()));
	setColorGalacticGrid(            Vec3f(conf->value("color/galactic_color", defaultColor).toString()));
	setColorSupergalacticGrid(       Vec3f(conf->value("color/supergalactic_color", defaultColor).toString()));
	setColorAzimuthalGrid(           Vec3f(conf->value("color/azimuthal_color", defaultColor).toString()));
	setColorEquatorLine(             Vec3f(conf->value("color/equator_color", defaultColor).toString()));
	setColorEquatorJ2000Line(        Vec3f(conf->value("color/equator_J2000_color", defaultColor).toString()));
	setColorEclipticLine(            Vec3f(conf->value("color/ecliptic_color", defaultColor).toString()));
	setColorEclipticJ2000Line(       Vec3f(conf->value("color/ecliptic_J2000_color", defaultColor).toString()));
	setColorInvariablePlaneLine(     Vec3f(conf->value("color/invariable_plane_color", defaultColor).toString()));
	setColorSolarEquatorLine(        Vec3f(conf->value("color/solar_equator_color", defaultColor).toString()));
	setColorPrecessionCircles(       Vec3f(conf->value("color/precession_circles_color", defaultColor).toString()));
	setColorMeridianLine(            Vec3f(conf->value("color/meridian_color", defaultColor).toString()));
	setColorHorizonLine(             Vec3f(conf->value("color/horizon_color", defaultColor).toString()));
	setColorGalacticEquatorLine(     Vec3f(conf->value("color/galactic_equator_color", defaultColor).toString()));
	setColorSupergalacticEquatorLine(Vec3f(conf->value("color/supergalactic_equator_color", defaultColor).toString()));
	setColorLongitudeLine(           Vec3f(conf->value("color/oc_longitude_color", defaultColor).toString()));
	setColorPrimeVerticalLine(       Vec3f(conf->value("color/prime_vertical_color", defaultColor).toString()));
	setColorCurrentVerticalLine(     Vec3f(conf->value("color/current_vertical_color", defaultColor).toString()));
	setColorColureLines(             Vec3f(conf->value("color/colures_color", defaultColor).toString()));
	setColorCircumpolarCircles(      Vec3f(conf->value("color/circumpolar_circles_color", defaultColor).toString()));
	setColorUmbraCircle(             Vec3f(conf->value("color/umbra_circle_color", defaultColor).toString()));
	setColorPenumbraCircle(          Vec3f(conf->value("color/penumbra_circle_color", defaultColor).toString()));
	setColorCelestialJ2000Poles(     Vec3f(conf->value("color/celestial_J2000_poles_color", defaultColor).toString()));
	setColorCelestialPoles(          Vec3f(conf->value("color/celestial_poles_color", defaultColor).toString()));
	setColorZenithNadir(             Vec3f(conf->value("color/zenith_nadir_color", defaultColor).toString()));
	setColorEclipticJ2000Poles(      Vec3f(conf->value("color/ecliptic_J2000_poles_color", defaultColor).toString()));
	setColorEclipticPoles(           Vec3f(conf->value("color/ecliptic_poles_color", defaultColor).toString()));
	setColorGalacticPoles(           Vec3f(conf->value("color/galactic_poles_color", defaultColor).toString()));
	setColorGalacticCenter(          Vec3f(conf->value("color/galactic_center_color", defaultColor).toString()));
	setColorSupergalacticPoles(      Vec3f(conf->value("color/supergalactic_poles_color", defaultColor).toString()));
	setColorEquinoxJ2000Points(      Vec3f(conf->value("color/equinox_J2000_points_color", defaultColor).toString()));
	setColorEquinoxPoints(           Vec3f(conf->value("color/equinox_points_color", defaultColor).toString()));
	setColorSolsticeJ2000Points(     Vec3f(conf->value("color/solstice_J2000_points_color", defaultColor).toString()));
	setColorSolsticePoints(          Vec3f(conf->value("color/solstice_points_color", defaultColor).toString()));
	setColorAntisolarPoint(          Vec3f(conf->value("color/antisolar_point_color", defaultColor).toString()));
	setColorApexPoints(              Vec3f(conf->value("color/apex_points_color", defaultColor).toString()));

	StelApp& app = StelApp::getInstance();
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateLabels()));
	connect(&app, SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSizeFromApp(int)));
	
	QString displayGroup = N_("Display Options");
	addAction("actionShow_Gridlines",                  displayGroup, N_("Grids and lines"), "gridlinesDisplayed");
	addAction("actionShow_Equatorial_Grid",            displayGroup, N_("Equatorial grid"), "equatorGridDisplayed", "E");
	addAction("actionShow_Azimuthal_Grid",             displayGroup, N_("Azimuthal grid"), "azimuthalGridDisplayed", "Z");
	addAction("actionShow_Ecliptic_Line",              displayGroup, N_("Ecliptic line"), "eclipticLineDisplayed", ",");
	addAction("actionShow_Ecliptic_J2000_Line",        displayGroup, N_("Ecliptic J2000 line"), "eclipticJ2000LineDisplayed");
	addAction("actionShow_Invariable_Plane_Line",      displayGroup, N_("Invariable Plane line"), "invariablePlaneLineDisplayed");
	addAction("actionShow_Solar_Equator_Line",         displayGroup, N_("Solar Equator Plane line"), "solarEquatorLineDisplayed");
	addAction("actionShow_Equator_Line",               displayGroup, N_("Equator line"), "equatorLineDisplayed", ".");
	addAction("actionShow_Equator_J2000_Line",         displayGroup, N_("Equator J2000 line"), "equatorJ2000LineDisplayed"); // or with Hotkey??
	addAction("actionShow_Meridian_Line",              displayGroup, N_("Meridian line"), "meridianLineDisplayed", ";");
	addAction("actionShow_Horizon_Line",               displayGroup, N_("Horizon line"), "horizonLineDisplayed", "H");
	addAction("actionShow_Equatorial_J2000_Grid",      displayGroup, N_("Equatorial J2000 grid"), "equatorJ2000GridDisplayed");
	addAction("actionShow_Ecliptic_J2000_Grid",        displayGroup, N_("Ecliptic J2000 grid"), "eclipticJ2000GridDisplayed");
	addAction("actionShow_Ecliptic_Grid",              displayGroup, N_("Ecliptic grid"), "eclipticGridDisplayed");
	addAction("actionShow_Galactic_Grid",              displayGroup, N_("Galactic grid"), "galacticGridDisplayed");
	addAction("actionShow_Galactic_Equator_Line",      displayGroup, N_("Galactic equator"), "galacticEquatorLineDisplayed");
	addAction("actionShow_Supergalactic_Grid",         displayGroup, N_("Supergalactic grid"), "supergalacticGridDisplayed");
	addAction("actionShow_Supergalactic_Equator_Line", displayGroup, N_("Supergalactic equator"), "supergalacticEquatorLineDisplayed");
	addAction("actionShow_Longitude_Line",             displayGroup, N_("Opposition/conjunction longitude line"), "longitudeLineDisplayed");
	addAction("actionShow_Precession_Circles",         displayGroup, N_("Precession Circles"), "precessionCirclesDisplayed");
	addAction("actionShow_Prime_Vertical_Line",        displayGroup, N_("Prime Vertical"), "primeVerticalLineDisplayed");
	addAction("actionShow_Current_Vertical_Line",      displayGroup, N_("Current Vertical"), "currentVerticalLineDisplayed");
	addAction("actionShow_Colure_Lines",               displayGroup, N_("Colure Lines"), "colureLinesDisplayed");
	addAction("actionShow_Circumpolar_Circles",        displayGroup, N_("Circumpolar Circles"), "circumpolarCirclesDisplayed");
	addAction("actionShow_Umbra_Circle",	           displayGroup, N_("Umbra Circle"), "umbraCircleDisplayed");
	addAction("actionShow_Penumbra_Circle",	           displayGroup, N_("Penumbra Circle"), "penumbraCircleDisplayed");
	addAction("actionShow_Celestial_J2000_Poles",      displayGroup, N_("Celestial J2000 poles"), "celestialJ2000PolesDisplayed");
	addAction("actionShow_Celestial_Poles",            displayGroup, N_("Celestial poles"), "celestialPolesDisplayed");
	addAction("actionShow_Zenith_Nadir",               displayGroup, N_("Zenith and nadir"), "zenithNadirDisplayed");
	addAction("actionShow_Ecliptic_J2000_Poles",       displayGroup, N_("Ecliptic J2000 poles"), "eclipticJ2000PolesDisplayed");
	addAction("actionShow_Ecliptic_Poles",             displayGroup, N_("Ecliptic poles"), "eclipticPolesDisplayed");
	addAction("actionShow_Galactic_Poles",             displayGroup, N_("Galactic poles"), "galacticPolesDisplayed");
	addAction("actionShow_Galactic_Center",            displayGroup, N_("Galactic center and anticenter"), "galacticCenterDisplayed");
	addAction("actionShow_Supergalactic_Poles",        displayGroup, N_("Supergalactic poles"), "supergalacticPolesDisplayed");
	addAction("actionShow_Equinox_J2000_Points",       displayGroup, N_("Equinox J2000 points"), "equinoxJ2000PointsDisplayed");
	addAction("actionShow_Equinox_Points",             displayGroup, N_("Equinox points"), "equinoxPointsDisplayed");
	addAction("actionShow_Solstice_J2000_Points",      displayGroup, N_("Solstice J2000 points"), "solsticeJ2000PointsDisplayed");
	addAction("actionShow_Solstice_Points",            displayGroup, N_("Solstice points"), "solsticePointsDisplayed");
	addAction("actionShow_Antisolar_Point",            displayGroup, N_("Antisolar point"), "antisolarPointDisplayed");
	addAction("actionShow_Apex_Points",                displayGroup, N_("Apex points"), "apexPointsDisplayed");
}

void GridLinesMgr::connectSolarSystem()
{
	SolarSystem *ss=GETSTELMODULE(SolarSystem);
	earth = ss->getEarth();
	SkyLine::setSolarSystem(ss);
}

void GridLinesMgr::update(double deltaTime)
{
	// Update faders
	equGrid->update(deltaTime);
	equJ2000Grid->update(deltaTime);
	eclJ2000Grid->update(deltaTime);
	eclGrid->update(deltaTime);
	galacticGrid->update(deltaTime);
	supergalacticGrid->update(deltaTime);
	aziGrid->update(deltaTime);
	equatorLine->update(deltaTime);
	equatorJ2000Line->update(deltaTime);
	eclipticLine->update(deltaTime);
	eclipticJ2000Line->update(deltaTime);
	invariablePlaneLine->update(deltaTime);
	solarEquatorLine->update(deltaTime);
	precessionCircleN->update(deltaTime);
	precessionCircleS->update(deltaTime);
	meridianLine->update(deltaTime);
	horizonLine->update(deltaTime);
	galacticEquatorLine->update(deltaTime);
	supergalacticEquatorLine->update(deltaTime);
	longitudeLine->update(deltaTime);
	primeVerticalLine->update(deltaTime);
	currentVerticalLine->update(deltaTime);
	colureLine_1->update(deltaTime);
	colureLine_2->update(deltaTime);
	circumpolarCircleN->update(deltaTime);
	circumpolarCircleS->update(deltaTime);
	umbraCircle->update(deltaTime);
	penumbraCircle->update(deltaTime);
	celestialJ2000Poles->update(deltaTime);
	celestialPoles->update(deltaTime);
	zenithNadir->update(deltaTime);
	eclipticJ2000Poles->update(deltaTime);
	eclipticPoles->update(deltaTime);
	galacticPoles->update(deltaTime);
	galacticCenter->update(deltaTime);
	supergalacticPoles->update(deltaTime);
	equinoxJ2000Points->update(deltaTime);
	equinoxPoints->update(deltaTime);
	solsticeJ2000Points->update(deltaTime);
	solsticePoints->update(deltaTime);
	antisolarPoint->update(deltaTime);
	apexPoints->update(deltaTime);
	apexPoints->updateLabel();	
}

void GridLinesMgr::draw(StelCore* core)
{
	if (!gridlinesDisplayed)
		return;

	galacticGrid->draw(core);
	supergalacticGrid->draw(core);
	equJ2000Grid->draw(core);
	equGrid->draw(core);
	aziGrid->draw(core);
	eclJ2000Grid->draw(core);
	// While ecliptic of J2000 may be helpful to get a feeling of the Z=0 plane of VSOP87,
	// ecliptic of date is related to Earth and does not make much sense for the other planets.
	// Of course, orbital plane of respective planet would be better, but is not implemented.
	if (core->getCurrentPlanet()==earth)
	{
		penumbraCircle->draw(core);
		umbraCircle->draw(core);
		eclGrid->draw(core);
		eclipticLine->draw(core);
		precessionCircleN->draw(core);
		precessionCircleS->draw(core);
		colureLine_1->draw(core);
		colureLine_2->draw(core);
		eclipticPoles->draw(core);
		equinoxPoints->draw(core);
		solsticePoints->draw(core);
		longitudeLine->draw(core);
	}

	// Lines after grids, to be able to e.g. draw equators in different color!
	galacticEquatorLine->draw(core);
	supergalacticEquatorLine->draw(core);
	eclipticJ2000Line->draw(core);	
	equatorJ2000Line->draw(core);
	equatorLine->draw(core);
	invariablePlaneLine->draw(core);
	solarEquatorLine->draw(core);
	meridianLine->draw(core);
	horizonLine->draw(core);
	primeVerticalLine->draw(core);
	currentVerticalLine->draw(core);
	circumpolarCircleN->draw(core);
	circumpolarCircleS->draw(core);
	celestialJ2000Poles->draw(core);
	celestialPoles->draw(core);
	zenithNadir->draw(core);
	eclipticJ2000Poles->draw(core);
	galacticPoles->draw(core);
	galacticCenter->draw(core);
	supergalacticPoles->draw(core);
	equinoxJ2000Points->draw(core);
	solsticeJ2000Points->draw(core);	
	apexPoints->draw(core);	
	antisolarPoint->draw(core);
}

void GridLinesMgr::updateLabels()
{
	equatorJ2000Line->updateLabel();
	equatorLine->updateLabel();
	eclipticLine->updateLabel();
	eclipticJ2000Line->updateLabel();
	invariablePlaneLine->updateLabel();
	solarEquatorLine->updateLabel();
	precessionCircleN->updateLabel();
	precessionCircleS->updateLabel();
	meridianLine->updateLabel();
	horizonLine->updateLabel();
	galacticEquatorLine->updateLabel();
	supergalacticEquatorLine->updateLabel();
	longitudeLine->updateLabel();
	primeVerticalLine->updateLabel();
	currentVerticalLine->updateLabel();
	colureLine_1->updateLabel();
	colureLine_2->updateLabel();
	circumpolarCircleN->updateLabel();
	circumpolarCircleS->updateLabel();
	umbraCircle->updateLabel();
	penumbraCircle->updateLabel();
	celestialJ2000Poles->updateLabel();
	celestialPoles->updateLabel();
	zenithNadir->updateLabel();
	eclipticJ2000Poles->updateLabel();
	eclipticPoles->updateLabel();
	galacticPoles->updateLabel();
	galacticCenter->updateLabel();
	supergalacticPoles->updateLabel();
	equinoxJ2000Points->updateLabel();
	equinoxPoints->updateLabel();
	solsticeJ2000Points->updateLabel();
	solsticePoints->updateLabel();
	antisolarPoint->updateLabel();
	apexPoints->updateLabel();	
}

//! Setter ("master switch") for displaying any grid/line.
void GridLinesMgr::setFlagGridlines(const bool displayed)
{
	if(displayed != gridlinesDisplayed)
	{
		gridlinesDisplayed=displayed;
		emit gridlinesDisplayedChanged(displayed);
	}
}
//! Accessor ("master switch") for displaying any grid/line.
bool GridLinesMgr::getFlagGridlines() const
{
	return gridlinesDisplayed;
}

//! Setter ("master switch by type") for displaying all grids esp. for scripting
void GridLinesMgr::setFlagAllGrids(const bool displayed)
{
	setFlagEquatorGrid(displayed);
	setFlagEclipticGrid(displayed);
	setFlagGalacticGrid(displayed);
	setFlagAzimuthalGrid(displayed);
	setFlagEquatorJ2000Grid(displayed);
	setFlagEclipticJ2000Grid(displayed);
	setFlagSupergalacticGrid(displayed);
}

//! Setter ("master switch by type") for displaying all lines esp. for scripting
void GridLinesMgr::setFlagAllLines(const bool displayed)
{
	setFlagColureLines(displayed);
	setFlagEquatorLine(displayed);
	setFlagHorizonLine(displayed);
	setFlagEclipticLine(displayed);
	setFlagMeridianLine(displayed);
	setFlagLongitudeLine(displayed);
	setFlagEquatorJ2000Line(displayed);
	setFlagEclipticJ2000Line(displayed);
	setFlagInvariablePlaneLine(displayed);
	setFlagSolarEquatorLine(displayed);
	setFlagPrecessionCircles(displayed);
	setFlagPrimeVerticalLine(displayed);
	setFlagCurrentVerticalLine(displayed);
	setFlagCircumpolarCircles(displayed);
	setFlagUmbraCircle(displayed);
	setFlagPenumbraCircle(displayed);
	setFlagGalacticEquatorLine(displayed);
	setFlagSupergalacticEquatorLine(displayed);
}

//! Setter ("master switch by type") for displaying all points esp. for scripting
void GridLinesMgr::setFlagAllPoints(const bool displayed)
{
	setFlagZenithNadir(displayed);
	setFlagEclipticPoles(displayed);
	setFlagEquinoxPoints(displayed);
	setFlagGalacticPoles(displayed);
	setFlagGalacticCenter(displayed);
	setFlagAntisolarPoint(displayed);
	setFlagCelestialPoles(displayed);
	setFlagSolsticePoints(displayed);
	setFlagEclipticJ2000Poles(displayed);
	setFlagEquinoxJ2000Points(displayed);
	setFlagSupergalacticPoles(displayed);
	setFlagCelestialJ2000Poles(displayed);
	setFlagSolsticeJ2000Points(displayed);
	setFlagApexPoints(displayed);	
}

//! Set flag for displaying Azimuthal Grid
void GridLinesMgr::setFlagAzimuthalGrid(const bool displayed)
{
	if(displayed != aziGrid->isDisplayed())
	{
		aziGrid->setDisplayed(displayed);
		emit azimuthalGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Azimuthal Grid
bool GridLinesMgr::getFlagAzimuthalGrid() const
{
	return aziGrid->isDisplayed();
}
Vec3f GridLinesMgr::getColorAzimuthalGrid() const
{
	return aziGrid->getColor();
}
void GridLinesMgr::setColorAzimuthalGrid(const Vec3f& newColor)
{
	if(newColor != aziGrid->getColor())
	{
		aziGrid->setColor(newColor);
		emit azimuthalGridColorChanged(newColor);
	}
}

//! Set flag for displaying Equatorial Grid
void GridLinesMgr::setFlagEquatorGrid(const bool displayed)
{
	if(displayed != equGrid->isDisplayed())
	{
		equGrid->setDisplayed(displayed);
		emit equatorGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial Grid
bool GridLinesMgr::getFlagEquatorGrid() const
{
	return equGrid->isDisplayed();
}
Vec3f GridLinesMgr::getColorEquatorGrid() const
{
	return equGrid->getColor();
}
void GridLinesMgr::setColorEquatorGrid(const Vec3f& newColor)
{
	if(newColor != equGrid->getColor())
	{
		equGrid->setColor(newColor);
		emit equatorGridColorChanged(newColor);
	}
}

//! Set flag for displaying Equatorial J2000 Grid
void GridLinesMgr::setFlagEquatorJ2000Grid(const bool displayed)
{
	if(displayed != equJ2000Grid->isDisplayed())
	{
		equJ2000Grid->setDisplayed(displayed);
		emit equatorJ2000GridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial J2000 Grid
bool GridLinesMgr::getFlagEquatorJ2000Grid() const
{
	return equJ2000Grid->isDisplayed();
}
Vec3f GridLinesMgr::getColorEquatorJ2000Grid() const
{
	return equJ2000Grid->getColor();
}
void GridLinesMgr::setColorEquatorJ2000Grid(const Vec3f& newColor)
{
	if(newColor != equJ2000Grid->getColor())
	{
		equJ2000Grid->setColor(newColor);
		emit equatorJ2000GridColorChanged(newColor);
	}
}

//! Set flag for displaying Ecliptic J2000 Grid
void GridLinesMgr::setFlagEclipticJ2000Grid(const bool displayed)
{
	if(displayed != eclJ2000Grid->isDisplayed())
	{
		eclJ2000Grid->setDisplayed(displayed);
		emit eclipticJ2000GridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic J2000 Grid
bool GridLinesMgr::getFlagEclipticJ2000Grid() const
{
	return eclJ2000Grid->isDisplayed();
}
Vec3f GridLinesMgr::getColorEclipticJ2000Grid() const
{
	return eclJ2000Grid->getColor();
}
void GridLinesMgr::setColorEclipticJ2000Grid(const Vec3f& newColor)
{
	if(newColor != eclJ2000Grid->getColor())
	{
		eclJ2000Grid->setColor(newColor);
		emit eclipticJ2000GridColorChanged(newColor);
	}
}

//! Set flag for displaying Ecliptic of Date Grid
void GridLinesMgr::setFlagEclipticGrid(const bool displayed)
{
	if(displayed != eclGrid->isDisplayed())
	{
		eclGrid->setDisplayed(displayed);
		emit eclipticGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic of Date Grid
bool GridLinesMgr::getFlagEclipticGrid() const
{
	return eclGrid->isDisplayed();
}
Vec3f GridLinesMgr::getColorEclipticGrid() const
{
	return eclGrid->getColor();
}
void GridLinesMgr::setColorEclipticGrid(const Vec3f& newColor)
{
	if(newColor != eclGrid->getColor())
	{
		eclGrid->setColor(newColor);
		emit eclipticGridColorChanged(newColor);
	}
}

//! Set flag for displaying Galactic Grid
void GridLinesMgr::setFlagGalacticGrid(const bool displayed)
{
	if(displayed != galacticGrid->isDisplayed())
	{
		galacticGrid->setDisplayed(displayed);
		emit galacticGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Galactic Grid
bool GridLinesMgr::getFlagGalacticGrid() const
{
	return galacticGrid->isDisplayed();
}
Vec3f GridLinesMgr::getColorGalacticGrid() const
{
	return galacticGrid->getColor();
}
void GridLinesMgr::setColorGalacticGrid(const Vec3f& newColor)
{
	if(newColor != galacticGrid->getColor())
	{
		galacticGrid->setColor(newColor);
		emit galacticGridColorChanged(newColor);
	}
}

//! Set flag for displaying Supergalactic Grid
void GridLinesMgr::setFlagSupergalacticGrid(const bool displayed)
{
	if(displayed != supergalacticGrid->isDisplayed())
	{
		supergalacticGrid->setDisplayed(displayed);
		emit supergalacticGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Supergalactic Grid
bool GridLinesMgr::getFlagSupergalacticGrid() const
{
	return supergalacticGrid->isDisplayed();
}
Vec3f GridLinesMgr::getColorSupergalacticGrid() const
{
	return supergalacticGrid->getColor();
}
void GridLinesMgr::setColorSupergalacticGrid(const Vec3f& newColor)
{
	if(newColor != supergalacticGrid->getColor())
	{
		supergalacticGrid->setColor(newColor);
		emit supergalacticGridColorChanged(newColor);
	}
}

//! Set flag for displaying Equatorial Line
void GridLinesMgr::setFlagEquatorLine(const bool displayed)
{
	if(displayed != equatorLine->isDisplayed())
	{
		equatorLine->setDisplayed(displayed);
		emit equatorLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial Line
bool GridLinesMgr::getFlagEquatorLine() const
{
	return equatorLine->isDisplayed();
}
//! Set flag for displaying Equatorial Line partitions
void GridLinesMgr::setFlagEquatorParts(const bool displayed)
{
	if(displayed != equatorLine->showsPartitions())
	{
		equatorLine->setPartitions(displayed);
		emit equatorPartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial Line partitions
bool GridLinesMgr::getFlagEquatorParts() const
{
	return equatorLine->showsPartitions();
}
void GridLinesMgr::setFlagEquatorLabeled(const bool displayed)
{
	if(displayed != equatorLine->isLabeled())
	{
		equatorLine->setLabeled(displayed);
		emit equatorPartsLabeledChanged(displayed);
	}
}
bool GridLinesMgr::getFlagEquatorLabeled() const
{
	return equatorLine->isLabeled();
}
Vec3f GridLinesMgr::getColorEquatorLine() const
{
	return equatorLine->getColor();
}
void GridLinesMgr::setColorEquatorLine(const Vec3f& newColor)
{
	if(newColor != equatorLine->getColor())
	{
		equatorLine->setColor(newColor);
		emit equatorLineColorChanged(newColor);
	}
}

//! Set flag for displaying J2000 Equatorial Line
void GridLinesMgr::setFlagEquatorJ2000Line(const bool displayed)
{
	if(displayed != equatorJ2000Line->isDisplayed())
	{
		equatorJ2000Line->setDisplayed(displayed);
		emit equatorJ2000LineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying J2000 Equatorial Line
bool GridLinesMgr::getFlagEquatorJ2000Line() const
{
	return equatorJ2000Line->isDisplayed();
}
//! Set flag for displaying J2000 Equatorial Line partitions
void GridLinesMgr::setFlagEquatorJ2000Parts(const bool displayed)
{
	if(displayed != equatorJ2000Line->showsPartitions())
	{
		equatorJ2000Line->setPartitions(displayed);
		emit equatorJ2000PartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying J2000 Equatorial Line partitions
bool GridLinesMgr::getFlagEquatorJ2000Parts() const
{
	return equatorJ2000Line->showsPartitions();
}
void GridLinesMgr::setFlagEquatorJ2000Labeled(const bool displayed)
{
	if(displayed != equatorJ2000Line->isLabeled())
	{
		equatorJ2000Line->setLabeled(displayed);
		emit equatorJ2000PartsLabeledChanged(displayed);
	}
}
bool GridLinesMgr::getFlagEquatorJ2000Labeled() const
{
	return equatorJ2000Line->isLabeled();
}
Vec3f GridLinesMgr::getColorEquatorJ2000Line() const
{
	return equatorJ2000Line->getColor();
}
void GridLinesMgr::setColorEquatorJ2000Line(const Vec3f& newColor)
{
	if(newColor != equatorJ2000Line->getColor())
	{
		equatorJ2000Line->setColor(newColor);
		emit equatorJ2000LineColorChanged(newColor);
	}
}

//! Set flag for displaying Ecliptic Line
void GridLinesMgr::setFlagEclipticLine(const bool displayed)
{
	if(displayed != eclipticLine->isDisplayed())
	{
		eclipticLine->setDisplayed(displayed);
		emit eclipticLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic Line
bool GridLinesMgr::getFlagEclipticLine() const
{
	return eclipticLine->isDisplayed();
}
//! Set flag for displaying Ecliptic Line partitions
void GridLinesMgr::setFlagEclipticParts(const bool displayed)
{
	if(displayed != eclipticLine->showsPartitions())
	{
		eclipticLine->setPartitions(displayed);
		emit eclipticPartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic Line partitions
bool GridLinesMgr::getFlagEclipticParts() const
{
	return eclipticLine->showsPartitions();
}
//! Set flag for displaying Ecliptic Line partitions
void GridLinesMgr::setFlagEclipticLabeled(const bool displayed)
{
	if(displayed != eclipticLine->isLabeled())
	{
		eclipticLine->setLabeled(displayed);
		emit eclipticPartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic Line partitions
bool GridLinesMgr::getFlagEclipticLabeled() const
{
	return eclipticLine->isLabeled();
}
Vec3f GridLinesMgr::getColorEclipticLine() const
{
	return eclipticLine->getColor();
}
void GridLinesMgr::setColorEclipticLine(const Vec3f& newColor)
{
	if(newColor != eclipticLine->getColor())
	{
		eclipticLine->setColor(newColor);
		emit eclipticLineColorChanged(newColor);
	}
}

//! Set flag for displaying Ecliptic J2000 Line
void GridLinesMgr::setFlagEclipticJ2000Line(const bool displayed)
{
	if(displayed != eclipticJ2000Line->isDisplayed())
	{
		eclipticJ2000Line->setDisplayed(displayed);
		emit eclipticJ2000LineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic J2000 Line
bool GridLinesMgr::getFlagEclipticJ2000Line() const
{
	return eclipticJ2000Line->isDisplayed();
}
//! Set flag for displaying Ecliptic J2000 Line partitions
void GridLinesMgr::setFlagEclipticJ2000Parts(const bool displayed)
{
	if(displayed != eclipticJ2000Line->showsPartitions())
	{
		eclipticJ2000Line->setPartitions(displayed);
		emit eclipticJ2000PartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic J2000 Line partitions
bool GridLinesMgr::getFlagEclipticJ2000Parts() const
{
	return eclipticJ2000Line->showsPartitions();
}
//! Set flag for displaying Ecliptic J2000 Line partitions
void GridLinesMgr::setFlagEclipticJ2000Labeled(const bool displayed)
{
	if(displayed != eclipticJ2000Line->isLabeled())
	{
		eclipticJ2000Line->setLabeled(displayed);
		emit eclipticJ2000PartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic J2000 Line partitions
bool GridLinesMgr::getFlagEclipticJ2000Labeled() const
{
	return eclipticJ2000Line->isLabeled();
}
Vec3f GridLinesMgr::getColorEclipticJ2000Line() const
{
	return eclipticJ2000Line->getColor();
}
void GridLinesMgr::setColorEclipticJ2000Line(const Vec3f& newColor)
{
	if(newColor != eclipticJ2000Line->getColor())
	{
		eclipticJ2000Line->setColor(newColor);
		emit eclipticJ2000LineColorChanged(newColor);
	}
}

//! Set flag for displaying Invariable Plane Line
void GridLinesMgr::setFlagInvariablePlaneLine(const bool displayed)
{
	if(displayed != invariablePlaneLine->isDisplayed())
	{
		invariablePlaneLine->setDisplayed(displayed);
		emit invariablePlaneLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Invariable Plane Line
bool GridLinesMgr::getFlagInvariablePlaneLine() const
{
	return invariablePlaneLine->isDisplayed();
}
Vec3f GridLinesMgr::getColorInvariablePlaneLine() const
{
	return invariablePlaneLine->getColor();
}
void GridLinesMgr::setColorInvariablePlaneLine(const Vec3f& newColor)
{
	if(newColor != invariablePlaneLine->getColor())
	{
		invariablePlaneLine->setColor(newColor);
		emit invariablePlaneLineColorChanged(newColor);
	}
}

//! Set flag for displaying Solar Equator Line
void GridLinesMgr::setFlagSolarEquatorLine(const bool displayed)
{
	if(displayed != solarEquatorLine->isDisplayed())
	{
		solarEquatorLine->setDisplayed(displayed);
		emit solarEquatorLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Solar Equator Line
bool GridLinesMgr::getFlagSolarEquatorLine() const
{
	return solarEquatorLine->isDisplayed();
}
//! Set flag for displaying Solar Equator Line partitions
void GridLinesMgr::setFlagSolarEquatorParts(const bool displayed)
{
	if(displayed != solarEquatorLine->showsPartitions())
	{
		solarEquatorLine->setPartitions(displayed);
		emit solarEquatorPartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Solar Equator Line partitions
bool GridLinesMgr::getFlagSolarEquatorParts() const
{
	return solarEquatorLine->showsPartitions();
}
//! Set flag for displaying Solar Equator Line partitions
void GridLinesMgr::setFlagSolarEquatorLabeled(const bool displayed)
{
	if(displayed != solarEquatorLine->isLabeled())
	{
		solarEquatorLine->setLabeled(displayed);
		emit solarEquatorPartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Solar Equator Line partitions
bool GridLinesMgr::getFlagSolarEquatorLabeled() const
{
	return solarEquatorLine->isLabeled();
}
Vec3f GridLinesMgr::getColorSolarEquatorLine() const
{
	return solarEquatorLine->getColor();
}
void GridLinesMgr::setColorSolarEquatorLine(const Vec3f& newColor)
{
	if(newColor != solarEquatorLine->getColor())
	{
		solarEquatorLine->setColor(newColor);
		emit solarEquatorLineColorChanged(newColor);
	}
}

//! Set flag for displaying Precession Circles
void GridLinesMgr::setFlagPrecessionCircles(const bool displayed)
{
	if(displayed != precessionCircleN->isDisplayed())
	{
		precessionCircleN->setDisplayed(displayed);
		precessionCircleS->setDisplayed(displayed);
		emit precessionCirclesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Precession Circles
bool GridLinesMgr::getFlagPrecessionCircles() const
{
	// precessionCircleS is always synchronous, no separate queries.
	return precessionCircleN->isDisplayed();
}
//! Set flag for displaying Precession Circle partitions
void GridLinesMgr::setFlagPrecessionParts(const bool displayed)
{
	if(displayed != precessionCircleN->showsPartitions())
	{
		precessionCircleN->setPartitions(displayed);
		precessionCircleS->setPartitions(displayed);
		emit precessionPartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Precession Circle partitions
bool GridLinesMgr::getFlagPrecessionParts() const
{
	// precessionCircleS is always synchronous, no separate queries.
	return precessionCircleN->showsPartitions();
}
//! Set flag for displaying Precession Circle partitions
void GridLinesMgr::setFlagPrecessionLabeled(const bool displayed)
{
	if(displayed != precessionCircleN->isLabeled())
	{
		precessionCircleN->setLabeled(displayed);
		precessionCircleS->setLabeled(displayed);
		emit precessionPartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Precession Circle partitions
bool GridLinesMgr::getFlagPrecessionLabeled() const
{
	// precessionCircleS is always synchronous, no separate queries.
	return precessionCircleN->isLabeled();
}
Vec3f GridLinesMgr::getColorPrecessionCircles() const
{
	return precessionCircleN->getColor();
}
void GridLinesMgr::setColorPrecessionCircles(const Vec3f& newColor)
{
	if(newColor != precessionCircleN->getColor())
	{
		precessionCircleN->setColor(newColor);
		precessionCircleS->setColor(newColor);
		emit precessionCirclesColorChanged(newColor);
	}
}

//! Set flag for displaying Meridian Line
void GridLinesMgr::setFlagMeridianLine(const bool displayed)
{
	if(displayed != meridianLine->isDisplayed())
	{
		meridianLine->setDisplayed(displayed);
		emit meridianLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Meridian Line
bool GridLinesMgr::getFlagMeridianLine() const
{
	return meridianLine->isDisplayed();
}
//! Set flag for displaying Meridian Line partitions
void GridLinesMgr::setFlagMeridianParts(const bool displayed)
{
	if(displayed != meridianLine->showsPartitions())
	{
		meridianLine->setPartitions(displayed);
		emit meridianPartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Meridian Line partitions
bool GridLinesMgr::getFlagMeridianParts() const
{
	return meridianLine->showsPartitions();
}
//! Set flag for displaying Meridian Line partitions
void GridLinesMgr::setFlagMeridianLabeled(const bool displayed)
{
	if(displayed != meridianLine->isLabeled())
	{
		meridianLine->setLabeled(displayed);
		emit meridianPartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Meridian Line partitions
bool GridLinesMgr::getFlagMeridianLabeled() const
{
	return meridianLine->isLabeled();
}
Vec3f GridLinesMgr::getColorMeridianLine() const
{
	return meridianLine->getColor();
}
void GridLinesMgr::setColorMeridianLine(const Vec3f& newColor)
{
	if(newColor != meridianLine->getColor())
	{
		meridianLine->setColor(newColor);
		emit meridianLineColorChanged(newColor);
	}
}

//! Set flag for displaying opposition/conjunction longitude line
void GridLinesMgr::setFlagLongitudeLine(const bool displayed)
{
	if(displayed != longitudeLine->isDisplayed())
	{
		longitudeLine->setDisplayed(displayed);
		emit longitudeLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying opposition/conjunction longitude line
bool GridLinesMgr::getFlagLongitudeLine() const
{
	return longitudeLine->isDisplayed();
}
//! Set flag for displaying opposition/conjunction longitude line partitions
void GridLinesMgr::setFlagLongitudeParts(const bool displayed)
{
	if(displayed != longitudeLine->showsPartitions())
	{
		longitudeLine->setPartitions(displayed);
		emit longitudePartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying opposition/conjunction longitude line partitions
bool GridLinesMgr::getFlagLongitudeParts() const
{
	return longitudeLine->showsPartitions();
}
//! Set flag for displaying opposition/conjunction longitude line partitions
void GridLinesMgr::setFlagLongitudeLabeled(const bool displayed)
{
	if(displayed != longitudeLine->isLabeled())
	{
		longitudeLine->setLabeled(displayed);
		emit longitudePartsLabeledChanged(displayed);
	}
}
bool GridLinesMgr::getFlagLongitudeLabeled() const
{
	return longitudeLine->isLabeled();
}
Vec3f GridLinesMgr::getColorLongitudeLine() const
{
	return longitudeLine->getColor();
}
void GridLinesMgr::setColorLongitudeLine(const Vec3f& newColor)
{
	if(newColor != longitudeLine->getColor())
	{
		longitudeLine->setColor(newColor);
		emit longitudeLineColorChanged(newColor);
	}
}

//! Set flag for displaying Horizon Line
void GridLinesMgr::setFlagHorizonLine(const bool displayed)
{
	if(displayed != horizonLine->isDisplayed())
	{
		horizonLine->setDisplayed(displayed);
		emit horizonLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Horizon Line
bool GridLinesMgr::getFlagHorizonLine() const
{
	return horizonLine->isDisplayed();
}
//! Set flag for displaying Horizon Line partitions
void GridLinesMgr::setFlagHorizonParts(const bool displayed)
{
	if(displayed != horizonLine->showsPartitions())
	{
		horizonLine->setPartitions(displayed);
		emit horizonPartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Horizon Line partitions
bool GridLinesMgr::getFlagHorizonParts() const
{
	return horizonLine->showsPartitions();
}
//! Set flag for displaying Horizon Line partitions
void GridLinesMgr::setFlagHorizonLabeled(const bool displayed)
{
	if(displayed != horizonLine->isLabeled())
	{
		horizonLine->setLabeled(displayed);
		emit horizonPartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Horizon Line partitions
bool GridLinesMgr::getFlagHorizonLabeled() const
{
	return horizonLine->isLabeled();
}
Vec3f GridLinesMgr::getColorHorizonLine() const
{
	return horizonLine->getColor();
}
void GridLinesMgr::setColorHorizonLine(const Vec3f& newColor)
{
	if(newColor != horizonLine->getColor())
	{
		horizonLine->setColor(newColor);
		emit horizonLineColorChanged(newColor);
	}
}

//! Set flag for displaying Galactic Equator Line
void GridLinesMgr::setFlagGalacticEquatorLine(const bool displayed)
{
	if(displayed != galacticEquatorLine->isDisplayed())
	{
		galacticEquatorLine->setDisplayed(displayed);
		emit galacticEquatorLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Galactic Equator Line
bool GridLinesMgr::getFlagGalacticEquatorLine() const
{
	return galacticEquatorLine->isDisplayed();
}
//! Set flag for displaying Galactic Equator Line partitions
void GridLinesMgr::setFlagGalacticEquatorParts(const bool displayed)
{
	if(displayed != galacticEquatorLine->showsPartitions())
	{
		galacticEquatorLine->setPartitions(displayed);
		emit galacticEquatorPartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Galactic Equator Line partitions
bool GridLinesMgr::getFlagGalacticEquatorParts() const
{
	return galacticEquatorLine->showsPartitions();
}
//! Set flag for displaying Galactic Equator Line partitions
void GridLinesMgr::setFlagGalacticEquatorLabeled(const bool displayed)
{
	if(displayed != galacticEquatorLine->isLabeled())
	{
		galacticEquatorLine->setLabeled(displayed);
		emit galacticEquatorPartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Galactic Equator Line partitions
bool GridLinesMgr::getFlagGalacticEquatorLabeled() const
{
	return galacticEquatorLine->isLabeled();
}
Vec3f GridLinesMgr::getColorGalacticEquatorLine() const
{
	return galacticEquatorLine->getColor();
}
void GridLinesMgr::setColorGalacticEquatorLine(const Vec3f& newColor)
{
	if(newColor != galacticEquatorLine->getColor())
	{
		galacticEquatorLine->setColor(newColor);
		emit galacticEquatorLineColorChanged(newColor);
	}
}

//! Set flag for displaying Supergalactic Equator Line
void GridLinesMgr::setFlagSupergalacticEquatorLine(const bool displayed)
{
	if(displayed != supergalacticEquatorLine->isDisplayed())
	{
		supergalacticEquatorLine->setDisplayed(displayed);
		emit supergalacticEquatorLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Supergalactic Equator Line
bool GridLinesMgr::getFlagSupergalacticEquatorLine() const
{
	return supergalacticEquatorLine->isDisplayed();
}
//! Set flag for displaying Supergalactic Equator Line partitions
void GridLinesMgr::setFlagSupergalacticEquatorParts(const bool displayed)
{
	if(displayed != supergalacticEquatorLine->showsPartitions())
	{
		supergalacticEquatorLine->setPartitions(displayed);
		emit supergalacticEquatorPartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Supergalactic Equator Line partitions
bool GridLinesMgr::getFlagSupergalacticEquatorParts() const
{
	return supergalacticEquatorLine->showsPartitions();
}
//! Set flag for displaying Supergalactic Equator Line partitions
void GridLinesMgr::setFlagSupergalacticEquatorLabeled(const bool displayed)
{
	if(displayed != supergalacticEquatorLine->isLabeled())
	{
		supergalacticEquatorLine->setLabeled(displayed);
		emit supergalacticEquatorPartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Supergalactic Equator Line partitions
bool GridLinesMgr::getFlagSupergalacticEquatorLabeled() const
{
	return supergalacticEquatorLine->isLabeled();
}
Vec3f GridLinesMgr::getColorSupergalacticEquatorLine() const
{
	return supergalacticEquatorLine->getColor();
}
void GridLinesMgr::setColorSupergalacticEquatorLine(const Vec3f& newColor)
{
	if(newColor != supergalacticEquatorLine->getColor())
	{
		supergalacticEquatorLine->setColor(newColor);
		emit supergalacticEquatorLineColorChanged(newColor);
	}
}

//! Set flag for displaying Prime Vertical Line
void GridLinesMgr::setFlagPrimeVerticalLine(const bool displayed)
{
	if(displayed != primeVerticalLine->isDisplayed())
	{
		primeVerticalLine->setDisplayed(displayed);
		emit  primeVerticalLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Prime Vertical Line
bool GridLinesMgr::getFlagPrimeVerticalLine() const
{
	return primeVerticalLine->isDisplayed();
}
//! Set flag for displaying Prime Vertical Line partitions
void GridLinesMgr::setFlagPrimeVerticalParts(const bool displayed)
{
	if(displayed != primeVerticalLine->showsPartitions())
	{
		primeVerticalLine->setPartitions(displayed);
		emit  primeVerticalPartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Prime Vertical Line partitions
bool GridLinesMgr::getFlagPrimeVerticalParts() const
{
	return primeVerticalLine->showsPartitions();
}
//! Set flag for displaying Prime Vertical Line partitions
void GridLinesMgr::setFlagPrimeVerticalLabeled(const bool displayed)
{
	if(displayed != primeVerticalLine->isLabeled())
	{
		primeVerticalLine->setLabeled(displayed);
		emit  primeVerticalPartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Prime Vertical Line partitions
bool GridLinesMgr::getFlagPrimeVerticalLabeled() const
{
	return primeVerticalLine->isLabeled();
}
Vec3f GridLinesMgr::getColorPrimeVerticalLine() const
{
	return primeVerticalLine->getColor();
}
void GridLinesMgr::setColorPrimeVerticalLine(const Vec3f& newColor)
{
	if(newColor != primeVerticalLine->getColor())
	{
		primeVerticalLine->setColor(newColor);
		emit primeVerticalLineColorChanged(newColor);
	}
}

//! Set flag for displaying Current Vertical Line
void GridLinesMgr::setFlagCurrentVerticalLine(const bool displayed)
{
	if(displayed != currentVerticalLine->isDisplayed())
	{
		currentVerticalLine->setDisplayed(displayed);
		emit  currentVerticalLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Current Vertical Line
bool GridLinesMgr::getFlagCurrentVerticalLine() const
{
	return currentVerticalLine->isDisplayed();
}
//! Set flag for displaying Current Vertical Line partitions
void GridLinesMgr::setFlagCurrentVerticalParts(const bool displayed)
{
	if(displayed != currentVerticalLine->showsPartitions())
	{
		currentVerticalLine->setPartitions(displayed);
		emit  currentVerticalPartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Current Vertical Line partitions
bool GridLinesMgr::getFlagCurrentVerticalParts() const
{
	return currentVerticalLine->showsPartitions();
}
//! Set flag for displaying Current Vertical Line partitions
void GridLinesMgr::setFlagCurrentVerticalLabeled(const bool displayed)
{
	if(displayed != currentVerticalLine->isLabeled())
	{
		currentVerticalLine->setLabeled(displayed);
		emit  currentVerticalPartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Current Vertical Line partitions
bool GridLinesMgr::getFlagCurrentVerticalLabeled() const
{
	return currentVerticalLine->isLabeled();
}
Vec3f GridLinesMgr::getColorCurrentVerticalLine() const
{
	return currentVerticalLine->getColor();
}
void GridLinesMgr::setColorCurrentVerticalLine(const Vec3f& newColor)
{
	if(newColor != currentVerticalLine->getColor())
	{
		currentVerticalLine->setColor(newColor);
		emit currentVerticalLineColorChanged(newColor);
	}
}

//! Set flag for displaying Colure Lines
void GridLinesMgr::setFlagColureLines(const bool displayed)
{
	if(displayed != colureLine_1->isDisplayed())
	{
		colureLine_1->setDisplayed(displayed);
		colureLine_2->setDisplayed(displayed);
		emit  colureLinesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Colure Lines
bool GridLinesMgr::getFlagColureLines() const
{
	return colureLine_1->isDisplayed();
}
//! Set flag for displaying Colure Line partitions
void GridLinesMgr::setFlagColureParts(const bool displayed)
{
	if(displayed != colureLine_1->showsPartitions())
	{
		colureLine_1->setPartitions(displayed);
		colureLine_2->setPartitions(displayed);
		emit  colurePartsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Colure Line partitions
bool GridLinesMgr::getFlagColureParts() const
{
	return colureLine_1->showsPartitions();
}
void GridLinesMgr::setFlagColureLabeled(const bool displayed)
{
	if(displayed != colureLine_1->isLabeled())
	{
		colureLine_1->setLabeled(displayed);
		colureLine_2->setLabeled(displayed);
		emit  colurePartsLabeledChanged(displayed);
	}
}
//! Get flag for displaying Colure Line partitions
bool GridLinesMgr::getFlagColureLabeled() const
{
	return colureLine_1->isLabeled();
}
Vec3f GridLinesMgr::getColorColureLines() const
{
	return colureLine_1->getColor();
}
void GridLinesMgr::setColorColureLines(const Vec3f& newColor)
{
	if(newColor != colureLine_1->getColor())
	{
		colureLine_1->setColor(newColor);
		colureLine_2->setColor(newColor);
		emit colureLinesColorChanged(newColor);
	}
}

//! Set flag for displaying Circumpolar Circles
void GridLinesMgr::setFlagCircumpolarCircles(const bool displayed)
{
	if(displayed != circumpolarCircleN->isDisplayed())
	{
		circumpolarCircleN->setDisplayed(displayed);
		circumpolarCircleS->setDisplayed(displayed);
		emit circumpolarCirclesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Circumpolar Circles
bool GridLinesMgr::getFlagCircumpolarCircles() const
{
	// circumpolarCircleS is always synchronous, no separate queries.
	return circumpolarCircleN->isDisplayed();
}
Vec3f GridLinesMgr::getColorCircumpolarCircles() const
{
	return circumpolarCircleN->getColor();
}
void GridLinesMgr::setColorCircumpolarCircles(const Vec3f& newColor)
{
	if(newColor != circumpolarCircleN->getColor())
	{
		circumpolarCircleN->setColor(newColor);
		circumpolarCircleS->setColor(newColor);
		emit circumpolarCirclesColorChanged(newColor);
	}
}

//! Set flag for displaying Umbra Circle
void GridLinesMgr::setFlagUmbraCircle(const bool displayed)
{
	if(displayed != umbraCircle->isDisplayed())
	{
		umbraCircle->setDisplayed(displayed);
		emit umbraCircleDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Umbra Circle
bool GridLinesMgr::getFlagUmbraCircle() const
{
	return umbraCircle->isDisplayed();
}
Vec3f GridLinesMgr::getColorUmbraCircle() const
{
	return umbraCircle->getColor();
}
void GridLinesMgr::setColorUmbraCircle(const Vec3f& newColor)
{
	if(newColor != umbraCircle->getColor())
	{
		umbraCircle->setColor(newColor);
		emit umbraCircleColorChanged(newColor);
	}
}

//! Set flag for displaying Penumbra Circle
void GridLinesMgr::setFlagPenumbraCircle(const bool displayed)
{
	if(displayed != penumbraCircle->isDisplayed())
	{
		penumbraCircle->setDisplayed(displayed);
		emit penumbraCircleDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Penumbra Circle
bool GridLinesMgr::getFlagPenumbraCircle() const
{
	return penumbraCircle->isDisplayed();
}
Vec3f GridLinesMgr::getColorPenumbraCircle() const
{
	return penumbraCircle->getColor();
}
void GridLinesMgr::setColorPenumbraCircle(const Vec3f& newColor)
{
	if(newColor != penumbraCircle->getColor())
	{
		penumbraCircle->setColor(newColor);
		emit penumbraCircleColorChanged(newColor);
	}
}

//! Set flag for displaying celestial poles of J2000
void GridLinesMgr::setFlagCelestialJ2000Poles(const bool displayed)
{
	if(displayed != celestialJ2000Poles->isDisplayed())
	{
		celestialJ2000Poles->setDisplayed(displayed);
		emit celestialJ2000PolesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying celestial poles of J2000
bool GridLinesMgr::getFlagCelestialJ2000Poles() const
{
	return celestialJ2000Poles->isDisplayed();
}
Vec3f GridLinesMgr::getColorCelestialJ2000Poles() const
{
	return celestialJ2000Poles->getColor();
}
void GridLinesMgr::setColorCelestialJ2000Poles(const Vec3f& newColor)
{
	if(newColor != celestialJ2000Poles->getColor())
	{
		celestialJ2000Poles->setColor(newColor);
		emit celestialJ2000PolesColorChanged(newColor);
	}
}

//! Set flag for displaying celestial poles
void GridLinesMgr::setFlagCelestialPoles(const bool displayed)
{
	if(displayed != celestialPoles->isDisplayed())
	{
		celestialPoles->setDisplayed(displayed);
		emit celestialPolesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying celestial poles
bool GridLinesMgr::getFlagCelestialPoles() const
{
	return celestialPoles->isDisplayed();
}
Vec3f GridLinesMgr::getColorCelestialPoles() const
{
	return celestialPoles->getColor();
}
void GridLinesMgr::setColorCelestialPoles(const Vec3f& newColor)
{
	if(newColor != celestialPoles->getColor())
	{
		celestialPoles->setColor(newColor);
		emit celestialPolesColorChanged(newColor);
	}
}

//! Set flag for displaying zenith and nadir
void GridLinesMgr::setFlagZenithNadir(const bool displayed)
{
	if(displayed != zenithNadir->isDisplayed())
	{
		zenithNadir->setDisplayed(displayed);
		emit zenithNadirDisplayedChanged(displayed);
	}
}
//! Get flag for displaying zenith and nadir
bool GridLinesMgr::getFlagZenithNadir() const
{
	return zenithNadir->isDisplayed();
}
Vec3f GridLinesMgr::getColorZenithNadir() const
{
	return zenithNadir->getColor();
}
void GridLinesMgr::setColorZenithNadir(const Vec3f& newColor)
{
	if(newColor != zenithNadir->getColor())
	{
		zenithNadir->setColor(newColor);
		emit zenithNadirColorChanged(newColor);
	}
}

//! Set flag for displaying ecliptic poles of J2000
void GridLinesMgr::setFlagEclipticJ2000Poles(const bool displayed)
{
	if(displayed != eclipticJ2000Poles->isDisplayed())
	{
		eclipticJ2000Poles->setDisplayed(displayed);
		emit eclipticJ2000PolesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying ecliptic poles of J2000
bool GridLinesMgr::getFlagEclipticJ2000Poles() const
{
	return eclipticJ2000Poles->isDisplayed();
}
Vec3f GridLinesMgr::getColorEclipticJ2000Poles() const
{
	return eclipticJ2000Poles->getColor();
}
void GridLinesMgr::setColorEclipticJ2000Poles(const Vec3f& newColor)
{
	if(newColor != eclipticJ2000Poles->getColor())
	{
		eclipticJ2000Poles->setColor(newColor);
		emit eclipticJ2000PolesColorChanged(newColor);
	}
}

//! Set flag for displaying ecliptic poles
void GridLinesMgr::setFlagEclipticPoles(const bool displayed)
{
	if(displayed != eclipticPoles->isDisplayed())
	{
		eclipticPoles->setDisplayed(displayed);
		emit eclipticPolesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying ecliptic poles
bool GridLinesMgr::getFlagEclipticPoles() const
{
	return eclipticPoles->isDisplayed();
}
Vec3f GridLinesMgr::getColorEclipticPoles() const
{
	return eclipticPoles->getColor();
}
void GridLinesMgr::setColorEclipticPoles(const Vec3f& newColor)
{
	if(newColor != eclipticPoles->getColor())
	{
		eclipticPoles->setColor(newColor);
		emit eclipticPolesColorChanged(newColor);
	}
}

//! Set flag for displaying galactic poles
void GridLinesMgr::setFlagGalacticPoles(const bool displayed)
{
	if(displayed != galacticPoles->isDisplayed())
	{
		galacticPoles->setDisplayed(displayed);
		emit galacticPolesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying galactic poles
bool GridLinesMgr::getFlagGalacticPoles() const
{
	return galacticPoles->isDisplayed();
}
Vec3f GridLinesMgr::getColorGalacticPoles() const
{
	return galacticPoles->getColor();
}
void GridLinesMgr::setColorGalacticPoles(const Vec3f& newColor)
{
	if(newColor != galacticPoles->getColor())
	{
		galacticPoles->setColor(newColor);
		emit galacticPolesColorChanged(newColor);
	}
}

//! Set flag for displaying galactic center and anticenter markers
void GridLinesMgr::setFlagGalacticCenter(const bool displayed)
{
	if(displayed != galacticCenter->isDisplayed())
	{
		galacticCenter->setDisplayed(displayed);
		emit galacticCenterDisplayedChanged(displayed);
	}
}
//! Get flag for displaying galactic center and anticenter markers
bool GridLinesMgr::getFlagGalacticCenter() const
{
	return galacticCenter->isDisplayed();
}
Vec3f GridLinesMgr::getColorGalacticCenter() const
{
	return galacticCenter->getColor();
}
void GridLinesMgr::setColorGalacticCenter(const Vec3f& newColor)
{
	if(newColor != galacticCenter->getColor())
	{
		galacticCenter->setColor(newColor);
		emit galacticCenterColorChanged(newColor);
	}
}

//! Set flag for displaying supergalactic poles
void GridLinesMgr::setFlagSupergalacticPoles(const bool displayed)
{
	if(displayed != supergalacticPoles->isDisplayed())
	{
		supergalacticPoles->setDisplayed(displayed);
		emit supergalacticPolesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying supergalactic poles
bool GridLinesMgr::getFlagSupergalacticPoles() const
{
	return supergalacticPoles->isDisplayed();
}
Vec3f GridLinesMgr::getColorSupergalacticPoles() const
{
	return supergalacticPoles->getColor();
}
void GridLinesMgr::setColorSupergalacticPoles(const Vec3f& newColor)
{
	if(newColor != supergalacticPoles->getColor())
	{
		supergalacticPoles->setColor(newColor);
		emit supergalacticPolesColorChanged(newColor);
	}
}

//! Set flag for displaying equinox points of J2000
void GridLinesMgr::setFlagEquinoxJ2000Points(const bool displayed)
{
	if(displayed != equinoxJ2000Points->isDisplayed())
	{
		equinoxJ2000Points->setDisplayed(displayed);
		emit equinoxJ2000PointsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying equinox points of J2000
bool GridLinesMgr::getFlagEquinoxJ2000Points() const
{
	return equinoxJ2000Points->isDisplayed();
}
Vec3f GridLinesMgr::getColorEquinoxJ2000Points() const
{
	return equinoxJ2000Points->getColor();
}
void GridLinesMgr::setColorEquinoxJ2000Points(const Vec3f& newColor)
{
	if(newColor != equinoxJ2000Points->getColor())
	{
		equinoxJ2000Points->setColor(newColor);
		emit equinoxJ2000PointsColorChanged(newColor);
	}
}

//! Set flag for displaying equinox points
void GridLinesMgr::setFlagEquinoxPoints(const bool displayed)
{
	if(displayed != equinoxPoints->isDisplayed())
	{
		equinoxPoints->setDisplayed(displayed);
		emit equinoxPointsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying equinox points
bool GridLinesMgr::getFlagEquinoxPoints() const
{
	return equinoxPoints->isDisplayed();
}
Vec3f GridLinesMgr::getColorEquinoxPoints() const
{
	return equinoxPoints->getColor();
}
void GridLinesMgr::setColorEquinoxPoints(const Vec3f& newColor)
{
	if(newColor != equinoxPoints->getColor())
	{
		equinoxPoints->setColor(newColor);
		emit equinoxPointsColorChanged(newColor);
	}
}

//! Set flag for displaying solstice points of J2000
void GridLinesMgr::setFlagSolsticeJ2000Points(const bool displayed)
{
	if(displayed != solsticeJ2000Points->isDisplayed())
	{
		solsticeJ2000Points->setDisplayed(displayed);
		emit solsticeJ2000PointsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying solstice points of J2000
bool GridLinesMgr::getFlagSolsticeJ2000Points() const
{
	return solsticeJ2000Points->isDisplayed();
}
Vec3f GridLinesMgr::getColorSolsticeJ2000Points() const
{
	return solsticeJ2000Points->getColor();
}
void GridLinesMgr::setColorSolsticeJ2000Points(const Vec3f& newColor)
{
	if(newColor != solsticeJ2000Points->getColor())
	{
		solsticeJ2000Points->setColor(newColor);
		emit solsticeJ2000PointsColorChanged(newColor);
	}
}

//! Set flag for displaying solstice points
void GridLinesMgr::setFlagSolsticePoints(const bool displayed)
{
	if(displayed != solsticePoints->isDisplayed())
	{
		solsticePoints->setDisplayed(displayed);
		emit solsticePointsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying solstice points
bool GridLinesMgr::getFlagSolsticePoints() const
{
	return solsticePoints->isDisplayed();
}
Vec3f GridLinesMgr::getColorSolsticePoints() const
{
	return solsticePoints->getColor();
}
void GridLinesMgr::setColorSolsticePoints(const Vec3f& newColor)
{
	if(newColor != solsticePoints->getColor())
	{
		solsticePoints->setColor(newColor);
		emit solsticePointsColorChanged(newColor);
	}
}

//! Set flag for displaying antisolar point
void GridLinesMgr::setFlagAntisolarPoint(const bool displayed)
{
	if(displayed != antisolarPoint->isDisplayed())
	{
		antisolarPoint->setDisplayed(displayed);
		emit antisolarPointDisplayedChanged(displayed);
	}
}
//! Get flag for displaying antisolar point
bool GridLinesMgr::getFlagAntisolarPoint() const
{
	return antisolarPoint->isDisplayed();
}
Vec3f GridLinesMgr::getColorAntisolarPoint() const
{
	return antisolarPoint->getColor();
}
void GridLinesMgr::setColorAntisolarPoint(const Vec3f& newColor)
{
	if(newColor != antisolarPoint->getColor())
	{
		antisolarPoint->setColor(newColor);
		emit antisolarPointColorChanged(newColor);
	}
}

//! Set flag for displaying vector point
void GridLinesMgr::setFlagApexPoints(const bool displayed)
{
	if(displayed != apexPoints->isDisplayed())
	{
		apexPoints->setDisplayed(displayed);
		emit apexPointsDisplayedChanged(displayed);
	}
}
//! Get flag for displaying vector point
bool GridLinesMgr::getFlagApexPoints() const
{
	return apexPoints->isDisplayed();
}
Vec3f GridLinesMgr::getColorApexPoints() const
{
	return apexPoints->getColor();
}
void GridLinesMgr::setColorApexPoints(const Vec3f& newColor)
{
	if(newColor != apexPoints->getColor())
	{
		apexPoints->setColor(newColor);
		emit apexPointsColorChanged(newColor);
	}
}

void GridLinesMgr::setLineThickness(const int thickness)
{
	int lineThickness = equGrid->getLineThickness();
	if (lineThickness!=thickness)
	{
		lineThickness=qBound(1, thickness, 5);
		// Grids
		equGrid->setLineThickness(lineThickness);
		equJ2000Grid->setLineThickness(lineThickness);
		galacticGrid->setLineThickness(lineThickness);
		supergalacticGrid->setLineThickness(lineThickness);
		eclGrid->setLineThickness(lineThickness);
		eclJ2000Grid->setLineThickness(lineThickness);
		aziGrid->setLineThickness(lineThickness);
		// Lines
		equatorLine->setLineThickness(lineThickness);
		equatorJ2000Line->setLineThickness(lineThickness);
		eclipticLine->setLineThickness(lineThickness);
		eclipticJ2000Line->setLineThickness(lineThickness);
		invariablePlaneLine->setLineThickness(lineThickness);
		solarEquatorLine->setLineThickness(lineThickness);
		precessionCircleN->setLineThickness(lineThickness);
		precessionCircleS->setLineThickness(lineThickness);
		meridianLine->setLineThickness(lineThickness);
		longitudeLine->setLineThickness(lineThickness);
		horizonLine->setLineThickness(lineThickness);
		galacticEquatorLine->setLineThickness(lineThickness);
		supergalacticEquatorLine->setLineThickness(lineThickness);
		primeVerticalLine->setLineThickness(lineThickness);
		currentVerticalLine->setLineThickness(lineThickness);
		colureLine_1->setLineThickness(lineThickness);
		colureLine_2->setLineThickness(lineThickness);
		circumpolarCircleN->setLineThickness(lineThickness);
		circumpolarCircleS->setLineThickness(lineThickness);
		umbraCircle->setLineThickness(lineThickness);
		penumbraCircle->setLineThickness(lineThickness);

		emit lineThicknessChanged(lineThickness);
	}
}

 int GridLinesMgr::getLineThickness() const
{
	return equGrid->getLineThickness();
}

 void GridLinesMgr::setPartThickness(const int thickness)
 {
	 int partThickness = equatorLine->getPartThickness();
	 if (partThickness!=thickness)
	 {
		 partThickness=qBound(1, thickness, 5);
		 // Lines
		 equatorLine->setPartThickness(partThickness);
		 equatorJ2000Line->setPartThickness(partThickness);
		 eclipticLine->setPartThickness(partThickness);
		 eclipticJ2000Line->setPartThickness(partThickness);
		 //invariablePlaneLine->setPartThickness(partThickness);
		 solarEquatorLine->setPartThickness(partThickness);
		 precessionCircleN->setPartThickness(partThickness);
		 precessionCircleS->setPartThickness(partThickness);
		 meridianLine->setPartThickness(partThickness);
		 longitudeLine->setPartThickness(partThickness);
		 horizonLine->setPartThickness(partThickness);
		 galacticEquatorLine->setPartThickness(partThickness);
		 supergalacticEquatorLine->setPartThickness(partThickness);
		 primeVerticalLine->setPartThickness(partThickness);
		 currentVerticalLine->setPartThickness(partThickness);
		 colureLine_1->setPartThickness(partThickness);
		 colureLine_2->setPartThickness(partThickness);
		 //circumpolarCircleN->setPartThickness(partThickness);
		 //circumpolarCircleS->setPartThickness(partThickness);

		 emit partThicknessChanged(partThickness);
	 }
 }

  int GridLinesMgr::getPartThickness() const
 {
	 return equatorLine->getPartThickness();
 }

void GridLinesMgr::setFontSizeFromApp(int size)
{
	const int gridFontSize=size-1;
	const int lineFontSize=size+1;
	const int pointFontSize=size+1;

	equGrid->setFontSize(gridFontSize);
	equJ2000Grid->setFontSize(gridFontSize);
	galacticGrid->setFontSize(gridFontSize);
	supergalacticGrid->setFontSize(gridFontSize);
	eclGrid->setFontSize(gridFontSize);
	eclJ2000Grid->setFontSize(gridFontSize);
	aziGrid->setFontSize(gridFontSize);
	equatorLine->setFontSize(lineFontSize);
	equatorJ2000Line->setFontSize(lineFontSize);
	eclipticLine->setFontSize(lineFontSize);
	eclipticJ2000Line->setFontSize(lineFontSize);
	invariablePlaneLine->setFontSize(lineFontSize);
	solarEquatorLine->setFontSize(lineFontSize);
	precessionCircleN->setFontSize(lineFontSize);
	precessionCircleS->setFontSize(lineFontSize);
	meridianLine->setFontSize(lineFontSize);
	longitudeLine->setFontSize(lineFontSize);
	horizonLine->setFontSize(lineFontSize);
	galacticEquatorLine->setFontSize(lineFontSize);
	supergalacticEquatorLine->setFontSize(lineFontSize);
	primeVerticalLine->setFontSize(lineFontSize);
	currentVerticalLine->setFontSize(lineFontSize);
	colureLine_1->setFontSize(lineFontSize);
	colureLine_2->setFontSize(lineFontSize);
	circumpolarCircleN->setFontSize(lineFontSize);
	circumpolarCircleS->setFontSize(lineFontSize);
	umbraCircle->setFontSize(lineFontSize);
	penumbraCircle->setFontSize(lineFontSize);
	celestialJ2000Poles->setFontSize(pointFontSize);
	celestialPoles->setFontSize(pointFontSize);
	zenithNadir->setFontSize(pointFontSize);
	eclipticJ2000Poles->setFontSize(pointFontSize);
	eclipticPoles->setFontSize(pointFontSize);
	galacticPoles->setFontSize(pointFontSize);
	galacticCenter->setFontSize(pointFontSize);
	supergalacticPoles->setFontSize(pointFontSize);
	equinoxJ2000Points->setFontSize(pointFontSize);
	equinoxPoints->setFontSize(pointFontSize);
	solsticeJ2000Points->setFontSize(pointFontSize);
	solsticePoints->setFontSize(pointFontSize);
	apexPoints->setFontSize(pointFontSize);	
}
