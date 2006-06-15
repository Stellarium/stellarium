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
#include <iomanip>
#include "stel_ui.h"
#include "stellastro.h"
#include "stelapp.h"

////////////////////////////////////////////////////////////////////////////////
//								CLASS FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

StelUI::StelUI(StelCore * _core, StelApp * _app) :
		baseFont(NULL),
		courierFont(NULL),
		tuiFont(NULL),

		FlagHelp(false), FlagInfos(false), FlagConfig(false), FlagSearch(false), FlagShowTuiMenu(0),

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
//		bt_flag_chart(NULL),
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
	app = _app;
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
	delete tuiFont; tuiFont = NULL;
	delete tex_up; tex_up = NULL;
	delete tex_down; tex_down = NULL;
	if (tui_root) delete tui_root; tui_root=NULL;
	Component::deleteScissor();
}

////////////////////////////////////////////////////////////////////////////////
void StelUI::init(const InitParser& conf)
{

	if(initialised) {

		// delete existing objects before recreating
		if(baseFont) delete baseFont;
		if(courierFont) delete courierFont;
		if(baseTex) delete baseTex;
		if(flipBaseTex) delete flipBaseTex;
		if(tex_up) delete tex_up;
		if(tex_down) delete tex_down;
		if(desktop) delete desktop;
	}

	// Ui section
	FlagShowFps			= conf.get_boolean("gui:flag_show_fps");
	FlagMenu			= conf.get_boolean("gui:flag_menu");
	FlagHelp			= conf.get_boolean("gui:flag_help");
	FlagInfos			= conf.get_boolean("gui:flag_infos");
	FlagShowTopBar		= conf.get_boolean("gui:flag_show_topbar");
	FlagShowTime		= conf.get_boolean("gui:flag_show_time");
	FlagShowDate		= conf.get_boolean("gui:flag_show_date");
	FlagShowAppName		= conf.get_boolean("gui:flag_show_appname");
	FlagShowFov			= conf.get_boolean("gui:flag_show_fov");
	FlagShowSelectedObjectInfo = conf.get_boolean("gui:flag_show_selected_object_info");
	BaseFontSize		= conf.get_double ("gui","base_font_size",15);
	BaseFontName        = conf.get_str("gui", "base_font_name", "DejaVuSans.ttf");
	FlagShowScriptBar	= conf.get_boolean("gui","flag_show_script_bar",false);
	MouseCursorTimeout  = conf.get_double("gui","mouse_cursor_timeout",0);

	// Text ui section
	FlagEnableTuiMenu = conf.get_boolean("tui:flag_enable_tui_menu");
	FlagShowGravityUi = conf.get_boolean("tui:flag_show_gravity_ui");
	FlagShowTuiDateTime = conf.get_boolean("tui:flag_show_tui_datetime");
	FlagShowTuiShortObjInfo = conf.get_boolean("tui:flag_show_tui_short_obj_info");

	BaseFontName = core->getDataDir() + BaseFontName;

	// TODO: can we get rid of this second font requirement?
	BaseCFontSize		= conf.get_double ("gui","base_cfont_size",12.5);
	BaseCFontName = core->getDataDir() + conf.get_str("gui", "base_cfont_name", "DejaVuSansMono.ttf");

	// Load standard font
	baseFont = new s_font(BaseFontSize, BaseFontName);
	if (!baseFont)
	{
		printf("ERROR WHILE CREATING FONT\n");
		exit(-1);
	}

	courierFont = new s_font(BaseCFontSize, BaseCFontName);
	if (!courierFont)
	{
		printf("ERROR WHILE CREATING FONT\n");
		exit(-1);
	}

	// set up mouse cursor timeout
	MouseTimeLeft = MouseCursorTimeout*1000;

	// Create standard texture
	baseTex = new s_texture("backmenu.png", TEX_LOAD_TYPE_PNG_ALPHA);
	flipBaseTex = new s_texture("backmenu_flip.png", TEX_LOAD_TYPE_PNG_ALPHA);

	tex_up = new s_texture("up.png");
	tex_down = new s_texture("down.png");

	// Set default Painter
	Painter p(baseTex, baseFont, s_color(0.5, 0.5, 0.5), s_color(1., 1., 1.));
	Component::setDefaultPainter(p);

	Component::initScissor(core->getViewportWidth(), core->getViewportHeight());

	desktop = new Container(true);
	desktop->reshape(0,0,core->getViewportWidth(),core->getViewportHeight());

	bt_flag_help_lbl = new Label(L"ERROR...");
	bt_flag_help_lbl->setPos(3,core->getViewportHeight()-41-(int)baseFont->getDescent());
	bt_flag_help_lbl->setVisible(0);

	bt_flag_time_control_lbl = new Label(L"ERROR...");
	bt_flag_time_control_lbl->setPos(core->getViewportWidth()-180,core->getViewportHeight()-41-(int)baseFont->getDescent());
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
	message_win = new StdTransBtWin(L"Message", 5000);
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

	dialog_win = new StdDlgWin(L"Stellarium");
	dialog_win->setOpaque(opaqueGUI);
	dialog_win->setDialogCallback(callback<void>(this, &StelUI::dialogCallback));
	desktop->addComponent(dialog_win);

	desktop->addComponent(createLicenceWindow());
	desktop->addComponent(createHelpWindow());
	desktop->addComponent(createConfigWindow());
	desktop->addComponent(createSearchWindow());

	initialised = true;

	setTitleObservatoryName(getTitleWithAltitude());
}


