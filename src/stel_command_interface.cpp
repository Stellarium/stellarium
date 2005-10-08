/*
 * Stellarium
 * This file Copyright (C) 2005 Robert Spearman
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

#include <iostream>
#include <sstream>
#include <string>
#include "stel_command_interface.h"
#include "stel_core.h"
#include "image.h"
#include "stellastro.h"

using namespace std;


StelCommandInterface::StelCommandInterface(stel_core * core) {
  stcore = core;
  audio = NULL;
}

StelCommandInterface::~StelCommandInterface() {
}

int StelCommandInterface::execute_command(string commandline ) {
  unsigned long int delay;
  return execute_command(commandline, delay, 1);  // Assumed to be trusted!
  // delay is ignored, as not needed by the ui callers
}

// for easy calling of simple commands with a double as last argument value
int StelCommandInterface::execute_command(string command, double arg) {
  unsigned long int delay;

  std::ostringstream commandline;
  commandline << command << arg;

  return execute_command(commandline.str(), delay, 1);  // Assumed to be trusted!
  // delay is ignored, as not needed by the ui callers
}

// for easy calling of simple commands with an int as last argument value
int StelCommandInterface::execute_command(string command, int arg) {
  unsigned long int delay;

  std::ostringstream commandline;
  commandline << command << arg;

  return execute_command(commandline.str(), delay, 1);  // Assumed to be trusted!
  // delay is ignored, as not needed by the ui callers
}


// called by script executors
// certain key settings can't be modified by scripts unless
// they are "trusted" - TODO details TBD when needed
int StelCommandInterface::execute_command(string commandline, unsigned long int &wait, bool trusted) {
  string command;
  stringHash_t args;
  int status = 0;  // true if command was understood
  int recordable = 1;  // true if command should be recorded (if recording)
  
  wait = 0;  // default, no wait between commands

  status = parse_command(commandline, command, args);

  // stellarium specific logic to run each command

  if(command == "flag") {

	  // TODO: loop if want to allow that syntax
	  if(args.begin() != args.end()) {  
	  
		  bool val;
		  status = set_flag( args.begin()->first, args.begin()->second, val, trusted);
	
		  // rewrite command for recording so that actual state is known (rather than "toggle")
		  if(args.begin()->second == "toggle") {
			  std::ostringstream oss;
			  oss << command << " " << args.begin()->first << " " << val;
			  commandline = oss.str();
		  }
	  } else status = 0;

  }  else if (command == "wait" && args["duration"]!="") {

    float fdelay = str_to_double(args["duration"]);
    if(fdelay > 0) wait = (int)(fdelay*1000);

  } else if (command == "set") {
    // set core variables

    // TODO: some bounds/error checking here

    if(args["atmosphere_fade_duration"]!="") stcore->AtmosphereFadeDuration = str_to_double(args["atmosphere_fade_duration"]);
    else if(args["auto_move_duration"]!="") stcore->auto_move_duration = str_to_double(args["auto_move_duration"]);
    else if(args["constellation_art_fade_duration"]!="") stcore->ConstellationArtFadeDuration = str_to_double(args["constellation_art_fade_duration"]);
    else if(args["constellation_art_intensity"]!="") stcore->ConstellationArtIntensity = str_to_double(args["constellation_art_intensity"]);
    else if(args["landscape_name"]!="") stcore->set_landscape(args["landscape_name"]);
    else if(args["max_mag_nebula_name"]!="") stcore->MaxMagNebulaName = str_to_double(args["max_mag_nebula_name"]);
    else if(args["max_mag_star_name"]!="") stcore->MaxMagStarName = str_to_double(args["max_mag_star_name"]);
    else if(args["moon_scale"]!="") {
      stcore->MoonScale = str_to_double(args["moon_scale"]);
      if(stcore->MoonScale<0) stcore->MoonScale=1;  // negative numbers reverse drawing!
      if (stcore->FlagMoonScaled) stcore->ssystem->get_moon()->set_sphere_scale(stcore->MoonScale);
    }
    else if(args["sky_culture"]!="") status = stcore->set_sky_culture(args["sky_culture"]);
//    else if(args["sky_locale"]!="") stcore->set_sky_locale(args["sky_locale"]); // Tony NOT SURE
    else if(args["sky_locale"]!="") stcore->set_system_locale_by_name(args["sky_locale"]);
    else if(args["star_mag_scale"]!="") stcore->StarMagScale = str_to_double(args["star_mag_scale"]);
    else if(args["star_scale"]!="") {
		float scale = str_to_double(args["star_scale"]);
		stcore->StarScale = scale;
		stcore->ssystem->set_object_scale(scale);
	} 
	else if(args["nebula_scale"]!="")
	{
		float scale = str_to_double(args["nebula_scale"]);
		stcore->NebulaScale = scale;
	} 
	else if(args["star_twinkle_amount"]!="") stcore->StarTwinkleAmount = str_to_double(args["star_twinkle_amount"]);
    else if(args["time_zone"]!="") stcore->observatory->set_custom_tz_name(args["time_zone"]);
    else if(args["milky_way_intensity"]!="") {
		stcore->MilkyWayIntensity = str_to_double(args["milky_way_intensity"]);
		stcore->milky_way->set_intensity(stcore->MilkyWayIntensity);

		// safety feature to be able to turn back on
		if(stcore->MilkyWayIntensity) stcore->FlagMilkyWay = 1;

	} else status = 0;


    if(trusted) {

    //    else if(args["base_font_size"]!="") stcore->BaseFontSize = str_to_double(args["base_font_size"]);
    //	else if(args["bbp_mode"]!="") stcore->BbpMode = str_to_double(args["bbp_mode"]);
    //    else if(args["date_display_format"]!="") stcore->DateDisplayFormat = args["date_display_format"];
    //	else if(args["fullscreen"]!="") stcore->Fullscreen = args["fullscreen"];
    //	else if(args["horizontal_offset"]!="") stcore->HorizontalOffset = str_to_double(args["horizontal_offset"]);
    //	else if(args["init_fov"]!="") stcore->InitFov = str_to_double(args["init_fov"]);
    //	else if(args["preset_sky_time"]!="") stcore->PresetSkyTime = str_to_double(args["preset_sky_time"]);
    //	else if(args["screen_h"]!="") stcore->ScreenH = str_to_double(args["screen_h"]);
    //	else if(args["screen_w"]!="") stcore->ScreenW = str_to_double(args["screen_w"]);
    //    else if(args["startup_time_mode"]!="") stcore->StartupTimeMode = args["startup_time_mode"];
    // else if(args["time_display_format"]!="") stcore->TimeDisplayFormat = args["time_display_format"];
      //else if(args["type"]!="") stcore->Type = args["type"];
      //else if(args["version"]!="") stcore->Version = str_to_double(args["version"]);
      //      else if(args["vertical_offset"]!="") stcore->VerticalOffset = str_to_double(args["vertical_offset"]);
      //else if(args["viewing_mode"]!="") stcore->ViewingMode = args["viewing_mode"];
      //else if(args["viewport"]!="") stcore->Viewport = args["viewport"];

    }

  } else if (command == "select") {

    // default is to deselect current object
    stcore->selected_object=NULL;

    if(args["hp"]!=""){
      unsigned int hpnum;
      std::istringstream istr(args["hp"]);
      istr >> hpnum;
      stcore->selected_object = stcore->hip_stars->searchHP(hpnum);
      stcore->asterisms->set_selected((Hip_Star*)stcore->selected_object);
      stcore->selected_planet=NULL;
    } else if(args["planet"]!=""){
      stcore->selected_object = stcore->selected_planet = stcore->ssystem->search(args["planet"]);
      stcore->asterisms->set_selected(NULL);
    } else if(args["nebula"]!=""){
      stcore->selected_object = stcore->nebulas->search(args["nebula"]);
      stcore->selected_planet=NULL;
      stcore->asterisms->set_selected(NULL);
    } else if(args["constellation"]!=""){
      unsigned int hpnum; // Tony - highlight the constellation and the first star in the list
      stcore->asterisms->set_selected(args["constellation"]);
      hpnum = stcore->asterisms->get_first_selected_HP();
      stcore->selected_object = stcore->hip_stars->searchHP(hpnum);
      stcore->asterisms->set_selected((Hip_Star*)stcore->selected_object);
      stcore->selected_planet=NULL;
    }

    if (stcore->selected_object) {
      if (stcore->navigation->get_flag_traking()) stcore->navigation->set_flag_lock_equ_pos(1);
      stcore->navigation->set_flag_traking(0);

	  // determine if selected object pointer should be displayed
	  if(args["pointer"]=="off" || args["pointer"]=="0") stcore->set_object_pointer_visibility(0);
	  else stcore->set_object_pointer_visibility(1);
    }

  } else if (command == "deselect") {
    stcore->selected_object = NULL;
    stcore->selected_planet = NULL;
    stcore->asterisms->set_selected(NULL);

  } else if(command == "look") {  // change direction of view
	  //	  double duration = str_to_pos_double(args["duration"]);

	  if(args["delta_az"]!="" || args["delta_alt"]!="") {
		  // immediately change viewing direction
		  // NOTE: necessitates turning off object tracking 
		  stcore->navigation->set_flag_traking(0);
		  stcore->navigation->update_move(str_to_double(args["delta_az"]),
										  str_to_double(args["delta_alt"]));
	  }	else status = 0;

	  // TODO, absolute settings


	  

  } else if(command == "zoom") {
	  double duration = str_to_pos_double(args["duration"]);

	  if(args["auto"]!="") {
		  // auto zoom using specified or default duration
		  if(args["duration"]=="") duration = stcore->auto_move_duration;
		  		  
		  if(args["auto"]=="out") stcore->auto_zoom_out(duration, 0);
		  else if(args["auto"]=="initial") stcore->auto_zoom_out(duration, 1);
		  else if(args["manual"]=="1") {
			  stcore->auto_zoom_in(duration, 1);  // have to explicity allow possible manual zoom 
		  } else stcore->auto_zoom_in(duration, 0);  

	  } else if (args["fov"]!="") {
		  // zoom to specific field of view
	  	  stcore->projection->zoom_to( str_to_double(args["fov"]), str_to_double(args["duration"]));

	  } else if (args["delta_fov"]!="") stcore->projection->change_fov(str_to_double(args["delta_fov"]));
	  // should we record absolute fov instead of delta? isn't usually smooth playback
	  else status = 0;

  } else if(command == "timerate") {   // NOTE: accuracy issue related to frame rate

    if(args["rate"]!="") {
      stcore->navigation->set_time_speed(str_to_double(args["rate"])*JD_SECOND);

    } else if(args["action"]=="pause") {
      stcore->FlagTimePause = !stcore->FlagTimePause;
      if(stcore->FlagTimePause) {
	stcore->temp_time_velocity = stcore->navigation->get_time_speed();
	stcore->navigation->set_time_speed(0);
      } else {
	stcore->navigation->set_time_speed(stcore->temp_time_velocity);
      }

    } else if(args["action"]=="resume") {
      stcore->FlagTimePause = 0;
      stcore->navigation->set_time_speed(stcore->temp_time_velocity);

    } else if(args["action"]=="increment") {
      // speed up time rate
      double s = stcore->navigation->get_time_speed();
      if (s>=JD_SECOND) s*=10.;
      else if (s<-JD_SECOND) s/=10.;
      else if (s>=0. && s<JD_SECOND) s=JD_SECOND;
      else if (s>=-JD_SECOND && s<0.) s=0.;
      stcore->navigation->set_time_speed(s);

      // for safest script replay, record as absolute amount
      commandline = "timerate rate " + double_to_str(s/JD_SECOND);

    } else if(args["action"]=="decrement") {
      double s = stcore->navigation->get_time_speed();
      if (s>JD_SECOND) s/=10.;
      else if (s<=-JD_SECOND) s*=10.;
      else if (s>-JD_SECOND && s<=0.) s=-JD_SECOND;
      else if (s>0. && s<=JD_SECOND) s=0.;
      stcore->navigation->set_time_speed(s);

      // for safest script replay, record as absolute amount
      commandline = "timerate rate " + double_to_str(s/JD_SECOND);
    } else status=0;
    
  } else if(command == "date") {
	  
	  // ISO 8601-like format [[+/-]YYYY-MM-DD]Thh:mm:ss (no timzone offset, T is literal)
	  if(args["local"]!="") {
		  double jd;
		  string new_date;

		  if(args["local"][0] == 'T') {
			  // set time only (don't change day)
			  string sky_date = stcore->observatory->get_ISO8601_time_local(stcore->navigation->get_JDay());
			  new_date = sky_date.substr(0,10) + args["local"];
		  } else new_date = args["local"];

		  if(string_to_jday( new_date, jd ) ) {
			  stcore->navigation->set_JDay(jd - stcore->observatory->get_GMT_shift(jd) * JD_HOUR);
		  } else {
			  cout << "Error parsing date: " << new_date << endl;
			  status = 0;
		  } 
		  
	  } else if(args["utc"]!="") {
		  double jd;
		  if(string_to_jday( args["utc"], jd ) ) {
			  stcore->navigation->set_JDay(jd);
		  } else {
			  cout << "Error parsing date." << endl;
			  status = 0;
		  }
		  
	  } else if(args["relative"]!="") {  // value is a float number of days
		  double days = str_to_double(args["relative"]);
		  stcore->navigation->set_JDay(stcore->navigation->get_JDay() + days );
	  } else if(args["load"]=="current") {
		  // set date to current date
 		  stcore->navigation->set_JDay(get_julian_from_sys());
	  } else if(args["load"]=="preset") {
		  // set date to preset (or current) date, based on user setup
		  // TODO: should this record as the actual date used?
		  if (stcore->StartupTimeMode=="preset" || stcore->StartupTimeMode=="Preset")
			  stcore->navigation->set_JDay(stcore->PresetSkyTime -
										 stcore->observatory->get_GMT_shift(stcore->PresetSkyTime) * JD_HOUR);
		  else stcore->navigation->set_JDay(get_julian_from_sys());

	  } else status=0;
	  
  } else if (command == "moveto") {
	  
	  if(args["lat"]!="" || args["lon"]!="" || args["alt"]!="") {

		  double lat = stcore->observatory->get_latitude();
		  double lon = stcore->observatory->get_longitude();
		  double alt = stcore->observatory->get_altitude();
		  string name;
		  int delay;
		  
		  if(args["name"]!="") name = args["name"];
		  if(args["lat"]!="") lat = str_to_double(args["lat"]);
		  if(args["lon"]!="") lon = str_to_double(args["lon"]);
		  if(args["alt"]!="") alt = str_to_double(args["alt"]);
		  delay = (int)(1000.*str_to_double(args["duration"]));
		  
		  stcore->observatory->move_to(lat,lon,alt,delay,name);
	  } else status = 0;

  } else if(command=="image") {

	  if(args["name"]=="") {
		  cout << "Image name required" << endl;
		  status = 0;
	  } else if(args["action"]=="drop") {
		  stcore->script_images->drop_image(args["name"]);
	  } else {
		  if(args["action"]=="load" && args["filename"]!="") {

			  // TODO: more image positioning coordinates
			  IMAGE_POSITIONING img_pos = POS_VIEWPORT;
			  if(args["coordinate_system"] == "horizontal") img_pos = POS_HORIZONTAL;
			  /*
			  else if(args["coordinates"] == "rade") img_pos = POS_EQUATORIAL;
			  else img_pos = POS_VIEWPORT;
			  */

			  if(stcore->scripts->is_playing()) 
				  stcore->script_images->load_image(stcore->scripts->get_script_path() + args["filename"], 
													args["name"], img_pos);
			  else stcore->script_images->load_image(stcore->DataRoot + "/" + args["filename"], 
													 args["name"], img_pos);
		  }

		  Image * img = stcore->script_images->get_image(args["name"]);
		  
		  if(img != NULL) {
			  if(args["alpha"]!="") img->set_alpha(str_to_double(args["alpha"]), 
												   str_to_double(args["duration"]));
			  if(args["scale"]!="") img->set_scale(str_to_double(args["scale"]), 
												   str_to_double(args["duration"]));
			  if(args["rotation"]!="") img->set_rotation(str_to_double(args["rotation"]), 
														 str_to_double(args["duration"]));
			  if(args["xpos"]!="" || args["ypos"]!="") 
				  img->set_location(str_to_double(args["xpos"]), args["xpos"]!="",
									str_to_double(args["ypos"]), args["ypos"]!="",
									str_to_double(args["duration"]));
			  // for more human readable scripts, as long as someone doesn't do both...
			  if(args["altitude"]!="" || args["azimuth"]!="") 
				  img->set_location(str_to_double(args["altitude"]), args["altitude"]!="",
									str_to_double(args["azimuth"]), args["azimuth"]!="",
									str_to_double(args["duration"]));
		  }
	  }
  } 
