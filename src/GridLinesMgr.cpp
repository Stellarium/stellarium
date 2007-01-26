#include "GridLinesMgr.hpp"

#include "StelApp.hpp"
#include "Navigator.hpp"
#include "Translator.hpp"
#include "Projector.hpp"
#include "LoadingBar.hpp"
#include "Fader.hpp"
#include "Planet.hpp"

// Class which manages a grid to display in the sky
class SkyGrid
{
public:
	// Create and precompute positions of a SkyGrid
	SkyGrid(Projector::FRAME_TYPE frame = Projector::FRAME_EARTH_EQU, unsigned int _nb_meridian = 24, unsigned int _nb_parallel = 18);
    virtual ~SkyGrid();
	void draw(const Projector* prj) const;
	void setFontSize(double newFontSize);
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void setFlagshow(bool b){fader = b;}
	bool getFlagshow(void) const {return fader;}
	void set_top_transparancy(bool b) { transparent_top= b; }
private:
	unsigned int nb_meridian;
	unsigned int nb_parallel;
	bool transparent_top;
	Vec3f color;
	Projector::FRAME_TYPE frameType;
	double fontSize;
	SFont& font;
	LinearFader fader;
};


// Class which manages a line to display around the sky like the ecliptic line
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
	SkyLine(SKY_LINE_TYPE _line_type = EQUATOR, double _radius = 1., unsigned int _nb_segment = 48);
    virtual ~SkyLine();
	void draw(Projector *prj,const Navigator *nav) const;
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void setFlagshow(bool b){fader = b;}
	bool getFlagshow(void) const {return fader;}
	void setFontSize(double newSize);
private:
	double radius;
	unsigned int nb_segment;
	SKY_LINE_TYPE line_type;
	Vec3f color;
	Vec3f* points;
	Projector::FRAME_TYPE frameType;
	LinearFader fader;
	double fontSize;
	SFont& font;
};


