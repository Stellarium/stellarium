/*
 * Copyright (C) 2003 Fabien Chéreau
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

// Main class for stellarium
// Manage all the objects to be used in the program

#ifndef _STEL_CORE_H_
#define _STEL_CORE_H_

using namespace std;

// Globals
typedef struct {
	int X_Resolution;
	int Y_Resolution;
	int bppMode;
	int Fullscreen;

    //Files location
    char TextureDir[255];
    char ConfigDir[255];
    char DataDir[255];
} core_globals;


class stel_core
{
public:
    stel_core();
    virtual ~stel_core();
	void init(void);
	void set_config_files(char * config_file, char * location_file);
	void load_config();

	// Update all the objects in function of the time
	void update(int delta_time);

	// Execute all the drawing functions
	void draw(int delta_time);

private:
	// Load the textures "for non object oriented stuff" TODO : remove that
	void load_base_textures(void);

	navigator * navigation;				// Manage all navigation parameters, coordinate transformations etc..
	stel_object * selected_object;		// The selected object in stellarium
	Hip_Star_mgr * hip_stars;			// Manage the hipparcos stars
	Constellation_mgr * asterisms;		// Manage constellations (boundaries, names etc..)
	Nebula_mgr * nebulas;				// Manage the nebulas
	stel_atmosphere * atmosphere;		// Atmosphere
	tone_reproductor * tone_converter;	// Tones conversion between stellarium world and display device


	s_texture * texIds[200];			// Common Textures TODO : remove that!

    int LandscapeNumber;		// number of the landscape

	float fps;
    float sky_brightness;

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
};

#endif // _STEL_CORE_H_
