/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

extern s_texture * texIds[200];            // Common Textures

SkyGrid::SkyGrid(SKY_GRID_TYPE grid_type, unsigned int _nb_meridian, unsigned int _nb_parallel, double _radius,
	unsigned int _nb_alt_segment, unsigned int _nb_azi_segment) :
	nb_meridian(_nb_meridian), nb_parallel(_nb_parallel), 	radius(_radius),
	nb_alt_segment(_nb_alt_segment), nb_azi_segment(_nb_azi_segment)
{
	transparent_top = true;
	color = Vec3f(0.2,0.2,0.2);
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
			alt_points[np][i] *= radius;
		}
	}
}

SkyGrid::~SkyGrid()
{
	for (unsigned int nm=0;nm<nb_meridian;++nm)
	{
		delete alt_points[nm];
	}
	delete alt_points;
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

	// Draw meridians
	for (unsigned int nm=0;nm<nb_meridian;++nm)
	{
		if (transparent_top)	// Transparency for the first and last points
		{
			if ((prj->*proj_func)(alt_points[nm][0], pt1) &&
				(prj->*proj_func)(alt_points[nm][1], pt2) )
			{
				glColor4f(color[0],color[1],color[2],0.f);
				glEnable(GL_BLEND);
				glBegin (GL_LINES);
					glVertex2f(pt1[0],pt1[1]);
					glColor3fv(color);
					glVertex2f(pt2[0],pt2[1]);
        		glEnd();
			}

			glDisable(GL_BLEND);
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
				}
			}

			if ((prj->*proj_func)(alt_points[nm][nb_alt_segment-1], pt1) &&
				(prj->*proj_func)(alt_points[nm][nb_alt_segment], pt2) )
			{
				glColor3fv(color);
				glEnable(GL_BLEND);
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


SkyLine::SkyLine(SKY_LINE_TYPE line_type, double _radius, unsigned int _nb_segment) :
	radius(_radius), nb_segment(_nb_segment)
{
	color = Vec3f(0.2,0.2,0.6);
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
	delete points;
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


Cardinals::Cardinals(const char* font_file, const char* tex_file, double size, double _radius) :
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
void Cardinals::draw(const Projector* prj) const
{

    glColor3fv(color);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Vec3f pos;
	Vec3d xy;

	prj->set_orthographic_projection();

	// N for North
	pos.set(-1.f, 0.f, 0.f);
	if (prj->project_local(pos,xy)) font->print(xy[0], xy[1], "N");

	// S for South
	pos.set(1.f, 0.f, 0.f);
	if (prj->project_local(pos,xy)) font->print(xy[0], xy[1], "S");

	// E for East
	pos.set(0.f, 1.f, 0.f);
	if (prj->project_local(pos,xy)) font->print(xy[0], xy[1], "E");

	// W for West
	pos.set(0.f, -1.f, 0.f);
	if (prj->project_local(pos,xy)) font->print(xy[0], xy[1], "W");

	prj->reset_perspective_projection();
}


// Draw the milky way : used too to 'clear' the buffer
void DrawMilkyWay(tone_reproductor * eye, const Projector* prj, const navigator* nav)
{
	return;
	// Scotopic color = 0.25, 0.25 in xyY mode. Milky way luminance ~= 0.001 cd/m^2
	float c[3] = {1.25f, 1.25f, 1.01f};
	eye->xyY_to_RGB(c);
	glColor3fv(c);

	glPushMatrix();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, texIds[2]->getID());
	glRotatef(206,1,0,0);
	glRotatef(-16+90,0,1,0);
	glRotatef(-84,0,0,1);

	prj->sSphere(1.,40,40,nav->get_earth_equ_to_eye_mat(),1);

	glPopMatrix();
}


// Draw the horizon fog
void DrawFog(float sky_brightness)
{
	glBlendFunc(GL_ONE, GL_ONE);
	glPushMatrix();
	glColor3f(0.2f+0.2f*sky_brightness, 0.2f+0.2f*sky_brightness, 0.2f+0.2f*sky_brightness);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, texIds[3]->getID());
	glTranslatef(0,0,-0.8);
	GLUquadricObj * Horizon=gluNewQuadric();
	gluQuadricTexture(Horizon,GL_TRUE);
	gluCylinder(Horizon,10,10,2.5,16,1);
	gluDeleteQuadric(Horizon);
	glPopMatrix();
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


// Draw the mountains with a few pieces of a big texture
void DrawDecor(int nb, float sky_brightness)
{   
	glPushMatrix();
	glColor3f(sky_brightness, sky_brightness, sky_brightness);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	float delta=1.98915/nb; // almost egal to sin((float)(PI/8))/cos((float)(PI/8))*10/2;
	for (int ntex=0;ntex<4*nb;ntex++)
	{   
		glBindTexture(GL_TEXTURE_2D, texIds[31+ntex%4]->getID());
		for (int nDecoupe=0;nDecoupe<4;nDecoupe++)
		{
			glBegin(GL_QUADS );
				//Haut Gauche
				glTexCoord2f((float)nDecoupe/4,0.5f);
				glVertex3f(10.0f,delta,0.5f);
				//Bas Gauche
				glTexCoord2f((float)nDecoupe/4,0.0f);
				glVertex3f(10.0f,delta,-0.4f);
				//Bas Droit
				glTexCoord2f((float)nDecoupe/4+0.25,0.0f);
				glVertex3f(10.0f,-delta,-0.4f);
				//Haut Droit
				glTexCoord2f((float)nDecoupe/4+0.25,0.5f);
				glVertex3f(10.0f,-delta,0.5f);
			glEnd ();
			glRotatef(22.5/nb,0,0,-1);
		}
	}
	glPopMatrix();
}


// Draw the ground
void DrawGround(float sky_brightness)
{
	glPushMatrix();
	glColor3f(sky_brightness/2, sky_brightness/2, sky_brightness/2);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, texIds[1]->getID());
	glBegin (GL_QUADS);
		glTexCoord2f (0.0f,0.0f);
		glVertex3f (-10.0f, -10.0f, -0.2f);
		glTexCoord2f (0.8f, 0.0f);
		glVertex3f(-10.0f, 10.0f, -0.2f);
		glTexCoord2f (0.8f, 0.8f);
		glVertex3f( 10.0f, 10.0f, -0.2f);
		glTexCoord2f (0.0f, 0.8f);
		glVertex3f( 10.0f, -10.0f, -0.2f);
	glEnd ();
	glPopMatrix();
}

