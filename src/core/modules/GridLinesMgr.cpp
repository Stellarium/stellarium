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

#include <set>
#include <QSettings>
#include <QDebug>
#include <QFontMetrics>

#include "GridLinesMgr.hpp"
#include "StelApp.hpp"

#include "renderer/StelCircleArcRenderer.hpp"
#include "renderer/StelRenderer.hpp"

#include "StelTranslator.hpp"
#include "StelProjector.hpp"
#include "StelFader.hpp"
#include "Planet.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"

//! @class SkyGrid
//! Class which manages a grid to display in the sky.
//! TODO needs support for DMS/DMS labelling, not only HMS/DMS
class SkyGrid
{
public:
	// Create and precompute positions of a SkyGrid
	SkyGrid(StelCore::FrameType frame);
	virtual ~SkyGrid();
	//! Draw the grid.
	//!
	//! @param core     The StelCore object.
	//! @param renderer Renderer to draw with.
	void draw(const StelCore* core, StelRenderer* renderer) const;
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
		EQUATOR,
		ECLIPTIC,
		MERIDIAN,
		HORIZON,
		GALACTICPLANE
	};
	// Create and precompute positions of a SkyGrid
	SkyLine(SKY_LINE_TYPE _line_type = EQUATOR);
	virtual ~SkyLine();
	//! Draw the line.
	//!
	//! @param core     The StelCore object.
	//! @param renderer Renderer to draw with.
	void draw(StelCore* core, StelRenderer* renderer) const;
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
	font.setPixelSize(12);
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

//! Data passed to viewportEdgeIntersectCallback.
struct ViewportEdgeIntersectCallbackData
{
	//! Construct ViewportEdgeIntersectCallbackData with specified font metrics.
	ViewportEdgeIntersectCallbackData(const QFontMetrics& metrics) 
		: fontMetrics(metrics){}

	//! Renderer to draw the label with.
	StelRenderer* renderer;
	//! Projector to project 3D coordinates to viewport.
	StelProjectorP projector;
	//! Color of label text.
	Vec4f textColor;
	//! Color of the sky line (or grid) being drawn.
	Vec4f skyLineColor;
	//! Metrics of the label font (so we know how to offset the label).
	const QFontMetrics fontMetrics;
	//! Label text.
	QString text;
	double raAngle;// Used for meridians
	StelCore::FrameType frameType;
};

//! Callback which draws the label of the grid at the edge of the viewport.
//!
//! @param screenPos 2D position of the intersection on the screen.
//! @param direction Normalized direction of the arc toward the inside of the viewport.
//! @param userData  ViewportEdgeIntersectCallbackData.
void viewportEdgeIntersectCallback(const Vec3d& screenPos, const Vec3d& direction, void* userData)
{
	ViewportEdgeIntersectCallbackData* d = 
		static_cast<ViewportEdgeIntersectCallbackData*>(userData);
	d->renderer->setGlobalColor(d->textColor);

	QString text;
	if (d->text.isEmpty())
	{
		// We are in the case of meridians,
		// we need to determine which of the 2 labels (3h or 15h) to use
		Vec3d tmpV;
		d->projector->unProject(screenPos, tmpV);
		double lon, lat, raAngle;
		StelUtils::rectToSphe(&lon, &lat, tmpV);

		const bool altAzOrGalactic = d->frameType == StelCore::FrameAltAz || 
		                             d->frameType == StelCore::FrameGalactic;
		raAngle = d->raAngle;
		if(altAzOrGalactic)
		{
			raAngle = M_PI - raAngle;
			lon     = M_PI - lon;
			if (raAngle < 0) {raAngle += 2.0 * M_PI;}
			if (lon < 0)     {lon     += 2.0 * M_PI;}
		}
		if (std::fabs(2.0 * M_PI - lon) < 0.01)
		{
			// We are at meridian 0
			lon = 0.0;
		}
		const double delta = raAngle < M_PI ? M_PI : -M_PI;
		double textAngle;
		if (std::fabs(lon - raAngle) < 0.01)
		{
			textAngle = 
				(d->frameType == StelCore::FrameGalactic) ? -raAngle + M_PI : raAngle;
		}
		else if(d->frameType == StelCore::FrameAltAz)
		{
			textAngle = 
				(std::fabs(raAngle - 2.0 * M_PI) < 0.01) ? 0.0 : raAngle + delta;
		}
		else if(d->frameType == StelCore::FrameGalactic)
		{
			textAngle = -raAngle - delta + M_PI;
		}
		else
		{
			textAngle = raAngle + delta;
		}

		text = altAzOrGalactic ? StelUtils::radToDmsStrAdapt(textAngle)
		                       : StelUtils::radToHmsStrAdapt(textAngle);
	}
	else
	{
		text = d->text;
	}

	float angleDeg = std::atan2(-direction[1], -direction[0]) * 180.0 / M_PI;
	float xshift = 6.0f;
	if (angleDeg > 90.0f || angleDeg < -90.0f)
	{
		angleDeg += 180.0f;
		xshift = -(d->fontMetrics.width(text)) - 6.0f;
	}

	d->renderer->drawText(TextParams(screenPos[0], screenPos[1], text)
                         .angleDegrees(angleDeg).shift(xshift, 3));
	d->renderer->setGlobalColor(d->skyLineColor);
	d->renderer->setBlendMode(BlendMode_Alpha);
}

