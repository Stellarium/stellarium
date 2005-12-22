/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#include <iostream>

#include "stel_ui.h"
#include "stellastro.h"

////////////////////////////////////////////////////////////////////////////////
//								CLASS FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

StelUI::StelUI(StelCore * _core) :
		baseFont(NULL),
		courierFont(NULL),

		top_bar_ctr(NULL),
		top_bar_date_lbl(NULL),
		top_bar_hour_lbl(NULL),
		top_bar_fps_lbl(NULL),
		top_bar_appName_lbl(NULL),
		top_bar_fov_lbl(NULL),

		bt_flag_ctr(NULL),
		bt_flag_constellation_draw(NULL),
		bt_flag_constellation_name(NULL),
		bt_flag_constellation_art(NULL),
		bt_flag_azimuth_grid(NULL),
		bt_flag_equator_grid(NULL),
		bt_flag_ground(NULL),
		bt_flag_cardinals(NULL),
		bt_flag_atmosphere(NULL),
		bt_flag_nebula_name(NULL),
		bt_flag_help(NULL),
		bt_flag_equatorial_mode(NULL),
		bt_flag_config(NULL),
		bt_flag_chart(NULL),
		bt_flag_night(NULL),
		bt_flag_search(NULL),
        bt_script(NULL),
		bt_flag_goto(NULL),
		bt_flag_help_lbl(NULL),
		info_select_ctr(NULL),
		info_select_txtlbl(NULL),

		licence_win(NULL),
		licence_txtlbl(NULL),

		help_win(NULL),
		help_txtlbl(NULL),

		config_win(NULL),
		search_win(NULL),
		dialog_win(NULL),
		tui_root(NULL)

{
	if (!_core)
	{
		printf("ERROR : In stel_ui constructor, unvalid core.");
		exit(-1);
	}
	core = _core;
	is_dragging = false;
	waitOnLocation = true;
	opaqueGUI = true;
	initialised = false;
}

/**********************************************************************************/
StelUI::~StelUI()
{
	delete desktop; 	desktop = NULL;
	delete baseFont; 	baseFont = NULL;
	delete baseTex; 	baseTex = NULL;
	delete flipBaseTex; flipBaseTex = NULL;
	delete courierFont; courierFont = NULL;
	delete tex_up; tex_up = NULL;
	delete tex_down; tex_down = NULL;
	if (tui_root) delete tui_root; tui_root=NULL;
	Component::deleteScissor();
}

////////////////////////////////////////////////////////////////////////////////
void StelUI::init(void)
{
	// Load standard font
	baseFont = new s_font(core->BaseFontSize, core->DataDir + core->BaseFontName);
	if (!baseFont)
	{
		printf("ERROR WHILE CREATING FONT\n");
		exit(-1);
	}

	courierFont = new s_font(core->BaseCFontSize, core->BaseCFontName);
	if (!courierFont)
	{
		printf("ERROR WHILE CREATING FONT\n");
		exit(-1);
	}

	// set up mouse cursor timeout
	MouseTimeLeft = core->MouseCursorTimeout*1000;

	// Create standard texture
	baseTex = new s_texture("backmenu", TEX_LOAD_TYPE_PNG_ALPHA);
	flipBaseTex = new s_texture("backmenu_flip", TEX_LOAD_TYPE_PNG_ALPHA);

	tex_up = new s_texture("up");
	tex_down = new s_texture("down");

	// Set default Painter
	Painter p(baseTex, baseFont, core->GuiBaseColor, core->GuiTextColor);
	Component::setDefaultPainter(p);

	Component::initScissor(core->screen_W, core->screen_H);

	desktop = new Container(true);
	desktop->reshape(0,0,core->screen_W,core->screen_H);

	bt_flag_help_lbl = new Label("ERROR...");
	bt_flag_help_lbl->setPos(3,core->screen_H-41-(int)baseFont->getDescent());
	bt_flag_help_lbl->setVisible(0);

	bt_flag_time_control_lbl = new Label("ERROR...");
	bt_flag_time_control_lbl->setPos(core->screen_W-180,core->screen_H-41-(int)baseFont->getDescent());
	bt_flag_time_control_lbl->setVisible(0);

	// Info on selected object
	info_select_ctr = new Container();
	info_select_ctr->reshape(0,15,300,200);
	info_select_txtlbl = new TextLabel();
    info_select_txtlbl->reshape(5,5,550,202);
	info_select_ctr->setVisible(1);
	info_select_ctr->addComponent(info_select_txtlbl);
	info_select_ctr->setGUIColorSchemeMember(false);
	desktop->addComponent(info_select_ctr);

	// TEST message window
	message_txtlbl = new TextLabel();
	message_txtlbl->adjustSize();
	message_txtlbl->setPos(10,10);
	message_win = new StdTransBtWin(_("Message"), 5000);
	message_win->setOpaque(opaqueGUI);
	message_win->reshape(300,200,400,100);
	message_win->addComponent(message_txtlbl);
	message_win->setVisible(false);
	desktop->addComponent(message_win);

	desktop->addComponent(createTopBar());
	desktop->addComponent(createFlagButtons());
	desktop->addComponent(createTimeControlButtons());
	desktop->addComponent(bt_flag_help_lbl);
	desktop->addComponent(bt_flag_time_control_lbl);

	dialog_win = new StdDlgWin(_("Stellarium"));
	dialog_win->setOpaque(opaqueGUI);
	dialog_win->setDialogCallback(callback<void>(this, &StelUI::dialogCallback));
	desktop->addComponent(dialog_win);

	desktop->addComponent(createLicenceWindow());
	desktop->addComponent(createHelpWindow());
	desktop->addComponent(createConfigWindow());
	desktop->addComponent(createSearchWindow());

	initialised = true;
}


////////////////////////////////////////////////////////////////////////////////
void StelUI::show_message(string _message, int _time_out)
{
	// draws a message window to display a message to user
	// if timeout is zero, won't time out
	// otherwise use miliseconds

	// TODO figure out how to size better for varying message lengths

	message_txtlbl->setLabel(_message);
	message_txtlbl->adjustSize();
	message_win->set_timeout(_time_out);
	message_win->setVisible(1);
}

