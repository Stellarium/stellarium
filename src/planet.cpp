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

#include <iostream>
#include "planet.h"
#include "navigator.h"
#include "s_font.h"

s_font* planet::planet_name_font = NULL;
float planet::star_scale = 1.f;
bool planet::gravity_label = false;
Vec3f planet::label_color = Vec3f(.4,.4,.8);
Vec3f planet::orbit_color = Vec3f(1,.6,1);

rotation_elements::rotation_elements() : period(1.), offset(0.), epoch(J2000),
		obliquity(0.), ascendingNode(0.), precessionRate(0.)
{
}

planet::planet(const string& _name, int _flagHalo, int _flag_lighting, double _radius, Vec3f _color,
	float _albedo, const string& tex_map_name, const string& tex_halo_name, pos_func_type _coord_func) :
		name(_name), flagHalo(_flagHalo), flag_lighting(_flag_lighting), radius(_radius), color(_color),
		albedo(_albedo), axis_rotation(0.),	tex_map(NULL), tex_halo(NULL), tex_big_halo(NULL), rings(NULL),
		sphere_scale(1.f), lastJD(J2000), last_orbitJD(0), deltaJD(JD_SECOND), orbit_cached(0),
		coord_func(_coord_func), parent(NULL)
{
	ecliptic_pos=Vec3d(0.,0.,0.);
	mat_local_to_parent = Mat4d::identity();
	tex_map = new s_texture(tex_map_name, TEX_LOAD_TYPE_PNG_SOLID_REPEAT);
	if (flagHalo) tex_halo = new s_texture(tex_halo_name);
	// temp
	common_name = _name;

	// 60 day trails
	DeltaTrail = 1;  
	// small increment like 0.125 would allow observation of latitude related wobble of moon
	// if decide to show moon trail
       	MaxTrail = 60;
	last_trailJD = 0; // for now
	trail_color = Vec3f(1,.7,.7);
	trail_on = 0;
	first_point = 1;
}


planet::~planet()
{
	if (tex_map) delete tex_map;
	tex_map = NULL;
	if (tex_halo) delete tex_halo;
	tex_halo = NULL;
	if (rings) delete rings;
	rings = NULL;
	if (tex_big_halo) delete tex_big_halo;
	tex_big_halo = NULL;
}

// Return the information string "ready to print" :)
void planet::get_info_string(char * s, const navigator * nav) const
{
	double tempDE, tempRA;
	static char scale_str[100];
	if (sphere_scale == 1.f) scale_str[0] = '\0';
	else sprintf(scale_str," (x%.1f)", sphere_scale);

	Vec3d equPos = get_earth_equ_pos(nav);
	rect_to_sphe(&tempRA,&tempDE,equPos);
	sprintf(s,_("Name :%s%s\nRA : %s\nDE : %s\nDistance : %.8f UA\nMagnitude : %.2f"),
	common_name.c_str(), scale_str, print_angle_hms(tempRA*180./M_PI).c_str(), print_angle_dms_stel(tempDE*180./M_PI).c_str(), equPos.length(),
	compute_magnitude(nav->get_observer_helio_pos()));
}

// Return the information string "ready to print" :)
void planet::get_short_info_string(char * s, const navigator * nav) const
{
	static char scale_str[100];
	if (sphere_scale == 1.f) scale_str[0] = '\0';
	else sprintf(scale_str," (x%.1f)", sphere_scale);
	sprintf(s,_("%s%s: mag %.1f"),common_name.c_str(), scale_str, compute_magnitude(nav->get_observer_helio_pos()));
}

double planet::get_close_fov(const navigator* nav) const
{
	return atanf(radius*sphere_scale*2.f/get_earth_equ_pos(nav).length())*180./M_PI * 4;
}

double planet::get_satellites_fov(const navigator * nav) const
{
	if (name=="Jupiter") return atanf(0.005/get_earth_equ_pos(nav).length())*180./M_PI * 4;
	if (name=="Saturn") return atanf(0.005/get_earth_equ_pos(nav).length())*180./M_PI * 4;
	return -1.;
}