// rms added color as parameter
SkyGrid::SkyGrid(Projector::FRAME_TYPE frame, unsigned int _nb_meridian, unsigned int _nb_parallel) :
	nb_meridian(_nb_meridian), nb_parallel(_nb_parallel), color(0.2,0.2,0.2), frameType(frame), fontSize(12),
	font(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize))
{
	transparent_top = true;
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
const static double RADIAN_MAS = 180./M_PI*1000.*60.*60.;
const static double RADIAN_DEG = 180./M_PI;
const static double DEGREE_MAS = 1000.*60.*60.;
const static double ARCMIN_MAS = 1000.*60;
const static double ARCSEC_MAS = 1000.;

//! Return the standard longitude in degree [-180;+180] for a position given in the viewport
static double getLonFrom2dPos(const Projector* prj, const Vec2d& p)
{
	Vec3d v;
	prj->unProject(p[0], p[1], v);
	return RADIAN_DEG*std::atan2(v[1],v[0]);
}

//! Return the standard latitude in degree [-90;+90] for a position given in the viewport
static double getLatFrom2dPos(const Projector* prj, const Vec2d& p)
{
	Vec3d v;
	prj->unProject(p[0], p[1], v);
	return RADIAN_DEG*std::asin(v[2]);
}


void rectToSpheLat180(double& lon, double& lat, const Vec3d& v)
{
	StelUtils::rect_to_sphe(&lon, &lat, v);
	lat += M_PI/2;
	// Longitude between -180 and 180..
	if (lon<0.)
	{
		lat = 2.*M_PI-lat;
		lon = -lon;
	}
}

void spheToRectLat180(double lon, double lat, Vec3d& v)
{
	if (lat>M_PI)
	{
		lat = 2.*M_PI-lat;
		lon = 2.*M_PI-lon;
	}
	lat -= M_PI/2;
	StelUtils::sphe_to_rect(lon, lat, v);
}

//! Return a special latitude in degree [0;+360] for a position given in the viewport
static double getLatFrom2dPosLat180(const Projector* prj, const Vec2d& p)
{
	Vec3d v;
	prj->unProject(p[0], p[1], v);
	double lon, lat;
	rectToSpheLat180(lon, lat, v);
	return RADIAN_DEG*lat;
}

//! Return a special longitude in degree [0;180] for a position given in the viewport
static double getLonFrom2dPosLat180(const Projector* prj, const Vec2d& p)
{
	Vec3d v;
	prj->unProject(p[0], p[1], v);
	double lon, lat;
	rectToSpheLat180(lon, lat, v);
	return RADIAN_DEG*lon;
}


//! Return the 2D position in the viewport from a longitude and latitude in radian
static Vec3d get2dPosFromSpherical(const Projector* prj, double lon, double lat)
{
	Vec3d v;
	Vec3d win;
	StelUtils::sphe_to_rect(lon, lat, v);
	prj->project(v, win);
	return win;
}


//! Return the 2D position in the viewport from a longitude and latitude in radian
static Vec3d get2dPosFromSphericalLat0180(const Projector* prj, double lon, double lat)
{
	Vec3d v;
	Vec3d win;
	spheToRectLat180(lon, lat, v);
	prj->project(v, win);
	return win;
}

//! Check if the given point from the viewport side is the beginning of a parallel or not
//! Beginning means that the direction of increasing longitude goes inside the viewport 
static bool isParallelEntering(const Projector* prj, const Vec2d& v, double lat)
{
	const double lon = getLonFrom2dPos(prj, v);
	return prj->check_in_viewport(get2dPosFromSpherical(prj, lon/RADIAN_DEG+0.001, lat));
}

//! Check if the given point from the viewport side is the beginning of a meridian or not
//! Beginning means that the direction of increasing latitude goes inside the viewport 
static bool isMeridianEnteringLat180(const Projector* prj, const Vec2d& v, double lon180)
{
	const double lat180 = getLatFrom2dPosLat180(prj, v);
	if (lat180>180.)
		return prj->check_in_viewport(get2dPosFromSphericalLat0180(prj, M_PI-lon180, lat180/RADIAN_DEG+0.001));
	else
		return prj->check_in_viewport(get2dPosFromSphericalLat0180(prj, lon180, lat180/RADIAN_DEG+0.001));
}


//! Return all the points p on the segment [p0 p1] for which the value of func(p) == k*stepMas
//! with a precision < 0.5 pixels
//! For each value of k*stepMas, the result is then sorted ordered according to the value of func2(p)
static void getPs(map<int, map<double, Vec2d> > & result, const Projector* prj, 
	const Vec2d& p0, const Vec2d& p1, double stepMas, 
	double (*func)(const Projector* prj, const Vec2d& p),
	double (*func2)(const Projector* prj, const Vec2d& p))
{
	const Vec2d deltaP(p1-p0);
	Vec2d p = p0;
	const Vec2d dPix1 = deltaP/(deltaP.length());	// 1 pixel step
	const Vec2d dPixPrec = deltaP/(deltaP.length()*2.);	// 0.5 pixel step
	double funcp, funcpDpix, target, deriv, u=0.;
	
	funcp = func(prj, p);
	funcpDpix = func(prj, p+dPixPrec);
	deriv = (funcpDpix-funcp)/0.5;
	target = stepMas*(std::floor(funcp/stepMas) + (deriv>0 ? 1:0));
	bool sureThatTargetExist = false;
	while (u<deltaP.length())
	{
		// Find next point
		if ((funcpDpix>=target && funcp<target) || (funcpDpix<=target && funcp>target))
		{
			// If more that one target was inside the range [funcp;funcpDpix] add them to the result list
			while ((funcpDpix>=target && funcp<target) || (funcpDpix<=target && funcp>target))
			{
				if (result.find((int)(target*DEGREE_MAS))!=result.end() && result[(int)(target*DEGREE_MAS)].find(func2(prj, p))!=result[(int)(target*DEGREE_MAS)].end())
					cerr << "Err" << endl;
				result[(int)(target*DEGREE_MAS)][func2(prj, p)]=p;
				target+=(deriv>0) ? stepMas:-stepMas;
			}
		
			p = p+dPixPrec;
			u+=0.5;
			funcp = funcpDpix;
			funcpDpix = func(prj, p+dPixPrec);
			deriv = (funcpDpix-funcp)/0.5;
			target = stepMas*(std::floor(funcp/stepMas) + (deriv>0 ? 1:0));
			sureThatTargetExist = false;
		}
		else
		{
			if ((deriv>0 && funcp>target) || (deriv<0 && funcp<target))
				sureThatTargetExist = true;	// We went too "far", thus we know that the target exists
				
			deriv = (funcpDpix-funcp)/0.5;
			if (sureThatTargetExist==false)
				target = stepMas*(std::floor(funcp/stepMas) + (deriv>0 ? 1:0));
			double dU = (target-funcp)/deriv;
			// TODO handle this properly, maybe using 2nd derivatives?
			if (fabs(dU)<0.05)
				dU = 0.05*(dU/fabs(dU));
			if (dU>100.)
				dU = 100.;
			if (dU<-100.)
				dU = -100;
			u += dU;
			p += dPix1*dU;
			funcp = func(prj, p);
			funcpDpix = func(prj, p+dPixPrec);
		}
	}
}


//! Return all the points p on the segment [p0 p1] for which the value of func(p) == k*stepMas
//! with a precision < 0.5 pixels
//! For each value of k*stepMas, the result is then sorted ordered according to the value of func2(p)
static void getPslow(map<int, map<double, Vec2d> > & result, const Projector* prj, 
	const Vec2d& p0, const Vec2d& p1, double stepMas, 
	double (*func)(const Projector* prj, const Vec2d& p),
	double (*func2)(const Projector* prj, const Vec2d& p))
{
	const double precision = 0.2;
	const Vec2d deltaP(p1-p0);
	Vec2d p = p0;
	const Vec2d dPix1 = deltaP/(deltaP.length());	// 1 pixel step
	const Vec2d dPixPrec = deltaP*precision/(deltaP.length());	// 0.5 pixel step
	double funcp, funcpDpix;
	
	funcp = func(prj, p);
	funcpDpix = func(prj, p+dPixPrec);
		
	for (double u=0;u<deltaP.length();u+=precision)
	{	
		if (funcp<funcpDpix)
		{
			// If targets are included inside the range, add them
			const double r1 = stepMas*(std::floor(funcp/stepMas));
			const double r2 = stepMas*(std::ceil(funcpDpix/stepMas));
			
			for (double v=r1;v<r2;v+=stepMas)
				if (funcp<=v && funcpDpix>v)
					result[(int)(v*DEGREE_MAS)][func2(prj, p)]=p;
		}
		else
		{
			// If targets are included inside the range, add them
			const double r1 = stepMas*(std::ceil(funcp/stepMas));
			const double r2 = stepMas*(std::floor(funcpDpix/stepMas));
			
			for (double v=r2;v<r1;v+=stepMas)
				if (funcp>=v && funcpDpix<v)
					result[(int)(v*DEGREE_MAS)][func2(prj, p)]=p;
		}
		
		p+=dPixPrec;
		funcp = funcpDpix;
		funcpDpix = func(prj,p);
	}
}

// Step sizes in arcsec
static const double STEP_SIZES_DMS[] = {1., 10., 60., 600., 3600., 3600.*10.};
static const double STEP_SIZES_HMS[] = {1., 10., 60., 600., 3600., 3600.*15.};

static double getClosestResolutionParallel(double pixelPerRad)
{
	double minResolution = 50.;
	double minSizeArcsec = minResolution/pixelPerRad*180./M_PI*3600;
	for (unsigned int i=0;i<6;++i)
		if (STEP_SIZES_DMS[i]>minSizeArcsec)
		{
			return STEP_SIZES_DMS[i]/3600.;
		}
	return 10.;
}

static double getClosestResolutionMeridian(double pixelPerRad)
{
	double minResolution = 50.;
	double minSizeArcsec = minResolution/pixelPerRad*180./M_PI*3600;
	for (unsigned int i=0;i<6;++i)
		if (STEP_SIZES_HMS[i]>minSizeArcsec)
		{
			return STEP_SIZES_HMS[i]/3600.;
		}
	return 10.;
}

void SkyGrid::draw(const Projector* prj) const
{
	if (!fader.getInterstate()) return;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

	glColor4f(color[0],color[1],color[2], fader.getInterstate());

	prj->setCurrentFrame(frameType);	// set 2D coordinate

	
	const double gridStepParallelDeg = getClosestResolutionParallel(prj->getPixelPerRad());
	const double gridStepMeridianDeg = getClosestResolutionMeridian(prj->getPixelPerRad());
	
	map<int, map<double, Vec2d> > resultsParallels;
	map<int, map<double, Vec2d> > resultsMeridians;
	const vector<Vec2d> viewportVertices = prj->getViewportVertices();
	for (unsigned int i=0;i<viewportVertices.size();++i)
	{
		// The segment of the viewport is between v0 and v1
		Vec2d vertex0 = viewportVertices[i];
		Vec2d vertex1 = viewportVertices[(i+1)%viewportVertices.size()];
		getPslow(resultsParallels, prj, vertex0, vertex1, gridStepParallelDeg, getLatFrom2dPos, getLonFrom2dPos);
		getPslow(resultsMeridians, prj, vertex0, vertex1, gridStepMeridianDeg, getLonFrom2dPosLat180, getLatFrom2dPosLat180);
	}

	for (map<int, map<double, Vec2d> >::const_iterator iter=resultsParallels.begin(); iter!=resultsParallels.end(); ++iter)
	{
		if (iter->second.size()%2!=0)
		{
			cerr << "Error parallel "<< iter->first/DEGREE_MAS << " " << iter->second.size() << endl;
//			Vec3d vv;
//			prj->unProject(iter->second.begin()->second[0], iter->second.begin()->second[1], vv);
//			prj->drawParallel(vv, 2.*M_PI, 48);
		}
		else
		{
			map<double, Vec2d>::const_iterator ii = iter->second.begin();
			if (!isParallelEntering(prj, iter->second.begin()->second, iter->first/RADIAN_MAS))
			{
				++ii;
			}
			
			Vec3d vv;
			double size;
			for (unsigned int i=0;i<iter->second.size()/2;++i)
			{
				prj->unProject(ii->second[0], ii->second[1], vv);
				double lon = ii->first;
				++ii;
				if (ii==iter->second.end())
					size = iter->second.begin()->first - lon;
				else
					size = ii->first - lon;
				if (size<0) size+=360.;
				prj->drawParallel(vv, size*M_PI/180., true, &font);
				++ii;
			}
		}
	}
	
	// Draw the parallels which didn't intersect the viewport but are in the screen
	// This can only happen for parallels around the poles fully included in the viewport (At least I hope!)
	Vec3d win;
	prj->project(Vec3d(0,0,1), win);
	if (prj->check_in_viewport(win))
	{
		const double lastLat = (--resultsParallels.end())->first/DEGREE_MAS;
		for (double lat=lastLat+gridStepParallelDeg;lat<90.;lat+=gridStepParallelDeg)
		{
			Vec3d vv(std::cos(lat*M_PI/180.), 0, std::sin(lat*M_PI/180.));
			prj->drawParallel(vv, 2.*M_PI);
		}
	}
	prj->project(Vec3d(0,0,-1), win);
	if (prj->check_in_viewport(win))
	{
		const double lastLat = resultsParallels.begin()->first/DEGREE_MAS;
		for (double lat=lastLat-gridStepParallelDeg;lat>-90.;lat-=gridStepParallelDeg)
		{
			Vec3d vv(std::cos(lat*M_PI/180.), 0, std::sin(lat*M_PI/180.));
			prj->drawParallel(vv, 2.*M_PI);
		}
	}
	
	// Draw meridians
	
	// Discriminate meridian categories
	map<int, map<double, Vec2d> > resultsMeridiansOrdered;
	for (map<int, map<double, Vec2d> >::const_iterator iter=resultsMeridians.begin(); iter!=resultsMeridians.end(); ++iter)
	{
		for (map<double, Vec2d>::const_iterator i=iter->second.begin();i!=iter->second.end();++i)
		{
			if (i->first>180.)
				resultsMeridiansOrdered[(int)(180.*DEGREE_MAS)-iter->first][i->first]=i->second;
			else
				resultsMeridiansOrdered[iter->first][i->first]=i->second;
		}
	}
	
	for (map<int, map<double, Vec2d> >::const_iterator iter=resultsMeridiansOrdered.begin(); iter!=resultsMeridiansOrdered.end(); ++iter)
	{
//		cerr << "------- lon180=" << iter->first/DEGREE_MAS << "--------" << endl;
//		for (map<double, Vec2d>::const_iterator k = iter->second.begin();k!=iter->second.end();++k)
//		{
//			cerr << k->second << " lat=" << k->first << (isMeridianEnteringLat180(prj, k->second, iter->first/RADIAN_MAS)?" *":"") << endl;
//		}
		
		if (iter->second.size()%2!=0)
		{
			cerr << "Error meridian " << iter->first/DEGREE_MAS << " " << iter->second.size() << endl;
		}
		else
		{
			map<double, Vec2d>::const_iterator ii = iter->second.begin();
			if (!isMeridianEnteringLat180(prj, iter->second.begin()->second, iter->first/RADIAN_MAS))
			{
				++ii;
			}
			
			Vec3d vv;
			double size;
			for (unsigned int i=0;i<iter->second.size()/2;++i)
			{
				prj->unProject(ii->second[0], ii->second[1], vv);
				double lon = ii->first;
				++ii;
				if (ii==iter->second.end())
					size = iter->second.begin()->first - lon;
				else
					size = ii->first - lon;
				if (size<0) size+=360.;
				prj->drawMeridian(vv, size*M_PI/180., true, &font);
				++ii;
			}
		}
	
		// Debug, draw a cross for all the points
//		for (map<double, Vec2d>::const_iterator i=iter->second.begin();i!=iter->second.end();++i)
//		{
//			if (isMeridianEnteringLat180(prj, i->second, iter->first/RADIAN_MAS))
//			{
//				glColor3f(1,1,0);
//			}
//			else
//			{
//				glColor3f(0,0,1);
//			}
//			glBegin(GL_LINES);
//				glVertex2f(i->second[0]-30,i->second[1]);
//				glVertex2f(i->second[0]+30,i->second[1]);
//				glVertex2f(i->second[0],i->second[1]-30);
//				glVertex2f(i->second[0],i->second[1]+30);
//			glEnd();
//			ostringstream oss;
//			oss << iter->first/DEGREE_MAS;
//			font.print(i->second[0],i->second[1]+30,oss.str());
//			font.print(i->second[0],i->second[1]-30,oss.str());
//			
//		}
	}
	
	// Draw meridian zero which can't be found by the normal algo..
	Vec3d vv(1,0,0);
	prj->drawMeridian(vv, 2.*M_PI, true, &font);

}


SkyLine::SkyLine(SKY_LINE_TYPE _line_type, double _radius, unsigned int _nb_segment) :
		radius(_radius), nb_segment(_nb_segment), color(0.f, 0.f, 1.f), fontSize(1.),
font(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize))
{
	float inclinaison = 0.f;
	line_type = _line_type;

	switch (line_type)
		{
		case LOCAL : frameType = Projector::FRAME_LOCAL; break;
		case MERIDIAN : frameType = Projector::FRAME_LOCAL;
			inclinaison = 90; break;
		case ECLIPTIC : frameType = Projector::FRAME_J2000;
			inclinaison = 23.4392803055555555556; break;
		case EQUATOR : frameType = Projector::FRAME_EARTH_EQU; break;
		default : frameType = Projector::FRAME_EARTH_EQU;
	}

	Mat4f r = Mat4f::xrotation(inclinaison*M_PI/180.f);

	// Ecliptic month labels need to be redone
	// correct for month labels
	// TODO: can make this more accurate
	//	if(line_type == ECLIPTIC ) r = r * Mat4f::zrotation(-77.9*M_PI/180.);

	// Points to draw along the circle
	points = new Vec3f[nb_segment+1];
	for (unsigned int i=0;i<nb_segment+1;++i)
	{
		StelUtils::sphe_to_rect((float)i/(nb_segment)*2.f*M_PI, 0.f, points[i]);
		points[i] *= radius;
		points[i].transfo4d(r);
	}
}

