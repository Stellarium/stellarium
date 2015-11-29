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
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelSkyDrawer.hpp"
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
	void setFontSize(double newFontSize);
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void setFadeDuration(float duration) {fader.setDuration((int)(duration*1000.f));}
	void setDisplayed(const bool displayed){fader = displayed;}
	bool isDisplayed(void) const {return fader;}
private:
	Vec3f color;
	StelCore::FrameType frameType;
	QFont font;
	LinearFader fader;
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
		LONGITUDE,
		PRIME_VERTICAL,
		COLURE_1,
		COLURE_2
	};
	// Create and precompute positions of a SkyGrid
	SkyLine(SKY_LINE_TYPE _line_type = EQUATOR_J2000);
	virtual ~SkyLine();
	void draw(StelCore* core) const;
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void setFadeDuration(float duration) {fader.setDuration((int)(duration*1000.f));}
	void setDisplayed(const bool displayed){fader = displayed;}
	bool isDisplayed(void) const {return fader;}
	void setFontSize(double newSize);
	//! Re-translates the label.
	void updateLabel();
private:
	SKY_LINE_TYPE line_type;
	Vec3f color;
	StelCore::FrameType frameType;
	LinearFader fader;
	QFont font;
	QString label;
};

// rms added color as parameter
SkyGrid::SkyGrid(StelCore::FrameType frame) : color(0.2,0.2,0.2), frameType(frame)
{
	// Font size is 12
	font.setPixelSize(StelApp::getInstance().getBaseFontSize()-1);
}

SkyGrid::~SkyGrid()
{
}

void SkyGrid::setFontSize(double newFontSize)
{
	font.setPixelSize(newFontSize);
}

// Conversion into mas = milli arcsecond
static const double RADIAN_MAS = 180./M_PI*1000.*60.*60.;
static const double DEGREE_MAS = 1000.*60.*60.;

// Step sizes in arcsec
static const double STEP_SIZES_DMS[] = {0.05, 0.2, 1., 5., 10., 60., 300., 600., 1200., 3600., 3600.*5., 3600.*10.};
static const double STEP_SIZES_HMS[] = {0.05, 0.2, 1.5, 7.5, 15., 15.*5., 15.*10., 15.*60., 15.*60.*5., 15.*60*10., 15.*60*60};

//! Return the angular grid step in degree which best fits the given scale
static double getClosestResolutionDMS(double pixelPerRad)
{
	double minResolution = 80.;
	double minSizeArcsec = minResolution/pixelPerRad*180./M_PI*3600;
	for (unsigned int i=0;i<12;++i)
		if (STEP_SIZES_DMS[i]>minSizeArcsec)
		{
			return STEP_SIZES_DMS[i]/3600.;
		}
	return 10.;
}

//! Return the angular grid step in degree which best fits the given scale
static double getClosestResolutionHMS(double pixelPerRad)
{
	double minResolution = 80.;
	double minSizeArcsec = minResolution/pixelPerRad*180./M_PI*3600;
	for (unsigned int i=0;i<11;++i)
		if (STEP_SIZES_HMS[i]>minSizeArcsec)
		{
			return STEP_SIZES_HMS[i]/3600.;
		}
	return 15.;
}

struct ViewportEdgeIntersectCallbackData
{
	ViewportEdgeIntersectCallbackData(StelPainter* p)
		: sPainter(p)
		, raAngle(0.0)
		, frameType(StelCore::FrameUninitialized) {;}
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
	Vec3d direc(direction);
	direc.normalize();
	const Vec4f tmpColor = d->sPainter->getColor();
	d->sPainter->setColor(d->textColor[0], d->textColor[1], d->textColor[2], d->textColor[3]);
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	bool useOldAzimuth = StelApp::getInstance().getFlagOldAzimuthUsage();

