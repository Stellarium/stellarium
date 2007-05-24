/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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
#include <sstream>
#include <iomanip>

#include "StelApp.hpp"
#include "SolarSystem.hpp"
#include "STexture.hpp"
#include "Planet.hpp"
#include "Navigator.hpp"
#include "Projector.hpp"
#include "SFont.hpp"
#include "sideral_time.h"
#include "StelTextureMgr.hpp"
#include "StelModuleMgr.hpp"

SFont* Planet::planet_name_font = NULL;
float Planet::object_scale = 1.f;
Vec3f Planet::label_color = Vec3f(.4,.4,.8);
Vec3f Planet::orbit_color = Vec3f(1,.6,1);
Vec3f Planet::trail_color = Vec3f(1,.7,.7);
LinearFader Planet::flagShow;

Planet::Planet(Planet *parent,
               const string& englishName,
               int flagHalo,
               int flag_lighting,
               double radius,
               double oblateness,
               Vec3f color,
               float albedo,
               const string& tex_map_name,
               const string& tex_halo_name,
               pos_func_type coord_func,
               OsulatingFunctType *osculating_func,
			   bool close_orbit,bool hidden) :
		englishName(englishName), flagHalo(flagHalo),
        flag_lighting(flag_lighting),
        radius(radius), one_minus_oblateness(1.0-oblateness),
        color(color), albedo(albedo), axis_rotation(0.), rings(NULL),
        sphere_scale(1.f),
        lastJD(J2000), last_orbitJD(0), deltaJD(JD_SECOND), orbit_cached(0),
        coord_func(coord_func), osculating_func(osculating_func),
        close_orbit(close_orbit), parent(parent), hidden(hidden)
{
	if (parent) parent->satellites.push_back(this);
	ecliptic_pos=Vec3d(0.,0.,0.);
    rot_local_to_parent = Mat4d::identity();
	StelApp::getInstance().getTextureManager().setDefaultParams();
	StelApp::getInstance().getTextureManager().setWrapMode(GL_REPEAT);
	tex_map = StelApp::getInstance().getTextureManager().createTexture(tex_map_name);
	StelApp::getInstance().getTextureManager().setDefaultParams();
	if (flagHalo) tex_halo = StelApp::getInstance().getTextureManager().createTexture(tex_halo_name);

	// 60 day trails
	DeltaTrail = 1;
	// small increment like 0.125 would allow observation of latitude related wobble of moon
	// if decide to show moon trail
	MaxTrail = 60;
	last_trailJD = 0; // for now
	trail_on = 0;
	first_point = 1;

	nameI18 = StelUtils::stringToWstring(englishName);
	if (englishName!="Pluto") {
	  deltaJD = 0.001*JD_SECOND;
	}
}

Planet::~Planet()
{
}

// Return the information string "ready to print" :)
wstring Planet::getInfoString(const Navigator * nav) const
{
	double tempDE, tempRA;
	wostringstream oss;

	oss << _(englishName);  // UI translation can differ from sky translation
	oss.setf(ios::fixed);
	oss.precision(1);
	if (sphere_scale != 1.f) oss << L" (x" << sphere_scale << L")";
	oss << endl;

	oss.precision(2);
	oss << _("Magnitude: ") << compute_magnitude(nav->getObserverHelioPos()) << endl;

	Vec3d equPos = get_earth_equ_pos(nav);
	StelUtils::rect_to_sphe(&tempRA,&tempDE,equPos);
	oss << _("RA/DE: ") << StelUtils::printAngleHMS(tempRA) << L"/" << StelUtils::printAngleDMS(tempDE) << endl;
	// calculate alt az position
	Vec3d localPos = nav->earth_equ_to_local(equPos);
	StelUtils::rect_to_sphe(&tempRA,&tempDE,localPos);
	tempRA = 3*M_PI - tempRA;  // N is zero, E is 90 degrees
	if(tempRA > M_PI*2) tempRA -= M_PI*2;
	oss << _("Az/Alt: ") << StelUtils::printAngleDMS(tempRA) << L"/" << StelUtils::printAngleDMS(tempDE) << endl;

	oss.precision(8);
	oss << _("Distance: ") << equPos.length() << _("AU") << endl;

	return oss.str();
}

//! Get sky label (sky translation)
wstring Planet::getSkyLabel(const Navigator * nav) const
{
	wstring ws = nameI18;
	ostringstream oss;
	oss.setf(ios::fixed);
	oss.precision(1);
	if (sphere_scale != 1.f)
	{
		oss << " (x" << sphere_scale << ")";
		ws += StelUtils::stringToWstring(oss.str());
	}
	return ws;
}


