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

// Class which handles a stellarium User Interface

#include "stel_ui.h"


/**********************************************************************************/
/*                                   CALLBACKS                                    */
/**********************************************************************************/

void stel_ui::BtFlagsOnClicCallBack(guiValue button,Component * bt)
{
	switch (bt->getID())
    {   case 1 :    core->FlagConstellationDrawing=!core->FlagConstellationDrawing;
                    bt->setActive(core->FlagConstellationDrawing);
                    return;
        case 2 :    core->FlagConstellationName=!core->FlagConstellationName;
                    bt->setActive(core->FlagConstellationName);
                    return;
        case 3 :    core->FlagAzimutalGrid=!core->FlagAzimutalGrid;
                    bt->setActive(core->FlagAzimutalGrid);
                    return;
        case 4 :    core->FlagEquatorialGrid=!core->FlagEquatorialGrid;
                    bt->setActive(core->FlagEquatorialGrid);
                    return;
        case 5 :    core->FlagGround=!core->FlagGround;
                    bt->setActive(core->FlagGround);
                    return;
        case 6 :    core->FlagFog=!core->FlagFog;
                    bt->setActive(core->FlagFog);
                    return;
        case 7 :    core->FlagRealTime=!core->FlagRealTime;
                    bt->setActive(core->FlagRealTime);
                    return;
        case 8 :    core->FlagAcceleredTime=!core->FlagAcceleredTime;
                    bt->setActive(core->FlagAcceleredTime);
                    return;
        case 9 :    core->FlagVeryFastTime=!core->FlagVeryFastTime;
                    bt->setActive(core->FlagVeryFastTime);
                    return;
        case 10 :   core->FlagCardinalPoints=!core->FlagCardinalPoints;
                    bt->setActive(core->FlagCardinalPoints);
                    return;
        case 11 :   core->FlagAtmosphere=!core->FlagAtmosphere;
                    bt->setActive(core->FlagAtmosphere);
                    return;
        case 12 :   core->FlagNebulaName=!core->FlagNebulaName;
                    bt->setActive(core->FlagNebulaName);
                    return;
        case 13 :   core->FlagHelp=!core->FlagHelp;
                    HelpWin->setVisible(core->FlagHelp);
                    bt->setActive(core->FlagHelp);
                    return;
//        case 14 :   core->FlagRealMode=!core->FlagRealMode;
//                    bt->setActive(core->FlagRealMode);
//                    Base->setVisible(!core->FlagRealMode);
//                    break;
        case 15 :   navigation.set_flag_lock_equ_pos(!navigation.get_flag_lock_equ_pos());
                    bt->setActive(navigation.get_flag_lock_equ_pos());
                    break;
        case 16 :   core->FlagConfig=!core->FlagConfig;
                    ConfigWin->setVisible(core->FlagConfig);
                    bt->setActive(core->FlagConfig);
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
        case 15 :btLegend->setLabel("Compensation of the Earth rotation"); return;
        case 16 :btLegend->setLabel("Configuration window"); return;
    }
}

/*******************************************************************************/

void HelpWinHideCallback(void)
{   core->FlagHelp=false;
    HelpWin->setVisible(false);
    BtHelp->setActive(false);
}

/*******************************************************************************/

void InfoWinHideCallback(void)
{   core->FlagInfos=false;
    InfoWin->setVisible(false);
}

/*******************************************************************************/

