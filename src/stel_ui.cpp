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
#include "stellastro.h"

////////////////////////////////////////////////////////////////////////////////
//								CLASS FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

stel_ui::stel_ui(stel_core * _core) :
	spaceFont(NULL),
	top_bar_ctr(NULL),
	top_bar_date_lbl(NULL),
	top_bar_hour_lbl(NULL),
	top_bar_fps_lbl(NULL),
	top_bar_appName_lbl(NULL),
	top_bar_fov_lbl(NULL),

	bt_flag_ctr(NULL),
	bt_flag_asterism_draw(NULL),
	bt_flag_asterism_name(NULL),
	bt_flag_azimuth_grid(NULL),
	bt_flag_equator_grid(NULL),
	bt_flag_ground(NULL),
	bt_flag_cardinals(NULL),
	bt_flag_atmosphere(NULL),
	bt_flag_nebula_name(NULL),
	bt_flag_help(NULL),
	bt_flag_follow_earth(NULL),
	bt_flag_config(NULL),
	bt_flag_help_lbl(NULL),

	info_select_ctr(NULL),
	info_select_txtlbl(NULL),

	licence_win(NULL),
	licence_txtlbl(NULL)
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
	baseTex = new s_texture("backmenu");

	// Set default Painter
	Painter p(baseTex, spaceFont, core->GuiBaseColor, core->GuiTextColor);
	Component::setDefaultPainter(p);

	Component::initScissor(core->screen_W, core->screen_H);

	desktop = new Container();
	desktop->reshape(0,0,core->screen_W,core->screen_H);

	bt_flag_help_lbl = new Label("Help for the Flag Buttons");
	bt_flag_help_lbl->setPos(3,core->screen_H-50);
	bt_flag_help_lbl->setVisible(0);

	// Info on selected object
	info_select_ctr = new FilledContainer();
	info_select_ctr->reshape(0,13,300,80);
    info_select_txtlbl = new TextLabel("Info");
    info_select_txtlbl->reshape(5,5,290,78);
    info_select_ctr->setVisible(0);
	info_select_ctr->addComponent(info_select_txtlbl);
	desktop->addComponent(info_select_ctr);

	desktop->addComponent(createTopBar());
	desktop->addComponent(createFlagButtons());
	desktop->addComponent(bt_flag_help_lbl);
	desktop->addComponent(createLicenceWindow());
}


Component* stel_ui::createTopBar(void)
{
    top_bar_date_lbl = new Label("-");	top_bar_date_lbl->setPos(2,2);
    top_bar_hour_lbl = new Label("-");	top_bar_hour_lbl->setPos(90,2);
    top_bar_fps_lbl = new Label("-");	top_bar_fps_lbl->setPos(core->screen_W-75,2);
    if (!core->FlagShowFps) top_bar_fps_lbl->setVisible(false);
    top_bar_fov_lbl = new Label("-");	top_bar_fov_lbl->setPos(core->screen_W-170,2);
    top_bar_appName_lbl = new Label(APP_NAME);
    top_bar_appName_lbl->setPos(core->screen_W/2-top_bar_appName_lbl->getSizex()/2,2);
    top_bar_ctr = new FilledContainer();
    top_bar_ctr->reshape(0,0,core->screen_W,14);
    top_bar_ctr->addComponent(top_bar_date_lbl);
    top_bar_ctr->addComponent(top_bar_hour_lbl);
    top_bar_ctr->addComponent(top_bar_fps_lbl);
    top_bar_ctr->addComponent(top_bar_fov_lbl);
    top_bar_ctr->addComponent(top_bar_appName_lbl);
	return top_bar_ctr;
}

