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

Nebula_mgr::Nebula_mgr() : fontColor(0.4,0.3,0.5), circleColor(0.8,0.8,0.1)
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
int Nebula_mgr::read(const string& font_fileName, const string& fileName, int barx, int bary, LoadingBar& lb)
{
	printf(_("Loading nebulas data "));
	
	std::ifstream inf(fileName.c_str());
	if (!inf.is_open())
	{
		printf("Can't open nebula catalog %s\n", fileName.c_str());
		assert(0);
	}
	
	// determine total number to be loaded for percent complete display
	string record;
	int total=0;
	while(!std::getline(inf, record).eof())
	{
		++total;
	}
	inf.clear();
	inf.seekg(0);
	
	printf(_("(%d deep space objects)...\n"), total);
	
	int current = 0;
	char tmpstr[512];
	while(!std::getline(inf, record).eof())
	{
		// Draw loading bar
		++current;
		snprintf(tmpstr, 512, _("Loading Nebula Data: %d/%d"), current, total);
		lb.SetMessage(tmpstr);
		lb.Draw((float)current/total);
		
		Nebula * e = new Nebula;
		int temp = e->read(record);
		if (!temp) // reading error
		{
			printf("Error while parsing nebula %s\n", e->name.c_str());
			delete e;
			e = NULL;
		} 
		else
		{
			neb_array.push_back(e);
		}
	}
	
	if (!Nebula::nebula_font) Nebula::nebula_font = new s_font(12.,"spacefont", font_fileName); // load Font
	if (!Nebula::nebula_font)
	{
		printf("Can't create nebulaFont\n");
		return(1);
	}
	
	Nebula::tex_circle = new s_texture("neb");   // Load circle texture
	
	return 0;
}

// Draw all the Nebulaes
void Nebula_mgr::draw(int names_ON, Projector* prj, const navigator * nav, tone_reproductor* eye, bool _gravity_label, float max_mag_name, bool bright_nebulae)
{
	Nebula::gravity_label = _gravity_label;

	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    Vec3f pXYZ; 

    vector<Nebula *>::iterator iter;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++) {   

      // correct for precession
      pXYZ = nav->prec_earth_equ_to_earth_equ((*iter)->XYZ);

      // project in 2D to check if the nebula is in screen
      if ( !prj->project_earth_equ_check(pXYZ,(*iter)->XY) ) continue;

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

// search by name
stel_object * Nebula_mgr::search(const string& name)
{
    vector<Nebula *>::iterator iter;

    for(iter=neb_array.begin();iter!=neb_array.end();iter++) {
      if ((*iter)->name == name) return (*iter);
    }

    return NULL;
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
