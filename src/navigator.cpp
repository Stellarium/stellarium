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

#include "navigator.h"
#include "stellarium.h"
#include "stel_utility.h"


void observator_pos::save(FILE * f)
{
	// Set the position values in config file format
	char * tempLatitude=get_humanr_location(latitude);
	char * tempLongitude=get_humanr_location(longitude);
	fprintf(f,"%s %s %d %d %s\n",tempLongitude, tempLatitude, altitude, time_zone, name);
	if (tempLatitude) delete tempLatitude;
	if (tempLongitude) delete tempLongitude;
}

void observator_pos::load(FILE * f)
{
	char * tempLatitude=NULL, * tempLongitude=NULL;
	fscanf(f,"%s %s %d %d %s\n",tempLongitude, tempLatitude, &altitude, &time_zone, name);
    // set the read latitude and longitude
    longitude=get_dec_location(tempLongitude);
    latitude=get_dec_location(tempLatitude);
	if (tempLatitude) delete tempLatitude;
	if (tempLongitude) delete tempLongitude;
}

// Load the position info in the file name given
void navigator::load_position(char * fileName)
{
	FILE * f = NULL;
	f=fopen(fileName,"rt");
	if (!f)
	{
        printf("ERROR %s NOT FOUND\n",fileName);
        exit(-1);
	}

	position.load(f);

	fclose(f);
}

// Save the position info in the file name given
void navigator::save_position(char * fileName)
{
	FILE * f = NULL;
	f=fopen(fileName,"wt");
	if (!f)
	{
        printf("ERROR %s NOT FOUND\n",fileName);
        exit(-1);
	}

	position.save(f);

	fclose(f);
}

// Init the viewing matrix, setting the field of view, the clipping planes, and screen size
void navigator::init_project_matrix(int w, int h, double near, double far)
{
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, (double)w/h, near, far);
    glMatrixMode(GL_MODELVIEW);
}


void navigator::turn_right(int s)
{
	deltaAz = (s!=0);
}

void navigator::turn_left(int s)
{
	deltaAz = -1*(s!=0);
}

void navigator::turn_up(int s)
{
	deltaAlt = (s!=0);
}

void navigator::turn_down(int s)
{
	deltaAlt = -1*(s!=0);
}

void navigator::zoom_in(int s)
{
	deltaFov = -1*(s!=0);
}

void navigator::zoom_out(int s)
{
	deltaFov = (s!=0);
}


void navigator::update_transform_matrices(void)
{
/* HERE UPDATE THIS
	Mat4d mat_helio_to_local;		// Transform from Heliocentric to Observator local coordinate
	Mat4d mat_local_to_helio;		// Transform from Observator local coordinate to Heliocentric
	Mat4d mat_local_to_equ;			// Transform from Observator local coordinate to Earth Equatorial
	Mat4d mat_equ_to_local;			// Transform from Observator local coordinate to Earth Equatorial
*/
}

void navigator::update_vision_vector(int delta_time)
{

    if (FlagAutoMove)
    {
		equ_vision = move.aim*move.coef;
        Vec3d temp = move.start*(1.0-move.coef);
        equ_vision+=temp;
        equ_vision.normalize();
        move.coef+=move.speed*delta_time;
        if (move.coef>=1.)
        {
			FlagAutoMove=false;
            equ_vision=move.aim;
        }
		// Recalc local vision vector
		local_vision=equ_to_local(&equ_vision);
    }
	else
	{
    	if (FlagTraking) // Equatorial vision vector locked on selected object
		{
			equ_vision=selected_object.get_equ_pos();
			// Recalc local vision vector
			local_vision=equ_to_local(&equ_vision);
		}
		else
		{
			if (FlagLockEquPos) // Equatorial vision vector locked
			{
				// Recalc local vision vector
				local_vision=equ_to_local(&equ_vision);
			}
			else // Local vision vector locked
			{
				// Recalc equatorial vision vector
				equ_vision=local_to_equ(&local_vision);
			}
		}
	}

	// Update now the deplacement
	update_move(delta_time);

}