// Set the orbital elements
void planet::set_rotation_elements(float _period, float _offset, double _epoch, float _obliquity, float _ascendingNode, float _precessionRate, double _sidereal_period )
{
    re.period = _period;
	re.offset = _offset;
    re.epoch = _epoch;
    re.obliquity = _obliquity;
    re.ascendingNode = _ascendingNode;
    re.precessionRate = _precessionRate;
    re.sidereal_period = _sidereal_period;  // used for drawing orbit lines

    delta_orbitJD = re.sidereal_period/ORBIT_SEGMENTS;
}


// Return the planet position in rectangular earth equatorial coordinate
Vec3d planet::get_earth_equ_pos(const navigator * nav) const
{
	Vec3d v = get_heliocentric_ecliptic_pos();
	return nav->helio_to_earth_pos_equ(v);		// this is earth equatorial but centered
												// on observer's position (latitude, longitude)
	//return navigation.helio_to_earth_equ(&v); this is the real equatorial centered on earth center
}

// Compute the position in the parent planet coordinate system
// Actually call the provided function to compute the ecliptical position
void planet::compute_position(double date)
{


	if (delta_orbitJD > 0 && (fabs(last_orbitJD-date)>delta_orbitJD || !orbit_cached))
	{

	  // calculate orbit first (for line drawing)
	  double date_increment = re.sidereal_period/ORBIT_SEGMENTS;
	  double calc_date;
	  //	  int delta_points = (int)(0.5 + (date - last_orbitJD)/date_increment);
	  int delta_points;

	  if( date > last_orbitJD ) {
	    delta_points = (int)(0.5 + (date - last_orbitJD)/date_increment);
	  } else {
	    delta_points = (int)(-0.5 + (date - last_orbitJD)/date_increment);
	  }
	  double new_date = last_orbitJD + delta_points*date_increment;

	  //	  printf( "Updating orbit coordinates for %s (delta %f) (%d points)\n", name.c_str(), delta_orbitJD, delta_points);


	  if( delta_points > 0 && delta_points < ORBIT_SEGMENTS && orbit_cached) {

	    for( int d=0; d<ORBIT_SEGMENTS; d++ ) {
	      if(d + delta_points >= ORBIT_SEGMENTS ) {
		// calculate new points
		calc_date = new_date + (d-ORBIT_SEGMENTS/2)*date_increment;  
		// date increments between points will not be completely constant though

		compute_trans_matrix(calc_date);
		coord_func(calc_date, ecliptic_pos);
		orbit[d] = get_heliocentric_ecliptic_pos();
	      } else {
		orbit[d] = orbit[d+delta_points];
	      }
	    }

	    last_orbitJD = new_date;

	  } else if( delta_points < 0 && abs(delta_points) < ORBIT_SEGMENTS  && orbit_cached) {

	    for( int d=ORBIT_SEGMENTS-1; d>=0; d-- ) {
	      if(d + delta_points < 0 ) {
		// calculate new points
		calc_date = new_date + (d-ORBIT_SEGMENTS/2)*date_increment;  

		compute_trans_matrix(calc_date);
		coord_func(calc_date, ecliptic_pos);
		orbit[d] = get_heliocentric_ecliptic_pos();
	      } else {
		orbit[d] = orbit[d+delta_points];
	      }
	    }

	    last_orbitJD = new_date;

	  } else if( delta_points || !orbit_cached) {

	    // update all points (less efficient)

	    for( int d=0; d<ORBIT_SEGMENTS; d++ ) {
	      calc_date = date + (d-ORBIT_SEGMENTS/2)*date_increment;
	      compute_trans_matrix(calc_date);
	      coord_func(calc_date, ecliptic_pos);
	      orbit[d] = get_heliocentric_ecliptic_pos();
	    } 

	    last_orbitJD = date;
	    orbit_cached = 1;
	  }


	  // calculate actual planet position
	  coord_func(date, ecliptic_pos);

	  lastJD = date;

	} else if (fabs(lastJD-date)>deltaJD) {
	  
	  // calculate actual planet position
	  coord_func(date, ecliptic_pos);
	  lastJD = date;
	}

}