SkyLine::~SkyLine()
{
	delete [] points;
	points = NULL;
}

void SkyLine::setFontSize(double newFontSize)
{
	fontSize = newFontSize;
	font = StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize);
}

void SkyLine::draw(Projector *prj,const Navigator *nav) const
{
	if (!fader.getInterstate()) return;

	Vec3d pt1;
	Vec3d pt2;

	glColor4f(color[0], color[1], color[2], fader.getInterstate());
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

  if (line_type == ECLIPTIC) {
      // special drawing of the ecliptic line
    const Mat4d m = nav->getHomePlanet()->getRotEquatorialToVsop87().transpose();
    const bool draw_labels = nav->getHomePlanet()->getEnglishName()=="Earth";
       // start labeling from the vernal equinox
    const double corr = draw_labels ? (atan2(m.r[4],m.r[0]) - 3*M_PI/6) : 0.0;
    Vec3d point(radius*cos(corr),radius*sin(corr),0.0);
    point.transfo4d(m);
    
    prj->setCurrentFrame(Projector::FRAME_EARTH_EQU);
    
    bool prev_on_screen = prj->project(point,pt1);
    for (unsigned int i=1;i<nb_segment+1;++i) {
      const double phi = corr+2*i*M_PI/nb_segment;
      Vec3d point(radius*cos(phi),radius*sin(phi),0.0);
      point.transfo4d(m);
      const bool on_screen = prj->project(point,pt2);
      if (on_screen && prev_on_screen) {
        const double dx = pt2[0]-pt1[0];
        const double dy = pt2[1]-pt1[1];
        const double dq = dx*dx+dy*dy;
        if (dq < 1024*1024) {
		  glBegin (GL_LINES);
		    glVertex2f(pt2[0],pt2[1]);
		    glVertex2f(pt1[0],pt1[1]);
       	  glEnd();
        }
		if (draw_labels && (i+2) % 4 == 0) {

			const double d = sqrt(dq);

			double angle = acos((pt1[1]-pt2[1])/d);
			if( pt1[0] < pt2[0] ) {
				angle *= -1;
			}

			// draw text label
			std::ostringstream oss;	

			oss << (i+3)/4;

			glPushMatrix();
			glTranslatef(pt2[0],pt2[1],0);
			glRotatef(-90+angle*180./M_PI,0,0,-1);

			glEnable(GL_TEXTURE_2D);

			font.print(0,-2,oss.str());
			glPopMatrix();
			glDisable(GL_TEXTURE_2D);

		}
      }
      prev_on_screen = on_screen;
      pt1 = pt2;
    }
  } else {

	prj->setCurrentFrame(frameType);
	for (unsigned int i=0;i<nb_segment;++i)
	{
		if (prj->project(points[i], pt1) && prj->project(points[i+1], pt2))
		{
          const double dx = pt1[0]-pt2[0];
          const double dy = pt1[1]-pt2[1];
          const double dq = dx*dx+dy*dy;
          if (dq < 1024*1024) {

			double angle;

			// TODO: allow for other numbers of meridians and parallels without
			// screwing up labels?

			glBegin (GL_LINES);
				glVertex2f(pt1[0],pt1[1]);
				glVertex2f(pt2[0],pt2[1]);
       		glEnd();


			if(line_type == MERIDIAN) {
				const double d = sqrt(dq);
				  
				angle = acos((pt1[1]-pt2[1])/d);
				if( pt1[0] < pt2[0] ) {
					angle *= -1;
				}

				// draw text label
				std::ostringstream oss;	
				
				if(i<=8) oss << (i+1)*10;
				else if(i<=16) {
					oss << (17-i)*10;
					angle += M_PI;
				}
				else oss << "";
				
				glPushMatrix();
				glTranslatef(pt2[0],pt2[1],0);
				glRotatef(180+angle*180./M_PI,0,0,-1);
				
				glBegin (GL_LINES);
				glVertex2f(-3,0);
				glVertex2f(3,0);
				glEnd();
				glEnable(GL_TEXTURE_2D);

				font.print(2,-2,oss.str());
				glPopMatrix();
				glDisable(GL_TEXTURE_2D);

			}

				  
			if(line_type == EQUATOR && (i+1) % 2 == 0) {

				const double d = sqrt(dq);
				  
				angle = acos((pt1[1]-pt2[1])/d);
				if( pt1[0] < pt2[0] ) {
					angle *= -1;
				}

				// draw text label
				std::ostringstream oss;	

				if((i+1)/2 == 24) oss << "0h";
				else oss << (i+1)/2 << "h";

				glPushMatrix();
				glTranslatef(pt2[0],pt2[1],0);
				glRotatef(180+angle*180./M_PI,0,0,-1);
				
				glBegin (GL_LINES);
				glVertex2f(-3,0);
				glVertex2f(3,0);
				glEnd();
				glEnable(GL_TEXTURE_2D);

				font.print(2,-2,oss.str());
				glPopMatrix();
				glDisable(GL_TEXTURE_2D);

			}

		  }

		}
	}
  }
}


