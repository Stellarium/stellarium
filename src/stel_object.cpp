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

#include "stellarium.h"
#include "stel_object.h"
#include "navigator.h"
#include "stel_utility.h"
#include "s_texture.h"
#include "planet.h"

s_texture * stel_object::pointer_star = NULL;
s_texture * stel_object::pointer_planet = NULL;
s_texture * stel_object::pointer_nebula = NULL;

int stel_object::local_time = 0;

// Draw a nice animated pointer around the object
void stel_object::draw_pointer(int delta_time, Projector* prj, navigator * nav)
{
	local_time+=delta_time;
	Vec3d pos=get_earth_equ_pos(nav);
	Vec3d screenpos;
	// Compute 2D pos and return if outside screen
	if (!prj->project_earth_equ(pos, screenpos)) return;
    prj->set_orthographic_projection();

	if (get_type()==STEL_OBJECT_NEBULA) glColor3f(0.4f,0.5f,0.8f);
	if (get_type()==STEL_OBJECT_PLANET) glColor3f(1.0f,0.3f,0.3f);

    if (get_type()==STEL_OBJECT_STAR)
    {
		glColor3fv(get_RGB());
		glBindTexture (GL_TEXTURE_2D, pointer_star->getID());
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glTranslatef(screenpos[0], screenpos[1], 0.0f);
        glRotatef((float)local_time/20.,0.,0.,1.);
        glBegin(GL_QUADS );
            glTexCoord2f(0.0f,0.0f);    glVertex3f(-13.,-13.,0.);      //Bas Gauche
            glTexCoord2f(1.0f,0.0f);    glVertex3f(13.,-13.,0.);       //Bas Droite
            glTexCoord2f(1.0f,1.0f);    glVertex3f(13.,13.,0.);        //Haut Droit
            glTexCoord2f(0.0f,1.0f);    glVertex3f(-13.,13.,0.);       //Haut Gauche
        glEnd ();
    }

	float size = get_on_screen_size(nav, prj);
	size+=20.f;
	size+=10.f*sin(0.002f * local_time);

    if (get_type()==STEL_OBJECT_NEBULA || get_type()==STEL_OBJECT_PLANET)
    {
		if (get_type()==STEL_OBJECT_PLANET)
			glBindTexture(GL_TEXTURE_2D, pointer_planet->getID());
        if (get_type()==STEL_OBJECT_NEBULA)
			glBindTexture(GL_TEXTURE_2D, pointer_nebula->getID());

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glTranslatef(screenpos[0], screenpos[1], 0.0f);
        if (get_type()==STEL_OBJECT_PLANET) glRotatef((float)local_time/100,0,0,-1);

        glTranslatef(-size/2, -size/2,0.0f);
        glRotatef(90,0,0,1);
        glBegin(GL_QUADS );
            glTexCoord2f(0.0f,0.0f);    glVertex3f(-10,-10,0);      //Bas Gauche
            glTexCoord2f(1.0f,0.0f);    glVertex3f(10,-10,0);       //Bas Droite
            glTexCoord2f(1.0f,1.0f);    glVertex3f(10,10,0);        //Haut Droit
            glTexCoord2f(0.0f,1.0f);    glVertex3f(-10,10,0);       //Haut Gauche
        glEnd ();

        glRotatef(-90,0,0,1);
        glTranslatef(0,size,0.0f);
        glBegin(GL_QUADS );
            glTexCoord2f(0.0f,0.0f);    glVertex3f(-10,-10,0);      //Bas Gauche
            glTexCoord2f(1.0f,0.0f);    glVertex3f(10,-10,0);       //Bas Droite
            glTexCoord2f(1.0f,1.0f);    glVertex3f(10,10,0);        //Haut Droit
            glTexCoord2f(0.0f,1.0f);    glVertex3f(-10,10,0);       //Haut Gauche
        glEnd ();

        glRotatef(-90,0,0,1);
        glTranslatef(0, size,0.0f);
        glBegin(GL_QUADS );
            glTexCoord2f(0.0f,0.0f);    glVertex3f(-10,-10,0);      //Bas Gauche
            glTexCoord2f(1.0f,0.0f);    glVertex3f(10,-10,0);       //Bas Droite
            glTexCoord2f(1.0f,1.0f);    glVertex3f(10,10,0);        //Haut Droit
            glTexCoord2f(0.0f,1.0f);    glVertex3f(-10,10,0);       //Haut Gauche
        glEnd ();

        glRotatef(-90,0,0,1);
        glTranslatef(0,size,0);
        glBegin(GL_QUADS );
            glTexCoord2f(0.0f,0.0f);    glVertex3f(-10,-10,0);      //Bas Gauche
            glTexCoord2f(1.0f,0.0f);    glVertex3f(10,-10,0);       //Bas Droite
            glTexCoord2f(1.0f,1.0f);    glVertex3f(10,10,0);        //Haut Droit
            glTexCoord2f(0.0f,1.0f);    glVertex3f(-10,10,0);       //Haut Gauche
        glEnd ();
    }

    prj->reset_perspective_projection();
}


void stel_object::get_info_string(char * s, navigator * nav) const
{
	sprintf(s,"No info for this object...");
}

void stel_object::init_textures(void)
{
	pointer_star = new s_texture("pointeur2");
	pointer_planet = new s_texture("pointeur4");
	pointer_nebula = new s_texture("pointeur5");
}