////////////////////////////////////////////////////////////////////////////////
void StelUI::show_message(wstring _message, int _time_out)
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
Component* StelUI::createTopBar(void)
{
	top_bar_date_lbl = new Label(L"-", baseFont);	top_bar_date_lbl->setPos(2,1);
	top_bar_hour_lbl = new Label(L"-", baseFont);	top_bar_hour_lbl->setPos(110,1);
	top_bar_fps_lbl = new Label(L"-", baseFont);	top_bar_fps_lbl->setPos(core->getViewportWidth()-100,1);
	top_bar_fov_lbl = new Label(L"-", baseFont);	top_bar_fov_lbl->setPos(core->getViewportWidth()-220,1);
	top_bar_appName_lbl = new Label(StelUtility::stringToWstring(APP_NAME), baseFont);
	top_bar_appName_lbl->setPos(core->getViewportWidth()/2-top_bar_appName_lbl->getSizex()/2,1);
	top_bar_ctr = new FilledContainer();
	top_bar_ctr->reshape(0,0,core->getViewportWidth(),(int)(baseFont->getLineHeight()+0.5)+5);
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
	top_bar_ctr->setVisible(FlagShowTopBar);
	if (!FlagShowTopBar) return;

	double jd = core->getJDay();

	if (FlagShowDate)
	{
		top_bar_date_lbl->setLabel(app->get_printable_date_local(jd));
		top_bar_date_lbl->adjustSize();
	}
	top_bar_date_lbl->setVisible(FlagShowDate);

	if (FlagShowTime)
	{
		top_bar_hour_lbl->setLabel(app->get_printable_time_local(jd));
		top_bar_hour_lbl->adjustSize();
	}
	top_bar_hour_lbl->setVisible(FlagShowTime);

	top_bar_appName_lbl->setVisible(FlagShowAppName);

	if (FlagShowFov)
	{
		wstringstream wos;
		wos << L"FOV=" << setprecision(3) << core->getFov() << L"\u00B0";
		top_bar_fov_lbl->setLabel(wos.str());
		top_bar_fov_lbl->adjustSize();
	}
	top_bar_fov_lbl->setVisible(FlagShowFov);

	if (FlagShowFps)
	{
		wstringstream wos;
		wos << L"FPS=" << setprecision(4) << app->fps;
		top_bar_fps_lbl->setLabel(wos.str());
		top_bar_fps_lbl->adjustSize();
	}
	top_bar_fps_lbl->setVisible(FlagShowFps);
}