	QString text;
	if (d->text.isEmpty())
	{
		// We are in the case of meridians, we need to determine which of the 2 labels (3h or 15h to use)
		Vec3d tmpV;
		d->sPainter->getProjector()->unProject(screenPos, tmpV);
		double lon, lat, textAngle;
		StelUtils::rectToSphe(&lon, &lat, tmpV);
		switch (d->frameType)
		{
			case StelCore::FrameAltAz:
			{
				double raAngle = ::fmod(M_PI-d->raAngle,2.*M_PI);
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
				double raAngle = d->raAngle;
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
			case StelCore::FrameGalactic:
			{
				double raAngle = M_PI-d->raAngle;
				lon = M_PI-lon;

				if (raAngle<0)
					raAngle=+2.*M_PI;

				if (lon<0)
					lon=+2.*M_PI;

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
					text = StelUtils::radToDmsStrAdapt(textAngle);
				break;
			}
			default:			
			{
				if (std::fabs(2.*M_PI-lon)<0.001)
				{
					// We are at meridian 0
					lon = 0.;
				}
				const double delta = d->raAngle<M_PI ? M_PI : -M_PI;
				if (std::fabs(lon-d->raAngle) < 1. || lon==0. || d->raAngle==M_PI)
					textAngle = d->raAngle;
				else
					textAngle = d->raAngle+delta;

				if (d->raAngle+delta==0.)
					textAngle = M_PI;

				if (withDecimalDegree)
					text = StelUtils::radToDecDegStr(textAngle, 4, false, true);
				else
					text = StelUtils::radToHmsStrAdapt(textAngle);
			}
		}
	}
	else
		text = d->text;

	double angleDeg = std::atan2(-direc[1], -direc[0])*180./M_PI;
	float xshift=6.f;
	if (angleDeg>90. || angleDeg<-90.)
	{
		angleDeg+=180.;
		xshift=-d->sPainter->getFontMetrics().width(text)-6.f;
	}

	d->sPainter->drawText(screenPos[0], screenPos[1], text, angleDeg, xshift, 3);
	d->sPainter->setColor(tmpColor[0], tmpColor[1], tmpColor[2], tmpColor[3]);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//! Draw the sky grid in the current frame
void SkyGrid::draw(const StelCore* core) const
{
	const StelProjectorP prj = core->getProjection(frameType, frameType!=StelCore::FrameAltAz ? StelCore::RefractionAuto : StelCore::RefractionOff);
	if (!fader.getInterstate())
		return;

	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();;

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

	const double gridStepParallelRad = M_PI/180.*getClosestResolutionDMS(prj->getPixelPerRadAtCenter());
	double gridStepMeridianRad;
	if (northPoleInViewport || southPoleInViewport)
		gridStepMeridianRad = (frameType==StelCore::FrameAltAz || frameType==StelCore::FrameGalactic) ? M_PI/180.* 10. : M_PI/180.* 15.;
	else
	{
		const double closestResLon = (frameType==StelCore::FrameAltAz || frameType==StelCore::FrameGalactic) ? getClosestResolutionDMS(prj->getPixelPerRadAtCenter()*std::cos(lat2)) : getClosestResolutionHMS(prj->getPixelPerRadAtCenter()*std::cos(lat2));
		gridStepMeridianRad = M_PI/180.* ((northPoleInViewport || southPoleInViewport) ? 15. : closestResLon);
	}

	// Get the bounding halfspace
	const SphericalCap& viewPortSphericalCap = prj->getBoundingCap();

	// Compute the first grid starting point. This point is close to the center of the screen
	// and lies at the intersection of a meridian and a parallel
	lon2 = gridStepMeridianRad*((int)(lon2/gridStepMeridianRad+0.5));
	lat2 = gridStepParallelRad*((int)(lat2/gridStepParallelRad+0.5));
	Vec3d firstPoint;
	StelUtils::spheToRect(lon2, lat2, firstPoint);
	firstPoint.normalize();

	// Q_ASSERT(viewPortSphericalCap.contains(firstPoint));

	// Initialize a painter and set OpenGL state
	StelPainter sPainter(prj);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	// OpenGL ES 2.0 doesn't have GL_LINE_SMOOTH
	#ifdef GL_LINE_SMOOTH
	if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
		glEnable(GL_LINE_SMOOTH);
	#endif

	// make text colors just a bit brighter. (But if >1, QColor::setRgb fails and makes text invisible.)
	Vec4f textColor(qMin(1.0f, 1.25f*color[0]), qMin(1.0f, 1.25f*color[1]), qMin(1.0f, 1.25f*color[2]), fader.getInterstate());
	sPainter.setColor(color[0],color[1],color[2], fader.getInterstate());

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
	int maxNbIter = (int)(M_PI/gridStepMeridianRad);
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
				sPainter.drawGreatCircleArc(fpt, rotFpt, NULL, viewportEdgeIntersectCallback, &userData);
				sPainter.drawGreatCircleArc(rotFpt, rotFpt2, NULL, viewportEdgeIntersectCallback, &userData);
				sPainter.drawGreatCircleArc(rotFpt2, fpt, NULL, viewportEdgeIntersectCallback, &userData);
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
		sPainter.drawGreatCircleArc(p1, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);
		sPainter.drawGreatCircleArc(p2, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);

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

			sPainter.drawGreatCircleArc(p1, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);
			sPainter.drawGreatCircleArc(p2, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);

			fpt.transfo4d(rotLon);
		}
	}

	/////////////////////////////////////////////////
	// Draw all the parallels (small circles)
	SphericalCap parallelSphericalCap(Vec3d(0,0,1), 0);
	rotLon = Mat4d::rotation(firstPoint^Vec3d(0,0,1), gridStepParallelRad);
	fpt = firstPoint;
	maxNbIter = (int)(M_PI/gridStepParallelRad)-1;
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
	// OpenGL ES 2.0 doesn't have GL_LINE_SMOOTH
	#ifdef GL_LINE_SMOOTH
	if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
		glDisable(GL_LINE_SMOOTH);
	#endif
}


SkyLine::SkyLine(SKY_LINE_TYPE _line_type) : color(0.f, 0.f, 1.f)
{
	// Font size is 14
	font.setPixelSize(StelApp::getInstance().getBaseFontSize()+1);
	line_type = _line_type;

	updateLabel();
}

SkyLine::~SkyLine()
{
}