// Return the information string "ready to print" :)
wstring Planet::getShortInfoString(const Navigator * nav) const
{
	wostringstream oss;

	oss << _(englishName);  // UI translation can differ from sky translation

	oss.setf(ios::fixed);
	oss.precision(1);
	if (sphere_scale != 1.f) oss << L" (x" << sphere_scale << L")";

	oss.precision(2);
	oss << L"  " << _("Magnitude: ") << compute_magnitude(nav->getObserverHelioPos());

	Vec3d equPos = get_earth_equ_pos(nav);
	oss.precision(5);
	oss << L"  " <<  _("Distance: ") << equPos.length() << _("AU");

	return oss.str();
}

float Planet::getSelectPriority(const Navigator *nav) const
{
	if( ((SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("ssystem"))->getFlagHints() )
	{
	// easy to select, especially pluto
		return get_mag(nav)-15.f;
	}
	else
	{
		return get_mag(nav) - 8.f;
	}
}

Vec3f Planet::getInfoColor(void) const
{
	return ((SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("ssystem"))->getNamesColor();
}


double Planet::get_close_fov(const Navigator* nav) const
{
	return std::atan(radius*sphere_scale*2.f/get_earth_equ_pos(nav).length())*180./M_PI * 4;
}

double Planet::get_satellites_fov(const Navigator * nav) const
{
	// TODO: calculate from satellite orbits rather than hard code
	if (englishName=="Jupiter") return std::atan(0.005f/get_earth_equ_pos(nav).length())*180./M_PI * 4;
	if (englishName=="Saturn") return std::atan(0.005f/get_earth_equ_pos(nav).length())*180./M_PI * 4;
	if (englishName=="Mars") return std::atan(0.0001f/get_earth_equ_pos(nav).length())*180./M_PI * 4;
	if (englishName=="Uranus") return std::atan(0.002f/get_earth_equ_pos(nav).length())*180./M_PI * 4;
	return -1.;
}

double Planet::get_parent_satellites_fov(const Navigator *nav) const {
  if (parent && parent->parent) return parent->get_satellites_fov(nav);
  return -1.0;
}

// Set the orbital elements
void Planet::set_rotation_elements(float _period, float _offset, double _epoch, float _obliquity, float _ascendingNode, float _precessionRate, double _sidereal_period )
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


// Return the Planet position in rectangular earth equatorial coordinate
Vec3d Planet::get_earth_equ_pos(const Navigator * nav) const
{
	Vec3d v = get_heliocentric_ecliptic_pos();
	return nav->helio_to_earth_pos_equ(v);		// this is earth equatorial but centered
	// on observer's position (latitude, longitude)
	//return navigation.helio_to_earth_equ(&v); this is the real equatorial centered on earth center
}

Vec3d Planet::getObsJ2000Pos(const Navigator *nav) const {
  return mat_vsop87_to_j2000.multiplyWithoutTranslation(
                               get_heliocentric_ecliptic_pos()
                               - nav->getObserverHelioPos());
}


// Compute the position in the parent Planet coordinate system
// Actually call the provided function to compute the ecliptical position
void Planet::computePositionWithoutOrbits(const double date) {
  if (fabs(lastJD-date)>deltaJD) {
    coord_func(date, ecliptic_pos);
    lastJD = date;
  }
}

void Planet::compute_position(const double date)
{

	if (delta_orbitJD > 0 && (fabs(last_orbitJD-date)>delta_orbitJD || !orbit_cached))
	{

		// calculate orbit first (for line drawing)
		double date_increment = re.sidereal_period/ORBIT_SEGMENTS;
		double calc_date;
		//	  int delta_points = (int)(0.5 + (date - last_orbitJD)/date_increment);
		int delta_points;

		if( date > last_orbitJD )
		{
			delta_points = (int)(0.5 + (date - last_orbitJD)/date_increment);
		}
		else
		{
			delta_points = (int)(-0.5 + (date - last_orbitJD)/date_increment);
		}
		double new_date = last_orbitJD + delta_points*date_increment;

		//	  printf( "Updating orbit coordinates for %s (delta %f) (%d points)\n", name.c_str(), delta_orbitJD, delta_points);


		if( delta_points > 0 && delta_points < ORBIT_SEGMENTS && orbit_cached)
		{

			for( int d=0; d<ORBIT_SEGMENTS; d++ )
			{
				if(d + delta_points >= ORBIT_SEGMENTS )
				{
					// calculate new points
					calc_date = new_date + (d-ORBIT_SEGMENTS/2)*date_increment;
					// date increments between points will not be completely constant though

					compute_trans_matrix(calc_date);
                    if (osculating_func) {
                      (*osculating_func)(date,calc_date,ecliptic_pos);
                    } else {
                      coord_func(calc_date, ecliptic_pos);
                    }
					orbit[d] = get_heliocentric_ecliptic_pos();
				}
				else
				{
					orbit[d] = orbit[d+delta_points];
				}
			}

			last_orbitJD = new_date;

		}
		else if( delta_points < 0 && abs(delta_points) < ORBIT_SEGMENTS  && orbit_cached)
		{

			for( int d=ORBIT_SEGMENTS-1; d>=0; d-- )
			{
				if(d + delta_points < 0 )
				{
					// calculate new points
					calc_date = new_date + (d-ORBIT_SEGMENTS/2)*date_increment;

					compute_trans_matrix(calc_date);
                    if (osculating_func) {
                      (*osculating_func)(date,calc_date,ecliptic_pos);
                    } else {
                      coord_func(calc_date, ecliptic_pos);
                    }
					orbit[d] = get_heliocentric_ecliptic_pos();
				}
				else
				{
					orbit[d] = orbit[d+delta_points];
				}
			}

			last_orbitJD = new_date;

		}
		else if( delta_points || !orbit_cached)
		{

			// update all points (less efficient)

			for( int d=0; d<ORBIT_SEGMENTS; d++ )
			{
				calc_date = date + (d-ORBIT_SEGMENTS/2)*date_increment;
				compute_trans_matrix(calc_date);
                if (osculating_func) {
                  (*osculating_func)(date,calc_date,ecliptic_pos);
                } else {
                  coord_func(calc_date, ecliptic_pos);
                }
				orbit[d] = get_heliocentric_ecliptic_pos();
			}

			last_orbitJD = date;
			if (!osculating_func) orbit_cached = 1;
		}


		// calculate actual Planet position
		coord_func(date, ecliptic_pos);

		lastJD = date;

	}
	else if (fabs(lastJD-date)>deltaJD)
	{

		// calculate actual Planet position
		coord_func(date, ecliptic_pos);
		lastJD = date;
	}

}

// Compute the transformation matrix from the local Planet coordinate to the parent Planet coordinate
void Planet::compute_trans_matrix(double jd)
{
	axis_rotation = getSiderealTime(jd);

	// Special case - heliocentric coordinates are on ecliptic,
    // not solar equator...
	if (parent) {
      rot_local_to_parent = Mat4d::zrotation(re.ascendingNode
                                            -re.precessionRate*(jd-re.epoch))
                          * Mat4d::xrotation(re.obliquity);
	}
}

Mat4d Planet::getRotEquatorialToVsop87(void) const {
  Mat4d rval = rot_local_to_parent;
  if (parent) for (const Planet *p=parent;p->parent;p=p->parent) {
    rval = p->rot_local_to_parent * rval;
  }
  return rval;
}

void Planet::setRotEquatorialToVsop87(const Mat4d &m) {
  Mat4d a = Mat4d::identity();
  if (parent) for (const Planet *p=parent;p->parent;p=p->parent) {
    a = p->rot_local_to_parent * a;
  }
  rot_local_to_parent = a.transpose() * m;
}


void Planet::averageRotationElements(const RotationElements &r,
                                     double f1,double f2) {
  re.offset = r.offset + fmod(re.offset - r.offset
                              + 360.0*( (lastJD-re.epoch)/re.period
                                      - (lastJD-r.epoch)/r.period),
                              360.0);
  re.epoch = r.epoch;
  re.period = r.period;
  if (re.offset - r.offset < -180.f) re.offset += 360.f; else
  if (re.offset - r.offset >  180.f) re.offset -= 360.f;
  re.offset = f1*re.offset + f2*r.offset;
}

// Compute the z rotation to use from equatorial to geographic coordinates
double Planet::getSiderealTime(double jd) const
{
	if (englishName=="Earth") return get_apparent_sidereal_time(jd);

	double t = jd - re.epoch;
	double rotations = t / (double) re.period;
	double wholeRotations = floor(rotations);
	double remainder = rotations - wholeRotations;

	return remainder * 360. + re.offset;
}

// Get the Planet position in the parent Planet ecliptic coordinate
Vec3d Planet::get_ecliptic_pos() const
{
	return ecliptic_pos;
}

// Return the heliocentric ecliptical position
// used only for earth shadow, lunar eclipse
Vec3d Planet::get_heliocentric_ecliptic_pos() const
{
	Vec3d pos = ecliptic_pos;
	const Planet *p = parent;
	if (p) while (p->parent)
	{
		pos += p->ecliptic_pos;
		p = p->parent;
	}
	return pos;
}

void Planet::set_heliocentric_ecliptic_pos(const Vec3d &pos)
{
	ecliptic_pos = pos;
	const Planet *p = parent;
	if (p) while (p->parent)
	{
		ecliptic_pos -= p->ecliptic_pos;
		p = p->parent;
	}
}

// Compute the distance to the given position in heliocentric coordinate (in AU)
double Planet::compute_distance(const Vec3d& obs_helio_pos)
{
	distance = (obs_helio_pos-get_heliocentric_ecliptic_pos()).length();
	return distance;
}

// Get the phase angle for an observer at pos obs_pos in the heliocentric coordinate (dist in AU)
double Planet::get_phase(Vec3d obs_pos) const {
  const double sq = obs_pos.lengthSquared();
  const Vec3d heliopos = get_heliocentric_ecliptic_pos();
  const double Rq = heliopos.lengthSquared();
  const double pq = (obs_pos - heliopos).lengthSquared();
  const double cos_chi = (pq + Rq - sq)/(2.0*sqrt(pq*Rq));
  return (1.0 - acos(cos_chi)/M_PI) * cos_chi
         + sqrt(1.0 - cos_chi*cos_chi) / M_PI;
}

float Planet::compute_magnitude(Vec3d obs_pos) const {
  const double sq = obs_pos.lengthSquared();
  if (parent == 0) {
      // sun
    return -26.73f + 2.5f*log10f(sq);
  }
  const Vec3d heliopos = get_heliocentric_ecliptic_pos();
  const double Rq = heliopos.lengthSquared();
  const double pq = (obs_pos - heliopos).lengthSquared();
  const double cos_chi = (pq + Rq - sq)/(2.0*sqrt(pq*Rq));
  const double phase = (1.0 - acos(cos_chi)/M_PI) * cos_chi
                       + sqrt(1.0 - cos_chi*cos_chi) / M_PI;
  double F = 2.0 * albedo * radius * radius * phase / (3.0*pq*Rq);

    // Check if the satellite is inside the inner shadow of the parent planet:
  if (parent->parent != 0) {
    const Vec3d parent_heliopos = parent->get_heliocentric_ecliptic_pos();
    const double parent_Rq = parent_heliopos.lengthSquared();
    const double pos_times_parent_pos = heliopos * parent_heliopos;
    if (pos_times_parent_pos > parent_Rq) {
        // The satellite is farther away from the sun than the parent planet.
      const double sun_radius = parent->parent->radius;
      const double sun_minus_parent_radius = sun_radius - parent->radius;
      const double quot = pos_times_parent_pos/parent_Rq;
        // compute d = distance from satellite center to border
        // of inner shadow. d>0 means inside the shadow cone.
      double d = sun_radius - sun_minus_parent_radius*quot
               - sqrt( (1.0-sun_minus_parent_radius/sqrt(parent_Rq))
                     * (Rq-pos_times_parent_pos*quot) );
      if (d >= radius) {
          // The satellite is totally inside the inner shadow.
        F *= 1e-9;
      } else if (d > -radius) {
          // The satellite is partly inside the inner shadow,
          // compute a fantasy value for the magnitude:
        d /= radius;
        F *= (0.5 - (asin(d)+d*sqrt(1.0-d*d))/M_PI);
      }
    }
  }

  const double rval = -26.73 - 2.5 * log10(F);
//cout << "Planet(" << getEnglishName()
//     << ")::compute_magnitude(" << obs_pos << "): "
//        "phase: " << phase
//     << ",F: " << F
//     << ",rval: " << rval
//     << endl;
  return rval;
}

float Planet::compute_magnitude(const Navigator * nav) const
{
	return compute_magnitude(nav->getObserverHelioPos());
}

void Planet::set_big_halo(const string& halotexfile)
{
	StelApp::getInstance().getTextureManager().setDefaultParams();
	tex_big_halo = StelApp::getInstance().getTextureManager().createTexture(halotexfile);
}

// Return the radius of a circle containing the object on screen
float Planet::getOnScreenSize(const Projector *prj, const Navigator *nav) const
{
	double rad;
	if(rings) rad = rings->get_size();
	else rad = radius;

	return std::atan(rad*sphere_scale*2.f/get_earth_equ_pos(nav).length())*180./M_PI/prj->getFov()*prj->getViewportHeight();
}

// Draw the Planet and all the related infos : name, circle etc..
double Planet::draw(Projector* prj, const Navigator * nav, const ToneReproducer* eye, int flag_point, bool stencil)
{
	if (hidden) return 0;

	Mat4d mat = Mat4d::translation(ecliptic_pos)
              * rot_local_to_parent;
	const Planet *p = parent;
	while (p && p->parent)
	{
		mat = Mat4d::translation(p->ecliptic_pos)
		    * mat
		    * p->rot_local_to_parent;
		p = p->parent;
	}

	// This removed totally the Planet shaking bug!!!
	mat = nav->get_helio_to_eye_mat() * mat;

	const Vec3d sun_pos = nav->get_helio_to_eye_mat()*Vec3d(0,0,0);
	glLightfv(GL_LIGHT0,GL_POSITION,Vec4f(sun_pos[0],sun_pos[1],sun_pos[2],1.f));

	if (this == nav->getHomePlanet()) {
		if (rings) rings->draw(prj,mat,1000.0);
		return 0;
	}


	// Compute the 2D position and check if in the screen
	float screen_sz = getOnScreenSize(prj, nav);
	float viewport_left = prj->getViewportPosX();
	float viewport_bottom = prj->getViewportPosY();
	
	prj->setCustomFrame(mat);
	if (prj->project(Vec3f(0,0,0), screenPos) &&
	        screenPos[1]>viewport_bottom - screen_sz && screenPos[1]<viewport_bottom + prj->getViewportHeight()+screen_sz &&
	        screenPos[0]>viewport_left - screen_sz && screenPos[0]<viewport_left + prj->getViewportWidth() + screen_sz)
	{
		// Draw the name, and the circle if it's not too close from the body it's turning around
		// this prevents name overlaping (ie for jupiter satellites)
		float ang_dist = 300.f*atan(get_ecliptic_pos().length()/get_earth_equ_pos(nav).length())/prj->getFov();
		if (ang_dist==0.f) ang_dist = 1.f; // if ang_dist == 0, the Planet is sun..

		// by putting here, only draw orbit if Planet is visible for clarity
		draw_orbit(nav, prj);  // TODO - fade in here also...

		draw_trail(nav, prj);

		if (ang_dist>0.25)
		{
			if (ang_dist>1.f) ang_dist = 1.f;
			//glColor4f(0.5f*ang_dist,0.5f*ang_dist,0.7f*ang_dist,1.f*ang_dist);
			
			draw_hints(nav, prj);
		}

		if (rings && screen_sz>1)
		{
			const double dist = get_earth_equ_pos(nav).length();
			double z_near = 0.9*(dist - rings->get_size());
			double z_far  = 1.1*(dist + rings->get_size());
			if (z_near < 0.0) z_near = 0.0;
			double n,f;
			prj->get_clipping_planes(&n,&f); // Save clipping planes
			prj->set_clipping_planes(z_near,z_far);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			draw_sphere(prj,mat,screen_sz);
			rings->draw(prj,mat,screen_sz);
			glDisable(GL_DEPTH_TEST);
			prj->set_clipping_planes(n,f);  // Restore old clipping planes
		}
		else
		{
			if(stencil) glEnable(GL_STENCIL_TEST);
			draw_sphere(prj, mat, screen_sz);
			if(stencil) glDisable(GL_STENCIL_TEST);
		}

		if (tex_halo)
		{
			if (flag_point) draw_point_halo(nav, prj, eye);
			else draw_halo(nav, prj, eye);
		}
		if (tex_big_halo) draw_big_halo(nav, prj, eye);
	}
	double distanceSquared =
		(screenPos[0] - previousScreenPos[0]) *
		(screenPos[0] - previousScreenPos[0]) +
		(screenPos[1] - previousScreenPos[1]) *
		(screenPos[1] - previousScreenPos[1]);
	previousScreenPos = screenPos;
	return distanceSquared;
}

	
void glCircle(const Vec3d& pos, float radius)
{
	float angle, facets;

	if (radius < 2) facets = 6;
	else facets = (int)(radius*3);
	
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < facets; i++)
	{
		angle = 2.0f*M_PI*i/facets;
		glVertex3f(pos[0] + radius * sin(angle), pos[1] + radius * cos(angle), 0.0f);
	}
	glEnd();
}


void Planet::draw_hints(const Navigator* nav, const Projector* prj)
{
	if(!hint_fader.getInterstate()) return;

	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	// Draw nameI18 + scaling if it's not == 1.
	float tmp = 10.f + getOnScreenSize(prj, nav)/sphere_scale/2.f; // Shift for nameI18 printing

	glColor4f(label_color[0], label_color[1], label_color[2],hint_fader.getInterstate());
	prj->drawText(planet_name_font,screenPos[0],screenPos[1], getSkyLabel(nav), 0, tmp, tmp, false);

	// hint disapears smoothly on close view
	tmp -= 10.f;
	if (tmp<1) tmp=1;
	glColor4f(label_color[0], label_color[1], label_color[2],hint_fader.getInterstate()/tmp);

	// Draw the 2D small circle
	glCircle(screenPos, 8);
}

void Planet::draw_sphere(const Projector* prj, const Mat4d& mat, float screen_sz)
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
	if (tex_map) tex_map->bind();

	prj->setCustomFrame(mat * Mat4d::zrotation(M_PI/180*(axis_rotation + 90.)));

    // Rotate and add an extra quarter rotation so that the planet texture map
    // fits to the observers position. No idea why this is necessary,
    // perhaps some openGl strangeness, or confusing sin/cos.
    prj->sSphere(radius*sphere_scale, one_minus_oblateness, nb_facet, nb_facet);

	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);

}

