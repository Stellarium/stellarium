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

int StelCommandInterface::execute_command(string commandline) {
  string command;
  stringHash_t args;
  int status = 0;

  status = parse_command(commandline, command, args);

  // stellarium specific logic to run each command

  if(command == "flag") {

    // TODO: loop if want to allow that syntax
    status = stcore->set_flag( args.begin()->first, args.begin()->second );

  } else if (command == "set") {


  } // else if (command == 

  if( status ) {

    // if recording commands, do that now...

    // temp debugging output
    cout << commandline << endl;


  }

  return(status);


}