////////////////////////////////////////////////////////////////////////////////
// Tony
void StelUI::gotoObject(void)
{
    core->navigation->move_to(core->selected_object->get_earth_equ_pos(core->navigation),
	                      core->auto_move_duration);
}

////////////////////////////////////////////////////////////////////////////////
Component* StelUI::createTopBar(void)
{
	top_bar_date_lbl = new Label("-", baseFont);	top_bar_date_lbl->setPos(2,2);
	top_bar_hour_lbl = new Label("-", baseFont);	top_bar_hour_lbl->setPos(110,2);
	top_bar_fps_lbl = new Label("-", baseFont);	top_bar_fps_lbl->setPos(core->screen_W-100,2);
	top_bar_fov_lbl = new Label("-", baseFont);	top_bar_fov_lbl->setPos(core->screen_W-220,2);
	top_bar_appName_lbl = new Label(APP_NAME, baseFont);
	top_bar_appName_lbl->setPos(core->screen_W/2-top_bar_appName_lbl->getSizex()/2,2);
	top_bar_ctr = new FilledContainer();
	top_bar_ctr->reshape(0,0,core->screen_W,(int)(baseFont->getLineHeight()+0.5)+5);
	top_bar_ctr->addComponent(top_bar_date_lbl);
	top_bar_ctr->addComponent(top_bar_hour_lbl);
	top_bar_ctr->addComponent(top_bar_fps_lbl);
	top_bar_ctr->addComponent(top_bar_fov_lbl);
	top_bar_ctr->addComponent(top_bar_appName_lbl);
	return top_bar_ctr;
}

////////////////////////////////////////////////////////////////////////////////
void StelUI::updateTopBar(void)
{
	top_bar_ctr->setVisible(core->FlagShowTopBar);
	if (!core->FlagShowTopBar) return;

	double jd = core->navigation->get_JDay();

	if (core->FlagShowDate)
	{
		if (core->FlagUTC_Time)
			top_bar_date_lbl->setLabel(core->observatory->get_printable_date_UTC(jd));
		else
			top_bar_date_lbl->setLabel(core->observatory->get_printable_date_local(jd));
		top_bar_date_lbl->adjustSize();
	}
	top_bar_date_lbl->setVisible(core->FlagShowDate);

	if (core->FlagShowTime)
	{
		if (core->FlagUTC_Time)
			top_bar_hour_lbl->setLabel(core->observatory->get_printable_time_UTC(jd) + " (UTC)");
		else
			top_bar_hour_lbl->setLabel(core->observatory->get_printable_time_local(jd));
		top_bar_hour_lbl->adjustSize();
	}
	top_bar_hour_lbl->setVisible(core->FlagShowTime);

	top_bar_appName_lbl->setVisible(core->FlagShowAppName);

	char str[30];
	if (core->FlagShowFov)
	{
		sprintf(str,"fov=%2.3f\6", core->projection->get_visible_fov());
		top_bar_fov_lbl->setLabel(str);
		top_bar_fov_lbl->adjustSize();
	}
	top_bar_fov_lbl->setVisible(core->FlagShowFov);

	if (core->FlagShowFps)
	{
		sprintf(str,"FPS:%4.2f",core->fps);
		top_bar_fps_lbl->setLabel(str);
		top_bar_fps_lbl->adjustSize();
	}
	top_bar_fps_lbl->setVisible(core->FlagShowFps);
}

// Create the button panel in the lower left corner
#define UI_PADDING 5
#define UI_BT 25
#define UI_SCRIPT_BAR 300
Component* StelUI::createFlagButtons(void)
{
    int x = 0;
    
	bt_flag_constellation_draw = new FlagButton(false, NULL, "bt_constellations");
	bt_flag_constellation_draw->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_constellation_draw->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_constellation_name = new FlagButton(false, NULL, "bt_const_names");
	bt_flag_constellation_name->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_constellation_name->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_constellation_art = new FlagButton(false, NULL, "bt_constart");
	bt_flag_constellation_art->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_constellation_art->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_azimuth_grid = new FlagButton(false, NULL, "bt_grid");
	bt_flag_azimuth_grid->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_azimuth_grid->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_equator_grid = new FlagButton(false, NULL, "bt_grid");
	bt_flag_equator_grid->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_equator_grid->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_ground = new FlagButton(false, NULL, "bt_ground");
	bt_flag_ground->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_ground->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_cardinals = new FlagButton(false, NULL, "bt_cardinal");
	bt_flag_cardinals->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_cardinals->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_atmosphere = new FlagButton(false, NULL, "bt_atmosphere");
	bt_flag_atmosphere->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_atmosphere->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_nebula_name = new FlagButton(false, NULL, "bt_nebula");
	bt_flag_nebula_name->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_nebula_name->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_help = new FlagButton(false, NULL, "bt_help");
	bt_flag_help->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_help->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_equatorial_mode = new FlagButton(false, NULL, "bt_follow");
	bt_flag_equatorial_mode->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_equatorial_mode->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_config = new FlagButton(false, NULL, "bt_config");
	bt_flag_config->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_config->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_chart = new FlagButton(false, NULL, "bt_chart");
	bt_flag_chart->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_chart->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_night = new FlagButton(false, NULL, "bt_nightview");
	bt_flag_night->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_night->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_quit = new FlagButton(true, NULL, "bt_quit");
	bt_flag_quit->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_quit->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_search = new FlagButton(true, NULL, "bt_search");
	bt_flag_search->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_search->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_script = new EditBox();
	bt_script->setAutoFocus(false);
	bt_script->setSize(299,24);
	bt_script->setOnKeyCallback(callback<void>(this, &StelUI::cbEditScriptKey));
	bt_script->setOnReturnKeyCallback(callback<void>(this, &StelUI::cbEditScriptExecute));
	bt_script->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_goto = new FlagButton(true, NULL, "bt_goto");
	bt_flag_goto->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_goto->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_ctr = new FilledContainer();
	bt_flag_ctr->addComponent(bt_flag_constellation_draw); 	bt_flag_constellation_draw->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_constellation_name);	bt_flag_constellation_name->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_constellation_art);	bt_flag_constellation_art->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_azimuth_grid); 	bt_flag_azimuth_grid->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_equator_grid);	bt_flag_equator_grid->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_ground);			bt_flag_ground->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_cardinals);		bt_flag_cardinals->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_atmosphere);		bt_flag_atmosphere->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_nebula_name);		bt_flag_nebula_name->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_equatorial_mode);	bt_flag_equatorial_mode->setPos(x,0);x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_goto);			bt_flag_goto->setPos(x,0); x+=UI_BT;

    x+= UI_PADDING;
    bt_flag_ctr->addComponent(bt_script);			bt_script->setPos(x,0);
	if (!core->FlagShowScriptBar)
    {
		bt_script->setVisible(false);
    }
    else
	{
		x+=UI_SCRIPT_BAR;
		x+= UI_PADDING;
	}

	bt_flag_ctr->addComponent(bt_flag_search);			bt_flag_search->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_config);			bt_flag_config->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_chart);			bt_flag_chart->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_night);			bt_flag_night->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_help);			bt_flag_help->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_quit);			bt_flag_quit->setPos(x,0); x+=UI_BT;

	bt_flag_ctr->setOnMouseInOutCallback(callback<void>(this, &StelUI::bt_flag_ctrOnMouseInOut));
	bt_flag_ctr->reshape(0, core->screen_H-25, x-1, 25);

	return bt_flag_ctr;

}

