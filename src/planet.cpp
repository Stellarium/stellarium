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


#include "planet.h"
#include "navigator.h"
#include "s_font.h"

s_font * planet_name_font;

rotation_elements::rotation_elements() : period(1.), offset(0.), epoch(J2000), obliquity(0.), ascendingNode(0.), precessionRate(0.)
{
}

planet::planet(char * _name, int _flagHalo, double _radius, vec3_t _color,
				s_texture * _planetTexture, s_texture * _haloTexture,
				void (*_coord_func)(double JD, double *, double *, double *)) :
					flagHalo(_flagHalo), radius(_radius), color(_color), axis_rotation(0.),
					planetTexture(_planetTexture), haloTexture(_haloTexture),
					coord_func(_coord_func), parent(NULL)
{
	ecliptic_pos=Vec3d(0.,0.,0.);
	name=strdup(_name);
}


planet::~planet()
{
	if (name) free(name);
	name=NULL;
}

void planet::set_rotation_elements(float _period, float _offset, double _epoch, float _obliquity, float _ascendingNode, float _precessionRate)
{
    re.period = _period;
	re.offset = _offset;
    re.epoch = _epoch;
    re.obliquity = _obliquity;
    re.ascendingNode = _ascendingNode;
    re.precessionRate = _precessionRate;
}

Vec3d planet::get_equ_pos(void)
{
	Vec3d v = get_heliocentric_ecliptic_pos();
	return navigation.helio_to_earth_equ(&v);
}

void planet::compute_position(double date)
{
	coord_func(date, &(ecliptic_pos[0]), &(ecliptic_pos[1]), &(ecliptic_pos[2]));
    // Compute for the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->compute_position(date);
        iter++;
    }
}

// Compute the matrices which converts from the parent's ecliptic coordinates to local ecliptic and oposite
void planet::compute_trans_matrix(double date)
{
	mat_parent_to_local =	Mat4d::yrotation(re.obliquity) /* Mat4d::zrotation(re.ascendingNode)*/ * Mat4d::translation(-ecliptic_pos);
	mat_local_to_parent =   Mat4d::translation(ecliptic_pos) /* Mat4d::zrotation(-re.ascendingNode)*/ * Mat4d::yrotation(-re.obliquity);


	compute_geographic_rotation(date);

    // Compute for the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->compute_trans_matrix(date);
        iter++;
    }
}



// Get a matrix which convert from local geographic coordinate to heliocentric ecliptic coordinate
Mat4d planet::get_helio_to_geo_matrix()
{
	Mat4d mat = mat_local_to_parent;

	mat = mat * Mat4d::zrotation(axis_rotation*M_PI/180.);
	planet * p = this;
	while (p->parent!=NULL)
	{
		mat = p->parent->mat_local_to_parent * mat;
		p=p->parent;
	}
	return mat;
}

// Compute the z rotation to use from equatorial to geographic coordinates
void planet::compute_geographic_rotation(double date)
{
    double t = date - re.epoch;
    double rotations = t / (double) re.period;
    double wholeRotations = floor(rotations);
    double remainder = rotations - wholeRotations;

	axis_rotation = -remainder * 360. - re.offset;
}

Vec3d planet::get_ecliptic_pos()
{
	return ecliptic_pos;
}

Vec3d planet::get_heliocentric_ecliptic_pos()
{
	Vec3d pos = ecliptic_pos;
	planet * p = this;
	while (p->parent!=NULL)
	{
		pos.transfo4d(p->mat_local_to_parent);
		p=p->parent;
	}
	return pos;
}

void planet::addSatellite(planet*p)
{
	satellites.push_back(p);
	p->parent=this;
}