void Planet::draw_halo(const Navigator* nav, const Projector* prj, const ToneReproducer* eye)
{
	float cmag;
	float rmag;

    float fov_q = prj->getFov();
    if (fov_q > 60) fov_q = 60;
    else if (fov_q < 0.1) fov_q = 0.1;
    fov_q = 1.f/(fov_q*fov_q);
    rmag = std::sqrt(eye->adapt_luminance(
        std::exp(-0.92103f*(compute_magnitude(nav->getObserverHelioPos())
                         + 12.12331f)) * 108064.73f * fov_q)) * 30.f;

//	rmag = eye->adapt_luminance(std::exp(-0.92103f*(compute_magnitude(nav->get_observer_helio_pos()) +
//	                                            12.12331f)) * 108064.73f);
//	rmag = rmag/std::pow(prj->getFov(),0.85f)*50.f;

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
//  try this one if you want to see a bright moon:
//if (rmag>4.f) {
//  rmag=4.f+2.f*std::sqrt(1.f+rmag-4.f)-2.f;
  if (rmag>8.f) {
    rmag=8.f+2.f*std::sqrt(1.f+rmag-8.f)-2.f;
  }
//}

//		if (rmag>5.f)
//		{
//			rmag=5.f+sqrt(rmag-5)/6;
//			if (rmag>9.f)
//			{
//				rmag=9.f;
//			}
//		}
	}


	// Global scaling
	rmag*=Planet::object_scale;

	glBlendFunc(GL_ONE, GL_ONE);
	float screen_r = getOnScreenSize(prj, nav);