void SkyLine::setFontSize(double newFontSize)
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
		case LONGITUDE:
			frameType = StelCore::FrameObservercentricEclipticOfDate; // For 0.14.2: Switch from Ecl2000 to FrameObservercentriEclipticOfDate
			// TRANSLATORS: Full term is "opposition/conjunction longitude"
			label = q_("O./C. longitude");
			break;
		case PRIME_VERTICAL:
			frameType=StelCore::FrameAltAz;
			label = q_("Prime Vertical");
			break;
		case COLURE_1:
			frameType=StelCore::FrameEquinoxEqu;
			label = q_("Equinoctial Colure");
			break;
		case COLURE_2:
			frameType=StelCore::FrameEquinoxEqu;
			label = q_("Solstitial Colure");
			break;
		default:
			Q_ASSERT(0);
	}
}

void SkyLine::draw(StelCore *core) const
{
	if (!fader.getInterstate())
		return;

	StelProjectorP prj = core->getProjection(frameType, frameType!=StelCore::FrameAltAz ? StelCore::RefractionAuto : StelCore::RefractionOff);

	// Get the bounding halfspace
	const SphericalCap& viewPortSphericalCap = prj->getBoundingCap();

	// Initialize a painter and set openGL state
	StelPainter sPainter(prj);
	sPainter.setColor(color[0], color[1], color[2], fader.getInterstate());
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	#ifdef GL_LINE_SMOOTH
	if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
		glEnable(GL_LINE_SMOOTH);
	#endif
	Vec4f textColor(color[0], color[1], color[2], 0);		
	textColor[3]=fader.getInterstate();

	ViewportEdgeIntersectCallbackData userData(&sPainter);	
	sPainter.setFont(font);
	userData.textColor = textColor;	
	userData.text = label;
	/////////////////////////////////////////////////
	// Draw the line

	// Precession circles are Small Circles, all others are Great Circles.
	if (line_type==PRECESSIONCIRCLE_N || line_type==PRECESSIONCIRCLE_S)
	{
		const double lat=(line_type==PRECESSIONCIRCLE_S ? -1.0 : 1.0) * (M_PI/2.0-getPrecessionAngleVondrakCurrentEpsilonA());
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
				const double lon2=120.0*M_PI/180.0;
				const double lon3=240.0*M_PI/180.0;
				StelUtils::spheToRect(lon1, lat, pt1); pt1.normalize();
				StelUtils::spheToRect(lon2, lat, pt2); pt2.normalize();
				StelUtils::spheToRect(lon3, lat, pt3); pt3.normalize();

				sPainter.drawSmallCircleArc(pt1, pt2, rotCenter, viewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(pt2, pt3, rotCenter, viewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(pt3, pt1, rotCenter, viewportEdgeIntersectCallback, &userData);
				#ifdef GL_LINE_SMOOTH
				if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
					glDisable(GL_LINE_SMOOTH);
				#endif
				glDisable(GL_BLEND);
				return;
			}
			else
			{
				#ifdef GL_LINE_SMOOTH
				if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
					glDisable(GL_LINE_SMOOTH);
				#endif
				glDisable(GL_BLEND);
				return;
			}
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

		sPainter.drawSmallCircleArc(p1, middlePoint, rotCenter,viewportEdgeIntersectCallback, &userData);
		sPainter.drawSmallCircleArc(p2, middlePoint, rotCenter, viewportEdgeIntersectCallback, &userData);

		// OpenGL ES 2.0 doesn't have GL_LINE_SMOOTH
		#ifdef GL_LINE_SMOOTH
		if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
			glDisable(GL_LINE_SMOOTH);
		#endif

		glDisable(GL_BLEND);


		return;
	}

	// All the other "lines" are Great Circles
	SphericalCap meridianSphericalCap(Vec3d(0,0,1), 0);	
	Vec3d fpt(1,0,0);
	if ((line_type==MERIDIAN) || (line_type==COLURE_1))
	{
		meridianSphericalCap.n.set(0,1,0);
	}
	if ((line_type==PRIME_VERTICAL) || (line_type==COLURE_2))
	{
		meridianSphericalCap.n.set(1,0,0);
		fpt.set(0,0,1);
	}

	if (line_type==LONGITUDE)
	{
		Vec3d coord;
		double lambda, beta;
		StelUtils::rectToSphe(&lambda, &beta, core->getCurrentPlanet()->getHeliocentricEclipticPos());
		StelUtils::spheToRect(lambda + M_PI/2., 0., coord);
		meridianSphericalCap.n.set(coord[0],coord[1],coord[2]);
		fpt.set(0,0,1);
	}

	Vec3d p1, p2;
	if (!SphericalCap::intersectionPoints(viewPortSphericalCap, meridianSphericalCap, p1, p2))
	{
		if ((viewPortSphericalCap.d<meridianSphericalCap.d && viewPortSphericalCap.contains(meridianSphericalCap.n))
			|| (viewPortSphericalCap.d<-meridianSphericalCap.d && viewPortSphericalCap.contains(-meridianSphericalCap.n)))
		{
			// The meridian is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
			const Mat4d& rotLon120 = Mat4d::rotation(meridianSphericalCap.n, 120.*M_PI/180.);
			Vec3d rotFpt=fpt;
			rotFpt.transfo4d(rotLon120);
			Vec3d rotFpt2=rotFpt;
			rotFpt2.transfo4d(rotLon120);
			sPainter.drawGreatCircleArc(fpt, rotFpt, NULL, viewportEdgeIntersectCallback, &userData);
			sPainter.drawGreatCircleArc(rotFpt, rotFpt2, NULL, viewportEdgeIntersectCallback, &userData);
			sPainter.drawGreatCircleArc(rotFpt2, fpt, NULL, viewportEdgeIntersectCallback, &userData);
			return;
		}
		else
			return;
	}


	Vec3d middlePoint = p1+p2;
	middlePoint.normalize();
	if (!viewPortSphericalCap.contains(middlePoint))
		middlePoint*=-1.;

	// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
	sPainter.drawGreatCircleArc(p1, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);
	sPainter.drawGreatCircleArc(p2, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);

	// OpenGL ES 2.0 doesn't have GL_LINE_SMOOTH
	#ifdef GL_LINE_SMOOTH
	if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
		glDisable(GL_LINE_SMOOTH);
	#endif

	glDisable(GL_BLEND);

// 	// Johannes: use a big radius as a dirty workaround for the bug that the
// 	// ecliptic line is not drawn around the observer, but around the sun:
// 	const Vec3d vv(1000000,0,0);

}

GridLinesMgr::GridLinesMgr()
{
	setObjectName("GridLinesMgr");
	equGrid = new SkyGrid(StelCore::FrameEquinoxEqu);
	equJ2000Grid = new SkyGrid(StelCore::FrameJ2000);
	eclJ2000Grid = new SkyGrid(StelCore::FrameObservercentricEclipticJ2000);
	eclGrid = new SkyGrid(StelCore::FrameObservercentricEclipticOfDate);
	galacticGrid = new SkyGrid(StelCore::FrameGalactic);
	aziGrid = new SkyGrid(StelCore::FrameAltAz);
	equatorLine = new SkyLine(SkyLine::EQUATOR_OF_DATE);
	equatorJ2000Line = new SkyLine(SkyLine::EQUATOR_J2000);
	eclipticJ2000Line = new SkyLine(SkyLine::ECLIPTIC_J2000); // previous eclipticLine
	eclipticLine = new SkyLine(SkyLine::ECLIPTIC_OF_DATE); // GZ NEW
	precessionCircleN = new SkyLine(SkyLine::PRECESSIONCIRCLE_N);
	precessionCircleS = new SkyLine(SkyLine::PRECESSIONCIRCLE_S);
	meridianLine = new SkyLine(SkyLine::MERIDIAN);
	horizonLine = new SkyLine(SkyLine::HORIZON);
	galacticEquatorLine = new SkyLine(SkyLine::GALACTICEQUATOR);
	longitudeLine = new SkyLine(SkyLine::LONGITUDE);
	primeVerticalLine = new SkyLine(SkyLine::PRIME_VERTICAL);
	colureLine_1 = new SkyLine(SkyLine::COLURE_1);
	colureLine_2 = new SkyLine(SkyLine::COLURE_2);
}

GridLinesMgr::~GridLinesMgr()
{
	delete equGrid;
	delete equJ2000Grid;
	delete eclJ2000Grid;
	delete eclGrid;
	delete galacticGrid;
	delete aziGrid;
	delete equatorLine;
	delete equatorJ2000Line;
	delete eclipticLine;
	delete eclipticJ2000Line;
	delete precessionCircleN;
	delete precessionCircleS;
	delete meridianLine;
	delete horizonLine;
	delete galacticEquatorLine;
	delete longitudeLine;
	delete primeVerticalLine;
	delete colureLine_1;
	delete colureLine_2;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double GridLinesMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10;
	return 0;
}

void GridLinesMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	setFlagAzimuthalGrid(conf->value("viewing/flag_azimuthal_grid").toBool());
	setFlagEquatorGrid(conf->value("viewing/flag_equatorial_grid").toBool());
	setFlagEquatorJ2000Grid(conf->value("viewing/flag_equatorial_J2000_grid").toBool());
	setFlagEclipticJ2000Grid(conf->value("viewing/flag_ecliptic_J2000_grid").toBool());
	setFlagEclipticGrid(conf->value("viewing/flag_ecliptic_grid").toBool());
	setFlagGalacticGrid(conf->value("viewing/flag_galactic_grid").toBool());
	setFlagEquatorLine(conf->value("viewing/flag_equator_line").toBool());
	setFlagEquatorJ2000Line(conf->value("viewing/flag_equator_J2000_line").toBool());
	setFlagEclipticLine(conf->value("viewing/flag_ecliptic_line").toBool());
	setFlagEclipticJ2000Line(conf->value("viewing/flag_ecliptic_J2000_line").toBool());
	setFlagPrecessionCircles(conf->value("viewing/flag_precession_circles").toBool());
	setFlagMeridianLine(conf->value("viewing/flag_meridian_line").toBool());
	setFlagHorizonLine(conf->value("viewing/flag_horizon_line").toBool());
	setFlagGalacticEquatorLine(conf->value("viewing/flag_galactic_equator_line").toBool());
	setFlagLongitudeLine(conf->value("viewing/flag_longitude_line").toBool());
	setFlagPrimeVerticalLine(conf->value("viewing/flag_prime_vertical_line").toBool());
	setFlagColureLines(conf->value("viewing/flag_colure_lines").toBool());
	
	StelApp& app = StelApp::getInstance();
	connect(&app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateLineLabels()));
	
	QString displayGroup = N_("Display Options");
	addAction("actionShow_Equatorial_Grid", displayGroup, N_("Equatorial grid"), "equatorGridDisplayed", "E");
	addAction("actionShow_Azimuthal_Grid", displayGroup, N_("Azimuthal grid"), "azimuthalGridDisplayed", "Z");
	addAction("actionShow_Ecliptic_Line", displayGroup, N_("Ecliptic line"), "eclipticLineDisplayed", ",");
	addAction("actionShow_Ecliptic_J2000_Line", displayGroup, N_("Ecliptic J2000 line"), "eclipticJ2000LineDisplayed");
	addAction("actionShow_Equator_Line", displayGroup, N_("Equator line"), "equatorLineDisplayed", ".");
	addAction("actionShow_Equator_J2000_Line", displayGroup, N_("Equator J2000 line"), "equatorJ2000LineDisplayed"); // or with Hotkey??
	addAction("actionShow_Meridian_Line", displayGroup, N_("Meridian line"), "meridianLineDisplayed", ";");
	addAction("actionShow_Horizon_Line", displayGroup, N_("Horizon line"), "horizonLineDisplayed", "H");
	addAction("actionShow_Equatorial_J2000_Grid", displayGroup, N_("Equatorial J2000 grid"), "equatorJ2000GridDisplayed");
	addAction("actionShow_Ecliptic_J2000_Grid", displayGroup, N_("Ecliptic J2000 grid"), "eclipticJ2000GridDisplayed");
	addAction("actionShow_Ecliptic_Grid", displayGroup, N_("Ecliptic grid"), "eclipticGridDisplayed");
	addAction("actionShow_Galactic_Grid", displayGroup, N_("Galactic grid"), "galacticGridDisplayed");
	addAction("actionShow_Galactic_Equator_Line", displayGroup, N_("Galactic equator"), "galacticEquatorLineDisplayed");
	addAction("actionShow_Longitude_Line", displayGroup, N_("Opposition/conjunction longitude line"), "longitudeLineDisplayed");
	addAction("actionShow_Precession_Circles", displayGroup, N_("Precession Circles"), "precessionCirclesDisplayed");
	addAction("actionShow_Prime_Vertical_Line", displayGroup, N_("Prime Vertical"), "primeVerticalLineDisplayed");
	addAction("actionShow_Colure_Lines", displayGroup, N_("Colure Lines"), "colureLinesDisplayed");
}