void planet::draw(void)
{
	glPushMatrix();

    glMultMatrixd(mat_local_to_parent);

    if (global.FlagPlanetsHintDrawing)
    // Draw the name, and the circle
    // Thanks to Nick Porcino for this addition
    {   
        double screenX, screenY, screenZ;
        Project(0., 0., 0., screenX, screenY, screenZ);

        if (screenZ < 1)
        {
            screenY = global.Y_Resolution - screenY;

            setOrthographicProjection(global.X_Resolution, global.Y_Resolution);    // 2D coordinate
            glEnable(GL_BLEND);
            glDisable(GL_LIGHTING); 
            glColor3f(0.5,0.5,0.7);
            float tmp = 10.;//radius/navigation.get_fov();
            planet_name_font->print((int)(screenX+tmp),(int)(screenY+tmp), name);

            // Draw the circle
			glDisable(GL_TEXTURE_2D);
            glBegin(GL_LINE_STRIP);
            for (float r = 0; r < 6.28; r += 0.2)
            {   
                glVertex3f(screenX + 8. * sin(r), screenY + 8. * cos(r), 0);
            }
            glEnd();
            resetPerspectiveProjection();                           // Restore the other coordinate
        }
    }
    
    //glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	glPushMatrix();

	// Rotate and add an extra half rotation because of the convention in all
    // planet texture maps where zero deg long. is in the middle of the texture.
	glRotatef(-axis_rotation - 180.,0.,0.,1.);

	glColor3f(1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, planetTexture->getID());
	GLUquadricObj * p=gluNewQuadric();
	gluQuadricTexture(p,GL_TRUE);
	gluQuadricOrientation(p, GLU_OUTSIDE);
	gluSphere(p,radius*1,40,40);
	gluDeleteQuadric(p);

	glPopMatrix();

    // Draw the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->draw();
        iter++;
    }

    glPopMatrix();

	glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
}

sun_planet::sun_planet(char * _name, int _flagHalo, double _radius, vec3_t _color,
				s_texture * _planetTexture, s_texture * _haloTexture, s_texture * _bigHaloTexture) : planet(_name,_flagHalo,_radius,_color,_planetTexture,_haloTexture,NULL)
{
	ecliptic_pos=Vec3d(0.,0.,0.);
	mat_local_to_parent = Mat4d::identity();
	name=strdup(_name);
}


void sun_planet::compute_position(double date)
{
    // The sun is fixed in the heliocentric coordinate
	//glEnable(GL_LIGHTING);

    float tmp[4] = {0,0,0,1};
    float tmp2[4] = {0.01,0.01,0.01,1};
    float tmp3[4] = {1000,1000,1000,1};
    float tmp4[4] = {1,1,1,1};
    glLightfv(GL_LIGHT1,GL_AMBIENT,tmp);
    glLightfv(GL_LIGHT1,GL_DIFFUSE,tmp3);
    glLightfv(GL_LIGHT1,GL_SPECULAR,tmp);

    glDisable(GL_LIGHT0);
    //glEnable(GL_LIGHT1);

    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT ,tmp);
    glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE ,tmp4);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION ,tmp);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS ,tmp);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR ,tmp);

    glLightfv(GL_LIGHT1,GL_POSITION,Vec3f(0.,0.,0.));

    // Compute for the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->compute_position(date);
        iter++;
    }
	glDisable(GL_LIGHTING);
}