// Create the button panel in the lower left corner
#define UI_PADDING 5
#define UI_BT 25
#define UI_SCRIPT_BAR 300
Component* StelUI::createFlagButtons(void)
{
	int x = 0;

	bt_flag_constellation_draw = new FlagButton(false, NULL, "bt_constellations.png");
	bt_flag_constellation_draw->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_constellation_draw->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_constellation_name = new FlagButton(false, NULL, "bt_const_names.png");
	bt_flag_constellation_name->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_constellation_name->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_constellation_art = new FlagButton(false, NULL, "bt_constart.png");
	bt_flag_constellation_art->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_constellation_art->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_azimuth_grid = new FlagButton(false, NULL, "bt_azgrid.png");
	bt_flag_azimuth_grid->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_azimuth_grid->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_equator_grid = new FlagButton(false, NULL, "bt_eqgrid.png");
	bt_flag_equator_grid->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_equator_grid->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_ground = new FlagButton(false, NULL, "bt_ground.png");
	bt_flag_ground->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_ground->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_cardinals = new FlagButton(false, NULL, "bt_cardinal.png");
	bt_flag_cardinals->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_cardinals->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_atmosphere = new FlagButton(false, NULL, "bt_atmosphere.png");
	bt_flag_atmosphere->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_atmosphere->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_nebula_name = new FlagButton(false, NULL, "bt_nebula.png");
	bt_flag_nebula_name->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_nebula_name->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_help = new FlagButton(false, NULL, "bt_help.png");
	bt_flag_help->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_help->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_equatorial_mode = new FlagButton(false, NULL, "bt_follow.png");
	bt_flag_equatorial_mode->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_equatorial_mode->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_config = new FlagButton(false, NULL, "bt_config.png");
	bt_flag_config->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_config->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));



	bt_flag_night = new FlagButton(false, NULL, "bt_nightview.png");
	bt_flag_night->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_night->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_quit = new FlagButton(true, NULL, "bt_quit.png");
	bt_flag_quit->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_quit->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_search = new FlagButton(true, NULL, "bt_search.png");
	bt_flag_search->setOnPressCallback(callback<void>(this, &StelUI::cb));
	bt_flag_search->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_script = new EditBox();
	bt_script->setAutoFocus(false);
	bt_script->setSize(299,24);
	bt_script->setOnKeyCallback(callback<void>(this, &StelUI::cbEditScriptKey));
	bt_script->setOnReturnKeyCallback(callback<void>(this, &StelUI::cbEditScriptExecute));
	bt_script->setOnMouseInOutCallback(callback<void>(this, &StelUI::cbr));

	bt_flag_goto = new FlagButton(true, NULL, "bt_goto.png");
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
	if (!FlagShowScriptBar)
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
//	bt_flag_ctr->addComponent(bt_flag_chart);			bt_flag_chart->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_night);			bt_flag_night->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_help);			bt_flag_help->setPos(x,0); x+=UI_BT;
	bt_flag_ctr->addComponent(bt_flag_quit);			bt_flag_quit->setPos(x,0); x+=UI_BT;

	bt_flag_ctr->setOnMouseInOutCallback(callback<void>(this, &StelUI::bt_flag_ctrOnMouseInOut));
	bt_flag_ctr->reshape(0, core->getViewportHeight()-25, x-1, 25);

	return bt_flag_ctr;

}

// Create the button panel in the lower right corner
Component* StelUI::createTimeControlButtons(void)
{
	bt_dec_time_speed = new FlagButton(false, NULL, "bt_rwd.png");
	bt_dec_time_speed->setOnPressCallback(callback<void>(this, &StelUI::bt_dec_time_speed_cb));
	bt_dec_time_speed->setOnMouseInOutCallback(callback<void>(this, &StelUI::tcbr));

	bt_real_time_speed = new FlagButton(false, NULL, "bt_realtime.png");
	bt_real_time_speed->setSize(24,24);
	bt_real_time_speed->setOnPressCallback(callback<void>(this, &StelUI::bt_real_time_speed_cb));
	bt_real_time_speed->setOnMouseInOutCallback(callback<void>(this, &StelUI::tcbr));

	bt_inc_time_speed = new FlagButton(false, NULL, "bt_fwd.png");
	bt_inc_time_speed->setOnPressCallback(callback<void>(this, &StelUI::bt_inc_time_speed_cb));
	bt_inc_time_speed->setOnMouseInOutCallback(callback<void>(this, &StelUI::tcbr));

	bt_time_now = new FlagButton(false, NULL, "bt_now.png");
	bt_time_now->setOnPressCallback(callback<void>(this, &StelUI::bt_time_now_cb));
	bt_time_now->setOnMouseInOutCallback(callback<void>(this, &StelUI::tcbr));

	bt_time_control_ctr = new FilledContainer();
	bt_time_control_ctr->addComponent(bt_dec_time_speed);	bt_dec_time_speed->setPos(0,0);
	bt_time_control_ctr->addComponent(bt_real_time_speed);	bt_real_time_speed->setPos(25,0);
	bt_time_control_ctr->addComponent(bt_inc_time_speed);	bt_inc_time_speed->setPos(50,0);
	bt_time_control_ctr->addComponent(bt_time_now);			bt_time_now->setPos(75,0);

	bt_time_control_ctr->setOnMouseInOutCallback(callback<void>(this, &StelUI::bt_time_control_ctrOnMouseInOut));
	bt_time_control_ctr->reshape(core->getViewportWidth()-4*25-1, core->getViewportHeight()-25, 4*25, 25);

	return bt_time_control_ctr;
}

void StelUI::bt_dec_time_speed_cb(void)
{
	double s = core->getTimeSpeed();
	if (s>JD_SECOND) s/=10.;
	else if (s<=-JD_SECOND) s*=10.;
	else if (s>-JD_SECOND && s<=0.) s=-JD_SECOND;
	else if (s>0. && s<=JD_SECOND) s=0.;
	core->setTimeSpeed(s);
}

void StelUI::bt_inc_time_speed_cb(void)
{
	double s = core->getTimeSpeed();
	if (s>=JD_SECOND) s*=10.;
	else if (s<-JD_SECOND) s/=10.;
	else if (s>=0. && s<JD_SECOND) s=JD_SECOND;
	else if (s>=-JD_SECOND && s<0.) s=0.;
	core->setTimeSpeed(s);
}

