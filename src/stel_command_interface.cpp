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


  } else if (command == "set") {


  } // else if (command == 

  if( status ) {

    // if recording commands, do that now
    stcore->scripts->record_command(commandline);

    //    cout << commandline << endl;

  } else {
    cout << "Could not execute: " << commandline << endl;
  }

  return(status);


}


