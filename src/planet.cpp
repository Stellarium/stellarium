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

s_font* planet::planet_name_font = NULL;

rotation_elements::rotation_elements() : period(1.), offset(0.), epoch(J2000),
		obliquity(0.), ascendingNode(0.), precessionRate(0.)
{
}

planet::planet(const char * _name, int _flagHalo, int _flag_lighting, double _radius, vec3_t _color,
	const char* tex_map_name, const char* tex_halo_name, pos_func_type _coord_func) :
		name(NULL), flagHalo(_flagHalo), flag_lighting(_flag_lighting), radius(_radius), color(_color), axis_rotation(0.),
		tex_map(NULL), tex_halo(NULL), coord_func(_coord_func), parent(NULL)
{
	ecliptic_pos=Vec3d(0.,0.,0.);
	mat_local_to_parent = Mat4d::identity();
	if (_name) name=strdup(_name);
	tex_map = new s_texture(tex_map_name, TEX_LOAD_TYPE_PNG_SOLID);
	if (flagHalo) tex_halo = new s_texture(tex_halo_name);
}


planet::~planet()
{
	if (name) free(name);
	name=NULL;
}

// Return the information string "ready to print" :)
void planet::get_info_string(char * s, navigator * nav) const
{
	double tempDE, tempRA;
	Vec3d equPos = get_earth_equ_pos(nav);
	rect_to_sphe(&tempRA,&tempDE,&equPos);
	sprintf(s,"Name :%s\nRA : %s\nDE : %s\n Distance : %.8f UA",
	name, print_angle_hms(tempRA*180./M_PI), print_angle_dms_stel(tempDE*180./M_PI), equPos.length());
}

// Set the orbital elements
void planet::set_rotation_elements(float _period, float _offset, double _epoch, float _obliquity, float _ascendingNode, float _precessionRate)
{
    re.period = _period;
	re.offset = _offset;
    re.epoch = _epoch;
    re.obliquity = _obliquity;
    re.ascendingNode = _ascendingNode;
    re.precessionRate = _precessionRate;
}


// Return the planet position in rectangular earth equatorial coordinate
Vec3d planet::get_earth_equ_pos(navigator * nav) const
{
	Vec3d v = get_heliocentric_ecliptic_pos();
	return nav->helio_to_earth_pos_equ(&v); 	// this is earth equatorial but centered
												// on observer's position (latitude, longitude)
	//return navigation.helio_to_earth_equ(&v); this is the real equatorial centered on earth center
}

// Compute the position in the parent planet coordinate system
// Actually call the provided function to compute the ecliptical position
void planet::compute_position(double date)
{
	coord_func(date, &(ecliptic_pos[0]), &(ecliptic_pos[1]), &(ecliptic_pos[2]));
}

// Compute the transformation matrix from the local planet coordinate to the parent planet coordinate
void planet::compute_trans_matrix(double date)
{
	mat_local_to_parent = Mat4d::translation(ecliptic_pos) // * Mat4d::zrotation(-re.ascendingNode)
		* Mat4d::xrotation(-re.obliquity);

	compute_geographic_rotation(date);
}


// Get a matrix which converts from heliocentric ecliptic coordinate to local geographic coordinate
Mat4d planet::get_helio_to_geo_matrix()
{
	Mat4d mat = mat_local_to_parent;
	mat = mat * Mat4d::zrotation(axis_rotation*M_PI/180.);

	// Iterate thru parents
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

	axis_rotation = remainder * 360. + re.offset;
}

// Get the planet position in the parent planet ecliptic coordinate
Vec3d planet::get_ecliptic_pos() const
{
	return ecliptic_pos;
}

// Return the heliocentric ecliptical position
Vec3d planet::get_heliocentric_ecliptic_pos() const
{
	Vec3d pos = ecliptic_pos;
	planet * p = parent;
	while (p!=NULL)
	{
		pos.transfo4d(p->mat_local_to_parent);
		p=p->parent;
	}
	return pos;
}

// Add the given planet in the satellite list
void planet::add_satellite(planet*p)
{
	satellites.push_back(p);
	p->parent=this;
}