void StelUI::bt_real_time_speed_cb(void)
{
	core->setTimeSpeed(JD_SECOND);
}

void StelUI::bt_time_now_cb(void)
{
	core->setJDay(get_julian_from_sys());
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
		wstring command = bt_script->getText();
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
	string command_string = StelUtility::wstringToString(bt_script->getText());
	cout << "Executing command: " << command_string << endl;

	bt_script->clearText();
	bt_script->setEditing(false);

	if (!app->commander->execute_command(command_string))
		bt_flag_help_lbl->setLabel(_("Invalid Script command"));
}

////////////////////////////////////////////////////////////////////////////////
void StelUI::cb(void)
{
	core->setFlagConstellationLines(bt_flag_constellation_draw->getState());
	core->setFlagConstellationNames(bt_flag_constellation_name->getState());
	core->setFlagConstellationArt(bt_flag_constellation_art->getState());
	core->setFlagAzimutalGrid(bt_flag_azimuth_grid->getState());
	core->setFlagEquatorGrid(bt_flag_equator_grid->getState());
	core->setFlagLandscape(bt_flag_ground->getState());
	core->setFlagCardinalsPoints(bt_flag_cardinals->getState());
	core->setFlagAtmosphere(bt_flag_atmosphere->getState());
	core->setFlagNebulaHints( bt_flag_nebula_name->getState() );
	FlagHelp 				= bt_flag_help->getState();
	help_win->setVisible(FlagHelp);
	core->setMountMode(bt_flag_equatorial_mode->getState() ? StelCore::MOUNT_EQUATORIAL : StelCore::MOUNT_ALTAZIMUTAL);
	FlagConfig			= bt_flag_config->getState();
// 	if  (app->getVisionModeChart() != bt_flag_chart->getState())
// 	{
// 		if (bt_flag_night->getState())
// 		{
// 			app->setVisionModeChart();
// 		}
// 		else
// 		{
// 			app->setVisionModeNormal();
// 		}
// 	}
	if  (app->getVisionModeNight() != bt_flag_night->getState())
	{
		if (bt_flag_night->getState())
		{
			app->setVisionModeNight();
		}
		else
		{
			app->setVisionModeNormal();
		}
	}
	config_win->setVisible(FlagConfig);

	FlagSearch			= bt_flag_search->getState();
	search_win->setVisible(FlagSearch);
	if (bt_flag_goto->getState()) core->gotoSelectedObject();
	bt_flag_goto->setState(false);

	if (!bt_flag_quit->getState()) app->quit();
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
wstring(L"                 \u2022   " APP_NAME "  May 2006  \u2022\n\n") +
L"\u2022   Copyright (c) 2000-2005 Fabien Chereau et al.\n\n" +
L"\u2022" + _("   Please check for newer versions and send bug reports\n\
    and comments to us at: http://www.stellarium.org\n\n") +
L"\u2022   This program is free software; you can redistribute it and/or\n\
modify it under the terms of the GNU General Public License\n\
as published by the Free Software Foundation; either version 2\n\
of the License, or (at your option) any later version.\n\n" +
L"This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; without even the implied\n\
warranty of MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.  See the GNU General Public\n\
License for more details.\n\n" +
L"You should have received a copy of the GNU General Public\n\
License along with this program; if not, write to:\n" +
L"Free Software Foundation, Inc.\n\
59 Temple Place - Suite 330\n\
Boston, MA  02111-1307, USA.\n\
http://www.fsf.org");
	licence_txtlbl->adjustSize();
	licence_txtlbl->setPos(10,10);
	licence_win = new StdBtWin(_("Information"));
	licence_win->setOpaque(opaqueGUI);
	licence_win->reshape(275,175,435,400);
	licence_win->addComponent(licence_txtlbl);
	licence_win->setVisible(FlagInfos);

	return licence_win;
}

