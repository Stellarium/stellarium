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
/*                               CLASS FUNCTIONS                                  */
/**********************************************************************************/

stel_ui::stel_ui(stel_core * _core) :
	spaceFont(NULL),
	containerTest(NULL)
{
	if (!_core)
	{
		printf("ERROR : In stel_ui constructor, unvalid core.");
		exit(-1);
	}
	core = _core;
}


void stel_ui::init(void)
{
    // Load standard font
    char tempName[255];
    strcpy(tempName,core->DataDir);
    strcat(tempName,"spacefont.txt");
    spaceFont = new s_font(13, "spacefont", tempName);
    if (!spaceFont)
    {
        printf("ERROR WHILE CREATING FONT\n");
        exit(-1);
    }

	// Create standard texture
	baseTex = new s_texture("jupiter");

	// Set default Painter
	Painter p(baseTex, spaceFont, s_color(0.5,1,0.5), s_color(1,1,1));
	Component::setDefaultPainter(p);

	Component::initScissor(core->screen_W, core->screen_H);

	desktop = new Container();

	desktop->reshape(0,0,core->screen_W,core->screen_H);

	containerTest = new FramedContainer();
	containerTest->reshape(100,100,100,200);

	containerTest2 = new FramedContainer();
	containerTest2->reshape(50,50,100,200);
	containerTest->addComponent(containerTest2);

	testTex = new s_texture("mars");
	btTest = new FilledButton();
	btTest->setTexture(testTex);
	btTest->setOnPressCallback(makeFunctor((s_pcallback0)0,*this,&stel_ui::btTestOnPress));

	desktop->addComponent(btTest);

	testLabel = new Label("Bonjour m'sieur!");
	desktop->addComponent(testLabel);
	testLabel->reshape(250,250,100,200);
	testLabel->adjustSize();

	desktop->addComponent(containerTest);

}

void stel_ui::btTestOnPress(void)
{
	printf("coucou!\n");
}

/**********************************************************************************/
stel_ui::~stel_ui()
{
    if (desktop) delete desktop;
    desktop = NULL;
    if (spaceFont) delete spaceFont;
    spaceFont = NULL;
	if (baseTex) delete baseTex;
	baseTex = NULL;
}

/*******************************************************************/
void stel_ui::draw(void)
{
    core->du->set_orthographic_projection();	// 2D coordinate
	Component::enableScissor();

    glScalef(1, -1, 1);						// invert the y axis, down is positive
    glTranslatef(0, -core->screen_H, 0);	// move the origin from the bottom left corner to the upper left corner


	desktop->draw();

	Component::disableScissor();
    core->du->reset_perspective_projection();	// Restore the other coordinate
}

/*******************************************************************************/
int stel_ui::handle_move(int x, int y)
{   
	return desktop->onMove(x, y);
}

/*******************************************************************************/
int stel_ui::handle_clic(Uint16 x, Uint16 y, Uint8 state, Uint8 button)
{   // Convert the name from GLU to my GUI
    enum S_GUI_VALUE bt;
    enum S_GUI_VALUE st;
    switch (button)
    {   case SDL_BUTTON_RIGHT : bt=S_GUI_MOUSE_RIGHT; break;
        case SDL_BUTTON_LEFT : bt=S_GUI_MOUSE_LEFT; break;
        case SDL_BUTTON_MIDDLE : bt=S_GUI_MOUSE_MIDDLE; break;
        default : bt=S_GUI_MOUSE_LEFT;
    }
    if (state==SDL_RELEASED) st=S_GUI_RELEASED; else st=S_GUI_PRESSED;

    // Send the mouse event to the User Interface
    if (desktop->onClic((int)x, (int)y, bt, st))
    {   // If a "unthru" widget was at the clic position
        return 1;
    }
    else
    // Manage the event for the main window
    {
		if (state==SDL_RELEASED) return 1;
        // Deselect the selected object
        if (button==SDL_BUTTON_RIGHT)
        {
			core->selected_object=NULL;
            return 1;
        }
        if (button==SDL_BUTTON_MIDDLE)
        {
			if (core->selected_object)
            {
				core->navigation->move_to(core->selected_object->get_earth_equ_pos());
            }
        }
        if (button==SDL_BUTTON_LEFT)
        {   
        	// CTRL + left clic = right clic for 1 button mouse
			if (SDL_GetModState() & KMOD_CTRL)
			{
				core->selected_object=NULL;
            	//InfoSelectLabel->setVisible(false);
            	return 1;
        	}
        	// Left or middle clic -> selection of an object
            core->selected_object=core->find_stel_object((int)x,(int)y);
            // If an object has been found
            if (core->selected_object)
            {
				//updateInfoSelectString();
				core->navigation->set_flag_traking(0);
            }
        }
    }
	return 0;
}

/*******************************************************************************/
int stel_ui::handle_keys(SDLKey key, S_GUI_VALUE state)
{
	desktop->onKey(key, state);
	/*if (state==GUI_DOWN)
    {   
    	if(key==SDLK_ESCAPE)
    	{ 	
    		//clearUi();  TODO : Find something for the end..
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
        	if (core->selected_object)
			{
				core->navigation->move_to(core->selected_object->get_earth_equ_pos());
				core->navigation->set_flag_traking(1);
			}
		}
        if(key==SDLK_i)
        {	
        	core->FlagInfos=!core->FlagInfos; 
            InfoWin->setVisible(core->FlagInfos);
		}
    }*/
    return 0;
}




/*******************************************************************************/
/*void stel_ui::updateStandardWidgets(void)
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
    sprintf(str,"FPS : %4.2f",core->fps);
    FPSLabel->setLabel(str);
    sprintf(str,"fov=%.3f\6", core->navigation->get_fov());
    FOVLabel->setLabel(str);
}
*/

/**********************************************************************/
// Update the infos about the selected object in the TextLabel widget
/*void stel_ui::updateInfoSelectString(void)
{
	char objectInfo[300];
    objectInfo[0]=0;
	core->selected_object->get_info_string(objectInfo);
    InfoSelectLabel->setLabel(objectInfo);
    InfoSelectLabel->setVisible(true);
	if (core->selected_object->get_type()==STEL_OBJECT_NEBULA) InfoSelectLabel->setColour(Vec3f(0.4f,0.5f,0.8f));
	if (core->selected_object->get_type()==STEL_OBJECT_PLANET) InfoSelectLabel->setColour(Vec3f(1.0f,0.3f,0.3f));
	if (core->selected_object->get_type()==STEL_OBJECT_STAR)   InfoSelectLabel->setColour(core->selected_object->get_RGB());
}*/