// Compute the transformation matrix from the local planet coordinate to the parent planet coordinate
void planet::compute_trans_matrix(double date)
{

	compute_geographic_rotation(date);

	//	mat_local_to_parent = Mat4d::translation(ecliptic_pos) // * Mat4d::zrotation(-re.ascendingNode)
	//	* Mat4d::xrotation(-re.obliquity);

	// re.ascendingNode is needed for correct Galilean moon positions viewed from Earth, for example
	// However, most values grabbed from celestia are incorrect, at least for stellarium
	// TODO: Figure out the discrepancy
	mat_local_to_parent = Mat4d::translation(ecliptic_pos) * Mat4d::zrotation(re.ascendingNode)
	  * Mat4d::xrotation(-re.obliquity); 
}


// Get a matrix which converts from heliocentric ecliptic coordinate to local geographic coordinate
Mat4d planet::get_helio_to_geo_matrix()
{
	Mat4d mat = mat_local_to_parent;
	mat = mat * Mat4d::zrotation(axis_rotation*M_PI/180.);

	// Iterate thru parents
	planet * p = parent;
	while (p!=NULL && p->parent!=NULL)
	{
		mat = p->mat_local_to_parent * mat;
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

// Compute the distance to the given position in heliocentric coordinate (in AU)
double planet::compute_distance(const Vec3d& obs_helio_pos)
{
	distance = (obs_helio_pos-get_heliocentric_ecliptic_pos()).length();
	return distance;
}

// Get the phase angle for an observer at pos obs_pos in the heliocentric coordinate (dist in AU)
double planet::get_phase(Vec3d obs_pos) const
{
	Vec3d heliopos = get_heliocentric_ecliptic_pos();
	double R = heliopos.length();
	double p = (obs_pos - heliopos).length();
	double s = obs_pos.length();
	double cos_chi = (p*p + R*R - s*s)/(2.f*p*R);

	return (1.f - acosf(cos_chi)/M_PI) * cos_chi + sqrt(1.f - cos_chi*cos_chi) / M_PI;
}

float planet::compute_magnitude(Vec3d obs_pos) const
{
	Vec3d heliopos = get_heliocentric_ecliptic_pos();
	double R = heliopos.length();
	// Fix bug for sun
	if (R<0.0000000000000001) return -26.73f;
	double p = (obs_pos - heliopos).length();
	double s = obs_pos.length();
	double cos_chi = (p*p + R*R - s*s)/(2.f*p*R);

	float phase = (1.f - acosf(cos_chi)/M_PI) * cos_chi + sqrt(1.f - cos_chi*cos_chi) / M_PI;
	float F = 0.666666667f * albedo * (radius*s/(R*p)) * (radius*s/(R*p)) * phase;
	return -26.73f - 2.5f * log10f(F);
}

float planet::compute_magnitude(const navigator * nav) const
{
	return compute_magnitude(nav->get_observer_helio_pos());
}

// Add the given planet in the satellite list
void planet::add_satellite(planet*p)
{
	satellites.push_back(p);
	p->parent=this;
}

void planet::set_big_halo(const string& halotexfile)
{
	tex_big_halo = new s_texture(halotexfile, TEX_LOAD_TYPE_PNG_SOLID);
}

// Return the radius of a circle containing the object on screen
float planet::get_on_screen_size(const Projector* prj, const navigator * nav)
{
	return atanf(radius*sphere_scale*2.f/get_earth_equ_pos(nav).length())*180./M_PI/prj->get_fov()*prj->viewH();
}

// Draw the planet and all the related infos : name, circle etc..
void planet::draw(int hint_ON, Projector* prj, const navigator * nav, const tone_reproductor* eye, 
		  int flag_point, int flag_orbits, int flag_trails)
{

	Mat4d mat = mat_local_to_parent;
	planet * p = parent;
	while (p!=NULL && p->parent!=NULL)
	{
		mat = p->mat_local_to_parent * mat;
		p = p->parent;
	}

	// This removed totally the planet shaking bug!!!
	mat = nav->get_helio_to_eye_mat() * mat;

	// Compute the 2D position and check if in the screen
	float screen_sz = get_on_screen_size(prj, nav);
	float viewport_left = prj->view_left();
	float viewport_bottom = prj->view_bottom();
	if (prj->project_custom(Vec3f(0,0,0), screenPos, mat) &&
		screenPos[1]>viewport_bottom - screen_sz && screenPos[1]<viewport_bottom + prj->viewH()+screen_sz &&
		screenPos[0]>viewport_left - screen_sz && screenPos[0]<viewport_left + prj->viewW() + screen_sz)
	{
		// Draw the name, and the circle if it's not too close from the body it's turning around
		// this prevents name overlaping (ie for jupiter satellites)
		float ang_dist = 300.f*atan(get_ecliptic_pos().length()/get_earth_equ_pos(nav).length())/prj->get_fov();
		if (ang_dist==0.f) ang_dist = 1.f; // if ang_dist == 0, the planet is sun..

		if( flag_orbits ) {
		  // by putting here, only draw orbit if planet if visible for clarity
		  draw_orbit(nav, prj);
		}

		if(flag_trails) draw_trail(nav, prj);

     	if (hint_ON && ang_dist>0.25)
    	{
			if (ang_dist>1.f) ang_dist = 1.f;
			//glColor4f(0.5f*ang_dist,0.5f*ang_dist,0.7f*ang_dist,1.f*ang_dist);
			draw_hints(nav, prj);
        }

		if (rings && screen_sz>1)
		{
			double dist = get_earth_equ_pos(nav).length();
			double n,f;
			prj->get_clipping_planes(&n, &f);	// Copy clipping planes
			prj->set_clipping_planes(dist-rings->get_size()*2, dist+rings->get_size()*2);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			draw_sphere(prj, mat, screen_sz);
			rings->draw(prj, mat);
			glDisable(GL_DEPTH_TEST);
			prj->set_clipping_planes(n ,f);	// Release old clipping planes
		}
		else draw_sphere(prj, mat, screen_sz);

		if (flag_point)
		{
			if (tex_halo) draw_point_halo(nav, prj, eye);
		}
		else
		{
			if (tex_halo) draw_halo(nav, prj, eye);
		}
		if (tex_big_halo) draw_big_halo(nav, prj, eye);
    }
}

void planet::draw_hints(const navigator* nav, const Projector* prj)
{

	prj->set_orthographic_projection();    // 2D coordinate

	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	// Draw common_name + scaling if it's not == 1.
	static char scale_str[100];
	if (sphere_scale == 1.f) sprintf(scale_str,"%s", common_name.c_str());
	else sprintf(scale_str,"%s (x%.1f)", common_name.c_str(), sphere_scale);
	float tmp = 10.f + get_on_screen_size(prj, nav)/sphere_scale/2.f; // Shift for common_name printing

	glColor4f(label_color[0], label_color[1], label_color[2],1.f);
	gravity_label ? prj->print_gravity180(planet_name_font, screenPos[0],screenPos[1], scale_str, 1, tmp, tmp) :
		planet_name_font->print(screenPos[0]+tmp,screenPos[1]+tmp, scale_str);

	// hint disapears smoothly on close view
	tmp -= 10.f;
	if (tmp<1) tmp=1;
 	glColor4f(label_color[0]/tmp, label_color[1]/tmp, label_color[2]/tmp,1.f/tmp);

	// Draw the 2D small circle
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_STRIP);
		for (float r=0.f; r<2.f*M_PI; r+=M_PI/5.f)
		{
			glVertex3f(screenPos[0] + 8. * sin(r), screenPos[1] + 8. * cos(r), 0.0f);
		}
	glEnd();

	prj->reset_perspective_projection();		// Restore the other coordinate
}

void planet::draw_sphere(const Projector* prj, const Mat4d& mat, float screen_sz)
{
	// Adapt the number of facets according with the size of the sphere for optimization
	int nb_facet = (int)(screen_sz * 40/50);	// 40 facets for 1024 pixels diameter on screen
	if (nb_facet<10) nb_facet = 10;
	if (nb_facet>40) nb_facet = 40;

    glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	if (flag_lighting) glEnable(GL_LIGHTING);
	else
	{
		glDisable(GL_LIGHTING);
		glColor3fv(color);
	}
	glBindTexture(GL_TEXTURE_2D, tex_map->getID());

	// Rotate and add an extra half rotation because of the convention in all
    // planet texture maps where zero deg long. is in the middle of the texture.
	prj->sSphere(radius*sphere_scale, nb_facet, nb_facet, mat * Mat4d::zrotation(M_PI/180*(axis_rotation + 180.)));

    glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
}

void planet::draw_halo(const navigator* nav, const Projector* prj, const tone_reproductor* eye)
{
	static float cmag;
	static float rmag;

	rmag = eye->adapt_luminance(expf(-0.92103f*(compute_magnitude(nav->get_observer_helio_pos()) +
		12.12331f)) * 108064.73f);
	rmag = rmag/powf(prj->get_fov(),0.85f)*50.f;

    cmag = 1.f;

    // if size of star is too small (blink) we put its size to 1.2 --> no more blink
    // And we compensate the difference of brighteness with cmag
    if (rmag<1.2f)
    {
        if (rmag<0.3f) return;
        cmag=rmag*rmag/1.44f;
        rmag=1.2f;
    }
	else
    {
		if (rmag>5.f)
    	{
        	rmag=5.f+sqrt(rmag-5)/6;
			if (rmag>9.f)
    		{
				rmag=9.f;
			}
    	}
	}


	// Global scaling
	rmag*=planet::star_scale;

	glBlendFunc(GL_ONE, GL_ONE);
	float screen_r = get_on_screen_size(prj, nav);
	cmag *= 0.5*rmag/screen_r;
	if (cmag>1.f) cmag = 1.f;

	if (rmag<screen_r)
	{
		cmag*=rmag/screen_r;
		rmag = screen_r;
	}

	prj->set_orthographic_projection();    	// 2D coordinate

	glBindTexture(GL_TEXTURE_2D, tex_halo->getID());
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glColor3f(color[0]*cmag, color[1]*cmag, color[2]*cmag);
	glTranslatef(screenPos[0], screenPos[1], 0.f);
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);	glVertex3f(-rmag, rmag,0.f);	// Bottom Left
		glTexCoord2i(1,0);	glVertex3f( rmag, rmag,0.f);	// Bottom Right
		glTexCoord2i(1,1);	glVertex3f( rmag,-rmag,0.f);	// Top Right
		glTexCoord2i(0,1);	glVertex3f(-rmag,-rmag,0.f);	// Top Left
	glEnd();

	prj->reset_perspective_projection();		// Restore the other coordinate
}