Component* StelUI::createHelpWindow(void)
{
	help_txtlbl = new TextLabel(
wstring(_("Movement & selection:\n\
Arrow Keys       : Change viewing RA/DE\n\
Page Up/Down     : Zoom\n\
CTRL+Up/Down     : Zoom\n\
Left Click       : Select object\n\
Right Click      : Unselect\n\
CTRL+Left Click  : Unselect\n\
\\                : Zoom out (planet + moons if applicable)\n\
/                : Zoom to selected object\n\
SPACE            : Center on selected object\n\
\n"))+
wstring(_("Display options:\n\
ENTER : Equatorial/altazimuthal mount\n\
F1  : Toggle fullscreen if possible.\n\
C   : Constellation lines       V   : Constellation labels\n\
R   : Constellation art         E   : Equatorial grid\n\
Z   : Azimuthal grid            N   : Nebula labels\n\
P   : Planet labels             G   : Ground\n\
A   : Atmosphere                F   : Fog\n\
Q   : Cardinal points           O   : Toggle moon scaling\n\
T   : Object tracking           S   : Stars\n\
4 , : Ecliptic line             5 . : Equator line\n\
\n")) +
wstring(_("Dialogs & other controls:\n\
H   : Help                      I   : About Stellarium\n\
M   : Text menu                 1 (one)  : Configuration\n\
CTRL + S : Take a screenshot\n\
CTRL + R : Toggle script recording\n\
CTRL + F : Toggle object finder\n\
\n")) + 
wstring(_("Time & Date:\n\
6   : Time rate pause           7   : Time rate 0\n\
8   : Set current time          J   : Decrease time rate\n\
K   : Normal time rate          L   : Increase time rate\n\
-   : Back 24 hours             =   : Forward 24 hours\n\
[   : Back 7 days               ]   : Forward 7 days\n\
\n")) + 
wstring(_("During Script Playback:\n\
CTRL + C : End Script\n\
6   : pause script              K   : resume script\n\
\n")) + 
wstring(_("Misc:\n\
9   : Toggle meteor shower rates\n")) + wstring(
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
	help_win->setVisible(FlagHelp);
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

	// draw first as windows should cover these up
	// also problem after 2dfullscreen with square viewport
	if (FlagShowGravityUi) draw_gravity_ui();
	if (FlagShowTuiMenu) draw_tui();

	// Special cool text transparency mode
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_BLEND);

	app->set2DfullscreenProjection();	// 2D coordinate
	Component::enableScissor();

	glScalef(1, -1, 1);						// invert the y axis, down is positive
	glTranslatef(0, -core->getViewportHeight(), 0);	// move the origin from the bottom left corner to the upper left corner

	desktop->draw();

	Component::disableScissor();
	app->restoreFrom2DfullscreenProjection();	// Restore the other coordinate

}

/*******************************************************************************/
int StelUI::handle_move(int x, int y)
{
	// Do not allow use of mouse while script is playing
	// otherwise script can get confused
	if(app->scripts->is_playing()) return 0;

	// Show cursor
	SDL_ShowCursor(1);
	MouseTimeLeft = MouseCursorTimeout*1000;

	if (desktop->onMove(x, y)) return 1;
	if (is_dragging)
	{
		if ((has_dragged || sqrtf((x-previous_x)*(x-previous_x)+(y-previous_y)*(y-previous_y))>4.))
		{
			has_dragged = true;
			core->setFlagTracking(false);
			core->dragView(previous_x, previous_y, x, y);
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
	if(app->scripts->is_playing()) return 0;

	// Make sure object pointer is turned on (script may have turned off)
	core->setFlagSelectedObjectPointer(true);

	// Show cursor
	SDL_ShowCursor(1);
	MouseTimeLeft = MouseCursorTimeout*1000;

	if (desktop->onClic((int)x, (int)y, button, state))
	{
		has_dragged = false;
		is_dragging = false;
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
		core->zoomTo(core->getAimFov()-app->MouseZoom*core->getAimFov()/60., 0.2);
		return 1;
	case S_GUI_MOUSE_WHEELDOWN :
		core->zoomTo(core->getAimFov()+app->MouseZoom*core->getAimFov()/60., 0.2);
		return 1;
	default: break;
	}

	// Manage the event for the main window
	{
		//if (state==S_GUI_PRESSED) return 1;
		// Deselect the selected object
		if (button==S_GUI_MOUSE_RIGHT && state==S_GUI_RELEASED)
		{
			app->commander->execute_command("select");
			return 1;
		}
		if (button==S_GUI_MOUSE_MIDDLE && state==S_GUI_RELEASED)
		{
			if (core->getFlagHasSelected())
			{
				core->gotoSelectedObject();
				core->setFlagTracking(true);
			}
		}
		if (button==S_GUI_MOUSE_LEFT && state==S_GUI_RELEASED && !has_dragged)
		{
			// CTRL + left clic = right clic for 1 button mouse
			if (SDL_GetModState() & KMOD_CTRL)
			{
				app->commander->execute_command("select");
				return 1;
			}

			// Try to select object at that position
			core->findAndSelect(x, y);

			// If an object was selected update informations
			if (core->getFlagHasSelected()) updateInfoSelectString();
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

		if (SDL_GetModState() & KMOD_CTRL) { // Ctrl
			switch (key) {
				case SDLK_0:core->telescopeGoto(0);return 0;
				case SDLK_1:core->telescopeGoto(1);return 0;
				case SDLK_2:core->telescopeGoto(2);return 0;
				case SDLK_3:core->telescopeGoto(3);return 0;
				case SDLK_4:core->telescopeGoto(4);return 0;
				case SDLK_5:core->telescopeGoto(5);return 0;
				case SDLK_6:core->telescopeGoto(6);return 0;
				case SDLK_7:core->telescopeGoto(7);return 0;
				case SDLK_8:core->telescopeGoto(8);return 0;
				case SDLK_9:core->telescopeGoto(9);return 0;
			}
		}

#ifndef MACOSX
		if(key==0x0011)
		{ // CTRL-Q
#else
		if (key == SDLK_q && SDL_GetModState() & KMOD_META)
		{
#endif
			app->quit();
		}


		// if script is running, only script control keys are accessible
		// to pause/resume/cancel the script
		// (otherwise script could get very confused by user interaction)
		if(app->scripts->is_playing())
		{

			// here reusing time control keys to control the script playback
			if(key==SDLK_6)
			{
				// pause/unpause script
				app->commander->execute_command( "script action pause");
				app->time_multiplier = 1;  // don't allow resumption of ffwd this way (confusing for audio)
			}
			else if(key==SDLK_k)
			{
				app->commander->execute_command( "script action resume");
				app->time_multiplier = 1;
			}
			else if(key==SDLK_7 || key==0x0003 || (key==SDLK_m && FlagEnableTuiMenu))
			{  // ctrl-c
				// TODO: should double check with user here...
				app->commander->execute_command( "script action end");
				if(key==SDLK_m) FlagShowTuiMenu = true;
			}
			// TODO n is bad key if ui allowed
			else if(key==SDLK_GREATER || key==SDLK_n)
			{
				app->commander->execute_command( "audio volume increment");
			}
			// TODO d is bad key if ui allowed
			else if(key==SDLK_LESS || key==SDLK_d)
			{
				app->commander->execute_command( "audio volume decrement");

			}
			else if(key==SDLK_j)
			{
				if(app->time_multiplier==2)
				{
					app->time_multiplier = 1;

					// restart audio in correct place
					app->commander->execute_command( "audio action sync");
				}
				else if(app->time_multiplier > 1 )
				{
					app->time_multiplier /= 2;
				}

			}
			else if(key==SDLK_l)
			{
				// stop audio since won't play at higher speeds
				app->commander->execute_command( "audio action pause");
				app->time_multiplier *= 2;
				if(app->time_multiplier>8) app->time_multiplier = 8;
			}
			else if(!app->scripts->get_allow_ui())
			{
				cout << "Playing a script.  Press CTRL-C (or 7) to stop." << endl;
			}

			if(!app->scripts->get_allow_ui()) return 0;  // only limited user interaction allowed with script

		}
		else
		{
			app->time_multiplier = 1;  // if no script in progress always real time

			// normal time controls here (taken over for script control above if playing a script)
			if(key==SDLK_k) app->commander->execute_command( "timerate rate 1");
			if(key==SDLK_l) app->commander->execute_command( "timerate action increment");
			if(key==SDLK_j) app->commander->execute_command( "timerate action decrement");
			if(key==SDLK_6) app->commander->execute_command( "timerate action pause");
			if(key==SDLK_7) app->commander->execute_command( "timerate rate 0");
			if(key==SDLK_8) app->commander->execute_command( "date load preset");

		}

#ifndef MACOSX
		if(key==0x0012)
		{ // CTRL-R
#else
		if (key == SDLK_r && SDL_GetModState() & KMOD_META)
		{
#endif
			if(app->scripts->is_recording())
			{
				app->commander->execute_command( "script action cancelrecord");
				show_message(_("Command recording stopped."), 3000);
			}
			else
			{
				app->commander->execute_command( "script action record");

				if(app->scripts->is_recording())
				{
					show_message(wstring( _("Recording commands to script file:\n")
					                      + StelUtility::stringToWstring(app->scripts->get_record_filename()) + L"\n\n"
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
			FlagSearch=false;
			search_win->setVisible(FlagSearch);

			// close config dialog
			FlagConfig = false;
			config_win->setVisible(FlagConfig);

			// close help dialog
			FlagHelp = false;
			help_win->setVisible(FlagHelp);

			// close information dialog
			FlagInfos = false;
			licence_win->setVisible(FlagInfos);
		}
		// END RFE 1310384

#ifndef MACOSX
		if(key==0x0006)
		{ // CTRL-f
#else
		if (key == SDLK_f && SDL_GetModState() & KMOD_META)
		{
#endif
			FlagSearch = !FlagSearch;
			search_win->setVisible(FlagSearch);
		}

		if(key==SDLK_r) app->commander->execute_command( "flag constellation_art toggle");


		if(key=='c') app->commander->execute_command( "flag constellation_drawing toggle");
		if(key=='b') app->commander->execute_command( "flag constellation_boundaries toggle");

		if(key=='d') app->commander->execute_command( "flag star_names toggle");

		if(key==SDLK_1)
		{
			FlagConfig=!FlagConfig;
			config_win->setVisible(FlagConfig);
		}
		if(key==SDLK_p)
		{
			if(!core->getFlagPlanetsHints())
			{
				app->commander->execute_command("flag planet_names on");
			}
			else if( !core->getFlagPlanetsOrbits())
			{
				app->commander->execute_command("flag planet_orbits on");
			}
			else
			{
				app->commander->execute_command("flag planet_orbits off");
				app->commander->execute_command("flag planet_names off");
			}
		}
		if(key==SDLK_v) app->commander->execute_command( "flag constellation_names toggle");
		if(key==SDLK_z)
		{
			if(!core->getFlagMeridianLine())
			{
				if(core->getFlagAzimutalGrid()) app->commander->execute_command( "flag azimuthal_grid 0");
				else app->commander->execute_command( "flag meridian_line 1");
			}
			else
			{
				app->commander->execute_command( "flag meridian_line 0");
				app->commander->execute_command( "flag azimuthal_grid 1");
			}
		}
		if(key==SDLK_e) app->commander->execute_command( "flag equatorial_grid toggle");

		if(key==SDLK_n) app->commander->execute_command( "flag nebula_names toggle");

		if(key==SDLK_g) app->commander->execute_command( "flag landscape toggle");
		if(key==SDLK_f) app->commander->execute_command( "flag fog toggle");
		if(key==SDLK_q) app->commander->execute_command( "flag cardinal_points toggle");
		if(key==SDLK_a) app->commander->execute_command( "flag atmosphere toggle");

		if(key==SDLK_h)
		{
			FlagHelp=!FlagHelp;
			help_win->setVisible(FlagHelp);
		}
		if(key==SDLK_COMMA || key==SDLK_4)
		{
			if(!core->getFlagEclipticLine())
			{
				app->commander->execute_command( "flag ecliptic_line on");
			}
			else if( !core->getFlagPlanetsTrails())
			{
				app->commander->execute_command( "flag object_trails on");
				core->startPlanetsTrails(true);
			}
			else
			{
				app->commander->execute_command( "flag object_trails off");
				core->startPlanetsTrails(false);
				app->commander->execute_command( "flag ecliptic_line off");
			}
		}
		if(key==SDLK_PERIOD || key==SDLK_5) app->commander->execute_command( "flag equator_line toggle");

		if(key==SDLK_t)
		{
			core->setFlagLockSkyPosition(!core->getFlagLockSkyPosition());
		}
		if(key==SDLK_s && !(SDL_GetModState() & KMOD_CTRL))
			app->commander->execute_command( "flag stars toggle");

		if(key==SDLK_SPACE) app->commander->execute_command("flag track_object on");

		if(key==SDLK_i)
		{
			FlagInfos=!FlagInfos;
			licence_win->setVisible(FlagInfos);
		}
		if(key==SDLK_EQUALS) app->commander->execute_command( "date relative 1");
		if(key==SDLK_MINUS) app->commander->execute_command( "date relative -1");

		if(key==SDLK_m && FlagEnableTuiMenu) FlagShowTuiMenu = true;  // not recorded

		if(key==SDLK_o) app->commander->execute_command( "flag moon_scaled toggle");

		if(key==SDLK_9)
		{
			int zhr = core->getMeteorsRate();

			if(zhr <= 10 )
			{
				app->commander->execute_command("meteors zhr 80");  // standard Perseids rate
			}
			else if( zhr <= 80 )
			{
				app->commander->execute_command("meteors zhr 10000"); // exceptional Leonid rate
			}
			else if( zhr <= 10000 )
			{
				app->commander->execute_command("meteors zhr 144000");  // highest ever recorded ZHR (1966 Leonids)
			}
			else
			{
				app->commander->execute_command("meteors zhr 10");  // set to default base rate (10 is normal, 0 would be none)
			}
		}

		if(key==SDLK_LEFTBRACKET) app->commander->execute_command( "date relative -7");
		if(key==SDLK_RIGHTBRACKET) app->commander->execute_command( "date relative 7");
		if(key==SDLK_SLASH)
		{
			if (SDL_GetModState() & KMOD_CTRL)  app->commander->execute_command( "zoom auto out");
			else
			{
				// here we help script recorders by selecting the right type of zoom option
				// based on current settings of manual or full auto zoom
				if(core->getFlagManualAutoZoom()) app->commander->execute_command( "zoom auto in manual 1");
				else app->commander->execute_command( "zoom auto in");
			}
		}
		if(key==SDLK_BACKSLASH) app->commander->execute_command( "zoom auto out");
		if(key==SDLK_x)
		{
			app->commander->execute_command( "flag show_tui_datetime toggle");

			// keep these in sync.  Maybe this should just be one flag.
			if(FlagShowTuiDateTime) app->commander->execute_command( "flag show_tui_short_obj_info on");
			else app->commander->execute_command( "flag show_tui_short_obj_info off");
		}
		if(key==SDLK_RETURN)
		{
			core->toggleMountMode();
		}
	}
	return 0;
}


// Update changing values
void StelUI::gui_update_widgets(int delta_time)
{
	updateTopBar();

	// handle mouse cursor timeout
	if(MouseCursorTimeout > 0)
	{
		if(MouseTimeLeft > delta_time) MouseTimeLeft -= delta_time;
		else
		{
			// hide cursor
			MouseTimeLeft = 0;
			SDL_ShowCursor(0);
		}
	}

	// update message win
	message_win->update(delta_time);


	if (FlagShowSelectedObjectInfo && core->getFlagHasSelected())
	{
		info_select_ctr->setVisible(true);
		updateInfoSelectString();
	}
	else
		info_select_ctr->setVisible(false);

	bt_flag_ctr->setVisible(FlagMenu);
	bt_time_control_ctr->setVisible(FlagMenu);

	bt_flag_constellation_draw->setState(core->getFlagConstellationLines());
	bt_flag_constellation_name->setState(core->getFlagConstellationNames());
	bt_flag_constellation_art->setState(core->getFlagConstellationArt());
	bt_flag_azimuth_grid->setState(core->getFlagAzimutalGrid());
	bt_flag_equator_grid->setState(core->getFlagEquatorGrid());
	bt_flag_ground->setState(core->getFlagLandscape());
	bt_flag_cardinals->setState(core->getFlagCardinalsPoints());
	bt_flag_atmosphere->setState(core->getFlagAtmosphere());
	bt_flag_nebula_name->setState(core->getFlagNebulaHints());
	bt_flag_help->setState(help_win->getVisible());
	bt_flag_equatorial_mode->setState(core->getMountMode()==StelCore::MOUNT_EQUATORIAL);
	bt_flag_config->setState(config_win->getVisible());
//	bt_flag_chart->setState(app->getVisionModeChart());
	bt_flag_night->setState(app->getVisionModeNight());
	bt_flag_search->setState(search_win->getVisible());
	bt_flag_goto->setState(false);

	bt_real_time_speed->setState(fabs(core->getTimeSpeed()-JD_SECOND)<0.000001);
	bt_inc_time_speed->setState((core->getTimeSpeed()-JD_SECOND)>0.0001);
	bt_dec_time_speed->setState((core->getTimeSpeed()-JD_SECOND)<-0.0001);
	// cache last time to prevent to much slow system call
	static double lastJD = 0;
	if (fabs(lastJD-core->getJDay())>JD_SECOND/4)
	{
		bt_time_now->setState(fabs(core->getJDay()-get_julian_from_sys())<JD_SECOND);
		lastJD = core->getJDay();
	}
	if (config_win->getVisible()) updateConfigForm();
}

// Update the infos about the selected object in the TextLabel widget
void StelUI::updateInfoSelectString(void)
{
	if (app->getVisionModeNight())
	{
		info_select_txtlbl->setTextColor(Vec3f(1.0,0.2,0.2));
	}
	else
	{
		info_select_txtlbl->setTextColor(core->getSelectedObjectInfoColor());
	}
	info_select_txtlbl->setLabel(core->getSelectedObjectInfo());
}

void StelUI::setTitleObservatoryName(const wstring& name)
{
	if (name == L"")
		top_bar_appName_lbl->setLabel(StelUtility::stringToWstring(APP_NAME));
	else
	{
		top_bar_appName_lbl->setLabel(StelUtility::stringToWstring(APP_NAME) + L" (" + name + L")");
	}
	top_bar_appName_lbl->setPos(core->getViewportWidth()/2-top_bar_appName_lbl->getSizex()/2,1);
}

wstring StelUI::getTitleWithAltitude(void)
{
	return core->getObservatory().getHomePlanetNameI18() +
        L", " + core->getObservatory().get_name() +
        L" @ " + StelUtility::doubleToWstring(core->getObservatory().get_altitude()) + L"m";
}

void StelUI::setColorScheme(const string& skinFile, const string& section)
{
	if (!desktop) return;

	InitParser conf;
	conf.load(skinFile);
	
	s_color GuiBaseColor		= StelUtility::str_to_vec3f(conf.get_str(section, "gui_base_color", "0.3,0.4,0.7"));
	s_color GuiTextColor		= StelUtility::str_to_vec3f(conf.get_str(section, "gui_text_color", "0.7,0.8,0.9"));
	
	desktop->setColorScheme(GuiBaseColor, GuiTextColor);
}
