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

#include "navigation.h"
#include "stellarium.h"

/*******************  My coordinate conventions :  *******************

Always count the RA angle from z axis to x axis 

        y
        |
        |   +A
        |DE.|
        | . |
        |___|____x
       / .  | .
     / RA . |.
   /...... .|
 z
                        */

// *******  Calc the Zenith position for the location and Time  ********
void ComputeZenith(void)
{   global.RaZenith = -AstroOps::greenwichSiderealTime(global.JDay) + global.ThePlace.longitude();
    global.RaZenith =  AstroOps::normalizeRadians(global.RaZenith);
    global.DeZenith =  Astro::PI_OVER_TWO - global.ThePlace.latitude();
}

// **** Increment/decrement smoothly the vision field and position *****
void Update_variables(void)
{   ComputeZenith();
    // to prevent strange behaviour, change the speed of mooving EVERY FRAMES
    // depending of the field of view : more it is zoomed, more the mooving 
    // speed is low (in angle)
    float depl=(float)0.6/(global.Fps+1);
    if (global.deltaAz<0)
    {   global.deltaAz = -depl*global.Fov/30;
    }
    else 
    {   if (global.deltaAz>0)
        {   global.deltaAz = depl*global.Fov/30;
        }
    }
    if (global.deltaAlt<0) 
    {   global.deltaAlt = -depl*global.Fov/30;
    }
    else 
    {   if (global.deltaAlt>0)
        {   global.deltaAlt = depl*global.Fov/30;
        }
    }
    if (global.deltaFov<0)
    {   global.deltaFov = -depl*global.Fov*5;
    }
    else 
    {   if (global.deltaFov>0)
        {   global.deltaFov = depl*global.Fov*5;
        }
    }

    // if we are zooming in or out
    if (global.deltaFov)
    {   if (global.Fov+global.deltaFov>0.005 && global.Fov+global.deltaFov<100)                 // change current zoom
            global.Fov+=global.deltaFov;
    }
    // if we are mooving in the Azimuthal angle (left/right)
    if (global.deltaAz) global.AzVision-=global.deltaAz;
    if (global.deltaAlt)
    {   if (global.AltVision+global.deltaAlt <= PI/2 && global.AltVision+global.deltaAlt >= -PI/2)
            global.AltVision+=global.deltaAlt;
    }
    
    // recalc all the position variables

    RADE_to_XYZ(global.AzVision, global.AltVision, global.XYZVisionAltAz);
    // Calc the equatorial coordinate of the direction of vision wich was in Altazimuthal coordinate
    global.XYZVision=global.XYZVisionAltAz;
    AltAz_to_equ(global.XYZVision,global.RaZenith,global.DeZenith);

    if (global.FlagAutoMove)
    {   global.XYZVision=global.Move.aim*global.Move.coef;
        vec3_t temp = global.Move.start*(1.0-global.Move.coef);
        global.XYZVision+=temp;
        global.XYZVision.Normalize();
        global.Move.coef+=global.Move.speed;
        if (global.Move.coef>=1.)
        {   global.FlagAutoMove=false;
            global.XYZVision=global.Move.aim;
        }
        // Recalc Altazimuthal pos
        global.XYZVisionAltAz=global.XYZVision;
        Equ_to_altAz(global.XYZVisionAltAz,global.RaZenith,global.DeZenith);
        XYZ_to_RADE(global.AzVision, global.AltVision, global.XYZVisionAltAz);
    }
    if (global.FlagTraking && !global.FlagAutoMove)
    {   global.XYZVision=global.SelectedObject.XYZ;
        global.XYZVisionAltAz=global.XYZVision;
        Equ_to_altAz(global.XYZVisionAltAz,global.RaZenith,global.DeZenith);
        XYZ_to_RADE(global.AzVision, global.AltVision, global.XYZVisionAltAz);
    }

    // compute sky brightness
    if (global.FlagAtmosphere)
    {
        vec3_t sunPos = SolarSystem->GetPos(0);
        sunPos.Normalize();
        Equ_to_altAz(sunPos, global.RaZenith, global.DeZenith);
        global.SkyBrightness=asin(sunPos[1])+0.1;
        if (global.SkyBrightness<0) global.SkyBrightness=0;
    }
    else
    {
        global.SkyBrightness=0;
    }
}