void planet::draw_point_halo(const navigator* nav, const Projector* prj, const tone_reproductor* eye)
{
	static float cmag;
	static float rmag;

	rmag = eye->adapt_luminance(expf(-0.92103f*(compute_magnitude(nav->get_observer_helio_pos()) +
		12.12331f)) * 108064.73f);
	rmag = rmag/powf(prj->get_fov(),0.85f)*10.f;

    cmag = 1.f;

    // if size of star is too small (blink) we put its size to 1.2 --> no more blink
    // And we compensate the difference of brighteness with cmag
	if (rmag<0.3f) return;
	cmag=rmag*rmag/(1.4f*1.4f);
	rmag=1.4f;

	// Global scaling
	//rmag*=planet::star_scale;

	glBlendFunc(GL_ONE, GL_ONE);
	float screen_r = get_on_screen_size(prj, nav);
	cmag *= rmag/screen_r;
	if (cmag>1.f) cmag = 1.f;

	if (rmag<screen_r)
	{
		cmag*=rmag/screen_r;
		rmag = screen_r;
	}
	prj->set_orthographic_projection();    	// 2D coordinate

	glBindTexture(GL_TEXTURE_2D, tex_halo->getID());
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glColor3f(color[0]*cmag, color[1]*cmag, color[2]*cmag);
	glTranslatef(screenPos[0], screenPos[1], 0.f);
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);	glVertex3f(-rmag, rmag,0.f);	// Bottom Left
		glTexCoord2i(1,0);	glVertex3f( rmag, rmag,0.f);	// Bottom Right
		glTexCoord2i(1,1);	glVertex3f( rmag,-rmag,0.f);	// Top Right
		glTexCoord2i(0,1);	glVertex3f(-rmag,-rmag,0.f);	// Top Left
	glEnd();

	prj->reset_perspective_projection();		// Restore the other coordinate
}