if (screen_r<1.f) screen_r=1.f;
	cmag *= 0.5*rmag/(screen_r*screen_r*screen_r);
	if (cmag>1.f) cmag = 1.f;

	if (rmag<screen_r)
	{
		cmag*=rmag/screen_r;
		rmag = screen_r;
	}

	if (tex_halo) tex_halo->bind();
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glColor3f(color[0]*cmag, color[1]*cmag, color[2]*cmag);
	
	prj->drawSprite2dMode(screenPos[0], screenPos[1], rmag*2);
}

void Planet::draw_point_halo(const Navigator* nav, const Projector* prj, const ToneReproducer* eye)
{
	float cmag;
	float rmag;

    float fov_q = prj->getFov();
    if (fov_q > 60) fov_q = 60;
    fov_q = 1.f/(fov_q*fov_q);
    rmag =
      std::sqrt(eye->adapt_luminance(
        std::exp(-0.92103f*(compute_magnitude(nav->getObserverHelioPos())
                         + 12.12331f)) * 108064.73f * fov_q)) * 6.f;

//	rmag = eye->adapt_luminance(std::exp(-0.92103f*(compute_magnitude(nav->get_observer_helio_pos()) +
//	                                            12.12331f)) * 108064.73f);
//	rmag = rmag/std::pow(prj->getFov(),0.85f)*10.f;

	cmag = 1.f;

	// if size of star is too small (blink) we put its size to 1.2 --> no more blink
	// And we compensate the difference of brighteness with cmag
	if (rmag<0.3f) return;
	cmag=rmag*rmag/(1.4f*1.4f);
	rmag=1.4f;

	// Global scaling
	//rmag*=Planet::star_scale;

	glBlendFunc(GL_ONE, GL_ONE);
	float screen_r = getOnScreenSize(prj, nav);
	cmag *= rmag/screen_r;
	if (cmag>1.f) cmag = 1.f;

	if (rmag<screen_r)
	{
		cmag*=rmag/screen_r;
		rmag = screen_r;
	}

	if (tex_halo) tex_halo->bind();
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glColor3f(color[0]*cmag, color[1]*cmag, color[2]*cmag);
	prj->drawSprite2dMode(screenPos[0], screenPos[1], rmag*2);
}

