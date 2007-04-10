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

/* This class handles parsing of a simple command syntax for scripting, 
   UI components, network commands, etc.
*/   

#ifndef _SCRIPT_MGR_H_
#define _SCRIPT_MGR_H_

using namespace std;

#include <string>
#include <fstream>
#include "StelModule.hpp"

// predeclaration
class StelCommandInterface;
class Script;

class ScriptMgr : public StelModule
{

 public:
  ScriptMgr(StelCommandInterface * command_interface);
  virtual ~ScriptMgr();
  virtual string getModuleID() const { return "script_mgr"; }
  
  virtual void init(const InitParser& conf, LoadingBar& lb) {return;}
  virtual double draw(Projector* prj, const Navigator * nav, ToneReproducer* eye) {return 0;}
  
  bool play_script(string script_file, string script_path);
  bool play_startup_script();
  void cancel_script();  // stop playing current script
  void pause_script();
  void resume_script();  // start playing paused script
  void record_script(string script_filename);  // begin recording user interactions
  void record_command(string commandline);     // record a command (if recording)
  void cancel_record_script();  // stop recording user interactions
  bool is_playing() { return playing; };     // is a script playing? 
  bool is_paused() { return play_paused; };     // is a script paused?
  bool is_recording() { return recording; };    // is a script being recorded? 
  virtual void update(double delta_time);  // execute commands in running script
  string get_script_list(string directory);  // get list of scripts in a directory
  string get_script_path();
  string get_record_filename() { return rec_filename; }  // file record is writing to
  void set_allow_ui(bool _aui) { allow_ui = _aui; }
  bool get_allow_ui() { return allow_ui; }
  void set_gui_debug(bool _gdebug) { gui_debug = _gdebug; }  // Should script errors be shown onscreen?
  bool get_gui_debug() { return gui_debug; }
  void set_removable_media_path(const string& path) { RemoveableScriptDirectory = path; };
  const string& get_removable_media_path(void) { return RemoveableScriptDirectory; };

 private:

  StelCommandInterface * commander;  // for executing script commands
  Script * script; // currently loaded script
  unsigned long int elapsed_time;  // ms since last script command executed
  unsigned long int wait_time;     // ms until next script command should be executed
  unsigned long int record_elapsed_time;  // ms since last command recorded
  bool recording;  // is a script being recorded?
  bool playing;    // is a script playing?  (could be paused)
  bool play_paused;// is script playback paused?
  fstream rec_file;
  string rec_filename;
  string RemoveableScriptDirectory;
  bool RemoveableDirectoryMounted;
  bool allow_ui;    // Allow user interface to function during scripts 
                    // (except for time related keys which control script playback)
  bool gui_debug;
};


#endif // _SCRIPT_MGR_H
