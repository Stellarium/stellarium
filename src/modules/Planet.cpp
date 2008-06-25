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

#include <iomanip>

#include <QTextStream>
#include <QString>
#include <QDebug>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "SkyDrawer.hpp"
#include "SolarSystem.hpp"
#include "STexture.hpp"
#include "Planet.hpp"
#include "Navigator.hpp"
#include "Projector.hpp"
#include "SFont.hpp"
#include "sideral_time.h"
#include "StelTextureMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StarMgr.hpp"

SFont* Planet::planet_name_font = NULL;
Vec3f Planet::label_color = Vec3f(0.4,0.4,0.8);
Vec3f Planet::orbit_color = Vec3f(1,0.6,1);
Vec3f Planet::trail_color = Vec3f(1,0.7,0.7);
STextureSP Planet::hintCircleTex;

Planet::Planet(Planet *parent,
               const QString& englishName,
               int flagHalo,
               int flag_lighting,
               double radius,
               double oblateness,
               Vec3f color,
               float albedo,
               const QString& tex_map_name,
               const QString& tex_halo_name,
               pos_func_type coord_func,
               OsulatingFunctType *osculating_func,
               bool aclose_orbit,
               bool hidden) 
	: englishName(englishName),
	  flagHalo(flagHalo),
	  flag_lighting(flag_lighting),
	  radius(radius), one_minus_oblateness(1.0-oblateness),
	  color(color), albedo(albedo), axis_rotation(0.), rings(NULL),
	  sphere_scale(1.f),
	  lastJD(J2000), 
	  coord_func(coord_func), osculating_func(osculating_func), parent(parent), hidden(hidden)
{
	last_orbitJD =0;
	deltaJD = JD_SECOND;
	orbit_cached = 0;
	close_orbit = aclose_orbit;
				 
	if (parent) 
		parent->satellites.push_back(this);
	ecliptic_pos=Vec3d(0.,0.,0.);
	rot_local_to_parent = Mat4d::identity();
	StelApp::getInstance().getTextureManager().setDefaultParams();
	StelApp::getInstance().getTextureManager().setWrapMode(GL_REPEAT);
	tex_map = StelApp::getInstance().getTextureManager().createTexture(tex_map_name);

	// 60 day trails
	DeltaTrail = 1;
	// small increment like 0.125 would allow observation of latitude related wobble of moon
	// if decide to show moon trail
	MaxTrail = 60;
	last_trailJD = 0; // for now
	trail_on = 0;
	first_point = 1;

	nameI18 = englishName;
	if (englishName!="Pluto") 
	{
		deltaJD = 0.001*JD_SECOND;
	}
}

Planet::~Planet()
{
	if (rings)
		delete rings;
}

// Return the information string "ready to print" :)
QString Planet::getInfoString(const StelCore* core) const
{
	const Navigator* nav = core->getNavigation();
	double tempDE, tempRA;
	
	QString str;
	QTextStream oss(&str);
	oss << QString("<font color=%1>").arg(StelUtils::vec3fToHtmlColor(getInfoColor()));
	
	oss << "<h2>" << q_(englishName);  // UI translation can differ from sky translation
	oss.setRealNumberNotation(QTextStream::FixedNotation);
	oss.setRealNumberPrecision(1);
	if (sphere_scale != 1.f)
		oss << QString::fromUtf8(" (\xC3\x97") << sphere_scale << ")";
	oss << "</h2>";

	oss << q_("Magnitude: <b>%1</b>").arg(getMagnitude(nav), 0, 'f', 2) << "<br>";

	Vec3d equPos = getObsEquatorialPos(nav);
	StelUtils::rect_to_sphe(&tempRA,&tempDE,equPos);
	oss << q_("RA/DE: %1/%2").arg(StelUtils::radToHmsStr(tempRA), StelUtils::radToDmsStr(tempDE)) << "<br>";
	// calculate alt az position
	Vec3d localPos = nav->earth_equ_to_local(equPos);
	StelUtils::rect_to_sphe(&tempRA,&tempDE,localPos);
	tempRA = 3*M_PI - tempRA;  // N is zero, E is 90 degrees
	if(tempRA > M_PI*2)
		tempRA -= M_PI*2;
	oss << q_("Az/Alt: %1/%2").arg(StelUtils::radToDmsStr(tempRA), StelUtils::radToDmsStr(tempDE)) << "<br>";

	// xgettext:no-c-format
	oss << q_("Distance: %1AU").arg(equPos.length(), 0, 'f', 8) << "<br>";
	oss << q_("Apparent diameter: %1").arg(StelUtils::radToDmsStr(2.*getAngularSize(core)*M_PI/180., true));
	return str;
}