void planet::draw_big_halo(const navigator* nav, const Projector* prj, const tone_reproductor* eye)
{
	glBlendFunc(GL_ONE, GL_ONE);
	float screen_r = get_on_screen_size(prj, nav);

	float rmag = big_halo_size/2;

	float cmag = rmag/screen_r;
	if (cmag>1.f) cmag = 1.f;

	if (rmag<screen_r*2)
	{
		cmag*=rmag/(screen_r*2);
		rmag = screen_r*2;
	}

	prj->set_orthographic_projection();    	// 2D coordinate

	glBindTexture(GL_TEXTURE_2D, tex_big_halo->getID());
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glColor3f(color[0]*cmag, color[1]*cmag, color[2]*cmag);
	glTranslatef(screenPos[0], screenPos[1], 0.f);
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);	glVertex3f(-rmag, rmag,0.f);	// Bottom Left
		glTexCoord2i(1,0);	glVertex3f( rmag, rmag,0.f);	// Bottom Right
		glTexCoord2i(1,1);	glVertex3f( rmag,-rmag,0.f);	// Top Right
		glTexCoord2i(0,1);	glVertex3f(-rmag,-rmag,0.f);	// Top Left
	glEnd();

	prj->reset_perspective_projection();		// Restore the other coordinate
}