GridLinesMgr::GridLinesMgr()
{
	dependenciesOrder["draw"]="stars";
	equ_grid = new SkyGrid(Projector::FRAME_EARTH_EQU);
	azi_grid = new SkyGrid(Projector::FRAME_LOCAL);
	equator_line = new SkyLine(SkyLine::EQUATOR);
	ecliptic_line = new SkyLine(SkyLine::ECLIPTIC);
	meridian_line = new SkyLine(SkyLine::MERIDIAN, 1, 36);
}

GridLinesMgr::~GridLinesMgr()
{
	delete equ_grid;
	delete azi_grid;
	delete equator_line;
	delete ecliptic_line;
	delete meridian_line;
}

void GridLinesMgr::init(const InitParser& conf, LoadingBar& lb)
{
	setFlagAzimutalGrid(conf.get_boolean("viewing:flag_azimutal_grid"));
	setFlagEquatorGrid(conf.get_boolean("viewing:flag_equatorial_grid"));
	setFlagEquatorLine(conf.get_boolean("viewing:flag_equator_line"));
	setFlagEclipticLine(conf.get_boolean("viewing:flag_ecliptic_line"));
	setFlagMeridianLine(conf.get_boolean("viewing:flag_meridian_line"));
}	
	
void GridLinesMgr::update(double deltaTime)
{
	// Update faders
	equ_grid->update(deltaTime);
	azi_grid->update(deltaTime);
	equator_line->update(deltaTime);
	ecliptic_line->update(deltaTime);
	meridian_line->update(deltaTime);
}

