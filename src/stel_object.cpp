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
#include "solarsystem.h"
#include "nebula_mgr.h"
#include "hip_star_mgr.h"

extern s_texture * texIds[200];            // Common Textures

static int local_time=0;

stel_object::stel_object()
{
}

stel_object::~stel_object()
{
}

// Draw a nice animated pointer around the object
void stel_object::draw_pointer(int delta_time)
{
	local_time+=delta_time;
	double x,y;
	Vec3d pos=get_equ_pos();
	Project(pos[0],pos[1],pos[2],x,y);
    setOrthographicProjection(global.X_Resolution, global.Y_Resolution);
    glPushMatrix();
    glLoadIdentity();

	if (get_type()==STEL_OBJECT_NEBULA) glColor3f(0.4f,0.5f,0.8f);
	if (get_type()==STEL_OBJECT_PLANET) glColor3f(1.0f,0.3f,0.3f);

    if (get_type()==STEL_OBJECT_STAR)
    {
		glColor3fv(get_RGB());
		glBindTexture (GL_TEXTURE_2D, texIds[12]->getID());
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glTranslatef(x, global.Y_Resolution-y,0.0f);
        glRotatef((float)local_time/20.,0.,0.,1.);
        glBegin(GL_QUADS );
            glTexCoord2f(0.0f,0.0f);    glVertex3f(-13.,-13.,0.);      //Bas Gauche
            glTexCoord2f(1.0f,0.0f);    glVertex3f(13.,-13.,0.);       //Bas Droite
            glTexCoord2f(1.0f,1.0f);    glVertex3f(13.,13.,0.);        //Haut Droit
            glTexCoord2f(0.0f,1.0f);    glVertex3f(-13.,13.,0.);       //Haut Gauche
        glEnd ();
    }

	double size=2000./navigation.get_fov();

    if (get_type()==STEL_OBJECT_NEBULA || get_type()==STEL_OBJECT_PLANET)
    {
		if (get_type()==STEL_OBJECT_PLANET)
        {
			glBindTexture(GL_TEXTURE_2D, texIds[26]->getID());
            if (size < 15) size=10;
            size+=navigation.get_fov()/4;
            size+=size*sin((float)local_time/400)/200*navigation.get_fov();
        }
        if (get_type()==STEL_OBJECT_NEBULA)
        {
			glBindTexture(GL_TEXTURE_2D, texIds[27]->getID());
            size*=10/navigation.get_fov();
            if (size<15) size=15;
            size+=size*sin((float)local_time/400)/10;
        }

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glTranslatef(x, global.Y_Resolution-y,0.0f);
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
    glPopMatrix();
    resetPerspectiveProjection();

}

/*****************************************************************************/
// find and select the "nearest" object from earth equatorial position
stel_object * find_stel_object(Vec3d v)
{
	stel_object * sobj = NULL;

	//if (global.FlagPlanets)
	//	sobj = Sun->search(v);

	if (sobj) return sobj;

	Vec3f u=Vec3f(v[0],v[1],v[2]);

	sobj = messiers->search(u);
	if (sobj) return sobj;

	if (global.FlagStars)
		sobj = HipVouteCeleste->search(u);

	return sobj;
}

/*****************************************************************************/
// find and select the "nearest" object from screen position
stel_object * find_stel_object(int x, int y)
{
    navigation.switch_to_earth_equatorial();
	Vec3d v = UnProject(x,y);

	return find_stel_object(v);
}

void stel_object::get_info_string(char * s)
{
	sprintf(s,"No info for this object...");
}

/*
char * stel_object::get_info_string(char * s)
{
	if (type==0)  //Star
    {
		glColor3f(0.4f,0.7f,0.3f);
        sprintf(objectInfo,"Info : %s\nName :%s\nHip : %.4d\nRA  : %.2dh %.2dm %.2fs\nDE  : %.2fdeg\nMag : %.2f",
            global.SelectedObject.CommonName==NULL ? "-" : global.SelectedObject.CommonName,
            global.SelectedObject.Name==NULL ? "-" : global.SelectedObject.Name,
            global.SelectedObject.HR,
            global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,
            global.SelectedObject.DE*180/PI,
            global.SelectedObject.Mag);
    }
    if (type==1)  //Nebulae
    {   glColor3f(0.4f,0.5f,0.8f);
        sprintf(objectInfo,"Name: %s\nNGC : %d\nRA  : %.2dh %.2dm %.2fs\nDE  : %.2fdeg\nMag : %.2f",
            global.SelectedObject.Name,
            global.SelectedObject.NGCNum,
            global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,
            global.SelectedObject.DE*180/PI,
            global.SelectedObject.Mag);
    }
    if (type==2)  //Planet
    {   glColor3f(1.0f,0.3f,0.3f);
        sprintf(objectInfo,"Name: %s\nRA  : %.2dh %.2dm %.2fs\nDE  : %.2fdeg\nDistance : %.4f AU",
            global.SelectedObject.Name,
            global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,
            global.SelectedObject.DE*180/PI,
            global.SelectedObject.Distance);
    }
}
*/
