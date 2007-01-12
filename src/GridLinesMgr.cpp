#include "GridLinesMgr.hpp"

#include "StelApp.hpp"
#include "Navigator.hpp"
#include "Translator.hpp"
#include "Projector.hpp"
#include "LoadingBar.hpp"
#include "Fader.hpp"
#include "planet.h"

// Class which manages a grid to display in the sky
class SkyGrid
{
public:
	enum SKY_GRID_TYPE
	{
		EQUATORIAL,
		ALTAZIMUTAL
	};
	// Create and precompute positions of a SkyGrid
	SkyGrid(SKY_GRID_TYPE grid_type = EQUATORIAL, unsigned int _nb_meridian = 24, unsigned int _nb_parallel = 17,
	 double _radius = 1., unsigned int _nb_alt_segment = 18, unsigned int _nb_azi_segment = 50);
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
	double radius;
	unsigned int nb_alt_segment;
	unsigned int nb_azi_segment;
	bool transparent_top;
	Vec3f color;
	Vec3f** alt_points;
	Vec3f** azi_points;
	bool (Projector::*proj_func)(const Vec3d&, Vec3d&) const;
	double fontSize;
	SFont& font;
	SKY_GRID_TYPE gtype;
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
	void draw(const Projector *prj,const Navigator *nav) const;
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
	bool (Projector::*proj_func)(const Vec3d&, Vec3d&) const;
	LinearFader fader;
	double fontSize;
	SFont& font;
};


// rms added color as parameter
SkyGrid::SkyGrid(SKY_GRID_TYPE grid_type, unsigned int _nb_meridian, unsigned int _nb_parallel, double _radius,
	unsigned int _nb_alt_segment, unsigned int _nb_azi_segment) :
	nb_meridian(_nb_meridian), nb_parallel(_nb_parallel), 	radius(_radius),
	nb_alt_segment(_nb_alt_segment), nb_azi_segment(_nb_azi_segment), color(0.2,0.2,0.2), fontSize(12),
	font(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize))
{
	transparent_top = true;
	gtype = grid_type;
	switch (grid_type)
	{
		case ALTAZIMUTAL : proj_func = &Projector::project_local; break;
		case EQUATORIAL : proj_func = &Projector::project_earth_equ; break;
		default : proj_func = &Projector::project_earth_equ;
	}

	// Alt points are the points to draw along the meridian
	alt_points = new Vec3f*[nb_meridian];
	for (unsigned int nm=0;nm<nb_meridian;++nm)
	{
		alt_points[nm] = new Vec3f[nb_alt_segment+1];
		for (unsigned int i=0;i<nb_alt_segment+1;++i)
		{
			StelUtils::sphe_to_rect((float)nm/(nb_meridian)*2.f*M_PI,
				(float)i/nb_alt_segment*M_PI-M_PI_2, alt_points[nm][i]);
			alt_points[nm][i] *= radius;
		}
	}

	// Alt points are the points to draw along the meridian
	azi_points = new Vec3f*[nb_parallel];
	for (unsigned int np=0;np<nb_parallel;++np)
	{
		azi_points[np] = new Vec3f[nb_azi_segment+1];
		for (unsigned int i=0;i<nb_azi_segment+1;++i)
		{
			StelUtils::sphe_to_rect((float)i/(nb_azi_segment)*2.f*M_PI,
				(float)(np+1)/(nb_parallel+1)*M_PI-M_PI_2, azi_points[np][i]);
			azi_points[np][i] *= radius;
		}
	}
}

SkyGrid::~SkyGrid()
{
	for (unsigned int nm=0;nm<nb_meridian;++nm)
	{
		delete [] alt_points[nm];
	}
	delete [] alt_points;
	
	for (unsigned int np=0;np<nb_parallel;++np)
	{
		delete [] azi_points[np];
	}
	delete [] azi_points;
}

void SkyGrid::setFontSize(double newFontSize)
{
	fontSize = newFontSize;
	font = StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize);
}

