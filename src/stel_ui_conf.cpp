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

#include "stel_ui.h"

using namespace s_gui;


Component* stel_ui::createConfigWindow(void)
{
	config_win = new StdBtWin("Configuration");
	config_win->reshape(300,200,400,350);
	config_win->setVisible(core->FlagConfig);

	config_tab_ctr = new TabContainer();
	config_tab_ctr->setSize(config_win->getSize());

	// The current drawing position
	int x,y;
	x=10; y=10;

	// Rendering options
	FilledContainer* tab_render = new FilledContainer();
	tab_render->setSize(config_tab_ctr->getSize());

	LabeledCheckBox* stars_cbx = new LabeledCheckBox(core->FlagStars, "Stars");
	stars_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(stars_cbx);
	stars_cbx->setPos(x,y); y+=15;

	LabeledCheckBox* star_names_cbx = new LabeledCheckBox(core->FlagStarName, "Star Names");
	star_names_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(star_names_cbx);
	star_names_cbx->setPos(x,y); y+=15;

	LabeledCheckBox* star_twinkle_cbx = new LabeledCheckBox(core->FlagStarTwinkle, "Star Twinkle");
	star_twinkle_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(star_twinkle_cbx);
	star_twinkle_cbx->setPos(x,y); y+=30;


	LabeledCheckBox* constellation_cbx = new LabeledCheckBox(core->FlagConstellationDrawing, "Constellations");
	constellation_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(constellation_cbx);
	constellation_cbx->setPos(x,y); y+=15;

	LabeledCheckBox* constellation_name_cbx = new LabeledCheckBox(core->FlagConstellationName, "Constellations Names");
	constellation_name_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(constellation_name_cbx);
	constellation_name_cbx->setPos(x,y); y+=15;

	LabeledCheckBox* sel_constellation_cbx = new LabeledCheckBox(core->FlagConstellationPick, "Selected Constellation Only");
	sel_constellation_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(sel_constellation_cbx);
	sel_constellation_cbx->setPos(x,y); y+=30;


	LabeledCheckBox* nebulas_cbx = new LabeledCheckBox(core->FlagNebula, "Nebulas");
	nebulas_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(nebulas_cbx);
	nebulas_cbx->setPos(x,y); y+=15;

	LabeledCheckBox* nebulas_names_cbx = new LabeledCheckBox(core->FlagNebulaName, "Nebulas Names");
	nebulas_names_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(nebulas_names_cbx);
	nebulas_names_cbx->setPos(x,y); y+=30;

	LabeledCheckBox* planets_cbx = new LabeledCheckBox(core->FlagPlanets, "Planets");
	planets_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(planets_cbx);
	planets_cbx->setPos(x,y); y+=15;

	LabeledCheckBox* planets_hints_cbx = new LabeledCheckBox(core->FlagPlanetsHints, "Planets Hints");
	planets_hints_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(planets_hints_cbx);
	planets_hints_cbx->setPos(x,y); y+=30;

	LabeledCheckBox* equator_grid_cbx = new LabeledCheckBox(core->FlagEquatorialGrid, "Equatorial Grid");
	equator_grid_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(equator_grid_cbx);
	equator_grid_cbx->setPos(x,y); y+=15;

	LabeledCheckBox* azimuth_grid_cbx = new LabeledCheckBox(core->FlagAzimutalGrid, "Azimuthal Grid");
	azimuth_grid_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(azimuth_grid_cbx);
	azimuth_grid_cbx->setPos(x,y); y+=15;

	LabeledCheckBox* equator_cbx = new LabeledCheckBox(core->FlagEquatorLine, "Equator Line");
	equator_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(equator_cbx);
	equator_cbx->setPos(x,y); y+=15;

	LabeledCheckBox* ecliptic_cbx = new LabeledCheckBox(core->FlagEclipticLine, "Ecliptic Line");
	ecliptic_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(ecliptic_cbx);
	ecliptic_cbx->setPos(x,y); y+=15;

	LabeledCheckBox* cardinal_cbx = new LabeledCheckBox(core->FlagCardinalPoints, "Cardinal Points");
	cardinal_cbx->setOnPressCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(cardinal_cbx);
	cardinal_cbx->setPos(x,y); y+=15;


	CursorBar* star_scale_cbar = new CursorBar(0,20,2);
	star_scale_cbar->setOnChangeCallback(makeFunctor((s_pcallback0)0,*this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(star_scale_cbar);
	star_scale_cbar->setPos(x,y); y+=15;

	// Date & Time options
	FilledContainer* tab_time = new FilledContainer();
	tab_time->setSize(config_tab_ctr->getSize());

	// Location options
	FilledContainer* tab_location = new FilledContainer();
	tab_location->setSize(config_tab_ctr->getSize());

	// GUI options
	FilledContainer* tab_gui = new FilledContainer();
	tab_gui->setSize(config_tab_ctr->getSize());

	config_tab_ctr->addTab(tab_render, "Rendering");
	config_tab_ctr->addTab(tab_time, "Date & Time");
	config_tab_ctr->addTab(tab_location, "Location");
	config_tab_ctr->addTab(tab_gui, "GUI");
	config_win->addComponent(config_tab_ctr);

	return config_win;
}

void stel_ui::updateConfigVariables(void)
{
	printf("updateConfigVariables\n");
}

void stel_ui::updateConfigForm(void)
{
	printf("updateConfigForm\n");
}

/*
void ConfigWinHideCallback(void) 
{
	global.FlagConfig=false;
	ConfigWin->setVisible(false);
	BtConfig->setActive(false);
}


void ChangeStarDrawNameBarOnChangeValue(float value,Component *)
{
	global.MaxMagStarName=value;
	char tempValueStr[30];
	sprintf(tempValueStr,"\1 Show if Magnitude < %.1f",global.MaxMagStarName);
	StarNameMagLabel->setLabel(tempValueStr);
}


void ChangeStarTwinkleBarOnChangeValue(float value,Component *)
{
	global.StarTwinkleAmount=value;
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Twinkle Amount : %.1f",global.StarTwinkleAmount);
    StarTwinkleLabel->setLabel(tempValueStr);
}


void ChangeStarScaleBarOnChangeValue(float value,Component *)
{
	global.StarScale=value;
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Scale : %.1f",global.StarScale);
    StarScaleLabel->setLabel(tempValueStr);
}


void ToggleStarNameOnClicCallback(guiValue button,Component *)
{
	if (global.FlagStarName)
	{
	   if (ToggleStarName)
			ToggleStarName->setLabel("OFF");
		global.FlagStarName=false;
	}
	else
	{
	    if (ToggleStarName) ToggleStarName->setLabel("ON");
		global.FlagStarName=true;
	}
}

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



void LatitudeBarOnChangeValue(float value, Component *)
{
    navigation.set_latitude(value);
    char tempValueStr[30];
    sprintf(tempValueStr,"\1 Latitude : %s",print_angle_dms_stel(navigation.get_latitude()));
    LatitudeLabel->setLabel(tempValueStr);
	EarthMap->setPointerPosition(vec2_t(EarthMap->getPointerPosition()[0],1-((value-90.)/180.) *
	 	(EarthMap->getSize())[1]));
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


void init_config_window(void)
{

    // Star Config container
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
 
    // Landcape config container
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


    // Location config container
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

    // Config window
    ConfigWin = new StdBtWin(40, 40, 532, 287, "Configuration", Base, spaceFont);
    ConfigWin->addComponent(StarConfigContainer);
    ConfigWin->addComponent(LandscapeConfigContainer);
    ConfigWin->addComponent(LocationConfigContainer);
    ConfigWin->setHideCallback(ConfigWinHideCallback);
    ConfigWin->setVisible(global.FlagConfig);

}
*/