#ifdef HAVE_SDL_MIXER_H
else if(command=="audio") {
  
	  if(args["action"]=="sync") {
		  if(audio) audio->sync();

	  } else if(args["action"]=="pause") {
		  if(audio) audio->pause();

	  } else if( args["action"]=="play" && args["filename"]!="") {
		  // only one track at a time allowed
		  if(audio) delete audio;
		  
		  // if from script, local to that path
		  string path;
		  if(stcore->scripts->is_playing()) path = stcore->scripts->get_script_path();
		  else path = stcore->DataRoot + "/";
		  
		  //		  cout << "audio path = " << path << endl;
		  
		  audio = new Audio(path + args["filename"], "default track", str_to_long(args["output_rate"]));
		  audio->play(args["loop"]=="on");

		  // if fast forwarding mute (pause) audio
		  if(stcore->get_time_multiplier()!=1) audio->pause();

	  } else if(args["volume"]!="") {

		  recordable = 0;
		  if(audio!=NULL) {
			  if(args["volume"] == "increment") {
				  audio->increment_volume();
			  } else if(args["volume"] == "decrement") {
				  audio->decrement_volume();
			  } else audio->set_volume( str_to_double(args["volume"]) );
		  }
	  } else status = 0;
  }
#endif 
else if(command=="script") {

    if(args["action"]=="end") {
      // stop script, audio, and unload any loaded images
      if(audio) {
		  delete audio;
		  audio = NULL;
      }
      stcore->scripts->cancel_script();
      stcore->script_images->drop_all_images();

    } else if(args["action"]=="play" && args["filename"]!="") {
		if(stcore->scripts->is_playing()) {
			
			// keep same script path 
			stcore->scripts->play_script(stcore->scripts->get_script_path() + args["filename"], 
										 stcore->scripts->get_script_path());
		} else {
			stcore->scripts->play_script(args["filename"], args["path"]);
		}

    } else if(args["action"]=="record") {
      stcore->scripts->record_script(args["filename"]);
      recordable = 0;  // don't record this command!
    } else if(args["action"]=="cancelrecord") {
      stcore->scripts->cancel_record_script();
      recordable = 0;  // don't record this command!
    } else if(args["action"]=="pause" && !stcore->scripts->is_paused()) {
      // n.b. action=pause TOGGLES pause
      if(audio) audio->pause();
      stcore->scripts->pause_script();
    } else if (args["action"]=="pause" || args["action"]=="resume") {
      stcore->scripts->resume_script();
#ifdef HAVE_SDL_MIXER_H
      if(audio) audio->sync();
#endif
    } else status =0;

  } else if(command=="clear") {

	  // set sky to known, standard states (used by scripts for simplicity)

	  if(args["state"] == "natural") {
		  execute_command("flag atmosphere on");
		  execute_command("flag landscape on");
	  } else {
		  execute_command("flag atmosphere off");
		  execute_command("flag landscape off");
	  }

	  // turn off all labels
	  execute_command("flag azimuthal_grid off");
	  execute_command("flag meridian_line off");
	  execute_command("flag cardinal_points off");
	  execute_command("flag constellation_art off");
	  execute_command("flag constellation_drawing off");
	  execute_command("flag constellation_names off");
	  execute_command("flag ecliptic_line off");
	  execute_command("flag equatorial_grid off");
	  execute_command("flag equator_line off");
	  execute_command("flag fog off");
	  execute_command("flag nebula_names off");
	  execute_command("flag object_trails off");
	  execute_command("flag planet_names off");
	  execute_command("flag planet_orbits off");
	  execute_command("flag show_tui_datetime off");
	  execute_command("flag star_names off");
	  execute_command("flag show_tui_short_obj_info off");

	  // make sure planets, stars, etc. are turned on!
	  // milkyway is left to user, for those without 3d cards
	  execute_command("flag stars on");
	  execute_command("flag planets on");
	  execute_command("flag nebulae on");

	  // also deselect everything, set to default fov and real time rate
	  execute_command("deselect");
	  execute_command("timerate rate 1");
	  execute_command("zoom auto initial");

  } else if(command=="landscape" && args["action"] == "load") {

	  // textures are relative to script
	  args["path"] = stcore->scripts->get_script_path();
	  if (stcore->landscape) delete stcore->landscape;
	  stcore->landscape = NULL;
	  stcore->landscape = Landscape::create_from_hash(args);
	  
	  stcore->observatory->set_landscape_name(args["name"]);  
	  // probably not particularly useful, as not in landscape file

  } else if(command=="meteors") {
	  if(args["zhr"]!="") {
		  stcore->meteors->set_ZHR(str_to_int(args["zhr"]));
	  } else status =0;

  } else if(command=="configuration") {

	  if(args["action"]=="load" && trusted) {
		  // eventually load/reload are not both needed, but for now this is called at startup, reload later
		  stcore->load_config();
		  recordable = 0;  // don't record as scripts can not run this

	  } else if(args["action"]=="reload") {

		  // on reload, be sure to reconfigure as necessary since stel_core::init isn't called

		  stcore->load_config();

		  if(stcore->asterisms) {
			  stcore->asterisms->set_art_intensity(stcore->ConstellationArtIntensity);
			  stcore->asterisms->set_art_fade_duration(stcore->ConstellationArtFadeDuration);
		  }
		  if (!stcore->FlagAtmosphere && stcore->tone_converter)
			  stcore->tone_converter->set_world_adaptation_luminance(3.75f);
		  if (stcore->atmosphere) stcore->atmosphere->set_fade_duration(stcore->AtmosphereFadeDuration);
		  stcore->observatory->load(stcore->ConfigDir + stcore->config_file, "init_location");
		  stcore->set_landscape(stcore->observatory->get_landscape_name());
		  
		  if (stcore->StartupTimeMode=="preset" || stcore->StartupTimeMode=="Preset")
			  {
				  stcore->navigation->set_JDay(stcore->PresetSkyTime -
											 stcore->observatory->get_GMT_shift(stcore->PresetSkyTime) * JD_HOUR);
			  }
		  else
			  {
				  stcore->navigation->set_JDay(get_julian_from_sys());
			  }
		  if(stcore->FlagObjectTrails && stcore->ssystem) stcore->ssystem->start_trails();
		  else  stcore->ssystem->end_trails();

//		  stcore->set_sky_locale(stcore->SkyLocale); Tony not sure
		  stcore->set_system_locale_by_name(stcore->SkyLocale);

		  string temp = stcore->SkyCulture;  // fool caching in below method
		  stcore->SkyCulture = "";
		  stcore->set_sky_culture(temp);
		  
		  system( ( stcore->DataDir + "script_load_config " ).c_str() );

	  } else status = 0;

  } else {
    cout << "Unrecognized or malformed command: " << command << endl;
    status = 0;
  }


  if(status ) {

    // if recording commands, do that now
    if(recordable) stcore->scripts->record_command(commandline);

    //    cout << commandline << endl;

  } else {
    cout << "Could not execute: " << commandline << endl;
  }

  return(status);


}