void Planet::draw_big_halo(const Navigator* nav, const Projector* prj, const ToneReproducer* eye)
{
	float cmag;
	float rmag;
    float fov_q = prj->getFov();
    if (fov_q > 60) fov_q = 60;
    else if (fov_q < 0.1) fov_q = 0.1;
    fov_q = 1.f/(fov_q*fov_q);
    rmag = std::sqrt(eye->adapt_luminance(
        std::exp(-0.92103f*(compute_magnitude(nav->getObserverHelioPos())
                         + 12.12331f)) * 108064.73f * fov_q)) * 30.f;
	cmag = 1.f;
	if (rmag<1.2f)
	{
		if (rmag<0.3f) return;
		cmag=rmag*rmag/1.44f;
		rmag=1.2f;
	}
	else
	{
      if (rmag>8.f) {
        rmag=8.f+2.f*std::sqrt(1.f+rmag-8.f)-2.f;
      }
	}
	rmag*=Planet::object_scale;
	glBlendFunc(GL_ONE, GL_ONE);
	float screen_r = 0.25*getOnScreenSize(prj, nav);
	if (screen_r<1.f) screen_r=1.f;
	cmag *= 0.5*rmag/(screen_r*screen_r*screen_r);
	if (cmag>1.f) cmag = 1.f;

	if (rmag<screen_r)
	{
		cmag*=rmag/screen_r;
		rmag = screen_r;
	}

	if (tex_big_halo) tex_big_halo->bind();
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glColor3f(color[0]*cmag, color[1]*cmag, color[2]*cmag);

	screen_r = 0.3*rmag; // just an arbitrary scaling
      // drawSprite2dMode does not work for the big halo,
      // perhaps it is too big?
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex2f(screenPos[0]-screen_r,screenPos[1]-screen_r);
		glTexCoord2i(1,0);
		glVertex2f(screenPos[0]+screen_r,screenPos[1]-screen_r);
		glTexCoord2i(1,1);
		glVertex2f(screenPos[0]+screen_r,screenPos[1]+screen_r);
		glTexCoord2i(0,1);
		glVertex2f(screenPos[0]-screen_r,screenPos[1]+screen_r);
	glEnd();

/*
	glBlendFunc(GL_ONE, GL_ONE);
	float screen_r = getOnScreenSize(prj, nav);

	float rmag = big_halo_size/2;

	float cmag = rmag/screen_r;
	if (cmag>1.f) cmag = 1.f;

	if (rmag<screen_r*2)
	{
		cmag*=rmag/(screen_r*2);
		rmag = screen_r*2;
	}

	if (tex_big_halo) tex_big_halo->bind();
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glColor3f(color[0]*cmag, color[1]*cmag, color[2]*cmag);
	prj->drawSprite2dMode(screenPos[0], screenPos[1], rmag*2);
*/
}