void GridLinesMgr::update(double deltaTime)
{
	// Update faders
	equGrid->update(deltaTime);
	equJ2000Grid->update(deltaTime);
	eclJ2000Grid->update(deltaTime);
	eclGrid->update(deltaTime);
	galacticGrid->update(deltaTime);
	aziGrid->update(deltaTime);
	equatorLine->update(deltaTime);
	equatorJ2000Line->update(deltaTime);
	eclipticLine->update(deltaTime);
	eclipticJ2000Line->update(deltaTime);
	precessionCircleN->update(deltaTime);
	precessionCircleS->update(deltaTime);
	meridianLine->update(deltaTime);
	horizonLine->update(deltaTime);
	galacticEquatorLine->update(deltaTime);
	longitudeLine->update(deltaTime);
	primeVerticalLine->update(deltaTime);
	colureLine_1->update(deltaTime);
	colureLine_2->update(deltaTime);
}

void GridLinesMgr::draw(StelCore* core)
{
	galacticGrid->draw(core);
	eclJ2000Grid->draw(core);
	// While ecliptic of J2000 may be helpful to get a feeling of the Z=0 plane of VSOP87,
	// ecliptic of date is related to Earth and does not make much sense for the other planets.
	// Of course, orbital plane of respective planet would be better, but is not implemented.
	if (core->getCurrentPlanet()->getEnglishName()=="Earth")
		eclGrid->draw(core);
	equJ2000Grid->draw(core);
	equGrid->draw(core);
	aziGrid->draw(core);
	// Lines after grids, to be able to e.g. draw equators in different color!
	galacticEquatorLine->draw(core);
	eclipticJ2000Line->draw(core);
	if (core->getCurrentPlanet()->getEnglishName()=="Earth")
	{
		eclipticLine->draw(core);
		precessionCircleN->draw(core);
		precessionCircleS->draw(core);
		colureLine_1->draw(core);
		colureLine_2->draw(core);
	}
	longitudeLine->draw(core);
	equatorJ2000Line->draw(core);
	equatorLine->draw(core);
	meridianLine->draw(core);
	horizonLine->draw(core);
	primeVerticalLine->draw(core);
}

