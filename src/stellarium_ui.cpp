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

#include "stellarium_ui.h"
#include "s_font.h"
#include "s_gui.h"
#include "s_texture.h"
#include "s_utility.h"
#include "s_gui_window.h"
#include "navigation.h"
#include "nebula_mgr.h"
#include "star_mgr.h"
#include "stellarium.h"
#include "parsecfg.h"

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
//Textured_Button * BtFog = NULL;
/*Textured_Button * BtRealTime = NULL;
Textured_Button * BtAcceleredTime = NULL;
Textured_Button * BtVeryFastTime = NULL;*/
Textured_Button * BtCardinalPoints = NULL;
Textured_Button * BtAtmosphere = NULL;
Textured_Button * BtNebula = NULL;
Textured_Button * BtHelp = NULL;
//Textured_Button * BtRealMode = NULL;
Textured_Button * BtFollowEarth = NULL;
Textured_Button * BtConfig = NULL;

FilledTextLabel * InfoSelectLabel = NULL;      // The TextLabel displaying teh infos about the selected object

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

StdBtWin * TimeControlWin = NULL;              // The window containing the configuration options
Container * TimeControlContainer = NULL;
Labeled_Button * TimeRW = NULL;
Labeled_Button * TimeFRW = NULL;
Labeled_Button * TimeF = NULL;
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
        case 14 :   global.FlagRealMode=!global.FlagRealMode;
                    bt->setActive(global.FlagRealMode);
                    Base->setVisible(!global.FlagRealMode);
                    break;
        case 15 :   global.FlagFollowEarth=!global.FlagFollowEarth;
                    bt->setActive(global.FlagFollowEarth);
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
    {   case 1 : btLegend->setLabel("Drawing of the Constellations"); return;
        case 2 : btLegend->setLabel("Names of the Constellations"); return;
        case 3 : btLegend->setLabel("Azimutal Grid"); return;
        case 4 : btLegend->setLabel("Equatorial Grid"); return;
        case 5 : btLegend->setLabel("Ground"); return;
        case 6 : btLegend->setLabel("Fog"); return;
        case 7 : btLegend->setLabel("Real Time Mode"); return;
        case 8 : btLegend->setLabel("Accelered Time Mode"); return;
        case 9 : btLegend->setLabel("Very Fast Time Mode"); return;
        case 10 :btLegend->setLabel("Cardinal Points"); return;
        case 11 :btLegend->setLabel("Atmosphere (+ and - to control the brightness)"); return;
        case 12 :btLegend->setLabel("Nebulas"); return;
        case 13 :btLegend->setLabel("Help"); return;
        case 14 :btLegend->setLabel("Real Mode"); return;
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
            ToggleStarName->setLabel("Show");
        global.FlagStarName=false;
    }
    else
    {   if (ToggleStarName) ToggleStarName->setLabel("Hide");
        global.FlagStarName=true;
    }
}

/*******************************************************************************/

void ToggleGroundOnClicCallback(guiValue button,Component *)
{   
    global.FlagGround=!global.FlagGround;
}

void ToggleFogOnClicCallback(guiValue button,Component *)
{   
    global.FlagFog=!global.FlagFog;
}

void ToggleAtmosphereOnClicCallback(guiValue button,Component *)
{   global.FlagAtmosphere=!global.FlagAtmosphere;
}

void ToggleMilkyWayOnClicCallback(guiValue button,Component *)
{   global.FlagMilkyWay=!global.FlagMilkyWay;
}

/*******************************************************************************/

void TimeControlBtOnClicCallback(guiValue button,Component * caller)
{   global.FlagVeryFastTime = false;
    global.FlagAcceleredTime = false;
    global.FlagRealTime = false;
    if(caller==TimeFRW)
    {   global.TimeDirection=-1;
        global.FlagVeryFastTime=true;
    }
    if(caller==TimeRW)
    {   global.TimeDirection=-1;
        global.FlagAcceleredTime=true;
    }
    if(caller==TimePause)
    {   global.TimeDirection= 1;
    }
    if(caller==TimeReal)
    {   global.TimeDirection= 1;
        global.FlagRealTime=true;
    }
    if(caller==TimeF)
    {   global.TimeDirection= 1;
        global.FlagAcceleredTime=true;
    }
    if(caller==TimeFF)
    {   global.TimeDirection= 1;
        global.FlagVeryFastTime=true;
    }
}

/**********************************************************************************/