double GridLinesMgr::draw(Projector *prj, const Navigator *nav, ToneReproducer *eye)
{
	// Draw the equatorial grid
	equ_grid->draw(prj);
	// Draw the altazimutal grid
	azi_grid->draw(prj);
	// Draw the celestial equator line
	equator_line->draw(prj,nav);
	// Draw the ecliptic line
	ecliptic_line->draw(prj,nav);
	// Draw the meridian line
	meridian_line->draw(prj,nav);
	return 0.;
}

void GridLinesMgr::setColorScheme(const InitParser& conf, const std::string& section)
{
	// Load colors from config file
	string defaultColor = conf.get_str(section,"default_color");
	setColorEquatorGrid(StelUtils::str_to_vec3f(conf.get_str(section,"equatorial_color", defaultColor)));
	setColorAzimutalGrid(StelUtils::str_to_vec3f(conf.get_str(section,"azimuthal_color", defaultColor)));
	setColorEquatorLine(StelUtils::str_to_vec3f(conf.get_str(section,"equator_color", defaultColor)));
	setColorEclipticLine(StelUtils::str_to_vec3f(conf.get_str(section,"ecliptic_color", defaultColor)));
	setColorMeridianLine(StelUtils::str_to_vec3f(conf.get_str(section,"meridian_color", defaultColor)));
}