// *********************************************************************
// Increment time every second for real time mode and every frame for accelered time mode
void Update_time(Planet_mgr &_SolarSystem)
{   global.Frame++;
    int DeltaTemps=glutGet(GLUT_ELAPSED_TIME)-global.Timefr;
    global.Timefr+=DeltaTemps;
    if (global.Timefr-global.Timebase > 1000) 
    {   global.Fps=global.Frame*1000.0/(global.Timefr-global.Timebase);     // Calc the PFS rate
        global.Frame = 0;
        global.Timebase+=1000;
        _SolarSystem.Compute(global.JDay,global.ThePlace);
    }
    if (global.FlagRealTime)                                // real time animation
    {   global.JDay+=global.TimeDirection*SECONDE*(float)DeltaTemps/1000.0;
    }
    if (global.FlagAcceleredTime)                           // accelered time animation
    {   global.JDay+=global.TimeDirection*MINUTE;
        _SolarSystem.Compute(global.JDay,global.ThePlace);
    }
    if (global.FlagVeryFastTime)                            // very fast time animation
    {   global.JDay+=global.TimeDirection*MINUTE*60*24;
        _SolarSystem.Compute(global.JDay,global.ThePlace);
    }
    if (global.FlagFollowEarth && global.FlagRealTime)
    {   // Calc RaVision and DaVision from the vector
        XYZ_to_RADE(global.RaVision, global.DeVision, global.XYZVision);
        global.RaVision-=2.*PI*SECONDE*(double)DeltaTemps/1000.0;
        RADE_to_XYZ(global.RaVision, global.DeVision, global.XYZVision);
        // Recalc Altazimuthal pos
        global.XYZVisionAltAz=global.XYZVision;
        Equ_to_altAz(global.XYZVisionAltAz,global.RaZenith,global.DeZenith);
        XYZ_to_RADE(global.AzVision, global.AltVision, global.XYZVisionAltAz);
    }
    if (global.SelectedObject.type==2 && global.FlagSelect) // if a planet is selected
    {   _SolarSystem.Compute(global.JDay,global.ThePlace);
        SolarSystem->InfoSelect(global.SelectedObject.XYZ, global.SelectedObject.RA,global.SelectedObject.DE, global.SelectedObject.Name, global.SelectedObject.Distance,global.SelectedObject.Size);
    }
}


// ****************  Change the coord system to altazimutal  *********
void Switch_to_altazimutal(void)
{   glLoadIdentity();
    // Change it... the vertical direction is always the vector 0,1,0 in Altazimuthal coordinate
    gluLookAt(0, 0, 0,          // Observer position
              global.XYZVisionAltAz[0],global.XYZVisionAltAz[1],global.XYZVisionAltAz[2],   // direction of vision
              0,1,0);           // Vertical vector
}

// *****************  Change the coord system to equatorial  **********
void Switch_to_equatorial(void)
{   glLoadIdentity();
    Switch_to_altazimutal();
    // Make the 2 rotations to transform the Altazimuthal system to Equatorial system
    glRotated(90.-global.DeZenith*180/PI,1,0,0);
    glRotated(global.RaZenith*180/PI,0,1,0);
}

// *****************  Move to the given equatorial coord  **********
void Move_To(vec3_t _aim)
{   global.Move.aim=_aim;
    global.Move.aim.Normalize();
    global.Move.aim*=2;
    global.Move.start=global.XYZVision;
    global.Move.start.Normalize();
    global.Move.speed=0.02*80/global.Fps;
    global.Move.coef=0;
    global.FlagAutoMove = true;
    global.FlagTraking = true;
}