void stel_ui::updateTopBar(void)
{
    char str[30];
	ln_date d;

    if (core->FlagUTC_Time) get_date(core->navigation->get_JDay(),&d);
    else get_date(core->navigation->get_JDay()+core->navigation->get_time_zone()*JD_HOUR,&d);

    sprintf(str,"%.2d/%.2d/%.4d",d.days,d.months,d.years);
    top_bar_date_lbl->setLabel(str);	top_bar_date_lbl->adjustSize();

    if (core->FlagUTC_Time) sprintf(str,"%.2d:%.2d:%.2d (UTC)",d.hours,d.minutes,(int)d.seconds);
    else sprintf(str,"%.2d:%.2d:%.2d",d.hours,d.minutes,(int)d.seconds);

    top_bar_hour_lbl->setLabel(str);	top_bar_hour_lbl->adjustSize();

    sprintf(str,"FPS : %4.2f",core->fps);
    top_bar_fps_lbl->setLabel(str); 	top_bar_fps_lbl->adjustSize();
    sprintf(str,"fov=%.3f\6", core->navigation->get_fov());
	top_bar_fov_lbl->setLabel(str);		top_bar_fov_lbl->adjustSize();
}

// Create the button panel in the lower left corner
Component* stel_ui::createFlagButtons(void)
{
	bt_flag_asterism_draw = new FlagButton(core->FlagAsterismDrawing, NULL, "Bouton1");
	bt_flag_asterism_draw->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb1));
	bt_flag_asterism_draw->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr1));

	bt_flag_asterism_name = new FlagButton(core->FlagAsterismName, NULL, "Bouton2");
	bt_flag_asterism_name->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb2));
	bt_flag_asterism_name->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr2));

	bt_flag_azimuth_grid = new FlagButton(core->FlagAzimutalGrid, NULL, "Bouton3");
	bt_flag_azimuth_grid->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb3));
	bt_flag_azimuth_grid->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr3));

	bt_flag_equator_grid = new FlagButton(core->FlagEquatorialGrid, NULL, "Bouton3");
	bt_flag_equator_grid->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb4));
	bt_flag_equator_grid->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr4));

	bt_flag_ground = new FlagButton(core->FlagGround, NULL, "Bouton4");
	bt_flag_ground->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb5));
	bt_flag_ground->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr5));

	bt_flag_cardinals = new FlagButton(core->FlagCardinalPoints, NULL, "Bouton8");
	bt_flag_cardinals->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb6));
	bt_flag_cardinals->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr6));

	bt_flag_atmosphere = new FlagButton(core->FlagAtmosphere, NULL, "Bouton9");
	bt_flag_atmosphere->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb7));
	bt_flag_atmosphere->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr7));

	bt_flag_nebula_name = new FlagButton(core->FlagNebulaName, NULL, "Bouton15");
	bt_flag_nebula_name->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb8));
	bt_flag_nebula_name->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr8));

	bt_flag_help = new FlagButton(core->FlagHelp, NULL, "Bouton11");
	bt_flag_help->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb9));
	bt_flag_help->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr9));

	bt_flag_follow_earth = new FlagButton(core->navigation->get_flag_lock_equ_pos(), NULL, "Bouton13");
	bt_flag_follow_earth->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb10));
	bt_flag_follow_earth->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr10));

	bt_flag_config = new FlagButton(core->FlagConfig, NULL, "Bouton16");
	bt_flag_config->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cb11));
	bt_flag_config->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::cbr11));

	bt_flag_ctr = new FilledContainer();
	bt_flag_ctr->addComponent(bt_flag_asterism_draw); 	bt_flag_asterism_draw->setPos(0,0);
	bt_flag_ctr->addComponent(bt_flag_asterism_name);	bt_flag_asterism_name->setPos(32,0);
	bt_flag_ctr->addComponent(bt_flag_azimuth_grid); 	bt_flag_azimuth_grid->setPos(64,0);
	bt_flag_ctr->addComponent(bt_flag_equator_grid);	bt_flag_equator_grid->setPos(96,0);
	bt_flag_ctr->addComponent(bt_flag_ground);			bt_flag_ground->setPos(128,0);
	bt_flag_ctr->addComponent(bt_flag_cardinals);		bt_flag_cardinals->setPos(160,0);
	bt_flag_ctr->addComponent(bt_flag_atmosphere);		bt_flag_atmosphere->setPos(192,0);
	bt_flag_ctr->addComponent(bt_flag_nebula_name);		bt_flag_asterism_draw->setPos(224,0);
	bt_flag_ctr->addComponent(bt_flag_help);			bt_flag_nebula_name->setPos(256,0);
	bt_flag_ctr->addComponent(bt_flag_follow_earth);	bt_flag_follow_earth->setPos(288,0);
	bt_flag_ctr->addComponent(bt_flag_config);			bt_flag_config->setPos(320,0);

	bt_flag_ctr->setOnMouseInOutCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::bt_flag_ctrOnMouseInOut));
	bt_flag_ctr->reshape(0, core->screen_H-32, 11*32, 32);

	return bt_flag_ctr;

}