void LatitudeBarOnChangeValue(float value,Component *)
{   
    value=90.-value;
    global.ThePlace.setLatitude(value);
    char tempValueStr[30];
    if (global.ThePlace.latitude()-PI/2<0)
        sprintf(tempValueStr,"\1 Latitude : %.2f N",-(global.ThePlace.latitude()*180./PI-90));
    else
        sprintf(tempValueStr,"\1 Latitude : %.2f S",global.ThePlace.latitude()*180./PI-90);
    LatitudeLabel->setLabel(tempValueStr);
}
void LongitudeBarOnChangeValue(float value,Component *)
{   
    char tempValueStr[30];
    if (value < 0)
        sprintf(tempValueStr,"\1 Longitude : %.2f W",-value);
    else 
        sprintf(tempValueStr,"\1 Longitude : %.2f E",value);
    LongitudeLabel->setLabel(tempValueStr);
    value=-value;
    if (value<0) value=360.+value;
    global.ThePlace.setLongitude(value);
}
void AltitudeBarOnChangeValue(float value,Component *)
{   global.Altitude=value;
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Altitude : %dm",global.Altitude);
    AltitudeLabel->setLabel(tempValueStr);
}
void TimeZoneBarOnChangeValue(float value,Component *)
{   global.TimeZone=value;
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 TimeZone : %d h",global.TimeZone);
    TimeZoneLabel->setLabel(tempValueStr);
}

void SaveLocationOnClicCallback(guiValue button,Component *)
{   
    double tempLatitude = LatitudeBar->getValue();
    double tempLongitude = LongitudeBar->getValue();
    cfgStruct cfgini2[] = 
    {// parameter               type        address of variable
        {"LATITUDE",            CFG_DOUBLE, &tempLatitude},
        {"LONGITUDE",           CFG_DOUBLE, &tempLongitude},
        {"ALTITUDE",            CFG_INT,    &global.Altitude},
        {"TIME_ZONE",           CFG_INT,    &global.TimeZone},
        {NULL, CFG_END, NULL}   /* no more parameters */
    };

    cfgDump("config/location.txt", cfgini2, CFG_SIMPLE, 0);
}