ring::ring(float _radius, const string& _texname) : radius(_radius), tex(NULL)
{
	tex = new s_texture(_texname,TEX_LOAD_TYPE_PNG_ALPHA);
}

ring::~ring()
{
	if (tex) delete tex;
	tex = NULL;
}

void ring::draw(const Projector* prj, const Mat4d& mat)
{
	glPushMatrix();
	glLoadMatrixd(mat);
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glRotatef(axis_rotation + 180.,0.,0.,1.);
	glColor3f(1.0f, 0.88f, 0.82f); // For saturn only..
    glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);

    glBindTexture (GL_TEXTURE_2D, tex->getID());
	double r=radius;
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); prj->sVertex3( -r,-r, 0., mat);	// Bottom left
		glTexCoord2f(1,0); prj->sVertex3( r, -r, 0., mat);	// Bottom right
		glTexCoord2f(1,1); prj->sVertex3(r, r, 0., mat);	// Top right
		glTexCoord2f(0,1); prj->sVertex3(-r,r, 0., mat);	// Top left
	glEnd ();
	glPopMatrix();
}


// draw orbital path of planet
void planet::draw_orbit(const navigator * nav, const Projector* prj) {

  Vec3d onscreen;

  if(!re.sidereal_period) return;

  prj->set_orthographic_projection();    // 2D coordinate

  glEnable(GL_BLEND);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);

  glColor3fv(orbit_color);
	
  int on=0;
  int d;
  for( int n=0; n<=ORBIT_SEGMENTS; n++) {
	  
    if( n==ORBIT_SEGMENTS ) {
      d = 0;  // connect loop
    } else {
      d = n;
    }

    // special case - use current planet position as center vertex so that draws
    // on it's orbit all the time (since segmented rather than smooth curve)
    if( n == ORBIT_SEGMENTS/2 ) {

      if(prj->project_helio(get_heliocentric_ecliptic_pos(), onscreen)) {
	if(!on) glBegin(GL_LINE_STRIP);
	glVertex3d(onscreen[0], onscreen[1], 0);
	on=1;
      } else if( on ) {
	glEnd();
	on=0;
      }
    } else {

      if(prj->project_helio(orbit[d],onscreen)) {
	if(!on) glBegin(GL_LINE_STRIP);
	glVertex3d(onscreen[0], onscreen[1], 0);
	on=1;
      } else if( on ) {
	glEnd();
	on=0;
      }
    }


    
  }
 
  if(on) glEnd(); 


  prj->reset_perspective_projection();		// Restore the other coordinate
  
  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
  glEnable(GL_TEXTURE_2D);
}



