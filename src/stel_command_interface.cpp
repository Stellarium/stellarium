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
}

StelCommandInterface::~StelCommandInterface() {
}

int StelCommandInterface::execute_command(string commandline ) {
  int delay;
  return execute_command(commandline, delay);
  // delay is ignored, as not needed by the ui callers
}

// called by script executors
int StelCommandInterface::execute_command(string commandline, int &wait) {
  string command;
  stringHash_t args;
  int status = 0;


  wait = 0;  // default, no wait between commands

  status = parse_command(commandline, command, args);

  // stellarium specific logic to run each command

  if(command == "flag") {

    // TODO: loop if want to allow that syntax
    status = stcore->set_flag( args.begin()->first, args.begin()->second );

  }  else if (command == "wait") {

    float fdelay;

    if(args["ms"] != "") fdelay = str_to_double(args["ms"]);
    else if(args["sec"] != "") fdelay = 1000*str_to_double(args["sec"]);

    if(fdelay >= 0) wait = (int)fdelay;

    //    cout << "wait is: " << wait << endl; 

  } else if (command == "select") {

    // default is to deselect
    stcore->selected_object=NULL;
    stcore->selected_planet=NULL;
    stcore->selected_constellation=NULL;

    if(args["hp"]!=""){
      unsigned int hpnum;
      std::istringstream istr(args["hp"]);
      istr >> hpnum;
      stcore->selected_object = stcore->hip_stars->search(hpnum);
      stcore->selected_constellation=stcore->asterisms->is_star_in((Hip_Star*)stcore->selected_object);
    } else if(args["planet"]!=""){
      stcore->selected_object = stcore->selected_planet = stcore->ssystem->search(args["planet"]);
    } else if(args["dso"]!=""){
      stcore->selected_object = stcore->nebulas->search(args["dso"]);
    }

    if (stcore->selected_object) {
      if (stcore->navigation->get_flag_traking()) stcore->navigation->set_flag_lock_equ_pos(1);
      stcore->navigation->set_flag_traking(0);
    }

  } else if(command == "autozoom") {

    if(args["direction"]=="out") stcore->auto_zoom_out(stcore->auto_move_duration);
    else stcore->auto_zoom_in(stcore->auto_move_duration);

  } else if(command == "timerate") {   // NOTE: accuracy issue related to frame rate

    float rate;
    std::istringstream istr(args["rate"]);
    istr >> rate;

    stcore->navigation->set_time_speed(rate*JD_SECOND);
  
  } else if(command == "date") {

    // ISO 8601-like format [+/-]YYYY-MM-DDThh:mm:ss (no timzone offset, T is literal)
    if(args["absolute"]!="") {
      double jd;
      if(string_to_jday( args["absolute"], jd ) ) {
	stcore->navigation->set_JDay(jd - stcore->observatory->get_GMT_shift(jd) * JD_HOUR);
      } else {
	cout << "Error parsing date." << endl;
	status = 0;
      }
    } else if(args["relative"]!="") {  // value is a float number of days
      double days = str_to_double(args["relative"]);
      stcore->navigation->set_JDay(stcore->navigation->get_JDay() + days );
    }
    
  } else if (command == "moveto") {

    if(args["lat"]!="" || args["lon"]!="" || args["alt"]!="") {

      double lat = stcore->observatory->get_latitude();
      double lon = stcore->observatory->get_longitude();
      double alt = stcore->observatory->get_altitude();
      int delay;

      if(args["lat"]!="") lat = str_to_double(args["lat"]);
      if(args["lon"]!="") lon = str_to_double(args["lon"]);
      if(args["alt"]!="") alt = str_to_double(args["alt"]);
      delay = str_to_int(args["duration"]);

      stcore->observatory->move_to(lat,lon,alt,delay);
    } else {
      cout << "Insufficient arguments" << endl;
      status = 0;
    }

  } else if(command=="imageload") {
    if(args["filename"]!="" && args["name"]!="") {
      stcore->images->load_image(args["filename"], args["name"]);
    } else {
      cout << "Incomplete or incorrect arguments." << endl;
    }

  } else if(command=="imageprop") {
    if(args["name"]!=""){
      Image * img = stcore->images->get_image(args["name"]);

      if(img != NULL ) {
	if(args["alpha"]!="") img->set_alpha(str_to_double(args["alpha"]), 
					     str_to_double(args["duration"]));
	if(args["scale"]!="") img->set_scale(str_to_double(args["scale"]), 
					     str_to_double(args["duration"]));
	if(args["rotation"]!="") img->set_rotation(str_to_double(args["rotation"]), 
					     str_to_double(args["duration"]));
      }

    }

  } else if(command=="script") {

    if(args["action"]=="end") {
      // stop script and unload any loaded images
      stcore->scripts->cancel_script();
      stcore->images->drop_all_images();
    }

    if(args["action"]=="play" && args["filename"]!="") {
      stcore->scripts->play_script(args["filename"]);
    }

    if(args["action"]=="pause") {
      stcore->scripts->pause_script();
    }

    if(args["action"]=="resume") {
      stcore->scripts->resume_script();
    }

  } else {
    cout << "Unrecognized command: " << commandline << endl;
    return 0;
  }

  if( status ) {

    // if recording commands, do that now
    stcore->scripts->record_command(commandline);

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
  double dbl;
  std::istringstream dstr( str );
    
  dstr >> dbl;
  return dbl;
}

int str_to_int(string str) {
  int integer;
  std::istringstream istr( str );
    
  istr >> integer;
  return integer;
}

