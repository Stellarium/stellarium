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

#include "SDL.h"

#include "stellarium_ui.h"
#include "s_font.h"
#include "s_gui.h"
#include "s_texture.h"
#include "stel_utility.h"
#include "s_gui_window.h"
#include "navigator.h"
#include "nebula_mgr.h"
#include "hip_star_mgr.h"
#include "stellarium.h"
#include "stel_config.h"
#include "stel_object.h"
#include "stellastro.h"

using namespace gui;

s_font * spaceFont = NULL;                     // The font used in the GraphicContext (=Skin)
GraphicsContext * gc = NULL;                   // The graphic context used to give the widget their drawing style
Container * Base = NULL;                       // The container who contains everything (=the desktop)
Label * btLegend = NULL;                       // The dynamic information about the button menu

FilledContainer * ContainerBtFlags = NULL;                  // Flags button List
Textured_Button * BtConstellationsDraw = NULL;
Textured_Button * BtConstellationsName = NULL;
Textured_Button * BtAzimutalGrid = NULL;
Textured_Button * BtEquatorialGrid = NULL;
Textured_Button * BtGround = NULL;
Textured_Button * BtCardinalPoints = NULL;
Textured_Button * BtAtmosphere = NULL;
Textured_Button * BtNebula = NULL;
Textured_Button * BtHelp = NULL;
Textured_Button * BtFollowEarth = NULL;
Textured_Button * BtConfig = NULL;
//Textured_Button * BtReal = NULL;

FilledTextLabel * InfoSelectLabel = NULL;      // The TextLabel displaying the infos about the selected object

FilledContainer * TopWindowsInfos = NULL;      // The top bar containing the infos
Label * DateLabel = NULL;
Label * HourLabel = NULL;
Label * FPSLabel = NULL;
Label * AppNameLabel = NULL;
Label * FOVLabel = NULL;

StdBtWin * HelpWin = NULL;                     // The window containing the help info
TextLabel * HelpTextLabel = NULL;              // The TextLabel containing the help infos

StdBtWin * InfoWin = NULL;                     // The window containing the info
TextLabel * InfoTextLabel = NULL;              // The TextLabel containing the infos

StdBtWin * ConfigWin = NULL;                   // The window containing the configuration options

FilledContainer * StarConfigContainer = NULL;
Labeled_Button * ToggleStarName = NULL;        
Label * StarLabel = NULL;
Label * StarNameLabel = NULL;
Label * StarNameMagLabel = NULL;
CursorBar * ChangeStarDrawNameBar = NULL;      // Cursor for the star name/magnitude
Label * StarTwinkleLabel = NULL;
CursorBar * ChangeStarTwinkleBar = NULL;       // Cursor for the star name/magnitude
Label * StarScaleLabel = NULL;
CursorBar * ChangeStarScaleBar = NULL;

FilledContainer * LandscapeConfigContainer = NULL;
Label * LandscapeLabel = NULL;
Labeled_Button * ToggleGround = NULL;
Labeled_Button * ToggleFog = NULL;
Labeled_Button * ToggleAtmosphere = NULL;
Labeled_Button * ToggleMilkyWay = NULL;

FilledContainer * LocationConfigContainer = NULL;
Label * LocationLabel = NULL;
CursorBar * LatitudeBar = NULL;
CursorBar * LongitudeBar = NULL;
CursorBar * AltitudeBar = NULL;
CursorBar * TimeZoneBar = NULL;
Label * LatitudeLabel = NULL;
Label * LongitudeLabel = NULL;
Label * AltitudeLabel = NULL;
Label * TimeZoneLabel = NULL;
Labeled_Button * SaveLocation = NULL;
ClickablePicture * EarthMap = NULL;

StdBtWin * TimeControlWin = NULL;              // The window containing the time controls
Container * TimeControlContainer = NULL;
Labeled_Button * TimeRW = NULL;
Labeled_Button * TimeFRW = NULL;
Labeled_Button * TimeF = NULL;
Labeled_Button * TimeNow = NULL;
Labeled_Button * TimeFF = NULL;
Labeled_Button * TimeReal = NULL;
Labeled_Button * TimePause = NULL;

/**********************************************************************************/
/*                                   CALLBACKS                                    */
/**********************************************************************************/