//! Draw the sky grid in the current frame
void SkyGrid::draw(const StelCore* core, StelRenderer* renderer) const
{
	const StelProjectorP projector = core->getProjection(frameType, frameType!=StelCore::FrameAltAz ? StelCore::RefractionAuto : StelCore::RefractionOff);
	if (!fader.getInterstate())
		return;

	// Look for all meridians and parallels intersecting with the disk bounding the viewport
	// Check whether the pole are in the viewport
	bool northPoleInViewport = false;
	bool southPoleInViewport = false;
	Vec3f win;
	if (projector->project(Vec3f(0,0,1), win) && projector->checkInViewport(win))
		northPoleInViewport = true;
	if (projector->project(Vec3f(0,0,-1), win) && projector->checkInViewport(win))
		southPoleInViewport = true;
	// Get the longitude and latitude resolution at the center of the viewport
	Vec3d centerV;
	projector->unProject(projector->getViewportPosX()+projector->getViewportWidth()/2, projector->getViewportPosY()+projector->getViewportHeight()/2+1, centerV);
	double lon2, lat2;
	StelUtils::rectToSphe(&lon2, &lat2, centerV);

	const double gridStepParallelRad = M_PI/180.*getClosestResolutionDMS(projector->getPixelPerRadAtCenter());
	double gridStepMeridianRad;
	if (northPoleInViewport || southPoleInViewport)
		gridStepMeridianRad = (frameType==StelCore::FrameAltAz || frameType==StelCore::FrameGalactic) ? M_PI/180.* 10. : M_PI/180.* 15.;
	else
	{
		const double closetResLon = (frameType==StelCore::FrameAltAz || frameType==StelCore::FrameGalactic) ? getClosestResolutionDMS(projector->getPixelPerRadAtCenter()*std::cos(lat2)) : getClosestResolutionHMS(projector->getPixelPerRadAtCenter()*std::cos(lat2));
		gridStepMeridianRad = M_PI/180.* ((northPoleInViewport || southPoleInViewport) ? 15. : closetResLon);
	}

	// Get the bounding halfspace
	const SphericalCap& viewPortSphericalCap = projector->getBoundingCap();

	// Compute the first grid starting point. This point is close to the center of the screen
	// and lays at the intersection of a meridien and a parallel
	lon2 = gridStepMeridianRad*((int)(lon2/gridStepMeridianRad+0.5));
	lat2 = gridStepParallelRad*((int)(lat2/gridStepParallelRad+0.5));
	Vec3d firstPoint;
	StelUtils::spheToRect(lon2, lat2, firstPoint);
	firstPoint.normalize();

	// Q_ASSERT(viewPortSphericalCap.contains(firstPoint));

	// Prepare for drawing
	renderer->setBlendMode(BlendMode_Alpha);
	Vec4f textColor(color[0], color[1], color[2], 0.0f);

	Vec4f skyGridColor;
	if (StelApp::getInstance().getVisionModeNight())
	{
		// instead of a filter which just zeros G&B, set the red
		// value to the mean brightness of RGB.
		float red = (color[0] + color[1] + color[2]) / 3.0;
		textColor[0] = red;
		textColor[1] = 0.; textColor[2] = 0.;
		skyGridColor = Vec4f(red, 0.0f, 0.0f, fader.getInterstate());
	}
	else
	{
		skyGridColor = Vec4f(color[0], color[1], color[2], fader.getInterstate());
	}
	renderer->setGlobalColor(skyGridColor);

	textColor *= 2;
	textColor[3] = fader.getInterstate();

	renderer->setFont(font);
	ViewportEdgeIntersectCallbackData userData =
		ViewportEdgeIntersectCallbackData(QFontMetrics(font));
	userData.renderer     = renderer;
	userData.projector    = projector;
	userData.textColor    = textColor;
	userData.frameType    = frameType;
	userData.skyLineColor = skyGridColor;

	/////////////////////////////////////////////////
	// Draw all the meridians (great circles)
	SphericalCap meridianSphericalCap(Vec3d(1,0,0), 0);
	Mat4d rotLon = Mat4d::zrotation(gridStepMeridianRad);
	Vec3d fpt = firstPoint;
	Vec3d p1, p2;
	int maxNbIter = (int)(M_PI/gridStepMeridianRad);
	int i;
	StelCircleArcRenderer circleRenderer(renderer, projector);
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
				circleRenderer.drawGreatCircleArc
					(fpt, rotFpt, NULL, viewportEdgeIntersectCallback, &userData); 
				circleRenderer.drawGreatCircleArc
					(rotFpt, rotFpt2, NULL, viewportEdgeIntersectCallback, &userData); 
				circleRenderer.drawGreatCircleArc
					(rotFpt2, fpt, NULL, viewportEdgeIntersectCallback, &userData); 
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
		circleRenderer.drawGreatCircleArc
			(p1, middlePoint, NULL, viewportEdgeIntersectCallback, &userData); 
		circleRenderer.drawGreatCircleArc
			(p2, middlePoint, NULL, viewportEdgeIntersectCallback, &userData); 

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

			circleRenderer.drawGreatCircleArc
				(p1, middlePoint, NULL, viewportEdgeIntersectCallback, &userData); 
			circleRenderer.drawGreatCircleArc
				(p2, middlePoint, NULL, viewportEdgeIntersectCallback, &userData); 

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
		userData.text = StelUtils::radToDmsStrAdapt(lat2);

		parallelSphericalCap.d = fpt[2];
		if (parallelSphericalCap.d>0.9999999)
			break;

		const Vec3d rotCenter(0,0,parallelSphericalCap.d);
		circleRenderer.setRotCenter(rotCenter);

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

				circleRenderer.drawSmallCircleArc
					(fpt, rotFpt, viewportEdgeIntersectCallback, &userData);
				circleRenderer.drawSmallCircleArc
					(rotFpt, rotFpt2, viewportEdgeIntersectCallback, &userData);
				circleRenderer.drawSmallCircleArc
					(rotFpt2, fpt, viewportEdgeIntersectCallback, &userData);
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

		circleRenderer.drawSmallCircleArc
			(p1, middlePoint, viewportEdgeIntersectCallback, &userData);
		circleRenderer.drawSmallCircleArc
			(p2, middlePoint, viewportEdgeIntersectCallback, &userData);

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
			userData.text = StelUtils::radToDmsStrAdapt(lat2);

			parallelSphericalCap.d = fpt[2];
			const Vec3d rotCenter(0,0,parallelSphericalCap.d);
			circleRenderer.setRotCenter(rotCenter);
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
					circleRenderer.drawSmallCircleArc
						(fpt, rotFpt, viewportEdgeIntersectCallback, &userData);
					circleRenderer.drawSmallCircleArc
						(rotFpt, rotFpt2, viewportEdgeIntersectCallback, &userData);
					circleRenderer.drawSmallCircleArc
						(rotFpt2, fpt, viewportEdgeIntersectCallback, &userData);
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

			circleRenderer.drawSmallCircleArc
				(p1, middlePoint, viewportEdgeIntersectCallback, &userData);
			circleRenderer.drawSmallCircleArc
				(p2, middlePoint, viewportEdgeIntersectCallback, &userData);

			fpt.transfo4d(rotLon);
		}
	}
}


