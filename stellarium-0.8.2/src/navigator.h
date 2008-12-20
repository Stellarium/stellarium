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

#ifndef _NAVIGATOR_H_
#define _NAVIGATOR_H_

#include "stellarium.h"
#include "observator.h"
#include "projector.h"
#include "vecmath.h"

// Conversion in standar Julian time format
#define JD_SECOND 0.000011574074074074074074
#define JD_MINUTE 0.00069444444444444444444
#define JD_HOUR   0.041666666666666666666
#define JD_DAY    1.

extern const Mat4d mat_j2000_to_vsop87;
extern const Mat4d mat_vsop87_to_j2000;

class StelObject;

// Class which manages a navigation context
// Manage date/time, viewing direction/fov, observer position, and coordinate changes
class Navigator
{
public:
	bool do_stop;
	double a_coef, b_coef, c_coef, d_coef;

	double qube_stoping(double coef);

	enum VIEWING_MODE_TYPE
	{
		VIEW_HORIZON,
		VIEW_EQUATOR
	};
	// Create and initialise to default a navigation context
	Navigator(Observator* obs, Projector *proj = NULL);
    virtual ~Navigator();

	// Init the viewing matrix, setting the field of view, the clipping planes, and screen size
	void init_project_matrix(int w, int h, double near, double far);

	void update_time(int delta_time);
	void update_transform_matrices(void);
	void update_vision_vector(int delta_time,const StelObject &selected);

	// Update the modelview matrices
	void update_model_view_mat(void);

	// Move to the given position in equatorial or local coordinate depending on _local_pos value
	void move_to(const Vec3d& _aim, float move_duration = 1., bool _local_pos = false, int zooming = 0);

	// Loads
	void load_position(const string&);		// Load the position info in the file name given
	void save_position(const string&);		// Save the position info in the file name given

	// Time controls
	void set_JDay(double JD) {JDay=JD;}
	double get_JDay(void) const {return JDay;}
	void set_time_speed(double ts) {time_speed=ts;}
	double get_time_speed(void) const {return time_speed;}

	// Flags controls
	void set_flag_tracking(int v) {flag_tracking=v;}
	int get_flag_tracking(void) const {return flag_tracking;}
	void set_flag_lock_equ_pos(int v) {flag_lock_equ_pos=v;}
	int get_flag_lock_equ_pos(void) const {return flag_lock_equ_pos;}

	// Get vision direction
	const Vec3d& get_equ_vision(void) const {return equ_vision;}
	const Vec3d& get_prec_equ_vision(void) const {return prec_equ_vision;}
	const Vec3d& get_local_vision(void) const {return local_vision;}

	void set_local_vision(const Vec3d& _pos);

	const Planet *getHomePlanet(void) const
                      {return position->getHomePlanet();}
                    // Return the observer heliocentric position
	Vec3d get_observer_helio_pos(void) const;

	// Place openGL in earth equatorial coordinates
	void switch_to_earth_equatorial(void) const { glLoadMatrixd(mat_earth_equ_to_eye); }

	// Place openGL in heliocentric ecliptical coordinates
	void switch_to_heliocentric(void) const { glLoadMatrixd(mat_helio_to_eye); }

	// Place openGL in local viewer coordinates (Usually somewhere on earth viewing in a specific direction)
	void switch_to_local(void) const { glLoadMatrixd(mat_local_to_eye); }
	
	void set_projector(Projector *proj) {projector = proj;};

	// Transform vector from local coordinate to equatorial
	Vec3d local_to_earth_equ(const Vec3d& v) const { return mat_local_to_earth_equ*v; }
    Vec3d local_to_eye(const Vec3d& v) const { return mat_local_to_eye*v; }
    Vec3d eye_to_local(const Vec3d& v) const { return mat_local_to_eye.inverse()*v; }

	// Transform vector from equatorial coordinate to local
	Vec3d earth_equ_to_local(const Vec3d& v) const { return mat_earth_equ_to_local*v; }

	Vec3d earth_equ_to_j2000(const Vec3d& v) const { return mat_earth_equ_to_j2000*v; }
	Vec3d j2000_to_earth_equ(const Vec3d& v) const { return mat_j2000_to_earth_equ*v; }

	// Transform vector from heliocentric coordinate to local
	Vec3d helio_to_local(const Vec3d& v) const { return mat_helio_to_local*v; }

	// Transform vector from heliocentric coordinate to earth equatorial,
    // only needed in meteor.cpp
	Vec3d helio_to_earth_equ(const Vec3d& v) const { return mat_helio_to_earth_equ*v; }

