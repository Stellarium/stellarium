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
    string sdelay;

    if(args["ms"] != "") sdelay = args["ms"];
    else if(args["sec"] != "") sdelay = args["sec"];

    std::istringstream istr(sdelay);
    istr >> fdelay;

    if(args["sec"] != "") fdelay *= 1000;

    fdelay += 0.5f;

    if(fdelay >= 0) wait = (int)fdelay;

    //    cout << "wait is: " << wait << endl; 

  } else if (command == "select") {
    if(args["hp"]!=""){
      unsigned int hpnum;
      std::istringstream istr(args["hp"]);
      istr >> hpnum;
      stcore->selected_object = stcore->hip_stars->search(hpnum);
    } else if(args["planet"]!=""){
      stcore->selected_object = stcore->ssystem->search(args["planet"]);
    } else if(args["dso"]!=""){
      stcore->selected_object = stcore->nebulas->search(args["dso"]);
    } else {
      // default is to deselect
      stcore->selected_object = NULL;
    }

    if (stcore->selected_object) {
      stcore->navigation->move_to(stcore->selected_object->get_earth_equ_pos(stcore->navigation),
					stcore->auto_move_duration);
      stcore->navigation->set_flag_traking(1);
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
    } else if(args["relative"]!="") {
      double days;
      if(string_to_days( args["relative"], days ) ) {
	stcore->navigation->set_JDay(stcore->navigation->get_JDay() + days );
      } else {
	cout << "Error parsing date." << endl;
	status = 0;
      } 

    }
    
  } else if (command == "set") {


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


// converts iso 8601 type string to days (for relative date changes)
// TODO: move to better location for reuse
int string_to_days(string date, double &jd) {

  float sign = 1;
  char tmp;
  float year, month, day, hour, minute, second;
  std::istringstream dstr( date );
    
  dstr >> year >> tmp >> month >> tmp >> day >> tmp >> hour >> tmp >> minute >> tmp >> second;
    
  //  cout << year << " " << month << " " << day << " " << hour << " " << minute << " " << second << endl;
 
  // bounds checking (0 is allowed for all, few upper limits)
  if( year > 100000 || year < -100000 || 
	month < 0 || day < 0 || hour < 0 || second < 0 ) return 0;

  if(year < 0) {
    sign = -1;
    year *= -1;
  }

  jd = sign * (year*365.25 + month*30.6001 + day 
    + hour*24.0 + minute * 1440.0 + second * 86400.0);

  //  cout << "relative days: " << jd << endl;

  return 1;

}
