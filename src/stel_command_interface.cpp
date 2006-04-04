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
#include <algorithm>

#include "stel_command_interface.h"
#include "stel_core.h"
#include "image.h"
#include "stellastro.h"

using namespace std;


StelCommandInterface::StelCommandInterface(StelCore * core, StelApp * app) {
  stcore = core;
  stapp = app;
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
  wstring debug_message;  // detailed errors can be placed in here for printout at end of method

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

    if(args["atmosphere_fade_duration"]!="") stcore->setAtmosphereFadeDuration(str_to_double(args["atmosphere_fade_duration"]));
    else if(args["auto_move_duration"]!="") stcore->auto_move_duration = str_to_double(args["auto_move_duration"]);
    else if(args["constellation_art_fade_duration"]!="") stcore->setConstellationArtFadeDuration(str_to_double(args["constellation_art_fade_duration"]));
    else if(args["constellation_art_intensity"]!="") stcore->setConstellationArtIntensity(str_to_double(args["constellation_art_intensity"]));
    else if(args["landscape_name"]!="") stcore->setLandscape(args["landscape_name"]);
    else if(args["max_mag_nebula_name"]!="") stcore->setNebulaMaxMagHints(str_to_double(args["max_mag_nebula_name"]));
    else if(args["max_mag_star_name"]!="") stcore->setMaxMagStarName(str_to_double(args["max_mag_star_name"]));
    else if(args["moon_scale"]!="") {
      stcore->setMoonScale(str_to_double(args["moon_scale"]));
    }
    else if(args["sky_culture"]!="") stcore->setSkyCultureDir(args["sky_culture"]);
//    else if(args["sky_locale"]!="") stcore->set_sky_locale(args["sky_locale"]); // Tony NOT SURE
    else if(args["sky_locale"]!="") stapp->setAppLanguage(args["sky_locale"]);
    else if(args["star_mag_scale"]!="") stcore->setStarMagScale(str_to_double(args["star_mag_scale"]));
    else if(args["star_scale"]!="") {
		float scale = str_to_double(args["star_scale"]);
		stcore->setStarScale(scale);
		stcore->setPlanetsScale(scale);
	} 
	else if(args["nebula_scale"]!="")
	{
		float scale = str_to_double(args["nebula_scale"]);
		stcore->setNebulaCircleScale(scale);
	} 
	else if(args["star_twinkle_amount"]!="") stcore->setStarTwinkleAmount(str_to_double(args["star_twinkle_amount"]));
    else if(args["time_zone"]!="") stcore->observatory->set_custom_tz_name(args["time_zone"]);
    else if(args["milky_way_intensity"]!="") {
		stcore->setMilkyWayIntensity(str_to_double(args["milky_way_intensity"]));
		// safety feature to be able to turn back on
		if(stcore->getMilkyWayIntensity()) stcore->setFlagMilkyWay(true);

	} else status = 0;


    if(trusted) {

    //    else if(args["base_font_size"]!="") stcore->BaseFontSize = str_to_double(args["base_font_size"]);
    //	else if(args["bbp_mode"]!="") stcore->BbpMode = str_to_double(args["bbp_mode"]);
    //    else if(args["date_display_format"]!="") stcore->DateDisplayFormat = args["date_display_format"];
    //	else if(args["fullscreen"]!="") stcore->Fullscreen = args["fullscreen"];
    //	else if(args["horizontal_offset"]!="") stcore->HorizontalOffset = str_to_double(args["horizontal_offset"]);
    //	else if(args["init_fov"]!="") stcore->InitFov = str_to_double(args["init_fov"]);
    //	else if(args["preset_sky_time"]!="") stapp->PresetSkyTime = str_to_double(args["preset_sky_time"]);
    //	else if(args["screen_h"]!="") stcore->ScreenH = str_to_double(args["screen_h"]);
    //	else if(args["screen_w"]!="") stcore->ScreenW = str_to_double(args["screen_w"]);
    //    else if(args["startup_time_mode"]!="") stapp->StartupTimeMode = args["startup_time_mode"];
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
      stcore->asterisms->setSelected((HipStar*)stcore->selected_object);
      stcore->ssystem->setSelected(NULL);
	}
	else if(args["star"]!="")
	{
      stcore->selected_object = stcore->hip_stars->search(args["star"]);
      stcore->asterisms->setSelected((HipStar*)stcore->selected_object);
      stcore->setPlanetsSelected("");
    } else if(args["planet"]!=""){
      stcore->setPlanetsSelected(args["planet"]);
      stcore->selected_object = stcore->ssystem->getSelected();
      stcore->asterisms->setSelected(NULL);
    } else if(args["nebula"]!=""){
      stcore->selected_object = stcore->nebulas->search(args["nebula"]);
      stcore->setPlanetsSelected("");
      stcore->asterisms->setSelected(NULL);
    } else if(args["constellation"]!=""){

		// Select only constellation, nothing else 	 
		stcore->asterisms->setSelected(args["constellation"]); 

		stcore->selected_object = NULL;
		stcore->setPlanetsSelected("");

    } else if(args["constellation_star"]!=""){

		// For Find capability, select a star in constellation so can center view on constellation 	 
		unsigned int hpnum; 
		stcore->asterisms->setSelected(args["constellation_star"]); 

		hpnum = stcore->asterisms->getFirstSelectedHP();
		stcore->selected_object = stcore->hip_stars->searchHP(hpnum); 	 
		stcore->asterisms->setSelected((HipStar*)stcore->selected_object); 	 
		stcore->setPlanetsSelected("");

		// Some stars are shared, so now force constellation
		stcore->asterisms->setSelected(args["constellation_star"]);

    }


    if (stcore->selected_object) {
      if (stcore->navigation->get_flag_traking()) stcore->navigation->set_flag_lock_equ_pos(1);
      stcore->navigation->set_flag_traking(0);

	  // determine if selected object pointer should be displayed
	  if(args["pointer"]=="off" || args["pointer"]=="0") stcore->setFlagSelectedObjectPointer(false);
	  else stcore->setFlagSelectedObjectPointer(true);
    }

  } else if (command == "deselect") {
    stcore->unSelect();

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
		  		  
		  if(args["auto"]=="out") stcore->autoZoomOut(duration, 0);
		  else if(args["auto"]=="initial") stcore->autoZoomOut(duration, 1);
		  else if(args["manual"]=="1") {
			  stcore->autoZoomIn(duration, 1);  // have to explicity allow possible manual zoom 
		  } else stcore->autoZoomIn(duration, 0);  

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
      stapp->FlagTimePause = !stapp->FlagTimePause;
      if(stapp->FlagTimePause) {
	stapp->temp_time_velocity = stcore->navigation->get_time_speed();
	stcore->navigation->set_time_speed(0);
      } else {
	stcore->navigation->set_time_speed(stapp->temp_time_velocity);
      }

    } else if(args["action"]=="resume") {
      stapp->FlagTimePause = 0;
      stcore->navigation->set_time_speed(stapp->temp_time_velocity);

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
			  debug_message = _("Error parsing date.");
			  status = 0;
		  } 
		  
	  } else if(args["utc"]!="") {
		  double jd;
		  if(string_to_jday( args["utc"], jd ) ) {
			  stcore->navigation->set_JDay(jd);
		  } else {
			  debug_message = _("Error parsing date.");
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
		  if (stapp->StartupTimeMode=="preset" || stapp->StartupTimeMode=="Preset")
			  stcore->navigation->set_JDay(stapp->PresetSkyTime -
										 stcore->observatory->get_GMT_shift(stapp->PresetSkyTime) * JD_HOUR);
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
		  
		  stcore->observatory->move_to(lat,lon,alt,delay,StelUtility::stringToWstring(name));
	  } else status = 0;

  } else if(command=="image") {

	  if(args["name"]=="") {
		  debug_message = _("Image name required.");
		  status = 0;
	  } else if(args["action"]=="drop") {
		  stcore->script_images->drop_image(args["name"]);
	  } else {
		  if(args["action"]=="load" && args["filename"]!="") {

			  // TODO: more image positioning coordinates
			  Image::IMAGE_POSITIONING img_pos = Image::POS_VIEWPORT;
			  if(args["coordinate_system"] == "horizontal") img_pos = Image::POS_HORIZONTAL;
			  /*
			  else if(args["coordinates"] == "rade") img_pos = POS_EQUATORIAL;
			  else img_pos = POS_VIEWPORT;
			  */

			  string image_filename;
			  if(stapp->scripts->is_playing()) 
				  image_filename = stapp->scripts->get_script_path() + args["filename"];
			  else 
				  image_filename = stcore->getDataRoot() + "/" + args["filename"];
				  
			  status = stcore->script_images->load_image(image_filename, args["name"], img_pos);

			  if(status==0) debug_message = _("Unable to open file: ") + StelUtility::stringToWstring(image_filename);
		  }

		  if( status ) {
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
			  } else {
				  debug_message = _("Unable to find image: ") + StelUtility::stringToWstring(args["name"]);
				  status=0;
			  }
		  }
	  }
  } 
  else if(command=="audio") {

#ifndef HAVE_SDL_MIXER_H
	  debug_message = _("This executable was compiled without audio support.");
	  status = 0;
#else
  
	  if(args["action"]=="sync") {
		  if(audio) audio->sync();

	  } else if(args["action"]=="pause") {
		  if(audio) audio->pause();

	  } else if( args["action"]=="play" && args["filename"]!="") {
		  // only one track at a time allowed
		  if(audio) delete audio;
		  
		  // if from script, local to that path
		  string path;
		  if(stapp->scripts->is_playing()) path = stapp->scripts->get_script_path();
		  else path = stcore->getDataRoot() + "/";
		  
		  //		  cout << "audio path = " << path << endl;
		  
		  audio = new Audio(path + args["filename"], "default track", str_to_long(args["output_rate"]));
		  audio->play(args["loop"]=="on");

		  // if fast forwarding mute (pause) audio
		  if(stapp->getTimeMultiplier()!=1) audio->pause();

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
#endif 
  }
  else if(command=="script") {

    if(args["action"]=="end") {
      // stop script, audio, and unload any loaded images
      if(audio) {
		  delete audio;
		  audio = NULL;
      }
      stapp->scripts->cancel_script();
      stcore->script_images->drop_all_images();

    } else if(args["action"]=="play" && args["filename"]!="") {
		if(stapp->scripts->is_playing()) {

			string script_path = stapp->scripts->get_script_path();

			// stop script, audio, and unload any loaded images
			if(audio) {
				delete audio;
				audio = NULL;
			}
			stapp->scripts->cancel_script();
			stcore->script_images->drop_all_images();

			// keep same script path 
			stapp->scripts->play_script(script_path + args["filename"], script_path);
		} else {
			stapp->scripts->play_script(args["path"] + args["filename"], args["path"]);
		}

    } else if(args["action"]=="record") {
      stapp->scripts->record_script(args["filename"]);
      recordable = 0;  // don't record this command!
    } else if(args["action"]=="cancelrecord") {
      stapp->scripts->cancel_record_script();
      recordable = 0;  // don't record this command!
    } else if(args["action"]=="pause" && !stapp->scripts->is_paused()) {
      // n.b. action=pause TOGGLES pause
      if(audio) audio->pause();
      stapp->scripts->pause_script();
    } else if (args["action"]=="pause" || args["action"]=="resume") {
      stapp->scripts->resume_script();
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
	  execute_command("flag constellation_boundaries off");
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
	  args["path"] = stapp->scripts->get_script_path();
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

// Fabien : this should be useless. If you need to reset an initial state after running a script, just run a reinit script.

// 	  if(args["action"]=="load" && trusted) {
// 		  // eventually load/reload are not both needed, but for now this is called at startup, reload later
// 		  // stapp->loadConfigFrom(stapp->getConfigFile());
// 		  recordable = 0;  // don't record as scripts can not run this
// 
// 	  } else if(args["action"]=="reload") {
// 
// 		  // on reload, be sure to reconfigure as necessary since StelCore::init isn't called
// 
// 		  stapp->loadConfigFrom(stapp->getConfigFile());
// 
// 		  if(stcore->asterisms) {
// 			  stcore->setConstellationArtIntensity(stcore->getConstellationArtIntensity());
// 			  stcore->setConstellationArtFadeDuration(stcore->getConstellationArtFadeDuration());
// 		  }
// 		  if (!stcore->getFlagAtmosphere() && stcore->tone_converter)
// 			  stcore->tone_converter->set_world_adaptation_luminance(3.75f);
// 		  //if (stcore->getFlagAtmosphere()) stcore->atmosphere->set_fade_duration(stcore->AtmosphereFadeDuration);
// 		  stcore->observatory->load(stapp->getConfigFile(), "init_location");
// 		  stcore->setLandscape(stcore->observatory->get_landscape_name());
// 		  
// 		  if (stapp->StartupTimeMode=="preset" || stapp->StartupTimeMode=="Preset")
// 			  {
// 				  stcore->navigation->set_JDay(stapp->PresetSkyTime -
// 											 stcore->observatory->get_GMT_shift(stapp->PresetSkyTime) * JD_HOUR);
// 			  }
// 		  else
// 			  {
// 				  stcore->navigation->set_JDay(get_julian_from_sys());
// 			  }
// 		  if(stcore->getFlagPlanetsTrails() && stcore->ssystem) stcore->startPlanetsTrails(true);
// 		  else stcore->startPlanetsTrails(false);
// 
// 		  string temp = stcore->skyCulture;  // fool caching in below method
// 		  stcore->skyCulture = "";
// 		  stcore->setSkyCulture(temp);
// 		  
// 		  system( ( stcore->getDataDir() + "script_loadConfig " ).c_str() );
// 
// 	  } else status = 0;

  } else {
    debug_message = _("Unrecognized or malformed command name.");
    status = 0;
  }

  if(status ) {

    // if recording commands, do that now
    if(recordable) stapp->scripts->record_command(commandline);

    //    cout << commandline << endl;

  } else {

	  // Show gui error window only if script asked for gui debugging
	  if(stapp->scripts->is_playing() && stapp->scripts->get_gui_debug()) 
		  stapp->ui->show_message(_("Could not execute command:") + wstring(L"\n\"") + 
		   StelUtility::stringToWstring(commandline) + wstring(L"\"\n\n") + debug_message, 7000);
			
	  cerr << "Could not execute: " << commandline << endl << StelUtility::wstringToString(debug_message) << endl;
  }

  return(status);

}


// set flags - TODO: StelCore flag variables will be replaced with object attributes
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
			else if(name=="enable_move_mouse") newval = (stapp->FlagEnableMoveMouse = !stapp->FlagEnableMoveMouse);
			else if(name=="menu") newval = (stapp->ui->FlagMenu = !stapp->ui->FlagMenu);
			else if(name=="help") newval = (stapp->ui->FlagHelp = !stapp->ui->FlagHelp);
			else if(name=="infos") newval = (stapp->ui->FlagInfos = !stapp->ui->FlagInfos);
			else if(name=="show_topbar") newval = (stapp->ui->FlagShowTopBar = !stapp->ui->FlagShowTopBar);
			else if(name=="show_time") newval = (stapp->ui->FlagShowTime = !stapp->ui->FlagShowTime);
			else if(name=="show_date") newval = (stapp->ui->FlagShowDate = !stapp->ui->FlagShowDate);
			else if(name=="show_appname") newval = (stapp->ui->FlagShowAppName = !stapp->ui->FlagShowAppName);
			else if(name=="show_fps") newval = (stapp->ui->FlagShowFps = !stapp->ui->FlagShowFps);
			else if(name=="show_fov") newval = (stapp->ui->FlagShowFov = !stapp->ui->FlagShowFov);
			else if(name=="enable_tui_menu") newval = (stapp->ui->FlagEnableTuiMenu = !stapp->ui->FlagEnableTuiMenu);
			else if(name=="show_gravity_ui") newval = (stapp->ui->FlagShowGravityUi = !stapp->ui->FlagShowGravityUi);
			else if(name=="gravity_labels") {
				newval = !stcore->getFlagGravityLabels();
				stcore->setFlagGravityLabels(newval);
				}
			else status = 0;  // no match here, anyway

		} else status = 0;


		if(name=="constellation_drawing") {
			newval = !stcore->getFlagConstellationLines();
			stcore->setFlagConstellationLines(newval);
		} 
		else if(name=="constellation_names") {
			newval = !stcore->getFlagConstellationNames();
			stcore->setFlagConstellationNames(newval);
		} 
		else if(name=="constellation_art") {
			newval = !stcore->getFlagConstellationArt();
			stcore->setFlagConstellationArt(newval);
		} 
		else if(name=="constellation_boundaries") {
			newval = !stcore->getFlagConstellationBoundaries();
			stcore->setFlagConstellationBoundaries(newval);
		} 
		else if(name=="constellation_pick") { 
             newval = !stcore->getFlagConstellationIsolateSelected();
             stcore->setFlagConstellationIsolateSelected(newval);
        }
		else if(name=="star_twinkle") {
			newval = !stcore->getFlagStarTwinkle();
			stcore->setFlagStarTwinkle(newval);
		}
		else if(name=="point_star") {
			newval = !stcore->getFlagPointStar();
			stcore->setFlagPointStar(newval);
		}
		else if(name=="show_selected_object_info") newval = (stapp->ui->FlagShowSelectedObjectInfo = !stapp->ui->FlagShowSelectedObjectInfo);
		else if(name=="show_tui_datetime") newval = (stapp->ui->FlagShowTuiDateTime = !stapp->ui->FlagShowTuiDateTime);
		else if(name=="show_tui_short_obj_info") newval = (stapp->ui->FlagShowTuiShortObjInfo = !stapp->ui->FlagShowTuiShortObjInfo);
		else if(name=="manual_zoom") newval = (stcore->FlagManualZoom = !stcore->FlagManualZoom);
		else if(name=="show_script_bar") newval = (stapp->ui->FlagShowScriptBar = !stapp->ui->FlagShowScriptBar);
		else if(name=="fog") {
			newval = !stcore->getFlagFog();
			stcore->setFlagFog(newval); 
		}
		else if(name=="atmosphere") {
			newval = !stcore->getFlagAtmosphere();
			stcore->setFlagAtmosphere(newval);
			if(!newval) stcore->setFlagFog(false);  // turn off fog with atmosphere
		}
		else if(name=="chart") {
			newval = !stapp->getVisionModeChart();
			if (newval) stapp->setVisionModeChart();
		}
		else if(name=="night") {
			newval = !stapp->getVisionModeNight();
			if (newval) stapp->setVisionModeNight();
		}
		//else if(name=="use_common_names") newval = (stcore->FlagUseCommonNames = !stcore->FlagUseCommonNames);
		else if(name=="azimuthal_grid") {
		newval = !stcore->getFlagAzimutalGrid();
		stcore->setFlagAzimutalGrid(newval);
		}
		else if(name=="equatorial_grid") {
		newval = !stcore->getFlagEquatorGrid();
		stcore->setFlagEquatorGrid(newval);
		}
		else if(name=="equator_line") {
		newval = !stcore->getFlagEquatorLine();
		stcore->setFlagEquatorLine(newval);
		}
		else if(name=="ecliptic_line") {
		newval = !stcore->getFlagEclipticLine();
		stcore->setFlagEclipticLine(newval);
		}
		else if(name=="meridian_line") {
		newval = !stcore->getFlagMeridianLine();
		 stcore->setFlagMeridianLine(newval);
		}
		else if(name=="cardinal_points") {
			newval = !stcore->cardinals_points->getFlagShow();
			stcore->cardinals_points->setFlagShow(newval);
		}
		else if(name=="moon_scaled") {
			newval = !stcore->getFlagMoonScaled();
			stcore->setFlagMoonScaled(newval);
		}
		else if(name=="landscape") {
			newval = !stcore->getFlagLandscape();
			stcore->setFlagLandscape(newval);
			}
		else if(name=="stars") {
			newval = !stcore->getFlagStars();
			stcore->setFlagStars(newval);
		}
		else if(name=="star_names") {
			newval = !stcore->getFlagStarName();
			stcore->setFlagStarName(newval);
		}
		else if(name=="planets") {
			newval = !stcore->getFlagPlanets();
			stcore->setFlagPlanets(newval);
			if (!stcore->getFlagPlanets()) stcore->setFlagPlanetsHints(false);
		}
		else if(name=="planet_names") {
			newval = !stcore->getFlagPlanetsHints();
			stcore->setFlagPlanetsHints(newval);
			if(stcore->getFlagPlanetsHints()) stcore->setFlagPlanets(true);  // for safety if script turns planets off
		}
		else if(name=="planet_orbits") {
		newval = !stcore->getFlagPlanetsOrbits();
		stcore->setFlagPlanetsOrbits(newval);
		}
		else if(name=="nebulae") {
			newval = !stcore->getFlagNebula();
			stcore->setFlagNebula(newval);
			}
		else if(name=="nebula_names") {
			newval = !stcore->nebulas->getFlagHints();
			if(newval) stcore->setFlagNebula(true);  // make sure visible
			stcore->setFlagNebulaHints(newval);
		}
		else if(name=="milky_way") {
			newval = !stcore->getFlagMilkyWay();
			stcore->setFlagMilkyWay(newval);
		}
		else if(name=="bright_nebulae") {
			newval = !stcore->getFlagNebulaBright();
			stcore->setFlagNebulaBright(newval);
			}
		else if(name=="object_trails")
		{
		newval = !stcore->getFlagPlanetsTrails();
		stcore->setFlagPlanetsTrails(newval);
		}
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
		else if(name=="script_gui_debug") {  // Not written to config - script specific
			newval = !stapp->scripts->get_gui_debug();
			stapp->scripts->set_gui_debug(newval);
		}
		else return(status);  // no matching flag found untrusted, but maybe trusted matched

	} else {

		newval = (value == "on" || value == "1");

		if(trusted) {
			// normal scripts shouldn't be able to change these user settings
			if(name=="enable_zoom_keys") stcore->FlagEnableZoomKeys = newval;
			else if(name=="enable_move_keys") stcore->FlagEnableMoveKeys = newval;
			else if(name=="enable_move_mouse") stapp->FlagEnableMoveMouse = newval;
			else if(name=="menu") stapp->ui->FlagMenu = newval;
			else if(name=="help") stapp->ui->FlagHelp = newval;
			else if(name=="infos") stapp->ui->FlagInfos = newval;
			else if(name=="show_topbar") stapp->ui->FlagShowTopBar = newval;
			else if(name=="show_time") stapp->ui->FlagShowTime = newval;
			else if(name=="show_date") stapp->ui->FlagShowDate = newval;
			else if(name=="show_appname") stapp->ui->FlagShowAppName = newval;
			else if(name=="show_fps") stapp->ui->FlagShowFps = newval;
			else if(name=="show_fov") stapp->ui->FlagShowFov = newval;
			else if(name=="enable_tui_menu") stapp->ui->FlagEnableTuiMenu = newval;
			else if(name=="show_gravity_ui") stapp->ui->FlagShowGravityUi = newval;
			else if(name=="gravity_labels") stcore->setFlagGravityLabels(newval);
			else status = 0;
		
		} else status = 0;

		if(name=="constellation_drawing") stcore->setFlagConstellationLines(newval);
		else if(name=="constellation_names") stcore->setFlagConstellationNames(newval);
		else if(name=="constellation_art") stcore->setFlagConstellationArt(newval);
		else if(name=="constellation_boundaries") stcore->setFlagConstellationBoundaries(newval);
     	else if(name=="constellation_pick") stcore->setFlagConstellationIsolateSelected(newval);
		else if(name=="star_twinkle") stcore->setFlagStarTwinkle(newval);
		else if(name=="point_star") stcore->setFlagPointStar(newval);
		else if(name=="show_selected_object_info") stapp->ui->FlagShowSelectedObjectInfo = newval;
		else if(name=="show_tui_datetime") stapp->ui->FlagShowTuiDateTime = newval;
		else if(name=="show_tui_short_obj_info") stapp->ui->FlagShowTuiShortObjInfo = newval;
		else if(name=="manual_zoom") stcore->FlagManualZoom = newval;
		else if(name=="show_script_bar") stapp->ui->FlagShowScriptBar = newval;
		else if(name=="fog") stcore->setFlagFog(newval);
		else if(name=="atmosphere") { 
			stcore->setFlagAtmosphere ( newval);
			if(!newval) stcore->setFlagFog(false);  // turn off fog with atmosphere
		}
		else if(name=="chart") {
			if (newval) stapp->setVisionModeChart();
		}
		else if(name=="night") {
			if (newval) stapp->setVisionModeNight();
		}
		else if(name=="chart" && newval) stapp->setVisionModeChart();
		else if(name=="azimuthal_grid") stcore->setFlagAzimutalGrid(newval);
		else if(name=="equatorial_grid") stcore->setFlagEquatorGrid(newval);
		else if(name=="equator_line") stcore->setFlagEquatorLine(newval);
		else if(name=="ecliptic_line") stcore->setFlagEclipticLine(newval);
		else if(name=="meridian_line") stcore->setFlagMeridianLine(newval);
		else if(name=="cardinal_points") stcore->cardinals_points->setFlagShow(newval);
		else if(name=="moon_scaled") stcore->setFlagMoonScaled(newval);
		else if(name=="landscape") stcore->setFlagLandscape(newval);
		else if(name=="stars") stcore->setFlagStars(newval);
		else if(name=="star_names") stcore->setFlagStarName(newval);
		else if(name=="planets") {
			stcore->setFlagPlanets(newval);
			if (!stcore->getFlagPlanets()) stcore->setFlagPlanetsHints(false);
		}
		else if(name=="planet_names") {
			stcore->setFlagPlanetsHints(newval);
			if(stcore->getFlagPlanetsHints()) stcore->setFlagPlanets(true);  // for safety if script turns planets off
		}
		else if(name=="planet_orbits") stcore->setFlagPlanetsOrbits(newval);
		else if(name=="nebulae") stcore->setFlagNebula(newval);
		else if(name=="nebula_names") {
			stcore->setFlagNebula(true);  // make sure visible
			stcore->setFlagNebulaHints(newval);
		}
		else if(name=="milky_way") stcore->setFlagMilkyWay(newval);
		else if(name=="bright_nebulae") stcore->setFlagNebulaBright(newval);
		else if(name=="object_trails") stcore->setFlagPlanetsTrails(newval);
		else if(name=="track_object") {
			if(newval && stcore->selected_object) {
				stcore->navigation->move_to(stcore->selected_object->get_earth_equ_pos(stcore->navigation),
									stcore->auto_move_duration);
				stcore->navigation->set_flag_traking(1);
			} else {
				stcore->navigation->set_flag_traking(0);
			}
		}
		else if(name=="script_gui_debug") stapp->scripts->set_gui_debug(newval); // Not written to config - script specific
		else return(status);

	}


	return(1);  // flag was found and updated

}


void StelCommandInterface::update(int delta_time) {
	if(audio) audio->update(delta_time);
}
