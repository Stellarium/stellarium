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
 
#include "stelconfig.h"
#include "DateOps.h"
#include "parsecfg.h"

double tempLatitude=0;
double tempLongitude=0;
char * tempDate = NULL;
char * tempTime = NULL;
    
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
    {"FLAG_FOLLOW_EARTH",   CFG_BOOL,   &global.FlagFollowEarth},
    {"FLAG_MENU",           CFG_BOOL,   &global.FlagMenu},
    {"FLAG_HELP",           CFG_BOOL,   &global.FlagHelp},
    {"FLAG_INFOS",          CFG_BOOL,   &global.FlagInfos},

    {"DATE",                CFG_STRING, &tempDate},
    {"TIME",                CFG_STRING, &tempTime},
    {NULL, CFG_END, NULL}   /* no more parameters */
};

cfgStruct cfgini2[] = 
{// parameter               type        address of variable
	{"LATITUDE",            CFG_DOUBLE, &tempLatitude},
	{"LONGITUDE",           CFG_DOUBLE, &tempLongitude},
	{"ALTITUDE",            CFG_INT,    &global.Altitude},
	{"TIME_ZONE",           CFG_INT,    &global.TimeZone},
    {"LANDSCAPE_NUMBER",    CFG_INT,    &global.LandscapeNumber},
	{NULL, CFG_END, NULL}   /* no more parameters */
};
    
// *******************  setDirectories  ******************
// Set the data directories : test the default installation dir
// and try to find the files somewhere else if not found there
void setDirectories(void)
{
	
	char dataRoot[255];
    char tempName[255];
    
    strcpy(tempName,CONFIG_DATA_DIR);
    strcat(tempName,"/data/hipparcos.fab");
    FILE * tempFile = fopen(tempName,"r");
    strcpy(dataRoot,CONFIG_DATA_DIR);
    if(!tempFile)
    {    
        tempFile = fopen("./data/hipparcos.fab","r");
        strcpy(dataRoot,".");
        if(!tempFile)
        {	  	
            strcpy(dataRoot,"..");
            tempFile = fopen("../data/hipparcos.fab","r");
            if(!tempFile)
            {
            	// Failure....
            	printf("ERROR : I can't find the datas directories in :\n");
            	printf("%s/ nor in ./ nor in ../\n",CONFIG_DATA_DIR);
                printf("You may fully install the software (on POSIX systems)\n");
                printf("or go in the stellarium pakage directory.\n");
                exit(-1);
            }
        }
    }
    fclose(tempFile);

    strcpy(global.DataDir,dataRoot);
    strcpy(global.TextureDir,dataRoot);
    strcat(global.DataDir,"/data/");
    strcat(global.TextureDir,"/textures/");


#if defined(WIN32) || defined(CYGWIN) || defined(__MINGW32__) || defined(MACOSX)
	strcpy(global.ConfigDir,dataRoot);
	strcat(global.ConfigDir,"/config/");
#else
    char * homeDir = getenv("HOME");
    char tempName2[255];
    if (strcmp(dataRoot,CONFIG_DATA_DIR))
		printf(">Found data files in %s : local version.\n",dataRoot);

	// If unix system, check if the file $HOME/.stellarium/config.txt exists,
	// if not, try to create it.
    strcpy(tempName,homeDir);
    strcat(tempName,"/.stellarium/config.txt");
	if (tempFile = fopen(tempName,"r"))
	{
		strcpy(global.ConfigDir,homeDir);
		strcat(global.ConfigDir,"/.stellarium/");
		fclose(tempFile);
	}
	else
	{
		printf("Will create config files in %s/.stellarium/\n",homeDir);
		if (tempFile = fopen(tempName,"w"))
		{
			strcpy(global.ConfigDir,homeDir);
			strcat(global.ConfigDir,"/.stellarium/");
			fclose(tempFile);
		}
		else
		{
			// Try to create the directory
			printf("Try to create directory %s/.stellarium/\n",homeDir);
			strcpy(tempName2,"mkdir ");
			strcat(tempName2,homeDir);
			strcat(tempName2,"/.stellarium/");
			system(tempName2);
			
			if (tempFile = fopen(tempName,"w"))
			{
				strcpy(global.ConfigDir,homeDir);
				strcat(global.ConfigDir,"/.stellarium/");
				fclose(tempFile);
			}
			else
			{
				printf("Can't create the file %s\n",tempName);
				printf("If the directory %s/.stellarium is missing please create it.\n",homeDir);
				printf("If not check that you have access to %s/.stellarium/\n",homeDir);
				exit(-1);
			}
		}
		// First launch for that user : set default options
    	strcpy(tempName,dataRoot);
    	strcat(tempName,"/config/default_config.txt");
    	strcpy(tempName2,dataRoot);
    	strcat(tempName2,"/config/default_location.txt");
		loadConfig(tempName,tempName2);
		printf("Init config and location files\n");
		dumpConfig();
		dumpLocation();
	}
#endif
}