void SkyGrid::draw(const Projector* prj) const
{
	if (!fader.getInterstate()) return;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	
	Vec3d pt1;
	Vec3d pt2;

	prj->set_orthographic_projection();	// set 2D coordinate

	// Draw meridians
	for (unsigned int nm=0;nm<nb_meridian;++nm)
	{
		if (transparent_top)	// Transparency for the first and last points
		{
			if ((prj->*proj_func)(alt_points[nm][0], pt1) &&
				(prj->*proj_func)(alt_points[nm][1], pt2) )
			{
              const double dx = pt1[0]-pt2[0];
              const double dy = pt1[1]-pt2[1];
              const double dq = dx*dx+dy*dy;
              if (dq < 1024*1024) {
				glColor4f(color[0],color[1],color[2],0.3f);

				glBegin (GL_LINES);
					glVertex2f(pt1[0],pt1[1]);
					glColor4f(color[0],color[1],color[2],fader.getInterstate());
					glVertex2f(pt2[0],pt2[1]);
        		glEnd();
              }
			}

			glColor4f(color[0],color[1],color[2],fader.getInterstate());

			for (unsigned int i=1;i<nb_alt_segment-1;++i)
			{
				if ((prj->*proj_func)(alt_points[nm][i], pt1) &&
					(prj->*proj_func)(alt_points[nm][i+1], pt2) )
				{
                  const double dx = pt1[0]-pt2[0];
                  const double dy = pt1[1]-pt2[1];
                  const double dq = dx*dx+dy*dy;
                  if (dq < 1024*1024) {
				    glBegin(GL_LINES);
				    glVertex2f(pt1[0],pt1[1]);
				    glVertex2f(pt2[0],pt2[1]);
				    glEnd();

				    static char str[255];	// TODO use c++ string 

				    glEnable(GL_TEXTURE_2D);

				    double angle;

				    // TODO: allow for other numbers of meridians and parallels without
				    // screwing up labels?
				    if( gtype == EQUATORIAL && i == 8 ) {

				      // draw labels along equator for RA
				      const double d = sqrt(dq);

				      angle = acos((pt1[1]-pt2[1])/d);
				      if( pt1[0] < pt2[0] ) {
				        angle *= -1;
				      }

				      sprintf( str, "%dh", nm);

				      prj->set_orthographic_projection();

				      glTranslatef(pt2[0],pt2[1],0);
				      glRotatef(90+angle*180./M_PI,0,0,-1);
				      font.print(2,-2,str);

				      prj->reset_perspective_projection();


				    } else if (nm % 8 == 0 && i != 16) {

				      const double d = sqrt(dq);

				      angle = acos((pt1[1]-pt2[1])/d);
				      if( pt1[0] < pt2[0] ) {
				        angle *= -1;
				      }

				      sprintf( str, "%d", (i-8)*10);

				      if( gtype == ALTAZIMUTAL || 
					  (gtype == EQUATORIAL && i > 8)) {
				        angle += M_PI;
				      }

				      prj->set_orthographic_projection();

				      glTranslatef(pt2[0],pt2[1],0);
				      glRotatef(angle*180./M_PI,0,0,-1);
				      font.print(2,-2,str);
				      prj->reset_perspective_projection();

				    }
				    glDisable(GL_TEXTURE_2D);
				  }
				}


			}

			if ((prj->*proj_func)(alt_points[nm][nb_alt_segment-1], pt1) &&
				(prj->*proj_func)(alt_points[nm][nb_alt_segment], pt2) )
			{
              const double dx = pt1[0]-pt2[0];
              const double dy = pt1[1]-pt2[1];
              const double dq = dx*dx+dy*dy;
              if (dq < 1024*1024) {
				glColor4f(color[0],color[1],color[2],fader.getInterstate());
				glBegin (GL_LINES);
					glVertex2f(pt1[0],pt1[1]);
					glColor4f(color[0],color[1],color[2],0.3f);
					glVertex2f(pt2[0],pt2[1]);
        		glEnd();
              }
			}

		}
		else
		{
			glColor4f(color[0],color[1],color[2],fader.getInterstate());
			for (unsigned int i=0;i<nb_alt_segment;++i)
			{
				if ((prj->*proj_func)(alt_points[nm][i], pt1) &&
					(prj->*proj_func)(alt_points[nm][i+1], pt2) )
				{
                  const double dx = pt1[0]-pt2[0];
                  const double dy = pt1[1]-pt2[1];
                  const double dq = dx*dx+dy*dy;
                  if (dq < 1024*1024) {
					glBegin (GL_LINES);
						glVertex2f(pt1[0],pt1[1]);
						glVertex2f(pt2[0],pt2[1]);
        			glEnd();
                  }
				}
			}
		}
	}

	// Draw parallels
	glColor4f(color[0],color[1],color[2],fader.getInterstate());
	for (unsigned int np=0;np<nb_parallel;++np)
	{
		for (unsigned int i=0;i<nb_azi_segment;++i)
		{
			if ((prj->*proj_func)(azi_points[np][i], pt1) &&
				(prj->*proj_func)(azi_points[np][i+1], pt2) )
			{
              const double dx = pt1[0]-pt2[0];
              const double dy = pt1[1]-pt2[1];
              const double dq = dx*dx+dy*dy;
              if (dq < 1024*1024) {
				glBegin (GL_LINES);
					glVertex2f(pt1[0],pt1[1]);
					glVertex2f(pt2[0],pt2[1]);
        		glEnd();
              }
			}
		}
	}

	prj->reset_perspective_projection();
}