SkyLine::SkyLine(SKY_LINE_TYPE _line_type) : color(0.f, 0.f, 1.f)
{
	font.setPixelSize(14);
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
		case ECLIPTIC:
			frameType = StelCore::FrameObservercentricEcliptic;
			label = q_("Ecliptic");
			break;
		case EQUATOR:
			frameType = StelCore::FrameEquinoxEqu;
			label = q_("Equator");
			break;
		case HORIZON:
			frameType = StelCore::FrameAltAz;
			label = q_("Horizon");
			break;
		case GALACTICPLANE:
			frameType = StelCore::FrameGalactic;
			label = q_("Galactic Plane");
			break;
	}
}

void SkyLine::draw(StelCore *core, StelRenderer* renderer) const
{
	if (!fader.getInterstate())
		return;

	StelProjectorP projector = core->getProjection(frameType, frameType!=StelCore::FrameAltAz ? StelCore::RefractionAuto : StelCore::RefractionOff);

	// Get the bounding halfspace
	const SphericalCap& viewPortSphericalCap = projector->getBoundingCap();

	Vec4f textColor, skyLineColor;
	if (StelApp::getInstance().getVisionModeNight())
	{
		// instead of a filter which just zeros G&B, set the red
		// value to the mean brightness of RGB.
		float red = (color[0] + color[1] + color[2]) / 3.0;
		textColor = Vec4f(red, 0.0f, 0.0f, 0.0f);
		skyLineColor = Vec4f(red, 0.0f, 0.0f, fader.getInterstate());
	}
	else
	{
		skyLineColor = Vec4f(color[0], color[1], color[2], fader.getInterstate());
		textColor = Vec4f(color[0], color[1], color[2], 0.0f);
	}
	renderer->setGlobalColor(skyLineColor);
	renderer->setBlendMode(BlendMode_Alpha);

	textColor *= 2;
	textColor[3] = fader.getInterstate();

	renderer->setFont(font);

	ViewportEdgeIntersectCallbackData userData = 
		ViewportEdgeIntersectCallbackData(QFontMetrics(font));
	userData.renderer     = renderer;
	userData.projector    = projector;
	userData.textColor    = textColor;
	userData.skyLineColor = skyLineColor;
	userData.text         = label;

	/////////////////////////////////////////////////
	// Draw the line
	SphericalCap meridianSphericalCap(Vec3d(0,0,1), 0);	
	Vec3d fpt(1,0,0);
	if (line_type==MERIDIAN)
	{
		meridianSphericalCap.n.set(0,1,0);
	}

	StelCircleArcRenderer circleRenderer(renderer, projector);
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

			circleRenderer.drawGreatCircleArc
				(fpt, rotFpt, NULL, viewportEdgeIntersectCallback, &userData); 
			circleRenderer.drawGreatCircleArc
				(rotFpt, rotFpt2, NULL, viewportEdgeIntersectCallback, &userData); 
			circleRenderer.drawGreatCircleArc
				(rotFpt2, fpt, NULL, viewportEdgeIntersectCallback, &userData); 
		}
		return;
	}


	Vec3d middlePoint = p1+p2;
	middlePoint.normalize();
	if (!viewPortSphericalCap.contains(middlePoint))
		middlePoint*=-1.;

	// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
	circleRenderer.drawGreatCircleArc
		(p1, middlePoint, NULL, viewportEdgeIntersectCallback, &userData); 
	circleRenderer.drawGreatCircleArc
		(p2, middlePoint, NULL, viewportEdgeIntersectCallback, &userData); 

