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

// class used to manage groups of Nebulas

#include "nebula_mgr.h"
#include "stellarium.h"
#include "s_texture.h"
#include "s_font.h"
#include "navigator.h"
#include "stel_sdl.h"

#define RADIUS_NEB 1.

Nebula_mgr::Nebula_mgr(Vec3f cfont, Vec3f ccircle) : defaultfontcolor(cfont), defaultcirclecolor(ccircle)
{
}

Nebula_mgr::~Nebula_mgr()
{
	vector<Nebula *>::iterator iter;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++)
    {
		delete (*iter);
    }
    if (Nebula::tex_circle) delete Nebula::tex_circle;
	Nebula::tex_circle = NULL;
	if (Nebula::nebula_font) delete Nebula::nebula_font;
	Nebula::nebula_font = NULL;
}

// read from stream
int Nebula_mgr::read(const string& font_fileName, const string& fileName, int barx, int bary)
{
  char tmpstr[255];
  int total=0;

  printf("Loading nebulas data...\n");

	FILE * fic;
	fic = fopen(fileName.c_str(),"r");
    if (!fic)
	{
		printf("ERROR : Can't open nebula catalog %s\n",fileName.c_str());
		exit(-1);
	}

    // determine total number to be loaded for percent complete display
    while( fgets(tmpstr,255,fic) ) {
      total++;
    }
    rewind(fic);

    printf("Reading a total of %d nebulas\n", total);

    if (!Nebula::nebula_font) Nebula::nebula_font = new s_font(12.,"spacefont", font_fileName); // load Font
    if (!Nebula::nebula_font)
    {
	    printf("Can't create nebulaFont\n");
        exit(1);
    }

    Nebula::tex_circle = new s_texture("neb");   // Load circle texture

    int current=0;

    glDisable(GL_BLEND);
    glLineWidth(2);


    Nebula * e = NULL;
    for(;;)
    {

      // *** TESTING ONLY!
      //      if(current/total > .70 ) {
      //	sleep(1);
      //}
      glClear(GL_COLOR_BUFFER_BIT);
      sprintf(tmpstr, "Loading deep space objects: %d/%d", current, total);

      glColor3f(1,1,1);

	  glDisable(GL_TEXTURE_2D);
      glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              // Bottom Right
        glVertex3f(barx+302,bary+22, 0.0f);
        glTexCoord2i(0,0);              // Bottom Left
        glVertex3f(barx-2,bary+22, 0.0f);
        glTexCoord2i(1,1);              // Top Right
        glVertex3f(barx+302, bary-2,0.0f);
        glTexCoord2i(0,1);              // Top Left
        glVertex3f(barx-2,bary-2,0.0f);
      glEnd ();

      glColor3f(0.0f,0.0f,1.0f);

      glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              // Bottom Right
        glVertex3f(barx+300*current/total,bary+20, 0.0f);
        glTexCoord2i(0,0);              // Bottom Left
        glVertex3f(barx,bary+20, 0.0f);
        glTexCoord2i(1,1);              // Top Right
        glVertex3f(barx+300*current/total, bary,0.0f);
        glTexCoord2i(0,1);              // Top Left
        glVertex3f(barx,bary,0.0f);
      glEnd ();

      glColor3f(1,1,1);

	  glEnable(GL_TEXTURE_2D);
      Nebula::nebula_font->print(barx-2,bary+35, tmpstr);



      SDL_GL_SwapBuffers();				// And swap the buffers

      e = new Nebula;
      int temp = e->read(fic);
      if (temp==0) // eof
        {
	  if (e) delete e;
	  e = NULL;
	  break;
        }
      e->setFontColor(defaultfontcolor);
      e->setCircleColor(defaultcirclecolor);
      if (temp==1 || temp==2) neb_array.push_back(e);

      current++;

    }
    fclose(fic);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1);

    return 0;
}

// Draw all the Nebulaes
void Nebula_mgr::draw(int names_ON, Projector* prj, const navigator * nav, tone_reproductor* eye, bool _gravity_label, float max_mag_name, bool bright_nebulae)
{
	Nebula::gravity_label = _gravity_label;

	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    vector<Nebula *>::iterator iter;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++)
    {   
        // project in 2D to check if the nebula is in screen
		if ( !prj->project_earth_equ_check((*iter)->XYZ,(*iter)->XY) ) continue;

		prj->set_orthographic_projection();
		if ((*iter)->get_on_screen_size(prj, nav)>5) (*iter)->draw_tex(prj, eye, bright_nebulae && (*iter)->get_on_screen_size(prj, nav)>15 );
    	if (names_ON && (*iter)->mag <= max_mag_name)
    	{
			(*iter)->draw_name(prj);
			(*iter)->draw_circle(prj, nav);
    	}
		prj->reset_perspective_projection();
	}
}

// Look for a nebulae by XYZ coords
stel_object * Nebula_mgr::search(Vec3f Pos)
{
	Pos.normalize();
    vector<Nebula *>::iterator iter;
    Nebula * plusProche=NULL;
    float anglePlusProche=0.;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++)
    {
		if ((*iter)->XYZ[0]*Pos[0]+(*iter)->XYZ[1]*Pos[1]+(*iter)->XYZ[2]*Pos[2]>anglePlusProche)
        {
			anglePlusProche=(*iter)->XYZ[0]*Pos[0]+(*iter)->XYZ[1]*Pos[1]+(*iter)->XYZ[2]*Pos[2];
            plusProche=(*iter);
        }
    }
    if (anglePlusProche>RADIUS_NEB*0.999)
    {
        return plusProche;
    }
    else return NULL;
}

// Return a stl vector containing the nebulas located inside the lim_fov circle around position v
vector<stel_object*> Nebula_mgr::search_around(Vec3d v, double lim_fov)
{
	vector<stel_object*> result;
    v.normalize();
	double cos_lim_fov = cos(lim_fov * M_PI/180.);
	static Vec3d equPos;

    vector<Nebula*>::iterator iter = neb_array.begin();
    while (iter != neb_array.end())
    {
        equPos = (*iter)->XYZ;
		equPos.normalize();
		if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cos_lim_fov)
		{
			result.push_back(*iter);
		}
        iter++;
    }
	return result;
}
