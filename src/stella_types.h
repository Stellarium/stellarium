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

#ifndef _STELLA_TYPE_H_
#define _STELLA_TYPE_H_

#include "vector.h"
#include "AstroOps.h"

typedef struct          // Struct used to store data on the selected star
{   vec3_t XYZ;
    float Mag;
    char * Name;
    unsigned int HR;
    unsigned int MessierNum;
    unsigned int NGCNum;
    float Size;
    float RA,DE;
    int RAh,RAm;
    double Distance;
    float RAs;
    vec3_t RGB;
    char * CommonName;
    char type;
}selected_object;

typedef struct          // Struct used to store data on the auto mov
{   vec3_t start;
    vec3_t aim;
    float speed;
    float coef;
}AutoMove;

typedef struct params{
    // Navigation
    double RaZenith,DeZenith;
    double AzVision,AltVision;
    double RaVision, DeVision;
    double deltaFov,deltaAlt,deltaAz;
    double Fov;
    vec3_t XYZVisionAltAz,XYZVision;
    selected_object SelectedObject;
    AutoMove Move;
    double TimeDirection;

    // Position
    ObsInfo ThePlace;   //(Longitude,Latitude, -2);
    double JDay;        // Julian day
    int TimeZone;
    int Altitude;       // Altitude in meter
    int LandscapeNumber; // number of the landscape 

    // technical params
    int X_Resolution;
    int Y_Resolution;
    int bppMode;
    int Fullscreen;

    //FPS Calc and time measurement
    float Fps;
    int Frame;
    int Timefr;
    int Timebase;

    // Drawing
    float SkyBrightness;
    float MaxMagStarName;
    float StarTwinkleAmount;
    float StarScale;

    // Flags
    int FlagUTC_Time;
    int FlagTraking;
    int FlagFollowEarth;
    int FlagAutoMove;
    int FlagFps;
    int FlagStars;
	int FlagStarName;
    int FlagPlanets;
    int FlagPlanetsHintDrawing;
    int FlagNebula;
    int FlagNebulaName;
    int FlagNebulaCircle;
    int FlagGround;
    int FlagHorizon;
    int FlagFog;
    int FlagAtmosphere;
    int FlagConstellationDrawing;
    int FlagConstellationName;
    int FlagAzimutalGrid;
    int FlagEquatorialGrid;
    int FlagEquator;
    int FlagEcliptic;
    int FlagCardinalPoints;
    int FlagRealMode;
    int FlagRealTime;
    int FlagAcceleredTime;
    int FlagVeryFastTime;
    int FlagMenu;
    int FlagHelp;
    int FlagInfos;
    int FlagSelect;
    int FlagMilkyWay;
    int FlagConfig;

} stellariumParams;

#endif /*_STELLA_TYPE_H_*/