// draw trail of planet as seen from earth
void planet::draw_trail(const navigator * nav, const Projector* prj) {

  Vec3d onscreen1;
  Vec3d onscreen2;

  //  if(!re.sidereal_period) return;   // limits to planets

  prj->set_orthographic_projection();    // 2D coordinate

  glEnable(GL_BLEND);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);

  glColor3fv(trail_color);
	
  list<TrailPoint>::iterator iter;  
  list<TrailPoint>::iterator nextiter;
  list<TrailPoint>::iterator begin = trail.begin();
  //  begin++;

  if(trail.begin() != trail.end()) {

    nextiter = trail.end();
    nextiter--;

    for( iter=nextiter; iter != begin; iter--) {

      nextiter--;
      if( prj->project_earth_equ_line_check( (*iter).point, onscreen1, (*(nextiter)).point, onscreen2) ) {
	glBegin(GL_LINE_STRIP);
	glVertex3d(onscreen1[0], onscreen1[1], 0);
	glVertex3d(onscreen2[0], onscreen2[1], 0);
	glEnd();
      }
    }
  }

  // draw final segment to finish at current planet position
  if( !first_point && prj->project_earth_equ_line_check( (*trail.begin()).point, onscreen1, get_earth_equ_pos(nav), onscreen2) ) {
    glBegin(GL_LINE_STRIP);
    glVertex3d(onscreen1[0], onscreen1[1], 0);
    glVertex3d(onscreen2[0], onscreen2[1], 0);
    glEnd();
  }


  prj->reset_perspective_projection();		// Restore the other coordinate
  
  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
  glEnable(GL_TEXTURE_2D);
}

// update trail points as needed
void planet::update_trail(const navigator* nav) {

  if(!trail_on) return;

  double date = nav->get_JDay();

  int dt=0;
  if(first_point || (dt=abs(int((date-last_trailJD)/DeltaTrail))) > MaxTrail) {
    dt=1; 
    // clear old trail
    trail.clear();
    first_point = 0;
  } 

  // Note that when jump by a week or day at a time, loose detail on trails
  // particularly for moon (if decide to show moon trail)

  // add only one point at a time, using current position only
  if(dt) {
    last_trailJD = date;
    TrailPoint tp;
    Vec3d v = get_heliocentric_ecliptic_pos();
    //      trail.push_front( nav->helio_to_earth_equ(v) );  // centered on earth
    tp.point = nav->helio_to_earth_pos_equ(v);  
    tp.date = date;
    trail.push_front( tp );  
      
    //      if( trail.size() > (unsigned int)MaxTrail ) {
    if( trail.size() > (unsigned int)MaxTrail ) {
      trail.pop_back();
    }
  }

  // because sampling depends on speed and frame rate, need to clear out
  // points if trail gets longer than desired
	
  list<TrailPoint>::iterator iter;  
  list<TrailPoint>::iterator end = trail.end();

  for( iter=trail.begin(); iter != end; iter++) {
    if( fabs((*iter).date - date)/DeltaTrail > MaxTrail ) {
      trail.erase(iter, end);
      break;
    }
  }
    
    
}



void planet::set_trail_color(const Vec3f _color) {
  trail_color = _color;
}

// start accumulating new trail data (clear old data)
void planet::start_trail(void) {
  first_point = 1;

  //  printf("trail for %s: %f\n", name.c_str(), re.sidereal_period);

  // only interested in trails for planets
  if(re.sidereal_period > 0) trail_on = 1;
}

// stop accumulating trail data
void planet::end_trail(void) {
  trail_on = 0;
}