/**********************************************************************************/
/*                                   INIT UI                                      */
/**********************************************************************************/
void initUi(void)
{   
    /*** fonts ***/
    spaceFont = new s_font(0.013*global.X_Resolution, "spacefont", DATA_DIR "/spacefont.txt");
    if (!spaceFont)
    {
        printf("ERROR WHILE CREATING FONT\n");
        exit(1);
    }
    
    /*** Colors ***/
    vec3_t clBlue(0.3,0.3,1.0);
    vec3_t clRed(1.0,0.3,0.3);
    vec3_t clGreen(0.3,1.0,0.3);
    vec3_t clWhite(1.,1.,1.);
    
    /*** Graphic Context ***/
    gc = new GraphicsContext(global.X_Resolution,global.Y_Resolution);
    if (!gc)
    {
        printf("ERROR WHILE CREATING GRAPHIC CONTEXT\n");
        exit(1);
    }
    gc->setFont(spaceFont);
    gc->backGroundTexture=new s_texture("backmenu",TEX_LOAD_TYPE_PNG_ALPHA);
    gc->headerTexture=new s_texture("headermenu",TEX_LOAD_TYPE_PNG_ALPHA);

    /*** temporary variables ***/
    int avgCharLen=gc->getFont()->getAverageCharLen();
    int lineHeight=gc->getFont()->getLineHeight();

/**********************************************************************************/
/*** Flag Buttons list ***/
    int btSize=4*avgCharLen;
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
    //BtFog = new Textured_Button(new s_texture("Bouton7"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,6,global.FlagFog);
    /*BtRealTime = new Textured_Button(new s_texture("Bouton5"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,7,global.FlagRealTime);
    BtAcceleredTime = new Textured_Button(new s_texture("Bouton6"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,8,global.FlagAcceleredTime);
    BtVeryFastTime = new Textured_Button(new s_texture("Bouton6"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,9,global.FlagVeryFastTime);
  */BtCardinalPoints = new Textured_Button(new s_texture("Bouton8"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,10,global.FlagCardinalPoints);
    BtAtmosphere = new Textured_Button(new s_texture("Bouton9"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,11,global.FlagAtmosphere);
    //BtRealMode = new Textured_Button(new s_texture("Bouton14"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,14,global.FlagRealMode);
    BtHelp = new Textured_Button(new s_texture("Bouton11"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,13,global.FlagHelp);
    BtFollowEarth = new Textured_Button(new s_texture("Bouton13"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,15,global.FlagFollowEarth);
    BtConfig = new Textured_Button(new s_texture("Bouton16"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,16,global.FlagConfig);

    /*** Button container ***/
    ContainerBtFlags = new FilledContainer();
    ContainerBtFlags->reshape(vec2_i(0,global.Y_Resolution-btSize),   vec2_i(btSize*11+1,btSize+1));
    ContainerBtFlags->addComponent(BtConstellationsDraw);
    ContainerBtFlags->addComponent(BtConstellationsName);
    ContainerBtFlags->addComponent(BtAzimutalGrid);
    ContainerBtFlags->addComponent(BtEquatorialGrid);
    ContainerBtFlags->addComponent(BtGround);
//    ContainerBtFlags->addComponent(BtFog);
    /*ContainerBtFlags->addComponent(BtRealTime);
    ContainerBtFlags->addComponent(BtAcceleredTime);
    ContainerBtFlags->addComponent(BtVeryFastTime);*/
    ContainerBtFlags->addComponent(BtCardinalPoints);
    ContainerBtFlags->addComponent(BtAtmosphere);
    ContainerBtFlags->addComponent(BtNebula);
    ContainerBtFlags->addComponent(BtHelp);
    //ContainerBtFlags->addComponent(BtRealMode);
    ContainerBtFlags->addComponent(BtFollowEarth);
    ContainerBtFlags->addComponent(BtConfig);

/**********************************************************************************/
/*** button legend label ***/
    btLegend = new Label("ERROR, this shouldn't be seen...");
    btLegend->reshape(vec2_i(3,global.Y_Resolution-btSize-lineHeight-5),vec2_i(100,17));
    btLegend->setVisible(false);

/**********************************************************************************/
/*** help TextLabel ***/
    HelpTextLabel = new TextLabel(
        "4 Directions : Deplacement RA/DE\n\
Page Up/Down : Zoom\n\
Left Mouse Clic : Select Star\n\
Right Mouse Clic  : Clear Pointer\n\
SPACE or Middle Mouse Clic:\n\
     Center On Selected Object\n\
C   : Drawing of the Constellations\n\
V   : Names of the Constellations\n\
E   : Equatorial Grid\n\
A   : Azimutal Grid\n\
N   : Nebulas\n\
P   : Planet Finder\n\
G   : Ground\n\
F   : Fog\n\
1   : Real Time Mode\n\
2   : Accelered Time Mode\n\
3   : Very Fast Time Mode\n\
Q   : Cardinal Points\n\
\1   : Atmosphere\n\
H   : Help\n\
4   : Ecliptic\n\
5   : Equator\n\
T   : Object Tracking\n\
S   : Stars\n\
I   : About Stellarium\n\
ESC : Quit\n\
F1/F2 : fullscreen window/small window\n\
(in windowed mode only)\n"
    ,gc->getFont());
    HelpTextLabel->reshape(vec2_i(avgCharLen*2,lineHeight),vec2_i(45*avgCharLen,35*lineHeight));

    HelpWin = new StdBtWin(global.X_Resolution/2-25*avgCharLen, global.Y_Resolution/2-30*lineHeight/2, 48*avgCharLen, 35*(lineHeight+1), "Help", Base);
    HelpWin->addComponent(HelpTextLabel);
    HelpWin->setVisible(global.FlagHelp);
    HelpWin->setHideCallback(HelpWinHideCallback);

/**********************************************************************************/
/*** Info TextLabel ***/
    InfoTextLabel = new TextLabel(
"                 \1   " APP_NAME "   May  2002   \1\n\
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
    InfoTextLabel->reshape(vec2_i(avgCharLen*3,lineHeight),vec2_i(62*avgCharLen,35*lineHeight));
    
    InfoWin = new StdBtWin(global.X_Resolution/2-25*avgCharLen, global.Y_Resolution/2-30*lineHeight/2, 67*avgCharLen, 25*(lineHeight+1), "About Stellarium", Base);
    InfoWin->addComponent(InfoTextLabel);
    InfoWin->setVisible(global.FlagInfos);
    InfoWin->setHideCallback(InfoWinHideCallback);

/**********************************************************************************/
/*** InfoSelect TextLabel ***/
    InfoSelectLabel = new FilledTextLabel("Info",gc->getFont());
    InfoSelectLabel->reshape(vec2_i(3,lineHeight+2),vec2_i(avgCharLen*50,7*lineHeight));
    InfoSelectLabel->setVisible(false);

/**********************************************************************************/
/*** TopWindowsInfos Container
    /*** Date Label ***/
    DateLabel = new Label("-");
    DateLabel->reshape(vec2_i(3,2), vec2_i(10*avgCharLen,lineHeight));
    /*** Hour Label ***/
    HourLabel = new Label("-");
    HourLabel->reshape(vec2_i(12*avgCharLen+15,2), vec2_i(6*avgCharLen,lineHeight));
    /*** FPS Label ***/
    FPSLabel = new Label("-");
    FPSLabel->reshape(vec2_i(global.X_Resolution-13*avgCharLen,2), vec2_i(10*avgCharLen,lineHeight));
    if (!global.FlagFps) FPSLabel->setVisible(false);
    /*** FOV Label ***/
    FOVLabel = new Label("-");
    FOVLabel->reshape(vec2_i(global.X_Resolution-26*avgCharLen,2), vec2_i(10*avgCharLen,lineHeight));
    /*** AppName Label ***/
    AppNameLabel = new Label(APP_NAME);
    AppNameLabel->reshape(vec2_i(global.X_Resolution/2-gc->getFont()->getStrLen(APP_NAME)/2,2), vec2_i(gc->getFont()->getStrLen(APP_NAME),lineHeight));
    /*** TopWindowsInfos Container ***/    
    TopWindowsInfos = new FilledContainer();
    TopWindowsInfos->reshape(vec2_i(0,0), vec2_i(global.X_Resolution,lineHeight+2));
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
    StarConfigContainer->reshape(vec2_i(avgCharLen,3),   vec2_i(30*avgCharLen, 11*(lineHeight+1)));
    
    int yt=3;
    
    StarLabel = new Label("STARS :");
    StarLabel->reshape(vec2_i(3,yt), vec2_i(10*avgCharLen,lineHeight));
    yt+=1.6*lineHeight;
    
    StarNameLabel = new Label("\1 Names :");
    StarNameLabel->reshape(vec2_i(15,yt), vec2_i(8*avgCharLen,lineHeight));
    if (global.FlagStarName) ToggleStarName = new Labeled_Button("Hide");
    else ToggleStarName = new Labeled_Button("Show");
    ToggleStarName->reshape(vec2_i(13*avgCharLen,yt-3),vec2_i(avgCharLen*8,lineHeight+4));
    ToggleStarName->setOnClicCallback(ToggleStarNameOnClicCallback);
    
    yt+=1.6*lineHeight;

    StarNameMagLabel = new Label("Show if Magnitude < --");
    StarNameMagLabel->reshape(vec2_i(15,yt), vec2_i(6*avgCharLen,lineHeight));
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Show if Magnitude < %.1f",global.MaxMagStarName);
    StarNameMagLabel->setLabel(tempValueStr);
    yt+=1.3*lineHeight;
    ChangeStarDrawNameBar = new CursorBar(vec2_i(2*avgCharLen,yt), vec2_i(25*avgCharLen,10), -1.5,7.,global.MaxMagStarName,ChangeStarDrawNameBarOnChangeValue);
    
    yt+=1.6*lineHeight;

    StarTwinkleLabel = new Label("\1 Twinkle Amount : --");
    char tempValue2Str[30];
    sprintf(tempValue2Str,"\1 Twinkle Amount : %.1f",global.StarTwinkleAmount);
    StarTwinkleLabel->setLabel(tempValue2Str);
    StarTwinkleLabel->reshape(vec2_i(15,yt), vec2_i(18*avgCharLen,lineHeight));
    yt+=1.3*lineHeight;
    ChangeStarTwinkleBar = new CursorBar(vec2_i(2*avgCharLen,yt), vec2_i(25*avgCharLen,10),0.,10.,global.StarTwinkleAmount,ChangeStarTwinkleBarOnChangeValue);

    yt+=1.6*lineHeight;

    StarScaleLabel = new Label("\1 Scale : --");
    char tempValue3Str[30];
    sprintf(tempValue3Str,"\1 Scale : %.1f",global.StarScale);
    StarScaleLabel->setLabel(tempValue3Str);
    StarScaleLabel->reshape(vec2_i(15,yt), vec2_i(18*avgCharLen,lineHeight));
    yt+=1.3*lineHeight;
    ChangeStarScaleBar = new CursorBar(vec2_i(2*avgCharLen,yt), vec2_i(25*avgCharLen,10),0.,10.,global.StarScale,ChangeStarScaleBarOnChangeValue);

    StarConfigContainer->addComponent(ChangeStarDrawNameBar);
    StarConfigContainer->addComponent(ToggleStarName);
    StarConfigContainer->addComponent(StarLabel);
    StarConfigContainer->addComponent(StarNameLabel);
    StarConfigContainer->addComponent(StarNameMagLabel);
    StarConfigContainer->addComponent(StarTwinkleLabel);
    StarConfigContainer->addComponent(ChangeStarTwinkleBar);
    StarConfigContainer->addComponent(StarScaleLabel);
    StarConfigContainer->addComponent(ChangeStarScaleBar);

    yt+=2.8*lineHeight;

    /*** Landcape config container ***/
    LandscapeConfigContainer = new FilledContainer();
    if (!LandscapeConfigContainer)
    {
        printf("ERROR WHILE CREATING UI CONTAINER\n");
        exit(1);
    }
    LandscapeConfigContainer->reshape(vec2_i(avgCharLen,yt), vec2_i(30*avgCharLen, 8*(lineHeight+1)));

    yt=3;

    LandscapeLabel = new Label("LANDSCAPE :");
    LandscapeLabel->reshape(vec2_i(3,yt), vec2_i(10*avgCharLen,lineHeight));

    yt+=1.8*lineHeight;
    
    ToggleGround = new Labeled_Button("Ground");
    ToggleGround->reshape(vec2_i(4*avgCharLen,yt-3),vec2_i(avgCharLen*22,lineHeight+4));
    ToggleGround->setOnClicCallback(ToggleGroundOnClicCallback);

    yt+=1.6*lineHeight;   

    ToggleFog = new Labeled_Button("Fog");
    ToggleFog->reshape(vec2_i(4*avgCharLen,yt-3),vec2_i(avgCharLen*22,lineHeight+4));
    ToggleFog->setOnClicCallback(ToggleFogOnClicCallback);

    yt+=1.6*lineHeight;   

    ToggleAtmosphere = new Labeled_Button("Atmosphere");
    ToggleAtmosphere->reshape(vec2_i(4*avgCharLen,yt-3),vec2_i(avgCharLen*22,lineHeight+4));
    ToggleAtmosphere->setOnClicCallback(ToggleAtmosphereOnClicCallback);

    yt+=1.6*lineHeight;   

    ToggleMilkyWay = new Labeled_Button("MilkyWay");
    ToggleMilkyWay->reshape(vec2_i(4*avgCharLen,yt-3),vec2_i(avgCharLen*22,lineHeight+4));
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
    LocationConfigContainer->reshape(vec2_i(avgCharLen*32,3), vec2_i(30*avgCharLen, 14*(lineHeight+1)));

    yt=3;

    LocationLabel = new Label("L0CATION :");
    LocationLabel->reshape(vec2_i(3,yt), vec2_i(10*avgCharLen,lineHeight));
    yt+=1.6*lineHeight; 

    LatitudeLabel = new Label("Latitude : ");
    LatitudeLabel->reshape(vec2_i(15,yt), vec2_i(12*avgCharLen,lineHeight));
    if (global.ThePlace.latitude()-PI/2<0)
        sprintf(tempValueStr,"\1 Latitude : %.2f N",-(global.ThePlace.latitude()*180./PI-90));
    else
        sprintf(tempValueStr,"\1 Latitude : %.2f S",global.ThePlace.latitude()*180./PI-90);
    LatitudeLabel->setLabel(tempValueStr);
    yt+=1.3*lineHeight;
    LatitudeBar = new CursorBar(vec2_i(2*avgCharLen,yt), vec2_i(25*avgCharLen,10),-90.,90.,-(global.ThePlace.latitude()*180/PI-90),LatitudeBarOnChangeValue);
    
    yt+=1.6*lineHeight; 
    
    LongitudeLabel = new Label("Longitude : ");
    LongitudeLabel->reshape(vec2_i(15,yt), vec2_i(12*avgCharLen,lineHeight));
    float temp = (2*PI-global.ThePlace.longitude())*180/PI;
    if (temp < 0)
        sprintf(tempValueStr,"\1 Longitude : %.2f W",-temp);
    else 
        sprintf(tempValueStr,"\1 Longitude : %.2f E",temp);
    LongitudeLabel->setLabel(tempValueStr);
    yt+=1.3*lineHeight;
    LongitudeBar = new CursorBar(vec2_i(2*avgCharLen,yt), vec2_i(25*avgCharLen,10),-180,180,(2*PI-global.ThePlace.longitude())*180/PI,LongitudeBarOnChangeValue);
    
    yt+=1.6*lineHeight; 

    AltitudeLabel = new Label("Altitude : ");
    AltitudeLabel->reshape(vec2_i(15,yt), vec2_i(12*avgCharLen,lineHeight));
    sprintf(tempValueStr,"\1 Altitude : %dm",global.Altitude);
    AltitudeLabel->setLabel(tempValueStr);
    yt+=1.3*lineHeight;
    AltitudeBar = new CursorBar(vec2_i(2*avgCharLen,yt), vec2_i(25*avgCharLen,10),-500.,10000.,global.Altitude,AltitudeBarOnChangeValue);
    
    yt+=1.6*lineHeight; 

    TimeZoneLabel = new Label("TimeZone : ");
    TimeZoneLabel->reshape(vec2_i(15,yt), vec2_i(12*avgCharLen,lineHeight));
    sprintf(tempValueStr,"\1 TimeZone : %d",global.TimeZone);
    TimeZoneLabel->setLabel(tempValueStr);
    yt+=1.3*lineHeight;
    TimeZoneBar = new CursorBar(vec2_i(2*avgCharLen,yt), vec2_i(25*avgCharLen,10),-12.,12.,global.TimeZone,TimeZoneBarOnChangeValue);
    
    yt+=1.6*lineHeight; 

    SaveLocation = new Labeled_Button("Save location");
    SaveLocation->reshape(vec2_i(4*avgCharLen,yt-3),vec2_i(avgCharLen*22,lineHeight+4));
    SaveLocation->setOnClicCallback(SaveLocationOnClicCallback);

    yt+=1.6*lineHeight; 

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

    /*** Config window ***/
    ConfigWin = new StdBtWin(40, 40, 63*avgCharLen, 22*(lineHeight+1), "Configuration", Base);
    ConfigWin->addComponent(StarConfigContainer);
    ConfigWin->addComponent(LandscapeConfigContainer);
    ConfigWin->addComponent(LocationConfigContainer);
    ConfigWin->setHideCallback(ConfigWinHideCallback);
    ConfigWin->setVisible(global.FlagConfig);

/**********************************************************************************/
/*** Time control win ***/
    TimeControlWin = new StdBtWin(gc->winW-120, gc->winH-lineHeight, 0, 0, "Time Control", Base);
    TimeControlWin->setInSize(vec2_i(120,lineHeight+2),*gc);
    TimeControlWin->setHideCallback(TimeControlWinHideCallback);

    TimeControlContainer = new Container();
    int btXSize = 19;
    int btYSize = lineHeight;
    TimeControlContainer->reshape(vec2_i(1,1),vec2_i(120,btYSize+2));
    TimeFRW = new Labeled_Button("\2\2");
    TimeFRW->reshape(vec2_i(0,0),vec2_i(btXSize,btYSize));
    TimeFRW->setOnClicCallback(TimeControlBtOnClicCallback);
    TimeRW = new Labeled_Button("\2");
    TimeRW->reshape(vec2_i(btXSize+1,0),vec2_i(btXSize,btYSize));
    TimeRW->setOnClicCallback(TimeControlBtOnClicCallback);
    TimePause = new Labeled_Button("\4");
    TimePause->reshape(vec2_i(btXSize*2+2,0),vec2_i(btXSize,btYSize));
    TimePause->setOnClicCallback(TimeControlBtOnClicCallback);
    TimeReal = new Labeled_Button("\5");
    TimeReal->reshape(vec2_i(btXSize*3+3,0),vec2_i(btXSize,btYSize));
    TimeReal->setOnClicCallback(TimeControlBtOnClicCallback);
    TimeF = new Labeled_Button("\3");
    TimeF->reshape(vec2_i(btXSize*4+4,0),vec2_i(btXSize,btYSize));
    TimeF->setOnClicCallback(TimeControlBtOnClicCallback);
    TimeFF = new Labeled_Button("\3\3");
    TimeFF->reshape(vec2_i(btXSize*5+5,0),vec2_i(btXSize,btYSize));
    TimeFF->setOnClicCallback(TimeControlBtOnClicCallback);

    TimeControlWin->reshape(vec2_i(gc->winW-1, gc->winH)-TimeControlWin->getSize(),TimeControlWin->getSize());

    TimeControlContainer->addComponent(TimeFRW);
    TimeControlContainer->addComponent(TimeRW);
    TimeControlContainer->addComponent(TimePause);
    TimeControlContainer->addComponent(TimeReal);
    TimeControlContainer->addComponent(TimeF);
    TimeControlContainer->addComponent(TimeFF);

    TimeControlWin->addComponent(TimeControlContainer);


/**********************************************************************************/
/*** Base container ***/
    Base = new Container();
    Base->reshape(vec2_i(0,0),vec2_i(global.X_Resolution,global.Y_Resolution));
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
/*                                    CLEAR UI                                    /*
/**********************************************************************************/
void clearUi(void)
{   if (gc) delete gc;
    gc = NULL;
    if (Base) delete Base;
    Base = NULL;
    if (spaceFont) delete spaceFont;
    spaceFont = NULL;
}


/*******************************************************************************/
void updateStandardWidgets(void)
{   // Update the date and time
    char str[30];
    int jour,mois;
    long annee;

    float reste;
    if (global.FlagUTC_Time)
    {   DateOps::dayToDmy((float)global.JDay,jour,mois,annee);
        reste=global.JDay-DateOps::dmyToDay(jour,mois,annee);
    }
    else
    {   DateOps::dayToDmy((float)global.JDay+(float)global.TimeZone*HEURE,jour,mois,annee);
        reste=global.JDay+(float)global.TimeZone*HEURE-DateOps::dmyToDay(jour,mois,annee);
    }
    double heure=reste*24;
    double minute=(double)(heure-(int)heure)*60;
    double seconde=(minute-(int)minute)*60;

    sprintf(str,"%.2d/%.2d/%.4d",jour,mois,(int)annee);
    DateLabel->setLabel(str);
    if (global.FlagUTC_Time)
    {   sprintf(str,"%.2d:%.2d:%.2d (UTC)",(int)heure,(int)minute,(int)seconde);
    }
    else
    {   sprintf(str,"%.2d:%.2d:%.2d",(int)heure,(int)minute,(int)seconde);
    }
    HourLabel->setLabel(str);
    sprintf(str,"FPS : %4.2f",global.Fps);
    FPSLabel->setLabel(str);
    sprintf(str,"fov=%.3f", global.Fov);
    FOVLabel->setLabel(str);
}


/*****************************************************************************/
// find and select the "nearest" object and retrieve his informations
void findObject(int x, int y)
{   // try to select an object
    GLdouble M[16]; 
    GLdouble P[16];
    GLdouble objx[1];
    GLdouble objy[1];
    GLdouble objz[1];
    GLint V[4];
    Switch_to_equatorial();
    // Convert x,y screen pos in 3D vector
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);
    vec3_t tempPointer;
    if (gluUnProject(x,global.Y_Resolution-y,1,M,P,V,objx,objy,objz))
    {   tempPointer[0]=*objx;
        tempPointer[1]=*objy;
        tempPointer[2]=*objz;
        if (!SolarSystem->Rechercher(tempPointer) && global.FlagPlanets)
        {   global.FlagSelect=true;
            SolarSystem->InfoSelect(global.SelectedObject.XYZ, global.SelectedObject.RA,global.SelectedObject.DE, global.SelectedObject.Name, global.SelectedObject.Distance,global.SelectedObject.Size);
            RA_en_hms(global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,global.SelectedObject.RA);
            global.SelectedObject.type=2;   //planet type
            global.SelectedObject.RGB[0]=1.0;
            global.SelectedObject.RGB[1]=0.3;
            global.SelectedObject.RGB[2]=0.3;
            global.SelectedObject.Size=global.Y_Resolution*(global.SelectedObject.Size/5);
            InfoSelectLabel->setColour(vec3_t(1.0,0.5,0.5));
        }
        else
        {   if (!messiers->Rechercher(tempPointer) /*&& FlagNeb*/)
            {   global.FlagSelect=true;
                messiers->InfoSelect(global.SelectedObject.XYZ,global.SelectedObject.RA,global.SelectedObject.DE,global.SelectedObject.Mag,global.SelectedObject.Name,global.SelectedObject.MessierNum,global.SelectedObject.NGCNum,global.SelectedObject.Size);
                global.SelectedObject.Size*=0.8;
                RA_en_hms(global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,global.SelectedObject.RA);
                global.SelectedObject.type=1;   //nebulae type
                global.SelectedObject.RGB[0]=0.4;
                global.SelectedObject.RGB[1]=0.5;
                global.SelectedObject.RGB[2]=0.8;
                InfoSelectLabel->setColour(vec3_t(0.4,0.5,0.8));
            }
            else
            {   if (!VouteCeleste->Rechercher(tempPointer) && global.FlagStars)
                {   global.FlagSelect=true;
                    VouteCeleste->InfoSelect(global.SelectedObject.XYZ,global.SelectedObject.RA,global.SelectedObject.DE,global.SelectedObject.Mag,global.SelectedObject.Name,global.SelectedObject.HR,global.SelectedObject.RGB,global.SelectedObject.CommonName);
                    RA_en_hms(global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,global.SelectedObject.RA);
                    global.SelectedObject.type=0;   //star type
                    global.SelectedObject.Size=10;
                    InfoSelectLabel->setColour(vec3_t(0.4,0.7,0.3));
                }
                else global.FlagSelect=false;
            }
        }
    }
}

/**********************************************************************/
// Update the infos about the selected object in the TextLabel widget
void updateInfoSelectString(void)
{   char objectInfo[300];
    objectInfo[0]=0;
    if (global.SelectedObject.type==0)  //Star
    {   glColor3f(0.4f,0.7f,0.3f);
        sprintf(objectInfo,"Info : %s\nName :%s\nHR  : %.4d\nRA  : %.2dh %.2dm %.2fs\nDE  : %.2fdeg\nMag : %.2f",
            global.SelectedObject.CommonName,
            global.SelectedObject.Name,
            global.SelectedObject.HR,
            global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,
            global.SelectedObject.DE*180/PI,
            global.SelectedObject.Mag); 
    }
    if (global.SelectedObject.type==1)  //Nebulae
    {   glColor3f(0.4f,0.5f,0.8f);
        sprintf(objectInfo,"Name: %s\nNGC : %d\nRA  : %.2dh %.2dm %.2fs\nDE  : %.2fdeg\nMag : %.2f",
            global.SelectedObject.Name,
            global.SelectedObject.NGCNum,
            global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,
            global.SelectedObject.DE*180/PI,
            global.SelectedObject.Mag);     
    }
    if (global.SelectedObject.type==2)  //Planet
    {   glColor3f(1.0f,0.3f,0.3f);
        sprintf(objectInfo,"Name: %s\nRA  : %.2dh %.2dm %.2fs\nDE  : %.2fdeg\nDistance : %.4f AU",
            global.SelectedObject.Name,
            global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,
            global.SelectedObject.DE*180/PI,
            global.SelectedObject.Distance);    
    }
    InfoSelectLabel->setLabel(objectInfo);
    InfoSelectLabel->setVisible(true);
}


/*******************************************************************/
void renderUi()
{   setOrthographicProjection(global.X_Resolution, global.Y_Resolution);    // 2D coordinate
    glPushMatrix();
    glLoadIdentity();

    updateStandardWidgets();

    glEnable(GL_SCISSOR_TEST);
    Base->render(*gc);
    glDisable(GL_SCISSOR_TEST);
    glPopMatrix();
    resetPerspectiveProjection();                                           // Restore the other coordinate
}

/*******************************************************************************/
void HandleMove(int x, int y)
{   Base->handleMouseMove(x, y);
}

/*******************************************************************************/
void HandleClic(int x, int y, int state, int button)
{   // Convert the name from GLU to my GUI
    enum guiValue bt;
    enum guiValue st;
    switch (button)
    {   case GLUT_RIGHT_BUTTON : bt=GUI_MOUSE_RIGHT; break;
        case GLUT_LEFT_BUTTON : bt=GUI_MOUSE_LEFT; break;
        case GLUT_MIDDLE_BUTTON : bt=GUI_MOUSE_MIDDLE; break;
    }
    if (state==GLUT_UP) st=GUI_UP; else st=GUI_DOWN;

    // Send the mouse event to the User Interface
    if (Base->handleMouseClic(x, y, st, bt))
    {   // If a "unthru" widget was at the clic position 
        return; 
    }
    else
    // Manage the event for the main window
    {   if (state==GLUT_UP) return;
        // Deselect the selected object
        if (button==GLUT_RIGHT_BUTTON)
        {   global.FlagSelect=false;
            global.FlagTraking = false;
            InfoSelectLabel->setVisible(false);
            return;
        }
        if (button==GLUT_MIDDLE_BUTTON)
        {   if (global.FlagSelect)
            {   Move_To(global.SelectedObject.XYZ);
            }
        }
        if (button==GLUT_LEFT_BUTTON)
        {   // Left or middle clic -> selection of an object
            findObject(x,y);
            // If an object has been found
            if (global.FlagSelect)
            {   updateInfoSelectString();
                if (global.FlagTraking)
                {   global.FlagTraking = false;
                }
            }
        }
    }
}

/*******************************************************************************/
void HandleNormalKey(unsigned char key, int state)
{   if (state==GUI_DOWN)
    {   switch(key)
        {   case 27 :   clearUi();
                        exit(0); 
                        break;
            case 'C' :
            case 'c' :  global.FlagConstellationDrawing=!global.FlagConstellationDrawing;
                        BtConstellationsDraw->setActive(global.FlagConstellationDrawing);
                        break;

            case 'P' :
            case 'p' :  global.FlagPlanetsHintDrawing=!global.FlagPlanetsHintDrawing;
                        break;
            
            case 'V' :
            case 'v' :  global.FlagConstellationName=!global.FlagConstellationName;
                        BtConstellationsName->setActive(global.FlagConstellationName);
                        break;
            case 'A' :
            case 'a' :  global.FlagAzimutalGrid=!global.FlagAzimutalGrid;
                        BtAzimutalGrid->setActive(global.FlagAzimutalGrid);
                        break;
            case 'E' :
            case 'e' :  global.FlagEquatorialGrid=!global.FlagEquatorialGrid;
                        BtEquatorialGrid->setActive(global.FlagEquatorialGrid);
                        break;
            case 'N' :
            case 'n' :  global.FlagNebulaName=!global.FlagNebulaName;
                        BtNebula->setActive(global.FlagNebulaName);
                        break;
            case 'G' :
            case 'g' :  global.FlagGround=!global.FlagGround;
                        BtGround->setActive(global.FlagGround);
                        break;
            case 'F' :
            case 'f' :  global.FlagFog=!global.FlagFog;
                        //BtFog->setActive(global.FlagFog);
                        break;
            case '1' :
            case '&' :  global.FlagRealTime=!global.FlagRealTime;
                        //BtRealTime->setActive(global.FlagRealTime);
                        break;
            case '2' :
            case '~' :  global.FlagAcceleredTime=!global.FlagAcceleredTime;
                        //BtAcceleredTime->setActive(global.FlagAcceleredTime);
                        break;
            case '3' :
            case '\"' : global.FlagVeryFastTime=!global.FlagVeryFastTime;
                        //BtVeryFastTime->setActive(global.FlagVeryFastTime);
                        break;
            case 'Q' :
            case 'q' :  global.FlagCardinalPoints=!global.FlagCardinalPoints;
                        BtCardinalPoints->setActive(global.FlagCardinalPoints);
                        break;
            case '*' :
            case 'µ' :  global.FlagAtmosphere=!global.FlagAtmosphere;
                        BtAtmosphere->setActive(global.FlagAtmosphere);
                        break;
            case 'R' :
            case 'r' :  global.FlagRealMode=!global.FlagRealMode;
                        TopWindowsInfos->setVisible(!global.FlagRealMode);
                        ContainerBtFlags->setVisible(!global.FlagRealMode);
                        break;
            case 'H' :
            case 'h' :  global.FlagHelp=!global.FlagHelp;
                        BtHelp->setActive(global.FlagHelp);
                        HelpWin->setVisible(global.FlagHelp);
                        if (global.FlagHelp && global.FlagInfos) global.FlagInfos=false;
                        break;
            case '4' :
            case '\'' : global.FlagEcliptic=!global.FlagEcliptic;
                        break;
            case '5' :
            case '(' :  global.FlagEquator=!global.FlagEquator;
                        break;
            case 'T' :
            case 't' :  global.FlagFollowEarth=!global.FlagFollowEarth;
                        BtFollowEarth->setActive(global.FlagFollowEarth);
                        break;
            case 'S' :
            case 's' :  global.FlagStars=!global.FlagStars;
                        break;

            case '+' :  global.SkyBrightness+=0.1;
                        if (global.SkyBrightness>1) global.SkyBrightness=1;
                        break;
            case '-' : global.SkyBrightness-=0.1;
                        if (global.SkyBrightness<0) global.SkyBrightness=0;
                        break;

            case ' ' :  if (global.FlagSelect) Move_To(global.SelectedObject.XYZ);
                        break;

            case 'I' :
            case 'i' : global.FlagInfos=!global.FlagInfos; 
                       InfoWin->setVisible(global.FlagInfos);
                        break;
        }
    }
}