// 	// Johannes: use a big radius as a dirty workaround for the bug that the
// 	// ecliptic line is not drawn around the observer, but around the sun:
// 	const Vec3d vv(1000000,0,0);
}

GridLinesMgr::GridLinesMgr()
{
	setObjectName("GridLinesMgr");
	equGrid           = new SkyGrid(StelCore::FrameEquinoxEqu);
	equJ2000Grid      = new SkyGrid(StelCore::FrameJ2000);
	eclJ2000Grid      = new SkyGrid(StelCore::FrameObservercentricEcliptic);
	galacticGrid      = new SkyGrid(StelCore::FrameGalactic);
	aziGrid           = new SkyGrid(StelCore::FrameAltAz);
	equatorLine       = new SkyLine(SkyLine::EQUATOR);
	eclipticLine      = new SkyLine(SkyLine::ECLIPTIC);
	meridianLine      = new SkyLine(SkyLine::MERIDIAN);
	horizonLine       = new SkyLine(SkyLine::HORIZON);
	galacticPlaneLine = new SkyLine(SkyLine::GALACTICPLANE);
}

GridLinesMgr::~GridLinesMgr()
{
	delete equGrid;
	delete equJ2000Grid;
	delete eclJ2000Grid;
	delete galacticGrid;
	delete aziGrid;
	delete equatorLine;
	delete eclipticLine;
	delete meridianLine;
	delete horizonLine;
	delete galacticPlaneLine;
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
	setFlagGalacticGrid(conf->value("viewing/flag_galactic_grid").toBool());
	setFlagEquatorLine(conf->value("viewing/flag_equator_line").toBool());
	setFlagEclipticLine(conf->value("viewing/flag_ecliptic_line").toBool());
	setFlagMeridianLine(conf->value("viewing/flag_meridian_line").toBool());
	setFlagHorizonLine(conf->value("viewing/flag_horizon_line").toBool());
	setFlagGalacticPlaneLine(conf->value("viewing/flag_galactic_plane_line").toBool());
	
	StelApp& app = StelApp::getInstance();
	connect(&app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateLineLabels()));
}

