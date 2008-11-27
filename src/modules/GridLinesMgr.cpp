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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#include <set>
#include <QSettings>
#include <QDebug>

#include "GridLinesMgr.hpp"
#include "MovementMgr.hpp"
#include "StelApp.hpp"
#include "Navigator.hpp"
#include "Translator.hpp"
#include "Projector.hpp"
#include "LoadingBar.hpp"
#include "StelFader.hpp"
#include "Planet.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelFont.hpp"
#include "StelStyle.hpp"
#include "StelPainter.hpp"

//! @class SkyGrid 
//! Class which manages a grid to display in the sky.
//! TODO needs support for DMS/DMS labelling, not only HMS/DMS
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
	void setFlagshow(bool b){fader = b;}
	bool getFlagshow(void) const {return fader;}
private:
	Vec3f color;
	StelCore::FrameType frameType;
	double fontSize;
	StelFont& font;
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
		LOCAL,
		MERIDIAN
	};
	// Create and precompute positions of a SkyGrid
	SkyLine(SKY_LINE_TYPE _line_type = EQUATOR);
    virtual ~SkyLine();
	void draw(StelCore* core) const;
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void setFadeDuration(float duration) {fader.setDuration((int)(duration*1000.f));}
	void setFlagshow(bool b){fader = b;}
	bool getFlagshow(void) const {return fader;}
	void setFontSize(double newSize);
private:
	SKY_LINE_TYPE line_type;
	Vec3f color;
	StelCore::FrameType frameType;
	LinearFader fader;
	double fontSize;
	StelFont& font;
};

// rms added color as parameter
SkyGrid::SkyGrid(StelCore::FrameType frame) : color(0.2,0.2,0.2), frameType(frame), fontSize(12),
	font(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize))
{
}

SkyGrid::~SkyGrid()
{
}

void SkyGrid::setFontSize(double newFontSize)
{
	fontSize = newFontSize;
	font = StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize);
}

// Conversion into mas = milli arcsecond
static const double RADIAN_MAS = 180./M_PI*1000.*60.*60.;
static const double DEGREE_MAS = 1000.*60.*60.;

// Step sizes in arcsec
static const double STEP_SIZES_DMS[] = {1., 5., 10., 60., 300., 600., 1200., 3600., 3600.*5., 3600.*10.};
static const double STEP_SIZES_HMS[] = {1.5, 7.5, 15., 15.*5., 15.*10., 15.*60., 15.*60.*5., 15.*60*10., 15.*60*60};

//! Return the angular grid step in degree which best fits the given scale 
static double getClosestResolutionDMS(double pixelPerRad)
{
	double minResolution = 80.;
	double minSizeArcsec = minResolution/pixelPerRad*180./M_PI*3600;
	for (unsigned int i=0;i<10;++i)
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
	for (unsigned int i=0;i<9;++i)
		if (STEP_SIZES_HMS[i]>minSizeArcsec)
		{
			return STEP_SIZES_HMS[i]/3600.;
		}
	return 15.;
}

struct ViewportEdgeIntersectCallbackData
{
	ViewportEdgeIntersectCallbackData(const StelPainter& p) : sPainter(p) {;}
	const StelPainter& sPainter;
	StelFont* font;
	Vec4f textColor;
	QString text;
};

// Callback which draws the label of the grid
void viewportEdgeIntersectCallback(const Vec3d& screenPos, const Vec3d& direction, const void* userData)
{
	const ViewportEdgeIntersectCallbackData* d = static_cast<const ViewportEdgeIntersectCallbackData*>(userData);
	Vec3d direc(direction);
	direc.normalize();
	GLfloat tmpColor[4];
	glGetFloatv(GL_CURRENT_COLOR, tmpColor);
	glColor4fv(d->textColor);
	
	double angleDeg = std::atan2(-direc[1], -direc[0])*180./M_PI;
	float xshift=6.f;
	if (angleDeg>90. || angleDeg<-90.)
	{
		angleDeg+=180.;
		xshift=-d->font->getStrLen(d->text)-6.f;
	}
	
	d->sPainter.drawText(d->font, screenPos[0], screenPos[1], d->text, angleDeg, xshift, 3);
	glColor4fv(tmpColor);
}

