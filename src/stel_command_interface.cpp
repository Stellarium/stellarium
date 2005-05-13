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
    bool val;
    status = stcore->set_flag( args.begin()->first, args.begin()->second, val, trusted);

    // rewrite command for recording so that actual state is known (rather than "toggle")
    if(args.begin()->second == "toggle") {
      std::ostringstream oss;
      oss << command << " " << args.begin()->first << " " << val;
      commandline = oss.str();
    }

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
      stcore->ssystem->get_moon()->set_sphere_scale(stcore->MoonScale);
    }
    else if(args["sky_culture"]!="") stcore->set_sky_culture(args["sky_culture"]);
    else if(args["sky_locale"]!="") stcore->set_sky_locale(args["sky_locale"]);
    else if(args["star_mag_scale"]!="") stcore->StarMagScale = str_to_double(args["star_mag_scale"]);
    else if(args["star_scale"]!="") {
		float scale = str_to_double(args["star_scale"]);
		stcore->StarScale = scale;
		stcore->ssystem->set_object_scale(scale);
	} else if(args["star_twinkle_amount"]!="") stcore->StarTwinkleAmount = str_to_double(args["star_twinkle_amount"]);
    else if(args["time_zone"]!="") stcore->observatory->set_custom_tz_name(args["time_zone"]);
    else if(args["milky_way_intensity"]!="") {
		stcore->MilkyWayIntensity = str_to_double(args["milky_way_intensity"]);
		stcore->milky_way->set_intensity(stcore->MilkyWayIntensity);
	}


    else status = 0;


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
      stcore->selected_object = stcore->hip_stars->search(hpnum);
      stcore->asterisms->set_selected(stcore->asterisms->is_star_in((Hip_Star*)stcore->selected_object));
      stcore->selected_planet=NULL;
    } else if(args["planet"]!=""){
      stcore->selected_object = stcore->selected_planet = stcore->ssystem->search(args["planet"]);
      stcore->asterisms->set_selected(NULL);
    } else if(args["nebula"]!=""){
      stcore->selected_object = stcore->nebulas->search(args["nebula"]);
      stcore->selected_planet=NULL;
      stcore->asterisms->set_selected(NULL);
    } else if(args["constellation"]!=""){
      stcore->asterisms->set_selected(stcore->asterisms->find_from_short_name(args["constellation"]));
      stcore->selected_object = NULL;
      stcore->selected_planet=NULL;
    }

    if (stcore->selected_object) {
      if (stcore->navigation->get_flag_traking()) stcore->navigation->set_flag_lock_equ_pos(1);
      stcore->navigation->set_flag_traking(0);
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
		  if(duration == 0 ) duration = stcore->auto_move_duration;
		  		  
		  if(args["auto"]=="out") stcore->auto_zoom_out(duration);
		  else stcore->auto_zoom_in(duration);
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

    // ISO 8601-like format [+/-]YYYY-MM-DDThh:mm:ss (no timzone offset, T is literal)
    if(args["local"]!="") {
      double jd;
      if(string_to_jday( args["local"], jd ) ) {
	stcore->navigation->set_JDay(jd - stcore->observatory->get_GMT_shift(jd) * JD_HOUR);
      } else {
	cout << "Error parsing date." << endl;
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
    } else status=0;
    
  } else if (command == "moveto") {

    if(args["lat"]!="" || args["lon"]!="" || args["alt"]!="") {

      double lat = stcore->observatory->get_latitude();
      double lon = stcore->observatory->get_longitude();
      double alt = stcore->observatory->get_altitude();
      int delay;

      if(args["lat"]!="") lat = str_to_double(args["lat"]);
      if(args["lon"]!="") lon = str_to_double(args["lon"]);
      if(args["alt"]!="") alt = str_to_double(args["alt"]);
      delay = (int)(1000.*str_to_double(args["duration"]));

      stcore->observatory->move_to(lat,lon,alt,delay);
    } else status = 0;

  } else if(command=="image") {

	  if(args["name"]=="") {
		  cout << "Image name required" << endl;
		  status = 0;
	  } else if(args["action"]=="drop") {
		  stcore->script_images->drop_image(args["name"]);
	  } else {
		  if(args["action"]=="load" && args["filename"]!="") {
			  if(stcore->scripts->is_playing()) 
				  stcore->script_images->load_image(stcore->scripts->get_script_path() + args["filename"], args["name"]);
			  else stcore->script_images->load_image(stcore->DataRoot + "/" + args["filename"], args["name"]);
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
				  img->set_location(str_to_double(args["xpos"]), 
									str_to_double(args["ypos"]), 
									str_to_double(args["duration"]));
		  }
	  }
  } else if(command=="audio" && args["action"]=="play" && args["filename"]!="") {
    // only one track at a time allowed
    if(audio) delete audio;

    // if from script, local to that path
    string path;
    if(stcore->scripts->is_playing()) path = stcore->scripts->get_script_path();
    else path = stcore->DataRoot + "/";

    cout << "audio path = " << path << endl;

    audio = new Audio(path + args["filename"], "default track");
    audio->play(args["loop"]=="on");

  } else if(command=="script") {

    if(args["action"]=="end") {
      // stop script, audio, and unload any loaded images
      stcore->scripts->cancel_script();
      if(audio) {
	delete audio;
	audio = NULL;
      }
      stcore->script_images->drop_all_images();

      // unmount disk if was mounted to run script
      // TODO unmount disk...
      stcore->ScriptRemoveableDiskMounted = 0;

    } else if(args["action"]=="play" && args["filename"]!="") {
      stcore->scripts->play_script(args["filename"], args["path"]);
    } else if(args["action"]=="record") {  // TEMP
      //    if(args["action"]=="record" && args["filename"]!="") {
      stcore->scripts->record_script(args["filename"]);
      recordable = 0;  // don't record this command!
    } else if(args["action"]=="cancelrecord") {
      stcore->scripts->cancel_record_script();
      recordable = 0;  // don't record this command!
    } else if(args["action"]=="pause" && !stcore->scripts->is_paused()) {
      // n.b. action=pause TOGGLES pause
      audio->pause();
      stcore->scripts->pause_script();
    } else if (args["action"]=="pause" || args["action"]=="resume") {
      stcore->scripts->resume_script();
      audio->resume();
    } else status =0;

  } else if(command=="clear") {

	  // turn off all labels (used by scripts for simplicity)
	  execute_command("flag atmosphere off");
	  execute_command("flag azimuthal_grid off");
	  execute_command("flag cardinal_points off");
	  execute_command("flag constellation_art off");
	  execute_command("flag constellation_drawing off");
	  execute_command("flag constellation_name off");
	  execute_command("flag ecliptic_line off");
	  execute_command("flag equatorial_grid off");
	  execute_command("flag equator_line off");
	  execute_command("flag fog off");
	  execute_command("flag ground off");
	  execute_command("flag nebula_name off");
	  execute_command("flag object_trails off");
	  execute_command("flag planets_hints off");
	  execute_command("flag planets_orbits off");
	  execute_command("flag show_tui_datetime off");
	  execute_command("flag star_name off");
	  execute_command("flag show_tui_short_obj_info off");

	  // make sure planets, stars, etc. are turned on!
	  // milkyway is left to user, for those without 3d cards
	  execute_command("flag stars on");
	  execute_command("flag planets on");
	  execute_command("flag nebula on");

	  // also deselect everything, set to default fov and real time rate
	  execute_command("deselect");
	  execute_command("timerate rate 1");
	  execute_command("zoom auto out");

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



// convert string int ISO 8601-like format [+/-]YYYY-MM-DDThh:mm:ss (no timzone offset)
// to julian day
// TODO: move to better location for reuse
int string_to_jday(string date, double &jd) {

    char tmp;
    int year, month, day, hour, minute, second;
    std::istringstream dstr( date );
    
    dstr >> year >> tmp >> month >> tmp >> day >> tmp >> hour >> tmp >> minute >> tmp >> second;
    
    // cout << year << " " << month << " " << day << " " << hour << " " << minute << " " << second << endl;

    // bounds checking (per s_tui time object)
    if( year > 100000 || year < -100000 || 
	month < 1 || month > 12 ||
	day < 1 || day > 31 ||
	hour < 0 || hour > 23 ||
	minute < 0 || minute > 59 ||
	second < 0 || second > 59) return 0;


    // code taken from s_tui.cpp
    if (month <= 2) {
        year--;
        month += 12;
    }

    // Correct for the lost days in Oct 1582 when the Gregorian calendar
    // replaced the Julian calendar.
    int B = -2;
    if (year > 1582 || (year == 1582 && (month > 10 || (month == 10 && day >= 15)))) {
      B = year / 400 - year / 100;
    }

    jd = ((floor(365.25 * year) +
            floor(30.6001 * (month + 1)) + B + 1720996.5 +
            day + hour / 24.0 + minute / 1440.0 + second / 86400.0));

    return 1;

}


double str_to_double(string str) {

	if(str=="") return 0;
	double dbl;
	std::istringstream dstr( str );
    
	dstr >> dbl;
	return dbl;
}

// always positive
double str_to_pos_double(string str) {

	if(str=="") return 0;
    double dbl;
    std::istringstream dstr( str );

    dstr >> dbl;
    if(dbl < 0 ) dbl *= -1;
    return dbl;
}


int str_to_int(string str) {

	if(str=="") return 0;
	int integer;
	std::istringstream istr( str );
    
	istr >> integer;
	return integer;
}

string double_to_str(double dbl) {

  std::ostringstream oss;
  oss << dbl;
  return oss.str();

}
