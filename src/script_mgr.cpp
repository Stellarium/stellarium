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
#include <string>
#include <dirent.h>
#include <stdio.h>
#include "script_mgr.h"


using namespace std;


ScriptMgr::ScriptMgr(StelCommandInterface *command_interface) {
  commander = command_interface;
  recording = 0;
  playing = 0;
  record_elapsed_time = 0;
}

ScriptMgr::~ScriptMgr() {
}

// path is used for loading script assets 
void ScriptMgr::play_script(string script_file, string script_path) {
  // load script...

  if(playing){
    cout << "Error: script already in progress.  Can't play " << script_file << endl;
    return;
  }

  script = new Script();
  if( script->load(script_file, script_path) ) {
    playing = 1;
    play_paused = 0;
    elapsed_time = wait_time = 0;

    /*    // temp
    string comd;
    while(script->next_command(comd)) {
      cout << "Command: " << comd << endl;
    }
    */

  } else {
    delete script;
  }
}

void ScriptMgr::cancel_script() {
  // delete script object...
  delete script;
  script = NULL;
  // images loaded are deleted from stel_command_interface directly
  playing = 0;
  play_paused = 0;
}


void ScriptMgr::pause_script() {
  play_paused = 1;
  // need to pause time as well
  commander->execute_command("timerate action pause");
}

void ScriptMgr::resume_script() { 
  play_paused = 0;
  commander->execute_command("timerate action resume");
}

void ScriptMgr::record_script(string script_filename) {

  // open file...

  recording = 1;
  record_elapsed_time = 0;
}

void ScriptMgr::record_command(string commandline) {

  if(recording) {
    // write to file...

    if(record_elapsed_time) {
      cout << "wait duration " << record_elapsed_time/1000.f << endl;
      record_elapsed_time = 0;
    }
    // temp:
    cout << commandline << endl;
  }
}

void ScriptMgr::cancel_record_script() {
  // close file...
  recording = 0;
}
	 

// runs maximum of one command per update 
// note that waits can drift by up to 1/fps seconds
void ScriptMgr::update(int delta_time) {

  if(recording) record_elapsed_time += delta_time;

  if(playing && !play_paused) {

    elapsed_time += delta_time;  // time elapsed since last command (should have been) executed

    if(elapsed_time >= wait_time) {
      // now time to run next command


      //      cout << "dt " << delta_time << " et: " << elapsed_time << endl;
      elapsed_time -= wait_time;
      string comd;
      unsigned long int wait;
      if(script->next_command(comd)) {
	commander->execute_command(comd, wait, 0);  // untrusted commands
	wait_time = wait; 

      } else {
	// script done
	cout << "Script completed." << endl;
	commander->execute_command("script action end");
      }
    }
  }

}  

// get a list of script files from directory
string ScriptMgr::get_script_list(string directory) {

  // TODO: This is POSIX specific

  struct dirent *entryp;
  DIR *dp;
  string result="";
  string tmp;

  if ((dp = opendir(directory.c_str())) == NULL) {
    cout << "Error reading script directory " << directory << endl;
    return "";
  }

  // TODO: sort the directory
  while ((entryp = readdir(dp)) != NULL) {
    tmp = entryp->d_name;

    if(tmp.length()>4 && tmp.find(".sts", tmp.length()-4)!=string::npos ) {
      result += tmp + "\n";
      cout << entryp->d_name << endl;
    }
  }
  closedir(dp);

  cout << "Result = " << result;
  return result;

}

string ScriptMgr::get_script_path() { 
  if(script) return script->get_path(); 
  return ""; 
}