void BtFlagsOnClicCallBack(guiValue button,Component * bt)
{   switch (bt->getID())
    {   case 1 :    global.FlagConstellationDrawing=!global.FlagConstellationDrawing;
                    bt->setActive(global.FlagConstellationDrawing);
                    return;
        case 2 :    global.FlagConstellationName=!global.FlagConstellationName;
                    bt->setActive(global.FlagConstellationName);
                    return;
        case 3 :    global.FlagAzimutalGrid=!global.FlagAzimutalGrid;
                    bt->setActive(global.FlagAzimutalGrid);
                    return;
        case 4 :    global.FlagEquatorialGrid=!global.FlagEquatorialGrid;
                    bt->setActive(global.FlagEquatorialGrid);
                    return;
        case 5 :    global.FlagGround=!global.FlagGround;
                    bt->setActive(global.FlagGround);
                    return;
        case 6 :    global.FlagFog=!global.FlagFog;
                    bt->setActive(global.FlagFog);
                    return;
        case 7 :    global.FlagRealTime=!global.FlagRealTime;
                    bt->setActive(global.FlagRealTime);
                    return;
        case 8 :    global.FlagAcceleredTime=!global.FlagAcceleredTime;
                    bt->setActive(global.FlagAcceleredTime);
                    return;
        case 9 :    global.FlagVeryFastTime=!global.FlagVeryFastTime;
                    bt->setActive(global.FlagVeryFastTime);
                    return;
        case 10 :   global.FlagCardinalPoints=!global.FlagCardinalPoints;
                    bt->setActive(global.FlagCardinalPoints);
                    return;
        case 11 :   global.FlagAtmosphere=!global.FlagAtmosphere;
                    bt->setActive(global.FlagAtmosphere);
                    return;
        case 12 :   global.FlagNebulaName=!global.FlagNebulaName;
                    bt->setActive(global.FlagNebulaName);
                    return;
        case 13 :   global.FlagHelp=!global.FlagHelp;
                    HelpWin->setVisible(global.FlagHelp);
                    bt->setActive(global.FlagHelp);
                    return;
//        case 14 :   global.FlagRealMode=!global.FlagRealMode;
//                    bt->setActive(global.FlagRealMode);
//                    Base->setVisible(!global.FlagRealMode);
//                    break;
        case 15 :   navigation.set_flag_lock_equ_pos(!navigation.get_flag_lock_equ_pos());
                    bt->setActive(navigation.get_flag_lock_equ_pos());
                    break;
        case 16 :   global.FlagConfig=!global.FlagConfig;
                    ConfigWin->setVisible(global.FlagConfig);
                    bt->setActive(global.FlagConfig);
                    break;

                    return;

    }
}

/*******************************************************************************/

void BtFlagsOnMouseOverCallBack(guiValue event,Component * bt)
{   if (event==GUI_MOUSE_LEAVE) btLegend->setVisible(false);
    else btLegend->setVisible(true);
    switch (bt->getID())
    {   case 1 : btLegend->setLabel("Drawing of the Constellations [C]"); return;
        case 2 : btLegend->setLabel("Names of the Constellations [V]"); return;
        case 3 : btLegend->setLabel("Azimutal Grid"); return;
        case 4 : btLegend->setLabel("Equatorial Grid"); return;
        case 5 : btLegend->setLabel("Ground [G]"); return;
        case 6 : btLegend->setLabel("Fog [F]"); return;
        case 7 : btLegend->setLabel("Real Time Mode [1]"); return;
        case 8 : btLegend->setLabel("Accelered Time Mode [2]"); return;
        case 9 : btLegend->setLabel("Very Fast Time Mode [3]"); return;
        case 10 :btLegend->setLabel("Cardinal Points [Q]"); return;
        case 11 :btLegend->setLabel("Atmosphere [A]"); return;
        case 12 :btLegend->setLabel("Nebulas [N]"); return;
        case 13 :btLegend->setLabel("Help [H]"); return;
//        case 14 :btLegend->setLabel("Real Mode"); return;
        case 15 :btLegend->setLabel("Compensation of the Earth rotation"); return;
        case 16 :btLegend->setLabel("Configuration window"); return;
    }
}

/*******************************************************************************/

void HelpWinHideCallback(void)
{   global.FlagHelp=false;
    HelpWin->setVisible(false);
    BtHelp->setActive(false);
}

/*******************************************************************************/

void InfoWinHideCallback(void)
{   global.FlagInfos=false;
    InfoWin->setVisible(false);
}

/*******************************************************************************/

void ConfigWinHideCallback(void) 
{   global.FlagConfig=false;
    ConfigWin->setVisible(false);
    BtConfig->setActive(false);
}

/*******************************************************************************/

void TimeControlWinHideCallback(void) 
{    TimeControlWin->setVisible(false);
}

/*******************************************************************************/

void ChangeStarDrawNameBarOnChangeValue(float value,Component *)
{   global.MaxMagStarName=value;
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Show if Magnitude < %.1f",global.MaxMagStarName);
    StarNameMagLabel->setLabel(tempValueStr);
}

/*******************************************************************************/

void ChangeStarTwinkleBarOnChangeValue(float value,Component *)
{   global.StarTwinkleAmount=value;
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Twinkle Amount : %.1f",global.StarTwinkleAmount);
    StarTwinkleLabel->setLabel(tempValueStr);
}

/*******************************************************************************/

void ChangeStarScaleBarOnChangeValue(float value,Component *)
{   global.StarScale=value;
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Scale : %.1f",global.StarScale);
    StarScaleLabel->setLabel(tempValueStr);
}

/*******************************************************************************/

void ToggleStarNameOnClicCallback(guiValue button,Component *)
{   if (global.FlagStarName)
    {   if (ToggleStarName)
            ToggleStarName->setLabel("OFF");
        global.FlagStarName=false;
    }
    else
    {   if (ToggleStarName) ToggleStarName->setLabel("ON");
        global.FlagStarName=true;
    }
}

/*******************************************************************************/

void ToggleGroundOnClicCallback(guiValue button,Component *)
{   
    global.FlagGround=!global.FlagGround;
    BtGround->setActive(global.FlagGround);
}

void ToggleFogOnClicCallback(guiValue button,Component *)
{   
    global.FlagFog=!global.FlagFog;
}

void ToggleAtmosphereOnClicCallback(guiValue button,Component *)
{   
	global.FlagAtmosphere=!global.FlagAtmosphere;
    BtAtmosphere->setActive(global.FlagAtmosphere);
}

