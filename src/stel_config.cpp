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

#include "stel_config.h"
#include "parsecfg.h"
#include "stellarium.h"
#include "navigator.h"
#include "stellastro.h"

char * tempLatitude=NULL;
char * tempLongitude=NULL;
int tempAltitude=0;
int tempTimeZone=0;
char * tempDate = NULL;
char * tempTime = NULL;
char * tempGuiBaseColor = NULL;
char * tempGuiTextColor = NULL;
double tempFov=60.;
int tempFlagFollowEarth=1;

// array of cfgStruct for reading on config file
cfgStruct cfgini[] =
{// parameter               type        address of variable
	{"FULLSCREEN",          CFG_BOOL,   &global.Fullscreen},
	{"X_RESOLUTION",        CFG_INT,    &global.X_Resolution},
    {"Y_RESOLUTION",        CFG_INT,    &global.Y_Resolution},
    {"BBP_MODE",            CFG_INT,    &global.bppMode},

    {"STAR_SCALE",          CFG_FLOAT,  &global.StarScale},
    {"STAR_TWINKLE_AMOUNT", CFG_FLOAT,  &global.StarTwinkleAmount},
    {"FLAG_FPS",            CFG_BOOL,   &global.FlagFps},
    {"FLAG_STARS",          CFG_BOOL,   &global.FlagStars},
    {"FLAG_STAR_NAME",      CFG_BOOL,   &global.FlagStarName},
    {"MAX_MAG_STAR_NAME",   CFG_FLOAT,  &global.MaxMagStarName},
    {"FLAG_PLANETS",        CFG_BOOL,   &global.FlagPlanets},
    {"FLAG_PLANETS_NAME",   CFG_BOOL,   &global.FlagPlanetsHintDrawing},
    {"FLAG_NEBULA",         CFG_BOOL,   &global.FlagNebula},
    {"FLAG_NEBULA_NAME",    CFG_BOOL,   &global.FlagNebulaName},
    {"FLAG_GROUND",         CFG_BOOL,   &global.FlagGround},
    {"FLAG_HORIZON",        CFG_BOOL,   &global.FlagHorizon},
    {"FLAG_FOG",            CFG_BOOL,   &global.FlagFog},
    {"FLAG_ATMOSPHERE",     CFG_BOOL,   &global.FlagAtmosphere},
    {"FLAG_CONSTELLATION_DRAWING",CFG_BOOL, &global.FlagConstellationDrawing},
    {"FLAG_CONSTELLATION_NAME",   CFG_BOOL, &global.FlagConstellationName},
    {"FLAG_AZIMUTAL_GRID",  CFG_BOOL,   &global.FlagAzimutalGrid},
    {"FLAG_EQUATORIAL_GRID",CFG_BOOL,   &global.FlagEquatorialGrid},
    {"FLAG_EQUATOR",        CFG_BOOL,   &global.FlagEquator},
    {"FLAG_ECLIPTIC",       CFG_BOOL,   &global.FlagEcliptic},
    {"FLAG_CARDINAL_POINTS",CFG_BOOL,   &global.FlagCardinalPoints},
    {"FLAG_MILKY_WAY",      CFG_BOOL,   &global.FlagMilkyWay},

    {"FLAG_UTC_TIME",       CFG_BOOL,   &global.FlagUTC_Time},
    {"FLAG_FOLLOW_EARTH",   CFG_BOOL,   &tempFlagFollowEarth},
    {"FLAG_MENU",           CFG_BOOL,   &global.FlagMenu},
    {"FLAG_HELP",           CFG_BOOL,   &global.FlagHelp},
    {"FLAG_INFOS",          CFG_BOOL,   &global.FlagInfos},

    {"DATE",                CFG_STRING, &tempDate},
    {"TIME",                CFG_STRING, &tempTime},

	{"GUI_BASE_COLOR",      CFG_STRING, &tempGuiBaseColor},
	{"GUI_TEXT_COLOR",      CFG_STRING, &tempGuiTextColor},

    {"LANDSCAPE_NUMBER",    CFG_INT,    &global.LandscapeNumber},

	{NULL, CFG_END, NULL}   /* no more parameters */
};