// Create the button panel in the lower right corner
Component* StelUI::createTimeControlButtons(void)
{
	bt_dec_time_speed = new LabeledButton("\2\2");
	bt_dec_time_speed->setSize(24,24);
	bt_dec_time_speed->setOnPressCallback(callback<void>(this, &StelUI::bt_dec_time_speed_cb));
	bt_dec_time_speed->setOnMouseInOutCallback(callback<void>(this, &StelUI::tcbr));

	bt_real_time_speed = new LabeledButton("\5");
	bt_real_time_speed->setSize(24,24);
	bt_real_time_speed->setOnPressCallback(callback<void>(this, &StelUI::bt_real_time_speed_cb));
	bt_real_time_speed->setOnMouseInOutCallback(callback<void>(this, &StelUI::tcbr));

	bt_inc_time_speed = new LabeledButton("\3\3");
	bt_inc_time_speed->setSize(24,24);
	bt_inc_time_speed->setOnPressCallback(callback<void>(this, &StelUI::bt_inc_time_speed_cb));
	bt_inc_time_speed->setOnMouseInOutCallback(callback<void>(this, &StelUI::tcbr));

	bt_time_now = new LabeledButton("N");
	bt_time_now->setSize(24,24);
	bt_time_now->setOnPressCallback(callback<void>(this, &StelUI::bt_time_now_cb));
	bt_time_now->setOnMouseInOutCallback(callback<void>(this, &StelUI::tcbr));

	bt_time_control_ctr = new FilledContainer();
	bt_time_control_ctr->addComponent(bt_dec_time_speed);	bt_dec_time_speed->setPos(0,0);
	bt_time_control_ctr->addComponent(bt_real_time_speed);	bt_real_time_speed->setPos(25,0);
	bt_time_control_ctr->addComponent(bt_inc_time_speed);	bt_inc_time_speed->setPos(50,0);
	bt_time_control_ctr->addComponent(bt_time_now);			bt_time_now->setPos(75,0);

	bt_time_control_ctr->setOnMouseInOutCallback(callback<void>(this, &StelUI::bt_time_control_ctrOnMouseInOut));
	bt_time_control_ctr->reshape(core->screen_W-4*25-1, core->screen_H-25, 4*25, 25);

	return bt_time_control_ctr;
}

void StelUI::bt_dec_time_speed_cb(void)
{
	double s = core->get_time_speed();
	if (s>JD_SECOND) s/=10.;
	else if (s<=-JD_SECOND) s*=10.;
	else if (s>-JD_SECOND && s<=0.) s=-JD_SECOND;
	else if (s>0. && s<=JD_SECOND) s=0.;
	core->set_time_speed(s);
}

void StelUI::bt_inc_time_speed_cb(void)
{
	double s = core->get_time_speed();
	if (s>=JD_SECOND) s*=10.;
	else if (s<-JD_SECOND) s/=10.;
	else if (s>=0. && s<JD_SECOND) s=JD_SECOND;
	else if (s>=-JD_SECOND && s<0.) s=0.;
	core->set_time_speed(s);
}

void StelUI::bt_real_time_speed_cb(void)
{
	core->set_time_speed(JD_SECOND);
}

void StelUI::bt_time_now_cb(void)
{
	core->set_JDay(get_julian_from_sys());
}

////////////////////////////////////////////////////////////////////////////////
// Script edit command line

void StelUI::cbEditScriptInOut(void)
{
	if (bt_script->getIsMouseOver())
	{
		bt_flag_help_lbl->setLabel(_("Script commander"));
    }
}

void StelUI::cbEditScriptKey(void)
{
	if (bt_script->getLastKey() == SDLK_SPACE || bt_script->getLastKey() == SDLK_TAB)
	{
		string command = bt_script->getText();
        transform(command.begin(), command.end(), command.begin(), ::tolower);
        if (bt_script->getLastKey() == SDLK_SPACE) command = command.substr(0,command.length()-1);
	}
	else if	(bt_script->getLastKey() == SDLK_ESCAPE)
	{
		bt_script->clearText();
	}
}

void StelUI::cbEditScriptExecute(void)
{
	cout << "Executing command: " << bt_script->getText() << endl;
	string command_string = bt_script->getText();

	bt_script->clearText();
	bt_script->setEditing(false);
     
	if (command_string == "goto")
	// Tony - testing
     {
        // this is a specified goto
        // test out on M83
	    int rahr = 13;
        float ramin = 37.1;
        int dedeg = -29;
        float demin = -52.0;

    	Vec3f XYZ;						// Cartesian equatorial position
        float RaRad = hms_to_rad(rahr, (double)ramin);
        float DecRad = dms_to_rad(dedeg, (double)demin);

       // Calc the Cartesian coord with RA and DE
       sphe_to_rect(RaRad,DecRad,XYZ);
       core->navigation->move_to(XYZ, core->auto_move_duration, false, 1);
                       
     }
     else
     {
        if (!core->commander->execute_command(command_string))
    		bt_flag_help_lbl->setLabel(_("Invalid Script command"));
    }
}

