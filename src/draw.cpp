/*
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�eau
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

#include "draw.h"
#include "s_texture.h"
#include "stel_utility.h"

// rms added color as parameter
SkyGrid::SkyGrid(SKY_GRID_TYPE grid_type, Vec3f grid_color, const string& font_file, const string& tex_file, unsigned int _nb_meridian, unsigned int _nb_parallel, double _radius,
	unsigned int _nb_alt_segment, unsigned int _nb_azi_segment) :
	nb_meridian(_nb_meridian), nb_parallel(_nb_parallel), 	radius(_radius),
	nb_alt_segment(_nb_alt_segment), nb_azi_segment(_nb_azi_segment)
{

	font = new s_font(12, tex_file, font_file);
	if (!font)
	{
		printf("Can't create skygrid font\n");
		exit(-1);
	}


	transparent_top = true;
	color = grid_color; // rms Vec3f(0.2,0.2,0.2);
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
			sphe_to_rect((float)nm/(nb_meridian)*2.f*M_PI,
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
			sphe_to_rect((float)i/(nb_azi_segment)*2.f*M_PI,
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
	
	if (font) delete font;
	font = NULL;

}

void SkyGrid::draw(const Projector* prj) const
{
	glColor3fv(color);
	glDisable(GL_TEXTURE_2D);
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	static Vec3d pt1;
	static Vec3d pt2;

	prj->set_orthographic_projection();	// set 2D coordinate
	glEnable(GL_BLEND);

	// Draw meridians
	for (unsigned int nm=0;nm<nb_meridian;++nm)
	{
		if (transparent_top)	// Transparency for the first and last points
		{
			if ((prj->*proj_func)(alt_points[nm][0], pt1) &&
				(prj->*proj_func)(alt_points[nm][1], pt2) )
			{
				glColor4f(color[0],color[1],color[2],0.f);

				glBegin (GL_LINES);
					glVertex2f(pt1[0],pt1[1]);
					glColor3fv(color);
					glVertex2f(pt2[0],pt2[1]);
        		glEnd();
			}

			glColor3fv(color);

			for (unsigned int i=1;i<nb_alt_segment-1;++i)
			{
				if ((prj->*proj_func)(alt_points[nm][i], pt1) &&
					(prj->*proj_func)(alt_points[nm][i+1], pt2) )
				{
				  

				  glBegin(GL_LINES);
				  glVertex2f(pt1[0],pt1[1]);
				  glVertex2f(pt2[0],pt2[1]);
				  glEnd();
				  
				  
				  static char str[255];	// TODO use c++ string 
				  
				  glEnable(GL_TEXTURE_2D);

				  double angle;
				  double d;

				  // TODO: allow for other numbers of meridians and parallels without
				  // screwing up labels?
				  if( gtype == EQUATORIAL && i == 8 ) {

				    // draw labels along equator for RA
				    d = sqrt( (pt1[0]-pt2[0])*(pt1[0]-pt2[0]) + (pt1[1]-pt2[1])*(pt1[1]-pt2[1]) );
				  
				    angle = acos((pt1[1]-pt2[1])/d);
				    if( pt1[0] < pt2[0] ) {
				      angle *= -1;
				    }

				    sprintf( str, "%dh", nm);

				    prj->set_orthographic_projection();
				  
				    glTranslatef(pt2[0],pt2[1],0);
				    glRotatef(90+angle*180./M_PI,0,0,-1);
				    font->print(2,-2,str);

				    prj->reset_perspective_projection();


				  } else if (nm % 8 == 0 && i != 16) {
					  
				    d = sqrt( (pt1[0]-pt2[0])*(pt1[0]-pt2[0]) + (pt1[1]-pt2[1])*(pt1[1]-pt2[1]) );
				  
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
				    font->print(2,-2,str);
				    prj->reset_perspective_projection();

				  }
				  glDisable(GL_TEXTURE_2D);
				  
				}


			}

			if ((prj->*proj_func)(alt_points[nm][nb_alt_segment-1], pt1) &&
				(prj->*proj_func)(alt_points[nm][nb_alt_segment], pt2) )
			{
				glColor3fv(color);
				glBegin (GL_LINES);
					glVertex2f(pt1[0],pt1[1]);
					glColor4f(color[0],color[1],color[2],0.f);
					glVertex2f(pt2[0],pt2[1]);
        		glEnd();
			}

		}
		else	// No transparency
		{
			glDisable(GL_BLEND);
			glColor3fv(color);
			for (unsigned int i=0;i<nb_alt_segment;++i)
			{
				if ((prj->*proj_func)(alt_points[nm][i], pt1) &&
					(prj->*proj_func)(alt_points[nm][i+1], pt2) )
				{
					glBegin (GL_LINES);
						glVertex2f(pt1[0],pt1[1]);
						glVertex2f(pt2[0],pt2[1]);
        			glEnd();
				}
			}
		}
	}

	// Draw parallels
	glColor3fv(color);
	glDisable(GL_BLEND);
	for (unsigned int np=0;np<nb_parallel;++np)
	{
		for (unsigned int i=0;i<nb_azi_segment;++i)
		{
			if ((prj->*proj_func)(azi_points[np][i], pt1) &&
				(prj->*proj_func)(azi_points[np][i+1], pt2) )
			{
				glBegin (GL_LINES);
					glVertex2f(pt1[0],pt1[1]);
					glVertex2f(pt2[0],pt2[1]);
        		glEnd();
			}
		}
	}

	prj->reset_perspective_projection();
}


SkyLine::SkyLine(SKY_LINE_TYPE line_type, Vec3f line_color, double _radius, unsigned int _nb_segment) :
	radius(_radius), nb_segment(_nb_segment)
{
        color = line_color;  
	float inclinaison = 0.f;
	switch (line_type)
	{
		case LOCAL : proj_func = &Projector::project_local; break;
		case ECLIPTIC : proj_func = &Projector::project_earth_equ;
						inclinaison = 23.45f; break;
		case EQUATOR : proj_func = &Projector::project_earth_equ; break;
		default : proj_func = &Projector::project_earth_equ;
	}

	Mat4f r = Mat4f::xrotation(inclinaison*M_PI/180.f);

	// Points to draw along the circle
	points = new Vec3f[nb_segment+1];
	for (unsigned int i=0;i<nb_segment+1;++i)
	{
		sphe_to_rect((float)i/(nb_segment)*2.f*M_PI, 0.f, points[i]);
		points[i] *= radius;
		points[i].transfo4d(r);
	}
}

SkyLine::~SkyLine()
{
	delete [] points;
	points = NULL;
}

void SkyLine::draw(const Projector* prj) const
{
	static Vec3d pt1;
	static Vec3d pt2;

	glColor3fv(color);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	prj->set_orthographic_projection();	// set 2D coordinate

	for (unsigned int i=0;i<nb_segment;++i)
	{
		if ((prj->*proj_func)(points[i], pt1) &&
			(prj->*proj_func)(points[i+1], pt2) )
		{
			glBegin (GL_LINES);
				glVertex2f(pt1[0],pt1[1]);
				glVertex2f(pt2[0],pt2[1]);
       		glEnd();
		}
	}

	prj->reset_perspective_projection();
}


Cardinals::Cardinals(const string& font_file, const string& tex_file, double size, double _radius) :
	radius(radius), font(NULL)
{
	font = new s_font(size, tex_file, font_file);
	if (!font)
	{
		printf("Can't create cardinalFont\n");
		exit(-1);
	}
	color = Vec3f(0.6,0.2,0.2);
}

Cardinals::~Cardinals()
{
    if (font) delete font;
	font = NULL;
}


// Draw the cardinals points : N S E W
// handles special cases at poles
void Cardinals::draw(const Projector* prj, double latitude, bool gravityON) const
{

  // direction text
  string d[4];

  // Improvement for gettext translation
  static string sNorth, sSouth, sEast, sWest;
  
  // North
  sNorth = _("N");
  // South
  sSouth = _("S");
  // East
  sEast = _("E");
  // West
  sWest = _("W");
  
  d[0] = sNorth;
  d[1] = sSouth;
  d[2] = sEast;
  d[3] = sWest;

  // fun polar special cases
  if(latitude ==  90.0 ) d[0] = d[1] = d[2] = d[3] = sSouth;
  if(latitude == -90.0 ) d[0] = d[1] = d[2] = d[3] = sNorth;

    glColor3fv(color);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Vec3f pos;
	Vec3d xy;

	prj->set_orthographic_projection();

	float shift = font->getStrLen(sNorth)/2;

	if (gravityON)
	{
		// N for North
		pos.set(-1.f, 0.f, 0.22f);
		if (prj->project_local(pos,xy)) prj->print_gravity180(font, xy[0], xy[1], d[0], -shift, -shift);

		// S for South
		pos.set(1.f, 0.f, 0.22f);
		if (prj->project_local(pos,xy)) prj->print_gravity180(font, xy[0], xy[1], d[1], -shift, -shift);

		// E for East
		pos.set(0.f, 1.f, 0.22f);
		if (prj->project_local(pos,xy)) prj->print_gravity180(font, xy[0], xy[1], d[2], -shift, -shift);

		// W for West
		pos.set(0.f, -1.f, 0.22f);
		if (prj->project_local(pos,xy)) prj->print_gravity180(font, xy[0], xy[1], d[3], -shift, -shift);
	}
	else
	{
		// N for North
		pos.set(-1.f, 0.f, 0.f);
		if (prj->project_local(pos,xy)) font->print(xy[0]-shift, xy[1]-shift, d[0]);

		// S for South
		pos.set(1.f, 0.f, 0.f);
		if (prj->project_local(pos,xy)) font->print(xy[0]-shift, xy[1]-shift, d[1]);

		// E for East
		pos.set(0.f, 1.f, 0.f);
		if (prj->project_local(pos,xy)) font->print(xy[0]-shift, xy[1]-shift, d[2]);

		// W for West
		pos.set(0.f, -1.f, 0.f);
		if (prj->project_local(pos,xy)) font->print(xy[0]-shift, xy[1]-shift, d[3]);
	}

	prj->reset_perspective_projection();
}


// Class which manages the displaying of the Milky Way
MilkyWay::MilkyWay(const string& tex_file, double _radius) : radius(_radius)
{
	tex = new s_texture(tex_file,TEX_LOAD_TYPE_PNG_SOLID_REPEAT);

	// Scotopic color = 0.25, 0.25 in xyY mode. Global stars luminance ~= 0.001 cd/m^2
	color = Vec3f(0.25f, 0.25f, 0.02f/tex->get_average_luminance());
}

MilkyWay::~MilkyWay()
{
	if (tex) delete tex;
	tex = NULL;
}

void MilkyWay::draw(tone_reproductor * eye, const Projector* prj, const navigator* nav) const
{
	static Vec3f c;
	c = color;
	eye->xyY_to_RGB(c);
	glColor3fv(c);

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, tex->getID());

	prj->sSphere(radius,20,20,
		     nav->get_prec_earth_equ_to_eye_mat()*
		     Mat4d::xrotation(M_PI/180*23)*
		     Mat4d::yrotation(M_PI/180*120)*
		     Mat4d::zrotation(M_PI/180*7), 1);

	glDisable(GL_CULL_FACE);
}


// Draw a point... (used for tests)
void DrawPoint(float X,float Y,float Z)
{       
	glColor3f(0.8, 1.0, 0.8);
	glDisable(GL_TEXTURE_2D);
	//glEnable(GL_BLEND);
	glPointSize(20.);
	glBegin(GL_POINTS);
		glVertex3f(X,Y,Z);
	glEnd();
}