	// Transform vector from heliocentric coordinate to false equatorial : equatorial
	// coordinate but centered on the observer position (usefull for objects close to earth)
	Vec3d helio_to_earth_pos_equ(const Vec3d& v) const { return mat_local_to_earth_equ*mat_helio_to_local*v; }


	// Return the modelview matrix for some coordinate systems
	const Mat4d& get_helio_to_eye_mat(void) const {return mat_helio_to_eye;}
	const Mat4d& get_earth_equ_to_eye_mat(void) const {return mat_earth_equ_to_eye;}
	const Mat4d& get_local_to_eye_mat(void) const {return mat_local_to_eye;}
	const Mat4d& get_j2000_to_eye_mat(void) const {return mat_j2000_to_eye;}

	void update_move(double deltaAz, double deltaAlt);

	void set_viewing_mode(VIEWING_MODE_TYPE view_mode);
	VIEWING_MODE_TYPE get_viewing_mode(void) const {return viewing_mode;}
	void switch_viewing_mode(void);

	void set_center_fov_ratio(float _cfr) {centerFovRatio = _cfr;};
	float get_center_fov_ratio(void) {return centerFovRatio;};

  // kornyakov: for flying to the selected object
  void set_helio_to_eye_mat(Mat4d& mat) {mat_helio_to_eye = mat;}
  void set_local_to_eye_mat(Mat4d& mat) {mat_local_to_eye = mat;}

  // kornyakov: flying to the selected object
  static double desiredAngleOfViewSelected;
  static double defaultAngleOfView;
  static int flyingSteps;
  static double distance2selected;
  static double currentTranslation;
  static double radiusOfSelected;
  static std::string nameOfSelected;
  static double currentAngleOfViewSelected;
  static double initialAngleOfViewSelected;

  double GetTranslationValue(void);
private:

  // Struct used to store data for auto mov
  typedef struct
  {
    Vec3d start;
    Vec3d aim;
    float speed;
    float coef;
    bool  local_pos;				// Define if the position are in equatorial or altazimutal
  }auto_move;

  
  float speed;
  float coef;

  // Variables for smooth x-rotating the eye matrixview
  float aimAngleX;      // target angle while we go alog x-axis, usually during tuning centerFovRatio
  float startAngleX;
  float currentAngleX;
  float centerFovRatio;

  //kornyakov: the following variables are needed for screen rotation
  //if during calibration front were not at the bottom of camera
  float aimAngleZ;
  float startAngleZ;
  float currentAngleZ;
  float centerRotationZ;

	// Matrices used for every coordinate transfo
	Mat4d mat_helio_to_local;		// Transform from Heliocentric to Observator local coordinate
	Mat4d mat_local_to_helio;		// Transform from Observator local coordinate to Heliocentric
	Mat4d mat_local_to_earth_equ;	// Transform from Observator local coordinate to Earth Equatorial
	Mat4d mat_earth_equ_to_local;	// Transform from Observator local coordinate to Earth Equatorial
	Mat4d mat_helio_to_earth_equ;	// Transform from Heliocentric to earth equatorial coordinate
	Mat4d mat_earth_equ_to_j2000;
	Mat4d mat_j2000_to_earth_equ;

	Mat4d mat_local_to_eye;			// Modelview matrix for observer local drawing
	Mat4d mat_earth_equ_to_eye;	// Modelview matrix for geocentric equatorial drawing
	Mat4d mat_j2000_to_eye;	    // precessed version
	Mat4d mat_helio_to_eye;			// Modelview matrix for heliocentric equatorial drawing

	// Vision variables
	Vec3d local_vision, equ_vision, prec_equ_vision;	// Viewing direction in local and equatorial coordinates
	int flag_tracking;				// Define if the selected object is followed
	int flag_lock_equ_pos;			// Define if the equatorial position is locked

	// Automove
	auto_move move;					// Current auto movement
  int flag_auto_move;			// Define if automove is on or off
	int zooming_mode;				// 0 : undefined, 1 zooming, -1 unzooming

	// Time variable
  double time_speed;				// Positive : forward, Negative : Backward, 1 = 1sec/sec
	double JDay;        			// Curent time in Julian day

	// Position variables
	Observator* position;
	
	//Projector
	Projector *projector;

	VIEWING_MODE_TYPE viewing_mode;   // defines if view corrects for horizon, or uses equatorial coordinates
};

#endif //_NAVIGATOR_H_