// Get a matrix which converts from the parent's ecliptic coordinates to local ecliptic
void sun_planet::compute_trans_matrix(double date)
{
    // Compute for the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->compute_trans_matrix(date);
        iter++;
    }
}

	/*
// Draw the planet (with special cases for saturn rings, sun, moon, etc..)
void Planet::Draw(vec3_t coordLight)
{
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);

    vec3_t normGeoCoord=GeoCoord;
    normGeoCoord.Normalize();
    glTranslatef(normGeoCoord[0]*DIST_PLANET,normGeoCoord[1]*DIST_PLANET,normGeoCoord[2]*DIST_PLANET);


    //Light
    vec3_t posSun = coordLight-GeoCoord;
    posSun.Normalize();

    float posLight[4] = {10000*posSun[0],10000*posSun[1],10000*posSun[2]};
    glLightfv(GL_LIGHT1,GL_POSITION,posLight);

    float rmag;
    if (num==0)                                                                     // Sun
    {   glBindTexture (GL_TEXTURE_2D, texIds[48]->getID());
        rmag=25*Rayon*DIST_PLANET/Distance_to_obs*(global.Fov/60);                  // Rayon du halo
        glColor3fv(Colour);
    }
    else
    {   glBindTexture (GL_TEXTURE_2D, texIds[25]->getID());
        glColor3fv(Colour*(1.8-global.SkyBrightness));                              // Calcul de la couleur
        rmag=300*Rayon*DIST_PLANET/Distance_to_obs*global.Fov/60;                   // Rayon de "l'eclat"
        if (num==5) rmag=150*Rayon*DIST_PLANET/Distance_to_obs*global.Fov/60;
    }

    if (num!=10)                                // != luna
    {   glEnable(GL_BLEND);
                                                // Draw a light point like a star for naked eye simulation
        glPushMatrix();
        glRotatef(RaRad*180/PI,0,1,0);
        glRotatef(DecRad*180/PI,-1,0,0);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2i(1,0);                  //Bas Droite
            glVertex3f(rmag,-rmag,0.0f);
            glTexCoord2i(0,0);                  //Bas Gauche
            glVertex3f(-rmag,-rmag,0.0f);
            glTexCoord2i(1,1);                  //Haut Droit
            glVertex3f(rmag,rmag,0.0f);
            glTexCoord2i(0,1);                  //Haut Gauche
            glVertex3f(-rmag,rmag,0.0f);
        glEnd ();
        glPopMatrix();      
    }


    if (num==10)
    {   glBindTexture (GL_TEXTURE_2D, texIds[50]->getID());
        rmag=20*Rayon*DIST_PLANET/Distance_to_obs*(global.Fov/60);                  // Lune : Rayon du halo
        glColor3f(0.07*(1-posSun.Dot(normGeoCoord)),0.07*(1-posSun.Dot(normGeoCoord)),0.07*(1-posSun.Dot(normGeoCoord)));
        glEnable(GL_BLEND);
                                        // Draw a light point like a star for naked eye simulation
        glPushMatrix();
        glRotatef(RaRad*180/PI,0,1,0);
        glRotatef(DecRad*180/PI,-1,0,0);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2i(1,0);                  //Bas Droite
            glVertex3f(rmag,-rmag,0.0f);
            glTexCoord2i(0,0);                  //Bas Gauche
            glVertex3f(-rmag,-rmag,0.0f);
            glTexCoord2i(1,1);                  //Haut Droit
            glVertex3f(rmag,rmag,0.0f);
            glTexCoord2i(0,1);                  //Haut Gauche
            glVertex3f(-rmag,rmag,0.0f);
        glEnd ();
        glPopMatrix();  
    }



    glRotatef(90,1,0,0);


    if (num==6)                             // Draw saturn rings 1/2
    {   double rAn=2.5*Rayon*DIST_PLANET/Distance_to_obs;
        glColor3f(1.0f, 1.0f, 1.0f);
        glBindTexture (GL_TEXTURE_2D, texIds[47]->getID());
        glEnable(GL_BLEND);
        glPushMatrix();

        glRotatef(RaRad*180./PI,0,0,1);
            glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(1,0);              //Bas Droite
            glVertex3f(rAn,-rAn,0.0f);
            glTexCoord2f(0,0);              //Bas Gauche
            glVertex3f(-rAn,-rAn,0.0f);
            glTexCoord2f(1,0.5);            //Haut Droit
            glVertex3f(rAn,0.0f,0.0f);
            glTexCoord2f(0,0.5);            //Haut Gauche
            glVertex3f(-rAn,0.0f,0.0f);
        glEnd ();
        glPopMatrix();
    }

    if (asin(Rayon*2/Distance_to_obs)*180./PI>3*global.Fov/global.Y_Resolution)     //Draw the sphere if big enough


    if (num==6)                                 // Draw saturn rings 2/2
    {   double rAn=2.5*Rayon*DIST_PLANET/Distance_to_obs;
        glColor3f(1.0f, 1.0f, 1.0f); //SkyBrightness
        glBindTexture (GL_TEXTURE_2D, texIds[47]->getID());
        glEnable(GL_BLEND);
        glPushMatrix();
        glRotatef(RaRad*180./PI+180,0,0,1);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(1,0);                  //Bas Droite
            glVertex3f(rAn,-rAn,0.0f);
            glTexCoord2f(0,0);                  //Bas Gauche
            glVertex3f(-rAn,-rAn,0.0f);
            glTexCoord2f(1,0.5);                //Haut Droit
            glVertex3f(rAn,0.0f,0.0f);
            glTexCoord2f(0,0.5);                //Haut Gauche
            glVertex3f(-rAn,0.0f,0.0f);
        glEnd ();
        glPopMatrix();
    }

    glPopMatrix();
}
*/