void GridLinesMgr::update(double deltaTime)
{
	// Update faders
	equGrid->update(deltaTime);
	equJ2000Grid->update(deltaTime);
	eclJ2000Grid->update(deltaTime);
	galacticGrid->update(deltaTime);
	aziGrid->update(deltaTime);
	equatorLine->update(deltaTime);
	eclipticLine->update(deltaTime);
	meridianLine->update(deltaTime);
	horizonLine->update(deltaTime);
	galacticPlaneLine->update(deltaTime);
}

void GridLinesMgr::draw(StelCore* core, class StelRenderer* renderer)
{
	equGrid->draw(core, renderer);
	galacticGrid->draw(core, renderer);
	equJ2000Grid->draw(core, renderer);
	eclJ2000Grid->draw(core, renderer);
	aziGrid->draw(core, renderer);
	equatorLine->draw(core, renderer);
	eclipticLine->draw(core, renderer);
	meridianLine->draw(core, renderer);
	horizonLine->draw(core, renderer);
	galacticPlaneLine->draw(core, renderer);
}

void GridLinesMgr::setStelStyle(const QString& section)
{
	QSettings* conf = StelApp::getInstance().getSettings();

	// Load colors from config file
	QString defaultColor = conf->value(section+"/default_color").toString();
	setColorEquatorGrid(StelUtils::strToVec3f(conf->value(section+"/equatorial_color", defaultColor).toString()));
	setColorEquatorJ2000Grid(StelUtils::strToVec3f(conf->value(section+"/equatorial_J2000_color", defaultColor).toString()));
	setColorEclipticJ2000Grid(StelUtils::strToVec3f(conf->value(section+"/ecliptic_J2000_color", defaultColor).toString()));
	setColorGalacticGrid(StelUtils::strToVec3f(conf->value(section+"/galactic_color", defaultColor).toString()));
	setColorAzimuthalGrid(StelUtils::strToVec3f(conf->value(section+"/azimuthal_color", defaultColor).toString()));
	setColorEquatorLine(StelUtils::strToVec3f(conf->value(section+"/equator_color", defaultColor).toString()));
	setColorEclipticLine(StelUtils::strToVec3f(conf->value(section+"/ecliptic_color", defaultColor).toString()));
	setColorMeridianLine(StelUtils::strToVec3f(conf->value(section+"/meridian_color", defaultColor).toString()));
	setColorHorizonLine(StelUtils::strToVec3f(conf->value(section+"/horizon_color", defaultColor).toString()));
	setColorGalacticPlaneLine(StelUtils::strToVec3f(conf->value(section+"/galactic_plane_color", defaultColor).toString()));
}

void GridLinesMgr::updateLineLabels()
{
	equatorLine->updateLabel();
	eclipticLine->updateLabel();
	meridianLine->updateLabel();
	horizonLine->updateLabel();
	galacticPlaneLine->updateLabel();
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

//! Set flag for displaying GalacticPlane Line
void GridLinesMgr::setFlagGalacticPlaneLine(const bool displayed)
{
	if(displayed != galacticPlaneLine->isDisplayed()) {
		galacticPlaneLine->setDisplayed(displayed);
		emit galacticPlaneLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying GalacticPlane Line
bool GridLinesMgr::getFlagGalacticPlaneLine(void) const
{
	return galacticPlaneLine->isDisplayed();
}
Vec3f GridLinesMgr::getColorGalacticPlaneLine(void) const
{
	return galacticPlaneLine->getColor();
}
void GridLinesMgr::setColorGalacticPlaneLine(const Vec3f& newColor)
{
	if(newColor != galacticPlaneLine->getColor()) {
		galacticPlaneLine->setColor(newColor);
		emit galacticPlaneLineColorChanged(newColor);
	}
}