//! Draw the sky grid in the current frame
void SkyGrid::draw(const StelCore* core) const
{
	const ProjectorP prj = core->getProjection(frameType);
	if (!fader.getInterstate())
		return;

	// Look for all meridians and parallels intersecting with the disk bounding the viewport
	// Check whether the pole are in the viewport
	bool northPoleInViewport = false;
	bool southPoleInViewport = false;
	Vec3d win;
	if (prj->project(Vec3d(0,0,1), win) && prj->checkInViewport(win))
		northPoleInViewport = true;
	if (prj->project(Vec3d(0,0,-1), win) && prj->checkInViewport(win))
		southPoleInViewport = true;
	// Get the longitude and latitude resolution at the center of the viewport
	Vec3d centerV;
	double lon0, lat0, lon1, lat1, lon2, lat2;
	prj->unProject(prj->getViewportPosX()+prj->getViewportWidth()/2, prj->getViewportPosY()+prj->getViewportHeight()/2, centerV);
	StelUtils::rectToSphe(&lon0, &lat0, centerV);
	prj->unProject(prj->getViewportPosX()+prj->getViewportWidth()/2+1, prj->getViewportPosY()+prj->getViewportHeight()/2, centerV);
	StelUtils::rectToSphe(&lon1, &lat1, centerV);
	prj->unProject(prj->getViewportPosX()+prj->getViewportWidth()/2, prj->getViewportPosY()+prj->getViewportHeight()/2+1, centerV);
	StelUtils::rectToSphe(&lon2, &lat2, centerV);
	const double gridStepParallelRad = M_PI/180.*getClosestResolutionDMS(1./std::sqrt((lat1-lat0)*(lat1-lat0)+(lat2-lat0)*(lat2-lat0)));
	const double closetResLon = (frameType==StelCore::FrameAltAz) ? 
		getClosestResolutionDMS(1./std::sqrt((lon1-lon0)*(lon1-lon0)+(lon2-lon0)*(lon2-lon0)))
		: getClosestResolutionHMS(1./std::sqrt((lon1-lon0)*(lon1-lon0)+(lon2-lon0)*(lon2-lon0)));
	const double gridStepMeridianRad = M_PI/180.* ((northPoleInViewport || southPoleInViewport) ? 15. : closetResLon);
	
	// Get the bounding halfspace
	const StelGeom::HalfSpace viewPortHalfSpace = prj->getBoundingHalfSpace();
	
	// Compute the first grid starting point. This point is close to the center of the screen
	// and lays at the intersection of a meridien and a parallel
	lon0 = gridStepMeridianRad*((int)(lon0/gridStepMeridianRad));
	lat0 = gridStepParallelRad*((int)(lat0/gridStepParallelRad));
	Vec3d firstPoint;
	StelUtils::spheToRect(lon0, lat0, firstPoint);
	firstPoint.normalize();
	Q_ASSERT(viewPortHalfSpace.contains(firstPoint));
	
	// Initialize a painter and set openGL state	
	StelPainter sPainter(prj);
	glEnable(GL_LINE_SMOOTH);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	Vec4f textColor(color[0], color[1], color[2], 0);
	if (StelApp::getInstance().getVisionModeNight())
	{
		// instead of a filter which just zeros G&B, set the red 
		// value to the mean brightness of RGB.
		float red = (color[0] + color[1] + color[2]) / 3.0;
		textColor[0] = red;
		textColor[1] = 0.; textColor[2] = 0.;
		glColor4f(red, 0, 0, fader.getInterstate());
	}
	else
	{
		glColor4f(color[0],color[1],color[2], fader.getInterstate());
	}

	textColor*=2;
	textColor[3]=fader.getInterstate();
	
	ViewportEdgeIntersectCallbackData userData(sPainter);
	userData.font = &font;
	userData.textColor = textColor;
	
	/////////////////////////////////////////////////
	// Draw all the meridians (great circles)
	StelGeom::HalfSpace meridianHalfSpace(Vec3d(1,0,0), 0);
	Mat4d rotLon = Mat4d::rotation(Vec3d(0,0,1), gridStepMeridianRad);
	Vec3d fpt = firstPoint;
	Vec3d p1, p2;
	int maxNbIter = (int)(M_PI/gridStepMeridianRad);
	int i;
	for (i=0; i<maxNbIter; ++i)
	{
		StelUtils::rectToSphe(&lon1, &lat1, fpt);
		userData.text = frameType==StelCore::FrameAltAz ? StelUtils::radToDmsStrAdapt(M_PI-lon1) : StelUtils::radToHmsStrAdapt(lon1);
		
		meridianHalfSpace.n = fpt^Vec3d(0,0,1);
		meridianHalfSpace.n.normalize();
		if (!planeIntersect2(viewPortHalfSpace, meridianHalfSpace, p1, p2))
		{
			if (viewPortHalfSpace.d<meridianHalfSpace.d && viewPortHalfSpace.contains(meridianHalfSpace.n))
			{
				// The meridian is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
				Mat4d rotLon120 = Mat4d::rotation(meridianHalfSpace.n, 120.*M_PI/180.);
				Vec3d rotFpt=fpt;
				rotFpt.transfo4d(rotLon120);
				Vec3d rotFpt2=rotFpt;
				rotFpt2.transfo4d(rotLon120);
				sPainter.drawSmallCircleArc(fpt, rotFpt, Vec3d(0,0,0), viewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(rotFpt, rotFpt2, Vec3d(0,0,0), viewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(rotFpt2, fpt, Vec3d(0,0,0), viewportEdgeIntersectCallback, &userData);
				fpt.transfo4d(rotLon);
				continue;
			}
			else
				break;
		}
		
		Vec3d middlePoint = p1+p2;
		middlePoint.normalize();
		if (!viewPortHalfSpace.contains(middlePoint))
			middlePoint*=-1.;
		
		// Debug
// 		prj->project(middlePoint, win);
// 		prj->drawText(&font, win[0], win[1], "M");
// 		prj->project(p1, win);
// 		prj->drawText(&font, win[0], win[1], "P1");
// 		prj->project(p2, win);
// 		prj->drawText(&font, win[0], win[1], "P2");
				
		// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
		sPainter.drawSmallCircleArc(p1, middlePoint, Vec3d(0,0,0), viewportEdgeIntersectCallback, &userData);
		sPainter.drawSmallCircleArc(p2, middlePoint, Vec3d(0,0,0), viewportEdgeIntersectCallback, &userData);
		
		fpt.transfo4d(rotLon);
	}
	
	if (i!=maxNbIter)
	{
		rotLon = Mat4d::rotation(Vec3d(0,0,1), -gridStepMeridianRad);
		fpt = firstPoint;
		fpt.transfo4d(rotLon);
		for (int j=0; j<maxNbIter-i; ++j)
		{
			StelUtils::rectToSphe(&lon1, &lat1, fpt);
			userData.text = frameType==StelCore::FrameAltAz ? StelUtils::radToDmsStrAdapt(M_PI-lon1) : StelUtils::radToHmsStrAdapt(lon1);
			
			meridianHalfSpace.n = fpt^Vec3d(0,0,1);
			meridianHalfSpace.n.normalize();
			if (!planeIntersect2(viewPortHalfSpace, meridianHalfSpace, p1, p2))
				break;
			
			Vec3d middlePoint = p1+p2;
			middlePoint.normalize();
			if (!viewPortHalfSpace.contains(middlePoint))
				middlePoint*=-1;
			
			sPainter.drawSmallCircleArc(p1, middlePoint, Vec3d(0,0,0), viewportEdgeIntersectCallback, &userData);
			sPainter.drawSmallCircleArc(p2, middlePoint, Vec3d(0,0,0), viewportEdgeIntersectCallback, &userData);
			
			fpt.transfo4d(rotLon);
		}
	}
	
	/////////////////////////////////////////////////
	// Draw all the parallels (small circles)
	StelGeom::HalfSpace parallelHalfSpace(Vec3d(0,0,1), 0);
	rotLon = Mat4d::rotation(firstPoint^Vec3d(0,0,1), gridStepParallelRad);
	fpt = firstPoint;
	maxNbIter = (int)(M_PI/gridStepParallelRad)-1;
	for (i=0; i<maxNbIter; ++i)
	{
		StelUtils::rectToSphe(&lon1, &lat1, fpt);
		userData.text = StelUtils::radToDmsStrAdapt(lat1);
		
		parallelHalfSpace.d = fpt[2];
		if (parallelHalfSpace.d>0.9999999)
			break;
		
		const Vec3d rotCenter(0,0,parallelHalfSpace.d);
		if (!planeIntersect2(viewPortHalfSpace, parallelHalfSpace, p1, p2))
		{
			if ((viewPortHalfSpace.d<parallelHalfSpace.d && viewPortHalfSpace.contains(parallelHalfSpace.n))
				|| (viewPortHalfSpace.d<-parallelHalfSpace.d && viewPortHalfSpace.contains(-parallelHalfSpace.n)))
			{
				// The parallel is fully included in the viewport, draw it in 3 sub-arcs to avoid lengths >= 180 deg
				Mat4d rotLon120 = Mat4d::rotation(Vec3d(0,0,1), 120.*M_PI/180.);
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
		if (!viewPortHalfSpace.contains(middlePoint))
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
			StelUtils::rectToSphe(&lon1, &lat1, fpt);
			userData.text = StelUtils::radToDmsStrAdapt(lat1);
			
			parallelHalfSpace.d = fpt[2];
			const Vec3d rotCenter(0,0,parallelHalfSpace.d);
			if (!planeIntersect2(viewPortHalfSpace, parallelHalfSpace, p1, p2))
			{
				if ((viewPortHalfSpace.d<parallelHalfSpace.d && viewPortHalfSpace.contains(parallelHalfSpace.n))
					 || (viewPortHalfSpace.d<-parallelHalfSpace.d && viewPortHalfSpace.contains(-parallelHalfSpace.n)))
				{
					// The parallel is fully included in the viewport, draw it in 3 sub-arcs to avoid lengths >= 180 deg
					Mat4d rotLon120 = Mat4d::rotation(Vec3d(0,0,1), 120.*M_PI/180.);
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
			if (!viewPortHalfSpace.contains(middlePoint))
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
}


SkyLine::SkyLine(SKY_LINE_TYPE _line_type) : color(0.f, 0.f, 1.f), fontSize(1.),
font(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize))
{
	line_type = _line_type;

	switch (line_type)
	{
		case LOCAL : frameType = StelCore::FrameAltAz; break;
		case MERIDIAN : frameType = StelCore::FrameAltAz; break;
		case ECLIPTIC : frameType = StelCore::FrameHelio; break;
		case EQUATOR : frameType = StelCore::FrameEquinoxEqu; break;
		default : frameType = StelCore::FrameEquinoxEqu;
	}
}

SkyLine::~SkyLine()
{
}

void SkyLine::setFontSize(double newFontSize)
{
	fontSize = newFontSize;
	font = StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize);
}

void SkyLine::draw(StelCore *core) const
{
	if (!fader.getInterstate())
		return;

	StelPainter sPainter(core->getProjection(frameType));
	glColor4f(color[0], color[1], color[2], fader.getInterstate());
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	glEnable(GL_LINE_SMOOTH);
	
	// Johannes: use a big radius as a dirty workaround for the bug that the
	// ecliptic line is not drawn around the observer, but around the sun:
	const Vec3d vv(1000000,0,0);
	if (line_type==MERIDIAN)
	{
		sPainter.drawMeridian(vv, 2.*M_PI, false, &font);
	}
	else
	{	
		sPainter.drawParallel(vv, 2.*M_PI, false, &font);
	}
}

GridLinesMgr::GridLinesMgr()
{
	setObjectName("GridLinesMgr");
	equGrid = new SkyGrid(StelCore::FrameEquinoxEqu);
	equJ2000Grid = new SkyGrid(StelCore::FrameJ2000);
	aziGrid = new SkyGrid(StelCore::FrameAltAz);
	equatorLine = new SkyLine(SkyLine::EQUATOR);
	eclipticLine = new SkyLine(SkyLine::ECLIPTIC);
	meridianLine = new SkyLine(SkyLine::MERIDIAN);
}

GridLinesMgr::~GridLinesMgr()
{
	delete equGrid;
	delete equJ2000Grid;
	delete aziGrid;
	delete equatorLine;
	delete eclipticLine;
	delete meridianLine;
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
	setFlagEquatorLine(conf->value("viewing/flag_equator_line").toBool());
	setFlagEclipticLine(conf->value("viewing/flag_ecliptic_line").toBool());
	setFlagMeridianLine(conf->value("viewing/flag_meridian_line").toBool());
}	
	
void GridLinesMgr::update(double deltaTime)
{
	// Update faders
	equGrid->update(deltaTime);
	equJ2000Grid->update(deltaTime);
	aziGrid->update(deltaTime);
	equatorLine->update(deltaTime);
	eclipticLine->update(deltaTime);
	meridianLine->update(deltaTime);
}

void GridLinesMgr::draw(StelCore* core)
{
	// Draw the equatorial grid
	equGrid->draw(core);
	// Draw the equatorial grid
	equJ2000Grid->draw(core);
	// Draw the altazimuthal grid
	aziGrid->draw(core);
	// Draw the celestial equator line
	equatorLine->draw(core);
	// Draw the ecliptic line
	eclipticLine->draw(core);
	// Draw the meridian line
	meridianLine->draw(core);
}

void GridLinesMgr::setStelStyle(const StelStyle& style)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	QString section = style.confSectionName;
	
	// Load colors from config file
	QString defaultColor = conf->value(section+"/default_color").toString();
	setColorEquatorGrid(StelUtils::strToVec3f(conf->value(section+"/equatorial_color", defaultColor).toString()));
	setColorEquatorJ2000Grid(StelUtils::strToVec3f(conf->value(section+"/equatorial_J2000_color", defaultColor).toString()));
	setColorAzimuthalGrid(StelUtils::strToVec3f(conf->value(section+"/azimuthal_color", defaultColor).toString()));
	setColorEquatorLine(StelUtils::strToVec3f(conf->value(section+"/equator_color", defaultColor).toString()));
	setColorEclipticLine(StelUtils::strToVec3f(conf->value(section+"/ecliptic_color", defaultColor).toString()));
	setColorMeridianLine(StelUtils::strToVec3f(conf->value(section+"/meridian_color", defaultColor).toString()));
}

//! Set flag for displaying Azimuthal Grid
void GridLinesMgr::setFlagAzimuthalGrid(bool b) {aziGrid->setFlagshow(b);}
//! Get flag for displaying Azimuthal Grid
bool GridLinesMgr::getFlagAzimuthalGrid(void) const {return aziGrid->getFlagshow();}
Vec3f GridLinesMgr::getColorAzimuthalGrid(void) const {return aziGrid->getColor();}

//! Set flag for displaying Equatorial Grid
void GridLinesMgr::setFlagEquatorGrid(bool b) {equGrid->setFlagshow(b);}
//! Get flag for displaying Equatorial Grid
bool GridLinesMgr::getFlagEquatorGrid(void) const {return equGrid->getFlagshow();}
Vec3f GridLinesMgr::getColorEquatorGrid(void) const {return equGrid->getColor();}

//! Set flag for displaying Equatorial J2000 Grid
void GridLinesMgr::setFlagEquatorJ2000Grid(bool b) {equJ2000Grid->setFlagshow(b);}
//! Get flag for displaying Equatorial J2000 Grid
bool GridLinesMgr::getFlagEquatorJ2000Grid(void) const {return equJ2000Grid->getFlagshow();}
Vec3f GridLinesMgr::getColorEquatorJ2000Grid(void) const {return equJ2000Grid->getColor();}

//! Set flag for displaying Equatorial Line
void GridLinesMgr::setFlagEquatorLine(bool b) {equatorLine->setFlagshow(b);}
//! Get flag for displaying Equatorial Line
bool GridLinesMgr::getFlagEquatorLine(void) const {return equatorLine->getFlagshow();}
Vec3f GridLinesMgr::getColorEquatorLine(void) const {return equatorLine->getColor();}

//! Set flag for displaying Ecliptic Line
void GridLinesMgr::setFlagEclipticLine(bool b) {eclipticLine->setFlagshow(b);}
//! Get flag for displaying Ecliptic Line
bool GridLinesMgr::getFlagEclipticLine(void) const {return eclipticLine->getFlagshow();}
Vec3f GridLinesMgr::getColorEclipticLine(void) const {return eclipticLine->getColor();}


//! Set flag for displaying Meridian Line
void GridLinesMgr::setFlagMeridianLine(bool b) {meridianLine->setFlagshow(b);}
//! Get flag for displaying Meridian Line
bool GridLinesMgr::getFlagMeridianLine(void) const {return meridianLine->getFlagshow();}
Vec3f GridLinesMgr::getColorMeridianLine(void) const {return meridianLine->getColor();}

void GridLinesMgr::setColorAzimuthalGrid(const Vec3f& v) { aziGrid->setColor(v);}
void GridLinesMgr::setColorEquatorGrid(const Vec3f& v) { equGrid->setColor(v);}
void GridLinesMgr::setColorEquatorJ2000Grid(const Vec3f& v) { equJ2000Grid->setColor(v);}
void GridLinesMgr::setColorEquatorLine(const Vec3f& v) { equatorLine->setColor(v);}
void GridLinesMgr::setColorEclipticLine(const Vec3f& v) { eclipticLine->setColor(v);}
void GridLinesMgr::setColorMeridianLine(const Vec3f& v) { meridianLine->setColor(v);}