void ToggleMilkyWayOnClicCallback(guiValue button,Component *)
{   
	global.FlagMilkyWay=!global.FlagMilkyWay;
}

/*******************************************************************************/

void TimeControlBtOnClicCallback(guiValue button,Component * caller)
{   
	if(caller==TimeNow)
    {	
		navigation.set_JDay(get_julian_from_sys());
    	return;
	}

    if(caller==TimeFRW)
    {
        navigation.set_time_speed(-JD_DAY);
    }
    if(caller==TimeRW)
    {
		navigation.set_time_speed(-JD_MINUTE*10.);
    }
    if(caller==TimePause)
    {
		navigation.set_time_speed(0.);
	}
    if(caller==TimeReal)
    {
		navigation.set_time_speed(JD_SECOND);
    }
    if(caller==TimeF)
    {
		navigation.set_time_speed(JD_MINUTE*10.);
    }
    if(caller==TimeFF)
    {
		navigation.set_time_speed(JD_DAY);
    }
}

/**********************************************************************************/

void LatitudeBarOnChangeValue(float value, Component *)
{
    navigation.set_latitude(value);
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Latitude : %s",print_angle_dms_stel(navigation.get_latitude()));
    LatitudeLabel->setLabel(tempValueStr);
	EarthMap->setPointerPosition(vec2_t(EarthMap->getPointerPosition()[0],1-((value-90.)/180.)*(EarthMap->getSize())[1]));
}
void LongitudeBarOnChangeValue(float value,Component *)
{
    navigation.set_longitude(value);
    char tempValueStr[30];
	sprintf(tempValueStr,"\1 Longitude : %s",print_angle_dms_stel(navigation.get_longitude()));
    LongitudeLabel->setLabel(tempValueStr);
	EarthMap->setPointerPosition(vec2_t((value+180.)/360.*EarthMap->getSize()[0],EarthMap->getPointerPosition()[1]));
}
void AltitudeBarOnChangeValue(float value,Component *)
{
    navigation.set_altitude((int)value);
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Altitude : %dm",navigation.get_altitude());
    AltitudeLabel->setLabel(tempValueStr);
}
void TimeZoneBarOnChangeValue(float value,Component *)
{   
    navigation.set_time_zone((int)value);
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 TimeZone : %d h",navigation.get_time_zone());
    TimeZoneLabel->setLabel(tempValueStr);
}

void SaveLocationOnClicCallback(guiValue button,Component *)
{  
	dumpLocation();
}

void EarthMapOnChangeValue(vec2_t posPoint,Component *)
{
	vec2_i sz = EarthMap->getSize();
	float longi = 360.0*posPoint[0]/sz[0]-180;
	float lat = -180.0*posPoint[1]/sz[1]+90;
	LongitudeBar->setValue(longi);
	LatitudeBar->setValue(lat);
}