// *******************  Read the configuration file  ******************
void loadConfig(char * configFile, char * locationFile)
{
    global.SkyBrightness=0;
    global.Fov=60.;
    global.X_Resolution=800;
    global.Y_Resolution=600;
    global.RaZenith=0;
    global.DeZenith=0;
    global.AzVision=0;
    global.deltaFov=0;
    global.deltaAlt=0;
    global.deltaAz=0;
    global.Fps=40;
    global.FlagSelect=false;
    global.FlagRealMode=false;
    global.FlagConfig = false;
    global.StarTwinkleAmount = 2;
    global.FlagTraking = false;
    global.TimeDirection = 1;
    global.Altitude = 100;
    global.FlagRealTime=1;
    
    if (cfgParse(configFile, cfgini, CFG_SIMPLE) == -1)
    {   
		printf("An error was detected in the config file\n");
        exit(-1);
    }
    
    if (cfgParse(locationFile, cfgini2, CFG_SIMPLE) == -1)
    {
		printf("An error was detected in the location file\n");
        exit(-1);
    }
    
    // init the time parameters with current time and date
    int d,m,y,hour,min,sec;
    time_t rawtime;
    tm * ptm;
    time ( &rawtime );
    ptm = gmtime ( &rawtime );
    y=ptm->tm_year+1900;
    m=ptm->tm_mon+1;
    d=ptm->tm_mday;
    hour=ptm->tm_hour;
    min=ptm->tm_min;
    sec=ptm->tm_sec;

    // If no date given -> default today
    if (tempDate!=NULL && strcmp(tempDate,"today"))
    {
    	if (tempDate!=NULL && (sscanf(tempDate,"%d/%d/%d\n",&m,&d,&y) != 3))
    	{   
			printf("ERROR, bad date format : please change config.txt\n\n");
        	exit(1);
        }
    }
	if (tempTime!=NULL && strcmp(tempTime,"now"))
	{
    	if (tempTime!=NULL && (sscanf(tempTime,"%d:%d:%d\n",&hour,&min,&sec) != 3))
    	{   
			printf("ERROR, bad time format : please change config.txt\n\n");
        	exit(1);
        }
    }

    if (m>12 || m<1 || d<1 || d>31)
    {   
	printf("ERROR, bad month value : please change config.txt\n\n");
        exit(0);
    }

    if (hour>23 || hour<0 || min<0 || min>59 || sec<0 || sec>59)
    {   
	printf("ERROR, bad time value : please change config.txt\n\n");
        exit(0);
    }

    if (tempDate) 
	delete tempDate;
    if (tempTime) 
    {   
        hour-=global.TimeZone;
        delete tempTime;    
    }
    // calc the julian date and store it in the global variable JDay
    global.JDay=DateOps::dmyToDay(d,m,y);
    global.JDay+=((double)hour*HEURE+((double)min+(double)sec/60)*MINUTE);


    // arrange the latitude/longitude to fit with astroOps conventions
    tempLatitude=90.-tempLatitude;
    tempLongitude=-tempLongitude;
    if (tempLongitude<0)
	tempLongitude=360.+tempLongitude;
    // Set the change in the position
    global.ThePlace.setLongitude(tempLongitude);
    global.ThePlace.setLatitude(tempLatitude);
}

// *******************  Dump the configuration file  **************************
void dumpConfig(void)
{
	char tempName[255];
    strcpy(tempName,global.ConfigDir);
    strcat(tempName,"config.txt");
    if (cfgDump(tempName, cfgini, CFG_SIMPLE, 0) == -1)
    {   
		printf("An error occured while saving configuration file.\n");
        return;
    }
}

void dumpLocation(void)
{
	tempLatitude = global.ThePlace.degLatitude();
	tempLatitude = 90.-tempLatitude;
    tempLongitude = global.ThePlace.degLongitude();
    if (tempLongitude>180) tempLongitude-=360;
	tempLongitude=-tempLongitude;
	
	char tempName[255];
    strcpy(tempName,global.ConfigDir);
    strcat(tempName,"location.txt");
    if (cfgDump(tempName, cfgini2, CFG_SIMPLE, 0) == -1)
    {   
		printf("An error occured while saving location file.\n");
        return;
    }
}