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

#include "vecmath.h"

typedef struct			// We Use A Struct To Hold Application Runtime Data
{
	bool Visible;		// Is The Application Visible? Or Iconified?
	bool MouseFocus;	// Is The Mouse Cursor In The Application Field?
	bool KeyboardFocus;	// Is The Input Focus On Our Application?
}S_AppStatus;


typedef struct params{
    //Files location
    char TextureDir[255];
    char ConfigDir[255];
    char DataDir[255];

    int LandscapeNumber;		// number of the landscape

    // technical params
    int X_Resolution;
    int Y_Resolution;
    int bppMode;
    int Fullscreen;

    //FPS Calc and time measurement
    float Fps;

    // Drawing
    float SkyBrightness;
    float MaxMagStarName;
    float StarTwinkleAmount;
    float StarScale;

	// GUI
	vec3_t GuiBaseColor;
	vec3_t GuiTextColor;

    // Flags
    int FlagUTC_Time;
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
    int FlagMilkyWay;
    int FlagConfig;
} stellariumParams;

#endif /*_STELLA_TYPE_H_*/