void stel_ui::cb1(void){core->FlagAsterismDrawing 	= bt_flag_asterism_draw->getState();}
void stel_ui::cb2(void){core->FlagAsterismName 		= bt_flag_asterism_name->getState();}
void stel_ui::cb3(void){core->FlagAzimutalGrid 		= bt_flag_azimuth_grid->getState();}
void stel_ui::cb4(void){core->FlagEquatorialGrid 	= bt_flag_equator_grid->getState();}
void stel_ui::cb5(void){core->FlagGround	 		= bt_flag_ground->getState();}
void stel_ui::cb6(void){core->FlagCardinalPoints	= bt_flag_cardinals->getState();}
void stel_ui::cb7(void){core->FlagAtmosphere 		= bt_flag_atmosphere->getState();}
void stel_ui::cb8(void){core->FlagNebulaName		= bt_flag_nebula_name->getState();}
void stel_ui::cb9(void){core->FlagHelp				= bt_flag_help->getState();}
void stel_ui::cb10(void){core->navigation->set_flag_lock_equ_pos(bt_flag_follow_earth->getState());}
void stel_ui::cb11(void){core->FlagConfig			= bt_flag_config->getState();}

void stel_ui::bt_flag_ctrOnMouseInOut(void)
{
	if (bt_flag_ctr->getIsMouseOver()) bt_flag_help_lbl->setVisible(1);
	else bt_flag_help_lbl->setVisible(0);
}

void stel_ui::cbr1(void) {if (bt_flag_asterism_draw->getIsMouseOver())
bt_flag_help_lbl->setLabel("Drawing of the Constellations [C]");}
void stel_ui::cbr2(void) {if (bt_flag_asterism_name->getIsMouseOver())
bt_flag_help_lbl->setLabel("Names of the Constellations [V]");}
void stel_ui::cbr3(void) {if (bt_flag_azimuth_grid->getIsMouseOver())
bt_flag_help_lbl->setLabel("Azimutal Grid");}
void stel_ui::cbr4(void) {if (bt_flag_equator_grid->getIsMouseOver())
bt_flag_help_lbl->setLabel("Equatorial Grid");}
void stel_ui::cbr5(void) {if (bt_flag_ground->getIsMouseOver())
bt_flag_help_lbl->setLabel("Ground [G]");}
void stel_ui::cbr6(void) {if (bt_flag_cardinals->getIsMouseOver())
bt_flag_help_lbl->setLabel("Cardinal Points [Q]");}
void stel_ui::cbr7(void) {if (bt_flag_atmosphere->getIsMouseOver())
bt_flag_help_lbl->setLabel("Atmosphere [A]");}
void stel_ui::cbr8(void) {if (bt_flag_nebula_name->getIsMouseOver())
bt_flag_help_lbl->setLabel("Nebulas [N]");}
void stel_ui::cbr9(void) {if (bt_flag_help->getIsMouseOver())
bt_flag_help_lbl->setLabel("Help [H]");}
void stel_ui::cbr10(void) {if (bt_flag_follow_earth->getIsMouseOver())
bt_flag_help_lbl->setLabel("Compensation of the Earth rotation");}
void stel_ui::cbr11(void) {if (bt_flag_config->getIsMouseOver())
bt_flag_help_lbl->setLabel("Configuration window");}