// set flags - TODO: stel_core flag variables will be replaced with object attributes
// if caller is not trusted, some flags can't be changed 
// newval is new value of flag changed

int StelCommandInterface::set_flag(string name, string value, bool &newval, bool trusted) {

	bool status = 1; 

	// value can be "on", "off", or "toggle"
	if(value == "toggle") {

		if(trusted) {
			// normal scripts shouldn't be able to change these user settings
			if(name=="enable_zoom_keys") newval = (stcore->FlagEnableZoomKeys = !stcore->FlagEnableZoomKeys);
			else if(name=="enable_move_keys") newval = (stcore->FlagEnableMoveKeys = !stcore->FlagEnableMoveKeys);
			else if(name=="enable_move_mouse") newval = (stcore->FlagEnableMoveMouse = !stcore->FlagEnableMoveMouse);
			else if(name=="menu") newval = (stcore->FlagMenu = !stcore->FlagMenu);
			else if(name=="help") newval = (stcore->FlagHelp = !stcore->FlagHelp);
			else if(name=="infos") newval = (stcore->FlagInfos = !stcore->FlagInfos);
			else if(name=="show_topbar") newval = (stcore->FlagShowTopBar = !stcore->FlagShowTopBar);
			else if(name=="show_time") newval = (stcore->FlagShowTime = !stcore->FlagShowTime);
			else if(name=="show_date") newval = (stcore->FlagShowDate = !stcore->FlagShowDate);
			else if(name=="show_appname") newval = (stcore->FlagShowAppName = !stcore->FlagShowAppName);
			else if(name=="show_fps") newval = (stcore->FlagShowFps = !stcore->FlagShowFps);
			else if(name=="show_fov") newval = (stcore->FlagShowFov = !stcore->FlagShowFov);
			else if(name=="enable_tui_menu") newval = (stcore->FlagEnableTuiMenu = !stcore->FlagEnableTuiMenu);
			else if(name=="show_gravity_ui") newval = (stcore->FlagShowGravityUi = !stcore->FlagShowGravityUi);
			else if(name=="utc_time") newval = (stcore->FlagUTC_Time = !stcore->FlagUTC_Time);
			else if(name=="gravity_labels") newval = (stcore->FlagGravityLabels = !stcore->FlagGravityLabels);
			else status = 0;  // no match here, anyway

		} else status = 0;


		if(name=="constellation_drawing") {
			newval = !stcore->asterisms->get_flag_lines();
			stcore->asterisms->set_flag_lines(newval);
		} 
		else if(name=="constellation_names") {
			newval = !stcore->asterisms->get_flag_names();
			stcore->asterisms->set_flag_names(newval);
		} 
		else if(name=="constellation_art") {
			newval = !stcore->asterisms->get_flag_art();
			stcore->asterisms->set_flag_art(newval);
		} 
		else if(name=="constellation_pick") { 
             newval = !stcore->asterisms->get_flag_isolate_selected();
             stcore->asterisms->set_flag_isolate_selected(newval);
        }
		else if(name=="star_twinkle") newval = (stcore->FlagStarTwinkle = !stcore->FlagStarTwinkle);
		else if(name=="point_star") newval = (stcore->FlagPointStar = !stcore->FlagPointStar);
		else if(name=="show_selected_object_info") newval = (stcore->FlagShowSelectedObjectInfo = !stcore->FlagShowSelectedObjectInfo);
		else if(name=="show_tui_datetime") newval = (stcore->FlagShowTuiDateTime = !stcore->FlagShowTuiDateTime);
		else if(name=="show_tui_short_obj_info") newval = (stcore->FlagShowTuiShortObjInfo = !stcore->FlagShowTuiShortObjInfo);
		else if(name=="manual_zoom") newval = (stcore->FlagManualZoom = !stcore->FlagManualZoom);
		else if(name=="show_script_bar") newval = (stcore->FlagShowScriptBar = !stcore->FlagShowScriptBar);
		else if(name=="fog") newval = (stcore->FlagFog = !stcore->FlagFog);
		else if(name=="atmosphere") {
			newval = (stcore->FlagAtmosphere = !stcore->FlagAtmosphere);
			if(!newval) stcore->FlagFog = 0;  // turn off fog with atmosphere
		}
		else if(name=="night") {
			newval = stcore->FlagNight = !stcore->FlagNight;
		}
		else if(name=="use_common_names") newval = (stcore->FlagUseCommonNames = !stcore->FlagUseCommonNames);
		else if(name=="azimuthal_grid") newval = (stcore->FlagAzimutalGrid = !stcore->FlagAzimutalGrid);
		else if(name=="equatorial_grid") newval = (stcore->FlagEquatorialGrid = !stcore->FlagEquatorialGrid);
		else if(name=="equator_line") newval = (stcore->FlagEquatorLine = !stcore->FlagEquatorLine);
		else if(name=="ecliptic_line") newval = (stcore->FlagEclipticLine = !stcore->FlagEclipticLine);
		else if(name=="meridian_line") newval = (stcore->FlagMeridianLine = !stcore->FlagMeridianLine);
		else if(name=="cardinal_points") {
			newval = !stcore->cardinals_points->get_flag_show();
			stcore->cardinals_points->set_flag_show(newval);
		}
		else if(name=="moon_scaled") {
			newval = (stcore->FlagMoonScaled = !stcore->FlagMoonScaled);
			if (newval)	stcore->ssystem->get_moon()->set_sphere_scale(stcore->MoonScale);
			else stcore->ssystem->get_moon()->set_sphere_scale(1.);
		}
		else if(name=="landscape") newval = (stcore->FlagLandscape = !stcore->FlagLandscape);
		else if(name=="stars") newval = (stcore->FlagStars = !stcore->FlagStars);
		else if(name=="star_names") {
			newval = (stcore->FlagStarName = !stcore->FlagStarName);
			if(newval) stcore->FlagStars = 1;  // make sure stars on if want labels
		}
		else if(name=="planets") {
			newval = (stcore->FlagPlanets = !stcore->FlagPlanets);
			if (!stcore->FlagPlanets) stcore->FlagPlanetsHints = 0;
		}
		else if(name=="planet_names") {
			newval = (stcore->FlagPlanetsHints = !stcore->FlagPlanetsHints);
			if(stcore->FlagPlanetsHints) stcore->FlagPlanets = 1;  // for safety if script turns planets off
		}
		else if(name=="planet_orbits") newval = (stcore->FlagPlanetsOrbits = !stcore->FlagPlanetsOrbits);
		else if(name=="nebulae") newval = (stcore->FlagNebula = !stcore->FlagNebula);
		else if(name=="nebula_names") {
			newval = !stcore->nebulas->get_flag_hints();
			if(newval) stcore->FlagNebula = 1;  // make sure visible
			stcore->nebulas->set_flag_hints(newval);
		}
        else if(name=="nebula_long_names") {  // Tony - added long names
			newval = !stcore->FlagNebulaLongName;
			if(newval) stcore->FlagNebula = 1;  // make sure visible
			stcore->nebulas->set_flag_hints(newval);
			stcore->FlagNebulaLongName = newval;
		}
		else if(name=="milky_way") newval = (stcore->FlagMilkyWay = !stcore->FlagMilkyWay);
		else if(name=="bright_nebulae") newval = (stcore->FlagBrightNebulae = !stcore->FlagBrightNebulae);
		else if(name=="object_trails") newval = (stcore->FlagObjectTrails = !stcore->FlagObjectTrails);
		else if(name=="track_object") {
			if(stcore->navigation->get_flag_traking() || !stcore->selected_object) {
				newval = 0;
				stcore->navigation->set_flag_traking(0);
			} else {
				stcore->navigation->move_to(stcore->selected_object->get_earth_equ_pos(stcore->navigation),
									stcore->auto_move_duration);
				stcore->navigation->set_flag_traking(1);
				newval = 1;
			}
		}
		else return(status);  // no matching flag found untrusted, but maybe trusted matched

	} else {

		newval = (value == "on" || value == "1");

		if(trusted) {
			// normal scripts shouldn't be able to change these user settings
			if(name=="enable_zoom_keys") stcore->FlagEnableZoomKeys = newval;
			else if(name=="enable_move_keys") stcore->FlagEnableMoveKeys = newval;
			else if(name=="enable_move_mouse") stcore->FlagEnableMoveMouse = newval;
			else if(name=="menu") stcore->FlagMenu = newval;
			else if(name=="help") stcore->FlagHelp = newval;
			else if(name=="infos") stcore->FlagInfos = newval;
			else if(name=="show_topbar") stcore->FlagShowTopBar = newval;
			else if(name=="show_time") stcore->FlagShowTime = newval;
			else if(name=="show_date") stcore->FlagShowDate = newval;
			else if(name=="show_appname") stcore->FlagShowAppName = newval;
			else if(name=="show_fps") stcore->FlagShowFps = newval;
			else if(name=="show_fov") stcore->FlagShowFov = newval;
			else if(name=="enable_tui_menu") stcore->FlagEnableTuiMenu = newval;
			else if(name=="show_gravity_ui") stcore->FlagShowGravityUi = newval;
			else if(name=="utc_time") stcore->FlagUTC_Time = newval;
			else if(name=="gravity_labels") stcore->FlagGravityLabels = newval;
			else status = 0;
		
		} else status = 0;

		if(name=="constellation_drawing") stcore->asterisms->set_flag_lines(newval);
		else if(name=="constellation_names") stcore->asterisms->set_flag_names(newval);
		else if(name=="constellation_art") stcore->asterisms->set_flag_art(newval);
     	else if(name=="constellation_pick") stcore->asterisms->set_flag_isolate_selected(newval);
		else if(name=="star_twinkle") stcore->FlagStarTwinkle = newval;
		else if(name=="point_star") stcore->FlagPointStar = newval;
		else if(name=="show_selected_object_info") stcore->FlagShowSelectedObjectInfo = newval;
		else if(name=="show_tui_datetime") stcore->FlagShowTuiDateTime = newval;
		else if(name=="show_tui_short_obj_info") stcore->FlagShowTuiShortObjInfo = newval;
		else if(name=="manual_zoom") stcore->FlagManualZoom = newval;
		else if(name=="show_script_bar") stcore->FlagShowScriptBar = newval;
		else if(name=="fog") stcore->FlagFog = newval;
		else if(name=="atmosphere") { 
			stcore->FlagAtmosphere = newval;
			if(!newval) stcore->FlagFog = 0;  // turn off fog with atmosphere
		}
		else if(name=="night") {
			stcore->FlagNight = newval;
		}
		else if(name=="use_common_names") stcore->FlagUseCommonNames = newval;
		else if(name=="azimuthal_grid") stcore->FlagAzimutalGrid = newval;
		else if(name=="equatorial_grid") stcore->FlagEquatorialGrid = newval;
		else if(name=="equator_line") stcore->FlagEquatorLine = newval;
		else if(name=="ecliptic_line") stcore->FlagEclipticLine = newval;
		else if(name=="meridian_line") stcore->FlagMeridianLine = newval;
		else if(name=="cardinal_points") stcore->cardinals_points->set_flag_show(newval);
		else if(name=="moon_scaled") {
			if((stcore->FlagMoonScaled = newval)) 
				stcore->ssystem->get_moon()->set_sphere_scale(stcore->MoonScale);
			else stcore->ssystem->get_moon()->set_sphere_scale(1.);
		}
		else if(name=="landscape") stcore->FlagLandscape = newval;
		else if(name=="stars") stcore->FlagStars = newval;
		else if(name=="star_names") {
			stcore->FlagStarName = newval;
			if(newval) stcore->FlagStars = 1;  // make sure stars on if want labels
		}
		else if(name=="planets") {
			stcore->FlagPlanets = newval;
			if (!stcore->FlagPlanets) stcore->FlagPlanetsHints = 0;
		}
		else if(name=="planet_names") {
			stcore->FlagPlanetsHints = newval;
			if(stcore->FlagPlanetsHints) stcore->FlagPlanets = 1;  // for safety if script turns planets off
		}
		else if(name=="planet_orbits") stcore->FlagPlanetsOrbits = newval;
		else if(name=="nebulae") stcore->FlagNebula = newval;
		else if(name=="nebula_names") {
			stcore->FlagNebula = 1;  // make sure visible
			stcore->nebulas->set_flag_hints(newval);
		}
		else if(name=="nebula_long_names") {         //Tony - added long names
			stcore->FlagNebula = 1;  // make sure visible
			stcore->nebulas->set_flag_hints(newval); // make sure visible
			stcore->FlagNebulaLongName = newval;
		}
		else if(name=="milky_way") stcore->FlagMilkyWay = newval;
		else if(name=="bright_nebulae") stcore->FlagBrightNebulae = newval;
		else if(name=="object_trails") stcore->FlagObjectTrails = newval;
		else if(name=="track_object") {
			if(newval && stcore->selected_object) {
				stcore->navigation->move_to(stcore->selected_object->get_earth_equ_pos(stcore->navigation),
									stcore->auto_move_duration);
				stcore->navigation->set_flag_traking(1);
			} else {
				stcore->navigation->set_flag_traking(0);
			}
		}
		else return(status);

	}


	return(1);  // flag was found and updated

}


void StelCommandInterface::update(int delta_time) {
	if(audio) audio->update(delta_time);
}