Ring::Ring(double radius_min,double radius_max,const string &texname)
     :radius_min(radius_min),radius_max(radius_max) {
	tex = StelApp::getInstance().getTextureManager().createTexture(texname);
}

Ring::~Ring(void) {
}

void Ring::draw(const Projector* prj,const Mat4d& mat,double screen_sz)
{
	screen_sz -= 50;
	screen_sz /= 250.0;
	if (screen_sz < 0.0) screen_sz = 0.0;
	else if (screen_sz > 1.0) screen_sz = 1.0;
	const int slices = 128+(int)((256-128)*screen_sz);
	const int stacks = 8+(int)((32-8)*screen_sz);

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glRotatef(axis_rotation + 180.,0.,0.,1.);
	glColor3f(1.0f, 0.88f, 0.82f); // For saturn only..
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	if (tex) tex->bind();

	// TODO: radial texture would look much better

	  // solve the ring wraparound by culling:
	  // decide if we are above or below the ring plane
	const double h = mat.r[ 8]*mat.r[12]
	               + mat.r[ 9]*mat.r[13]
	               + mat.r[10]*mat.r[14];
	prj->setCustomFrame(mat);
	prj->sRing(radius_min,radius_max,(h<0.0)?slices:-slices,stacks, 0);
	glDisable(GL_CULL_FACE);
}