// *******************  Read the configuration file  ******************
void loadConfig(char * configFile, char * locationFile)
{

	navigation.load_position(locationFile);

    global.SkyBrightness=0;
    global.X_Resolution=800;
    global.Y_Resolution=600;
    global.Fps=60;
    global.FlagConfig = false;
    global.StarTwinkleAmount = 2;

    if (cfgParse(configFile, cfgini, CFG_SIMPLE) == -1)
    {   
		printf("An error was detected in the config file\n");
        exit(-1);
    }

	//navigation.set_flag_lock_equ_pos(tempFlagFollowEarth);

    // init the time parameters with current time and date
	ln_date date;
	get_ln_date_from_sys(&date);

    // If no date given -> default today
    if (tempDate!=NULL && strcmp(tempDate,"today"))
    {
    	if (tempDate!=NULL && (sscanf(tempDate,"%d/%d/%d\n",&(date.months),&(date.days),&(date.years)) != 3))
    	{
			printf("ERROR, bad date format : please change config.txt\n\n");
        	exit(-1);
        }
    }

	if (tempTime!=NULL && strcmp(tempTime,"now"))
	{
    	if (tempTime!=NULL && (sscanf(tempTime,"%d:%d:%lf\n",&(date.hours),&(date.minutes),&(date.seconds)) != 3))
    	{
			printf("ERROR, bad time format : please change config.txt\n\n");
        	exit(-1);
        }
    }

    if (date.months>12 || date.months<1 || date.days<1 || date.days>31)
    {
		printf("ERROR, bad month value : please change config.txt\n\n");
        exit(-1);
    }

    if (date.hours>23 || date.hours<0 || date.minutes<0 || date.minutes>59 || date.seconds<0 || date.seconds>=60)
    {
		printf("ERROR, bad time value : please change config.txt\n\n");
        exit(-1);
    }

    if (tempDate) delete tempDate;
	tempDate=NULL;

    // calc the julian date and store it in the global variable JDay
    navigation.set_JDay(get_julian_day(&date));

	float r,g,b;
	if (tempGuiBaseColor!=NULL)
	{
    	if (sscanf(tempGuiBaseColor,"%f;%f;%f\n",&r,&g,&b) != 3)
    	{
			printf("ERROR, bad gui base color format : please change config.txt\n\n");
        	exit(-1);
        }
		global.GuiBaseColor=vec3_t(r,g,b);
    }

	if (tempGuiTextColor!=NULL)
	{
    	if (sscanf(tempGuiTextColor,"%f;%f;%f\n",&r,&g,&b) != 3)
    	{
			printf("ERROR, bad gui text color format : please change config.txt\n\n");
        	exit(-1);
        }
		global.GuiTextColor=vec3_t(r,g,b);
    }

	if (tempGuiBaseColor) delete tempGuiBaseColor;
	tempGuiBaseColor=NULL;

	if (tempGuiTextColor) delete tempGuiTextColor;
	tempGuiTextColor=NULL;

}

// *******************  Dump the configuration file  **************************
void dumpConfig(void)
{
	char tempName[255];
    strcpy(tempName,global.ConfigDir);
    strcat(tempName,"config.txt");

	// Init the strings to copy in the config file
	tempDate = strdup("today");
	tempTime = strdup("now");
	tempGuiBaseColor = new char[255];
	tempGuiTextColor = new char[255];
	sprintf(tempGuiBaseColor,"%.2f;%.2f;%.2f",global.GuiBaseColor[0],global.GuiBaseColor[1],global.GuiBaseColor[2]);
	sprintf(tempGuiTextColor,"%.2f;%.2f;%.2f",global.GuiTextColor[0],global.GuiTextColor[1],global.GuiTextColor[2]);

	// Dump the config
    if (cfgDump(tempName, cfgini, CFG_SIMPLE, 0) == -1)
    {
		printf("An error occured while saving configuration file.\n");
        exit(-1);
    }

	// Free the strings
	if (tempGuiBaseColor) delete tempGuiBaseColor;
	tempGuiBaseColor=NULL;
	if (tempGuiTextColor) delete tempGuiTextColor;
	tempGuiTextColor=NULL;
    if (tempDate) delete tempDate;
	tempDate=NULL;
	if (tempTime) delete tempTime;
	tempTime=NULL;

	// Add a little help to say where the explanation is.
    FILE * f=fopen(tempName,"a+t");
    if (!f)
    {
        printf("ERROR : can't open the configuration file : %s",tempName);
        return;
    }
    fprintf(f,"#\n# See the file default_config.txt for options description.\n#");
    fclose(f);
}

void dumpLocation(void)
{
	char tempName[255];
    strcpy(tempName,global.ConfigDir);
    strcat(tempName,"location.txt");

	// Dump the location
	navigation.save_position(tempName);

	// Add info about the help.
    FILE * f=fopen(tempName,"a+t");
    if (!f)
    {
        printf("ERROR : can't open the location file : %s",tempName);
        return;
    }
    fprintf(f,"#\n# See the file location_examples.txt for options description.\n# Go on this website to findout what your location is : http://www.heavens-above.com/countries.asp\n#");
    fclose(f);

}