/**********************************************************************************/
/*                                   INIT UI                                      */
/**********************************************************************************/
void initUi(void)
{   
    /*** fonts ***/
    char tempName[255];
    strcpy(tempName,global.DataDir);
    strcat(tempName,"spacefont.txt");
    spaceFont = new s_font(13, "spacefont", tempName);
    if (!spaceFont)
    {
        printf("ERROR WHILE CREATING FONT\n");
        exit(1);
    }

    /*** Colors ***/
    vec3_t clWhite(1.,1.,1.);

    /*** Graphic Context ***/
    gc = new GraphicsContext(global.X_Resolution,global.Y_Resolution);
    if (!gc)
    {
        printf("ERROR WHILE CREATING GRAPHIC CONTEXT\n");
        exit(-1);
    }

	gc->baseColor=global.GuiBaseColor;
	gc->textColor=global.GuiTextColor;

    gc->setFont(spaceFont);
    gc->backGroundTexture=new s_texture("backmenu",TEX_LOAD_TYPE_PNG_ALPHA);
    gc->headerTexture=new s_texture("headermenu",TEX_LOAD_TYPE_PNG_ALPHA);

    /*** temporary variables ***/
    int avgCharLen=(int)gc->getFont()->getAverageCharLen();
    int lineHeight=(int)gc->getFont()->getLineHeight();

/**********************************************************************************/
/*** Flag Buttons list ***/
    int btSize=24;
    int xpos=0;
    vec2_i pos(xpos,0);
    vec2_i btsz(btSize,btSize);
    vec2_i step(btSize,0);

    BtConstellationsDraw = new Textured_Button(new s_texture("Bouton1"),pos,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,1,global.FlagConstellationDrawing);
    BtConstellationsName = new Textured_Button(new s_texture("Bouton2"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,2,global.FlagConstellationName);
    BtAzimutalGrid = new Textured_Button(new s_texture("Bouton3"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,3,global.FlagAzimutalGrid);
    BtEquatorialGrid = new Textured_Button(new s_texture("Bouton3"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,4,global.FlagEquatorialGrid);
    BtNebula = new Textured_Button(new s_texture("Bouton15"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,12,global.FlagNebulaName);
    BtGround = new Textured_Button(new s_texture("Bouton4"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,5,global.FlagGround);
    BtCardinalPoints = new Textured_Button(new s_texture("Bouton8"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,10,global.FlagCardinalPoints);
    BtAtmosphere = new Textured_Button(new s_texture("Bouton9"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,11,global.FlagAtmosphere);
    BtHelp = new Textured_Button(new s_texture("Bouton11"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,13,global.FlagHelp);
    BtFollowEarth = new Textured_Button(new s_texture("Bouton13"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,15,navigation.get_flag_lock_equ_pos());
    BtConfig = new Textured_Button(new s_texture("Bouton16"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,16,global.FlagConfig);

    /*** Button container ***/
    ContainerBtFlags = new FilledContainer();
    ContainerBtFlags->reshape(0,global.Y_Resolution-btSize/*-1*/,btSize*11,btSize/*+1*/);
    ContainerBtFlags->addComponent(BtConstellationsDraw);
    ContainerBtFlags->addComponent(BtConstellationsName);
    ContainerBtFlags->addComponent(BtAzimutalGrid);
    ContainerBtFlags->addComponent(BtEquatorialGrid);
    ContainerBtFlags->addComponent(BtGround);
    ContainerBtFlags->addComponent(BtCardinalPoints);
    ContainerBtFlags->addComponent(BtAtmosphere);
    ContainerBtFlags->addComponent(BtNebula);
    ContainerBtFlags->addComponent(BtHelp);
    ContainerBtFlags->addComponent(BtFollowEarth);
    ContainerBtFlags->addComponent(BtConfig);

/**********************************************************************************/
/*** button legend label ***/
    btLegend = new Label("ERROR, this shouldn't be seen...");
    btLegend->reshape(3,global.Y_Resolution-btSize-lineHeight-5,100,17);
    btLegend->setVisible(false);

/**********************************************************************************/
/*** help TextLabel ***/
    HelpTextLabel = new TextLabel(
        "4 Directions : Deplacement RA/DE\n\
Page Up/Down : Zoom\n\
CTRL + Up/Down : Zoom\n\
Left Mouse Clic : Select Star\n\
Right Mouse Clic  : Clear Pointer\n\
CTRL + Left Mouse Clic  : Clear Pointer\n\
SPACE or Middle Mouse Clic:\n\
     Center On Selected Object\n\
C   : Drawing of the Constellations\n\
V   : Names of the Constellations\n\
E   : Equatorial Grid\n\
Z   : Azimutal Grid\n\
N   : Nebulas\n\
P   : Planet Finder\n\
G   : Ground\n\
F   : Fog\n\
1   : Real Time Mode\n\
2   : Accelered Time Mode\n\
3   : Very Fast Time Mode\n\
Q   : Cardinal Points\n\
A   : Atmosphere\n\
H   : Help\n\
4   : Ecliptic\n\
5   : Equator\n\
T   : Object Tracking\n\
S   : Stars\n\
I   : About Stellarium\n\
ESC : Quit\n\
F1  : Toggle fullscreen if possible.\n"
    ,gc->getFont());
    HelpTextLabel->reshape(avgCharLen*2,lineHeight,45*avgCharLen,35*lineHeight);

    HelpWin = new StdBtWin(global.X_Resolution/2-25*avgCharLen, global.Y_Resolution/2-30*lineHeight/2, 48*avgCharLen, 32*(lineHeight+1), "Help", Base, spaceFont);
    HelpWin->addComponent(HelpTextLabel);
    HelpWin->setVisible(global.FlagHelp);
    HelpWin->setHideCallback(HelpWinHideCallback);

/**********************************************************************************/
/*** Info TextLabel ***/
    InfoTextLabel = new TextLabel(
"                 \1   " APP_NAME "  April 2003  \1\n\
 \n\
\1   Copyright (c) 2002 Fabien Chereau\n\
 \n\
\1   Please send bug report & comments to stellarium@free.fr\n\n\
 \n\
\1   This program is free software; you can redistribute it and/or\n\
modify it under the terms of the GNU General Public License\n\
as published by the Free Software Foundation; either version 2\n\
of the License, or (at your option) any later version.\n\
 \n\
This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; without even the implied\n\
warranty ofMERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.  See the GNU General Public\n\
License for more details.\n\
 \n\
You should have received a copy of the GNU General Public\n\
License along with this program; if not, write to the\n\
Free Software Foundation, Inc., 59 Temple Place - Suite 330\n\
Boston, MA  02111-1307, USA.\n"
    ,gc->getFont());
    InfoTextLabel->reshape(avgCharLen*3,lineHeight,62*avgCharLen,35*lineHeight);
    
    InfoWin = new StdBtWin(global.X_Resolution/2-25*avgCharLen, global.Y_Resolution/2-30*lineHeight/2, 67*avgCharLen, 25*(lineHeight+1), "About Stellarium", Base, spaceFont);
    InfoWin->addComponent(InfoTextLabel);
    InfoWin->setVisible(global.FlagInfos);
    InfoWin->setHideCallback(InfoWinHideCallback);

/**********************************************************************************/
/*** InfoSelect TextLabel ***/
    InfoSelectLabel = new FilledTextLabel("Info",gc->getFont());
    InfoSelectLabel->reshape(3,lineHeight+2,avgCharLen*50,6*lineHeight);
    InfoSelectLabel->setVisible(false);

/**********************************************************************************/
/*** TopWindowsInfos Container ***/
    /*** Date Label ***/
    DateLabel = new Label("-");
    DateLabel->reshape(3,2,10*avgCharLen,lineHeight);
    /*** Hour Label ***/
    HourLabel = new Label("-");
    HourLabel->reshape(12*avgCharLen+15,2,6*avgCharLen,lineHeight);
    /*** FPS Label ***/
    FPSLabel = new Label("-");
    FPSLabel->reshape(global.X_Resolution-13*avgCharLen,2,10*avgCharLen,lineHeight);
    if (!global.FlagFps) FPSLabel->setVisible(false);
    /*** FOV Label ***/
    FOVLabel = new Label("-");
    FOVLabel->reshape(global.X_Resolution-26*avgCharLen,2,10*avgCharLen,lineHeight);
    /*** AppName Label ***/
    AppNameLabel = new Label(APP_NAME);
    AppNameLabel->reshape((int)(global.X_Resolution/2-gc->getFont()->getStrLen(APP_NAME)/2),2, (int)(gc->getFont()->getStrLen(APP_NAME)),lineHeight);
    /*** TopWindowsInfos Container ***/    
    TopWindowsInfos = new FilledContainer();
    TopWindowsInfos->reshape(0,0,global.X_Resolution,lineHeight+2);
    TopWindowsInfos->addComponent(DateLabel);
    TopWindowsInfos->addComponent(HourLabel);
    TopWindowsInfos->addComponent(FPSLabel);
    TopWindowsInfos->addComponent(FOVLabel);
    TopWindowsInfos->addComponent(AppNameLabel);

/**********************************************************************************/
/*** ConfigWindow ***/
    /*** Star Config container ***/
    StarConfigContainer = new FilledContainer();
    if (!StarConfigContainer)
    {
        printf("ERROR WHILE CREATING UI CONTAINER\n");
        exit(1);
    }
    StarConfigContainer->reshape(4,3,180,150);

    StarLabel = new Label("STARS :");
    StarLabel->reshape(3,3,15,15);

    StarNameLabel = new Label("\1 Names :");
    StarNameLabel->reshape(15,20,20,15);
    if (global.FlagStarName) ToggleStarName = new Labeled_Button("ON");
    else ToggleStarName = new Labeled_Button("OFF");
    ToggleStarName->reshape(80,18,50,18);
    ToggleStarName->setOnClicCallback(ToggleStarNameOnClicCallback);
 
    StarNameMagLabel = new Label("Show if Magnitude < --");
    StarNameMagLabel->reshape(15,45,20,15);
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Show if Magnitude < %.1f",global.MaxMagStarName);
    StarNameMagLabel->setLabel(tempValueStr);
    ChangeStarDrawNameBar = new CursorBar(vec2_i(15,60), vec2_i(150,10), -1.5,7.,global.MaxMagStarName,ChangeStarDrawNameBarOnChangeValue);
    
    StarTwinkleLabel = new Label("\1 Twinkle Amount : --");
    char tempValue2Str[30];
    sprintf(tempValue2Str,"\1 Twinkle Amount : %.1f",global.StarTwinkleAmount);
    StarTwinkleLabel->setLabel(tempValue2Str);
    StarTwinkleLabel->reshape(15,80,50,15);
    ChangeStarTwinkleBar = new CursorBar(vec2_i(15,95), vec2_i(150,10),0.,10.,global.StarTwinkleAmount,ChangeStarTwinkleBarOnChangeValue);

    StarScaleLabel = new Label("\1 Scale : --");
    char tempValue3Str[30];
    sprintf(tempValue3Str,"\1 Scale : %.1f",global.StarScale);
    StarScaleLabel->setLabel(tempValue3Str);
    StarScaleLabel->reshape(15,115,50,15);
    ChangeStarScaleBar = new CursorBar(vec2_i(15,130), vec2_i(150,10),0.,10.,global.StarScale,ChangeStarScaleBarOnChangeValue);

    StarConfigContainer->addComponent(ChangeStarDrawNameBar);
    StarConfigContainer->addComponent(ToggleStarName);
    StarConfigContainer->addComponent(StarLabel);
    StarConfigContainer->addComponent(StarNameLabel);
    StarConfigContainer->addComponent(StarNameMagLabel);
    StarConfigContainer->addComponent(StarTwinkleLabel);
    StarConfigContainer->addComponent(ChangeStarTwinkleBar);
    StarConfigContainer->addComponent(StarScaleLabel);
    StarConfigContainer->addComponent(ChangeStarScaleBar);
 
    /*** Landcape config container ***/
    LandscapeConfigContainer = new FilledContainer();
    if (!LandscapeConfigContainer)
    {
        printf("ERROR WHILE CREATING UI CONTAINER\n");
        exit(1);
    }
    LandscapeConfigContainer->reshape(4,157,180,110);

    LandscapeLabel = new Label("LANDSCAPE :");
    LandscapeLabel->reshape(3,3,50,15);

    ToggleGround = new Labeled_Button("Ground");
    ToggleGround->reshape(25,23,130,18);
    ToggleGround->setOnClicCallback(ToggleGroundOnClicCallback);

    ToggleFog = new Labeled_Button("Fog");
    ToggleFog->reshape(25,43,130,18);
    ToggleFog->setOnClicCallback(ToggleFogOnClicCallback);


    ToggleAtmosphere = new Labeled_Button("Atmosphere");
    ToggleAtmosphere->reshape(25,63,130,18);
    ToggleAtmosphere->setOnClicCallback(ToggleAtmosphereOnClicCallback);

    ToggleMilkyWay = new Labeled_Button("MilkyWay");
    ToggleMilkyWay->reshape(25,83,130,18);
    ToggleMilkyWay->setOnClicCallback(ToggleMilkyWayOnClicCallback);

    LandscapeConfigContainer->addComponent(LandscapeLabel);
    LandscapeConfigContainer->addComponent(ToggleGround);
    LandscapeConfigContainer->addComponent(ToggleFog);
    LandscapeConfigContainer->addComponent(ToggleAtmosphere);
    LandscapeConfigContainer->addComponent(ToggleMilkyWay);


    /*** Location config container ***/
    LocationConfigContainer = new FilledContainer();
    if (!LocationConfigContainer)
    {
        printf("ERROR WHILE CREATING UI CONTAINER\n");
        exit(1);
    }
    LocationConfigContainer->reshape(187,3,340,264);

    LocationLabel = new Label("LOCATION :");
    LocationLabel->reshape(3,3,15,15);

    LatitudeLabel = new Label("Latitude : ");
    LatitudeLabel->reshape(15,20,20,15);
	sprintf(tempValueStr,"\1 Latitude : %s",print_angle_dms_stel(navigation.get_latitude()));
    LatitudeLabel->setLabel(tempValueStr);

    LatitudeBar = new CursorBar(vec2_i(15,35), vec2_i(150,10),-90.,90.,
		navigation.get_latitude(),LatitudeBarOnChangeValue);

    LongitudeLabel = new Label("Longitude : ");
    LongitudeLabel->reshape(15,60,20,15);
	sprintf(tempValueStr,"\1 Longitude : %s",print_angle_dms_stel(navigation.get_longitude()));
    LongitudeLabel->setLabel(tempValueStr);
    LongitudeBar = new CursorBar(vec2_i(15,75), vec2_i(150,10),-180,180,
		navigation.get_longitude(),LongitudeBarOnChangeValue);

    AltitudeLabel = new Label("Altitude : ");
    AltitudeLabel->reshape(170,20,20,15);
    sprintf(tempValueStr,"\1 Altitude : %dm",navigation.get_altitude());
    AltitudeLabel->setLabel(tempValueStr);
    AltitudeBar = new CursorBar(vec2_i(170,35), vec2_i(150,10),-500., 10000.,navigation.get_altitude(),AltitudeBarOnChangeValue);

    TimeZoneLabel = new Label("TimeZone : ");
    TimeZoneLabel->reshape(170,60,20,15);
    sprintf(tempValueStr,"\1 TimeZone : %d",navigation.get_time_zone());
    TimeZoneLabel->setLabel(tempValueStr);
    TimeZoneBar = new CursorBar(vec2_i(170,75), vec2_i(150,10), -12.,13.,navigation.get_time_zone(),TimeZoneBarOnChangeValue);
    
    SaveLocation = new Labeled_Button("Save location");
    SaveLocation->reshape(120,240,100,20);
    SaveLocation->setOnClicCallback(SaveLocationOnClicCallback);

    EarthMap = new ClickablePicture(vec2_i(30,95),vec2_i(280,140),new s_texture("earthmap"),EarthMapOnChangeValue);
	EarthMap->setPointerPosition(vec2_t((LongitudeBar->getValue()+180.)/360.*EarthMap->getSize()[0],
		(-LatitudeBar->getValue()+90.)/180.*EarthMap->getSize()[1]));

    LocationConfigContainer->addComponent(LocationLabel);
    LocationConfigContainer->addComponent(LatitudeBar);
    LocationConfigContainer->addComponent(LongitudeBar);
    LocationConfigContainer->addComponent(AltitudeBar);
    LocationConfigContainer->addComponent(TimeZoneBar);
    LocationConfigContainer->addComponent(LatitudeLabel);
    LocationConfigContainer->addComponent(LongitudeLabel);
    LocationConfigContainer->addComponent(AltitudeLabel);
    LocationConfigContainer->addComponent(TimeZoneLabel);
    LocationConfigContainer->addComponent(SaveLocation);
    LocationConfigContainer->addComponent(EarthMap);

    /*** Config window ***/
    ConfigWin = new StdBtWin(40, 40, 532, 287, "Configuration", Base, spaceFont);
    ConfigWin->addComponent(StarConfigContainer);
    ConfigWin->addComponent(LandscapeConfigContainer);
    ConfigWin->addComponent(LocationConfigContainer);
    ConfigWin->setHideCallback(ConfigWinHideCallback);
    ConfigWin->setVisible(global.FlagConfig);

/**********************************************************************************/
/*** Time control win ***/
    TimeControlWin = new StdBtWin(gc->winW-140, gc->winH-lineHeight, 140, 200, "Time Control", Base, spaceFont);
    TimeControlWin->setInSize(vec2_i(140,lineHeight));
    TimeControlWin->setHideCallback(TimeControlWinHideCallback);

    TimeControlContainer = new Container();
    int btXSize = 19;
    int btYSize = lineHeight;
    TimeControlContainer->reshape(0,0,140,btYSize);
    TimeFRW = new Labeled_Button("\2\2");
    TimeFRW->reshape(0,0,btXSize,btYSize);
    TimeFRW->setOnClicCallback(TimeControlBtOnClicCallback);
    TimeRW = new Labeled_Button("\2");
    TimeRW->reshape(btXSize+1,0,btXSize,btYSize);
    TimeRW->setOnClicCallback(TimeControlBtOnClicCallback);
    TimePause = new Labeled_Button("\4");
    TimePause->reshape(btXSize*2+2,0,btXSize,btYSize);
    TimePause->setOnClicCallback(TimeControlBtOnClicCallback);
    TimeReal = new Labeled_Button("\5");
    TimeReal->reshape(btXSize*3+3,0,btXSize,btYSize);
    TimeReal->setOnClicCallback(TimeControlBtOnClicCallback);
    TimeF = new Labeled_Button("\3");
    TimeF->reshape(btXSize*4+4,0,btXSize,btYSize);
    TimeF->setOnClicCallback(TimeControlBtOnClicCallback);
    TimeFF = new Labeled_Button("\3\3");
    TimeFF->reshape(btXSize*5+5,0,btXSize,btYSize);
    TimeFF->setOnClicCallback(TimeControlBtOnClicCallback);
    TimeNow = new Labeled_Button("N");
    TimeNow->reshape(btXSize*6+6,0,btXSize,btYSize);
    TimeNow->setOnClicCallback(TimeControlBtOnClicCallback);

    TimeControlWin->reshape(vec2_i(gc->winW-1-TimeControlWin->getSize()[0], gc->winH-TimeControlWin->getSize()[1]),TimeControlWin->getSize());

    TimeControlContainer->addComponent(TimeFRW);
    TimeControlContainer->addComponent(TimeRW);
    TimeControlContainer->addComponent(TimePause);
    TimeControlContainer->addComponent(TimeNow);
    TimeControlContainer->addComponent(TimeReal);
    TimeControlContainer->addComponent(TimeF);
    TimeControlContainer->addComponent(TimeFF);

    TimeControlWin->addComponent(TimeControlContainer);


/**********************************************************************************/
/*** Base container ***/
    Base = new Container();
    Base->reshape(0,0,global.X_Resolution,global.Y_Resolution);
    Base->addComponent(ContainerBtFlags);
    Base->addComponent(btLegend);
    Base->addComponent(InfoSelectLabel);
    Base->addComponent(TopWindowsInfos);
    Base->addComponent(InfoWin);
    Base->addComponent(HelpWin);
    Base->addComponent(ConfigWin);
    Base->addComponent(TimeControlWin);
}


/**********************************************************************************/
/*                                    CLEAR UI                                    */
/**********************************************************************************/
void clearUi(void)
{
    if (Base) delete Base;
    Base = NULL;
    if (gc) delete gc;
    gc = NULL;
    if (spaceFont) delete spaceFont;
    spaceFont = NULL;
}


/*******************************************************************************/
void updateStandardWidgets(void)
{   // Update the date and time
    char str[30];
	ln_date d;

    if (global.FlagUTC_Time)
    {   
		get_date(navigation.get_JDay(),&d);
    }
    else
    {
		get_date(navigation.get_JDay()+navigation.get_time_zone()*JD_HOUR,&d);
    }

    sprintf(str,"%.2d/%.2d/%.4d",d.days,d.months,d.years);
    DateLabel->setLabel(str);
    if (global.FlagUTC_Time)
    {
		sprintf(str,"%.2d:%.2d:%.2d (UTC)",d.hours,d.minutes,(int)d.seconds);
    }
    else
    {   sprintf(str,"%.2d:%.2d:%.2d",d.hours,d.minutes,(int)d.seconds);
    }
    HourLabel->setLabel(str);
    sprintf(str,"FPS : %4.2f",global.Fps);
    FPSLabel->setLabel(str);
    sprintf(str,"fov=%.3f\6", navigation.get_fov());
    FOVLabel->setLabel(str);
}


/**********************************************************************/
// Update the infos about the selected object in the TextLabel widget
void updateInfoSelectString(void)
{
	char objectInfo[300];
    objectInfo[0]=0;
	selected_object->get_info_string(objectInfo);
    InfoSelectLabel->setLabel(objectInfo);
    InfoSelectLabel->setVisible(true);
	if (selected_object->get_type()==STEL_OBJECT_NEBULA) InfoSelectLabel->setColour(Vec3f(0.4f,0.5f,0.8f));
	if (selected_object->get_type()==STEL_OBJECT_PLANET) InfoSelectLabel->setColour(Vec3f(1.0f,0.3f,0.3f));
	if (selected_object->get_type()==STEL_OBJECT_STAR)   InfoSelectLabel->setColour(selected_object->get_RGB());
}


/*******************************************************************/
void renderUi()
{
    setOrthographicProjection(global.X_Resolution, global.Y_Resolution);    // 2D coordinate
    glPushMatrix();
    glLoadIdentity();

    updateStandardWidgets();

    glEnable(GL_SCISSOR_TEST);
    vec2_i sz = Base->getSize();
	gc->scissorPos[0]=0;
	gc->scissorPos[1]=0;
    glScissor(gc->scissorPos[0], gc->winH - gc->scissorPos[1] - sz[1], sz[0], sz[1]);
    
    Base->render(*gc);
    glDisable(GL_SCISSOR_TEST);
    glPopMatrix();
    resetPerspectiveProjection();             // Restore the other coordinate
}

/*******************************************************************************/
void GuiHandleMove(Uint16 x, Uint16 y)
{   
	Base->handleMouseMove((int)x, (int)y);
}

/*******************************************************************************/
void GuiHandleClic(Uint16 x, Uint16 y, Uint8 state, Uint8 button)
{   // Convert the name from GLU to my GUI
    enum guiValue bt;
    enum guiValue st;
    switch (button)
    {   case SDL_BUTTON_RIGHT : bt=GUI_MOUSE_RIGHT; break;
        case SDL_BUTTON_LEFT : bt=GUI_MOUSE_LEFT; break;
        case SDL_BUTTON_MIDDLE : bt=GUI_MOUSE_MIDDLE; break;
        default : bt=GUI_MOUSE_LEFT;
    }
    if (state==SDL_RELEASED) st=GUI_UP; else st=GUI_DOWN;

    // Send the mouse event to the User Interface
    if (Base->handleMouseClic((int)x, (int)y, st, bt))
    {   // If a "unthru" widget was at the clic position 
        return; 
    }
    else
    // Manage the event for the main window
    {   if (state==SDL_RELEASED) return;
        // Deselect the selected object
        if (button==SDL_BUTTON_RIGHT)
        {
			selected_object=NULL;
            InfoSelectLabel->setVisible(false);
            return;
        }
        if (button==SDL_BUTTON_MIDDLE)
        {
			if (selected_object)
            {
				navigation.move_to(selected_object->get_earth_equ_pos());
            }
        }
        if (button==SDL_BUTTON_LEFT)
        {   
        	// CTRL + left clic = right clic for 1 button mouse
			if (SDL_GetModState() & KMOD_CTRL)
			{
				selected_object=NULL;
            	InfoSelectLabel->setVisible(false);
            	return;
        	}
        	// Left or middle clic -> selection of an object
            selected_object=stel_object::find_stel_object((int)x,(int)y);
            // If an object has been found
            if (selected_object)
            {
				updateInfoSelectString();
				navigation.set_flag_traking(0);
            }
        }
    }
}

/*******************************************************************************/
bool GuiHandleKeys(SDLKey key, int state)
{
	if (state==GUI_DOWN)
    {   
    	if(key==SDLK_ESCAPE)
    	{ 	
    		clearUi();
            exit(0); 
		}
        if(key==SDLK_c)
        {	
        	global.FlagConstellationDrawing=!global.FlagConstellationDrawing;
		}
        if(key==SDLK_p)
        {	
        	global.FlagPlanetsHintDrawing=!global.FlagPlanetsHintDrawing;
		}
        if(key==SDLK_v)
        {	
        	global.FlagConstellationName=!global.FlagConstellationName;
            BtConstellationsName->setActive(global.FlagConstellationName);
		}
        if(key==SDLK_z)
        {	
        	global.FlagAzimutalGrid=!global.FlagAzimutalGrid;
            BtAzimutalGrid->setActive(global.FlagAzimutalGrid);
		}
        if(key==SDLK_e)
        {	
        	global.FlagEquatorialGrid=!global.FlagEquatorialGrid;
            BtEquatorialGrid->setActive(global.FlagEquatorialGrid);
		}
        if(key==SDLK_n)
        {	
        	global.FlagNebulaName=!global.FlagNebulaName;
            BtNebula->setActive(global.FlagNebulaName);
		}
        if(key==SDLK_g)
        {	
        	global.FlagGround=!global.FlagGround;
            BtGround->setActive(global.FlagGround);
		}
        if(key==SDLK_f)
        {	
        	global.FlagFog=!global.FlagFog;
		}
        if(key==SDLK_1)
        {	
        	global.FlagRealTime=!global.FlagRealTime;
		}
        if(key==SDLK_2)
        {	
        	global.FlagAcceleredTime=!global.FlagAcceleredTime;
		}
        if(key==SDLK_3)
        {	
        	global.FlagVeryFastTime=!global.FlagVeryFastTime;
		}
        if(key==SDLK_q)
        {	
        	global.FlagCardinalPoints=!global.FlagCardinalPoints;
            BtCardinalPoints->setActive(global.FlagCardinalPoints);
		}
        if(key==SDLK_a)
        {	
        	global.FlagAtmosphere=!global.FlagAtmosphere;
            BtAtmosphere->setActive(global.FlagAtmosphere);
		}
        if(key==SDLK_r)
        {	
        	global.FlagRealMode=!global.FlagRealMode;
            TopWindowsInfos->setVisible(!global.FlagRealMode);
            ContainerBtFlags->setVisible(!global.FlagRealMode);
		}
        if(key==SDLK_h)
        {	
        	global.FlagHelp=!global.FlagHelp;
            BtHelp->setActive(global.FlagHelp);
            HelpWin->setVisible(global.FlagHelp);
            if (global.FlagHelp && global.FlagInfos) global.FlagInfos=false;
		}
        if(key==SDLK_4)
        {	
        	global.FlagEcliptic=!global.FlagEcliptic;
		}
        if(key==SDLK_5)
        {	
        	global.FlagEquator=!global.FlagEquator;
		}
        if(key==SDLK_t)
        {
			navigation.set_flag_lock_equ_pos(!navigation.get_flag_lock_equ_pos());
            BtFollowEarth->setActive(navigation.get_flag_lock_equ_pos());
		}
        if(key==SDLK_s)
        {	
        	global.FlagStars=!global.FlagStars;
		}
        if(key==SDLK_SPACE)
        {	
        	if (selected_object)
			{
				navigation.move_to(selected_object->get_earth_equ_pos());
				navigation.set_flag_traking(1);
			}
		}
        if(key==SDLK_i)
        {	
        	global.FlagInfos=!global.FlagInfos; 
            InfoWin->setVisible(global.FlagInfos);
		}
    }
    return false;
}