////////////////////////////////////////////////////////////////////////////////
void StelUI::cb(void)
{
	core->constellation_set_flag_lines(bt_flag_constellation_draw->getState());
	core->constellation_set_flag_names(bt_flag_constellation_name->getState());
	core->constellation_set_flag_art(  bt_flag_constellation_art->getState());
	core->FlagAzimutalGrid 		= bt_flag_azimuth_grid->getState();
	core->FlagEquatorialGrid 	= bt_flag_equator_grid->getState();
	core->FlagLandscape	 		= bt_flag_ground->getState();
	core->cardinals_points->set_flag_show(bt_flag_cardinals->getState());
	core->FlagAtmosphere 		= bt_flag_atmosphere->getState();
	core->nebulas->set_flag_hints( bt_flag_nebula_name->getState() );
	core->FlagHelp 				= bt_flag_help->getState();
	help_win->setVisible(core->FlagHelp);
	core->navigation->set_viewing_mode(bt_flag_equatorial_mode->getState() ? Navigator::VIEW_EQUATOR : Navigator::VIEW_HORIZON);
	core->FlagConfig			= bt_flag_config->getState();
	core->FlagChart				= bt_flag_chart->getState();
	if  (core->FlagNight != bt_flag_night->getState())
	{
		core->FlagNight				= bt_flag_night->getState();
		if (!core->FlagNight) desktop->setColorScheme(core->GuiBaseColor, core->GuiTextColor);
		else desktop->setColorScheme(core->GuiBaseColorr, core->GuiTextColorr);
	}
	core->SetDrawMode();
	config_win->setVisible(core->FlagConfig);

	core->FlagSearch			= bt_flag_search->getState();
	search_win->setVisible(core->FlagSearch);
	if (bt_flag_goto->getState() && core->selected_object)
        gotoObject();
	bt_flag_goto->setState(false);

	if (!bt_flag_quit->getState()) core->quit();
}

void StelUI::bt_flag_ctrOnMouseInOut(void)
{
	if (bt_flag_ctr->getIsMouseOver()) bt_flag_help_lbl->setVisible(1);
	else bt_flag_help_lbl->setVisible(0);
}

void StelUI::bt_time_control_ctrOnMouseInOut(void)
{
	if (bt_time_control_ctr->getIsMouseOver()) bt_flag_time_control_lbl->setVisible(1);
	else bt_flag_time_control_lbl->setVisible(0);
}