// The window containing the info (licence)
Component* stel_ui::createLicenceWindow(void)
{
	licence_txtlbl = new TextLabel(
"                 \1   " APP_NAME "  July 2003  \1\n\
 \n\
\1   Copyright (c) 2000-2003 Fabien Chereau\n\
 \n\
\1   Please check last version and send bug report & comments\n\n\
on stellarium web page : http://stellarium.free.fr\n\n\
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
Boston, MA  02111-1307, USA.\n");
	licence_txtlbl->adjustSize();
	licence_txtlbl->setPos(10,10);
	licence_win = new StdBtWin("Infos");
	licence_win->reshape(300,200,400,350);
	licence_win->addComponent(licence_txtlbl);

	return licence_win;
}

/**********************************************************************************/
stel_ui::~stel_ui()
{
    if (desktop) delete desktop; 		desktop = NULL;
    if (spaceFont) delete spaceFont; 	spaceFont = NULL;
	if (baseTex) delete baseTex; 		baseTex = NULL;

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
int stel_ui::handle_clic(Uint16 x, Uint16 y, Uint8 button, Uint8 state)
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
    {
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
			info_select_ctr->setVisible(0);
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
            	info_select_ctr->setVisible(0);
            	return 1;
        	}
        	// Left or middle clic -> selection of an object
            core->selected_object=core->find_stel_object((int)x, core->screen_H-(int)y);
            // If an object has been found
            if (core->selected_object)
            {
				updateInfoSelectString();
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
	if (state==S_GUI_PRESSED)
    {   
    	if(key==SDLK_ESCAPE)
    	{ 	
			return 0;	// Will quit properly from stel_sdl
		}
        if(key==SDLK_c)
        {	
        	core->FlagAsterismDrawing=!core->FlagAsterismDrawing;
		}
        if(key==SDLK_p)
        {	
        	core->FlagPlanetsHintDrawing=!core->FlagPlanetsHintDrawing;
		}
        if(key==SDLK_v)
        {
        	core->FlagAsterismName=!core->FlagAsterismName;
            //BtConstellationsName->setActive(core->FlagConstellationName);
		}
        if(key==SDLK_z)
        {	
        	core->FlagAzimutalGrid=!core->FlagAzimutalGrid;
            //BtAzimutalGrid->setActive(core->FlagAzimutalGrid);
		}
        if(key==SDLK_e)
        {	
        	core->FlagEquatorialGrid=!core->FlagEquatorialGrid;
            //BtEquatorialGrid->setActive(core->FlagEquatorialGrid);
		}
        if(key==SDLK_n)
        {	
        	core->FlagNebulaName=!core->FlagNebulaName;
            //BtNebula->setActive(core->FlagNebulaName);
		}
        if(key==SDLK_g)
        {	
        	core->FlagGround=!core->FlagGround;
            //BtGround->setActive(core->FlagGround);
		}
        if(key==SDLK_f)
        {	
        	core->FlagFog=!core->FlagFog;
		}
        if(key==SDLK_q)
        {	
        	core->FlagCardinalPoints=!core->FlagCardinalPoints;
            //BtCardinalPoints->setActive(core->FlagCardinalPoints);
		}
        if(key==SDLK_a)
        {	
        	core->FlagAtmosphere=!core->FlagAtmosphere;
            //BtAtmosphere->setActive(core->FlagAtmosphere);
		}
        if(key==SDLK_h)
        {	
        	core->FlagHelp=!core->FlagHelp;
            //BtHelp->setActive(core->FlagHelp);
            //HelpWin->setVisible(core->FlagHelp);
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
			core->navigation->set_flag_lock_equ_pos(!core->navigation->get_flag_lock_equ_pos());
            //BtFollowEarth->setActive(navigation.get_flag_lock_equ_pos());
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
            licence_win->setVisible(core->FlagInfos);
		}
    }
    return 0;
}


// Update changing values
void stel_ui::update(void)
{
	updateTopBar();
}



// Update the infos about the selected object in the TextLabel widget
void stel_ui::updateInfoSelectString(void)
{
	char objectInfo[300];
    objectInfo[0]=0;
	core->selected_object->get_info_string(objectInfo);
    info_select_txtlbl->setLabel(objectInfo);
    info_select_ctr->setVisible(1);
	if (core->selected_object->get_type()==STEL_OBJECT_NEBULA)
		info_select_txtlbl->setTextColor(Vec3f(0.4f,0.5f,0.8f));
	if (core->selected_object->get_type()==STEL_OBJECT_PLANET)
		info_select_txtlbl->setTextColor(Vec3f(1.0f,0.3f,0.3f));
	if (core->selected_object->get_type()==STEL_OBJECT_STAR)
		info_select_txtlbl->setTextColor(core->selected_object->get_RGB());
}