// draw orbital path of Planet
void Planet::draw_orbit(const Navigator * nav, const Projector* prj)
{
	if(!orbit_fader.getInterstate()) return;
	if(!re.sidereal_period) return;

	prj->setCurrentFrame(Projector::FRAME_HELIO);    // 2D coordinate

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	glColor4f(orbit_color[0], orbit_color[1], orbit_color[2], orbit_fader.getInterstate());

	int on=0;
	int d;
	Vec3d onscreen;
	for( int n=0; n<=ORBIT_SEGMENTS; n++)
	{
		if( n==ORBIT_SEGMENTS )
		{
			d = 0;  // connect loop
			if (!close_orbit) break;
		}
		else
		{
			d = n;
		}

		// special case - use current Planet position as center vertex so that draws
		// on it's orbit all the time (since segmented rather than smooth curve)
		if( n == ORBIT_SEGMENTS/2 )
		{
			if(prj->project(get_heliocentric_ecliptic_pos(), onscreen))
			{
				if(!on) glBegin(GL_LINE_STRIP);
				glVertex3d(onscreen[0], onscreen[1], 0);
				on=1;
			}
			else if( on )
			{
				glEnd();
				on=0;
			}
		}
		else
		{
			if(prj->project(orbit[d],onscreen))
			{
				if(!on) glBegin(GL_LINE_STRIP);
				glVertex3d(onscreen[0], onscreen[1], 0);
				on=1;
			}
			else if( on )
			{
				glEnd();
				on=0;
			}
		}
	}

	if(on) glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
}