// Increment/decrement smoothly the vision field and position
void navigator::update_move(int delta_time)
{
	// the more it is zoomed, the more the mooving speed is low (in angle)
    double depl=move_speed*delta_time*fov;
    if (deltaAz<0)
    {
		deltaAz = -depl/30;
    }
    else
    {
		if (deltaAz>0)
        {
			deltaAz = (depl/30);
        }
    }
    if (deltaAlt<0)
    {
		deltaAlt = -depl/30;
    }
    else
    {
		if (deltaAlt>0)
        {
			deltaAlt = depl/30;
        }
    }
    if (deltaFov<0)
    {
		deltaFov = -depl*5;
    }
    else
    {
		if (deltaFov>0)
        {
			deltaFov = depl*5;
        }
    }

    // if we are zooming in or out
    if (deltaFov)
    {
		if (fov+deltaFov>0.005 && fov+deltaFov<100)  fov+=deltaFov;
    }

	double azVision, altVision;
	rect_to_sphe(&azVision,&altVision,&local_vision);

    // if we are mooving in the Azimuthal angle (left/right)
    if (deltaAz) azVision-=deltaAz;
    if (deltaAlt)
    {
		if (altVision+deltaAlt <= PI/2 && altVision+deltaAlt >= -PI/2) altVision+=deltaAlt;
    }

    // recalc all the position variables

    sphe_to_rect(azVision, altVision, &local_vision);

    // Calc the equatorial coordinate of the direction of vision wich was in Altazimuthal coordinate
    equ_vision=local_to_equ(&local_vision);

}

// *********************************************************************
// Increment time
void navigator::update_time(int delta_time)
{

    /*if (global.FlagRealTime)                                // real time animation
    {
		global.JDay+=global.TimeDirection*SECONDE*(float)DeltaTemps/1000.0;
    }
    if (global.FlagAcceleredTime)                           // accelered time animation
    {
		global.JDay+=global.TimeDirection*MINUTE;
        _Sun.computePosition(global.JDay);
    }
    if (global.FlagVeryFastTime)                            // very fast time animation
    {
		global.JDay+=global.TimeDirection*MINUTE*60*24;
        _Sun.computePosition(global.JDay);
    }*/

	JDay+=time_speed*(double)delta_time/1000.;

}



// Place openGL in earth equatorial coordinates
void navigator::switch_to_equatorial(void)
{
	switch_to_local();
    // Make the 2 rotations to transform the Altazimuthal system to Equatorial system
    /*glRotated(90.-global.DeZenith*180./PI,1,0,0);
    glRotated(global.RaZenith*180./PI,0,1,0);*/
}

// Place openGL in heliocentric coordinates
void navigator::switch_to_heliocentric(void)
{
}

// Place openGL in local viewer coordinates (Usually somewhere on earth viewing in a specific direction)
void navigator::switch_to_local(void)
{
	// Temp altaz
	glLoadIdentity();
	//Set a transfo matrix with the vertical at the zenith
	gluLookAt(0., 0., 0.,          // Observer position
              local_vision[0],local_vision[1],local_vision[2],   // direction of vision
              0.,1.,0.);           // Vertical vector
}

// Transform vector from local coordinate to equatorial
Vec3d navigator::local_to_equ(Vec3d * v)
{
	return v->transfo4d(mat_local_to_equ);
}

// Transform vector from equatorial coordinate to local
Vec3d navigator::equ_to_local(Vec3d * v)
{
	return v->transfo4d(mat_equ_to_local);
}

// Transform vector from heliocentric coordinate to local
Vec3d navigator::helio_to_local(Vec3d* v)
{
	return v->transfo4d(mat_helio_to_local);;
}

// *****************  Move to the given equatorial coord  **********
void navigator::move_to(Vec3d _aim)
{
	move.aim=_aim;
    move.aim.normalize();
    move.aim*=2;
    move.start=equ_vision;
    move.start.normalize();
    move.speed=0.5;
    move.coef=0;
    FlagAutoMove = true;
    FlagTraking = true;
}