//! Get sky label (sky translation)
QString Planet::getSkyLabel(const Navigator * nav) const
{
	QString str;
	QTextStream oss(&str);
	oss.setRealNumberPrecision(2);
	oss << nameI18;

	if (sphere_scale != 1.f)
	{
		oss << QString::fromUtf8(" (\xC3\x97") << sphere_scale << ")";
	}
	return str;
}


// Return the information string "ready to print" :)
QString Planet::getShortInfoString(const StelCore * core) const
{
	const Navigator* nav = core->getNavigation();
	QString str;
	QTextStream oss(&str);

	oss << englishName;  // UI translation can differ from sky translation

	oss.setRealNumberNotation(QTextStream::FixedNotation);
	oss.setRealNumberPrecision(1);
	if (sphere_scale != 1.f)
		oss << QString::fromUtf8(" (\xC3\x97") << sphere_scale << ")";

	oss << "  " << q_("Magnitude: %1").arg(getMagnitude(nav), 0, 'f', 2);

	Vec3d equPos = getObsEquatorialPos(nav);
	oss << "  " << q_("Distance: %1AU").arg(equPos.length(), 0, 'f', 5);
	return str;
}

float Planet::getSelectPriority(const Navigator *nav) const
{
	if( ((SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem"))->getFlagHints() )
	{
	// easy to select, especially pluto
		return getMagnitude(nav)-15.f;
	}
	else
	{
		return getMagnitude(nav) - 8.f;
	}
}

Vec3f Planet::getInfoColor(void) const
{
	return ((SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem"))->getLabelsColor();
}


double Planet::getCloseViewFov(const Navigator* nav) const
{
	return std::atan(radius*sphere_scale*2.f/getObsEquatorialPos(nav).length())*180./M_PI * 4;
}

double Planet::get_satellites_fov(const Navigator * nav) const
{
	// TODO: calculate from satellite orbits rather than hard code
	if (englishName=="Jupiter") return std::atan(0.005f/getObsEquatorialPos(nav).length())*180./M_PI * 4;
	if (englishName=="Saturn") return std::atan(0.005f/getObsEquatorialPos(nav).length())*180./M_PI * 4;
	if (englishName=="Mars") return std::atan(0.0001f/getObsEquatorialPos(nav).length())*180./M_PI * 4;
	if (englishName=="Uranus") return std::atan(0.002f/getObsEquatorialPos(nav).length())*180./M_PI * 4;
	return -1.;
}

double Planet::get_parent_satellites_fov(const Navigator *nav) const 
{
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

Vec3d Planet::getObsJ2000Pos(const Navigator *nav) const 
{
	return mat_vsop87_to_j2000.multiplyWithoutTranslation(get_heliocentric_ecliptic_pos() - nav->getObserverHelioPos());
}

// Compute the position in the parent Planet coordinate system
// Actually call the provided function to compute the ecliptical position
void Planet::computePositionWithoutOrbits(const double date) 
{
	if (fabs(lastJD-date)>deltaJD) 
	{
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
		// int delta_points = (int)(0.5 + (date - last_orbitJD)/date_increment);
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

		// qDebug( "Updating orbit coordinates for %s (delta %f) (%d points)\n", name.c_str(), delta_orbitJD, delta_points);

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
					if (osculating_func) 
					{
						(*osculating_func)(date,calc_date,ecliptic_pos);
					} 
					else 
					{
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
					} 
					else 
					{
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
				if (osculating_func) 
				{
					(*osculating_func)(date,calc_date,ecliptic_pos);
				} 
				else 
				{
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
	if (parent) 
	{
		rot_local_to_parent = Mat4d::zrotation(re.ascendingNode
		                      - re.precessionRate*(jd-re.epoch))
		                      * Mat4d::xrotation(re.obliquity);
	}
}

Mat4d Planet::getRotEquatorialToVsop87(void) const 
{
	Mat4d rval = rot_local_to_parent;
	if (parent)
	{
		for (const Planet *p=parent;p->parent;p=p->parent) 
			rval = p->rot_local_to_parent * rval;
	}
	return rval;
}

void Planet::setRotEquatorialToVsop87(const Mat4d &m) 
{
	Mat4d a = Mat4d::identity();
	if (parent)
	{
		for (const Planet *p=parent;p->parent;p=p->parent) 
			a = p->rot_local_to_parent * a;
	}
	rot_local_to_parent = a.transpose() * m;
}


// Compute the z rotation to use from equatorial to geographic coordinates
double Planet::getSiderealTime(double jd) const
{
	if (englishName=="Earth")
	{
		return get_apparent_sidereal_time(jd);
	}
 
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
	const Planet *pp = parent;
	if (pp)
	{
		while (pp->parent)
		{
			pos += pp->ecliptic_pos;
			pp = pp->parent;
		}
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
double Planet::get_phase(Vec3d obs_pos) const 
{
	const double sq = obs_pos.lengthSquared();
	const Vec3d heliopos = get_heliocentric_ecliptic_pos();
	const double Rq = heliopos.lengthSquared();
	const double pq = (obs_pos - heliopos).lengthSquared();
	const double cos_chi = (pq + Rq - sq)/(2.0*sqrt(pq*Rq));
	return (1.0 - acos(cos_chi)/M_PI) * cos_chi + sqrt(1.0 - cos_chi*cos_chi) / M_PI;
}

float Planet::getMagnitude(const Navigator * nav) const 
{
	Vec3d obs_pos = nav->getObserverHelioPos();
	const double sq = obs_pos.lengthSquared();
	if (parent == 0) {
		// sun
		return -26.73f + 2.5f*std::log10(sq);
	}
	const Vec3d heliopos = get_heliocentric_ecliptic_pos();
	const double Rq = heliopos.lengthSquared();
	const double pq = (obs_pos - heliopos).lengthSquared();
	const double cos_chi = (pq + Rq - sq)/(2.0*sqrt(pq*Rq));
	const double phase = (1.0 - std::acos(cos_chi)/M_PI) * cos_chi + std::sqrt(1.0 - cos_chi*cos_chi) / M_PI;
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
			- std::sqrt( (1.0-sun_minus_parent_radius/sqrt(parent_Rq))
			* (Rq-pos_times_parent_pos*quot) );
			if (d >= radius) 
			{
				// The satellite is totally inside the inner shadow.
				F *= 1e-9;
			} 
			else if (d > -radius) 
			{
				// The satellite is partly inside the inner shadow,
				// compute a fantasy value for the magnitude:
				d /= radius;
				F *= (0.5 - (std::asin(d)+d*std::sqrt(1.0-d*d))/M_PI);
			}
		}
	}

	const double rval = -26.73 - 2.5 * std::log10(F);
	//qDebug() << "Planet(" << getEnglishName()
	//         << ")::getMagnitude(" << obs_pos << "): "
	//         << "phase: " << phase
	//         << ",F: " << F
	//         << ",rval: " << rval;
	return rval;
}

double Planet::getAngularSize(const StelCore* core) const
{
	double rad = radius;
	if (rings)
		rad = rings->get_size();
	return std::atan2(rad*sphere_scale,getObsJ2000Pos(core->getNavigation()).length()) * 180./M_PI;
}


double Planet::getSpheroidAngularSize(const StelCore* core) const
{
	return std::atan2(radius*sphere_scale,getObsJ2000Pos(core->getNavigation()).length()) * 180./M_PI;
}

// Draw the Planet and all the related infos : name, circle etc..
void Planet::draw(StelCore* core, float maxMagLabels)
{
	if (hidden)
		return;

	Navigator* nav = core->getNavigation();
	Projector* prj = core->getProjection();
	
	Mat4d mat = Mat4d::translation(ecliptic_pos) * rot_local_to_parent;
	const Planet *p = parent;
	while (p && p->parent)
	{
		mat = Mat4d::translation(p->ecliptic_pos) * mat * p->rot_local_to_parent;
		p = p->parent;
	}

	// This removed totally the Planet shaking bug!!!
	mat = nav->get_helio_to_eye_mat() * mat;

	const Vec3d sun_pos = nav->get_helio_to_eye_mat()*Vec3d(0,0,0);
	glLightfv(GL_LIGHT0,GL_POSITION,Vec4f(sun_pos[0],sun_pos[1],sun_pos[2],1.f));

	if (this == nav->getHomePlanet())
	{
		if (rings)
			rings->draw(prj,mat,1000.0);
		return;
	}

	// Compute the 2D position and check if in the screen
	float screen_sz = getOnScreenSize(core);
	float viewport_left = prj->getViewportPosX();
	float viewport_bottom = prj->getViewportPosY();
	
	prj->setCustomFrame(mat);
	if (prj->project(Vec3f(0,0,0), screenPos) &&
	        screenPos[1]>viewport_bottom - screen_sz && screenPos[1]<viewport_bottom + prj->getViewportHeight()+screen_sz &&
	        screenPos[0]>viewport_left - screen_sz && screenPos[0]<viewport_left + prj->getViewportWidth() + screen_sz)
	{
		// Draw the name, and the circle if it's not too close from the body it's turning around
		// this prevents name overlaping (ie for jupiter satellites)
		float ang_dist = 300.f*atan(get_ecliptic_pos().length()/getObsEquatorialPos(nav).length())/prj->getFov();
		if (ang_dist==0.f)
			ang_dist = 1.f; // if ang_dist == 0, the Planet is sun..

		// by putting here, only draw orbit if Planet is visible for clarity
		draw_orbit(nav, prj);  // TODO - fade in here also...
		draw_trail(nav, prj);

		if (maxMagLabels>getMagnitude(nav) && ang_dist>0.25)
		{
			if (ang_dist>1.f) ang_dist = 1.f;
			//glColor4f(0.5f*ang_dist,0.5f*ang_dist,0.7f*ang_dist,1.f*ang_dist);
			
			draw_hints(core);
		}

		draw3dModel(core,mat,screen_sz);
	}
	return;
}

void Planet::draw3dModel(StelCore* core, const Mat4d& mat, float screen_sz)
{
	Navigator* nav = core->getNavigation();
	Projector* prj = core->getProjection();
	
	// Prepare openGL lighting parameters according to luminance
	float surfArcMin2 = getSpheroidAngularSize(core)*60;
	surfArcMin2 = surfArcMin2*surfArcMin2*M_PI;
	
	core->getSkyDrawer()->preDrawSky3dModel(surfArcMin2, getMagnitude(core->getNavigation()), flag_lighting);
	
	if (screen_sz>1.)
	{
		if (rings)
		{
			const double dist = getObsEquatorialPos(nav).length();
			double z_near = 0.9*(dist - rings->get_size());
			double z_far  = 1.1*(dist + rings->get_size());
			if (z_near < 0.0) z_near = 0.0;
			double n,f;
			prj->get_clipping_planes(&n,&f); // Save clipping planes
			prj->set_clipping_planes(z_near,z_far);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			draw_sphere(core,mat,screen_sz);
			glDisable(GL_LIGHTING);
			rings->draw(prj,mat,screen_sz);
			glEnable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			prj->set_clipping_planes(n,f);  // Restore old clipping planes
		}
		else
		{
			SolarSystem* ssm = (SolarSystem*)GETSTELMODULE("SolarSystem");
			if (this==ssm->getMoon() && ssm->near_lunar_eclipse())
			{
				// TODO: moon magnitude label during eclipse isn't accurate...
				// special case to update stencil buffer for drawing lunar eclipses
				glClear(GL_STENCIL_BUFFER_BIT);
				glClearStencil(0x0);
				glStencilFunc(GL_ALWAYS, 0x1, 0x1);
				glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);
				glEnable(GL_STENCIL_TEST);
				draw_sphere(core, mat, screen_sz);
				glDisable(GL_STENCIL_TEST);
			}
			else
			{
				draw_sphere(core, mat, screen_sz);
			}
		}
	}
	
	//qDebug() << nameI18;
	core->getSkyDrawer()->postDrawSky3dModel(screenPos[0],screenPos[1], surfArcMin2, getMagnitude(core->getNavigation()), color);
}

void Planet::draw_hints(const StelCore* core)
{
	if (!labelsFader.getInterstate())
		return;

	const Navigator* nav = core->getNavigation();
	const Projector* prj = core->getProjection();
	
	// Draw nameI18 + scaling if it's not == 1.
	float tmp = 10.f + getOnScreenSize(core)/sphere_scale/2.f; // Shift for nameI18 printing

	glColor4f(label_color[0], label_color[1], label_color[2],labelsFader.getInterstate());
	prj->drawText(planet_name_font,screenPos[0],screenPos[1], getSkyLabel(nav), 0, tmp, tmp, false);

	// hint disapears smoothly on close view
	if (!hint_fader.getInterstate())
		return;
	tmp -= 10.f;
	if (tmp<1) tmp=1;
	glColor4f(label_color[0], label_color[1], label_color[2],labelsFader.getInterstate()*hint_fader.getInterstate()/tmp*0.7);

	// Draw the 2D small circle
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Planet::hintCircleTex->bind();
	prj->drawSprite2dMode(screenPos[0], screenPos[1], 22);
}

void Planet::draw_sphere(StelCore* core, const Mat4d& mat, float screen_sz)
{
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	
	if (tex_map)
		tex_map->bind();

	// Rotate and add an extra quarter rotation so that the planet texture map
	// fits to the observers position. No idea why this is necessary,
	// perhaps some openGl strangeness, or confusing sin/cos.
	core->getProjection()->setCustomFrame(mat * Mat4d::zrotation(M_PI/180*(axis_rotation + 90.)));
	
	// Draw the spheroid itself
	// Adapt the number of facets according with the size of the sphere for optimization
	int nb_facet = (int)(screen_sz * 40/50);	// 40 facets for 1024 pixels diameter on screen
	if (nb_facet<10) nb_facet = 10;
	if (nb_facet>40) nb_facet = 40;
	glShadeModel(GL_SMOOTH);
	core->getProjection()->sSphere(radius*sphere_scale, one_minus_oblateness, nb_facet, nb_facet);
	glShadeModel(GL_FLAT);
	glDisable(GL_CULL_FACE);
}


Ring::Ring(double radius_min,double radius_max,const QString &texname)
     :radius_min(radius_min),radius_max(radius_max) 
{
	tex = StelApp::getInstance().getTextureManager().createTexture(texname);
}

Ring::~Ring(void) 
{
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
	glColor3f(1.0f, 0.88f, 0.82f); // For saturn only..
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	if (tex) tex->bind();

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

	prj->setCurrentFrame(Projector::FRAME_J2000);

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
	if( !first_point && prj->projectLineCheck( (*trail.begin()).point, onscreen1, getObsEquatorialPos(nav), onscreen2) )
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
		//Vec3d v = get_heliocentric_ecliptic_pos();
		//      trail.push_front( nav->helio_to_earth_equ(v) );  // centered on earth
		tp.point = getObsJ2000Pos(nav);//nav->helio_to_earth_pos_equ(v);
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
		//  qDebug("trail for %s: %f\n", name.c_str(), re.sidereal_period);
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
	labelsFader.update(delta_time);
	orbit_fader.update(delta_time);
	trail_fader.update(delta_time);
}