void GridLinesMgr::setStelStyle(const QString& section)
{
	QSettings* conf = StelApp::getInstance().getSettings();

	// Load colors from config file
	QString defaultColor = conf->value(section+"/default_color").toString();
	setColorEquatorGrid(StelUtils::strToVec3f(conf->value(section+"/equatorial_color", defaultColor).toString()));
	setColorEquatorJ2000Grid(StelUtils::strToVec3f(conf->value(section+"/equatorial_J2000_color", defaultColor).toString()));
	setColorEclipticJ2000Grid(StelUtils::strToVec3f(conf->value(section+"/ecliptical_J2000_color", defaultColor).toString()));
	setColorEclipticGrid(StelUtils::strToVec3f(conf->value(section+"/ecliptical_color", defaultColor).toString()));
	setColorGalacticGrid(StelUtils::strToVec3f(conf->value(section+"/galactic_color", defaultColor).toString()));
	setColorAzimuthalGrid(StelUtils::strToVec3f(conf->value(section+"/azimuthal_color", defaultColor).toString()));
	setColorEquatorLine(StelUtils::strToVec3f(conf->value(section+"/equator_color", defaultColor).toString()));
	setColorEclipticLine(StelUtils::strToVec3f(conf->value(section+"/ecliptic_color", defaultColor).toString()));
	setColorEclipticJ2000Line(StelUtils::strToVec3f(conf->value(section+"/ecliptic_J2000_color", defaultColor).toString()));
	setColorPrecessionCircles(StelUtils::strToVec3f(conf->value(section+"/precession_circles_color", defaultColor).toString()));
	setColorMeridianLine(StelUtils::strToVec3f(conf->value(section+"/meridian_color", defaultColor).toString()));
	setColorHorizonLine(StelUtils::strToVec3f(conf->value(section+"/horizon_color", defaultColor).toString()));
	setColorGalacticEquatorLine(StelUtils::strToVec3f(conf->value(section+"/galactic_equator_color", defaultColor).toString()));
	setColorLongitudeLine(StelUtils::strToVec3f(conf->value(section+"/longitude_color", defaultColor).toString()));
	setColorPrimeVerticalLine(StelUtils::strToVec3f(conf->value(section+"/prime_vertical_color", defaultColor).toString()));
	setColorColureLines(StelUtils::strToVec3f(conf->value(section+"/colures_color", defaultColor).toString()));
}