// Draw the planet and all the related infos : name, circle etc..
void planet::draw(int hint_ON, draw_utility * du, navigator * nav)
{
	Mat4d mat = mat_local_to_parent;
	planet * p = parent;
	while (p!=NULL)
	{
		mat = p->mat_local_to_parent * mat;
		p = p->parent;
	}

	glPushMatrix();
    glMultMatrixd(mat); // Go in planet local coordinate

	// Compute the 2D position
	du->project(0., 0., 0., screenPos[0], screenPos[1], screenPos[2]);

	// Check if in the screen
	Vec3d earthEquPos = get_earth_equ_pos(nav);
	float lim = atan(radius/earthEquPos.length())*180./M_PI/du->fov;
	if (screenPos[2] < 1 &&
		screenPos[0]>-lim*du->screenW && screenPos[0]<(1.f+lim)*du->screenW &&
		screenPos[1]>-lim*du->screenH && screenPos[1]<(1.f+lim)*du->screenH)
	{

		if (tex_halo) draw_halo(du);

		// Draw the name, and the circle if it's not too close from the body it's turning around
		// this prevents name overlaping (ie for jupiter satellites)
    	if (hint_ON && atan(get_ecliptic_pos().length()/earthEquPos.length())/du->fov>0.0005)
    	{
			draw_hints(earthEquPos, du);
        }

		glPushMatrix();
		// Rotate and add an extra half rotation because of the convention in all
    	// planet texture maps where zero deg long. is in the middle of the texture.
		glRotatef(axis_rotation + 180.,0.,0.,1.);
		draw_sphere();
		glPopMatrix();
    }

    glPopMatrix();

    glDisable(GL_CULL_FACE);
}

void planet::draw_hints(Vec3d earthEquPos, draw_utility * du)
{
	du->set_orthographic_projection();    // 2D coordinate

	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glColor3f(0.5,0.5,0.7);
	float tmp = 8.f + radius/earthEquPos.length()*du->screenH*60./du->fov; // Shift for name printing
	planet_name_font->print(screenPos[0]+tmp,screenPos[1]+tmp, name);

	// Draw the 2D small circle : disapears smoothly on close view
	tmp-=8.;
	if (tmp<1) tmp=1.;
	glColor4f(0.5/tmp,0.5/tmp,0.7/tmp,1/tmp);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_STRIP);
	for (float r = 0; r < 6.28; r += 0.2)
	{
		glVertex3f(screenPos[0] + 8. * sin(r), screenPos[1] + 8. * cos(r), 0.0f);
	}
	glEnd();

	du->reset_perspective_projection();		// Restore the other coordinate
}

void planet::draw_sphere(void)
{
    glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	if (flag_lighting) glEnable(GL_LIGHTING);
	else glDisable(GL_LIGHTING);
	glColor3fv(color);
	glBindTexture(GL_TEXTURE_2D, tex_map->getID());
	GLUquadricObj * p = gluNewQuadric();
	gluQuadricTexture(p,GL_TRUE);
	gluSphere(p,radius,40,40);
	gluDeleteQuadric(p);
}

void planet::draw_halo(draw_utility * du)
{
	float rmag = 10;//lim*du->screenH*100;
	if (rmag>0.5)
	{
		float cmag=1.;
		if (rmag<1.2)
		{
			cmag=pow(rmag,2)/1.44;
			rmag=1.2;
		}
		else
		{
			if (rmag>8.)
			{
				rmag=8.;
			}
		}

		du->set_orthographic_projection();    	// 2D coordinate
		glBindTexture(GL_TEXTURE_2D, tex_halo->getID());
		glEnable(GL_BLEND);
			glDisable(GL_LIGHTING);
			glEnable(GL_TEXTURE_2D);
			glColor3f(cmag,cmag,cmag);
			glTranslatef(screenPos[0],screenPos[1],0.);
			glBegin(GL_QUADS);
				glTexCoord2i(0,0);	glVertex3f(-rmag, rmag,0.f);	// Bottom Left
				glTexCoord2i(1,0);	glVertex3f( rmag, rmag,0.f);	// Bottom Right
				glTexCoord2i(1,1);	glVertex3f( rmag,-rmag,0.f);	// Top Right
				glTexCoord2i(0,1);	glVertex3f(-rmag,-rmag,0.f);	// Top Left
			glEnd();

		du->reset_perspective_projection();		// Restore the other coordinate
	}
}


ring::ring(float _radius, s_texture * _tex) : radius(_radius), tex(_tex)
{
}

void ring::draw(navigator* nav)
{
	//glRotatef(axis_rotation + 180.,0.,0.,1.);
	glColor3f(1.0f, 0.88f, 0.82f); // For saturn only..
    glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
    glBindTexture (GL_TEXTURE_2D, tex->getID());
	float r=radius;
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex3d( r,-r, 0.);	// Bottom left
		glTexCoord2f(1,0); glVertex3d( r, r, 0.);	// Bottom right
		glTexCoord2f(1,1); glVertex3d(-r, r, 0.);	// Top right
		glTexCoord2f(0,1); glVertex3d(-r,-r, 0.);	// Top left
	glEnd ();
	glDisable(GL_DEPTH_TEST);
}


	/*
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