void StelUI::cbr(void)
{
	if (bt_flag_constellation_draw->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Drawing of the Constellations [C]"));
	if (bt_flag_constellation_name->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Names of the Constellations [V]"));
	if (bt_flag_constellation_art->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Constellations Art [R]"));
	if (bt_flag_azimuth_grid->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Azimuthal Grid [Z]"));
	if (bt_flag_equator_grid->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Equatorial Grid [E]"));
	if (bt_flag_ground->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Ground [G]"));
	if (bt_flag_cardinals->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Cardinal Points [Q]"));
	if (bt_flag_atmosphere->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Atmosphere [A]"));
	if (bt_flag_nebula_name->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Nebulas [N]"));
	if (bt_flag_help->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Help [H]"));
	if (bt_flag_equatorial_mode->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Equatorial/Altazimuthal Mount [ENTER]"));
	if (bt_flag_config->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Configuration window"));
	if (bt_flag_chart->getIsMouseOver())
		bt_flag_help_lbl->setLabel("Chart mode");
	if (bt_flag_night->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Night (red) mode"));
	if (bt_flag_quit->getIsMouseOver())
#ifndef MACOSX
		bt_flag_help_lbl->setLabel(_("Quit [CTRL + Q]"));
#else
		bt_flag_help_lbl->setLabel(_("Quit [CMD + Q]"));
#endif
	if (bt_flag_search->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Search for object"));
	if (bt_script->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Script commander"));
	if (bt_flag_goto->getIsMouseOver())
		bt_flag_help_lbl->setLabel(_("Goto selected object [SPACE]"));
}

void StelUI::tcbr(void)
{
	if (bt_dec_time_speed->getIsMouseOver())
		bt_flag_time_control_lbl->setLabel(_("Decrease Time Speed [J]"));
	if (bt_real_time_speed->getIsMouseOver())
		bt_flag_time_control_lbl->setLabel(_("Real Time Speed [K]"));
	if (bt_inc_time_speed->getIsMouseOver())
		bt_flag_time_control_lbl->setLabel(_("Increase Time Speed [L]"));
	if (bt_time_now->getIsMouseOver())
		bt_flag_time_control_lbl->setLabel(_("Return to Current Time"));
}

// The window containing the info (licence)
Component* StelUI::createLicenceWindow(void)
{
	licence_txtlbl = new TextLabel(
string("                 \1   " APP_NAME "  August 2005  \1\n\n") +
"\1   Copyright (c) 2000-2005 Fabien Chereau et al.\n\n" +
_("\1   Please check for newer versions and send bug reports\n\
and comments to us at: http://stellarium.sourceforge.net\n\n") +
"\1   This program is free software; you can redistribute it and/or\n\
modify it under the terms of the GNU General Public License\n\
as published by the Free Software Foundation; either version 2\n\
of the License, or (at your option) any later version.\n\n" +
"This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; without even the implied\n\
warranty of MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.  See the GNU General Public\n\
License for more details.\n\n" +
"You should have received a copy of the GNU General Public\n\
License along with this program; if not, write to:\n" +
"Free Software Foundation, Inc.\n\
59 Temple Place - Suite 330\n\
Boston, MA  02111-1307, USA.\n\
http://www.fsf.org");
	licence_txtlbl->adjustSize();
	licence_txtlbl->setPos(10,10);
	licence_win = new StdBtWin(_("Information"));
	licence_win->setOpaque(opaqueGUI);
	licence_win->reshape(275,175,450,400);
	licence_win->addComponent(licence_txtlbl);
	licence_win->setVisible(core->FlagInfos);

	return licence_win;
}

Component* StelUI::createHelpWindow(void)
{
	help_txtlbl = new TextLabel(
	                  string(_("\
Movement & selection:\n\
Arrow Keys       : Change viewing RA/DE\n\
Page Up/Down     : Zoom\n\
CTRL+Up/Down     : Zoom\n\
Left Click       : Select object\n\
Right Click      : Unselect\n\
CTRL+Left Click  : Unselect\n\
\\                : Zoom out (planet + moons if applicable)\n\
/                : Zoom to selected object\n\
SPACE            : Center on selected object\n\
\n\
Display options:\n\
ENTER : Equatorial/altazimuthal mount\n\
F1  : Toggle fullscreen if possible.\n\
C   : Constellation lines  V   : Constellation labels\n\
R   : Constellation art    E   : Equatorial grid\n\
Z   : Azimuthal grid       N   : Nebula labels\n\
P   : Planet labels        G   : Ground\n\
A   : Atmosphere           F   : Fog\n\
Q   : Cardinal points      O   : Toggle moon scaling\n\
T   : Object tracking      S   : Stars\n\
4 , : Ecliptic line        5 . : Equator line\n\
\n\
Dialogs & other controls:\n\
H   : Help                 I   : About Stellarium\n\
M   : Text menu            1 (one)  : Configuration\n\
CTRL + S : Take a screenshot\n\
CTRL + R : Toggle script recording\n\
CTRL + F : Toggle object finder\n\
\n\
Time & Date:\n\
6   : Time rate pause      7   : Time rate 0\n\
8   : Set current time     J   : Decrease time rate\n\
K   : Normal time rate     L   : Increase time rate\n\
-   : Back 24 hours        =   : Forward 24 hours\n\
[   : Back 7 days          ]   : Forward 7 days\n\
\n\
During Script Playback:\n\
CTRL + C : End Script\n\
6   : pause script         K   : resume script\n\
\n\
Misc:\n\
9   : Toggle meteor shower rates\n")) + string(
#ifndef MACOSX
	                      _("CTRL + Q : Quit\n")
#else
	                      _("CMD + Q  : Quit\n")
#endif

	                  ),courierFont);

//	help_txtlbl->adjustSize();
	help_txtlbl->setPos(10,10);
	help_win = new StdBtWin(_("Help"));
	help_win->setOpaque(opaqueGUI);
	help_win->reshape(215,70,580,600);
	help_win->addComponent(help_txtlbl);
	help_win->setVisible(core->FlagHelp);
	help_win->setOnHideBtCallback(callback<void>(this, &StelUI::help_win_hideBtCallback));
	return help_win;
}

void StelUI::help_win_hideBtCallback(void)
{
	help_win->setVisible(0);
}


/*******************************************************************/
void StelUI::draw(void)
{
	// Special cool text transparency mode
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_BLEND);

	core->projection->set_2Dfullscreen_projection();	// 2D coordinate
	Component::enableScissor();

	glScalef(1, -1, 1);						// invert the y axis, down is positive
	glTranslatef(0, -core->screen_H, 0);	// move the origin from the bottom left corner to the upper left corner

	desktop->draw();

	Component::disableScissor();
	core->projection->restore_from_2Dfullscreen_projection();	// Restore the other coordinate
}

/*******************************************************************************/
int StelUI::handle_move(int x, int y)
{
	// Do not allow use of mouse while script is playing
	// otherwise script can get confused
	if(core->scripts->is_playing()) return 0;

	// Show cursor
	SDL_ShowCursor(1);
	MouseTimeLeft = core->MouseCursorTimeout*1000; 

	if (desktop->onMove(x, y)) return 1;
	if (is_dragging)
	{
		if ((has_dragged || sqrtf((x-previous_x)*(x-previous_x)+(y-previous_y)*(y-previous_y))>4.))
		{
			has_dragged = true;
			core->navigation->set_flag_traking(0);
			Vec3d tempvec1, tempvec2;
			double az1, alt1, az2, alt2;
			if (core->navigation->get_viewing_mode()==Navigator::VIEW_HORIZON)
			{
				core->projection->unproject_local(x,core->screen_H-y, tempvec2);
				core->projection->unproject_local(previous_x,core->screen_H-previous_y, tempvec1);
			}
			else
			{
				core->projection->unproject_earth_equ(x,core->screen_H-y, tempvec2);
				core->projection->unproject_earth_equ(previous_x,core->screen_H-previous_y, tempvec1);
			}
			rect_to_sphe(&az1, &alt1, tempvec1);
			rect_to_sphe(&az2, &alt2, tempvec2);
			core->navigation->update_move(az2-az1, alt1-alt2);
			previous_x = x;
			previous_y = y;
			return 1;
		}
	}
	return 0;
}

/*******************************************************************************/
int StelUI::handle_clic(Uint16 x, Uint16 y, S_GUI_VALUE button, S_GUI_VALUE state)
{
	// Do not allow use of mouse while script is playing
	// otherwise script can get confused
	if(core->scripts->is_playing()) return 0;

	// Show cursor
	SDL_ShowCursor(1);
	MouseTimeLeft = core->MouseCursorTimeout*1000; 

	if (desktop->onClic((int)x, (int)y, button, state))
    {
        has_dragged = true;
		return 1;
	}

	switch (button)
	{
	case S_GUI_MOUSE_RIGHT : break;
	case S_GUI_MOUSE_LEFT :
		if (state==S_GUI_PRESSED)
		{
			is_dragging = true;
			has_dragged = false;
			previous_x = x;
			previous_y = y;
		}
		else
		{
			is_dragging = false;
		}
		break;
	case S_GUI_MOUSE_MIDDLE : break;
	case S_GUI_MOUSE_WHEELUP :
		core->zoom_in(state==S_GUI_PRESSED);
		core->update_move( core->get_mouse_zoom());
		return 1;
	case S_GUI_MOUSE_WHEELDOWN :
		core->zoom_out(state==S_GUI_PRESSED);
		core->update_move( core->get_mouse_zoom());
		return 1;
	default: break;
	}

	// Send the mouse event to the User Interface
//	if (desktop->onClic((int)x, (int)y, button, state))
//    {
//        has_dragged = true;
//		return 1;
//	}
//	else
		// Manage the event for the main window
	{
		if (state==S_GUI_PRESSED) return 1;
		// Deselect the selected object
		if (button==S_GUI_MOUSE_RIGHT)
		{
			core->commander->execute_command("select");
			return 1;
		}
		if (button==S_GUI_MOUSE_MIDDLE)
		{
			if (core->selected_object)
			{
				core->navigation->move_to(core->selected_object->get_earth_equ_pos(core->navigation),
				                          core->auto_move_duration);
				core->navigation->set_flag_traking(1);
			}
		}
		if (button==S_GUI_MOUSE_LEFT && !has_dragged)
		{
			// CTRL + left clic = right clic for 1 button mouse
			if (SDL_GetModState() & KMOD_CTRL)
			{
				core->commander->execute_command("select");
				return 1;
			}

			// Left clic -> selection of an object
			StelObject* tempselect= core->clever_find((int)x, core->screen_H-(int)y);

			// Unselect on second clic on the same object
			if (core->selected_object!=NULL && core->selected_object==tempselect)
			{
				core->commander->execute_command("select");
			}
			else
			{
				core->selected_object = core->clever_find((int)x, core->screen_H-(int)y);
			}

			// If an object has been found
			if (core->selected_object)
			{

				core->set_object_pointer_visibility(1);  // by default draw pointer around object
				updateInfoSelectString();
				// If an object was selected keep the earth following
				if (core->navigation->get_flag_traking()) core->navigation->set_flag_lock_equ_pos(1);
				core->navigation->set_flag_traking(0);

				if (core->selected_object->get_type()==StelObject::STEL_OBJECT_STAR)
				{
					core->asterisms->set_selected((HipStar*)core->selected_object);

					// potentially record this action
					std::ostringstream oss;
					oss << ((HipStar *)core->selected_object)->get_hp_number();
					core->scripts->record_command("select hp " + oss.str());

				}
				else
				{
					core->asterisms->set_selected(NULL);
				}

				if (core->selected_object->get_type()==StelObject::STEL_OBJECT_PLANET)
				{
					core->selected_planet=(Planet*)core->selected_object;

					// potentially record this action
					core->scripts->record_command("select planet " + ((Planet *)core->selected_object)->getEnglishName());

				}
				else
				{
					core->selected_planet=NULL;
				}

				if (core->selected_object->get_type()==StelObject::STEL_OBJECT_NEBULA)
				{

					// potentially record this action
					core->scripts->record_command("select nebula " + ((Nebula *)core->selected_object)->get_name());

				}

			}
			else
			{
				core->asterisms->set_selected(NULL);
				core->selected_planet=NULL;
			}
		}
	}
	return 0;
}


/*******************************************************************************/
int StelUI::handle_keys(Uint16 key, S_GUI_VALUE state)
{

	if (desktop->onKey(key, state))
	   return 1;

	if (state==S_GUI_PRESSED)
	{


#ifndef MACOSX
		if(key==0x0011)
		{ // CTRL-Q
#else
		if (key == SDLK_q && SDL_GetModState() & KMOD_META)
		{
#endif
			core->quit();
		}

#ifndef MACOSX
		if(key==0x0010)
		{ // CTRL-P  Play startup script, NOW!
#else
		if (key == SDLK_p && SDL_GetModState() & KMOD_META)
		{
#endif

			// first clear out audio and images kept in core
			core->commander->execute_command( "script action end");
			core->scripts->play_startup_script();
			return 1;
		}


		// if script is running, only script control keys are accessible
		// to pause/resume/cancel the script
		// (otherwise script could get very confused by user interaction)
		if(core->scripts->is_playing())
		{

			// here reusing time control keys to control the script playback
			if(key==SDLK_6)
			{
				// pause/unpause script
				core->commander->execute_command( "script action pause");
				core->time_multiplier = 1;  // don't allow resumption of ffwd this way (confusing for audio)
			}
			else if(key==SDLK_k)
			{
				core->commander->execute_command( "script action resume");
				core->time_multiplier = 1;
			}
			else if(key==SDLK_7 || key==0x0003 || (key==SDLK_m && core->FlagEnableTuiMenu))
			{  // ctrl-c
				// TODO: should double check with user here...
				core->commander->execute_command( "script action end");
				if(key==SDLK_m) core->FlagShowTuiMenu = true;
			}
			// TODO n is bad key if ui allowed
			else if(key==SDLK_GREATER || key==SDLK_n)
			{
				core->commander->execute_command( "audio volume increment");
			}
			// TODO d is bad key if ui allowed
			else if(key==SDLK_LESS || key==SDLK_d)
			{
				core->commander->execute_command( "audio volume decrement");

			}
			else if(key==SDLK_j)
			{
				if(core->time_multiplier==2) {
					core->time_multiplier = 1;

					// restart audio in correct place
					core->commander->execute_command( "audio action sync");
				} else if(core->time_multiplier > 1 ) {
					core->time_multiplier /= 2;
				}

			}
			else if(key==SDLK_l)
			{
				// stop audio since won't play at higher speeds
				core->commander->execute_command( "audio action pause");
				core->time_multiplier *= 2;
				if(core->time_multiplier>8) core->time_multiplier = 8;
			}
			else if(!core->scripts->get_allow_ui()) {
				cout << "Playing a script.  Press CTRL-C (or 7) to stop." << endl;
			}

			if(!core->scripts->get_allow_ui()) return 0;  // only limited user interaction allowed with script

		} else {
			core->time_multiplier = 1;  // if no script in progress always real time

			// normal time controls here (taken over for script control above if playing a script)
			if(key==SDLK_k) core->commander->execute_command( "timerate rate 1");
			if(key==SDLK_l) core->commander->execute_command( "timerate action increment");
			if(key==SDLK_j) core->commander->execute_command( "timerate action decrement");
			if(key==SDLK_6) core->commander->execute_command( "timerate action pause");
			if(key==SDLK_7) core->commander->execute_command( "timerate rate 0");
			if(key==SDLK_8) core->commander->execute_command( "date load preset");

		}

#ifndef MACOSX
		if(key==0x0012)
		{ // CTRL-R
#else
		if (key == SDLK_r && SDL_GetModState() & KMOD_META)
		{
#endif
			if(core->scripts->is_recording())
			{
				core->commander->execute_command( "script action cancelrecord");
				show_message(_("Command recording stopped."), 3000);
			}
			else
			{
				core->commander->execute_command( "script action record");

				if(core->scripts->is_recording())
				{
					show_message(string( _("Recording commands to script file:\n")
					                     + core->scripts->get_record_filename() + "\n\n"
					                     + _("Hit CTRL-R again to stop.\n")), 4000);
				}
				else
				{
					show_message(_("Error: Unable to open script file to record commands."), 3000);
				}
			}
		}

		// RFE 1310384, ESC closes dialogs
		if(key==SDLK_ESCAPE)
		{
			// close search mode
			core->FlagSearch=false;
			search_win->setVisible(core->FlagSearch);
	
			// close config dialog
			core->FlagConfig = false;
			config_win->setVisible(core->FlagConfig);
	
			// close help dialog
			core->FlagHelp = false;
			help_win->setVisible(core->FlagHelp);
	
			// close information dialog
			core->FlagInfos = false;
			licence_win->setVisible(core->FlagInfos);
		}
		// END RFE 1310384

#ifndef MACOSX
		if(key==0x0006)
		{ // CTRL-f
#else
		if (key == SDLK_f && SDL_GetModState() & KMOD_META)
		{
#endif
			core->FlagSearch = !core->FlagSearch;
			search_win->setVisible(core->FlagSearch);
		}

		if(key==SDLK_r) core->commander->execute_command( "flag constellation_art toggle");

		//		printf( "key = %d\n", key);

		if(key=='c') core->commander->execute_command( "flag constellation_drawing toggle");
		if(key=='C') core->commander->execute_command( "flag constellation_boundaries toggle");

		if(key=='d') core->commander->execute_command( "flag star_names toggle");
		if(key=='D')
		{
			// TODO: move to execute_command so can record
			if (core->hip_stars->get_starname_format() < 2)
				core->hip_stars->set_starname_format(core->hip_stars->get_starname_format()+1);
			else
				core->hip_stars->set_starname_format(0);
		}

		if(key==SDLK_1)
		{
			core->FlagConfig=!core->FlagConfig;
			config_win->setVisible(core->FlagConfig);
		}
		if(key==SDLK_p)
		{
			if(!core->FlagPlanetsHints)
			{
				core->commander->execute_command("flag planet_names on");
			}
			else if( !core->FlagPlanetsOrbits)
			{
				core->commander->execute_command("flag planet_orbits on");
			}
			else
			{
				core->commander->execute_command("flag planet_orbits off");
				core->commander->execute_command("flag planet_names off");
			}
		}
		if(key==SDLK_v) core->commander->execute_command( "flag constellation_names toggle");
		if(key==SDLK_z) {
			if(!core->FlagMeridianLine) {
				if(core->FlagAzimutalGrid) core->commander->execute_command( "flag azimuthal_grid 0");
				else core->commander->execute_command( "flag meridian_line 1");
			} else {
				core->commander->execute_command( "flag meridian_line 0");
				core->commander->execute_command( "flag azimuthal_grid 1");
			}
		}
		if(key==SDLK_e) core->commander->execute_command( "flag equatorial_grid toggle");

  		if(key==SDLK_n) core->commander->execute_command( "flag nebula_names toggle");
		if(key=='N') {
			// TODO this should be recordable via an execute_command
			// TODO should toggle between just nebulas with images and all nebulas
			// TODO when these are turned off, should not be selectable either!
			core->nebulas->set_show_ngc(!core->nebulas->get_show_ngc());

			/* TODO: move into nebula - pick based on screen size
			// Tony for long nebula names - toggles between no name, shortname and longname
	

			if (core->nebulas->get_nebulaname_format() < 3)
				core->nebulas->set_nebulaname_format(core->nebulas->get_nebulaname_format()+1);
			else core->nebulas->set_nebulaname_format(0);
			*/
		}

/*
            if (!core->nebulas->get_flag_hints())
            {
               core->commander->execute_command( "flag nebula_names on");
            }
            else if (!core->FlagNebulaLongName)
            {
                 core->commander->execute_command( "flag nebula_long_names on");
            }
            else
            {
                 core->commander->execute_command( "flag nebula_names off");
                 core->commander->execute_command( "flag nebula_long_names off");
            }
*/
		if(key==SDLK_g) core->commander->execute_command( "flag landscape toggle");
		if(key==SDLK_f) core->commander->execute_command( "flag fog toggle");
		if(key==SDLK_q) core->commander->execute_command( "flag cardinal_points toggle");
		if(key==SDLK_a) core->commander->execute_command( "flag atmosphere toggle");

		if(key==SDLK_h)
		{
			core->FlagHelp=!core->FlagHelp;
			help_win->setVisible(core->FlagHelp);
		}
		if(key==SDLK_COMMA || key==SDLK_4)
		{
			if(!core->FlagEclipticLine)
			{
				core->commander->execute_command( "flag ecliptic_line on");
			}
			else if( !core->FlagObjectTrails)
			{
				core->commander->execute_command( "flag object_trails on");
				core->ssystem->start_trails();
			}
			else
			{
				core->commander->execute_command( "flag object_trails off");
				core->ssystem->end_trails();
				core->commander->execute_command( "flag ecliptic_line off");
			}
		}
		if(key==SDLK_PERIOD || key==SDLK_5) core->commander->execute_command( "flag equator_line toggle");

		if(key==SDLK_t)
		{
			core->navigation->set_flag_lock_equ_pos(!core->navigation->get_flag_lock_equ_pos());
		}
		if(key==SDLK_s && !(SDL_GetModState() & KMOD_CTRL))
			core->commander->execute_command( "flag stars toggle");

		if(key==SDLK_SPACE) core->commander->execute_command("flag track_object on");

		if(key==SDLK_i)
		{
			core->FlagInfos=!core->FlagInfos;
			licence_win->setVisible(core->FlagInfos);
		}
		if(key==SDLK_EQUALS) core->commander->execute_command( "date relative 1");
		if(key==SDLK_MINUS) core->commander->execute_command( "date relative -1");

		if(key==SDLK_m && core->FlagEnableTuiMenu) core->FlagShowTuiMenu = true;  // not recorded

		if(key==SDLK_o) core->commander->execute_command( "flag moon_scaled toggle");

		if(key==SDLK_9)
		{
			int zhr = core->meteors->get_ZHR();

			if(zhr <= 10 )
			{
				core->commander->execute_command("meteors zhr 80");  // standard Perseids rate
			}
			else if( zhr <= 80 )
			{
				core->commander->execute_command("meteors zhr 10000"); // exceptional Leonid rate
			}
			else if( zhr <= 10000 )
			{
				core->commander->execute_command("meteors zhr 144000");  // highest ever recorded ZHR (1966 Leonids)
			}
			else
			{
				core->commander->execute_command("meteors zhr 10");  // set to default base rate (10 is normal, 0 would be none)
			}
		}

		if(key==SDLK_LEFTBRACKET) core->commander->execute_command( "date relative -7");
		if(key==SDLK_RIGHTBRACKET) core->commander->execute_command( "date relative 7");
		if(key==SDLK_SLASH)
		{
			if (SDL_GetModState() & KMOD_CTRL)  core->commander->execute_command( "zoom auto out");
			else {
				// here we help script recorders by selecting the right type of zoom option
				// based on current settings of manual or full auto zoom
				if(core->FlagManualZoom) core->commander->execute_command( "zoom auto in manual 1");
				else core->commander->execute_command( "zoom auto in");
			}
		}
		if(key==SDLK_BACKSLASH) core->commander->execute_command( "zoom auto out");
		if(key==SDLK_x)
		{
			core->commander->execute_command( "flag show_tui_datetime toggle");

			// keep these in sync.  Maybe this should just be one flag.
			if(core->FlagShowTuiDateTime) core->commander->execute_command( "flag show_tui_short_obj_info on");
			else core->commander->execute_command( "flag show_tui_short_obj_info off");
		}
		if(key==SDLK_RETURN)
		{
			core->navigation->switch_viewing_mode();
		}
	}
	return 0;
}


// Update changing values
void StelUI::gui_update_widgets(int delta_time)
{
	updateTopBar();

	// handle mouse cursor timeout
	if(core->MouseCursorTimeout > 0) {
		if(MouseTimeLeft > delta_time) MouseTimeLeft -= delta_time;
		else {
			// hide cursor
			MouseTimeLeft = 0;
			SDL_ShowCursor(0);
		}
	}

	// update message win
	message_win->update(delta_time);

/*
	// To prevent a minor bug
	if (!core->FlagShowSelectedObjectInfo || !core->selected_object) 
		info_select_ctr->setVisible(0);
	else 
	if (core->selected_object)
	{
		info_select_ctr->setVisible(1);
		if(core->FlagShowSelectedObjectInfo) updateInfoSelectString();
	}
*/
	// TONY
	if (core->FlagShowSelectedObjectInfo && core->selected_object) 
		updateInfoSelectString();

	bt_flag_ctr->setVisible(core->FlagMenu);
	bt_time_control_ctr->setVisible(core->FlagMenu);

	bt_flag_constellation_draw->setState(core->constellation_get_flag_lines());
	bt_flag_constellation_name->setState(core->constellation_get_flag_names());
	bt_flag_constellation_art->setState(core->constellation_get_flag_art());
	bt_flag_azimuth_grid->setState(core->FlagAzimutalGrid);
	bt_flag_equator_grid->setState(core->FlagEquatorialGrid);
	bt_flag_ground->setState(core->FlagLandscape);
	bt_flag_cardinals->setState(core->cardinals_points->get_flag_show());
	bt_flag_atmosphere->setState(core->FlagAtmosphere);
	bt_flag_nebula_name->setState(core->nebulas->get_flag_hints());
	bt_flag_help->setState(help_win->getVisible());
	bt_flag_equatorial_mode->setState(core->navigation->get_viewing_mode()==Navigator::VIEW_EQUATOR);
	bt_flag_config->setState(config_win->getVisible());
	bt_flag_chart->setState(core->FlagChart);
	bt_flag_night->setState(core->FlagNight);
	bt_flag_search->setState(search_win->getVisible()); 
	bt_flag_goto->setState(false); 

	if (config_win->getVisible()) updateConfigForm();
}

// Update the infos about the selected object in the TextLabel widget
void StelUI::updateInfoSelectString(void)
{
//	if (core->FlagShowSelectedObjectInfo)
//	{
//		info_select_ctr->setVisible(1);
	if (draw_mode == DM_NORMAL)
	{
		if (core->selected_object->get_type()==StelObject::STEL_OBJECT_NEBULA)
			info_select_txtlbl->setTextColor(core->NebulaLabelColor[draw_mode]);
		else if (core->selected_object->get_type()==StelObject::STEL_OBJECT_PLANET)
			info_select_txtlbl->setTextColor(core->PlanetNamesColor[draw_mode]);
		else if (core->selected_object->get_type()==StelObject::STEL_OBJECT_STAR)
			info_select_txtlbl->setTextColor(core->selected_object->get_RGB());
	}
	else
		info_select_txtlbl->setTextColor(Vec3f(1.0,0.2,0.2));
	
	info_select_txtlbl->setLabel(core->selected_object->get_info_string(core->navigation));
//	}
}

void StelUI::setTitleObservatoryName(const string& name)
{
	if (name == "")
		top_bar_appName_lbl->setLabel(APP_NAME);
	else
	{
		std::ostringstream oss;
		oss << APP_NAME << " (" << name << ")";
		top_bar_appName_lbl->setLabel(oss.str());
	}
	top_bar_appName_lbl->setPos(core->screen_W/2-top_bar_appName_lbl->getSizex()/2,2);
}

string StelUI::getTitleWithAltitude(void)
{
	std::ostringstream oss;
	oss << core->observatory->get_name() << " @ " << core->observatory->get_altitude() << "m";

	return oss.str();
}