SkyLine::SkyLine(SKY_LINE_TYPE _line_type, double _radius, unsigned int _nb_segment) :
		radius(_radius), nb_segment(_nb_segment), color(0.f, 0.f, 1.f), fontSize(1.),
font(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize))
{
	float inclinaison = 0.f;
	line_type = _line_type;

	switch (line_type)
		{
		case LOCAL : proj_func = &Projector::project_local; break;
		case MERIDIAN : proj_func = &Projector::project_local; 
			inclinaison = 90; break;
		case ECLIPTIC : proj_func = &Projector::project_j2000;
			inclinaison = 23.4392803055555555556; break;
		case EQUATOR : proj_func = &Projector::project_earth_equ; break;
		default : proj_func = &Projector::project_earth_equ;
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

void SkyLine::draw(const Projector *prj,const Navigator *nav) const
{
	if (!fader.getInterstate()) return;

	Vec3d pt1;
	Vec3d pt2;

	glColor4f(color[0], color[1], color[2], fader.getInterstate());
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

	prj->set_orthographic_projection();	// set 2D coordinate

  if (line_type == ECLIPTIC) {
      // special drawing of the ecliptic line
    const Mat4d m = nav->getHomePlanet()->getRotEquatorialToVsop87().transpose();
    const bool draw_labels = nav->getHomePlanet()->getEnglishName()=="Earth";
       // start labeling from the vernal equinox
    const double corr = draw_labels ? (atan2(m.r[4],m.r[0]) - 3*M_PI/6) : 0.0;
    Vec3d point(radius*cos(corr),radius*sin(corr),0.0);
    point.transfo4d(m);
    bool prev_on_screen = prj->project_earth_equ(point,pt1);
    for (unsigned int i=1;i<nb_segment+1;++i) {
      const double phi = corr+2*i*M_PI/nb_segment;
      Vec3d point(radius*cos(phi),radius*sin(phi),0.0);
      point.transfo4d(m);
      const bool on_screen = prj->project_earth_equ(point,pt2);
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

	for (unsigned int i=0;i<nb_segment;++i)
	{
		if ((prj->*proj_func)(points[i], pt1) &&
			(prj->*proj_func)(points[i+1], pt2) )
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
	prj->reset_perspective_projection();
}


GridLinesMgr::GridLinesMgr()
{
	dependenciesOrder["draw"]="stars";
	equ_grid = new SkyGrid(SkyGrid::EQUATORIAL);
	azi_grid = new SkyGrid(SkyGrid::ALTAZIMUTAL);
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