void TimeControlWinHideCallback(void) 
{    TimeControlWin->setVisible(false);
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
/*                               CLASS FUNCTIONS                                  */
/**********************************************************************************/

stel_ui::stel_ui(stel_core * _core)
{
	if (!_core)
	{
		printf("ERROR : In stel_ui constructor, unvalid core.");
		exit(-1);
	}
	core = _core;
}

/*

	s_font * spaceFont = NULL;		// The font used in the GraphicContext (=Skin)
	GraphicsContext * gc = NULL;	// The graphic context used to give the widget their drawing style
	Container * Base = NULL;		// The container which contains everything (=the desktop)


	// Flags buttons (the buttons in the bottom left corner)
	FilledContainer * ContainerBtFlags = NULL;		// The containers for the button
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
	Label * btLegend = NULL;	// The dynamic information about the button under the mouse

	// The TextLabel displaying the infos about the selected object
	FilledTextLabel * InfoSelectLabel = NULL;

	// The top bar containing the main infos (date, time, fps etc...)
	FilledContainer * TopWindowsInfos = NULL;
	Label * DateLabel = NULL;
	Label * HourLabel = NULL;
	Label * FPSLabel = NULL;
	Label * AppNameLabel = NULL;
	Label * FOVLabel = NULL;

	// The window containing the help info
	StdBtWin * HelpWin = NULL;
	TextLabel * HelpTextLabel = NULL;

	// The window containing the info (licence)
	StdBtWin * InfoWin = NULL;
	TextLabel * InfoTextLabel = NULL;

	// The window containing the time controls
	StdBtWin * TimeControlWin = NULL;
	Container * TimeControlContainer = NULL;
	Labeled_Button * TimeRW = NULL;
	Labeled_Button * TimeFRW = NULL;
	Labeled_Button * TimeF = NULL;
	Labeled_Button * TimeNow = NULL;
	Labeled_Button * TimeFF = NULL;
	Labeled_Button * TimeReal = NULL;
	Labeled_Button * TimePause = NULL;


	// Configuration window for stellarium
	// All the function related with the configuration window are in
	// the file stel_ui_conf.cpp

	// The window containing the configuration options
	StdBtWin * ConfigWin = NULL;

	// Star configuration subsection
	FilledContainer * StarConfigContainer = NULL;
	Labeled_Button * ToggleStarName = NULL;
	Label * StarLabel = NULL;
	Label * StarNameLabel = NULL;
	Label * StarNameMagLabel = NULL;
	CursorBar * ChangeStarDrawNameBar = NULL;	// Cursor for the star name/magnitude
	Label * StarTwinkleLabel = NULL;
	CursorBar * ChangeStarTwinkleBar = NULL;	// Cursor for the star name/magnitude
	Label * StarScaleLabel = NULL;
	CursorBar * ChangeStarScaleBar = NULL;

	// Landscape configuration subsection
	FilledContainer * LandscapeConfigContainer = NULL;
	Label * LandscapeLabel = NULL;
	Labeled_Button * ToggleGround = NULL;
	Labeled_Button * ToggleFog = NULL;
	Labeled_Button * ToggleAtmosphere = NULL;
	Labeled_Button * ToggleMilkyWay = NULL;

	// Location configuration subsection
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
	*/

stel_ui::init()
{
    /*** fonts ***/
    char tempName[255];
    strcpy(tempName,core->DataDir);
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
    gc = new GraphicsContext(core->X_Resolution,core->Y_Resolution);
    if (!gc)
    {
        printf("ERROR WHILE CREATING GRAPHIC CONTEXT\n");
        exit(-1);
    }

	gc->baseColor=core->GuiBaseColor;
	gc->textColor=core->GuiTextColor;

    gc->setFont(spaceFont);
    gc->backGroundTexture=new s_texture("backmenu",TEX_LOAD_TYPE_PNG_ALPHA);
    gc->headerTexture=new s_texture("headermenu",TEX_LOAD_TYPE_PNG_ALPHA);

    /*** temporary variables ***/
    int avgCharLen=(int)gc->getFont()->getAverageCharLen();
    int lineHeight=(int)gc->getFont()->getLineHeight();


/*** Flag Buttons list ***/
    int btSize=24;
    int xpos=0;
    vec2_i pos(xpos,0);
    vec2_i btsz(btSize,btSize);
    vec2_i step(btSize,0);

    BtConstellationsDraw = new Textured_Button(new s_texture("Bouton1"),pos,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,1,core->FlagConstellationDrawing);
    BtConstellationsName = new Textured_Button(new s_texture("Bouton2"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,2,core->FlagConstellationName);
    BtAzimutalGrid = new Textured_Button(new s_texture("Bouton3"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,3,core->FlagAzimutalGrid);
    BtEquatorialGrid = new Textured_Button(new s_texture("Bouton3"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,4,core->FlagEquatorialGrid);
    BtNebula = new Textured_Button(new s_texture("Bouton15"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,12,core->FlagNebulaName);
    BtGround = new Textured_Button(new s_texture("Bouton4"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,5,core->FlagGround);
    BtCardinalPoints = new Textured_Button(new s_texture("Bouton8"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,10,core->FlagCardinalPoints);
    BtAtmosphere = new Textured_Button(new s_texture("Bouton9"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,11,core->FlagAtmosphere);
    BtHelp = new Textured_Button(new s_texture("Bouton11"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,13,core->FlagHelp);
    BtFollowEarth = new Textured_Button(new s_texture("Bouton13"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,15,navigation.get_flag_lock_equ_pos());
    BtConfig = new Textured_Button(new s_texture("Bouton16"),pos+=step,btsz,clWhite,clWhite/2,BtFlagsOnClicCallBack,BtFlagsOnMouseOverCallBack,16,core->FlagConfig);

    /*** Button container ***/
    ContainerBtFlags = new FilledContainer();
    ContainerBtFlags->reshape(0,core->Y_Resolution-btSize/*-1*/,btSize*11,btSize/*+1*/);
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

/*** button legend label ***/
    btLegend = new Label("ERROR, this shouldn't be seen...");
    btLegend->reshape(3,core->Y_Resolution-btSize-lineHeight-5,100,17);
    btLegend->setVisible(false);

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

    HelpWin = new StdBtWin(core->X_Resolution/2-25*avgCharLen, core->Y_Resolution/2-30*lineHeight/2, 48*avgCharLen, 32*(lineHeight+1), "Help", Base, spaceFont);
    HelpWin->addComponent(HelpTextLabel);
    HelpWin->setVisible(core->FlagHelp);
    HelpWin->setHideCallback(HelpWinHideCallback);


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
    
    InfoWin = new StdBtWin(core->X_Resolution/2-25*avgCharLen, core->Y_Resolution/2-30*lineHeight/2, 67*avgCharLen, 25*(lineHeight+1), "About Stellarium", Base, spaceFont);
    InfoWin->addComponent(InfoTextLabel);
    InfoWin->setVisible(core->FlagInfos);
    InfoWin->setHideCallback(InfoWinHideCallback);

/*** InfoSelect TextLabel ***/
    InfoSelectLabel = new FilledTextLabel("Info",gc->getFont());
    InfoSelectLabel->reshape(3,lineHeight+2,avgCharLen*50,6*lineHeight);
    InfoSelectLabel->setVisible(false);

/*** TopWindowsInfos Container ***/
    /*** Date Label ***/
    DateLabel = new Label("-");
    DateLabel->reshape(3,2,10*avgCharLen,lineHeight);
    /*** Hour Label ***/
    HourLabel = new Label("-");
    HourLabel->reshape(12*avgCharLen+15,2,6*avgCharLen,lineHeight);
    /*** FPS Label ***/
    FPSLabel = new Label("-");
    FPSLabel->reshape(core->X_Resolution-13*avgCharLen,2,10*avgCharLen,lineHeight);
    if (!core->FlagFps) FPSLabel->setVisible(false);
    /*** FOV Label ***/
    FOVLabel = new Label("-");
    FOVLabel->reshape(core->X_Resolution-26*avgCharLen,2,10*avgCharLen,lineHeight);
    /*** AppName Label ***/
    AppNameLabel = new Label(APP_NAME);
    AppNameLabel->reshape((int)(core->X_Resolution/2-gc->getFont()->getStrLen(APP_NAME)/2),2, (int)(gc->getFont()->getStrLen(APP_NAME)),lineHeight);
    /*** TopWindowsInfos Container ***/    
    TopWindowsInfos = new FilledContainer();
    TopWindowsInfos->reshape(0,0,core->X_Resolution,lineHeight+2);
    TopWindowsInfos->addComponent(DateLabel);
    TopWindowsInfos->addComponent(HourLabel);
    TopWindowsInfos->addComponent(FPSLabel);
    TopWindowsInfos->addComponent(FOVLabel);
    TopWindowsInfos->addComponent(AppNameLabel);


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



/*** Base container ***/
    Base = new Container();
    Base->reshape(0,0,core->X_Resolution,core->Y_Resolution);
    Base->addComponent(ContainerBtFlags);
    Base->addComponent(btLegend);
    Base->addComponent(InfoSelectLabel);
    Base->addComponent(TopWindowsInfos);
    Base->addComponent(InfoWin);
    Base->addComponent(HelpWin);
    Base->addComponent(ConfigWin);
    Base->addComponent(TimeControlWin);

	init_config_window();
}


/**********************************************************************************/
stel_ui::~stel_ui()
{
    if (Base) delete Base;
    Base = NULL;
    if (gc) delete gc;
    gc = NULL;
    if (spaceFont) delete spaceFont;
    spaceFont = NULL;
}

/*******************************************************************/
void stel_ui::render(void)
{
    setOrthographicProjection(core->X_Resolution, core->Y_Resolution);    // 2D coordinate

    updateStandardWidgets();

    glEnable(GL_SCISSOR_TEST);
    vec2_i sz = Base->getSize();
	gc->scissorPos[0]=0;
	gc->scissorPos[1]=0;
    glScissor(gc->scissorPos[0], gc->winH - gc->scissorPos[1] - sz[1], sz[0], sz[1]);
    
    Base->render(*gc);
    glDisable(GL_SCISSOR_TEST);

    resetPerspectiveProjection();             // Restore the other coordinate
}

/*******************************************************************************/
void stel_ui::handle_move(int x, int y)
{   
	Base->handleMouseMove(x, y);
}

/*******************************************************************************/
void stel_ui::handle_clic(Uint16 x, Uint16 y, Uint8 state, Uint8 button)
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
        	core->FlagConstellationDrawing=!core->FlagConstellationDrawing;
		}
        if(key==SDLK_p)
        {	
        	core->FlagPlanetsHintDrawing=!core->FlagPlanetsHintDrawing;
		}
        if(key==SDLK_v)
        {	
        	core->FlagConstellationName=!core->FlagConstellationName;
            BtConstellationsName->setActive(core->FlagConstellationName);
		}
        if(key==SDLK_z)
        {	
        	core->FlagAzimutalGrid=!core->FlagAzimutalGrid;
            BtAzimutalGrid->setActive(core->FlagAzimutalGrid);
		}
        if(key==SDLK_e)
        {	
        	core->FlagEquatorialGrid=!core->FlagEquatorialGrid;
            BtEquatorialGrid->setActive(core->FlagEquatorialGrid);
		}
        if(key==SDLK_n)
        {	
        	core->FlagNebulaName=!core->FlagNebulaName;
            BtNebula->setActive(core->FlagNebulaName);
		}
        if(key==SDLK_g)
        {	
        	core->FlagGround=!core->FlagGround;
            BtGround->setActive(core->FlagGround);
		}
        if(key==SDLK_f)
        {	
        	core->FlagFog=!core->FlagFog;
		}
        if(key==SDLK_1)
        {	
        	core->FlagRealTime=!core->FlagRealTime;
		}
        if(key==SDLK_2)
        {	
        	core->FlagAcceleredTime=!core->FlagAcceleredTime;
		}
        if(key==SDLK_3)
        {	
        	core->FlagVeryFastTime=!core->FlagVeryFastTime;
		}
        if(key==SDLK_q)
        {	
        	core->FlagCardinalPoints=!core->FlagCardinalPoints;
            BtCardinalPoints->setActive(core->FlagCardinalPoints);
		}
        if(key==SDLK_a)
        {	
        	core->FlagAtmosphere=!core->FlagAtmosphere;
            BtAtmosphere->setActive(core->FlagAtmosphere);
		}
        if(key==SDLK_r)
        {	
        	core->FlagRealMode=!core->FlagRealMode;
            TopWindowsInfos->setVisible(!core->FlagRealMode);
            ContainerBtFlags->setVisible(!core->FlagRealMode);
		}
        if(key==SDLK_h)
        {	
        	core->FlagHelp=!core->FlagHelp;
            BtHelp->setActive(core->FlagHelp);
            HelpWin->setVisible(core->FlagHelp);
            if (core->FlagHelp && core->FlagInfos) core->FlagInfos=false;
		}
        if(key==SDLK_4)
        {	
        	core->FlagEcliptic=!core->FlagEcliptic;
		}
        if(key==SDLK_5)
        {	
        	core->FlagEquator=!core->FlagEquator;
		}
        if(key==SDLK_t)
        {
			navigation.set_flag_lock_equ_pos(!navigation.get_flag_lock_equ_pos());
            BtFollowEarth->setActive(navigation.get_flag_lock_equ_pos());
		}
        if(key==SDLK_s)
        {	
        	core->FlagStars=!core->FlagStars;
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
        	core->FlagInfos=!core->FlagInfos; 
            InfoWin->setVisible(core->FlagInfos);
		}
    }
    return false;
}




/*******************************************************************************/
void updateStandardWidgets(void)
{   // Update the date and time
    char str[30];
	ln_date d;

    if (core->FlagUTC_Time)
    {   
		get_date(navigation.get_JDay(),&d);
    }
    else
    {
		get_date(navigation.get_JDay()+navigation.get_time_zone()*JD_HOUR,&d);
    }

    sprintf(str,"%.2d/%.2d/%.4d",d.days,d.months,d.years);
    DateLabel->setLabel(str);
    if (core->FlagUTC_Time)
    {
		sprintf(str,"%.2d:%.2d:%.2d (UTC)",d.hours,d.minutes,(int)d.seconds);
    }
    else
    {   sprintf(str,"%.2d:%.2d:%.2d",d.hours,d.minutes,(int)d.seconds);
    }
    HourLabel->setLabel(str);
    sprintf(str,"FPS : %4.2f",core->Fps);
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