// draw trail of Planet as seen from earth
void Planet::draw_trail(const Navigator * nav, const Projector* prj)
{
	if(!trail_fader.getInterstate()) return;

	Vec3d onscreen1;
	Vec3d onscreen2;

	prj->setCurrentFrame(Projector::FRAME_EARTH_EQU);

	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	glColor3fv(trail_color*trail_fader.getInterstate());

	list<TrailPoint>::iterator iter;
	list<TrailPoint>::iterator nextiter;
	list<TrailPoint>::iterator begin = trail.begin();
	//  begin++;

	if(trail.begin() != trail.end())
	{

		nextiter = trail.end();
		nextiter--;

		for( iter=nextiter; iter != begin; iter--)
		{

			nextiter--;
			if( prj->projectLineCheck( (*iter).point, onscreen1, (*(nextiter)).point, onscreen2) )
			{
				glBegin(GL_LINE_STRIP);
				glVertex2d(onscreen1[0], onscreen1[1]);
				glVertex2d(onscreen2[0], onscreen2[1]);
				glEnd();
			}
		}
	}

	// draw final segment to finish at current Planet position
	if( !first_point && prj->projectLineCheck( (*trail.begin()).point, onscreen1, get_earth_equ_pos(nav), onscreen2) )
	{
		glBegin(GL_LINE_STRIP);
		glVertex2d(onscreen1[0], onscreen1[1]);
		glVertex2d(onscreen2[0], onscreen2[1]);
		glEnd();
	}

	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
}

// update trail points as needed
void Planet::update_trail(const Navigator* nav)
{
	if(!trail_on) return;

	double date = nav->getJDay();

	int dt=0;
	if(first_point || (dt=abs(int((date-last_trailJD)/DeltaTrail))) > MaxTrail)
	{
		dt=1;
		// clear old trail
		trail.clear();
		first_point = 0;
	}

	// Note that when jump by a week or day at a time, loose detail on trails
	// particularly for moon (if decide to show moon trail)

	// add only one point at a time, using current position only
	if(dt)
	{
		last_trailJD = date;
		TrailPoint tp;
		Vec3d v = get_heliocentric_ecliptic_pos();
		//      trail.push_front( nav->helio_to_earth_equ(v) );  // centered on earth
		tp.point = nav->helio_to_earth_pos_equ(v);
		tp.date = date;
		trail.push_front( tp );

		//      if( trail.size() > (unsigned int)MaxTrail ) {
		if( trail.size() > (unsigned int)MaxTrail )
		{
			trail.pop_back();
		}
	}

	// because sampling depends on speed and frame rate, need to clear out
	// points if trail gets longer than desired

	list<TrailPoint>::iterator iter;
	list<TrailPoint>::iterator end = trail.end();

	for( iter=trail.begin(); iter != end; iter++)
	{
		if( fabs((*iter).date - date)/DeltaTrail > MaxTrail )
		{
			trail.erase(iter, end);
			break;
		}
	}
}

// Start/stop accumulating new trail data (clear old data)
void Planet::startTrail(bool b)
{
	if (b)
	{
		first_point = 1;
		//  printf("trail for %s: %f\n", name.c_str(), re.sidereal_period);
		// only interested in trails for planets
		if(re.sidereal_period > 0) trail_on = 1;
	}
	else
	{
		trail_on = 0;
	}
}

void Planet::update(int delta_time)
{
	hint_fader.update(delta_time);
	orbit_fader.update(delta_time);
	trail_fader.update(delta_time);
}