//! Set flag for displaying Azimutal Grid
void GridLinesMgr::setFlagAzimutalGrid(bool b) {azi_grid->setFlagshow(b);}
//! Get flag for displaying Azimutal Grid
bool GridLinesMgr::getFlagAzimutalGrid(void) const {return azi_grid->getFlagshow();}
Vec3f GridLinesMgr::getColorAzimutalGrid(void) const {return azi_grid->getColor();}

//! Set flag for displaying Equatorial Grid
void GridLinesMgr::setFlagEquatorGrid(bool b) {equ_grid->setFlagshow(b);}
//! Get flag for displaying Equatorial Grid
bool GridLinesMgr::getFlagEquatorGrid(void) const {return equ_grid->getFlagshow();}
Vec3f GridLinesMgr::getColorEquatorGrid(void) const {return equ_grid->getColor();}

//! Set flag for displaying Equatorial Line
void GridLinesMgr::setFlagEquatorLine(bool b) {equator_line->setFlagshow(b);}
//! Get flag for displaying Equatorial Line
bool GridLinesMgr::getFlagEquatorLine(void) const {return equator_line->getFlagshow();}
Vec3f GridLinesMgr::getColorEquatorLine(void) const {return equator_line->getColor();}

//! Set flag for displaying Ecliptic Line
void GridLinesMgr::setFlagEclipticLine(bool b) {ecliptic_line->setFlagshow(b);}
//! Get flag for displaying Ecliptic Line
bool GridLinesMgr::getFlagEclipticLine(void) const {return ecliptic_line->getFlagshow();}
Vec3f GridLinesMgr::getColorEclipticLine(void) const {return ecliptic_line->getColor();}


//! Set flag for displaying Meridian Line
void GridLinesMgr::setFlagMeridianLine(bool b) {meridian_line->setFlagshow(b);}
//! Get flag for displaying Meridian Line
bool GridLinesMgr::getFlagMeridianLine(void) const {return meridian_line->getFlagshow();}
Vec3f GridLinesMgr::getColorMeridianLine(void) const {return meridian_line->getColor();}

void GridLinesMgr::setColorAzimutalGrid(const Vec3f& v) { azi_grid->setColor(v);}
void GridLinesMgr::setColorEquatorGrid(const Vec3f& v) { equ_grid->setColor(v);}
void GridLinesMgr::setColorEquatorLine(const Vec3f& v) { equator_line->setColor(v);}
void GridLinesMgr::setColorEclipticLine(const Vec3f& v) { ecliptic_line->setColor(v);}
void GridLinesMgr::setColorMeridianLine(const Vec3f& v) { meridian_line->setColor(v);}