void GridLinesMgr::updateLineLabels()
{
	equatorJ2000Line->updateLabel();
	equatorLine->updateLabel();
	eclipticLine->updateLabel();
	eclipticJ2000Line->updateLabel();
	precessionCircleN->updateLabel();
	precessionCircleS->updateLabel();
	meridianLine->updateLabel();
	horizonLine->updateLabel();
	galacticEquatorLine->updateLabel();
	longitudeLine->updateLabel();
	primeVerticalLine->updateLabel();
	colureLine_1->updateLabel();
	colureLine_2->updateLabel();
}

//! Set flag for displaying Azimuthal Grid
void GridLinesMgr::setFlagAzimuthalGrid(const bool displayed)
{
	if(displayed != aziGrid->isDisplayed()) {
		aziGrid->setDisplayed(displayed);
		emit azimuthalGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Azimuthal Grid
bool GridLinesMgr::getFlagAzimuthalGrid(void) const
{
	return aziGrid->isDisplayed();
}
Vec3f GridLinesMgr::getColorAzimuthalGrid(void) const
{
	return aziGrid->getColor();
}
void GridLinesMgr::setColorAzimuthalGrid(const Vec3f& newColor)
{
	if(newColor != aziGrid->getColor()) {
		aziGrid->setColor(newColor);
		emit azimuthalGridColorChanged(newColor);
	}
}

//! Set flag for displaying Equatorial Grid
void GridLinesMgr::setFlagEquatorGrid(const bool displayed)
{
	if(displayed != equGrid->isDisplayed()) {
		equGrid->setDisplayed(displayed);
		emit equatorGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial Grid
bool GridLinesMgr::getFlagEquatorGrid(void) const
{
	return equGrid->isDisplayed();
}
Vec3f GridLinesMgr::getColorEquatorGrid(void) const
{
	return equGrid->getColor();
}
void GridLinesMgr::setColorEquatorGrid(const Vec3f& newColor)
{
	if(newColor != equGrid->getColor()) {
		equGrid->setColor(newColor);
		emit equatorGridColorChanged(newColor);
	}
}

//! Set flag for displaying Equatorial J2000 Grid
void GridLinesMgr::setFlagEquatorJ2000Grid(const bool displayed)
{
	if(displayed != equJ2000Grid->isDisplayed()) {
		equJ2000Grid->setDisplayed(displayed);
		emit equatorJ2000GridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial J2000 Grid
bool GridLinesMgr::getFlagEquatorJ2000Grid(void) const
{
	return equJ2000Grid->isDisplayed();
}
Vec3f GridLinesMgr::getColorEquatorJ2000Grid(void) const
{
	return equJ2000Grid->getColor();
}
void GridLinesMgr::setColorEquatorJ2000Grid(const Vec3f& newColor)
{
	if(newColor != equJ2000Grid->getColor()) {
		equJ2000Grid->setColor(newColor);
		emit equatorJ2000GridColorChanged(newColor);
	}
}

//! Set flag for displaying Ecliptic J2000 Grid
void GridLinesMgr::setFlagEclipticJ2000Grid(const bool displayed)
{
	if(displayed != eclJ2000Grid->isDisplayed()) {
		eclJ2000Grid->setDisplayed(displayed);
		emit eclipticJ2000GridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic J2000 Grid
bool GridLinesMgr::getFlagEclipticJ2000Grid(void) const
{
	return eclJ2000Grid->isDisplayed();
}
Vec3f GridLinesMgr::getColorEclipticJ2000Grid(void) const
{
	return eclJ2000Grid->getColor();
}
void GridLinesMgr::setColorEclipticJ2000Grid(const Vec3f& newColor)
{
	if(newColor != eclJ2000Grid->getColor()) {
		eclJ2000Grid->setColor(newColor);
		emit eclipticJ2000GridColorChanged(newColor);
	}
}

//! Set flag for displaying Ecliptic of Date Grid
void GridLinesMgr::setFlagEclipticGrid(const bool displayed)
{
	if(displayed != eclGrid->isDisplayed()) {
		eclGrid->setDisplayed(displayed);
		emit eclipticGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic of Date Grid
bool GridLinesMgr::getFlagEclipticGrid(void) const
{
	return eclGrid->isDisplayed();
}
Vec3f GridLinesMgr::getColorEclipticGrid(void) const
{
	return eclGrid->getColor();
}
void GridLinesMgr::setColorEclipticGrid(const Vec3f& newColor)
{
	if(newColor != eclGrid->getColor()) {
		eclGrid->setColor(newColor);
		emit eclipticGridColorChanged(newColor);
	}
}

//! Set flag for displaying Galactic Grid
void GridLinesMgr::setFlagGalacticGrid(const bool displayed)
{
	if(displayed != galacticGrid->isDisplayed()) {
		galacticGrid->setDisplayed(displayed);
		emit galacticGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Galactic Grid
bool GridLinesMgr::getFlagGalacticGrid(void) const
{
	return galacticGrid->isDisplayed();
}
Vec3f GridLinesMgr::getColorGalacticGrid(void) const
{
	return galacticGrid->getColor();
}
void GridLinesMgr::setColorGalacticGrid(const Vec3f& newColor)
{
	if(newColor != galacticGrid->getColor()) {
		galacticGrid->setColor(newColor);
		emit galacticGridColorChanged(newColor);
	}
}

//! Set flag for displaying Equatorial Line
void GridLinesMgr::setFlagEquatorLine(const bool displayed)
{
	if(displayed != equatorLine->isDisplayed()) {
		equatorLine->setDisplayed(displayed);
		emit equatorLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial Line
bool GridLinesMgr::getFlagEquatorLine(void) const
{
	return equatorLine->isDisplayed();
}
Vec3f GridLinesMgr::getColorEquatorLine(void) const
{
	return equatorLine->getColor();
}
void GridLinesMgr::setColorEquatorLine(const Vec3f& newColor)
{
	if(newColor != equatorLine->getColor()) {
		equatorLine->setColor(newColor);
		emit equatorLineColorChanged(newColor);
	}
}

//! Set flag for displaying J2000 Equatorial Line
void GridLinesMgr::setFlagEquatorJ2000Line(const bool displayed)
{
	if(displayed != equatorJ2000Line->isDisplayed()) {
		equatorJ2000Line->setDisplayed(displayed);
		emit equatorJ2000LineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying J2000 Equatorial Line
bool GridLinesMgr::getFlagEquatorJ2000Line(void) const
{
	return equatorJ2000Line->isDisplayed();
}
Vec3f GridLinesMgr::getColorEquatorJ2000Line(void) const
{
	return equatorJ2000Line->getColor();
}
void GridLinesMgr::setColorEquatorJ2000Line(const Vec3f& newColor)
{
	if(newColor != equatorJ2000Line->getColor()) {
		equatorJ2000Line->setColor(newColor);
		emit equatorJ2000LineColorChanged(newColor);
	}
}

//! Set flag for displaying Ecliptic Line
void GridLinesMgr::setFlagEclipticLine(const bool displayed)
{
	if(displayed != eclipticLine->isDisplayed()) {
		eclipticLine->setDisplayed(displayed);
		emit eclipticLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic Line
bool GridLinesMgr::getFlagEclipticLine(void) const
{
	return eclipticLine->isDisplayed();
}
Vec3f GridLinesMgr::getColorEclipticLine(void) const
{
	return eclipticLine->getColor();
}
void GridLinesMgr::setColorEclipticLine(const Vec3f& newColor)
{
	if(newColor != eclipticLine->getColor()) {
		eclipticLine->setColor(newColor);
		emit eclipticLineColorChanged(newColor);
	}
}

//! Set flag for displaying Ecliptic J2000 Line
void GridLinesMgr::setFlagEclipticJ2000Line(const bool displayed)
{
	if(displayed != eclipticJ2000Line->isDisplayed()) {
		eclipticJ2000Line->setDisplayed(displayed);
		emit eclipticJ2000LineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic J2000 Line
bool GridLinesMgr::getFlagEclipticJ2000Line(void) const
{
	return eclipticJ2000Line->isDisplayed();
}
Vec3f GridLinesMgr::getColorEclipticJ2000Line(void) const
{
	return eclipticJ2000Line->getColor();
}
void GridLinesMgr::setColorEclipticJ2000Line(const Vec3f& newColor)
{
	if(newColor != eclipticJ2000Line->getColor()) {
		eclipticJ2000Line->setColor(newColor);
		emit eclipticJ2000LineColorChanged(newColor);
	}
}

//! Set flag for displaying Precession Circles
void GridLinesMgr::setFlagPrecessionCircles(const bool displayed)
{
	if(displayed != precessionCircleN->isDisplayed()) {
		precessionCircleN->setDisplayed(displayed);
		precessionCircleS->setDisplayed(displayed);
		emit precessionCirclesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Precession Circles
bool GridLinesMgr::getFlagPrecessionCircles(void) const
{
	// precessionCircleS is always synchronous, no separate queries.
	return precessionCircleN->isDisplayed();
}
Vec3f GridLinesMgr::getColorPrecessionCircles(void) const
{
	return precessionCircleN->getColor();
}
void GridLinesMgr::setColorPrecessionCircles(const Vec3f& newColor)
{
	if(newColor != precessionCircleN->getColor()) {
		precessionCircleN->setColor(newColor);
		precessionCircleS->setColor(newColor);
		emit precessionCirclesColorChanged(newColor);
	}
}

//! Set flag for displaying Meridian Line
void GridLinesMgr::setFlagMeridianLine(const bool displayed)
{
	if(displayed != meridianLine->isDisplayed()) {
		meridianLine->setDisplayed(displayed);
		emit meridianLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Meridian Line
bool GridLinesMgr::getFlagMeridianLine(void) const
{
	return meridianLine->isDisplayed();
}
Vec3f GridLinesMgr::getColorMeridianLine(void) const
{
	return meridianLine->getColor();
}
void GridLinesMgr::setColorMeridianLine(const Vec3f& newColor)
{
	if(newColor != meridianLine->getColor()) {
		meridianLine->setColor(newColor);
		emit meridianLineColorChanged(newColor);
	}
}

//! Set flag for displaying opposition/conjunction longitude line
void GridLinesMgr::setFlagLongitudeLine(const bool displayed)
{
	if(displayed != longitudeLine->isDisplayed()) {
		longitudeLine->setDisplayed(displayed);
		emit longitudeLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying opposition/conjunction longitude line
bool GridLinesMgr::getFlagLongitudeLine(void) const
{
	return longitudeLine->isDisplayed();
}
Vec3f GridLinesMgr::getColorLongitudeLine(void) const
{
	return longitudeLine->getColor();
}
void GridLinesMgr::setColorLongitudeLine(const Vec3f& newColor)
{
	if(newColor != longitudeLine->getColor()) {
		longitudeLine->setColor(newColor);
		emit longitudeLineColorChanged(newColor);
	}
}

//! Set flag for displaying Horizon Line
void GridLinesMgr::setFlagHorizonLine(const bool displayed)
{
	if(displayed != horizonLine->isDisplayed()) {
		horizonLine->setDisplayed(displayed);
		emit horizonLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Horizon Line
bool GridLinesMgr::getFlagHorizonLine(void) const
{
	return horizonLine->isDisplayed();
}
Vec3f GridLinesMgr::getColorHorizonLine(void) const
{
	return horizonLine->getColor();
}
void GridLinesMgr::setColorHorizonLine(const Vec3f& newColor)
{
	if(newColor != horizonLine->getColor()) {
		horizonLine->setColor(newColor);
		emit horizonLineColorChanged(newColor);
	}
}

//! Set flag for displaying Galactic Equator Line
void GridLinesMgr::setFlagGalacticEquatorLine(const bool displayed)
{
	if(displayed != galacticEquatorLine->isDisplayed()) {
		galacticEquatorLine->setDisplayed(displayed);
		emit galacticEquatorLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Galactic Equator Line
bool GridLinesMgr::getFlagGalacticEquatorLine(void) const
{
	return galacticEquatorLine->isDisplayed();
}
Vec3f GridLinesMgr::getColorGalacticEquatorLine(void) const
{
	return galacticEquatorLine->getColor();
}
void GridLinesMgr::setColorGalacticEquatorLine(const Vec3f& newColor)
{
	if(newColor != galacticEquatorLine->getColor()) {
		galacticEquatorLine->setColor(newColor);
		emit galacticEquatorLineColorChanged(newColor);
	}
}

//! Set flag for displaying Prime Vertical Line
void GridLinesMgr::setFlagPrimeVerticalLine(const bool displayed)
{
	if(displayed != primeVerticalLine->isDisplayed()) {
		primeVerticalLine->setDisplayed(displayed);
		emit  primeVerticalLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Prime Vertical Line
bool GridLinesMgr::getFlagPrimeVerticalLine(void) const
{
	return primeVerticalLine->isDisplayed();
}
Vec3f GridLinesMgr::getColorPrimeVerticalLine(void) const
{
	return primeVerticalLine->getColor();
}
void GridLinesMgr::setColorPrimeVerticalLine(const Vec3f& newColor)
{
	if(newColor != primeVerticalLine->getColor()) {
		primeVerticalLine->setColor(newColor);
		emit primeVerticalLineColorChanged(newColor);
	}
}

//! Set flag for displaying Colure Lines
void GridLinesMgr::setFlagColureLines(const bool displayed)
{
	if(displayed != colureLine_1->isDisplayed()) {
		colureLine_1->setDisplayed(displayed);
		colureLine_2->setDisplayed(displayed);
		emit  colureLinesDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Colure Lines
bool GridLinesMgr::getFlagColureLines(void) const
{
	return colureLine_1->isDisplayed();
}
Vec3f GridLinesMgr::getColorColureLines(void) const
{
	return colureLine_1->getColor();
}
void GridLinesMgr::setColorColureLines(const Vec3f& newColor)
{
	if(newColor != colureLine_1->getColor()) {
		colureLine_1->setColor(newColor);
		colureLine_2->setColor(newColor);
		emit colureLinesColorChanged(newColor);
	}
}
